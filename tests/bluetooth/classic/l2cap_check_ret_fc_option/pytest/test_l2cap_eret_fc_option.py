# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import re
import time

from conftest import (
    L2CAP_SERVER_PSM_ERET,
    logger,
)

MODE = "eret"


def l2cap_br_clear(client, server):
    client.exec_command("br clear all")
    server.exec_command("br clear all")


def test_l2cap_eret_mode_TC05(client, server):
    """
    verify RETRANSMISSION AND FLOW CONTROL OPTION from server and client side in eret mode.
    """
    L2CAP_CHAN_IUT_ID_ERET = 0

    l2cap_br_clear(client, server)
    time.sleep(1)
    logger.info(f'acl connect {server.addr}')
    client.iexpect(f'br connect {server.addr}', 'Connected', timeout=10)
    time.sleep(0.5)

    logger.info('set server  max_transmit = 7, set client max_transmit = 8')
    client.iexpect('l2cap_br modify_max_transmit 8', "MaxTransmit is 8")
    server.iexpect('l2cap_br modify_max_transmit 7', "MaxTransmit is 7")

    logger.info(
        f'client create l2cap. psm = {L2CAP_SERVER_PSM_ERET}, mode = eret, mode_option = false'
    )
    client.iexpect(
        f'l2cap_br connect {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} eret',
        'It is enhance retransmission mode',
    )

    logger.info('Check server configuration in server side')
    lines = server.exec_command(
        f'l2cap_br search_conf_param_options {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} local'
    )

    logger.info(f"{lines}")
    found = False
    for line in lines:
        if "mps" in line:
            found = True
            break

    if found is False:
        lines = server._wait_for_shell_response("mps")

    logger.info(f"{lines}")
    for line in lines:
        if "local" in line and "max_transmit" in line:
            param_list = line.strip().split(' ')[-1].split(',')
            for param in param_list:
                param_list = param.split('=')
                assert len(param_list) == 2
                name = param_list[0]
                value = int(re.sub(r'[^\d]', '', param_list[1]))
                if name == 'max_transmit':
                    assert value == 7
                elif name in ["ret_timeout", "monitor_timeout"]:
                    assert value == 2000
                elif name == "max_window":
                    assert value == 5
                elif name == 'mps':
                    assert value == 48
            break

    logger.info('Check client configuration in server side')
    lines = server.exec_command(
        f'l2cap_br search_conf_param_options {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} peer'
    )
    logger.info(f"{lines}")
    found = False
    for line in lines:
        if "mps" in line:
            found = True
            break

    if found is False:
        lines = server._wait_for_shell_response("mps")

    logger.info(f"{lines}")
    for line in lines:
        if "peer" in line and "max_transmit" in line:
            param_list = line.strip().split(' ')[-1].split(',')
            for param in param_list:
                param_list = param.split('=')
                assert len(param_list) == 2
                name = param_list[0]
                value = int(re.sub(r'[^\d]', '', param_list[1]))
                if name == 'max_transmit':
                    assert value == 8
                elif name in ["ret_timeout", "monitor_timeout"]:
                    assert value == 1000
                elif name == "max_window":
                    assert value == 5
                elif name == 'mps':
                    assert value == 48
            break

    logger.info('Check client configuration in client side')
    lines = client.exec_command(
        f'l2cap_br search_conf_param_options {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} local'
    )
    logger.info(f"{lines}")
    found = False
    for line in lines:
        if "mps" in line:
            found = True
            break

    if found is False:
        lines = client._wait_for_shell_response("mps")

    logger.info(f"{lines}")
    for line in lines:
        if "local" in line and "max_transmit" in line:
            param_list = line.strip().split(' ')[-1].split(',')
            for param in param_list:
                param_list = param.split('=')
                assert len(param_list) == 2
                name = param_list[0]
                value = int(re.sub(r'[^\d]', '', param_list[1]))
                if name == 'max_transmit':
                    assert value == 8
                elif name in ["ret_timeout", "monitor_timeout"]:
                    assert value == 1000
                elif name == "max_window":
                    assert value == 5
                elif name == 'mps':
                    assert value == 48
            break

    logger.info('Check server configuration in client side')
    lines = client.exec_command(
        f'l2cap_br search_conf_param_options {str(hex(L2CAP_SERVER_PSM_ERET))[2:]} peer'
    )
    logger.info(f"{lines}")
    found = False
    for line in lines:
        if "mps" in line:
            found = True
            break

    if found is False:
        lines = client._wait_for_shell_response("mps")

    logger.info(f"{lines}")
    for line in lines:
        if "peer" in line and "max_transmit" in line:
            param_list = line.strip().split(' ')[-1].split(',')
            for param in param_list:
                param_list = param.split('=')
                assert len(param_list) == 2
                name = param_list[0]
                value = int(re.sub(r'[^\d]', '', param_list[1]))
                if name == 'max_transmit':
                    assert value == 7
                elif name in ["ret_timeout", "monitor_timeout"]:
                    assert value == 2000
                elif name == "max_window":
                    assert value == 5
                elif name == 'mps':
                    assert value == 48
            break

    client.iexpect(
        f'l2cap_br disconnect {L2CAP_CHAN_IUT_ID_ERET}',
        f'Channel {L2CAP_CHAN_IUT_ID_ERET} disconnected',
    )

    client.iexpect('bt disconnect', r'Disconnected')
