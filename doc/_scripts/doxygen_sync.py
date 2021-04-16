"""
Doxygen Sync Tool
#################

Utility to synchronize a new Doxygen build with a previous one. The aim of this
script is to simulate incremental Doxygen builds so that tools post-processing
Doxygen XML files can work in a more efficient manner.

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

import argparse
import filecmp
from pathlib import Path
import shutil


def doxygen_sync(outdir: Path, latest_outdir: Path) -> None:
    """Synchronize Doxygen output with a previous build.

    This function makes sure that only new, deleted or changed files are
    actually modified in the Doxygen XML output. Latest HTML content is just
    moved.

    Args:
        outdir: Doxygen output directory.
        latest_outdir: Latest Doxygen build output directory.
    """

    htmldir = outdir / "html"
    latest_htmldir = latest_outdir / "html"

    xmldir = outdir / "xml"
    latest_xmldir = latest_outdir / "xml"

    if not latest_htmldir.exists() or not latest_xmldir.exists():
        return

    outdir.mkdir(exist_ok=True)

    if htmldir.exists():
        shutil.rmtree(htmldir)
    latest_htmldir.rename(htmldir)

    if xmldir.exists():
        dcmp = filecmp.dircmp(latest_xmldir, xmldir)

        for file in dcmp.right_only:
            (Path(dcmp.right) / file).unlink()

        for file in dcmp.left_only + dcmp.diff_files:
            shutil.copy(Path(dcmp.left) / file, Path(dcmp.right) / file)

        shutil.rmtree(latest_xmldir)
    else:
        latest_xmldir.rename(xmldir)


if __name__ == "__main__":
    parser = argparse.ArgumentParser("Doxygen Sync Tool")
    parser.add_argument(
        "-o",
        dest="output",
        type=Path,
        required=True,
        help="Doxygen build output path",
    )
    parser.add_argument(
        "-l",
        dest="latest_output",
        type=Path,
        required=True,
        help="Latest Doxygen build output path",
    )

    args = parser.parse_args()
    doxygen_sync(args.output, args.latest_output)
