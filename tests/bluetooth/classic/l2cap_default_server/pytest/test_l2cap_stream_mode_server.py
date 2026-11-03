# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    L2CAP_SERVER_PSM_STREAM,
    MODE_OPTION,
    logger,
)


def l2cap_br_clear(client, server):
    client.exec_command("br clear all")
    server.exec_command("br clear all")


def test_l2cap_stream_mode_TC01(client, server):
    """
    L2CAP server create l2cap connection with parameters: mode = stream,mode_optional = false
    when l2cap client in different modes.
    """
    L2CAP_CHAN_IUT_ID_STREAM = 0
    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_STREAM}, mode_option = false')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} mode_optional 0",
    )

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', r'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = basic, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} basic',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = basic, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} basic {MODE_OPTION}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = ret, \
            mode_option = false. stack support eret.',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} ret',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = ret, \
            mode_option = true. stack support eret'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} ret {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = fc, \
            mode_option = false. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} fc',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = fc, \
            mode_option = true. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} fc {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = eret, \
            mode_option = false. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} eret',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = eret, \
            mode_option = true. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} eret {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = stream, mode_option = true.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_stream_mode_TC02(client, server):
    """
    L2CAP server create l2cap connection with parameters: mode = stream,mode_optional = true
    when l2cap client in different modes.
    """
    L2CAP_CHAN_IUT_ID_STREAM = 0
    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_STREAM}, mode_option = true')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 1',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} mode_optional 1",
    )

    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', r'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = basic, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} basic', 'It is basic mode'
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = basic, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} basic {MODE_OPTION}',
        'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = ret, \
            mode_option = false. stack support eret.',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} ret',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = ret, \
            mode_option = true. stack support eret'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} ret {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = fc, \
            mode_option = false. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} fc',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = fc, \
            mode_option = true. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} fc {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = eret, \
            mode_option = false. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} eret',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = eret, \
            mode_option = true. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} eret {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream', 'It is streaming mode'
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = stream, mode_option = true.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_stream_mode_TC03(client, server):
    """
    When client in idle/busy status, l2cap server in idle status, Data send behavior
    """
    L2CAP_CHAN_IUT_ID_STREAM = 0
    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_STREAM}, mode_option = false')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} mode_optional 0",
    )

    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', r'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream', 'It is streaming mode'
    )

    logger.info("set server and client in idle status")
    server.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} appl status 0",
    )
    client.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} appl status 0",
    )

    logger.info("server send data1, client recv data1 successfully.")
    data1 = "server_send_data"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data1}  {str(hex(len(data1)))[2:]}'
    )
    client._wait_for_shell_response(f"{data1}")

    logger.info("set client in busy status")
    client.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 1',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} appl status 1",
    )

    logger.info("server send data2, client recv data2 successfully.")
    data2 = "server_send_data2"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data2}  {str(hex(len(data2)))[2:]}'
    )
    client._wait_for_shell_response(f"{data1}")

    logger.info("server send data3, client recv data3 fail.")
    data3 = "server_send_data3"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data3}  {str(hex(len(data3)))[2:]}'
    )
    client._wait_for_shell_response(" to allocate buffer for SDU", timeout=5)

    logger.info("server send data4, client recv data4 fail.")
    data4 = "server_send_data4"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data4}  {str(hex(len(data4)))[2:]}'
    )
    client._wait_for_shell_response(" to allocate buffer for SDU", timeout=5)

    logger.info("set client in idle status")
    client.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} appl status 0",
    )

    logger.info("server send data5, client recv data1 successfully.")
    data5 = "server_send_data5"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data5}  {str(hex(len(data5)))[2:]}'
    )
    client._wait_for_shell_response(f"{data5}")

    server.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )
    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_stream_mode_TC04(client, server):
    """
    When client in idle/busy status, l2cap server in idle status, Data receive behavior
    """
    L2CAP_CHAN_IUT_ID_STREAM = 0
    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_STREAM}, mode_option = false')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} mode_optional 0",
    )

    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', r'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_STREAM}, mode = stream, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} stream', 'It is streaming mode'
    )

    logger.info("set server and client in idle status")
    server.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} appl status 0",
    )
    client.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} appl status 0",
    )

    logger.info("client send data1, server recv data1 successfully.")
    data1 = "server_recv_data"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data1}  {str(hex(len(data1)))[2:]}'
    )
    server._wait_for_shell_response(f"{data1}")

    logger.info("set server in busy status")
    server.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 1',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} appl status 1",
    )

    logger.info("client send data2, server recv data2 successfully.")
    data2 = "server_recv_data2"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data2}  {str(hex(len(data2)))[2:]}'
    )
    server._wait_for_shell_response(f"{data1}")

    logger.info("client send data3, server recv data3 fail.")
    data3 = "server_recv_data3"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data3}  {str(hex(len(data3)))[2:]}'
    )
    server._wait_for_shell_response(" to allocate buffer for SDU", timeout=5)

    logger.info("client send data4, server recv data4 fail.")
    data4 = "server_recv_data4"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data4}  {str(hex(len(data4)))[2:]}'
    )
    server._wait_for_shell_response(" to allocate buffer for SDU", timeout=5)

    logger.info("set server in idle status")
    server.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_STREAM))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_STREAM))} appl status 0",
    )

    logger.info("client send data5, server recv data5 successfully.")
    data5 = "server_recv_data5"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_STREAM} {data5}  {str(hex(len(data5)))[2:]}'
    )
    server._wait_for_shell_response(f"{data5}")

    server.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_STREAM}',
        f'Channel {L2CAP_CHAN_IUT_ID_STREAM} disconnected',
    )
    server.iexpect('bt disconnect', r'Disconnected')
