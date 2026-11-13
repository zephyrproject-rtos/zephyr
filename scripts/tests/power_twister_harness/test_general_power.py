# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import importlib.util
import os
from pathlib import Path
from types import SimpleNamespace

import pytest

pytest_plugins = ['pytester']

ZEPHYR_BASE = Path(__file__).resolve().parents[3]
HARNESS_DIR = ZEPHYR_BASE / 'scripts' / 'pylib' / 'power-twister-harness'
TWISTER_DIR = ZEPHYR_BASE / 'scripts' / 'pylib' / 'pytest-twister-harness' / 'src'


@pytest.fixture
def harness(monkeypatch):
    monkeypatch.setenv('ZEPHYR_BASE', str(ZEPHYR_BASE))
    monkeypatch.setenv('PYTHONPATH', os.pathsep.join((str(HARNESS_DIR), str(TWISTER_DIR))))
    monkeypatch.syspath_prepend(str(TWISTER_DIR))
    monkeypatch.syspath_prepend(str(HARNESS_DIR))

    import general_power

    spec = importlib.util.spec_from_file_location(
        'power_harness_test', HARNESS_DIR / 'test_power.py'
    )
    power_test = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(power_test)
    return SimpleNamespace(general=general_power, test=power_test)


@pytest.fixture
def probe(harness, tmp_path):
    monitor = harness.general.GeneralPowerShield()
    monitor.probe_settings = harness.general.default_probe_settings()
    monitor.is_initialized = True
    monitor.dft = SimpleNamespace(device_config=SimpleNamespace(build_dir=tmp_path))
    return monitor


def reading_log(*values, count=None):
    count = len(values) if count is None else count
    return (
        'ADC sequence reading [1]:\n'
        f'- adc@1234, channel 0, {count} sequence samples:\n'
        + '\n'.join(f'- - {raw} = {voltage}mV' for raw, voltage in values)
    )


def set_sample(probe, harness, log):
    parsed = probe._parse_adc_data_log(log)
    probe.samples = [harness.general.ADCReadingsData.from_dict(parsed)]


def run_power_test(probe, harness, monkeypatch):
    monkeypatch.setattr(probe, 'measure', lambda duration: None)
    dut = SimpleNamespace(
        device_config=SimpleNamespace(
            build_dir=probe.dft.device_config.build_dir, platform='test/platform'
        )
    )
    harness.test.test_power_harness(probe, {'measurement_duration': 1}, (dut, probe.dft))


@pytest.mark.parametrize('scope', ['function', 'session'])
def test_fixture_scope_registration(harness, pytester, scope):
    pytester.makeconftest(
        (HARNESS_DIR / 'conftest.py').read_text(encoding='utf-8')
        + '\n_add_power_options = pytest_addoption\n'
        'def pytest_addoption(parser):\n'
        '    _add_power_options(parser)\n'
        f'    parser.addoption("--dut-scope", default="{scope}")\n'
        + '\n@pytest.fixture(scope="session")\n'
        'def duts():\n'
        '    return [object()]\n'
    )
    pytester.makepyfile(
        'def test_measurement_duts(measurement_duts):\n'
        '    assert measurement_duts[0] is not None\n'
        '    assert measurement_duts[1] is None\n'
    )
    result = pytester.runpytest_subprocess('-q')
    result.assert_outcomes(passed=1)


@pytest.mark.parametrize('values', [[(-10, -5)], [(10, -5)], [(-10, 5)], [(0, 0)]])
def test_signed_readings(harness, values):
    parsed = harness.general.GeneralPowerShield._parse_adc_data_log(reading_log(*values))
    readings = parsed['adcs']['adc@1234']['channel_0']['readings']
    assert readings == [{'raw': raw, 'voltage_mv': voltage} for raw, voltage in values]


@pytest.mark.parametrize(
    'log',
    [
        '',
        'unrelated log',
        '- adc@1234, channel 0, 1 sequence samples:\n- - 10 = 5mV',
        'ADC sequence reading [1]:',
        'ADC sequence reading [1]:\n- adc@bad header',
        reading_log(count=0),
        reading_log(count=1),
        reading_log((10, 5), count=2),
        reading_log((10, 5), (10, 5), count=1),
        reading_log((10, 5)).replace('- - 10 = 5mV', '- - invalid'),
    ],
)
def test_reject_invalid_adc_response(harness, log):
    with pytest.raises(ValueError):
        harness.general.GeneralPowerShield._parse_adc_data_log(log)


@pytest.mark.parametrize('count, values', [(0, []), (1, []), (2, [5])])
def test_reject_invalid_channel_readings(harness, count, values):
    channel = harness.general.ChannelReadings(
        count, [harness.general.Reading(10, value) for value in values]
    )
    with pytest.raises(ValueError, match='sample count'):
        channel.get_average_voltage()


def test_empty_measurement_fails_without_csv(probe, harness, monkeypatch, tmp_path):
    with pytest.raises(ValueError, match='No ADC samples'):
        run_power_test(probe, harness, monkeypatch)
    assert not list(tmp_path.glob('*.csv'))


def test_malformed_measurement_fails_without_csv(probe, tmp_path):
    probe.dft.write = lambda data: None
    probe.dft.readlines_until = lambda **kwargs: reading_log(count=1).splitlines()
    with pytest.raises(ValueError, match='incomplete sample count'):
        probe.measure(1)
    assert probe.samples == []
    assert not list(tmp_path.glob('*.csv'))


def test_missing_channel_fails_without_csv(probe, harness, monkeypatch, tmp_path):
    set_sample(probe, harness, reading_log((10, 5)))
    with pytest.raises(ValueError, match='Missing configured ADC channels'):
        run_power_test(probe, harness, monkeypatch)
    assert not list(tmp_path.glob('*.csv'))


def test_differential_current_only(probe, harness, monkeypatch, tmp_path):
    probe.probe_settings.routes[0].type = 'diff'
    assert probe.probe_settings.channel_set == {0}
    set_sample(probe, harness, reading_log((-10, -5)))
    assert probe.get_data() == {'route_0': [-50.0]}
    assert probe.voltage_mv == {}
    run_power_test(probe, harness, monkeypatch)
    outputs = list(tmp_path.glob('*.csv'))
    assert len(outputs) == 1
    assert '_current_data_' in outputs[0].name
    assert outputs[0].read_text().splitlines() == ['Sample_Index,route_0_Current_mA', '0,-50.0']


@pytest.mark.parametrize('voltage, current, power', [(5, 50.0, 0.25), (0, 0.0, 0.0)])
def test_single_ended_outputs(probe, harness, monkeypatch, tmp_path, voltage, current, power):
    log = reading_log((10, voltage)) + ('\n- adc@1234, channel 1, 1 sequence samples:\n- - 0 = 0mV')
    set_sample(probe, harness, log)
    run_power_test(probe, harness, monkeypatch)
    expected = {'current': current, 'voltage': float(voltage), 'power': power}
    for kind, value in expected.items():
        outputs = list(tmp_path.glob(f'*_{kind}_data_*.csv'))
        assert len(outputs) == 1
        assert outputs[0].read_text().splitlines()[1] == f'0,{value}'


def test_mixed_routes(probe, harness, monkeypatch, tmp_path):
    probe.probe_settings.routes.append(
        harness.general.RouteConfig(
            id=1,
            name='diff',
            shunt_resistor=0.1,
            type='diff',
            channels=harness.general.ChannelPair(channels_p=2, channels_n=3),
        )
    )
    log = reading_log((10, 5)) + (
        '\n- adc@1234, channel 1, 1 sequence samples:\n- - 0 = 0mV'
        '\n- adc@1234, channel 2, 1 sequence samples:\n- - -10 = -5mV'
    )
    set_sample(probe, harness, log)
    assert probe.get_data() == {'route_0': [50.0], 'diff': [-50.0]}
    assert probe.voltage_mv == {'route_0': [5.0]}
    run_power_test(probe, harness, monkeypatch)
    current = next(tmp_path.glob('*_current_data_*.csv')).read_text().splitlines()
    assert current == ['Sample_Index,route_0_Current_mA,diff_Current_mA', '0,50.0,-50.0']
    for kind in ('voltage', 'power'):
        assert 'diff' not in next(tmp_path.glob(f'*_{kind}_data_*.csv')).read_text()


@pytest.mark.parametrize('data', [{}, {'0': []}, {'0': [5], '1': []}, {'0': [5, 6], '1': [1]}])
def test_reject_incomplete_routes(probe, data):
    with pytest.raises(ValueError):
        probe._process_routes(data)


def test_missing_channel_is_not_paired_across_sequences(probe, harness):
    first = probe._parse_adc_data_log(reading_log((10, 5)))
    second = probe._parse_adc_data_log(reading_log((10, 5)).replace('channel 0', 'channel 1'))
    probe.samples = [
        harness.general.ADCReadingsData.from_dict(sample) for sample in (first, second)
    ]
    with pytest.raises(ValueError, match='Missing configured ADC channels'):
        probe.get_data()


def test_no_routes_fails(probe, harness):
    set_sample(probe, harness, reading_log((10, 5)))
    probe.probe_settings.routes = []
    with pytest.raises(ValueError, match='No ADC routes'):
        probe.get_data()


def test_stm_measurement_still_validates_rms(harness, tmp_path, monkeypatch):
    monitor = SimpleNamespace(
        measure=lambda duration: None,
        get_data=lambda: [0.001],
        power_shield_conf=SimpleNamespace(),
    )
    dut = SimpleNamespace(device_config=SimpleNamespace(build_dir=tmp_path, platform='test'))
    monkeypatch.setattr(harness.test, 'current_RMS', lambda *args, **kwargs: [0.001])
    data = {
        'measurement_duration': 1,
        'elements_to_trim': 0,
        'num_of_transitions': 1,
        'min_peak_distance': 1,
        'min_peak_height': 0,
        'peak_padding': 0,
        'expected_rms_values': [1.0],
        'tolerance_percentage': 20,
    }
    harness.test.test_power_harness(monitor, data, (dut, None))
    assert monitor.power_shield_conf.output_file == str(tmp_path / 'power_raw_data.csv')
    data['expected_rms_values'] = [10.0]
    with pytest.raises(AssertionError, match='out of tolerance'):
        harness.test.test_power_harness(monitor, data, (dut, None))
