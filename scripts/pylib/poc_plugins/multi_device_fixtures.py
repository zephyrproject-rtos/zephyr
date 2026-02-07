# Copyright (c) 2023 Nordic Semiconductor ASA
# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import copy
import logging
import time
from collections.abc import Generator
from pathlib import Path

import pytest
from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.hardware_adapter import HardwareAdapter
from twister_harness.fixtures import determine_scope
from twister_harness.helpers.shell import Shell
from twister_harness.helpers.utils import find_in_config
from twister_harness.twister_harness_config import DeviceConfig, TwisterHarnessConfig
from twisterlib.hardwaremap import HardwareMap

logger = logging.getLogger(__name__)


class DefaultTwisterOptionsWrapper:
    """
    Default wrapper class for Twister options.

    These default parameters will be overwritten by the attributes of the hardware object,
    which is loaded with the HardwareMap class from the multi_instance_harness Twister fixture,
    as defined in the hardware map YAML file.
    """

    device_flash_timeout: float = 60.0  # [s]
    device_flash_with_test: bool = True
    flash_before: bool = False


class TwisterEnvWrapper:
    """Wrapper class for twister environment options"""

    def __init__(self, options: DefaultTwisterOptionsWrapper):
        self.options = options


class HarnessHardwareAdapter(HardwareAdapter):
    """Adapter class for real device."""

    def __init__(self, device_config: DeviceConfig) -> None:
        super().__init__(device_config)

        self.device_log_path: Path = device_config.build_dir / f"device_{device_config.id}.log"
        self.handler_log_path: Path = device_config.build_dir / f"handler_{device_config.id}.log"

        self._log_files: list[Path] = [self.handler_log_path, self.device_log_path]


@pytest.fixture(scope=determine_scope)
def harness_build_dirs(request: pytest.FixtureRequest) -> list[str]:
    """
    return [build_dir] for one harness devices as default.
    if there are multiple harness devices, need to be overridden in test suite.

    such as, flash differernt build dirs for harness devices,
    the harness_build_dirs should be set as: [build_dir_1, build_dir_2, ...],
    build_dir_1 could be build_dir, or from required_build_dirs fixture.
    """
    build_dir = request.config.getoption('--build-dir')
    return [build_dir]


@pytest.fixture(scope=determine_scope)
def harness_devices(
    request: pytest.FixtureRequest,
    dut: DeviceAdapter,
    twister_harness_config: TwisterHarnessConfig,
    harness_build_dirs: list[str],
) -> Generator[DeviceAdapter, None, None]:
    """
    Create a list of harness device objects.

    Args:
        request (pytest.FixtureRequest): The pytest fixture request.
        dut (DeviceAdapter): The main device under test.
        twister_harness_config (TwisterHarnessConfig): The Twister Harness configuration.
        harness_build_dirs (list[str]): list of build directories for harness devices.

    Yields:
        list[DeviceAdapter]: list of harness device adapter instances.
    """
    multi_instance_harness: str | None = None
    for fixture in dut.device_config.fixtures:
        if fixture.startswith("multi_instance_harness"):
            multi_instance_harness = fixture.split(sep=":", maxsplit=1)[1]
            break
    if not multi_instance_harness:
        pytest.fail("No multi_instance_harness fixture, do not need to call this pytest fixture")
    if not Path(multi_instance_harness).exists():
        pytest.fail(f"multi_instance_harness: {multi_instance_harness} not found")

    # reuse twister HardwareMap class to load harness device config yaml
    env = TwisterEnvWrapper(DefaultTwisterOptionsWrapper())
    harness_device_hwm = HardwareMap(env)
    harness_device_hwm.load(multi_instance_harness)
    logger.info(f"harness_device_hwm[0]: {harness_device_hwm.duts[0]}")

    if not harness_device_hwm.duts or (len(harness_build_dirs) > len(harness_device_hwm.duts)):
        pytest.fail(
            "Please check the harness_devices yaml, the number of devices and images do not match"
        )
        return

    # Reuse most dut config for harness_device, only build and flash different app into them
    dut_device_config: DeviceConfig = twister_harness_config.devices[0]
    logger.info(f"dut_device_config: {dut_device_config}")

    harness_devices_list: list[DeviceAdapter] = []
    for index, harness_build_dir in enumerate(harness_build_dirs):
        harness_hw = harness_device_hwm.duts[index]
        harness_device_config = copy.deepcopy(dut_device_config)
        # Assign all attributes of harness_hw to harness_device_config
        for attr in vars(harness_hw):
            setattr(harness_device_config, attr, getattr(harness_hw, attr))
        harness_device_config.build_dir = Path(harness_build_dir)

        # Init harness device as DuT
        device_obj = HarnessHardwareAdapter(harness_device_config)
        device_obj.initialize_log_files(request.node.name)
        harness_devices_list.append(device_obj)

    try:
        for device_obj in harness_devices_list:
            device_obj.launch()
        yield harness_devices_list
    finally:
        for device_obj in harness_devices_list:
            device_obj.close()


@pytest.fixture(scope=determine_scope)
def harness_shells(harness_devices: list[DeviceAdapter]) -> list[Shell]:
    """
    Create a list of ready-to-use shell interfaces for harness devices.

    Args:
        harness_devices (list[DeviceAdapter]): list of harness device adapter instances.

    Returns:
        list[Shell]: list of shell interfaces for harness devices.
    """
    shells: list[Shell] = []
    for device_obj in harness_devices:
        shell_interface = Shell(device_obj, timeout=20.0)
        prompt = find_in_config(
            Path(device_obj.device_config.app_build_dir) / "zephyr" / ".config",
            "CONFIG_SHELL_PROMPT_UART",
        )
        if prompt:
            shell_interface.prompt = prompt
        logger.info("Wait for prompt")
        if not shell_interface.wait_for_prompt():
            pytest.fail("Prompt not found")
        if device_obj.device_config.type == "hardware":
            # After booting up the device, there might appear additional logs
            # after first prompt, so we need to wait and clear the buffer
            time.sleep(0.5)
            device_obj.clear_buffer()
        shells.append(shell_interface)
    return shells
