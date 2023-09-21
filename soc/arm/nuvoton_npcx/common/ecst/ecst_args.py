#!/usr/bin/env python3
#
# Copyright (c) 2020 Nuvoton Technology Corporation
#
# SPDX-License-Identifier: Apache-2.0

# This file contains general functions for ECST application

import sys
import argparse
import colorama
from colorama import Fore

INVALID_INPUT = -1
EXIT_FAILURE_STATUS = 1

# Header common fields
FW_HDR_CRC_DISABLE = 0x00
FW_HDR_CRC_ENABLE = 0x02
FW_CRC_DISABLE = 0x00
FW_CRC_ENABLE = 0x02

SPI_CLOCK_RATIO_1 = 0x00
SPI_CLOCK_RATIO_2 = 0x08

SPI_UNLIMITED_BURST_DISABLE = 0x00
SPI_UNLIMITED_BURST_ENABLE = 0x08

# Verbose related values
NO_VERBOSE = 0
REG_VERBOSE = 1
SUPER_VERBOSE = 1

# argument default values.
DEFAULT_MODE = "bt"
SPI_MAX_CLOCK_DEFAULT = "20"
FLASH_SIZE_DEFAULT = "16"
SPI_CLOCK_RATIO_DEFAULT = 1
PASTE_FIRMWARE_HEADER_DEFAULT = 0x00000000
DEFAULT_VERBOSE = NO_VERBOSE
SPI_MODE_VAL_DEFAULT = 'normal'
SPI_UNLIMITED_BURST_DEFAULT = SPI_UNLIMITED_BURST_DISABLE
FW_HDR_CRC_DEFAULT = FW_HDR_CRC_ENABLE
FW_CRC_DEFAULT = FW_CRC_ENABLE
FW_CRC_START_OFFSET_DEFAULT = 0x0
POINTER_OFFSET_DEFAULT = 0x0

# Chips: convert from name to index.
CHIPS_INFO = {
    'npcx7m5': {'ram_address': 0x100a8000, 'ram_size': 0x20000},
    'npcx7m6': {'ram_address': 0x10090000, 'ram_size': 0x40000},
    'npcx7m7': {'ram_address': 0x10070000, 'ram_size': 0x60000},
    'npcx9m3': {'ram_address': 0x10080000, 'ram_size': 0x50000},
    'npcx9m6': {'ram_address': 0x10090000, 'ram_size': 0x40000},
    'npcx9m7': {'ram_address': 0x10070000, 'ram_size': 0x60000},
    'npcx4m3': {'ram_address': 0x10088000, 'ram_size': 0x50000},
    'npcx4m8': {'ram_address': 0x10060000, 'ram_size': 0x7c800},
}
DEFAULT_CHIP = 'npcx7m6'

# RAM related values
RAM_ADDR = 0x00
RAM_SIZE = 0x01

class EcstArgs:
    """creates an arguments object for the ECST,
    the arguments are taken from the command line and/or
    argument file
    """
    error_args = None

    mode = DEFAULT_MODE
    help = False
    verbose = DEFAULT_VERBOSE
    super_verbose = False
    input = None
    output = None
    args_file = None
    chip_name = DEFAULT_CHIP
    chip_ram_address = CHIPS_INFO[DEFAULT_CHIP]['ram_address']
    chip_ram_size = CHIPS_INFO[DEFAULT_CHIP]['ram_address']
    firmware_header_crc = FW_HDR_CRC_DEFAULT
    firmware_crc = FW_CRC_DEFAULT
    spi_flash_maximum_clock = SPI_MAX_CLOCK_DEFAULT
    spi_flash_clock_ratio = SPI_CLOCK_RATIO_DEFAULT
    unlimited_burst_mode = SPI_UNLIMITED_BURST_DEFAULT
    spi_flash_read_mode = SPI_MODE_VAL_DEFAULT
    firmware_load_address = None
    firmware_entry_point = None
    use_arm_reset = True
    firmware_crc_start = FW_CRC_START_OFFSET_DEFAULT
    firmware_crc_size = None
    firmware_length = None
    flash_size = FLASH_SIZE_DEFAULT
    paste_firmware_header = PASTE_FIRMWARE_HEADER_DEFAULT
    pointer = POINTER_OFFSET_DEFAULT
    bh_offset = None

    def __init__(self):

        arguments = _create_parser("")
        valid_arguments = arguments[0]
        invalid_arguments = arguments[1]
        self.error_args = invalid_arguments

        _populate_args(self, valid_arguments)
        _populate_chip_fields(self)

def _populate_chip_fields(self):
    """populate the chip related fields for the ecst"""
    self.chip_name = self.chip_name
    chip = str(self.chip_name).lower()

    if chip not in CHIPS_INFO:
        self.chip_name = INVALID_INPUT
        return

    self.chip_ram_address = CHIPS_INFO[chip]['ram_address']
    self.chip_ram_size = CHIPS_INFO[chip]['ram_size']
    if self.firmware_load_address is None:
        self.firmware_load_address = self.chip_ram_address

def _populate_args(self, argument_list):
    """populate the ecst arguments according to the command line/ args file"""
    for arg in vars(argument_list):
        if (arg == "input") & (argument_list.input is not None):
            self.input = argument_list.input

        elif (arg == "output") & (argument_list.output is not None):
            self.output = argument_list.output

        elif (arg == "chip") & (argument_list.chip is not None):
            self.chip_name = argument_list.chip
            _populate_chip_fields(self)

        elif (arg == "verbose") & argument_list.verbose:
            self.verbose = REG_VERBOSE

        elif (arg == "super_verbose") & argument_list.super_verbose:
            self.verbose = SUPER_VERBOSE

        elif (arg == "spi_flash_maximum_clock") & \
                (argument_list.spi_flash_maximum_clock is not None):
            self.spi_flash_maximum_clock =\
                argument_list.spi_flash_maximum_clock

        elif (arg == "spi_flash_clock_ratio") & \
                (argument_list.spi_flash_clock_ratio is not None):
            if argument_list.spi_flash_clock_ratio.isdigit():
                self.spi_flash_clock_ratio =\
                    int(argument_list.spi_flash_clock_ratio)
            else:
                self.spi_flash_clock_ratio = INVALID_INPUT

        elif (arg == "firmware_header_crc") &\
                argument_list.firmware_header_crc:
            self.firmware_header_crc = FW_HDR_CRC_DISABLE

        elif (arg == "firmware_crc") & argument_list.firmware_crc:
            self.firmware_crc = FW_CRC_DISABLE

        elif (arg == "spi_read_mode") &\
                (argument_list.spi_read_mode is not None):

            self.spi_flash_read_mode = argument_list.spi_read_mode

        elif (arg == "flash_size") & (argument_list.flash_size is not None):
            self.flash_size = argument_list.flash_size

        elif (arg == "paste_firmware_header") & \
                (argument_list.paste_firmware_header is not None):
            if _is_hex(argument_list.paste_firmware_header):
                self.paste_firmware_header =\
                    int(argument_list.paste_firmware_header, 16)
            else:
                self.paste_firmware_header = INVALID_INPUT

def _create_parser(arg_list):
    """create argument parser according to pre-defined arguments

    :param arg_list: when empty, parses command line arguments,
    else parses the given string
    """

    parser = argparse.ArgumentParser(conflict_handler='resolve', allow_abbrev=False)
    parser.add_argument("-i", nargs='?', dest="input")
    parser.add_argument("-o", nargs='?', dest="output")
    parser.add_argument("-chip", dest="chip")
    parser.add_argument("-v", action="store_true", dest="verbose")
    parser.add_argument("-vv", action="store_true", dest="super_verbose")
    parser.add_argument("-nohcrc", action="store_true",
                        dest="firmware_header_crc")
    parser.add_argument("-nofcrc", action="store_true", dest="firmware_crc")
    parser.add_argument("-spimaxclk", nargs='?',
                        dest="spi_flash_maximum_clock")
    parser.add_argument("-spiclkratio", nargs='?',
                        dest="spi_flash_clock_ratio")
    parser.add_argument("-spireadmode", nargs='?', dest="spi_read_mode")
    parser.add_argument("-flashsize", nargs='?', dest="flash_size")
    parser.add_argument("-ph", nargs='?', dest="paste_firmware_header")

    args = parser.parse_known_args(arg_list.split())

    if arg_list == "":
        args = parser.parse_known_args()

    return args

def _file_to_line(arg_file):
    """helper to convert a text file to one line string
    used to parse the arguments in a given argfile

    :param arg_file: the file to manipulate
    """
    with open(arg_file, "r") as arg_file_to_read:
        data = arg_file_to_read.read().strip()
    arg_file_to_read.close()

    return data

def _is_hex(val):
    """helper to determine whether an input is a hex
    formatted number

    :param val: input to be checked
    """
    if val.startswith("0x") or val.startswith("0X"):
        val = val[2:]
    hex_digits = set("0123456789abcdefABCDEF")
    for char in val:
        if char not in hex_digits:
            return False
    return True

def exit_with_failure(message):
    """formatted failure message printer, prints the
    relevant error message and exits the application.

    :param message: the error message to be printed
    """

    message = '\n' + message
    message += '\n'
    message += '******************************\n'
    message += '***        FAILED          ***\n'
    message += '******************************\n'
    print(Fore.RED + message)

    sys.exit(EXIT_FAILURE_STATUS)

colorama.init()
