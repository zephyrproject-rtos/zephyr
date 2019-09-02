#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sathish Kuttan <sathish.k.kuttan@intel.com>

# This file defines a message class that contains functions to create
# commands to the target and to parse responses from the target.

import bitstruct

class Message:
    """
    Message class containing the methods to create command messages and
    parse response messages.
    """
    message_id = {1: 'Control'}
    cmd_rsp = {2: 'Load Firmware',
            4: 'Mode Select',
            0x10: 'Memory Read',
            0x11: 'Memory Write',
            0x12: 'Memory Block Write',
            0x13: 'Execute',
            0x14: 'Wait',
            0x20: 'Ready'}
    tx_data = None
    tx_bulk_data = None
    tx_index = 0
    cmd_word_fmt = 'u1 u1 u1 u5 u16 u8'
    cmd_keys = ['cmd', 'rsvd1', 'rsp', 'msg_id', 'rsvd2', 'cmd_rsp']

    def __init__(self):
        """
        Intialize a byte array of 64 bytes for command messages
        Intialize another byte array of 4096 bytes for bulk messages
        """
        self.tx_data = bytearray(64)
        self.tx_bulk_data = bytearray(4096)

    def init_tx_data(self):
        """
        Intialize transmit message buffers to zeros
        """
        for index in range(len(self.tx_data)):
            self.tx_data[index] = 0
        self.tx_index = 0

    @staticmethod
    def endian_swap(dst, dst_offset, src):
        """
        Performs a byte swap of a 32-bit word to change it's endianness
        """
        for index in range(0, len(src), 4):
            dst[dst_offset + index + 0] = src[index +  3]
            dst[dst_offset + index + 1] = src[index +  2]
            dst[dst_offset + index + 2] = src[index +  1]
            dst[dst_offset + index + 3] = src[index +  0]

    def print_cmd_message(self):
        """
        Prints the contents of the command message buffer
        """
        for index in range(0, self.tx_index, 4):
            offset = index * 8
            word = bitstruct.unpack_from('u32', self.tx_data, offset)
            print('Index: %2d Content: 0x%08x' %(index, word[0]))

    def print_response(self, msg, verbose=False):
        """
        Parses and prints the contents of the response message
        """
        unpacked = bitstruct.unpack_from_dict(self.cmd_word_fmt,
                self.cmd_keys, msg)
        msg_id = unpacked['msg_id']
        rsp = unpacked['cmd_rsp']
        if msg_id == 0 and rsp == 0:
            print('RSP <<< NULL.')
        else:
            print('RSP <<< %s.' % self.cmd_rsp[rsp])
            if verbose:
                count = bitstruct.unpack_from('u32', msg, 4 * 8)[0]
                count &= 0x1ff
                for index in range(0, 8 + (count * 4), 4):
                    offset = index * 8
                    word = bitstruct.unpack_from('u32', msg, offset)
                    print('Index: %2d Content: 0x%08x' %(index, word[0]))

    def get_cmd_code(self, cmd):
        """
        Looks up the command and returns the numeric code
        """
        index = list(self.cmd_rsp.values()).index(cmd)
        return list(self.cmd_rsp.keys())[index]

    def print_cmd_code(self, cmd):
        """
        Prints the numeric code for the given command
        """
        key = self.get_cmd_code(cmd)
        print('CMD >>> %s. Command Code: 0x%02x' % (cmd, key))

    def create_null_cmd(self):
        """
        Creates a NULL command
        """
        print('CMD >>> NULL.')
        for index in range(len(self.tx_data)):
            self.tx_data[index] = 0
        self.tx_index = len(self.tx_data)
        return self.tx_data

    def create_memwrite_cmd(self, tuple):
        """
        Creates a memory write command with memory address and value pairs
        """
        cmd = 'Memory Write'
        print('CMD >>> %s.' % cmd)
        code = self.get_cmd_code(cmd)
        self.init_tx_data()

        index = list(self.message_id.values()).index('Control')
        msg_id = list(self.message_id.keys())[index]
        bitstruct.pack_into_dict(self.cmd_word_fmt, self.cmd_keys,
                self.tx_data, 0, {'cmd': 1, 'rsvd1': 0, 'rsp':  0,
                'msg_id': msg_id, 'rsvd2': 0, 'cmd_rsp': code})
        self.tx_index += 4
        bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8,
                len(tuple))
        self.tx_index += 4
        for elm in tuple:
            bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8, elm)
            self.tx_index += 4
        return self.tx_data

    def create_memread_cmd(self, tuple):
        """
        Creates a memory read command with memory addresses
        """
        cmd = 'Memory Read'
        print('CMD >>> %s.' % cmd)
        code = self.get_cmd_code(cmd)
        self.init_tx_data()

        index = list(self.message_id.values()).index('Control')
        msg_id = list(self.message_id.keys())[index]
        bitstruct.pack_into_dict(self.cmd_word_fmt, self.cmd_keys,
                self.tx_data, 0, {'cmd': 1, 'rsvd1': 0, 'rsp':  0,
                'msg_id': msg_id, 'rsvd2': 0, 'cmd_rsp': code})
        self.tx_index += 4
        bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8,
                len(tuple))
        self.tx_index += 4
        for elm in tuple:
            bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8, elm)
            self.tx_index += 4
        return self.tx_data

    def create_loadfw_cmd(self, size, sha):
        """
        Creates a command to load firmware with associated parameters
        """
        cmd = 'Load Firmware'
        print('CMD >>> %s.' % cmd)
        code = self.get_cmd_code(cmd)

        FW_NO_EXEC_FLAG = (1 << 26)
        SEL_HP_CLK = (1 << 21)
        LD_FW_HEADER_LEN = 3

        count_flags = FW_NO_EXEC_FLAG | SEL_HP_CLK
        count_flags |= (LD_FW_HEADER_LEN + int(len(sha) / 4))

        self.init_tx_data()

        index = list(self.message_id.values()).index('Control')
        msg_id = list(self.message_id.keys())[index]
        bitstruct.pack_into_dict(self.cmd_word_fmt, self.cmd_keys,
                self.tx_data, 0, {'cmd': 1, 'rsvd1': 0, 'rsp':  0,
                'msg_id': msg_id, 'rsvd2': 0, 'cmd_rsp': code})
        self.tx_index += 4
        bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8, count_flags)
        self.tx_index += 4
        bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8, 0xbe000000)
        self.tx_index += 4
        bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8, 0)
        self.tx_index += 4
        bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8, size)
        self.tx_index += 4
        self.endian_swap(self.tx_data, self.tx_index, sha)
        self.tx_index += len(sha)
        return self.tx_data

    def create_execfw_cmd(self):
        """
        Creates a command to excute firmware
        """
        cmd = 'Execute'
        print('CMD >>> %s.' % cmd)
        code = self.get_cmd_code(cmd)

        EXE_FW_HEADER_LEN = 1

        count = EXE_FW_HEADER_LEN

        self.init_tx_data()

        index = list(self.message_id.values()).index('Control')
        msg_id = list(self.message_id.keys())[index]
        bitstruct.pack_into_dict(self.cmd_word_fmt, self.cmd_keys,
                self.tx_data, 0, {'cmd': 1, 'rsvd1': 0, 'rsp':  0,
                'msg_id': msg_id, 'rsvd2': 0, 'cmd_rsp': code})
        self.tx_index += 4
        bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8, count)
        self.tx_index += 4
        bitstruct.pack_into('u32', self.tx_data, self.tx_index * 8, 0xbe000000)
        self.tx_index += 4
        return self.tx_data

    def create_bulk_message(self, data):
        """
        Copies the input byte stream to the bulk message buffer
        """
        self.endian_swap(self.tx_bulk_data, 0, data)
        return self.tx_bulk_data[:len(data)]

    def get_bulk_message_size(self):
        """
        Returns the size of the bulk message buffer
        """
        return len(self.tx_bulk_data)
