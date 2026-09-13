#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

"""
Check for stack-allocated kernel objects.

This script walks the DWARF debug information of the final ELF and
reports every automatic (stack-allocated) variable whose type is, or
transitively contains as a member, one of the tracked kernel object
types. Pointer members are not followed, as pointing to a kernel object
carries no lifetime requirement.
"""

import argparse
import sys
from collections.abc import Callable
from typing import NamedTuple

from elftools.dwarf.compileunit import CompileUnit
from elftools.dwarf.die import DIE
from elftools.dwarf.dwarfinfo import DWARFInfo
from elftools.elf.elffile import ELFFile

# DWARF location expression opcodes of interest
DW_OP_fbreg = 0x91

# DWARF tags, attributes and forms of interest
_TAG_WRAPPER_TYPES = ("DW_TAG_typedef", "DW_TAG_const_type", "DW_TAG_volatile_type")
_TAG_COMPOSITE_TYPES = ("DW_TAG_structure_type", "DW_TAG_union_type")
_TAG_ARRAY_TYPE = "DW_TAG_array_type"
_TAG_MEMBER = "DW_TAG_member"
_TAG_SUBPROGRAM = "DW_TAG_subprogram"
_TAG_LEXICAL_BLOCK = "DW_TAG_lexical_block"
_TAG_SCOPES = (_TAG_SUBPROGRAM, "DW_TAG_inlined_subroutine", _TAG_LEXICAL_BLOCK)
_TAG_DATA_OBJECTS = ("DW_TAG_variable", "DW_TAG_formal_parameter")

_AT_TYPE = "DW_AT_type"
_AT_NAME = "DW_AT_name"
_AT_LOCATION = "DW_AT_location"
_AT_DECL_FILE = "DW_AT_decl_file"
_AT_DECL_LINE = "DW_AT_decl_line"
_AT_ORIGINS = ("DW_AT_abstract_origin", "DW_AT_specification")

_FORM_LOCATION_EXPRS = ("DW_FORM_exprloc", "DW_FORM_block1")

# Encoding of DWARF strings
_ENCODING = "utf-8"

# Placeholders for missing debug information
_ANONYMOUS = "<anonymous>"
_UNKNOWN = "<unknown>"

# Kernel object types integrated with the object core framework
# (CONFIG_OBJ_CORE_*) or the object tracking lists
# (CONFIG_TRACING_OBJECT_TRACKING). Instances of these must not live on a
# thread's stack while a tracking facility is enabled.
TRACKED_TYPES = {
    "k_condvar",
    "k_event",
    "k_fifo",
    "k_lifo",
    "k_mbox",
    "k_mem_slab",
    "k_msgq",
    "k_mutex",
    "k_pipe",
    "k_queue",
    "k_sem",
    "k_stack",
    "k_thread",
    "k_timer",
}


class Finding(NamedTuple):
    """A stack-allocated kernel object found in the debug information."""

    path: str
    line: int
    function: str
    variable: str
    type_name: str


class TypeClassifier:
    """Classify DWARF type DIEs as (containing) tracked kernel objects.

    The result of classify() is a list of (member path, type name)
    tuples, one per tracked kernel object reachable from the type
    without following pointers.
    """

    def __init__(self) -> None:
        self._cache: dict[int, list[tuple[str, str]]] = {}

    def _resolve(self, die: DIE) -> DIE | None:
        """Peel typedefs and cv-qualifiers off a type DIE."""
        seen = set()
        while die is not None and die.tag in _TAG_WRAPPER_TYPES:
            if die.offset in seen or _AT_TYPE not in die.attributes:
                return None
            seen.add(die.offset)
            die = die.get_DIE_from_attribute(_AT_TYPE)
        return die

    def classify(self, die: DIE) -> list[tuple[str, str]]:
        resolved = self._resolve(die)
        if resolved is None:
            return []

        die = resolved

        if die.offset in self._cache:
            return self._cache[die.offset]

        # Break recursion cycles: assume no hit while resolving
        self._cache[die.offset] = []
        hits = []

        if die.tag in _TAG_COMPOSITE_TYPES:
            name = die.attributes.get(_AT_NAME)
            if name is not None and name.value.decode(_ENCODING) in TRACKED_TYPES:
                hits.append(("", name.value.decode(_ENCODING)))
            else:
                for child in die.iter_children():
                    if child.tag != _TAG_MEMBER:
                        continue
                    if _AT_TYPE not in child.attributes:
                        continue
                    member = child.attributes.get(_AT_NAME)
                    member_name = (
                        member.value.decode(_ENCODING) if member is not None else _ANONYMOUS
                    )
                    for path, type_name in self.classify(child.get_DIE_from_attribute(_AT_TYPE)):
                        sep = "." if path else ""
                        hits.append((f"{member_name}{sep}{path}", type_name))
        elif die.tag == _TAG_ARRAY_TYPE:
            if _AT_TYPE in die.attributes:
                for path, type_name in self.classify(die.get_DIE_from_attribute(_AT_TYPE)):
                    sep = "." if path else ""
                    hits.append((f"[]{sep}{path}", type_name))

        # Pointers, references and everything else carry no lifetime
        # requirement and are not followed.

        self._cache[die.offset] = hits
        return hits


def die_function_name(die: DIE) -> str:
    """Best-effort name of the subprogram DIE owning a variable."""
    seen = set()
    while die is not None:
        if die.offset in seen:
            break
        seen.add(die.offset)

        name = die.attributes.get(_AT_NAME)
        if name is not None:
            return name.value.decode(_ENCODING)

        for ref in _AT_ORIGINS:
            if ref in die.attributes:
                die = die.get_DIE_from_attribute(ref)
                break
        else:
            break
    return _UNKNOWN


def cu_file_table(dwarfinfo: DWARFInfo, cu: CompileUnit) -> Callable[[int], str]:
    """Return a function mapping DW_AT_decl_file indices to paths."""
    try:
        lineprog = dwarfinfo.line_program_for_CU(cu)
        if lineprog is None:
            return lambda idx: f"<file {idx}>"
        entries = lineprog.header["file_entry"]
        offset = 0 if lineprog.header.version >= 5 else 1

        def lookup(idx: int) -> str:
            try:
                return entries[idx - offset].name.decode(_ENCODING)
            except (IndexError, AttributeError):
                return f"<file {idx}>"

        return lookup
    except Exception:
        return lambda idx: f"<file {idx}>"


def variable_on_stack(die: DIE) -> bool:
    """True if the variable DIE has a frame-base-relative location."""
    loc = die.attributes.get(_AT_LOCATION)
    if loc is None or loc.form not in _FORM_LOCATION_EXPRS:
        return False

    return len(loc.value) > 0 and loc.value[0] == DW_OP_fbreg


def scan_subprogram(
    die: DIE,
    classifier: TypeClassifier,
    filename_of: Callable[[int], str],
    function: str,
    findings: set[Finding],
) -> None:
    for child in die.iter_children():
        if child.tag in _TAG_SCOPES:
            child_function = function
            if child.tag != _TAG_LEXICAL_BLOCK:
                child_function = die_function_name(child)
            scan_subprogram(child, classifier, filename_of, child_function, findings)
            continue

        if child.tag not in _TAG_DATA_OBJECTS:
            continue

        if not variable_on_stack(child) or _AT_TYPE not in child.attributes:
            continue

        hits = classifier.classify(child.get_DIE_from_attribute(_AT_TYPE))
        if not hits:
            continue

        var = child.attributes.get(_AT_NAME)
        var_name = var.value.decode(_ENCODING) if var is not None else _ANONYMOUS

        decl_file = child.attributes.get(_AT_DECL_FILE)
        decl_line = child.attributes.get(_AT_DECL_LINE)
        path = filename_of(decl_file.value) if decl_file is not None else _UNKNOWN
        line = decl_line.value if decl_line is not None else 0

        for member_path, type_name in hits:
            sep = "." if member_path else ""
            findings.add(Finding(path, line, function, f"{var_name}{sep}{member_path}", type_name))


def scan(elf_path: str) -> set[Finding]:
    findings: set[Finding] = set()

    with open(elf_path, "rb") as fp:
        elf = ELFFile(fp)

        if not elf.has_dwarf_info():
            sys.exit(f"ELF file {elf_path} has no DWARF debug information")

        dwarfinfo = elf.get_dwarf_info()
        classifier = TypeClassifier()

        for cu in dwarfinfo.iter_CUs():
            filename_of = cu_file_table(dwarfinfo, cu)

            top = cu.get_top_DIE()
            for die in top.iter_children():
                if die.tag != _TAG_SUBPROGRAM:
                    continue
                scan_subprogram(die, classifier, filename_of, die_function_name(die), findings)

    return findings


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument("--elf-file", required=True, help="ELF file to scan")
    parser.add_argument(
        "--always-succeed",
        action="store_true",
        help="always exit with a return code of 0, used for reporting",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    findings = scan(args.elf_file)

    for finding in sorted(findings):
        print(
            f"{finding.path}:{finding.line}: error: stack-allocated "
            f"kernel object 'struct {finding.type_name}' at "
            f"'{finding.variable}' in '{finding.function}()'",
            file=sys.stderr,
        )

    if findings:
        print(
            f"{len(findings)} stack-allocated kernel object(s) found: "
            "these corrupt the kernel object tracking lists "
            "(CONFIG_OBJ_CORE, CONFIG_TRACING_OBJECT_TRACKING) when "
            "their stack frame is left",
            file=sys.stderr,
        )
        if not args.always_succeed:
            sys.exit(1)


if __name__ == "__main__":
    main()
