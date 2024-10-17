#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for quarantine.py classes' methods
"""

import mock
import os
import pytest
import textwrap

from twisterlib.quarantine import QuarantineException, \
                                  QuarantineElement, \
                                  QuarantineData


TESTDATA_1 = [
    (
        ['dummy scenario', 'another scenario'],
        ['dummy platform', 'another platform'],
        ['dummy architecture', 'another architecture'],
        ['dummy simulation', 'another simulation'],
        None,
        []
    ),
    (
        ['all'],
        ['dummy platform', 'another platform'],
        ['dummy architecture', 'another architecture'],
        ['dummy simulation', 'another simulation'],
        None,
        ['scenarios']
    ),
    (
        ['dummy scenario', 'another scenario'],
        ['dummy platform', 'all'],
        ['all', 'another architecture'],
        ['dummy simulation', 'another simulation'],
        None,
        ['platforms', 'architectures']
    ),
    (
        ['all', 'another scenario'],
        [],
        [],
        ['all', 'all'],
        QuarantineException,
        ['scenarios', 'platforms', 'architectures', 'simulations']
    ),
]


@pytest.mark.parametrize(
    'scenarios, platforms, architectures, ' \
    'simulations, expected_exception, empty_filter_attrs',
    TESTDATA_1,
    ids=[
        'no empties',
        'all scenarios',
        'all platforms and architectures',
        'exception'
    ]
)
def test_quarantineelement_post_init(
    scenarios,
    platforms,
    architectures,
    simulations,
    expected_exception,
    empty_filter_attrs
):
    if expected_exception:
        with pytest.raises(expected_exception):
            quarantine_element = QuarantineElement(
                scenarios=scenarios,
                platforms=platforms,
                architectures=architectures,
                simulations=simulations
            )
    else:
        quarantine_element = QuarantineElement(
            scenarios=scenarios,
            platforms=platforms,
            architectures=architectures,
            simulations=simulations
        )

        for attr in ['scenarios', 'platforms', 'architectures', 'simulations']:
            if attr in empty_filter_attrs:
                assert getattr(quarantine_element, attr) == []
            else:
                assert getattr(quarantine_element, attr) != []


def test_quarantinedata_post_init():
    quarantine_element_dict = {
        'scenarios': ['all'],
        'platforms': ['dummy platform'],
        'architectures': [],
        'simulations': ['dummy simulation', 'another simulation']
    }

    quarantine_element = QuarantineElement(
        platforms=['dummy platform'],
        architectures=[],
        simulations=['dummy simulation', 'another simulation']
    )

    quarantine_data_qlist = [quarantine_element, quarantine_element_dict]

    quarantine_data = QuarantineData(quarantine_data_qlist)

    assert quarantine_data.qlist[0] == quarantine_data.qlist[1]


TESTDATA_2 = [
    (
        '',
        QuarantineData()
    ),
    (
        textwrap.dedent("""
        [
            {
                \"scenarios\": [\"all\"],
                \"platforms\": [\"dummy platform\"],
                \"architectures\": [],
                \"simulations\": [\"dummy simulation\", \"another simulation\"]
            }
        ]
        """),
        QuarantineData(
            [
                QuarantineElement(
                    scenarios=[],
                    platforms=['dummy platform'],
                    architectures=[],
                    simulations=['dummy simulation', 'another simulation']
                )
            ]
        )
    ),
    (
        textwrap.dedent("""
        [
            {
                \"I\": [\"am\"],
                \"not\": \"a\",
                \"valid\": [],
                \"JSON\": [\"for\", \"this\"]
            }
        ]
        """),
        QuarantineException
    )
]


@pytest.mark.parametrize(
    'file_contents, expected',
    TESTDATA_2,
    ids=['empty', 'valid', 'not valid']
)
def test_quarantinedata_load_data_from_yaml(file_contents, expected):
    with mock.patch('builtins.open', mock.mock_open(read_data=file_contents)):
        if isinstance(expected, type) and issubclass(expected, Exception):
            with pytest.raises(expected):
                res = QuarantineData.load_data_from_yaml(
                    os.path.join('dummy', 'path')
                )
        else:
            res = QuarantineData.load_data_from_yaml(
                os.path.join('dummy', 'path')
            )

            assert res == expected


TESTDATA_3 = [
    (
        'good scenario',
        'good platform',
        'good arch',
        'good sim',
        None
    ),
    (
        'good scenario',
        'very bad dummy platform',
        'good arch',
        'good sim',
        0
    ),
    (
        'bad scenario 1',
        'good platform',
        'good arch',
        'bad sim',
        1
    ),
    (
        'bad scenario 1',
        'good platform',
        'good arch',
        'sim for scenario 1',
        None
    ),
    (
        'good scenario',
        'good platform',
        'unsupported arch 1',
        'good sim',
        2
    )
]


@pytest.mark.parametrize(
    'scenario, platform, architecture, simulation, expected_idx',
    TESTDATA_3,
    ids=[
        'not quarantined',
        'quarantined platform',
        'quarantined scenario with sim',
        'not quarantined with bad scenario',
        'quarantined arch'
    ]
)
def test_quarantinedata_get_matched_quarantine(
    scenario,
    platform,
    architecture,
    simulation,
    expected_idx
):
    qlist = [
        QuarantineElement(
            scenarios=['all'],
            platforms=['very bad dummy platform'],
            architectures=['all'],
            simulations=['all']
        ),
        QuarantineElement(
            scenarios=['bad scenario 1', 'bad scenario 2'],
            platforms=['all'],
            architectures=['all'],
            simulations=['bad sim']
        ),
        QuarantineElement(
            scenarios=['all'],
            platforms=['all'],
            architectures=['unsupported arch 1'],
            simulations=['all']
        ),
    ]

    quarantine_data = QuarantineData(qlist)

    if expected_idx is None:
        assert quarantine_data.get_matched_quarantine(
            scenario=scenario,
            platform=platform,
            architecture=architecture,
            simulator_name=simulation
        ) is None
    else:
        assert quarantine_data.get_matched_quarantine(
            scenario=scenario,
            platform=platform,
            architecture=architecture,
            simulator_name=simulation
        ) == qlist[expected_idx]
