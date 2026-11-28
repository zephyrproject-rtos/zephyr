# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for the fixture registry (twisterlib.fixtures)."""

import textwrap
from unittest import mock

import pytest
from jsonschema.exceptions import ValidationError
from twisterlib.error import ConfigurationError
from twisterlib.fixtures import FIXTURES_REGISTRY_PATH, FixtureRegistry
from twisterlib.testplan import TestPlan
from twisterlib.testsuitedata import HarnessConfig, RequiredDevice

REGISTRY_A = {
    'gpio_loopback': 'description: Two GPIO pins shorted together.\n',
    'usb_hci': 'description: A USB Bluetooth controller.\nnotes: Needs a host with usbip.\n',
}

REGISTRY_B = {
    'my_module_loopback': 'description: Loopback for an out-of-tree module.\n',
}


def write_registry(tmp_path, name, fixtures):
    """Create registry directory ``name`` holding one file per fixture."""
    path = tmp_path / name
    path.mkdir()
    for fixture, content in fixtures.items():
        (path / f'{fixture}.yaml').write_text(content)
    (path / 'README.rst').write_text('not a fixture\n')
    return str(path)


def test_in_tree_registry_loads():
    registry = FixtureRegistry()
    assert registry.dirs == [FIXTURES_REGISTRY_PATH]
    assert 'gpio_loopback' in registry
    assert 'bsim_multi_test' in registry


def test_registry_lookup(tmp_path):
    registry = FixtureRegistry([write_registry(tmp_path, 'a', REGISTRY_A)])

    assert 'gpio_loopback' in registry
    assert 'gpio_loopback:pins=1,2' in registry
    assert 'spi_loopback' not in registry
    assert registry.unknown(['usb_hci', 'spi_loopback', 'gpio_loopback', 'i2c_ram']) == [
        'spi_loopback',
        'i2c_ram',
    ]
    assert registry.fixtures['usb_hci']['notes'] == 'Needs a host with usbip.'


def test_registry_extends(tmp_path):
    registry = FixtureRegistry(
        [
            write_registry(tmp_path, 'a', REGISTRY_A),
            write_registry(tmp_path, 'b', REGISTRY_B),
        ]
    )
    assert 'my_module_loopback' in registry
    assert 'gpio_loopback' in registry


def test_registry_duplicate_name(tmp_path):
    with pytest.raises(ConfigurationError, match="'gpio_loopback' is already registered"):
        FixtureRegistry(
            [
                write_registry(tmp_path, 'a', REGISTRY_A),
                write_registry(tmp_path, 'b', REGISTRY_A),
            ]
        )


def test_registry_missing_dir(tmp_path):
    with pytest.raises(ConfigurationError, match='fixture registry directory not found'):
        FixtureRegistry([str(tmp_path / 'missing')])


def test_registry_bad_name(tmp_path):
    with pytest.raises(ConfigurationError, match='Bad-Name.yaml: fixture names consist of'):
        FixtureRegistry([write_registry(tmp_path, 'a', {'Bad-Name': 'description: x\n'})])


@pytest.mark.parametrize(
    'content',
    [
        'notes: x\n',
        'description: x\nwiring: y\n',
        'description: x\nreferences: not-a-list\n',
    ],
    ids=['no description', 'unknown key', 'bad references'],
)
def test_registry_schema(tmp_path, content):
    with pytest.raises(ValidationError):
        FixtureRegistry([write_registry(tmp_path, 'a', {'gpio_loopback': content})])


def make_suite(fixture=None, required_fixtures=None):
    harness_config = HarnessConfig(fixture=fixture)
    if required_fixtures:
        harness_config.required_devices = [RequiredDevice(fixture=required_fixtures)]
    return mock.Mock(name='suite', harness_config=harness_config)


@pytest.mark.parametrize(
    'fixture, required_fixtures',
    [
        (None, None),
        ('gpio_loopback', None),
        (['gpio_loopback', 'usb_hci'], None),
        (None, ['usb_hci']),
    ],
    ids=['none', 'string', 'list', 'required device'],
)
def test_check_suite_registered(tmp_path, fixture, required_fixtures):
    registry = FixtureRegistry([write_registry(tmp_path, 'a', REGISTRY_A)])
    registry.check_suite(make_suite(fixture, required_fixtures), 'tests.yaml')


@pytest.mark.parametrize(
    'fixture, required_fixtures, expected',
    [
        ('spi_loopback', None, 'spi_loopback'),
        (['gpio_loopback', 'spi_loopback', 'i2c_ram'], None, 'i2c_ram, spi_loopback'),
        ('gpio_loopback', ['nope'], 'nope'),
    ],
    ids=['string', 'list', 'required device'],
)
def test_check_suite_unregistered(tmp_path, fixture, required_fixtures, expected):
    registry = FixtureRegistry([write_registry(tmp_path, 'a', REGISTRY_A)])
    suite = make_suite(fixture, required_fixtures)
    suite.name = 'drivers.foo'
    with pytest.raises(ConfigurationError) as excinfo:
        registry.check_suite(suite, 'tests.yaml')
    assert str(excinfo.value).startswith('tests.yaml: ')
    assert f"'drivers.foo' uses fixtures not listed in the fixture registry: {expected}." in str(
        excinfo.value
    )


@pytest.mark.parametrize(
    'fixture, extra_registry, expected_errors',
    [
        ('gpio_loopback', False, 0),
        ('not_a_fixture', False, 1),
        ('my_module_loopback', False, 1),
        ('my_module_loopback', True, 0),
    ],
    ids=['registered', 'unregistered', 'out-of-tree unregistered', 'out-of-tree registered'],
)
def test_testplan_rejects_unregistered_fixture(
    class_env, tmp_path, fixture, extra_registry, expected_errors
):
    suite_dir = tmp_path / 'tests' / 'foo'
    suite_dir.mkdir(parents=True)
    (suite_dir / 'tests.yaml').write_text(
        textwrap.dedent(f"""\
        tests:
          dummy.foo:
            harness: console
            harness_config:
              fixture: {fixture}
        """)
    )

    class_env.test_roots = [str(tmp_path / 'tests')]
    if extra_registry:
        class_env.options.fixture_registry = [write_registry(tmp_path, 'b', REGISTRY_B)]
    testplan = TestPlan(class_env)

    count = testplan.add_testsuites()

    assert testplan.load_errors == expected_errors
    assert count == 1 - expected_errors


def test_hardware_map_fixture_warning(class_env, caplog):
    dut = mock.Mock(platform='frdm_k64f', id='1234', fixtures=['gpio_loopback:1', 'not_a_fixture'])
    class_env.hwm = mock.Mock(duts=[dut])
    testplan = TestPlan(class_env)

    testplan.check_hardware_map_fixtures()

    assert "frdm_k64f (1234): fixture 'not_a_fixture' is not listed" in caplog.text
    assert 'gpio_loopback' not in caplog.text
