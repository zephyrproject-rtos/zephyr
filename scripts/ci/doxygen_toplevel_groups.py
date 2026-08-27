#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Doxygen Top-level Group Check

A Doxygen group that no other group pulls in with @ingroup or @addtogroup ends up at the root of
the API documentation index. Those top-level categories are meant to stay few and stable, so this
script compares them against an allowlist and fails on any addition, or on an allowlisted group
that is no longer top level.

The set is read from Doxygen's XML, not from the sources: a group's parent may be declared in
another file, or via @addtogroup nested inside a third group, and Doxygen merges all such
declarations. The rule below is the one doc/_extensions/zephyr/api_overview.py uses to render the
index -- a group is top-level if no other group lists it as an <innergroup>.
"""

import argparse
import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import NamedTuple

ZEPHYR_BASE = Path(__file__).resolve().parents[2]
ALLOWLIST = ZEPHYR_BASE / "doc" / "_doxygen" / "toplevel_groups.txt"


class Problem(NamedTuple):
    """A finding to report, optionally anchored to a line of source.

    `summary` names the kind of problem, `message` the group it applies to, and `remedy` how to
    resolve it. Both surfaces announce the kind already -- an annotation through its title, the job
    summary through a heading -- so `message` need not restate it, and the summary can group
    findings under that heading and state `remedy` once instead of on every line.

    An annotation always covers a single group, so `summary` is singular; the summary heading spans
    every finding of a kind and so needs `summary_plural`, which cannot be derived by suffixing an
    "s" ("entry" -> "entries").

    `message` and `remedy` are markdown: names and Doxygen commands are wrapped in backticks so
    that the summary renders them as code rather than, say, turning @ingroup into a user mention.
    """

    summary: str
    summary_plural: str
    message: str
    remedy: str
    file: str | None = None
    line: int | None = None


def quantify(count: int, singular: str, plural: str) -> str:
    """Render `count` with its noun agreeing in number."""
    return f"{count} {singular if count == 1 else plural}"


def repo_relative(path: str) -> str | None:
    """Map a path recorded by Doxygen onto one relative to ZEPHYR_BASE.

    Doxygen writes absolute paths from the machine it ran on, which need not share a prefix with
    this checkout, so fall back to the longest trailing portion naming an existing file. Drop the
    anchor first: joining an absolute path onto ZEPHYR_BASE discards ZEPHYR_BASE, which would let
    a file outside the repository match.
    """
    candidate = Path(path)
    try:
        return str(candidate.relative_to(ZEPHYR_BASE))
    except ValueError:
        pass

    parts = candidate.parts[1:] if candidate.is_absolute() else candidate.parts
    for i in range(len(parts)):
        suffix = Path(*parts[i:])
        if (ZEPHYR_BASE / suffix).exists():
            return str(suffix)
    return None


def declaration_of(name: str, source: str | None) -> tuple[str | None, int | None]:
    """Locate the @defgroup declaring `name`, as a (path relative to ZEPHYR_BASE, line) pair."""
    cmd = ["git", "grep", "-n", "-E", rf"@defgroup[[:space:]]+{re.escape(name)}([[:space:]]|$)"]
    if source:
        cmd += ["--", source]

    try:
        out = subprocess.run(
            cmd, cwd=ZEPHYR_BASE, capture_output=True, text=True, check=False
        ).stdout
    except OSError:
        return None, None

    for line in out.splitlines():
        path, _, rest = line.partition(":")
        lineno, _, _ = rest.partition(":")
        if lineno.isdigit():
            return path, int(lineno)
    return None, None


def group_source(compounddef: ET.Element) -> str | None:
    """Path of the file Doxygen read a group from, relative to ZEPHYR_BASE, if it can be known.

    A group carries no location of its own, but its members do.
    """
    location = compounddef.find("./sectiondef/memberdef/location")
    if location is None or not location.get("file"):
        return None
    return repo_relative(location.get("file"))


def toplevel_groups(xml_dir: Path) -> dict[str, tuple[str, ET.Element]]:
    """Return {group name: (title, compounddef)} for every top-level group in a Doxygen XML dir."""
    index = ET.parse(xml_dir / "index.xml").getroot()
    refids = {
        compound.get("refid"): compound.find("name").text
        for compound in index
        if compound.get("kind") == "group"
    }

    nested: set[str] = set()
    groups: dict[str, tuple[str, ET.Element]] = {}
    for refid in refids:
        compounddef = ET.parse(xml_dir / f"{refid}.xml").getroot().find("compounddef")
        nested.update(inner.get("refid") for inner in compounddef.findall("innergroup"))
        title = compounddef.find("title")
        groups[refid] = (title.text if title is not None else "", compounddef)

    return {refids[refid]: groups[refid] for refid in refids if refid not in nested}


def read_allowlist(path: Path) -> set[str]:
    """Read the allowlist, one group name per line. '#' starts a comment."""
    lines = path.read_text(encoding="utf-8").splitlines()
    return {name for line in lines if (name := line.split("#", 1)[0].strip())}


def find_problems(xml_dir: Path) -> list[Problem]:
    """Compare the top-level groups against the allowlist."""
    toplevel = toplevel_groups(xml_dir)
    allowed = read_allowlist(ALLOWLIST)

    problems = []
    for name, (title, compounddef) in sorted(toplevel.items()):
        if name in allowed:
            continue
        file, line = declaration_of(name, group_source(compounddef))
        problems.append(
            Problem(
                summary="New top-level Doxygen group",
                summary_plural="New top-level Doxygen groups",
                message=f'`{name}` ("{title}")',
                remedy=(
                    "Give the group a parent with `@ingroup` so that it nests under an existing "
                    "category. Where a new top-level category really is intended, allowlist the "
                    "group in `doc/_doxygen/toplevel_groups.txt`; that requires approval from the "
                    "documentation maintainers."
                ),
                file=file,
                line=line,
            )
        )

    for name in sorted(allowed - set(toplevel)):
        problems.append(
            Problem(
                summary="Stale top-level group allowlist entry",
                summary_plural="Stale top-level group allowlist entries",
                message=f"`{name}`",
                remedy=(
                    "The group now has a parent, or no longer exists, so its line in "
                    "`doc/_doxygen/toplevel_groups.txt` is obsolete. Delete the line to keep the "
                    "allowlist in step with the API index."
                ),
            )
        )

    return problems


def annotate(problems: list[Problem]) -> None:
    """Print each problem as a GitHub Actions error annotation.

    Each annotation is read on its own, away from the job summary, so it carries its remedy too.
    Annotations render as plain text, hence stripping the markdown backticks.
    """
    for problem in problems:
        location = f"file={problem.file},line={problem.line}," if problem.file else ""
        text = f"{problem.message}: {problem.remedy}".replace("`", "")
        print(f"::error {location}title={problem.summary}::{text}")


def write_summary(problems: list[Problem], summary_file: str) -> None:
    """Append a markdown summary of the problems to `summary_file`."""
    try:
        with open(summary_file, "a", encoding="utf-8") as f:
            f.write("\n## 🗂️ Doxygen Top-level Group Check Results\n\n")

            if not problems:
                f.write("✅ **No new top-level Doxygen groups!**\n")
                return

            count = quantify(len(problems), "problem", "problems")
            f.write(f"> [!CAUTION]\n> **{count} with the top-level API documentation index.**\n\n")

            kinds: dict[str, list[Problem]] = {}
            for problem in problems:
                kinds.setdefault(problem.summary, []).append(problem)

            for found in kinds.values():
                heading = found[0].summary if len(found) == 1 else found[0].summary_plural
                f.write(f"### {heading}\n\n{found[0].remedy}\n\n")
                for problem in found:
                    location = f" — `{problem.file}:{problem.line}`" if problem.file else ""
                    f.write(f"- {problem.message}{location}\n")
                f.write("\n")

            f.write(
                "See the project's [Doxygen style guidelines]"
                "(https://docs.zephyrproject.org/latest/contribute/style/doxygen.html).\n"
            )
    except OSError as e:
        print(f"::warning::Failed to write summary file '{summary_file}': {e}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument("--xml-dir", required=True, help="Path to the Doxygen XML output directory")
    args = parser.parse_args()

    xml_dir = Path(args.xml_dir)
    if not (xml_dir / "index.xml").exists():
        print(f"::error::Doxygen XML index not found under '{xml_dir}'. Was Doxygen run?")
        return 1
    if not ALLOWLIST.exists():
        print(f"::error::Allowlist not found: {ALLOWLIST}")
        return 1

    try:
        problems = find_problems(xml_dir)
    except (ET.ParseError, OSError) as e:
        print(f"::error::Failed to read Doxygen XML under '{xml_dir}': {e}")
        return 1

    annotate(problems)
    if summary_file := os.environ.get("GITHUB_STEP_SUMMARY"):
        write_summary(problems, summary_file)

    if problems:
        count = quantify(len(problems), "problem", "problems")
        print(f"Found {count} with the top-level Doxygen groups.")
        return 1

    print("No new top-level Doxygen groups.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
