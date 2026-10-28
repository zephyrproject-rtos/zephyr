#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the devicetree functions of the filter expression parser
"""

from unittest import mock

import expr_parser


def _node(compat, status='okay', **props):
    node = mock.Mock()
    node.matching_compat = compat
    node.compats = [compat]
    node.status = status
    node.props = {name.replace('_', '-'): mock.Mock(val=val) for name, val in props.items()}
    return node


def test_dt_compat_any_prop_enabled():
    """A property set on any enabled node with the compatible"""
    edt = mock.Mock()
    edt.nodes = [
        _node('vnd,dma', scatter_gather=False),
        _node('vnd,dma', scatter_gather=True),
    ]
    expr = 'dt_compat_any_prop_enabled("vnd,dma", "scatter-gather")'
    assert expr_parser.parse(expr, {}, edt)

    edt.nodes[1].status = 'disabled'
    assert not expr_parser.parse(expr, {}, edt)

    assert not expr_parser.parse('dt_compat_any_prop_enabled("vnd,dma", "cyclic")', {}, edt)
    assert not expr_parser.parse(
        'dt_compat_any_prop_enabled("vnd,other", "scatter-gather")', {}, edt
    )
