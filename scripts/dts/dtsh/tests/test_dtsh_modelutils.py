# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.modelutils module."""

# Relax pylint a bit for unit tests.
# pylint: disable=too-many-locals
# pylint: disable=too-many-branches
# pylint: disable=missing-function-docstring


from typing import List, Tuple, Type
import operator
import re
import sys

import pytest

from dtsh.model import DTNode, DTNodeSorter
from dtsh.modelutils import (
    # Text-based criteria.
    DTNodeTextCriterion,
    DTNodeWithPath,
    DTNodeWithName,
    DTNodeWithUnitName,
    DTNodeWithCompatible,
    DTNodeWithBinding,
    DTNodeWithVendor,
    DTNodeWithDeviceLabel,
    DTNodeWithNodeLabel,
    DTNodeWithAlias,
    DTNodeWithChosen,
    DTNodeAlsoKnownAs,
    DTNodeWithBus,
    DTNodeWithOnBus,
    DTNodeWithDescription,
    # Integer-based criteria.
    DTNodeIntCriterion,
    DTNodeWithUnitAddr,
    DTNodeWithIrqNumber,
    DTNodeWithIrqPriority,
    DTNodeWithRegAddr,
    DTNodeWithRegSize,
    DTNodeWithBindingDepth,
    # Sorters.
    DTNodeSortByPathName,
    DTNodeSortByNodeName,
    DTNodeSortByUnitName,
    DTNodeSortByUnitAddr,
    DTNodeSortByCompatible,
    DTNodeSortByBinding,
    DTNodeSortByVendor,
    DTNodeSortByDeviceLabel,
    DTNodeSortByNodeLabel,
    DTNodeSortByAlias,
    DTNodeSortByBus,
    DTNodeSortByOnBus,
    DTNodeSortByDepOrdinal,
    DTNodeSortByIrqNumber,
    DTNodeSortByIrqPriority,
    DTNodeSortByRegAddr,
    DTNodeSortByRegSize,
    DTNodeSortByBindingDepth,
    # Virtual trees.
    DTWalkableComb,
)

from .dtsh_uthelpers import DTShTests


def test_dtnode_sorters() -> None:
    # Crash-test all sorters.
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    sorters: List[DTNodeSorter] = [
        DTNodeSortByPathName(),
        DTNodeSortByNodeName(),
        DTNodeSortByUnitName(),
        DTNodeSortByUnitAddr(),
        DTNodeSortByCompatible(),
        DTNodeSortByBinding(),
        DTNodeSortByVendor(),
        DTNodeSortByDeviceLabel(),
        DTNodeSortByNodeLabel(),
        DTNodeSortByAlias(),
        DTNodeSortByBus(),
        DTNodeSortByOnBus(),
        DTNodeSortByDepOrdinal(),
        DTNodeSortByIrqNumber(),
        DTNodeSortByIrqPriority(),
        DTNodeSortByRegAddr(),
        DTNodeSortByRegSize(),
        DTNodeSortByBindingDepth(),
    ]
    for sorter in sorters:
        sorter.sort(nodes)
        sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_path_name() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByPathName()

    assert sorted([node.path for node in dtmodel]) == [
        node.path for node in sorter.sort(nodes)
    ]
    assert sorted([node.path for node in dtmodel], reverse=True) == [
        node.path for node in sorter.sort(nodes, reverse=True)
    ]


def test_dtnodesorter_by_node_name() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByNodeName()

    assert sorted([node.name for node in dtmodel]) == [
        node.name for node in sorter.sort(nodes)
    ]
    assert sorted([node.name for node in dtmodel], reverse=True) == [
        node.name for node in sorter.sort(nodes, reverse=True)
    ]


def test_dtnodesorter_by_unit_name() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByUnitName()

    assert sorted([node.unit_name for node in dtmodel]) == [
        node.unit_name for node in sorter.sort(nodes)
    ]
    assert sorted([node.unit_name for node in dtmodel], reverse=True) == [
        node.unit_name for node in sorter.sort(nodes, reverse=True)
    ]


def test_dtnodesorter_by_unit_addr() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByUnitAddr()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.unit_addr is not None:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: x.unit_addr)  # type: ignore
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_compatible() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByCompatible()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.compatibles:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: min(x.compatibles))
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    # Note: here sorted(reverse=True) is not granted to answer
    # the same order as sorted().reverse().
    sorted_sortable = sorted(sortable, key=lambda x: max(x.compatibles))
    sorted_sortable.reverse()
    unsortable.reverse()

    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_binding() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByBinding()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.compatible:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: x.compatible)  # type: ignore
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_vendor() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByVendor()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.vendor:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: x.vendor.name)  # type: ignore
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_device_label() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByDeviceLabel()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.label:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: x.label)  # type: ignore
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_node_label() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByNodeLabel()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.labels:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: min(x.labels))
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    # Note: here sorted(reverse=True) is not granted to answer
    # the same order as sorted().reverse().
    sorted_sortable = sorted(sortable, key=lambda x: max(x.labels))
    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_alias() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByAlias()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.aliases:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: min(x.aliases))
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    # Note: here sorted(reverse=True) is not granted to answer
    # the same order as sorted().reverse().
    sorted_sortable = sorted(sortable, key=lambda x: max(x.aliases))
    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_bus() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByBus()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.buses:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: min(x.buses))
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    # Note: here sorted(reverse=True) is not granted to answer
    # the same order as sorted().reverse().
    sorted_sortable = sorted(sortable, key=lambda x: max(x.buses))
    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_on_bus() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByOnBus()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.on_bus:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: x.on_bus)  # type: ignore
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_dep_ordinal() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByDepOrdinal()

    assert sorted([node.dep_ordinal for node in dtmodel]) == [
        node.dep_ordinal for node in sorter.sort(nodes)
    ]
    assert sorted([node.dep_ordinal for node in dtmodel], reverse=True) == [
        node.dep_ordinal for node in sorter.sort(nodes, reverse=True)
    ]


def test_dtnodesorter_by_irq_number() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByIrqNumber()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.interrupts:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(
        sortable, key=lambda node: min(irq.number for irq in node.interrupts)
    )
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    # Note: here sorted(reverse=True) is not granted to answer
    # the same order as sorted().reverse().
    sorted_sortable = sorted(
        sortable, key=lambda node: max(irq.number for irq in node.interrupts)
    )
    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_irq_priority() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByIrqPriority()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.interrupts:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(
        sortable,
        key=lambda node: min(
            irq.priority if irq.priority is not None else sys.maxsize
            for irq in node.interrupts
        ),
    )
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    # Note: here sorted(reverse=True) is not granted to answer
    # the same order as sorted().reverse().
    sorted_sortable = sorted(
        sortable,
        key=lambda node: max(
            irq.priority if irq.priority is not None else sys.maxsize
            for irq in node.interrupts
        ),
    )
    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_reg_addr() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByRegAddr()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.registers:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(
        sortable, key=lambda x: min(reg.address for reg in x.registers)
    )
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    # Note: here sorted(reverse=True) is not granted to answer
    # the same order as sorted().reverse().
    sorted_sortable = sorted(
        sortable, key=lambda x: max(reg.address for reg in x.registers)
    )
    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_reg_size() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByRegSize()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.registers:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(
        sortable, key=lambda x: min(reg.size for reg in x.registers)
    )
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    # Note: here sorted(reverse=True) is not granted to answer
    # the same order as sorted().reverse().
    sorted_sortable = sorted(
        sortable, key=lambda x: max(reg.size for reg in x.registers)
    )
    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodesorter_by_cb_depth() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)
    sorter = DTNodeSortByBindingDepth()

    sortable: List[DTNode] = []
    unsortable: List[DTNode] = []
    for node in nodes:
        if node.binding:
            sortable.append(node)
        else:
            unsortable.append(node)

    sorted_sortable = sorted(sortable, key=lambda x: x.binding.cb_depth)  # type: ignore
    assert [*sorted_sortable, *unsortable] == sorter.sort(nodes)

    sorted_sortable.reverse()
    unsortable.reverse()
    assert [*unsortable, *sorted_sortable] == sorter.sort(nodes, reverse=True)


def test_dtnodecriterion_text_pattern() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Criteria with the attributes they depend on.
    cls_criteria: List[Tuple[Type[DTNodeTextCriterion], str]] = [
        (DTNodeWithPath, "path"),
        (DTNodeWithName, "name"),
        (DTNodeWithUnitName, "unit_name"),
        (DTNodeWithCompatible, "compatibles"),
        (DTNodeWithBinding, "binding"),
        (DTNodeWithVendor, "vendor"),
        (DTNodeWithDeviceLabel, "label"),
        (DTNodeWithNodeLabel, "labels"),
        (DTNodeWithAlias, "aliases"),
        (DTNodeWithChosen, "chosen"),
        (DTNodeWithBus, "buses"),
        (DTNodeWithOnBus, "on_bus"),
        (DTNodeWithDescription, "description"),
    ]

    criterion_by_attr: List[Tuple[DTNodeTextCriterion, str]] = [
        (cls_criterion("*"), attr) for cls_criterion, attr in cls_criteria
    ]
    criterion_by_attr_re: List[Tuple[DTNodeTextCriterion, str]] = [
        (cls_criterion(".*", re_strict=True), attr)
        for cls_criterion, attr in cls_criteria
    ]

    with_any_label_or_alias = DTNodeAlsoKnownAs("*")
    re_any_label_or_alias = DTNodeAlsoKnownAs(".*", re_strict=True)

    # Match nodes for which the attribute has a value.
    for node in nodes:
        for criterion, attr in criterion_by_attr:
            if getattr(node, attr):
                assert criterion.match(node)
            else:
                assert not criterion.match(node)

        for criterion, attr in criterion_by_attr_re:
            if getattr(node, attr):
                assert criterion.match(node)
            else:
                assert not criterion.match(node)

        if node.aliases or node.label or node.labels:
            assert with_any_label_or_alias.match(node)
            assert re_any_label_or_alias.match(node)
        else:
            assert not with_any_label_or_alias.match(node)
            assert not re_any_label_or_alias.match(node)

    for cls_criterion in [cls for cls, _ in cls_criteria]:
        with pytest.raises(re.error):
            # RE strict: "*" is a repeat qualifier, not a wild-card,
            # and there's nothing to repeat.
            cls_criterion("*", re_strict=True)


def test_dtnodecriterion_with_path() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/flash-controller@4001e000"]

    # Plain text search.
    assert DTNodeWithPath("soc").match(node)
    assert DTNodeWithPath("controller").match(node)
    assert not DTNodeWithPath("Flash").match(node)
    assert DTNodeWithPath("Flash", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithPath("*@4001e000").match(node)
    assert not DTNodeWithPath("/Soc*").match(node)
    assert DTNodeWithPath("/Soc*", ignore_case=True).match(node)

    # Strict RE.
    assert not DTNodeWithPath(".*Controller.*", re_strict=True).match(node)
    assert DTNodeWithPath(
        ".*Controller.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_name() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/flash-controller@4001e000"]

    # Plain text search.
    assert not DTNodeWithName("soc").match(node)
    assert DTNodeWithName("controller").match(node)
    assert not DTNodeWithName("Flash").match(node)
    assert DTNodeWithName("Flash", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithName("*@4001e000").match(node)
    assert not DTNodeWithName("Flash*").match(node)
    assert DTNodeWithName("Flash*", ignore_case=True).match(node)

    # Strict RE.
    assert not DTNodeWithName(".*Controller.*", re_strict=True).match(node)
    assert DTNodeWithName(
        ".*Controller.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_unit_name() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/flash-controller@4001e000"]

    # Plain text search.
    assert DTNodeWithUnitName("controller").match(node)
    assert not DTNodeWithUnitName("Flash").match(node)
    assert DTNodeWithUnitName("Flash", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithUnitName("*controller").match(node)
    assert not DTNodeWithUnitName("Flash*").match(node)
    assert DTNodeWithUnitName("Flash*", ignore_case=True).match(node)

    # Strict RE.
    assert not DTNodeWithUnitName(".*Controller.*", re_strict=True).match(node)
    assert DTNodeWithUnitName(
        ".*Controller.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_compatible() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/egu@40014000"]
    assert ["nordic,nrf-egu", "nordic,nrf-swi"] == node.compatibles

    # Plain text search.
    assert DTNodeWithCompatible("egu").match(node)
    assert DTNodeWithCompatible("swi").match(node)
    assert not DTNodeWithCompatible("nRF").match(node)
    assert DTNodeWithCompatible("nRF", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithCompatible("*egu").match(node)
    assert DTNodeWithCompatible("*swi").match(node)
    assert not DTNodeWithCompatible("Nordic*").match(node)
    assert DTNodeWithCompatible("Nordic*", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithCompatible(".*egu", re_strict=True).match(node)
    assert DTNodeWithCompatible(".*swi", re_strict=True).match(node)
    assert not DTNodeWithCompatible(".*nRF.*", re_strict=True).match(node)
    assert DTNodeWithCompatible(
        ".*nRF.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_binding() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/i2c@40003000/bme680@76"]
    assert node.binding
    assert "bosch,bme680" == node.binding.compatible
    assert (
        "The BME680 is an integrated environmental sensor that measures"
        == node.binding.get_headline()
    )

    # Plain text search.
    assert DTNodeWithBinding("bme").match(node)
    assert not DTNodeWithBinding("Environmental").match(node)
    assert DTNodeWithBinding("Environmental", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithBinding("bosch*").match(node)
    assert DTNodeWithBinding("*bme680").match(node)
    assert not DTNodeWithBinding("*Environmental*").match(node)
    assert DTNodeWithBinding("*Environmental*", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithBinding("bosch.*", re_strict=True).match(node)
    assert DTNodeWithBinding(".*bme680", re_strict=True).match(node)
    assert not DTNodeWithBinding(".*Sensor.*", re_strict=True).match(node)
    assert DTNodeWithBinding(
        ".*Sensor.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_vendor() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/i2c@40003000/bme680@76"]
    assert node.vendor
    assert "bosch" == node.vendor.prefix
    assert "Bosch Sensortec GmbH" == node.vendor.name

    # Plain text search.
    assert DTNodeWithVendor("bosch").match(node)
    assert not DTNodeWithVendor("sensortec").match(node)
    assert DTNodeWithVendor("sensortec", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithVendor("Bosch*").match(node)
    assert not DTNodeWithVendor("*gmbh").match(node)
    assert DTNodeWithVendor("*gmbh", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithVendor("bosch.*", re_strict=True).match(node)
    assert not DTNodeWithVendor(".*sensor.*", re_strict=True).match(node)
    assert DTNodeWithVendor(
        ".*sensor.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_device_label() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/leds/led_0"]
    assert "Green LED 0" == node.label

    # Plain text search.
    assert DTNodeWithDeviceLabel("LED").match(node)
    assert not DTNodeWithDeviceLabel("green").match(node)
    assert DTNodeWithDeviceLabel("green", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithDeviceLabel("*0").match(node)
    assert not DTNodeWithDeviceLabel("*led*").match(node)
    assert DTNodeWithDeviceLabel("*led*", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithDeviceLabel("Green.*", re_strict=True).match(node)
    assert not DTNodeWithDeviceLabel(".*led.*", re_strict=True).match(node)
    assert DTNodeWithDeviceLabel(
        ".*led.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_node_label() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/i2c@40003000"]
    assert ["i2c0", "arduino_i2c"] == node.labels

    # Plain text search.
    assert DTNodeWithNodeLabel("i2c").match(node)
    assert not DTNodeWithNodeLabel("Arduino").match(node)
    assert DTNodeWithNodeLabel("Arduino", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithNodeLabel("*i2c*").match(node)
    assert not DTNodeWithNodeLabel("Arduino*").match(node)
    assert DTNodeWithNodeLabel("Arduino*", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithNodeLabel(".*i2c.*", re_strict=True).match(node)
    assert not DTNodeWithNodeLabel("Arduino.*", re_strict=True).match(node)
    assert DTNodeWithNodeLabel(
        "Arduino.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_alias() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/leds/led_0"]
    assert ["led0", "bootloader-led0", "mcuboot-led0"] == node.aliases

    # Plain text search.
    assert DTNodeWithAlias("led").match(node)
    assert DTNodeWithAlias("boot").match(node)
    assert not DTNodeWithAlias("MCU").match(node)
    assert DTNodeWithAlias("MCU", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithAlias("led*").match(node)
    assert DTNodeWithAlias("boot*").match(node)
    assert not DTNodeWithAlias("MCU*").match(node)
    assert DTNodeWithAlias("MCU*", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithAlias("led.*", re_strict=True).match(node)
    assert DTNodeWithAlias("boot.*", re_strict=True).match(node)
    assert not DTNodeWithAlias("MCU.*", re_strict=True).match(node)
    assert DTNodeWithAlias("MCU.*", re_strict=True, ignore_case=True).match(
        node
    )


def test_dtnodecriterion_with_chosen() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/random@4000d000"]
    assert ["zephyr,entropy"] == node.chosen

    # Plain text search.
    assert DTNodeWithChosen("entropy").match(node)
    assert not DTNodeWithChosen("Zephyr").match(node)
    assert DTNodeWithChosen("Zephyr", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithChosen("*entropy").match(node)
    assert not DTNodeWithChosen("Zephyr*").match(node)
    assert DTNodeWithChosen("Zephyr*", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithChosen(".*entropy", re_strict=True).match(node)
    assert not DTNodeWithChosen("Zephyr.*", re_strict=True).match(node)
    assert DTNodeWithChosen("Zephyr.*", re_strict=True, ignore_case=True).match(
        node
    )


def test_dtnodecriterion_with_bus() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/i2c@40003000"]
    assert ["i2c"] == node.buses

    # Plain text search.
    assert DTNodeWithBus("i2c").match(node)
    assert not DTNodeWithBus("I2C").match(node)
    assert DTNodeWithBus("I2C", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithBus("i*c").match(node)
    assert not DTNodeWithBus("I*C").match(node)
    assert DTNodeWithBus("I*C", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithBus(r"i[\d]c", re_strict=True).match(node)
    assert not DTNodeWithBus(r"I[\d]C", re_strict=True).match(node)
    assert DTNodeWithBus(r"I[\d]C", re_strict=True, ignore_case=True).match(
        node
    )


def test_dtnodecriterion_with_on_bus() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/i2c@40003000/bme680@76"]
    assert "i2c" == node.on_bus

    # Plain text search.
    assert DTNodeWithOnBus("i2c").match(node)
    assert not DTNodeWithOnBus("I2C").match(node)
    assert DTNodeWithOnBus("I2C", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeWithOnBus("i*c").match(node)
    assert not DTNodeWithOnBus("I*C").match(node)
    assert DTNodeWithOnBus("I*C", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeWithOnBus(r"i[\d]c", re_strict=True).match(node)
    assert not DTNodeWithOnBus(r"I[\d]C", re_strict=True).match(node)
    assert DTNodeWithOnBus(r"I[\d]C", re_strict=True, ignore_case=True).match(
        node
    )


def test_dtnodecriterion_with_desc() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/i2c@40003000/bme680@76"]
    assert node.binding
    assert (
        "The BME680 is an integrated environmental sensor that measures"
        == node.binding.get_headline()
    )

    # Plain text search.
    assert not DTNodeWithDescription("bme").match(node)
    assert DTNodeWithDescription("bme", ignore_case=True).match(node)

    # Wild-card substitution.
    assert not DTNodeWithDescription("*Environmental*").match(node)
    assert DTNodeWithDescription("*Environmental*", ignore_case=True).match(
        node
    )

    # Strict RE.
    assert not DTNodeWithDescription(".*Sensor.*", re_strict=True).match(node)
    assert DTNodeWithDescription(
        ".*Sensor.*", re_strict=True, ignore_case=True
    ).match(node)


def test_dtnodecriterion_with_aka() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/leds/led_0"]
    assert "Green LED 0" == node.label
    assert ["led0"] == node.labels
    assert ["led0", "bootloader-led0", "mcuboot-led0"] == node.aliases

    # Plain text search.
    assert DTNodeAlsoKnownAs("LED").match(node)
    assert DTNodeAlsoKnownAs("boot").match(node)
    assert not DTNodeAlsoKnownAs("MCU").match(node)
    assert DTNodeAlsoKnownAs("MCU", ignore_case=True).match(node)

    # Wild-card substitution.
    assert DTNodeAlsoKnownAs("*LED*").match(node)
    assert DTNodeAlsoKnownAs("boot*").match(node)
    assert not DTNodeAlsoKnownAs("MCU*").match(node)
    assert DTNodeAlsoKnownAs("MCU*", ignore_case=True).match(node)

    # Strict RE.
    assert DTNodeAlsoKnownAs(".*LED.*", re_strict=True).match(node)
    assert DTNodeAlsoKnownAs("boot*", re_strict=True).match(node)
    assert not DTNodeAlsoKnownAs("MCU*", re_strict=True).match(node)
    assert DTNodeAlsoKnownAs("MCU*", re_strict=True, ignore_case=True).match(
        node
    )


def test_dtnodecriterion_integer_expr() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    nodes = list(dtmodel)

    # Criteria with the attributes they depend on.
    # Would be "*" command argument value.
    criterion_by_attr: List[Tuple[DTNodeIntCriterion, str]] = [
        (DTNodeWithUnitAddr(None, None), "unit_addr"),
        (DTNodeWithIrqNumber(None, None), "interrupts"),
        (DTNodeWithIrqPriority(None, None), "interrupts"),
        (DTNodeWithRegAddr(None, None), "registers"),
        (DTNodeWithRegSize(None, None), "registers"),
        (DTNodeWithBindingDepth(None, None), "binding"),
    ]

    # Match nodes for which the attribute has a value.
    for node in nodes:
        for criterion, attr in criterion_by_attr:
            attval = getattr(node, attr)
            if (attval is not None) and (attval != []):
                assert criterion.match(node)
            else:
                assert not criterion.match(node)


def test_dtnodecriterion_with_unit_addr() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/timer@4000a000"]
    assert 0x4000A000 == node.unit_addr

    # Strict equality without operator.
    assert DTNodeWithUnitAddr(None, 0x4000A000).match(node)

    # Comparison operators.
    assert not DTNodeWithUnitAddr(operator.lt, 0x4000A000).match(node)
    assert not DTNodeWithUnitAddr(operator.gt, 0x4000A000).match(node)
    assert DTNodeWithUnitAddr(operator.eq, 0x4000A000).match(node)
    assert DTNodeWithUnitAddr(operator.le, 0x4000A000).match(node)
    assert DTNodeWithUnitAddr(operator.ge, 0x4000A000).match(node)
    assert not DTNodeWithUnitAddr(operator.ne, 0x4000A000).match(node)


def test_dtnodecriterion_with_irq_number() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/timer@4000a000"]
    assert node.interrupts
    irq_n = node.interrupts[0].number
    assert 10 == irq_n

    # Strict equality without operator.
    assert DTNodeWithIrqNumber(None, irq_n).match(node)

    # Comparison operators.
    assert not DTNodeWithIrqNumber(operator.lt, irq_n).match(node)
    assert not DTNodeWithIrqNumber(operator.gt, irq_n).match(node)
    assert DTNodeWithIrqNumber(operator.eq, irq_n).match(node)
    assert DTNodeWithIrqNumber(operator.le, irq_n).match(node)
    assert DTNodeWithIrqNumber(operator.ge, irq_n).match(node)
    assert not DTNodeWithIrqNumber(operator.ne, irq_n).match(node)


def test_dtnodecriterion_with_irq_priority() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/timer@4000a000"]
    assert node.interrupts
    irq_prio = node.interrupts[0].priority
    assert 1 == irq_prio

    # Strict equality without operator.
    assert DTNodeWithIrqPriority(None, irq_prio).match(node)

    # Comparison operators.
    assert not DTNodeWithIrqPriority(operator.lt, irq_prio).match(node)
    assert not DTNodeWithIrqPriority(operator.gt, irq_prio).match(node)
    assert DTNodeWithIrqPriority(operator.eq, irq_prio).match(node)
    assert DTNodeWithIrqPriority(operator.le, irq_prio).match(node)
    assert DTNodeWithIrqPriority(operator.ge, irq_prio).match(node)
    assert not DTNodeWithIrqPriority(operator.ne, irq_prio).match(node)


def test_dtnodecriterion_with_reg_addr() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/gpio@50000000"]
    assert [0x50000000, 0x50000500] == [reg.address for reg in node.registers]

    # Strict equality without operator.
    assert DTNodeWithRegAddr(None, 0x50000000).match(node)
    assert DTNodeWithRegAddr(None, 0x50000500).match(node)

    # Comparison operators.
    assert not DTNodeWithRegAddr(operator.lt, 0x50000000).match(node)
    assert DTNodeWithRegAddr(operator.lt, 0x50000500).match(node)
    assert not DTNodeWithRegAddr(operator.gt, 0x50000500).match(node)
    assert DTNodeWithRegAddr(operator.gt, 0x50000000).match(node)
    assert not DTNodeWithRegAddr(operator.gt, 0x50000500).match(node)
    assert DTNodeWithRegAddr(operator.eq, 0x50000000).match(node)
    assert DTNodeWithRegAddr(operator.eq, 0x50000500).match(node)
    assert DTNodeWithRegAddr(operator.le, 0x50000000).match(node)
    assert DTNodeWithRegAddr(operator.ge, 0x50000500).match(node)
    # Equality for each address fails because of the other one.
    assert DTNodeWithRegAddr(operator.ne, 0x50000000).match(node)
    assert DTNodeWithRegAddr(operator.ne, 0x50000500).match(node)


def test_dtnode_with_reg_size() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/gpio@50000000"]
    assert [512, 768] == [reg.size for reg in node.registers]

    # Strict equality without operator.
    assert DTNodeWithRegSize(None, 512).match(node)
    assert DTNodeWithRegSize(None, 768).match(node)

    # Comparison operators.
    assert not DTNodeWithRegSize(operator.lt, 512).match(node)
    assert DTNodeWithRegSize(operator.lt, 768).match(node)
    assert DTNodeWithRegSize(operator.gt, 512).match(node)
    assert not DTNodeWithRegSize(operator.gt, 768).match(node)
    assert DTNodeWithRegSize(operator.eq, 512).match(node)
    assert DTNodeWithRegSize(operator.eq, 768).match(node)
    assert DTNodeWithRegSize(operator.le, 512).match(node)
    assert DTNodeWithRegSize(operator.ge, 768).match(node)
    # Equality for each size fails because of the other one.
    assert DTNodeWithRegSize(operator.ne, 512).match(node)
    assert DTNodeWithRegSize(operator.ne, 768).match(node)

    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/soc/gpio@50000000"]


def test_dtnode_with_cb_depth() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    node = dtmodel["/pin-controller/uart0_default/group1"]
    assert node.binding
    assert 2 == node.binding.cb_depth

    # Strict equality without operator.
    assert DTNodeWithBindingDepth(None, 2).match(node)

    # Comparison operators.
    assert not DTNodeWithBindingDepth(operator.lt, 2).match(node)
    assert not DTNodeWithBindingDepth(operator.gt, 2).match(node)
    assert DTNodeWithBindingDepth(operator.eq, 2).match(node)
    assert DTNodeWithBindingDepth(operator.le, 2).match(node)
    assert DTNodeWithBindingDepth(operator.ge, 2).match(node)
    assert not DTNodeWithBindingDepth(operator.ne, 2).match(node)


def test_dtwalkable_comb() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()

    walkable = DTWalkableComb(dtmodel.root, [])
    N = 0
    for _ in walkable.walk():
        N += 1
    assert 0 == N

    dt_soc = dtmodel["/soc"]
    dt_flashctrl = dt_soc.get_child("flash-controller@4001e000")
    dt_flash0 = dt_flashctrl.get_child("flash@0")
    dt_partitions = dt_flash0.get_child("partitions")
    dt_partition0 = dt_partitions.get_child("partition@0")
    dt_cpus = dtmodel["/cpus"]
    dt_cpu0 = dt_cpus.get_child("cpu@0")
    dt_cpu_itm = dt_cpu0.get_child("itm@e0000000")
    leaves = [dt_cpu_itm, dt_partition0]

    walkable = DTWalkableComb(dtmodel.root, leaves)
    assert [
        dtmodel.root,
        dt_soc,
        dt_flashctrl,
        dt_flash0,
        dt_partitions,
        dt_partition0,
        dt_cpus,
        dt_cpu0,
        dt_cpu_itm,
    ] == list(walkable.walk())

    walkable = DTWalkableComb(dt_soc, leaves)
    assert [
        dt_soc,
        dt_flashctrl,
        dt_flash0,
        dt_partitions,
        dt_partition0,
    ] == list(walkable.walk())

    walkable = DTWalkableComb(dtmodel.root, leaves)
    order_by = DTNodeSorter()
    assert [
        dtmodel.root,
        dt_cpus,
        dt_cpu0,
        dt_cpu_itm,
        dt_soc,
        dt_flashctrl,
        dt_flash0,
        dt_partitions,
        dt_partition0,
    ] == list(walkable.walk(order_by=order_by))

    assert [
        dtmodel.root,
        dt_soc,
        dt_flashctrl,
        dt_flash0,
        dt_partitions,
        dt_partition0,
        dt_cpus,
        dt_cpu0,
        dt_cpu_itm,
    ] == list(walkable.walk(order_by=order_by, reverse=True))
