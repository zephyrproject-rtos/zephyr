#!/usr/bin/env python3
"""List files whose license is not allowed and not yet documented.

Zephyr as a whole is Apache-2.0; per the project charter CC-BY-4.0 is additionally allowed for
documentation. Any other license (or CC-BY-4.0 on a non-documentation file) is a *licensing
exception* that must be described in the ``[[annotations]]`` blocks of ``REUSE.toml``.

This script uses the ``reuse`` library to resolve the actual license of every file in the tree and
reports the ones that carry a non-allowed license without such an exception, i.e. the files that
still need a ``REUSE.toml`` entry.

Run it standalone (the whole tree, or the paths given on the command line)::

    scripts/list_undocumented_licenses.py
    scripts/list_undocumented_licenses.py subsys/foo/bar.c

or import :func:`undocumented` to reuse the same logic (e.g. from the docs licensing extension or
the CI compliance check). Exit status is non-zero when anything is reported.

SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
"""

from __future__ import annotations

import argparse
import sys
from collections.abc import Iterable, Iterator
from pathlib import Path, PurePath

from license_expression import get_spdx_licensing
from reuse.global_licensing import AnnotationsItem, ReuseTOML
from reuse.project import Project

ZEPHYR_BASE = Path(__file__).resolve().parents[1]

#: License(s) that apply to the tree as a whole and never need an exception.
DEFAULT_LICENSES = frozenset({"Apache-2.0"})

#: Extra license(s) allowed without an exception, keyed by file suffix
#: (CC-BY-4.0 is documentation-only per the project charter).
EXTRA_LICENSES = {".rst": frozenset({"CC-BY-4.0"})}

#: A REUSE annotation is a documented exception only if it carries this key;
#: the licensing page is built from those annotations.
MARKER_KEY = "Zephyr-Description"

#: SPDX license database, used to tell a real license from parser noise.
_SPDX_LICENSING = get_spdx_licensing()


def is_known_license(expr: str) -> bool:
    """Return whether *expr* is a recognized SPDX license expression.

    ``reuse`` extracts every ``SPDX-License-Identifier`` fragment it finds, including ones embedded
    in tooling source (files that generate or parse SPDX tags) or in malformed comment banners.
    Those parse to junk symbols; ignoring anything SPDX does not recognize keeps the report to real
    licenses.
    """
    expr = expr.strip()
    if not expr:
        return False
    try:
        info = _SPDX_LICENSING.validate(expr)
    except Exception:
        return False
    return not info.errors and not info.invalid_symbols


def allowed_licenses(path: str | PurePath) -> frozenset[str]:
    """Return the licenses that need no exception for *path*, given its type."""
    return DEFAULT_LICENSES | EXTRA_LICENSES.get(PurePath(path).suffix, frozenset())


def documented_exceptions(project: Project) -> list[AnnotationsItem]:
    """Return the REUSE.toml annotations that document a licensing exception.

    An annotation is a documented exception when it has an explicitly set ``Zephyr-Description``
    and ``SPDX-FileComment`` justification..
    """
    toml = ReuseTOML.from_file(project.root / "REUSE.toml")
    return [item for item in toml.annotations if MARKER_KEY in item.custom_properties]


def resolved_licenses(project: Project, path: Path) -> set[str]:
    """Return the SPDX license expression(s) ``reuse`` resolves for *path*."""
    info = project.reuse_info_of(path)
    return {str(expr) for item in info for expr in item.spdx_expressions}


def undocumented(
    project: Project,
    paths: Iterable[Path | str] | None = None,
) -> Iterator[tuple[str, set[str]]]:
    """Yield ``(relative path, licenses)`` for files that still need an exception.

    A file is reported when ``reuse`` resolves a license for it that is not allowed for its type and
    no documented exception matches it. *paths* may be tree-relative or absolute; it defaults to
    every file in the tree.
    """
    exceptions = documented_exceptions(project)
    for path in project.all_files() if paths is None else paths:
        abspath = project.root / path  # an absolute *path* overrides project.root
        rel = abspath.relative_to(project.root).as_posix()
        licenses = {lic for lic in resolved_licenses(project, abspath) if is_known_license(lic)}
        extra = licenses - allowed_licenses(rel)
        if extra and not any(item.matches(rel) for item in exceptions):
            yield rel, extra


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0], allow_abbrev=False)
    parser.add_argument(
        "paths",
        nargs="*",
        type=Path,
        help="files to check (default: every file in the tree)",
    )
    args = parser.parse_args()

    project = Project.from_directory(ZEPHYR_BASE)
    paths = [p.resolve() for p in args.paths] or None

    found = False
    for rel, licenses in sorted(undocumented(project, paths)):
        print(f"{rel}: {', '.join(sorted(licenses))}")
        found = True

    if found:
        sys.stdout.flush()
        print(
            "\nThe files above carry a license other than the project defaults. Importing code"
            "under another license has prerequisites; make sure the steps in"
            "https://docs.zephyrproject.org/latest/contribute/guidelines.html#components-using-other-licenses"
            "have been followed before documenting each component as an [[annotations]] entry in"
            "REUSE.toml.",
            file=sys.stderr,
        )
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
