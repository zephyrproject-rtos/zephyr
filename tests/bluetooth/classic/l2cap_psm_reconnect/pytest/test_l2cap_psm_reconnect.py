# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    L2CAP_SERVER_PSM_BASIC,
    logger,
)


def l2cap_br_clear(client, server):
    client.exec_command("br clear all")
    server.exec_command("br clear all")


def test_l2cap_psm_reconnect(client, server):
    """
    L2CAP connection based the same psm twice, and the second time is fail.
    """

    l2cap_br_clear(client, server)
    time.sleep(1)

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)

    logger.info(f"first connect, psm is {L2CAP_SERVER_PSM_BASIC}")
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} basic', 'Channel 0 connected'
    )

    logger.info(f"second connect, psm is {L2CAP_SERVER_PSM_BASIC}")
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} basic', 'Channel 1 disconnected'
    )
