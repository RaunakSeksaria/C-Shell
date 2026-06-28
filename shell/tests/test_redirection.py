"""I/O redirection: >, >>, < operators."""


def test_output_redirect_creates_file(sh, tmp_path):
    outfile = tmp_path / "out.txt"
    sh.run(f"echo hello > {outfile}")
    assert outfile.read_text() == "hello\n"


def test_output_redirect_truncates_existing_content(sh, tmp_path):
    outfile = tmp_path / "out.txt"
    outfile.write_text("old content\n")
    sh.run(f"echo new > {outfile}")
    assert outfile.read_text() == "new\n"


def test_append_redirect_adds_to_existing_file(sh, tmp_path):
    outfile = tmp_path / "out.txt"
    sh.run(f"echo line1 > {outfile}")
    sh.run(f"echo line2 >> {outfile}")
    content = outfile.read_text()
    assert "line1" in content
    assert "line2" in content
    assert content.index("line1") < content.index("line2")


def test_input_redirect_feeds_file_to_command(sh, tmp_path):
    infile = tmp_path / "input.txt"
    infile.write_text("from file\n")
    out = sh.run(f"cat < {infile}")
    assert out == "from file"


def test_combined_input_and_output_redirect(sh, tmp_path):
    infile = tmp_path / "in.txt"
    outfile = tmp_path / "out.txt"
    infile.write_text("redirected\n")
    sh.run(f"cat < {infile} > {outfile}")
    assert outfile.read_text() == "redirected\n"


def test_missing_input_file_reports_error(sh):
    out = sh.run("cat < /nonexistent_input_file_xyz.txt")
    assert "No such file or directory" in out


def test_output_redirect_in_pipe_chain(sh, tmp_path):
    # Redirect applies to last command in the pipe
    outfile = tmp_path / "out.txt"
    sh.run(f"echo hello > {outfile}")
    sh.run(f"cat {outfile} | cat > {tmp_path}/out2.txt")
    assert (tmp_path / "out2.txt").read_text() == "hello\n"
