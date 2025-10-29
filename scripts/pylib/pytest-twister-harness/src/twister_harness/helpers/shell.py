# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import re
import time

from dataclasses import dataclass, field
from inspect import signature

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.exceptions import TwisterHarnessTimeoutException

logger = logging.getLogger(__name__)


class Shell:
    """
    Helper class that provides methods used to interact with shell application.
    """

    def __init__(
        self, device: DeviceAdapter, prompt: str = 'uart:~$', timeout: float | None = None
    ) -> None:
        self._device: DeviceAdapter = device
        self.prompt: str = prompt
        self.base_timeout: float = timeout or device.base_timeout

    def wait_for_prompt(self, timeout: float | None = None) -> bool:
        """
        Send every 0.5 second "enter" command to the device until shell prompt
        statement will occur (return True) or timeout will be exceeded (return
        False).
        """
        timeout = timeout or self.base_timeout
        timeout_time = time.time() + timeout
        self._device.clear_buffer()
        while time.time() < timeout_time:
            self._device.write(b'\n')
            try:
                line = self._device.readline(timeout=0.5, print_output=False)
            except TwisterHarnessTimeoutException:
                # ignore read timeout and try to send enter once again
                continue
            if self.prompt in line:
                logger.debug('Got prompt')
                return True
        return False

    def exec_command(
        self, command: str, timeout: float | None = None, print_output: bool = True
    ) -> list[str]:
        """
        Send shell command to a device and return response. Method returns printout of
        the executed command.
        """
        timeout = timeout or self.base_timeout
        command_ext = f'{command}\n'
        regex_prompt = re.escape(self.prompt)
        regex_command = f'.*{re.escape(command)}'

        # Execute command
        self._device.clear_buffer()
        self._device.write(command_ext.encode())
        lines: list[str] = []
        # wait for device command print - it should be done immediately after sending
        # command to device.
        lines.extend(
            self._device.readlines_until(
                regex=regex_command, timeout=1.0, print_output=print_output
            )
        )

        # Send single enter to get next prompt after command execution, as it signals
        # that execution finished.
        self._device.write(b"\n")

        # wait for device command execution
        lines.extend(
            self._device.readlines_until(
                regex=regex_prompt, timeout=timeout, print_output=print_output
            )
        )
        return lines

    def get_filtered_output(self, command_lines: list[str]) -> list[str]:
        """
        Filter out prompts and log messages

        Take the output of exec_command, which can contain log messages and command prompts,
        and filter them to obtain only the command output.

        Example:
            >>> # equivalent to `lines = shell.exec_command("kernel version")`
            >>> lines = [
            >>>    'uart:~$',                    # filter prompts
            >>>    'Zephyr version 3.6.0',       # keep this line
            >>>    'uart:~$ <dbg> debug message' # filter log messages
            >>> ]
            >>> filtered_output = shell.get_filtered_output(output)
            >>> filtered_output
            ['Zephyr version 3.6.0']

        :param command_lines: List of strings i.e. the output of `exec_command`.
        :return: A list of strings containing, excluding prompts and log messages.
        """
        regex_filter = re.compile(
            '|'.join([re.escape(self.prompt), '<dbg>', '<inf>', '<wrn>', '<err>'])
        )
        return list(filter(lambda l: not regex_filter.search(l), command_lines))


@dataclass
class ShellMCUbootArea:
    name: str
    version: str
    image_size: str
    magic: str = 'unset'
    swap_type: str = 'none'
    copy_done: str = 'unset'
    image_ok: str = 'unset'

    @classmethod
    def from_kwargs(cls, **kwargs) -> ShellMCUbootArea:
        cls_fields = {field for field in signature(cls).parameters}
        native_args = {}
        for name, val in kwargs.items():
            if name in cls_fields:
                native_args[name] = val
        return cls(**native_args)


@dataclass
class ShellMCUbootCommandParsed:
    """
    Helper class to keep data from `mcuboot` shell command.
    """

    areas: list[ShellMCUbootArea] = field(default_factory=list)

    @classmethod
    def create_from_cmd_output(cls, cmd_output: list[str]) -> ShellMCUbootCommandParsed:
        """
        Factory to create class from the output of `mcuboot` shell command.
        """
        areas: list[dict] = []
        re_area = re.compile(r'(.+ area.*):\s*$')
        re_key = re.compile(r'(?P<key>.+):(?P<val>.+)')
        for line in cmd_output:
            if m := re_area.search(line):
                areas.append({'name': m.group(1)})
            elif areas:
                if m := re_key.search(line):
                    areas[-1][m.group('key').strip().replace(' ', '_')] = m.group('val').strip()
        data_areas: list[ShellMCUbootArea] = []
        for area in areas:
            data_areas.append(ShellMCUbootArea.from_kwargs(**area))

        return cls(data_areas)
