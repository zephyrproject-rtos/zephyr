#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for hardwaremap.py classes' methods
"""

import mock
import pytest
import sys

from pathlib import Path

from twisterlib.hardwaremap import(
    DUT,
    HardwareMap
)


@pytest.fixture
def mocked_hm():
    duts = [
        DUT(platform='p1', id=1, serial='s1', product='pr1', connected=True),
        DUT(platform='p2', id=2, serial='s2', product='pr2', connected=False),
        DUT(platform='p3', id=3, serial='s3', product='pr3', connected=True),
        DUT(platform='p4', id=4, serial='s4', product='pr4', connected=False),
        DUT(platform='p5', id=5, serial='s5', product='pr5', connected=True),
        DUT(platform='p6', id=6, serial='s6', product='pr6', connected=False),
        DUT(platform='p7', id=7, serial='s7', product='pr7', connected=True),
        DUT(platform='p8', id=8, serial='s8', product='pr8', connected=False)
    ]

    hm = HardwareMap(env=mock.Mock())
    hm.duts = duts
    hm.detected = duts[:5]

    return hm


TESTDATA_1 = [
    (
        {},
        {'baud': 115200, 'lock': mock.ANY, 'flash_timeout': 60},
        '<None (None) on None>'
    ),
    (
        {
            'id': 'dummy id',
            'serial': 'dummy serial',
            'serial_baud': 4400,
            'platform': 'dummy platform',
            'product': 'dummy product',
            'serial_pty': 'dummy serial pty',
            'connected': True,
            'runner_params': ['dummy', 'runner', 'params'],
            'pre_script': 'dummy pre script',
            'post_script': 'dummy post script',
            'post_flash_script': 'dummy post flash script',
            'runner': 'dummy runner',
            'flash_timeout': 30,
            'flash_with_test': True
        },
        {
            'lock': mock.ANY,
            'id': 'dummy id',
            'serial': 'dummy serial',
            'baud': 4400,
            'platform': 'dummy platform',
            'product': 'dummy product',
            'serial_pty': 'dummy serial pty',
            'connected': True,
            'runner_params': ['dummy', 'runner', 'params'],
            'pre_script': 'dummy pre script',
            'post_script': 'dummy post script',
            'post_flash_script': 'dummy post flash script',
            'runner': 'dummy runner',
            'flash_timeout': 30,
            'flash_with_test': True
        },
        '<dummy platform (dummy product) on dummy serial>'
    ),
]


@pytest.mark.parametrize(
    'kwargs, expected_dict, expected_repr',
    TESTDATA_1,
    ids=['no information', 'full information']
)
def test_dut(kwargs, expected_dict, expected_repr):
    d = DUT(**kwargs)

    assert d.available
    assert d.counter == 0

    d.available = False
    d.counter = 1

    assert not d.available
    assert d.counter == 1

    assert d.to_dict() == expected_dict
    assert d.__repr__() == expected_repr


TESTDATA_2 = [
    ('ghm.yaml', mock.ANY, mock.ANY, [], mock.ANY, mock.ANY, mock.ANY, 0,
     True, True, False, False, False, False, []),
    (None, False, 'hm.yaml', [], mock.ANY, mock.ANY, mock.ANY, 0,
     False, False, True, True, False, False, []),
    (None, True, 'hm.yaml', [], mock.ANY, mock.ANY, ['fix'], 1,
     False, False, True, False, False, True, ['p1', 'p3', 'p5', 'p7']),
    (None, True, 'hm.yaml', ['pX'], mock.ANY, mock.ANY, ['fix'], 1,
     False, False, True, False, False, True, ['pX']),
    (None, True, None, ['p'], 's', None, ['fix'], 1,
     False, False, False, False, True, True, ['p']),
    (None, True, None, ['p'], None, 'spty', ['fix'], 1,
     False, False, False, False, True, True, ['p']),
]


@pytest.mark.parametrize(
    'generate_hardware_map, device_testing, hardware_map, platform,' \
    ' device_serial, device_serial_pty, fixtures,' \
    ' return_code, expect_scan, expect_save, expect_load,' \
    ' expect_dump, expect_add_device, expect_fixtures, expected_platforms',
    TESTDATA_2,
    ids=['generate hardware map', 'existing hardware map',
         'device testing with hardware map, no platform',
         'device testing with hardware map with platform',
         'device testing with device serial',
         'device testing with device serial pty']
)
def test_hardwaremap_discover(
    caplog,
    mocked_hm,
    generate_hardware_map,
    device_testing,
    hardware_map,
    platform,
    device_serial,
    device_serial_pty,
    fixtures,
    return_code,
    expect_scan,
    expect_save,
    expect_load,
    expect_dump,
    expect_add_device,
    expect_fixtures,
    expected_platforms
):
    def mock_load(*args):
        mocked_hm.platform = platform

    mocked_hm.scan = mock.Mock()
    mocked_hm.save = mock.Mock()
    mocked_hm.load = mock.Mock(side_effect=mock_load)
    mocked_hm.dump = mock.Mock()
    mocked_hm.add_device = mock.Mock()

    mocked_hm.options.device_flash_with_test = True
    mocked_hm.options.device_flash_timeout = 15
    mocked_hm.options.pre_script = 'dummy pre script'
    mocked_hm.options.platform = platform
    mocked_hm.options.device_serial = device_serial
    mocked_hm.options.device_serial_pty = device_serial_pty
    mocked_hm.options.device_testing = device_testing
    mocked_hm.options.hardware_map = hardware_map
    mocked_hm.options.persistent_hardware_map = mock.Mock()
    mocked_hm.options.generate_hardware_map = generate_hardware_map
    mocked_hm.options.fixture = fixtures

    returncode = mocked_hm.discover()

    assert returncode == return_code

    if expect_scan:
        mocked_hm.scan.assert_called_once_with(
            persistent=mocked_hm.options.persistent_hardware_map
        )
    if expect_save:
        mocked_hm.save.assert_called_once_with(
            mocked_hm.options.generate_hardware_map
        )
    if expect_load:
        mocked_hm.load.assert_called_once_with(
            mocked_hm.options.hardware_map
        )
    if expect_dump:
        mocked_hm.dump.assert_called_once_with(
            connected_only=True
        )
    if expect_add_device:
        mocked_hm.add_device.assert_called_once()

    if expect_fixtures:
        assert all(
            [all(
                    [fixture in dut.fixtures for fixture in fixtures]
            ) for dut in mocked_hm.duts]
        )

    assert sorted(expected_platforms) == sorted(mocked_hm.options.platform)


def test_hardwaremap_summary(capfd, mocked_hm):
    selected_platforms = ['p0', 'p1', 'p6', 'p7']

    mocked_hm.summary(selected_platforms)

    expected = """
Hardware distribution summary:

| Board   |   ID |   Counter |
|---------|------|-----------|
| p1      |    1 |         0 |
| p7      |    7 |         0 |
"""

    out, err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    assert expected in out


TESTDATA_3 = [
    (True),
    (False)
]


@pytest.mark.parametrize(
    'is_pty',
    TESTDATA_3,
    ids=['pty', 'not pty']
)
def test_hardwaremap_add_device(is_pty):
    hm = HardwareMap(env=mock.Mock())

    serial = 'dummy'
    platform = 'p0'
    pre_script = 'dummy pre script'
    hm.add_device(serial, platform, pre_script, is_pty)

    assert len(hm.duts) == 1
    if is_pty:
        assert hm.duts[0].serial_pty == 'dummy' if is_pty else None
        assert hm.duts[0].serial is None
    else:
        assert hm.duts[0].serial_pty is None
        assert hm.duts[0].serial == 'dummy'


def test_hardwaremap_load():
    map_file = \
"""
- id: id0
  platform: p0
  product: pr0
  runner: r0
  flash_with_test: True
  flash_timeout: 15
  baud: 14400
  fixtures:
  - dummy fixture 1
  - dummy fixture 2
  connected: True
  serial: 'dummy'
- id: id1
  platform: p1
  product: pr1
  runner: r1
  connected: True
  serial_pty: 'dummy'
- id: id2
  platform: p2
  product: pr2
  runner: r2
  connected: True
"""
    map_filename = 'map-file.yaml'

    builtin_open = open

    def mock_open(*args, **kwargs):
        if args[0] == map_filename:
            return mock.mock_open(read_data=map_file)(*args, **kwargs)
        return builtin_open(*args, **kwargs)

    hm = HardwareMap(env=mock.Mock())
    hm.options.device_flash_timeout = 30
    hm.options.device_flash_with_test = False

    with mock.patch('builtins.open', mock_open):
        hm.load(map_filename)

    expected = {
        'id0': {
            'platform': 'p0',
            'product': 'pr0',
            'runner': 'r0',
            'flash_timeout': 15,
            'flash_with_test': True,
            'baud': 14400,
            'fixtures': ['dummy fixture 1', 'dummy fixture 2'],
            'connected': True,
            'serial': 'dummy',
            'serial_pty': None,
        },
        'id1': {
            'platform': 'p1',
            'product': 'pr1',
            'runner': 'r1',
            'flash_timeout': 30,
            'flash_with_test': False,
            'baud': 115200,
            'fixtures': [],
            'connected': True,
            'serial': None,
            'serial_pty': 'dummy',
        },
    }

    for dut in hm.duts:
        assert dut.id in expected
        assert all([getattr(dut, k) == v for k, v in expected[dut.id].items()])


TESTDATA_4 = [
    (
        True,
        'Linux',
        ['<p1 (pr1) on s1>', '<p2 (pr2) on s2>', '<p3 (pr3) on s3>',
         '<p4 (pr4) on s4>', '<p5 (pr5) on s5>',
         '<unknown (TI product) on /dev/serial/by-id/basic-file1>',
         '<unknown (product123) on dummy device>',
         '<unknown (unknown) on /dev/serial/by-id/basic-file2-link>']
    ),
    (
        True,
        'nt',
        ['<p1 (pr1) on s1>', '<p2 (pr2) on s2>', '<p3 (pr3) on s3>',
         '<p4 (pr4) on s4>', '<p5 (pr5) on s5>',
         '<unknown (TI product) on /dev/serial/by-id/basic-file1>',
         '<unknown (product123) on dummy device>',
         '<unknown (unknown) on /dev/serial/by-id/basic-file2>']
    ),
    (
        False,
        'Linux',
        ['<p1 (pr1) on s1>', '<p2 (pr2) on s2>', '<p3 (pr3) on s3>',
         '<p4 (pr4) on s4>', '<p5 (pr5) on s5>',
         '<unknown (TI product) on /dev/serial/by-id/basic-file1>',
         '<unknown (product123) on dummy device>',
         '<unknown (unknown) on /dev/serial/by-id/basic-file2>']
    )
]


@pytest.mark.parametrize(
    'persistent, system, expected_reprs',
    TESTDATA_4,
    ids=['linux persistent map', 'no map (not linux)', 'no map (nonpersistent)']
)
def test_hardwaremap_scan(
    caplog,
    mocked_hm,
    persistent,
    system,
    expected_reprs
):
    def mock_resolve(path):
        if str(path).endswith('-link'):
            return Path(str(path)[:-5])
        return path

    def mock_iterdir(path):
        return [
            Path(path / 'basic-file1'),
            Path(path / 'basic-file2-link')
        ]

    def mock_exists(path):
        return True

    mocked_hm.manufacturer = ['dummy manufacturer', 'Texas Instruments']
    mocked_hm.runner_mapping = {
        'dummy runner': ['product[0-9]+',],
        'other runner': ['other TI product', 'TI product']
    }

    comports_mock = [
        mock.Mock(
            manufacturer='wrong manufacturer',
            location='wrong location',
            serial_number='wrong number',
            product='wrong product',
            device='wrong device'
        ),
        mock.Mock(
            manufacturer='dummy manufacturer',
            location='dummy location',
            serial_number='dummy number',
            product=None,
            device='/dev/serial/by-id/basic-file2'
        ),
        mock.Mock(
            manufacturer='dummy manufacturer',
            location='dummy location',
            serial_number='dummy number',
            product='product123',
            device='dummy device'
        ),
        mock.Mock(
            manufacturer='Texas Instruments',
            location='serial1',
            serial_number='TI1',
            product='TI product',
            device='TI device1'
        ),
        mock.Mock(
            manufacturer='Texas Instruments',
            location='serial0',
            serial_number='TI0',
            product='TI product',
            device='/dev/serial/by-id/basic-file1'
        ),
    ]

    with mock.patch('platform.system', return_value=system), \
         mock.patch('serial.tools.list_ports.comports',
                    return_value=comports_mock), \
         mock.patch('twisterlib.hardwaremap.Path.resolve',
                    autospec=True, side_effect=mock_resolve), \
         mock.patch('twisterlib.hardwaremap.Path.iterdir',
                    autospec=True, side_effect=mock_iterdir), \
         mock.patch('twisterlib.hardwaremap.Path.exists',
                    autospec=True, side_effect=mock_exists):
        mocked_hm.scan(persistent)

    assert sorted([d.__repr__() for d in mocked_hm.detected]) == \
           sorted(expected_reprs)

    assert 'Scanning connected hardware...' in caplog.text
    assert 'Unsupported device (wrong manufacturer): %s' % comports_mock[0] \
           in caplog.text


TESTDATA_5 = [
    (
        None,
        [{
            'platform': 'p1',
            'id': 1,
            'runner': mock.ANY,
            'serial': 's1',
            'product': 'pr1',
            'connected': True
        },
        {
            'platform': 'p2',
            'id': 2,
            'runner': mock.ANY,
            'serial': 's2',
            'product': 'pr2',
            'connected': False
        },
        {
            'platform': 'p3',
            'id': 3,
            'runner': mock.ANY,
            'serial': 's3',
            'product': 'pr3',
            'connected': True
        },
        {
            'platform': 'p4',
            'id': 4,
            'runner': mock.ANY,
            'serial': 's4',
            'product': 'pr4',
            'connected': False
        },
        {
            'platform': 'p5',
            'id': 5,
            'runner': mock.ANY,
            'serial': 's5',
            'product': 'pr5',
            'connected': True
        }]
    ),
    (
        '',
        [{
            'serial': 's1',
            'baud': 115200,
            'platform': 'p1',
            'connected': True,
            'id': 1,
            'product': 'pr1',
            'lock': mock.ANY,
            'flash_timeout': 60
        },
        {
            'serial': 's2',
            'baud': 115200,
            'platform': 'p2',
            'id': 2,
            'product': 'pr2',
            'lock': mock.ANY,
            'flash_timeout': 60
        },
        {
            'serial': 's3',
            'baud': 115200,
            'platform': 'p3',
            'connected': True,
            'id': 3,
            'product': 'pr3',
            'lock': mock.ANY,
            'flash_timeout': 60
        },
        {
            'serial': 's4',
            'baud': 115200,
            'platform': 'p4',
            'id': 4,
            'product': 'pr4',
            'lock': mock.ANY,
            'flash_timeout': 60
        },
        {
            'serial': 's5',
            'baud': 115200,
            'platform': 'p5',
            'connected': True,
            'id': 5,
            'product': 'pr5',
            'lock': mock.ANY,
            'flash_timeout': 60
        }]
    ),
    (
"""
- id: 4
  platform: p4
  product: pr4
  connected: True
  serial: s4
- id: 0
  platform: p0
  product: pr0
  connected: True
  serial: s0
- id: 10
  platform: p10
  product: pr10
  connected: False
  serial: s10
- id: 5
  platform: p5-5
  product: pr5-5
  connected: True
  serial: s5-5
""",
        [{
            'id': 0,
            'platform': 'p0',
            'product': 'pr0',
            'connected': False,
            'serial': None
        },
        {
            'id': 4,
            'platform': 'p4',
            'product': 'pr4',
            'connected': True,
            'serial': 's4'
        },
        {
            'id': 5,
            'platform': 'p5-5',
            'product': 'pr5-5',
            'connected': False,
            'serial': None
        },
        {
            'id': 10,
            'platform': 'p10',
            'product': 'pr10',
            'connected': False,
            'serial': None
        },
        {
            'serial': 's1',
            'baud': 115200,
            'platform': 'p1',
            'connected': True,
            'id': 1,
            'product': 'pr1',
            'lock': mock.ANY,
            'flash_timeout': 60
        },
        {
            'serial': 's2',
            'baud': 115200,
            'platform': 'p2',
            'id': 2,
            'product': 'pr2',
            'lock': mock.ANY,
            'flash_timeout': 60
        },
        {
            'serial': 's3',
            'baud': 115200,
            'platform': 'p3',
            'connected': True,
            'id': 3,
            'product': 'pr3',
            'lock': mock.ANY,
            'flash_timeout': 60
        },
        {
            'serial': 's5',
            'baud': 115200,
            'platform': 'p5',
            'connected': True,
            'id': 5,
            'product': 'pr5',
            'lock': mock.ANY,
            'flash_timeout': 60
        }]
    ),
]


@pytest.mark.parametrize(
    'hwm, expected_dump',
    TESTDATA_5,
    ids=['no map', 'empty map', 'map exists']
)
def test_hardwaremap_save(mocked_hm, hwm, expected_dump):
    read_mock = mock.mock_open(read_data=hwm)
    write_mock = mock.mock_open()

    def mock_open(filename, mode):
        if mode == 'r':
            return read_mock()
        elif mode == 'w':
            return write_mock()


    mocked_hm.load = mock.Mock()
    mocked_hm.dump = mock.Mock()

    open_mock = mock.Mock(side_effect=mock_open)
    dump_mock = mock.Mock()

    with mock.patch('os.path.exists', return_value=hwm is not None), \
         mock.patch('builtins.open', open_mock), \
         mock.patch('twisterlib.hardwaremap.yaml.dump', dump_mock):
        mocked_hm.save('hwm.yaml')

    dump_mock.assert_called_once_with(expected_dump, mock.ANY, Dumper=mock.ANY,
                                      default_flow_style=mock.ANY)


TESTDATA_6 = [
    (
        ['p1', 'p3', 'p5', 'p7'],
        [],
        True,
        True,
"""
| Platform   |   ID | Serial device   |
|------------|------|-----------------|
| p1         |    1 | s1              |
| p3         |    3 | s3              |
| p5         |    5 | s5              |
"""
    ),
    (
        [],
        ['?', '??', '???'],
        False,
        False,
"""
| ?   |   ?? | ???   |
|-----|------|-------|
| p1  |    1 | s1    |
| p2  |    2 | s2    |
| p3  |    3 | s3    |
| p4  |    4 | s4    |
| p5  |    5 | s5    |
| p6  |    6 | s6    |
| p7  |    7 | s7    |
| p8  |    8 | s8    |
"""
    ),
]


@pytest.mark.parametrize(
    'filtered, header, connected_only, detected, expected_out',
    TESTDATA_6,
    ids=['detected no header', 'all with header']
)
def test_hardwaremap_dump(
    capfd,
    mocked_hm,
    filtered,
    header,
    connected_only,
    detected,
    expected_out
):
    mocked_hm.dump(filtered, header, connected_only, detected)

    out, err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    assert out.strip() == expected_out.strip()
