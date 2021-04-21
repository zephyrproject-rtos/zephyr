#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import abc
import binascii
import logging


logger = logging.getLogger("gdbstub")


class GdbStub(abc.ABC):
    def __init__(self, logfile, elffile):
        self.logfile = logfile
        self.elffile = elffile
        self.socket = None
        self.gdb_signal = None

        mem_regions = list()

        for r in logfile.get_memory_regions():
            mem_regions.append(r)

        for r in elffile.get_memory_regions():
            mem_regions.append(r)

        self.mem_regions = mem_regions

    def get_gdb_packet(self):
        socket = self.socket
        if socket is None:
            return None

        data = b''
        checksum = 0
        # Wait for '$'
        while True:
            ch = socket.recv(1)
            if ch == b'$':
                break

        # Get a full packet
        while True:
            ch = socket.recv(1)
            if ch == b'#':
                # End of packet
                break

            checksum += ord(ch)
            data += ch

        # Get checksum (2-bytes)
        ch = socket.recv(2)
        in_chksum = ord(binascii.unhexlify(ch))

        logger.debug(f"Received GDB packet: {data}")

        if (checksum % 256) == in_chksum:
            # ACK
            logger.debug("ACK")
            socket.send(b'+')

            return data
        else:
            # NACK
            logger.debug(f"NACK (checksum {in_chksum} != {checksum}")
            socket.send(b'-')

            return None

    def put_gdb_packet(self, data):
        socket = self.socket
        if socket is None:
            return

        checksum = 0
        for d in data:
            checksum += d

        pkt = b'$' + data + b'#'

        checksum = checksum % 256
        pkt += format(checksum, "02X").encode()

        logger.debug(f"Sending GDB packet: {pkt}")

        socket.send(pkt)

    def handle_signal_query_packet(self):
        # the '?' packet
        pkt = b'S'
        pkt += format(self.gdb_signal, "02X").encode()

        self.put_gdb_packet(pkt)

    @abc.abstractmethod
    def handle_register_group_read_packet(self):
        # the 'g' packet for reading a group of registers
        pass

    def handle_register_group_write_packet(self):
        # the 'G' packet for writing to a group of registers
        #
        # We don't support writing so return error
        self.put_gdb_packet(b"E01")

    def handle_register_single_read_packet(self, pkt):
        # the 'p' packet for reading a single register
        self.put_gdb_packet(b"E01")

    def handle_register_single_write_packet(self, pkt):
        # the 'P' packet for writing to registers
        #
        # We don't support writing so return error
        self.put_gdb_packet(b"E01")

    def handle_memory_read_packet(self, pkt):
        # the 'm' packet for reading memory: m<addr>,<len>

        def get_mem_region(addr):
            for r in self.mem_regions:
                if r['start'] <= addr <= r['end']:
                    return r

            return None

        # extract address and length from packet
        # and convert them into usable integer values
        str_addr, str_length = pkt[1:].split(b',')
        s_addr = int(b'0x' + str_addr, 16)
        length = int(b'0x' + str_length, 16)

        # FIXME: Need more efficient way of extracting memory content
        remaining = length
        addr = s_addr
        barray = b''
        r = get_mem_region(addr)
        while remaining > 0:
            if r is None:
                barray = None
                break

            if addr > r['end']:
                r = get_mem_region(addr)
                continue

            offset = addr - r['start']
            barray += r['data'][offset:offset+1]

            addr += 1
            remaining -= 1

        if barray is not None:
            pkt = binascii.hexlify(barray)
            self.put_gdb_packet(pkt)
        else:
            self.put_gdb_packet(b"E01")

    def handle_memory_write_packet(self, pkt):
        # the 'M' packet for writing to memory
        #
        # We don't support writing so return error
        self.put_gdb_packet(b"E02")

    def handle_general_query_packet(self, pkt):
        self.put_gdb_packet(b'')

    def run(self, socket):
        self.socket = socket

        while True:
            pkt = self.get_gdb_packet()
            if pkt is None:
                continue

            pkt_type = pkt[0:1]
            logger.debug(f"Got packet type: {pkt_type}")

            if pkt_type == b'?':
                self.handle_signal_query_packet()
            elif pkt_type in (b'C', b'S'):
                # Continue/stepping execution, which is not supported.
                # So signal exception again
                self.handle_signal_query_packet()
            elif pkt_type == b'g':
                self.handle_register_group_read_packet()
            elif pkt_type == b'G':
                self.handle_register_group_write_packet()
            elif pkt_type == b'p':
                self.handle_register_single_read_packet(pkt)
            elif pkt_type == b'P':
                self.handle_register_single_write_packet(pkt)
            elif pkt_type == b'm':
                self.handle_memory_read_packet(pkt)
            elif pkt_type == b'M':
                self.handle_memory_write_packet(pkt)
            elif pkt_type == b'q':
                self.handle_general_query_packet(pkt)
            elif pkt_type == b'k':
                # GDB quits
                break
            else:
                self.put_gdb_packet(b'')
