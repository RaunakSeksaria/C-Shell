"""Signal handling: Ctrl-C, Ctrl-Z, Ctrl-D, fg, bg."""
import re
import time
import pytest


@pytest.mark.slow
def test_ctrl_c_kills_foreground_but_not_shell(sh):
    sh.shell.sendline("sleep 10")
    time.sleep(0.15)
    sh.shell.sendintr()
    sh.expect_prompt()
    # Shell is still alive
    assert sh.run("echo alive") == "alive"


@pytest.mark.slow
def test_ctrl_z_stops_foreground_and_adds_to_bg(sh):
    sh.shell.sendline("sleep 10")
    time.sleep(0.15)
    sh.shell.sendcontrol('z')
    # Signal handler prints "[N] Stopped sleep"
    sh.shell.expect(re.compile(r'\[\d+\] Stopped sleep'), timeout=TIMEOUT)
    sh.expect_prompt()
    # Job should now appear in activities as Stopped
    out = sh.run("activities")
    assert "sleep" in out
    assert "Stopped" in out


@pytest.mark.slow
def test_ctrl_d_exits_shell_cleanly(sh):
    sh.shell.sendcontrol('d')
    sh.shell.expect_exact("logout", timeout=TIMEOUT)


@pytest.mark.slow
def test_fg_resumes_stopped_job_and_shell_waits(sh):
    sh.shell.sendline("sleep 10")
    time.sleep(0.15)
    sh.shell.sendcontrol('z')
    sh.shell.expect(re.compile(r'\[\d+\] Stopped'), timeout=TIMEOUT)
    sh.expect_prompt()

    sh.shell.sendline("fg")
    time.sleep(0.15)
    # Shell should be blocked waiting for sleep; kill it with Ctrl-C
    sh.shell.sendintr()
    sh.expect_prompt()
    assert sh.run("echo back") == "back"


TIMEOUT = 5.0
