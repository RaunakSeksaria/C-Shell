#include <signal.h>
#include "command.h"
#include "background.h"
#include <string.h>
#include <stdio.h>

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
