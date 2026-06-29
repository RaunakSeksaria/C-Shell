"""hop builtin: cd equivalent with ~, .., -, and multi-arg support."""


def test_hop_no_args_returns_to_home(sh, tmp_path):
    sub = tmp_path / "subdir"
    sub.mkdir()
    sh.run(f"hop {sub}")
    sh.run("hop")
    assert sh.cwd() == str(tmp_path)


def test_cd_is_an_alias_for_hop(sh, tmp_path):
    sub = tmp_path / "subdir"
    sub.mkdir()
    sh.run(f"cd {sub}")
    assert sh.cwd() == str(sub)
    sh.run("cd ..")
    assert sh.cwd() == str(tmp_path)


def test_hop_tilde(sh, tmp_path):
    sub = tmp_path / "subdir"
    sub.mkdir()
    sh.run(f"hop {sub}")
    sh.run("hop ~")
    assert sh.cwd() == str(tmp_path)


def test_hop_dotdot(sh, tmp_path):
    sub = tmp_path / "a" / "b"
    sub.mkdir(parents=True)
    sh.run(f"hop {sub}")
    sh.run("hop ..")
    assert sh.cwd() == str(tmp_path / "a")


def test_hop_dash_goes_to_previous(sh, tmp_path):
    sub = tmp_path / "dest"
    sub.mkdir()
    sh.run(f"hop {sub}")
    sh.run("hop ~")
    sh.run("hop -")
    assert sh.cwd() == str(sub)


def test_hop_dash_without_prior_history_is_silent_noop(sh, tmp_path):
    # has_previous == 0 on startup — hop - should silently skip
    out = sh.run("hop -")
    assert "No such directory" not in out
    assert sh.cwd() == str(tmp_path)


def test_hop_nonexistent_directory(sh):
    out = sh.run("hop /this/path/cannot/possibly/exist/xyzabc")
    assert "No such directory" in out


def test_hop_absolute_path(sh, tmp_path):
    sub = tmp_path / "target"
    sub.mkdir()
    sh.run(f"hop {sub}")
    assert sh.cwd() == str(sub)


def test_hop_tilde_prefix_path(sh, tmp_path):
    sub = tmp_path / "child"
    sub.mkdir()
    sh.run("hop ~/child")
    assert sh.cwd() == str(sub)


def test_hop_multi_arg_chain(sh, tmp_path):
    # hop .. .. ~ /  — mirrors the existing test.py TestPartB.test_part1
    sh.run(f"hop {tmp_path}/a/b/c")  # this path doesn't exist; hop will fail
    # Use a real deep path
    deep = tmp_path / "a" / "b" / "c"
    deep.mkdir(parents=True)
    sh.run(f"hop {deep}")
    sh.run("hop .. .. ..")
    assert sh.cwd() == str(tmp_path)
