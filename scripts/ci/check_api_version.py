#!/usr/bin/env python3
# Copyright (c) 2025 The Zephyr Project
# SPDX-License-Identifier: Apache-2.0

"""
Verify that public API ``@version`` tags in Zephyr header files have been
updated correctly during a release cycle.

For every public header under ``include/zephyr/`` that was modified between a
base git ref and HEAD, the script checks that each ``@defgroup`` block's
``@version`` tag was incremented exactly once and follows semantic-versioning
rules:

* Only fixes           → patch bump only           (X.Y.Z → X.Y.Z+1)
* Fixes + new features → minor bump, patch → 0    (X.Y.Z → X.Y+1.0)
* Breaking changes     → major bump, minor/patch → 0  (X.Y.Z → X+1.0.0)

Usage::

    python3 scripts/ci/check_api_version.py --base v4.2.0

Exit code is non-zero if any violation is found.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Regex
# ---------------------------------------------------------------------------

# Matches "@version X.Y.Z" inside a Doxygen comment.
# Named groups use strict SemVer integers (no leading zeros except "0").
_VERSION_RE = re.compile(
    r"@version\s+"
    r"(?P<major>0|[1-9]\d*)"
    r"\."
    r"(?P<minor>0|[1-9]\d*)"
    r"\."
    r"(?P<patch>0|[1-9]\d*)"
)

# Matches "@defgroup <name> ..." – used to attribute a @version to an API group.
_DEFGROUP_RE = re.compile(r"@defgroup\s+(?P<name>\S+)")

# Public API header root (relative to Zephyr tree root)
_PUBLIC_INCLUDE = "include/zephyr"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _parse_versions(text: str, fallback_name: str) -> dict:
    """Return ``{defgroup_name: (major, minor, patch)}`` parsed from *text*.

    A ``@version`` tag is attributed to the nearest preceding ``@defgroup``
    in the same file.  If no ``@defgroup`` precedes it, *fallback_name* is
    used as the key.  First occurrence per group wins.
    """
    result: dict = {}
    current_group: str | None = None
    for line in text.splitlines():
        m = _DEFGROUP_RE.search(line)
        if m:
            current_group = m.group("name")
        m = _VERSION_RE.search(line)
        if m:
            ver = (int(m.group("major")),
                   int(m.group("minor")),
                   int(m.group("patch")))
            key = current_group if current_group else fallback_name
            result.setdefault(key, ver)
    return result


def _git_show(ref: str, rel_path: str, cwd: Path) -> str | None:
    """Return content of *rel_path* at git *ref*, or ``None`` if absent."""
    try:
        r = subprocess.run(
            ["git", "show", f"{ref}:{rel_path}"],
            capture_output=True,
            text=True,
            cwd=cwd,
        )
        return r.stdout if r.returncode == 0 else None
    except FileNotFoundError:
        return None


def _changed_public_headers(root: Path, base_ref: str, head_ref: str) -> list:
    """Return relative paths of changed public header files."""
    try:
        r = subprocess.run(
            ["git", "diff", "--name-only", f"{base_ref}..{head_ref}"],
            capture_output=True,
            text=True,
            check=True,
            cwd=root,
        )
    except subprocess.CalledProcessError as exc:
        print(f"ERROR: git diff failed: {exc.stderr.strip()}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print("ERROR: git not found in PATH.", file=sys.stderr)
        sys.exit(1)

    return [
        p for p in r.stdout.splitlines()
        if p.startswith(_PUBLIC_INCLUDE + "/") and p.endswith(".h")
    ]


# ---------------------------------------------------------------------------
# Problem container
# ---------------------------------------------------------------------------

class Problem:
    ERROR = "error"
    WARNING = "warning"

    def __init__(self, level: str, group: str, header: str, msg: str):
        self.level = level
        self.group = group
        self.header = header
        self.msg = msg

    def __str__(self) -> str:
        return f"[{self.level.upper()}] {self.header} (@defgroup {self.group}): {self.msg}"


# ---------------------------------------------------------------------------
# Core checker
# ---------------------------------------------------------------------------

def check_api_versions(root: Path, base_ref: str, head_ref: str = "HEAD") -> list:
    """Check ``@version`` tags for all changed public headers.

    Compares *base_ref* against *head_ref* (default ``HEAD``).
    Returns a list of :class:`Problem` objects.
    """
    problems: list = []
    changed = _changed_public_headers(root, base_ref, head_ref)

    if not changed:
        return problems

    for rel in changed:
        # Read both versions via git to avoid any working-tree noise
        base_text = _git_show(base_ref, rel, root)
        head_text = _git_show(head_ref, rel, root)

        if head_text is None:
            # File deleted in this cycle – nothing to version-check
            continue

        head_groups = _parse_versions(head_text, rel)

        if not head_groups:
            # Header has no @version tags – not a versioned public API
            continue

        if base_text is None:
            # Brand-new file introduced this cycle
            for grp, ver in head_groups.items():
                if ver == (0, 0, 0):
                    problems.append(Problem(
                        Problem.ERROR, grp, rel,
                        "New API has @version 0.0.0 – assign a proper initial "
                        "version (see "
                        "https://docs.zephyrproject.org/latest/develop/api/"
                        "api_lifecycle.html).",
                    ))
            continue

        base_groups = _parse_versions(base_text, rel)

        for grp, head_ver in head_groups.items():
            hM, hm, hp = head_ver

            if grp not in base_groups:
                # @defgroup / @version added mid-cycle – treat as new API
                if head_ver == (0, 0, 0):
                    problems.append(Problem(
                        Problem.ERROR, grp, rel,
                        "New API has @version 0.0.0 – assign a proper initial "
                        "version.",
                    ))
                continue

            base_ver = base_groups[grp]
            bM, bm, bp = base_ver

            # Version must have been bumped
            if head_ver == base_ver:
                problems.append(Problem(
                    Problem.ERROR, grp, rel,
                    f"@version not incremented (still {bM}.{bm}.{bp}) despite "
                    f"changes since {base_ref}. Bump the version following "
                    "https://docs.zephyrproject.org/latest/develop/api/overview.html.",
                ))
                continue

            # Version must not go backwards
            if head_ver < base_ver:
                problems.append(Problem(
                    Problem.ERROR, grp, rel,
                    f"@version regressed: {bM}.{bm}.{bp} → {hM}.{hm}.{hp}.",
                ))
                continue

            # Validate SemVer bump rules
            if hM > bM:
                # Major bump: minor and patch must be reset to 0
                if hm != 0 or hp != 0:
                    problems.append(Problem(
                        Problem.ERROR, grp, rel,
                        f"Major bump {bM}.{bm}.{bp} → {hM}.{hm}.{hp}: "
                        "minor and patch must be reset to 0.",
                    ))
                if hM - bM > 1:
                    problems.append(Problem(
                        Problem.ERROR, grp, rel,
                        f"Major version may increase by at most 1 per release "
                        f"cycle ({bM} → {hM}).",
                    ))
            elif hm > bm:
                # Minor bump: patch must be reset to 0
                if hp != 0:
                    problems.append(Problem(
                        Problem.ERROR, grp, rel,
                        f"Minor bump {bM}.{bm}.{bp} → {hM}.{hm}.{hp}: "
                        "patch must be reset to 0.",
                    ))
                if hm - bm > 1:
                    problems.append(Problem(
                        Problem.ERROR, grp, rel,
                        f"Minor version may increase by at most 1 per release "
                        f"cycle ({bm} → {hm}).",
                    ))
            else:
                # Patch bump only
                if hp - bp > 1:
                    problems.append(Problem(
                        Problem.ERROR, grp, rel,
                        f"Patch version may increase by at most 1 per release "
                        f"cycle ({bp} → {hp}).",
                    ))

    return problems


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    p.add_argument(
        "--base",
        required=True,
        metavar="REF",
        help="Git ref representing the start of the release cycle "
             "(e.g. v4.2.0 or the merge-base SHA).",
    )
    p.add_argument(
        "--head",
        default="HEAD",
        metavar="REF",
        help="Git ref to compare against (default: HEAD).",
    )
    p.add_argument(
        "--root",
        default=".",
        metavar="DIR",
        help="Root of the Zephyr source tree (default: current directory).",
    )
    return p


def main(argv=None) -> int:
    args = _build_parser().parse_args(argv)
    root = Path(args.root).resolve()

    problems = check_api_versions(root, args.base, args.head)

    errors = [p for p in problems if p.level == Problem.ERROR]
    warnings = [p for p in problems if p.level == Problem.WARNING]

    for p in warnings:
        print(str(p))
    for p in errors:
        print(str(p), file=sys.stderr)

    if errors:
        print(
            f"\n{len(errors)} error(s) found. "
            "Fix @version tags before tagging the release.",
            file=sys.stderr,
        )
        return 1

    print(f"API version check passed ({len(warnings)} warning(s)).")
    return 0


if __name__ == "__main__":
    sys.exit(main())