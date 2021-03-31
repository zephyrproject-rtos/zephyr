"""
Doxyrunner Sphinx Plugin
########################

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0

Introduction
============

This Sphinx plugin can be used to run Doxygen build as part of the Sphinx build
process. It is meant to be used with other plugins such as ``breathe`` in order
to improve the user experience. The principal features offered by this plugin
are:

- Doxygen build is run before Sphinx reads input files
- Doxyfile can be optionally pre-processed so that variables can be inserted
- Changes in the Doxygen input files are tracked so that Doxygen build is only
  run if necessary.
- Synchronizes Doxygen XML output so that even if Doxygen is run only changed,
  deleted or added files are modified.

References:

-  https://github.com/michaeljones/breathe/issues/420

Configuration options
=====================

- ``doxyrunner_doxygen``: Path to the Doxygen binary.
- ``doxyrunner_doxyfile``: Path to Doxyfile.
- ``doxyrunner_outdir``: Doxygen build output directory (inserted to
  ``OUTPUT_DIRECTORY``)
- ``doxyrunner_fmt``: Flag to indicate if Doxyfile should be formatted.
- ``doxyrunner_fmt_vars``: Format variables dictionary (name: value).
- ``doxyrunner_fmt_pattern``: Format pattern.
- ``doxyrunner_silent``: If Doxygen output should be logged or not. Note that
  this option may not have any effect if ``QUIET`` is set to ``YES``.
"""

import filecmp
from pathlib import Path
import pickle
import re
import shlex
import shutil
from subprocess import Popen, PIPE, STDOUT
import tempfile
from typing import List, Dict, Optional, Any

from sphinx.application import Sphinx
from sphinx.util import logging


__version__ = "0.1.0"


logger = logging.getLogger(__name__)


def get_doxygen_option(doxyfile: str, option: str) -> List[str]:
    """Obtain the value of a Doxygen option.

    Args:
        doxyfile: Content of the Doxyfile.
        option: Option to be retrieved.

    Notes:
        Does not support appended values.

    Returns:
        Option values.
    """

    option_re = re.compile(r"^\s*([A-Z0-9_]+)\s*=\s*(.*)$")
    multiline_re = re.compile(r"^\s*(.*)$")

    values = []
    found = False
    finished = False
    for line in doxyfile.splitlines():
        if not found:
            m = option_re.match(line)
            if not m or m.group(1) != option:
                continue

            found = True
            value = m.group(2)
        else:
            m = multiline_re.match(line)
            if not m:
                raise ValueError(f"Unexpected line content: {line}")

            value = m.group(1)

        # check if it is a multiline value
        finished = not value.endswith("\\")

        # strip backslash
        if not finished:
            value = value[:-1]

        # split values
        values += shlex.split(value.replace("\\", "\\\\"))

        if finished:
            break

    return values


def process_doxyfile(
    doxyfile: str,
    outdir: Path,
    fmt: bool = False,
    fmt_pattern: Optional[str] = None,
    fmt_vars: Optional[Dict[str, str]] = None,
) -> str:
    """Process Doxyfile.

    Args:
        doxyfile: Path to the Doxyfile.
        outdir: Output directory of the Doxygen build.
        fmt: If Doxyfile should be formatted.
        fmt_pattern: Format pattern.
        fmt_vars: Format variables.

     Returns:
        Processed Doxyfile content.
    """

    with open(doxyfile) as f:
        content = f.read()

    content = re.sub(
        r"^\s*OUTPUT_DIRECTORY\s*=.*$",
        f"OUTPUT_DIRECTORY={outdir.as_posix()}",
        content,
        flags=re.MULTILINE,
    )

    if fmt:
        if not fmt_pattern or not fmt_vars:
            raise ValueError("Invalid formatting pattern or variables")

        for var, value in fmt_vars.items():
            content = content.replace(fmt_pattern.format(var), value)

    return content


def doxygen_input_has_changed(doxyfile: str, cache_dir: Path) -> bool:
    """Check if Doxygen input files have changed.

    Args:
        doxyfile: Doxyfile content.
        cache_dir: Directory where cache file is located.

    Returns:
        True if changed, False otherwise.
    """

    # obtain Doxygen input files and patterns
    input_files = get_doxygen_option(doxyfile, "INPUT")
    if not input:
        raise ValueError("No INPUT set in Doxyfile")

    file_patterns = get_doxygen_option(doxyfile, "FILE_PATTERNS")
    if not file_patterns:
        raise ValueError("No FILE_PATTERNS set in Doxyfile")

    # build a dict of input files <-> current modification time
    files = dict()
    for file in input_files:
        path = Path(file)
        if path.is_file():
            files[path] = path.stat().st_mtime_ns
        else:
            for pattern in file_patterns:
                for p_file in path.glob("**/" + pattern):
                    files[p_file] = p_file.stat().st_mtime_ns

    # check if any file has changed
    dirty = True
    files_cache_file = cache_dir / "doxygen.cache"
    if files_cache_file.exists():
        with open(files_cache_file, "rb") as f:
            files_cache = pickle.load(f)
        dirty = files != files_cache

    if not dirty:
        return False

    # store current state
    with open(files_cache_file, "wb") as f:
        pickle.dump(files, f)

    return True


def run_doxygen(doxygen: str, doxyfile: str, silent: bool = False) -> None:
    """Run Doxygen build.

    Args:
        doxygen: Path to Doxygen binary.
        doxyfile: Doxyfile content.
        silent: If Doxygen output should be logged or not.
    """

    f_doxyfile = tempfile.NamedTemporaryFile("w", delete=False)
    f_doxyfile.write(doxyfile)
    f_doxyfile.close()

    p = Popen([doxygen, f_doxyfile.name], stdout=PIPE, stderr=STDOUT, encoding="utf-8")
    while True:
        line = p.stdout.readline()  # type: ignore
        if line and not silent:
            logger.info(line.rstrip())
        if p.poll() is not None:
            break

    Path(f_doxyfile.name).unlink()

    if p.returncode:
        raise IOError(f"Doxygen process returned non-zero ({p.returncode})")


def sync_doxygen(doxyfile: str, new: Path, prev: Path) -> None:
    """Synchronize Doxygen output with a previous build.

    This function makes sure that only new, deleted or changed files are
    actually modified in the Doxygen XML output. Latest HTML content is just
    moved.

    Args:
        doxyfile: Contents of the Doxyfile.
        new: Newest Doxygen build output directory.
        prev: Previous Doxygen build output directory.
    """

    html_output = get_doxygen_option(doxyfile, "HTML_OUTPUT")
    if not html_output:
        raise ValueError("No HTML_OUTPUT set in Doxyfile")

    xml_output = get_doxygen_option(doxyfile, "XML_OUTPUT")
    if not xml_output:
        raise ValueError("No XML_OUTPUT set in Doxyfile")

    new_htmldir = new / html_output[0]
    prev_htmldir = prev / html_output[0]

    new_xmldir = new / xml_output[0]
    prev_xmldir = prev / xml_output[0]

    if prev_htmldir.exists():
        shutil.rmtree(prev_htmldir)
    new_htmldir.rename(prev_htmldir)

    if prev_xmldir.exists():
        dcmp = filecmp.dircmp(new_xmldir, prev_xmldir)

        for file in dcmp.right_only:
            (Path(dcmp.right) / file).unlink()

        for file in dcmp.left_only + dcmp.diff_files:
            shutil.copy(Path(dcmp.left) / file, Path(dcmp.right) / file)

        shutil.rmtree(new_xmldir)
    else:
        new_xmldir.rename(prev_xmldir)


def doxygen_build(app: Sphinx) -> None:
    """Doxyrunner entry point.

    Args:
        app: Sphinx application instance.
    """

    if app.config.doxyrunner_outdir:
        outdir = Path(app.config.doxyrunner_outdir)
    else:
        outdir = Path(app.outdir) / "_doxygen"

    outdir.mkdir(exist_ok=True)
    tmp_outdir = outdir / "tmp"

    logger.info("Preparing Doxyfile...")
    doxyfile = process_doxyfile(
        app.config.doxyrunner_doxyfile,
        tmp_outdir,
        app.config.doxyrunner_fmt,
        app.config.doxyrunner_fmt_pattern,
        app.config.doxyrunner_fmt_vars,
    )

    logger.info("Checking if Doxygen needs to be run...")
    changed = doxygen_input_has_changed(doxyfile, outdir)
    if not changed:
        logger.info("Doxygen build will be skipped (no changes)!")
        return

    logger.info("Running Doxygen...")
    run_doxygen(
        app.config.doxyrunner_doxygen,
        doxyfile,
        app.config.doxyrunner_silent,
    )

    logger.info("Syncing Doxygen output...")
    sync_doxygen(doxyfile, tmp_outdir, outdir)

    shutil.rmtree(tmp_outdir)


def setup(app: Sphinx) -> Dict[str, Any]:
    app.add_config_value("doxyrunner_doxygen", "doxygen", "env")
    app.add_config_value("doxyrunner_doxyfile", None, "env")
    app.add_config_value("doxyrunner_outdir", None, "env")
    app.add_config_value("doxyrunner_fmt", False, "env")
    app.add_config_value("doxyrunner_fmt_vars", {}, "env")
    app.add_config_value("doxyrunner_fmt_pattern", "@{}@", "env")
    app.add_config_value("doxyrunner_silent", False, "")

    app.connect("builder-inited", doxygen_build)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
