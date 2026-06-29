#include "background.h"
#include "command.h"
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

BackgroundJob bg_jobs[MAX_BG_JOBS];
int bg_job_count = 0;
int next_job_number = 1;

// Reaps finished background jobs without blocking (WNOHANG); run once before
// every prompt. When a job is removed we shift the tail down and deliberately
// do not advance i, so the entry that slides into slot i is still examined.
void check_background_jobs(void) {
    for (int i = 0; i < bg_job_count; ) {
        int status;
        pid_t result = waitpid(bg_jobs[i].pid, &status, WNOHANG);

        if (result == 0) {
            i++;
            continue;
        } else if (result == -1) {
            for (int j = i; j < bg_job_count - 1; j++) bg_jobs[j] = bg_jobs[j+1];
            bg_job_count--;
            continue;
        } else {
            if (WIFEXITED(status)) {
                printf("%s with pid %d exited normally\n", bg_jobs[i].command_name, bg_jobs[i].pid);
            } else {
                printf("%s with pid %d exited abnormally\n", bg_jobs[i].command_name, bg_jobs[i].pid);
            }
            for (int j = i; j < bg_job_count - 1; j++) bg_jobs[j] = bg_jobs[j+1];
            bg_job_count--;
        }
    }
}

static int cmp_activity(const void *a, const void *b) {
    const struct activity_info *ia = a;
    const struct activity_info *ib = b;
    return strcmp(ia->command_name, ib->command_name);
}

int activities_command(void) {
    check_background_jobs();

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

        if (result == 0) {
            // Nothing waiting in the queue. A Ctrl-Z stop was already consumed
            // by the foreground waitpid (execute.c / fg_builtin), so the last
            // state we recorded is the authoritative one.
            strcpy(list[n].state, bg_jobs[i].state);
        } else if (result > 0) {
            if (WIFSTOPPED(status)) {
                strcpy(list[n].state, "Stopped");
                strcpy(bg_jobs[i].state, "Stopped");
            } else if (WIFCONTINUED(status)) {
                strcpy(list[n].state, "Running");
                strcpy(bg_jobs[i].state, "Running");
            } else {
                continue;
            }
        } else {
            strcpy(list[n].state, bg_jobs[i].state);
        }
        n++;
    }

    qsort(list, n, sizeof(struct activity_info), cmp_activity);

    for (int i = 0; i < n; i++) {
        printf("[%d] : %s - %s\n", list[i].pid, list[i].command_name, list[i].state);
    }

    return 0;
}

// Parks a just-stopped foreground job in the background table as "Stopped" so it
// appears in activities and can be resumed with fg/bg. Called from the waitpid
// sites in normal context — which is what lets the SIGTSTP handler stay
// async-signal-safe (it only forwards the signal).
void park_stopped_job(pid_t pid, const char *name, const char *full_command) {
    if (bg_job_count >= MAX_BG_JOBS) return;

    BackgroundJob *job = &bg_jobs[bg_job_count++];
    job->job_number = next_job_number++;
    job->pid = pid;
    strncpy(job->command_name, name, sizeof(job->command_name) - 1);
    job->command_name[sizeof(job->command_name) - 1] = '\0';
    strncpy(job->full_command, full_command, sizeof(job->full_command) - 1);
    job->full_command[sizeof(job->full_command) - 1] = '\0';
    strcpy(job->state, "Stopped");
    printf("[%d] Stopped %s\n", job->job_number, name);
}

int fg_builtin(int argc, char *argv[]) {
    check_background_jobs();

    int target_job = -1;

    if (argc == 1) {
        if (bg_job_count == 0) {
            fprintf(stderr, "No such job\n");
            return -1;
        }
        int max_job_num = -1;
        for (int i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].job_number > max_job_num) {
                max_job_num = bg_jobs[i].job_number;
                target_job = i;
            }
        }
    } else if (argc == 2) {
        int job_num = atoi(argv[1]);
        if (job_num <= 0) {
            fprintf(stderr, "No such job\n");
            return -1;
        }
        for (int i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].job_number == job_num) {
                target_job = i;
                break;
            }
        }
        if (target_job == -1) {
            fprintf(stderr, "No such job\n");
            return -1;
        }
    } else {
        fprintf(stderr, "fg: Invalid Syntax!\n");
        return -1;
    }

    printf("%s\n", bg_jobs[target_job].full_command);

    fg_pid = bg_jobs[target_job].pid;
    strncpy(fg_command, bg_jobs[target_job].command_name, sizeof(fg_command)-1);
    fg_command[sizeof(fg_command)-1] = '\0';
    strncpy(fg_full_command, bg_jobs[target_job].full_command, sizeof(fg_full_command)-1);
    fg_full_command[sizeof(fg_full_command)-1] = '\0';

    if (strcmp(bg_jobs[target_job].state, "Stopped") == 0) {
        kill(-bg_jobs[target_job].pid, SIGCONT);
    }

    for (int i = target_job; i < bg_job_count - 1; i++) bg_jobs[i] = bg_jobs[i + 1];
    bg_job_count--;

    fg_stop_requested = 0;
    int status = 0;
    pid_t resumed = fg_pid;
    while (1) {
        pid_t w = waitpid(fg_pid, &status, WUNTRACED);
        if (w == fg_pid) break;
        if (w == -1 && errno == EINTR) {
            if (fg_stop_requested) break;
            continue;
        }
        break;
    }
    fg_pid = -1;
    if (fg_stop_requested || WIFSTOPPED(status)) {
        fg_stop_requested = 0;
        park_stopped_job(resumed, fg_command, fg_full_command);
    }

    return 0;
}

int bg_builtin(int argc, char *argv[]) {
    check_background_jobs();

    int target_job = -1;

    if (argc == 1) {
        if (bg_job_count == 0) {
            fprintf(stderr, "No such job\n");
            return -1;
        }
        int max_job_num = -1;
        for (int i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].job_number > max_job_num) {
                max_job_num = bg_jobs[i].job_number;
                target_job = i;
            }
        }
    } else if (argc == 2) {
        int job_num = atoi(argv[1]);
        if (job_num <= 0) {
            fprintf(stderr, "No such job\n");
            return -1;
        }
        for (int i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].job_number == job_num) {
                target_job = i;
                break;
            }
        }
        if (target_job == -1) {
            fprintf(stderr, "No such job\n");
            return -1;
        }
    } else {
        fprintf(stderr, "bg: Invalid Syntax!\n");
        return -1;
    }

    if (strcmp(bg_jobs[target_job].state, "Running") == 0) {
        fprintf(stderr, "Job already running\n");
        return -1;
    }

    kill(-bg_jobs[target_job].pid, SIGCONT);
    strcpy(bg_jobs[target_job].state, "Running");
    printf("[%d] %s &\n", bg_jobs[target_job].job_number, bg_jobs[target_job].command_name);

    return 0;
}
