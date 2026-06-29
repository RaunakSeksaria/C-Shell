#include <signal.h>
#include "command.h"
#include "background.h"
#include <string.h>
#include <stdio.h>

// The shell catches SIGINT/SIGTSTP and forwards them to the foreground job
// instead of acting on them itself, so Ctrl-C / Ctrl-Z hit the running command
// while the shell survives. The negative pid targets the job's whole process
// group, reaching any children it spawned; with no foreground job the signal
// is simply swallowed.
void sigint_handler(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        kill(-fg_pid, SIGINT);
    }
}

void sigtstp_handler(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        kill(-fg_pid, SIGTSTP);
        // Park the stopped job in the background table so it appears in
        // `activities` and can be resumed later with fg/bg.
        if (bg_job_count < MAX_BG_JOBS) {
            bg_jobs[bg_job_count].job_number = next_job_number++;
            bg_jobs[bg_job_count].pid = fg_pid;
            strncpy(bg_jobs[bg_job_count].command_name, fg_command, 255);
            bg_jobs[bg_job_count].command_name[255] = '\0';
            strncpy(bg_jobs[bg_job_count].full_command, fg_full_command, 1023);
            bg_jobs[bg_job_count].full_command[1023] = '\0';
            strcpy(bg_jobs[bg_job_count].state, "Stopped");
            bg_job_count++;
        }
        printf("[%d] Stopped %s\n", next_job_number - 1, fg_command);
        fflush(stdout);
        fg_pid = -1;
    }
}
