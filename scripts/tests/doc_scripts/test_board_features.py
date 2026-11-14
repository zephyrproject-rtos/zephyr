#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for the board hardware features table and the board finder index built from it."""

import itertools
from types import SimpleNamespace

from board_features import collect_target_features
from dts_binding_types import ZEPHYR_BASE

ADC_BINDING = ZEPHYR_BASE / "dts/bindings/adc/vnd,adc.yaml"
DAC_BINDING = ZEPHYR_BASE / "dts/bindings/dac/vnd,dac.yaml"
SOC_DTS = ZEPHYR_BASE / "dts/arm/vnd/soc.dtsi"


def make_node(compats, matching_compat="first", binding_path=ADC_BINDING, status="okay"):
    if matching_compat == "first":
        matching_compat = compats[0]
    return SimpleNamespace(
        compats=compats,
        matching_compat=matching_compat,
        binding_path=None if binding_path is None else str(binding_path),
        status=status,
        filename=str(SOC_DTS),
        lineno=1,
        title="Some device",
        description="Some device.",
    )


def collect(nodes, board_name="some_board"):
    return collect_target_features(nodes, board_name, lambda node: node.description)


def test_two_nodes_of_one_kind_share_a_row():
    features, compatibles = collect(
        [
            make_node(["vnd,adc", "vnd,adc-v1"]),
            make_node(["vnd,adc", "vnd,adc-v2"], status="disabled"),
        ]
    )

    row = features["adc"]["vnd,adc"]
    assert len(row["okay_nodes"]) == 1
    assert len(row["disabled_nodes"]) == 1
    assert row["compatibles"] == ["vnd,adc", "vnd,adc-v1", "vnd,adc-v2"]
    assert compatibles == ["vnd,adc", "vnd,adc-v1", "vnd,adc-v2"]


def test_every_compatible_of_a_node_is_listed():
    features, compatibles = collect([make_node(["vnd,adc", "vnd,adc-v1", "snps,dwmac"])])

    assert features["adc"]["vnd,adc"]["compatibles"] == ["vnd,adc", "vnd,adc-v1", "snps,dwmac"]
    assert compatibles == ["snps,dwmac", "vnd,adc", "vnd,adc-v1"]


def test_primary_compatible_comes_first():
    node = make_node(["vnd,unbound", "vnd,adc", "vnd,generic"], matching_compat="vnd,adc")

    features, _ = collect([node])

    assert features["adc"]["vnd,adc"]["compatibles"] == ["vnd,adc", "vnd,unbound", "vnd,generic"]


def test_index_equals_displayed_set():
    features, compatibles = collect(
        [
            make_node(["vnd,adc", "vnd,shared"]),
            make_node(["vnd,dac", "vnd,shared", "vnd,dac-v1"], binding_path=DAC_BINDING),
            make_node(["vnd,shared"], matching_compat="vnd,shared", binding_path=DAC_BINDING),
        ]
    )

    displayed = {
        compatible
        for rows in features.values()
        for row in rows.values()
        for compatible in row["compatibles"]
    }
    assert compatibles == sorted(displayed)
    assert "vnd,shared" in features["adc"]["vnd,adc"]["compatibles"]
    assert "vnd,shared" in features["dac"]["vnd,shared"]["compatibles"]


def test_zephyr_prefixed_secondary_is_dropped():
    features, compatibles = collect([make_node(["vnd,adc", "zephyr,fake-adc"])])

    assert features["adc"]["vnd,adc"]["compatibles"] == ["vnd,adc"]
    assert compatibles == ["vnd,adc"]


def test_node_bound_to_zephyr_compatible_is_dropped_with_its_other_strings():
    features, compatibles = collect([make_node(["zephyr,uart-emul", "vnd,uart"])])

    assert features == {}
    assert compatibles == []


def test_zephyr_prefixed_strings_are_kept_on_native_sim():
    features, compatibles = collect(
        [make_node(["zephyr,native-uart", "zephyr,other"])], board_name="native_sim"
    )

    assert features["adc"]["zephyr,native-uart"]["compatibles"] == [
        "zephyr,native-uart",
        "zephyr,other",
    ]
    assert compatibles == ["zephyr,native-uart", "zephyr,other"]


def test_nodes_without_binding_are_skipped():
    features, compatibles = collect(
        [
            make_node(["vnd,nobinding"], matching_compat=None, binding_path=None),
            make_node(["vnd,other"], matching_compat=None),
        ]
    )

    assert features == {}
    assert compatibles == []


def test_output_does_not_depend_on_node_order():
    nodes = [
        make_node(["vnd,b", "vnd,b-v1"]),
        make_node(["vnd,a", "vnd,a-v1"]),
        make_node(["vnd,b", "vnd,b-v2"]),
        make_node(["vnd,c"]),
    ]

    results = [collect(list(order)) for order in itertools.permutations(nodes)]

    def comparable(result):
        features, compatibles = result
        rows = {
            (binding_type, primary): frozenset(row["compatibles"])
            for binding_type, binding_rows in features.items()
            for primary, row in binding_rows.items()
        }
        return rows, compatibles

    assert all(comparable(result) == comparable(results[0]) for result in results)
