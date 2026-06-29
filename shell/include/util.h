#pragma once
#include <stddef.h>

// Allocation wrappers that abort on failure. A command-line shell has no
// meaningful way to recover from OOM mid-command, so rather than threading an
// unrecoverable NULL check through every call site we fail fast and loud. The
// returns_nonnull attribute also lets the static analyzer (gcc -fanalyzer)
// prove these never yield NULL, keeping call sites warning-free.
void *xmalloc(size_t size) __attribute__((malloc, returns_nonnull));
void *xrealloc(void *ptr, size_t size) __attribute__((returns_nonnull));
char *xstrdup(const char *s) __attribute__((returns_nonnull));
