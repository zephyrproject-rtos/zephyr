#!/usr/bin/env python3
# Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

"""Provide TF-M's Python tools to a TF-M build without installing them.

TF-M's build invokes its Python helpers in two ways: as console scripts by bare
name (mcuboot_imagesign_wrapper, hex_generation, ...) and as plain scripts that
import its packages (tfm_tools, bl1, bl2, ...).  Both normally require TF-M's
Python package to be installed, which a Zephyr board build must not do.

Both are satisfied here from the build directory instead, driven by TF-M's own
pyproject.toml so this does not drift when TF-M changes:

  - the packages are made importable through a directory of links, needed
    because [tool.setuptools.package-dir] maps package names onto directories
    with different names (tfm_tools -> tools/modules);
  - every entry in [project.scripts] gets a small wrapper that sets PYTHONPATH
    to that directory and calls the entry point.

The tools' own runtime dependencies (imgtool, click, cbor2) still come from the
environment, exactly as they do for any other board that builds TF-M.
"""

import argparse
import os
import stat
import sys
import tomllib

WRAPPER = """#!/bin/sh
# Generated for the Corstone-1000 TF-M build: do not edit.
PYTHONPATH="{pythonpath}${{PYTHONPATH:+:$PYTHONPATH}}" \\
  exec "{python}" {args} "$@"
"""


def write_script(path, text):
    with open(path, "w") as f:
        f.write(text)
    os.chmod(path, os.stat(path).st_mode | stat.S_IXUSR | stat.S_IXGRP |
             stat.S_IXOTH)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--tfm-dir", required=True)
    p.add_argument("--pythonpath-dir", required=True)
    p.add_argument("--scripts-dir", required=True)
    p.add_argument("--python", required=True)
    args = p.parse_args()

    pyproject = os.path.join(args.tfm_dir, "pyproject.toml")
    if not os.path.exists(pyproject):
        sys.exit(f"{pyproject} not found; cannot determine TF-M's Python tools")
    with open(pyproject, "rb") as f:
        cfg = tomllib.load(f)

    package_dir = cfg["tool"]["setuptools"]["package-dir"]
    scripts = cfg["project"]["scripts"]

    os.makedirs(args.pythonpath_dir, exist_ok=True)
    os.makedirs(args.scripts_dir, exist_ok=True)

    for package, subdir in package_dir.items():
        target = os.path.join(args.tfm_dir, subdir)
        if not os.path.isdir(target):
            sys.exit(f"TF-M package directory '{subdir}' is missing")
        link = os.path.join(args.pythonpath_dir, package)
        if os.path.islink(link) or os.path.exists(link):
            os.remove(link)
        os.symlink(target, link)

    for script, entry in scripts.items():
        module, _, func = entry.partition(":")
        code = (f"import sys; from {module} import {func}; "
                f"sys.exit({func}())")
        write_script(
            os.path.join(args.scripts_dir, script),
            WRAPPER.format(pythonpath=args.pythonpath_dir,
                           python=args.python,
                           args=f"-c '{code}'"))

    # Interpreter wrapper.  TF-M's own rules run some helpers as
    #   cmake -E env PYTHONPATH=<tfm>/tools/modules python3 <script>
    # which points PYTHONPATH inside the tfm_tools package rather than at its
    # parent, so their "import tfm_tools.foo" only resolves from an installed
    # copy.  Prepending our directory to whatever PYTHONPATH TF-M sets fixes
    # those imports too.
    write_script(
        os.path.join(args.scripts_dir, "python3-tfm"),
        WRAPPER.format(pythonpath=args.pythonpath_dir,
                       python=args.python, args=""))

    print(f"TF-M Python tools: {len(scripts)} entry points, "
          f"{len(package_dir)} packages, no installation")


if __name__ == "__main__":
    main()
