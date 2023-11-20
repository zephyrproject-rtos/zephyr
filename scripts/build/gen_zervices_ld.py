#!/usr/bin/env python3
#
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import io
import re
import yaml

from dataclasses import dataclass, field
from enum import auto, Enum
from graphlib import TopologicalSorter, CycleError

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


class Separator(Enum):
    EARLY = auto()
    PRE_KERNEL_1 = auto()
    PRE_KERNEL_2 = auto()
    POST_KERNEL = auto()
    APPLICATION = auto()
    # SMP is deprecated, but still there
    SMP = auto()

    @property
    def compatible(self):
        return self.name

    def __init__(self, value):
        Enum.__init__(self, value)
        self.instance = 0
        self.before = []
        self.after = []
        self.sanitised_before = []
        self.sanitised_after = []


@dataclass
class Entry:
    fullname: str
    compatible: str
    instance: int = 0
    before: list = field(default_factory=list)
    after: list = field(default_factory=list)
    sanitised_before: list = field(default_factory=list)
    sanitised_after: list = field(default_factory=list)

    def __hash__(self):
        return hash(self.fullname)

    def __repr__(self):
        return f"{self.compatible}#{self.instance} ({self.fullname})"


@dataclass
class Dependency:
    compatible: str
    instance: int or None


def parse_args():
    global args
    parser = argparse.ArgumentParser()
    parser.add_argument("--map", help="Zephyr map file")
    parser.add_argument("--elf", help="Zephyr elf file")
    parser.add_argument("--output", required=True, help="Output file")
    parser.add_argument("--device-tree-generated", help="Output file")
    parser.add_argument("--config", help="Config file")
    args = parser.parse_args()


def get_raw_entries(map_file):
    with open(map_file) as f:
        contents = f.read()
        return re.findall(r"\.z_init_.*___init_.*|__init_[A-Z0-9_]+start", contents)


def get_instance(raw, dts_generated):
    if "device_dts_ord_" in raw:
        ordinal = re.search(r"device_dts_ord_([0-9]+)", raw).group(1)
        devname = re.search(f"DT_N_S_(.*)_ORD {ordinal}", dts_generated).group(1)
        return int(re.search(r"INST_([0-9]+)_.*" f"{devname}", dts_generated).group(1))
    else:
        return 0


def get_compatible(raw, dts_generated):
    if "device_dts_ord_" in raw:
        ordinal = re.search(r"device_dts_ord_([0-9]+)", raw).group(1)
        devname = re.search(f"DT_N_S_(.*)_ORD {ordinal}", dts_generated).group(1)
        compatibles = re.search(
            f"{devname}" r'_P_compatible \{([a-z0-9," -]+)\}', dts_generated
        ).group(1)
        # TODO properly handle multiple compatibles. Right now we just take the
        # first one
        compatible = re.search(r'"([a-z0-9,-]+)"', compatibles).group(1)
        return compatible
    else:
        return re.search(r"___init_(.*)", raw).group(1)


def new_dts_entry(raw, dts_generated):
    return Entry(
        raw, get_compatible(raw, dts_generated), get_instance(raw, dts_generated)
    )


def collect_entries(args):
    entries = []

    raw_entries = get_raw_entries(args.map)

    with open(args.device_tree_generated) as f:
        dts_generated = f.read()

    for r in raw_entries:
        if r.startswith(".z_init"):
            entry = new_dts_entry(r, dts_generated)
        else:
            entry = Separator[re.search(r"([A-Z][A-Z0-9_]+)_start", r).group(1)]
        entry.after = [entries[-1]] if len(entries) > 0 else []
        entries.append(entry)

    return entries


def generate_linker_snippet(args, entries):
    # Add a dummy SMP entry at the end, check_init_priorities.py expects it
    entries.append(Separator.SMP)

    with io.StringIO() as output:
        for entry in entries:
            if type(entry) == Separator:
                print(f"__init_{entry.name}_start = .;", file=output)
            else:
                print(f"KEEP(*({entry.fullname}))", file=output)

        return output.getvalue()


def new_dep(dep):
    dep = dep.split("#")
    prio = None if len(dep) == 1 else int(dep[1])
    return Dependency(dep[0], prio)


# This is based on elf_parser.py, ideally, we could reuse it.
def deps_from_elf_sym(elf, sym):
    deps = []
    addr = sym.entry.st_value
    length = sym.entry.st_size
    if length == 1: # Only the \0 at the end
        return deps
    section = elf.get_section(sym.entry['st_shndx'])
    data = section.data()
    offset = addr - (0 if elf['e_type'] == 'ET_REL' else section['sh_addr'])
    assert offset + length <= len(data)
    raw = data[offset:offset + length - 1].decode("utf-8").split(";")
    for r in raw:
        deps.append(new_dep(r))
    return deps


def get_zervice_deps_from_kconfig(args, zervice_name):
    before = []
    after = []
    zervice_name = zervice_name.upper()
    with open(args.config, "r") as f:
        content = f.read()
        matches = re.search(f'CONFIG_ZERVICE_{zervice_name}_BEFORE="([\w;,-]+)"',
                            content)
        if matches:
            before = [new_dep(d) for d in matches.group(1).split(";")]

        matches = re.search(f'CONFIG_ZERVICE_{zervice_name}_AFTER="([\w;,-]+)"',
                            content)
        if matches:
            after = [new_dep(d) for d in matches.group(1).split(";")]

    return (before, after)


def get_zervice_deps_from_elf(args, zervice_name):
    before = []
    after = []
    with open(args.elf, "rb") as f:
        elf = ELFFile(f)
        for section in elf.iter_sections():
            if isinstance(section, SymbolTableSection):
                for sym in section.iter_symbols():
                    if sym.name == f"__init_{zervice_name}_after":
                        after = deps_from_elf_sym(elf, sym)
                    if sym.name == f"__init_{zervice_name}_before":
                        before = deps_from_elf_sym(elf, sym)

    if len(before) == 0 and len(after) == 0:
        raise Exception(f'No dependency is provided for "{zervice_name}"')

    return (before, after)


def match(dep, entry):
    if dep.instance is None:
        return dep.compatible == entry.compatible

    return dep.compatible == entry.compatible and dep.instance == entry.instance


def collect_zervices(args):
    zervice_entries = []
    with open(args.map) as f:
        contents = f.read()
        raw_entries = re.findall(
            r"\.z_zervice_[a-zA-Z0-9_]+$", contents, flags=re.MULTILINE
        )

        for r in raw_entries:
            entry = Entry(r, r[len(".z_zervice_") :])
            before, after = get_zervice_deps_from_kconfig(args,
                                                          entry.compatible)
            if len(before) == 0 and len(after) == 0:
                before, after = get_zervice_deps_from_elf(args,
                                                          entry.compatible)
            entry.before = before
            entry.after = after

            zervice_entries.append(entry)

    return zervice_entries


def sort_zervice_entries(zervice_entries):
    new_entries = []
    topological_sorter = TopologicalSorter()

    zc = [ze.compatible for ze in zervice_entries]

    for ze in zervice_entries:
        topological_sorter.add(
            ze.compatible,
            *[dep.compatible for dep in ze.before + ze.after if dep.compatible in zc],
        )

    for e in topological_sorter.static_order():
        new_entries.append([ze for ze in zervice_entries if ze.compatible == e][0])

    return new_entries


def sort_entries(entries):
    topological_sorter = TopologicalSorter()

    for e in entries:
        # add all predecessors of this entry
        topological_sorter.add(e, *e.sanitised_after)
        # add current entry as predecessor for the other entries
        for before in e.sanitised_before:
            topological_sorter.add(before, e)

    try:
        entries = [*topological_sorter.static_order()]
    except CycleError as ce:
        raise Exception(f"Circular dependencies: {[e.compatible for e in ce.args[1]]}")

    return entries


def sanitise_deps(entry, deps, entries, after):
    sanitised = []
    for dep in deps:
        if type(dep) in [Entry, Separator]:
            sanitised.append(dep)
        elif type(dep) == Dependency:
            candidates = [e for e in entries if match(dep, e)]
            if len(candidates) == 0:
                continue

            if len(candidates) == 1:
                sanitised.append(candidates[0])
                continue

            if after:
                sanitised.append(candidates[-1])
            else:
                sanitised.append(candidates[0])

        else:
            raise Exception(f"Unexpected dependency type {type(dep)}")

    if len(sanitised) == 0:
        deps = [
            f"{dep.compatible}"
            if dep.instance is None
            else f"{dep.compatible}#{dep.instance}"
            for dep in deps
        ]
        raise Exception(
            f"No dependency found for '{entry.compatible}', tried {deps}"
        )

    return sanitised


def as_compatible(dep, dts_generated):
    if type(dep) != Dependency:
        return dep

    # Is this dep a "chosen" one?
    dep_underscored = dep.compatible.replace(',', '_').replace('-',  '_')
    matches = re.search(f"DT_CHOSEN_{dep_underscored}\s+([\w]+)",
                        dts_generated)
    if matches:
        dev = matches.group(1)
        dep.compatible = re.search(f"{dev}"r'_P_compatible \{"([a-z0-9,-]+)"\}',
                                   dts_generated).group(1)

    return dep


# 'before' and 'after' of entries have a mix of Entry/Separator/Dependency
# instances. This method transform Dependency in the right Entry/Separator,
# and remove dependencies that were not found (but it raises an error if *no*
# dependency remains)
# TODO: does a concept of mandatory and optional deps make sense?
def sanitise_entries(entries):
    # First pass, get compatible for "chosen"
    # TODO: should we care about "alias" as well?
    with open(args.device_tree_generated) as f:
        dts_generated = f.read()
        for e in entries:
            if hasattr(e, "before") and len(e.before) > 0:
                e.before = [as_compatible(d, dts_generated) for d in e.before]
            if hasattr(e, "after") and len(e.after) > 0:
                e.after = [as_compatible(d, dts_generated) for d in e.after]

    for e in entries:
        if hasattr(e, "before") and len(e.before) > 0:
            e.sanitised_before = sanitise_deps(e, e.before, entries, False)
        if hasattr(e, "after") and len(e.after) > 0:
            e.sanitised_after = sanitise_deps(e, e.after, entries, True)


def main():
    parse_args()

    with open(args.output, "w") as f:
        entries = collect_entries(args)
        entries += collect_zervices(args)

        sanitise_entries(entries)
        entries = sort_entries(entries)

        print(generate_linker_snippet(args, entries), file=f)


if __name__ == "__main__":
    main()
