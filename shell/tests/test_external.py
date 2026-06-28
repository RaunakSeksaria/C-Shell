"""External command execution: fork/exec path."""

import os


def test_echo_single_arg(sh):
    assert sh.run("echo hello") == "hello"


def test_echo_multiple_args(sh):
    assert sh.run("echo foo bar baz") == "foo bar baz"


def test_unknown_command_reports_error(sh):
    out = sh.run("this_command_does_not_exist_xyzabc")
    assert "Command not found!" in out


def test_pwd_shows_launch_directory(sh, tmp_path):
    assert sh.run("pwd") == str(tmp_path)


def test_env_var_expansion(sh):
    out = sh.run("echo $HOME")
    assert out == os.environ.get("HOME", "")
