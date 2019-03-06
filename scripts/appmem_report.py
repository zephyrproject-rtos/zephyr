#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Userspace memory partition information script

For all memory partitions declared with K_APPMEM_PARTITION_DEFINE()
and populated with data at build time using K_APP_DMEM/K_APP_BMEM
macros, show how much data is in each partition.

We also show how much space is wasted due to memory partition size
or alignment constraints; for example, on a system that requires
partitions to be both sized and aligned to a power-of-two,
how much memory each partition wastes. We also show how much
memory is wasted due to gaps between the partitions.
"""

import sys
import subprocess
import re
import argparse

start_regex = re.compile(r'z_data_smem_(.*)_part_start')
end_regex = re.compile(r'z_data_smem_(.*)_part_end')
bss_end_regex = re.compile(r'z_data_smem_(.*)_bss_end')

starts = {}
ends = {}
bss_ends = {}
parts = []

parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("kernel", help="kernel binary")
args = parser.parse_args()

data = subprocess.Popen(["nm", "-n", args.kernel], stdout=subprocess.PIPE)

def check_regex(regex, addr, text, d):
    x = regex.match(text)
    if not x:
        return None
    part = x.groups()[0]
    d[part] = addr
    return part

for line in data.stdout.readlines():
    addr, _, sym = line.split()
    addr = int(addr, 16)
    sym = sym.decode("UTF-8")

    part_name = check_regex(start_regex, addr, sym, starts)
    if part_name and part_name not in parts:
        parts.append(part_name)


    check_regex(end_regex, addr, sym, ends)
    check_regex(bss_end_regex, addr, sym, bss_ends)


print("Userspace memory partition report:")
print("Partition                     Total       Used        Wasted      Efficiency")
print("----------------------------------------------------------------------------")

total_wasted = 0
total_data = 0
total_used = 0

for i in range(len(parts)):
    k = parts[i]
    total_size = ends[k] - starts[k]
    total_used += total_size
    data_size = bss_ends[k] - starts[k]
    total_data += data_size
    padding = ends[k] - bss_ends[k]
    total_wasted += padding
    used = (float(data_size) / float(total_size))

    if (i > 0):
        gap = starts[parts[i]] - ends[parts[i - 1]]
        print("  gap {0:12}".format(hex(gap)))
        total_wasted += gap
        total_used += gap

    print("{0:30}{1:12}{2:12}{3:12}{4:.2%}".format(k, hex(total_size),
        hex(data_size), hex(padding), used))

print("\n{0:30}{1:12}{2:12}{3:12}{4:.2%}".format("total", hex(total_used),
        hex(total_data), hex(total_wasted), float(total_data) / float(total_used)))


