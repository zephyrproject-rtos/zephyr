# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import textwrap
from pathlib import Path

import pytest

pytest_plugins = ['pytester']


@pytest.mark.parametrize(
    'import_path, class_name, device_type',
    [
        ('twister_harness.device.binary_adapter', 'NativeSimulatorAdapter', 'native'),
        ('twister_harness.device.qemu_adapter', 'QemuAdapter', 'qemu'),
        ('twister_harness.device.hardware_adapter', 'HardwareAdapter', 'hardware'),
    ],
    ids=[
        'native',
        'qemu',
        'hardware',
    ]
)
def test_if_adapter_is_chosen_properly(
    import_path: str,
    class_name: str,
    device_type: str,
    tmp_path: Path,
    twister_harness: str,
    pytester: pytest.Pytester,
):
    pytester.makepyfile(
        textwrap.dedent(
            f"""
            from twister_harness import DeviceAdapter
            from {import_path} import {class_name}

            def test_plugin(device_object):
                assert isinstance(device_object, DeviceAdapter)
                assert type(device_object) == {class_name}
            """
        )
    )

    build_dir = tmp_path / 'build_dir'
    os.mkdir(build_dir)
    pytester.syspathinsert(twister_harness)
    result = pytester.runpytest(
        '--twister-harness',
        f'--build-dir={build_dir}',
        f'--device-type={device_type}',
        '-p', 'twister_harness.plugin'
    )

    assert result.ret == 0
    result.assert_outcomes(passed=1, failed=0, errors=0, skipped=0)
