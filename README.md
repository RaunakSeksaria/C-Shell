# shell

A POSIX shell written from scratch in C — interactive prompt, pipes, I/O redirection, job control, and history that persists across sessions.

It follows a clean **tokenize → parse → execute** pipeline: a hand-written lexer, a recursive-descent grammar validator, and an executor that forks/`exec`s external commands, wires up pipelines with `dup2`, and forwards terminal signals to the foreground job. Built under strict POSIX flags with zero warnings, covered by 84 integration tests, and gated by static analysis on every change.

<!--
  Recorded demo: drop a GIF at docs/demo.gif and uncomment the line below to
  replace the static transcript with a moving one.

      ![demo](docs/demo.gif)

  To record one:
    asciinema rec demo.cast        # run the sequence below, then Ctrl-D to stop
    agg demo.cast docs/demo.gif    # convert to GIF — https://github.com/asciinema/agg
  Suggested sequence:
    reveal -l | head -3
    echo counting these words | wc -w
    echo persisted to history > note.txt ; cat note.txt
    sleep 30 &
    activities
    log
-->

## Demo

```text
# the start directory becomes "home"; the prompt collapses it to ~
<user@host:~> hop src
<user@host:~/src> hop -
<user@host:~>

# pipelines and I/O redirection
<user@host:~> echo counting these words | wc -w
3
<user@host:~> echo persisted to history > note.txt
<user@host:~> cat < note.txt
persisted to history

# background jobs and job control
<user@host:~> sleep 30 &
[1] 48217
<user@host:~> activities
[48217] : sleep - Running

# history persists across sessions; each whole line is stored as one entry
<user@host:~> log
hop src
hop -
echo counting these words | wc -w
echo persisted to history > note.txt
cat < note.txt
sleep 30 &
activities
```

`Ctrl-Z` stops the foreground job and parks it (`[1] Stopped sleep`); `fg`/`bg` resume it; `Ctrl-C` interrupts the job without killing the shell; `Ctrl-D` cleans up background jobs and exits.

## Features

- **Interactive prompt** — `<user@host:cwd>`, with the launch directory shown as `~` (and paths beneath it as `~/sub`).
- **External commands** — arbitrary programs via `fork` + `execvp`, with argument `$VAR` expansion.
- **Pipelines** — `a | b | c`, one child per stage, stdio wired with `pipe`/`dup2`.
- **I/O redirection** — `<`, `>`, `>>`, combinable with pipes; last redirection of a kind wins.
- **Sequencing & background** — `;` runs commands in order; `&` runs without blocking the prompt.
- **Job control** — `Ctrl-C`/`Ctrl-Z`/`Ctrl-D`, `fg`, `bg`, and `activities` to list running/stopped jobs.
- **Built-ins** — `hop` (cd, with `~`/`-`/`..`), `reveal` (ls, with `-a`/`-l`), `log` (persistent history with `purge` and `execute <n>`), `ping` (send a signal to a pid).

## Quick start

Everything runs from the `shell/` directory:

```bash
cd shell
make all      # build → shell.out
./shell.out   # start the shell
```

Sources compile with `-std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror` — only the C POSIX library is used.

## Quality & tooling

This is the part that separates it from a typical toy shell:

| Concern | Approach |
|---|---|
| **Tests** | 84 integration tests driving a real PTY via `pexpect` (`uv run python -m pytest tests/`), plus `subprocess`-based tests that verify errors go to `stderr` and results to `stdout` independently. |
| **Static analysis** | `make analyze` recompiles every source with `gcc -fanalyzer`; with `-Werror` in effect, any finding fails the build. `make check` = build + analyze (the CI gate). |
| **Memory safety** | All heap allocation goes through abort-on-OOM `xmalloc`/`xrealloc`/`xstrdup` wrappers, marked `returns_nonnull` so both readers and the analyzer can assume allocations never return `NULL`. |
| **Test-suite lint** | The Python test suite is kept `ruff`-clean (`uv run ruff check tests/`). |

```bash
make check                        # build + static-analysis gate
uv run python -m pytest tests/    # full test suite
```

## Architecture

`take_input → tokenize → parse_input → execute_command`. The lexer produces zero-copy tokens that point into the input buffer; the recursive-descent parser only validates shape; the executor re-walks the tokens to run them, checking built-ins before `exec` so commands like `hop` and `log` mutate the shell's own state rather than a throwaway child's. Each concern lives in its own `.c`/`.h` pair under `shell/`.

See **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** for the grammar, pipeline internals, signal/process-group handling, and the design decisions behind them.

## Project layout

```
shell/
  src/        # implementation (lexer, parser, executor, builtins, signals, utils)
  include/    # public headers, one per module
  tests/      # pytest + pexpect integration suite
  Makefile    # all / clean / re / analyze / check
docs/
  ARCHITECTURE.md
```

## Limitations & roadmap

Known gaps, kept honest:

- No quoting or escaping yet (`echo "a b"` keeps the quotes); no command substitution (`$(...)`).
- No short-circuit operators (`&&`, `||`).
- Job control is approximate: foreground children aren't isolated into their own
  process groups (`setpgid`/`tcsetpgrp`), so `Ctrl-Z`/`fg`/`bg` rely on terminal
  group delivery rather than the shell driving each job's group directly. Proper
  per-job process groups are the next step here.
- Planned: arrow-key history navigation and line editing (raw-mode `termios`).
