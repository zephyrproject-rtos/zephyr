#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import abc
import binascii
import logging

from coredump_parser.elf_parser import ThreadInfoOffset


logger = logging.getLogger("gdbstub")


class GdbStub(abc.ABC):
    def __init__(self, logfile, elffile):
        self.logfile = logfile
        self.elffile = elffile
        self.socket = None
        self.gdb_signal = None
        self.thread_ptrs = list()
        self.selected_thread = 0

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

    def get_memory(self, start_address, length):
        def get_mem_region(addr):
            for r in self.mem_regions:
                if r['start'] <= addr < r['end']:
                    return r

            return None

        # FIXME: Need more efficient way of extracting memory content
        remaining = length
        addr = start_address
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

        return barray

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

        # extract address and length from packet
        # and convert them into usable integer values
        str_addr, str_length = pkt[1:].split(b',')
        s_addr = int(b'0x' + str_addr, 16)
        length = int(b'0x' + str_length, 16)

        barray = self.get_memory(s_addr, length)

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
        if self.arch_supports_thread_operations() and self.elffile.has_kernel_thread_info():
            # For packets qfThreadInfo/qsThreadInfo, obtain a list of all active thread IDs
            if pkt[0:12] == b"qfThreadInfo":
                threads_metadata_data = self.logfile.get_threads_metadata()["data"]
                size_t_size = self.elffile.get_kernel_thread_info_size_t_size()

                # First, find and store the thread that _kernel considers current
                k_curr_thread_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_K_CURR_THREAD)
                curr_thread_ptr_bytes = threads_metadata_data[k_curr_thread_offset:(k_curr_thread_offset + size_t_size)]
                curr_thread_ptr = int.from_bytes(curr_thread_ptr_bytes, "little")
                self.thread_ptrs.append(curr_thread_ptr)

                thread_count = 1
                response = b"m1"

                # Next, find the pointer to the linked list of threads in the _kernel struct
                k_threads_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_K_THREADS)
                thread_ptr_bytes = threads_metadata_data[k_threads_offset:(k_threads_offset + size_t_size)]
                thread_ptr = int.from_bytes(thread_ptr_bytes, "little")

                if thread_ptr != curr_thread_ptr:
                    self.thread_ptrs.append(thread_ptr)
                    thread_count += 1
                    response += b"," + bytes(str(thread_count), 'ascii')

                # Next walk the linked list, counting the number of threads and construct the response for qfThreadInfo along the way
                t_next_thread_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_T_NEXT_THREAD)
                while thread_ptr is not None:
                    thread_ptr_bytes = self.get_memory(thread_ptr + t_next_thread_offset, size_t_size)

                    if thread_ptr_bytes is not None:
                        thread_ptr = int.from_bytes(thread_ptr_bytes, "little")
                        if thread_ptr == 0:
                            thread_ptr = None
                            continue

                        if thread_ptr != curr_thread_ptr:
                            self.thread_ptrs.append(thread_ptr)
                            thread_count += 1
                            response += b"," + bytes(f'{thread_count:x}', 'ascii')
                    else:
                        thread_ptr = None

                self.put_gdb_packet(response)
            elif pkt[0:12] == b"qsThreadInfo":
                self.put_gdb_packet(b"l")

            # For qThreadExtraInfo, obtain a printable string description of thread attributes for the provided thread
            elif pkt[0:16] == b"qThreadExtraInfo":
                thread_info_bytes = b''

                thread_index_str = ''
                for n in range(17, len(pkt)):
                    thread_index_str += chr(pkt[n])

                thread_id = int(thread_index_str, 16)
                if len(self.thread_ptrs) > thread_id:
                    thread_info_bytes += b'name: '
                    thread_ptr = self.thread_ptrs[thread_id - 1]
                    t_name_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_T_NAME)

                    thread_name_next_byte = self.get_memory(thread_ptr + t_name_offset, 1)
                    index = 0
                    while (thread_name_next_byte is not None) and (thread_name_next_byte != b'\x00'):
                        thread_info_bytes += thread_name_next_byte

                        index += 1
                        thread_name_next_byte = self.get_memory(thread_ptr + t_name_offset + index, 1)

                    t_state_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_T_STATE)
                    thread_state_byte = self.get_memory(thread_ptr + t_state_offset, 1)
                    if thread_state_byte is not None:
                        thread_state = int.from_bytes(thread_state_byte, "little")
                        thread_info_bytes += b', state: ' + bytes(hex(thread_state), 'ascii')

                    t_user_options_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_T_USER_OPTIONS)
                    thread_user_options_byte = self.get_memory(thread_ptr + t_user_options_offset, 1)
                    if thread_user_options_byte is not None:
                        thread_user_options = int.from_bytes(thread_user_options_byte, "little")
                        thread_info_bytes += b', user_options: ' + bytes(hex(thread_user_options), 'ascii')

                    t_prio_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_T_PRIO)
                    thread_prio_byte = self.get_memory(thread_ptr + t_prio_offset, 1)
                    if thread_prio_byte is not None:
                        thread_prio = int.from_bytes(thread_prio_byte, "little")
                        thread_info_bytes += b', prio: ' + bytes(hex(thread_prio), 'ascii')

                self.put_gdb_packet(binascii.hexlify(thread_info_bytes))
            else:
                self.put_gdb_packet(b'')
        else:
            self.put_gdb_packet(b'')

    def arch_supports_thread_operations(self):
        return False

    def handle_thread_alive_packet(self, pkt):
        # the 'T' packet for finding out if a thread is alive.
        if self.arch_supports_thread_operations() and self.elffile.has_kernel_thread_info():
            # Reply OK to report thread alive, allowing GDB to perform other thread operations
            self.put_gdb_packet(b'OK')
        else:
            self.put_gdb_packet(b'')

    def handle_thread_register_group_read_packet(self):
        self.put_gdb_packet(b'')

    def handle_thread_op_packet(self, pkt):
        # the 'H' packet for setting thread for subsequent operations.
        if self.arch_supports_thread_operations() and self.elffile.has_kernel_thread_info():
            if pkt[0:2] == b"Hg":
                thread_index_str = ''
                for n in range(2, len(pkt)):
                    thread_index_str += chr(pkt[n])

                # Thread-id of '0' indicates an arbitrary process or thread
                if thread_index_str in ('0', ''):
                    self.selected_thread = 0
                    self.handle_register_group_read_packet()
                    return

                self.selected_thread = int(thread_index_str, 16) - 1
                self.handle_thread_register_group_read_packet()
            else:
                self.put_gdb_packet(b'')
        else:
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
            elif pkt_type == b'T':
                self.handle_thread_alive_packet(pkt)
            elif pkt_type == b'H':
                self.handle_thread_op_packet(pkt)
            elif pkt_type == b'k':
                # GDB quits
                break
            else:
                self.put_gdb_packet(b'')
