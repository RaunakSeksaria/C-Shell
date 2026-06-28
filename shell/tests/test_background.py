"""Background job execution: &, activities, completion messages."""

import re
import time

import pytest

from tests.conftest import nonempty_lines


def test_background_prints_job_number_and_pid(sh):
    out = sh.run("sleep 1 &")
    assert re.search(r"\[\d+\] \d+", out)


def test_background_returns_prompt_without_waiting(sh):
    start = time.monotonic()
    sh.run("sleep 10 &")
    elapsed = time.monotonic() - start
    # sleep 10 would block for ~10s; prompt should return almost instantly
    assert elapsed < 3.0


def test_activities_shows_running_job(sh):
    sh.run("sleep 30 &")
    out = sh.run("activities")
    assert "sleep" in out
    assert "Running" in out


def test_activities_shows_no_jobs_when_empty(sh):
    out = sh.run("activities")
    assert "No active background processes" in out


@pytest.mark.slow
def test_completion_message_on_next_prompt(sh):
    sh.run("sleep 0.1 &")
    time.sleep(0.4)
    # Empty command triggers check_background_jobs before printing the prompt
    out = sh.run("")
    assert "exited normally" in out


@pytest.mark.slow
def test_activities_sorted_lexicographically(sh):
    # Start two background jobs; activities should sort by command name
    sh.run("sleep 30 &")
    sh.run("cat /dev/zero &")
    out = sh.run("activities")
    lines = nonempty_lines(out)
    names = [m.group(1) for line in lines if (m := re.search(r"- (\S+)", line))]
    assert names == sorted(names)
