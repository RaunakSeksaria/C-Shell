"""log builtin: persistent command history (max 15, no consecutive dupes)."""

from tests.conftest import ShellFixture, nonempty_lines


def test_command_stored_in_history(sh):
    sh.run("echo hello")
    assert "echo hello" in sh.run("log")


def test_log_itself_not_stored(sh):
    sh.run("echo one")
    sh.run("log")
    entries = nonempty_lines(sh.run("log"))
    assert all(not e.startswith("log") for e in entries)


def test_consecutive_duplicate_not_stored(sh):
    sh.run("echo same")
    sh.run("echo same")
    entries = nonempty_lines(sh.run("log"))
    assert entries.count("echo same") == 1


def test_non_consecutive_duplicate_is_stored_again(sh):
    sh.run("echo one")
    sh.run("echo two")
    sh.run("echo one")
    entries = nonempty_lines(sh.run("log"))
    assert entries.count("echo one") == 2


def test_log_purge_clears_history(sh):
    sh.run("echo hello")
    sh.run("log purge")
    assert sh.run("log") == ""


def test_log_execute_reruns_by_index(sh):
    sh.run("echo marker_xyz")
    # entry 1 is the most recent: echo marker_xyz
    out = sh.run("log execute 1")
    assert "marker_xyz" in out


def test_log_execute_invalid_index(sh):
    out = sh.run("log execute 999")
    assert "valid index" in out


def test_log_execute_not_stored_in_history(sh):
    sh.run("echo alpha")
    sh.run("log execute 1")
    entries = nonempty_lines(sh.run("log"))
    assert not any(e.startswith("log") for e in entries)


def test_log_capped_at_15_entries(sh):
    sh.run("log purge")
    for i in range(20):
        sh.run(f"echo cmd{i}")
    entries = nonempty_lines(sh.run("log"))
    assert len(entries) == 15


def test_log_oldest_entry_dropped_when_full(sh):
    sh.run("log purge")
    for i in range(20):
        sh.run(f"echo cmd{i}")
    entries = nonempty_lines(sh.run("log"))
    assert "echo cmd0" not in entries
    assert "echo cmd19" in entries


def test_history_persists_across_sessions(tmp_path):
    s1 = ShellFixture(tmp_path)
    s1.expect_prompt()
    s1.run("echo persistent_entry")
    s1.close()

    s2 = ShellFixture(tmp_path)
    s2.expect_prompt()
    history = s2.run("log")
    s2.close()

    assert "echo persistent_entry" in history
