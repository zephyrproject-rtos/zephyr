# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Extract used source information from an ELF file's DWARF debug info.

The DWARF line programs of a linked image tell us which source lines actually
contributed code to it.  Two views of that data are exposed:

* :func:`collect_used_lines` returns the raw ``{source path: {used line}}``
  mapping.  Its key set is the set of source files present in the final image,
  which the ``prune-sources`` analysis uses to drop sources that contributed
  nothing.
* :func:`extract_source_ranges` folds those lines into contiguous
  :class:`SourceRange` objects (with source-file byte offsets), which the
  ``snippets`` analysis emits as SPDX Snippets.

Callers may restrict results to a known set of paths via ``known_paths``.

Requires ``pyelftools`` (already listed in ``requirements-base.txt``).
"""

from __future__ import annotations

import logging
import os
from dataclasses import dataclass

_logger = logging.getLogger(__name__)


@dataclass
class SourceRange:
    """A contiguous block of used source lines within a single file.

    ``start_byte`` / ``end_byte`` are byte offsets *within the source file*
    (not within the binary), as required by the SPDX Snippet model.
    """

    path: str
    start_line: int
    end_line: int
    start_byte: int
    end_byte: int


def collect_used_lines(
    elf_path: str,
    known_paths: set[str] | None = None,
) -> dict[str, set[int]]:
    """Return the used source lines per file from an ELF's DWARF debug info.

    This is the single DWARF pass shared by the ``prune-sources`` and
    ``snippets`` analyses: the key set answers "which source files are in the
    final image?" and the line sets answer "which lines of each?".

    Args:
        elf_path: Path to the compiled ELF file (must carry DWARF debug info).
        known_paths: When provided, only source files whose ``os.path.realpath``
            is in this set are included in the result.  Pass ``None`` to collect
            all files referenced by DWARF.

    Returns:
        Dict mapping absolute (realpath) source path to the set of 1-based line
        numbers used from that file.  Returns an empty dict when the ELF has no
        DWARF info or cannot be opened.
    """
    try:
        from elftools.elf.elffile import ELFFile
    except ImportError:
        _logger.error(
            "pyelftools is required for --analyze-elf; "
            "install it with: pip install pyelftools"
        )
        return {}

    file_lines: dict[str, set[int]] = {}

    try:
        with open(elf_path, "rb") as f:
            elf = ELFFile(f)
            if not elf.has_dwarf_info():
                _logger.warning("ELF file has no DWARF debug info: %s", elf_path)
                return {}
            _collect_lines(elf.get_dwarf_info(), file_lines, known_paths)
    except OSError as exc:
        _logger.error("Cannot open ELF file %s: %s", elf_path, exc)
        return {}

    _logger.debug(
        "Collected used lines from %s: %d source files", elf_path, len(file_lines)
    )
    return file_lines


def ranges_from_line_map(file_lines: dict[str, set[int]]) -> dict[str, list[SourceRange]]:
    """Fold a ``{path: {used lines}}`` mapping into contiguous source ranges.

    Args:
        file_lines: Used lines per source file, as returned by
            :func:`collect_used_lines`.

    Returns:
        Dict mapping absolute (realpath) source path to a list of
        :class:`SourceRange` objects, sorted by ``start_line``.  Files whose
        ranges cannot be resolved (e.g. the source is unreadable) are omitted.
    """
    result: dict[str, list[SourceRange]] = {}
    for path, lines in file_lines.items():
        ranges = _lines_to_ranges(path, sorted(lines))
        if ranges:
            result[path] = ranges
    return result


def extract_source_ranges(
    elf_path: str,
    known_paths: set[str] | None = None,
) -> dict[str, list[SourceRange]]:
    """Return contiguous used-line ranges per source file from ELF DWARF debug info.

    Convenience wrapper over :func:`collect_used_lines` followed by
    :func:`ranges_from_line_map`.

    Args:
        elf_path: Path to the compiled ELF file (must carry DWARF debug info).
        known_paths: When provided, only source files whose ``os.path.realpath``
            is in this set are included in the result.  Pass ``None`` to collect
            all files referenced by DWARF.

    Returns:
        Dict mapping absolute (realpath) source path to a list of
        :class:`SourceRange` objects, sorted by ``start_line``.  Returns an
        empty dict when the ELF has no DWARF info or cannot be opened.
    """
    return ranges_from_line_map(collect_used_lines(elf_path, known_paths))


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------


def _collect_lines(
    dwarf,
    file_lines: dict[str, set[int]],
    known_paths: set[str] | None,
) -> None:
    """Walk every DWARF line program and accumulate used line numbers per file."""
    for CU in dwarf.iter_CUs():
        lp = dwarf.line_program_for_CU(CU)
        if lp is None:
            continue

        top_die = CU.get_top_DIE()
        comp_dir_attr = top_die.attributes.get("DW_AT_comp_dir")
        comp_dir = (
            comp_dir_attr.value.decode("utf-8", errors="replace") if comp_dir_attr else ""
        )

        lp_header = lp.header
        file_entries = lp_header.file_entry
        include_dirs = lp_header.include_directory

        for entry in lp.get_entries():
            state = entry.state
            if state is None or state.end_sequence or state.line == 0:
                continue
            try:
                path = _resolve_file(
                    state.file, file_entries, include_dirs, comp_dir, lp_header.version
                )
            except (IndexError, AttributeError):
                continue
            if known_paths is not None and path not in known_paths:
                continue
            file_lines.setdefault(path, set()).add(state.line)


def _resolve_file(
    file_idx: int,
    file_entries,
    include_dirs,
    comp_dir: str,
    version: int,
) -> str:
    """Resolve a DWARF file-table index to an absolute realpath.

    Up to v4 the file table is 1-based and directory 0 is implicitly the
    compilation directory; from v5 both tables are 0-based and directory 0 is
    an explicit entry holding it.
    """
    if version >= 5:
        fe = file_entries[file_idx]
    else:
        fe = file_entries[file_idx - 1]

    name = fe.name.decode("utf-8", errors="replace")
    dir_idx = fe.dir_index

    if version >= 5:
        raw = include_dirs[dir_idx].decode("utf-8", errors="replace")
        base = raw if os.path.isabs(raw) else os.path.join(comp_dir, raw)
    elif dir_idx == 0:
        base = comp_dir
    else:
        raw = include_dirs[dir_idx - 1].decode("utf-8", errors="replace")
        base = raw if os.path.isabs(raw) else os.path.join(comp_dir, raw)

    full = name if os.path.isabs(name) else os.path.join(base, name)
    return os.path.realpath(full)


def _build_line_offsets(path: str) -> dict[int, tuple[int, int]]:
    """Return ``{line_number: (start_byte, end_byte)}`` for every line in *path*.

    Line numbers are 1-based.  ``end_byte`` points to the last byte of the line
    *before* the newline.  Returns an empty dict when the file cannot be read.
    """
    try:
        with open(path, "rb") as f:
            content = f.read()
    except OSError:
        return {}

    offsets: dict[int, tuple[int, int]] = {}
    pos = 0
    for lineno, chunk in enumerate(content.split(b"\n"), start=1):
        offsets[lineno] = (pos, pos + len(chunk))
        pos += len(chunk) + 1  # +1 for the '\n' separator
    return offsets


def _lines_to_ranges(path: str, sorted_lines: list[int]) -> list[SourceRange]:
    """Convert a sorted list of used line numbers to contiguous :class:`SourceRange` objects."""
    if not sorted_lines:
        return []

    offsets = _build_line_offsets(path)
    if not offsets:
        return []

    max_line = max(offsets)
    ranges: list[SourceRange] = []
    start = sorted_lines[0]
    prev = sorted_lines[0]

    for line in sorted_lines[1:]:
        if line > prev + 1:
            r = _make_range(path, start, prev, offsets, max_line)
            if r is not None:
                ranges.append(r)
            start = line
        prev = line

    r = _make_range(path, start, prev, offsets, max_line)
    if r is not None:
        ranges.append(r)

    return ranges


def _make_range(
    path: str,
    start_line: int,
    end_line: int,
    offsets: dict[int, tuple[int, int]],
    max_line: int,
) -> SourceRange | None:
    end_line = min(end_line, max_line)
    if start_line not in offsets or end_line not in offsets:
        return None
    return SourceRange(
        path=path,
        start_line=start_line,
        end_line=end_line,
        start_byte=offsets[start_line][0],
        end_byte=offsets[end_line][1],
    )
