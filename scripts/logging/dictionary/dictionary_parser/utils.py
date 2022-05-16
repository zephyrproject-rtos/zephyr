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

    with open(hexfile, "r", encoding="iso-8859-1") as hfile:
        for line in hfile.readlines():
            hex_str = line.strip()

            bin_str = binascii.unhexlify(hex_str)
            bin_data += bin_str

    return bin_data


def extract_one_string_in_section(section, str_ptr):
    """Extract one string in an ELF section"""
    data = section['data']
    max_offset = section['size']
    offset = str_ptr - section['start']

    if offset < 0 or offset >= max_offset:
        return None

    ret_str = ""

    while (offset < max_offset) and (data[offset] != 0):
        ret_str += chr(data[offset])
        offset += 1

    return ret_str


def find_string_in_mappings(string_mappings, str_ptr):
    """
    Find string pointed by string_ptr in the string mapping
    list. Return None if not found.
    """
    if string_mappings is None:
        return None

    if len(string_mappings) == 0:
        return None

    if str_ptr in string_mappings:
        return string_mappings[str_ptr]

    # No direct match on pointer value.
    # This may be a combined string. So check for that.
    for ptr, string in string_mappings.items():
        if ptr <= str_ptr < (ptr + len(string)):
            whole_str = string_mappings[ptr]
            return whole_str[str_ptr - ptr:]

    return None
