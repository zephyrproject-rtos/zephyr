#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Create the kernel's page tables for x86 CPUs.

For additional detail on paging and x86 memory management, please
consult the IA Architecture SW Developer Manual, volume 3a, chapter 4.

This script produces the initial page tables installed into the CPU
at early boot. These pages will have an identity mapping of the kernel
image. The script takes the 'zephyr_prebuilt.elf' as input to obtain region
sizes, certain memory addresses, and configuration values.

If CONFIG_SRAM_REGION_PERMISSIONS is not enabled, the kernel image will be
mapped with the Present and Write bits set. The linker scripts shouldn't
add page alignment padding between sections.

If CONFIG_SRAM_REGION_PERMISSIONS is enabled, the access permissions
vary:
  - By default, the Present, Write, and Execute Disable bits are
    set.
  - The __text_region region will have Present and User bits set
  - The __rodata_region region will have Present, User, and Execute
    Disable bits set
  - On x86_64, the _locore region will have Present set and
    the _lorodata region will have Present and Execute Disable set.

This script will establish a dual mapping at the address defined by
CONFIG_KERNEL_VM_BASE if it is not the same as CONFIG_SRAM_BASE_ADDRESS.

  - The double-mapping is used to transition the
    instruction pointer from a physical address at early boot to the
    virtual address where the kernel is actually linked.

  - The mapping is always double-mapped at the top-level paging structure
    and the physical/virtual base addresses must have the same alignment
    with respect to the scope of top-level paging structure entries.
    This allows the same second-level paging structure(s) to be used for
    both memory bases.

  - The double-mapping is needed so that we can still fetch instructions
    from identity-mapped physical addresses after we program this table
    into the MMU, then jump to the equivalent virtual address.
    The kernel then unlinks the identity mapping before continuing,
    the address space is purely virtual after that.

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
import ctypes
import os
import struct
import re
import textwrap

from packaging import version

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

if version.parse(elftools.__version__) < version.parse('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")


def bit(pos):
    """Get value by shifting 1 by pos"""
    return 1 << pos


# Page table entry flags
FLAG_P = bit(0)
FLAG_RW = bit(1)
FLAG_US = bit(2)
FLAG_CD = bit(4)
FLAG_D = bit(6)
FLAG_SZ = bit(7)
FLAG_G = bit(8)
FLAG_XD = bit(63)

FLAG_IGNORED0 = bit(9)
FLAG_IGNORED1 = bit(10)
FLAG_IGNORED2 = bit(11)

ENTRY_RW = FLAG_RW | FLAG_IGNORED0
ENTRY_US = FLAG_US | FLAG_IGNORED1
ENTRY_XD = FLAG_XD | FLAG_IGNORED2

# PD_LEVEL and PT_LEVEL are used as list index to PtableSet.levels[]
# to get table from back of list.
PD_LEVEL = -2
PT_LEVEL = -1


def debug(text):
    """Display verbose debug message"""
    if not args.verbose:
        return
    sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")


def verbose(text):
    """Display --verbose --verbose message"""
    if args.verbose and args.verbose > 1:
        sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")


def error(text):
    """Display error message and exit program"""
    sys.exit(os.path.basename(sys.argv[0]) + ": " + text)


def align_check(base, size, scope=4096):
    """Make sure base and size are page-aligned"""
    if (base % scope) != 0:
        error("unaligned base address %x" % base)
    if (size % scope) != 0:
        error("Unaligned region size 0x%x for base %x" % (size, base))


def dump_flags(flags):
    """Translate page table flags into string"""
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

    if flags & FLAG_SZ:
        ret += "SZ "

    if flags & FLAG_CD:
        ret += "CD "

    if flags & FLAG_D:
        ret += "D "

    return ret.strip()


def round_up(val, align):
    """Round up val to the next multiple of align"""
    return (val + (align - 1)) & (~(align - 1))


def round_down(val, align):
    """Round down val to the previous multiple of align"""
    return val & (~(align - 1))


# Hard-coded flags for intermediate paging levels. Permissive, we only control
# access or set caching properties at leaf levels.
INT_FLAGS = FLAG_P | FLAG_RW | FLAG_US

class MMUTable():
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

        verbose("%s: mapping 0x%x to 0x%x : %s" %
                (self.__class__.__name__,
                 phys_addr, virt_addr, dump_flags(entry_flags)))

        self.entries[index] = ((phys_addr & self.addr_mask) |
                               (entry_flags & self.supported_flags))

    def set_perms(self, virt_addr, entry_flags):
        """"For the table entry corresponding to the provided virtual address,
        update just the flags, leaving the physical mapping alone.
        Unsupported flags will be filtered out."""
        index = self.entry_index(virt_addr)

        verbose("%s: changing perm at 0x%x : %s" %
                (self.__class__.__name__,
                 virt_addr, dump_flags(entry_flags)))

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
    supported_flags = INT_FLAGS | FLAG_SZ | FLAG_CD

class PdptPAE(Pdpt):
    """Page directory pointer table for PAE"""
    num_entries = 4

class Pd(MMUTable):
    """Page directory for 32-bit"""
    addr_shift = 22
    addr_mask = 0xFFFFF000
    type_code = 'I'
    num_entries = 1024
    supported_flags = INT_FLAGS | FLAG_SZ | FLAG_CD

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
    supported_flags = (FLAG_P | FLAG_RW | FLAG_US | FLAG_G | FLAG_CD | FLAG_D |
                       FLAG_IGNORED0 | FLAG_IGNORED1)

class PtXd(Pt):
    """Page table for either PAE or IA-32e"""
    addr_mask = 0x07FFFFFFFFFFF000
    type_code = 'Q'
    num_entries = 512
    supported_flags = (FLAG_P | FLAG_RW | FLAG_US | FLAG_G | FLAG_XD | FLAG_CD |
                       FLAG_D | FLAG_IGNORED0 | FLAG_IGNORED1 | FLAG_IGNORED2)


class PtableSet():
    """Represents a complete set of page tables for any paging mode"""

    def __init__(self, pages_start):
        """Instantiate a set of page tables which will be located in the
        image starting at the provided physical memory location"""
        self.toplevel = self.levels[0]()
        self.page_pos = pages_start

        debug("%s starting at physical address 0x%x" %
              (self.__class__.__name__, self.page_pos))

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

    def is_mapped(self, virt_addr, level):
        """
        Return True if virt_addr has already been mapped.

        level_from_last == 0 only searches leaf level page tables.
        level_from_last == 1 searches both page directories and page tables.

        """
        table = self.toplevel
        num_levels = len(self.levels) + level + 1
        has_mapping = False

        # Create and link up intermediate tables if necessary
        for depth in range(0, num_levels):
            # Create child table if needed
            if table.has_entry(virt_addr):
                if depth == num_levels:
                    has_mapping = True
                else:
                    table = self.tables[table.lookup(virt_addr)]

            if has_mapping:
                # pylint doesn't like break in the above if-block
                break

        return has_mapping

    def is_region_mapped(self, virt_base, size, level=PT_LEVEL):
        """Find out if a region has been mapped"""
        align_check(virt_base, size)
        for vaddr in range(virt_base, virt_base + size, 4096):
            if self.is_mapped(vaddr, level):
                return True

        return False

    def new_child_table(self, table, virt_addr, depth):
        """Create a new child table"""
        new_table_addr = self.get_new_mmutable_addr()
        new_table = self.levels[depth]()
        debug("new %s at physical addr 0x%x"
                      % (self.levels[depth].__name__, new_table_addr))
        self.tables[new_table_addr] = new_table
        table.map(virt_addr, new_table_addr, INT_FLAGS)

        return new_table

    def map_page(self, virt_addr, phys_addr, flags, reserve, level=PT_LEVEL):
        """Map a virtual address to a physical address in the page tables,
        with provided access flags"""
        table = self.toplevel

        num_levels = len(self.levels) + level + 1

        # Create and link up intermediate tables if necessary
        for depth in range(1, num_levels):
            # Create child table if needed
            if not table.has_entry(virt_addr):
                table = self.new_child_table(table, virt_addr, depth)
            else:
                table = self.tables[table.lookup(virt_addr)]

        # Set up entry in leaf page table
        if not reserve:
            table.map(virt_addr, phys_addr, flags)

    def reserve(self, virt_base, size, to_level=PT_LEVEL):
        """Reserve page table space with already aligned virt_base and size"""
        debug("Reserving paging structures for 0x%x (0x%x)" %
              (virt_base, size))

        align_check(virt_base, size)

        # How much memory is covered by leaf page table
        scope = 1 << self.levels[PD_LEVEL].addr_shift

        if virt_base % scope != 0:
            error("misaligned virtual address space, 0x%x not a multiple of 0x%x" %
                  (virt_base, scope))

        for addr in range(virt_base, virt_base + size, scope):
            self.map_page(addr, 0, 0, True, to_level)

    def reserve_unaligned(self, virt_base, size, to_level=PT_LEVEL):
        """Reserve page table space with virt_base and size alignment"""
        # How much memory is covered by leaf page table
        scope = 1 << self.levels[PD_LEVEL].addr_shift

        mem_start = round_down(virt_base, scope)
        mem_end = round_up(virt_base + size, scope)
        mem_size = mem_end - mem_start

        self.reserve(mem_start, mem_size, to_level)

    def map(self, phys_base, virt_base, size, flags, level=PT_LEVEL):
        """Map an address range in the page tables provided access flags.
        If virt_base is None, identity mapping using phys_base is done.
        """
        is_identity_map = virt_base is None or virt_base == phys_base

        if virt_base is None:
            virt_base = phys_base

        scope = 1 << self.levels[level].addr_shift

        debug("Mapping 0x%x (0x%x) to 0x%x: %s" %
                (phys_base, size, virt_base, dump_flags(flags)))

        align_check(phys_base, size, scope)
        align_check(virt_base, size, scope)
        for paddr in range(phys_base, phys_base + size, scope):
            if is_identity_map and paddr == 0 and level == PT_LEVEL:
                # Never map the NULL page at page table level.
                continue

            vaddr = virt_base + (paddr - phys_base)

            self.map_page(vaddr, paddr, flags, False, level)

    def identity_map_unaligned(self, phys_base, size, flags, level=PT_LEVEL):
        """Identity map a region of memory"""
        scope = 1 << self.levels[level].addr_shift

        phys_aligned_base = round_down(phys_base, scope)
        phys_aligned_end = round_up(phys_base + size, scope)
        phys_aligned_size = phys_aligned_end - phys_aligned_base

        self.map(phys_aligned_base, None, phys_aligned_size, flags, level)

    def map_region(self, name, flags, virt_to_phys_offset, level=PT_LEVEL):
        """Map a named region"""
        if not isdef(name + "_start"):
            # Region may not exists
            return

        region_start = syms[name + "_start"]
        region_end = syms[name + "_end"]
        region_size = region_end - region_start

        region_start_phys = region_start

        if virt_to_phys_offset is not None:
            region_start_phys += virt_to_phys_offset

        self.map(region_start_phys, region_start, region_size, flags, level)

    def set_region_perms(self, name, flags, level=PT_LEVEL):
        """Set access permissions for a named region that is already mapped

        The bounds of the region will be looked up in the symbol table
        with _start and _size suffixes. The physical address mapping
        is unchanged and this will not disturb any double-mapping."""
        if not isdef(name + "_start"):
            # Region may not exists
            return

        # Doesn't matter if this is a virtual address, we have a
        # either dual mapping or it's the same as physical
        base = syms[name + "_start"]

        if isdef(name + "_size"):
            size = syms[name + "_size"]
        else:
            region_end = syms[name + "_end"]
            size = region_end - base

        if size == 0:
            return

        debug("change flags for %s at 0x%x (0x%x): %s" %
              (name, base, size, dump_flags(flags)))

        num_levels = len(self.levels) + level + 1
        scope = 1 << self.levels[level].addr_shift

        align_check(base, size, scope)

        try:
            for addr in range(base, base + size, scope):
                # Never map the NULL page
                if addr == 0:
                    continue

                table = self.toplevel
                for _ in range(1, num_levels):
                    table = self.tables[table.lookup(addr)]
                table.set_perms(addr, flags)
        except KeyError:
            error("no mapping for %s region 0x%x (size 0x%x)" %
                  (name, base, size))

    def write_output(self, filename):
        """Write the page tables to the output file in binary format"""
        written_size = 0

        with open(filename, "wb") as output_fp:
            for addr in sorted(self.tables):
                mmu_table = self.tables[addr]
                mmu_table_bin = mmu_table.get_binary()
                output_fp.write(mmu_table_bin)
                written_size += len(mmu_table_bin)

            # We always have the top-level table be last. This is because
            # in PAE, the top-level PDPT has only 4 entries and is not a
            # full page in size. We do not put it in the tables dictionary
            # and treat it as a special case.
            debug("top-level %s at physical addr 0x%x" %
                  (self.toplevel.__class__.__name__,
                   self.get_new_mmutable_addr()))
            top_level_bin = self.toplevel.get_binary()
            output_fp.write(top_level_bin)
            written_size += len(top_level_bin)

        return written_size

# Paging mode classes, we'll use one depending on configuration
class Ptables32bit(PtableSet):
    """32-bit Page Tables"""
    levels = [Pd, Pt]

class PtablesPAE(PtableSet):
    """PAE Page Tables"""
    levels = [PdptPAE, PdXd, PtXd]

class PtablesIA32e(PtableSet):
    """Page Tables under IA32e mode"""
    levels = [Pml4, Pdpt, PdXd, PtXd]


def parse_args():
    """Parse command line arguments"""
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("-k", "--kernel", required=True,
                        help="path to prebuilt kernel ELF binary")
    parser.add_argument("-o", "--output", required=True,
                        help="output file")
    parser.add_argument("--map", action='append',
                        help=textwrap.dedent('''\
                            Map extra memory:
                            <physical address>,<size>[,<flags:LUWXD>[,<virtual address>]]
                            where flags can be empty or combination of:
                                L - Large page (2MB or 4MB),
                                U - Userspace accessible,
                                W - Writable,
                                X - Executable,
                                D - Cache disabled.
                            Default is
                                small (4KB) page,
                                supervisor only,
                                read only,
                                and execution disabled.
                            '''))
    parser.add_argument("-v", "--verbose", action="count",
                        help="Print extra debugging information")
    args = parser.parse_args()
    if "VERBOSE" in os.environ:
        args.verbose = 1


def get_symbols(elf_obj):
    """Get all symbols from the ELF file"""
    for section in elf_obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()}

    raise LookupError("Could not find symbol table")

def isdef(sym_name):
    """True if symbol is defined in ELF file"""
    return sym_name in syms


def find_symbol(obj, name):
    """Find symbol object from ELF file"""
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            for sym in section.iter_symbols():
                if sym.name == name:
                    return sym

    return None


def map_extra_regions(pt):
    """Map extra regions specified in command line"""
    # Extract command line arguments
    mappings = []

    for entry in args.map:
        elements = entry.split(',')

        if len(elements) < 2:
            error("Not enough arguments for --map %s" % entry)

        one_map = {}

        one_map['cmdline'] = entry
        one_map['phys'] = int(elements[0], 0)
        one_map['size']= int(elements[1], 0)
        one_map['large_page'] = False

        flags = FLAG_P | ENTRY_XD
        if len(elements) > 2:
            map_flags = elements[2]

            # Check for allowed flags
            if not bool(re.match('^[LUWXD]*$', map_flags)):
                error("Unrecognized flags: %s" % map_flags)

            flags = FLAG_P | ENTRY_XD
            if 'W' in map_flags:
                flags |= ENTRY_RW
            if 'X' in map_flags:
                flags &= ~ENTRY_XD
            if 'U' in map_flags:
                flags |= ENTRY_US
            if 'L' in map_flags:
                flags |=  FLAG_SZ
                one_map['large_page'] = True
            if 'D' in map_flags:
                flags |= FLAG_CD

        one_map['flags'] = flags

        if len(elements) > 3:
            one_map['virt'] = int(elements[3], 16)
        else:
            one_map['virt'] = one_map['phys']

        mappings.append(one_map)

    # Map the regions
    for one_map in mappings:
        phys = one_map['phys']
        size = one_map['size']
        flags = one_map['flags']
        virt = one_map['virt']
        level = PD_LEVEL if one_map['large_page'] else PT_LEVEL

        # Check if addresses have already been mapped.
        # Error out if so as they could override kernel mappings.
        if pt.is_region_mapped(virt, size, level):
            error(("Region 0x%x (%d) already been mapped "
                   "for --map %s" % (virt, size, one_map['cmdline'])))

        # Reserve space in page table, and map the region
        pt.reserve_unaligned(virt, size, level)
        pt.map(phys, virt, size, flags, level)


def main():
    """Main program"""
    global syms
    parse_args()

    with open(args.kernel, "rb") as elf_fp:
        kernel = ELFFile(elf_fp)
        syms = get_symbols(kernel)

        sym_dummy_pagetables = find_symbol(kernel, "dummy_pagetables")
        if sym_dummy_pagetables:
            reserved_pt_size = sym_dummy_pagetables['st_size']
        else:
            reserved_pt_size = None

    if isdef("CONFIG_X86_64"):
        pclass = PtablesIA32e
    elif isdef("CONFIG_X86_PAE"):
        pclass = PtablesPAE
    else:
        pclass = Ptables32bit

    debug("building %s" % pclass.__name__)

    vm_base = syms["CONFIG_KERNEL_VM_BASE"]
    vm_size = syms["CONFIG_KERNEL_VM_SIZE"]
    vm_offset = syms["CONFIG_KERNEL_VM_OFFSET"]

    sram_base = syms["CONFIG_SRAM_BASE_ADDRESS"]
    sram_size = syms["CONFIG_SRAM_SIZE"] * 1024

    mapped_kernel_base = syms["z_mapped_start"]
    mapped_kernel_size = syms["z_mapped_size"]

    if isdef("CONFIG_SRAM_OFFSET"):
        sram_offset = syms["CONFIG_SRAM_OFFSET"]
    else:
        sram_offset = 0

    # Figure out if there is any need to do virtual-to-physical
    # address translation
    virt_to_phys_offset = (sram_base + sram_offset) - (vm_base + vm_offset)

    if isdef("CONFIG_ARCH_MAPS_ALL_RAM"):
        image_base = sram_base
        image_size = sram_size
    else:
        image_base = mapped_kernel_base
        image_size = mapped_kernel_size

    image_base_phys = image_base + virt_to_phys_offset

    ptables_phys = syms["z_x86_pagetables_start"] + virt_to_phys_offset

    debug("Address space: 0x%x - 0x%x size 0x%x" %
          (vm_base, vm_base + vm_size - 1, vm_size))

    debug("Zephyr image: 0x%x - 0x%x size 0x%x" %
          (image_base, image_base + image_size - 1, image_size))

    if virt_to_phys_offset != 0:
        debug("Physical address space: 0x%x - 0x%x size 0x%x" %
              (sram_base, sram_base + sram_size - 1, sram_size))

    is_perm_regions = isdef("CONFIG_SRAM_REGION_PERMISSIONS")

    # Are pages in non-boot, non-pinned sections present at boot.
    is_generic_section_present = isdef("CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT")

    if image_size >= vm_size:
        error("VM size is too small (have 0x%x need more than 0x%x)" % (vm_size, image_size))

    map_flags = 0

    if is_perm_regions:
        # Don't allow execution by default for any pages. We'll adjust this
        # in later calls to pt.set_region_perms()
        map_flags = ENTRY_XD

    pt = pclass(ptables_phys)
    # Instantiate all the paging structures for the address space
    pt.reserve(vm_base, vm_size)
    # Map the zephyr image
    if is_generic_section_present:
        map_flags = map_flags | FLAG_P
        pt.map(image_base_phys, image_base, image_size, map_flags | ENTRY_RW)
    else:
        # When generic linker sections are not present in physical memory,
        # the corresponding virtual pages should not be mapped to non-existent
        # physical pages. So simply identity map them to create the page table
        # entries but without the present bit set.
        # Boot and pinned sections (if configured) will be mapped to
        # physical memory below.
        pt.map(image_base, image_base, image_size, map_flags | ENTRY_RW)

    if virt_to_phys_offset != 0:
        # Need to identity map the physical address space
        # as it is needed during early boot process.
        # This will be unmapped once z_x86_mmu_init()
        # is called.
        # Note that this only does the identity mapping
        # at the page directory level to minimize wasted space.
        pt.reserve_unaligned(image_base_phys, image_size, to_level=PD_LEVEL)
        pt.identity_map_unaligned(image_base_phys, image_size,
                                  FLAG_P | FLAG_RW | FLAG_SZ, level=PD_LEVEL)

    if isdef("CONFIG_X86_64"):
        # 64-bit has a special region in the first 64K to bootstrap other CPUs
        # from real mode
        locore_base = syms["_locore_start"]
        locore_size = syms["_lodata_end"] - locore_base
        debug("Base addresses: physical 0x%x size 0x%x" % (locore_base,
                                                         locore_size))
        pt.map(locore_base, None, locore_size, map_flags | FLAG_P | ENTRY_RW)

    if isdef("CONFIG_XIP"):
        # Additionally identity-map all ROM as read-only
        pt.map(syms["CONFIG_FLASH_BASE_ADDRESS"], None,
               syms["CONFIG_FLASH_SIZE"] * 1024, map_flags | FLAG_P)

    if isdef("CONFIG_LINKER_USE_BOOT_SECTION"):
        pt.map_region("lnkr_boot", map_flags | FLAG_P | ENTRY_RW, virt_to_phys_offset)

    if isdef("CONFIG_LINKER_USE_PINNED_SECTION"):
        pt.map_region("lnkr_pinned", map_flags | FLAG_P | ENTRY_RW, virt_to_phys_offset)

    # Process extra mapping requests
    if args.map:
        map_extra_regions(pt)

    # Adjust mapped region permissions if configured
    if is_perm_regions:
        # Need to accomplish the following things:
        # - Text regions need the XD flag cleared and RW flag removed
        #   if not built with gdbstub support
        # - Rodata regions need the RW flag cleared
        # - User mode needs access as we currently do not separate application
        #   text/rodata from kernel text/rodata
        if isdef("CONFIG_GDBSTUB"):
            flags = ENTRY_US | ENTRY_RW
        else:
            flags = ENTRY_US

        if is_generic_section_present:
            flags = flags | FLAG_P

        pt.set_region_perms("__text_region", flags)

        if isdef("CONFIG_LINKER_USE_BOOT_SECTION"):
            pt.set_region_perms("lnkr_boot_text", flags | FLAG_P)

        if isdef("CONFIG_LINKER_USE_PINNED_SECTION"):
            pt.set_region_perms("lnkr_pinned_text", flags | FLAG_P)

        flags = ENTRY_US | ENTRY_XD
        if is_generic_section_present:
            flags = flags | FLAG_P

        pt.set_region_perms("__rodata_region", flags)

        if isdef("CONFIG_LINKER_USE_BOOT_SECTION"):
            pt.set_region_perms("lnkr_boot_rodata", flags | FLAG_P)

        if isdef("CONFIG_LINKER_USE_PINNED_SECTION"):
            pt.set_region_perms("lnkr_pinned_rodata", flags | FLAG_P)

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

        if isdef("CONFIG_HW_SHADOW_STACK"):
            pt.set_region_perms("__x86shadowstack", FLAG_P | FLAG_D | ENTRY_XD)

    written_size = pt.write_output(args.output)
    debug("Written %d bytes to %s" % (written_size, args.output))

    # Warn if reserved page table is not of correct size
    if reserved_pt_size and written_size != reserved_pt_size:
        # Figure out how many extra pages needed
        size_diff = written_size - reserved_pt_size
        page_size = syms["CONFIG_MMU_PAGE_SIZE"]
        extra_pages_needed = int(round_up(size_diff, page_size) / page_size)

        if isdef("CONFIG_X86_EXTRA_PAGE_TABLE_PAGES"):
            extra_pages_kconfig = syms["CONFIG_X86_EXTRA_PAGE_TABLE_PAGES"]
            if isdef("CONFIG_X86_64"):
                extra_pages_needed += ctypes.c_int64(extra_pages_kconfig).value
            else:
                extra_pages_needed += ctypes.c_int32(extra_pages_kconfig).value

        reason = "big" if reserved_pt_size > written_size else "small"

        error(("Reserved space for page table is too %s."
               " Set CONFIG_X86_EXTRA_PAGE_TABLE_PAGES=%d") %
               (reason, extra_pages_needed))


if __name__ == "__main__":
    main()
