#!/usr/bin/env python3
#
# Copyright (c) 2024 Nuvoton Technology Corporation
#
# SPDX-License-Identifier: Apache-2.0

# This script will append/paste specific header to tell ROM code (Booter) of
# NPCM eSIO series how to load the firmware from flash to code ram
# Usage python3 ${ZEPHYR_BASE}/scripts/esiost.py
#                    -i in_file.bin -o out_file.bin
#                    [-chip <name>] [-v]

import sys
import hashlib
from colorama import init, Fore
from esiost_args import EsiostArgs, exit_with_failure
from pathlib import Path

# ESIOST
ESIOST_VER = "1.0.0"

# Offsets inside the header
HDR_ANCHOR_OFFSET                       = 0x0
HDR_FW_ENTRY_POINT_OFFSET               = 0x21C
HDR_FW_FLASH_ADDR_START_LOAD_OFFSET     = 0x220
HDR_FW_FLASH_ADDR_END_LOAD_OFFSET       = 0x224
HDR_FW_LOAD_START_ADDR_OFFSET           = 0x228
HDR_FW_LENGTH_OFFSET                    = 0x22C
HDR_FW_LOAD_HASH_OFFSET                 = 0x480
HDR_FW_SEG1_START_OFFSET                = 0x4C0
HDR_FW_SEG1_SIZE_OFFSET                 = 0x4C4
HDR_FW_SEG2_START_OFFSET                = 0x4C8
HDR_FW_SEG2_SIZE_OFFSET                 = 0x4CC
HDR_FW_SEG3_START_OFFSET                = 0x4D0
HDR_FW_SEG3_SIZE_OFFSET                 = 0x4D4
HDR_FW_SEG4_START_OFFSET                = 0x4D8
HDR_FW_SEG4_SIZE_OFFSET                 = 0x4DC
HDR_FW_SEG1_HASH_OFFSET                 = 0x500
HDR_FW_SEG2_HASH_OFFSET                 = 0x540
HDR_FW_SEG3_HASH_OFFSET                 = 0x580
HDR_FW_SEG4_HASH_OFFSET                 = 0x5C0
FW_IMAGE_OFFSET                         = 0x600

ARM_FW_ENTRY_POINT_OFFSET               = 0x004

# Header field known values
FW_HDR_ANCHOR      = '%FiMg94@'
FW_HDR_SEG1_START  = 0x210
FW_HDR_SEG1_SIZE   = 0x2F0
FW_HDR_SEG2_START  = 0x0
FW_HDR_SEG2_SIZE   = 0x0
FW_HDR_SEG3_START  = 0x600
FW_HDR_SEG4_START  = 0x0
FW_HDR_SEG4_SIZE   = 0x0

# Header fields default values.
ADDR_16_BYTES_ALIGNED_MASK = 0x0000000f
ADDR_4_BYTES_ALIGNED_MASK = 0x00000003
ADDR_4K_BYTES_ALIGNED_MASK = 0x00000fff

INVALID_INPUT = -1
HEADER_SIZE = FW_IMAGE_OFFSET

# Verbose related values
NO_VERBOSE = 0
REG_VERBOSE = 0

# Success/failure codes
EXIT_SUCCESS_STATUS = 0
EXIT_FAILURE_STATUS = 1

def _bt_mode_handler(esiost_args):
    """creates the bootloader table using the provided arguments.

    :param esiost_args: the object representing the command line arguments.
    """

    output_file = _set_input_and_output(esiost_args)
    _check_chip(output_file, esiost_args)

    _copy_image(output_file, esiost_args)
    _set_anchor(output_file, esiost_args)
    _set_firmware_load_start_address(output_file, esiost_args)
    _set_firmware_entry_point(output_file, esiost_args)
    _set_firmware_length(output_file, esiost_args)
    _set_firmware_load_hash(output_file, esiost_args)
    _set_firmware_segment(output_file, esiost_args)
    _set_firmware_segment_hash(output_file, esiost_args)

    _exit_with_success()

def _set_input_and_output(esiost_args):
    """checks the input file and output and sets the output file.

    checks input file existence, creates an output file according
    to the 'output' argument.

    Note: input file size has to be greater than 0, and named differently
    from output file

    :param esiost_args: the object representing the command line arguments.

    :returns: output file path object, or -1 if fails
    """
    input_file = esiost_args.input
    output = esiost_args.output
    input_file_size = 0

    if not input_file:
        exit_with_failure("Define input file, using -i flag")

    input_file_path = Path(input_file)

    if not input_file_path.exists():
        exit_with_failure(f'Cannot open {input_file}')
    elif input_file_path.stat().st_size == 0:
        exit_with_failure(f'BIN Input file ({input_file}) is empty')
    else:
        input_file_size = input_file_path.stat().st_size

    if not output:
        output_file = Path("out_" + input_file_path.name)
    else:
        output_file = Path(output)

    if output_file.exists():
        if output_file.samefile(input_file_path):
            exit_with_failure(f'Input file name {input_file} '
                              f'should be differed from'
                              f' Output file name {output}')
        output_file.unlink()

    output_file.touch()

    if esiost_args.verbose == REG_VERBOSE:
        print(Fore.LIGHTCYAN_EX + f'\nBIN file: {input_file}, size:'
              f' {input_file_size} bytes')
        print(f'Output file name: {output_file.name} \n')

    return output_file

def _check_chip(output, esiost_args):
    """checks if the chip entered is a legal chip, generates an error
    and closes the application, deletes the output file if the chip name
    is illegal.

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """

    if esiost_args.chip_name == INVALID_INPUT:
        message = f'Invalid chip name, '
        message += "should be npcm400."
        _exit_with_failure_delete_file(output, message)

def _set_anchor(output, esiost_args):
    """writes the anchor value to the output file

    :param output: the output file object.
    :param esiost_args: the object representing the command line arguments.
    """

    if len(FW_HDR_ANCHOR) > 8:
        message = f'ANCHOR max support 8 bytes'
        _exit_with_failure_delete_file(output, message)

    with output.open("r+b") as output_file:
        output_file.seek(HDR_ANCHOR_OFFSET)
        anchor_hex = FW_HDR_ANCHOR.encode('ascii')
        output_file.write(anchor_hex)
        if esiost_args.verbose == REG_VERBOSE:
            print(f'- HDR - FW Header ANCHOR                 - Offset '
                  f'{HDR_ANCHOR_OFFSET}    -  %s' % FW_HDR_ANCHOR)

    output_file.close()

def _set_firmware_load_start_address(output, esiost_args):
    """writes the fw load address to the output file

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """
    input_file_path = Path(esiost_args.input)

    start_ram = esiost_args.chip_ram_address
    end_ram = start_ram + esiost_args.chip_ram_size
    fw_load_addr = esiost_args.firmware_load_address
    fw_length = esiost_args.firmware_length
    fw_end_addr = fw_load_addr + fw_length
    start_flash_addr = esiost_args.chip_flash_address + HEADER_SIZE
    end_flash_addr = start_flash_addr + fw_length

    start_ram_to_print = _hex_print_format(start_ram)
    end_ram_to_print = _hex_print_format(end_ram)
    fw_load_addr_to_print = _hex_print_format(fw_load_addr)
    fw_end_addr_to_print = _hex_print_format(fw_end_addr)

    if fw_length == INVALID_INPUT:
        message = f'Cannot read firmware length'
        _exit_with_failure_delete_file(output, message)

    if fw_load_addr is INVALID_INPUT:
        message = f'Cannot read FW Load start address'
        _exit_with_failure_delete_file(output, message)

    if fw_load_addr & ADDR_16_BYTES_ALIGNED_MASK != 0:
        message = f'Firmware load address ({fw_load_addr_to_print}) ' \
            f'is not 16 bytes aligned'
        _exit_with_failure_delete_file(output, message)

    if (fw_load_addr > end_ram) or (fw_load_addr < start_ram):
        message = f'Firmware load address ({fw_load_addr_to_print}) ' \
            f'should be between start ({start_ram_to_print}) '\
            f'and end ({end_ram_to_print}) of RAM'
        _exit_with_failure_delete_file(output, message)

    with output.open("r+b") as output_file:
        # check fw_entry pt location in flash or not
        with input_file_path.open("r+b") as input_file:
            input_file.seek(ARM_FW_ENTRY_POINT_OFFSET)
            fw_arm_entry_byte = input_file.read(4)
            fw_arm_entry_pt = int.from_bytes(fw_arm_entry_byte, "little")

            if fw_arm_entry_pt == 0:
                input_file.seek(ARM_FW_ENTRY_POINT_OFFSET + HEADER_SIZE)
                fw_arm_entry_byte = input_file.read(4)
                fw_arm_entry_pt = int.from_bytes(fw_arm_entry_byte, "little")
            else:
                if fw_end_addr > end_ram:
                    message = f'Firmware end address ({fw_end_addr_to_print}) should be '
                    message += f'less than end of RAM address ({end_ram_to_print})'
                    _exit_with_failure_delete_file(output, message)

            if start_flash_addr < fw_arm_entry_pt < end_flash_addr:
                fw_load_addr = 0x0
                start_flash_addr = 0x0
                end_flash_addr = 0x0

        input_file.close()

        # set start load flash address
        output_file.seek(HDR_FW_FLASH_ADDR_START_LOAD_OFFSET)
        output_file.write(start_flash_addr.to_bytes(4, "little"))

        # set end load flash address
        output_file.seek(HDR_FW_FLASH_ADDR_END_LOAD_OFFSET)
        output_file.write(end_flash_addr.to_bytes(4, "little"))

        # set load start address (RAM)
        output_file.seek(HDR_FW_LOAD_START_ADDR_OFFSET)
        output_file.write(fw_load_addr.to_bytes(4, "little"))

        if esiost_args.verbose == REG_VERBOSE:
            print(f'- HDR - FW load start address            - Offset '
                  f'{HDR_FW_LOAD_START_ADDR_OFFSET}  -  '
                  f'{_hex_print_format(fw_load_addr)}')
            print(f'- HDR - flash load start address         - Offset '
                  f'{HDR_FW_FLASH_ADDR_START_LOAD_OFFSET}  -  '
                  f'{_hex_print_format(start_flash_addr)}')
            print(f'- HDR - flash load end address           - Offset '
                  f'{HDR_FW_FLASH_ADDR_END_LOAD_OFFSET}  -  '
                  f'{_hex_print_format(end_flash_addr)}')

    output_file.close()

def _set_firmware_entry_point(output, esiost_args):
    """writes the fw entry point to the output file.
    proportions:

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """
    input_file_path = Path(esiost_args.input)
    fw_entry_pt = esiost_args.firmware_entry_point
    start_flash_addr = esiost_args.chip_flash_address + HEADER_SIZE
    end_flash_addr = start_flash_addr + esiost_args.chip_flash_size

    # check if fwep flag wasn't set and set it to fw load address if needed
    if fw_entry_pt is None:
        fw_entry_pt = esiost_args.firmware_load_address

    # check fw_entry pt location in flash or not
    with input_file_path.open("r+b") as input_file:
        input_file.seek(ARM_FW_ENTRY_POINT_OFFSET)
        fw_arm_entry_byte = input_file.read(4)
        fw_arm_entry_pt = int.from_bytes(fw_arm_entry_byte, "little")

        if fw_arm_entry_pt == 0:
            input_file.seek(ARM_FW_ENTRY_POINT_OFFSET + HEADER_SIZE)
            fw_arm_entry_byte = input_file.read(4)
            fw_arm_entry_pt = int.from_bytes(fw_arm_entry_byte, "little")

        if start_flash_addr < fw_arm_entry_pt < end_flash_addr:
            fw_entry_pt = start_flash_addr

    input_file.close()

    with output.open("r+b") as output_file:
        output_file.seek(HDR_FW_ENTRY_POINT_OFFSET)
        output_file.write(fw_entry_pt.to_bytes(4, "little"))
    output_file.close()

    if esiost_args.verbose == REG_VERBOSE:
        print(f'- HDR - FW Entry point                   - Offset '
              f'{HDR_FW_ENTRY_POINT_OFFSET}  -  '
              f'{_hex_print_format(fw_entry_pt)}')

def _openssl_digest(filepath):
    """Computes the SHA-256 digest of a file using hashlib.

    :param filepath: Path to the file to digest.
    :return: The SHA-256 digest of the file as a bytearray.
    """
    sha256_hash = hashlib.sha256()
    with open(filepath, "rb") as f:
        # Read and update hash string value in blocks of 4K
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return bytearray(sha256_hash.digest())

def _set_firmware_length(output, esiost_args):
    """writes the flash size value to the output file
    Note: the firmware length value has already been checked before
    this method

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """

    fw_length = esiost_args.firmware_length
    fw_length_to_print = _hex_print_format(fw_length)

    with output.open("r+b") as output_file:
        output_file.seek(HDR_FW_LENGTH_OFFSET)
        output_file.write(fw_length.to_bytes(4, "big"))
        if esiost_args.verbose == REG_VERBOSE:
            print(f'- HDR - FW Length                        - Offset '
                  f'{HDR_FW_LENGTH_OFFSET}  -  '
                  f'{fw_length_to_print}')
    output_file.close()

def _set_firmware_load_hash(output, esiost_args):
    """writes the load hash value to the output file
    Note: the firmware length value has already been checked before
    this method

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """
    sha256_hash = hashlib.sha256()
    with output.open("r+b") as f:
        f.seek(HEADER_SIZE)
        # Read and update hash string value in blocks of 4K
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)

    hash_data = bytearray(sha256_hash.digest())

    with output.open("r+b") as output_file:
        output_file.seek(HDR_FW_LOAD_HASH_OFFSET)
        output_file.write(hash_data)
    output_file.close()

def _set_firmware_segment(output, esiost_args):
    """writes the segment start and size value to the output file
    Note: the firmware length value has already been checked before
    this method

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """

    fw_length = esiost_args.firmware_length

    with output.open("r+b") as output_file:
        # set segment_1 start and size
        output_file.seek(HDR_FW_SEG1_START_OFFSET)
        output_file.write(FW_HDR_SEG1_START.to_bytes(4, "little"))
        output_file.seek(HDR_FW_SEG1_SIZE_OFFSET)
        output_file.write(FW_HDR_SEG1_SIZE.to_bytes(4, "little"))

        # set segment_2 start and size
        output_file.seek(HDR_FW_SEG2_START_OFFSET)
        output_file.write(FW_HDR_SEG2_START.to_bytes(4, "little"))
        output_file.seek(HDR_FW_SEG2_SIZE_OFFSET)
        output_file.write(FW_HDR_SEG2_SIZE.to_bytes(4, "little"))

        # set segment_3 start and size
        output_file.seek(HDR_FW_SEG3_START_OFFSET)
        output_file.write(FW_HDR_SEG3_START.to_bytes(4, "little"))
        output_file.seek(HDR_FW_SEG3_SIZE_OFFSET)
        output_file.write(fw_length.to_bytes(4, "little"))

        # set segment_4 start and size
        output_file.seek(HDR_FW_SEG4_START_OFFSET)
        output_file.write(FW_HDR_SEG4_START.to_bytes(4, "little"))
        output_file.seek(HDR_FW_SEG4_SIZE_OFFSET)
        output_file.write(FW_HDR_SEG4_SIZE.to_bytes(4, "little"))

        segment1_start_to_print = _hex_print_format(FW_HDR_SEG1_START)
        segment1_size_to_print = _hex_print_format(FW_HDR_SEG1_SIZE)
        segment2_start_to_print = _hex_print_format(FW_HDR_SEG2_START)
        segment2_size_to_print = _hex_print_format(FW_HDR_SEG2_SIZE)
        segment3_start_to_print = _hex_print_format(FW_HDR_SEG3_START)
        segment3_size_to_print = _hex_print_format(fw_length)
        segment4_start_to_print = _hex_print_format(FW_HDR_SEG4_START)
        segment4_size_to_print = _hex_print_format(FW_HDR_SEG4_SIZE)

        if esiost_args.verbose == REG_VERBOSE:
            print(f'- HDR - Segment1 start address           - Offset '
                  f'{HDR_FW_SEG1_START_OFFSET} -  '
                  f'{segment1_start_to_print}')
            print(f'- HDR - Segment1 size                    - Offset '
                  f'{HDR_FW_SEG1_SIZE_OFFSET} -  '
                  f'{segment1_size_to_print}')
            print(f'- HDR - Segment2 start address           - Offset '
                  f'{HDR_FW_SEG2_START_OFFSET} -  '
                  f'{segment2_start_to_print}')
            print(f'- HDR - Segment2 size                    - Offset '
                  f'{HDR_FW_SEG2_SIZE_OFFSET} -  '
                  f'{segment2_size_to_print}')
            print(f'- HDR - Segment3 start address           - Offset '
                  f'{HDR_FW_SEG3_START_OFFSET} -  '
                  f'{segment3_start_to_print}')
            print(f'- HDR - Segment3 size                    - Offset '
                  f'{HDR_FW_SEG3_SIZE_OFFSET} -  '
                  f'{segment3_size_to_print}')
            print(f'- HDR - Segment4 start address           - Offset '
                  f'{HDR_FW_SEG4_START_OFFSET} -  '
                  f'{segment4_start_to_print}')
            print(f'- HDR - Segment4 size                    - Offset '
                  f'{HDR_FW_SEG4_SIZE_OFFSET} -  '
                  f'{segment4_size_to_print}')

    output_file.close()

def _clearup_tempfiles(output, esiost_args):
    """clearup the tempfiles

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """

    output_file = Path(output)

    seg1_file = Path("seg1_" + output_file.name)
    if seg1_file.exists():
        seg1_file.unlink()

def _set_firmware_segment_hash(output, esiost_args):
    """Writes the segment hash value to the output file.
    Note: the firmware length value has already been checked before this method.

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """

    # Generate segment files
    with output.open("r+b") as output_file:
        # seg1
        output_file.seek(HDR_FW_SEG1_START_OFFSET)
        seg1_start = int.from_bytes(output_file.read(4), "little")
        output_file.seek(HDR_FW_SEG1_SIZE_OFFSET)
        seg1_size = int.from_bytes(output_file.read(4), "little")
        output_file.seek(seg1_start)

        seg1_data = output_file.read(seg1_size)

        seg1_file_path = Path("seg1_" + output_file.name)
        with seg1_file_path.open("wb") as seg1_file:
            seg1_file.write(seg1_data)

        # set hash

        # seg1 hash
        hash_data = _openssl_digest(seg1_file_path)
        output_file.seek(HDR_FW_SEG1_HASH_OFFSET)
        output_file.write(hash_data)

        # seg3 hash
        sha256_hash = hashlib.sha256()
        output_file.seek(HEADER_SIZE)
        # Read and update hash string value in blocks of 4K
        for byte_block in iter(lambda: output_file.read(4096), b""):
            sha256_hash.update(byte_block)

        hash_data = bytearray(sha256_hash.digest())

        output_file.seek(HDR_FW_SEG3_HASH_OFFSET)
        output_file.write(hash_data)

    _clearup_tempfiles(output, esiost_args)

def _copy_image(output, esiost_args):
    """copies the fw image from the input file to the output file
    if firmware header offset is defined, just copies the input file to the
    output file

    :param output: the output file object,
    :param esiost_args: the object representing the command line arguments.
    """

    # check input file offset
    with open(esiost_args.input, "rb") as firmware_image:
        firmware_image.seek(ARM_FW_ENTRY_POINT_OFFSET)
        fw_arm_entry_byte = firmware_image.read(4)
        fw_arm_entry_pt = int.from_bytes(fw_arm_entry_byte, "little")
        if fw_arm_entry_pt != 0:
            image_offset = 0
            input_file_size = Path(esiost_args.input).stat().st_size
        else:
            firmware_image.seek(ARM_FW_ENTRY_POINT_OFFSET + HEADER_SIZE)
            fw_arm_entry_byte = firmware_image.read(4)
            fw_arm_entry_pt = int.from_bytes(fw_arm_entry_byte, "little")
            if fw_arm_entry_pt == 0:
                sys.exit(EXIT_FAILURE_STATUS)
            else:
                image_offset = 1
                input_file_size = Path(esiost_args.input).stat().st_size - HEADER_SIZE

    firmware_image.close()

    with open(esiost_args.input, "rb") as firmware_image:
        with open(output, "r+b") as output_file:
            if image_offset == 0:
                output_file.seek(HEADER_SIZE)
            for line in firmware_image:
                output_file.write(line)
        output_file.close()
    firmware_image.close()

    # update firmware length if needed
    fw_length = esiost_args.firmware_length
    if fw_length is None:
        esiost_args.firmware_length = input_file_size

def _hex_print_format(value):
    """hex representation of an integer

    :param value: an integer to be represented in hex
    """
    return "0x{:08x}".format(value)

def _exit_with_failure_delete_file(output, message):
    """formatted failure message printer, prints the
    relevant error message, deletes the output file,
    and exits the application.

    :param message: the error message to be printed
    """
    output_file = Path(output)
    if output_file.exists():
        output_file.unlink()

    message = '\n' + message
    message += '\n'
    message += '******************************\n'
    message += '***        FAILED          ***\n'
    message += '******************************\n'
    print(Fore.RED + message)

    sys.exit(EXIT_FAILURE_STATUS)

def _exit_with_success():
    """formatted success message printer, prints the
    success message and exits the application.
    """
    message = '\n'
    message += '******************************\n'
    message += '***        SUCCESS         ***\n'
    message += '******************************\n'
    print(Fore.GREEN + message)

    sys.exit(EXIT_SUCCESS_STATUS)

def main():
    """main of the application
    """
    init()  # colored print initialization for windows

    if len(sys.argv) < 2:
        sys.exit(EXIT_FAILURE_STATUS)

    esiost_obj = EsiostArgs()

    if esiost_obj.error_args:
        for err_arg in esiost_obj.error_args:
            message = f'unKnown flag: {err_arg}'
            exit_with_failure(message)
        sys.exit(EXIT_SUCCESS_STATUS)

    # Start to handle booter header table
    _bt_mode_handler(esiost_obj)

if __name__ == '__main__':
    main()
