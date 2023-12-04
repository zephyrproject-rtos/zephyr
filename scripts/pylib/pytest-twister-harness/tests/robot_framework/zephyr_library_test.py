import pathlib
from unittest.mock import patch

import pytest
from twister_harness import DeviceAdapter
from twister_harness.device.binary_adapter import NativeSimulatorAdapter
from twister_harness.robot_framework.ZephyrLibrary import ZephyrLibrary
from twister_harness.twister_harness_config import DeviceConfig, TwisterHarnessConfig


@pytest.fixture
def twister_harness_config(tmp_path):
    device_config = DeviceConfig(type="custom", build_dir=tmp_path)
    twister_harness_config = TwisterHarnessConfig()
    twister_harness_config.devices = [device_config]
    yield twister_harness_config


@pytest.fixture
def zephyr_library(twister_harness_config):
    with patch.object(
        ZephyrLibrary,
        "_get_twister_harness_config",
        return_value=twister_harness_config,
    ):
        yield ZephyrLibrary()


def test__init__correctly_initialized(zephyr_library, twister_harness_config):
    assert zephyr_library.device is None
    assert zephyr_library.shell is None
    assert zephyr_library is not None

    assert zephyr_library.twister_harness_config == twister_harness_config


def test__run_device__correctly_setup(
    zephyr_library, shell_simulator_adapter: NativeSimulatorAdapter
):
    zephyr_library.device = shell_simulator_adapter

    zephyr_library.run_device()


def test__run_command__can_execute_commands(
    zephyr_library, shell_simulator_adapter: NativeSimulatorAdapter
):
    zephyr_library.device = shell_simulator_adapter

    zephyr_library.run_device()
    result = zephyr_library.run_command("zen")

    assert result is not None
    assert isinstance(result, list)
