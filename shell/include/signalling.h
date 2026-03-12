#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

void sigint_handler(int sig);
void sigtstp_handler(int sig);

#endif // SIGNAL_HANDLER_H
