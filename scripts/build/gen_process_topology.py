#!/usr/bin/env python3
#
# Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Assemble the static process topology into the ELF at link time.

K_SESSION_DEFINE(), K_PROCESS_GROUP_DEFINE(), and K_PROCESS_DEFINE_EXT() carry
only child->parent backreferences in their C initializers (group.session,
process.pgrp): the reverse-direction membership lists (session.pgrps,
group.members) are circular doubly-linked lists spanning translation units,
which no C initializer can express. This pass patches them directly into
.data, so the topology is valid in the ELF image before any C runs.

This pass is mandatory for builds that declare static topology, exactly as
gen_kobject_list.py is mandatory for user-mode kernel objects: there is no
boot-time fallback.

Struct member offsets are taken from the DWARF debug info, so the pass adapts
to configuration-dependent layouts without a generated header.
"""

import argparse
import struct
import sys

from elftools.elf.elffile import ELFFile

K_SESSION_FLAG_STATIC = 0x01


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("elf", help="zephyr.elf to patch in place")
    parser.add_argument("-v", "--verbose", action="store_true")
    return parser.parse_args()


class Image:
    """Minimal address-space view over the ELF file, with write-back."""

    def __init__(self, path):
        self.path = path
        self.f = open(path, "r+b")
        self.elf = ELFFile(self.f)
        self.ptr_size = self.elf.elfclass // 8
        self.endian = "<" if self.elf.little_endian else ">"
        self.segments = [
            (seg["p_vaddr"], seg["p_filesz"], seg["p_offset"])
            for seg in self.elf.iter_segments()
            if seg["p_type"] == "PT_LOAD"
        ]
        self.symtab = self.elf.get_section_by_name(".symtab")

    def sym(self, name):
        syms = self.symtab.get_symbol_by_name(name)
        if not syms:
            return None
        return syms[0]["st_value"]

    def offset_of(self, vaddr):
        for base, size, off in self.segments:
            if base <= vaddr < base + size:
                return off + (vaddr - base)
        raise ValueError(f"address 0x{vaddr:x} is not file-backed")

    def read_ptr(self, vaddr):
        self.f.seek(self.offset_of(vaddr))
        raw = self.f.read(self.ptr_size)
        return struct.unpack(self.endian + ("I" if self.ptr_size == 4 else "Q"), raw)[0]

    def write_ptr(self, vaddr, value):
        self.f.seek(self.offset_of(vaddr))
        self.f.write(struct.pack(self.endian + ("I" if self.ptr_size == 4 else "Q"), value))

    def read_u8(self, vaddr):
        self.f.seek(self.offset_of(vaddr))
        return self.f.read(1)[0]

    def struct_layout(self, tag):
        """(size, {member: offset}) for a DWARF structure type named tag."""
        dwarf = self.elf.get_dwarf_info()
        for cu in dwarf.iter_CUs():
            for die in cu.iter_DIEs():
                if die.tag != "DW_TAG_structure_type":
                    continue
                name = die.attributes.get("DW_AT_name")
                if name is None or name.value.decode() != tag:
                    continue
                size_attr = die.attributes.get("DW_AT_byte_size")
                if size_attr is None:
                    continue
                members = {}
                for child in die.iter_children():
                    if child.tag != "DW_TAG_member":
                        continue
                    mname = child.attributes["DW_AT_name"].value.decode()
                    members[mname] = child.attributes["DW_AT_data_member_location"].value
                if members:
                    return size_attr.value, members
        raise KeyError(f"struct {tag} not found in DWARF")


def section_array(img, name, size):
    start = img.sym(f"_{name}_list_start")
    end = img.sym(f"_{name}_list_end")
    if start is None or end is None or end <= start:
        return []
    return list(range(start, end, size))


def link_dlist(img, head_vaddr, node_vaddrs, verbose, what):
    """Write a circular sys_dlist: head {head, tail} plus each node {next, prev}."""
    if not node_vaddrs:
        return
    chain = [head_vaddr] + node_vaddrs
    n = len(chain)
    for i, node in enumerate(chain):
        nxt = chain[(i + 1) % n]
        prv = chain[(i - 1) % n]
        img.write_ptr(node, nxt)
        img.write_ptr(node + img.ptr_size, prv)
    if verbose:
        print(f"  {what}: {len(node_vaddrs)} member(s)")


def main():
    args = parse_args()
    img = Image(args.elf)

    try:
        sess_size, sess = img.struct_layout("k_session")
        grp_size, grp = img.struct_layout("k_process_group")
        proc_size, proc = img.struct_layout("k_process")
    except KeyError:
        # CONFIG_PROCESS is off (or debug info is absent): nothing to assemble
        return 0

    sessions = section_array(img, "k_session", sess_size)
    groups = section_array(img, "k_process_group", grp_size)
    processes = section_array(img, "k_process", proc_size)

    static_sessions = [
        s for s in sessions if img.read_u8(s + sess["flags"]) & K_SESSION_FLAG_STATIC
    ]
    topo_groups = [g for g in groups if img.read_ptr(g + grp["session"]) in static_sessions]

    if args.verbose and (static_sessions or topo_groups):
        print(f"{args.elf}: assembling static process topology")

    for s in static_sessions:
        members = [
            g + grp["session_node"] for g in topo_groups if img.read_ptr(g + grp["session"]) == s
        ]
        link_dlist(img, s + sess["pgrps"], members, args.verbose, f"session @0x{s:x}")

    for g in topo_groups:
        members = [
            p + proc["pgrp_node"] for p in processes if img.read_ptr(p + proc["pgrp"]) == g
        ]
        link_dlist(img, g + grp["members"], members, args.verbose, f"group @0x{g:x}")

    img.f.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
