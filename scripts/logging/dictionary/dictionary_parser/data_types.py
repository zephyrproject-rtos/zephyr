#!/usr/bin/env python3
#
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Contains a class to describe data types used for
dictionary logging.
"""

import struct

class DataTypes():
    """Class regarding data types, their alignments and sizes"""
    INT = 0
    UINT = 1
    LONG = 2
    ULONG = 3
    LONG_LONG = 4
    ULONG_LONG = 5
    PTR = 6
    DOUBLE = 7
    LONG_DOUBLE = 8
    NUM_TYPES = 9

    def __init__(self, database):
        self.database = database
        self.data_types = {}

        if database.is_tgt_64bit():
            self.add_data_type(self.LONG, "q")
            self.add_data_type(self.LONG_LONG, "q")
            self.add_data_type(self.PTR, "Q")
        else:
            self.add_data_type(self.LONG, "i")
            self.add_data_type(self.LONG_LONG, "q")
            self.add_data_type(self.PTR, "I")

        self.add_data_type(self.INT, "i")
        self.add_data_type(self.DOUBLE, "d")
        self.add_data_type(self.LONG_DOUBLE, "d")


    @staticmethod
    def get_stack_min_align(arch, is_tgt_64bit):
        '''
        Correspond to the VA_STACK_ALIGN and VA_STACK_MIN_ALIGN
        in cbprintf_internal.h. Note that there might be some
	variations that is obtained via actually running through
	the log parser.

        Return a tuple where the first element is stack alignment
        value. The second element is true if alignment needs to
        be further refined according to data type, false if not.
        '''
        if arch == "arc":
            if is_tgt_64bit:
                need_further_align = True
                stack_min_align = 8
            else:
                need_further_align = False
                stack_min_align = 1

        elif arch == "arm64":
            need_further_align = True
            stack_min_align = 8

        elif arch == "sparc":
            need_further_align = False
            stack_min_align = 1

        elif arch == "x86":
            if is_tgt_64bit:
                need_further_align = True
                stack_min_align = 8
            else:
                need_further_align = False
                stack_min_align = 1

        elif arch == "riscv32e":
            need_further_align = False
            stack_min_align = 1

        elif arch == "riscv":
            need_further_align = True

            if is_tgt_64bit:
                stack_min_align = 8
            else:
                stack_min_align = 1

        elif arch == "nios2":
            need_further_align = False
            stack_min_align = 1

        else:
            need_further_align = True
            stack_min_align = 1

        return (stack_min_align, need_further_align)


    @staticmethod
    def get_data_type_align(data_type, is_tgt_64bit):
        '''
        Get the alignment for a particular data type.
        '''
        if data_type == DataTypes.LONG_LONG:
            align = 8
        elif data_type == DataTypes.LONG:
            if is_tgt_64bit:
                align = 8
            else:
                align = 4
        else:
            # va_list alignment is at least a integer
            align = 4

        return align


    def add_data_type(self, data_type, fmt):
        """Add one data type"""
        if self.database.is_tgt_little_endian():
            endianness = "<"
        else:
            endianness = ">"

        formatter = endianness + fmt

        self.data_types[data_type] = {}
        self.data_types[data_type]['fmt'] = formatter

        size = struct.calcsize(formatter)

        if data_type == self.LONG_DOUBLE:
            # Python doesn't have long double but we still
            # need to skip correct number of bytes
            size = 16

        self.data_types[data_type]['sizeof'] = size

        # Might need actual number for different architectures
        # but these seem to work fine for now.
        if self.database.is_tgt_64bit():
            align = 8
        else:
            align = 4

        # 'align' is used to "jump" over an argument so it has
        # to be at least size of the data type.
        align = max(align, size)
        self.data_types[data_type]['align'] = align

        # 'stack_align' should correspond to VA_STACK_ALIGN
        # in cbprintf_internal.h
        stack_align, need_more_align = DataTypes.get_stack_min_align(
                                        self.database.get_arch(),
                                        self.database.is_tgt_64bit())

        if need_more_align:
            stack_align = DataTypes.get_data_type_align(data_type,
                                                        self.database.is_tgt_64bit())

        self.data_types[data_type]['stack_align'] = stack_align


    def get_sizeof(self, data_type):
        """Get sizeof() of a data type"""
        return self.data_types[data_type]['sizeof']


    def get_alignment(self, data_type):
        """Get the alignment of a data type"""
        return self.data_types[data_type]['align']


    def get_stack_alignment(self, data_type):
        """Get the stack alignment of a data type"""
        return self.data_types[data_type]['stack_align']


    def get_formatter(self, data_type):
        """Get the formatter for a data type"""
        return self.data_types[data_type]['fmt']
