#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to generate gperf tables of kernel object metadata

User mode threads making system calls reference kernel objects by memory
address, as the kernel/driver APIs in Zephyr are the same for both user
and supervisor contexts. It is necessary for the kernel to be able to
validate accesses to kernel objects to make the following assertions:

    - That the memory address points to a kernel object

    - The kernel object is of the expected type for the API being invoked

    - The kernel object is of the expected initialization state

    - The calling thread has sufficient permissions on the object

For more details see the :ref:`kernelobjects` section in the documentation.

The zephyr build generates an intermediate ELF binary, zephyr_prebuilt.elf,
which this script scans looking for kernel objects by examining the DWARF
debug information to look for instances of data structures that are considered
kernel objects. For device drivers, the API struct pointer populated at build
time is also examined to disambiguate between various device driver instances
since they are all 'struct device'.

This script can generate five different output files:

    - A gperf script to generate the hash table mapping kernel object memory
      addresses to kernel object metadata, used to track permissions,
      object type, initialization state, and any object-specific data.

    - A header file containing generated macros for validating driver instances
      inside the system call handlers for the driver subsystem APIs.

    - A code fragment included by kernel.h with one enum constant for
      each kernel object type and each driver instance.

    - The inner cases of a switch/case C statement, included by
      kernel/userspace.c, mapping the kernel object types and driver
      instances to their human-readable representation in the
      otype_to_str() function.

    - The inner cases of a switch/case C statement, included by
      kernel/userspace.c, mapping kernel object types to their sizes.
      This is used for allocating instances of them at runtime
      (CONFIG_DYNAMIC_OBJECTS) in the obj_size_get() function.
"""

import sys
import argparse
import math
import os
import struct
import json
from packaging import version

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

if version.parse(elftools.__version__) < version.parse('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")

from collections import OrderedDict

# Keys in this dictionary are structs which should be recognized as kernel
# objects. Values are a tuple:
#
#  - The first item is None, or the name of a Kconfig that
#    indicates the presence of this object's definition in case it is not
#    available in all configurations.
#
#  - The second item is a boolean indicating whether it is permissible for
#    the object to be located in user-accessible memory.
#
#  - The third items is a boolean indicating whether this item can be
#    dynamically allocated with k_object_alloc(). Keep this in sync with
#    the switch statement in z_impl_k_object_alloc().
#
# Key names in all caps do not correspond to a specific data type but instead
# indicate that objects of its type are of a family of compatible data
# structures

# Regular dictionaries are ordered only with Python 3.6 and
# above. Good summary and pointers to official documents at:
# https://stackoverflow.com/questions/39980323/are-dictionaries-ordered-in-python-3-6
kobjects = OrderedDict([
    ("k_mem_slab", (None, False, True)),
    ("k_msgq", (None, False, True)),
    ("k_mutex", (None, False, True)),
    ("k_pipe", (None, False, True)),
    ("k_queue", (None, False, True)),
    ("k_poll_signal", (None, False, True)),
    ("k_sem", (None, False, True)),
    ("k_stack", (None, False, True)),
    ("k_thread", (None, False, True)), # But see #
    ("k_timer", (None, False, True)),
    ("z_thread_stack_element", (None, False, False)),
    ("device", (None, False, False)),
    ("NET_SOCKET", (None, False, False)),
    ("net_if", (None, False, False)),
    ("sys_mutex", (None, True, False)),
    ("k_futex", (None, True, False)),
    ("k_condvar", (None, False, True)),
    ("k_event", ("CONFIG_EVENTS", False, True)),
    ("ztest_suite_node", ("CONFIG_ZTEST", True, False)),
    ("ztest_suite_stats", ("CONFIG_ZTEST", True, False)),
    ("ztest_unit_test", ("CONFIG_ZTEST", True, False)),
    ("ztest_test_rule", ("CONFIG_ZTEST", True, False)),
    ("rtio", ("CONFIG_RTIO", False, False)),
    ("rtio_iodev", ("CONFIG_RTIO", False, False)),
    ("rtio_pool", ("CONFIG_RTIO", False, False)),
    ("sensor_decoder_api", ("CONFIG_SENSOR_ASYNC_API", True, False))
])

def kobject_to_enum(kobj):
    if kobj.startswith("k_") or kobj.startswith("z_"):
        name = kobj[2:]
    else:
        name = kobj

    return "K_OBJ_%s" % name.upper()

subsystems = [
    # Editing the list is deprecated, add the __subsystem sentinel to your driver
    # api declaration instead. e.x.
    #
    # __subsystem struct my_driver_api {
    #    ....
    #};
]

# Names of all structs tagged with __net_socket, found by parse_syscalls.py
net_sockets = [ ]

def subsystem_to_enum(subsys):
    if not subsys.endswith("_driver_api"):
        raise Exception("__subsystem is missing _driver_api suffix: (%s)" % subsys)

    return "K_OBJ_DRIVER_" + subsys[:-11].upper()

# --- debug stuff ---

scr = os.path.basename(sys.argv[0])

def debug(text):
    if not args.verbose:
        return
    sys.stdout.write(scr + ": " + text + "\n")

def error(text):
    sys.exit("%s ERROR: %s" % (scr, text))

def debug_die(die, text):
    lp_header = die.dwarfinfo.line_program_for_CU(die.cu).header
    files = lp_header["file_entry"]
    includes = lp_header["include_directory"]

    fileinfo = files[die.attributes["DW_AT_decl_file"].value - 1]
    filename = fileinfo.name.decode("utf-8")
    filedir = includes[fileinfo.dir_index - 1].decode("utf-8")

    path = os.path.join(filedir, filename)
    lineno = die.attributes["DW_AT_decl_line"].value

    debug(str(die))
    debug("File '%s', line %d:" % (path, lineno))
    debug("    %s" % text)

# -- ELF processing

DW_OP_addr = 0x3
DW_OP_plus_uconst = 0x23
DW_OP_fbreg = 0x91
STACK_TYPE = "z_thread_stack_element"
thread_counter = 0
sys_mutex_counter = 0
futex_counter = 0
stack_counter = 0

# Global type environment. Populated by pass 1.
type_env = {}
extern_env = {}

class KobjectInstance:
    def __init__(self, type_obj, addr):
        self.addr = addr
        self.type_obj = type_obj

        # Type name determined later since drivers needs to look at the
        # API struct address
        self.type_name = None
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
    elif name in net_sockets:
        type_env[offset] = KobjectType(offset, "NET_SOCKET", size)
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

        if "DW_AT_upper_bound" in child.attributes:
            ub = child.attributes["DW_AT_upper_bound"]

            if not ub.form.startswith("DW_FORM_data"):
                continue

            elements.append(ub.value + 1)
        # in DWARF 4, e.g. ARC Metaware toolchain, DW_AT_count is used
        # not DW_AT_upper_bound
        elif "DW_AT_count" in child.attributes:
            ub = child.attributes["DW_AT_count"]

            if not ub.form.startswith("DW_FORM_data"):
                continue

            elements.append(ub.value)
        else:
            continue

    if not elements:
        if type_offset in type_env:
            mt = type_env[type_offset]
            if mt.has_kobject():
                if isinstance(mt, KobjectType) and mt.name == STACK_TYPE:
                    elements.append(1)
                    type_env[die.offset] = ArrayType(die.offset, elements, type_offset)
    else:
        type_env[die.offset] = ArrayType(die.offset, elements, type_offset)


def analyze_typedef(die):
    type_offset = die_get_type_offset(die)

    if type_offset not in type_env:
        return

    type_env[die.offset] = type_env[type_offset]


def unpack_pointer(elf, data, offset):
    endian_code = "<" if elf.little_endian else ">"
    if elf.elfclass == 32:
        size_code = "I"
        size = 4
    else:
        size_code = "Q"
        size = 8

    return struct.unpack(endian_code + size_code,
                         data[offset:offset + size])[0]


def addr_deref(elf, addr):
    for section in elf.iter_sections():
        start = section['sh_addr']
        end = start + section['sh_size']

        if start <= addr < end:
            data = section.data()
            offset = addr - start
            return unpack_pointer(elf, data, offset)

    return 0


def device_get_api_addr(elf, addr):
    # See include/device.h for a description of struct device
    offset = 8 if elf.elfclass == 32 else 16
    return addr_deref(elf, addr + offset)


def find_kobjects(elf, syms):
    global thread_counter
    global sys_mutex_counter
    global futex_counter
    global stack_counter

    if not elf.has_dwarf_info():
        sys.exit("ELF file has no DWARF information")

    app_smem_start = syms["_app_smem_start"]
    app_smem_end = syms["_app_smem_end"]

    if "CONFIG_LINKER_USE_PINNED_SECTION" in syms and "_app_smem_pinned_start" in syms:
        app_smem_pinned_start = syms["_app_smem_pinned_start"]
        app_smem_pinned_end = syms["_app_smem_pinned_end"]
    else:
        app_smem_pinned_start = app_smem_start
        app_smem_pinned_end = app_smem_end

    user_stack_start = syms["z_user_stacks_start"]
    user_stack_end = syms["z_user_stacks_end"]

    di = elf.get_dwarf_info()

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

        if name.startswith("__init_sys_init"):
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
            debug_die(die,
                      "No location information for object '%s'; possibly stack allocated"
                      % name)
            continue

        loc = die.attributes["DW_AT_location"]
        if loc.form not in ("DW_FORM_exprloc", "DW_FORM_block1"):
            debug_die(die, "kernel object '%s' unexpected location format" %
                      name)
            continue

        opcode = loc.value[0]
        if opcode != DW_OP_addr:

            # Check if frame pointer offset DW_OP_fbreg
            if opcode == DW_OP_fbreg:
                debug_die(die, "kernel object '%s' found on stack" % name)
            else:
                debug_die(die,
                          "kernel object '%s' unexpected exprloc opcode %s" %
                          (name, hex(opcode)))
            continue

        if "CONFIG_64BIT" in syms:
            addr = ((loc.value[1] << 0 ) | (loc.value[2] << 8)  |
                    (loc.value[3] << 16) | (loc.value[4] << 24) |
                    (loc.value[5] << 32) | (loc.value[6] << 40) |
                    (loc.value[7] << 48) | (loc.value[8] << 56))
        else:
            addr = ((loc.value[1] << 0 ) | (loc.value[2] << 8)  |
                    (loc.value[3] << 16) | (loc.value[4] << 24))

            # Handle a DW_FORM_exprloc that contains a DW_OP_addr, followed immediately by
            # a DW_OP_plus_uconst.
            if len(loc.value) >= 7 and loc.value[5] == DW_OP_plus_uconst:
                addr += (loc.value[6])

        if addr == 0:
            # Never linked; gc-sections deleted it
            continue

        type_obj = type_env[type_offset]
        objs = type_obj.get_kobjects(addr)
        all_objs.update(objs)

        debug("symbol '%s' at %s contains %d object(s)"
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

        _, user_ram_allowed, _ = kobjects[ko.type_obj.name]
        if (not user_ram_allowed and
            ((app_smem_start <= addr < app_smem_end)
             or (app_smem_pinned_start <= addr < app_smem_pinned_end))):
            debug("object '%s' found in invalid location %s"
                  % (ko.type_obj.name, hex(addr)))
            continue

        if (ko.type_obj.name == STACK_TYPE and
                (addr < user_stack_start or addr >= user_stack_end)):
            debug("skip kernel-only stack at %s" % hex(addr))
            continue

        # At this point we know the object will be included in the gperf table
        if ko.type_obj.name == "k_thread":
            # Assign an ID for this thread object, used to track its
            # permissions to other kernel objects
            ko.data = thread_counter
            thread_counter = thread_counter + 1
        elif ko.type_obj.name == "sys_mutex":
            ko.data = "&kernel_mutexes[%d]" % sys_mutex_counter
            sys_mutex_counter += 1
        elif ko.type_obj.name == "k_futex":
            ko.data = "&futex_data[%d]" % futex_counter
            futex_counter += 1
        elif ko.type_obj.name == STACK_TYPE:
            stack_counter += 1

        if ko.type_obj.name != "device":
            # Not a device struct so we immediately know its type
            ko.type_name = kobject_to_enum(ko.type_obj.name)
            ret[addr] = ko
            continue

        # Device struct. Need to get the address of its API struct,
        # if it has one.
        apiaddr = device_get_api_addr(elf, addr)
        if apiaddr not in all_objs:
            if apiaddr == 0:
                debug("device instance at 0x%x has no associated subsystem"
                      % addr)
            else:
                debug("device instance at 0x%x has unknown API 0x%x"
                      % (addr, apiaddr))
            # API struct does not correspond to a known subsystem, skip it
            continue

        apiobj = all_objs[apiaddr]
        ko.type_name = subsystem_to_enum(apiobj.type_obj.name)
        ret[addr] = ko

    debug("found %d kernel object instances total" % len(ret))

    # 1. Before python 3.7 dict order is not guaranteed. With Python
    #    3.5 it doesn't seem random with *integer* keys but can't
    #    rely on that.
    # 2. OrderedDict means _insertion_ order, so not enough because
    #    built from other (random!) dicts: need to _sort_ first.
    # 3. Sorting memory address looks good.
    return OrderedDict(sorted(ret.items()))

def get_symbols(elf):
    for section in elf.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()}

    raise LookupError("Could not find symbol table")


# -- GPERF generation logic

header = """%compare-lengths
%define lookup-function-name z_object_lookup
%language=ANSI-C
%global-table
%struct-type
%{
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/internal/syscall_handler.h>
#include <string.h>
%}
struct k_object;
"""

# Different versions of gperf have different prototypes for the lookup
# function, best to implement the wrapper here. The pointer value itself is
# turned into a string, we told gperf to expect binary strings that are not
# NULL-terminated.
footer = """%%
struct k_object *z_object_gperf_find(const void *obj)
{
    return z_object_lookup((const char *)obj, sizeof(void *));
}

void z_object_gperf_wordlist_foreach(_wordlist_cb_func_t func, void *context)
{
    int i;

    for (i = MIN_HASH_VALUE; i <= MAX_HASH_VALUE; i++) {
        if (wordlist[i].name != NULL) {
            func(&wordlist[i], context);
        }
    }
}

#ifndef CONFIG_DYNAMIC_OBJECTS
struct k_object *k_object_find(const void *obj)
	ALIAS_OF(z_object_gperf_find);

void k_object_wordlist_foreach(_wordlist_cb_func_t func, void *context)
	ALIAS_OF(z_object_gperf_wordlist_foreach);
#endif
"""


def write_gperf_table(fp, syms, objs, little_endian, static_begin, static_end):
    fp.write(header)
    if sys_mutex_counter != 0:
        fp.write("static struct k_mutex kernel_mutexes[%d] = {\n"
                 % sys_mutex_counter)
        for i in range(sys_mutex_counter):
            fp.write("Z_MUTEX_INITIALIZER(kernel_mutexes[%d])" % i)
            if i != sys_mutex_counter - 1:
                fp.write(", ")
        fp.write("};\n")

    if futex_counter != 0:
        fp.write("static struct z_futex_data futex_data[%d] = {\n"
                 % futex_counter)
        for i in range(futex_counter):
            fp.write("Z_FUTEX_DATA_INITIALIZER(futex_data[%d])" % i)
            if i != futex_counter - 1:
                fp.write(", ")
        fp.write("};\n")

    metadata_names = {
        "K_OBJ_THREAD" : "thread_id",
        "K_OBJ_SYS_MUTEX" : "mutex",
        "K_OBJ_FUTEX" : "futex_data"
    }

    if "CONFIG_GEN_PRIV_STACKS" in syms:
        metadata_names["K_OBJ_THREAD_STACK_ELEMENT"] = "stack_data"
        if stack_counter != 0:
            # Same as K_KERNEL_STACK_ARRAY_DEFINE, but routed to a different
            # memory section.
            fp.write("static uint8_t Z_GENERIC_SECTION(.priv_stacks.noinit) "
                     " __aligned(Z_KERNEL_STACK_OBJ_ALIGN)"
                     " priv_stacks[%d][K_KERNEL_STACK_LEN(CONFIG_PRIVILEGED_STACK_SIZE)];\n"
                     % stack_counter)

            fp.write("static const struct z_stack_data stack_data[%d] = {\n"
                     % stack_counter)
            counter = 0
            for _, ko in objs.items():
                if ko.type_name != "K_OBJ_THREAD_STACK_ELEMENT":
                    continue

                # ko.data currently has the stack size. fetch the value to
                # populate the appropriate entry in stack_data, and put
                # a reference to the entry in stack_data into the data value
                # instead
                size = ko.data
                ko.data = "&stack_data[%d]" % counter
                fp.write("\t{ %d, (uint8_t *)(&priv_stacks[%d]) }"
                         % (size, counter))
                if counter != (stack_counter - 1):
                    fp.write(",")
                fp.write("\n")
                counter += 1
            fp.write("};\n")
    else:
        metadata_names["K_OBJ_THREAD_STACK_ELEMENT"] = "stack_size"

    fp.write("%%\n")
    # Setup variables for mapping thread indexes
    thread_max_bytes = syms["CONFIG_MAX_THREAD_BYTES"]
    thread_idx_map = {}

    for i in range(0, thread_max_bytes):
        thread_idx_map[i] = 0xFF

    for obj_addr, ko in objs.items():
        obj_type = ko.type_name
        # pre-initialized objects fall within this memory range, they are
        # either completely initialized at build time, or done automatically
        # at boot during some PRE_KERNEL_* phase
        initialized = static_begin <= obj_addr < static_end
        is_driver = obj_type.startswith("K_OBJ_DRIVER_")

        if "CONFIG_64BIT" in syms:
            format_code = "Q"
        else:
            format_code = "I"

        if little_endian:
            endian = "<"
        else:
            endian = ">"

        byte_str = struct.pack(endian + format_code, obj_addr)
        fp.write("\"")
        for byte in byte_str:
            val = "\\x%02x" % byte
            fp.write(val)

        flags = "0"
        if initialized:
            flags += " | K_OBJ_FLAG_INITIALIZED"
        if is_driver:
            flags += " | K_OBJ_FLAG_DRIVER"

        if ko.type_name in metadata_names:
            tname = metadata_names[ko.type_name]
        else:
            tname = "unused"

        fp.write("\", {0}, %s, %s, { .%s = %s }\n" % (obj_type, flags,
		tname, str(ko.data)))

        if obj_type == "K_OBJ_THREAD":
            idx = math.floor(ko.data / 8)
            bit = ko.data % 8
            thread_idx_map[idx] = thread_idx_map[idx] & ~(2**bit)

    fp.write(footer)

    # Generate the array of already mapped thread indexes
    fp.write('\n')
    fp.write('Z_GENERIC_DOT_SECTION(data)\n')
    fp.write('uint8_t _thread_idx_map[%d] = {' % (thread_max_bytes))

    for i in range(0, thread_max_bytes):
        fp.write(' 0x%x, ' % (thread_idx_map[i]))

    fp.write('};\n')


driver_macro_tpl = """
#define K_SYSCALL_DRIVER_%(driver_upper)s(ptr, op) K_SYSCALL_DRIVER_GEN(ptr, op, %(driver_lower)s, %(driver_upper)s)
"""


def write_validation_output(fp):
    fp.write("#ifndef DRIVER_VALIDATION_GEN_H\n")
    fp.write("#define DRIVER_VALIDATION_GEN_H\n")

    fp.write("""#define K_SYSCALL_DRIVER_GEN(ptr, op, driver_lower_case, driver_upper_case) \\
		(K_SYSCALL_OBJ(ptr, K_OBJ_DRIVER_##driver_upper_case) || \\
		 K_SYSCALL_DRIVER_OP(ptr, driver_lower_case##_driver_api, op))
                """)

    for subsystem in subsystems:
        subsystem = subsystem.replace("_driver_api", "")

        fp.write(driver_macro_tpl % {
            "driver_lower": subsystem.lower(),
            "driver_upper": subsystem.upper(),
        })

    fp.write("#endif /* DRIVER_VALIDATION_GEN_H */\n")


def write_kobj_types_output(fp):
    fp.write("/* Core kernel objects */\n")
    for kobj, obj_info in kobjects.items():
        dep, _, _ = obj_info
        if kobj == "device":
            continue

        if dep:
            fp.write("#ifdef %s\n" % dep)

        fp.write("%s,\n" % kobject_to_enum(kobj))

        if dep:
            fp.write("#endif\n")

    fp.write("/* Driver subsystems */\n")
    for subsystem in subsystems:
        subsystem = subsystem.replace("_driver_api", "").upper()
        fp.write("K_OBJ_DRIVER_%s,\n" % subsystem)


def write_kobj_otype_output(fp):
    fp.write("/* Core kernel objects */\n")
    for kobj, obj_info in kobjects.items():
        dep, _, _ = obj_info
        if kobj == "device":
            continue

        if dep:
            fp.write("#ifdef %s\n" % dep)

        fp.write('case %s: ret = "%s"; break;\n' %
                 (kobject_to_enum(kobj), kobj))
        if dep:
            fp.write("#endif\n")

    fp.write("/* Driver subsystems */\n")
    for subsystem in subsystems:
        subsystem = subsystem.replace("_driver_api", "")
        fp.write('case K_OBJ_DRIVER_%s: ret = "%s driver"; break;\n' % (
            subsystem.upper(),
            subsystem
        ))


def write_kobj_size_output(fp):
    fp.write("/* Non device/stack objects */\n")
    for kobj, obj_info in kobjects.items():
        dep, _, alloc = obj_info

        if not alloc:
            continue

        if dep:
            fp.write("#ifdef %s\n" % dep)

        fp.write('case %s: ret = sizeof(struct %s); break;\n' %
                 (kobject_to_enum(kobj), kobj))
        if dep:
            fp.write("#endif\n")


def parse_subsystems_list_file(path):
    with open(path, "r") as fp:
        subsys_list = json.load(fp)
    subsystems.extend(subsys_list["__subsystem"])
    net_sockets.extend(subsys_list["__net_socket"])

def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("-k", "--kernel", required=False,
                        help="Input zephyr ELF binary")
    parser.add_argument(
        "-g", "--gperf-output", required=False,
        help="Output list of kernel object addresses for gperf use")
    parser.add_argument(
        "-V", "--validation-output", required=False,
        help="Output driver validation macros")
    parser.add_argument(
        "-K", "--kobj-types-output", required=False,
        help="Output k_object enum constants")
    parser.add_argument(
        "-S", "--kobj-otype-output", required=False,
        help="Output case statements for otype_to_str()")
    parser.add_argument(
        "-Z", "--kobj-size-output", required=False,
        help="Output case statements for obj_size_get()")
    parser.add_argument("-i", "--include-subsystem-list", required=False, action='append',
        help='''Specifies a file with a JSON encoded list of subsystem names to append to
        the driver subsystems list. Can be specified multiple times:
        -i file1 -i file2 ...''')

    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print extra debugging information")
    args = parser.parse_args()
    if "VERBOSE" in os.environ:
        args.verbose = 1


def main():
    parse_args()

    if args.include_subsystem_list is not None:
        for list_file in args.include_subsystem_list:
            parse_subsystems_list_file(list_file)

    if args.gperf_output:
        assert args.kernel, "--kernel ELF required for --gperf-output"
        elf = ELFFile(open(args.kernel, "rb"))
        syms = get_symbols(elf)
        max_threads = syms["CONFIG_MAX_THREAD_BYTES"] * 8
        objs = find_kobjects(elf, syms)
        if not objs:
            sys.stderr.write("WARNING: zero kobject found in %s\n"
                             % args.kernel)

        if thread_counter > max_threads:
            sys.exit("Too many thread objects ({})\n"
                     "Increase CONFIG_MAX_THREAD_BYTES to {}"
                     .format(thread_counter, -(-thread_counter // 8)))

        with open(args.gperf_output, "w") as fp:
            write_gperf_table(fp, syms, objs, elf.little_endian,
                              syms["_static_kernel_objects_begin"],
                              syms["_static_kernel_objects_end"])

    if args.validation_output:
        with open(args.validation_output, "w") as fp:
            write_validation_output(fp)

    if args.kobj_types_output:
        with open(args.kobj_types_output, "w") as fp:
            write_kobj_types_output(fp)

    if args.kobj_otype_output:
        with open(args.kobj_otype_output, "w") as fp:
            write_kobj_otype_output(fp)

    if args.kobj_size_output:
        with open(args.kobj_size_output, "w") as fp:
            write_kobj_size_output(fp)


if __name__ == "__main__":
    main()
