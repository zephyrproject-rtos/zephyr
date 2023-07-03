#!/usr/bin/env python3

# Copyright 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

"""
Checks the initialization priorities

This script parses the object files in the specified build directory, creates a
list of known devices and their effective initialization priorities and
compares that with the device dependencies inferred from the devicetree
hierarchy.

This can be used to detect devices that are initialized in the incorrect order,
but also devices that are initialized at the same priority but depends on each
other, which can potentially break if the linking order is changed.
"""

import argparse
import logging
import os
import pathlib
import pickle
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
from elftools.elf.sections import SymbolTableSection

# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..",
                                "dts", "python-devicetree", "src"))
from devicetree import edtlib  # pylint: disable=unused-import

# Prefix used for relocation sections containing initialization data, as in
# sequence of "struct init_entry".
_INIT_SECTION_PREFIX = (".rel.z_init_", ".rela.z_init_")

# Prefix used for "struct device" reference initialized based on devicetree
# entries with a known ordinal.
_DEVICE_ORD_PREFIX = "__device_dts_ord_"

# File name suffix for object files to be scanned.
_OBJ_FILE_SUFFIX = ".c.obj"

# Defined init level in order of priority.
_DEVICE_INIT_LEVELS = ["EARLY", "PRE_KERNEL_1", "PRE_KERNEL_2", "POST_KERNEL",
                      "APPLICATION", "SMP"]

# File name to check for detecting and skiping nested build directories.
_BUILD_DIR_DETECT_FILE = "CMakeCache.txt"

# List of compatibles for node where the initialization priority should be the
# opposite of the device tree inferred dependency.
_INVERTED_PRIORITY_COMPATIBLES = frozenset()

class Priority:
    """Parses and holds a device initialization priority.

    Parses an ELF section name for the corresponding initialization level and
    priority, for example ".rel.z_init_PRE_KERNEL_155_" for "PRE_KERNEL_1 55".

    The object can be used for comparing levels with one another.

    Attributes:
        name: the section name
    """
    def __init__(self, name):
        for idx, level in enumerate(_DEVICE_INIT_LEVELS):
            if level in name:
                _, priority = name.strip("_").split(level)
                self._level = idx
                self._priority = int(priority)
                self._level_priority = self._level * 100 + self._priority
                return

        raise ValueError("Unknown level in %s" % name)

    def __repr__(self):
        return "<%s %s %d>" % (self.__class__.__name__,
                               _DEVICE_INIT_LEVELS[self._level], self._priority)

    def __str__(self):
        return "%s %d" % (_DEVICE_INIT_LEVELS[self._level], self._priority)

    def __lt__(self, other):
        return self._level_priority < other._level_priority

    def __eq__(self, other):
        return self._level_priority == other._level_priority

    def __hash__(self):
        return self._level_priority


class ZephyrObjectFile:
    """Load an object file and finds the device defined within it.

    Load an object file and scans the relocation sections looking for the known
    ones containing initialization callbacks. Then finds what device ordinals
    are being initialized at which priority and stores the list internally.

    A dictionary of {ordinal: Priority} is available in the defined_devices
    class variable.

    Attributes:
        file_path: path of the file to be loaded.
    """
    def __init__(self, file_path):
        self.file_path = file_path
        self._elf = ELFFile(open(file_path, "rb"))
        self._load_symbols()
        self._find_defined_devices()

    def _load_symbols(self):
        """Initialize the symbols table."""
        self._symbols = {}

        for section in self._elf.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue

            for num, sym in enumerate(section.iter_symbols()):
                if sym.name:
                    self._symbols[num] = sym.name

    def _device_ord_from_rel(self, rel):
        """Find a device ordinal from a device symbol name."""
        sym_id = rel["r_info_sym"]
        sym_name = self._symbols.get(sym_id, None)

        if not sym_name:
            return None

        if not sym_name.startswith(_DEVICE_ORD_PREFIX):
            return None

        _, device_ord = sym_name.split(_DEVICE_ORD_PREFIX)
        return int(device_ord)

    def _find_defined_devices(self):
        """Find the device structures defined in the object file."""
        self.defined_devices = {}

        for section in self._elf.iter_sections():
            if not isinstance(section, RelocationSection):
                continue

            if not section.name.startswith(_INIT_SECTION_PREFIX):
                continue

            prio = Priority(section.name)

            for rel in section.iter_relocations():
                device_ord = self._device_ord_from_rel(rel)
                if not device_ord:
                    continue

                if device_ord in self.defined_devices:
                    raise ValueError(
                            f"Device {device_ord} already defined, stale "
                            "object files in the build directory? "
                            "Try running a clean build.")

                self.defined_devices[device_ord] = prio

    def __repr__(self):
        return (f"<{self.__class__.__name__} {self.file_path} "
                f"defined_devices: {self.defined_devices}>")

class Validator():
    """Validates the initialization priorities.

    Scans through a build folder for object files and list all the device
    initialization priorities. Then compares that against the EDT derived
    dependency list and log any found priority issue.

    Attributes:
        build_dir: the build directory to scan
        edt_pickle_path: path of the EDT pickle file
        log: a logging.Logger object
    """
    def __init__(self, build_dir, edt_pickle_path, log):
        self.log = log

        edtser = pathlib.Path(build_dir, edt_pickle_path)
        with open(edtser, "rb") as f:
            edt = pickle.load(f)

        self._ord2node = edt.dep_ord2node

        self._objs = []
        for file in self._find_build_objfiles(build_dir, is_root=True):
            obj = ZephyrObjectFile(file)
            if obj.defined_devices:
                self._objs.append(obj)
                for dev in obj.defined_devices:
                    dev_path = self._ord2node[dev].path
                    self.log.debug(f"{file}: {dev_path}")

        self._dev_priorities = {}
        for obj in self._objs:
            for dev, prio in obj.defined_devices.items():
                if dev in self._dev_priorities:
                    dev_path = self._ord2node[dev].path
                    raise ValueError(
                            f"ERROR: device {dev} ({dev_path}) already defined")
                self._dev_priorities[dev] = prio

        self.warnings = 0
        self.errors = 0

    def _find_build_objfiles(self, build_dir, is_root=False):
        """Find all project object files, skip sub-build directories."""
        if not is_root and pathlib.Path(build_dir, _BUILD_DIR_DETECT_FILE).exists():
            return

        for file in pathlib.Path(build_dir).iterdir():
            if file.is_file() and file.name.endswith(_OBJ_FILE_SUFFIX):
                yield file
            if file.is_dir():
                for file in self._find_build_objfiles(file.resolve()):
                    yield file

    def _check_dep(self, dev_ord, dep_ord):
        """Validate the priority between two devices."""
        if dev_ord == dep_ord:
            return

        dev_node = self._ord2node[dev_ord]
        dep_node = self._ord2node[dep_ord]

        if dev_node._binding and dep_node._binding:
            dev_compat = dev_node._binding.compatible
            dep_compat = dep_node._binding.compatible
            if (dev_compat, dep_compat) in _INVERTED_PRIORITY_COMPATIBLES:
                self.log.info(f"Swapped priority: {dev_compat}, {dep_compat}")
                dev_ord, dep_ord = dep_ord, dev_ord

        dev_prio = self._dev_priorities.get(dev_ord, None)
        dep_prio = self._dev_priorities.get(dep_ord, None)

        if not dev_prio or not dep_prio:
            return

        if dev_prio == dep_prio:
            self.warnings += 1
            self.log.warning(
                    f"{dev_node.path} {dev_prio} == {dep_node.path} {dep_prio}")
        elif dev_prio < dep_prio:
            self.errors += 1
            self.log.error(
                    f"{dev_node.path} {dev_prio} < {dep_node.path} {dep_prio}")
        else:
            self.log.info(
                    f"{dev_node.path} {dev_prio} > {dep_node.path} {dep_prio}")

    def _check_edt_r(self, dev_ord, dev):
        """Recursively check for dependencies of a device."""
        for dep in dev.depends_on:
            self._check_dep(dev_ord, dep.dep_ordinal)
        if dev._binding and dev._binding.child_binding:
            for child in dev.children.values():
                if "compatible" in child.props:
                    continue
                if dev._binding.path != child._binding.path:
                    continue
                self._check_edt_r(dev_ord, child)

    def check_edt(self):
        """Scan through all known devices and validate the init priorities."""
        for dev_ord in self._dev_priorities:
            dev = self._ord2node[dev_ord]
            self._check_edt_r(dev_ord, dev)

def _parse_args(argv):
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument("-d", "--build-dir", default="build",
                        help="build directory to use")
    parser.add_argument("-v", "--verbose", action="count",
                        help=("enable verbose output, can be used multiple times "
                              "to increase verbosity level"))
    parser.add_argument("-w", "--fail-on-warning", action="store_true",
                        help="fail on both warnings and errors")
    parser.add_argument("--always-succeed", action="store_true",
                        help="always exit with a return code of 0, used for testing")
    parser.add_argument("-o", "--output",
                        help="write the output to a file in addition to stdout")
    parser.add_argument("--edt-pickle", default=pathlib.Path("zephyr", "edt.pickle"),
                        help="path to read the pickled edtlib.EDT object from",
                        type=pathlib.Path)

    return parser.parse_args(argv)

def _init_log(verbose, output):
    """Initialize a logger object."""
    log = logging.getLogger(__file__)

    console = logging.StreamHandler()
    console.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    log.addHandler(console)

    if output:
        file = logging.FileHandler(output)
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

    log.info(f"check_init_priorities build_dir: {args.build_dir}")

    validator = Validator(args.build_dir, args.edt_pickle, log)
    validator.check_edt()

    if args.always_succeed:
        return 0

    if args.fail_on_warning and validator.warnings:
        return 1

    if validator.errors:
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
