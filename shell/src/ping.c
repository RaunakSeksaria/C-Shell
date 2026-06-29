#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

int ping_command(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Invalid syntax!\n");
        return -1;
    }

    char *endptr;
    long pid_val = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || pid_val <= 0) {
        fprintf(stderr, "Invalid syntax!\n");
        return -1;
    }

    long sig_val = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid syntax!\n");
        return -1;
    }

    // Wrap the requested number into the valid signal range [0, 31] so any
    // input maps to a real signal rather than being rejected.
    int actual_signal = sig_val % 32;

    if (kill((pid_t)pid_val, actual_signal) == -1) {
        if (errno == ESRCH) {
            fprintf(stderr, "No such process found\n");
        } else {
            perror("kill");
        }
        return -1;
    }

    printf("Sent signal %ld to process with pid %ld\n", sig_val, pid_val);
    return 0;
}
