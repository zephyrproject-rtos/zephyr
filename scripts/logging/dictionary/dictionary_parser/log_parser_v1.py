#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Dictionary-based Logging Parser Version 1

This contains the implementation of the parser for
version 1 databases.
"""

import logging
import math
import struct
import colorama
from colorama import Fore

from .log_parser import (LogParser, get_log_level_str_color, formalize_fmt_string)
from .data_types import DataTypes


HEX_BYTES_IN_LINE = 16

# Need to keep sync with struct log_dict_output_msg_hdr in
# include/logging/log_output_dict.h.
#
# struct log_dict_output_normal_msg_hdr_t {
#     uint8_t type;
#     uint32_t domain:3;
#     uint32_t level:3;
#     uint32_t package_len:10;
#     uint32_t data_len:12;
#     uintptr_t source;
#     log_timestamp_t timestamp;
# } __packed;
#
# Note "type" and "timestamp" are encoded separately below.
FMT_MSG_HDR_32 = "II"
FMT_MSG_HDR_64 = "IQ"

# Message type
# 0: normal message
# 1: number of dropped messages
FMT_MSG_TYPE = "B"

# Depends on CONFIG_LOG_TIMESTAMP_64BIT
FMT_MSG_TIMESTAMP_32 = "I"
FMT_MSG_TIMESTAMP_64 = "Q"

# Keep message types in sync with include/logging/log_output_dict.h
MSG_TYPE_NORMAL = 0
MSG_TYPE_DROPPED = 1

# Number of dropped messages
FMT_DROPPED_CNT = "H"


logger = logging.getLogger("parser")


class LogParserV1(LogParser):
    """Log Parser V1"""
    def __init__(self, database):
        super().__init__(database=database)

        if self.database.is_tgt_little_endian():
            endian = "<"
        else:
            endian = ">"

        self.fmt_msg_type = endian + FMT_MSG_TYPE
        self.fmt_dropped_cnt = endian + FMT_DROPPED_CNT

        if self.database.is_tgt_64bit():
            self.fmt_msg_hdr = endian + FMT_MSG_HDR_64
        else:
            self.fmt_msg_hdr = endian + FMT_MSG_HDR_32

        if "CONFIG_LOG_TIMESTAMP_64BIT" in self.database.get_kconfigs():
            self.fmt_msg_timestamp = endian + FMT_MSG_TIMESTAMP_64
        else:
            self.fmt_msg_timestamp = endian + FMT_MSG_TIMESTAMP_32


    def __get_string(self, arg, arg_offset, string_tbl):
        one_str = self.database.find_string(arg)
        if one_str is not None:
            ret = one_str
        else:
            # The index from the string table is basically
            # the order in va_list. Need to add to the index
            # to skip the packaged string header and
            # the format string.
            str_idx = arg_offset + self.data_types.get_sizeof(DataTypes.PTR) * 2
            str_idx /= self.data_types.get_sizeof(DataTypes.INT)

            if int(str_idx) not in string_tbl:
                ret = "<string@0x{0:x}>".format(arg)
            else:
                ret = string_tbl[int(str_idx)]

        return ret


    def process_one_fmt_str(self, fmt_str, arg_list, string_tbl):
        """Parse the format string to extract arguments from
        the binary arglist and return a tuple usable with
        Python's string formatting"""
        idx = 0
        arg_offset = 0
        arg_data_type = None
        is_parsing = False
        do_extract = False

        args = []

        # Translated from cbvprintf_package()
        for idx, fmt in enumerate(fmt_str):
            if not is_parsing:
                if fmt == '%':
                    is_parsing = True
                    arg_data_type = DataTypes.INT
                    continue

            elif fmt == '%':
                # '%%' -> literal percentage sign
                is_parsing = False
                continue

            elif fmt == '*':
                pass

            elif fmt.isdecimal() or str.lower(fmt) == 'l' \
                or fmt in (' ', '#', '-', '+', '.', 'h'):
                # formatting modifiers, just ignore
                continue

            elif fmt in ('j', 'z', 't'):
                # intmax_t, size_t or ptrdiff_t
                arg_data_type = DataTypes.LONG

            elif fmt in ('c', 'd', 'i', 'o', 'u', 'x', 'X'):
                unsigned = fmt in ('c', 'o', 'u', 'x', 'X')

                if fmt_str[idx - 1] == 'l':
                    if fmt_str[idx - 2] == 'l':
                        arg_data_type = DataTypes.ULONG_LONG if unsigned else DataTypes.LONG_LONG
                    else:
                        arg_data_type = DataTypes.ULONG if unsigned else DataTypes.LONG
                else:
                    arg_data_type = DataTypes.UINT if unsigned else DataTypes.INT

                is_parsing = False
                do_extract = True

            elif fmt in ('s', 'p', 'n'):
                arg_data_type = DataTypes.PTR

                is_parsing = False
                do_extract = True

            elif str.lower(fmt) in ('a', 'e', 'f', 'g'):
                # Python doesn't do"long double".
                #
                # Parse it as double (probably incorrect), but
                # still have to skip enough bytes.
                if fmt_str[idx - 1] == 'L':
                    arg_data_type = DataTypes.LONG_DOUBLE
                else:
                    arg_data_type = DataTypes.DOUBLE

                is_parsing = False
                do_extract = True

            else:
                is_parsing = False
                continue

            if do_extract:
                do_extract = False

                align = self.data_types.get_alignment(arg_data_type)
                size = self.data_types.get_sizeof(arg_data_type)
                unpack_fmt = self.data_types.get_formatter(arg_data_type)

                # Align the argument list by rounding up
                stack_align = self.data_types.get_stack_alignment(arg_data_type)
                if stack_align > 1:
                    arg_offset = int((arg_offset + (align - 1)) / align) * align

                one_arg = struct.unpack_from(unpack_fmt, arg_list, arg_offset)[0]

                if fmt == 's':
                    one_arg = self.__get_string(one_arg, arg_offset, string_tbl)

                args.append(one_arg)
                arg_offset += size

                # Align the offset
                if stack_align > 1:
                    arg_offset = int((arg_offset + align - 1) / align) * align

        return tuple(args)


    @staticmethod
    def extract_string_table(str_tbl):
        """Extract string table in a packaged log message"""
        tbl = {}

        one_str = ""
        next_new_string = True
        # Translated from cbvprintf_package()
        for one_ch in str_tbl:
            if next_new_string:
                str_idx = one_ch
                next_new_string = False
                continue

            if one_ch == 0:
                tbl[str_idx] = one_str
                one_str = ""
                next_new_string = True
                continue

            one_str += chr(one_ch)

        return tbl


    @staticmethod
    def print_hexdump(hex_data, prefix_len, color):
        """Print hex dump"""
        hex_vals = ""
        chr_vals = ""
        chr_done = 0

        for one_hex in hex_data:
            hex_vals += "%x " % one_hex
            chr_vals += chr(one_hex)
            chr_done += 1

            if chr_done == HEX_BYTES_IN_LINE / 2:
                hex_vals += " "
                chr_vals += " "

            elif chr_done == HEX_BYTES_IN_LINE:
                print(f"{color}%s%s|%s{Fore.RESET}" % ((" " * prefix_len),
                      hex_vals, chr_vals))
                hex_vals = ""
                chr_vals = ""
                chr_done = 0

        if len(chr_vals) > 0:
            hex_padding = "   " * (HEX_BYTES_IN_LINE - chr_done)
            print(f"{color}%s%s%s|%s{Fore.RESET}" % ((" " * prefix_len),
                  hex_vals, hex_padding, chr_vals))


    def parse_one_normal_msg(self, logdata, offset):
        """Parse one normal log message and print the encoded message"""
        # Parse log message header
        log_desc, source_id = struct.unpack_from(self.fmt_msg_hdr, logdata, offset)
        offset += struct.calcsize(self.fmt_msg_hdr)

        timestamp = struct.unpack_from(self.fmt_msg_timestamp, logdata, offset)[0]
        offset += struct.calcsize(self.fmt_msg_timestamp)

        # domain_id, level, pkg_len, data_len
        domain_id = log_desc & 0x07
        level = (log_desc >> 3) & 0x07
        pkg_len = (log_desc >> 6) & int(math.pow(2, 10) - 1)
        data_len = (log_desc >> 16) & int(math.pow(2, 12) - 1)

        level_str, color = get_log_level_str_color(level)
        source_id_str = self.database.get_log_source_string(domain_id, source_id)

        # Skip over data to point to next message (save as return value)
        next_msg_offset = offset + pkg_len + data_len

        # Offset from beginning of cbprintf_packaged data to end of va_list arguments
        offset_end_of_args = struct.unpack_from("B", logdata, offset)[0]
        offset_end_of_args *= self.data_types.get_sizeof(DataTypes.INT)
        offset_end_of_args += offset

        # Extra data after packaged log
        extra_data = logdata[(offset + pkg_len):next_msg_offset]

        # Number of appended strings in package
        num_packed_strings = struct.unpack_from("B", logdata, offset+1)[0]

        # Number of read-only string indexes
        num_ro_str_indexes = struct.unpack_from("B", logdata, offset+2)[0]
        offset_end_of_args += num_ro_str_indexes

        # Number of read-write string indexes
        num_rw_str_indexes = struct.unpack_from("B", logdata, offset+3)[0]
        offset_end_of_args += num_rw_str_indexes

        # Extract the string table in the packaged log message
        string_tbl = self.extract_string_table(logdata[offset_end_of_args:(offset + pkg_len)])

        if len(string_tbl) != num_packed_strings:
            logger.error("------ Error extracting string table")
            return None

        # Skip packaged string header
        offset += self.data_types.get_sizeof(DataTypes.PTR)

        # Grab the format string
        #
        # Note the negative offset to __get_string(). It is because
        # the offset begins at 0 for va_list. However, the format string
        # itself is before the va_list, so need to go back the width of
        # a pointer.
        fmt_str_ptr = struct.unpack_from(self.data_types.get_formatter(DataTypes.PTR),
                                         logdata, offset)[0]
        fmt_str = self.__get_string(fmt_str_ptr,
                                    -self.data_types.get_sizeof(DataTypes.PTR),
                                    string_tbl)
        offset += self.data_types.get_sizeof(DataTypes.PTR)

        if not fmt_str:
            logger.error("------ Error getting format string at 0x%x", fmt_str_ptr)
            return None

        args = self.process_one_fmt_str(fmt_str, logdata[offset:offset_end_of_args], string_tbl)

        fmt_str = formalize_fmt_string(fmt_str)
        log_msg = fmt_str % args

        if level == 0:
            print(f"{log_msg}", end='')
        else:
            log_prefix = f"[{timestamp:>10}] <{level_str}> {source_id_str}: "
            print(f"{color}%s%s{Fore.RESET}" % (log_prefix, log_msg))

        if data_len > 0:
            # Has hexdump data
            self.print_hexdump(extra_data, len(log_prefix), color)

        # Point to next message
        return next_msg_offset


    def parse_log_data(self, logdata, debug=False):
        """Parse binary log data and print the encoded log messages"""
        offset = 0

        while offset < len(logdata):
            # Get message type
            msg_type = struct.unpack_from(self.fmt_msg_type, logdata, offset)[0]
            offset += struct.calcsize(self.fmt_msg_type)

            if msg_type == MSG_TYPE_DROPPED:
                num_dropped = struct.unpack_from(self.fmt_dropped_cnt, logdata, offset)
                offset += struct.calcsize(self.fmt_dropped_cnt)

                print(f"--- {num_dropped} messages dropped ---")

            elif msg_type == MSG_TYPE_NORMAL:
                ret = self.parse_one_normal_msg(logdata, offset)
                if ret is None:
                    return False

                offset = ret

            else:
                logger.error("------ Unknown message type: %s", msg_type)
                return False

        return True

colorama.init()
