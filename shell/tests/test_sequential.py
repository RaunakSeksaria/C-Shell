"""Semicolon-separated sequential command execution."""


def test_both_commands_run(sh):
    out = sh.run("echo first ; echo second")
    assert "first" in out
    assert "second" in out


def test_output_order_preserved(sh):
    out = sh.run("echo aaa ; echo bbb")
    assert out.index("aaa") < out.index("bbb")


def test_second_runs_after_first_fails(sh):
    out = sh.run("this_command_does_not_exist ; echo still_ran")
    assert "still_ran" in out


def test_three_commands_sequential(sh):
    out = sh.run("echo one ; echo two ; echo three")
    assert out.index("one") < out.index("two") < out.index("three")
