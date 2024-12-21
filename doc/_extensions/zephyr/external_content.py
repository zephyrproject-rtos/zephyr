"""
External content
################

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0

Introduction
============

This extension allows to import sources from directories out of the Sphinx
source directory. They are copied to the source directory before starting the
build. Note that the copy is *smart*, that is, only updated files are actually
copied. Therefore, incremental builds detect changes correctly and behave as
expected.

Paths for external content included via e.g. figure, literalinclude, etc.
are adjusted as needed.

Configuration options
=====================

- ``external_content_contents``: A list of external contents. Each entry is
  a tuple with two fields: the external base directory and a file glob pattern.
- ``external_content_directives``: A list of directives that should be analyzed
  and their paths adjusted if necessary. Defaults to ``DEFAULT_DIRECTIVES``.
- ``external_content_keep``: A list of file globs (relative to the destination
  directory) that should be kept even if they do not exist in the source
  directory. This option can be useful for auto-generated files in the
  destination directory.
"""

import filecmp
import os
import re
import shutil
import tempfile
from pathlib import Path
from typing import Any

from sphinx.application import Sphinx

__version__ = "0.1.0"


DEFAULT_DIRECTIVES = ("figure", "image", "include", "literalinclude")
"""Default directives for included content."""


def adjust_includes(
    fname: Path,
    basepath: Path,
    directives: list[str],
    encoding: str,
    dstpath: Path | None = None,
) -> None:
    """Adjust included content paths.

    Args:
        fname: File to be processed.
        basepath: Base path to be used to resolve content location.
        directives: Directives to be parsed and adjusted.
        encoding: Sources encoding.
        dstpath: Destination path for fname if its path is not the actual destination.
    """

    if fname.suffix != ".rst":
        return

    dstpath = dstpath or fname.parent

    def _adjust(m):
        directive, fpath = m.groups()

        # ignore absolute paths
        if fpath.startswith("/"):
            fpath_adj = fpath
        else:
            fpath_adj = Path(os.path.relpath(basepath / fpath, dstpath)).as_posix()

        return f".. {directive}:: {fpath_adj}"

    with open(fname, "r+", encoding=encoding) as f:
        content = f.read()
        content_adj, modified = re.subn(
            r"\.\. (" + "|".join(directives) + r")::\s*([^`\n]+)", _adjust, content
        )
        if modified:
            f.seek(0)
            f.write(content_adj)
            f.truncate()


def sync_contents(app: Sphinx) -> None:
    """Synchronize external contents.

    Args:
        app: Sphinx application instance.
    """

    srcdir = Path(app.srcdir).resolve()
    to_copy = []
    to_delete = set(f for f in srcdir.glob("**/*") if not f.is_dir())
    to_keep = set(
        f
        for k in app.config.external_content_keep
        for f in srcdir.glob(k)
        if not f.is_dir()
    )

    for content in app.config.external_content_contents:
        prefix_src, glob = content
        for src in prefix_src.glob(glob):
            if src.is_dir():
                to_copy.extend(
                    [(f, prefix_src) for f in src.glob("**/*") if not f.is_dir()]
                )
            else:
                to_copy.append((src, prefix_src))

    for entry in to_copy:
        src, prefix_src = entry
        dst = (srcdir / src.relative_to(prefix_src)).resolve()

        if dst in to_delete:
            to_delete.remove(dst)

        if not dst.parent.exists():
            dst.parent.mkdir(parents=True)

        # just copy if it does not exist
        if not dst.exists():
            shutil.copy(src, dst)
            adjust_includes(
                dst,
                src.parent,
                app.config.external_content_directives,
                app.config.source_encoding,
            )
        # if origin file is modified only copy if different
        elif src.stat().st_mtime > dst.stat().st_mtime:
            with tempfile.TemporaryDirectory() as td:
                # adjust origin includes before comparing
                src_adjusted = Path(td) / src.name
                shutil.copy(src, src_adjusted)
                adjust_includes(
                    src_adjusted,
                    src.parent,
                    app.config.external_content_directives,
                    app.config.source_encoding,
                    dstpath=dst.parent,
                )

                if not filecmp.cmp(src_adjusted, dst):
                    dst.unlink()
                    shutil.move(os.fspath(src_adjusted), os.fspath(dst))

    # remove any previously copied file not present in the origin folder,
    # excepting those marked to be kept.
    for file in to_delete - to_keep:
        file.unlink()


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_config_value("external_content_contents", [], "env")
    app.add_config_value("external_content_directives", DEFAULT_DIRECTIVES, "env")
    app.add_config_value("external_content_keep", [], "")

    app.connect("builder-inited", sync_contents)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
