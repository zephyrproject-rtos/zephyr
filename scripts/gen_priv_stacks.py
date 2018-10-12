#!/usr/bin/env python3
#
# Copyright (c) 2017 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0

import sys
import argparse
import struct
from elf_helper import ElfHelper

kobjects = [
        "k_stack",
        "_k_thread_stack_element",
        ]


header = """%compare-lengths
%define lookup-function-name _k_priv_stack_map_lookup
%language=ANSI-C
%global-table
%struct-type
"""


priv_stack_decl_temp = ("static u8_t __used"
                        " __aligned(CONFIG_PRIVILEGED_STACK_SIZE)"
                        " priv_stack_%x[CONFIG_PRIVILEGED_STACK_SIZE];\n")
priv_stack_decl_size = "CONFIG_PRIVILEGED_STACK_SIZE"


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
u8_t *_k_priv_stack_find(void *obj)
{
    const struct _k_priv_stack_map *map =
        _k_priv_stack_map_lookup((const char *)obj, sizeof(void *));
    return map->priv_stack_addr;
}
"""


def write_gperf_table(fp, eh, objs, static_begin, static_end):
    fp.write(header)

    # priv stack declarations
    fp.write("%{\n")
    fp.write(includes)
    for obj_addr, ko in objs.items():
        fp.write(priv_stack_decl_temp % (obj_addr))
    fp.write("%}\n")

    # structure declaration
    fp.write(structure)

    for obj_addr, ko in objs.items():
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

    thread_counter = eh.get_thread_counter()
    if thread_counter > max_threads:
        sys.stderr.write("Too many thread objects (%d)\n" % thread_counter)
        sys.stderr.write("Increase CONFIG_MAX_THREAD_BYTES to %d\n",
                         -(-thread_counter // 8))
        sys.exit(1)

    with open(args.output, "w") as fp:
        write_gperf_table(fp, eh, objs, syms["_static_kernel_objects_begin"],
                          syms["_static_kernel_objects_end"])


if __name__ == "__main__":
    main()
