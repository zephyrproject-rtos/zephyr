#!/usr/bin/env python3
#
# Copyright (c) 2022, CSIRO
#
# SPDX-License-Identifier: Apache-2.0

import struct
import sys
from packaging import version

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

if version.parse(elftools.__version__) < version.parse('0.24'):
    sys.exit("pyelftools is out of date, need version 0.24 or later")

class _Symbol:
    """
    Parent class for objects derived from an elf symbol.
    """
    def __init__(self, elf, sym):
        self.elf = elf
        self.sym = sym
        self.data = self.elf.symbol_data(sym)

    def _data_native_read(self, offset):
        (format, size) = self.elf.native_struct_format
        return struct.unpack(format, self.data[offset:offset + size])[0]

class DevicePM(_Symbol):
    """
    Represents information about device PM capabilities.
    """
    required_ld_consts = [
        "_PM_DEVICE_STRUCT_FLAGS_OFFSET",
        "_PM_DEVICE_FLAG_PD"
    ]

    def __init__(self, elf, sym):
        super().__init__(elf, sym)
        self.flags = self._data_native_read(self.elf.ld_consts['_PM_DEVICE_STRUCT_FLAGS_OFFSET'])

    @property
    def is_power_domain(self):
        return self.flags & (1 << self.elf.ld_consts["_PM_DEVICE_FLAG_PD"])

class DeviceOrdinals(_Symbol):
    """
    Represents information about device dependencies.
    """
    DEVICE_HANDLE_SEP = -32768
    DEVICE_HANDLE_ENDS = 32767
    DEVICE_HANDLE_NULL = 0

    def __init__(self, elf, sym):
        super().__init__(elf, sym)
        format = "<" if self.elf.little_endian else ">"
        format += "{:d}h".format(len(self.data) // 2)
        self._ordinals = struct.unpack(format, self.data)
        self._ordinals_split = []

        # Split ordinals on DEVICE_HANDLE_SEP
        prev =  1
        for idx, val in enumerate(self._ordinals, 1):
            if val == self.DEVICE_HANDLE_SEP:
                self._ordinals_split.append(self._ordinals[prev:idx-1])
                prev = idx
        self._ordinals_split.append(self._ordinals[prev:])

    @property
    def self_ordinal(self):
        return self._ordinals[0]

    @property
    def ordinals(self):
        return self._ordinals_split

class Device(_Symbol):
    """
    Represents information about a device object and its references to other objects.
    """
    required_ld_consts = [
        "_DEVICE_STRUCT_HANDLES_OFFSET",
        "_DEVICE_STRUCT_PM_OFFSET"
    ]

    def __init__(self, elf, sym):
        super().__init__(elf, sym)
        self.edt_node = None
        self.handle = None
        self.ordinals = None
        self.pm = None

        # Devicetree dependencies, injected dependencies, supported devices
        self.devs_depends_on = set()
        self.devs_depends_on_injected = set()
        self.devs_supports = set()

        # Point to the handles instance associated with the device;
        # assigned by correlating the device struct handles pointer
        # value with the addr of a Handles instance.
        ordinal_offset = self.elf.ld_consts['_DEVICE_STRUCT_HANDLES_OFFSET']
        self.obj_ordinals = self._data_native_read(ordinal_offset)
        self.obj_pm = None
        if '_DEVICE_STRUCT_PM_OFFSET' in self.elf.ld_consts:
            pm_offset = self.elf.ld_consts['_DEVICE_STRUCT_PM_OFFSET']
            self.obj_pm = self._data_native_read(pm_offset)

    @property
    def ordinal(self):
        return self.ordinals.self_ordinal

class ZephyrElf:
    """
    Represents information about devices in an elf file.
    """
    def __init__(self, kernel, edt, device_start_symbol):
        self.elf = ELFFile(open(kernel, "rb"))
        self.edt = edt
        self.devices = []
        self.ld_consts = self._symbols_find_value(set([device_start_symbol, *Device.required_ld_consts, *DevicePM.required_ld_consts]))
        self._device_parse_and_link()

    @property
    def little_endian(self):
        """
        True if the elf file is for a little-endian architecture.
        """
        return self.elf.little_endian

    @property
    def native_struct_format(self):
        """
        Get the struct format specifier and byte size of the native machine type.
        """
        format = "<" if self.little_endian else ">"
        if self.elf.elfclass == 32:
            format += "I"
            size = 4
        else:
            format += "Q"
            size = 8
        return (format, size)

    def symbol_data(self, sym):
        """
        Retrieve the raw bytes associated with a symbol from the elf file.
        """
        addr = sym.entry.st_value
        len = sym.entry.st_size
        for section in self.elf.iter_sections():
            start = section['sh_addr']
            end = start + section['sh_size']

            if (start <= addr) and (addr + len) <= end:
                offset = addr - section['sh_addr']
                return bytes(section.data()[offset:offset + len])

    def _symbols_find_value(self, names):
        symbols = {}
        for section in self.elf.iter_sections():
            if isinstance(section, SymbolTableSection):
                for sym in section.iter_symbols():
                    if sym.name in names:
                        symbols[sym.name] = sym.entry.st_value
        return symbols

    def _object_find_named(self, prefix, cb):
        for section in self.elf.iter_sections():
            if isinstance(section, SymbolTableSection):
                for sym in section.iter_symbols():
                    if sym.entry.st_info.type != 'STT_OBJECT':
                        continue
                    if sym.name.startswith(prefix):
                        cb(sym)

    def _link_devices(self, devices):
        # Compute the dependency graph induced from the full graph restricted to the
        # the nodes that exist in the application.  Note that the edges in the
        # induced graph correspond to paths in the full graph.
        root = self.edt.dep_ord2node[0]

        for ord, dev in devices.items():
            n = self.edt.dep_ord2node[ord]

            deps = set(n.depends_on)
            while len(deps) > 0:
                dn = deps.pop()
                if dn.dep_ordinal in devices:
                    # this is used
                    dev.devs_depends_on.add(devices[dn.dep_ordinal])
                elif dn != root:
                    # forward the dependency up one level
                    for ddn in dn.depends_on:
                        deps.add(ddn)

            sups = set(n.required_by)
            while len(sups) > 0:
                sn = sups.pop()
                if sn.dep_ordinal in devices:
                    dev.devs_supports.add(devices[sn.dep_ordinal])
                else:
                    # forward the support down one level
                    for ssn in sn.required_by:
                        sups.add(ssn)

    def _link_injected(self, devices):
        for dev in devices.values():
            injected = dev.ordinals.ordinals[1]
            for inj in injected:
                if inj in devices:
                    dev.devs_depends_on_injected.add(devices[inj])
                    devices[inj].devs_supports.add(dev)

    def _device_parse_and_link(self):
        # Find all PM structs
        pm_structs = {}
        def _on_pm(sym):
            pm_structs[sym.entry.st_value] = DevicePM(self, sym)
        self._object_find_named('__pm_device_', _on_pm)

        # Find all ordinal arrays
        ordinal_arrays = {}
        def _on_ordinal(sym):
            ordinal_arrays[sym.entry.st_value] = DeviceOrdinals(self, sym)
        self._object_find_named('__devicehdl_', _on_ordinal)

        # Find all device structs
        def _on_device(sym):
            self.devices.append(Device(self, sym))
        self._object_find_named('__device_', _on_device)

        # Sort the device array by address for handle calculation
        self.devices = sorted(self.devices, key = lambda k: k.sym.entry.st_value)

        # Assign handles to the devices
        for idx, dev in enumerate(self.devices):
            dev.handle = 1 + idx

        # Link devices structs with PM and ordinals
        for dev in self.devices:
            if dev.obj_pm in pm_structs:
                dev.pm = pm_structs[dev.obj_pm]
            if dev.obj_ordinals in ordinal_arrays:
                dev.ordinals = ordinal_arrays[dev.obj_ordinals]
                if dev.ordinal != DeviceOrdinals.DEVICE_HANDLE_NULL:
                    dev.edt_node = self.edt.dep_ord2node[dev.ordinal]

        # Create mapping of ordinals to devices
        devices_by_ord = {d.ordinal: d for d in self.devices if d.edt_node}

        # Link devices to each other based on the EDT tree
        self._link_devices(devices_by_ord)

        # Link injected devices to each other
        self._link_injected(devices_by_ord)

    def device_dependency_graph(self, title, comment):
        """
        Construct a graphviz Digraph of the relationships between devices.
        """
        import graphviz
        dot = graphviz.Digraph(title, comment=comment)
        # Split iteration so nodes and edges are grouped in source
        for dev in self.devices:
            if dev.ordinal == DeviceOrdinals.DEVICE_HANDLE_NULL:
                text = '{:s}\\nHandle: {:d}'.format(dev.sym.name, dev.handle)
            else:
                n = self.edt.dep_ord2node[dev.ordinal]
                text = '{:s}\\nOrdinal: {:d} | Handle: {:d}\\n{:s}'.format(
                    n.name, dev.ordinal, dev.handle, n.path
                )
            dot.node(str(dev.ordinal), text)
        for dev in self.devices:
            for sup in dev.devs_supports:
                dot.edge(str(dev.ordinal), str(sup.ordinal))
        return dot
