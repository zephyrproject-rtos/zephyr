# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for twisterlib.twister_main module."""

from pathlib import Path
from types import SimpleNamespace

import pytest
from twisterlib.twister_main import Twister, catch_system_exit_exception


@pytest.fixture
def outdir(tmp_path) -> Path:
    """Return twister output temporay directory."""
    outdir_path = tmp_path / "twister-out"
    return outdir_path


@pytest.fixture
def options(outdir) -> SimpleNamespace:
    """Return fake default options."""
    opts = SimpleNamespace(
        no_clean=False,
        only_failed=False,
        test_only=False,
        list_tests=False,
        list_tags=False,
        test_tree=False,
        report_summary=None,
        outdir=str(outdir),
        clobber_output=False,
        last_metrics=False,
    )
    return opts


@catch_system_exit_exception
def _func_raising_system_exit(code):
    raise SystemExit(code)


@pytest.mark.parametrize(
    "value, expected_return",
    [
        (0, 0),
        (1, 1),
        (2, 2),
        (-1, -1),
        (None, 0),
        ("", 1),  # code is not int/None -> return 1
        ("error", 1),
    ],
    ids=["int-0", "int-1", "int-2", "int-neg1", "None", "empty-str", "non-int-str"],
)
def test_catch_system_exit_exception_return_codes(value, expected_return):
    """Decorator should catch SystemExit and return code (int), 0 for None, 1 for other non-int."""
    assert _func_raising_system_exit(value) == expected_return


def test_catch_system_exit_exception_no_exit():
    """Decorator should return normal return value when no SystemExit is raised."""

    @catch_system_exit_exception
    def no_exit():
        return 42

    assert no_exit() == 42


def test_twister_clean_previous_results_no_clean_keeps_artifacts(outdir, options, capsys):
    """When no_clean is True and outdir exists, print message and do not delete."""
    options.no_clean = True
    outdir.mkdir()
    (outdir / "dummy").write_text("x")
    t = Twister(options, options)
    result = t.clean_previous_results()
    assert result is None
    out, _ = capsys.readouterr()
    assert "Keeping artifacts untouched" in out


def test_twister_clean_previous_results_outdir_missing(tmp_path, options, capsys):
    """When outdir does not exist, no cleanup and return None."""
    options.outdir = str(tmp_path / "nonexistent")
    options.no_clean = False
    t = Twister(options, options)
    result = t.clean_previous_results()
    assert result is None
    out, _ = capsys.readouterr()
    assert "Keeping artifacts untouched" not in out
    assert "Renaming previous output directory" not in out


def test_twister_clean_previous_results_clobber_deletes(outdir, options, capsys):
    """When clobber_output is True and outdir exists, directory is removed."""
    outdir.mkdir()
    (outdir / "file").write_text("x")
    options.clobber_output = True
    t = Twister(options, options)
    result = t.clean_previous_results()
    assert result is None
    assert not outdir.exists()
    out, _ = capsys.readouterr()
    assert "Deleting output directory" in out


def test_twister_clean_previous_results_rename_to_dot_n(outdir, options, capsys):
    """When outdir exists and not clobber, rename to outdir.1."""
    options.clobber_output = None
    t = Twister(options, options)
    outdir.mkdir()
    result = t.clean_previous_results()
    assert result is None
    assert not outdir.exists()
    assert (outdir.parent / f"{outdir.name}.1").exists()
    out, _ = capsys.readouterr()
    assert "Renaming previous output directory" in out


def test_twister_clean_previous_results_last_metrics_no_file(outdir, options):
    """When last_metrics is True and twister.json missing, sys.exit is raised (caught by run())."""
    options.last_metrics = True
    t = Twister(options, options)

    with pytest.raises(SystemExit, match="Can't compare metrics with non existing file"):
        t.clean_previous_results()


def test_twister_clean_previous_results_last_metrics_with_file(outdir, options):
    """When last_metrics is True and twister.json exists, return its content."""
    outdir.mkdir()
    json_path = outdir / "twister.json"
    json_path.write_text('{"foo": 1}')
    options.last_metrics = True

    t = Twister(options, options)
    result = t.clean_previous_results()
    assert result == '{"foo": 1}'


def test_twister_export_previous_results_with_baseline(outdir, options):
    """export_previous_results with last_metrics and previous_results writes baseline.json."""
    outdir.mkdir()
    options.last_metrics = True
    t = Twister(options, options)
    prev = '{"metrics": 1}'
    path = t.export_previous_results(prev)
    assert path == str(outdir / "baseline.json")
    assert (outdir / "baseline.json").read_text() == prev


def test_twister_export_previous_results_last_metrics_false(options):
    """export_previous_results with last_metrics False returns None even if previous_results set."""
    options.last_metrics = False
    t = Twister(options, options)
    path = t.export_previous_results("something")
    assert path is None
