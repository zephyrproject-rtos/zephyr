# Copyright (c) 2019 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# SPDX-License-Identifier: BSD-3-Clause

# Tip: You can view just the documentation with 'pydoc3 devicetree.edtlib'

"""
Library for working with devicetrees at a higher level compared to dtlib. Like
dtlib, this library presents a tree of devicetree nodes, but the nodes are
augmented with information from bindings and include some interpretation of
properties. Some of this interpretation is based on conventions established
by the Linux kernel, so the Documentation/devicetree/bindings in the Linux
source code is sometimes good reference material.

Bindings are YAML files that describe devicetree nodes. Devicetree
nodes are usually mapped to bindings via their 'compatible = "..."' property,
but a binding can also come from a 'child-binding:' key in the binding for the
parent devicetree node.

Each devicetree node (dtlib.Node) gets a corresponding edtlib.Node instance,
which has all the information related to the node.

The top-level entry points for the library are the EDT and Binding classes.
See their constructor docstrings for details. There is also a
bindings_from_paths() helper function.
"""

# NOTE: tests/test_edtlib.py is the test suite for this library.

# Implementation notes
# --------------------
#
# A '_' prefix on an identifier in Python is a convention for marking it private.
# Please do not access private things. Instead, think of what API you need, and
# add it.
#
# This module is not meant to have any global state. It should be possible to
# create several EDT objects with independent binding paths and flags. If you
# need to add a configuration parameter or the like, store it in the EDT
# instance, and initialize it e.g. with a constructor argument.
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

from collections import defaultdict
from copy import deepcopy
from dataclasses import dataclass
from typing import Any, Callable, Dict, Iterable, List, NoReturn, \
    Optional, Set, TYPE_CHECKING, Tuple, Union
import logging
import os
import re

import yaml
try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    # This makes e.g. gen_defines.py more than twice as fast.
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader     # type: ignore

from devicetree.dtlib import DT, DTError, to_num, to_nums, Type
from devicetree.dtlib import Node as dtlib_Node
from devicetree.dtlib import Property as dtlib_Property
from devicetree.grutils import Graph
from devicetree._private import _slice_helper

#
# Public classes
#


class Binding:
    """
    Represents a parsed binding.

    These attributes are available on Binding objects:

    path:
      The absolute path to the file defining the binding.

    description:
      The free-form description of the binding, or None.

    compatible:
      The compatible string the binding matches.

      This may be None. For example, it's None when the Binding is inferred
      from node properties. It can also be None for Binding objects created
      using 'child-binding:' with no compatible.

    prop2specs:
      A dict mapping property names to PropertySpec objects
      describing those properties' values.

    specifier2cells:
      A dict that maps specifier space names (like "gpio",
      "clock", "pwm", etc.) to lists of cell names.

      For example, if the binding YAML contains 'pin' and 'flags' cell names
      for the 'gpio' specifier space, like this:

          gpio-cells:
          - pin
          - flags

      Then the Binding object will have a 'specifier2cells' attribute mapping
      "gpio" to ["pin", "flags"]. A missing key should be interpreted as zero
      cells.

    raw:
      The binding as an object parsed from YAML.

    bus:
      If nodes with this binding's 'compatible' describe a bus, a string
      describing the bus type (like "i2c") or a list describing supported
      protocols (like ["i3c", "i2c"]). None otherwise.

      Note that this is the raw value from the binding where it can be
      a string or a list. Use "buses" instead unless you need the raw
      value, where "buses" is always a list.

    buses:
      Deprived property from 'bus' where 'buses' is a list of bus(es),
      for example, ["i2c"] or ["i3c", "i2c"]. Or an empty list if there is
      no 'bus:' in this binding.

    on_bus:
      If nodes with this binding's 'compatible' appear on a bus, a string
      describing the bus type (like "i2c"). None otherwise.

    child_binding:
      If this binding describes the properties of child nodes, then
      this is a Binding object for those children; it is None otherwise.
      A Binding object's 'child_binding.child_binding' is not None if there
      are multiple levels of 'child-binding' descriptions in the binding.
    """

    def __init__(self, path: Optional[str], fname2path: Dict[str, str],
                 raw: Any = None, require_compatible: bool = True,
                 require_description: bool = True,
                 inc_allowlist: Optional[List[str]] = None,
                 inc_blocklist: Optional[List[str]] = None):
        """
        Binding constructor.

        path:
          Path to binding YAML file. May be None.

        fname2path:
          Map from include files to their absolute paths. Must
          not be None, but may be empty.

        raw:
          Optional raw content in the binding.
          This does not have to have any "include:" lines resolved.
          May be left out, in which case 'path' is opened and read.
          This can be used to resolve child bindings, for example.

        require_compatible:
          If True, it is an error if the binding does not contain a
          "compatible:" line. If False, a missing "compatible:" is
          not an error. Either way, "compatible:" must be a string
          if it is present in the binding.

        require_description:
          If True, it is an error if the binding does not contain a
          "description:" line. If False, a missing "description:" is
          not an error. Either way, "description:" must be a string
          if it is present in the binding.

        inc_allowlist:
          The property-allowlist filter set by including bindings.

        inc_blocklist:
          The property-blocklist filter set by including bindings.
        """
        self.path: Optional[str] = path
        self._fname2path: Dict[str, str] = fname2path

        self._inc_allowlist: Optional[List[str]] = inc_allowlist
        self._inc_blocklist: Optional[List[str]] = inc_blocklist

        if raw is None:
            if path is None:
                _err("you must provide either a 'path' or a 'raw' argument")
            with open(path, encoding="utf-8") as f:
                raw = yaml.load(f, Loader=_BindingLoader)

        # Get the properties this binding modifies
        # before we merge the included ones.
        last_modified_props = list(raw.get("properties", {}).keys())

        # Map property names to their specifications:
        # - first, _merge_includes() will recursively populate prop2specs with
        #   the properties specified by the included bindings
        # - eventually, we'll update prop2specs with the properties
        #   this binding itself defines or modifies
        self.prop2specs: Dict[str, 'PropertySpec'] = {}

        # Merge any included files into self.raw. This also pulls in
        # inherited child binding definitions, so it has to be done
        # before initializing those.
        self.raw: dict = self._merge_includes(raw, self.path)

        # Recursively initialize any child bindings. These don't
        # require a 'compatible' or 'description' to be well defined,
        # but they must be dicts.
        if "child-binding" in raw:
            if not isinstance(raw["child-binding"], dict):
                _err(f"malformed 'child-binding:' in {self.path}, "
                     "expected a binding (dictionary with keys/values)")
            self.child_binding: Optional['Binding'] = Binding(
                path, fname2path,
                raw=raw["child-binding"],
                require_compatible=False,
                require_description=False)
        else:
            self.child_binding = None

        # Make sure this is a well defined object.
        self._check(require_compatible, require_description)

        # Update specs with the properties this binding defines or modifies.
        for prop_name in last_modified_props:
            self.prop2specs[prop_name] = PropertySpec(prop_name, self)

        # Initialize look up tables.
        self.specifier2cells: Dict[str, List[str]] = {}
        for key, val in self.raw.items():
            if key.endswith("-cells"):
                self.specifier2cells[key[:-len("-cells")]] = val

    def __repr__(self) -> str:
        if self.compatible:
            compat = f" for compatible '{self.compatible}'"
        else:
            compat = ""
        basename = os.path.basename(self.path or "")
        return f"<Binding {basename}" + compat + ">"

    @property
    def description(self) -> Optional[str]:
        "See the class docstring"
        return self.raw.get('description')

    @property
    def compatible(self) -> Optional[str]:
        "See the class docstring"
        return self.raw.get('compatible')

    @property
    def bus(self) -> Union[None, str, List[str]]:
        "See the class docstring"
        return self.raw.get('bus')

    @property
    def buses(self) -> List[str]:
        "See the class docstring"
        if self.raw.get('bus') is not None:
            return self._buses
        else:
            return []

    @property
    def on_bus(self) -> Optional[str]:
        "See the class docstring"
        return self.raw.get('on-bus')

    def _merge_includes(self, raw: dict, binding_path: Optional[str]) -> dict:
        # Constructor helper. Merges included files in
        # 'raw["include"]' into 'raw' using 'self._include_paths' as a
        # source of include files, removing the "include" key while
        # doing so.
        #
        # This treats 'binding_path' as the binding file being built up
        # and uses it for error messages.

        if "include" not in raw:
            return raw

        include = raw.pop("include")

        # First, merge the included files together. If more than one included
        # file has a 'required:' for a particular property, OR the values
        # together, so that 'required: true' wins.

        merged: Dict[str, Any] = {}

        if isinstance(include, str):
            # Simple scalar string case
            # Load YAML file and register property specs into prop2specs.
            inc_raw = self._load_raw(include, self._inc_allowlist,
                                     self._inc_blocklist)

            _merge_props(merged, inc_raw, None, binding_path,  False)
        elif isinstance(include, list):
            # List of strings and maps. These types may be intermixed.
            for elem in include:
                if isinstance(elem, str):
                    # Load YAML file and register property specs into prop2specs.
                    inc_raw = self._load_raw(elem, self._inc_allowlist,
                                             self._inc_blocklist)

                    _merge_props(merged, inc_raw, None, binding_path, False)
                elif isinstance(elem, dict):
                    name = elem.pop('name', None)

                    # Merge this include property-allowlist filter
                    # with filters from including bindings.
                    allowlist = elem.pop('property-allowlist', None)
                    if allowlist is not None:
                        if self._inc_allowlist:
                            allowlist.extend(self._inc_allowlist)
                    else:
                        allowlist = self._inc_allowlist

                    # Merge this include property-blocklist filter
                    # with filters from including bindings.
                    blocklist = elem.pop('property-blocklist', None)
                    if blocklist is not None:
                        if self._inc_blocklist:
                            blocklist.extend(self._inc_blocklist)
                    else:
                        blocklist = self._inc_blocklist

                    child_filter = elem.pop('child-binding', None)

                    if elem:
                        # We've popped out all the valid keys.
                        _err(f"'include:' in {binding_path} should not have "
                             f"these unexpected contents: {elem}")

                    _check_include_dict(name, allowlist, blocklist,
                                        child_filter, binding_path)

                    # Load YAML file, and register (filtered) property specs
                    # into prop2specs.
                    contents = self._load_raw(name,
                                              allowlist, blocklist,
                                              child_filter)

                    _merge_props(merged, contents, None, binding_path, False)
                else:
                    _err(f"all elements in 'include:' in {binding_path} "
                         "should be either strings or maps with a 'name' key "
                         "and optional 'property-allowlist' or "
                         f"'property-blocklist' keys, but got: {elem}")
        else:
            # Invalid item.
            _err(f"'include:' in {binding_path} "
                 f"should be a string or list, but has type {type(include)}")

        # Next, merge the merged included files into 'raw'. Error out if
        # 'raw' has 'required: false' while the merged included files have
        # 'required: true'.

        _merge_props(raw, merged, None, binding_path, check_required=True)

        return raw


    def _load_raw(self, fname: str,
                  allowlist: Optional[List[str]] = None,
                  blocklist: Optional[List[str]] = None,
                  child_filter: Optional[dict] = None) -> dict:
        # Returns the contents of the binding given by 'fname' after merging
        # any bindings it lists in 'include:' into it, according to the given
        # property filters.
        #
        # Will also register the (filtered) included property specs
        # into prop2specs.

        path = self._fname2path.get(fname)

        if not path:
            _err(f"'{fname}' not found")

        with open(path, encoding="utf-8") as f:
            contents = yaml.load(f, Loader=_BindingLoader)
            if not isinstance(contents, dict):
                _err(f'{path}: invalid contents, expected a mapping')

        # Apply constraints to included YAML contents.
        _filter_properties(contents,
                           allowlist, blocklist,
                           child_filter, self.path)

        # Register included property specs.
        self._add_included_prop2specs(fname, contents, allowlist, blocklist)

        return self._merge_includes(contents, path)

    def _add_included_prop2specs(self, fname: str, contents: dict,
                                 allowlist: Optional[List[str]] = None,
                                 blocklist: Optional[List[str]] = None) -> None:
        # Registers the properties specified by an included binding file
        # into the properties this binding supports/requires (aka prop2specs).
        #
        # Consider "this" binding B includes I1 which itself includes I2.
        #
        # We assume to be called in that order:
        # 1) _add_included_prop2spec(B, I1)
        # 2) _add_included_prop2spec(B, I2)
        #
        # Where we don't want I2 "taking ownership" for properties
        # modified by I1.
        #
        # So we:
        # - first create a binding that represents the included file
        # - then add the property specs defined by this binding to prop2specs,
        #   without overriding the specs modified by an including binding
        #
        # Note: Unfortunately, we can't cache these base bindings,
        # as a same YAML file may be included with different filters
        # (property-allowlist and such), leading to different contents.

        inc_binding = Binding(
            self._fname2path[fname],
            self._fname2path,
            contents,
            require_compatible=False,
            require_description=False,
            # Recursively pass filters to included bindings.
            inc_allowlist=allowlist,
            inc_blocklist=blocklist,
        )

        for prop, spec in inc_binding.prop2specs.items():
            if prop not in self.prop2specs:
                self.prop2specs[prop] = spec

    def _check(self, require_compatible: bool, require_description: bool):
        # Does sanity checking on the binding.

        raw = self.raw

        if "compatible" in raw:
            compatible = raw["compatible"]
            if not isinstance(compatible, str):
                _err(f"malformed 'compatible: {compatible}' "
                     f"field in {self.path} - "
                     f"should be a string, not {type(compatible).__name__}")
        elif require_compatible:
            _err(f"missing 'compatible' in {self.path}")

        if "description" in raw:
            description = raw["description"]
            if not isinstance(description, str) or not description:
                _err(f"malformed or empty 'description' in {self.path}")
        elif require_description:
            _err(f"missing 'description' in {self.path}")

        # Allowed top-level keys. The 'include' key should have been
        # removed by _load_raw() already.
        ok_top = {"description", "compatible", "bus", "on-bus",
                  "properties", "child-binding"}

        # Descriptive errors for legacy bindings.
        legacy_errors = {
            "#cells": "expected *-cells syntax",
            "child": "use 'bus: <bus>' instead",
            "child-bus": "use 'bus: <bus>' instead",
            "parent": "use 'on-bus: <bus>' instead",
            "parent-bus": "use 'on-bus: <bus>' instead",
            "sub-node": "use 'child-binding' instead",
            "title": "use 'description' instead",
        }

        for key in raw:
            if key in legacy_errors:
                _err(f"legacy '{key}:' in {self.path}, {legacy_errors[key]}")

            if key not in ok_top and not key.endswith("-cells"):
                _err(f"unknown key '{key}' in {self.path}, "
                     "expected one of {', '.join(ok_top)}, or *-cells")

        if "bus" in raw:
            bus = raw["bus"]
            if not isinstance(bus, str) and \
               (not isinstance(bus, list) and \
                not all(isinstance(elem, str) for elem in bus)):
                _err(f"malformed 'bus:' value in {self.path}, "
                     "expected string or list of strings")

            if isinstance(bus, list):
                self._buses = bus
            else:
                # Convert bus into a list
                self._buses = [bus]

        if "on-bus" in raw and \
           not isinstance(raw["on-bus"], str):
            _err(f"malformed 'on-bus:' value in {self.path}, "
                 "expected string")

        self._check_properties()

        for key, val in raw.items():
            if key.endswith("-cells"):
                if not isinstance(val, list) or \
                   not all(isinstance(elem, str) for elem in val):
                    _err(f"malformed '{key}:' in {self.path}, "
                         "expected a list of strings")

    def _check_properties(self) -> None:
        # _check() helper for checking the contents of 'properties:'.

        raw = self.raw

        if "properties" not in raw:
            return

        ok_prop_keys = {"description", "type", "required",
                        "enum", "const", "default", "deprecated",
                        "specifier-space"}

        for prop_name, options in raw["properties"].items():
            for key in options:
                if key not in ok_prop_keys:
                    _err(f"unknown setting '{key}' in "
                         f"'properties: {prop_name}: ...' in {self.path}, "
                         f"expected one of {', '.join(ok_prop_keys)}")

            _check_prop_by_type(prop_name, options, self.path)

            for true_false_opt in ["required", "deprecated"]:
                if true_false_opt in options:
                    option = options[true_false_opt]
                    if not isinstance(option, bool):
                        _err(f"malformed '{true_false_opt}:' setting '{option}' "
                             f"for '{prop_name}' in 'properties' in {self.path}, "
                             "expected true/false")

            if options.get("deprecated") and options.get("required"):
                _err(f"'{prop_name}' in 'properties' in {self.path} should not "
                      "have both 'deprecated' and 'required' set")

            if "description" in options and \
               not isinstance(options["description"], str):
                _err("missing, malformed, or empty 'description' for "
                     f"'{prop_name}' in 'properties' in {self.path}")

            if "enum" in options and not isinstance(options["enum"], list):
                _err(f"enum in {self.path} for property '{prop_name}' "
                     "is not a list")


class PropertySpec:
    """
    Represents a "property specification", i.e. the description of a
    property provided by a binding file, like its type and description.

    These attributes are available on PropertySpec objects:

    binding:
      The Binding object which defined this property.

    name:
      The property's name.

    path:
      The file where this property was defined. In case a binding includes
      other bindings, this is the file where the property was last modified.

    type:
      The type of the property as a string, as given in the binding.

    description:
      The free-form description of the property as a string, or None.

    enum:
      A list of values the property may take as given in the binding, or None.

    enum_tokenizable:
      True if enum is not None and all the values in it are tokenizable;
      False otherwise.

      A property must have string type and an "enum:" in its binding to be
      tokenizable. Additionally, the "enum:" values must be unique after
      converting all non-alphanumeric characters to underscores (so "foo bar"
      and "foo_bar" in the same "enum:" would not be tokenizable).

    enum_upper_tokenizable:
      Like 'enum_tokenizable', with the additional restriction that the
      "enum:" values must be unique after uppercasing and converting
      non-alphanumeric characters to underscores.

    const:
      The property's constant value as given in the binding, or None.

    default:
      The property's default value as given in the binding, or None.

    deprecated:
      True if the property is deprecated; False otherwise.

    required:
      True if the property is marked required; False otherwise.

    specifier_space:
      The specifier space for the property as given in the binding, or None.
    """

    def __init__(self, name: str, binding: Binding):
        self.binding: Binding = binding
        self.name: str = name
        self._raw: Dict[str, Any] = self.binding.raw["properties"][name]

    def __repr__(self) -> str:
        return f"<PropertySpec {self.name} type '{self.type}'>"

    @property
    def path(self) -> Optional[str]:
        "See the class docstring"
        return self.binding.path

    @property
    def type(self) -> str:
        "See the class docstring"
        return self._raw["type"]

    @property
    def description(self) -> Optional[str]:
        "See the class docstring"
        return self._raw.get("description")

    @property
    def enum(self) -> Optional[list]:
        "See the class docstring"
        return self._raw.get("enum")

    @property
    def enum_tokenizable(self) -> bool:
        "See the class docstring"
        if not hasattr(self, '_enum_tokenizable'):
            if self.type != 'string' or self.enum is None:
                self._enum_tokenizable = False
            else:
                # Saving _as_tokens here lets us reuse it in
                # enum_upper_tokenizable.
                self._as_tokens = [re.sub(_NOT_ALPHANUM_OR_UNDERSCORE,
                                          '_', value)
                                   for value in self.enum]
                self._enum_tokenizable = (len(self._as_tokens) ==
                                          len(set(self._as_tokens)))

        return self._enum_tokenizable

    @property
    def enum_upper_tokenizable(self) -> bool:
        "See the class docstring"
        if not hasattr(self, '_enum_upper_tokenizable'):
            if not self.enum_tokenizable:
                self._enum_upper_tokenizable = False
            else:
                self._enum_upper_tokenizable = \
                    (len(self._as_tokens) ==
                     len(set(x.upper() for x in self._as_tokens)))
        return self._enum_upper_tokenizable

    @property
    def const(self) -> Union[None, int, List[int], str, List[str]]:
        "See the class docstring"
        return self._raw.get("const")

    @property
    def default(self) -> Union[None, int, List[int], str, List[str]]:
        "See the class docstring"
        return self._raw.get("default")

    @property
    def required(self) -> bool:
        "See the class docstring"
        return self._raw.get("required", False)

    @property
    def deprecated(self) -> bool:
        "See the class docstring"
        return self._raw.get("deprecated", False)

    @property
    def specifier_space(self) -> Optional[str]:
        "See the class docstring"
        return self._raw.get("specifier-space")

PropertyValType = Union[int, str,
                        List[int], List[str],
                        'Node', List['Node'],
                        List[Optional['ControllerAndData']],
                        bytes, None]


@dataclass
class Property:
    """
    Represents a property on a Node, as set in its DT node and with
    additional info from the 'properties:' section of the binding.

    Only properties mentioned in 'properties:' get created. Properties of type
    'compound' currently do not get Property instances, as it's not clear
    what to generate for them.

    These attributes are available on Property objects. Several are
    just convenience accessors for attributes on the PropertySpec object
    accessible via the 'spec' attribute.

    These attributes are available on Property objects:

    spec:
      The PropertySpec object which specifies this property.

    val:
      The value of the property, with the format determined by spec.type,
      which comes from the 'type:' string in the binding.

        - For 'type: int/array/string/string-array', 'val' is what you'd expect
          (a Python integer or string, or a list of them)

        - For 'type: phandle' and 'type: path', 'val' is the pointed-to Node
          instance

        - For 'type: phandles', 'val' is a list of the pointed-to Node
          instances

        - For 'type: phandle-array', 'val' is a list of ControllerAndData
          instances. See the documentation for that class.

    node:
      The Node instance the property is on

    name:
      Convenience for spec.name.

    description:
      Convenience for spec.description with leading and trailing whitespace
      (including newlines) removed. May be None.

    type:
      Convenience for spec.type.

    val_as_token:
      The value of the property as a token, i.e. with non-alphanumeric
      characters replaced with underscores. This is only safe to access
      if 'spec.enum_tokenizable' returns True.

    enum_index:
      The index of 'val' in 'spec.enum' (which comes from the 'enum:' list
      in the binding), or None if spec.enum is None.
    """

    spec: PropertySpec
    val: PropertyValType
    node: 'Node'

    @property
    def name(self) -> str:
        "See the class docstring"
        return self.spec.name

    @property
    def description(self) -> Optional[str]:
        "See the class docstring"
        return self.spec.description.strip() if self.spec.description else None

    @property
    def type(self) -> str:
        "See the class docstring"
        return self.spec.type

    @property
    def val_as_token(self) -> str:
        "See the class docstring"
        assert isinstance(self.val, str)
        return str_as_token(self.val)

    @property
    def enum_index(self) -> Optional[int]:
        "See the class docstring"
        enum = self.spec.enum
        return enum.index(self.val) if enum else None


@dataclass
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
      The starting address of the register, in the parent address space, or None
      if #address-cells is zero. Any 'ranges' properties are taken into account.

    size:
      The length of the register in bytes
    """

    node: 'Node'
    name: Optional[str]
    addr: Optional[int]
    size: Optional[int]


@dataclass
class Range:
    """
    Represents a translation range on a node as described by the 'ranges' property.

    These attributes are available on Range objects:

    node:
      The Node instance this range is from

    child_bus_cells:
      The number of cells used to describe a child bus address.

    child_bus_addr:
      A physical address within the child bus address space, or None if the
      child's #address-cells equals 0.

    parent_bus_cells:
      The number of cells used to describe a parent bus address.

    parent_bus_addr:
      A physical address within the parent bus address space, or None if the
      parent's #address-cells equals 0.

    length_cells:
      The number of cells used to describe the size of range in
      the child's address space.

    length:
      The size of the range in the child address space, or None if the
      child's #size-cells equals 0.
    """
    node: 'Node'
    child_bus_cells: int
    child_bus_addr: Optional[int]
    parent_bus_cells: int
    parent_bus_addr: Optional[int]
    length_cells: int
    length: Optional[int]


@dataclass
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

    basename:
      Basename for the controller when supporting named cells
    """
    node: 'Node'
    controller: 'Node'
    data: dict
    name: Optional[str]
    basename: Optional[str]


@dataclass
class PinCtrl:
    """
    Represents a pin control configuration for a set of pins on a device,
    e.g. pinctrl-0 or pinctrl-1.

    These attributes are available on PinCtrl objects:

    node:
      The Node instance the pinctrl-* property is on

    name:
      The name of the configuration, as given in pinctrl-names, or None if
      there is no pinctrl-names property

    name_as_token:
      Like 'name', but with non-alphanumeric characters converted to underscores.

    conf_nodes:
      A list of Node instances for the pin configuration nodes, e.g.
      the nodes pointed at by &state_1 and &state_2 in

          pinctrl-0 = <&state_1 &state_2>;
    """

    node: 'Node'
    name: Optional[str]
    conf_nodes: List['Node']

    @property
    def name_as_token(self):
        "See the class docstring"
        return str_as_token(self.name) if self.name is not None else None


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
      the node name has no unit-address portion. PCI devices use a different
      node name format ...@<dev>,<func> or ...@<dev> (e.g. "pcie@1,0"), in
      this case None is returned.

    description:
      The description string from the binding for the node, or None if the node
      has no binding. Leading and trailing whitespace (including newlines) is
      removed.

    path:
      The devicetree path of the node

    label:
      The text from the 'label' property on the node, or None if the node has
      no 'label'

    labels:
      A list of all of the devicetree labels for the node, in the same order
      as the labels appear, but with duplicates removed.

      This corresponds to the actual devicetree source labels, unlike the
      "label" attribute, which is the value of a devicetree property named
      "label".

    parent:
      The Node instance for the devicetree parent of the Node, or None if the
      node is the root node

    children:
      A dictionary with the Node instances for the devicetree children of the
      node, indexed by name

    dep_ordinal:
      A non-negative integer value such that the value for a Node is
      less than the value for all Nodes that depend on it.

      The ordinal is defined for all Nodes, and is unique among nodes in its
      EDT 'nodes' list.

    required_by:
      A list with the nodes that directly depend on the node

    depends_on:
      A list with the nodes that the node directly depends on

    status:
      The node's status property value, as a string, or "okay" if the node
      has no status property set. If the node's status property is "ok",
      it is converted to "okay" for consistency.

    read_only:
      True if the node has a 'read-only' property, and False otherwise

    matching_compat:
      The 'compatible' string for the binding that matched the node, or None if
      the node has no binding

    binding_path:
      The path to the binding file for the node, or None if the node has no
      binding

    compats:
      A list of 'compatible' strings for the node, in the same order that
      they're listed in the .dts file

    ranges:
      A list of Range objects extracted from the node's ranges property.
      The list is empty if the node does not have a range property.

    regs:
      A list of Register objects for the node's registers

    props:
      A dict that maps property names to Property objects.
      Property objects are created for all devicetree properties on the node
      that are mentioned in 'properties:' in the binding.

    aliases:
      A list of aliases for the node. This is fetched from the /aliases node.

    interrupts:
      A list of ControllerAndData objects for the interrupts generated by the
      node. The list is empty if the node does not generate interrupts.

    pinctrls:
      A list of PinCtrl objects for the pinctrl-<index> properties on the
      node, sorted by index. The list is empty if the node does not have any
      pinctrl-<index> properties.

    buses:
      If the node is a bus node (has a 'bus:' key in its binding), then this
      attribute holds the list of supported bus types, e.g. ["i2c"], ["spi"]
      or ["i3c", "i2c"] if multiple protocols are supported via the same bus.
      If the node is not a bus node, then this attribute is an empty list.

    on_buses:
      The bus the node appears on, e.g. ["i2c"], ["spi"] or ["i3c", "i2c"] if
      multiple protocols are supported via the same bus. The bus is determined
      by searching upwards for a parent node whose binding has a 'bus:' key,
      returning the value of the first 'bus:' key found. If none of the node's
      parents has a 'bus:' key, this attribute is an empty list.

    bus_node:
      Like on_bus, but contains the Node for the bus controller, or None if the
      node is not on a bus.

    flash_controller:
      The flash controller for the node. Only meaningful for nodes representing
      flash partitions.

    spi_cs_gpio:
      The device's SPI GPIO chip select as a ControllerAndData instance, if it
      exists, and None otherwise. See
      Documentation/devicetree/bindings/spi/spi-controller.yaml in the Linux kernel.

    gpio_hogs:
      A list of ControllerAndData objects for the GPIOs hogged by the node. The
      list is empty if the node does not hog any GPIOs. Only relevant for GPIO hog
      nodes.

    is_pci_device:
      True if the node is a PCI device.
    """

    def __init__(self,
                 dt_node: dtlib_Node,
                 edt: 'EDT',
                 compats: List[str]):
        '''
        For internal use only; not meant to be used outside edtlib itself.
        '''
        # Public, some of which are initialized properly later:
        self.edt: 'EDT' = edt
        self.dep_ordinal: int = -1
        self.matching_compat: Optional[str] = None
        self.binding_path: Optional[str] = None
        self.compats: List[str] = compats
        self.ranges: List[Range] = []
        self.regs: List[Register] = []
        self.props: Dict[str, Property] = {}
        self.interrupts: List[ControllerAndData] = []
        self.pinctrls: List[PinCtrl] = []
        self.bus_node: Optional['Node'] = None

        # Private, don't touch outside the class:
        self._node: dtlib_Node = dt_node
        self._binding: Optional[Binding] = None

    @property
    def name(self) -> str:
        "See the class docstring"
        return self._node.name

    @property
    def unit_addr(self) -> Optional[int]:
        "See the class docstring"

        # TODO: Return a plain string here later, like dtlib.Node.unit_addr?

        # PCI devices use a different node name format (e.g. "pcie@1,0")
        if "@" not in self.name or self.is_pci_device:
            return None

        try:
            addr = int(self.name.split("@", 1)[1], 16)
        except ValueError:
            _err(f"{self!r} has non-hex unit address")

        return _translate(addr, self._node)

    @property
    def description(self) -> Optional[str]:
        "See the class docstring."
        if self._binding:
            return self._binding.description
        return None

    @property
    def path(self) ->  str:
        "See the class docstring"
        return self._node.path

    @property
    def label(self) -> Optional[str]:
        "See the class docstring"
        if "label" in self._node.props:
            return self._node.props["label"].to_string()
        return None

    @property
    def labels(self) -> List[str]:
        "See the class docstring"
        return self._node.labels

    @property
    def parent(self) -> Optional['Node']:
        "See the class docstring"
        return self.edt._node2enode.get(self._node.parent) # type: ignore

    @property
    def children(self) -> Dict[str, 'Node']:
        "See the class docstring"
        # Could be initialized statically too to preserve identity, but not
        # sure if needed. Parent nodes being initialized before their children
        # would need to be kept in mind.
        return {name: self.edt._node2enode[node]
                for name, node in self._node.nodes.items()}

    def child_index(self, node) -> int:
        """Get the index of *node* in self.children.
        Raises KeyError if the argument is not a child of this node.
        """
        if not hasattr(self, '_child2index'):
            # Defer initialization of this lookup table until this
            # method is callable to handle parents needing to be
            # initialized before their chidlren. By the time we
            # return from __init__, 'self.children' is callable.
            self._child2index: Dict[str, int] = {}
            for index, child_path in enumerate(child.path for child in
                                               self.children.values()):
                self._child2index[child_path] = index

        return self._child2index[node.path]

    @property
    def required_by(self) -> List['Node']:
        "See the class docstring"
        return self.edt._graph.required_by(self)

    @property
    def depends_on(self) -> List['Node']:
        "See the class docstring"
        return self.edt._graph.depends_on(self)

    @property
    def status(self) -> str:
        "See the class docstring"
        status = self._node.props.get("status")

        if status is None:
            as_string = "okay"
        else:
            as_string = status.to_string()

        if as_string == "ok":
            as_string = "okay"

        return as_string

    @property
    def read_only(self) -> bool:
        "See the class docstring"
        return "read-only" in self._node.props

    @property
    def aliases(self) -> List[str]:
        "See the class docstring"
        return [alias for alias, node in self._node.dt.alias2node.items()
                if node is self._node]

    @property
    def buses(self) -> List[str]:
        "See the class docstring"
        if self._binding:
            return self._binding.buses
        return []

    @property
    def on_buses(self) -> List[str]:
        "See the class docstring"
        bus_node = self.bus_node
        return bus_node.buses if bus_node else []

    @property
    def flash_controller(self) -> 'Node':
        "See the class docstring"

        # The node path might be something like
        # /flash-controller@4001E000/flash@0/partitions/partition@fc000. We go
        # up two levels to get the flash and check its compat. The flash
        # controller might be the flash itself (for cases like NOR flashes).
        # For the case of 'soc-nv-flash', we assume the controller is the
        # parent of the flash node.

        if not self.parent or not self.parent.parent:
            _err(f"flash partition {self!r} lacks parent or grandparent node")

        controller = self.parent.parent
        if controller.matching_compat == "soc-nv-flash":
            if controller.parent is None:
                _err(f"flash controller '{controller.path}' cannot be the root node")
            return controller.parent
        return controller

    @property
    def spi_cs_gpio(self) -> Optional[ControllerAndData]:
        "See the class docstring"

        if not ("spi" in self.on_buses
                and self.bus_node
                and "cs-gpios" in self.bus_node.props):
            return None

        if not self.regs:
            _err(f"{self!r} needs a 'reg' property, to look up the "
                 "chip select index for SPI")

        parent_cs_lst = self.bus_node.props["cs-gpios"].val
        if TYPE_CHECKING:
            assert isinstance(parent_cs_lst, list)

        # cs-gpios is indexed by the unit address
        cs_index = self.regs[0].addr
        if TYPE_CHECKING:
            assert isinstance(cs_index, int)

        if cs_index >= len(parent_cs_lst):
            _err(f"index from 'regs' in {self!r} ({cs_index}) "
                 "is >= number of cs-gpios in "
                 f"{self.bus_node!r} ({len(parent_cs_lst)})")

        ret = parent_cs_lst[cs_index]
        if TYPE_CHECKING:
            assert isinstance(ret, ControllerAndData)
        return ret

    @property
    def gpio_hogs(self) -> List[ControllerAndData]:
        "See the class docstring"

        if "gpio-hog" not in self.props:
            return []

        if not self.parent or not "gpio-controller" in self.parent.props:
            _err(f"GPIO hog {self!r} lacks parent GPIO controller node")

        if not "#gpio-cells" in self.parent._node.props:
            _err(f"GPIO hog {self!r} parent node lacks #gpio-cells")

        n_cells = self.parent._node.props["#gpio-cells"].to_num()
        res = []

        for item in _slice(self._node, "gpios", 4*n_cells,
                           f"4*(<#gpio-cells> (= {n_cells})"):
            controller = self.parent
            res.append(ControllerAndData(
                node=self, controller=controller,
                data=self._named_cells(controller, item, "gpio"),
                name=None, basename="gpio"))

        return res

    @property
    def is_pci_device(self) -> bool:
        "See the class docstring"
        return 'pcie' in self.on_buses

    def __repr__(self) -> str:
        if self.binding_path:
            binding = "binding " + self.binding_path
        else:
            binding = "no binding"
        return f"<Node {self.path} in '{self.edt.dts_path}', {binding}>"

    def _init_binding(self) -> None:
        # Initializes Node.matching_compat, Node._binding, and
        # Node.binding_path.
        #
        # Node._binding holds the data from the node's binding file, in the
        # format returned by PyYAML (plain Python lists, dicts, etc.), or None
        # if the node has no binding.

        # This relies on the parent of the node having already been
        # initialized, which is guaranteed by going through the nodes in
        # node_iter() order.

        if self.path in self.edt._infer_binding_for_paths:
            self._binding_from_properties()
            return

        if self.compats:
            on_buses = self.on_buses

            for compat in self.compats:
                # When matching, respect the order of the 'compatible' entries,
                # and for each one first try to match against an explicitly
                # specified bus (if any) and then against any bus. This is so
                # that matching against bindings which do not specify a bus
                # works the same way in Zephyr as it does elsewhere.
                binding = None

                for bus in on_buses:
                    if (compat, bus) in self.edt._compat2binding:
                        binding = self.edt._compat2binding[compat, bus]
                        break

                if not binding:
                    if (compat, None) in self.edt._compat2binding:
                        binding = self.edt._compat2binding[compat, None]
                    else:
                        continue

                self.binding_path = binding.path
                self.matching_compat = compat
                self._binding = binding
                return
        else:
            # No 'compatible' property. See if the parent binding has
            # a compatible. This can come from one or more levels of
            # nesting with 'child-binding:'.

            binding_from_parent = self._binding_from_parent()
            if binding_from_parent:
                self._binding = binding_from_parent
                self.binding_path = self._binding.path
                self.matching_compat = self._binding.compatible

                return

        # No binding found
        self._binding = self.binding_path = self.matching_compat = None

    def _binding_from_properties(self) -> None:
        # Sets up a Binding object synthesized from the properties in the node.

        if self.compats:
            _err(f"compatible in node with inferred binding: {self.path}")

        # Synthesize a 'raw' binding as if it had been parsed from YAML.
        raw: Dict[str, Any] = {
            'description': 'Inferred binding from properties, via edtlib.',
            'properties': {},
        }
        for name, prop in self._node.props.items():
            pp: Dict[str, str] = {}
            if prop.type == Type.EMPTY:
                pp["type"] = "boolean"
            elif prop.type == Type.BYTES:
                pp["type"] = "uint8-array"
            elif prop.type == Type.NUM:
                pp["type"] = "int"
            elif prop.type == Type.NUMS:
                pp["type"] = "array"
            elif prop.type == Type.STRING:
                pp["type"] = "string"
            elif prop.type == Type.STRINGS:
                pp["type"] = "string-array"
            elif prop.type == Type.PHANDLE:
                pp["type"] = "phandle"
            elif prop.type == Type.PHANDLES:
                pp["type"] = "phandles"
            elif prop.type == Type.PHANDLES_AND_NUMS:
                pp["type"] = "phandle-array"
            elif prop.type == Type.PATH:
                pp["type"] = "path"
            else:
                _err(f"cannot infer binding from property: {prop} "
                     f"with type {prop.type!r}")
            raw['properties'][name] = pp

        # Set up Node state.
        self.binding_path = None
        self.matching_compat = None
        self.compats = []
        self._binding = Binding(None, {}, raw=raw, require_compatible=False)

    def _binding_from_parent(self) -> Optional[Binding]:
        # Returns the binding from 'child-binding:' in the parent node's
        # binding.

        if not self.parent:
            return None

        pbinding = self.parent._binding
        if not pbinding:
            return None

        if pbinding.child_binding:
            return pbinding.child_binding

        return None

    def _bus_node(self, support_fixed_partitions_on_any_bus: bool = True
                  ) -> Optional['Node']:
        # Returns the value for self.bus_node. Relies on parent nodes being
        # initialized before their children.

        if not self.parent:
            # This is the root node
            return None

        # Treat 'fixed-partitions' as if they are not on any bus.  The reason is
        # that flash nodes might be on a SPI or controller or SoC bus.  Having
        # bus be None means we'll always match the binding for fixed-partitions
        # also this means want processing the fixed-partitions node we wouldn't
        # try to do anything bus specific with it.
        if support_fixed_partitions_on_any_bus and "fixed-partitions" in self.compats:
            return None

        if self.parent.buses:
            # The parent node is a bus node
            return self.parent

        # Same bus node as parent (possibly None)
        return self.parent.bus_node

    def _init_props(self, default_prop_types: bool = False,
                    err_on_deprecated: bool = False) -> None:
        # Creates self.props. See the class docstring. Also checks that all
        # properties on the node are declared in its binding.

        self.props = {}

        node = self._node
        if self._binding:
            prop2specs = self._binding.prop2specs
        else:
            prop2specs = None

        # Initialize self.props
        if prop2specs:
            for prop_spec in prop2specs.values():
                self._init_prop(prop_spec, err_on_deprecated)
            self._check_undeclared_props()
        elif default_prop_types:
            for name in node.props:
                if name not in _DEFAULT_PROP_SPECS:
                    continue
                prop_spec = _DEFAULT_PROP_SPECS[name]
                val = self._prop_val(name, prop_spec.type, False, False, None,
                                     None, err_on_deprecated)
                self.props[name] = Property(prop_spec, val, self)

    def _init_prop(self, prop_spec: PropertySpec,
                   err_on_deprecated: bool) -> None:
        # _init_props() helper for initializing a single property.
        # 'prop_spec' is a PropertySpec object from the node's binding.

        name = prop_spec.name
        prop_type = prop_spec.type
        if not prop_type:
            _err(f"'{name}' in {self.binding_path} lacks 'type'")

        val = self._prop_val(name, prop_type, prop_spec.deprecated,
                             prop_spec.required, prop_spec.default,
                             prop_spec.specifier_space, err_on_deprecated)

        if val is None:
            # 'required: false' property that wasn't there, or a property type
            # for which we store no data.
            return

        enum = prop_spec.enum
        if enum and val not in enum:
            _err(f"value of property '{name}' on {self.path} in "
                 f"{self.edt.dts_path} ({val!r}) is not in 'enum' list in "
                 f"{self.binding_path} ({enum!r})")

        const = prop_spec.const
        if const is not None and val != const:
            _err(f"value of property '{name}' on {self.path} in "
                 f"{self.edt.dts_path} ({val!r}) "
                 "is different from the 'const' value specified in "
                 f"{self.binding_path} ({const!r})")

        # Skip properties that start with '#', like '#size-cells', and mapping
        # properties like 'gpio-map'/'interrupt-map'
        if name[0] == "#" or name.endswith("-map"):
            return

        self.props[name] = Property(prop_spec, val, self)

    def _prop_val(self, name: str, prop_type: str,
                  deprecated: bool, required: bool,
                  default: PropertyValType,
                  specifier_space: Optional[str],
                  err_on_deprecated: bool) -> PropertyValType:
        # _init_prop() helper for getting the property's value
        #
        # name:
        #   Property name from binding
        #
        # prop_type:
        #   Property type from binding (a string like "int")
        #
        # deprecated:
        #   True if the property is deprecated
        #
        # required:
        #   True if the property is required to exist
        #
        # default:
        #   Default value to use when the property doesn't exist, or None if
        #   the binding doesn't give a default value
        #
        # specifier_space:
        #   Property specifier-space from binding (if prop_type is "phandle-array")
        #
        # err_on_deprecated:
        #   If True, a deprecated property is an error instead of warning.

        node = self._node
        prop = node.props.get(name)

        if prop and deprecated:
            msg = (f"'{name}' is marked as deprecated in 'properties:' "
                   f"in {self.binding_path} for node {node.path}.")
            if err_on_deprecated:
                _err(msg)
            else:
                _LOG.warning(msg)

        if not prop:
            if required and self.status == "okay":
                _err(f"'{name}' is marked as required in 'properties:' in "
                     f"{self.binding_path}, but does not appear in {node!r}")

            if default is not None:
                # YAML doesn't have a native format for byte arrays. We need to
                # convert those from an array like [0x12, 0x34, ...]. The
                # format has already been checked in
                # _check_prop_by_type().
                if prop_type == "uint8-array":
                    return bytes(default) # type: ignore
                return default

            return False if prop_type == "boolean" else None

        if prop_type == "boolean":
            if prop.type != Type.EMPTY:
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
            if prop.type not in (Type.PHANDLE, Type.PHANDLES, Type.PHANDLES_AND_NUMS):
                _err(f"expected property '{name}' in {node.path} in "
                     f"{node.dt.filename} to be assigned "
                     f"with '{name} = < &foo ... &bar 1 ... &baz 2 3 >' "
                     f"(a mix of phandles and numbers), not '{prop}'")

            return self._standard_phandle_val_list(prop, specifier_space)

        if prop_type == "path":
            return self.edt._node2enode[prop.to_path()]

        # prop_type == "compound". Checking that the 'type:'
        # value is valid is done in _check_prop_by_type().
        #
        # 'compound' is a dummy type for properties that don't fit any of the
        # patterns above, so that we can require all entries in 'properties:'
        # to have a 'type: ...'. No Property object is created for it.
        return None

    def _check_undeclared_props(self) -> None:
        # Checks that all properties are declared in the binding

        for prop_name in self._node.props:
            # Allow a few special properties to not be declared in the binding
            if prop_name.endswith("-controller") or \
               prop_name.startswith("#") or \
               prop_name in {
                   "compatible", "status", "ranges", "phandle",
                   "interrupt-parent", "interrupts-extended", "device_type"}:
                continue

            if TYPE_CHECKING:
                assert self._binding

            if prop_name not in self._binding.prop2specs:
                _err(f"'{prop_name}' appears in {self._node.path} in "
                     f"{self.edt.dts_path}, but is not declared in "
                     f"'properties:' in {self.binding_path}")

    def _init_ranges(self) -> None:
        # Initializes self.ranges
        node = self._node

        self.ranges = []

        if "ranges" not in node.props:
            return

        raw_child_address_cells = node.props.get("#address-cells")
        parent_address_cells = _address_cells(node)
        if raw_child_address_cells is None:
            child_address_cells = 2 # Default value per DT spec.
        else:
            child_address_cells = raw_child_address_cells.to_num()
        raw_child_size_cells = node.props.get("#size-cells")
        if raw_child_size_cells is None:
            child_size_cells = 1 # Default value per DT spec.
        else:
            child_size_cells = raw_child_size_cells.to_num()

        # Number of cells for one translation 3-tuple in 'ranges'
        entry_cells = child_address_cells + parent_address_cells + child_size_cells

        if entry_cells == 0:
            if len(node.props["ranges"].value) == 0:
                return
            else:
                _err(f"'ranges' should be empty in {self._node.path} since "
                     f"<#address-cells> = {child_address_cells}, "
                     f"<#address-cells for parent> = {parent_address_cells} and "
                     f"<#size-cells> = {child_size_cells}")

        for raw_range in _slice(node, "ranges", 4*entry_cells,
                                f"4*(<#address-cells> (= {child_address_cells}) + "
                                "<#address-cells for parent> "
                                f"(= {parent_address_cells}) + "
                                f"<#size-cells> (= {child_size_cells}))"):

            child_bus_cells = child_address_cells
            if child_address_cells == 0:
                child_bus_addr = None
            else:
                child_bus_addr = to_num(raw_range[:4*child_address_cells])
            parent_bus_cells = parent_address_cells
            if parent_address_cells == 0:
                parent_bus_addr = None
            else:
                parent_bus_addr = to_num(
                    raw_range[(4*child_address_cells):
                              (4*child_address_cells + 4*parent_address_cells)])
            length_cells = child_size_cells
            if child_size_cells == 0:
                length = None
            else:
                length = to_num(
                    raw_range[(4*child_address_cells + 4*parent_address_cells):])

            self.ranges.append(Range(self, child_bus_cells, child_bus_addr,
                                     parent_bus_cells, parent_bus_addr,
                                     length_cells, length))

    def _init_regs(self) -> None:
        # Initializes self.regs

        node = self._node

        self.regs = []

        if "reg" not in node.props:
            return

        address_cells = _address_cells(node)
        size_cells = _size_cells(node)

        for raw_reg in _slice(node, "reg", 4*(address_cells + size_cells),
                              f"4*(<#address-cells> (= {address_cells}) + "
                              f"<#size-cells> (= {size_cells}))"):
            if address_cells == 0:
                addr = None
            else:
                addr = _translate(to_num(raw_reg[:4*address_cells]), node)
            if size_cells == 0:
                size = None
            else:
                size = to_num(raw_reg[4*address_cells:])
            # Size zero is ok for PCI devices
            if size_cells != 0 and size == 0 and not self.is_pci_device:
                _err(f"zero-sized 'reg' in {self._node!r} seems meaningless "
                     "(maybe you want a size of one or #size-cells = 0 "
                     "instead)")

            # We'll fix up the name when we're done.
            self.regs.append(Register(self, None, addr, size))

        _add_names(node, "reg", self.regs)

    def _init_pinctrls(self) -> None:
        # Initializes self.pinctrls from any pinctrl-<index> properties

        node = self._node

        # pinctrl-<index> properties
        pinctrl_props = [prop for name, prop in node.props.items()
                         if re.match("pinctrl-[0-9]+", name)]
        # Sort by index
        pinctrl_props.sort(key=lambda prop: prop.name)

        # Check indices
        for i, prop in enumerate(pinctrl_props):
            if prop.name != "pinctrl-" + str(i):
                _err(f"missing 'pinctrl-{i}' property on {node!r} "
                     "- indices should be contiguous and start from zero")

        self.pinctrls = []
        for prop in pinctrl_props:
            # We'll fix up the names below.
            self.pinctrls.append(PinCtrl(
                node=self,
                name=None,
                conf_nodes=[self.edt._node2enode[node]
                            for node in prop.to_nodes()]))

        _add_names(node, "pinctrl", self.pinctrls)

    def _init_interrupts(self) -> None:
        # Initializes self.interrupts

        node = self._node

        self.interrupts = []

        for controller_node, data in _interrupts(node):
            # We'll fix up the names below.
            controller = self.edt._node2enode[controller_node]
            self.interrupts.append(ControllerAndData(
                node=self, controller=controller,
                data=self._named_cells(controller, data, "interrupt"),
                name=None, basename=None))

        _add_names(node, "interrupt", self.interrupts)

    def _standard_phandle_val_list(
            self,
            prop: dtlib_Property,
            specifier_space: Optional[str]
    ) -> List[Optional[ControllerAndData]]:
        # Parses a property like
        #
        #     <prop.name> = <phandle cell phandle cell ...>;
        #
        # where each phandle points to a controller node that has a
        #
        #     #<specifier_space>-cells = <size>;
        #
        # property that gives the number of cells in the value after the
        # controller's phandle in the property.
        #
        # E.g. with a property like
        #
        #     pwms = <&foo 1 2 &bar 3>;
        #
        # If 'specifier_space' is "pwm", then we should have this elsewhere
        # in the tree:
        #
        #     foo: ... {
        #             #pwm-cells = <2>;
        #     };
        #
        #     bar: ... {
        #             #pwm-cells = <1>;
        #     };
        #
        # These values can be given names using the <specifier_space>-names:
        # list in the binding for the phandle nodes.
        #
        # Also parses any
        #
        #     <specifier_space>-names = "...", "...", ...
        #
        # Returns a list of Optional[ControllerAndData] instances.
        #
        # An index is None if the underlying phandle-array element is
        # unspecified.

        if not specifier_space:
            if prop.name.endswith("gpios"):
                # There's some slight special-casing for *-gpios properties in that
                # e.g. foo-gpios still maps to #gpio-cells rather than
                # #foo-gpio-cells
                specifier_space = "gpio"
            else:
                # Strip -s. We've already checked that property names end in -s
                # if there is no specifier space in _check_prop_by_type().
                specifier_space = prop.name[:-1]

        res: List[Optional[ControllerAndData]] = []

        for item in _phandle_val_list(prop, specifier_space):
            if item is None:
                res.append(None)
                continue

            controller_node, data = item
            mapped_controller, mapped_data = \
                _map_phandle_array_entry(prop.node, controller_node, data,
                                         specifier_space)

            controller = self.edt._node2enode[mapped_controller]
            # We'll fix up the names below.
            res.append(ControllerAndData(
                node=self, controller=controller,
                data=self._named_cells(controller, mapped_data,
                                       specifier_space),
                name=None, basename=specifier_space))

        _add_names(self._node, specifier_space, res)

        return res

    def _named_cells(
            self,
            controller: 'Node',
            data: bytes,
            basename: str
    ) -> Dict[str, int]:
        # Returns a dictionary that maps <basename>-cells names given in the
        # binding for 'controller' to cell values. 'data' is the raw data, as a
        # byte array.

        if not controller._binding:
            _err(f"{basename} controller {controller._node!r} "
                 f"for {self._node!r} lacks binding")

        if basename in controller._binding.specifier2cells:
            cell_names: List[str] = controller._binding.specifier2cells[basename]
        else:
            # Treat no *-cells in the binding the same as an empty *-cells, so
            # that bindings don't have to have e.g. an empty 'clock-cells:' for
            # '#clock-cells = <0>'.
            cell_names = []

        data_list = to_nums(data)
        if len(data_list) != len(cell_names):
            _err(f"unexpected '{basename}-cells:' length in binding for "
                 f"{controller._node!r} - {len(cell_names)} "
                 f"instead of {len(data_list)}")

        return dict(zip(cell_names, data_list))


class EDT:
    """
    Represents a devicetree augmented with information from bindings.

    These attributes are available on EDT objects:

    nodes:
      A list of Node objects for the nodes that appear in the devicetree

    compat2nodes:
      A collections.defaultdict that maps each 'compatible' string that appears
      on some Node to a list of Nodes with that compatible.
      The collection is sorted so that enabled nodes appear first in the
      collection.

    compat2okay:
      Like compat2nodes, but just for nodes with status 'okay'.

    compat2notokay:
      Like compat2nodes, but just for nodes with status not 'okay'.

    compat2vendor:
      A collections.defaultdict that maps each 'compatible' string that appears
      on some Node to a vendor name parsed from vendor_prefixes.

    compat2model:
      A collections.defaultdict that maps each 'compatible' string that appears
      on some Node to a model name parsed from that compatible.

    label2node:
      A dict that maps a node label to the node with that label.

    dep_ord2node:
      A dict that maps an ordinal to the node with that dependency ordinal.

    chosen_nodes:
      A dict that maps the properties defined on the devicetree's /chosen
      node to their values. 'chosen' is indexed by property name (a string),
      and values are converted to Node objects. Note that properties of the
      /chosen node which can't be converted to a Node are not included in
      the value.

    dts_path:
      The .dts path passed to __init__()

    dts_source:
      The final DTS source code of the loaded devicetree after merging nodes
      and processing /delete-node/ and /delete-property/, as a string

    bindings_dirs:
      The bindings directory paths passed to __init__()

    scc_order:
      A list of lists of Nodes. All elements of each list
      depend on each other, and the Nodes in any list do not depend
      on any Node in a subsequent list. Each list defines a Strongly
      Connected Component (SCC) of the graph.

      For an acyclic graph each list will be a singleton. Cycles
      will be represented by lists with multiple nodes. Cycles are
      not expected to be present in devicetree graphs.

    The standard library's pickle module can be used to marshal and
    unmarshal EDT objects.
    """

    def __init__(self,
                 dts: Optional[str],
                 bindings_dirs: List[str],
                 warn_reg_unit_address_mismatch: bool = True,
                 default_prop_types: bool = True,
                 support_fixed_partitions_on_any_bus: bool = True,
                 infer_binding_for_paths: Optional[Iterable[str]] = None,
                 vendor_prefixes: Optional[Dict[str, str]] = None,
                 werror: bool = False):
        """EDT constructor.

        dts:
          Path to devicetree .dts file. Passing None for this value
          is only for internal use; do not do that outside of edtlib.

        bindings_dirs:
          List of paths to directories containing bindings, in YAML format.
          These directories are recursively searched for .yaml files.

        warn_reg_unit_address_mismatch (default: True):
          If True, a warning is logged if a node has a 'reg' property where
          the address of the first entry does not match the unit address of the
          node

        default_prop_types (default: True):
          If True, default property types will be used when a node has no
          bindings.

        support_fixed_partitions_on_any_bus (default True):
          If True, set the Node.bus for 'fixed-partitions' compatible nodes
          to None.  This allows 'fixed-partitions' binding to match regardless
          of the bus the 'fixed-partition' is under.

        infer_binding_for_paths (default: None):
          An iterable of devicetree paths identifying nodes for which bindings
          should be inferred from the node content.  (Child nodes are not
          processed.)  Pass none if no nodes should support inferred bindings.

        vendor_prefixes (default: None):
          A dict mapping vendor prefixes in compatible properties to their
          descriptions. If given, compatibles in the form "manufacturer,device"
          for which "manufacturer" is neither a key in the dict nor a specially
          exempt set of grandfathered-in cases will cause warnings.

        werror (default: False):
          If True, some edtlib specific warnings become errors. This currently
          errors out if 'dts' has any deprecated properties set, or an unknown
          vendor prefix is used.
        """
        # All instance attributes should be initialized here.
        # This makes it easy to keep track of them, which makes
        # implementing __deepcopy__() easier.
        # If you change this, make sure to update __deepcopy__() too,
        # and update the tests for that method.

        # Public attributes (the rest are properties)
        self.nodes: List[Node] = []
        self.compat2nodes: Dict[str, List[Node]] = defaultdict(list)
        self.compat2okay: Dict[str, List[Node]] = defaultdict(list)
        self.compat2notokay: Dict[str, List[Node]] = defaultdict(list)
        self.compat2vendor: Dict[str, str] = defaultdict(str)
        self.compat2model: Dict[str, str]  = defaultdict(str)
        self.label2node: Dict[str, Node] = {}
        self.dep_ord2node: Dict[int, Node] = {}
        self.dts_path: str = dts # type: ignore
        self.bindings_dirs: List[str] = list(bindings_dirs)

        # Saved kwarg values for internal use
        self._warn_reg_unit_address_mismatch: bool = warn_reg_unit_address_mismatch
        self._default_prop_types: bool = default_prop_types
        self._fixed_partitions_no_bus: bool = support_fixed_partitions_on_any_bus
        self._infer_binding_for_paths: Set[str] = set(infer_binding_for_paths or [])
        self._vendor_prefixes: Dict[str, str] = vendor_prefixes or {}
        self._werror: bool = bool(werror)

        # Other internal state
        self._compat2binding: Dict[Tuple[str, Optional[str]], Binding] = {}
        self._graph: Graph = Graph()
        self._binding_paths: List[str] = _binding_paths(self.bindings_dirs)
        self._binding_fname2path: Dict[str, str] = {
            os.path.basename(path): path
            for path in self._binding_paths
        }
        self._node2enode: Dict[dtlib_Node, Node] = {}

        if dts is not None:
            try:
                self._dt = DT(dts)
            except DTError as e:
                raise EDTError(e) from e
            self._finish_init()

    def _finish_init(self) -> None:
        # This helper exists to make the __deepcopy__() implementation
        # easier to keep in sync with __init__().
        _check_dt(self._dt)

        self._init_compat2binding()
        self._init_nodes()
        self._init_graph()
        self._init_luts()

        self._check()

    def get_node(self, path: str) -> Node:
        """
        Returns the Node at the DT path or alias 'path'. Raises EDTError if the
        path or alias doesn't exist.
        """
        try:
            return self._node2enode[self._dt.get_node(path)]
        except DTError as e:
            _err(e)

    @property
    def chosen_nodes(self) -> Dict[str, Node]:
        ret: Dict[str, Node] = {}

        try:
            chosen = self._dt.get_node("/chosen")
        except DTError:
            return ret

        for name, prop in chosen.props.items():
            try:
                node = prop.to_path()
            except DTError:
                # DTS value is not phandle or string, or path doesn't exist
                continue

            ret[name] = self._node2enode[node]

        return ret

    def chosen_node(self, name: str) -> Optional[Node]:
        """
        Returns the Node pointed at by the property named 'name' in /chosen, or
        None if the property is missing
        """
        return self.chosen_nodes.get(name)

    @property
    def dts_source(self) -> str:
        return f"{self._dt}"

    def __repr__(self) -> str:
        return f"<EDT for '{self.dts_path}', binding directories " \
            f"'{self.bindings_dirs}'>"

    def __deepcopy__(self, memo) -> 'EDT':
        """
        Implements support for the standard library copy.deepcopy()
        function on EDT instances.
        """

        ret = EDT(
            None,
            self.bindings_dirs,
            warn_reg_unit_address_mismatch=self._warn_reg_unit_address_mismatch,
            default_prop_types=self._default_prop_types,
            support_fixed_partitions_on_any_bus=self._fixed_partitions_no_bus,
            infer_binding_for_paths=set(self._infer_binding_for_paths),
            vendor_prefixes=dict(self._vendor_prefixes),
            werror=self._werror
        )
        ret.dts_path = self.dts_path
        ret._dt = deepcopy(self._dt, memo)
        ret._finish_init()
        return ret

    @property
    def scc_order(self) -> List[List[Node]]:
        try:
            return self._graph.scc_order()
        except Exception as e:
            raise EDTError(e)

    def _process_properties_r(self, root_node, props_node):
        """
        Process props_node properties for dependencies, and add those as
        dependencies of root_node. Then walk through all the props_node
        children and do the same recursively, maintaining the same root_node.

        This ensures that on a node with child nodes, the parent node includes
        the dependencies of all the child nodes as well as its own.
        """
        # A Node depends on any Nodes present in 'phandle',
        # 'phandles', or 'phandle-array' property values.
        for prop in props_node.props.values():
            if prop.type == 'phandle':
                self._graph.add_edge(root_node, prop.val)
            elif prop.type == 'phandles':
                if TYPE_CHECKING:
                    assert isinstance(prop.val, list)
                for phandle_node in prop.val:
                    self._graph.add_edge(root_node, phandle_node)
            elif prop.type == 'phandle-array':
                if TYPE_CHECKING:
                    assert isinstance(prop.val, list)
                for cd in prop.val:
                    if cd is None:
                        continue
                    if TYPE_CHECKING:
                        assert isinstance(cd, ControllerAndData)
                    self._graph.add_edge(root_node, cd.controller)

        # A Node depends on whatever supports the interrupts it
        # generates.
        for intr in props_node.interrupts:
            self._graph.add_edge(root_node, intr.controller)

        # If the binding defines child bindings, link the child properties to
        # the root_node as well.
        if props_node._binding and props_node._binding.child_binding:
            for child in props_node.children.values():
                if "compatible" in child.props:
                    # Not a child node, normal node on a different binding.
                    continue
                self._process_properties_r(root_node, child)

    def _process_properties(self, node):
        """
        Add node dependencies based on own as well as child node properties,
        start from the node itself.
        """
        self._process_properties_r(node, node)

    def _init_graph(self) -> None:
        # Constructs a graph of dependencies between Node instances,
        # which is usable for computing a partial order over the dependencies.
        # The algorithm supports detecting dependency loops.
        #
        # Actually computing the SCC order is lazily deferred to the
        # first time the scc_order property is read.

        for node in self.nodes:
            # Always insert root node
            if not node.parent:
                self._graph.add_node(node)

            # A Node always depends on its parent.
            for child in node.children.values():
                self._graph.add_edge(child, node)

            self._process_properties(node)

    def _init_compat2binding(self) -> None:
        # Creates self._compat2binding, a dictionary that maps
        # (<compatible>, <bus>) tuples (both strings) to Binding objects.
        #
        # The Binding objects are created from YAML files discovered
        # in self.bindings_dirs as needed.
        #
        # For example, self._compat2binding["company,dev", "can"]
        # contains the Binding for the 'company,dev' device, when it
        # appears on the CAN bus.
        #
        # For bindings that don't specify a bus, <bus> is None, so that e.g.
        # self._compat2binding["company,notonbus", None] is the Binding.
        #
        # Only bindings for 'compatible' strings that appear in the devicetree
        # are loaded.

        dt_compats = _dt_compats(self._dt)
        # Searches for any 'compatible' string mentioned in the devicetree
        # files, with a regex
        dt_compats_search = re.compile(
            "|".join(re.escape(compat) for compat in dt_compats)
        ).search

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
                raw = yaml.load(contents, Loader=_BindingLoader)
            except yaml.YAMLError as e:
                _err(
                        f"'{binding_path}' appears in binding directories "
                        f"but isn't valid YAML: {e}")

            # Convert the raw data to a Binding object, erroring out
            # if necessary.
            binding = self._binding(raw, binding_path, dt_compats)

            # Register the binding in self._compat2binding, along with
            # any child bindings that have their own compatibles.
            while binding is not None:
                if binding.compatible:
                    self._register_binding(binding)
                binding = binding.child_binding

    def _binding(self,
                 raw: Optional[dict],
                 binding_path: str,
                 dt_compats: Set[str]) -> Optional[Binding]:
        # Convert a 'raw' binding from YAML to a Binding object and return it.
        #
        # Error out if the raw data looks like an invalid binding.
        #
        # Return None if the file doesn't contain a binding or the
        # binding's compatible isn't in dt_compats.

        # Get the 'compatible:' string.
        if raw is None or "compatible" not in raw:
            # Empty file, binding fragment, spurious file, etc.
            return None

        compatible = raw["compatible"]

        if compatible not in dt_compats:
            # Not a compatible we care about.
            return None

        # Initialize and return the Binding object.
        return Binding(binding_path, self._binding_fname2path, raw=raw)

    def _register_binding(self, binding: Binding) -> None:
        # Do not allow two different bindings to have the same
        # 'compatible:'/'on-bus:' combo
        if TYPE_CHECKING:
            assert binding.compatible
        old_binding = self._compat2binding.get((binding.compatible,
                                                binding.on_bus))
        if old_binding:
            msg = (f"both {old_binding.path} and {binding.path} have "
                   f"'compatible: {binding.compatible}'")
            if binding.on_bus is not None:
                msg += f" and 'on-bus: {binding.on_bus}'"
            _err(msg)

        # Register the binding.
        self._compat2binding[binding.compatible, binding.on_bus] = binding

    def _init_nodes(self) -> None:
        # Creates a list of edtlib.Node objects from the dtlib.Node objects, in
        # self.nodes

        for dt_node in self._dt.node_iter():
            # Warning: We depend on parent Nodes being created before their
            # children. This is guaranteed by node_iter().
            if "compatible" in dt_node.props:
                compats = dt_node.props["compatible"].to_strings()
            else:
                compats = []
            node = Node(dt_node, self, compats)
            node.bus_node = node._bus_node(self._fixed_partitions_no_bus)
            node._init_binding()
            node._init_regs()
            node._init_ranges()

            self.nodes.append(node)
            self._node2enode[dt_node] = node

        for node in self.nodes:
            # These depend on all Node objects having been created, because
            # they (either always or sometimes) reference other nodes, so we
            # run them separately
            node._init_props(default_prop_types=self._default_prop_types,
                             err_on_deprecated=self._werror)
            node._init_interrupts()
            node._init_pinctrls()

        if self._warn_reg_unit_address_mismatch:
            # This warning matches the simple_bus_reg warning in dtc
            for node in self.nodes:
                # Address mismatch is ok for PCI devices
                if (node.regs and node.regs[0].addr != node.unit_addr and
                        not node.is_pci_device):
                    _LOG.warning("unit address and first address in 'reg' "
                                 f"(0x{node.regs[0].addr:x}) don't match for "
                                 f"{node.path}")

    def _init_luts(self) -> None:
        # Initialize node lookup tables (LUTs).

        for node in self.nodes:
            for label in node.labels:
                self.label2node[label] = node

            for compat in node.compats:
                if node.status == "okay":
                    self.compat2okay[compat].append(node)
                else:
                    self.compat2notokay[compat].append(node)

                if compat in self.compat2vendor:
                    continue

                # The regular expression comes from dt-schema.
                compat_re = r'^[a-zA-Z][a-zA-Z0-9,+\-._]+$'
                if not re.match(compat_re, compat):
                    _err(f"node '{node.path}' compatible '{compat}' "
                         'must match this regular expression: '
                         f"'{compat_re}'")

                if ',' in compat and self._vendor_prefixes:
                    vendor, model = compat.split(',', 1)
                    if vendor in self._vendor_prefixes:
                        self.compat2vendor[compat] = self._vendor_prefixes[vendor]
                        self.compat2model[compat] = model

                    # As an exception, the root node can have whatever
                    # compatibles it wants. Other nodes get checked.
                    elif node.path != '/':
                        if self._werror:
                            handler_fn: Any = _err
                        else:
                            handler_fn = _LOG.warning
                        handler_fn(
                            f"node '{node.path}' compatible '{compat}' "
                            f"has unknown vendor prefix '{vendor}'")

        for compat, nodes in self.compat2okay.items():
            self.compat2nodes[compat].extend(nodes)

        for compat, nodes in self.compat2notokay.items():
            self.compat2nodes[compat].extend(nodes)

        for nodeset in self.scc_order:
            node = nodeset[0]
            self.dep_ord2node[node.dep_ordinal] = node

    def _check(self) -> None:
        # Tree-wide checks and warnings.

        for binding in self._compat2binding.values():
            for spec in binding.prop2specs.values():
                if not spec.enum or spec.type != 'string':
                    continue

                if not spec.enum_tokenizable:
                    _LOG.warning(
                        f"compatible '{binding.compatible}' "
                        f"in binding '{binding.path}' has non-tokenizable enum "
                        f"for property '{spec.name}': " +
                        ', '.join(repr(x) for x in spec.enum))
                elif not spec.enum_upper_tokenizable:
                    _LOG.warning(
                        f"compatible '{binding.compatible}' "
                        f"in binding '{binding.path}' has enum for property "
                        f"'{spec.name}' that is only tokenizable "
                        'in lowercase: ' +
                        ', '.join(repr(x) for x in spec.enum))

        # Validate the contents of compatible properties.
        for node in self.nodes:
            if 'compatible' not in node.props:
                continue

            compatibles = node.props['compatible'].val

            # _check() runs after _init_compat2binding() has called
            # _dt_compats(), which already converted every compatible
            # property to a list of strings. So we know 'compatibles'
            # is a list, but add an assert for future-proofing.
            assert isinstance(compatibles, list)

            for compat in compatibles:
                # This is also just for future-proofing.
                assert isinstance(compat, str)


def bindings_from_paths(yaml_paths: List[str],
                        ignore_errors: bool = False) -> List[Binding]:
    """
    Get a list of Binding objects from the yaml files 'yaml_paths'.

    If 'ignore_errors' is True, YAML files that cause an EDTError when
    loaded are ignored. (No other exception types are silenced.)
    """

    ret = []
    fname2path = {os.path.basename(path): path for path in yaml_paths}
    for path in yaml_paths:
        try:
            ret.append(Binding(path, fname2path))
        except EDTError:
            if ignore_errors:
                continue
            raise

    return ret


class EDTError(Exception):
    "Exception raised for devicetree- and binding-related errors"

#
# Public global functions
#


def load_vendor_prefixes_txt(vendor_prefixes: str) -> Dict[str, str]:
    """Load a vendor-prefixes.txt file and return a dict
    representation mapping a vendor prefix to the vendor name.
    """
    vnd2vendor: Dict[str, str] = {}
    with open(vendor_prefixes, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()

            if not line or line.startswith('#'):
                # Comment or empty line.
                continue

            # Other lines should be in this form:
            #
            # <vnd><TAB><vendor>
            vnd_vendor = line.split('\t', 1)
            assert len(vnd_vendor) == 2, line
            vnd2vendor[vnd_vendor[0]] = vnd_vendor[1]
    return vnd2vendor

#
# Private global functions
#


def _dt_compats(dt: DT) -> Set[str]:
    # Returns a set() with all 'compatible' strings in the devicetree
    # represented by dt (a dtlib.DT instance)

    return {compat
            for node in dt.node_iter()
                if "compatible" in node.props
                    for compat in node.props["compatible"].to_strings()}


def _binding_paths(bindings_dirs: List[str]) -> List[str]:
    # Returns a list with the paths to all bindings (.yaml files) in
    # 'bindings_dirs'

    binding_paths = []

    for bindings_dir in bindings_dirs:
        for root, _, filenames in os.walk(bindings_dir):
            for filename in filenames:
                if filename.endswith(".yaml") or filename.endswith(".yml"):
                    binding_paths.append(os.path.join(root, filename))

    return binding_paths


def _binding_inc_error(msg):
    # Helper for reporting errors in the !include implementation

    raise yaml.constructor.ConstructorError(None, None, "error: " + msg)


def _check_include_dict(name: Optional[str],
                        allowlist: Optional[List[str]],
                        blocklist: Optional[List[str]],
                        child_filter: Optional[dict],
                        binding_path: Optional[str]) -> None:
    # Check that an 'include:' named 'name' with property-allowlist
    # 'allowlist', property-blocklist 'blocklist', and
    # child-binding filter 'child_filter' has valid structure.

    if name is None:
        _err(f"'include:' element in {binding_path} "
             "should have a 'name' key")

    if allowlist is not None and blocklist is not None:
        _err(f"'include:' of file '{name}' in {binding_path} "
             "should not specify both 'property-allowlist:' "
             "and 'property-blocklist:'")

    while child_filter is not None:
        child_copy = deepcopy(child_filter)
        child_allowlist: Optional[List[str]] = \
            child_copy.pop('property-allowlist', None)
        child_blocklist: Optional[List[str]] = \
            child_copy.pop('property-blocklist', None)
        next_child_filter: Optional[dict] = \
            child_copy.pop('child-binding', None)

        if child_copy:
            # We've popped out all the valid keys.
            _err(f"'include:' of file '{name}' in {binding_path} "
                 "should not have these unexpected contents in a "
                 f"'child-binding': {child_copy}")

        if child_allowlist is not None and child_blocklist is not None:
            _err(f"'include:' of file '{name}' in {binding_path} "
                 "should not specify both 'property-allowlist:' and "
                 "'property-blocklist:' in a 'child-binding:'")

        child_filter = next_child_filter


def _filter_properties(raw: dict,
                       allowlist: Optional[List[str]],
                       blocklist: Optional[List[str]],
                       child_filter: Optional[dict],
                       binding_path: Optional[str]) -> None:
    # Destructively modifies 'raw["properties"]' and
    # 'raw["child-binding"]', if they exist, according to
    # 'allowlist', 'blocklist', and 'child_filter'.

    props = raw.get('properties')
    _filter_properties_helper(props, allowlist, blocklist, binding_path)

    child_binding = raw.get('child-binding')
    while child_filter is not None and child_binding is not None:
        _filter_properties_helper(child_binding.get('properties'),
                                  child_filter.get('property-allowlist'),
                                  child_filter.get('property-blocklist'),
                                  binding_path)
        child_filter = child_filter.get('child-binding')
        child_binding = child_binding.get('child-binding')


def _filter_properties_helper(props: Optional[dict],
                              allowlist: Optional[List[str]],
                              blocklist: Optional[List[str]],
                              binding_path: Optional[str]) -> None:
    if props is None or (allowlist is None and blocklist is None):
        return

    _check_prop_filter('property-allowlist', allowlist, binding_path)
    _check_prop_filter('property-blocklist', blocklist, binding_path)

    if allowlist is not None:
        allowset = set(allowlist)
        to_del = [prop for prop in props if prop not in allowset]
    else:
        if TYPE_CHECKING:
            assert blocklist
        blockset = set(blocklist)
        to_del = [prop for prop in props if prop in blockset]

    for prop in to_del:
        del props[prop]


def _check_prop_filter(name: str, value: Optional[List[str]],
                       binding_path: Optional[str]) -> None:
    # Ensure an include: ... property-allowlist or property-blocklist
    # is a list.

    if value is None:
        return

    if not isinstance(value, list):
        _err(f"'{name}' value {value} in {binding_path} should be a list")


def _merge_props(to_dict: dict,
                 from_dict: dict,
                 parent: Optional[str],
                 binding_path: Optional[str],
                 check_required: bool = False):
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
            _err(f"{binding_path} (in '{parent}'): '{prop}' "
                 f"from included file overwritten ('{from_dict[prop]}' "
                 f"replaced with '{to_dict[prop]}')")
        elif prop == "required":
            # Need a separate check here, because this code runs before
            # Binding._check()
            if not (isinstance(from_dict["required"], bool) and
                    isinstance(to_dict["required"], bool)):
                _err(f"malformed 'required:' setting for '{parent}' in "
                     f"'properties' in {binding_path}, expected true/false")

            # 'required: true' takes precedence
            to_dict["required"] = to_dict["required"] or from_dict["required"]


def _bad_overwrite(to_dict: dict, from_dict: dict, prop: str,
                   check_required: bool) -> bool:
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


def _check_prop_by_type(prop_name: str,
                        options: dict,
                        binding_path: Optional[str]) -> None:
    # Binding._check_properties() helper. Checks 'type:', 'default:',
    # 'const:' and # 'specifier-space:' for the property named 'prop_name'

    prop_type = options.get("type")
    default = options.get("default")
    const = options.get("const")

    if prop_type is None:
        _err(f"missing 'type:' for '{prop_name}' in 'properties' in "
             f"{binding_path}")

    ok_types = {"boolean", "int", "array", "uint8-array", "string",
                "string-array", "phandle", "phandles", "phandle-array",
                "path", "compound"}

    if prop_type not in ok_types:
        _err(f"'{prop_name}' in 'properties:' in {binding_path} "
             f"has unknown type '{prop_type}', expected one of " +
             ", ".join(ok_types))

    if "specifier-space" in options and prop_type != "phandle-array":
        _err(f"'specifier-space' in 'properties: {prop_name}' "
             f"has type '{prop_type}', expected 'phandle-array'")

    if prop_type == "phandle-array":
        if not prop_name.endswith("s") and not "specifier-space" in options:
            _err(f"'{prop_name}' in 'properties:' in {binding_path} "
                 f"has type 'phandle-array' and its name does not end in 's', "
                 f"but no 'specifier-space' was provided.")

    # If you change const_types, be sure to update the type annotation
    # for PropertySpec.const.
    const_types = {"int", "array", "uint8-array", "string", "string-array"}
    if const and prop_type not in const_types:
        _err(f"const in {binding_path} for property '{prop_name}' "
             f"has type '{prop_type}', expected one of " +
             ", ".join(const_types))

    # Check default

    if default is None:
        return

    if prop_type in {"boolean", "compound", "phandle", "phandles",
                     "phandle-array", "path"}:
        _err("'default:' can't be combined with "
             f"'type: {prop_type}' for '{prop_name}' in "
             f"'properties:' in {binding_path}")

    def ok_default() -> bool:
        # Returns True if 'default' is an okay default for the property's type.
        # If you change this, be sure to update the type annotation for
        # PropertySpec.default.

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
        _err(f"'default: {default}' is invalid for '{prop_name}' "
             f"in 'properties:' in {binding_path}, "
             f"which has type {prop_type}")


def _translate(addr: int, node: dtlib_Node) -> int:
    # Recursively translates 'addr' on 'node' to the address space(s) of its
    # parent(s), by looking at 'ranges' properties. Returns the translated
    # address.

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

    for raw_range in _slice(node.parent, "ranges", 4*entry_cells,
                            f"4*(<#address-cells> (= {child_address_cells}) + "
                            "<#address-cells for parent> "
                            f"(= {parent_address_cells}) + "
                            f"<#size-cells> (= {child_size_cells}))"):
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


def _add_names(node: dtlib_Node, names_ident: str, objs: Any) -> None:
    # Helper for registering names from <foo>-names properties.
    #
    # node:
    #   Node which has a property that might need named elements.
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
            _err(f"{full_names_ident} property in {node.path} "
                 f"in {node.dt.filename} has {len(names)} strings, "
                 f"expected {len(objs)} strings")

        for obj, name in zip(objs, names):
            if obj is None:
                continue
            obj.name = name
    else:
        for obj in objs:
            if obj is not None:
                obj.name = None


def _interrupt_parent(start_node: dtlib_Node) -> dtlib_Node:
    # Returns the node pointed at by the closest 'interrupt-parent', searching
    # the parents of 'node'. As of writing, this behavior isn't specified in
    # the DT spec., but seems to match what some .dts files except.

    node: Optional[dtlib_Node] = start_node

    while node:
        if "interrupt-parent" in node.props:
            return node.props["interrupt-parent"].to_node()
        node = node.parent

    _err(f"{start_node!r} has an 'interrupts' property, but neither the node "
         f"nor any of its parents has an 'interrupt-parent' property")


def _interrupts(node: dtlib_Node) -> List[Tuple[dtlib_Node, bytes]]:
    # Returns a list of (<controller>, <data>) tuples, with one tuple per
    # interrupt generated by 'node'. <controller> is the destination of the
    # interrupt (possibly after mapping through an 'interrupt-map'), and <data>
    # the data associated with the interrupt (as a 'bytes' object).

    # Takes precedence over 'interrupts' if both are present
    if "interrupts-extended" in node.props:
        prop = node.props["interrupts-extended"]

        ret: List[Tuple[dtlib_Node, bytes]] = []
        for entry in _phandle_val_list(prop, "interrupt"):
            if entry is None:
                _err(f"node '{node.path}' interrupts-extended property "
                     "has an empty element")
            iparent, spec = entry
            ret.append(_map_interrupt(node, iparent, spec))
        return ret

    if "interrupts" in node.props:
        # Treat 'interrupts' as a special case of 'interrupts-extended', with
        # the same interrupt parent for all interrupts

        iparent = _interrupt_parent(node)
        interrupt_cells = _interrupt_cells(iparent)

        return [_map_interrupt(node, iparent, raw)
                for raw in _slice(node, "interrupts", 4*interrupt_cells,
                                  "4*<#interrupt-cells>")]

    return []


def _map_interrupt(
        child: dtlib_Node,
        parent: dtlib_Node,
        child_spec: bytes
) -> Tuple[dtlib_Node, bytes]:
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
            _err(f"missing #address-cells on {node!r} "
                 "(while handling interrupt-map)")
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


def _map_phandle_array_entry(
        child: dtlib_Node,
        parent: dtlib_Node,
        child_spec: bytes,
        basename: str
) -> Tuple[dtlib_Node, bytes]:
    # Returns a (<controller>, <data>) tuple with the final destination after
    # mapping through any '<basename>-map' (e.g. gpio-map) properties. See
    # _map_interrupt().

    def spec_len_fn(node):
        prop_name = f"#{basename}-cells"
        if prop_name not in node.props:
            _err(f"expected '{prop_name}' property on {node!r} "
                 f"(referenced by {child!r})")
        return node.props[prop_name].to_num()

    # Do not require <prefix>-controller for anything but interrupts for now
    return _map(basename, child, parent, child_spec, spec_len_fn,
                require_controller=False)


def _map(
        prefix: str,
        child: dtlib_Node,
        parent: dtlib_Node,
        child_spec: bytes,
        spec_len_fn: Callable[[dtlib_Node], int],
        require_controller: bool
) -> Tuple[dtlib_Node, bytes]:
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
            _err(f"expected '{prefix}-controller' property on {parent!r} "
                 f"(referenced by {child!r})")

        # No mapping
        return (parent, child_spec)

    masked_child_spec = _mask(prefix, child, parent, child_spec)

    raw = map_prop.value
    while raw:
        if len(raw) < len(child_spec):
            _err(f"bad value for {map_prop!r}, missing/truncated child data")
        child_spec_entry = raw[:len(child_spec)]
        raw = raw[len(child_spec):]

        if len(raw) < 4:
            _err(f"bad value for {map_prop!r}, missing/truncated phandle")
        phandle = to_num(raw[:4])
        raw = raw[4:]

        # Parent specified in *-map
        map_parent = parent.dt.phandle2node.get(phandle)
        if not map_parent:
            _err(f"bad phandle ({phandle}) in {map_prop!r}")

        map_parent_spec_len = 4*spec_len_fn(map_parent)
        if len(raw) < map_parent_spec_len:
            _err(f"bad value for {map_prop!r}, missing/truncated parent data")
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

    _err(f"child specifier for {child!r} ({child_spec!r}) "
         f"does not appear in {map_prop!r}")


def _mask(
        prefix: str,
        child: dtlib_Node,
        parent: dtlib_Node,
        child_spec: bytes
) -> bytes:
    # Common code for handling <prefix>-mask properties, e.g. interrupt-mask.
    # See _map() for the parameters.

    mask_prop = parent.props.get(prefix + "-map-mask")
    if not mask_prop:
        # No mask
        return child_spec

    mask = mask_prop.value
    if len(mask) != len(child_spec):
        _err(f"{child!r}: expected '{prefix}-mask' in {parent!r} "
             f"to be {len(child_spec)} bytes, is {len(mask)} bytes")

    return _and(child_spec, mask)


def _pass_thru(
        prefix: str,
        child: dtlib_Node,
        parent: dtlib_Node,
        child_spec: bytes,
        parent_spec: bytes
) -> bytes:
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
        _err(f"{child!r}: expected '{prefix}-map-pass-thru' in {parent!r} "
             f"to be {len(child_spec)} bytes, is {len(pass_thru)} bytes")

    res = _or(_and(child_spec, pass_thru),
              _and(parent_spec, _not(pass_thru)))

    # Truncate to length of parent spec.
    return res[-len(parent_spec):]


def _raw_unit_addr(node: dtlib_Node) -> bytes:
    # _map_interrupt() helper. Returns the unit address (derived from 'reg' and
    # #address-cells) as a raw 'bytes'

    if 'reg' not in node.props:
        _err(f"{node!r} lacks 'reg' property "
             "(needed for 'interrupt-map' unit address lookup)")

    addr_len = 4*_address_cells(node)

    if len(node.props['reg'].value) < addr_len:
        _err(f"{node!r} has too short 'reg' property "
             "(while doing 'interrupt-map' unit address lookup)")

    return node.props['reg'].value[:addr_len]


def _and(b1: bytes, b2: bytes) -> bytes:
    # Returns the bitwise AND of the two 'bytes' objects b1 and b2. Pads
    # with ones on the left if the lengths are not equal.

    # Pad on the left, to equal length
    maxlen = max(len(b1), len(b2))
    return bytes(x & y for x, y in zip(b1.rjust(maxlen, b'\xff'),
                                       b2.rjust(maxlen, b'\xff')))


def _or(b1: bytes, b2: bytes) -> bytes:
    # Returns the bitwise OR of the two 'bytes' objects b1 and b2. Pads with
    # zeros on the left if the lengths are not equal.

    # Pad on the left, to equal length
    maxlen = max(len(b1), len(b2))
    return bytes(x | y for x, y in zip(b1.rjust(maxlen, b'\x00'),
                                       b2.rjust(maxlen, b'\x00')))


def _not(b: bytes) -> bytes:
    # Returns the bitwise not of the 'bytes' object 'b'

    # ANDing with 0xFF avoids negative numbers
    return bytes(~x & 0xFF for x in b)


def _phandle_val_list(
        prop: dtlib_Property,
        n_cells_name: str
) -> List[Optional[Tuple[dtlib_Node, bytes]]]:
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
    # Each tuple in the return value is a (<node>, <value>) pair, where <node>
    # is the node pointed at by <phandle>. If <phandle> does not refer
    # to a node, the entire list element is None.

    full_n_cells_name = f"#{n_cells_name}-cells"

    res: List[Optional[Tuple[dtlib_Node, bytes]]] = []

    raw = prop.value
    while raw:
        if len(raw) < 4:
            # Not enough room for phandle
            _err("bad value for " + repr(prop))
        phandle = to_num(raw[:4])
        raw = raw[4:]

        node = prop.node.dt.phandle2node.get(phandle)
        if not node:
            # Unspecified phandle-array element. This is valid; a 0
            # phandle value followed by no cells is an empty element.
            res.append(None)
            continue

        if full_n_cells_name not in node.props:
            _err(f"{node!r} lacks {full_n_cells_name}")

        n_cells = node.props[full_n_cells_name].to_num()
        if len(raw) < 4*n_cells:
            _err("missing data after phandle in " + repr(prop))

        res.append((node, raw[:4*n_cells]))
        raw = raw[4*n_cells:]

    return res


def _address_cells(node: dtlib_Node) -> int:
    # Returns the #address-cells setting for 'node', giving the number of <u32>
    # cells used to encode the address in the 'reg' property
    if TYPE_CHECKING:
        assert node.parent

    if "#address-cells" in node.parent.props:
        return node.parent.props["#address-cells"].to_num()
    return 2  # Default value per DT spec.


def _size_cells(node: dtlib_Node) -> int:
    # Returns the #size-cells setting for 'node', giving the number of <u32>
    # cells used to encode the size in the 'reg' property
    if TYPE_CHECKING:
        assert node.parent

    if "#size-cells" in node.parent.props:
        return node.parent.props["#size-cells"].to_num()
    return 1  # Default value per DT spec.


def _interrupt_cells(node: dtlib_Node) -> int:
    # Returns the #interrupt-cells property value on 'node', erroring out if
    # 'node' has no #interrupt-cells property

    if "#interrupt-cells" not in node.props:
        _err(f"{node!r} lacks #interrupt-cells")
    return node.props["#interrupt-cells"].to_num()


def _slice(node: dtlib_Node,
           prop_name: str,
           size: int,
           size_hint: str) -> List[bytes]:
    return _slice_helper(node, prop_name, size, size_hint, EDTError)


def _check_dt(dt: DT) -> None:
    # Does devicetree sanity checks. dtlib is meant to be general and
    # anything-goes except for very special properties like phandle, but in
    # edtlib we can be pickier.

    # Check that 'status' has one of the values given in the devicetree spec.

    # Accept "ok" for backwards compatibility
    ok_status = {"ok", "okay", "disabled", "reserved", "fail", "fail-sss"}

    for node in dt.node_iter():
        if "status" in node.props:
            try:
                status_val = node.props["status"].to_string()
            except DTError as e:
                # The error message gives the path
                _err(str(e))

            if status_val not in ok_status:
                _err(f"unknown 'status' value \"{status_val}\" in {node.path} "
                     f"in {node.dt.filename}, expected one of " +
                     ", ".join(ok_status) +
                     " (see the devicetree specification)")

        ranges_prop = node.props.get("ranges")
        if ranges_prop:
            if ranges_prop.type not in (Type.EMPTY, Type.NUMS):
                _err(f"expected 'ranges = < ... >;' in {node.path} in "
                     f"{node.dt.filename}, not '{ranges_prop}' "
                     "(see the devicetree specification)")


def _err(msg) -> NoReturn:
    raise EDTError(msg)

# Logging object
_LOG = logging.getLogger(__name__)

# Regular expression for non-alphanumeric-or-underscore characters.
_NOT_ALPHANUM_OR_UNDERSCORE = re.compile(r'\W', re.ASCII)


def str_as_token(val: str) -> str:
    """Return a canonical representation of a string as a C token.

    This converts special characters in 'val' to underscores, and
    returns the result."""

    return re.sub(_NOT_ALPHANUM_OR_UNDERSCORE, '_', val)


# Custom PyYAML binding loader class to avoid modifying yaml.Loader directly,
# which could interfere with YAML loading in clients
class _BindingLoader(Loader):
    pass


# Add legacy '!include foo.yaml' handling
_BindingLoader.add_constructor("!include", _binding_include)

#
# "Default" binding for properties which are defined by the spec.
#
# Zephyr: do not change the _DEFAULT_PROP_TYPES keys without
# updating the documentation for the DT_PROP() macro in
# include/devicetree.h.
#

_DEFAULT_PROP_TYPES: Dict[str, str] = {
    "compatible": "string-array",
    "status": "string",
    "ranges": "compound",  # NUMS or EMPTY
    "reg": "array",
    "reg-names": "string-array",
    "label": "string",
    "interrupts": "array",
    "interrupts-extended": "compound",
    "interrupt-names": "string-array",
    "interrupt-controller": "boolean",
}

_STATUS_ENUM: List[str] = "ok okay disabled reserved fail fail-sss".split()

def _raw_default_property_for(
        name: str
) -> Dict[str, Union[str, bool, List[str]]]:
    ret: Dict[str, Union[str, bool, List[str]]] = {
        'type': _DEFAULT_PROP_TYPES[name],
        'required': False,
    }
    if name == 'status':
        ret['enum'] = _STATUS_ENUM
    return ret

_DEFAULT_PROP_BINDING: Binding = Binding(
    None, {},
    raw={
        'properties': {
            name: _raw_default_property_for(name)
            for name in _DEFAULT_PROP_TYPES
        },
    },
    require_compatible=False, require_description=False,
)

_DEFAULT_PROP_SPECS: Dict[str, PropertySpec] = {
    name: PropertySpec(name, _DEFAULT_PROP_BINDING)
    for name in _DEFAULT_PROP_TYPES
}
