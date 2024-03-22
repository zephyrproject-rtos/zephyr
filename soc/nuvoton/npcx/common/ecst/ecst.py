#!/usr/bin/env python3
#
# Copyright (c) 2020 Nuvoton Technology Corporation
#
# SPDX-License-Identifier: Apache-2.0

# This script will append/paste specific header to tell ROM code (Booter) of
# NPCX EC series how to load the firmware from flash to code ram
# Usage python3 ${ZEPHYR_BASE}/scripts/ecst.py
#                    -i in_file.bin -o out_file.bin
#                    [-chip <name>] [-v|-vv]
#                    [-nohcrc] [-nofcrc] [-ph <offset>]
#                    [-flashsize <1|2|4|8|16>]
#                    [-spimaxclk <20|25|33|40|50>]
#                    [-spireadmode <normal|fast|dual|quad>]

import sys
from colorama import init, Fore
from ecst_args import EcstArgs, exit_with_failure
from pathlib import Path

# ECST Version
ECST_VER = "2.0.1"

# Offsets inside the header
HDR_ANCHOR_OFFSET = 0
HDR_EXTENDED_ANCHOR_OFFSET = 4
HDR_SPI_MAX_CLK_OFFSET = 6
HDR_SPI_READ_MODE_OFFSET = 7
HDR_ERR_DETECTION_CONF_OFFSET = 8
HDR_FW_LOAD_START_ADDR_OFFSET = 9
HDR_FW_ENTRY_POINT_OFFSET = 13
HDR_FW_ERR_DETECT_START_ADDR_OFFSET = 17
HDR_FW_ERR_DETECT_END_ADDR_OFFSET = 21
HDR_FW_LENGTH_OFFSET = 25
HDR_FLASH_SIZE_OFFSET = 29
OTP_WRITE_PROTECT_OFFSET = 30
KEY_VALID_OFFSET = 31
FIRMWARE_VALID_OFFSET = 32
RESERVED_BYTES_OFFSET = 33
HDR_FW_HEADER_SIG_OFFSET = 56
HDR_FW_IMAGE_SIG_OFFSET = 60
FW_IMAGE_OFFSET = 64

SIGNATURE_OFFSET = 0
POINTER_OFFSET = 4
ARM_FW_ENTRY_POINT_OFFSET = 4

# Header field known values
FW_HDR_ANCHOR = 0x2A3B4D5E
FW_HDR_EXT_ANCHOR_ENABLE = 0xAB1E
FW_HDR_EXT_ANCHOR_DISABLE = 0x54E1
FW_HDR_CRC_DISABLE = 0x00
FW_HDR_CRC_ENABLE = 0x02
FW_CRC_DISABLE = 0x00
FW_CRC_ENABLE = 0x02
HDR_PTR_SIGNATURE = 0x55AA650E

BOOTLOADER_TABLE_MODE = "bt"

# SPI related values
SPI_MAX_CLOCK_20_MHZ_VAL = "20"
SPI_MAX_CLOCK_25_MHZ_VAL = "25"
SPI_MAX_CLOCK_33_MHZ_VAL = "33"
SPI_MAX_CLOCK_40_MHZ_VAL = "40"
SPI_MAX_CLOCK_50_MHZ_VAL = "50"

SPI_MAX_CLOCK_20_MHZ = 0x00
SPI_MAX_CLOCK_25_MHZ = 0x01
SPI_MAX_CLOCK_33_MHZ = 0x02
SPI_MAX_CLOCK_40_MHZ = 0x03
SPI_MAX_CLOCK_50_MHZ = 0x04

SPI_CLOCK_RATIO_1_VAL = 1
SPI_CLOCK_RATIO_2_VAL = 2

SPI_CLOCK_RATIO_1 = 0x00
SPI_CLOCK_RATIO_2 = 0x08

SPI_NORMAL_MODE_VAL = 'normal'
SPI_SINGLE_MODE_VAL = 'fast'
SPI_DUAL_MODE_VAL = 'dual'
SPI_QUAD_MODE_VAL = 'quad'

SPI_NORMAL_MODE = 0x00
SPI_SINGLE_MODE = 0x01
SPI_DUAL_MODE = 0x03
SPI_QUAD_MODE = 0x04

# Flash related values
FLASH_SIZE_1_MBYTES_VAL = "1"
FLASH_SIZE_2_MBYTES_VAL = "2"
FLASH_SIZE_4_MBYTES_VAL = "4"
FLASH_SIZE_8_MBYTES_VAL = "8"
FLASH_SIZE_16_MBYTES_VAL = "16"

FLASH_SIZE_1_MBYTES = 0x01
FLASH_SIZE_2_MBYTES = 0x03
FLASH_SIZE_4_MBYTES = 0x07
FLASH_SIZE_8_MBYTES = 0x0f
FLASH_SIZE_8_MBYTES = 0x0f
FLASH_SIZE_16_MBYTES = 0x1f

MAX_FLASH_SIZE = 0x03ffffff

# Header fields default values.
ADDR_16_BYTES_ALIGNED_MASK = 0x0000000f
ADDR_4_BYTES_ALIGNED_MASK = 0x00000003
ADDR_4K_BYTES_ALIGNED_MASK = 0x00000fff

NUM_OF_BYTES = 32
INVALID_INPUT = -1
HEADER_SIZE = 64
PAD_BYTE = b'\x00'
BYTES_TO_PAD = HDR_FW_HEADER_SIG_OFFSET - RESERVED_BYTES_OFFSET

# Verbose related values
NO_VERBOSE = 0
REG_VERBOSE = 1
SUPER_VERBOSE = 1

# Success/failure codes
EXIT_SUCCESS_STATUS = 0
EXIT_FAILURE_STATUS = 1

def _bt_mode_handler(ecst_args):
    """creates the bootloader table using the provided arguments.

    :param ecst_args: the object representing the command line arguments.
    """

    output_file = _set_input_and_output(ecst_args)
    _check_chip(output_file, ecst_args)

    if ecst_args.paste_firmware_header != 0:
        _check_firmware_header_offset(output_file, ecst_args)

    _copy_image(output_file, ecst_args)
    _set_anchor(output_file, ecst_args)
    _set_extended_anchor(output_file, ecst_args)
    _set_spi_flash_maximum_clock(output_file, ecst_args)
    _set_spi_flash_mode(output_file, ecst_args)
    _set_error_detection_configuration(output_file, ecst_args)
    _set_firmware_load_start_address(output_file, ecst_args)
    _set_firmware_entry_point(output_file, ecst_args)
    _set_firmware_crc_start_and_size(output_file, ecst_args)
    _set_firmware_length(output_file, ecst_args)
    _set_flash_size(output_file, ecst_args)
    _set_reserved_bytes(output_file, ecst_args)
    _set_firmware_header_crc_signature(output_file, ecst_args)
    _set_firmware_image_crc_signature(output_file, ecst_args)

    _exit_with_success()

def _set_input_and_output(ecst_args):
    """checks the input file and output and sets the output file.

    checks input file existence, creates an output file according
    to the 'output' argument.

    Note: input file size has to be greater than 0, and named differently
    from output file

    :param ecst_args: the object representing the command line arguments.

    :returns: output file path object, or -1 if fails
    """
    input_file = ecst_args.input
    output = ecst_args.output
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

    if ecst_args.verbose == REG_VERBOSE:
        print(Fore.LIGHTCYAN_EX + f'\nBIN file: {input_file}, size:'
              f' {input_file_size} bytes')
        print(f'Output file name: {output_file.name} \n')

    return output_file

def _check_chip(output, ecst_args):
    """checks if the chip entered is a legal chip, generates an error
    and closes the application, deletes the output file if the chip name
    is illegal.

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """

    if ecst_args.chip_name == INVALID_INPUT:
        message = f'Invalid chip name, '
        message += "should be npcx4m3, npcx4m8, npcx9m8, npcx9m7, npcx9m6, " \
                   "npcx7m7, npcx7m6, npcx7m5."
        _exit_with_failure_delete_file(output, message)

def _set_anchor(output, ecst_args):
    """writes the anchor value to the output file

    :param output: the output file object.
    :param ecst_args: the object representing the command line arguments.
    """

    with output.open("r+b") as output_file:
        output_file.seek(HDR_ANCHOR_OFFSET + ecst_args.paste_firmware_header)
        output_file.write(FW_HDR_ANCHOR.to_bytes(4, "little"))
        if ecst_args.verbose == REG_VERBOSE:
            print(f'- HDR - FW Header ANCHOR                 - Offset '
                  f'{HDR_ANCHOR_OFFSET}  -  {_hex_print_format(FW_HDR_ANCHOR)}')

    output_file.close()

def _set_extended_anchor(output, ecst_args):
    """writes the extended anchor value to the output file

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """

    with output.open("r+b") as output_file:
        output_file.seek(HDR_EXTENDED_ANCHOR_OFFSET + \
        ecst_args.paste_firmware_header)
        if ecst_args.firmware_header_crc is FW_HDR_CRC_ENABLE:
            output_file.write(FW_HDR_EXT_ANCHOR_ENABLE.to_bytes(2, "little"))
            anchor_to_print = _hex_print_format(FW_HDR_EXT_ANCHOR_ENABLE)
        else:
            output_file.write(FW_HDR_EXT_ANCHOR_DISABLE.to_bytes(2, "little"))
            anchor_to_print = _hex_print_format(FW_HDR_EXT_ANCHOR_DISABLE)

        if ecst_args.verbose == REG_VERBOSE:
            print(f'- HDR - Header EXTENDED ANCHOR           - Offset'
                  f' {HDR_EXTENDED_ANCHOR_OFFSET}  -  {anchor_to_print}')
        output_file.close()


def _check_firmware_header_offset(output, ecst_args):
    """checks if the firmware header offset entered is valid.
    proportions:

    firmware header offset is a non-negative integer.
    firmware header offset is 16 bytes aligned
    firmware header offset equals/smaller than input file minus
    FW HEADER SIZE (64 KB)
    input file size is bigger than FW HEADER SIZE (64 KB)

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """

    input_file = Path(ecst_args.input)
    paste_fw_offset = ecst_args.paste_firmware_header
    input_file_size = input_file.stat().st_size

    if paste_fw_offset == INVALID_INPUT:
        _exit_with_failure_delete_file(output, "Cannot read paste"
                                               " firmware offset")

    paste_fw_offset_to_print = _hex_print_format(paste_fw_offset)

    if paste_fw_offset & ADDR_16_BYTES_ALIGNED_MASK != 0:
        message = f'Paste firmware address ({paste_fw_offset_to_print}) ' \
            f'is not 16 bytes aligned'
        _exit_with_failure_delete_file(output, message)

    if input_file_size <= HEADER_SIZE:
        message = f' input file size ({input_file_size} bytes) ' \
            f'should be bigger than fw header size ({HEADER_SIZE} bytes)'
        _exit_with_failure_delete_file(output, message)

    if input_file_size - HEADER_SIZE < paste_fw_offset:
        message = f'FW offset ({paste_fw_offset_to_print})should be less ' \
            f'than input file size ({input_file_size}) minus fw header size' \
            f' {HEADER_SIZE}'
        _exit_with_failure_delete_file(output, message)

def _set_spi_flash_maximum_clock(output, ecst_args):
    """Sets the maximum allowable clock frequency (for firmware loading);
    also sets the ratio between the Core clock
    and the SPI clock for the specified mode.
    writes the data into the output file.
    the application is closed, and an error is generated
    if the fields are not valid.

    Bits 2-0 - SPI MAX Clock
    Bit 3: SPI Clock Ratio

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """
    spi_max_clock_to_write = 0

    with output.open("r+b") as output_file:
        output_file.seek(HDR_SPI_MAX_CLK_OFFSET +
                         ecst_args.paste_firmware_header)
        spi_max_clock = ecst_args.spi_flash_maximum_clock
        spi_clock_ratio = ecst_args.spi_flash_clock_ratio

        if spi_clock_ratio == SPI_CLOCK_RATIO_1_VAL:
            spi_clock_ratio = SPI_CLOCK_RATIO_1
        elif spi_clock_ratio == SPI_CLOCK_RATIO_2_VAL:
            spi_clock_ratio = SPI_CLOCK_RATIO_2
        elif spi_clock_ratio == INVALID_INPUT:
            message = f'Cannot read SPI Clock Ratio'
            output_file.close()
            _exit_with_failure_delete_file(output, message)
        else:
            message = f'Invalid SPI Core Clock Ratio (3) - it should be 1 or 2'
            output_file.close()
            _exit_with_failure_delete_file(output, message)

        if spi_max_clock == SPI_MAX_CLOCK_20_MHZ_VAL:
            spi_max_clock_to_write = SPI_MAX_CLOCK_20_MHZ | spi_clock_ratio
            output_file.write(spi_max_clock_to_write.to_bytes(1, "little"))

        elif spi_max_clock == SPI_MAX_CLOCK_25_MHZ_VAL:
            spi_max_clock_to_write = SPI_MAX_CLOCK_25_MHZ | spi_clock_ratio
            output_file.write(spi_max_clock_to_write.to_bytes(1, "little"))

        elif spi_max_clock == SPI_MAX_CLOCK_33_MHZ_VAL:
            spi_max_clock_to_write = SPI_MAX_CLOCK_33_MHZ | spi_clock_ratio
            output_file.write(spi_max_clock_to_write.to_bytes(1, "little"))

        elif spi_max_clock == SPI_MAX_CLOCK_40_MHZ_VAL:
            spi_max_clock_to_write = SPI_MAX_CLOCK_40_MHZ | spi_clock_ratio
            output_file.write(spi_max_clock_to_write.to_bytes(1, "little"))

        elif spi_max_clock == SPI_MAX_CLOCK_50_MHZ_VAL:
            spi_max_clock_to_write = SPI_MAX_CLOCK_50_MHZ | spi_clock_ratio
            output_file.write(spi_max_clock_to_write.to_bytes(1, "little"))

        elif not str(spi_max_clock).isdigit():
            output_file.close()
            _exit_with_failure_delete_file(output,
                                           "Cannot read SPI Flash Max Clock")
        else:
            message = f'Invalid SPI Flash MAX Clock size ({spi_max_clock}) '
            message += '- it should be 20, 25, 33, 40 or 50 MHz'
            output_file.close()
            _exit_with_failure_delete_file(output, message)

        if ecst_args.verbose == REG_VERBOSE:
            print(f'- HDR - SPI flash MAX Clock              - Offset '
                  f'{HDR_SPI_MAX_CLK_OFFSET}  -  '
                  f'{_hex_print_format(spi_max_clock_to_write)}')
        output_file.close()

def _set_spi_flash_mode(output, ecst_args):
    """Sets the read mode used for firmware loading and enables
    the Unlimited Burst functionality.
    writes the data into the output file.
    the application is closed, and an error is generated
    if the fields are not valid.

    Bits 2-0 - SPI Flash Read Mode
    Bit 3: Unlimited Burst Mode

    Note: unlimburst is not relevant for npcx5mn chips family.

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """

    spi_flash_read_mode = ecst_args.spi_flash_read_mode
    spi_unlimited_burst_mode = ecst_args.unlimited_burst_mode
    spi_read_mode_to_write = 0

    with output.open("r+b") as output_file:
        output_file.seek(HDR_SPI_READ_MODE_OFFSET +
                         ecst_args.paste_firmware_header)
        if spi_flash_read_mode == SPI_NORMAL_MODE_VAL:
            spi_read_mode_to_write = SPI_NORMAL_MODE | spi_unlimited_burst_mode
            output_file.write(spi_read_mode_to_write.to_bytes(1, "little"))
        elif spi_flash_read_mode == SPI_SINGLE_MODE_VAL:
            spi_read_mode_to_write = SPI_SINGLE_MODE | spi_unlimited_burst_mode
            output_file.write(spi_read_mode_to_write.to_bytes(1, "little"))
        elif spi_flash_read_mode == SPI_DUAL_MODE_VAL:
            spi_read_mode_to_write = SPI_DUAL_MODE | spi_unlimited_burst_mode
            output_file.write(spi_read_mode_to_write.to_bytes(1, "little"))
        elif spi_flash_read_mode == SPI_QUAD_MODE_VAL:
            spi_read_mode_to_write = SPI_QUAD_MODE | spi_unlimited_burst_mode
            output_file.write(spi_read_mode_to_write.to_bytes(1, "little"))
        else:
            message = f'Invalid SPI Flash Read Mode ({spi_flash_read_mode}),'
            message += 'it should be normal, fast, dual, quad'
            output_file.close()
            _exit_with_failure_delete_file(output, message)

        if ecst_args.verbose == REG_VERBOSE:
            print(f'- HDR - SPI flash Read Mode              - Offset '
                  f'{HDR_SPI_READ_MODE_OFFSET}  -  '
                  f'{_hex_print_format(spi_read_mode_to_write)}')

    output_file.close()

def _set_error_detection_configuration(output, ecst_args):
    """writes the error detection configuration value (enabled/disabled)
    to the output file

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """
    with output.open("r+b") as output_file:
        output_file.seek(HDR_ERR_DETECTION_CONF_OFFSET +
                         ecst_args.paste_firmware_header)
        if ecst_args.firmware_crc == FW_CRC_ENABLE:
            err_detect_config_to_print = _hex_print_format(FW_CRC_ENABLE)
            output_file.write(FW_CRC_ENABLE.to_bytes(1, "little"))
        else:
            err_detect_config_to_print = _hex_print_format(FW_CRC_DISABLE)
            output_file.write(FW_CRC_DISABLE.to_bytes(1, "little"))

        if ecst_args.verbose == REG_VERBOSE:
            print(f'- HDR - FW CRC Enabled                   - Offset '
                  f'{HDR_ERR_DETECTION_CONF_OFFSET}  -  '
                  f'{err_detect_config_to_print}')

        output_file.close()

def _set_firmware_load_start_address(output, ecst_args):
    """writes the fw load address to the output file

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """
    start_ram = ecst_args.chip_ram_address
    end_ram = start_ram + ecst_args.chip_ram_size
    fw_load_addr = ecst_args.firmware_load_address
    fw_length = ecst_args.firmware_length
    fw_end_addr = fw_load_addr + fw_length

    start_ram_to_print = _hex_print_format(start_ram)
    end_ram_to_print = _hex_print_format(end_ram)
    fw_load_addr_to_print = _hex_print_format(fw_load_addr)
    fw_length_to_print = _hex_print_format(fw_length)
    fw_end_addr_to_print = _hex_print_format(fw_end_addr)

    if fw_length == INVALID_INPUT:
        message = f'Cannot read firmware length'
        _exit_with_failure_delete_file(output, message)

    if fw_length & ADDR_16_BYTES_ALIGNED_MASK != 0:
        message = f'Firmware length ({fw_length_to_print}) ' \
            f'is not 16 bytes aligned'
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

    if fw_end_addr > end_ram:
        message = f'Firmware end address ({fw_end_addr_to_print}) should be '
        message += f'less than end of RAM address ({end_ram_to_print})'
        _exit_with_failure_delete_file(output, message)

    with output.open("r+b") as output_file:
        output_file.seek(HDR_FW_LOAD_START_ADDR_OFFSET +
                         ecst_args.paste_firmware_header)
        output_file.write(ecst_args.firmware_load_address.
                          to_bytes(4, "little"))

        output_file.seek(HDR_FW_LENGTH_OFFSET +
                         ecst_args.paste_firmware_header)
        output_file.write(fw_length.to_bytes(4, "little"))

        if ecst_args.verbose == REG_VERBOSE:
            print(f'- HDR - FW load start address            - Offset '
                  f'{HDR_FW_LOAD_START_ADDR_OFFSET}  -  '
                  f'{fw_load_addr_to_print}')
    output_file.close()

def _set_firmware_entry_point(output, ecst_args):
    """writes the fw entry point to the output file.
    proportions:

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """
    entry_pt_none = False
    input_file_path = Path(ecst_args.input)
    fw_entry_pt = ecst_args.firmware_entry_point
    fw_use_arm_reset = ecst_args.use_arm_reset
    fw_length = ecst_args.firmware_length
    fw_load_addr = ecst_args.firmware_load_address
    fw_end_addr = fw_load_addr + fw_length

    # check if fwep flag wasn't set and set it to fw load address if needed
    if fw_entry_pt is None:
        entry_pt_none = True
        fw_entry_pt = ecst_args.firmware_load_address

    if not entry_pt_none and fw_use_arm_reset:
        message = f'-usearmrst not allowed, FW entry point already set using '\
            f'-fwep !'
        _exit_with_failure_delete_file(output, message)

    with input_file_path.open("r+b") as input_file:
        with output.open("r+b") as output_file:
            if fw_use_arm_reset:
                input_file.seek(ARM_FW_ENTRY_POINT_OFFSET +
                                ecst_args.paste_firmware_header)
                fw_entry_byte = input_file.read(4)
                fw_entry_pt = int.from_bytes(fw_entry_byte, "little")

            if fw_entry_pt < fw_load_addr or fw_entry_pt > fw_end_addr:
                output_file.close()
                input_file.close()
                message = f'Firmware entry point ' \
                    f'({_hex_print_format(fw_entry_pt)}) ' \
                    f'should be between the FW load address ' \
                    f'({_hex_print_format(fw_load_addr)})' \
                    f' and FW end address ({_hex_print_format(fw_end_addr)})'

                _exit_with_failure_delete_file(output, message)

            output_file.seek(HDR_FW_ENTRY_POINT_OFFSET +
                             ecst_args.paste_firmware_header)
            output_file.write(fw_entry_pt.to_bytes(4, "little"))
        output_file.close()
    input_file.close()

    if ecst_args.verbose == REG_VERBOSE:
        print(f'- HDR - FW Entry point                   - Offset '
              f'{HDR_FW_ENTRY_POINT_OFFSET} -  '
              f'{_hex_print_format(fw_entry_pt)}')

def _set_firmware_crc_start_and_size(output, ecst_args):
    """writes the fw crc start address and the crc size to the output file.
    proportions:
    --crc start address should be 4 byte aligned, bigger than crc end address
    --crc size should be 4 byte aligned, and be set to firmware length minus
    crc start offset by default
    --crc end address is crc start address + crc size bytes

    the application is closed, and an error is generated if the mentioned
    fields not comply with the proportions.

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """
    fw_crc_size = ecst_args.firmware_crc_size
    fw_crc_start = ecst_args.firmware_crc_start

    if fw_crc_size is None:
        fw_crc_end = ecst_args.firmware_length - 1
        # default value for crc size
        fw_crc_size = ecst_args.firmware_length - fw_crc_start
        ecst_args.firmware_crc_size = fw_crc_size
    else:
        fw_crc_end = fw_crc_start + fw_crc_size - 1

    fw_crc_start_to_print = _hex_print_format(fw_crc_start)
    fw_crc_end_to_print = _hex_print_format(fw_crc_end)
    fw_length_to_print = _hex_print_format(ecst_args.firmware_length)
    fw_crc_size_to_print = _hex_print_format(fw_crc_size)

    if fw_crc_start & ADDR_4_BYTES_ALIGNED_MASK != 0:
        message = f'Firmware crc offset address ' \
            f'({fw_crc_start_to_print}) is not 4 bytes aligned'
        _exit_with_failure_delete_file(output, message)

    if fw_crc_start > fw_crc_end:
        message = f'CRC start address ({fw_crc_start_to_print}) should' \
            f' be less or equal to CRC end address ({fw_crc_end_to_print}) \n'
        _exit_with_failure_delete_file(output, message)

    if fw_crc_size & ADDR_4_BYTES_ALIGNED_MASK != 0:
        message = f'Firmware crc size ({fw_crc_size_to_print}) ' \
            f'is not 4 bytes aligned'
        _exit_with_failure_delete_file(output, message)

    if fw_crc_end > ecst_args.firmware_length - 1:
        message = f'CRC end address ({fw_crc_end_to_print}) should be less' \
            f' than the FW length ({fw_length_to_print}) \n'
        _exit_with_failure_delete_file(output, message)

    with output.open("r+b") as output_file:
        output_file.seek(HDR_FW_ERR_DETECT_START_ADDR_OFFSET +
                         ecst_args.paste_firmware_header)
        output_file.write(fw_crc_start.to_bytes(4, "little"))

        output_file.seek(HDR_FW_ERR_DETECT_END_ADDR_OFFSET +
                         ecst_args.paste_firmware_header)
        output_file.write(fw_crc_end.to_bytes(4, "little"))
        output_file.close()

    if ecst_args.verbose == REG_VERBOSE:
        print(f'- HDR - FW CRC Start                     - Offset '
              f'{HDR_FW_ERR_DETECT_START_ADDR_OFFSET} -  '
              f'{fw_crc_start_to_print}')

        print(f'- HDR - FW CRC End                       - Offset '
              f'{HDR_FW_ERR_DETECT_END_ADDR_OFFSET} -  '
              f'{fw_crc_end_to_print}')

def _set_firmware_length(output, ecst_args):
    """writes the flash size value to the output file
    Note: the firmware length value has already been checked before
    this method

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """

    fw_length = ecst_args.firmware_length
    fw_length_to_print = _hex_print_format(fw_length)

    with output.open("r+b") as output_file:
        output_file.seek(HDR_FW_LENGTH_OFFSET +
                         ecst_args.paste_firmware_header)
        output_file.write(fw_length.to_bytes(4, "little"))
        if ecst_args.verbose == REG_VERBOSE:
            print(f'- HDR - FW Length                        - Offset '
                  f'{HDR_FW_LENGTH_OFFSET} -  '
                  f'{fw_length_to_print}')
    output_file.close()

def _set_flash_size(output, ecst_args):
    """writes the flash size value to the output file
    valid values are 1,2,4,8 or 16 (default is 16).
    the application is closed and the output file is deleted
    if the flash size is invalid

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """
    flash_size_to_print = ""

    with output.open("r+b") as output_file:
        output_file.seek(HDR_FLASH_SIZE_OFFSET +
                         ecst_args.paste_firmware_header)
        flash_size = ecst_args.flash_size

        if flash_size == FLASH_SIZE_1_MBYTES_VAL:
            output_file.write(FLASH_SIZE_1_MBYTES.to_bytes(4, "little"))
            flash_size_to_print = _hex_print_format(FLASH_SIZE_1_MBYTES)
        elif flash_size == FLASH_SIZE_2_MBYTES_VAL:
            output_file.write(FLASH_SIZE_2_MBYTES.to_bytes(4, "little"))
            flash_size_to_print = _hex_print_format(FLASH_SIZE_2_MBYTES)
        elif flash_size == FLASH_SIZE_4_MBYTES_VAL:
            output_file.write(FLASH_SIZE_4_MBYTES.to_bytes(4, "little"))
            flash_size_to_print = _hex_print_format(FLASH_SIZE_4_MBYTES)
        elif flash_size == FLASH_SIZE_8_MBYTES_VAL:
            output_file.write(FLASH_SIZE_8_MBYTES.to_bytes(4, "little"))
            flash_size_to_print = _hex_print_format(FLASH_SIZE_8_MBYTES)
        elif flash_size == FLASH_SIZE_16_MBYTES_VAL:
            output_file.write(FLASH_SIZE_16_MBYTES.to_bytes(4, "little"))
            flash_size_to_print = _hex_print_format(FLASH_SIZE_16_MBYTES)
        elif not flash_size.isdigit():
            output_file.close()
            _exit_with_failure_delete_file(output, "Cannot read Flash size")
        else:
            message = f'Invalid flash size ({flash_size} MBytes), ' \
                f' it should be 0.5, 1, 2, 4, 8, 16 MBytes \n' \
                f' please note - for 0.5 MBytes flash, enter \'1\' '
            output_file.close()
            _exit_with_failure_delete_file(output, message)

    if ecst_args.verbose == REG_VERBOSE:
        print(f'- HDR - Flash size                       - Offset '
              f'{HDR_FLASH_SIZE_OFFSET} - '
              f' {flash_size_to_print}')
    output_file.close()

def _set_reserved_bytes(output, ecst_args):
    """fills the reserved part of the image at the relevant offset
    with the PAD_BYTE

    :param output: the output file object
    :param ecst_args: the object representing the command line arguments.
    """
    with output.open("r+b") as output_file:
        for i in range(BYTES_TO_PAD):
            output_file.seek(RESERVED_BYTES_OFFSET +
                             ecst_args.paste_firmware_header + i)
            output_file.write(PAD_BYTE)
    output_file.close()

def _set_firmware_header_crc_signature(output, ecst_args):
    """writes the firmware header crc signature (4 bytes)
    to the output file

    :param output: the output file object
    :param ecst_args: the object representing the command line arguments.
    """
    crc_to_print = _hex_print_format(0)

    # calculating crc only if the header crc check is enabled
    if ecst_args.firmware_header_crc != FW_HDR_CRC_DISABLE:

        with output.open("r+b") as output_file:
            crc_calc = 0xffffffff
            table = _create_table()

            for i in range(HDR_FW_HEADER_SIG_OFFSET):
                output_file.seek(ecst_args.paste_firmware_header + i)
                current = output_file.read(1)
                crc_calc = _crc_update(
                    int.from_bytes(current, "little"), crc_calc, table)

            crc = _finalize_crc(crc_calc)
            crc_to_write = crc.to_bytes(4, "little")
            crc_to_print = _hex_print_format(crc)
            output_file.seek(ecst_args.paste_firmware_header +
                             HDR_FW_HEADER_SIG_OFFSET)
            output_file.write(crc_to_write)

        output_file.close()

    if ecst_args.verbose == REG_VERBOSE:
        print(f'- HDR - Header CRC                       - Offset '
              f'{HDR_FW_HEADER_SIG_OFFSET} - '
              f' {crc_to_print}')

def _set_firmware_image_crc_signature(output, ecst_args):
    """writes the firmware image crc signature (4 bytes)
    to the output file.

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """
    # calculating crc only if the image crc check is enabled
    crc_to_print = _hex_print_format(0)
    if ecst_args.firmware_crc != FW_CRC_DISABLE:

        with output.open("r+b") as output_file:
            crc_calc = 0xffffffff
            table = _create_table()

            output_file.seek(FW_IMAGE_OFFSET + ecst_args.paste_firmware_header +
                             ecst_args.firmware_crc_start)
            for _ in range(ecst_args.firmware_crc_size):
                current = output_file.read(1)
                crc_calc = _crc_update(int.from_bytes(current, "little"), \
                crc_calc, table)

            crc = _finalize_crc(crc_calc)
            crc_to_write = crc.to_bytes(4, "little")
            crc_to_print = _hex_print_format(crc)
            output_file.seek(HDR_FW_IMAGE_SIG_OFFSET +
                             ecst_args.paste_firmware_header)
            output_file.write(crc_to_write)

        output_file.close()

    if ecst_args.verbose == REG_VERBOSE:
        print(f'- HDR - Header CRC                       - Offset '
              f'{HDR_FW_IMAGE_SIG_OFFSET} - '
              f' {crc_to_print}')

def _copy_image(output, ecst_args):
    """copies the fw image from the input file to the output file
    if firmware header offset is defined, just copies the input file to the
    output file

    :param output: the output file object,
    :param ecst_args: the object representing the command line arguments.
    """
    with open(ecst_args.input, "rb") as firmware_image:
        with open(output, "r+b") as output_file:
            if ecst_args.paste_firmware_header == 0:
                output_file.seek(FW_IMAGE_OFFSET)
            else:
                output_file.seek(0)
            for line in firmware_image:
                output_file.write(line)
        output_file.close()
    firmware_image.close()

    # pad fw image to be 16 byte aligned if needed
    input_file_size = Path(ecst_args.input).stat().st_size
    bytes_to_pad_num = abs((16 - input_file_size) % 16)

    with open(output, "r+b") as output_file:
        i = bytes_to_pad_num
        while i != 0:
            output_file.seek(0, 2)  # seek end of file
            output_file.write(PAD_BYTE)
            i -= 1
    output_file.close()

    # update firmware length if needed
    fw_length = ecst_args.firmware_length
    if fw_length is None:
        if ecst_args.paste_firmware_header == 0:
            ecst_args.firmware_length = input_file_size + bytes_to_pad_num
        else:
            ecst_args.firmware_length = input_file_size - HEADER_SIZE - \
                                                ecst_args.paste_firmware_header

def _merge_file_with_bt_header(ecst_args):
    """Merge the BT with the BH according to the bhoffset and pointer flags

    :param ecst_args: the object representing the command line arguments.
    """
    bh_index = ecst_args.bh_offset
    file_name_to_print = ""

    if not ecst_args.input:
        exit_with_failure('No input BIN file selected for'
                          'Bootloader header file.')
    else:
        input_file = Path(ecst_args.input)
        if not input_file.exists():
            exit_with_failure(f'Cannot open {ecst_args.input}')

        if input_file.stat().st_size < bh_index:

            message = f'Bootloader header offset ({bh_index} bytes) should be '
            message += f'less than file size ' \
                f'({input_file.stat().st_size } bytes)'
            exit_with_failure(message)

        # create a file if the output parameter is not None,
        # otherwise write to the input file
        if ecst_args.output is not None:
            output_file = Path(ecst_args.output)
            output_file.touch()
            with input_file.open("r+b") as firmware_image:
                with output_file.open("r+b") as output_file:
                    file_name_to_print = output_file.name
                    for line in firmware_image:
                        output_file.write(line)
                    output_file.seek(bh_index)
                    output_file.write(HDR_PTR_SIGNATURE.to_bytes(4, "little"))
                    output_file.seek(bh_index + 4)
                    output_file.write(ecst_args.pointer.to_bytes(4, "little"))
                output_file.close()
            firmware_image.close()

        else:
            with input_file.open("r+b") as file_to_merge:
                file_name_to_print = file_to_merge.name
                file_to_merge.seek(bh_index + SIGNATURE_OFFSET)
                file_to_merge.write(HDR_PTR_SIGNATURE.to_bytes(4, "little"))
                file_to_merge.seek(bh_index + POINTER_OFFSET)
                file_to_merge.write(ecst_args.pointer.to_bytes(4, "little"))
            file_to_merge.close()

    if ecst_args.verbose == REG_VERBOSE:
        print(Fore.LIGHTCYAN_EX + f'BootLoader Header file:'
              f' {file_name_to_print}')
        print(f'Offset: {_hex_print_format(ecst_args.bh_offset)},'
              f' Signature: {_hex_print_format(HDR_PTR_SIGNATURE)},'
              f' Pointer: {_hex_print_format(ecst_args.pointer)}')

def _create_bt_header(ecst_args):
    """create bootloader table header, consist of 4 bytes signature and
    4 bytes pointer

    :param ecst_args: the object representing the command line arguments.
    """
    if not ecst_args.output:
        exit_with_failure("No output file selected for "
                          "Bootloader header file.")
    else:
        output_file = Path(ecst_args.output)
        if not output_file.exists():
            output_file.touch()
        with open(output_file, "r+b") as boot_loader_header_file:
            boot_loader_header_file.seek(SIGNATURE_OFFSET)
            boot_loader_header_file.write(HDR_PTR_SIGNATURE.to_bytes(4, \
            "little"))
            boot_loader_header_file.seek(POINTER_OFFSET)
            boot_loader_header_file.write(ecst_args.pointer.to_bytes(4, \
            "little"))
        boot_loader_header_file.close()
        if ecst_args.verbose == REG_VERBOSE:
            print(Fore.LIGHTCYAN_EX + f'BootLoader Header file:'
                  f' {output_file.name}')
            print(f'Signature: {_hex_print_format(HDR_PTR_SIGNATURE)}, '
                  f'Pointer: {_hex_print_format(ecst_args.pointer)}')

def _create_table():
    """helper for crc calculation"""
    table = []
    for i in range(256):
        k = i
        for _ in range(8):
            if k & 1:
                k = (k >> 1) ^ 0xEDB88320
            else:
                k >>= 1
        table.append(k)
    return table

def _crc_update(cur, crc, table):
    """helper for crc calculation

    :param cur
    :param crc
    :param table
    """
    l_crc = 0x000000ff & cur

    tmp = crc ^ l_crc
    crc = (crc >> 8) ^ table[(tmp & 0xff)]
    return crc

def _finalize_crc(crc):
    """helper for crc calculation

    :param crc
    """
    final_crc = 0
    for j in range(NUM_OF_BYTES):
        current_bit = crc & (1 << j)
        current_bit = current_bit >> j
        final_crc |= current_bit << (NUM_OF_BYTES - 1) - j
    return final_crc

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

    ecst_obj = EcstArgs()

    if ecst_obj.error_args:
        for err_arg in ecst_obj.error_args:
            message = f'unKnown flag: {err_arg}'
            exit_with_failure(message)
        sys.exit(EXIT_SUCCESS_STATUS)

    # Start to handle booter header table
    _bt_mode_handler(ecst_obj)

if __name__ == '__main__':
    main()
