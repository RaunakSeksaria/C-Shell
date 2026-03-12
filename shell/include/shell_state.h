#ifndef SHELL_STATE_H
#define SHELL_STATE_H

#include <limits.h>

// Shared state between hop and reveal commands
extern char previous_dir[PATH_MAX];
extern int has_previous;

// Shell home directory (directory where shell was started)
extern char shell_home_dir[PATH_MAX];

#endif // SHELL_STATE_H