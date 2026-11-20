# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    L2CAP_SERVER_PSM_BASIC,
    L2CAP_SERVER_PSM_ERET,
    L2CAP_SERVER_PSM_FC,
    L2CAP_SERVER_PSM_RET,
    L2CAP_SERVER_PSM_STREAM,
    MODE_OPTION,
    logger,
)

MODE = "stream"


def l2cap_br_clear(client, server):
    client.exec_command("br clear all")
    server.exec_command("br clear all")


def test_l2cap_stream_mode_client_TC01(client, server):
    """
    L2CAP client Creation with valid mode.client create connection with
    parameters:mode=stream,mode_optional=false when server in different modes.
    """

    L2CAP_CHAN_IUT_ID = 0
    l2cap_br_clear(client, server)
    time.sleep(1)

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)

    logger.info(f'client create l2cap connection, mode = {MODE}, mode_option = false')

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_BASIC},mode=basic, mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} 0',
    )
    client.exec_command(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} {MODE}',
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_BASIC},mode=basic,mode_option = true')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} {MODE}',
        f'Channel {L2CAP_CHAN_IUT_ID} disconnected',
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_RET}, mode=ret,mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_RET))[2:]} 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_RET))[2:]} {MODE}',
        f'Channel {L2CAP_CHAN_IUT_ID} disconnected',
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_RET},mode=ret, mode_option = true')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_RET))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_RET))[2:]} {MODE}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_FC},mode=fc, mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_FC))[2:]} 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_FC))[2:]} {MODE}',
        f'Channel {L2CAP_CHAN_IUT_ID} disconnected',
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_FC},mode=fc, mode_option = true')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_FC))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_FC))[2:]} {MODE}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_ERET},mode=eret, mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} {MODE}',
        f'Channel {L2CAP_CHAN_IUT_ID} disconnected',
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_ERET},mode=eret, mode_option = true')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} {MODE}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(
        f'server register , psm = {L2CAP_SERVER_PSM_STREAM},mode=stream, mode_option = false'
    )
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} {MODE}',
        'It is streaming mode',
    )

    logger.info(
        f'server register , psm = {L2CAP_SERVER_PSM_STREAM},mode=stream, mode_option = true'
    )
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} {MODE}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    client.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_stream_mode_client_TC02(client, server):
    """
    L2CAP client Creation with valid mode.client create connection with
    parameters:mode=stream,mode_optional=true when server in different modes.
    """

    L2CAP_CHAN_IUT_ID = 0
    l2cap_br_clear(client, server)
    time.sleep(1)

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)

    logger.info(f'client create l2cap connection, mode = {MODE}, mode_option = true')

    logger.info(f'server register,psm = {L2CAP_SERVER_PSM_BASIC},mode=basic, mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} 0',
    )

    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} {MODE} {MODE_OPTION}',
        'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_BASIC},mode=basic,mode_option = true')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_BASIC))[2:]} {MODE} {MODE_OPTION}',
        'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_RET}, mode=ret,mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_RET))[2:]} 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_RET))[2:]} {MODE} {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_RET},mode=ret, mode_option = true')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_RET))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_RET))[2:]} {MODE} {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_FC},mode=fc, mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_FC))[2:]} 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_FC))[2:]} {MODE} {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_FC},mode=fc, mode_option = true')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_FC))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_FC))[2:]} {MODE} {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_ERET},mode=eret, mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} {MODE} {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(f'server register , psm = {L2CAP_SERVER_PSM_ERET},mode=eret, mode_option = true')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} {MODE} {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(
        f'server register , psm = {L2CAP_SERVER_PSM_STREAM},mode=stream, mode_option = false'
    )
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} {MODE} {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    logger.info(
        f'server register , psm = {L2CAP_SERVER_PSM_STREAM},mode=stream, mode_option = true'
    )
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 1',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} {MODE} {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID}', f'Channel {L2CAP_CHAN_IUT_ID} disconnected'
    )

    client.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_stream_mode_client_TC03(client, server):
    """
    When server in idle/busy status, l2cap client in idle status, Data send behavior.
    """

    L2CAP_CHAN_IUT_ID_STREAM = 0
    l2cap_br_clear(client, server)

    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_STREAM}, mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )

    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', r'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap connection. psm = {L2CAP_SERVER_PSM_STREAM}, \
            mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream', 'It is streaming mode'
    )

    logger.info("set server and client in idle status")
    server.exec_command(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )
    client.exec_command(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )

    logger.info("client send data1, server recv data1 successfully.")
    data1 = "client_send_data"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data1}  {str(hex(len(data1)))[2:]}'
    )
    server._wait_for_shell_response(f"{data1}")

    logger.info("set server in busy status")
    server.exec_command(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 1',
    )

    logger.info("client send data2, server recv data2 successfully.")
    data2 = "client_send_data2"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data2}  {str(hex(len(data2)))[2:]}'
    )
    server._wait_for_shell_response(f"{data1}")

    logger.info("client send data3, server recv data3 failly.")
    data3 = "client_send_data3"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data3}  {str(hex(len(data3)))[2:]}'
    )
    server._wait_for_shell_response(" to allocate buffer for SDU", timeout=5)

    logger.info("client send data4, server recv data4 failly.")
    data4 = "client_send_data4"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data4}  {str(hex(len(data4)))[2:]}'
    )
    server._wait_for_shell_response(" to allocate buffer for SDU", timeout=5)

    logger.info("set server in idle status")
    server.exec_command(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )

    logger.info("client send data5, server recv data1 successfully.")
    data5 = "client_send_data5"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data5}  {str(hex(len(data5)))[2:]}'
    )
    server._wait_for_shell_response(f"{data5}")

    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )
    client.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_stream_mode_client_TC04(client, server):
    """
    When server in idle status, l2cap client in idle/busy status, Data receive behavior.
    """

    L2CAP_CHAN_IUT_ID_STREAM = 0
    l2cap_br_clear(client, server)

    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_STREAM}, mode_option = false')
    server.exec_command(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )

    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', r'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap connection. psm = {L2CAP_SERVER_PSM_STREAM}, \
            mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream', 'It is streaming mode'
    )

    logger.info("set server and client in idle status")
    server.exec_command(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )
    client.exec_command(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )

    logger.info("server send data1, client recv data1 successfully.")
    data1 = "client_recv_data"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data1}  {str(hex(len(data1)))[2:]}'
    )
    client._wait_for_shell_response(f"{data1}")

    logger.info("set client in busy status")
    client.exec_command(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 1',
    )

    logger.info("server send data2, client recv data2 successfully.")
    data2 = "client_recv_data2"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data2}  {str(hex(len(data2)))[2:]}'
    )
    client._wait_for_shell_response(f"{data1}")

    logger.info("server send data3, client recv data3 failly.")
    data3 = "client_recv_data3"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data3}  {str(hex(len(data3)))[2:]}'
    )
    client._wait_for_shell_response(" to allocate buffer for SDU", timeout=5)

    logger.info("server send data4, client recv data4 failly.")
    data4 = "client_recv_data4"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data4}  {str(hex(len(data4)))[2:]}'
    )
    client._wait_for_shell_response(" to allocate buffer for SDU", timeout=5)

    logger.info("set client in idle status")
    client.exec_command(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
    )

    logger.info("server send data5, client recv data5 successfully.")
    data5 = "client_recv_data5"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data5}  {str(hex(len(data5)))[2:]}'
    )
    client._wait_for_shell_response(f"{data5}")

    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )
    client.iexpect('bt disconnect', r'Disconnected')
