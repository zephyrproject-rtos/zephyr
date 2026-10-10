#!/usr/bin/env python3
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Normalize include guards for Zephyr public headers.

This script walks every ``*.h`` file under ``include/zephyr/`` (recursively) and
ensures each one is protected by an include guard macro named::

    ZEPHYR_INCLUDE_<xxx>

where ``<xxx>`` reflects the header path relative to ``include/zephyr/``,
converted to uppercase with every non-alphanumeric character replaced by a
single underscore ``_``.  A trailing underscore is appended to match the
existing Zephyr convention (e.g. ``ZEPHYR_INCLUDE_DRIVERS_I2C_H_`` for
``include/zephyr/drivers/i2c.h``).

For each header the script will:

* rename an existing ``#ifndef`` / ``#define`` / ``#endif`` guard whose name
  does not match the expected one;
* convert a ``#pragma once`` into a classic guard;
* add a guard to a header that has none.

A few more requirements:
* a guard located after leading #include directives is updated where it sits;
* a guard defined as "#define <NAME> 1" keeps its name but is normalized;
* a "#if !defined(<NAME>)" opener is normalized to "#ifndef <NAME>";
* a guard whose name already matches but ends with no trailing '_' or with two
  trailing '_' characters is left unmodified;
* don't modify header files that already have a matches guard even if not placed
  as the first meaningful directive in the file (possibly after heading inline comments);
* this script can only operate on files located in include/zephyr/.

By default the script performs a dry run and only reports what it would change.
Pass ``--apply`` (or ``-w``) to write the modifications back to disk.
'''

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# A preprocessor identifier line, e.g. "#ifndef  FOO_H_" or "# define FOO_H_".
IFNDEF_RE = re.compile(r"^\s*#\s*ifndef\s+([A-Za-z_]\w*)\s*$")
# An equivalent guard opener: "#if !defined(FOO_H_)" (optional spaces allowed).
IF_NOT_DEFINED_RE = re.compile(r"^\s*#\s*if\s+!\s*defined\s*\(\s*([A-Za-z_]\w*)\s*\)\s*$")
# The guard's #define, optionally with a "1" value, e.g. "#define FOO_H_ 1".
DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)(\s+1)?\s*$")
ENDIF_RE = re.compile(r"^\s*#\s*endif\b")
PRAGMA_ONCE_RE = re.compile(r"^\s*#\s*pragma\s+once\s*$")
# An #include directive, which may legitimately precede the include guard.
INCLUDE_RE = re.compile(r"^\s*#\s*include\b")


def expected_guard(header: Path, include_root: Path) -> str:
    """Return the canonical guard macro for *header* relative to *include_root*.

    ``include_root`` is expected to be the ``include/zephyr`` directory.  The
    resulting macro is ``ZEPHYR_INCLUDE_`` followed by the relative path with
    every character that is not alphanumeric or an underscore replaced by a
    single ``_`` (existing underscores are preserved, not collapsed) plus a
    trailing ``_``.  For example ``sys/__assert.h`` yields
    ``ZEPHYR_INCLUDE_SYS___ASSERT_H_``.
    """
    rel = header.relative_to(include_root)
    token = re.sub(r"[^0-9A-Za-z_]", "_", rel.as_posix()).upper()
    return f"ZEPHYR_INCLUDE_{token}_"


def _is_comment_or_blank(line: str, in_block_comment: bool) -> tuple[bool, bool]:
    """Classify *line* as part of the leading comment/blank region.

    Returns a tuple ``(is_header_region, in_block_comment)`` where
    ``is_header_region`` is True while the line is blank, a ``//`` comment, or
    inside / part of a ``/* ... */`` block comment.
    """
    stripped = line.strip()
    if in_block_comment:
        if "*/" in stripped:
            return True, False
        return True, True
    if stripped == "":
        return True, False
    if stripped.startswith("//"):
        return True, False
    if stripped.startswith("/*"):
        # Single-line block comment closes on the same line.
        if "*/" in stripped[2:]:
            return True, False
        return True, True
    return False, False


def find_leading_region_end(lines: list[str]) -> int:
    """Return the index where the include guard should be inserted.

    Walks the leading run of blank lines and comments (the file "heading",
    typically the SPDX/license block).  A comment block that contains a Doxygen
    ``@brief`` tag is treated as documentation for the file's contents rather
    than as part of the heading, so the guard is inserted *before* such a block
    instead of after it.
    """
    idx = 0
    n = len(lines)
    while idx < n:
        stripped = lines[idx].strip()

        if stripped == "":
            idx += 1
            continue

        if stripped.startswith("//"):
            if "@brief" in stripped:
                return idx
            idx += 1
            continue

        if stripped.startswith("/*"):
            block_start = idx
            has_brief = "@brief" in lines[idx]
            # Advance to the end of the block comment (it may be single-line).
            while "*/" not in lines[idx]:
                idx += 1
                if idx >= n:
                    break
                has_brief = has_brief or "@brief" in lines[idx]
            if has_brief:
                return block_start
            idx += 1
            continue

        # First real code/directive ends the heading region.
        return idx

    return n


def detect_guard(lines: list[str]) -> tuple[int, int, str, str] | None:
    """Detect a classic include guard.

    Returns ``(open_idx, define_idx, name, is_ifndef)`` or ``None`` if no guard
    is found.  ``is_ifndef`` is True for ``#ifndef NAME`` openers and False for
    the equivalent ``#if !defined(NAME)`` form.  The ``#define`` must follow the
    opener (ignoring blank and comment lines) and use the same macro name.

    Some headers place the guard *after* one or more ``#include`` directives.
    Such leading ``#include`` lines are skipped so the guard is still detected
    (and later renamed in place) rather than treated as missing.
    """
    in_block = False
    for idx, line in enumerate(lines):
        is_header, in_block = _is_comment_or_blank(line, in_block)
        if is_header:
            continue
        # An #include may legitimately precede the guard; keep looking.
        if INCLUDE_RE.match(line):
            continue
        m = IFNDEF_RE.match(line)
        is_ifndef = m is not None
        if not m:
            m = IF_NOT_DEFINED_RE.match(line)
        if not m:
            # First real code/directive is not an include guard opener.
            return None
        name = m.group(1)
        # Find the next meaningful line; it must be "#define <name>".
        in_block2 = False
        for jdx in range(idx + 1, len(lines)):
            is_h, in_block2 = _is_comment_or_blank(lines[jdx], in_block2)
            if is_h:
                continue
            d = DEFINE_RE.match(lines[jdx])
            if d and d.group(1) == name:
                return idx, jdx, name, is_ifndef
            return None
        return None
    return None


def find_last_endif(lines: list[str]) -> int | None:
    """Return the index of the final ``#endif`` line, or None."""
    for idx in range(len(lines) - 1, -1, -1):
        if ENDIF_RE.match(lines[idx]):
            return idx
    return None


def _eol(lines: list[str]) -> str:
    """Best-effort detection of the dominant line ending."""
    for line in lines:
        if line.endswith("\r\n"):
            return "\r\n"
        if line.endswith("\n"):
            return "\n"
    return "\n"


def find_include_root(path: Path) -> Path | None:
    """Return the ``include/zephyr`` directory that *path* is or lives under.

    Guard names are derived from the header path relative to ``include/zephyr``,
    so that root must be found regardless of whether *path* points at a single
    header, at ``include/zephyr`` itself, or at a sub-directory such as
    ``include/zephyr/console``.  The path itself and all of its ancestors are
    examined; ``None`` is returned when *path* is not located under an
    ``include/zephyr`` directory (the only tree this script applies to).
    """
    resolved = path.resolve()
    for candidate in (resolved, *resolved.parents):
        if candidate.name == "zephyr" and candidate.parent.name == "include":
            return candidate
    return None


def _name_acceptable(name: str, guard: str) -> bool:
    """True if *name* is the canonical *guard* or a tolerated variant of it.

    A guard already matching the expected name is left untouched even when it
    ends with no trailing underscore or with a doubled trailing underscore
    (e.g. ``ZEPHYR_INCLUDE_FOO_H`` or ``ZEPHYR_INCLUDE_FOO_H__``).
    """
    guard_core = guard.rstrip("_")
    return name in (guard, guard_core, f"{guard_core}__")


def has_matching_guard(lines: list[str], guard: str) -> bool:
    """True if an acceptable guard already exists anywhere in *lines*.

    ``detect_guard`` only recognizes a guard placed at the top of the file
    (after the heading comment and any leading ``#include``s).  Some headers
    place the guard deeper inside the file; this scans the whole file for an
    ``#ifndef NAME`` / ``#if !defined(NAME)`` opener immediately followed by a
    matching ``#define NAME`` whose name is acceptable, so such a guard is left
    untouched instead of getting a duplicate added.
    """
    for idx, line in enumerate(lines):
        m = IFNDEF_RE.match(line) or IF_NOT_DEFINED_RE.match(line)
        if not m or not _name_acceptable(m.group(1), guard):
            continue
        name = m.group(1)
        in_block = False
        for jdx in range(idx + 1, len(lines)):
            is_h, in_block = _is_comment_or_blank(lines[jdx], in_block)
            if is_h:
                continue
            d = DEFINE_RE.match(lines[jdx])
            return bool(d and d.group(1) == name)
    return False


def process(text: str, guard: str) -> tuple[str, str]:
    """Return ``(new_text, action)`` applying *guard* to header *text*.

    ``action`` is a short human-readable description of what (if anything)
    changed: ``"ok"``, ``"renamed"``, ``"pragma"``, or ``"added"``.
    """
    lines = text.splitlines(keepends=True)
    eol = _eol(lines)

    def close_guard(name: str) -> None:
        # Ensure a trailing newline, then append the closing #endif.
        if lines and not lines[-1].endswith(("\n", "\r")):
            lines[-1] += eol
        lines.append(f"{eol}#endif /* {name} */{eol}")

    existing = detect_guard(lines)
    if existing is not None:
        open_idx, define_idx, name, is_ifndef = existing
        # Accept the canonical name, and tolerate a guard that matches it but
        # ends with no trailing underscore or a doubled trailing underscore
        # (e.g. ZEPHYR_INCLUDE_FOO_H or ZEPHYR_INCLUDE_FOO_H__); preserve those.
        name_ok = _name_acceptable(name, guard)
        # The opener is always normalized to "#ifndef <NAME>", even when the
        # header used the "#if !defined(<NAME>)" form.  Nothing changes only if
        # the name is already acceptable and the opener is already "#ifndef".
        if name_ok and is_ifndef:
            return text, "ok"
        # Keep an already-acceptable name; otherwise switch to the canonical one.
        new_name = name if name_ok else guard
        lines[open_idx] = f"#ifndef {new_name}{eol}"
        lines[define_idx] = f"#define {new_name}{eol}"
        endif_idx = find_last_endif(lines)
        if endif_idx is not None:
            lines[endif_idx] = f"#endif /* {new_name} */{eol}"
        return "".join(lines), "renamed"

    # No guard at the top of the file, but an acceptable one may already exist
    # deeper inside; if so, leave the file untouched rather than duplicating it.
    if has_matching_guard(lines, guard):
        return text, "ok"

    # No classic guard. Convert a "#pragma once" if present.
    for idx, line in enumerate(lines):
        if PRAGMA_ONCE_RE.match(line):
            lines[idx] = f"#ifndef {guard}{eol}#define {guard}{eol}"
            close_guard(guard)
            return "".join(lines), "pragma"

    # No guard at all: insert one after the leading comment/blank region.
    insert_at = find_leading_region_end(lines)
    lines.insert(insert_at, f"#ifndef {guard}{eol}#define {guard}{eol}{eol}")
    close_guard(guard)
    return "".join(lines), "added"


def main() -> int:
    parser = argparse.ArgumentParser(
        description='''
    Normalize include guards for Zephyr public headers in include/zephyr.
    Walk every ``*.h`` file under include/zephyr/ and ensures each one is
    protected by an include guard which name reflects the header file path,
    starting by conventional ZEPHYR_INCLUDE_ prefix.''',
        allow_abbrev=False,
    )

    parser.add_argument(
        'path',
        nargs="*",
        default=None,
        metavar="PATH",
        help="""
        A list of paths under include/zephyr/: the include/zephyr directory
        itself, its sub-directoreis and/or *.h header files. Default to
        <repo>/include/zephyr relative to this script if no path provided.""",
    )
    parser.add_argument(
        '-w', '--apply', action='store_true', help='Write changes to disk (default is a dry run).'
    )
    parser.add_argument(
        '-v', '--verbose', action='store_true', help='List every header, including unchanged ones.'
    )
    args = parser.parse_args()

    target_list = []
    if args.path is None:
        # scripts/zephyr_header_guards.py -> repo root is the parent of scripts/.
        target_list = Path(__file__).resolve().parent.parent / "include" / "zephyr"
    else:
        target_list = args.path

    total = 0
    counts = {"ok": 0, "renamed": 0, "pragma": 0, "added": 0, "error": 0}
    for raw in target_list:
        target = Path(raw).resolve()

        if not target.exists():
            print(f"error: no such file or directory: {target}", file=sys.stderr)
            return 2

        if target.suffix not in ".h":
            if args.verbose:
                print(f"SKIPPED: not a header file: {target}")
            continue

        # The convention only applies under include/zephyr/, and the guard name
        # always reflects the full header path relative to that root.  Locate it
        # (works for a single file, the root itself, or any sub-directory) and
        # reject anything outside of include/zephyr/.
        include_root = find_include_root(target)
        if include_root is None:
            if args.verbose:
                print(f"SKIPPED: path is not under include/zephyr/: {target}", file=sys.stderr)
            continue

        if target.is_dir():
            headers = sorted(target.rglob("*.h"))
        else:
            headers = [target]

        for header in headers:
            guard = expected_guard(header, include_root)
            try:
                original = header.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                print(f"SKIP (non-utf8): {header}", file=sys.stderr)
                counts["error"] += 1
                continue

            new_text, action = process(original, guard)
            counts[action] += 1

            rel = header.relative_to(include_root)
            if action != "ok":
                print(f"{action.upper():8} {rel}  ->  {guard}")
                if args.apply and new_text != original:
                    header.write_text(new_text, encoding="utf-8")
            elif args.verbose:
                print(f"{'OK':8} {rel}")

        total += len(headers)

    print(
        f"\n{total} header(s): "
        f"{counts['ok']} ok, {counts['renamed']} renamed, "
        f"{counts['pragma']} pragma-converted, {counts['added']} added, "
        f"{counts['error']} skipped."
    )
    if not args.apply:
        changed = counts["renamed"] + counts["pragma"] + counts["added"]
        if changed:
            print("Dry run: re-run with --apply (-w) to write these changes.")
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
