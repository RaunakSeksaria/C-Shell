"""
Shared fixture for shell integration tests.

Each test gets a fresh shell process (via pexpect PTY) rooted at a
pytest-provided tmp_path.  That directory becomes the shell's "home"
(shell_home_dir), so the prompt shows ~ when you are there.
"""

import re
from pathlib import Path

import pexpect
import pytest
from psutil import Process

SHELL_BIN = str(Path(__file__).parent.parent / "shell.out")
TIMEOUT = 5.0

# Matches the shell prompt <user@host:path> — the \r\n exclusion prevents
# matching commands that contain < and @ across multiple output lines.
PROMPT_PAT = re.compile(r"<[^@>\r\n]+@[^:>\r\n]+:[^>\r\n]+> ")


def nonempty_lines(text: str) -> list[str]:
    """Split output into stripped, non-empty lines."""
    return [line.strip() for line in text.split("\n") if line.strip()]


class ShellFixture:
    def __init__(self, home: Path):
        self.home = str(home)
        self.shell = pexpect.spawn(
            SHELL_BIN,
            cwd=self.home,
            encoding="utf-8",
            codec_errors="replace",
            timeout=TIMEOUT,
        )
        self._proc = Process(pid=self.shell.pid)

    def cwd(self) -> str:
        """Return the shell process's actual working directory."""
        return self._proc.cwd()

    def expect_prompt(self):
        self.shell.expect(PROMPT_PAT, timeout=TIMEOUT)

    def run(self, cmd: str) -> str:
        """Send cmd, wait for the next prompt, return the output text."""
        self.shell.sendline(cmd)
        self.expect_prompt()
        out = self.shell.before
        # Strip the PTY echo of the command itself
        echo = cmd + "\r\n"
        if out.startswith(echo):
            out = out[len(echo) :]
        return out.replace("\r\n", "\n").strip()

    def close(self):
        self.shell.close(force=True)


@pytest.fixture
def sh(tmp_path):
    fixture = ShellFixture(tmp_path)
    fixture.expect_prompt()
    yield fixture
    fixture.close()
