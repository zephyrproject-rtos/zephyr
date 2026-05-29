# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass
class ShellCommand:
    command: str
    expected: str | None = None


@dataclass
class RequiredDevice:
    platform: str | None = None
    fixture: list[str] = field(default_factory=list)
    application: str | None = None


@dataclass
class Record:
    regex: list[str] = field(default_factory=list)
    merge: bool | None = None
    as_json: list[str] = field(default_factory=list)


@dataclass
class HarnessConfig:
    power_measurements: Any | None = None
    shell_commands_file: str | None = None
    shell_commands: list[ShellCommand] = field(default_factory=list)
    display_capture_config: str | None = None
    type: str | None = None
    fixture: list[str] | None = None
    ordered: bool | None = None
    pytest_root: list[str] = field(default_factory=list)
    pytest_args: list[str] = field(default_factory=list)
    pytest_dut_scope: str | None = None
    required_devices: list[RequiredDevice] = field(default_factory=list)
    ctest_args: list[str] = field(default_factory=list)
    regex: list[str] = field(default_factory=list)
    robot_testsuite: Any | None = None  # schema has no type defined
    robot_option: Any | None = None  # schema has no type defined
    record: Record | None = None
    bsim_exe_name: str | None = None
    tests_scripts: list[str] = field(default_factory=list)
    ztest_suite_repeat: int | None = None
    ztest_test_repeat: int | None = None
    ztest_test_shuffle: bool = False

    def __post_init__(self):
        if self.fixture and isinstance(self.fixture, str):
            self.fixture = [self.fixture]
        if not self.pytest_root:
            self.pytest_root = ['pytest']

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> HarnessConfig:
        """Create HarnessConfig from dictionary loaded from YAML."""
        if 'shell_commands' in data:
            data['shell_commands'] = [ShellCommand(**cmd) for cmd in data['shell_commands']]
        if 'required_devices' in data:
            data['required_devices'] = [RequiredDevice(**dev) for dev in data['required_devices']]
        return cls(**data)

    # Below methods added to allow dict-like access to the dataclass fields,
    # which is used in some places in the codebase
    def get(self, key: str, default=None):
        value = getattr(self, key, default)
        return default if value is None else value

    def __getitem__(self, key: str):
        return getattr(self, key)

    def __setitem__(self, key, value):
        setattr(self, key, value)


@dataclass
class RequiredApplication:
    name: str
    platform: str | None = None
