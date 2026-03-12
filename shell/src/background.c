//  LLM Generated Code Begins ##############

#include "background.h"
#include "command.h"
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>

BackgroundJob bg_jobs[MAX_BG_JOBS];
int bg_job_count = 0;
int next_job_number = 1;

// Call this before parsing new input
void check_background_jobs() {
    for (int i = 0; i < bg_job_count; ) {
        int status;
        pid_t result = waitpid(bg_jobs[i].pid, &status, WNOHANG);

        if (result == 0) {
            // Child still running
            i++;
            continue;
        } else if (result == -1) {
            // Error or no such child; remove the job
            for (int j = i; j < bg_job_count - 1; j++) {
                bg_jobs[j] = bg_jobs[j+1];
            }
            bg_job_count--;
            continue;
        } else {
            // Child changed state (exited or signaled)
            if (WIFEXITED(status)) {
                printf("%s with pid %d exited normally\n", bg_jobs[i].command_name, bg_jobs[i].pid);
            } else {
                printf("%s with pid %d exited abnormally\n", bg_jobs[i].command_name, bg_jobs[i].pid);
            }
            // Remove job from list
            for (int j = i; j < bg_job_count - 1; j++) {
                bg_jobs[j] = bg_jobs[j+1];
            }
            bg_job_count--;
        }
    }
}

static int cmp_activity(const void *a, const void *b) {
    const struct activity_info *ia = a;
    const struct activity_info *ib = b;
    return strcmp(ia->command_name, ib->command_name);
}

int activities_command(int argc, char *argv[]) {
    (void)argc;  // unused
    (void)argv;

    check_background_jobs();  // clean up finished jobs

    if (bg_job_count == 0) {
        printf("No active background processes\n");
        return 0;
    }

    struct activity_info list[MAX_BG_JOBS];
    int n = 0;

    for (int i = 0; i < bg_job_count; i++) {
        int status;
        pid_t result = waitpid(bg_jobs[i].pid, &status,
                               WNOHANG | WUNTRACED | WCONTINUED);

        strncpy(list[n].command_name, bg_jobs[i].command_name,
                sizeof(list[n].command_name) - 1);
        list[n].command_name[sizeof(list[n].command_name) - 1] = '\0';
        list[n].pid = bg_jobs[i].pid;

        // Update the actual state based on current process status
        if (result == 0) {
            // Process is still running
            strcpy(list[n].state, "Running");
            strcpy(bg_jobs[i].state, "Running"); // Update stored state
        } else if (result > 0) {
            if (WIFSTOPPED(status)) {
                strcpy(list[n].state, "Stopped");
                strcpy(bg_jobs[i].state, "Stopped"); // Update stored state
            } else if (WIFCONTINUED(status)) {
                strcpy(list[n].state, "Running");
                strcpy(bg_jobs[i].state, "Running"); // Update stored state
            } else {
                // Process ended, will be cleaned up later
                continue;
            }
        } else {
            // Error - use stored state as fallback
            strcpy(list[n].state, bg_jobs[i].state);
        }
        n++;
    }

    qsort(list, n, sizeof(struct activity_info), cmp_activity);

    for (int i = 0; i < n; i++) {
        printf("[%d] : %s - %s\n",
               list[i].pid, list[i].command_name, list[i].state);
    }

    return 0;
}

int fg_builtin(int argc, char *argv[]) {
    check_background_jobs();  // clean up finished jobs first
    
    int target_job = -1;
    
    if (argc == 1) {
        // No job number provided, use most recently created job
        if (bg_job_count == 0) {
            printf("No such job\n");
            return -1;
        }
        // Find the job with highest job number (most recent)
        int max_job_num = -1;
        int max_idx = -1;
        for (int i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].job_number > max_job_num) {
                max_job_num = bg_jobs[i].job_number;
                max_idx = i;
            }
        }
        target_job = max_idx;
    } else if (argc == 2) {
        // Job number provided
        int job_num = atoi(argv[1]);
        if (job_num <= 0) {
            printf("No such job\n");
            return -1;
        }
        
        // Find job by job number
        for (int i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].job_number == job_num) {
                target_job = i;
                break;
            }
        }
        
        if (target_job == -1) {
            printf("No such job\n");
            return -1;
        }
    } else {
        printf("fg: Invalid Syntax!\n");
        return -1;
    }
    
    // Print the full command being brought to foreground
    printf("%s\n", bg_jobs[target_job].full_command);
    
    // Set as foreground process
    fg_pid = bg_jobs[target_job].pid;
    strncpy(fg_command, bg_jobs[target_job].command_name, sizeof(fg_command)-1);
    fg_command[sizeof(fg_command)-1] = '\0';
    
    // If job is stopped, send SIGCONT to resume it
    if (strcmp(bg_jobs[target_job].state, "Stopped") == 0) {
        kill(-bg_jobs[target_job].pid, SIGCONT);
    }
    
    // Remove job from background list
    for (int i = target_job; i < bg_job_count - 1; i++) {
        bg_jobs[i] = bg_jobs[i + 1];
    }
    bg_job_count--;
    
    // Wait for the process to complete or stop
    int status;
    waitpid(fg_pid, &status, WUNTRACED);
    fg_pid = -1;  // reset after process finishes/stops
    
    return 0;
}

int bg_builtin(int argc, char *argv[]) {
    check_background_jobs();  // clean up finished jobs first
    
    int target_job = -1;
    
    if (argc == 1) {
        // No job number provided, use most recently created job
        if (bg_job_count == 0) {
            printf("No such job\n");
            return -1;
        }
        // Find the job with highest job number (most recent)
        int max_job_num = -1;
        int max_idx = -1;
        for (int i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].job_number > max_job_num) {
                max_job_num = bg_jobs[i].job_number;
                max_idx = i;
            }
        }
        target_job = max_idx;
    } else if (argc == 2) {
        // Job number provided
        int job_num = atoi(argv[1]);
        if (job_num <= 0) {
            printf("No such job\n");
            return -1;
        }
        
        // Find job by job number
        for (int i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].job_number == job_num) {
                target_job = i;
                break;
            }
        }
        
        if (target_job == -1) {
            printf("No such job\n");
            return -1;
        }
    } else {
        printf("bg: Invalid Syntax!\n");
        return -1;
    }
    
    // Check if job is already running
    if (strcmp(bg_jobs[target_job].state, "Running") == 0) {
        printf("Job already running\n");
        return -1;
    }
    
    // Send SIGCONT to resume the stopped job
    kill(-bg_jobs[target_job].pid, SIGCONT);
    
    // Update job state to running
    strcpy(bg_jobs[target_job].state, "Running");
    
    // Print the resuming message
    printf("[%d] %s &\n", bg_jobs[target_job].job_number, bg_jobs[target_job].command_name);
    
    return 0;
}
// ############## LLM Generated Code Ends ################
