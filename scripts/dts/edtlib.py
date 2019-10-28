# Copyright (c) 2019 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# SPDX-License-Identifier: BSD-3-Clause

# Tip: You can view just the documentation with 'pydoc3 edtlib'

"""
Library for working with devicetrees at a higher level compared to dtlib. Like
dtlib, this library presents a tree of devicetree nodes, but the nodes are
augmented with information from bindings and include some interpretation of
properties.

Bindings are files that describe devicetree nodes. Devicetree nodes are usually
mapped to bindings via their 'compatible = "..."' property, but a binding can
also come from a 'child-binding:' key in the binding for the parent devicetree
node.

Each devicetree node (dtlib.Node) gets a corresponding edtlib.Node instance,
which has all the information related to the node.

The top-level entry point of the library is the EDT class. EDT.__init__() takes
a .dts file to parse and a list of paths to directories containing bindings.
"""

import os
import re
import sys

import yaml
try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    # This makes e.g. gen_defines.py more than twice as fast.
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader

from dtlib import DT, DTError, to_num, to_nums, TYPE_EMPTY, TYPE_NUMS, \
                  TYPE_PHANDLE, TYPE_PHANDLES_AND_NUMS

# NOTE: testedtlib.py is the test suite for this library. It can be run
# directly as a script:
#
#   ./testedtlib.py

# Implementation notes
# --------------------
#
# A '_' prefix on an identifier in Python is a convention for marking it private.
# Please do not access private things. Instead, think of what API you need, and
# add it.
#
# This library is layered on top of dtlib, and is not meant to expose it to
# clients. This keeps the header generation script simple.
#
# General biased advice:
#
# - Consider using @property for APIs that don't need parameters. It makes
#   functions look like attributes, which is less awkward in clients, and makes
#   it easy to switch back and forth between variables and functions.
#
# - Think about the data type of the thing you're exposing. Exposing something
#   as e.g. a list or a dictionary is often nicer and more flexible than adding
#   a function.
#
# - Avoid get_*() prefixes on functions. Name them after the thing they return
#   instead. This often makes the code read more naturally in callers.
#
#   Also, consider using @property instead of get_*().
#
# - Don't expose dtlib stuff directly.
#
# - Add documentation for any new APIs you add.
#
#   The convention here is that docstrings (quoted strings) are used for public
#   APIs, and "doc comments" for internal functions.
#
#   @properties are documented in the class docstring, as if they were
#   variables. See the existing @properties for a template.
#
# - Please use ""-quoted strings instead of ''-quoted strings, just to make
#   things consistent (''-quoting is more common otherwise in Python)

#
# Public classes
#


class EDT:
    """
    Represents a devicetree augmented with information from bindings.

    These attributes are available on EDT objects:

    nodes:
      A list of Node objects for the nodes that appear in the devicetree

    dts_path:
      The .dts path passed to __init__()

    bindings_dirs:
      The bindings directory paths passed to __init__()
    """
    def __init__(self, dts, bindings_dirs, warn_file=None):
        """
        EDT constructor. This is the top-level entry point to the library.

        dts:
          Path to devicetree .dts file

        bindings_dirs:
          List of paths to directories containing bindings, in YAML format.
          These directories are recursively searched for .yaml files.

        warn_file:
          'file' object to write warnings to. If None, sys.stderr is used.
        """
        # Do this indirection with None in case sys.stderr is deliberately
        # overridden
        self._warn_file = sys.stderr if warn_file is None else warn_file

        self.dts_path = dts
        self.bindings_dirs = bindings_dirs

        self._dt = DT(dts)
        _check_dt(self._dt)

        self._init_compat2binding(bindings_dirs)
        self._init_nodes()

    def get_node(self, path):
        """
        Returns the Node at the DT path or alias 'path'. Raises EDTError if the
        path or alias doesn't exist.
        """
        try:
            return self._node2enode[self._dt.get_node(path)]
        except DTError as e:
            _err(e)

    def chosen_node(self, name):
        """
        Returns the Node pointed at by the property named 'name' in /chosen, or
        None if the property is missing
        """
        try:
            chosen = self._dt.get_node("/chosen")
        except DTError:
            # No /chosen node
            return None

        if name not in chosen.props:
            return None

        # to_path() checks that the node exists
        return self._node2enode[chosen.props[name].to_path()]

    def __repr__(self):
        return "<EDT for '{}', binding directories '{}'>".format(
            self.dts_path, self.bindings_dirs)

    def _init_compat2binding(self, bindings_dirs):
        # Creates self._compat2binding. This is a dictionary that maps
        # (<compatible>, <bus>) tuples (both strings) to (<binding>, <path>)
        # tuples. <binding> is the binding in parsed PyYAML format, and <path>
        # the path to the binding (nice for binding-related error messages).
        #
        # For example, self._compat2binding["company,dev", "can"] contains the
        # binding/path for the 'company,dev' device, when it appears on the CAN
        # bus.
        #
        # For bindings that don't specify a bus, <bus> is None, so that e.g.
        # self._compat2binding["company,notonbus", None] contains the binding.
        #
        # Only bindings for 'compatible' strings that appear in the devicetree
        # are loaded.

        # Add legacy '!include foo.yaml' handling. Do Loader.add_constructor()
        # instead of yaml.add_constructor() to be compatible with both version
        # 3.13 and version 5.1 of PyYAML.
        Loader.add_constructor("!include", _binding_include)

        dt_compats = _dt_compats(self._dt)
        # Searches for any 'compatible' string mentioned in the devicetree
        # files, with a regex
        dt_compats_search = re.compile(
            "|".join(re.escape(compat) for compat in dt_compats)
        ).search

        self._binding_paths = _binding_paths(bindings_dirs)

        self._compat2binding = {}
        for binding_path in self._binding_paths:
            with open(binding_path, encoding="utf-8") as f:
                contents = f.read()

            # As an optimization, skip parsing files that don't contain any of
            # the .dts 'compatible' strings, which should be reasonably safe
            if not dt_compats_search(contents):
                continue

            # Load the binding and check that it actually matches one of the
            # compatibles. Might get false positives above due to comments and
            # stuff.

            try:
                # Parsed PyYAML output (Python lists/dictionaries/strings/etc.,
                # representing the file)
                binding = yaml.load(contents, Loader=Loader)
            except yaml.YAMLError as e:
                self._warn("'{}' appears in binding directories but isn't "
                           "valid YAML: {}".format(binding_path, e))
                continue

            binding_compat = self._binding_compat(binding, binding_path)
            if binding_compat not in dt_compats:
                # Either not a binding (binding_compat is None -- might be a
                # binding fragment or a spurious file), or a binding whose
                # compatible does not appear in the devicetree (picked up via
                # some unrelated text in the binding file that happened to
                # match a compatible)
                continue

            # It's a match. Merge in the included bindings, do sanity checks,
            # and register the binding.

            binding = self._merge_included_bindings(binding, binding_path)
            self._check_binding(binding, binding_path)

            bus = _binding_bus(binding)

            # Do not allow two different bindings to have the same
            # 'compatible:'/'parent-bus:' combo
            old_binding = self._compat2binding.get((binding_compat, bus))
            if old_binding:
                msg = "both {} and {} have 'compatible: {}'".format(
                    old_binding[1], binding_path, binding_compat)
                if bus is not None:
                    msg += " and 'parent-bus: {}'".format(bus)
                _err(msg)

            self._compat2binding[binding_compat, bus] = (binding, binding_path)

    def _binding_compat(self, binding, binding_path):
        # Returns the string listed in 'compatible:' in 'binding', or None if
        # no compatible is found. Only takes 'self' for the sake of
        # self._warn().
        #
        # Also searches for legacy compatibles on the form
        #
        #   properties:
        #       compatible:
        #           constraint: <string>

        def new_style_compat():
            # New-style 'compatible: "foo"' compatible

            if binding is None or "compatible" not in binding:
                # Empty file, binding fragment, spurious file, or old-style
                # compat
                return None

            compatible = binding["compatible"]
            if not isinstance(compatible, str):
                _err("malformed 'compatible:' field in {} - should be a string"
                     .format(binding_path))

            return compatible

        def old_style_compat():
            # Old-style 'constraint: "foo"' compatible

            try:
                return binding["properties"]["compatible"]["constraint"]
            except Exception:
                return None

        new_compat = new_style_compat()
        old_compat = old_style_compat()
        if old_compat:
            self._warn("The 'properties: compatible: constraint: ...' way of "
                       "specifying the compatible in {} is deprecated. Put "
                       "'compatible: \"{}\"' at the top level of the binding "
                       "instead.".format(binding_path, old_compat))

            if new_compat:
                _err("compatibles for {} should be specified with either "
                     "'compatible:' at the top level or with the legacy "
                     "'properties: compatible: constraint: ...' field, not "
                     "both".format(binding_path))

            return old_compat

        return new_compat

    def _merge_included_bindings(self, binding, binding_path):
        # Merges any bindings listed in the 'include:' section of 'binding'
        # into the top level of 'binding'. Also supports the legacy
        # 'inherits: !include ...' syntax for including bindings.
        #
        # Properties in 'binding' take precedence over properties from included
        # bindings.

        fnames = []

        if "include" in binding:
            include = binding.pop("include")
            if isinstance(include, str):
                fnames.append(include)
            elif isinstance(include, list):
                if not all(isinstance(elm, str) for elm in include):
                    _err("all elements in 'include:' in {} should be strings"
                         .format(binding_path))
                fnames += include
            else:
                _err("'include:' in {} should be a string or a list of strings"
                     .format(binding_path))

        # Legacy syntax
        if "inherits" in binding:
            self._warn("the 'inherits:' syntax in {} is deprecated and will "
                       "be removed - please use 'include: foo.yaml' or "
                       "'include: [foo.yaml, bar.yaml]' instead"
                       .format(binding_path))

            inherits = binding.pop("inherits")
            if not isinstance(inherits, list) or \
               not all(isinstance(elm, str) for elm in inherits):
                _err("malformed 'inherits:' in " + binding_path)
            fnames += inherits

        if not fnames:
            return binding

        # Got a list of included files in 'fnames'. Now we need to merge them
        # together and then merge them into 'binding'.

        # First, merge the included files together. If more than one included
        # file has a 'required:' for a particular property, OR the values
        # together, so that 'required: true' wins.

        merged_included = self._load_binding(fnames[0])
        for fname in fnames[1:]:
            included = self._load_binding(fname)
            _merge_props(merged_included, included, None, binding_path,
                         check_required=False)

        # Next, merge the merged included files into 'binding'. Error out if
        # 'binding' has 'required: false' while the merged included files have
        # 'required: true'.

        _merge_props(binding, merged_included, None, binding_path,
                     check_required=True)

        return binding

    def _load_binding(self, fname):
        # Returns the contents of the binding given by 'fname' after merging
        # any bindings it lists in 'include:' into it. 'fname' is just the
        # basename of the file, so we check that there aren't multiple
        # candidates.

        paths = [path for path in self._binding_paths
                 if os.path.basename(path) == fname]

        if not paths:
            _err("'{}' not found".format(fname))

        if len(paths) > 1:
            _err("multiple candidates for included file '{}': {}"
                 .format(fname, ", ".join(paths)))

        with open(paths[0], encoding="utf-8") as f:
            return self._merge_included_bindings(
                yaml.load(f, Loader=Loader),
                paths[0])

    def _init_nodes(self):
        # Creates a list of edtlib.Node objects from the dtlib.Node objects, in
        # self.nodes

        # Maps each dtlib.Node to its corresponding edtlib.Node
        self._node2enode = {}

        self.nodes = []

        for dt_node in self._dt.node_iter():
            # Warning: We depend on parent Nodes being created before their
            # children. This is guaranteed by node_iter().
            node = Node()
            node.edt = self
            node._node = dt_node
            node._init_binding()
            node._init_regs()
            node._set_instance_no()

            self.nodes.append(node)
            self._node2enode[dt_node] = node

        for node in self.nodes:
            # Node._init_props() depends on all Node objects having been
            # created, due to 'type: phandle/phandles/phandle-array', so we run
            # it separately. Property.val includes the pointed-to Node
            # instance(s) for phandles, so the Node objects must exist.
            node._init_props()
            node._init_interrupts()

    def _check_binding(self, binding, binding_path):
        # Does sanity checking on 'binding'. Only takes 'self' for the sake of
        # self._warn().

        for prop in "title", "description":
            if prop not in binding:
                _err("missing '{}' property in {}".format(prop, binding_path))

            if not isinstance(binding[prop], str) or not binding[prop]:
                _err("missing, malformed, or empty '{}' in {}"
                     .format(prop, binding_path))

        ok_top = {"title", "description", "compatible", "properties", "#cells",
                  "parent-bus", "child-bus", "parent", "child",
                  "child-binding", "sub-node"}

        for prop in binding:
            if prop not in ok_top and not prop.endswith("-cells"):
                _err("unknown key '{}' in {}, expected one of {}, or *-cells"
                     .format(prop, binding_path, ", ".join(ok_top)))

        for pc in "parent", "child":
            # 'parent/child-bus:'
            bus_key = pc + "-bus"
            if bus_key in binding and \
               not isinstance(binding[bus_key], str):
                self._warn("malformed '{}:' value in {}, expected string"
                           .format(bus_key, binding_path))

            # Legacy 'child/parent: bus: ...' keys
            if pc in binding:
                self._warn("'{0}: bus: ...' in {1} is deprecated and will be "
                           "removed - please use a top-level '{0}-bus:' key "
                           "instead (see binding-template.yaml)"
                           .format(pc, binding_path))

                # Just 'bus:' is expected
                if binding[pc].keys() != {"bus"}:
                    _err("expected (just) 'bus:' in '{}:' in {}"
                         .format(pc, binding_path))

                if not isinstance(binding[pc]["bus"], str):
                    _err("malformed '{}: bus:' value in {}, expected string"
                         .format(pc, binding_path))

        self._check_binding_properties(binding, binding_path)

        if "child-binding" in binding:
            if not isinstance(binding["child-binding"], dict):
                _err("malformed 'child-binding:' in {}, expected a binding "
                     "(dictionary with keys/values)".format(binding_path))

            self._check_binding(binding["child-binding"], binding_path)

        if "sub-node" in binding:
            self._warn("'sub-node: properties: ...' in {} is deprecated and "
                       "will be removed - please give a full binding for the "
                       "child node in 'child-binding:' instead (see "
                       "binding-template.yaml)".format(binding_path))

            if binding["sub-node"].keys() != {"properties"}:
                _err("expected (just) 'properties:' in 'sub-node:' in {}"
                     .format(binding_path))

            self._check_binding_properties(binding["sub-node"], binding_path)

        if "#cells" in binding:
            self._warn('"#cells:" in {} is deprecated and will be removed - '
                       "please put 'interrupt-cells:', 'pwm-cells:', "
                       "'gpio-cells:', etc., instead. The name should match "
                       "the name of the corresponding phandle-array property "
                       "(see binding-template.yaml)".format(binding_path))

        def ok_cells_val(val):
            # Returns True if 'val' is an okay value for '*-cells:' (or the
            # legacy '#cells:')

            return isinstance(val, list) and \
                   all(isinstance(elm, str) for elm in val)

        for key, val in binding.items():
            if key.endswith("-cells") or key == "#cells":
                if not ok_cells_val(val):
                    _err("malformed '{}:' in {}, expected a list of strings"
                         .format(key, binding_path))

    def _check_binding_properties(self, binding, binding_path):
        # _check_binding() helper for checking the contents of 'properties:'.
        # Only takes 'self' for the sake of self._warn().

        if "properties" not in binding:
            return

        ok_prop_keys = {"description", "type", "required", "category",
                        "constraint", "enum", "const", "default"}

        for prop_name, options in binding["properties"].items():
            for key in options:
                if key == "category":
                    self._warn(
                        "please put 'required: {}' instead of 'category: {}' "
                        "in properties: {}: ...' in {} - 'category' will be "
                        "removed".format(
                            "true" if options["category"] == "required"
                                else "false",
                            options["category"], prop_name, binding_path))

                if key not in ok_prop_keys:
                    _err("unknown setting '{}' in 'properties: {}: ...' in {}, "
                         "expected one of {}".format(
                             key, prop_name, binding_path,
                             ", ".join(ok_prop_keys)))

            _check_prop_type_and_default(
                prop_name, options.get("type"),
                options.get("required") or options.get("category") == "required",
                options.get("default"), binding_path)

            if "required" in options and not isinstance(options["required"], bool):
                _err("malformed 'required:' setting '{}' for '{}' in 'properties' "
                     "in {}, expected true/false"
                     .format(options["required"], prop_name, binding_path))

            if "description" in options and \
               not isinstance(options["description"], str):
                _err("missing, malformed, or empty 'description' for '{}' in "
                     "'properties' in {}".format(prop_name, binding_path))

            if "enum" in options and not isinstance(options["enum"], list):
                _err("enum in {} for property '{}' is not a list"
                     .format(binding_path, prop_name))

            if "const" in options and not isinstance(options["const"], (int, str)):
                _err("const in {} for property '{}' is not a scalar"
                     .format(binding_path, prop_name))

    def _warn(self, msg):
        print("warning: " + msg, file=self._warn_file)


class Node:
    """
    Represents a devicetree node, augmented with information from bindings, and
    with some interpretation of devicetree properties. There's a one-to-one
    correspondence between devicetree nodes and Nodes.

    These attributes are available on Node objects:

    edt:
      The EDT instance this node is from

    name:
      The name of the node

    unit_addr:
      An integer with the ...@<unit-address> portion of the node name,
      translated through any 'ranges' properties on parent nodes, or None if
      the node name has no unit-address portion

    description:
      The description string from the binding for the node, or None if the node
      has no binding. Leading and trailing whitespace (including newlines) is
      removed.

    path:
      The devicetree path of the node

    label:
      The text from the 'label' property on the node, or None if the node has
      no 'label'

    parent:
      The Node instance for the devicetree parent of the Node, or None if the
      node is the root node

    children:
      A dictionary with the Node instances for the devicetree children of the
      node, indexed by name

    enabled:
      True unless the node has 'status = "disabled"'

    read_only:
      True if the node has a 'read-only' property, and False otherwise

    instance_no:
      Dictionary that maps each 'compatible' string for the node to a unique
      index among all nodes that have that 'compatible' string.

      As an example, 'instance_no["foo,led"] == 3' can be read as "this is the
      fourth foo,led node".

      Only enabled nodes (status != "disabled") are counted. 'instance_no' is
      meaningless for disabled nodes.

    matching_compat:
      The 'compatible' string for the binding that matched the node, or None if
      the node has no binding

    binding_path:
      The path to the binding file for the node, or None if the node has no
      binding

    compats:
      A list of 'compatible' strings for the node, in the same order that
      they're listed in the .dts file

    regs:
      A list of Register objects for the node's registers

    props:
      A dictionary that maps property names to Property objects. Property
      objects are created for all devicetree properties on the node that are
      mentioned in 'properties:' in the binding.

    aliases:
      A list of aliases for the node. This is fetched from the /aliases node.

    interrupts:
      A list of ControllerAndData objects for the interrupts generated by the
      node

    bus:
      The bus for the node as specified in its binding, e.g. "i2c" or "spi".
      None if the binding doesn't specify a bus.

    flash_controller:
      The flash controller for the node. Only meaningful for nodes representing
      flash partitions.
    """
    @property
    def name(self):
        "See the class docstring"
        return self._node.name

    @property
    def unit_addr(self):
        "See the class docstring"

        # TODO: Return a plain string here later, like dtlib.Node.unit_addr?

        if "@" not in self.name:
            return None

        try:
            addr = int(self.name.split("@", 1)[1], 16)
        except ValueError:
            _err("{!r} has non-hex unit address".format(self))

        addr = _translate(addr, self._node)

        if self.regs and self.regs[0].addr != addr:
            self.edt._warn("unit-address and first reg (0x{:x}) don't match "
                           "for {}".format(self.regs[0].addr, self.name))

        return addr

    @property
    def description(self):
        "See the class docstring."
        if self._binding and "description" in self._binding:
            return self._binding["description"].strip()
        return None

    @property
    def path(self):
        "See the class docstring"
        return self._node.path

    @property
    def label(self):
        "See the class docstring"
        if "label" in self._node.props:
            return self._node.props["label"].to_string()
        return None

    @property
    def parent(self):
        "See the class docstring"
        return self.edt._node2enode.get(self._node.parent)

    @property
    def children(self):
        "See the class docstring"
        # Could be initialized statically too to preserve identity, but not
        # sure if needed. Parent nodes being initialized before their children
        # would need to be kept in mind.
        return {name: self.edt._node2enode[node]
                for name, node in self._node.nodes.items()}

    @property
    def enabled(self):
        "See the class docstring"
        return "status" not in self._node.props or \
            self._node.props["status"].to_string() != "disabled"

    @property
    def read_only(self):
        "See the class docstring"
        return "read-only" in self._node.props

    @property
    def aliases(self):
        "See the class docstring"
        return [alias for alias, node in self._node.dt.alias2node.items()
                if node is self._node]

    @property
    def bus(self):
        "See the class docstring"
        return _binding_bus(self._binding)

    @property
    def flash_controller(self):
        "See the class docstring"

        # The node path might be something like
        # /flash-controller@4001E000/flash@0/partitions/partition@fc000. We go
        # up two levels to get the flash and check its compat. The flash
        # controller might be the flash itself (for cases like NOR flashes).
        # For the case of 'soc-nv-flash', we assume the controller is the
        # parent of the flash node.

        if not self.parent or not self.parent.parent:
            _err("flash partition {!r} lacks parent or grandparent node"
                 .format(self))

        controller = self.parent.parent
        if controller.matching_compat == "soc-nv-flash":
            return controller.parent
        return controller

    def __repr__(self):
        return "<Node {} in '{}', {}>".format(
            self.path, self.edt.dts_path,
            "binding " + self.binding_path if self.binding_path
                else "no binding")

    def _init_binding(self):
        # Initializes Node.matching_compat, Node._binding, and
        # Node.binding_path.
        #
        # Node._binding holds the data from the node's binding file, in the
        # format returned by PyYAML (plain Python lists, dicts, etc.), or None
        # if the node has no binding.

        # This relies on the parent of the node having already been
        # initialized, which is guaranteed by going through the nodes in
        # node_iter() order.

        if "compatible" in self._node.props:
            self.compats = self._node.props["compatible"].to_strings()
            bus = self._bus_from_parent_binding()

            for compat in self.compats:
                if (compat, bus) in self.edt._compat2binding:
                    # Binding found
                    self.matching_compat = compat
                    self._binding, self.binding_path = \
                        self.edt._compat2binding[compat, bus]

                    return
        else:
            # No 'compatible' property. See if the parent binding has a
            # 'child-binding:' key that gives the binding (or a legacy
            # 'sub-node:' key).

            self.compats = []

            binding_from_parent = self._binding_from_parent()
            if binding_from_parent:
                self._binding = binding_from_parent
                self.binding_path = self.parent.binding_path
                self.matching_compat = self.parent.matching_compat

                return

        # No binding found
        self._binding = self.binding_path = self.matching_compat = None

    def _binding_from_parent(self):
        # Returns the binding from 'child-binding:' in the parent node's
        # binding (or from the legacy 'sub-node:' key), or None if missing

        if not self.parent:
            return None

        pbinding = self.parent._binding
        if not pbinding:
            return None

        if "child-binding" in pbinding:
            return pbinding["child-binding"]

        # Backwards compatibility
        if "sub-node" in pbinding:
            return {"title": pbinding["title"],
                    "description": pbinding["description"],
                    "properties": pbinding["sub-node"]["properties"]}

        return None

    def _bus_from_parent_binding(self):
        # _init_binding() helper. Returns the bus specified by 'child-bus: ...'
        # in the parent binding (or the legacy 'child: bus: ...'), or None if
        # missing.

        if not self.parent:
            return None

        binding = self.parent._binding
        if not binding:
            return None

        if "child-bus" in binding:
            return binding["child-bus"]

        # Legacy key
        if "child" in binding:
            # _check_binding() has checked that the "bus" key exists
            return binding["child"]["bus"]

        return None

    def _init_props(self):
        # Creates self.props. See the class docstring. Also checks that all
        # properties on the node are declared in its binding.

        self.props = {}

        if not self._binding:
            return

        # Initialize self.props
        if "properties" in self._binding:
            for name, options in self._binding["properties"].items():
                self._init_prop(name, options)

        self._check_undeclared_props()

    def _init_prop(self, name, options):
        # _init_props() helper for initializing a single property

        prop_type = options.get("type")
        if not prop_type:
            _err("'{}' in {} lacks 'type'".format(name, self.binding_path))

        val = self._prop_val(
            name, prop_type,
            options.get("required") or options.get("category") == "required",
            options.get("default"))

        if val is None:
            # 'required: false' property that wasn't there, or a property type
            # for which we store no data.
            return

        enum = options.get("enum")
        if enum and val not in enum:
            _err("value of property '{}' on {} in {} ({!r}) is not in 'enum' "
                 "list in {} ({!r})"
                 .format(name, self.path, self.edt.dts_path, val,
                         self.binding_path, enum))

        const = options.get("const")
        if const is not None and val != const:
            _err("value of property '{}' on {} in {} ({!r}) is different from "
                 "the 'const' value specified in {} ({!r})"
                 .format(name, self.path, self.edt.dts_path, val,
                         self.binding_path, const))

        # Skip properties that start with '#', like '#size-cells', and mapping
        # properties like 'gpio-map'/'interrupt-map'
        if name[0] == "#" or name.endswith("-map"):
            return

        prop = Property()
        prop.node = self
        prop.name = name
        prop.description = options.get("description")
        if prop.description:
            prop.description = prop.description.strip()
        prop.val = val
        prop.type = prop_type
        prop.enum_index = None if enum is None else enum.index(val)

        self.props[name] = prop

    def _prop_val(self, name, prop_type, required, default):
        # _init_prop() helper for getting the property's value
        #
        # name:
        #   Property name from binding
        #
        # prop_type:
        #   Property type from binding (a string like "int")
        #
        # optional:
        #   True if the property isn't required to exist
        #
        # default:
        #   Default value to use when the property doesn't exist, or None if
        #   the binding doesn't give a default value

        node = self._node
        prop = node.props.get(name)

        if not prop:
            if required and self.enabled:
                _err("'{}' is marked as required in 'properties:' in {}, but "
                     "does not appear in {!r}".format(
                         name, self.binding_path, node))

            if default is not None:
                # YAML doesn't have a native format for byte arrays. We need to
                # convert those from an array like [0x12, 0x34, ...]. The
                # format has already been checked in
                # _check_prop_type_and_default().
                if prop_type == "uint8-array":
                    return bytes(default)
                return default

            return False if prop_type == "boolean" else None

        if prop_type == "boolean":
            if prop.type is not TYPE_EMPTY:
                _err("'{0}' in {1!r} is defined with 'type: boolean' in {2}, "
                     "but is assigned a value ('{3}') instead of being empty "
                     "('{0};')".format(name, node, self.binding_path, prop))
            return True

        if prop_type == "int":
            return prop.to_num()

        if prop_type == "array":
            return prop.to_nums()

        if prop_type == "uint8-array":
            return prop.to_bytes()

        if prop_type == "string":
            return prop.to_string()

        if prop_type == "string-array":
            return prop.to_strings()

        if prop_type == "phandle":
            return self.edt._node2enode[prop.to_node()]

        if prop_type == "phandles":
            return [self.edt._node2enode[node] for node in prop.to_nodes()]

        if prop_type == "phandle-array":
            # This type is a bit high-level for dtlib as it involves
            # information from bindings and *-names properties, so there's no
            # to_phandle_array() in dtlib. Do the type check ourselves.
            if prop.type not in (TYPE_PHANDLE, TYPE_PHANDLES_AND_NUMS):
                _err("expected property '{0}' in {1} in {2} to be assigned "
                     "with '{0} = < &foo 1 2 ... &bar 3 4 ... >' (a mix of "
                     "phandles and numbers), not '{3}'"
                     .format(name, node.path, node.dt.filename, prop))

            return self._standard_phandle_val_list(prop)

        # prop_type == "compound". We have already checked that the 'type:'
        # value is valid, in _check_binding().
        #
        # 'compound' is a dummy type for properties that don't fit any of the
        # patterns above, so that we can require all entries in 'properties:'
        # to have a 'type: ...'. No Property object is created for it.
        return None

    def _check_undeclared_props(self):
        # Checks that all properties are declared in the binding

        if "properties" in self._binding:
            declared_props = self._binding["properties"].keys()
        else:
            declared_props = set()

        for prop_name in self._node.props:
            # Allow a few special properties to not be declared in the binding
            if prop_name.endswith("-controller") or \
               prop_name.startswith("#") or \
               prop_name.startswith("pinctrl-") or \
               prop_name in {
                   "compatible", "status", "ranges", "phandle",
                   "interrupt-parent", "interrupts-extended", "device_type"}:
                continue

            if prop_name not in declared_props:
                _err("'{}' appears in {} in {}, but is not declared in "
                     "'properties:' in {}"
                     .format(prop_name, self._node.path, self.edt.dts_path,
                             self.binding_path))

    def _init_regs(self):
        # Initializes self.regs

        node = self._node

        self.regs = []

        if "reg" not in node.props:
            return

        address_cells = _address_cells(node)
        size_cells = _size_cells(node)

        for raw_reg in _slice(node, "reg", 4*(address_cells + size_cells)):
            reg = Register()
            reg.node = self
            reg.addr = _translate(to_num(raw_reg[:4*address_cells]), node)
            reg.size = to_num(raw_reg[4*address_cells:])
            if size_cells != 0 and reg.size == 0:
                _err("zero-sized 'reg' in {!r} seems meaningless (maybe you "
                     "want a size of one or #size-cells = 0 instead)"
                     .format(self._node))

            self.regs.append(reg)

        _add_names(node, "reg", self.regs)

    def _init_interrupts(self):
        # Initializes self.interrupts

        node = self._node

        self.interrupts = []

        for controller_node, data in _interrupts(node):
            interrupt = ControllerAndData()
            interrupt.node = self
            interrupt.controller = self.edt._node2enode[controller_node]
            interrupt.data = self._named_cells(interrupt.controller, data,
                                               "interrupt")

            self.interrupts.append(interrupt)

        _add_names(node, "interrupt", self.interrupts)

    def _standard_phandle_val_list(self, prop):
        # Parses a property like
        #
        #     <name>s = <phandle value phandle value ...>
        #     (e.g., pwms = <&foo 1 2 &bar 3 4>)
        #
        # , where each phandle points to a node that has a
        #
        #     #<name>-cells = <size>
        #
        # property that gives the number of cells in the value after the
        # phandle. These values are given names in *-cells in the binding for
        # the controller.
        #
        # Also parses any
        #
        #     <name>-names = "...", "...", ...
        #
        # Returns a list of ControllerAndData instances.

        if prop.name.endswith("gpios"):
            # There's some slight special-casing for *-gpios properties in that
            # e.g. foo-gpios still maps to #gpio-cells rather than
            # #foo-gpio-cells
            basename = "gpio"
        else:
            # Strip -s. We've already checked that the property names end in -s
            # in _check_binding().
            basename = prop.name[:-1]

        res = []

        for controller_node, data in _phandle_val_list(prop, basename):
            mapped_controller, mapped_data = \
                _map_phandle_array_entry(prop.node, controller_node, data,
                                         basename)

            entry = ControllerAndData()
            entry.node = self
            entry.controller = self.edt._node2enode[mapped_controller]
            entry.data = self._named_cells(entry.controller, mapped_data,
                                           basename)

            res.append(entry)

        _add_names(self._node, basename, res)

        return res

    def _named_cells(self, controller, data, basename):
        # Returns a dictionary that maps <basename>-cells names given in the
        # binding for 'controller' to cell values. 'data' is the raw data, as a
        # byte array.

        if not controller._binding:
            _err("{} controller {!r} for {!r} lacks binding"
                 .format(basename, controller._node, self._node))

        if basename + "-cells" in controller._binding:
            cell_names = controller._binding[basename + "-cells"]
        elif "#cells" in controller._binding:
            # Backwards compatibility
            cell_names = controller._binding["#cells"]
        else:
            # Treat no *-cells in the binding the same as an empty *-cells, so
            # that bindings don't have to have e.g. an empty 'clock-cells:' for
            # '#clock-cells = <0>'.
            cell_names = []

        data_list = to_nums(data)
        if len(data_list) != len(cell_names):
            _err("unexpected '{}-cells:' length in binding for {!r} - {} "
                 "instead of {}"
                 .format(basename, controller._node, len(cell_names),
                         len(data_list)))

        return dict(zip(cell_names, data_list))

    def _set_instance_no(self):
        # Initializes self.instance_no

        self.instance_no = {}

        for compat in self.compats:
            self.instance_no[compat] = 0
            for other_node in self.edt.nodes:
                if compat in other_node.compats and other_node.enabled:
                    self.instance_no[compat] += 1


class Register:
    """
    Represents a register on a node.

    These attributes are available on Register objects:

    node:
      The Node instance this register is from

    name:
      The name of the register as given in the 'reg-names' property, or None if
      there is no 'reg-names' property

    addr:
      The starting address of the register, in the parent address space. Any
      'ranges' properties are taken into account.

    size:
      The length of the register in bytes
    """
    def __repr__(self):
        fields = []

        if self.name is not None:
            fields.append("name: " + self.name)
        fields.append("addr: " + hex(self.addr))
        fields.append("size: " + hex(self.size))

        return "<Register, {}>".format(", ".join(fields))


class ControllerAndData:
    """
    Represents an entry in an 'interrupts' or 'type: phandle-array' property
    value, e.g. <&ctrl-1 4 0> in

        cs-gpios = <&ctrl-1 4 0 &ctrl-2 3 4>;

    These attributes are available on ControllerAndData objects:

    node:
      The Node instance the property appears on

    controller:
      The Node instance for the controller (e.g. the controller the interrupt
      gets sent to for interrupts)

    data:
      A dictionary that maps names from the *-cells key in the binding for the
      controller to data values, e.g. {"pin": 4, "flags": 0} for the example
      above.

      'interrupts = <1 2>' might give {"irq": 1, "level": 2}.

    name:
      The name of the entry as given in
      'interrupt-names'/'gpio-names'/'pwm-names'/etc., or None if there is no
      *-names property
    """
    def __repr__(self):
        fields = []

        if self.name is not None:
            fields.append("name: " + self.name)

        fields.append("controller: {}".format(self.controller))
        fields.append("data: {}".format(self.data))

        return "<ControllerAndData, {}>".format(", ".join(fields))


class Property:
    """
    Represents a property on a Node, as set in its DT node and with
    additional info from the 'properties:' section of the binding.

    Only properties mentioned in 'properties:' get created. Properties of type
    'compound' currently do not get Property instances, as I'm not sure what
    information to store for them.

    These attributes are available on Property objects:

    node:
      The Node instance the property is on

    name:
      The name of the property

    description:
      The description string from the property as given in the binding, or None
      if missing. Leading and trailing whitespace (including newlines) is
      removed.

    type:
      A string with the type of the property, as given in the binding.

    val:
      The value of the property, with the format determined by the 'type:' key
      from the binding.

        - For 'type: int/array/string/string-array', 'val' is what you'd expect
          (a Python integer or string, or a list of them)

        - For 'type: phandle', 'val' is the pointed-to Node instance

        - For 'type: phandles', 'val' is a list of the pointed-to Node
          instances

        - For 'type: phandle-array', 'val' is a list of ControllerAndData
          instances. See the documentation for that class.

    enum_index:
      The index of the property's value in the 'enum:' list in the binding, or
      None if the binding has no 'enum:'
    """
    def __repr__(self):
        fields = ["name: " + self.name,
                  # repr() to deal with lists
                  "type: " + self.type,
                  "value: " + repr(self.val)]

        if self.enum_index is not None:
            fields.append("enum index: {}".format(self.enum_index))

        return "<Property, {}>".format(", ".join(fields))


class EDTError(Exception):
    "Exception raised for devicetree- and binding-related errors"


#
# Public global functions
#


def spi_dev_cs_gpio(node):
    # Returns an SPI device's GPIO chip select if it exists, as a
    # ControllerAndData instance, and None otherwise. See
    # Documentation/devicetree/bindings/spi/spi-bus.txt in the Linux kernel.

    if not (node.bus == "spi" and node.parent and
            "cs-gpios" in node.parent.props):
        return None

    if not node.regs:
        _err("{!r} needs a 'reg' property, to look up the chip select index "
             "for SPI".format(node))

    parent_cs_lst = node.parent.props["cs-gpios"].val

    # cs-gpios is indexed by the unit address
    cs_index = node.regs[0].addr
    if cs_index >= len(parent_cs_lst):
        _err("index from 'regs' in {!r} ({}) is >= number of cs-gpios "
             "in {!r} ({})".format(
                 node, cs_index, node.parent, len(parent_cs_lst)))

    return parent_cs_lst[cs_index]


#
# Private global functions
#


def _dt_compats(dt):
    # Returns a set() with all 'compatible' strings in the devicetree
    # represented by dt (a dtlib.DT instance)

    return {compat
            for node in dt.node_iter()
                if "compatible" in node.props
                    for compat in node.props["compatible"].to_strings()}


def _binding_paths(bindings_dirs):
    # Returns a list with the paths to all bindings (.yaml files) in
    # 'bindings_dirs'

    binding_paths = []

    for bindings_dir in bindings_dirs:
        for root, _, filenames in os.walk(bindings_dir):
            for filename in filenames:
                if filename.endswith(".yaml"):
                    binding_paths.append(os.path.join(root, filename))

    return binding_paths


def _binding_bus(binding):
    # Returns the bus specified by 'parent-bus: ...' in the binding (or the
    # legacy 'parent: bus: ...'), or None if missing

    if not binding:
        return None

    if "parent-bus" in binding:
        return binding["parent-bus"]

    # Legacy key
    if "parent" in binding:
        # _check_binding() has checked that the "bus" key exists
        return binding["parent"]["bus"]

    return None


def _binding_inc_error(msg):
    # Helper for reporting errors in the !include implementation

    raise yaml.constructor.ConstructorError(None, None, "error: " + msg)


def _merge_props(to_dict, from_dict, parent, binding_path, check_required):
    # Recursively merges 'from_dict' into 'to_dict', to implement 'include:'.
    #
    # If 'from_dict' and 'to_dict' contain a 'required:' key for the same
    # property, then the values are ORed together.
    #
    # If 'check_required' is True, then an error is raised if 'from_dict' has
    # 'required: true' while 'to_dict' has 'required: false'. This prevents
    # bindings from "downgrading" requirements from bindings they include,
    # which might help keep bindings well-organized.
    #
    # It's an error for most other keys to appear in both 'from_dict' and
    # 'to_dict'. When it's not an error, the value in 'to_dict' takes
    # precedence.
    #
    # 'parent' is the name of the parent key containing 'to_dict' and
    # 'from_dict', and 'binding_path' is the path to the top-level binding.
    # These are used to generate errors for sketchy property overwrites.

    for prop in from_dict:
        if isinstance(to_dict.get(prop), dict) and \
           isinstance(from_dict[prop], dict):
            _merge_props(to_dict[prop], from_dict[prop], prop, binding_path,
                         check_required)
        elif prop not in to_dict:
            to_dict[prop] = from_dict[prop]
        elif _bad_overwrite(to_dict, from_dict, prop, check_required):
            _err("{} (in '{}'): '{}' from included file overwritten "
                 "('{}' replaced with '{}')".format(
                     binding_path, parent, prop, from_dict[prop],
                     to_dict[prop]))
        elif prop == "required":
            # Need a separate check here, because this code runs before
            # _check_binding()
            if not (isinstance(from_dict["required"], bool) and
                    isinstance(to_dict["required"], bool)):
                _err("malformed 'required:' setting for '{}' in 'properties' "
                     "in {}, expected true/false".format(parent, binding_path))

            # 'required: true' takes precedence
            to_dict["required"] = to_dict["required"] or from_dict["required"]
        elif prop == "category":
            # Legacy property key. 'category: required' takes precedence.
            if "required" in (to_dict["category"], from_dict["category"]):
                to_dict["category"] = "required"


def _bad_overwrite(to_dict, from_dict, prop, check_required):
    # _merge_props() helper. Returns True in cases where it's bad that
    # to_dict[prop] takes precedence over from_dict[prop].

    if to_dict[prop] == from_dict[prop]:
        return False

    # These are overridden deliberately
    if prop in {"title", "description", "compatible"}:
        return False

    if prop == "required":
        if not check_required:
            return False
        return from_dict[prop] and not to_dict[prop]

    # Legacy property key
    if prop == "category":
        if not check_required:
            return False
        return from_dict[prop] == "required" and to_dict[prop] == "optional"

    return True


def _binding_include(loader, node):
    # Implements !include, for backwards compatibility. '!include [foo, bar]'
    # just becomes [foo, bar].

    if isinstance(node, yaml.ScalarNode):
        # !include foo.yaml
        return [loader.construct_scalar(node)]

    if isinstance(node, yaml.SequenceNode):
        # !include [foo.yaml, bar.yaml]
        return loader.construct_sequence(node)

    _binding_inc_error("unrecognised node type in !include statement")


def _check_prop_type_and_default(prop_name, prop_type, required, default,
                                 binding_path):
    # _check_binding() helper. Checks 'type:' and 'default:' for the property
    # named 'prop_name'

    if prop_type is None:
        _err("missing 'type:' for '{}' in 'properties' in {}"
             .format(prop_name, binding_path))

    ok_types = {"boolean", "int", "array", "uint8-array", "string",
                "string-array", "phandle", "phandles", "phandle-array",
                "compound"}

    if prop_type not in ok_types:
        _err("'{}' in 'properties:' in {} has unknown type '{}', expected one "
             "of {}".format(prop_name, binding_path, prop_type,
                            ", ".join(ok_types)))

    if prop_type == "phandle-array" and not prop_name.endswith("s"):
        _err("'{}' in 'properties:' in {} is 'type: phandle-array', but its "
             "name does not end in -s. This is required since property names "
             "like '#pwm-cells' and 'pwm-names' get derived from 'pwms', for "
             "example.".format(prop_name, binding_path))

    # Check default

    if default is None:
        return

    if required:
        _err("'default:' for '{}' in 'properties:' in {} is meaningless in "
             "combination with 'required: true'"
             .format(prop_name, binding_path))

    if prop_type in {"boolean", "compound", "phandle", "phandles",
                     "phandle-array"}:
        _err("'default:' can't be combined with 'type: {}' for '{}' in "
             "'properties:' in {}".format(prop_type, prop_name, binding_path))

    def ok_default():
        # Returns True if 'default' is an okay default for the property's type

        if prop_type == "int" and isinstance(default, int) or \
           prop_type == "string" and isinstance(default, str):
            return True

        # array, uint8-array, or string-array

        if not isinstance(default, list):
            return False

        if prop_type == "array" and \
           all(isinstance(val, int) for val in default):
            return True

        if prop_type == "uint8-array" and \
           all(isinstance(val, int) and 0 <= val <= 255 for val in default):
            return True

        # string-array
        return all(isinstance(val, str) for val in default)

    if not ok_default():
        _err("'default: {}' is invalid for '{}' in 'properties:' in {}, which "
             "has type {}".format(default, prop_name, binding_path, prop_type))


def _translate(addr, node):
    # Recursively translates 'addr' on 'node' to the address space(s) of its
    # parent(s), by looking at 'ranges' properties. Returns the translated
    # address.
    #
    # node:
    #   dtlib.Node instance

    if not node.parent or "ranges" not in node.parent.props:
        # No translation
        return addr

    if not node.parent.props["ranges"].value:
        # DT spec.: "If the property is defined with an <empty> value, it
        # specifies that the parent and child address space is identical, and
        # no address translation is required."
        #
        # Treat this the same as a 'range' that explicitly does a one-to-one
        # mapping, as opposed to there not being any translation.
        return _translate(addr, node.parent)

    # Gives the size of each component in a translation 3-tuple in 'ranges'
    child_address_cells = _address_cells(node)
    parent_address_cells = _address_cells(node.parent)
    child_size_cells = _size_cells(node)

    # Number of cells for one translation 3-tuple in 'ranges'
    entry_cells = child_address_cells + parent_address_cells + child_size_cells

    for raw_range in _slice(node.parent, "ranges", 4*entry_cells):
        child_addr = to_num(raw_range[:4*child_address_cells])
        raw_range = raw_range[4*child_address_cells:]

        parent_addr = to_num(raw_range[:4*parent_address_cells])
        raw_range = raw_range[4*parent_address_cells:]

        child_len = to_num(raw_range)

        if child_addr <= addr < child_addr + child_len:
            # 'addr' is within range of a translation in 'ranges'. Recursively
            # translate it and return the result.
            return _translate(parent_addr + addr - child_addr, node.parent)

    # 'addr' is not within range of any translation in 'ranges'
    return addr


def _add_names(node, names_ident, objs):
    # Helper for registering names from <foo>-names properties.
    #
    # node:
    #   edtlib.Node instance
    #
    # names-ident:
    #   The <foo> part of <foo>-names, e.g. "reg" for "reg-names"
    #
    # objs:
    #   list of objects whose .name field should be set

    full_names_ident = names_ident + "-names"

    if full_names_ident in node.props:
        names = node.props[full_names_ident].to_strings()
        if len(names) != len(objs):
            _err("{} property in {} has {} strings, expected {} strings"
                 .format(full_names_ident, node.name, len(names), len(objs)))

        for obj, name in zip(objs, names):
            obj.name = name
    else:
        for obj in objs:
            obj.name = None


def _interrupt_parent(node):
    # Returns the node pointed at by the closest 'interrupt-parent', searching
    # the parents of 'node'. As of writing, this behavior isn't specified in
    # the DT spec., but seems to match what some .dts files except.

    while node:
        if "interrupt-parent" in node.props:
            return node.props["interrupt-parent"].to_node()
        node = node.parent

    _err("{!r} has an 'interrupts' property, but neither the node nor any "
         "of its parents has an 'interrupt-parent' property".format(node))


def _interrupts(node):
    # Returns a list of (<controller>, <data>) tuples, with one tuple per
    # interrupt generated by 'node'. <controller> is the destination of the
    # interrupt (possibly after mapping through an 'interrupt-map'), and <data>
    # the data associated with the interrupt (as a 'bytes' object).

    # Takes precedence over 'interrupts' if both are present
    if "interrupts-extended" in node.props:
        prop = node.props["interrupts-extended"]
        return [_map_interrupt(node, iparent, spec)
                for iparent, spec in _phandle_val_list(prop, "interrupt")]

    if "interrupts" in node.props:
        # Treat 'interrupts' as a special case of 'interrupts-extended', with
        # the same interrupt parent for all interrupts

        iparent = _interrupt_parent(node)
        interrupt_cells = _interrupt_cells(iparent)

        return [_map_interrupt(node, iparent, raw)
                for raw in _slice(node, "interrupts", 4*interrupt_cells)]

    return []


def _map_interrupt(child, parent, child_spec):
    # Translates an interrupt headed from 'child' to 'parent' with data
    # 'child_spec' through any 'interrupt-map' properties. Returns a
    # (<controller>, <data>) tuple with the final destination after mapping.

    if "interrupt-controller" in parent.props:
        return (parent, child_spec)

    def own_address_cells(node):
        # Used for parents pointed at by 'interrupt-map'. We can't use
        # _address_cells(), because it's the #address-cells property on 'node'
        # itself that matters.

        address_cells = node.props.get("#address-cells")
        if not address_cells:
            _err("missing #address-cells on {!r} (while handling interrupt-map)"
                 .format(node))
        return address_cells.to_num()

    def spec_len_fn(node):
        # Can't use _address_cells() here, because it's the #address-cells
        # property on 'node' itself that matters
        return own_address_cells(node) + _interrupt_cells(node)

    parent, raw_spec = _map(
        "interrupt", child, parent, _raw_unit_addr(child) + child_spec,
        spec_len_fn, require_controller=True)

    # Strip the parent unit address part, if any
    return (parent, raw_spec[4*own_address_cells(parent):])


def _map_phandle_array_entry(child, parent, child_spec, basename):
    # Returns a (<controller>, <data>) tuple with the final destination after
    # mapping through any '<basename>-map' (e.g. gpio-map) properties. See
    # _map_interrupt().

    def spec_len_fn(node):
        prop_name = "#{}-cells".format(basename)
        if prop_name not in node.props:
            _err("expected '{}' property on {!r} (referenced by {!r})"
                 .format(prop_name, node, child))
        return node.props[prop_name].to_num()

    # Do not require <prefix>-controller for anything but interrupts for now
    return _map(basename, child, parent, child_spec, spec_len_fn,
                require_controller=False)


def _map(prefix, child, parent, child_spec, spec_len_fn, require_controller):
    # Common code for mapping through <prefix>-map properties, e.g.
    # interrupt-map and gpio-map.
    #
    # prefix:
    #   The prefix, e.g. "interrupt" or "gpio"
    #
    # child:
    #   The "sender", e.g. the node with 'interrupts = <...>'
    #
    # parent:
    #   The "receiver", e.g. a node with 'interrupt-map = <...>' or
    #   'interrupt-controller' (no mapping)
    #
    # child_spec:
    #   The data associated with the interrupt/GPIO/etc., as a 'bytes' object,
    #   e.g. <1 2> for 'foo-gpios = <&gpio1 1 2>'.
    #
    # spec_len_fn:
    #   Function called on a parent specified in a *-map property to get the
    #   length of the parent specifier (data after phandle in *-map), in cells
    #
    # require_controller:
    #   If True, the final controller node after mapping is required to have
    #   to have a <prefix>-controller property.

    map_prop = parent.props.get(prefix + "-map")
    if not map_prop:
        if require_controller and prefix + "-controller" not in parent.props:
            _err("expected '{}-controller' property on {!r} "
                 "(referenced by {!r})".format(prefix, parent, child))

        # No mapping
        return (parent, child_spec)

    masked_child_spec = _mask(prefix, child, parent, child_spec)

    raw = map_prop.value
    while raw:
        if len(raw) < len(child_spec):
            _err("bad value for {!r}, missing/truncated child data"
                 .format(map_prop))
        child_spec_entry = raw[:len(child_spec)]
        raw = raw[len(child_spec):]

        if len(raw) < 4:
            _err("bad value for {!r}, missing/truncated phandle"
                 .format(map_prop))
        phandle = to_num(raw[:4])
        raw = raw[4:]

        # Parent specified in *-map
        map_parent = parent.dt.phandle2node.get(phandle)
        if not map_parent:
            _err("bad phandle ({}) in {!r}".format(phandle, map_prop))

        map_parent_spec_len = 4*spec_len_fn(map_parent)
        if len(raw) < map_parent_spec_len:
            _err("bad value for {!r}, missing/truncated parent data"
                 .format(map_prop))
        parent_spec = raw[:map_parent_spec_len]
        raw = raw[map_parent_spec_len:]

        # Got one *-map row. Check if it matches the child data.
        if child_spec_entry == masked_child_spec:
            # Handle *-map-pass-thru
            parent_spec = _pass_thru(
                prefix, child, parent, child_spec, parent_spec)

            # Found match. Recursively map and return it.
            return _map(prefix, parent, map_parent, parent_spec, spec_len_fn,
                        require_controller)

    _err("child specifier for {!r} ({}) does not appear in {!r}"
         .format(child, child_spec, map_prop))


def _mask(prefix, child, parent, child_spec):
    # Common code for handling <prefix>-mask properties, e.g. interrupt-mask.
    # See _map() for the parameters.

    mask_prop = parent.props.get(prefix + "-map-mask")
    if not mask_prop:
        # No mask
        return child_spec

    mask = mask_prop.value
    if len(mask) != len(child_spec):
        _err("{!r}: expected '{}-mask' in {!r} to be {} bytes, is {} bytes"
             .format(child, prefix, parent, len(child_spec), len(mask)))

    return _and(child_spec, mask)


def _pass_thru(prefix, child, parent, child_spec, parent_spec):
    # Common code for handling <prefix>-map-thru properties, e.g.
    # interrupt-pass-thru.
    #
    # parent_spec:
    #   The parent data from the matched entry in the <prefix>-map property
    #
    # See _map() for the other parameters.

    pass_thru_prop = parent.props.get(prefix + "-map-pass-thru")
    if not pass_thru_prop:
        # No pass-thru
        return parent_spec

    pass_thru = pass_thru_prop.value
    if len(pass_thru) != len(child_spec):
        _err("{!r}: expected '{}-map-pass-thru' in {!r} to be {} bytes, is {} bytes"
             .format(child, prefix, parent, len(child_spec), len(pass_thru)))

    res = _or(_and(child_spec, pass_thru),
              _and(parent_spec, _not(pass_thru)))

    # Truncate to length of parent spec.
    return res[-len(parent_spec):]


def _raw_unit_addr(node):
    # _map_interrupt() helper. Returns the unit address (derived from 'reg' and
    # #address-cells) as a raw 'bytes'

    if 'reg' not in node.props:
        _err("{!r} lacks 'reg' property (needed for 'interrupt-map' unit "
             "address lookup)".format(node))

    addr_len = 4*_address_cells(node)

    if len(node.props['reg'].value) < addr_len:
        _err("{!r} has too short 'reg' property (while doing 'interrupt-map' "
             "unit address lookup)".format(node))

    return node.props['reg'].value[:addr_len]


def _and(b1, b2):
    # Returns the bitwise AND of the two 'bytes' objects b1 and b2. Pads
    # with ones on the left if the lengths are not equal.

    # Pad on the left, to equal length
    maxlen = max(len(b1), len(b2))
    return bytes(x & y for x, y in zip(b1.rjust(maxlen, b'\xff'),
                                       b2.rjust(maxlen, b'\xff')))


def _or(b1, b2):
    # Returns the bitwise OR of the two 'bytes' objects b1 and b2. Pads with
    # zeros on the left if the lengths are not equal.

    # Pad on the left, to equal length
    maxlen = max(len(b1), len(b2))
    return bytes(x | y for x, y in zip(b1.rjust(maxlen, b'\x00'),
                                       b2.rjust(maxlen, b'\x00')))


def _not(b):
    # Returns the bitwise not of the 'bytes' object 'b'

    # ANDing with 0xFF avoids negative numbers
    return bytes(~x & 0xFF for x in b)


def _phandle_val_list(prop, n_cells_name):
    # Parses a '<phandle> <value> <phandle> <value> ...' value. The number of
    # cells that make up each <value> is derived from the node pointed at by
    # the preceding <phandle>.
    #
    # prop:
    #   dtlib.Property with value to parse
    #
    # n_cells_name:
    #   The <name> part of the #<name>-cells property to look for on the nodes
    #   the phandles point to, e.g. "gpio" for #gpio-cells.
    #
    # Returns a list of (<node>, <value>) tuples, where <node> is the node
    # pointed at by <phandle>.

    full_n_cells_name = "#{}-cells".format(n_cells_name)

    res = []

    raw = prop.value
    while raw:
        if len(raw) < 4:
            # Not enough room for phandle
            _err("bad value for " + repr(prop))
        phandle = to_num(raw[:4])
        raw = raw[4:]

        node = prop.node.dt.phandle2node.get(phandle)
        if not node:
            _err("bad phandle in " + repr(prop))

        if full_n_cells_name not in node.props:
            _err("{!r} lacks {}".format(node, full_n_cells_name))

        n_cells = node.props[full_n_cells_name].to_num()
        if len(raw) < 4*n_cells:
            _err("missing data after phandle in " + repr(prop))

        res.append((node, raw[:4*n_cells]))
        raw = raw[4*n_cells:]

    return res


def _address_cells(node):
    # Returns the #address-cells setting for 'node', giving the number of <u32>
    # cells used to encode the address in the 'reg' property

    if "#address-cells" in node.parent.props:
        return node.parent.props["#address-cells"].to_num()
    return 2  # Default value per DT spec.


def _size_cells(node):
    # Returns the #size-cells setting for 'node', giving the number of <u32>
    # cells used to encode the size in the 'reg' property

    if "#size-cells" in node.parent.props:
        return node.parent.props["#size-cells"].to_num()
    return 1  # Default value per DT spec.


def _interrupt_cells(node):
    # Returns the #interrupt-cells property value on 'node', erroring out if
    # 'node' has no #interrupt-cells property

    if "#interrupt-cells" not in node.props:
        _err("{!r} lacks #interrupt-cells".format(node))
    return node.props["#interrupt-cells"].to_num()


def _slice(node, prop_name, size):
    # Splits node.props[prop_name].value into 'size'-sized chunks, returning a
    # list of chunks. Raises EDTError if the length of the property is not
    # evenly divisible by 'size'.

    raw = node.props[prop_name].value
    if len(raw) % size:
        _err("'{}' property in {!r} has length {}, which is not evenly "
             "divisible by {}".format(prop_name, node, len(raw), size))

    return [raw[i:i + size] for i in range(0, len(raw), size)]


def _check_dt(dt):
    # Does devicetree sanity checks. dtlib is meant to be general and
    # anything-goes except for very special properties like phandle, but in
    # edtlib we can be pickier.

    # Check that 'status' has one of the values given in the devicetree spec.

    ok_status = {"okay", "disabled", "reserved", "fail", "fail-sss"}

    for node in dt.node_iter():
        if "status" in node.props:
            try:
                status_val = node.props["status"].to_string()
            except DTError as e:
                # The error message gives the path
                _err(str(e))

            if status_val not in ok_status:
                _err("unknown 'status' value \"{}\" in {} in {}, expected one "
                     "of {} (see the devicetree specification)"
                     .format(status_val, node.path, node.dt.filename,
                             ", ".join(ok_status)))

        ranges_prop = node.props.get("ranges")
        if ranges_prop:
            if ranges_prop.type not in (TYPE_EMPTY, TYPE_NUMS):
                _err("expected 'ranges = < ... >;' in {} in {}, not '{}' "
                     "(see the devicetree specification)"
                     .format(node.path, node.dt.filename, ranges_prop))


def _err(msg):
    raise EDTError(msg)
