#!/usr/bin/env python3
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Normalize quoted includes that do not resolve to a local header.

This script inspects the ``#include`` directives of the source files passed on
the command line.  For every double-quoted directive::

    #include "SOMEPATH"

it checks whether ``SOMEPATH`` names a file that actually exists relative to the
directory containing the source file being checked.  When it does, the quoted
form is correct (it refers to a project-local header) and is left untouched.
When it does not, the directive is rewritten to use angle brackets::

    #include <SOMEPATH>

so that the header is resolved through the system include path instead.

By default the script performs a dry run and only reports what it would change.
Pass ``--apply`` (or ``-w``) to write the modifications back to disk.
'''

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# An #include directive using double quotes, e.g. '#include "sys/foo.h"'.  The
# quoted header path is captured (group "path") together with the text before
# and after it so the line can be rewritten while preserving its layout.  The
# trailing line ending, if any, is captured separately as group "eol".
QUOTED_INCLUDE_RE = re.compile(
    r'^(?P<pre>\s*#\s*include\s*)"(?P<path>[^"]*)"(?P<post>.*?)(?P<eol>\r?\n?)$'
)

# File name extensions that carry #include directives and are therefore
# processed; any other file passed on the command line is skipped.
SOURCE_SUFFIXES = (
    ".c",
    ".cpp",
    ".C",
    ".s",
    ".S",
    ".asm",
    ".h",
    ".dts",
    ".dtsi",
    ".overlay",
)


def normalize_quoted_include(text: str, source_dir: Path) -> tuple[str, list[tuple[int, str]]]:
    """Return ``(new_text, changes)`` for *text* located in *source_dir*.

    Every ``#include "SOMEPATH"`` whose ``SOMEPATH`` does not exist relative to
    *source_dir* is rewritten to ``#include <SOMEPATH>``.  ``changes`` is the
    list of ``(line_number, path)`` tuples (1-based) that were rewritten.
    """
    changes = []
    lines = text.splitlines(keepends=True)
    for idx, line in enumerate(lines):
        match = QUOTED_INCLUDE_RE.match(line)
        if not match:
            continue
        header = match.group("path")
        # A header that exists relative to the source file is a genuine local
        # include and must keep its quotes; only rewrite the others.
        if (source_dir / header).is_file():
            continue
        lines[idx] = f'{match.group("pre")}<{header}>{match.group("post")}{match.group("eol")}'
        changes.append((idx + 1, header))
    return "".join(lines), changes


def process_file(path: Path, apply: bool, verbose: bool) -> tuple[int, bool]:
    """Check *path* and optionally rewrite it.

    Returns ``(num_changes, error)`` where ``num_changes`` is the number of
    rewritten directives and ``error`` is True when the file could not be read.
    """
    try:
        original = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        print(f"SKIP (non-utf8): {path}", file=sys.stderr)
        return 0, True
    except OSError as exc:
        print(f"error: could not read {path}: {exc}", file=sys.stderr)
        return 0, True

    new_text, changes = normalize_quoted_include(original, path.parent)

    for line_number, header in changes:
        print(f'{path}:{line_number}: #include "{header}" -> #include <{header}>')

    if changes and apply and new_text != original:
        path.write_text(new_text, encoding="utf-8")
    elif not changes and verbose:
        print(f"OK    {path}")

    return len(changes), False


def main() -> int:
    parser = argparse.ArgumentParser(
        description='''
    Rewrite quoted includes that do not resolve to a local header.  For each
    ``#include "SOMEPATH"`` whose SOMEPATH does not exist relative to the source
    file's directory, replace it with ``#include <SOMEPATH>``.''',
        allow_abbrev=False,
    )

    parser.add_argument(
        'paths',
        nargs="+",
        metavar="FILE",
        help="Path(s) of one or more source files and/or directory to check.",
    )
    parser.add_argument(
        '-w', '--apply', action='store_true', help='Write changes to disk (default is a dry run).'
    )
    parser.add_argument(
        '-v',
        '--verbose',
        action='store_true',
        help='List every file checked, including unchanged ones.',
    )
    args = parser.parse_args()

    total_files = 0
    total_changes = 0
    errors = 0
    for raw in args.paths:
        target = Path(raw).resolve()

        if target.is_dir():
            files_list = []
            for p in SOURCE_SUFFIXES:
                files_list += sorted(target.rglob("*" + p))
        elif target.is_file():
            if target.suffix not in SOURCE_SUFFIXES:
                if args.verbose:
                    print(f"SKIP (unsupported extension): {target}")
                continue
            files_list = [target]
        else:
            print(f"Invalid path: {target}", file=sys.stderr)
            errors += 1
            continue

        for path in files_list:
            changes, error = process_file(path, args.apply, args.verbose)
            total_files += 1
            total_changes += changes
            errors += int(error)

    print(
        f"\n{total_files} file(s): {total_changes} directive(s) "
        f"{'rewritten' if args.apply else 'to rewrite'}, {errors} error(s)."
    )

    if errors:
        return 2
    if total_changes and not args.apply:
        print("Dry run: re-run with --apply (-w) to write these changes.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
