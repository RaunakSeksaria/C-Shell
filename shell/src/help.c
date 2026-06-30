#include "help.h"
#include <stdio.h>

int help_command(void) {
    printf(
        "Built-in commands:\n"
        "  hop / cd [dir]          change directory (supports ~, -, ..)\n"
        "  reveal [-a] [-l] [dir]  list directory contents\n"
        "  log [purge|execute N]   command history (persists across sessions)\n"
        "  ping <pid> <signal>     send a signal to a process\n"
        "  activities              list background / stopped jobs\n"
        "  fg / bg [job]           resume a job in the fore/background\n"
        "  help                    show this message\n"
        "\n"
        "Operators:\n"
        "  cmd1 | cmd2             pipe stdout into stdin\n"
        "  cmd1 ; cmd2             run in sequence\n"
        "  cmd &                   run in the background\n"
        "  cmd < in > out >> app   input / output / append redirection\n");
    return 0;
}
