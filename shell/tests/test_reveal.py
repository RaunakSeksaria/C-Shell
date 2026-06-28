"""reveal builtin: ls equivalent with -a and -l flags."""

from tests.conftest import nonempty_lines


def test_reveal_lists_visible_files(sh, tmp_path):
    (tmp_path / "apple.txt").write_text("a")
    (tmp_path / "banana.txt").write_text("b")
    out = sh.run("reveal")
    assert "apple.txt" in out
    assert "banana.txt" in out


def test_reveal_hides_dotfiles_by_default(sh, tmp_path):
    (tmp_path / "visible.txt").write_text("")
    (tmp_path / ".hidden").write_text("")
    out = sh.run("reveal")
    assert "visible.txt" in out
    assert ".hidden" not in out


def test_reveal_a_shows_dotfiles(sh, tmp_path):
    (tmp_path / ".hidden").write_text("")
    out = sh.run("reveal -a")
    assert ".hidden" in out


def test_reveal_l_puts_each_file_on_own_line(sh, tmp_path):
    (tmp_path / "one.txt").write_text("")
    (tmp_path / "two.txt").write_text("")
    out = sh.run("reveal -l")
    names = nonempty_lines(out)
    # Every entry is a single filename with no spaces
    assert all(" " not in name for name in names)
    assert "one.txt" in names
    assert "two.txt" in names


def test_reveal_sorted_alphabetically(sh, tmp_path):
    for name in ["zebra.txt", "apple.txt", "mango.txt"]:
        (tmp_path / name).write_text("")
    out = sh.run("reveal -l")
    names = nonempty_lines(out)
    assert names == sorted(names)


def test_reveal_tilde_lists_home(sh, tmp_path):
    (tmp_path / "file.txt").write_text("")
    out = sh.run("reveal ~")
    assert "file.txt" in out


def test_reveal_nonexistent_path(sh):
    out = sh.run("reveal /does/not/exist/xyzabc")
    assert "No such directory" in out


def test_reveal_combined_flags(sh, tmp_path):
    (tmp_path / ".git").mkdir()
    (tmp_path / "src").mkdir()
    out = sh.run("reveal -la")
    names = nonempty_lines(out)
    assert ".git" in names
    assert "src" in names
    assert names == sorted(names)
