#!/usr/bin/env python3
#
# Copyright (c) 2021 ASPEED Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

import sys

if __name__ == "__main__":
    src = sys.argv[1]
    dst = sys.argv[2]

    with open(src,'rb') as r:
        src_image = r.read()

    # get src size and round up to 4-byte align
    src_len = len(src_image)
    src_len_align = (src_len + 3) & (~3)
    src_len_byte = src_len_align.to_bytes(4, byteorder='little', signed=False)

    dst_image = src_len_byte + src_image

    with open(dst, 'w+b') as w:
        w.write(dst_image)
