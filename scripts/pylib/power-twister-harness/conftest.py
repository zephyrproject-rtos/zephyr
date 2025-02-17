# Copyright: (c)  2025, Intel Corporation
import pytest
from twister_harness import DeviceAdapter


def pytest_addoption(parser):
    parser.addoption('--testdata')


@pytest.fixture
def probe_class(request):
    # Return the class based on the request
    if request.param == 'stm_powershield':
        from scripts.pm.stm32l562e_dk.PowerShield import PowerShield

        return PowerShield


@pytest.fixture(name='probe_path', scope='session')
def fixture_probe_path(request, dut: DeviceAdapter):
    for fixture in dut.device_config.fixtures:
        if fixture.startswith('pm_probe'):
            probe_path = fixture.split(':')[1]
    return probe_path
