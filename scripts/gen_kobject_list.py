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
from elf_helper import ElfHelper, kobject_to_enum

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

# Regular dictionaries are ordered only with Python 3.6 and
# above. Good summary and pointers to official documents at:
# https://stackoverflow.com/questions/39980323/are-dictionaries-ordered-in-python-3-6
kobjects = OrderedDict([
    ("k_mem_slab", (None, False)),
    ("k_msgq", (None, False)),
    ("k_mutex", (None, False)),
    ("k_pipe", (None, False)),
    ("k_queue", (None, False)),
    ("k_poll_signal", (None, False)),
    ("k_sem", (None, False)),
    ("k_stack", (None, False)),
    ("k_thread", (None, False)),
    ("k_timer", (None, False)),
    ("z_thread_stack_element", (None, False)),
    ("device", (None, False)),
    ("sys_mutex", (None, True)),
    ("k_futex", (None, True))
])

subsystems = [
    # Editing the list is deprecated, add the __subsystem sentinal to your driver
    # api declaration instead. e.x.
    #
    # __subsystem struct my_driver_api {
    #    ....
    #};
]

header = """%compare-lengths
%define lookup-function-name z_object_lookup
%language=ANSI-C
%global-table
%struct-type
%{
#include <kernel.h>
#include <toolchain.h>
#include <syscall_handler.h>
#include <string.h>
%}
struct z_object;
"""

# Different versions of gperf have different prototypes for the lookup
# function, best to implement the wrapper here. The pointer value itself is
# turned into a string, we told gperf to expect binary strings that are not
# NULL-terminated.
footer = """%%
struct z_object *z_object_gperf_find(void *obj)
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
struct z_object *z_object_find(void *obj)
	ALIAS_OF(z_object_gperf_find);

void z_object_wordlist_foreach(_wordlist_cb_func_t func, void *context)
	ALIAS_OF(z_object_gperf_wordlist_foreach);
#endif
"""


def write_gperf_table(fp, eh, objs, static_begin, static_end):
    syms = eh.get_symbols()

    fp.write(header)
    num_mutexes = eh.get_sys_mutex_counter()
    if num_mutexes != 0:
        fp.write("static struct k_mutex kernel_mutexes[%d] = {\n" % num_mutexes)
        for i in range(num_mutexes):
            fp.write("_K_MUTEX_INITIALIZER(kernel_mutexes[%d])" % i)
            if i != num_mutexes - 1:
                fp.write(", ")
        fp.write("};\n")

    num_futex = eh.get_futex_counter()
    if num_futex != 0:
        fp.write("static struct z_futex_data futex_data[%d] = {\n" % num_futex)
        for i in range(num_futex):
            fp.write("Z_FUTEX_DATA_INITIALIZER(futex_data[%d])" % i)
            if i != num_futex - 1:
                fp.write(", ")
        fp.write("};\n")

    metadata_names = {
        "K_OBJ_THREAD" : "thread_id",
        "K_OBJ_SYS_MUTEX" : "mutex",
        "K_OBJ_FUTEX" : "futex_data"
    }

    if "CONFIG_GEN_PRIV_STACKS" in syms:
        metadata_names["K_OBJ_THREAD_STACK_ELEMENT"] = "stack_data"
        num_stack = eh.get_stack_counter()
        if num_stack != 0:
            fp.write("static u8_t Z_GENERIC_SECTION(.priv_stacks.noinit) "
                     " __aligned(Z_PRIVILEGE_STACK_ALIGN)"
                     " priv_stacks[%d][CONFIG_PRIVILEGED_STACK_SIZE];\n"
                     % num_stack);

            fp.write("static struct z_stack_data stack_data[%d] = {\n"
                     % num_stack)
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
                fp.write("\t{ %d, (u8_t *)(&priv_stacks[%d]) }"
                         % (size, counter))
                if counter != (num_stack - 1):
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

        if eh.little_endian:
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

        fp.write("\", {}, %s, %s, { .%s = %s }\n" % (obj_type, flags,
		tname, str(ko.data)))

        if obj_type == "K_OBJ_THREAD":
            idx = math.floor(ko.data / 8)
            bit = ko.data % 8
            thread_idx_map[idx] = thread_idx_map[idx] & ~(2**bit)

    fp.write(footer)

    # Generate the array of already mapped thread indexes
    fp.write('\n')
    fp.write('Z_GENERIC_SECTION(.kobject_data.data) ')
    fp.write('u8_t _thread_idx_map[%d] = {' % (thread_max_bytes))

    for i in range(0, thread_max_bytes):
        fp.write(' 0x%x, ' % (thread_idx_map[i]))

    fp.write('};\n')


driver_macro_tpl = """
#define Z_SYSCALL_DRIVER_%(driver_upper)s(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, %(driver_lower)s, %(driver_upper)s)
"""


def write_validation_output(fp):
    fp.write("#ifndef DRIVER_VALIDATION_GEN_H\n")
    fp.write("#define DRIVER_VALIDATION_GEN_H\n")

    fp.write("""#define Z_SYSCALL_DRIVER_GEN(ptr, op, driver_lower_case, driver_upper_case) \\
		(Z_SYSCALL_OBJ(ptr, K_OBJ_DRIVER_##driver_upper_case) || \\
		 Z_SYSCALL_DRIVER_OP(ptr, driver_lower_case##_driver_api, op))
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
        dep, _ = obj_info
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
        dep, _ = obj_info
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
        dep, _ = obj_info
        # device handled by default case. Stacks are not currently handled,
        # if they eventually are it will be a special case.
        if kobj in {"device", "z_thread_stack_element"}:
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
    subsystems.extend(subsys_list)

def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

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
        eh = ElfHelper(args.kernel, args.verbose, kobjects, subsystems)
        syms = eh.get_symbols()
        max_threads = syms["CONFIG_MAX_THREAD_BYTES"] * 8
        objs = eh.find_kobjects(syms)
        if not objs:
            sys.stderr.write("WARNING: zero kobject found in %s\n"
                             % args.kernel)

        thread_counter = eh.get_thread_counter()
        if thread_counter > max_threads:
            sys.exit("Too many thread objects ({})\n"
                     "Increase CONFIG_MAX_THREAD_BYTES to {}"
                     .format(thread_counter, -(-thread_counter // 8)))

        with open(args.gperf_output, "w") as fp:
            write_gperf_table(fp, eh, objs,
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
