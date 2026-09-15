#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for GitDiffCheck in scripts/ci/check_compliance.py.

The check runs `git diff --check` and reads what it prints, so these drive it
against throwaway repositories rather than a recorded string. What the check
has to recognise is git's output, and that is git's to define: a test that
asserts against a hand-written copy of it would keep passing after git changed
the wording.
"""

import subprocess
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "ci"))

import check_compliance  # noqa: E402

CONFLICT = "a\n<<<<<<< HEAD\nb\n=======\nc\n>>>>>>> them\n"


def _git(repo, *args):
    subprocess.run(
        ("git", *args), cwd=repo, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )


@pytest.fixture
def repo(tmp_path):
    """A git repository with one clean commit to hang the next one off."""
    _git(tmp_path, "init", "-q")
    _git(tmp_path, "config", "user.email", "compliance@example.com")
    _git(tmp_path, "config", "user.name", "Compliance Test")
    # These decide what git prints, or whether it commits at all: signing is
    # off so the fixture does not need a key when the person running it has
    # commit.gpgsign set globally.
    _git(tmp_path, "config", "commit.gpgsign", "false")
    _git(tmp_path, "config", "core.autocrlf", "false")
    _git(tmp_path, "config", "core.whitespace", "blank-at-eol,space-before-tab,blank-at-eof")
    (tmp_path / "base.txt").write_text("base\n", newline="\n")
    _git(tmp_path, "add", "-A")
    _git(tmp_path, "commit", "-qm", "base")
    return tmp_path


def check(repo, files, monkeypatch):
    """Commit 'files' and return what GitDiffCheck says about that commit.

    Returns the failure text, or None when the check passed.
    """
    for name, content in files.items():
        path = repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, newline="")
    _git(repo, "add", "-A")
    _git(repo, "commit", "-qm", "the commit under test")

    # git() has no cwd of its own here, and COMMIT_RANGE is only bound by main().
    monkeypatch.chdir(repo)
    monkeypatch.setattr(check_compliance, "COMMIT_RANGE", "HEAD~1..HEAD", raising=False)

    test = check_compliance.GitDiffCheck()
    test.run()
    results = test.case.result
    return results[0].text if results else None


def test_a_conflict_marker_is_reported(repo, monkeypatch):
    """The check is named for this, and it is the message git does not end in
    a period."""
    out = check(repo, {"merged.c": CONFLICT}, monkeypatch)

    assert out is not None, "a commit adding conflict markers passed the check"
    assert "leftover conflict marker" in out
    # git flags every marker in the file, not just the first.
    assert out.count("leftover conflict marker") == 3
    # The report has to say where: a message with no file and no line in it
    # tells the contributor nothing.
    assert "merged.c" in out
    assert "merged.c:2:" in out


def test_a_dotfile_path_is_reported(repo, monkeypatch):
    """git prints the path as it finds it, so a report can begin with a dot.

    Nothing may be assumed about the first character: the tree has many paths
    under .github/ alone.
    """
    out = check(repo, {".github/workflows/ci.yml": CONFLICT}, monkeypatch)

    assert out is not None, "a dotfile full of conflict markers passed the check"
    assert out.count("leftover conflict marker") == 3


def test_a_path_starting_with_a_space_is_reported(repo, monkeypatch):
    """The path is unquoted, so the report line itself can start with a space."""
    out = check(repo, {" lead.c": "int a; \n"}, monkeypatch)

    assert out is not None, "a path beginning with a space passed the check"
    assert "trailing whitespace" in out


def test_trailing_whitespace_is_reported(repo, monkeypatch):
    out = check(repo, {"trail.c": "int a; \n"}, monkeypatch)

    assert out is not None
    assert "trailing whitespace" in out


def test_a_blank_line_at_eof_is_reported(repo, monkeypatch):
    out = check(repo, {"eof.c": "int a;\n\n"}, monkeypatch)

    assert out is not None
    assert "blank line at EOF" in out


def test_a_space_before_a_tab_is_reported(repo, monkeypatch):
    out = check(repo, {"sbt.c": " \tint a;\n"}, monkeypatch)

    assert out is not None
    assert "space before tab" in out


def test_conflict_markers_and_whitespace_together(repo, monkeypatch):
    """Both kinds in one commit: neither may hide the other."""
    out = check(repo, {"both.c": "<<<<<<< HEAD\nint a; \n"}, monkeypatch)

    assert out is not None
    assert "leftover conflict marker" in out
    assert "trailing whitespace" in out


def test_a_clean_commit_is_not_reported(repo, monkeypatch):
    """The control: without this, every other assertion here is satisfied by a
    check that fails on everything."""
    out = check(repo, {"clean.c": "int a;\n"}, monkeypatch)

    assert out is None, f"a clean commit was reported: {out}"


def test_an_added_line_that_looks_like_a_report_is_not_itself_reported(repo, monkeypatch):
    """git echoes the offending line after its report, prefixed with '+'.

    A line whose own text is shaped like "<path>:<line>: <message>" must not be
    counted as a second problem. Indented, because that is how the C it would
    come from is written, and the indentation is what a guard looking past the
    '+' would trip over.
    """
    out = check(repo, {"echo.c": '\tprintf("parse.c:12: bad thing"); \n'}, monkeypatch)

    assert out is not None
    assert out.count("trailing whitespace") == 1
    assert "bad thing" not in out


def test_diff_and_patch_files_are_left_alone(repo, monkeypatch):
    """Conflict markers are the point of a .patch file, so the pathspec skips
    them."""
    out = check(repo, {"sample.patch": CONFLICT, "sample.diff": CONFLICT}, monkeypatch)

    assert out is None, f"a patch file was reported: {out}"
