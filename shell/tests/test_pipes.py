"""Pipe execution: fork/exec chain with dup2-wired stdio."""


def test_single_pipe(sh):
    assert sh.run("echo hello | cat") == "hello"


def test_three_stage_pipe(sh):
    assert sh.run("echo hello | cat | cat") == "hello"


def test_pipe_with_output_redirect(sh, tmp_path):
    outfile = tmp_path / "out.txt"
    sh.run(f"echo piped | cat > {outfile}")
    assert outfile.read_text() == "piped\n"


def test_pipe_passes_file_content(sh, tmp_path):
    infile = tmp_path / "input.txt"
    infile.write_text("hello world\n")
    out = sh.run(f"cat {infile} | cat")
    assert out == "hello world"


def test_pipe_word_count(sh, tmp_path):
    # "Hi There!" has 1 line, 2 words, 10 chars (including newline)
    # No quotes used — this shell has no quoting support
    outfile = tmp_path / "wc.txt"
    sh.run(f"echo Hi There! | cat | wc > {outfile}")
    nums = list(map(int, outfile.read_text().split()))
    assert nums == [1, 2, 10]
