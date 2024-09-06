#!/usr/bin/env python3

# Copyright 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

"""
Checks the initialization priorities

This script parses a Zephyr executable file, creates a list of known devices
and their effective initialization priorities and compares that with the device
dependencies inferred from the devicetree hierarchy.

This can be used to detect devices that are initialized in the incorrect order,
but also devices that are initialized at the same priority but depends on each
other, which can potentially break if the linking order is changed.

Optionally, it can also produce a human readable list of the initialization
calls for the various init levels.
"""

import argparse
import logging
import os
import pathlib
import pickle
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..",
                                "dts", "python-devicetree", "src"))
from devicetree import edtlib  # pylint: disable=unused-import

# Prefix used for "struct device" reference initialized based on devicetree
# entries with a known ordinal.
_DEVICE_ORD_PREFIX = "__device_dts_ord_"

# Defined init level in order of priority.
_DEVICE_INIT_LEVELS = ["EARLY", "PRE_KERNEL_1", "PRE_KERNEL_2", "POST_KERNEL",
                      "APPLICATION", "SMP"]

# List of compatibles for nodes where we don't check the priority.
_IGNORE_COMPATIBLES = frozenset([
        # There is no direct dependency between the CDC ACM UART and the USB
        # device controller, the logical connection is established after USB
        # device support is enabled.
        "zephyr,cdc-acm-uart",
        ])

class Priority:
    """Parses and holds a device initialization priority.

    The object can be used for comparing levels with one another.

    Attributes:
        name: the section name
    """
    def __init__(self, level, priority):
        for idx, level_name in enumerate(_DEVICE_INIT_LEVELS):
            if level_name == level:
                self._level = idx
                self._priority = priority
                # Tuples compare elementwise in order
                self._level_priority = (self._level, self._priority)
                return

        raise ValueError("Unknown level in %s" % level)

    def __repr__(self):
        return "<%s %s %d>" % (self.__class__.__name__,
                               _DEVICE_INIT_LEVELS[self._level], self._priority)

    def __str__(self):
        return "%s+%d" % (_DEVICE_INIT_LEVELS[self._level], self._priority)

    def __lt__(self, other):
        return self._level_priority < other._level_priority

    def __eq__(self, other):
        return self._level_priority == other._level_priority

    def __hash__(self):
        return self._level_priority


class ZephyrInitLevels:
    """Load an executable file and find the initialization calls and devices.

    Load a Zephyr executable file and scan for the list of initialization calls
    and defined devices.

    The list of devices is available in the "devices" class variable in the
    {ordinal: Priority} format, the list of initilevels is in the "initlevels"
    class variables in the {"level name": ["call", ...]} format.

    Attributes:
        file_path: path of the file to be loaded.
    """
    def __init__(self, file_path):
        self.file_path = file_path
        self._elf = ELFFile(open(file_path, "rb"))
        self._load_objects()
        self._load_level_addr()
        self._process_initlevels()

    def _load_objects(self):
        """Initialize the object table."""
        self._objects = {}

        for section in self._elf.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue

            for sym in section.iter_symbols():
                if (sym.name and
                    sym.entry.st_size > 0 and
                    sym.entry.st_info.type in ["STT_OBJECT", "STT_FUNC"]):
                    self._objects[sym.entry.st_value] = (
                            sym.name, sym.entry.st_size, sym.entry.st_shndx)

    def _load_level_addr(self):
        """Find the address associated with known init levels."""
        self._init_level_addr = {}

        for section in self._elf.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue

            for sym in section.iter_symbols():
                for level in _DEVICE_INIT_LEVELS:
                    name = f"__init_{level}_start"
                    if sym.name == name:
                        self._init_level_addr[level] = sym.entry.st_value
                    elif sym.name == "__init_end":
                        self._init_level_end = sym.entry.st_value

        if len(self._init_level_addr) != len(_DEVICE_INIT_LEVELS):
            raise ValueError(f"Missing init symbols, found: {self._init_level_addr}")

        if not self._init_level_end:
            raise ValueError(f"Missing init section end symbol")

    def _device_ord_from_name(self, sym_name):
        """Find a device ordinal from a symbol name."""
        if not sym_name:
            return None

        if not sym_name.startswith(_DEVICE_ORD_PREFIX):
            return None

        _, device_ord = sym_name.split(_DEVICE_ORD_PREFIX)
        return int(device_ord)

    def _object_name(self, addr):
        if not addr:
            return "NULL"
        elif addr in self._objects:
            return self._objects[addr][0]
        else:
            return "unknown"

    def _initlevel_pointer(self, addr, idx, shidx):
        elfclass = self._elf.elfclass
        if elfclass == 32:
            ptrsize = 4
        elif elfclass == 64:
            ptrsize = 8
        else:
            raise ValueError(f"Unknown pointer size for ELF class f{elfclass}")

        section = self._elf.get_section(shidx)
        start = section.header.sh_addr
        data = section.data()

        offset = addr - start

        start = offset + ptrsize * idx
        stop = offset + ptrsize * (idx + 1)

        return int.from_bytes(data[start:stop], byteorder="little")

    def _process_initlevels(self):
        """Process the init level and find the init functions and devices."""
        self.devices = {}
        self.initlevels = {}

        for i, level in enumerate(_DEVICE_INIT_LEVELS):
            start = self._init_level_addr[level]
            if i + 1 == len(_DEVICE_INIT_LEVELS):
                stop = self._init_level_end
            else:
                stop = self._init_level_addr[_DEVICE_INIT_LEVELS[i + 1]]

            self.initlevels[level] = []

            priority = 0
            addr = start
            while addr < stop:
                if addr not in self._objects:
                    raise ValueError(f"no symbol at addr {addr:08x}")
                obj, size, shidx = self._objects[addr]

                arg0_name = self._object_name(self._initlevel_pointer(addr, 0, shidx))
                arg1_name = self._object_name(self._initlevel_pointer(addr, 1, shidx))

                self.initlevels[level].append(f"{obj}: {arg0_name}({arg1_name})")

                ordinal = self._device_ord_from_name(arg1_name)
                if ordinal:
                    prio = Priority(level, priority)
                    self.devices[ordinal] = (prio, arg0_name)

                addr += size
                priority += 1

class Validator():
    """Validates the initialization priorities.

    Scans through a build folder for object files and list all the device
    initialization priorities. Then compares that against the EDT derived
    dependency list and log any found priority issue.

    Attributes:
        elf_file_path: path of the ELF file
        edt_pickle: name of the EDT pickle file
        log: a logging.Logger object
    """
    def __init__(self, elf_file_path, edt_pickle, log):
        self.log = log

        edt_pickle_path = pathlib.Path(
                pathlib.Path(elf_file_path).parent,
                edt_pickle)
        with open(edt_pickle_path, "rb") as f:
            edt = pickle.load(f)

        self._ord2node = edt.dep_ord2node

        self._obj = ZephyrInitLevels(elf_file_path)

        self.errors = 0

    def _check_dep(self, dev_ord, dep_ord):
        """Validate the priority between two devices."""
        if dev_ord == dep_ord:
            return

        dev_node = self._ord2node[dev_ord]
        dep_node = self._ord2node[dep_ord]

        if dev_node._binding:
            dev_compat = dev_node._binding.compatible
            if dev_compat in _IGNORE_COMPATIBLES:
                self.log.info(f"Ignoring priority: {dev_node._binding.compatible}")
                return

        dev_prio, dev_init = self._obj.devices.get(dev_ord, (None, None))
        dep_prio, dep_init = self._obj.devices.get(dep_ord, (None, None))

        if not dev_prio or not dep_prio:
            return

        if dev_prio == dep_prio:
            raise ValueError(f"{dev_node.path} and {dep_node.path} have the "
                             f"same priority: {dev_prio}")
        elif dev_prio < dep_prio:
            if not self.errors:
                self.log.error("Device initialization priority validation failed, "
                               "the sequence of initialization calls does not match "
                               "the devicetree dependencies.")
            self.errors += 1
            self.log.error(
                    f"{dev_node.path} <{dev_init}> is initialized before its dependency "
                    f"{dep_node.path} <{dep_init}> ({dev_prio} < {dep_prio})")
        else:
            self.log.info(
                    f"{dev_node.path} <{dev_init}> {dev_prio} > "
                    f"{dep_node.path} <{dep_init}> {dep_prio}")

    def check_edt(self):
        """Scan through all known devices and validate the init priorities."""
        for dev_ord in self._obj.devices:
            dev = self._ord2node[dev_ord]
            for dep in dev.depends_on:
                self._check_dep(dev_ord, dep.dep_ordinal)

    def print_initlevels(self):
        for level, calls in self._obj.initlevels.items():
            print(level)
            for call in calls:
                print(f"  {call}")

def _parse_args(argv):
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument("-f", "--elf-file", default=pathlib.Path("build", "zephyr", "zephyr.elf"),
                        help="ELF file to use")
    parser.add_argument("-v", "--verbose", action="count",
                        help=("enable verbose output, can be used multiple times "
                              "to increase verbosity level"))
    parser.add_argument("--always-succeed", action="store_true",
                        help="always exit with a return code of 0, used for testing")
    parser.add_argument("-o", "--output",
                        help="write the output to a file in addition to stdout")
    parser.add_argument("-i", "--initlevels", action="store_true",
                        help="print the initlevel functions instead of checking the device dependencies")
    parser.add_argument("--edt-pickle", default=pathlib.Path("edt.pickle"),
                        help="name of the pickled edtlib.EDT file",
                        type=pathlib.Path)

    return parser.parse_args(argv)

def _init_log(verbose, output):
    """Initialize a logger object."""
    log = logging.getLogger(__file__)

    console = logging.StreamHandler()
    console.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    log.addHandler(console)

    if output:
        file = logging.FileHandler(output, mode="w")
        file.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
        log.addHandler(file)

    if verbose and verbose > 1:
        log.setLevel(logging.DEBUG)
    elif verbose and verbose > 0:
        log.setLevel(logging.INFO)
    else:
        log.setLevel(logging.WARNING)

    return log

def main(argv=None):
    args = _parse_args(argv)

    log = _init_log(args.verbose, args.output)

    log.info(f"check_init_priorities: {args.elf_file}")

    validator = Validator(args.elf_file, args.edt_pickle, log)
    if args.initlevels:
        validator.print_initlevels()
    else:
        validator.check_edt()

    if args.always_succeed:
        return 0

    if validator.errors:
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
