# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging

from bumble.pairing import PairingDelegate

from utility.common import wait_for_shell_response

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


async def bumble_l2cap_disconnect(shell, dut, incoming_channel, iut_id=L2CAP_CHAN_IUT_ID):
    try:
        await incoming_channel.disconnect()
    except Exception as e:
        logger.error('Fail to send l2cap disconnect command!')
        raise e
    assert incoming_channel.disconnection_result is None

    found, _ = await wait_for_shell_response(dut, f"Channel {iut_id} disconnected")
    assert found is True
