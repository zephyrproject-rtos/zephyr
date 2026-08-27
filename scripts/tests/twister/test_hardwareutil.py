# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for hardwareutil.py classes' methods
"""
# ruff: noqa: E501

from unittest import mock

import pytest
from twisterlib.error import NoDeviceAvailableException, TwisterException
from twisterlib.hardwaremap import DUT, HardwareMap
from twisterlib.hardwareutil import HardwareReservationManager
from twisterlib.testsuitedata import HarnessConfig, RequiredDevice


@pytest.fixture
def hwmap() -> HardwareMap:
    hwmap = HardwareMap(options=mock.Mock())
    # fmt: off
    hwmap.duts = [
        DUT(id='dut1', platform='platform1', fixtures=[], serial='serial1'),
        DUT(id='dut2', platform='platform1', fixtures=['fixture1', 'fixture2:param'], serial='serial2'),
        DUT(id='dut-with-core', platform='platform-with-core', serial='serial3'),
        DUT(id='dut-with-core', serial='serial3-core'),
    ]
    # fmt: on
    return hwmap


def test_hardwareutil_reserve_dut(hwmap: HardwareMap):
    hwmgr = HardwareReservationManager(hwmap, platform='platform1', harness_config=mock.Mock())
    # Reserve a DUT with platform1
    matched_duts = hwmgr._get_matched_duts('platform1')
    dut = hwmgr.reserve_dut(matched_duts)
    assert dut == hwmap.duts[0]
    assert dut.available == 0
    # check only one DUT is reserved
    assert sum(1 for d in hwmap.duts if d.available == 0) == 1

    # Reserve another DUT with platform1 and fixture2
    matched_duts = hwmgr._get_matched_duts('platform1', ['fixture1', 'fixture2'])
    dut2 = hwmgr.reserve_dut(matched_duts)
    assert dut2 == hwmap.duts[1]
    assert dut2.available == 0

    # Attempt to reserve a DUT with platform1 again
    with pytest.raises(NoDeviceAvailableException):
        matched_duts = hwmgr._get_matched_duts('platform1')
        hwmgr.reserve_dut(matched_duts)

    # Attempt to reserve a DUT with platform-with-core, so second should be set as unavailable
    duts_with_core = [dut for dut in hwmap.duts if dut.id == 'dut-with-core']
    matched_duts = hwmgr._get_matched_duts('platform-with-core')
    dut3 = hwmgr.reserve_dut(matched_duts)
    assert dut3 == duts_with_core[0]
    assert all(d.available == 0 for d in duts_with_core)


def test_hardwareutil_reserve_dut_less_failures(hwmap: HardwareMap):
    hwmgr = HardwareReservationManager(hwmap, platform='platform1', harness_config=mock.Mock())
    # Set failures for DUTs
    hwmap.duts[0].failures = 5
    hwmap.duts[1].failures = 2

    # Reserve a DUT with platform1
    matched_duts = hwmgr._get_matched_duts('platform1')
    dut = hwmgr.reserve_dut(matched_duts)
    assert dut == hwmap.duts[1]
    # check only one DUT is reserved
    assert sum(1 for d in hwmap.duts if d.available == 0) == 1


def test_hardwareutil_reserve_dut_exceptions(hwmap: HardwareMap):
    hwmgr = HardwareReservationManager(hwmap, platform='platform1', harness_config=mock.Mock())

    # Attempt to reserve a DUT with a non-existent platform
    with pytest.raises(TwisterException):
        hwmgr._get_matched_duts('non_existent_platform')

    # Attempt to reserve a DUT with platform1 and a non-existent fixture
    with pytest.raises(TwisterException):
        hwmgr._get_matched_duts('platform1', ['non_existent_fixture'])

    # Attempt to reserve a DUT with platform1 and one fixture matched but not the other
    with pytest.raises(TwisterException):
        hwmgr._get_matched_duts('platform1', ['fixture1', 'non_existent_fixture'])

    for dut in hwmap.duts:
        dut.available = 0
    with pytest.raises(NoDeviceAvailableException):
        matched_duts = hwmgr._get_matched_duts('platform1')
        hwmgr.reserve_dut(matched_duts)


def test_hardwareutil_release_dut(hwmap: HardwareMap):
    hwmgr = HardwareReservationManager(hwmap, platform='platform1', harness_config=mock.Mock())

    for dut in hwmap.duts:
        dut.available = 0

    hwmgr.release_dut(hwmap.duts[0])
    assert sum(1 for d in hwmap.duts if d.available == 1) == 1
    assert hwmap.duts[0].available == 1

    # release the second one which has the core sharing the same hardware ID and different serial,
    # so both should be released
    duts_with_core = [dut for dut in hwmap.duts if dut.id == 'dut-with-core']
    hwmgr.release_dut(duts_with_core[0])

    assert duts_with_core[0].available == 1
    assert duts_with_core[1].available == 1
    assert sum(1 for d in hwmap.duts if d.available == 1) == 3


@pytest.fixture
def hwmgr() -> HardwareReservationManager:
    hwmap = HardwareMap(options=mock.Mock())
    # fmt: off
    hwmap.duts = [
        DUT(id='dut1', platform='platform1', serial='serial1', fixtures=['fixture1:param1']),
        DUT(id='dut2', platform='platform1', serial='serial2'),
        DUT(id='dut3', platform='platform1', serial='serial3', fixtures=['fixture1:param2', 'fixture2']),
        DUT(id='dut4', platform='platform1', serial='serial4', fixtures=['fixture1:param3', 'fixture3']),
        DUT(id='dut5', platform='platform1', serial='serial5', fixtures=['fixture1:param2']),
        DUT(id='dut6', platform='platform1', serial='serial6', fixtures=['fixture1:param2', 'fixture2']),
        DUT(id='dut7', platform='platform1', serial='serial7', fixtures=['fixture1:param3']),
        DUT(id='dut8', platform='platform2', serial='serial8'),
    ]
    # fmt: on
    return HardwareReservationManager(hwmap, platform='platform1', harness_config=HarnessConfig())


@pytest.mark.parametrize(
    ('harness_config', 'reserved_duts'),
    [
        (HarnessConfig(), ['dut1']),  # basic
        (HarnessConfig(fixture='fixture1'), ['dut1']),  # dut_fixture
        (HarnessConfig(fixture='fixtureX'), []),  # no_matching_fixture
        (HarnessConfig(fixture=['fixture1', 'fixture2']), ['dut3']),  # dut_two_fixtures
        (  # required_devices
            HarnessConfig(required_devices=[RequiredDevice(), RequiredDevice()]),
            ['dut1', 'dut2', 'dut3'],
        ),
        (  # required_other_platform
            HarnessConfig(required_devices=[RequiredDevice(platform='platform2')]),
            ['dut1', 'dut8'],
        ),
        (  # required_not_found_platform
            HarnessConfig(required_devices=[RequiredDevice(platform='platformX')]),
            [],
        ),
        (  # fixture_and_required_devices
            HarnessConfig(
                fixture='fixture2', required_devices=[RequiredDevice(fixture=['fixture2'])]
            ),
            ['dut3', 'dut6'],
        ),
        (  # fixture_with_param_and_required_devices
            HarnessConfig(
                fixture=['fixture1'], required_devices=[RequiredDevice(fixture=['fixture1'])]
            ),
            ['dut3', 'dut5'],
        ),
        (  # two_fixtures_with_param_and_required_devices
            HarnessConfig(
                fixture=['fixture1', 'fixture2'],
                required_devices=[RequiredDevice(fixture=['fixture1', 'fixture2'])],
            ),
            ['dut3', 'dut6'],
        ),
        (  # check_if_required_dev_is_not_reserved_first
            HarnessConfig(
                fixture=['fixture1'],
                required_devices=[RequiredDevice(fixture=['fixture1', 'fixture3'])],
            ),
            ['dut7', 'dut4'],
        ),
        (  # no_req_devs_with_same_fixtures
            HarnessConfig(
                fixture=['fixture1', 'fixture3'],
                required_devices=[RequiredDevice(fixture=['fixture1', 'fixture3'])],
            ),
            [],
        ),
    ],
    ids=[
        'basic',
        'dut_fixture',
        'no_matching_fixture',
        'dut_two_fixtures',
        'required_devices',
        'required_other_platform',
        'required_not_found_platform',
        'fixture_and_required_devices',
        'fixture_with_param_and_required_devices',
        'two_fixtures_with_param_and_required_devices',
        'check_if_required_dev_is_not_reserved_first',
        'no_req_devs_with_same_fixtures',
    ],
)
def test_hardwareutil_reserve_duts(
    hwmgr: HardwareReservationManager, harness_config: HarnessConfig, reserved_duts: list[str]
):
    hwmgr.harness_config = harness_config

    can_run = hwmgr.is_runnable()
    if not reserved_duts:
        assert not can_run
        with pytest.raises(TwisterException):
            hwmgr.reserve_duts()
        return

    assert can_run
    hwmgr.reserve_duts()
    assert len(hwmgr.reserved_duts) == len(reserved_duts)
    assert reserved_duts == [d.id for d in hwmgr.reserved_duts]
    assert hwmgr.reserved_duts[0].counter == 1

    hwmgr.release_duts()
    assert len(hwmgr.reserved_duts) == 0
    assert all(d.available == 1 for d in hwmgr.hwm.duts)
