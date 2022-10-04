#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2020 Nordic Semiconductor NA
#
# SPDX-License-Identifier: Apache-2.0
"""Translate generic handles into ones optimized for the application.

Immutable device data includes information about dependencies,
e.g. that a particular sensor is controlled through a specific I2C bus
and that it signals event on a pin on a specific GPIO controller.
This information is encoded in the first-pass binary using identifiers
derived from the devicetree.  This script extracts those identifiers
and replaces them with ones optimized for use with the devices
actually present.

For example the sensor might have a first-pass handle defined by its
devicetree ordinal 52, with the I2C driver having ordinal 24 and the
GPIO controller ordinal 14.  The runtime ordinal is the index of the
corresponding device in the static devicetree array, which might be 6,
5, and 3, respectively.

The output is a C source file that provides alternative definitions
for the array contents referenced from the immutable device objects.
In the final link these definitions supersede the ones in the
driver-specific object file.
"""

import sys
import argparse
import os
import struct
import pickle
from packaging import version

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection
import elftools.elf.enums

# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..',
                                'dts', 'python-devicetree', 'src'))
from devicetree import edtlib  # pylint: disable=unused-import

if version.parse(elftools.__version__) < version.parse('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")

scr = os.path.basename(sys.argv[0])

def debug(text):
    if not args.verbose:
        return
    sys.stdout.write(scr + ": " + text + "\n")

def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-k", "--kernel", required=True,
                        help="Input zephyr ELF binary")
    parser.add_argument("-d", "--num-dynamic-devices", required=False, default=0,
                        type=int, help="Input number of dynamic devices allowed")
    parser.add_argument("-o", "--output-source", required=True,
            help="Output source file")

    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print extra debugging information")

    parser.add_argument("-z", "--zephyr-base",
                        help="Path to current Zephyr base. If this argument \
                        is not provided the environment will be checked for \
                        the ZEPHYR_BASE environment variable.")

    parser.add_argument("-s", "--start-symbol", required=True,
                        help="Symbol name of the section which contains the \
                        devices. The symbol name must point to the first \
                        device in that section.")

    args = parser.parse_args()
    if "VERBOSE" in os.environ:
        args.verbose = 1

    ZEPHYR_BASE = args.zephyr_base or os.getenv("ZEPHYR_BASE")

    if ZEPHYR_BASE is None:
        sys.exit("-z / --zephyr-base not provided. Please provide "
                 "--zephyr-base or set ZEPHYR_BASE in environment")

    sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/dts"))


def symbol_data(elf, sym):
    addr = sym.entry.st_value
    len = sym.entry.st_size
    for section in elf.iter_sections():
        start = section['sh_addr']
        end = start + section['sh_size']

        if (start <= addr) and (addr + len) <= end:
            offset = addr - section['sh_addr']
            return bytes(section.data()[offset:offset + len])

def symbol_handle_data(elf, sym):
    data = symbol_data(elf, sym)
    if data:
        format = "<" if elf.little_endian else ">"
        format += "%uh" % (len(data) / 2)
        return struct.unpack(format, data)

# These match the corresponding constants in <device.h>
DEVICE_HANDLE_SEP = -32768
DEVICE_HANDLE_ENDS = 32767
DEVICE_HANDLE_NULL = 0
def handle_name(hdl):
    if hdl == DEVICE_HANDLE_SEP:
        return "DEVICE_HANDLE_SEP"
    if hdl == DEVICE_HANDLE_ENDS:
        return "DEVICE_HANDLE_ENDS"
    if hdl == 0:
        return "DEVICE_HANDLE_NULL"
    return str(int(hdl))

class Device:
    """
    Represents information about a device object and its references to other objects.
    """
    def __init__(self, elf, ld_constants, sym, addr):
        self.elf = elf
        self.ld_constants = ld_constants
        self.sym = sym
        self.addr = addr
        # Point to the handles instance associated with the device;
        # assigned by correlating the device struct handles pointer
        # value with the addr of a Handles instance.
        self.__handles = None
        self.__pm = None

    @property
    def obj_handles(self):
        """
        Returns the value from the device struct handles field, pointing to the
        array of handles for devices this device depends on.
        """
        if self.__handles is None:
            data = symbol_data(self.elf, self.sym)
            format = "<" if self.elf.little_endian else ">"
            if self.elf.elfclass == 32:
                format += "I"
                size = 4
            else:
                format += "Q"
                size = 8
            offset = self.ld_constants["_DEVICE_STRUCT_HANDLES_OFFSET"]
            self.__handles = struct.unpack(format, data[offset:offset + size])[0]
        return self.__handles

    @property
    def obj_pm(self):
        """
        Returns the value from the device struct pm field, pointing to the
        pm struct for this device.
        """
        if self.__pm is None:
            data = symbol_data(self.elf, self.sym)
            format = "<" if self.elf.little_endian else ">"
            if self.elf.elfclass == 32:
                format += "I"
                size = 4
            else:
                format += "Q"
                size = 8
            offset = self.ld_constants["_DEVICE_STRUCT_PM_OFFSET"]
            self.__pm = struct.unpack(format, data[offset:offset + size])[0]
        return self.__pm

class PMDevice:
    """
    Represents information about a pm_device object and its references to other objects.
    """
    def __init__(self, elf, ld_constants, sym, addr):
        self.elf = elf
        self.ld_constants = ld_constants
        self.sym = sym
        self.addr = addr

        # Point to the device instance associated with the pm_device;
        self.__flags = None

    def is_domain(self):
        """
        Checks if the device that this pm struct belongs is a power domain.
        """
        if self.__flags is None:
            data = symbol_data(self.elf, self.sym)
            format = "<" if self.elf.little_endian else ">"
            if self.elf.elfclass == 32:
                format += "I"
                size = 4
            else:
                format += "Q"
                size = 8
            offset = self.ld_constants["_PM_DEVICE_STRUCT_FLAGS_OFFSET"]
            self.__flags = struct.unpack(format, data[offset:offset + size])[0]
        return self.__flags & (1 << self.ld_constants["_PM_DEVICE_FLAG_PD"])

class Handles:
    def __init__(self, sym, addr, handles, node):
        self.sym = sym
        self.addr = addr
        self.handles = handles
        self.node = node
        self.dep_ord = None
        self.dev_deps = None
        self.ext_deps = None

def main():
    parse_args()

    assert args.kernel, "--kernel ELF required to extract data"
    elf = ELFFile(open(args.kernel, "rb"))

    edtser = os.path.join(os.path.split(args.kernel)[0], "edt.pickle")
    with open(edtser, 'rb') as f:
        edt = pickle.load(f)

    pm_devices = {}
    devices = []
    handles = []
    # Leading _ are stripped from the stored constant key

    want_constants = set([args.start_symbol,
                          "_DEVICE_STRUCT_SIZEOF",
                          "_DEVICE_STRUCT_HANDLES_OFFSET"])
    if args.num_dynamic_devices != 0:
        want_constants.update(["_PM_DEVICE_FLAG_PD",
                               "_DEVICE_STRUCT_PM_OFFSET",
                               "_PM_DEVICE_STRUCT_FLAGS_OFFSET"])
    ld_constants = dict()

    for section in elf.iter_sections():
        if isinstance(section, SymbolTableSection):
            for sym in section.iter_symbols():
                if sym.name in want_constants:
                    ld_constants[sym.name] = sym.entry.st_value
                    continue
                if sym.entry.st_info.type != 'STT_OBJECT':
                    continue
                if sym.name.startswith("__device"):
                    addr = sym.entry.st_value
                    if sym.name.startswith("__device_"):
                        devices.append(Device(elf, ld_constants, sym, addr))
                        debug("device %s" % (sym.name,))
                    elif sym.name.startswith("__devicehdl_"):
                        hdls = symbol_handle_data(elf, sym)

                        # The first element of the hdls array is the dependency
                        # ordinal of the device, which identifies the devicetree
                        # node.
                        node = edt.dep_ord2node[hdls[0]] if (hdls and hdls[0] != 0) else None
                        handles.append(Handles(sym, addr, hdls, node))
                        debug("handles %s %d %s" % (sym.name, hdls[0] if hdls else -1, node))
                if sym.name.startswith("__pm_device__") and not sym.name.endswith("_slot"):
                    addr = sym.entry.st_value
                    pm_devices[addr] = PMDevice(elf, ld_constants, sym, addr)
                    debug("pm device %s" % (sym.name,))

    assert len(want_constants) == len(ld_constants), "linker map data incomplete"

    devices = sorted(devices, key = lambda k: k.sym.entry.st_value)

    device_start_addr = ld_constants[args.start_symbol]
    device_size = 0

    assert len(devices) == len(handles), 'mismatch devices and handles'

    used_nodes = set()
    for handle in handles:
        handle.device = None
        for device in devices:
            if handle.addr == device.obj_handles:
                handle.device = device
                break
        device = handle.device
        assert device, 'no device for %s' % (handle.sym.name,)

        device.handle = handle

        if device_size == 0:
            device_size = device.sym.entry.st_size

        # The device handle is one plus the ordinal of this device in
        # the device table.
        device.dev_handle = 1 + int((device.sym.entry.st_value - device_start_addr) / device_size)
        debug("%s dev ordinal %d" % (device.sym.name, device.dev_handle))

        n = handle.node
        if n is not None:
            debug("%s dev ordinal %d\n\t%s" % (n.path, device.dev_handle, ' ; '.join(str(_) for _ in handle.handles)))
            used_nodes.add(n)
            n.__device = device
        else:
            debug("orphan %d" % (device.dev_handle,))
        hv = handle.handles
        hvi = 1
        handle.dev_deps = []
        handle.dev_injected = []
        handle.dev_sups = []
        hdls = handle.dev_deps
        while hvi < len(hv):
            h = hv[hvi]
            if h == DEVICE_HANDLE_ENDS:
                break
            if h == DEVICE_HANDLE_SEP:
                if hdls == handle.dev_deps:
                    hdls = handle.dev_injected
                else:
                    hdls = handle.dev_sups
            else:
                hdls.append(h)
                n = edt
            hvi += 1

    # Compute the dependency graph induced from the full graph restricted to the
    # the nodes that exist in the application.  Note that the edges in the
    # induced graph correspond to paths in the full graph.
    root = edt.dep_ord2node[0]
    assert root not in used_nodes

    for n in used_nodes:
        # Where we're storing the final set of nodes: these are all used
        n.__depends = set()
        n.__supports = set()

        deps = set(n.depends_on)
        debug("\nNode: %s\nOrig deps:\n\t%s" % (n.path, "\n\t".join([dn.path for dn in deps])))
        while len(deps) > 0:
            dn = deps.pop()
            if dn in used_nodes:
                # this is used
                n.__depends.add(dn)
            elif dn != root:
                # forward the dependency up one level
                for ddn in dn.depends_on:
                    deps.add(ddn)
        debug("Final deps:\n\t%s\n" % ("\n\t".join([ _dn.path for _dn in n.__depends])))

        sups = set(n.required_by)
        debug("\nOrig sups:\n\t%s" % ("\n\t".join([dn.path for dn in sups])))
        while len(sups) > 0:
            sn = sups.pop()
            if sn in used_nodes:
                # this is used
                n.__supports.add(sn)
            else:
                # forward the support down one level
                for ssn in sn.required_by:
                    sups.add(ssn)
        debug("\nFinal sups:\n\t%s" % ("\n\t".join([_sn.path for _sn in n.__supports])))

    with open(args.output_source, "w") as fp:
        fp.write('#include <zephyr/device.h>\n')
        fp.write('#include <zephyr/toolchain.h>\n')

        for dev in devices:
            hs = dev.handle
            assert hs, "no hs for %s" % (dev.sym.name,)
            dep_paths = []
            inj_paths = []
            sup_paths = []
            hdls = []

            sn = hs.node
            if sn:
                hdls.extend(dn.__device.dev_handle for dn in sn.__depends)
                for dn in sn.depends_on:
                    if dn in sn.__depends:
                        dep_paths.append(dn.path)
                    else:
                        dep_paths.append('(%s)' % dn.path)
            # Force separator to signal start of injected dependencies
            hdls.append(DEVICE_HANDLE_SEP)
            for inj in hs.dev_injected:
                if inj not in edt.dep_ord2node:
                    continue
                expected = edt.dep_ord2node[inj]
                if expected in used_nodes:
                    inj_paths.append(expected.path)
                    hdls.append(expected.__device.dev_handle)

            # Force separator to signal start of supported devices
            hdls.append(DEVICE_HANDLE_SEP)
            if len(hs.dev_sups) > 0:
                for dn in sn.required_by:
                    if dn in sn.__supports:
                        sup_paths.append(dn.path)
                    else:
                        sup_paths.append('(%s)' % dn.path)
                hdls.extend(dn.__device.dev_handle for dn in sn.__supports)

            if args.num_dynamic_devices != 0:
                pm = pm_devices.get(dev.obj_pm)
                if pm and pm.is_domain():
                    hdls.extend(DEVICE_HANDLE_NULL for dn in range(args.num_dynamic_devices))

            # Terminate the array with the end symbol
            hdls.append(DEVICE_HANDLE_ENDS)

            lines = [
                '',
                '/* %d : %s:' % (dev.dev_handle, (sn and sn.path) or "sysinit"),
            ]

            if len(dep_paths) > 0:
                lines.append(' * Direct Dependencies:')
                lines.append(' *   - %s' % ('\n *   - '.join(dep_paths)))
            if len(inj_paths) > 0:
                lines.append(' * Injected Dependencies:')
                lines.append(' *   - %s' % ('\n *   - '.join(inj_paths)))
            if len(sup_paths) > 0:
                lines.append(' * Supported:')
                lines.append(' *   - %s' % ('\n *   - '.join(sup_paths)))

            lines.extend([
                ' */',
                'const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))',
                '%s[] = { %s };' % (hs.sym.name, ', '.join([handle_name(_h) for _h in hdls])),
                '',
            ])

            fp.write('\n'.join(lines))

if __name__ == "__main__":
    main()
