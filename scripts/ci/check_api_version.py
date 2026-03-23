#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Verify that public API ``@version`` tags have been updated correctly during a
release cycle, using Doxygen XML as the authoritative source.

The script takes two pre-built Doxygen XML directories — one generated from the
base ref (start of the release cycle) and one from the current tree (HEAD) —
and compares the ``@version`` field of every API group (``@defgroup``) present
in both.  For groups that exist in both trees it validates that:

* The version was incremented exactly once, following SemVer rules:

  - Only fixes             → patch bump only              (X.Y.Z → X.Y.Z+1)
  - Fixes + new features   → minor bump, patch reset to 0 (X.Y.Z → X.Y+1.0)
  - Breaking changes       → major bump, minor/patch → 0  (X.Y.Z → X+1.0.0)

* No component was incremented by more than 1 in a single release cycle.

Groups present only in HEAD are treated as new APIs; those present only in the
base are ignored (removed APIs are not a versioning concern here).

Usage::

    # Build Doxygen XML for both refs first, then run:
    python3 scripts/ci/check_api_version.py \\
        --base-xml path/to/base/doxygen/xml \\
        --head-xml path/to/head/doxygen/xml

Exit code is non-zero if any error is found.
"""

import argparse
import sys
from pathlib import Path

try:
    import doxmlparser.compound as cpd
    import doxmlparser.index as idx
except ImportError:
    print(
        "ERROR: doxmlparser not found. Install it with: pip install doxmlparser",
        file=sys.stderr,
    )
    sys.exit(1)

# Doxygen XML helpers


def _get_version_from_compounddef(compounddef) -> str | None:
    """Return the ``@version`` string from a parsed compounddef, or ``None``.

    Doxygen encodes ``@version <text>`` as::

        <detaileddescription>
          <para>
            <simplesect kind="version">
              <para><text></para>
            </simplesect>
          </para>
        </detaileddescription>
    """
    dd = compounddef.get_detaileddescription()
    if dd is None:
        return None
    for para in dd.get_para():
        for sect in para.get_simplesect():
            if sect.get_kind() == "version":
                paras = sect.get_para()
                if paras:
                    return paras[0].get_valueOf_().strip() or None
    return None


def parse_api_versions(xml_dir: Path) -> dict:
    """Return ``{group_name: version_str}`` for all groups in *xml_dir*.

    Parses ``index.xml`` to enumerate ``group`` compounds, then reads each
    compound XML file to extract the ``@version`` field via
    :func:`_get_version_from_compounddef`.  Groups without a ``@version`` tag
    are omitted.

    :param xml_dir: Directory containing Doxygen XML output
                    (must contain ``index.xml``).
    :raises SystemExit: If ``index.xml`` is missing.
    """
    index_file = xml_dir / "index.xml"
    if not index_file.is_file():
        print(
            f"ERROR: {index_file} not found. "
            "Generate Doxygen XML with GENERATE_XML=YES before running this script.",
            file=sys.stderr,
        )
        sys.exit(1)

    result: dict = {}
    root = idx.parse(str(index_file), True)

    for compound in root.get_compound():
        if compound.get_kind() != "group":
            continue
        refid = compound.get_refid()
        name = compound.get_name()
        if not refid or not name:
            continue

        compound_file = xml_dir / f"{refid}.xml"
        if not compound_file.is_file():
            continue

        tree = cpd.parse(str(compound_file), True)
        for compounddef in tree.get_compounddef():
            version = _get_version_from_compounddef(compounddef)
            if version is not None:
                result[name] = version
                break  # first compounddef wins

    return result


# SemVer helpers


def _parse_semver(version: str) -> tuple | None:
    """Parse ``"X.Y.Z"`` and return ``(major, minor, patch)``, or ``None``."""
    parts = version.split(".")
    if len(parts) != 3:
        return None
    try:
        return (int(parts[0]), int(parts[1]), int(parts[2]))
    except ValueError:
        return None


# Problem container


class Problem:
    ERROR = "error"
    WARNING = "warning"

    def __init__(self, level: str, group: str, msg: str) -> None:
        self.level = level
        self.group = group
        self.msg = msg

    def __str__(self) -> str:
        return f"[{self.level.upper()}] @defgroup {self.group}: {self.msg}"


# Core checker


def check_api_versions(base_versions: dict, head_versions: dict) -> list:
    """Compare *base_versions* and *head_versions* and return problems.

    Both dicts have the shape ``{group_name: version_str}`` as returned by
    :func:`parse_api_versions`.

    :returns: List of :class:`Problem` instances (empty means all OK).
    """
    problems: list = []

    for group, head_ver_str in head_versions.items():
        head_ver = _parse_semver(head_ver_str)
        if head_ver is None:
            problems.append(
                Problem(
                    Problem.ERROR,
                    group,
                    f"@version {head_ver_str!r} is not a valid semantic version (expected X.Y.Z).",
                )
            )
            continue

        hM, hm, hp = head_ver

        if group not in base_versions:
            # New API introduced this release cycle
            if head_ver == (0, 0, 0):
                problems.append(
                    Problem(
                        Problem.ERROR,
                        group,
                        "New API has @version 0.0.0 – assign a proper initial version. "
                        "See https://docs.zephyrproject.org/latest/develop/api/api_lifecycle.html",
                    )
                )
            # New API with a real version → OK, nothing to compare against
            continue

        base_ver_str = base_versions[group]
        base_ver = _parse_semver(base_ver_str)
        if base_ver is None:
            problems.append(
                Problem(
                    Problem.WARNING,
                    group,
                    f"Base @version {base_ver_str!r} is not a valid semantic version "
                    "– skipping comparison.",
                )
            )
            continue

        bM, bm, bp = base_ver

        # No change
        if head_ver == base_ver:
            problems.append(
                Problem(
                    Problem.WARNING,
                    group,
                    f"@version not incremented (still {base_ver_str}). "
                    "Confirm no public API changes were made this release cycle. "
                    "See https://docs.zephyrproject.org/latest/develop/api/overview.html",
                )
            )
            continue

        # Regression
        if head_ver < base_ver:
            problems.append(
                Problem(
                    Problem.ERROR,
                    group,
                    f"@version regressed: {base_ver_str} → {head_ver_str}.",
                )
            )
            continue

        # Version went forward — validate SemVer bump rules
        if hM > bM:
            # Major bump: minor and patch must be reset to 0
            if hm != 0 or hp != 0:
                problems.append(
                    Problem(
                        Problem.ERROR,
                        group,
                        f"Major bump {base_ver_str} → {head_ver_str}: "
                        "minor and patch must be reset to 0.",
                    )
                )
            if hM - bM > 1:
                problems.append(
                    Problem(
                        Problem.ERROR,
                        group,
                        f"Major version may increase by at most 1 per release cycle ({bM} → {hM}).",
                    )
                )
        elif hm > bm:
            # Minor bump: patch must be reset to 0
            if hp != 0:
                problems.append(
                    Problem(
                        Problem.ERROR,
                        group,
                        f"Minor bump {base_ver_str} → {head_ver_str}: patch must be reset to 0.",
                    )
                )
            if hm - bm > 1:
                problems.append(
                    Problem(
                        Problem.ERROR,
                        group,
                        f"Minor version may increase by at most 1 per release cycle ({bm} → {hm}).",
                    )
                )
        else:
            # Patch-only bump
            if hp - bp > 1:
                problems.append(
                    Problem(
                        Problem.ERROR,
                        group,
                        f"Patch version may increase by at most 1 per release cycle ({bp} → {hp}).",
                    )
                )

    return problems


# CLI


def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    p.add_argument(
        "--base-xml",
        required=True,
        metavar="DIR",
        help="Directory containing Doxygen XML output for the base ref "
        "(start of the release cycle).",
    )
    p.add_argument(
        "--head-xml",
        required=True,
        metavar="DIR",
        help="Directory containing Doxygen XML output for the current tree.",
    )
    return p


def main(argv=None) -> int:
    args = _build_parser().parse_args(argv)
    base_xml = Path(args.base_xml)
    head_xml = Path(args.head_xml)

    base_versions = parse_api_versions(base_xml)
    head_versions = parse_api_versions(head_xml)

    problems = check_api_versions(base_versions, head_versions)
    errors = [p for p in problems if p.level == Problem.ERROR]
    warnings = [p for p in problems if p.level == Problem.WARNING]

    for p in warnings:
        print(str(p))
    for p in errors:
        print(str(p), file=sys.stderr)

    if errors:
        print(
            f"\n{len(errors)} error(s) found. Fix @version tags before tagging the release.",
            file=sys.stderr,
        )
        return 1

    print(f"API version check passed ({len(warnings)} warning(s)).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
