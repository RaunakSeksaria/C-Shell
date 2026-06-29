#pragma once
#include <sys/types.h>

typedef struct {
    int job_number;
    pid_t pid;
    char command_name[256];
    char full_command[1024];  // Store full command for fg display
    char state[16];           // "Running" or "Stopped"
} BackgroundJob;

struct activity_info {
    pid_t pid;
    char command_name[256];
    char state[16];
};

#define MAX_BG_JOBS 128

extern BackgroundJob bg_jobs[MAX_BG_JOBS];
extern int bg_job_count;
extern int next_job_number;

void check_background_jobs(void);
int activities_command(void);
int fg_builtin(int argc, char *argv[]);
int bg_builtin(int argc, char *argv[]);

// Park a just-stopped foreground job (Ctrl-Z) in the background table.
void park_stopped_job(pid_t pid, const char *name, const char *full_command);
