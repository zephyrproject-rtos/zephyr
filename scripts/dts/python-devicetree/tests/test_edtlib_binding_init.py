# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Christophe Dufaza

"""Unit tests dedicated to edtlib.Binding objects initialization.

Running the assumption that any (valid) YAML binding file is
something we can make a Binding instance with:
- check which properties are defined at which level (binding, child-binding,
  grandchild-binding, etc) and their specifications once the binding
  is initialized
- check how including bindings are permitted to specialize
  the specifications of inherited properties
- check the rules applied when overwriting a binding's description
  or compatible string (at the binding, child-binding, etc, levels)

At any level, an including binding is permitted to:
- filter the properties it chooses to inherit with either "property:allowlist"
  or "property:blocklist" but not both
- extend inherited properties:
  - override (implicit or) explicit "required: false" with "required: true"
  - add constraints to the possible value(s) of a property with "const:"
    or "enum:"
  - add a "default:" value to a property
  - add or overwrite a property's "description:"; when overwritten multiple
    times by several included binding files, "include:"ed first "wins"
- define new properties

At any level, an including binding is NOT permitted to:
- remove a requirement by overriding "required: true" with "required: false"
- change existing constrains applied to the possible values of
  an inherited property with "const:" or "enum:"
- change the "default:" value of an inherited property
- change the "type:" of an inherited property

Rules applying to bindings' descriptions and compatible strings:
- included files can't overwrite the description or compatible string set
  by the including binding (despite that "description:" appears before
  "include:"): this seems consistent, the top-level binding file "wins"
- an including binding can overwrite descriptions and compatible strings
  inherited at the child-binding levels: this seems consistent,
  the top-level binding file "wins"
- when we include multiple files overwriting a description or compatible
  string inherited at the child-binding levels, order of inclusion matters,
  the first "wins"; this is consistent with property descriptions

For all tests, the entry point is a Binding instance initialized
by loading the YAML file which represents the test case: our focus here
really is what happens when we (recursively) call Binding's constructor,
independently of any actual devicetree model (edtlib.EDT instance).
"""

# pylint: disable=too-many-statements

import contextlib
import os
from collections.abc import Generator
from typing import Any

import pytest
from devicetree import edtlib

YAML_KNOWN_BASE_BINDINGS: dict[str, str] = {
    # Base properties, bottom of the diamond test case.
    "base.yaml": "test-bindings-init/base.yaml",
    # Amended properties, left (first) "include:" in the diamond test case.
    "base_amend.yaml": "test-bindings-init/base_amend.yaml",
    # Amended properties, right (last) "include:" in the diamond test case.
    "thing.yaml": "test-bindings-init/thing.yaml",
    # Used for testing property filters when "A includes B includes C".
    "simple.yaml": "test-bindings-init/simple.yaml",
    "simple_inherit.yaml": "test-bindings-init/simple_inherit.yaml",
    "simple_allowlist.yaml": "test-bindings-init/simple_allowlist.yaml",
    "simple_blocklist.yaml": "test-bindings-init/simple_blocklist.yaml",
    # Test applied rules for compatible strings and descriptions.
    "compat_desc_base.yaml": "test-bindings-init/compat_desc_base.yaml",
    "compat_desc.yaml": "test-bindings-init/compat_desc.yaml",
}


def load_binding(path: str) -> edtlib.Binding:
    """Load YAML file as Binding instance,
    using YAML_BASE to resolve includes.

    Args:
        path: Path relative to ZEPHYR_BASE/scripts/dts/python-devicetree/tests.
    """
    with _from_here():
        binding = edtlib.Binding(
            path=path,
            fname2path=YAML_KNOWN_BASE_BINDINGS,
            raw=None,
            require_compatible=False,
            require_description=False,
        )
    return binding


def child_binding_of(path: str) -> edtlib.Binding:
    """Load YAML file as Binding instance, and returns its child-binding.
    The child-binding must exist.

    Args:
        path: Path relative to ZEPHYR_BASE/scripts/dts/python-devicetree/tests.
    """
    binding = load_binding(path)
    assert binding.child_binding
    return binding.child_binding


def grandchild_binding_of(path: str) -> edtlib.Binding:
    """Load YAML file as Binding instance, and returns its grandchild-binding.
    The grandchild-binding must exist.

    Args:
        path: Path relative to ZEPHYR_BASE/scripts/dts/python-devicetree/tests.
    """
    child_binding = child_binding_of(path)
    assert child_binding.child_binding
    return child_binding.child_binding


def verify_expected_propspec(
    propspec: edtlib.PropertySpec,
    /,
    *,
    # Most properties are integers.
    expect_type: str = "int",
    expect_req: bool = False,
    expect_desc: str | None = None,
    expect_enum: list[int | str] | None = None,
    expect_const: Any | None = None,
    expect_default: Any | None = None,
) -> None:
    """Compare a property specification with the definitions
    we (finally) expect.

    All definitions are tested for equality.

    Args:
        propsec: The property specification to verify.
        expect_type: The expected property type definition.
        expect_req: Whether the property is expected to be required.
        expect_desc: The expected property description.
        expect_enum: The expected property "enum:" definition.
        expect_const: The expected property "const:" definition.
        expect_default: The expected property "default:" definition.
    """
    assert expect_type == propspec.type
    assert expect_req == propspec.required
    assert expect_desc == propspec.description
    assert expect_enum == propspec.enum
    assert expect_const == propspec.const
    assert expect_default == propspec.default


def verify_binding_propspecs_consistency(binding: edtlib.Binding) -> None:
    """Verify consistency between what's in Binding.prop2specs
    and Binding.raw.

    Asserts that:
        Binding.raw["properties"][prop] == Binding.prop2specs[prop]._raw

    If the binding has a child-binding, also recursively verify child-bindings.

    NOTE: do not confuse with binding.prop2specs[prop].binding == binding,
    which we do not assume here.
    """
    if binding.prop2specs:
        assert set(binding.raw["properties"].keys()) == set(binding.prop2specs.keys())
        assert all(
            binding.raw["properties"][prop] == propspec._raw
            for prop, propspec in binding.prop2specs.items()
        )
    if binding.child_binding:
        verify_binding_propspecs_consistency(binding.child_binding)


def test_expect_propspecs_inherited_bindings() -> None:
    """Test the basics of including property specifications.

    Specifications are simply inherited without modifications
    up to the grandchild-binding level.

    Check that we actually inherit all expected definitions as-is.
    """
    binding = load_binding("test-bindings-init/base_inherit.yaml")

    # Binding level.
    assert {
        "prop-1",
        "prop-2",
        "prop-enum",
        "prop-req",
        "prop-const",
        "prop-default",
    } == set(binding.prop2specs.keys())
    propspec = binding.prop2specs["prop-1"]
    verify_expected_propspec(propspec, expect_desc="Base property 1.")
    propspec = binding.prop2specs["prop-2"]
    verify_expected_propspec(propspec, expect_type="string")
    propspec = binding.prop2specs["prop-enum"]
    verify_expected_propspec(propspec, expect_type="string", expect_enum=["FOO", "BAR"])
    propspec = binding.prop2specs["prop-const"]
    verify_expected_propspec(propspec, expect_const=8)
    propspec = binding.prop2specs["prop-req"]
    verify_expected_propspec(propspec, expect_req=True)
    propspec = binding.prop2specs["prop-default"]
    verify_expected_propspec(propspec, expect_default=1)

    # Child-Binding level.
    assert binding.child_binding
    child_binding = binding.child_binding
    assert {
        "child-prop-1",
        "child-prop-2",
        "child-prop-enum",
        "child-prop-req",
        "child-prop-const",
        "child-prop-default",
    } == set(child_binding.prop2specs.keys())
    propspec = child_binding.prop2specs["child-prop-1"]
    verify_expected_propspec(propspec, expect_desc="Base child-prop 1.")
    propspec = child_binding.prop2specs["child-prop-2"]
    verify_expected_propspec(propspec, expect_type="string")
    propspec = child_binding.prop2specs["child-prop-enum"]
    verify_expected_propspec(propspec, expect_type="string", expect_enum=["CHILD_FOO", "CHILD_BAR"])
    propspec = child_binding.prop2specs["child-prop-const"]
    verify_expected_propspec(propspec, expect_const=16)
    propspec = child_binding.prop2specs["child-prop-req"]
    verify_expected_propspec(propspec, expect_req=True)
    propspec = child_binding.prop2specs["child-prop-default"]
    verify_expected_propspec(propspec, expect_default=2)

    # GrandChild-Binding level.
    assert child_binding.child_binding
    grandchild_binding = child_binding.child_binding
    assert {
        "grandchild-prop-1",
        "grandchild-prop-2",
        "grandchild-prop-enum",
        "grandchild-prop-req",
        "grandchild-prop-const",
        "grandchild-prop-default",
    } == set(grandchild_binding.prop2specs.keys())
    propspec = grandchild_binding.prop2specs["grandchild-prop-1"]
    verify_expected_propspec(propspec, expect_desc="Base grandchild-prop 1.")
    propspec = grandchild_binding.prop2specs["grandchild-prop-2"]
    verify_expected_propspec(propspec, expect_type="string")
    propspec = grandchild_binding.prop2specs["grandchild-prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["GRANDCHILD_FOO", "GRANDCHILD_BAR"],
    )
    propspec = grandchild_binding.prop2specs["grandchild-prop-const"]
    verify_expected_propspec(propspec, expect_const=32)
    propspec = grandchild_binding.prop2specs["grandchild-prop-req"]
    verify_expected_propspec(propspec, expect_req=True)
    propspec = grandchild_binding.prop2specs["grandchild-prop-default"]
    verify_expected_propspec(propspec, expect_default=3)


def test_expect_propspecs_amended_bindings() -> None:
    """Test the basics of including and amending property specifications.

    Base specifications are included once at the binding level:

        include: base.yaml
        properties:
          # Amend base.yaml
        child-binding:
          properties:
            # Amend base.yaml
          child-binding:
            properties:
              # Amend base.yaml

    Check that we finally get the expected property specifications
    up to the grandchild-binding level.
    """
    binding = load_binding("test-bindings-init/base_amend.yaml")

    # Binding level.
    #
    assert {
        "prop-1",
        "prop-2",
        "prop-enum",
        "prop-req",
        "prop-const",
        "prop-default",
        "prop-new",
    } == set(binding.prop2specs.keys())
    propspec = binding.prop2specs["prop-1"]
    verify_expected_propspec(
        propspec,
        # Amended in base_amend.yaml.
        expect_desc="Overwritten description.",
        expect_const=0xF0,
    )
    propspec = binding.prop2specs["prop-2"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        # Amended in base_amend.yaml.
        expect_desc="New description.",
        expect_enum=["EXT_FOO", "EXT_BAR"],
        expect_default="EXT_FOO",
    )
    propspec = binding.prop2specs["prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["FOO", "BAR"],
        # Amended in base_amend.yaml.
        expect_req=True,
    )
    # Inherited from base.yaml without modification.
    propspec = binding.prop2specs["prop-const"]
    verify_expected_propspec(propspec, expect_const=8)
    propspec = binding.prop2specs["prop-req"]
    verify_expected_propspec(propspec, expect_req=True)
    propspec = binding.prop2specs["prop-default"]
    verify_expected_propspec(propspec, expect_default=1)

    # New property in base_amend.yaml.
    propspec = binding.prop2specs["prop-new"]
    verify_expected_propspec(propspec)

    # Child-Binding level.
    #
    assert binding.child_binding
    child_binding = binding.child_binding
    assert {
        "child-prop-1",
        "child-prop-2",
        "child-prop-enum",
        "child-prop-req",
        "child-prop-const",
        "child-prop-default",
        "child-prop-new",
    } == set(child_binding.prop2specs.keys())
    propspec = child_binding.prop2specs["child-prop-1"]
    verify_expected_propspec(
        propspec,
        # Amended in base_amend.yaml.
        expect_desc="Overwritten description (child).",
        expect_const=0xF1,
    )
    propspec = child_binding.prop2specs["child-prop-2"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        # Amended in base_amend.yaml.
        expect_desc="New description (child).",
        expect_enum=["CHILD_EXT_FOO", "CHILD_EXT_BAR"],
        expect_default="CHILD_EXT_FOO",
    )
    propspec = child_binding.prop2specs["child-prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["CHILD_FOO", "CHILD_BAR"],
        # Amended in base_amend.yaml.
        expect_req=True,
    )
    # Inherited from base.yaml without modification.
    propspec = child_binding.prop2specs["child-prop-const"]
    verify_expected_propspec(propspec, expect_const=16)
    propspec = child_binding.prop2specs["child-prop-req"]
    verify_expected_propspec(propspec, expect_req=True)
    propspec = child_binding.prop2specs["child-prop-default"]
    verify_expected_propspec(propspec, expect_default=2)

    # New property in base_amend.yaml.
    propspec = child_binding.prop2specs["child-prop-new"]
    verify_expected_propspec(propspec)

    # GrandChild-Binding level.
    #
    assert child_binding.child_binding
    grandchild_binding = child_binding.child_binding
    assert {
        "grandchild-prop-1",
        "grandchild-prop-2",
        "grandchild-prop-enum",
        "grandchild-prop-req",
        "grandchild-prop-const",
        "grandchild-prop-default",
        "grandchild-prop-new",
    } == set(grandchild_binding.prop2specs.keys())
    propspec = grandchild_binding.prop2specs["grandchild-prop-1"]
    verify_expected_propspec(
        propspec,
        # Amended in base_amend.yaml.
        expect_desc="Overwritten description (grandchild).",
        expect_const=0xF2,
    )
    propspec = grandchild_binding.prop2specs["grandchild-prop-2"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        # Amended in base_amend.yaml.
        expect_desc="New description (grandchild).",
        expect_enum=["GRANDCHILD_EXT_FOO", "GRANDCHILD_EXT_BAR"],
        expect_default="GRANDCHILD_EXT_FOO",
    )
    propspec = grandchild_binding.prop2specs["grandchild-prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["GRANDCHILD_FOO", "GRANDCHILD_BAR"],
        # Amended in base_amend.yaml.
        expect_req=True,
    )
    # Inherited from base.yaml without modification.
    propspec = grandchild_binding.prop2specs["grandchild-prop-const"]
    verify_expected_propspec(propspec, expect_const=32)
    propspec = grandchild_binding.prop2specs["grandchild-prop-req"]
    verify_expected_propspec(propspec, expect_req=True)
    propspec = grandchild_binding.prop2specs["grandchild-prop-default"]
    verify_expected_propspec(propspec, expect_default=3)

    # New property in base_amend.yaml.
    propspec = grandchild_binding.prop2specs["grandchild-prop-new"]
    verify_expected_propspec(propspec)


def test_expect_propspecs_multi_child_binding() -> None:
    """Test including base bindings at multiple levels.

    Base specifications are included at the binding, child-binding
    and child-binding levels:

        include: base.yaml
        child-binding:
          include: base.yaml
          child-binding:
            include: base.yaml

    This test checks that we finally get the expected property specifications
    at the child-binding level.
    """
    binding = child_binding_of("test-bindings-init/base_multi.yaml")

    assert {
        # From top-level "include:" element.
        "child-prop-1",
        "child-prop-2",
        "child-prop-enum",
        # From "child-binding: include:" element.
        "prop-1",
        "prop-2",
        "prop-enum",
    } == set(binding.prop2specs.keys())

    # Inherited from base.yaml without modification.
    propspec = binding.prop2specs["child-prop-2"]
    verify_expected_propspec(propspec, expect_type="string")
    propspec = binding.prop2specs["child-prop-enum"]
    verify_expected_propspec(propspec, expect_type="string", expect_enum=["CHILD_FOO", "CHILD_BAR"])

    propspec = binding.prop2specs["child-prop-1"]
    verify_expected_propspec(
        propspec,
        expect_desc="Base child-prop 1.",
        # Amended in base_multi.yaml.
        expect_const=0xF1,
    )
    propspec = binding.prop2specs["prop-1"]
    verify_expected_propspec(
        propspec,
        expect_desc="Base property 1.",
        # Amended in base_multi.yaml.
        expect_const=0xF1,
    )
    propspec = binding.prop2specs["prop-2"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        # Amended in base_multi.yaml.
        expect_desc="New description (child).",
    )
    propspec = binding.prop2specs["prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["FOO", "BAR"],
        # Amended in base_multi.yaml.
        expect_default="FOO",
        expect_req=True,
    )


def test_expect_propspecs_multi_grandchild_binding() -> None:
    """Test including base bindings at multiple levels.

    This test checks that we finally get the expected property specifications
    at the grandchild-binding level.

    See also: test_expect_propspecs_multi_child_binding()
    """
    binding = grandchild_binding_of("test-bindings-init/base_multi.yaml")

    assert {
        # From top-level "include:" element.
        "grandchild-prop-1",
        "grandchild-prop-2",
        "grandchild-prop-enum",
        # From "child-binding: include:" element.
        "child-prop-1",
        "child-prop-2",
        "child-prop-enum",
        # From "child-binding: child-binding: include:" element.
        "prop-1",
        "prop-2",
        "prop-enum",
    } == set(binding.prop2specs.keys())

    # Inherited from base.yaml without modification.
    propspec = binding.prop2specs["grandchild-prop-2"]
    verify_expected_propspec(propspec, expect_type="string")
    propspec = binding.prop2specs["grandchild-prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["GRANDCHILD_FOO", "GRANDCHILD_BAR"],
    )
    propspec = binding.prop2specs["child-prop-2"]
    verify_expected_propspec(propspec, expect_type="string")
    propspec = binding.prop2specs["child-prop-enum"]
    verify_expected_propspec(propspec, expect_type="string", expect_enum=["CHILD_FOO", "CHILD_BAR"])

    propspec = binding.prop2specs["grandchild-prop-1"]
    verify_expected_propspec(
        propspec,
        expect_desc="Base grandchild-prop 1.",
        # Amended in base_multi.yaml.
        expect_const=0xF2,
    )
    propspec = binding.prop2specs["child-prop-1"]
    verify_expected_propspec(
        propspec,
        expect_desc="Base child-prop 1.",
        # Amended in base_multi.yaml.
        expect_const=0xF2,
    )
    propspec = binding.prop2specs["prop-1"]
    verify_expected_propspec(
        propspec,
        expect_desc="Base property 1.",
        # Amended in base_amend.yaml.
        expect_const=0xF2,
    )
    propspec = binding.prop2specs["prop-2"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        # Amended in base_amend.yaml.
        expect_desc="New description (grandchild).",
    )
    propspec = binding.prop2specs["prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["FOO", "BAR"],
        # Amended in base_amend.yaml.
        expect_req=True,
        expect_default="FOO",
    )


def test_expect_propspecs_multi_grand_grandchild_binding() -> None:
    """Test including base bindings at multiple levels.

    This test checks that we finally get the expected property specifications
    at the grand-grandchild-binding level.

    See also: test_expect_propspecs_multi_child_binding()
    """
    binding = grandchild_binding_of("test-bindings-init/base_multi.yaml").child_binding
    assert binding

    assert {
        # From "child-binding: include:" element.
        "child-prop-1",
        "child-prop-2",
        "child-prop-enum",
        # From "child-binding: child-binding: include:" element.
        "grandchild-prop-1",
        "grandchild-prop-2",
        "grandchild-prop-enum",
    } == set(binding.prop2specs.keys())

    # Inherited from base.yaml without modification.
    propspec = binding.prop2specs["child-prop-1"]
    verify_expected_propspec(propspec, expect_desc="Base child-prop 1.")
    propspec = binding.prop2specs["child-prop-2"]
    verify_expected_propspec(propspec, expect_type="string")
    propspec = binding.prop2specs["child-prop-enum"]
    verify_expected_propspec(propspec, expect_type="string", expect_enum=["CHILD_FOO", "CHILD_BAR"])
    propspec = binding.prop2specs["grandchild-prop-1"]
    verify_expected_propspec(propspec, expect_desc="Base grandchild-prop 1.")
    propspec = binding.prop2specs["grandchild-prop-2"]
    verify_expected_propspec(propspec, expect_type="string")
    propspec = binding.prop2specs["grandchild-prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["GRANDCHILD_FOO", "GRANDCHILD_BAR"],
    )


def test_expect_propspecs_diamond_binding() -> None:
    """Test property specifications produced by diamond inheritance.

    This test checks that we finally get the expected property specifications
    at the binding level.
    """
    binding = load_binding("test-bindings-init/diamond.yaml")

    assert {
        # From base.yaml, amended in base_amend.yaml (left),
        # last modified in thing.yaml (right).
        "prop-1",
        # From base.yaml, amended in base_amend.yaml (left),
        # and thing.yaml (right), last modified in diamond.yaml(top).
        "prop-enum",
        # From base.yaml, inherited in base_amend.yaml (left).
        "prop-default",
        # From thing.yaml (right).
        "prop-thing",
        # From diamond.yaml (top).
        "prop-diamond",
    } == set(binding.prop2specs.keys())

    # Inherited from base.yaml without modification.
    propspec = binding.prop2specs["prop-default"]
    verify_expected_propspec(propspec, expect_default=1)
    # Inherited from thing.yaml without modification.
    propspec = binding.prop2specs["prop-thing"]
    verify_expected_propspec(propspec, expect_desc="Thing property.")

    # New property in diamond.yaml.
    propspec = binding.prop2specs["prop-diamond"]
    verify_expected_propspec(propspec)

    propspec = binding.prop2specs["prop-1"]
    verify_expected_propspec(
        propspec,
        # From base_amend.yaml.
        expect_const=0xF0,
        # Included first wins.
        expect_desc="Overwritten description.",
        # From thing.yaml.
        expect_default=1,
    )
    propspec = binding.prop2specs["prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["FOO", "BAR"],
        # From base_amend.yaml.
        expect_req=True,
        # From diamond.yaml.
        expect_desc="Overwritten in diamond.yaml.",
        expect_default="FOO",
    )


def test_expect_propspecs_diamond_child_binding() -> None:
    """Test property specifications produced by diamond inheritance.

    This test checks that we finally get the expected property specifications
    at the child-binding level.
    """
    binding = child_binding_of("test-bindings-init/diamond.yaml")

    assert {
        # From base.yaml, amended in base_amend.yaml (left),
        # last modified in thing.yaml (right).
        "child-prop-1",
        # From base.yaml, amended in base_amend.yaml (left),
        # and thing.yaml (right), last modified in diamond.yaml(top).
        "child-prop-enum",
        # From base.yaml, inherited in base_amend.yaml (left).
        "child-prop-default",
        # From thing.yaml (right).
        "child-prop-thing",
        # From diamond.yaml (top).
        "child-prop-diamond",
    } == set(binding.prop2specs.keys())

    propspec = binding.prop2specs["child-prop-1"]
    verify_expected_propspec(
        propspec,
        # From base_amend.yaml.
        expect_const=0xF1,
        # Included first wins.
        expect_desc="Overwritten description (child).",
        # From thing.yaml.
        expect_default=2,
    )

    propspec = binding.prop2specs["child-prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["CHILD_FOO", "CHILD_BAR"],
        # From base_amend.yaml.
        # ORed with thing.yaml.
        expect_req=True,
        # From diamond.yaml.
        expect_default="CHILD_FOO",
        expect_desc="Overwritten in diamond.yaml (child).",
    )

    # Inherited from base.yaml without modification.
    propspec = binding.prop2specs["child-prop-default"]
    verify_expected_propspec(propspec, expect_default=2)
    # Inherited from thing.yaml without modification.
    propspec = binding.prop2specs["child-prop-thing"]
    verify_expected_propspec(propspec, expect_desc="Thing child-binding property.")

    # New property in diamond.yaml.
    propspec = binding.prop2specs["child-prop-diamond"]
    verify_expected_propspec(propspec)


def test_expect_propspecs_diamond_grandchild_binding() -> None:
    """Test property specifications produced by diamond inheritance.

    This test checks that we finally get the expected property specifications
    at the grandchild-binding level.
    """
    binding = grandchild_binding_of("test-bindings-init/diamond.yaml")

    assert {
        # From base.yaml, amended in base_amend.yaml (left),
        # last modified in thing.yaml (right).
        "grandchild-prop-1",
        # From base.yaml, amended in base_amend.yaml (left),
        # last modified in diamond.yaml (top).
        "grandchild-prop-enum",
        # From base.yaml, inherited in base_amend.yaml (left).
        "grandchild-prop-default",
        # From thing.yaml (right).
        "grandchild-prop-thing",
        # From diamond.yaml (top).
        "grandchild-prop-diamond",
    } == set(binding.prop2specs.keys())

    propspec = binding.prop2specs["grandchild-prop-1"]
    verify_expected_propspec(
        propspec,
        # From base_amend.yaml.
        expect_const=0xF2,
        # Included first wins.
        expect_desc="Overwritten description (grandchild).",
        # From thing.yaml.
        expect_default=3,
    )

    propspec = binding.prop2specs["grandchild-prop-enum"]
    verify_expected_propspec(
        propspec,
        expect_type="string",
        expect_enum=["GRANDCHILD_FOO", "GRANDCHILD_BAR"],
        # From base_amend.yaml.
        # ORed with thing.yaml.
        expect_req=True,
        # From diamond.yaml.
        expect_default="GRANDCHILD_FOO",
        expect_desc="Overwritten in diamond.yaml (grandchild).",
    )

    # Inherited from base.yaml without modification.
    propspec = binding.prop2specs["grandchild-prop-default"]
    verify_expected_propspec(propspec, expect_default=3)
    # Inherited from thing.yaml without modification.
    propspec = binding.prop2specs["grandchild-prop-thing"]
    verify_expected_propspec(propspec, expect_desc="Thing grandchild-binding property.")

    # New property in diamond.yaml.
    propspec = binding.prop2specs["grandchild-prop-diamond"]
    verify_expected_propspec(propspec)


def test_binding_description_overwrite() -> None:
    """Test whether we can overwrite a binding's description.

    Included files can't overwrite the description set by the including binding
    (despite that "description:" appears before "include:").

    This seems consistent: the top-level binding file "wins".
    """
    binding = load_binding("test-bindings-init/compat_desc.yaml")
    assert binding.description == "Binding description."

    binding = load_binding("test-bindings-init/compat_desc_multi.yaml")
    assert binding.description == "Binding description (multi)."


def test_binding_compat_overwrite() -> None:
    """Test whether we can overwrite a binding's compatible string.

    Included files can't overwrite the compatible string set by the
    including binding (despite that "compatible:" appears before "include:").

    This seems consistent: the top-level binding file "wins".
    """
    binding = load_binding("test-bindings-init/compat_desc.yaml")
    assert binding.compatible == "vnd,compat-desc"

    binding = load_binding("test-bindings-init/compat_desc_multi.yaml")
    assert binding.compatible == "vnd,compat-desc-multi"


def test_child_binding_description_overwrite() -> None:
    """Test whether we can overwrite a child-binding's description.

    An including binding can overwrite an inherited child-binding's description.

    When we include multiple files overwriting the description
    at the child-binding level, the first "wins".
    """
    child_binding = child_binding_of("test-bindings-init/compat_desc.yaml")
    # Overwrite inherited description.
    assert child_binding.description == "Child-binding description."

    child_binding = child_binding_of("test-bindings-init/compat_desc_multi.yaml")
    # When inherited multiple times, the first "description:" wins.
    assert child_binding.description == "Child-binding description (base)."


def test_child_binding_compat_overwrite() -> None:
    """Test whether we can overwrite a child-binding's compatible string.

    An including binding can overwrite an inherited child-binding's
    compatible string.

    When we include multiple files overwriting the compatible string
    at the child-binding level, the first "wins".
    """
    child_binding = child_binding_of("test-bindings-init/compat_desc.yaml")
    # Overwrite inherited description.
    assert child_binding.compatible == "vnd,child-compat-desc"

    child_binding = child_binding_of("test-bindings-init/compat_desc_multi.yaml")
    # When inherited multiple times, the first "compatible:" wins.
    assert child_binding.compatible == "vnd,child-compat-desc-base"


def test_grandchild_binding_description_overwrite() -> None:
    """Test whether we can overwrite a grandchild-binding's description.

    See also: test_child_binding_description_overwrite()
    """
    grandchild_binding = grandchild_binding_of("test-bindings-init/compat_desc.yaml")
    assert grandchild_binding.description == "Grandchild-binding description."

    grandchild_binding = grandchild_binding_of("test-bindings-init/compat_desc_multi.yaml")
    assert grandchild_binding.description == "Grandchild-binding description (base)."


def test_grandchild_binding_compat_overwrite() -> None:
    """Test whether we can overwrite a grandchild-binding's compatible string.

    See also: test_child_binding_compat_overwrite()
    """
    grandchild_binding = grandchild_binding_of("test-bindings-init/compat_desc.yaml")
    # Overwrite inherited description.
    assert grandchild_binding.compatible == "vnd,grandchild-compat-desc"

    grandchild_binding = grandchild_binding_of("test-bindings-init/compat_desc_multi.yaml")
    # When inherited multiple times, the first "compatible:" wins.
    assert grandchild_binding.compatible == "vnd,grandchild-compat-desc-base"


def test_filter_inherited_propspecs_basics() -> None:
    """Test the basics of filtering properties inherited via an intermediary
    binding file.

    Use-case "B filters I includes X":
    - X is a base binding file, specifying common properties
    - I is an intermediary binding file, which includes X without modification
      nor filter
    - B includes I, filtering the properties it chooses to inherit
      with an allowlist or a blocklist

    Checks, up to the grandchild-binding level, that the properties inherited
    from X via I are actually filtered as B intends to.
    """
    # Binding level.
    binding = load_binding("test-bindings-init/simple_allowlist.yaml")
    # Allowed properties.
    assert {"prop-1", "prop-2"} == set(binding.prop2specs.keys())
    binding = load_binding("test-bindings-init/simple_blocklist.yaml")
    # Non blocked properties.
    assert {"prop-2", "prop-3"} == set(binding.prop2specs.keys())

    # Child-binding level.
    child_binding = child_binding_of("test-bindings-init/simple_allowlist.yaml")
    # Allowed properties.
    assert {"child-prop-1", "child-prop-2"} == set(child_binding.prop2specs.keys())
    child_binding = child_binding_of("test-bindings-init/simple_blocklist.yaml")
    # Non blocked properties.
    assert {"child-prop-2", "child-prop-3"} == set(child_binding.prop2specs.keys())

    # GrandChild-binding level.
    grandchild_binding = grandchild_binding_of("test-bindings-init/simple_allowlist.yaml")
    # Allowed properties.
    assert {"grandchild-prop-1", "grandchild-prop-2"} == set(grandchild_binding.prop2specs.keys())
    grandchild_binding = grandchild_binding_of("test-bindings-init/simple_blocklist.yaml")
    # Non blocked properties.
    assert {"grandchild-prop-2", "grandchild-prop-3"} == set(grandchild_binding.prop2specs.keys())


def test_filter_inherited_propspecs_among_allowed() -> None:
    """Test filtering properties which have been allowed by an intermediary
    binding file.

    Complementary to test_filter_inherited_propspecs_basics().

    Use-case "B filters I filters X":
    - X is a base binding file, specifying common properties
    - I is an intermediary binding file, filtering the properties specified
      in X with an allowlist
    - B includes I, filtering the properties it chooses to inherit
      also with an allowlist

    Checks, up to the grandchild-binding level, that B inherits the properties
    specified in X which are first allowed in I, then also allowed in B.

    For that, we check that if B allows only properties that are not allowed in I,
    we then end up with no property at all.
    """
    binding = load_binding("test-bindings-init/filter_among_allowed.yaml")
    assert not set(binding.prop2specs.keys())
    assert binding.child_binding
    child_binding = binding.child_binding
    assert not set(child_binding.prop2specs.keys())
    assert child_binding.child_binding
    grandchild_binding = child_binding.child_binding
    assert not set(grandchild_binding.prop2specs.keys())


def test_filter_inherited_propspecs_among_notblocked() -> None:
    """Test filtering properties which have not been blocked by an intermediary
    binding file.

    Complementary to test_filter_inherited_propspecs_basics().

    Use-case "B filters I filters X":
    - X is a base binding file, specifying common properties
    - I is an intermediary binding file, filtering the properties specified
      in X with a blocklist
    - B includes I, filtering the properties it chooses to inherit
      also with a blocklist

    Checks, up to the grandchild-binding level, that B inherits the properties
    specified in X which are not blocked in I, then neither blocked in B.

    For that, we check that if B blocks all properties that are not blocked in I,
    we then end up with no property at all.
    """
    binding = load_binding("test-bindings-init/filter_among_notblocked.yaml")
    assert not set(binding.prop2specs.keys())
    assert binding.child_binding
    child_binding = binding.child_binding
    assert not set(child_binding.prop2specs.keys())
    assert child_binding.child_binding
    grandchild_binding = child_binding.child_binding
    assert not set(grandchild_binding.prop2specs.keys())


def test_filter_inherited_propspecs_allows_notblocked() -> None:
    """Test allowing properties which have not been blocked by an intermediary
    binding file.

    Complementary to test_filter_inherited_propspecs_basics().

    Use-case "B filters I filters X":
    - X is a base binding file, specifying common properties
    - I is an intermediary binding file, filtering the properties specified
      in X with a blocklist
    - B includes I, filtering the properties it chooses to inherit
      also with an allowlist

    Checks, up to the grandchild-binding level, that B inherits the properties
    specified in X which are not blocked in I, then allowed in B.

    Permits to verify we can apply both "property-allowlist"
    and "property-blocklist" filters to the same file,
    as long as they're not applied simultaneously
    (i.e. within the same YAML "include:" map).
    """
    binding = load_binding("test-bindings-init/filter_allows_notblocked.yaml")
    assert {"prop-2"} == set(binding.prop2specs.keys())
    assert binding.child_binding
    child_binding = binding.child_binding
    assert {"child-prop-2"} == set(child_binding.prop2specs.keys())
    assert child_binding.child_binding
    grandchild_binding = child_binding.child_binding
    assert {"grandchild-prop-2"} == set(grandchild_binding.prop2specs.keys())


def test_invalid_binding_type_override() -> None:
    """An including binding should not try to override the "type:"
    of an inherited property.

    Tested up to the grandchild-binding level.
    """
    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_proptype.yaml")
    assert "prop-1" in str(e)
    assert "'int' replaced with 'string'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_child_proptype.yaml")
    assert "child-prop-1" in str(e)
    assert "'int' replaced with 'string'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_grandchild_proptype.yaml")
    assert "grandchild-prop-1" in str(e)
    assert "'int' replaced with 'string'" in str(e)


def test_invalid_binding_const_override() -> None:
    """An including binding should not try to override the "const:" value
    in a property specification inherited from an included file.

    Tested up to the grandchild-binding level.
    """
    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_propconst.yaml")
    assert "prop-const" in str(e)
    assert "'8' replaced with '999'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_child_propconst.yaml")
    assert "child-prop-const" in str(e)
    assert "'16' replaced with '999'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_grandchild_propconst.yaml")
    assert "grandchild-prop-const" in str(e)
    assert "'32' replaced with '999'" in str(e)


def test_invalid_binding_required_override() -> None:
    """An including binding should not try to override "required: true"
    in a property specification inherited from an included file.

    Tested up to the grandchild-binding level.
    """
    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_propreq.yaml")
    assert "prop-req" in str(e)
    assert "'True' replaced with 'False'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_child_propreq.yaml")
    assert "child-prop-req" in str(e)
    assert "'True' replaced with 'False'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_grandchild_propreq.yaml")
    assert "grandchild-prop-req" in str(e)
    assert "'True' replaced with 'False'" in str(e)


def test_invalid_binding_default_override() -> None:
    """An including binding should not try to override the "default:" value
    in a property specification inherited from an included file.

    Tested up to the grandchild-binding level.
    """
    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_propdefault.yaml")
    assert "prop-default" in str(e)
    assert "'1' replaced with '999'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_child_propdefault.yaml")
    assert "child-prop-default" in str(e)
    assert "'2' replaced with '999'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_grandchild_propdefault.yaml")
    assert "grandchild-prop-default" in str(e)
    assert "'3' replaced with '999'" in str(e)


def test_invalid_binding_enum_override() -> None:
    """An including binding should not try to override the "enum:" values
    in a property specification inherited from an included file.

    Tested up to the grandchild-binding level.
    """
    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_propenum.yaml")
    assert "prop-enum" in str(e)
    assert "'['FOO', 'BAR']' replaced with '['OTHER_FOO', 'OTHER_BAR']'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_child_propenum.yaml")
    assert "child-prop-enum" in str(e)
    assert "'['CHILD_FOO', 'CHILD_BAR']' replaced with '['OTHER_FOO', 'OTHER_BAR']'" in str(e)

    with pytest.raises(edtlib.EDTError) as e:
        load_binding("test-bindings-init/invalid_grandchild_propenum.yaml")
    assert "grandchild-prop-enum" in str(e)
    assert (
        "'['GRANDCHILD_FOO', 'GRANDCHILD_BAR']' replaced with '['OTHER_FOO', 'OTHER_BAR']'"
        in str(e)
    )


def test_bindings_propspecs_consistency() -> None:
    """Verify property specifications consistency.

    Consistency is recursively checked for all defined properties,
    from top-level binding files down to their child bindings.

    Consistency is checked with:
        Binding.raw["properties"][prop] == Binding.prop2specs[prop]._raw

    See verify_binding_propspecs_consistency().
    """
    binding = load_binding("test-bindings-init/base_inherit.yaml")
    verify_binding_propspecs_consistency(binding)

    binding = load_binding("test-bindings-init/base_amend.yaml")
    verify_binding_propspecs_consistency(binding)

    binding = load_binding("test-bindings-init/base_multi.yaml")
    verify_binding_propspecs_consistency(binding)

    binding = load_binding("test-bindings-init/thing.yaml")
    verify_binding_propspecs_consistency(binding)

    binding = load_binding("test-bindings-init/diamond.yaml")
    verify_binding_propspecs_consistency(binding)


# Borrowed from test_edtlib.py.
@contextlib.contextmanager
def _from_here() -> Generator[None, None, None]:
    cwd = os.getcwd()
    try:
        os.chdir(os.path.dirname(__file__))
        yield
    finally:
        os.chdir(cwd)


def _basename(path: str | None) -> str:
    return os.path.basename(path or "?")
