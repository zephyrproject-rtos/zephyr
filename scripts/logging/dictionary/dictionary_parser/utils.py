#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Utilities for Dictionary-based Logging Parser
"""

import binascii


def convert_hex_file_to_bin(hexfile):
    """This converts a file in hexadecimal to binary"""
    bin_data = b''

    with open(hexfile, "r") as hfile:
        for line in hfile.readlines():
            hex_str = line.strip()

            bin_str = binascii.unhexlify(hex_str)
            bin_data += bin_str

    return bin_data


def extract_string_from_section(section, str_ptr):
    """Extract all the strings in an ELF section"""
    data = section['data']
    max_offset = section['size']
    offset = str_ptr - section['start']

    if offset < 0 or offset >= max_offset:
        return None

    ret_str = ""

    while (data[offset] != 0) and (offset < max_offset):
        ret_str += chr(data[offset])
        offset += 1

    return ret_str
