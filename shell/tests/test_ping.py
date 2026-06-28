"""ping builtin: sends kill(pid, signal % 32)."""

import os


def test_ping_sends_signal_to_self(sh):
    # Signal 0 just checks if the process exists — safe to send to ourselves
    pid = os.getpid()
    out = sh.run(f"ping {pid} 0")
    assert f"Sent signal 0 to process with pid {pid}" in out


def test_ping_applies_modulo_32(sh):
    pid = os.getpid()
    # signal 32 % 32 == 0 — kill(pid, 0) is a safe existence check
    out = sh.run(f"ping {pid} 32")
    assert "Sent signal 32 to process with pid" in out


def test_ping_nonexistent_process(sh):
    # PID 2147483647 is unlikely to exist
    out = sh.run("ping 2147483647 9")
    assert "No such process found" in out


def test_ping_non_numeric_pid(sh):
    out = sh.run("ping notapid 9")
    assert "Invalid syntax!" in out


def test_ping_non_numeric_signal(sh):
    pid = os.getpid()
    out = sh.run(f"ping {pid} notasig")
    assert "Invalid syntax!" in out


def test_ping_wrong_arg_count(sh):
    out = sh.run("ping 123")
    assert "Invalid syntax!" in out
