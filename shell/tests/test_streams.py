"""Stream routing: errors go to stderr, results go to stdout.

These use subprocess with separate pipes rather than the pexpect PTY fixture.
On a PTY the two streams merge, so the stdout/stderr distinction can only be
verified when they are captured independently.
"""

import subprocess
from pathlib import Path

SHELL_BIN = str(Path(__file__).parent.parent / "shell.out")


def run_piped(commands: str, cwd: Path) -> tuple[str, str]:
    """Feed commands on stdin (pipe, not a TTY); return (stdout, stderr)."""
    proc = subprocess.run(
        [SHELL_BIN],
        input=commands,
        capture_output=True,
        text=True,
        cwd=str(cwd),
        timeout=10,
    )
    return proc.stdout, proc.stderr


def test_command_not_found_on_stderr(tmp_path):
    out, err = run_piped("nonexistent_cmd_xyzabc\n", tmp_path)
    assert "Command not found!" in err
    assert "Command not found!" not in out


def test_invalid_syntax_on_stderr(tmp_path):
    out, err = run_piped("| broken\n", tmp_path)
    assert "Invalid Syntax!" in err
    assert "Invalid Syntax!" not in out


def test_hop_error_on_stderr(tmp_path):
    out, err = run_piped("hop /no/such/directory/xyzabc\n", tmp_path)
    assert "No such directory" in err
    assert "No such directory" not in out


def test_ping_error_on_stderr(tmp_path):
    out, err = run_piped("ping notapid 9\n", tmp_path)
    assert "Invalid syntax!" in err
    assert "Invalid syntax!" not in out


def test_normal_output_on_stdout(tmp_path):
    out, err = run_piped("echo hello\n", tmp_path)
    assert "hello" in out


def test_command_not_found_does_not_pollute_pipe(tmp_path):
    # A missing command mid-pipeline must not feed its error into the next stage.
    out, err = run_piped("nonexistent_cmd_xyzabc | cat\n", tmp_path)
    assert "Command not found!" not in out
    assert "Command not found!" in err


def test_long_command_line_does_not_overflow(tmp_path):
    # A line longer than the old fixed 4096-byte buffers in reconstruct_command
    # and expand_env_vars must not overflow. Driven via a pipe (not a PTY) to
    # avoid the terminal's canonical-mode line-length limit.
    arg = "a" * 8000
    out, _ = run_piped("echo " + arg + "\n", tmp_path)
    assert arg in out
