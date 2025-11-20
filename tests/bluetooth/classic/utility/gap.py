# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging

from bumble.core import BT_BR_EDR_TRANSPORT
from bumble.hci import (
    HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR,
)

from utility.common import wait_for_shell_response

logger = logging.getLogger(__name__)


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
