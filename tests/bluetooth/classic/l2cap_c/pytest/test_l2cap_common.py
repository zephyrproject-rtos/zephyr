# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging

from bumble.core import BT_BR_EDR_TRANSPORT
from bumble.hci import (
    HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR,
)
from bumble.pairing import PairingDelegate

logger = logging.getLogger(__name__)

L2CAP_SERVER_PSM = 0x1001
L2CAP_CHAN_IUT_ID = 0
MAX_MTU = 0xFFFF
MIN_MTU = 0x30
STRESS_TEST_MAX_COUNT = 50


class Delegate(PairingDelegate):
    def __init__(
        self,
        dut,
        io_capability,
    ):
        super().__init__(
            io_capability,
        )
        self.dut = dut


async def device_power_on(device) -> None:
    while True:
        try:
            await device.power_on()
            break
        except Exception:
            continue


async def wait_for_shell_response(dut, message, max_time=5):
    found = False
    lines = []
    try:
        for _ in range(0, max_time):
            if found is False:
                read_lines = dut.readlines()
                for line in read_lines:
                    if message in line:
                        found = True
                        break
                lines = lines + read_lines
                await asyncio.sleep(1)
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e
    return found, lines


async def send_cmd_to_iut(shell, dut, cmd, parse=None, max_time=10):
    found = False
    lines = shell.exec_command(cmd)
    if parse is not None:
        for line in lines:
            if parse in line:
                found = True
        if found is False:
            found, lines = await wait_for_shell_response(dut, parse, max_time)
    else:
        found = True
    logger.info(f'{lines}')
    assert found is True
    return lines


async def bumble_acl_connect(shell, dut, device, target_address):
    connection = None
    try:
        connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
        logger.info(f'=== Connected to {connection.peer_address}!')
    except Exception as e:
        logger.error(f'Fail to connect to {target_address}!')
        raise e
    return connection


async def bumble_acl_disconnect(shell, dut, device, connection):
    await device.disconnect(
        connection, reason=HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR
    )
    found, lines = await wait_for_shell_response(dut, "Disconnected:")
    logger.info(f'lines : {lines}')
    assert found is True
    return found, lines


async def bumble_l2cap_disconnect(shell, dut, incoming_channel, iut_id=L2CAP_CHAN_IUT_ID):
    try:
        await incoming_channel.disconnect()
    except Exception as e:
        logger.error('Fail to send l2cap disconnect command!')
        raise e
    assert incoming_channel.disconnection_result is None

    found, _ = await wait_for_shell_response(dut, f"Channel {iut_id} disconnected")
    assert found is True
