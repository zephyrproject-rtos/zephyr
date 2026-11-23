#!/usr/bin/env python3
#
# Copyright (c) 2024 Nuvoton Technology Corporation
#
# SPDX-License-Identifier: Apache-2.0

# This file contains general functions for ESIOST application

import sys
import argparse
import colorama
from colorama import Fore

INVALID_INPUT = -1
EXIT_FAILURE_STATUS = 1

# Verbose related values
NO_VERBOSE = 0
REG_VERBOSE = 1

# argument default values.
DEFAULT_VERBOSE = NO_VERBOSE

# Chips: convert from name to index.
CHIPS_INFO = {
    'npcm400': {'flash_address': 0x80000, 'flash_size': 0x20000, 'ram_address': 0x10008000, 'ram_size': 0xC0000},
}

DEFAULT_CHIP = 'npcm400'

class EsiostArgs:
    """creates an arguments object for the ESIOST,
    the arguments are taken from the command line and/or
    argument file
    """
    error_args = None

    help = False
    verbose = DEFAULT_VERBOSE
    input = None
    output = None
    args_file = None
    chip_name = DEFAULT_CHIP
    chip_ram_address = CHIPS_INFO[DEFAULT_CHIP]['ram_address']
    chip_ram_size = CHIPS_INFO[DEFAULT_CHIP]['ram_address']
    chip_flash_address = CHIPS_INFO[DEFAULT_CHIP]['flash_address']
    chip_flash_size = CHIPS_INFO[DEFAULT_CHIP]['flash_size']
    firmware_load_address = None
    firmware_entry_point = None
    firmware_length = None

    def __init__(self):

        arguments = _create_parser("")
        valid_arguments = arguments[0]
        invalid_arguments = arguments[1]
        self.error_args = invalid_arguments

        _populate_args(self, valid_arguments)
        _populate_chip_fields(self)

def _populate_chip_fields(self):
    """populate the chip related fields for the esiost"""
    self.chip_name = self.chip_name
    chip = str(self.chip_name).lower()

    if chip not in CHIPS_INFO:
        self.chip_name = INVALID_INPUT
        return

    self.chip_ram_address = CHIPS_INFO[chip]['ram_address']
    self.chip_ram_size = CHIPS_INFO[chip]['ram_size']
    self.chip_flash_address = CHIPS_INFO[DEFAULT_CHIP]['flash_address']
    self.chip_flash_size = CHIPS_INFO[DEFAULT_CHIP]['flash_size']
    if self.firmware_load_address is None:
        self.firmware_load_address = self.chip_ram_address

def _populate_args(self, argument_list):
    """populate the esiost arguments according to the command line/ args file"""
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

    args = parser.parse_known_args(arg_list.split())

    if arg_list == "":
        args = parser.parse_known_args()

    return args

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
