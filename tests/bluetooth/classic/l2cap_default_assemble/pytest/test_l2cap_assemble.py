# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    L2CAP_SERVER_PSM_ERET,
    L2CAP_SERVER_PSM_STREAM,
    logger,
)

BT_L2CAP_MPS = 48


def l2cap_br_clear(client, server):
    client.exec_command("br clear all")
    server.exec_command("br clear all")


def test_l2cap_eret_mode_TC07(client, server):
    """
    verify Reassembly from server and client side in eret mode.
    """
    L2CAP_CHAN_IUT_ID_ERET = 0

    l2cap_br_clear(client, server)

    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = eret, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} eret',
        'It is enhance retransmission mode',
    )

    logger.info('TEST client send data to server')

    logger.info('client send data, len = 0')
    data = "this_is_data"
    data_len = 0
    assert data_len == 0
    client.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data} {str(hex(data_len))}')
    server._wait_for_shell_response("len 0")

    logger.info('client send data1, len < BT_L2CAP_MPS')
    data1 = "this_is_data1"
    data_len = len(data1)
    assert data_len < BT_L2CAP_MPS
    client.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data1} {str(hex(data_len))}')
    server._wait_for_shell_response(f"{data1}")

    logger.info('client send data2, BT_L2CAP_MPS< len < 2*BT_L2CAP_MPS')
    data2 = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM1234567qwerty!@#"
    data_len = len(data2)
    assert BT_L2CAP_MPS < data_len < 2 * BT_L2CAP_MPS
    client.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data2} {str(hex(data_len))}')
    server._wait_for_shell_response(f"{data2}")

    logger.info('TEST server send data to client')

    logger.info('server send data, len = 0')
    data = "this_is_data"
    data_len = 0
    assert data_len == 0
    server.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data} {str(hex(data_len))}')
    client._wait_for_shell_response("len 0")

    logger.info('server send data3, len < BT_L2CAP_MPS')
    data3 = "this_is_server_send"
    data_len = len(data3)
    assert data_len < BT_L2CAP_MPS
    server.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data3} {str(hex(data_len))}')
    client._wait_for_shell_response(f"{data3}")

    logger.info('server send data4, BT_L2CAP_MPS< len < 2*BT_L2CAP_MPS')
    data4 = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM1234567qwerty_ab"
    data_len = len(data4)
    assert BT_L2CAP_MPS < data_len < 2 * BT_L2CAP_MPS
    server.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data4} {str(hex(data_len))}')
    client._wait_for_shell_response(f"{data4}")

    server.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_stream_mode_TC06(client, server):
    """
    verify Reassembly from server and client side in eret mode.
    """
    L2CAP_CHAN_IUT_ID_STREAM = 0

    l2cap_br_clear(client, server)

    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream', 'It is streaming mode'
    )

    logger.info('TEST client send data to server')

    logger.info('client send data, len = 0')
    data = "this_is_data"
    data_len = 0
    assert data_len == 0
    client.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data} {str(hex(data_len))}')
    server._wait_for_shell_response("len 0")

    logger.info('client send data1, len < BT_L2CAP_MPS')
    data1 = "this_is_data1"
    data_len = len(data1)
    assert data_len < BT_L2CAP_MPS
    client.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data1} {str(hex(data_len))}')
    server._wait_for_shell_response(f"{data1}")

    logger.info('client send data2, BT_L2CAP_MPS< len < BT_L2CAP_MPS < 2*BT_L2CAP_MPS')
    data2 = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM1234567qwerty!@#"
    data_len = len(data2)
    assert BT_L2CAP_MPS < data_len < 2 * BT_L2CAP_MPS
    client.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data2} {str(hex(data_len))}')
    server._wait_for_shell_response(f"{data2}")

    logger.info('TEST server send data to client')

    logger.info('client send data, len = 0')
    data = "this_is_data"
    data_len = 0
    assert data_len == 0
    server.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data} {str(hex(data_len))}')
    client._wait_for_shell_response("len 0")

    logger.info('client send data3, len < BT_L2CAP_MPS')
    data3 = "this_is_server_send"
    data_len = len(data3)
    assert data_len < BT_L2CAP_MPS
    server.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data3} {str(hex(data_len))}')
    client._wait_for_shell_response(f"{data3}")

    logger.info('server send data4, BT_L2CAP_MPS< len < BT_L2CAP_MPS < 2*BT_L2CAP_MPS')
    data4 = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM1234567qwerty_ab"
    data_len = len(data4)
    assert BT_L2CAP_MPS < data_len < 2 * BT_L2CAP_MPS
    server.exec_command(f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data4} {str(hex(data_len))}')
    client._wait_for_shell_response(f"{data4}")

    server.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')
