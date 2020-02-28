#!/usr/bin/env python3
#
# Copyright (c) 2017 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0

"""
Script to generate gperf tables mapping threads to their privileged mode stacks

Some MPU devices require that memory region definitions be aligned to their
own size, which must be a power of two. This introduces difficulties in
reserving memory for the thread's supervisor mode stack inline with the
K_THREAD_STACK_DEFINE() macro.

Instead, the stack used when a user thread elevates privileges is allocated
elsewhere in memory, and a gperf table is created to be able to quickly
determine where the supervisor mode stack is in memory. This is accomplished
by scanning the DWARF debug information in zephyr_prebuilt.elf, identifying
instances of 'struct k_thread', and emitting a gperf configuration file which
allocates memory for each thread's privileged stack and creates the table
mapping thread addresses to these stacks.
"""

import sys
import argparse
import struct
from elf_helper import ElfHelper

kobjects = {
        "_k_thread_stack_element": (None, False)
}


header = """%compare-lengths
%define lookup-function-name z_priv_stack_map_lookup
%language=ANSI-C
%global-table
%struct-type
"""


# Each privilege stack buffer needs to respect the alignment
# constraints as specified in arm/arch.h.
priv_stack_decl_temp = ("static u8_t __used"
                        " __aligned(Z_PRIVILEGE_STACK_ALIGN)"
                        " priv_stack_%x[CONFIG_PRIVILEGED_STACK_SIZE];\n")


includes = """#include <kernel.h>
#include <string.h>
"""


structure = """struct _k_priv_stack_map {
    char *name;
    u8_t *priv_stack_addr;
};
%%
"""


# Different versions of gperf have different prototypes for the lookup
# function, best to implement the wrapper here. The pointer value itself is
# turned into a string, we told gperf to expect binary strings that are not
# NULL-terminated.
footer = """%%
u8_t *z_priv_stack_find(void *obj)
{
    const struct _k_priv_stack_map *map =
        z_priv_stack_map_lookup((const char *)obj, sizeof(void *));
    return map->priv_stack_addr;
}
"""


def write_gperf_table(fp, eh, objs):
    fp.write(header)

    # priv stack declarations
    fp.write("%{\n")
    fp.write(includes)
    for obj_addr in objs:
        fp.write(priv_stack_decl_temp % (obj_addr))
    fp.write("%}\n")

    # structure declaration
    fp.write(structure)

    for obj_addr in objs:
        byte_str = struct.pack("<I" if eh.little_endian else ">I", obj_addr)
        fp.write("\"")
        for byte in byte_str:
            val = "\\x%02x" % byte
            fp.write(val)

        fp.write("\",priv_stack_%x\n" % obj_addr)

    fp.write(footer)


def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-k", "--kernel", required=True,
                        help="Input zephyr ELF binary")
    parser.add_argument(
        "-o", "--output", required=True,
        help="Output list of kernel object addresses for gperf use")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print extra debugging information")
    args = parser.parse_args()


def main():
    parse_args()

    eh = ElfHelper(args.kernel, args.verbose, kobjects, [])
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

    with open(args.output, "w") as fp:
        write_gperf_table(fp, eh, objs)


if __name__ == "__main__":
    main()
