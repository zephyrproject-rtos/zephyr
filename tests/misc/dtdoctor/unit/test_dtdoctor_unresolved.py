#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for the diagnosis of node identifiers that do not resolve to a node."""

import dtdoctor_analyzer
import pytest
from conftest import DTS_REFERENCES


@pytest.fixture
def edt(make_edt):
    return make_edt(DTS_REFERENCES)[0]


def diagnose(edt, ident):
    return "\n".join(dtdoctor_analyzer.handle_unresolved_node_id(edt, ident))


def test_missing_alias(edt):
    out = diagnose(edt, "DT_N_ALIAS_led1_P_gpios_IDX_0_PH_ORD")
    assert "DT_ALIAS(led1) refers to the devicetree alias 'led1', which is not defined." in out
    assert "Defined aliases: my-foo" in out
    assert "led1 = &<node-label>;" in out


def test_missing_alias_name_uses_dashes(edt):
    out = diagnose(edt, "DT_N_ALIAS_eeprom_0_ORD")
    assert "DT_ALIAS(eeprom_0) refers to the devicetree alias 'eeprom-0'" in out
    assert "eeprom-0 = &<node-label>;" in out


def test_missing_alias_with_label_of_that_name(edt):
    out = diagnose(edt, "DT_N_ALIAS_led0_P_gpios_IDX_0_PH_ORD")
    assert "The node /led-0 has the label 'led0'." in out
    assert "DT_NODELABEL(led0)" in out
    assert "led0 = &led0;" in out


def test_missing_nodelabel_similar_labels(edt):
    out = diagnose(edt, "DT_N_NODELABEL_foo_de_ORD")
    assert "DT_NODELABEL(foo_de) refers to the node label 'foo_de', which no node has." in out
    assert "Similar node labels: foo_dev" in out


def test_missing_nodelabel_with_alias_of_that_name(edt):
    out = diagnose(edt, "DT_N_NODELABEL_my_foo_ORD")
    assert "The alias 'my-foo' points to /foo-device." in out
    assert "DT_ALIAS(my_foo)" in out


def test_missing_chosen(edt):
    out = diagnose(edt, "DT_CHOSEN_zephyr_shell_uart_ORD")
    assert "DT_CHOSEN(zephyr_shell_uart) refers to a /chosen property that is not defined." in out
    assert "Defined /chosen properties: zephyr,console" in out
    assert "zephyr,shell-uart = &<node-label>;" in out


def test_missing_instance_unknown_compatible(edt):
    out = diagnose(edt, "DT_N_INST_0_vnd_nope_ORD")
    assert "DT_INST(0, vnd_nope)" in out
    assert "no devicetree node has it" in out


def test_missing_instance_out_of_range(edt):
    out = diagnose(edt, "DT_N_INST_2_vnd_foo_device_ORD")
    assert "only 2 node(s) have this compatible" in out
    assert "'vnd,foo-device'" in out
    assert " - 0: foo_dev: /foo-device (status 'okay')" in out
    assert " - 1: bar_dev: /bar-device (status 'disabled')" in out


def test_existing_instance_yields_nothing(edt):
    # DT_N_INST_1_vnd_foo_device is defined, so the failure is elsewhere
    assert dtdoctor_analyzer.handle_unresolved_node_id(edt, "DT_N_INST_1_vnd_foo_device_ORD") == []


@pytest.mark.parametrize('ident', ['__ORD', '__P_gpios_IDX_0_PH_ORD'], ids=['bare', 'with-prop'])
def test_invalid_node(edt, ident):
    assert "DT_INVALID_NODE" in diagnose(edt, ident)


@pytest.mark.parametrize(
    'ident',
    ['DT_N_ALIAS_led0', 'DT_N_FOO_bar_ORD', 'not_a_node_id_ORD'],
    ids=['no-ord-suffix', 'unknown-kind', 'not-a-node-id'],
)
def test_unrecognized_identifier_yields_nothing(edt, ident):
    assert dtdoctor_analyzer.handle_unresolved_node_id(edt, ident) == []


def test_main_end_to_end_unresolved(make_edt, make_pickle, run_analyzer):
    edt, _ = make_edt(DTS_REFERENCES)
    symbol = "__device_dts_ord_DT_N_ALIAS_led1_P_gpios_IDX_0_PH_ORD"
    rc, out, _ = run_analyzer(make_pickle(edt), f"'{symbol}' undeclared here")
    assert rc == 0
    assert "DT Doctor" in out
    assert "DT_ALIAS(led1)" in out


def test_main_unrecognized_identifier_silent(make_edt, make_pickle, run_analyzer):
    edt, _ = make_edt(DTS_REFERENCES)
    rc, out, err = run_analyzer(make_pickle(edt), "__device_dts_ord_DT_N_FOO_bar_ORD")
    assert rc == 1
    assert out == ""
    assert err == ""


def test_wrap_list_keeps_names_whole():
    names = [f"zephyr,long-property-name-{i}" for i in range(6)]
    lines = dtdoctor_analyzer.wrap_list("Defined /chosen properties:", names)
    assert len(lines) > 1
    assert all(len(line) <= 76 for line in lines)
    assert all(any(name in line for line in lines) for name in names)


def test_missing_property(edt):
    out = diagnose(edt, "DT_N_S_led_0_P_gpios_IDX_0_PH_ORD")
    assert "'led0: /led-0' has no 'gpios' property." in out


def test_missing_property_through_alias(edt):
    # DT_ALIAS(my_foo) resolved, so the identifier is the node's path identifier
    out = diagnose(edt, "DT_N_S_foo_device_P_io_channels_IDX_0_PH_ORD")
    assert "'foo_dev: /foo-device' (alias 'my-foo') has no 'io-channels' property." in out
    assert "Properties set on this node: compatible" in out


def test_property_on_node_without_binding(edt):
    out = diagnose(edt, "DT_N_S_raw_led_P_gpios_IDX_0_PH_ORD")
    assert "'raw_led: /raw-led' has a 'gpios' property, but the node has no" in out
    assert "binding, so no devicetree macros are generated for it." in out


def test_missing_child(edt):
    out = diagnose(edt, "DT_N_S_foo_devic_ORD")
    assert "'/' has no child node matching 'foo_devic'." in out
    assert "Similar child nodes: foo-device" in out


def test_missing_grandchild(edt):
    out = diagnose(edt, "DT_N_S_foo_device_S_child_ORD")
    assert "'foo_dev: /foo-device' (alias 'my-foo') has no child node matching 'child'." in out


def test_declared_property_yields_nothing(edt):
    # The node and property exist; the failure is elsewhere (e.g. an out-of-range index)
    assert (
        dtdoctor_analyzer.handle_unresolved_node_id(edt, "DT_N_S_foo_device_P_compatible_ORD") == []
    )
