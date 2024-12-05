#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import struct


# Note: keep sync with C code
COREDUMP_HDR_ID = b'ZE'
COREDUMP_HDR_VER = 2
LOG_HDR_STRUCT = "<ccHHBBI"
LOG_HDR_SIZE = struct.calcsize(LOG_HDR_STRUCT)

COREDUMP_ARCH_HDR_ID = b'A'
LOG_ARCH_HDR_STRUCT = "<cHH"
LOG_ARCH_HDR_SIZE = struct.calcsize(LOG_ARCH_HDR_STRUCT)

COREDUMP_THREADS_META_HDR_ID = b'T'
LOG_THREADS_META_HDR_STRUCT = "<cHH"
LOG_THREADS_META_HDR_SIZE = struct.calcsize(LOG_THREADS_META_HDR_STRUCT)

COREDUMP_MEM_HDR_ID = b'M'
COREDUMP_MEM_HDR_VER = 1
LOG_MEM_HDR_STRUCT = "<cH"
LOG_MEM_HDR_SIZE = struct.calcsize(LOG_MEM_HDR_STRUCT)


logger = logging.getLogger("parser")


def reason_string(reason):
    # Keep sync with "enum k_fatal_error_reason"
    ret = "(Unknown)"

    if reason == 0:
        ret = "K_ERR_CPU_EXCEPTION"
    elif reason == 1:
        ret = "K_ERR_SPURIOUS_IRQ"
    elif reason == 2:
        ret = "K_ERR_STACK_CHK_FAIL"
    elif reason == 3:
        ret = "K_ERR_KERNEL_OOPS"
    elif reason == 4:
        ret = "K_ERR_KERNEL_PANIC"

    return ret


class CoredumpLogFile:
    """
    Process the binary coredump file for register block
    and memory blocks.
    """

    def __init__(self, logfile):
        self.logfile = logfile
        self.fd = None

        self.log_hdr = None
        self.arch_data = list()
        self.memory_regions = list()
        self.threads_metadata = {"hdr_ver" : None, "data" : None}

    def open(self):
        self.fd = open(self.logfile, "rb")

    def close(self):
        self.fd.close()

    def get_arch_data(self):
        return self.arch_data

    def get_memory_regions(self):
        return self.memory_regions

    def get_threads_metadata(self):
        return self.threads_metadata

    def parse_arch_section(self):
        hdr = self.fd.read(LOG_ARCH_HDR_SIZE)
        _, hdr_ver, num_bytes = struct.unpack(LOG_ARCH_HDR_STRUCT, hdr)

        arch_data = self.fd.read(num_bytes)

        self.arch_data = {"hdr_ver" : hdr_ver, "data" : arch_data}

        return True

    def parse_threads_metadata_section(self):
        hdr = self.fd.read(LOG_THREADS_META_HDR_SIZE)
        _, hdr_ver, num_bytes = struct.unpack(LOG_THREADS_META_HDR_STRUCT, hdr)

        data = self.fd.read(num_bytes)

        self.threads_metadata = {"hdr_ver" : hdr_ver, "data" : data}

        return True

    def parse_memory_section(self):
        hdr = self.fd.read(LOG_MEM_HDR_SIZE)
        _, hdr_ver = struct.unpack(LOG_MEM_HDR_STRUCT, hdr)

        if hdr_ver != COREDUMP_MEM_HDR_VER:
            logger.error(f"Memory block version: {hdr_ver}, expected {COREDUMP_MEM_HDR_VER}!")
            return False

        # Figure out how to read the start and end addresses
        ptr_fmt = None
        if self.log_hdr["ptr_size"] == 64:
            ptr_fmt = "QQ"
        elif self.log_hdr["ptr_size"] == 32:
            ptr_fmt = "II"
        else:
            return False

        data = self.fd.read(struct.calcsize(ptr_fmt))
        saddr, eaddr = struct.unpack(ptr_fmt, data)

        size = eaddr - saddr

        data = self.fd.read(size)

        mem = {"start": saddr, "end": eaddr, "data": data}
        self.memory_regions.append(mem)

        logger.info("Memory: 0x%x to 0x%x of size %d" %
                    (saddr, eaddr, size))

        return True

    def parse(self):
        if self.fd is None:
            self.open()

        hdr = self.fd.read(LOG_HDR_SIZE)
        id1, id2, hdr_ver, tgt_code, ptr_size, flags, reason = struct.unpack(LOG_HDR_STRUCT, hdr)

        if (id1 + id2) != COREDUMP_HDR_ID:
            # ID in header does not match
            logger.error("Log header ID not found...")
            return False

        if hdr_ver > COREDUMP_HDR_VER:
            logger.error(f"Log version: {hdr_ver}, expected: {COREDUMP_HDR_VER}!")
            return False

        ptr_size = 2 ** ptr_size

        self.log_hdr = {
                        "hdr_version": hdr_ver,
                        "tgt_code": tgt_code,
                        "ptr_size": ptr_size,
                        "flags": flags,
                        "reason": reason,
                        }

        logger.info("Reason: {0}".format(reason_string(reason)))
        logger.info(f"Pointer size {ptr_size}")

        del id1, id2, hdr_ver, tgt_code, ptr_size, flags, reason

        while True:
            section_id = self.fd.read(1)
            if not section_id:
                # no more data to read
                break

            self.fd.seek(-1, 1) # go back 1 byte
            if section_id == COREDUMP_ARCH_HDR_ID:
                if not self.parse_arch_section():
                    logger.error("Cannot parse architecture section")
                    return False
            elif section_id == COREDUMP_THREADS_META_HDR_ID:
                if not self.parse_threads_metadata_section():
                    logger.error("Cannot parse threads metadata section")
                    return False
            elif section_id == COREDUMP_MEM_HDR_ID:
                if not self.parse_memory_section():
                    logger.error("Cannot parse memory section")
                    return False
            else:
                # Unknown section in log file
                logger.error(f"Unknown section in log file with ID {section_id}")
                return False

        return True
