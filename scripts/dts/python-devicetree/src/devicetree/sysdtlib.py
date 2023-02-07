# Copyright (c) 2022-2023, Nordic Semiconductor ASA
# SPDX-License-Identifier: BSD-3-Clause

"""
A library for extracting regular devicetrees from system devicetree (SDT)
files.

See the specification in the lopper repository for more information on
system devicetree: https://github.com/devicetree-org/lopper
"""

from __future__ import annotations

from copy import deepcopy
from dataclasses import dataclass
from typing import Iterable, NoReturn, Optional, TYPE_CHECKING
import logging

from devicetree.dtlib import DT, DTError, Node, Property, Register, to_num
from devicetree.dtlib import Type as PropType
from devicetree._private import _slice_helper

class SysDTError(Exception):
    "Exception raised for system devicetree-related errors"

class SysDT(DT):
    """
    Represents a system devicetree.

    The main user-facing API is the convert_to_dt() method.
    """

    def __init__(self, *args, **kwargs):
        # TODO: support /omit-if-no-ref/ "correctly"
        #
        # It's not clear what the semantics should be. Options:
        #
        #     1. Propagate /omit-if-no-ref/ annotations
        #        to the result, so the final DTS can add more?
        #
        #     2. Apply /omit-if-no-ref/ twice: once
        #        at system DT level, and again at DT level?
        #
        #     3. Do something else?
        #
        # Defer this decision until we have more use cases analyzed.
        super().__init__(*args, **kwargs)

    def convert_to_dt(self, domain_path: str) -> DT:
        """
        Convert this system devicetree to a regular DT object by
        selecting an execution domain of interest. The CPU cluster,
        its address map, and the CPU mask are inferred from the cpus
        property in the execution domain node.

        domain_path:
          Path to the execution domain node. Its compatible property
          must include the value "openamp,domain-v1".
        """
        # TODO
        #
        # - support or explicitly disallow the following standard
        #   properties:
        #
        #     - #memory-flags-cells
        #     - memory
        #     - #sram-flags-cells
        #     - sram
        #     - #memory-implicit-default-cells
        #     - memory-implicit-default
        #     - #sram-implicit-default-cells
        #     - sram-implicit-default

        dt = deepcopy(self)

        domain = Domain(dt.get_node(domain_path))
        self._setup_cpus_for(dt, domain)
        self._restrict_dt_to(dt, domain)
        self._clean_up(domain)

        return dt

    def _setup_cpus_for(self, dt: DT, domain: Domain):
        # Destructively modify the devicetree to just the CPUs requested
        # for the given domain:
        #
        # - mask out any CPUs the domain doesn't use
        # - move the domain's CPU cluster to /cpus

        # We don't expect a default cluster to exist. (We're about to
        # move the selected cpu cluster to /cpus, which will error out
        # if it already exists). If we need to make this more robust
        # later, we can.
        if dt.has_node('/cpus'):
            _err("a /cpus node already exists: default CPU clusters "
                 "are currently unsupported")

        # Delete any masked-out CPUs from the tree; the final
        # devicetree should not know about them.
        domain_cpu_set = set(domain.cpus)
        for cpu in domain.cpu_cluster.cpus:
            if cpu not in domain_cpu_set:
                self._delete_node(cpu)

        # Move the remaining nodes in the domain's CPU cluster to
        # /cpus, where they are expected in a standard devicetree.
        dt.move_node(domain.cpu_cluster.node, '/cpus')

    def _restrict_dt_to(self, dt: DT, domain: Domain):
        # Destructively modify the devicetree to remove hardware
        # resources that aren't visible to the given domain:
        #
        # - convert indirect-bus nodes to simple-bus nodes
        #   (since the domain's CPU cluster is already /cpus)
        # - rewrite resource addresses to match the domain's
        #   address map

        mapped_nodes: set[Node] = set()
        for entry in domain.address_map:
            _apply_map_entry(domain, entry, mapped_nodes)

        if not mapped_nodes:
            _err(f"domain '{domain.node.path}' has an empty address map")

        # Delete any indirect-bus nodes that we didn't end up referencing.
        # Delete every simple-bus node, because we can't reference those
        # unless they are explicitly mapped in.
        # list() avoids errors from modification during iteration.
        referenced_buses = [entry.ref_node for entry in domain.address_map]
        for bus_node in referenced_buses:
            if 'indirect-bus' not in _compatible(bus_node):
                raise NotImplementedError('non-indirect-bus nodes in an address map')  # TODO
        for node in list(dt.node_iter()):
            compats = set(_compatible(node))
            if 'indirect-bus' in compats and node not in referenced_buses:
                self._delete_node(node)
            elif 'simple-bus' in compats:
                self._delete_node(node)

        # Delete any children of an indirect-bus that we didn't end up
        # referencing.
        for bus_node in referenced_buses:
            for child_node in bus_node.nodes.values():
                if child_node.nodes:
                    raise NotImplementedError('non-leaf nodes underneath indirect buses') # TODO
                if child_node not in mapped_nodes:
                    self._delete_node(child_node)

        # Move the domain's chosen and move reserved-memory to top level.
        domain_nodes = domain.node.nodes
        if 'chosen' in domain_nodes:
            dt.move_node(domain_nodes['chosen'], '/chosen')
        if 'reserved-memory' in domain_nodes:
            dt.move_node(domain_nodes['reserved-memory'], '/reserved-memory')

        # Convert each indirect-bus to a simple-bus now that all the
        # nodes inside have their correct addresses.
        for bus_node in referenced_buses:
            compatible = bus_node.props['compatible']
            if compatible.offset_labels:
                # There doesn't seem to be a use case for an internal label
                # within this property value. Forbid it so that the following
                # reset to the compatible value doesn't drop any labels.
                _err(f"indirect bus node '{bus_node.path}' compatible "
                     'property value may not contain internal labels')
            compatible.value = compatible.value.replace(b'indirect-bus',
                                                        b'simple-bus')

    @staticmethod
    def _delete_node(node):
        # TODO add and use a new dtlib public API for deletion
        # instead of using this private node method.
        #
        #  - ensure we handle the subtree rooted at 'node'
        #  - make sure we error out on dangling phandles
        node._del()

    @staticmethod
    def _clean_up(domain: Domain) -> None:
        # Any address-map properties are a system devicetree
        # extension. Remove them before emitting the DT in case
        # another address map referred to resources that are
        # no longer visible to us.
        cluster = domain.cpu_cluster.node
        for prop in list(cluster.props):
            if prop.startswith('address-map-') or prop == 'address-map':
                del cluster.props[prop]

class CpuCluster:
    '''Represents a CPU cluster in the system devicetree.

    These attributes are available on CpuCluster instances:

    node:
      The dtlib.Node (with compatible "cpus,cluster") representing
      the cluster itself.

    cpus:
      A list of dtlib.Node, one for each CPU in the cluster.
      A node is considered a CPU if its name starts with 'cpu@'
      or if it has a device_type property set to 'cpu'.

    ranges_address_cells:
      The value of the #ranges-address-cells property of the cluster,
      as an int.

    ranges_size_cells:
      The value of the #ranges-size-cells property of the cluster,
      as an int.
    '''

    def __init__(self, node: Node):
        self._check_node(node)
        self.node = node
        self.ranges_address_cells = \
            node.props['#ranges-address-cells'].to_num()
        self.ranges_size_cells = \
            node.props['#ranges-size-cells'].to_num()

    @property
    def cpus(self):
        '''
        See the class documentation.
        '''
        return [child for child in self.node.nodes.values() if
                self._is_cpu_node(child)]

    @staticmethod
    def _check_node(node: Node):
        # Verify the node complies with the system DT spec for
        # CPU cluster nodes.

        if 'cpus,cluster' not in _compatible(node):
            _err(f"CPU cluster '{node.path}' must have a compatible "
                 'property with "cpus,cluster" in the value')
        if ('#address-cells' not in node.props or
            node.props['#address-cells'].to_num() != 1):
            _err(f"CPU cluster '{node.path}' must have a "
                 '#address-cells property set to <1>')
        if ('#size-cells' not in node.props or
            node.props['#size-cells'].to_num() != 0):
            _err(f"CPU cluster '{node.path}' must have a "
                 '#size-cells property set to <0>')
        if '#ranges-address-cells' not in node.props:
            _err(f"CPU cluster '{node.path}' must have a "
                 '#ranges-address-cells property')
        if '#ranges-size-cells' not in node.props:
            _err(f"CPU cluster '{node.path}' must have a "
                 '#ranges-size-cells property')

    @staticmethod
    def _is_cpu_node(node: Node):
        # Does the given node look like a CPU?

        return (_to_string(node, 'device_type') == 'cpu' or
                node.name.startswith('cpu@'))


class Domain:
    '''Represents an execution domain in the system devicetree.

    These attributes are available on Domain instances:

    node:
      The dtlib.Node underneath /domains corresponding to the domain.

    cpu_cluster:
      A CpuCluster instance representing the CPU cluster that this
      domain node's 'cpus' property points to in its cpu-cluster cell.

    cpu_mask:
      The cpu-mask cell in the domain node's cpus property, as an int.

    execution_level:
      The execution-level cell in the domain node's cpus property,
      as an int.

    cpus:
      A list of dtlib.Node instances representing the CPU nodes in
      'cpu_cluster' which are selected by 'cpu_mask'.

    address_map:
      A list[AddressMapEntry] for this domain. This is normally determined
      by the 'address-map' property in the domain's CPU cluster.

      As a Zephyr-specific extension to the system DT specification, we
      use the 'address-map-secure' property instead on Arm v8-M processors
      if execution_level has bit 31 set, indicating the domain
      runs secure world firmware. (Using bit 31 in this way is analogous to
      the system DT spec's binding for Cortex-R5 and Cortex-A53 CPUs.)
    '''

    # TODO:
    # - finalize execution-level semantics for CPU clusters in
    #   architectures we are interested in

    # To handle the Zephyr extension for Arm v8-M CPU address maps.
    _EXECUTION_LEVEL_ARM_V8M_SECURE = (1 << 31)

    def __init__(self, node: Node):
        self._check_node(node)
        self.node = node

        cpu_cluster, cpu_mask, execution_level = (
            to_num(field) for field in _slice(
            node, 'cpus', 4,
            '&cpu-cluster (=4 bytes) cpu-mask (=4 bytes) '
            'execution-level (=4 bytes)'))
        self.cpu_cluster = CpuCluster(node.dt.phandle2node[cpu_cluster])
        self.cpu_mask = cpu_mask
        self.execution_level = execution_level

        self.cpus = [cpu for cpu in self.cpu_cluster.cpus
                     if self.cpu_mask & (1 << _unit_addr(cpu))]
        if not self.cpus:
            _err(f"domain '{node.path}' has no CPUs; "
                 f"check domain cpu-mask '{hex(self.cpu_mask)}' against "
                 f"CPU nodes in CPU cluster '{node.path}'")

        self.address_map = self._decoded_address_map()

    @staticmethod
    def _check_node(node):
        # Do some basic validation on the devicetree node we are being
        # created from.

        # Verify requirements.
        if not node.path.startswith('/domains/'):
            _err(f"domain '{node.path}' must be in the subtree "
                 'rooted at /domains')

        if 'openamp,domain-v1' not in _compatible(node):
            _err(f"domain '{node.path}' must have a compatible "
                 'property with "openamp,domain-v1" in the value')

        cpus = node.props.get('cpus')
        if cpus is None:
            _err(f"domain '{node.path}' is missing required property 'cpus'")
        if cpus.type is not PropType.PHANDLES_AND_NUMS:
            _err(f"domain '{node.path}' property 'cpus' should "
                 'have the form <&cpu-cluster cpu-mask execution-level>')
        if 'id' not in node.props:
           err(f"domain '{node.path}' is missing required property 'id'")

        # Error out on unsupported specification features.
        for prop in ['#access-flags-cells',
                     'access',
                     '#access-implicit-default-cells',
                     'access-implicit-default']:
            if prop in node.props:
                _err(f"domain '{node.path}' contains unsupported "
                     f'property {prop}')

    def _decoded_address_map(self) -> list[AddressMapEntry]:
        # Decode the domain's 'address-map' property.

        dt = self.node.dt
        root = dt.get_node('/')
        if '#address-cells' not in root.props:
            _err('the root node must have a #address-cells property set')
        cluster = self.cpu_cluster

        # Number of bytes in each node-address field.
        node_address_size = 4 * cluster.ranges_address_cells
        # Number of bytes in each root-node-address field.
        root_node_address_size = 4 * root.props['#address-cells'].to_num()
        # Number of bytes in each length field.
        length_size = 4 * cluster.ranges_size_cells

        # Split the address map property into quartets.
        entry_size = (node_address_size +
                      4 +  # ref-node is a phandle
                      root_node_address_size +
                      length_size)
        entries = _slice(self.cpu_cluster.node,
                        self._address_map_prop,
                        entry_size,
                        f'''\
4 * #ranges-address-cells (= {node_address_size}) + \
4 (ref-node phandle) + \
4 * root #address-cells (= {root_node_address_size}) + \
4 * #ranges-size-cells (= {length_size})''')

        # Enumerate the quartets
        ref_node_offset = node_address_size
        root_node_address_offset = ref_node_offset + 4
        length_offset = root_node_address_offset + root_node_address_size
        ret = []
        for entry in entries:
            ret.append(AddressMapEntry(
                node_address=to_num(entry[:node_address_size]),
                ref_node=dt.phandle2node[to_num(entry[ref_node_offset:
                                                      ref_node_offset + 4])],
                root_node_address=to_num(entry[root_node_address_offset:
                                               root_node_address_offset +
                                               root_node_address_size]),
                length=to_num(entry[length_offset:
                                    length_offset + length_size])))


        return ret

    @property
    def _address_map_prop(self):
        # Get the name of the address map property to use for the
        # given domain, handling Arm v8-M designs as a special case
        # that may have 'address-map-secure' properties.

        first_cpu_compat = set(_compatible(self.cpus[0]))
        if not all(set(_compatible(cpu)) == first_cpu_compat
                   for cpu in self.cpus):
            _LOG.warning(f"domain '{self.node.path}' CPUs have differing "
                         'compatible properties; unexpected address map '
                         'behavior may result')

        def arm_v8m_address_map():
            if self.execution_level & self._EXECUTION_LEVEL_ARM_V8M_SECURE:
                return 'address-map-secure'
            return 'address-map'

        # A map from CPU compatibles to a selector function for the
        # appropriate cluster-level address map property to use, in
        # cases where we want to be able to override the default.
        #
        # We're making this extensible in case other CPU families have
        # similar problems with the system DT spec as it stands.
        _compat2address_map_sel = {
            'arm,cortex-m23': arm_v8m_address_map,
            'arm,cortex-m33': arm_v8m_address_map,
            'arm,cortex-m33f': arm_v8m_address_map,
        }

        for compat in first_cpu_compat:
            if compat in _compat2address_map_sel:
                return _compat2address_map_sel[compat]()

        return 'address-map'


@dataclass
class AddressMapEntry:
    '''
    Represents an entry in a CPU cluster's address-map property
    within the system devicetree.

    The following attributes are available on AddressMapEntry instances:

    node_address:
      The physical address within the CPU cluster's address space
      that the resources described by the entry appear in, as an int.

    ref_node:
      The dtlib.Node whose resources are mapped into the CPU cluster's
      address space by this entry.

    root_node_address:
      The physical starting address, within the root node's address space,
      of the resources whose addresses are being mapped in, as an int

    length:
      The size of the address range being mapped into the CPU cluster's
      address space, in bytes, as an int.
    '''

    node_address: int
    ref_node: Node
    root_node_address: int
    length: int


def _err(msg: str) -> NoReturn:
    raise SysDTError(msg)


def _slice(node: Node, prop_name: str, size: int, size_hint: str):
    return _slice_helper(node, prop_name, size, size_hint, SysDTError)


def _to_string(node: Node, propname: str) -> Optional[str]:
    prop = node.props.get(propname)
    if prop is None:
        return None
    return prop.to_string()


def _to_strings(node: Node, propname: str) -> list[str]:
    prop = node.props.get(propname)
    if not prop:
        return []
    return prop.to_strings()


def _compatible(node: Node) -> list[str]:
    return _to_strings(node, 'compatible')


def _unit_addr(node: Node) -> int:
    unit_addr = node.unit_addr
    if not unit_addr:
        _err(f"node '{node.path}' must have a unit address "
             "(the name should look like 'foo@abcd1234', not 'foo')'")

    try:
        return int(unit_addr, 16)
    except ValueError:
        _err(f"node '{node.path}' has unit address '{unit_addr}': expected "
             'a hexadecimal number without a leading 0x')

def _apply_map_entry(domain: Domain,
                     entry: AddressMapEntry,
                     mapped_nodes: set[Node]):
    # Helper for _restrict_dt_to(). Applies an address map entry to a
    # devicetree node by rewriting its reg property as needed. If the
    # node was in fact mapped in, it will be added to mapped_nodes.
    # (If the node is already in mapped_nodes, an error is raised as
    # it seems incorrect to map the same device in at two addressses
    # explicitly.)

    bus_node = entry.ref_node
    dt = bus_node.dt
    base, length = entry.root_node_address, entry.length
    end = base + length

    # TODO verify this is correct for the new regs.
    cluster = domain.cpu_cluster
    new_address_cells = cluster.ranges_address_cells
    new_size_cells = cluster.ranges_size_cells

    if base != 0:
        # The spec semantics aren't clear to me right now
        raise NotImplementedError('nonzero root-node-address') # TODO

    _verify_is_indirect_bus(bus_node)
    _verify_has_no_ranges(bus_node)

    def reg_has_overlap(reg: Register) -> bool:
        # Checks if a register overlaps the address range
        # specified by 'entry'.
        if TYPE_CHECKING:
            assert reg.addr is not None
        return base <= reg.addr < end

    def new_address(reg: Register) -> int:
        if TYPE_CHECKING:
            assert reg.addr is not None
        return entry.node_address + base + reg.addr

    def new_size(reg: Register) -> int:
        # Clamp a register block's size to fit within
        # the span given by 'entry'.
        if TYPE_CHECKING:
            assert reg.addr is not None and reg.size is not None
        if reg.addr + reg.size >= end: # check depends on base==0!
            # The specification says these are the semantics,
            # but it's probably a mistake on the DT author's part.
            _LOG.warning(
                f"node '{reg.node.path}' register blocks "
                'were clamped to fit in the CPU cluster address map; '
                'this may be a misconfigured address map')
            return length - reg.addr
        return reg.size

    # Keeping track of what to move and doing it later prevents
    # errors due to modifying the tree while iterating over it.
    movements: dict[Node, str] = {}
    for child_node in bus_node.node_iter():
        if child_node is bus_node: # skip the bus node itself
            continue
        _verify_has_no_ranges(child_node)
        if child_node.parent is not bus_node:
            raise NotImplementedError('non-leaf nodes underneath indirect buses') # TODO

        if 'reg' not in child_node.props:
            # TODO verify address-map semantics for nodes without 'reg'
            #
            # It seems important to be able to map them in for
            # things like fixed-clock nodes, so just assume
            # they are all part of the address map for now.
            mapped_nodes.add(child_node)
            continue

        # Convert reg properties and unit addresses in the
        # indirect bus to the domain's address map.
        # TODO: support multiple register blocks per node

        regs = child_node.regs

        for reg in regs:
            if reg.addr is None or reg.size is None:
                # This will affect reg_has_overlap(), new_address(),
                # new_size() above.
                raise NotImplementedError('how to handle this?') # TODO

        new_regs_ints = [(new_address(reg), new_size(reg))
                         for reg in regs if reg_has_overlap(reg)]
        if not new_regs_ints:
            # Node's registers don't fit in the mapped entry.
            # We'll delete it later if we don't find another
            # entry that does map it in.
            continue

        if child_node in mapped_nodes:
            # It seems doubtful that we need to specify concrete
            # semantics here, so let's just error out for now.
            # We can always handle this case later if needed.
            _err(f"node '{child_node.path}' appears in multiple "
                 "address map entries for CPU cluster "
                 f"'{cluster.node.path}', which "
                 "was selected by execution domain "
                 f"'{domain.node.path}'")

        child_node.props['reg'].value = b''.join(
            addr.to_bytes(new_address_cells*4, 'big') +
            size.to_bytes(new_size_cells*4, 'big')
            for (addr, size) in new_regs_ints
        )
        new_reg = child_node.regs[0]
        if TYPE_CHECKING:
            assert new_reg.addr is not None

        basename = child_node.name.partition('@')[0]
        new_unit_addr = hex(new_reg.addr)[2:]
        new_name = f'{basename}@{new_unit_addr}'
        movements[child_node] = f'{child_node.parent.path}/{new_name}'
        mapped_nodes.add(child_node)

    for node, new_path in movements.items():
        dt.move_node(node, new_path)

def _verify_is_indirect_bus(node: Node):
    # Need support for mapping in simple-bus and device nodes directly.
    if 'indirect-bus' not in _compatible(node):
        raise NotImplementedError('non-indirect-bus nodes in an address map')  # TODO

def _verify_has_no_ranges(node: Node):
    if 'ranges' in node.props:
        raise NotImplementedError("'ranges' within indirect-bus node hierarchies")  # TODO

# Logging object
_LOG = logging.getLogger(__name__)
