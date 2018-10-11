#!/usr/bin/python
# -*- coding: utf-8 -*-

import ctypes
import hashlib
import binascii

MAGIC = "$B!N"
VERSION = [0x01,0x02]
VERSION_OFFSET = 4

_fields_V1 = [
    # Always equal to $B!N
    ("magic", ctypes.c_char * 4),

    # Header format version
    ("version", ctypes.c_ubyte),
    ("major", ctypes.c_ubyte),
    ("minor", ctypes.c_ubyte),
    ("patch", ctypes.c_ubyte),

    # Human-friendly version string, free format (not NULL terminated)
    # Advised format is: PPPPXXXXXX-YYWWTBBBB
    #  - PPPP  : product code, e.g ATP1
    #  - XXXXXX: binary info. Usually contains information such as the
    #    binary type (bootloader, application), build variant (unit tests,
    #    debug, release), release/branch name
    #  - YY    : year last 2 digits
    #  - WW    : work week number
    #  - T     : build type, e.g. [Engineering], [W]eekly, [L]atest,
    #    [R]elease, [P]roduction, [F]actory, [C]ustom
    #  - BBBB  : build number, left padded with zeros
    # Examples:
    #  - ATP1BOOT01-1503W0234
    #  - CLRKAPP123-1502R0013
    ("version_string", ctypes.c_ubyte * 20),

    # Micro-SHA1 (first 4 bytes of the SHA1) of the binary payload excluding
    # this header. It allows to uniquely identify the exact binary used.
    ("hash", ctypes.c_ubyte * 4),

    # Position of the payload relative to the address of this structure
    ("offset", ctypes.c_int32),
    ("reserved_1", ctypes.c_ubyte * 4),

    # Size of the payload, i.e. the full binary on which the hash was
    # computed (excluding this header). The beginning of the payload
    # is assumed to start right after the last byte of this structure.
    ("size", ctypes.c_uint32),
    ("reserved_2", ctypes.c_ubyte * 4)
]

_fields_V2 = [
    # Always equal to $B!N
    ("magic", ctypes.c_char * 4),

    # Header format version
    ("version", ctypes.c_ubyte),
    ("major", ctypes.c_ubyte),
    ("minor", ctypes.c_ubyte),
    ("patch", ctypes.c_ubyte),

    # Human-friendly version string, free format (not NULL terminated)
    # Advised format is: PPPPXXXXXX-YYWWTBBBB
    #  - PPPP  : product code, e.g ATP1
    #  - XXXXXX: binary info. Usually contains information such as the
    #    binary type (bootloader, application), build variant (unit tests,
    #    debug, release), release/branch name
    #  - YY    : year last 2 digits
    #  - WW    : work week number
    #  - T     : build type, e.g. [Engineering], [W]eekly, [L]atest,
    #    [R]elease, [P]roduction, [F]actory, [C]ustom
     #  - BBBB  : build number, left padded with zeros
    # Examples:
    #  - ATP1BOOT01-1503W0234
    #  - CLRKAPP123-1502R0013
    ("version_string", ctypes.c_ubyte * 20),

    # Meta-SHA1
    ("build_hash", ctypes.c_ubyte * 4),

    # Micro-SHA1 (first 4 bytes of the SHA1) of the binary payload excluding
    # this header. It allows to uniquely identify the exact binary used.
    ("hash", ctypes.c_ubyte * 4),

    # Position of the payload relative to the address of this structure
    ("offset", ctypes.c_int32),
    ("reserved_1", ctypes.c_ubyte * 4),

    # Size of the payload, i.e. the full binary on which the hash was
    # computed (excluding this header). The beginning of the payload
    # is assumed to start right after the last byte of this structure.
    ("size", ctypes.c_uint32),
    ("reserved_2", ctypes.c_ubyte * 4)
]

def ctypes_factory(buf):
    if buf[VERSION_OFFSET] == 1:
        return type("Int", (BinaryVersionHeader,), {"_fields_": _fields_V1}).from_buffer_copy(buf)
    elif buf[VERSION_OFFSET] == 2:
        return type("Int", (BinaryVersionHeader,), {"_fields_": _fields_V2}).from_buffer_copy(buf)
    else:
        assert False

class BinaryVersionHeader(ctypes.Structure):

    def get_printable_build_hash(self):
        return binascii.hexlify(self.build_hash)

