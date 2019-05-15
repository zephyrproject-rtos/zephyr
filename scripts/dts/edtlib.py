import fnmatch
import os
import re

import dtlib


class EDT:
    def __init__(self, dts, bindings_dir):
        self._create_compat2binding(bindings_dir)
        self._create_devices(dts)

    def _create_compat2binding(self, bindings_dir):
        # Creates self._compat2binding, which maps compat strings to paths to
        # bindings (.yaml files) that implement the compat

        self._compat2binding = {}

        def extract_compat(binding_path):
            with open(binding_path) as binding:
                for line in binding:
                    match = re.match(r'\s+constraint:\s*"([^"]*)"', line)
                    if match:
                        self._compat2binding[match.group(1)] = binding_path
                        return

        for root, _, filenames in os.walk(bindings_dir):
            for filename in fnmatch.filter(filenames, "*.yaml"):
                extract_compat(os.path.join(root, filename))

    def _create_devices(self, dts):
        # Creates self.devices, which maps device names to Device instances.
        # Currently, a device is defined as a node whose 'compatible' property
        # contains a compat string covered by some binding.

        self.devices = {}

        for node in dtlib.DT(dts).node_iter():
            if "compatible" in node.props:
                for comp in node.props["compatible"].to_strings():
                    if comp in self._compat2binding:
                        self._create_device(node)

    def _create_device(self, node):
        # Creates and registers a Device for 'node'

        dev = Device()
        dev.name = node.name
        dev.regs = _regs(node)

        self.devices[dev.name] = dev

    def __repr__(self):
        return "<EDT, {} devices>".format(len(self.devices))


class Device:
    def __repr__(self):
        return "<Device {}, {} regs>".format(
            self.name, len(self.regs))


class Register:
    def __repr__(self):
        fields = []

        if self.name is not None:
            fields.append("name: " + self.name)

        fields.append("addr: " + hex(self.addr))

        if self.size:
            fields.append("size: " + hex(self.size))

        return "<Register, {}>".format(", ".join(fields))


class EDTError(Exception):
    "Exception raised for Extended Device Tree-related errors"


def _regs(node):
    # Returns a list of Register instances for 'node'

    if "reg" not in node.props:
        return []

    address_cells = _address_cells(node)
    size_cells = _size_cells(node)

    regs = []
    for raw_reg in _slice(node, "reg", 4*(address_cells + size_cells)):
        reg = Register()
        reg.addr = _translate(dtlib.to_num(raw_reg[:4*address_cells]), node)
        if size_cells != 0:
            reg.size = dtlib.to_num(raw_reg[4*address_cells:])
        else:
            reg.size = None

        regs.append(reg)

    if "reg-names" in node.props:
        reg_names = node.props["reg-names"].to_strings()
        if len(reg_names) != len(regs):
            raise EDTError(
                "'reg-names' property in {} has {} strings, but there are "
                "{} registers".format(node.name, len(reg_names), len(regs)))

        for reg, name in zip(regs, reg_names):
            reg.name = name
    else:
        for reg in regs:
            reg.name = None

    return regs


def _translate(addr, node):
    # Recursively translates 'addr' on 'node' to the address space(s) of its
    # parent(s), by looking at 'ranges' properties. Returns the translated
    # address.

    if "ranges" not in node.parent.props:
        # No translation
        return addr

    # Gives the size of each component in a translation 3-tuple in 'ranges'
    child_address_cells = _address_cells(node)
    parent_address_cells = _address_cells(node.parent)
    child_size_cells = _size_cells(node)

    # Number of cells for one translation 3-tuple in 'ranges'
    entry_cells = child_address_cells + parent_address_cells + child_size_cells

    for raw_range in _slice(node.parent, "ranges", 4*entry_cells):
        child_addr = dtlib.to_num(raw_range[:4*child_address_cells])
        child_len = dtlib.to_num(
            raw_range[4*(child_address_cells + parent_address_cells):])

        if child_addr <= addr <= child_addr + child_len:
            # 'addr' is within range of a translation in 'ranges'. Recursively
            # translate it and return the result.
            parent_addr = dtlib.to_num(
                raw_range[4*child_address_cells:
                          4*(child_address_cells + parent_address_cells)])
            return _translate(parent_addr + addr - child_addr, node.parent)

    # 'addr' is not within range of any translation in 'ranges'
    return addr


def _address_cells(node):
    # Returns the #address-cells setting for 'node'
    # TODO: explain?

    if "#address-cells" in node.parent.props:
        return node.parent.props["#address-cells"].to_num()
    return 2  # Default value per DT spec.


def _size_cells(node):
    # Returns the #size-cells setting for 'node'
    # TODO: explain?

    if "#size-cells" in node.parent.props:
        return node.parent.props["#size-cells"].to_num()
    return 1  # Default value per DT spec.


def _slice(node, prop_name, size):
    # Splits node.props[prop_name].value into 'size'-sized chunks, returning a
    # list of chunks. Raises EDTError if the length of the property is not
    # evenly divisible by 'size'.

    raw = node.props[prop_name].value
    if len(raw) % size:
        raise EDTError(
            "'{}' property in {} has length {}, which is not evenly divisible "
            "by {}".format(prop_name, len(raw), size))

    return [raw[i:i + size] for i in range(0, len(raw), size)]
