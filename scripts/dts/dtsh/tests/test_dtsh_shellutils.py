# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.shellutils module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=pointless-statement
# pylint: disable=too-many-statements
# pylint: disable=too-many-statements

from typing import List, Tuple

import pytest

from dtsh.shell import DTSh, DTShCommand, DTShError
from dtsh.modelutils import (
    # Text-based criteria.
    DTNodeWithPath,
    DTNodeWithName,
    DTNodeWithUnitName,
    DTNodeWithCompatible,
    DTNodeWithBinding,
    DTNodeWithVendor,
    DTNodeWithDescription,
    DTNodeWithBus,
    DTNodeWithOnBus,
    DTNodeWithDeviceLabel,
    DTNodeWithNodeLabel,
    DTNodeWithAlias,
    DTNodeWithChosen,
    DTNodeAlsoKnownAs,
)
from dtsh.shellutils import (
    DTShFlagReverse,
    DTShFlagEnabledOnly,
    DTShFlagPager,
    DTShFlagRegex,
    DTShFlagIgnoreCase,
    DTShFlagCount,
    DTShFlagTreeLike,
    DTShFlagNoChildren,
    DTShFlagRecursive,
    DTShFlagLogicalOr,
    DTShFlagLogicalNot,
    DTShArgFixedDepth,
    # Arguments for text-based criteria.
    DTShArgTextCriterion,
    DTShArgNodeWithPath,
    DTShArgNodeWithStatus,
    DTShArgNodeWithName,
    DTShArgNodeWithUnitName,
    DTShArgNodeWithCompatible,
    DTShArgNodeWithBinding,
    DTShArgNodeWithVendor,
    DTShArgNodeWithDescription,
    DTShArgNodeWithBus,
    DTShArgNodeWithOnBus,
    DTShArgNodeWithDeviceLabel,
    DTShArgNodeWithNodeLabel,
    DTShArgNodeWithAlias,
    DTShArgNodeWithChosen,
    DTShArgNodeAlsoKnownAs,
    # Arguments for integer-based criteria.
    DTShArgIntCriterion,
    DTShArgNodeWithUnitAddr,
    DTShArgNodeWithIrqNumber,
    DTShArgNodeWithIrqPriority,
    DTShArgNodeWithRegAddr,
    DTShArgNodeWithRegSize,
    DTShArgNodeWithBindingDepth,
    DTShArgOrderBy,
    DTShParamDTPath,
    DTShParamDTPaths,
    DTShParamAlias,
    DTShParamChosen,
    DTSH_NODE_ORDER_BY,
    DTSH_ARG_NODE_CRITERIA,
)

from .dtsh_uthelpers import DTShTests


def test_dtshflag_reverse() -> None:
    flag = DTShFlagReverse()
    DTShTests.check_flag(flag)


def test_dtshflag_enabled_only() -> None:
    flag = DTShFlagEnabledOnly()
    DTShTests.check_flag(flag)


def test_dtshflag_pager() -> None:
    flag = DTShFlagPager()
    DTShTests.check_flag(flag)


def test_dtshflag_regex() -> None:
    flag = DTShFlagRegex()
    DTShTests.check_flag(flag)


def test_dtshflag_ignore_case() -> None:
    flag = DTShFlagIgnoreCase()
    DTShTests.check_flag(flag)


def test_dtshflag_count() -> None:
    flag = DTShFlagCount()
    DTShTests.check_flag(flag)


def test_dtshflag_tree_like() -> None:
    flag = DTShFlagTreeLike()
    DTShTests.check_flag(flag)


def test_dtshflag_nochildren() -> None:
    flag = DTShFlagNoChildren()
    DTShTests.check_flag(flag)


def test_dtshflag_recursive() -> None:
    flag = DTShFlagRecursive()
    DTShTests.check_flag(flag)


def test_dtshflag_logical_or() -> None:
    flag = DTShFlagLogicalOr()
    DTShTests.check_flag(flag)


def test_dtshflag_logical_not() -> None:
    flag = DTShFlagLogicalNot()
    DTShTests.check_flag(flag)


def test_dtsharg_fixed_depth() -> None:
    arg = DTShArgFixedDepth()
    # Default value.
    assert 0 == arg.depth

    DTShTests.check_arg(arg, parsed="2")
    assert 2 == arg.depth

    with pytest.raises(DTShError):
        arg.parsed("not a number")

    with pytest.raises(DTShError):
        # Negative numbers not allowed.
        arg.parsed("-2")


def test_dtsharg_criterion_integer_expr() -> None:
    # Criteria (args) with the attributes they depend on.
    arg_on_attr: List[Tuple[DTShArgIntCriterion, str]] = [
        (DTShArgNodeWithUnitAddr(), "unit_addr"),
        (DTShArgNodeWithIrqNumber(), "interrupts"),
        (DTShArgNodeWithIrqPriority(), "interrupts"),
        (DTShArgNodeWithRegAddr(), "registers"),
        (DTShArgNodeWithRegSize(), "registers"),
        (DTShArgNodeWithBindingDepth(), "binding"),
    ]

    # Check that no one has been forgotten.
    assert len(
        list(
            arg
            for arg in DTSH_ARG_NODE_CRITERIA
            if isinstance(arg, DTShArgIntCriterion)
        )
    ) == len(arg_on_attr)

    for arg, attr in arg_on_attr:
        # Check and parse "*": should match all nodes for which
        # the attribute has a value.
        DTShTests.check_arg(arg, parsed="*")

        criterion = arg.get_criterion()
        assert criterion

        for node in DTShTests.get_sample_dtmodel():
            attval = getattr(node, attr)
            if (attval is not None) and (attval != []):
                assert criterion.match(node)
            else:
                assert not criterion.match(node)

        with pytest.raises(DTShError):
            # Should be an int or "*".
            arg.parsed("X")

        with pytest.raises(DTShError):
            # Invalid operator "X".
            arg.parsed("X 2")


def test_dtsharg_with_unit_addr() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/i2c@40003000/bme680@76"]
    assert 0x76 == node.unit_addr
    arg = DTShArgNodeWithUnitAddr()

    # Without operator, defaults to equality.
    arg.parsed("0x76")
    criterion = arg.get_criterion()
    assert criterion
    assert criterion.match(node)
    # Test once with another unit address.
    assert not criterion.match(dtmodel["/soc/i2c@40003000"])

    # With comparison operators.
    arg.parsed("= 0x76")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("!= 0x76")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed(">= 0x76")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("<= 0x76")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed(">= 0")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("<= 0")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed("> 0x76")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed("< 0x76")
    assert not arg.get_criterion().match(node)  # type: ignore


def test_dtsharg_with_irq_number() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/timer@4000a000"]
    assert node.interrupts
    10 == node.interrupts[0].number
    arg = DTShArgNodeWithIrqNumber()

    # Without operator, defaults to equality.
    arg.parsed("10")
    criterion = arg.get_criterion()
    assert criterion
    assert criterion.match(node)
    # Test once with another IRQ number.
    assert not criterion.match(dtmodel["/soc/spi@40004000"])

    # With comparison operators.
    arg.parsed("= 10")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("!= 10")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed(">= 10")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("<= 10")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed(">= 0")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("<= 0")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed("> 10")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed("< 10")
    assert not arg.get_criterion().match(node)  # type: ignore


def test_dtsharg_with_irq_priority() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/timer@4000a000"]
    assert node.interrupts
    assert 1 == node.interrupts[0].priority
    arg = DTShArgNodeWithIrqPriority()

    # Without operator, defaults to equality.
    arg.parsed("1")
    criterion = arg.get_criterion()
    assert criterion
    assert criterion.match(node)
    # Test once with another IRQ priority.
    assert not criterion.match(dtmodel["/soc/gpiote@40006000"])

    # With comparison operators.
    arg.parsed("= 1")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("!= 1")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed(">= 1")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("<= 1")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed(">= 0")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("<= 0")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed("> 1")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed("< 1")
    assert not arg.get_criterion().match(node)  # type: ignore


def test_dtsharg_with_reg_addr() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/gpio@50000000"]
    assert [0x50000000, 0x50000500] == [reg.address for reg in node.registers]

    arg = DTShArgNodeWithRegAddr()

    # Without operator, defaults to equality.
    arg.parsed("0x50000000")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("0x50000500")
    assert arg.get_criterion().match(node)  # type: ignore
    # Test once with another address.
    assert not arg.get_criterion().match(dtmodel["/soc/gpiote@40006000"])  # type: ignore

    # With comparison operators.
    arg.parsed("= 0x50000000")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("= 0x50000500")
    assert arg.get_criterion().match(node)  # type: ignore

    # Values are mutually exclusive.
    arg.parsed("!= 0x50000000")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("!= 0x50000500")
    assert arg.get_criterion().match(node)  # type: ignore

    arg.parsed(">= 0x50000000")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed(">= 0x50000500")
    assert arg.get_criterion().match(node)  # type: ignore

    arg.parsed("<= 0x50000000")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("<= 0x50000500")
    assert arg.get_criterion().match(node)  # type: ignore

    arg.parsed("> 0x50000000")
    assert arg.get_criterion().match(node)  # type: ignore
    arg.parsed("> 0x50000500")
    assert not arg.get_criterion().match(node)  # type: ignore

    arg.parsed("< 0x50000000")
    assert not arg.get_criterion().match(node)  # type: ignore
    arg.parsed("< 0x50000500")
    assert arg.get_criterion().match(node)  # type: ignore


def test_dtsharg_with_reg_size() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    dt_gpio = dtmodel["/soc/gpio@50000300"]
    assert [512, 768] == [reg.size for reg in dt_gpio.registers]
    dt_clock = dtmodel["/soc/clock@40000000"]
    # 4 kB.
    assert [0x1000] == [reg.size for reg in dt_clock.registers]
    dt_flash = dtmodel["/soc/flash-controller@4001e000/flash@0"]
    # 1 MB.
    assert [0x100000] == [reg.size for reg in dt_flash.registers]

    arg = DTShArgNodeWithRegSize()

    # Without operator, defaults to equality.
    arg.parsed("512")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("768")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("0x1000")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("4k")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("4kB")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("0x100000")
    assert arg.get_criterion().match(dt_flash)  # type: ignore
    arg.parsed("1M")
    assert arg.get_criterion().match(dt_flash)  # type: ignore
    arg.parsed("1MB")
    assert arg.get_criterion().match(dt_flash)  # type: ignore

    # Equality with comparison operators.
    arg.parsed("= 512")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("= 768")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("= 0x1000")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("= 4k")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("= 4kB")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("= 0x100000")
    assert arg.get_criterion().match(dt_flash)  # type: ignore
    arg.parsed("= 1M")
    assert arg.get_criterion().match(dt_flash)  # type: ignore
    arg.parsed("= 1MB")
    assert arg.get_criterion().match(dt_flash)  # type: ignore

    # Other comparison operators.
    arg.parsed(">= 512")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed(">= 768")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed(">= 0x1000")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed(">= 4k")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed(">= 1M")
    assert arg.get_criterion().match(dt_flash)  # type: ignore
    arg.parsed("<= 512")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("<= 768")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("<= 0x1000")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("<= 4k")
    assert arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("<= 1M")
    assert arg.get_criterion().match(dt_flash)  # type: ignore

    arg.parsed("> 512")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("> 768")
    assert not arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("> 0x1000")
    assert not arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("> 4k")
    assert not arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("> 1M")
    assert not arg.get_criterion().match(dt_flash)  # type: ignore

    arg.parsed("< 512")
    assert not arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("< 768")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("< 0x1000")
    assert not arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("< 4k")
    assert not arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("< 1M")
    assert not arg.get_criterion().match(dt_flash)  # type: ignore

    arg.parsed("!= 0x1000")
    assert not arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("!= 4k")
    assert not arg.get_criterion().match(dt_clock)  # type: ignore
    arg.parsed("!= 1M")
    assert not arg.get_criterion().match(dt_flash)  # type: ignore
    # Values are mutually exclusive.
    arg.parsed("!= 512")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore
    arg.parsed("!= 768")
    assert arg.get_criterion().match(dt_gpio)  # type: ignore


def test_dtsharg_criterion_text_pattern() -> None:
    # Criteria (args) with the attributes they depend on.
    arg_on_attr: List[Tuple[DTShArgTextCriterion, str]] = [
        (DTShArgNodeWithPath(), "path"),
        (DTShArgNodeWithStatus(), "status"),
        (DTShArgNodeWithName(), "name"),
        (DTShArgNodeWithUnitName(), "unit_name"),
        (DTShArgNodeWithCompatible(), "compatibles"),
        (DTShArgNodeWithBinding(), "binding"),
        (DTShArgNodeWithVendor(), "vendor"),
        (DTShArgNodeWithDescription(), "description"),
        (DTShArgNodeWithBus(), "buses"),
        (DTShArgNodeWithOnBus(), "on_bus"),
        (DTShArgNodeWithDeviceLabel(), "label"),
        (DTShArgNodeWithNodeLabel(), "labels"),
        (DTShArgNodeWithAlias(), "aliases"),
        (DTShArgNodeWithChosen(), "chosen"),
    ]

    # Check that no one has been forgotten.
    assert (
        len(
            list(
                arg
                for arg in DTSH_ARG_NODE_CRITERIA
                if isinstance(arg, DTShArgTextCriterion)
            )
        )
        # DTShArgNodeAlsoKnownAs may be provided by several attributes,
        # and is not in our list.
        == len(arg_on_attr) + 1
    )

    for arg, attr in arg_on_attr:
        # Check and parse "*": should match all nodes for which
        # the attribute has a value.
        DTShTests.check_arg(arg, parsed="*")

        criterion = arg.get_criterion()
        assert criterion

        for node in DTShTests.get_sample_dtmodel():
            attval = getattr(node, attr)
            if (attval is not None) and (attval != []):
                assert criterion.match(node)
            else:
                assert not criterion.match(node)

        with pytest.raises(DTShError):
            # RE strict: "*" is a repeat qualifier, not a wild-card,
            # and there's nothing to repeat.
            arg.get_criterion(re_strict=True)


def test_dtsharg_with_path() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithPath) is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithPath()

    pattern = "timer"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithPath(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*Timer*"
    ignore_case = True

    raw_criterion = DTNodeWithPath(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*Timer.*"
    re_strict = True

    raw_criterion = DTNodeWithPath(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_status() -> None:
    dt = DTShTests.get_sample_dtmodel()
    # Enabled.
    dt_i2c0 = dt.labeled_nodes["i2c0"]
    # Disabled.
    dt_i2c1 = dt.labeled_nodes["i2c1"]
    arg = DTShArgNodeWithStatus()

    arg.parsed("ok")
    criterion = arg.get_criterion()
    assert criterion
    assert criterion.match(dt_i2c0)
    assert not criterion.match(dt_i2c1)

    arg.parsed("okay")
    criterion = arg.get_criterion()
    assert criterion
    assert criterion.match(dt_i2c0)
    assert not criterion.match(dt_i2c1)

    arg.parsed("dis")
    criterion = arg.get_criterion()
    assert criterion
    assert not criterion.match(dt_i2c0)
    assert criterion.match(dt_i2c1)


def test_dtsharg_with_name() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithName) is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithName()

    pattern = "timer"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithName(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*Timer*"
    ignore_case = True

    raw_criterion = DTNodeWithName(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*Timer.*"
    re_strict = True

    raw_criterion = DTNodeWithName(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_unit_name() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithUnitName) is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithUnitName()

    pattern = "timer"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithUnitName(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*Timer*"
    ignore_case = True

    raw_criterion = DTNodeWithUnitName(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*Timer.*"
    re_strict = True

    raw_criterion = DTNodeWithUnitName(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_compatible() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithCompatible)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithCompatible()

    pattern = "nrf-twi"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithCompatible(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*nRF*"
    ignore_case = True

    raw_criterion = DTNodeWithCompatible(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*nRF.*"
    re_strict = True

    raw_criterion = DTNodeWithCompatible(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_binding() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithBinding)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithBinding()

    pattern = "bosch"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithBinding(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*Sensor*"
    ignore_case = True

    raw_criterion = DTNodeWithBinding(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*Sensor.*"
    re_strict = True

    raw_criterion = DTNodeWithBinding(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_vendor() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithVendor)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithVendor()

    pattern = "bosch"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithVendor(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*gmbh*"
    ignore_case = True

    raw_criterion = DTNodeWithVendor(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*gmbh.*"
    re_strict = True

    raw_criterion = DTNodeWithVendor(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_device_label() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithDeviceLabel)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithDeviceLabel()

    pattern = "led"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithDeviceLabel(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*LED*"
    ignore_case = True

    raw_criterion = DTNodeWithDeviceLabel(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*LED.*"
    re_strict = True

    raw_criterion = DTNodeWithDeviceLabel(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_node_label() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithNodeLabel)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithNodeLabel()

    pattern = "led"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithNodeLabel(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*LED*"
    ignore_case = True

    raw_criterion = DTNodeWithNodeLabel(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*LED.*"
    re_strict = True

    raw_criterion = DTNodeWithNodeLabel(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_alias() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithAlias)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithAlias()

    pattern = "i2c"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithAlias(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*I2C*"
    ignore_case = True

    raw_criterion = DTNodeWithAlias(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*I2C.*"
    re_strict = True

    raw_criterion = DTNodeWithAlias(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_chosen() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithChosen)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithChosen()

    pattern = "entropy"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithChosen(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*Zephyr*"
    ignore_case = True

    raw_criterion = DTNodeWithChosen(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*Zephyr.*"
    re_strict = True

    raw_criterion = DTNodeWithChosen(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_bus() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithBus)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithBus()

    pattern = "i2c"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithBus(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*I2C*"
    ignore_case = True

    raw_criterion = DTNodeWithBus(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = r"I[\d]C"
    re_strict = True

    raw_criterion = DTNodeWithBus(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_on_bus() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithOnBus)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithOnBus()

    pattern = "i2c"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithOnBus(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*I2C*"
    ignore_case = True

    raw_criterion = DTNodeWithOnBus(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = r"I[\d]C"
    re_strict = True

    raw_criterion = DTNodeWithOnBus(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_desc() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeWithDescription)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeWithDescription()

    pattern = "BME"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeWithDescription(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*Sensor*"
    ignore_case = True

    raw_criterion = DTNodeWithDescription(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*Sensor.*"
    re_strict = True

    raw_criterion = DTNodeWithDescription(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_with_aka() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Here we only test arguments passing,
    # the criterion itself (DTNodeAlsoKnownAs)
    # is tested in test_dtsh_modelutils.
    arg = DTShArgNodeAlsoKnownAs()

    pattern = "led"
    re_strict = False
    ignore_case = False

    raw_criterion = DTNodeAlsoKnownAs(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = "*LED*"
    ignore_case = True

    raw_criterion = DTNodeAlsoKnownAs(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)

    pattern = ".*LED.*"
    re_strict = True

    raw_criterion = DTNodeAlsoKnownAs(
        pattern, re_strict=re_strict, ignore_case=ignore_case
    )
    arg.parsed(pattern)
    arg_criterion = arg.get_criterion(
        re_strict=re_strict, ignore_case=ignore_case
    )
    assert arg_criterion
    for node in nodes:
        assert raw_criterion.match(node) == arg_criterion.match(node)


def test_dtsharg_orderby() -> None:
    arg = DTShArgOrderBy()

    # Here we only test the argument meta-data and parsing,
    # the sorters are tested in test_dtsh_modelutils.
    for key, order_by in DTSH_NODE_ORDER_BY.items():
        assert key == order_by.key
        assert order_by.brief

        assert not arg.isset
        assert arg.sorter is None

        DTShTests.check_arg(arg, parsed=key)
        assert arg.isset
        assert order_by.sorter is arg.sorter
        arg.reset()


def test_dtsharg_orderby_autocomp() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [])
    arg = DTShArgOrderBy()

    # All argument candidate values.
    assert sorted(DTSH_NODE_ORDER_BY.keys(), key=lambda x: x.lower()) == [
        state.rlstr for state in arg.autocomp("", sh)
    ]

    # Exact match.
    for key in DTSH_NODE_ORDER_BY:
        assert key == arg.autocomp(key, sh)[0].rlstr

    # No match.
    assert not arg.autocomp("not a key", sh)


def test_dtshparam_dtpath() -> None:
    param = DTShParamDTPath()
    assert param.brief
    assert "[PATH]" == param.usage
    # Default value.
    assert "" == param.path

    # Parameter's state and multiplicity.
    DTShTests.check_param(param)

    # Won't fault, the path parameter is returned unchanged,
    # path resolution is deferred to command execution.
    param.parsed(["value"])
    assert "value" == param.path


def test_dtshparam_dtpath_autocomp() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [])
    param = DTShParamDTPath()

    # Auto-complete DT path.
    assert sorted([node.name for node in dtmodel.root.children]) == [
        state.rlstr for state in param.autocomp("", sh)
    ]
    # Exact match.
    assert ["soc"] == [state.rlstr for state in param.autocomp("so", sh)]
    # No match.
    assert [] == param.autocomp("not/a/path", sh)

    # Auto-complete DT label.
    assert ["&i2c0", "&i2c0_default", "&i2c0_sleep"] == [
        state.rlstr for state in param.autocomp("&i2c0", sh)
    ]
    # Exact match, substitute with path.
    assert ["/pin-controller/i2c0_sleep"] == [
        state.rlstr for state in param.autocomp("&i2c0_s", sh)
    ]


def test_dtshparam_dtpaths() -> None:
    param = DTShParamDTPaths()
    assert param.brief
    assert "[PATH ...]" == param.usage
    # Default value.
    assert [""] == param.paths

    # Parameter's state and multiplicity.
    DTShTests.check_param(param)

    # Won't fault, the path parameter is returned unchanged,
    # path resolution is deferred to command execution.
    param.parsed(["value"])
    assert ["value"] == param.paths


def test_dtshparam_dtpaths_expand() -> None:
    param = DTShParamDTPaths()
    flag_enable_only = DTShFlagEnabledOnly()
    cmd = DTShCommand("cmd", "", [flag_enable_only], param)
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])

    assert [DTSh.PathExpansion(".", [sh.cwd])] == param.expand(cmd, sh)

    param.parsed(["."])
    assert [DTSh.PathExpansion(".", [sh.cwd])] == param.expand(cmd, sh)

    param.parsed(["*"])
    assert [DTSh.PathExpansion("", sh.cwd.children)] == param.expand(cmd, sh)

    param.parsed(["*leds"])
    assert [
        DTSh.PathExpansion("", [sh.dt["/leds"], sh.dt["/pwmleds"]])
    ] == param.expand(cmd, sh)

    sh.cd("soc")
    param.parsed(["../leds*"])
    assert [DTSh.PathExpansion("..", [sh.dt["/leds"]])] == param.expand(cmd, sh)


def test_dtshparam_alias() -> None:
    param = DTShParamAlias()
    assert param.brief
    assert "[NAME]" == param.usage
    assert [] == param.raw
    # Default value (means all aliased nodes).
    assert "" == param.alias

    # Parameter's state and multiplicity.
    DTShTests.check_param(param)

    # Won't fault, the alias parameter is returned unchanged,
    # name resolution is deferred to command execution.
    param.parsed(["not-an-alias"])
    assert ["not-an-alias"] == param.raw
    assert "not-an-alias" == param.alias


def test_dtshparam_alias_autocomp() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [])
    param = DTShParamAlias()

    assert ["led0", "led1", "led2", "led3"] == [
        state.rlstr for state in param.autocomp("led", sh)
    ]
    # Exact match.
    assert ["led0"] == [state.rlstr for state in param.autocomp("led0", sh)]
    # No match.
    assert [] == param.autocomp("ledX", sh)


def test_dtshparam_chosen() -> None:
    param = DTShParamChosen()
    assert param.brief
    assert "[NAME]" == param.usage
    assert [] == param.raw
    # Default value (means all chosen nodes).
    assert "" == param.chosen

    # Won't fault, the alias parameter is returned unchanged,
    # name resolution is deferred to command execution.
    param.parsed(["not-a-chosen"])
    assert ["not-a-chosen"] == param.raw
    assert "not-a-chosen" == param.chosen


def test_dtshparam_chosen_autocomp() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [])
    param = DTShParamChosen()

    assert ["zephyr,bt-c2h-uart", "zephyr,bt-mon-uart"] == [
        state.rlstr for state in param.autocomp("zephyr,bt", sh)
    ]
    # Exact match.
    assert ["zephyr,bt-mon-uart"] == [
        state.rlstr for state in param.autocomp("zephyr,bt-mon", sh)
    ]
    # No match.
    assert [] == param.autocomp("Zephyr,", sh)
