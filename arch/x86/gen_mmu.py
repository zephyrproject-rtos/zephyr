#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Create the kernel's page tables for x86 CPUs.

For additional detail on paging and x86 memory management, please
consult the IA Architecture SW Developer Manual, volume 3a, chapter 4.

This script produces the initial page tables installed into the CPU
at early boot. These pages will have an identity mapping at
CONFIG_SRAM_BASE_ADDRESS of size CONFIG_SRAM_SIZE. The script takes
the 'zephyr_prebuilt.elf' as input to obtain region sizes, certain
memory addresses, and configuration values.

If CONFIG_SRAM_REGION_PERMISSIONS is not enabled, all RAM will be
mapped with the Present and Write bits set. The linker scripts shouldn't
add page alignment padding between sections.

If CONFIG_SRAM_REGION_PERMISSIONS is enabled, the access permissions
vary:
  - By default, the Present, Write, and Execute Disable bits are
    set.
  - The _image_text region will have Present and User bits set
  - The _image_rodata region will have Present, User, and Execute
    Disable bits set
  - On x86_64, the _locore region will have Present set and
    the _lorodata region will have Present and Execute Disable set.

Because the set of page tables are linked together by physical address,
we must know a priori the physical address of each table. The linker
script must define a z_x86_pagetables_start symbol where the page
tables will be placed, and this memory address must not shift between
prebuilt and final ELF builds. This script will not work on systems
where the physical load address of the kernel is unknown at build time.

64-bit systems will always build IA-32e page tables. 32-bit systems
build PAE page tables if CONFIG_X86_PAE is set, otherwise standard
32-bit page tables are built.

The kernel will expect to find the top-level structure of the produced
page tables at the physical address corresponding to the symbol
z_x86_kernel_ptables. The linker script will need to set that symbol
to the end of the binary produced by this script, minus the size of the
top-level paging structure as it is written out last.
"""

import sys
import array
import argparse
import os
import struct
import elftools
from distutils.version import LooseVersion
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

if LooseVersion(elftools.__version__) < LooseVersion('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")


def bit(pos):
    return 1 << pos


# Page table entry flags
FLAG_P = bit(0)
FLAG_RW = bit(1)
FLAG_US = bit(2)
FLAG_G = bit(8)
FLAG_XD = bit(63)

FLAG_IGNORED0 = bit(9)
FLAG_IGNORED1 = bit(10)
FLAG_IGNORED2 = bit(11)

ENTRY_RW = FLAG_RW | FLAG_IGNORED0
ENTRY_US = FLAG_US | FLAG_IGNORED1
ENTRY_XD = FLAG_XD | FLAG_IGNORED2

def debug(text):
    if not args.verbose:
        return
    sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")


def error(text):
    sys.exit(os.path.basename(sys.argv[0]) + ": " + text)


def align_check(base, size):
    if (base % 4096) != 0:
        error("unaligned base address %x" % base)
    if (size % 4096) != 0:
        error("Unaligned region size %d for base %x" % (size, base))


def dump_flags(flags):
    ret = ""

    if flags & FLAG_P:
        ret += "P "

    if flags & FLAG_RW:
        ret += "RW "

    if flags & FLAG_US:
        ret += "US "

    if flags & FLAG_G:
        ret += "G "

    if flags & FLAG_XD:
        ret += "XD "

    return ret.strip()

# Hard-coded flags for intermediate paging levels. Permissive, we only control
# access or set caching properties at leaf levels.
INT_FLAGS = FLAG_P | FLAG_RW | FLAG_US

class MMUTable(object):
    """Represents a particular table in a set of page tables, at any level"""

    def __init__(self):
        self.entries = array.array(self.type_code,
                                   [0 for i in range(self.num_entries)])

    def get_binary(self):
        """Return a bytearray representation of this table"""
        # Always little-endian
        ctype = "<" + self.type_code
        entry_size = struct.calcsize(ctype)
        ret = bytearray(entry_size * self.num_entries)

        for i in range(self.num_entries):
            struct.pack_into(ctype, ret, entry_size * i, self.entries[i])
        return ret

    @property
    def supported_flags(self):
        """Class property indicating what flag bits are supported"""
        raise NotImplementedError()

    @property
    def addr_shift(self):
        """Class property for how much to shift virtual addresses to obtain
        the appropriate index in the table for it"""
        raise NotImplementedError()

    @property
    def addr_mask(self):
        """Mask to apply to an individual entry to get the physical address
        mapping"""
        raise NotImplementedError()

    @property
    def type_code(self):
        """Struct packing letter code for table entries. Either I for
        32-bit entries, or Q for PAE/IA-32e"""
        raise NotImplementedError()

    @property
    def num_entries(self):
        """Number of entries in the table. Varies by table type and paging
        mode"""
        raise NotImplementedError()

    def entry_index(self, virt_addr):
        """Get the index of the entry in this table that corresponds to the
        provided virtual address"""
        return (virt_addr >> self.addr_shift) & (self.num_entries - 1)

    def has_entry(self, virt_addr):
        """Indicate whether an entry is present in this table for the provided
        virtual address"""
        index = self.entry_index(virt_addr)

        return (self.entries[index] & FLAG_P) != 0

    def lookup(self, virt_addr):
        """Look up the physical mapping for a virtual address.

        If this is a leaf table, this is the physical address mapping. If not,
        this is the physical address of the next level table"""
        index = self.entry_index(virt_addr)

        return self.entries[index] & self.addr_mask

    def map(self, virt_addr, phys_addr, entry_flags):
        """For the table entry corresponding to the provided virtual address,
        set the corresponding physical entry in the table. Unsupported flags
        will be filtered out.

        If this is a leaf table, this is the physical address mapping. If not,
        this is the physical address of the next level table"""
        index = self.entry_index(virt_addr)

        self.entries[index] = ((phys_addr & self.addr_mask) |
                               (entry_flags & self.supported_flags))

    def set_perms(self, virt_addr, entry_flags):
        """"For the table entry corresponding to the provided virtual address,
        update just the flags, leaving the physical mapping alone.
        Unsupported flags will be filtered out."""
        index = self.entry_index(virt_addr)

        self.entries[index] = ((self.entries[index] & self.addr_mask) |
                               (entry_flags & self.supported_flags))


# Specific supported table types
class Pml4(MMUTable):
    """Page mapping level 4 for IA-32e"""
    addr_shift = 39
    addr_mask = 0x7FFFFFFFFFFFF000
    type_code = 'Q'
    num_entries = 512
    supported_flags = INT_FLAGS

class Pdpt(MMUTable):
    """Page directory pointer table for IA-32e"""
    addr_shift = 30
    addr_mask = 0x7FFFFFFFFFFFF000
    type_code = 'Q'
    num_entries = 512
    supported_flags = INT_FLAGS

class PdptPAE(Pdpt):
    """Page directory pointer table for PAE"""
    num_entries = 4

class Pd(MMUTable):
    """Page directory for 32-bit"""
    addr_shift = 22
    addr_mask = 0xFFFFF000
    type_code = 'I'
    num_entries = 1024
    supported_flags = INT_FLAGS

class PdXd(Pd):
    """Page directory for either PAE or IA-32e"""
    addr_shift = 21
    addr_mask = 0x7FFFFFFFFFFFF000
    num_entries = 512
    type_code = 'Q'

class Pt(MMUTable):
    """Page table for 32-bit"""
    addr_shift = 12
    addr_mask = 0xFFFFF000
    type_code = 'I'
    num_entries = 1024
    supported_flags = (FLAG_P | FLAG_RW | FLAG_US | FLAG_G |
                       FLAG_IGNORED0 | FLAG_IGNORED1)

class PtXd(Pt):
    """Page table for either PAE or IA-32e"""
    addr_mask = 0x07FFFFFFFFFFF000
    type_code = 'Q'
    num_entries = 512
    supported_flags = (FLAG_P | FLAG_RW | FLAG_US | FLAG_G | FLAG_XD |
                       FLAG_IGNORED0 | FLAG_IGNORED1 | FLAG_IGNORED2)


class PtableSet(object):
    """Represents a complete set of page tables for any paging mode"""

    def __init__(self, pages_start):
        """Instantiate a set of page tables which will be located in the
        image starting at the provided physical memory location"""
        self.page_pos = pages_start
        self.toplevel = self.levels[0]()

        debug("%s starting at physical address 0x%x" %
              (self.__class__.__name__, pages_start))

        # Database of page table pages. Maps physical memory address to
        # MMUTable objects, excluding the top-level table which is tracked
        # separately. Starts out empty as we haven't mapped anything and
        # the top-level table is tracked separately.
        self.tables = {}

    def get_new_mmutable_addr(self):
        """If we need to instantiate a new MMUTable, return a physical
        address location for it"""
        ret = self.page_pos
        self.page_pos += 4096
        return ret

    @property
    def levels(self):
        """Class hierarchy of paging levels, with the first entry being
        the toplevel table class, and the last entry always being
        some kind of leaf page table class (Pt or PtXd)"""
        raise NotImplementedError()

    def map_page(self, virt_addr, phys_addr, flags):
        """Map a virtual address to a physical address in the page tables,
        with provided access flags"""
        table = self.toplevel

        # Create and link up intermediate tables if necessary
        for depth in range(1, len(self.levels)):
            # Create child table if needed
            if not table.has_entry(virt_addr):
                new_table_addr = self.get_new_mmutable_addr()
                new_table = self.levels[depth]()
                debug("new %s at physical addr 0x%x"
                      % (self.levels[depth].__name__, new_table_addr))
                self.tables[new_table_addr] = new_table
                table.map(virt_addr, new_table_addr, INT_FLAGS)
                table = new_table
            else:
                table = self.tables[table.lookup(virt_addr)]

        # Set up entry in leaf page table
        table.map(virt_addr, phys_addr, flags)

    def map(self, phys_base, size, flags):
        """Identity map an address range in the page tables, with provided
        access flags.
        """
        debug("Identity-mapping 0x%x (%d): %s" %
              (phys_base, size, dump_flags(flags)))

        align_check(phys_base, size)
        for addr in range(phys_base, phys_base + size, 4096):
            if addr == 0:
                # Never map the NULL page
                continue

            self.map_page(addr, addr, flags)

    def set_region_perms(self, name, flags):
        """Set access permissions for a named region that is already mapped

        The bounds of the region will be looked up in the symbol table
        with _start and _size suffixes. The physical address mapping
        is unchanged and this will not disturb any double-mapping."""

        # Doesn't matter if this is a virtual address, we have a
        # either dual mapping or it's the same as physical
        base = syms[name + "_start"]
        size = syms[name + "_size"]

        debug("change flags for %s at 0x%x (%d): %s" %
              (name, base, size, dump_flags(flags)))
        align_check(base, size)

        try:
            for addr in range(base, base + size, 4096):
                # Never map the NULL page
                if addr == 0:
                    continue

                table = self.toplevel
                for _ in range(1, len(self.levels)):
                    table = self.tables[table.lookup(addr)]
                table.set_perms(addr, flags)
        except KeyError:
            error("no mapping for %s region 0x%x (size 0x%x)" %
                  (name, base, size))

    def write_output(self, filename):
        """Write the page tables to the output file in binary format"""
        with open(filename, "wb") as fp:
            for addr in sorted(self.tables):
                mmu_table = self.tables[addr]
                fp.write(mmu_table.get_binary())

            # We always have the top-level table be last. This is because
            # in PAE, the top-level PDPT has only 4 entries and is not a
            # full page in size. We do not put it in the tables dictionary
            # and treat it as a special case.
            debug("top-level %s at physical addr 0x%x" %
                  (self.toplevel.__class__.__name__,
                   self.get_new_mmutable_addr()))
            fp.write(self.toplevel.get_binary())

# Paging mode classes, we'll use one depending on configuration
class Ptables32bit(PtableSet):
    levels = [Pd, Pt]

class PtablesPAE(PtableSet):
    levels = [PdptPAE, PdXd, PtXd]

class PtablesIA32e(PtableSet):
    levels = [Pml4, Pdpt, PdXd, PtXd]


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-k", "--kernel", required=True,
                        help="path to prebuilt kernel ELF binary")
    parser.add_argument("-o", "--output", required=True,
                        help="output file")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print extra debugging information")
    args = parser.parse_args()
    if "VERBOSE" in os.environ:
        args.verbose = True


def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()}

    raise LookupError("Could not find symbol table")

def isdef(sym_name):
    return sym_name in syms

def main():
    global syms
    parse_args()

    with open(args.kernel, "rb") as fp:
        kernel = ELFFile(fp)
        syms = get_symbols(kernel)

    if isdef("CONFIG_X86_64"):
        pclass = PtablesIA32e
    elif isdef("CONFIG_X86_PAE"):
        pclass = PtablesPAE
    else:
        pclass = Ptables32bit

    debug("building %s" % pclass.__name__)

    ram_base = syms["CONFIG_SRAM_BASE_ADDRESS"]
    ram_size = syms["CONFIG_SRAM_SIZE"] * 1024
    ptables_phys = syms["z_x86_pagetables_start"]

    debug("Base addresses: physical 0x%x size %d" % (ram_base, ram_size))

    is_perm_regions = isdef("CONFIG_SRAM_REGION_PERMISSIONS")

    if is_perm_regions:
        # Don't allow execution by default for any pages. We'll adjust this
        # in later calls to pt.set_region_perms()
        map_flags = FLAG_P |  ENTRY_XD
    else:
        map_flags = FLAG_P

    pt = pclass(ptables_phys)
    pt.map(ram_base, ram_size, map_flags | ENTRY_RW)

    if isdef("CONFIG_XIP"):
        # Additionally identity-map all ROM as read-only
        pt.map(syms["CONFIG_FLASH_BASE_ADDRESS"],
               syms["CONFIG_FLASH_SIZE"] * 1024, map_flags)

    # Adjust mapped region permissions if configured
    if is_perm_regions:
        # Need to accomplish the following things:
        # - Text regions need the XD flag cleared and RW flag removed
        #   if not built with gdbstub support
        # - Rodata regions need the RW flag cleared
        # - User mode needs access as we currently do not separate application
        #   text/rodata from kernel text/rodata
        if isdef("CONFIG_GDBSTUB"):
            pt.set_region_perms("_image_text", FLAG_P | ENTRY_US | ENTRY_RW)
        else:
            pt.set_region_perms("_image_text", FLAG_P | ENTRY_US)
        pt.set_region_perms("_image_rodata", FLAG_P | ENTRY_US | ENTRY_XD)

        if isdef("CONFIG_COVERAGE_GCOV") and isdef("CONFIG_USERSPACE"):
            # If GCOV is enabled, user mode must be able to write to its
            # common data area
            pt.set_region_perms("__gcov_bss",
                                FLAG_P | ENTRY_RW | ENTRY_US | ENTRY_XD)

        if isdef("CONFIG_X86_64"):
            # Set appropriate permissions for locore areas much like we did
            # with the main text/rodata regions

            if isdef("CONFIG_X86_KPTI"):
                # Set the User bit for the read-only locore/lorodata areas.
                # This ensures they get mapped into the User page tables if
                # KPTI is turned on. There is no sensitive data in them, and
                # they contain text/data needed to take an exception or
                # interrupt.
                flag_user = ENTRY_US
            else:
                flag_user = 0

            pt.set_region_perms("_locore", FLAG_P | flag_user)
            pt.set_region_perms("_lorodata", FLAG_P | ENTRY_XD | flag_user)

    pt.write_output(args.output)

if __name__ == "__main__":
    main()
