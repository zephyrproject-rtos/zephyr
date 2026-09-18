#!/usr/bin/env python3

# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Minimal Mach-O reader for Zephyr build-time image checks."""

import re
import struct

# A dyld chained rebase holds the offset from the image base in its low bits
_CHAINED_REBASE_TARGET_MASK = (1 << 36) - 1


class _MachOSection:
    """A section from a 64-bit Mach-O image."""

    def __init__(self, index, name, address, size, offset):
        self.index = index
        self.name = name
        self.address = address
        self.size = size
        self.offset = offset


class _MachOSymbol:
    """A symbol from a 64-bit Mach-O image."""

    def __init__(self, name, address, section_index):
        self.name = name
        self.address = address
        self.section_index = section_index
        self.size = 0


class MachOFile:
    """Read the subset of a 64-bit Mach-O image needed by the init checker."""

    _MAGIC_64 = 0xFEEDFACF
    _MAGIC_64_BE = 0xCFFAEDFE
    _LC_SEGMENT_64 = 0x19
    _LC_SYMTAB = 0x2
    _N_TYPE = 0x0E
    _N_SECT = 0x0E
    """Section holding the init entries of one level, see Z_INIT_ENTRY_SECTION()"""
    _INIT_SECTION_RE = re.compile(r"^zi([0-5])$")

    @staticmethod
    def is_macho(image_file):
        """Return true if the file starts with a supported Mach-O magic."""
        position = image_file.tell()
        magic = image_file.read(4)
        image_file.seek(position)

        return magic in (b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xcf")

    def __init__(self, image_file):
        self._data = image_file.read()
        self._sections = {}
        self._symbols = []
        self._endian = None
        self._image_base = None
        self._image_end = None
        self.pointer_size = 8

        self._parse()

    @staticmethod
    def _decode_name(raw_name):
        return raw_name.split(b"\0", 1)[0].decode("ascii")

    def _unpack(self, format_string, offset):
        size = struct.calcsize(format_string)
        if (offset < 0) or (offset + size > len(self._data)):
            raise ValueError("Mach-O structure extends beyond the file")

        return struct.unpack_from(format_string, self._data, offset)

    def _parse(self):
        if len(self._data) < 32:
            raise ValueError("Mach-O file is shorter than its header")

        magic = struct.unpack_from("<I", self._data, 0)[0]
        if magic == self._MAGIC_64:
            self._endian = "<"
        elif magic == self._MAGIC_64_BE:
            self._endian = ">"
        else:
            raise ValueError("Only 64-bit Mach-O images are supported")

        _, _, _, _, command_count, _, _, _ = self._unpack(self._endian + "IiiIIIII", 0)
        command_offset = 32
        symtab = None

        for _ in range(command_count):
            command, command_size = self._unpack(self._endian + "II", command_offset)
            if command_size < 8 or command_offset + command_size > len(self._data):
                raise ValueError("Invalid Mach-O load command size")

            if command == self._LC_SEGMENT_64:
                self._parse_segment(command_offset, command_size)
            elif command == self._LC_SYMTAB:
                symtab = self._unpack(self._endian + "IIII", command_offset + 8)

            command_offset += command_size

        if symtab is None:
            raise ValueError("Mach-O image has no symbol table")

        self._parse_symbols(*symtab)

    def _parse_segment(self, command_offset, command_size):
        segment_format = self._endian + "II16sQQQQiiII"
        segment_size = struct.calcsize(segment_format)
        if command_size < segment_size:
            raise ValueError("Invalid Mach-O segment command size")

        (
            _,
            _,
            segment_name,
            vmaddr,
            vmsize,
            _,
            _,
            _,
            _,
            section_count,
            _,
        ) = self._unpack(segment_format, command_offset)

        if self._decode_name(segment_name) == "__TEXT":
            self._image_base = vmaddr
            self._image_end = vmaddr + vmsize
        section_offset = command_offset + segment_size
        section_format = self._endian + "16s16sQQIIIIIIII"
        section_size = struct.calcsize(section_format)

        if section_offset + section_count * section_size > command_offset + command_size:
            raise ValueError("Mach-O segment contains truncated sections")

        for section_index in range(section_count):
            values = self._unpack(section_format, section_offset + section_index * section_size)
            section = _MachOSection(
                len(self._sections) + 1,
                self._decode_name(values[0]),
                values[2],
                values[3],
                values[4],
            )
            self._sections[section.index] = section

    def _parse_symbols(self, symbol_offset, symbol_count, string_offset, string_size):
        symbol_format = self._endian + "IBBHQ"
        symbol_size = struct.calcsize(symbol_format)
        string_end = string_offset + string_size

        if string_end > len(self._data):
            raise ValueError("Mach-O string table extends beyond the file")

        symbols_by_section = {}
        for symbol_index in range(symbol_count):
            values = self._unpack(symbol_format, symbol_offset + symbol_index * symbol_size)
            string_index, symbol_type, section_index, _, address = values

            if (symbol_type & self._N_TYPE) != self._N_SECT:
                continue
            if section_index not in self._sections or address == 0:
                continue
            if string_index >= string_size:
                raise ValueError("Mach-O symbol string index is out of bounds")

            string_start = string_offset + string_index
            string_end_index = self._data.find(b"\0", string_start, string_end)
            if string_end_index < 0:
                raise ValueError("Mach-O symbol is not NUL-terminated")

            name = self._data[string_start:string_end_index].decode("ascii")
            if name.startswith("_"):
                name = name[1:]

            symbol = _MachOSymbol(name, address, section_index)
            self._symbols.append(symbol)
            symbols_by_section.setdefault(section_index, []).append(symbol)

        for section_index, symbols in symbols_by_section.items():
            section = self._sections[section_index]
            symbols.sort(key=lambda symbol: symbol.address)

            # Mach-O symbols carry no size: a symbol reaches up to the next one
            # at a higher address, or to the end of its section. Symbols sharing
            # an address, an alias and the symbol it names, share their size.
            next_address = section.address + section.size
            for symbol in reversed(symbols):
                if symbol.address < next_address:
                    size = next_address - symbol.address
                    next_address = symbol.address
                symbol.size = size

    def symbols(self):
        """Return symbols with addresses in the linked image."""
        return self._symbols

    def read_pointer(self, address, index, section_index):
        """Read a pointer from an image section."""
        section = self._sections.get(section_index)
        if section is None or address < section.address:
            raise ValueError(f"Address {address:016x} is not in a Mach-O section")

        offset = section.offset + address - section.address + index * self.pointer_size
        if offset + self.pointer_size > section.offset + section.size:
            raise ValueError("Mach-O pointer extends beyond its section")

        return self._resolve_pointer(self._unpack(self._endian + "Q", offset)[0])

    def _resolve_pointer(self, value):
        """Return the address a stored pointer refers to.

        A pointer into the image is stored as a dyld chained fixup rather than as
        the address itself: for a rebase, which is what a pointer to something in
        the same image is, the low 36 bits hold the offset from the image base
        and the rest holds the chain. A bind, the high bit, refers to a symbol in
        another image and has no address here.
        """
        if value == 0 or self._image_base is None:
            return value
        if self._image_base <= value < self._image_end:
            return value
        if value & (1 << 63):
            return 0

        return self._image_base + (value & _CHAINED_REBASE_TARGET_MASK)

    def init_entries(self):
        """Return init-entry symbols grouped by their init level ordinal."""
        entries = {level: [] for level in range(6)}

        for section in self._sections.values():
            match = self._INIT_SECTION_RE.match(section.name)
            if match is None:
                continue

            level = int(match.group(1))
            entries[level].extend(
                symbol
                for symbol in self._symbols
                if symbol.section_index == section.index
                and symbol.name.startswith("__init_")
                and not symbol.name.startswith("__init_order_")
            )

        for level in entries:
            entries[level].sort(key=lambda symbol: symbol.address)

        return entries
