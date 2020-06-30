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
from distutils.version import LooseVersion

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection
import elftools.elf.enums

if LooseVersion(elftools.__version__) < LooseVersion('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/dts"))

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
    parser.add_argument("-o", "--output-source", required=True,
            help="Output source file")

    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print extra debugging information")
    args = parser.parse_args()
    if "VERBOSE" in os.environ:
        args.verbose = 1


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
            offset = self.ld_constants["DEVICE_STRUCT_HANDLES_OFFSET"]
            self.__handles = struct.unpack(format, data[offset:offset + size])[0]
        return self.__handles

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

    devices = []
    handles = []
    # Leading _ are stripped from the stored constant key
    want_constants = set(["__device_start",
                          "_DEVICE_STRUCT_SIZEOF",
                          "_DEVICE_STRUCT_HANDLES_OFFSET"])
    ld_constants = dict()

    for section in elf.iter_sections():
        if isinstance(section, SymbolTableSection):
            for sym in section.iter_symbols():
                if sym.name in want_constants:
                    ld_constants[sym.name.lstrip("_")] = sym.entry.st_value
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

    assert len(want_constants) == len(ld_constants), "linker map data incomplete"

    devices = sorted(devices, key = lambda k: k.sym.entry.st_value)

    device_start_addr = ld_constants["device_start"]
    device_size = 0

    assert len(devices) == len(handles), 'mismatch devices and handles'

    used_nodes = set()
    for handle in handles:
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
        handle.ext_deps = []
        deps = handle.dev_deps
        while True:
            h = hv[hvi]
            if h == DEVICE_HANDLE_ENDS:
                break
            if h == DEVICE_HANDLE_SEP:
                deps = handle.ext_deps
            else:
                deps.append(h)
                n = edt
            hvi += 1

    # Compute the dependency graph induced from the full graph restricted to the
    # the nodes that exist in the application.  Note that the edges in the
    # induced graph correspond to paths in the full graph.
    root = edt.dep_ord2node[0]
    assert root not in used_nodes

    for sn in used_nodes:
        # Where we're storing the final set of nodes: these are all used
        sn.__depends = set()

        deps = set(sn.depends_on)
        debug("\nNode: %s\nOrig deps:\n\t%s" % (sn.path, "\n\t".join([dn.path for dn in deps])))
        while len(deps) > 0:
            dn = deps.pop()
            if dn in used_nodes:
                # this is used
                sn.__depends.add(dn)
            elif dn != root:
                # forward the dependency up one level
                for ddn in dn.depends_on:
                    deps.add(ddn)
        debug("final deps:\n\t%s\n" % ("\n\t".join([ _dn.path for _dn in sn.__depends])))

    with open(args.output_source, "w") as fp:
        fp.write('#include <device.h>\n')
        fp.write('#include <toolchain.h>\n')

        for dev in devices:
            hs = dev.handle
            assert hs, "no hs for %s" % (dev.sym.name,)
            dep_paths = []
            ext_paths = []
            hdls = []

            sn = hs.node
            if sn:
                hdls.extend(dn.__device.dev_handle for dn in sn.__depends)
                for dn in sn.depends_on:
                    if dn in sn.__depends:
                        dep_paths.append(dn.path)
                    else:
                        dep_paths.append('(%s)' % dn.path)
            if len(hs.ext_deps) > 0:
                # TODO: map these to something smaller?
                ext_paths.extend(map(str, hs.ext_deps))
                hdls.append(DEVICE_HANDLE_SEP)
                hdls.extend(hs.ext_deps)

            # When CONFIG_USERSPACE is enabled the pre-built elf is
            # also used to get hashes that identify kernel objects by
            # address.  We can't allow the size of any object in the
            # final elf to change.
            while len(hdls) < len(hs.handles):
                hdls.append(DEVICE_HANDLE_ENDS)
            assert len(hdls) == len(hs.handles), "%s handle overflow" % (dev.sym.name,)

            lines = [
                '',
                '/* %d : %s:' % (dev.dev_handle, (sn and sn.path) or "sysinit"),
            ]

            if len(dep_paths) > 0:
                lines.append(' * - %s' % ('\n * - '.join(dep_paths)))
            if len(ext_paths) > 0:
                lines.append(' * + %s' % ('\n * + '.join(ext_paths)))

            lines.extend([
                ' */',
                'const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))',
                '%s[] = { %s };' % (hs.sym.name, ', '.join([handle_name(_h) for _h in hdls])),
                '',
            ])

            fp.write('\n'.join(lines))

if __name__ == "__main__":
    main()
