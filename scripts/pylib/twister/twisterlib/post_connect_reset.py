# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0
"""Start a device after Twister has flashed it and attached its console.

The configuration comes from the ``post_connect_reset`` entry of the hardware
map or of the board ``testing`` section. Supported methods:

- ``serial``: drive the DTR/RTS lines of the console port.
- ``west``: run ``west reset``.
- ``none``: do nothing.
"""

from __future__ import annotations

import logging
import subprocess
import time
from typing import Any

logger = logging.getLogger('twister')

DEFAULT_TIMEOUT_S = 30


class PostConnectResetError(Exception):
    pass


def resolve(
    dut_config: dict[str, Any] | None,
    platform_config: dict[str, Any] | None,
    serial_pty: bool = False,
) -> dict[str, Any] | None:
    """Return the configuration to use, or None when there is no post-connect reset.

    The hardware map entry takes precedence over the board one, and ``method: none`` disables
    the step. The serial method is skipped with ``serial_pty``, which has no DTR/RTS lines, so
    the runner resets the device as without this step.
    """
    config = dut_config or platform_config
    if config is None or config.get('method') == 'none':
        return None
    if serial_pty and config.get('method') == 'serial':
        logger.debug('Post-connect reset method serial skipped with serial_pty')
        return None
    return config


def is_serial(config: dict[str, Any] | None) -> bool:
    return config is not None and config.get('method') == 'serial'


def apply_serial_lines(ser: Any, state: dict[str, Any] | None) -> None:
    """Set the DTR/RTS lines listed in ``state``, leaving the others unchanged."""
    if not state:
        return
    if 'dtr' in state:
        ser.dtr = state['dtr']
    if 'rts' in state:
        ser.rts = state['rts']


def _run_command(command: list[str], timeout: float) -> None:
    logger.debug('Post-connect reset command: %s', ' '.join(command))
    try:
        subprocess.run(
            command, check=True, timeout=timeout, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        )
    except subprocess.CalledProcessError as exc:
        output = exc.stdout.decode(errors='ignore') if exc.stdout else ''
        raise PostConnectResetError(
            f'{command[0]} exited with {exc.returncode}: {output.strip()}'
        ) from exc
    except subprocess.TimeoutExpired as exc:
        raise PostConnectResetError(f'{command[0]} timed out after {timeout} s') from exc
    except OSError as exc:
        raise PostConnectResetError(f'Cannot run {command[0]}: {exc}') from exc


def run(
    config: dict[str, Any], ser: Any = None, west_reset_command: list[str] | None = None
) -> None:
    """Start the device with the configured method.

    :param config: ``post_connect_reset`` configuration
    :param ser: open serial port, required by the ``serial`` method
    :param west_reset_command: ``west reset`` command line, required by the ``west`` method
    :raises PostConnectResetError: if the device could not be started
    """
    method = config.get('method')
    timeout = config.get('timeout', DEFAULT_TIMEOUT_S)
    logger.debug('Post-connect reset, method: %s', method)

    if method == 'serial':
        if ser is None:
            raise PostConnectResetError('Method serial needs an open serial port')
        for step in config.get('sequence', []):
            apply_serial_lines(ser, step)
            if 'delay_ms' in step:
                time.sleep(step['delay_ms'] / 1000)
    elif method == 'west':
        if not west_reset_command:
            raise PostConnectResetError('Method west needs a west reset command')
        _run_command(west_reset_command, timeout)
    else:
        raise PostConnectResetError(f'Unknown method: {method}')
