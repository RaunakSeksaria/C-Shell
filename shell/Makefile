# Makefile for C Shell Project

# 1. VARIABLES
# ==============================================================================
# Compiler
CC = gcc

# Compiler flags
# -I$(INCDIR) tells the compiler to look for header files in the 'include' directory.
CFLAGS = -std=c99 \
         -D_POSIX_C_SOURCE=200809L \
         -D_XOPEN_SOURCE=700 \
         -Wall -Wextra -Werror \
         -Wno-unused-parameter \
         -fno-asm \
         -g # Added for debugging symbols

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build

# Final executable name
TARGET = shell.out

# Source files
# Using wildcard is more maintainable than listing files manually.
# It automatically finds all .c files in the source directory.
SRCS = $(wildcard $(SRCDIR)/*.c)

# Object files
# This creates a list of object files in the build directory.
# e.g., src/main.c -> build/main.o
OBJS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRCS))


# 2. RULES
# ==============================================================================
# Default goal: build the entire project.
all: $(TARGET)

# Rule to link object files into the final executable.
# '$^' is an automatic variable that stands for all prerequisites (the object files).
# '$@' is an automatic variable that stands for the target name.
$(TARGET): $(OBJS)
	@echo "Linking..."
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Build finished: $(TARGET)"

# Pattern rule to compile a .c source file from src/ into a .o object file in build/
# '$<' is an automatic variable that stands for the first prerequisite (the .c file).
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR) # Create the build directory if it doesn't exist.
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@


# 3. PHONY TARGETS (Utility Commands)
# ==============================================================================
# A phony target is one that is not a file.
.PHONY: all clean re analyze check

# 'clean' rule: remove all generated files.
clean:
	@echo "Cleaning project..."
	rm -rf $(BUILDDIR) $(TARGET)
	@echo "Clean complete."

# 're' rule: rebuild the entire project from scratch.
re: clean all

# 'analyze' rule: run GCC's static analyzer over every source file. CFLAGS
# already carries -Werror, so any finding fails the build — this is the gate
# CI runs to keep the tree analyzer-clean. Kept separate from 'all' because
# -fanalyzer is too slow to pay for on every incremental build.
analyze:
	@echo "Running static analysis (gcc -fanalyzer)..."
	@set -e; for src in $(SRCS); do \
		$(CC) $(CFLAGS) -fanalyzer -I$(INCDIR) -c $$src -o /dev/null; \
	done
	@echo "Static analysis clean."

# 'check' rule: the full pre-merge gate — build cleanly, then analyze.
check: all analyze