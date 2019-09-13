#!/usr/bin/env python3
#
# Copyright (c) 2017-2018 Linaro
#
# SPDX-License-Identifier: Apache-2.0

import sys
import os
import struct
from distutils.version import LooseVersion

from collections import OrderedDict

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

if LooseVersion(elftools.__version__) < LooseVersion('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")


def subsystem_to_enum(subsys):
    return "K_OBJ_DRIVER_" + subsys[:-11].upper()


def kobject_to_enum(kobj):
    if kobj.startswith("k_") or kobj.startswith("_k_"):
        name = kobj[2:]
    else:
        name = kobj

    return "K_OBJ_%s" % name.upper()


DW_OP_addr = 0x3
DW_OP_fbreg = 0x91
STACK_TYPE = "_k_thread_stack_element"
thread_counter = 0
sys_mutex_counter = 0
futex_counter = 0

# Global type environment. Populated by pass 1.
type_env = {}
extern_env = {}
kobjects = {}
subsystems = {}

# --- debug stuff ---

scr = os.path.basename(sys.argv[0])

# --- type classes ----


class KobjectInstance:
    def __init__(self, type_obj, addr):
        global thread_counter
        global sys_mutex_counter
        global futex_counter

        self.addr = addr
        self.type_obj = type_obj

        # Type name determined later since drivers needs to look at the
        # API struct address
        self.type_name = None

        if self.type_obj.name == "k_thread":
            # Assign an ID for this thread object, used to track its
            # permissions to other kernel objects
            self.data = thread_counter
            thread_counter = thread_counter + 1
        elif self.type_obj.name == "sys_mutex":
            self.data = "(u32_t)(&kernel_mutexes[%d])" % sys_mutex_counter
            sys_mutex_counter += 1
        elif self.type_obj.name == "k_futex":
            self.data = "(u32_t)(&futex_data[%d])" % futex_counter
            futex_counter += 1
        else:
            self.data = 0


class KobjectType:
    def __init__(self, offset, name, size, api=False):
        self.name = name
        self.size = size
        self.offset = offset
        self.api = api

    def __repr__(self):
        return "<kobject %s>" % self.name

    @staticmethod
    def has_kobject():
        return True

    def get_kobjects(self, addr):
        return {addr: KobjectInstance(self, addr)}


class ArrayType:
    def __init__(self, offset, elements, member_type):
        self.elements = elements
        self.member_type = member_type
        self.offset = offset

    def __repr__(self):
        return "<array of %d>" % self.member_type

    def has_kobject(self):
        if self.member_type not in type_env:
            return False

        return type_env[self.member_type].has_kobject()

    def get_kobjects(self, addr):
        mt = type_env[self.member_type]

        # Stacks are arrays of _k_stack_element_t but we want to treat
        # the whole array as one kernel object (a thread stack)
        # Data value gets set to size of entire region
        if isinstance(mt, KobjectType) and mt.name == STACK_TYPE:
            # An array of stacks appears as a multi-dimensional array.
            # The last size is the size of each stack. We need to track
            # each stack within the array, not as one huge stack object.
            *dimensions, stacksize = self.elements
            num_members = 1
            for e in dimensions:
                num_members = num_members * e

            ret = {}
            for i in range(num_members):
                a = addr + (i * stacksize)
                o = mt.get_kobjects(a)
                o[a].data = stacksize
                ret.update(o)
            return ret

        objs = {}

        # Multidimensional array flattened out
        num_members = 1
        for e in self.elements:
            num_members = num_members * e

        for i in range(num_members):
            objs.update(mt.get_kobjects(addr + (i * mt.size)))
        return objs


class AggregateTypeMember:
    def __init__(self, offset, member_name, member_type, member_offset):
        self.member_name = member_name
        self.member_type = member_type
        if isinstance(member_offset, list):
            # DWARF v2, location encoded as set of operations
            # only "DW_OP_plus_uconst" with ULEB128 argument supported
            if member_offset[0] == 0x23:
                self.member_offset = member_offset[1] & 0x7f
                for i in range(1, len(member_offset)-1):
                    if member_offset[i] & 0x80:
                        self.member_offset += (
                            member_offset[i+1] & 0x7f) << i*7
            else:
                raise Exception("not yet supported location operation (%s:%d:%d)" %
                        (self.member_name, self.member_type, member_offset[0]))
        else:
            self.member_offset = member_offset

    def __repr__(self):
        return "<member %s, type %d, offset %d>" % (
            self.member_name, self.member_type, self.member_offset)

    def has_kobject(self):
        if self.member_type not in type_env:
            return False

        return type_env[self.member_type].has_kobject()

    def get_kobjects(self, addr):
        mt = type_env[self.member_type]
        return mt.get_kobjects(addr + self.member_offset)


class ConstType:
    def __init__(self, child_type):
        self.child_type = child_type

    def __repr__(self):
        return "<const %d>" % self.child_type

    def has_kobject(self):
        if self.child_type not in type_env:
            return False

        return type_env[self.child_type].has_kobject()

    def get_kobjects(self, addr):
        return type_env[self.child_type].get_kobjects(addr)


class AggregateType:
    def __init__(self, offset, name, size):
        self.name = name
        self.size = size
        self.offset = offset
        self.members = []

    def add_member(self, member):
        self.members.append(member)

    def __repr__(self):
        return "<struct %s, with %s>" % (self.name, self.members)

    def has_kobject(self):
        result = False

        bad_members = []

        for member in self.members:
            if member.has_kobject():
                result = True
            else:
                bad_members.append(member)
                # Don't need to consider this again, just remove it

        for bad_member in bad_members:
            self.members.remove(bad_member)

        return result

    def get_kobjects(self, addr):
        objs = {}
        for member in self.members:
            objs.update(member.get_kobjects(addr))
        return objs


# --- helper functions for getting data from DIEs ---

def die_get_spec(die):
    if 'DW_AT_specification' not in die.attributes:
        return None

    spec_val = die.attributes["DW_AT_specification"].value

    # offset of the DW_TAG_variable for the extern declaration
    offset = spec_val + die.cu.cu_offset

    return extern_env.get(offset)


def die_get_name(die):
    if 'DW_AT_name' not in die.attributes:
        die = die_get_spec(die)
        if not die:
            return None

    return die.attributes["DW_AT_name"].value.decode("utf-8")


def die_get_type_offset(die):
    if 'DW_AT_type' not in die.attributes:
        die = die_get_spec(die)
        if not die:
            return None

    return die.attributes["DW_AT_type"].value + die.cu.cu_offset


def die_get_byte_size(die):
    if 'DW_AT_byte_size' not in die.attributes:
        return 0

    return die.attributes["DW_AT_byte_size"].value


def analyze_die_struct(die):
    name = die_get_name(die) or "<anon>"
    offset = die.offset
    size = die_get_byte_size(die)

    # Incomplete type
    if not size:
        return

    if name in kobjects:
        type_env[offset] = KobjectType(offset, name, size)
    elif name in subsystems:
        type_env[offset] = KobjectType(offset, name, size, api=True)
    else:
        at = AggregateType(offset, name, size)
        type_env[offset] = at

        for child in die.iter_children():
            if child.tag != "DW_TAG_member":
                continue
            data_member_location = child.attributes.get("DW_AT_data_member_location")
            if not data_member_location:
                continue

            child_type = die_get_type_offset(child)
            member_offset = data_member_location.value
            cname = die_get_name(child) or "<anon>"
            m = AggregateTypeMember(child.offset, cname, child_type,
                                    member_offset)
            at.add_member(m)

        return


def analyze_die_const(die):
    type_offset = die_get_type_offset(die)
    if not type_offset:
        return

    type_env[die.offset] = ConstType(type_offset)


def analyze_die_array(die):
    type_offset = die_get_type_offset(die)
    elements = []

    for child in die.iter_children():
        if child.tag != "DW_TAG_subrange_type":
            continue
        if "DW_AT_upper_bound" not in child.attributes:
            continue

        ub = child.attributes["DW_AT_upper_bound"]
        if not ub.form.startswith("DW_FORM_data"):
            continue

        elements.append(ub.value + 1)

    if not elements:
        if type_offset in type_env.keys():
            mt = type_env[type_offset]
            if mt.has_kobject():
                if isinstance(mt, KobjectType) and mt.name == STACK_TYPE:
                    elements.append(1)
                    type_env[die.offset] = ArrayType(die.offset, elements, type_offset)
    else:
        type_env[die.offset] = ArrayType(die.offset, elements, type_offset)


def analyze_typedef(die):
    type_offset = die_get_type_offset(die)

    if type_offset not in type_env.keys():
        return

    type_env[die.offset] = type_env[type_offset]


def addr_deref(elf, addr):
    for section in elf.iter_sections():
        start = section['sh_addr']
        end = start + section['sh_size']

        if start <= addr < end:
            data = section.data()
            offset = addr - start
            return struct.unpack("<I" if elf.little_endian else ">I",
                                 data[offset:offset + 4])[0]

    return 0


def device_get_api_addr(elf, addr):
    return addr_deref(elf, addr + 4)


def get_filename_lineno(die):
    lp_header = die.dwarfinfo.line_program_for_CU(die.cu).header
    files = lp_header["file_entry"]
    includes = lp_header["include_directory"]

    fileinfo = files[die.attributes["DW_AT_decl_file"].value - 1]
    filename = fileinfo.name.decode("utf-8")
    filedir = includes[fileinfo.dir_index - 1].decode("utf-8")

    path = os.path.join(filedir, filename)
    lineno = die.attributes["DW_AT_decl_line"].value
    return (path, lineno)


class ElfHelper:

    def __init__(self, filename, verbose, kobjs, subs):
        self.verbose = verbose
        self.fp = open(filename, "rb")
        self.elf = ELFFile(self.fp)
        self.little_endian = self.elf.little_endian
        global kobjects
        global subsystems
        kobjects = kobjs
        subsystems = subs

    def find_kobjects(self, syms):
        if not self.elf.has_dwarf_info():
            sys.exit("ELF file has no DWARF information")

        app_smem_start = syms["_app_smem_start"]
        app_smem_end = syms["_app_smem_end"]

        di = self.elf.get_dwarf_info()

        variables = []

        # Step 1: collect all type information.
        for CU in di.iter_CUs():
            for die in CU.iter_DIEs():
                # Unions are disregarded, kernel objects should never be union
                # members since the memory is not dedicated to that object and
                # could be something else
                if die.tag == "DW_TAG_structure_type":
                    analyze_die_struct(die)
                elif die.tag == "DW_TAG_const_type":
                    analyze_die_const(die)
                elif die.tag == "DW_TAG_array_type":
                    analyze_die_array(die)
                elif die.tag == "DW_TAG_typedef":
                    analyze_typedef(die)
                elif die.tag == "DW_TAG_variable":
                    variables.append(die)

        # Step 2: filter type_env to only contain kernel objects, or structs
        # and arrays of kernel objects
        bad_offsets = []
        for offset, type_object in type_env.items():
            if not type_object.has_kobject():
                bad_offsets.append(offset)

        for offset in bad_offsets:
            del type_env[offset]

        # Step 3: Now that we know all the types we are looking for, examine
        # all variables
        all_objs = {}

        for die in variables:
            name = die_get_name(die)
            if not name:
                continue

            if name.startswith("__device_sys_init"):
                # Boot-time initialization function; not an actual device
                continue

            type_offset = die_get_type_offset(die)

            # Is this a kernel object, or a structure containing kernel
            # objects?
            if type_offset not in type_env:
                continue

            if "DW_AT_declaration" in die.attributes:
                # Extern declaration, only used indirectly
                extern_env[die.offset] = die
                continue

            if "DW_AT_location" not in die.attributes:
                self.debug_die(
                    die,
                    "No location information for object '%s'; possibly"
                    " stack allocated" % name)
                continue

            loc = die.attributes["DW_AT_location"]
            if loc.form != "DW_FORM_exprloc" and \
               loc.form != "DW_FORM_block1":
                self.debug_die(
                    die,
                    "kernel object '%s' unexpected location format" %
                    name)
                continue

            opcode = loc.value[0]
            if opcode != DW_OP_addr:

                # Check if frame pointer offset DW_OP_fbreg
                if opcode == DW_OP_fbreg:
                    self.debug_die(die, "kernel object '%s' found on stack" %
                                   name)
                else:
                    self.debug_die(
                        die,
                        "kernel object '%s' unexpected exprloc opcode %s" %
                        (name, hex(opcode)))
                continue

            addr = (loc.value[1] | (loc.value[2] << 8) |
                    (loc.value[3] << 16) | (loc.value[4] << 24))

            if addr == 0:
                # Never linked; gc-sections deleted it
                continue

            type_obj = type_env[type_offset]
            objs = type_obj.get_kobjects(addr)
            all_objs.update(objs)

            self.debug("symbol '%s' at %s contains %d object(s)"
                       % (name, hex(addr), len(objs)))

        # Step 4: objs is a dictionary mapping variable memory addresses to
        # their associated type objects. Now that we have seen all variables
        # and can properly look up API structs, convert this into a dictionary
        # mapping variables to the C enumeration of what kernel object type it
        # is.
        ret = {}
        for addr, ko in all_objs.items():
            # API structs don't get into the gperf table
            if ko.type_obj.api:
                continue

            _, user_ram_allowed = kobjects[ko.type_obj.name]
            if not user_ram_allowed and app_smem_start <= addr < app_smem_end:
                self.debug_die(die,
                               "object '%s' found in invalid location %s"
                               % (name, hex(addr)))
                continue

            if ko.type_obj.name != "device":
                # Not a device struct so we immediately know its type
                ko.type_name = kobject_to_enum(ko.type_obj.name)
                ret[addr] = ko
                continue

            # Device struct. Need to get the address of its API struct,
            # if it has one.
            apiaddr = device_get_api_addr(self.elf, addr)
            if apiaddr not in all_objs:
                if apiaddr == 0:
                    self.debug("device instance at 0x%x has no associated subsystem"
                            % addr)
                else:
                    self.debug("device instance at 0x%x has unknown API 0x%x"
                            % (addr, apiaddr))
                # API struct does not correspond to a known subsystem, skip it
                continue

            apiobj = all_objs[apiaddr]
            ko.type_name = subsystem_to_enum(apiobj.type_obj.name)
            ret[addr] = ko

        self.debug("found %d kernel object instances total" % len(ret))

        # 1. Before python 3.7 dict order is not guaranteed. With Python
        #    3.5 it doesn't seem random with *integer* keys but can't
        #    rely on that.
        # 2. OrderedDict means _insertion_ order, so not enough because
        #    built from other (random!) dicts: need to _sort_ first.
        # 3. Sorting memory address looks good.
        return OrderedDict(sorted(ret.items()))

    def get_symbols(self):
        for section in self.elf.iter_sections():
            if isinstance(section, SymbolTableSection):
                return {sym.name: sym.entry.st_value
                        for sym in section.iter_symbols()}

        raise LookupError("Could not find symbol table")

    def debug(self, text):
        if not self.verbose:
            return
        sys.stdout.write(scr + ": " + text + "\n")

    @staticmethod
    def error(text):
        sys.exit("%s ERROR: %s" % (scr, text))

    def debug_die(self, die, text):
        fn, ln = get_filename_lineno(die)

        self.debug(str(die))
        self.debug("File '%s', line %d:" % (fn, ln))
        self.debug("    %s" % text)

    @staticmethod
    def get_thread_counter():
        return thread_counter

    @staticmethod
    def get_sys_mutex_counter():
        return sys_mutex_counter

    @staticmethod
    def get_futex_counter():
        return futex_counter
