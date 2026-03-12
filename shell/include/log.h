#ifndef LOG_H
#define LOG_H

// Function to handle the log command
int log_command(int argc, char *argv[]);

// Function to store a command in the log
void store_command(const char *command);

#endif // LOG_H