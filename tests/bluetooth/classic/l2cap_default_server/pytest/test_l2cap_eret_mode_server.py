# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    L2CAP_SERVER_PSM_ERET,
    MODE_OPTION,
    logger,
)


def l2cap_br_clear(client, server):
    client.exec_command("br clear all")
    server.exec_command("br clear all")


def test_l2cap_eret_mode_TC01(client, server):
    """
    L2CAP server create l2cap connection with parameters: mode = eret,mode_optional = false
    when l2cap client in different modes.
    """
    L2CAP_CHAN_IUT_ID_ERET = 0
    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_ERET}, mode_option = false')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} mode_optional 0",
    )

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', r'Connected', timeout=10)

    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = basic, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} basic',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = basic, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} basic {MODE_OPTION}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET},\
            mode = ret, mode_option = false. stack support eret.',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} ret',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = ret, \
            mode_option = true. stack support eret'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} ret {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = fc, \
            mode_option = false. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} fc',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = fc, \
            mode_option = true. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} fc {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = eret, \
            mode_option = false. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} eret',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = eret, \
            mode_option = true. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} eret {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = stream, \
            mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} stream',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = stream, \
            mode_option = true.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} stream {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_eret_mode_TC02(client, server):
    """
    L2CAP server create l2cap connection with parameters: mode = eret,mode_optional = true
    when l2cap client in different modes.
    """
    L2CAP_CHAN_IUT_ID_ERET = 0
    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_ERET}, mode_option = true')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 1',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} mode_optional 1",
    )

    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)

    time.sleep(0.5)

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = basic, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} basic', 'It is basic mode'
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = basic, mode_option = true'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} basic {MODE_OPTION}',
        'It is basic mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = ret, \
            mode_option = false. stack support eret.',
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} ret',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = ret, \
            mode_option = true. stack support eret'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} ret {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = fc, \
            mode_option = false. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} fc',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = fc, \
            mode_option = true. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} fc {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = eret, \
            mode_option = false. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} eret',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = eret, \
            mode_option = true. stack support eret.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} eret {MODE_OPTION}',
        'It is enhance retransmission mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = stream, \
            mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} stream', 'It is streaming mode'
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = stream, \
            mode_option = true.'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} stream {MODE_OPTION}',
        'It is streaming mode',
    )
    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_eret_mode_TC03(client, server):
    """
    When client in idle status, l2cap server in idle/busy status, Data receive behavior.
    """
    L2CAP_CHAN_IUT_ID_ERET = 0
    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_ERET}, mode_option = false')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} mode_optional 0",
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = eret, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} eret',
        'It is enhance retransmission mode',
    )

    logger.info('set server and client appl status = idle')
    server.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} appl status 0",
    )
    client.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} appl status 0",
    )

    logger.info('client send data1, server recv data1 and result is successful')
    data1 = "server_recv_data"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data1}  {str(hex(len(data1)))[2:]}'
    )
    server._wait_for_shell_response(f"{data1}", timeout=12)

    logger.info('set l2cap server busy status')
    server.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 1',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} appl status 1",
    )

    logger.info('client send data2, server recv data2 and result is successful')
    data2 = "server_recv_data_2"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data2}  {str(hex(len(data2)))[2:]}'
    )
    server._wait_for_shell_response(f"{data2}", timeout=12)

    logger.info('client send data3, server recv data3 and result is fail')
    data3 = "server_recv_data_3"
    client.iexpect(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data3}  {str(hex(len(data3)))[2:]}',
        "RNR p=0 f=0 on chan",
    )
    server._wait_for_shell_response("to allocate buffer for SDU")

    logger.info('set l2cap server idle status')
    server.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} appl status 0",
    )
    server._wait_for_shell_response(f"{data3}", timeout=12)
    client._wait_for_shell_response(r'Retransmission I-frame', timeout=12)

    logger.info('client send data4, server recv data4 and result is successful')
    data4 = "server_recv_data_4"
    client.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data4} {str(hex(len(data4)))[2:]}'
    )
    server._wait_for_shell_response(f"{data4}", timeout=12)

    server.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )
    server.iexpect('bt disconnect', r'Disconnected')


def test_l2cap_eret_mode_TC04(client, server):
    """
    When client in idle/busy status, l2cap server in idle status, Data send behavior.
    """
    L2CAP_CHAN_IUT_ID_ERET = 0
    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)
    time.sleep(0.5)

    logger.info(f'server registered , psm = {L2CAP_SERVER_PSM_ERET}, mode_option = false')
    server.iexpect(
        f'l2cap_br modify_mop {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} mode_optional 0",
    )

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = eret, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} eret',
        'It is enhance retransmission mode',
    )

    logger.info('set server and client appl status = idle')
    server.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} appl status 0",
    )
    client.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} appl status 0",
    )

    logger.info('server send data1, client recv data1 and result is successful')
    data1 = "server_send_data"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data1}  {str(hex(len(data1)))[2:]}'
    )
    client._wait_for_shell_response(f"{data1}")

    logger.info('set l2cap client busy status')
    client.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 1',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} appl status 1",
    )

    logger.info('server send data2, client recv data2 and result is successful')
    data2 = "server_send_data_2"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data2}  {str(hex(len(data2)))[2:]}'
    )
    client._wait_for_shell_response(f"{data2}")

    logger.info('server send data3, client recv data3 and result is fail')
    data3 = "server_send_data_3"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data3}  {str(hex(len(data3)))[2:]}'
    )
    client._wait_for_shell_response(" to allocate buffer for SDU")
    server._wait_for_shell_response("RNR p=0 f=0 on chan")

    logger.info('set l2cap client idle status')
    client.iexpect(
        f'l2cap_br modify_appl_status {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} 0',
        f"psm {str(int(L2CAP_SERVER_PSM_ERET))} appl status 0",
    )
    client._wait_for_shell_response(f"{data3}", timeout=12)
    server._wait_for_shell_response(r'Retransmission I-frame', timeout=12)

    logger.info('server send data4, client recv data4 and result is successful')
    data4 = "server_send_data_4"
    server.exec_command(
        f'l2cap_br send {L2CAP_CHAN_IUT_ID_ERET} {data4}  {str(hex(len(data4)))[2:]}'
    )
    client._wait_for_shell_response(f"{data4}", timeout=12)

    server.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )
    server.iexpect('bt disconnect', r'Disconnected')
