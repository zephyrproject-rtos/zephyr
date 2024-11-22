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
- ``doxyrunner_outdir_var``: Variable representing the Doxygen build output
  directory, as used by ``OUTPUT_DIRECTORY``. This can be useful if other
  Doxygen variables reference to the output directory.
- ``doxyrunner_fmt``: Flag to indicate if Doxyfile should be formatted.
- ``doxyrunner_fmt_vars``: Format variables dictionary (name: value).
- ``doxyrunner_fmt_pattern``: Format pattern.
- ``doxyrunner_silent``: If Doxygen output should be logged or not. Note that
  this option may not have any effect if ``QUIET`` is set to ``YES``.
"""

import filecmp
import hashlib
import re
import shlex
import shutil
import tempfile
from pathlib import Path
from subprocess import PIPE, STDOUT, Popen
from typing import Any

from sphinx.application import Sphinx
from sphinx.environment import BuildEnvironment
from sphinx.util import logging

__version__ = "0.1.0"


logger = logging.getLogger(__name__)


def hash_file(file: Path) -> str:
    """Compute the hash (SHA256) of a file in text mode.

    Args:
        file: File to be hashed.

    Returns:
        Hash.
    """

    with open(file, encoding="utf-8") as f:
        sha256 = hashlib.sha256(f.read().encode("utf-8"))

    return sha256.hexdigest()

def get_doxygen_option(doxyfile: str, option: str) -> list[str]:
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
    silent: bool,
    fmt: bool = False,
    fmt_pattern: str | None = None,
    fmt_vars: dict[str, str] | None = None,
    outdir_var: str | None = None,
) -> str:
    """Process Doxyfile.

    Notes:
        OUTPUT_DIRECTORY, WARN_FORMAT and QUIET are overridden to satisfy
        extension operation needs.

    Args:
        doxyfile: Path to the Doxyfile.
        outdir: Output directory of the Doxygen build.
        silent: If Doxygen should be run in quiet mode or not.
        fmt: If Doxyfile should be formatted.
        fmt_pattern: Format pattern.
        fmt_vars: Format variables.
        outdir_var: Variable representing output directory.

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

    content = re.sub(
        r"^\s*WARN_FORMAT\s*=.*$",
        'WARN_FORMAT="$file:$line: $text"',
        content,
        flags=re.MULTILINE,
    )

    content = re.sub(
        r"^\s*QUIET\s*=.*$",
        "QUIET=" + "YES" if silent else "NO",
        content,
        flags=re.MULTILINE,
    )

    if fmt:
        if not fmt_pattern or not fmt_vars:
            raise ValueError("Invalid formatting pattern or variables")

        if outdir_var:
            fmt_vars = fmt_vars.copy()
            fmt_vars[outdir_var] = outdir.as_posix()

        for var, value in fmt_vars.items():
            content = content.replace(fmt_pattern.format(var), value)

    return content


def doxygen_input_has_changed(env: BuildEnvironment, doxyfile: str) -> bool:
    """Check if Doxygen input files have changed.

    Args:
        env: Sphinx build environment instance.
        doxyfile: Doxyfile content.

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

    # build a set with input files hash
    cache = set()
    for file in input_files:
        path = Path(file)
        if path.is_file():
            cache.add(hash_file(path))
        else:
            for pattern in file_patterns:
                for p_file in path.glob("**/" + pattern):
                    cache.add(hash_file(p_file))

    # check if any file has changed
    if hasattr(env, "doxyrunner_cache") and env.doxyrunner_cache == cache:
        return False

    # store current state
    env.doxyrunner_cache = cache

    return True


def process_doxygen_output(line: str, silent: bool) -> None:
    """Process a line of Doxygen program output.

    This function will map Doxygen output to the Sphinx logger output. Errors
    and warnings will be converted to Sphinx errors and warnings. Other
    messages, if not silent, will be mapped to the info logger channel.

    Args:
        line: Doxygen program line.
        silent: True if regular messages should be logged, False otherwise.
    """

    m = re.match(r"(.*):(\d+): ([a-z]+): (.*)", line)
    if m:
        type = m.group(3)
        message = f"{m.group(1)}:{m.group(2)}: {m.group(4)}"
        if type == "error":
            logger.error(message)
        elif type == "warning":
            logger.warning(message)
        else:
            logger.info(message)
    elif not silent:
        logger.info(line)


def run_doxygen(doxygen: str, doxyfile: str, silent: bool = False) -> None:
    """Run Doxygen build.

    Args:
        doxygen: Path to Doxygen binary.
        doxyfile: Doxyfile content.
        silent: If Doxygen output should be logged or not.
    """

    with tempfile.NamedTemporaryFile("w", delete=False) as f_doxyfile:
        f_doxyfile.write(doxyfile)
        f_doxyfile_name = f_doxyfile.name

    p = Popen([doxygen, f_doxyfile_name], stdout=PIPE, stderr=STDOUT, encoding="utf-8")
    while True:
        line = p.stdout.readline()  # type: ignore
        if line:
            process_doxygen_output(line.rstrip(), silent)
        if p.poll() is not None:
            break

    Path(f_doxyfile_name).unlink()

    if p.returncode:
        raise OSError(f"Doxygen process returned non-zero ({p.returncode})")


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

    generate_html = get_doxygen_option(doxyfile, "GENERATE_HTML")
    if generate_html[0] == "YES":
        html_output = get_doxygen_option(doxyfile, "HTML_OUTPUT")
        if not html_output:
            raise ValueError("No HTML_OUTPUT set in Doxyfile")

        new_htmldir = new / html_output[0]
        prev_htmldir = prev / html_output[0]

        if prev_htmldir.exists():
            shutil.rmtree(prev_htmldir)
        new_htmldir.rename(prev_htmldir)

    xml_output = get_doxygen_option(doxyfile, "XML_OUTPUT")
    if not xml_output:
        raise ValueError("No XML_OUTPUT set in Doxyfile")

    new_xmldir = new / xml_output[0]
    prev_xmldir = prev / xml_output[0]

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
        app.config.doxyrunner_silent,
        app.config.doxyrunner_fmt,
        app.config.doxyrunner_fmt_pattern,
        app.config.doxyrunner_fmt_vars,
        app.config.doxyrunner_outdir_var,
    )

    logger.info("Checking if Doxygen needs to be run...")
    app.env.doxygen_input_changed = doxygen_input_has_changed(app.env, doxyfile)
    if not app.env.doxygen_input_changed:
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


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_config_value("doxyrunner_doxygen", "doxygen", "env")
    app.add_config_value("doxyrunner_doxyfile", None, "env")
    app.add_config_value("doxyrunner_outdir", None, "env")
    app.add_config_value("doxyrunner_outdir_var", None, "env")
    app.add_config_value("doxyrunner_fmt", False, "env")
    app.add_config_value("doxyrunner_fmt_vars", {}, "env")
    app.add_config_value("doxyrunner_fmt_pattern", "@{}@", "env")
    app.add_config_value("doxyrunner_silent", True, "")

    app.connect("builder-inited", doxygen_build)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
