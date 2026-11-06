# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    L2CAP_SERVER_PSM_BASIC,
    MODE_OPTION,
    logger,
)


def l2cap_br_clear(client, server):
    client.exec_command("br clear all")
    server.exec_command("br clear all")


def test_l2cap_basic_mode_TC01(client, server):
    """
    L2CAP server create l2cap connection with
    parameters: mode = basic,mode_optional = false
    when l2cap client in different modes
    """

    L2CAP_CHAN_IUT_ID_BASIC = 0

    l2cap_br_clear(client, server)

    time.sleep(1)

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_BASIC}, mode_option = false')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} 0',
        f'psm {str(int(L2CAP_SERVER_PSM_BASIC))} mode_optional 0',
    )

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = basic, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} basic', 'It is basic mode'
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = basic, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} basic {MODE_OPTION}',
        r'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = ret, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} ret',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = ret, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} ret {MODE_OPTION}',
        r'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = fc, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} fc',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = fc, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} fc {MODE_OPTION}',
        r'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = eret, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} eret',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = eret, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} eret {MODE_OPTION}',
        r'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} stream',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = stream, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} stream {MODE_OPTION}',
        r'It is basic mode',
    )

    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_basic_mode_TC02(client, server):
    """
    l2CAP server create l2cap connection with
    parameters: mode = basic,mode_optional = true
    when l2cap client in different modes
    """

    L2CAP_CHAN_IUT_ID_BASIC = 0
    l2cap_br_clear(client, server)
    time.sleep(1)

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_BASIC}, mode_option = true')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} 1',
        f'psm {str(int(L2CAP_SERVER_PSM_BASIC))} mode_optional 1',
    )

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', r'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = basic, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} basic', 'It is basic mode'
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = basic, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} basic {MODE_OPTION}',
        'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = ret, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} ret',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = ret, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} ret {MODE_OPTION}',
        'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = fc, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} fc',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = fc, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} fc {MODE_OPTION}',
        'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = eret, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} eret',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = eret, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} eret {MODE_OPTION}',
        'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} stream',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_BASIC}, mode = stream, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} stream {MODE_OPTION}',
        'It is basic mode',
    )
    server.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_basic_mode_TC03(client, server):
    """
    L2CAP data which length from 0 to  {mtu} transfer successfully in basic mode.
    """

    L2CAP_CHAN_IUT_ID_BASIC = 0
    mtu = 50
    data = "1234567891234567891234567891233454656787899323455667"
    l2cap_br_clear(client, server)
    time.sleep(1)

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)

    logger.info('set mtu 50 in server and client')
    client.exec_command(f'l2cap_br change_mtu {str(hex(mtu))}')
    server.exec_command(f'l2cap_br change_mtu {str(hex(mtu))}')

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_BASIC},mode=basic, mode_option = false')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} 0',
        f'psm {str(int(L2CAP_SERVER_PSM_BASIC))} mode_optional 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} basic', 'It is basic mode'
    )

    logger.info("server send data which length from 0 to 50, client can recv data successfully.")
    for length in range(mtu + 1):
        server.exec_command(
            f'l2cap_br send {L2CAP_CHAN_IUT_ID_BASIC} {data}  {str(hex(length))[2:]}'
        )
        if length == 0:
            client._wait_for_shell_response("len 0", timeout=2)
        else:
            client._wait_for_shell_response("Incoming data :" + data[:length], timeout=2)

    logger.info("server send data which length is 51 fail")
    server.iexpect(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_BASIC} {data}  {str(hex(mtu + 1))[2:]}',
        "attempt to send",
    )

    logger.info("client send data which length from 0 to 50, server can recv data successfully.")
    for length in range(mtu + 1):
        client.exec_command(
            f'l2cap_br send {L2CAP_CHAN_IUT_ID_BASIC} {data}  {str(hex(length))[2:]}'
        )
        if length == 0:
            server._wait_for_shell_response("len 0", timeout=2)
        else:
            server._wait_for_shell_response("Incoming data :" + data[:length], timeout=2)

    server.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_BASIC}',
        f'Channel {L2CAP_CHAN_IUT_ID_BASIC} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')
