"""help builtin: lists built-in commands and operators."""


def test_help_lists_all_builtins(sh):
    out = sh.run("help")
    for name in ["hop", "cd", "reveal", "log", "ping", "activities", "fg", "bg", "help"]:
        assert name in out


def test_help_lists_operators(sh):
    out = sh.run("help")
    assert "pipe" in out
    assert "background" in out
    assert "redirection" in out
