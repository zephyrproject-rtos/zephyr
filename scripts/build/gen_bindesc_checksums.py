#!/usr/bin/env python3

import sys
import hashlib
from elftools.elf.elffile import ELFFile
from elftools.elf.constants import SH_FLAGS


BINDESC_OFFSETOF_DATA = 4
HASH_ALGORITHMS = {
    "md5": hashlib.md5,
}


def set_checksums(filename):
    with open(filename, 'rb+') as file_stream:
        elffile = ELFFile(file_stream)

        # Variable for storing the raw bytes of the image
        image_data = b""

        # Iterate over the segments to produce the image to calculate the
        # checksums over.
        for segment in elffile.iter_segments():
            if segment.header.p_type != "PT_LOAD":
                continue

            if segment.header.p_filesz == 0:
                continue

            for section in elffile.iter_sections():
                if segment.section_in_segment(section):
                    if not (section.name == "rom_start" and segment.section_in_segment(section)):
                        # Don't include rom_start in the checksum calculation
                        image_data += section.data()

        symbol_table = elffile.get_section_by_name(".symtab")

        # The binary descriptors to modify are found in the rom_start section
        rom_start = elffile.get_section_by_name("rom_start")

        for symbol in symbol_table.iter_symbols():
            # Iterate over all the symbols, modify the checksums
            if symbol.name.startswith("bindesc_entry_checksum"):
                # Determine the alorithm and data type
                _, _, _, algorithm, data_type = symbol.name.split("_")

                # Calculate the checksum
                checksum = HASH_ALGORITHMS[algorithm](image_data)
                if data_type == "bytes":
                    to_write = checksum.digest()
                else:
                    to_write = checksum.hexdigest().encode()

                # Calculate the offset of the ELF file to write the checksum to
                # The formula is: offset of rom_start + address of the symbol minus
                # the base address of rom_start + offset of the data inside the descriptor
                # entry.
                offset = rom_start.header.sh_offset + \
                    symbol.entry.st_value - rom_start.header.sh_addr + \
                          BINDESC_OFFSETOF_DATA

                # Write the hash to the file
                elffile.stream.seek(offset)
                elffile.stream.write(to_write)


def main():
    set_checksums(sys.argv[1])

if __name__ == "__main__":
    main()
