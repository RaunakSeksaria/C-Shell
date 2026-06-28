"""Parser: valid and invalid command syntax.

These tests verify the tokenizer/parser boundary — commands that should be
rejected before execution (Invalid Syntax!) vs. commands that should reach
the executor even if they fail to run.
"""

import pytest

INVALID = [
    "| echo hi",  # pipe at start — atomic requires NAME first
    "; echo hi",  # semicolon at start
    "echo ; ;",  # consecutive separators
    "echo | ; echo",  # pipe then semicolon (semicolon is not atomic)
    "echo > > file",  # two consecutive output redirects with no filename between
    "echo ;",  # trailing semicolon (only trailing & is valid)
]

VALID = [
    "echo hello",
    "echo a | cat",
    "echo a ; echo b",
    "echo hello > /dev/null",
    "echo &",  # trailing & is valid (background)
    "cat < /dev/null",
    "echo a ; echo b &",  # trailing & on a sequence is valid
]


@pytest.mark.parametrize("cmd", INVALID)
def test_invalid_syntax_rejected(sh, cmd):
    assert "Invalid Syntax!" in sh.run(cmd)


@pytest.mark.parametrize("cmd", VALID)
def test_valid_syntax_not_rejected(sh, cmd):
    assert "Invalid Syntax!" not in sh.run(cmd)
