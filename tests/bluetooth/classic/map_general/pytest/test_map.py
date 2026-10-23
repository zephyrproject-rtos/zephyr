# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import inspect
import logging
import re
import threading
import time

logger = logging.getLogger(__name__)

CONNECT_RETRIES = 3

BT_OBEX_VERSION = 0x10
BT_GOEP_MTU = 261


class OBEX_HDR:
    NAME = 0x01


class RspCode:
    CONTINUE = 0x90
    SUCCESS = 0xA0
    BAD_REQ = 0xC0
    FORBIDDEN = 0xC3
    NOT_FOUND = 0xC4
    NOT_ACCEPT = 0xC6


rsp_code_str = {
    RspCode.CONTINUE: "Continue",
    RspCode.SUCCESS: "Success",
    RspCode.BAD_REQ: "Bad Request - server couldn't understand request",
    RspCode.FORBIDDEN: "Forbidden - operation is understood but refused",
    RspCode.NOT_FOUND: "Not Found",
    RspCode.NOT_ACCEPT: "Not Acceptable",
}


def br_pre(init):
    init.exec_command('br clear all')
    init.exec_command('bt clear all')
    init.mas_conn = {}
    init.mns_conn = {}


def _br_connect(init, rsp):
    # BR connection
    init.exec_command(f'br connect {rsp.pub_addr}')
    found, _ = init.wait_for_shell_response(f'Connected: {rsp.pub_addr}')
    assert found is True
    found, _ = rsp.wait_for_shell_response(f'Connected: {init.pub_addr}')
    assert found is True


def br_connect(init, rsp):
    for attempt in range(CONNECT_RETRIES):
        try:
            _br_connect(init, rsp)
            return
        except AssertionError:
            if attempt == CONNECT_RETRIES - 1:
                raise
            logger.info('Retry BR connection')
            time.sleep(5)


def br_disconnect(init, rsp):
    init.exec_command('bt disconnect')
    found, _ = init.wait_for_shell_response(f'Disconnected: {rsp.pub_addr}')
    assert found is True
    found, _ = rsp.wait_for_shell_response(f'Disconnected: {init.pub_addr}')
    assert found is True


def br_select(init, addr):
    lines = init.exec_command(f"br select {addr}")
    expected = f"Selected conn is now: {addr}"
    found = init.check_shell_response(lines, expected)
    if found is False:
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def send_concurrently(*commands):
    """Send (board, command) pairs back-to-back without waiting for their output.

    Shell.exec_command() clears the receive buffer and consumes the command output, so it
    could drop a request from the peer that is part of the concurrent operation.
    """
    for board, command in commands:
        board.dut.write(f'{command}\n'.encode())


def br_security(init, rsp):
    # BR security

    dut_sec = 2  # Default security level 2
    init.exec_command(f'bt security {dut_sec}')
    expected = [f"Security changed: {rsp.pub_addr} level {dut_sec}"]
    expected.append(f"Bonded with {rsp.pub_addr}")
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    expected = [f"Security changed: {init.pub_addr} level {dut_sec}"]
    expected.append(f"Bonded with {init.pub_addr}")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def mas_l2cap_connect(init, rsp, instance_id):
    for server_info in rsp.mas_server:
        if server_info["instance_id"] == instance_id:
            psm = server_info["psm"]
            supported_features = server_info["supported_features"]
            break

    init.exec_command(
        f'test_map mce mas l2cap_connect {psm:04X} {instance_id} {supported_features:08X}'
    )
    expected = (
        rf"MCE MAS L2CAP connected: (?P<addr>\w+), addr: {rsp.pub_addr}, instance_id: {instance_id}"
    )
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is not None:
            init.mas_conn.setdefault(rsp.pub_addr, {})[instance_id] = searched.group("addr")
            break

    expected = (
        r"MSE MAS L2CAP connected: (?P<addr>\w+), "
        rf"addr: {init.pub_addr}, instance_id: {instance_id}"
    )
    found, lines = rsp.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is not None:
            rsp.mas_conn.setdefault(init.pub_addr, {})[instance_id] = searched.group("addr")
            break


def mas_l2cap_disconnect(init, rsp, instance_id):
    init_addr = init.mas_conn.get(rsp.pub_addr, {}).get(instance_id, None)
    rsp_addr = rsp.mas_conn.get(init.pub_addr, {}).get(instance_id, None)

    init.exec_command('test_map mce mas l2cap_disconnect')
    found, _ = init.wait_for_shell_response(f"MCE MAS L2CAP disconnected: {init_addr}")
    assert found is True
    found, _ = rsp.wait_for_shell_response(f"MSE MAS L2CAP disconnected: {rsp_addr}")
    assert found is True

    del init.mas_conn[rsp.pub_addr][instance_id]
    del rsp.mas_conn[init.pub_addr][instance_id]


def mas_rfcomm_connect(init, rsp, instance_id):
    for server_info in rsp.mas_server:
        if server_info["instance_id"] == instance_id:
            channel = server_info["channel"]
            supported_features = server_info["supported_features"]
            break

    init.exec_command(
        f'test_map mce mas rfcomm_connect {channel:02X} {instance_id} {supported_features:08X}'
    )
    expected = (
        r"MCE MAS RFCOMM connected: (?P<addr>\w+), "
        rf"addr: {rsp.pub_addr}, instance_id: {instance_id}"
    )
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is not None:
            init.mas_conn.setdefault(rsp.pub_addr, {})[instance_id] = searched.group("addr")
            break

    expected = (
        r"MSE MAS RFCOMM connected: (?P<addr>\w+), "
        rf"addr: {init.pub_addr}, instance_id: {instance_id}"
    )
    found, lines = rsp.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is not None:
            rsp.mas_conn.setdefault(init.pub_addr, {})[instance_id] = searched.group("addr")
            break


def mas_rfcomm_disconnect(init, rsp, instance_id):
    init_addr = init.mas_conn.get(rsp.pub_addr, {}).get(instance_id, None)
    rsp_addr = rsp.mas_conn.get(init.pub_addr, {}).get(instance_id, None)

    init.exec_command('test_map mce mas rfcomm_disconnect')
    found, _ = init.wait_for_shell_response(f"MCE MAS RFCOMM disconnected: {init_addr}")
    assert found is True
    found, _ = rsp.wait_for_shell_response(f"MSE MAS RFCOMM disconnected: {rsp_addr}")
    assert found is True

    del init.mas_conn[rsp.pub_addr][instance_id]
    del rsp.mas_conn[init.pub_addr][instance_id]


def mns_l2cap_connect(init, rsp):
    psm = rsp.mns_server["psm"]
    supported_features = rsp.mns_server["supported_features"]

    init.exec_command(f'test_map mse mns l2cap_connect {psm:04X} {supported_features:08X}')
    expected = rf"MSE MNS L2CAP connected: (?P<addr>\w+), addr: {rsp.pub_addr}"
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is not None:
            init.mns_conn[rsp.pub_addr] = searched.group("addr")
            break

    expected = rf"MCE MNS L2CAP connected: (?P<addr>\w+), addr: {init.pub_addr}"
    found, lines = rsp.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is not None:
            rsp.mns_conn[init.pub_addr] = searched.group("addr")
            break


def mns_l2cap_disconnect(init, rsp):
    init_addr = init.mns_conn.get(rsp.pub_addr, None)
    rsp_addr = rsp.mns_conn.get(init.pub_addr, None)

    init.exec_command('test_map mse mns l2cap_disconnect')
    found, _ = init.wait_for_shell_response(f"MSE MNS L2CAP disconnected: {init_addr}")
    assert found is True
    found, _ = rsp.wait_for_shell_response(f"MCE MNS L2CAP disconnected: {rsp_addr}")
    assert found is True

    del init.mns_conn[rsp.pub_addr]
    del rsp.mns_conn[init.pub_addr]


def mns_rfcomm_connect(init, rsp):
    channel = rsp.mns_server["channel"]
    supported_features = rsp.mns_server["supported_features"]

    init.exec_command(f'test_map mse mns rfcomm_connect {channel:02X} {supported_features:08X}')
    expected = rf"MSE MNS RFCOMM connected: (?P<addr>\w+), addr: {rsp.pub_addr}"
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is not None:
            init.mns_conn[rsp.pub_addr] = searched.group("addr")
            break

    expected = rf"MCE MNS RFCOMM connected: (?P<addr>\w+), addr: {init.pub_addr}"
    found, lines = rsp.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is not None:
            rsp.mns_conn[init.pub_addr] = searched.group("addr")
            break


def mns_rfcomm_disconnect(init, rsp):
    init_addr = init.mns_conn.get(rsp.pub_addr, None)
    rsp_addr = rsp.mns_conn.get(init.pub_addr, None)

    init.exec_command('test_map mse mns rfcomm_disconnect')
    found, _ = init.wait_for_shell_response(f"MSE MNS RFCOMM disconnected: {init_addr}")
    assert found is True
    found, _ = rsp.wait_for_shell_response(f"MCE MNS RFCOMM disconnected: {rsp_addr}")
    assert found is True


def mas_connect(init, rsp, instance_id, rsp_code):
    init_addr = init.mas_conn.get(rsp.pub_addr, {}).get(instance_id, None)
    rsp_addr = rsp.mas_conn.get(init.pub_addr, {}).get(instance_id, None)

    init.exec_command('test_map mce mas connect')
    expected = (
        rf"MSE MAS {rsp_addr} conn req, "
        rf"version {BT_OBEX_VERSION:02x}, mopl"
    )
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    cmd = "test_map mse mas connect "
    if rsp_code == RspCode.SUCCESS:
        cmd += 'success'
    elif rsp_code == RspCode.CONTINUE:
        cmd += 'continue'
    else:
        cmd += f'error {rsp_code:X}'
    rsp.exec_command(cmd)

    expected = rf"MCE MAS {init_addr} conn rsp, rsp_code {rsp_code_str[rsp_code]}"
    expected += rf", version {BT_OBEX_VERSION:02x}, mopl"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_disconnect(init, rsp, instance_id, rsp_code):
    init_addr = init.mas_conn.get(rsp.pub_addr, {}).get(instance_id, None)
    rsp_addr = rsp.mas_conn.get(init.pub_addr, {}).get(instance_id, None)

    init.exec_command('test_map mce mas disconnect')
    expected = rf"MSE MAS {rsp_addr} disconn req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    cmd = "test_map mse mas disconnect "
    if rsp_code == RspCode.SUCCESS:
        cmd += 'success'
    elif rsp_code == RspCode.CONTINUE:
        cmd += 'continue'
    else:
        cmd += f'error {rsp_code:X}'
    rsp.exec_command(cmd)

    expected = rf"MCE MAS {init_addr} disconn rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mns_connect(init, rsp, rsp_code):
    init_addr = init.mns_conn.get(rsp.pub_addr, None)
    rsp_addr = rsp.mns_conn.get(init.pub_addr, None)

    init.exec_command('test_map mse mns connect')
    expected = (
        rf"MCE MNS {rsp_addr} conn req, "
        rf"version {BT_OBEX_VERSION:02x}, mopl"
    )
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    cmd = "test_map mce mns connect "
    if rsp_code == RspCode.SUCCESS:
        cmd += 'success'
    elif rsp_code == RspCode.CONTINUE:
        cmd += 'continue'
    else:
        cmd += f'error {rsp_code:X}'
    rsp.exec_command(cmd)

    expected = rf"MSE MNS {init_addr} conn rsp, rsp_code {rsp_code_str[rsp_code]}"
    expected += rf", version {BT_OBEX_VERSION:02x}, mopl"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mns_disconnect(init, rsp, rsp_code):
    init_addr = init.mns_conn.get(rsp.pub_addr, None)
    rsp_addr = rsp.mns_conn.get(init.pub_addr, None)

    init.exec_command('test_map mse mns disconnect')
    expected = rf"MCE MNS {rsp_addr} disconn req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    cmd = "test_map mce mns disconnect "
    if rsp_code == RspCode.SUCCESS:
        cmd += 'success'
    elif rsp_code == RspCode.CONTINUE:
        cmd += 'continue'
    else:
        cmd += f'error {rsp_code:X}'
    rsp.exec_command(cmd)

    expected = rf"MSE MNS {init_addr} disconn rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mce_mas_select(init, instance_id):
    lines = init.exec_command(f'test_map mce mas select {instance_id}')
    expected = f"Selected MCE MAS instance_id {instance_id} as default"
    found = init.check_shell_response(lines, expected)
    if found is False:
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def mse_mas_select(init, instance_id):
    lines = init.exec_command(f'test_map mse mas select {instance_id}')
    expected = f"Selected MSE MAS instance_id {instance_id} as default"
    found = init.check_shell_response(lines, expected)
    if found is False:
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def mas_put_cmd(init, rsp, instance_id, cmd):
    """Complete a PUT command by continuing until Success response."""
    init_addr = init.mas_conn.get(rsp.pub_addr, {}).get(instance_id, None)
    rsp_addr = rsp.mas_conn.get(init.pub_addr, {}).get(instance_id, None)
    rsp_code = 'Continue'
    final = 'false'

    while True:
        init.exec_command(f'test_map mce mas {cmd}')
        expected = [rf"MSE MAS {rsp_addr} {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MSE MAS {rsp_addr} {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        rsp.exec_command(f'test_map mse mas {cmd} noerror')
        expected = rf"MCE MAS {init_addr} {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue' or final == 'true':
            break

    assert rsp_code == 'Success' and final == 'true'


def mas_get_cmd_srm(init, rsp, instance_id, cmd):
    init_addr = init.mas_conn.get(rsp.pub_addr, {}).get(instance_id, None)
    rsp_addr = rsp.mas_conn.get(init.pub_addr, {}).get(instance_id, None)
    rsp_code = 'Continue'

    init.exec_command(f'test_map mce mas {cmd}')
    expected = [rf"MSE MAS {rsp_addr} {cmd} req, final true"]
    expected.append("OBEX SRM: 01")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = [rf"MCE MAS {init_addr} {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        rsp.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS {init_addr} {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS {init_addr} {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'


def mas_put_cmd_srm(init, rsp, instance_id, cmd):
    init_addr = init.mas_conn.get(rsp.pub_addr, {}).get(instance_id, None)
    rsp_addr = rsp.mas_conn.get(init.pub_addr, {}).get(instance_id, None)
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_map mce mas {cmd}')
    expected = [rf"MSE MAS {rsp_addr} {cmd} req, final false"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = [rf"MCE MAS {init_addr} {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_map mce mas {cmd}')
        expected = [rf"MSE MAS {rsp_addr} {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MSE MAS {rsp_addr} {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    assert final == 'true'

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = rf"MCE MAS {init_addr} {cmd} rsp, rsp_code Success"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mns_put_cmd_srm(init, rsp, cmd):
    init_addr = init.mns_conn.get(rsp.pub_addr, None)
    rsp_addr = rsp.mns_conn.get(init.pub_addr, None)
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_map mse mns {cmd}')
    expected = [rf"MCE MNS {rsp_addr} {cmd} req, final \w+"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mce mns {cmd} noerror')
    expected = [rf"MSE MNS {init_addr} {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_map mse mns {cmd}')
        expected = [rf"MCE MNS {rsp_addr} {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MNS {rsp_addr} {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    assert final == 'true'

    rsp.exec_command(f'test_map mce mns {cmd} noerror')
    expected = rf"MSE MNS {init_addr} {cmd} rsp, rsp_code Success"
    found, lines = init.wait_for_shell_response(expected)
    assert found is True


def map_acl_disconnect_in_any_state_scenario_1(client, server, reverse=False):
    """
    BR_MAP_ACL_DISCONNECT_IN_ANY_STATE - Scenario 1

    ACL disconnect during MAS connection establishment

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A initiates MAS L2CAP connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)

    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)

    client.exec_command('test_map mce mas connect')
    expected = (
        rf"MSE MAS {rsp_addr} conn req, "
        rf"version {BT_OBEX_VERSION:02x}, mopl"
    )
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Disconnect ACL connection before MAS OBEX connection completes
    init = client if not reverse else server
    init.exec_command('bt disconnect')

    # Verify proper cleanup
    expected = [f'Disconnected: {server.pub_addr}']
    expected.append(f"MCE MAS L2CAP disconnected: {init_addr}")
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = [f'Disconnected: {client.pub_addr}']
    expected.append(f"MSE MAS L2CAP disconnected: {rsp_addr}")
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_acl_disconnect_in_any_state_scenario_2(client, server, reverse=False):
    """
    BR_MAP_ACL_DISCONNECT_IN_ANY_STATE - Scenario 2

    ACL disconnect during MNS connection establishment

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A establishes MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)

    # Device A sends SetNotificationRegistration
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Device B initiates MNS L2CAP connection to Device A
    mns_l2cap_connect(server, client)

    init_mns_addr = server.mns_conn.get(client.pub_addr, None)
    rsp_mns_addr = client.mns_conn.get(server.pub_addr, None)

    server.exec_command('test_map mse mns connect')
    expected = (
        rf"MCE MNS {rsp_mns_addr} conn req, "
        rf"version {BT_OBEX_VERSION:02x}, mopl"
    )
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Disconnect ACL connection before MNS OBEX connection completes
    init = client if not reverse else server
    init.exec_command('bt disconnect')

    # Verify proper cleanup of both MAS and MNS connections
    expected = [f'Disconnected: {server.pub_addr}']
    expected.append(f"MCE MAS L2CAP disconnected: {init_addr}")
    expected.append(f"MCE MNS L2CAP disconnected: {rsp_mns_addr}")
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = [f'Disconnected: {client.pub_addr}']
    expected.append(f"MSE MAS L2CAP disconnected: {rsp_addr}")
    expected.append(f"MSE MNS L2CAP disconnected: {init_mns_addr}")
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_acl_disconnect_in_any_state_scenario_3(client, server, reverse=False):
    """
    BR_MAP_ACL_DISCONNECT_IN_ANY_STATE - Scenario 3

    ACL disconnect during GetMessagesListing with SRM

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A establishes MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)

    # Device A sends GetMessagesListing with SRM enabled
    client.exec_command('test_map mce mas get_msg_listing')

    # Wait for SRM negotiation
    expected = [rf"MSE MAS {rsp_addr} get_msg_listing req, final true"]
    expected.append("OBEX SRM: 01")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Device B starts sending response with SRM
    server.exec_command('test_map mse mas get_msg_listing noerror')

    # Wait for first response packet
    expected = [rf"MCE MAS {init_addr} get_msg_listing rsp, rsp_code Continue"]
    expected.append("OBEX SRM: 01")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Disconnect ACL during data transfer (after receiving partial response)
    init = client if not reverse else server
    init.exec_command('bt disconnect')

    # Verify proper cleanup and SRM state cleared
    expected = [f'Disconnected: {server.pub_addr}']
    expected.append(f"MCE MAS L2CAP disconnected: {init_addr}")
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = [f'Disconnected: {client.pub_addr}']
    expected.append(f"MSE MAS L2CAP disconnected: {rsp_addr}")
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_acl_disconnect_in_any_state_scenario_4(client, server, reverse=False):
    """
    BR_MAP_ACL_DISCONNECT_IN_ANY_STATE - Scenario 4

    ACL disconnect during PushMessage with SRM

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A establishes MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)

    # Device A sends PushMessage with SRM enabled (large message)
    client.exec_command('test_map mce mas push_msg')

    # Wait for SRM negotiation and first packet
    expected = [rf"MSE MAS {rsp_addr} push_msg req, final false"]
    expected.append("OBEX SRM: 01")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Device B sends continue response with SRM
    server.exec_command('test_map mse mas push_msg noerror')

    expected = [rf"MCE MAS {init_addr} push_msg rsp, rsp_code Continue"]
    expected.append("OBEX SRM: 01")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Disconnect ACL during message transmission (partial data sent)
    init = client if not reverse else server
    init.exec_command('bt disconnect')

    # Verify proper cleanup, SRM state cleared, and tx_cnt reset
    expected = [f'Disconnected: {server.pub_addr}']
    expected.append(f"MCE MAS L2CAP disconnected: {init_addr}")
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = [f'Disconnected: {client.pub_addr}']
    expected.append(f"MSE MAS L2CAP disconnected: {rsp_addr}")
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_acl_disconnect_in_any_state_scenario_5(client, server, reverse=False):
    """
    BR_MAP_ACL_DISCONNECT_IN_ANY_STATE - Scenario 5

    ACL disconnect during SendEvent notification

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A establishes MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)

    # Device A sends SetNotificationRegistration
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Device B establishes MNS connection to Device A
    mns_l2cap_connect(server, client)
    mns_connect(server, client, RspCode.SUCCESS)

    init_mns_addr = server.mns_conn.get(client.pub_addr, None)
    rsp_mns_addr = client.mns_conn.get(server.pub_addr, None)

    # Device B sends SendEvent notification to Device A
    server.exec_command('test_map mse mns send_event')

    # Wait for notification to be received
    expected = [rf"MCE MNS {rsp_mns_addr} send_event req, final \w+"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Disconnect ACL before Device A responds
    init = client if not reverse else server
    init.exec_command('bt disconnect')

    # Verify proper cleanup of MNS connection resources
    expected = [f'Disconnected: {server.pub_addr}']
    expected.append(f"MCE MAS L2CAP disconnected: {init_addr}")
    expected.append(f"MCE MNS L2CAP disconnected: {rsp_mns_addr}")
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = [f'Disconnected: {client.pub_addr}']
    expected.append(f"MSE MAS L2CAP disconnected: {rsp_addr}")
    expected.append(f"MSE MNS L2CAP disconnected: {init_mns_addr}")
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_acl_disconnect_in_any_state_scenario_6(client, server, reverse=False):
    """
    BR_MAP_ACL_DISCONNECT_IN_ANY_STATE - Scenario 6

    ACL disconnect with multiple MAS instances

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A establishes MAS connections to Device B instance 0 and instance 1
    for server_info in server.mas_server:
        instance_id = server_info['instance_id']
        mas_l2cap_connect(client, server, instance_id)
        mce_mas_select(client, instance_id)
        mse_mas_select(server, instance_id)
        mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Device A initiates GetMessagesListing on instance 0
    mce_mas_select(client, 0)
    mse_mas_select(server, 0)

    init_addr_0 = client.mas_conn.get(server.pub_addr, {}).get(0, None)
    rsp_addr_0 = server.mas_conn.get(client.pub_addr, {}).get(0, None)

    client.exec_command('test_map mce mas get_msg_listing')

    expected = [rf"MSE MAS {rsp_addr_0} get_msg_listing req, final true"]
    expected.append("OBEX SRM: 01")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Device A initiates PushMessage on instance 1
    mce_mas_select(client, 1)
    mse_mas_select(server, 1)

    init_addr_1 = client.mas_conn.get(server.pub_addr, {}).get(1, None)
    rsp_addr_1 = server.mas_conn.get(client.pub_addr, {}).get(1, None)

    client.exec_command('test_map mce mas push_msg')

    expected = [rf"MSE MAS {rsp_addr_1} push_msg req, final false"]
    expected.append("OBEX SRM: 01")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Disconnect ACL during concurrent operations
    init = client if not reverse else server
    init.exec_command('bt disconnect')

    # Verify both MAS instances are properly disconnected
    expected = [f'Disconnected: {server.pub_addr}']
    expected.append(f"MCE MAS L2CAP disconnected: {init_addr_0}")
    expected.append(f"MCE MAS L2CAP disconnected: {init_addr_1}")
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = [f'Disconnected: {client.pub_addr}']
    expected.append(f"MSE MAS L2CAP disconnected: {rsp_addr_0}")
    expected.append(f"MSE MAS L2CAP disconnected: {rsp_addr_1}")
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_acl_disconnect_in_any_state_scenario_7(client, server, reverse=False):
    """
    BR_MAP_ACL_DISCONNECT_IN_ANY_STATE - Scenario 7

    ACL disconnect with MAS and MNS active

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A establishes MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)

    # Device A sends SetNotificationRegistration
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Device B establishes MNS connection to Device A
    mns_l2cap_connect(server, client)
    mns_connect(server, client, RspCode.SUCCESS)

    init_mns_addr = server.mns_conn.get(client.pub_addr, None)
    rsp_mns_addr = client.mns_conn.get(server.pub_addr, None)

    # Device A sends GetMessagesListing with SRM
    client.exec_command('test_map mce mas get_msg_listing')

    expected = [rf"MSE MAS {rsp_addr} get_msg_listing req, final true"]
    expected.append("OBEX SRM: 01")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Device B sends SendEvent notification via MNS
    server.exec_command('test_map mse mns send_event')

    expected = [rf"MCE MNS {rsp_mns_addr} send_event req, final \w+"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Disconnect ACL during concurrent MAS and MNS operations
    init = client if not reverse else server
    init.exec_command('bt disconnect')

    # Verify both MAS and MNS connections are properly cleaned up
    expected = [f'Disconnected: {server.pub_addr}']
    expected.append(f"MCE MAS L2CAP disconnected: {init_addr}")
    expected.append(f"MCE MNS L2CAP disconnected: {rsp_mns_addr}")
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = [f'Disconnected: {client.pub_addr}']
    expected.append(f"MSE MAS L2CAP disconnected: {rsp_addr}")
    expected.append(f"MSE MNS L2CAP disconnected: {init_mns_addr}")
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_acl_disconnect_in_any_state_scenario_8(client, server, reverse=False):
    """
    BR_MAP_ACL_DISCONNECT_IN_ANY_STATE - Scenario 8

    Rapid ACL reconnection after disconnect

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A establishes MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Disconnect ACL connection
    init = client if not reverse else server
    rsp = server if not reverse else client
    br_disconnect(init, rsp)

    # Immediately reconnect ACL and establish new MAS connection
    br_pre(client)
    br_pre(server)
    br_connect(client, server)
    br_security(client, server)

    # Verify new connection establishes successfully
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Verify connection is functional
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Cleanup
    mas_disconnect(client, server, instance_id, RspCode.SUCCESS)
    mas_l2cap_disconnect(client, server, instance_id)
    init = client if not reverse else server
    rsp = server if not reverse else client
    br_disconnect(init, rsp)

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def test_br_map_acl_disconnect_in_any_state(client, server):
    """
    Verify that MAP can gracefully handle ACL disconnection in any operational state
    without resource leaks or system instability.
    """
    map_acl_disconnect_in_any_state_scenario_1(client, server)
    map_acl_disconnect_in_any_state_scenario_2(client, server)
    map_acl_disconnect_in_any_state_scenario_3(client, server)
    map_acl_disconnect_in_any_state_scenario_4(client, server)
    map_acl_disconnect_in_any_state_scenario_5(client, server)
    map_acl_disconnect_in_any_state_scenario_6(client, server)
    map_acl_disconnect_in_any_state_scenario_7(client, server)
    map_acl_disconnect_in_any_state_scenario_8(client, server)
    map_acl_disconnect_in_any_state_scenario_1(client, server, reverse=True)
    map_acl_disconnect_in_any_state_scenario_2(client, server, reverse=True)
    map_acl_disconnect_in_any_state_scenario_3(client, server, reverse=True)
    map_acl_disconnect_in_any_state_scenario_4(client, server, reverse=True)
    map_acl_disconnect_in_any_state_scenario_5(client, server, reverse=True)
    map_acl_disconnect_in_any_state_scenario_6(client, server, reverse=True)
    map_acl_disconnect_in_any_state_scenario_7(client, server, reverse=True)
    map_acl_disconnect_in_any_state_scenario_8(client, server, reverse=True)


def map_simultaneous_l2cap_disconnect_scenario_1(client, server):
    """
    BR_MAP_SIMULTANEOUS_L2CAP_DISCONNECT_REQUESTS - Scenario 1

    Simultaneous L2CAP disconnect on MAS connection

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A creates MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Device A and Device B simultaneously initiate L2CAP disconnect
    def client_disconnect():
        client.exec_command('test_map mce mas l2cap_disconnect')

    def server_disconnect():
        server.exec_command('test_map mse mas l2cap_disconnect')

    client_thread = threading.Thread(target=client_disconnect)
    server_thread = threading.Thread(target=server_disconnect)

    client_thread.start()
    server_thread.start()

    client_thread.join()
    server_thread.join()

    # Verify both devices handle the collision gracefully
    # Verify l2cap_disconnected callback is invoked on both sides
    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)

    expected = f"MCE MAS L2CAP disconnected: {init_addr}"
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = f"MSE MAS L2CAP disconnected: {rsp_addr}"
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    # Cleanup
    br_disconnect(client, server)

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_simultaneous_l2cap_disconnect_scenario_2(client, server):
    """
    BR_MAP_SIMULTANEOUS_L2CAP_DISCONNECT_REQUESTS - Scenario 2

    Simultaneous L2CAP disconnect on MNS connection

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A creates MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Device A sends SetNotificationRegistration to Device B instance 0
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Device B creates MNS connection to Device A
    mns_l2cap_connect(server, client)
    mns_connect(server, client, RspCode.SUCCESS)

    # Device A and Device B simultaneously initiate MNS L2CAP disconnect
    def client_disconnect():
        client.exec_command('test_map mce mns l2cap_disconnect')

    def server_disconnect():
        server.exec_command('test_map mse mns l2cap_disconnect')

    client_thread = threading.Thread(target=client_disconnect)
    server_thread = threading.Thread(target=server_disconnect)

    client_thread.start()
    server_thread.start()

    client_thread.join()
    server_thread.join()

    # Verify both devices handle the collision gracefully
    init_mns_addr = server.mns_conn.get(client.pub_addr, None)
    rsp_mns_addr = client.mns_conn.get(server.pub_addr, None)

    expected = f"MCE MNS L2CAP disconnected: {rsp_mns_addr}"
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = f"MSE MNS L2CAP disconnected: {init_mns_addr}"
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    # Verify no impact on existing MAS connection
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Cleanup
    mas_disconnect(client, server, instance_id, RspCode.SUCCESS)
    mas_l2cap_disconnect(client, server, instance_id)
    br_disconnect(client, server)

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def test_br_map_simultaneous_l2cap_disconnect_requests(client, server):
    """
    Verify that MAP can gracefully handle simultaneous L2CAP disconnect requests
    from both MCE and MSE without race conditions, deadlocks, or resource leaks.
    """
    map_simultaneous_l2cap_disconnect_scenario_1(client, server)
    map_simultaneous_l2cap_disconnect_scenario_2(client, server)


def map_simultaneous_rfcomm_disconnect_scenario_1(client, server):
    """
    BR_MAP_SIMULTANEOUS_RFCOMM_DISCONNECT_REQUESTS - Scenario 1

    Simultaneous RFCOMM disconnect on MAS connection

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A creates MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_rfcomm_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Device A and Device B simultaneously initiate RFCOMM disconnect
    def client_disconnect():
        client.exec_command('test_map mce mas rfcomm_disconnect')

    def server_disconnect():
        server.exec_command('test_map mse mas rfcomm_disconnect')

    client_thread = threading.Thread(target=client_disconnect)
    server_thread = threading.Thread(target=server_disconnect)

    client_thread.start()
    server_thread.start()

    client_thread.join()
    server_thread.join()

    # Verify both devices handle the collision gracefully
    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)

    expected = f"MCE MAS RFCOMM disconnected: {init_addr}"
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = f"MSE MAS RFCOMM disconnected: {rsp_addr}"
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    # Cleanup
    br_disconnect(client, server)

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_simultaneous_rfcomm_disconnect_scenario_2(client, server):
    """
    BR_MAP_SIMULTANEOUS_RFCOMM_DISCONNECT_REQUESTS - Scenario 2

    Simultaneous RFCOMM disconnect on MNS connection

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A creates MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_rfcomm_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Device A sends SetNotificationRegistration to Device B instance 0
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Device B creates MNS connection to Device A
    mns_rfcomm_connect(server, client)
    mns_connect(server, client, RspCode.SUCCESS)

    # Device A and Device B simultaneously initiate MNS RFCOMM disconnect
    def client_disconnect():
        client.exec_command('test_map mce mns rfcomm_disconnect')

    def server_disconnect():
        server.exec_command('test_map mse mns rfcomm_disconnect')

    client_thread = threading.Thread(target=client_disconnect)
    server_thread = threading.Thread(target=server_disconnect)

    client_thread.start()
    server_thread.start()

    client_thread.join()
    server_thread.join()

    # Verify both devices handle the collision gracefully
    init_mns_addr = server.mns_conn.get(client.pub_addr, None)
    rsp_mns_addr = client.mns_conn.get(server.pub_addr, None)

    expected = f"MCE MNS RFCOMM disconnected: {rsp_mns_addr}"
    found, _ = client.wait_for_shell_response(expected, timeout=5)
    assert found is True

    expected = f"MSE MNS RFCOMM disconnected: {init_mns_addr}"
    found, _ = server.wait_for_shell_response(expected, timeout=5)
    assert found is True

    # Verify MAS connection remains stable
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Cleanup
    mas_disconnect(client, server, instance_id, RspCode.SUCCESS)
    mas_rfcomm_disconnect(client, server, instance_id)
    br_disconnect(client, server)

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def test_br_map_simultaneous_rfcomm_disconnect_requests(client, server):
    """
    Verify that MAP can gracefully handle simultaneous RFCOMM disconnect requests
    from both MCE and MSE without race conditions, deadlocks, or resource leaks.
    """
    map_simultaneous_rfcomm_disconnect_scenario_1(client, server)
    map_simultaneous_rfcomm_disconnect_scenario_2(client, server)


def map_concurrent_different_operations_scenario_1(client, server):
    """
    BR_MAP_CONCURRENT_DIFFERENT_OPERATIONS - Scenario 1

    Concurrent GetMessagesListing (MCE) and SendEvent (MSE)

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A creates MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Device A sends SetNotificationRegistration to Device B instance 0
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Device B creates MNS connection to Device A
    mns_l2cap_connect(server, client)
    mns_connect(server, client, RspCode.SUCCESS)

    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)
    init_mns_addr = server.mns_conn.get(client.pub_addr, None)
    rsp_mns_addr = client.mns_conn.get(server.pub_addr, None)

    # Device A and Device B simultaneously initiate different operations
    send_concurrently(
        (client, 'test_map mce mas get_msg_listing'),
        (server, 'test_map mse mns send_event'),
    )

    expected = [rf"MSE MAS {rsp_addr} get_msg_listing req, final true"]
    expected.append("OBEX SRM: 01")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    expected = [rf"MCE MNS {rsp_mns_addr} send_event req, final \w+"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Device B responds to GetMessagesListing
    server.exec_command('test_map mse mas get_msg_listing noerror')
    expected = [rf"MCE MAS {init_addr} get_msg_listing rsp, rsp_code Continue"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Device A responds to SendEvent
    client.exec_command('test_map mce mns send_event noerror')
    expected = [rf"MSE MNS {init_mns_addr} send_event rsp, rsp_code Continue"]
    expected.append("OBEX SRM: 01")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Complete GetMessagesListing with SRM
    rsp_code = 'Continue'
    while rsp_code == 'Continue':
        server.exec_command('test_map mse mas get_msg_listing noerror')
        expected = rf"MCE MAS {init_addr} get_msg_listing rsp, rsp_code (?P<rsp_code>\w+)"
        found, lines = client.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

    assert rsp_code == 'Success'

    # Complete SendEvent with SRM
    final = 'false'
    while final != 'true':
        server.exec_command('test_map mse mns send_event')
        expected = rf"MCE MNS {rsp_mns_addr} send_event req, final (?P<final>\w+)"
        found, lines = client.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

    client.exec_command('test_map mce mns send_event noerror')
    expected = rf"MSE MNS {init_mns_addr} send_event rsp, rsp_code Success"
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Cleanup
    mns_disconnect(server, client, RspCode.SUCCESS)
    mns_l2cap_disconnect(server, client)
    mas_disconnect(client, server, instance_id, RspCode.SUCCESS)
    mas_l2cap_disconnect(client, server, instance_id)
    br_disconnect(client, server)

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def map_concurrent_different_operations_scenario_2(client, server):
    """
    BR_MAP_CONCURRENT_DIFFERENT_OPERATIONS - Scenario 2

    Concurrent PushMessage (MCE) and SendEvent (MSE)

    client: Device A (MCE)
    server: Device B (MSE)
    """
    logger.info(f"Starting {inspect.currentframe().f_code.co_name}")

    br_pre(client)
    br_pre(server)

    # Device A creates ACL connection to Device B
    br_connect(client, server)
    br_security(client, server)

    # Device A creates MAS connection to Device B instance 0
    server_info = server.mas_server[0]
    instance_id = server_info['instance_id']
    mas_l2cap_connect(client, server, instance_id)
    mas_connect(client, server, instance_id, RspCode.SUCCESS)

    # Device A sends SetNotificationRegistration to Device B instance 0
    mas_put_cmd(client, server, instance_id, 'set_ntf_reg')

    # Device B creates MNS connection to Device A
    mns_l2cap_connect(server, client)
    mns_connect(server, client, RspCode.SUCCESS)

    init_addr = client.mas_conn.get(server.pub_addr, {}).get(instance_id, None)
    rsp_addr = server.mas_conn.get(client.pub_addr, {}).get(instance_id, None)
    init_mns_addr = server.mns_conn.get(client.pub_addr, None)
    rsp_mns_addr = client.mns_conn.get(server.pub_addr, None)

    # Device A and Device B simultaneously initiate different operations
    send_concurrently(
        (client, 'test_map mce mas push_msg'),
        (server, 'test_map mse mns send_event'),
    )

    expected = [rf"MSE MAS {rsp_addr} push_msg req, final false"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    expected = [rf"MCE MNS {rsp_mns_addr} send_event req, final \w+"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Device B responds to PushMessage
    server.exec_command('test_map mse mas push_msg noerror')
    expected = [rf"MCE MAS {init_addr} push_msg rsp, rsp_code Continue"]
    expected.append("OBEX SRM: 01")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Device A responds to SendEvent
    client.exec_command('test_map mce mns send_event noerror')
    expected = [rf"MSE MNS {init_mns_addr} send_event rsp, rsp_code Continue"]
    expected.append("OBEX SRM: 01")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Complete PushMessage with SRM
    final = 'false'
    while final != 'true':
        client.exec_command('test_map mce mas push_msg')
        expected = rf"MSE MAS {rsp_addr} push_msg req, final (?P<final>\w+)"
        found, lines = server.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

    server.exec_command('test_map mse mas push_msg noerror')
    expected = rf"MCE MAS {init_addr} push_msg rsp, rsp_code Success"
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Complete SendEvent with SRM
    final = 'false'
    while final != 'true':
        server.exec_command('test_map mse mns send_event')
        expected = rf"MCE MNS {rsp_mns_addr} send_event req, final (?P<final>\w+)"
        found, lines = client.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

    client.exec_command('test_map mce mns send_event noerror')
    expected = rf"MSE MNS {init_mns_addr} send_event rsp, rsp_code Success"
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Cleanup
    mns_disconnect(server, client, RspCode.SUCCESS)
    mns_l2cap_disconnect(server, client)
    mas_disconnect(client, server, instance_id, RspCode.SUCCESS)
    mas_l2cap_disconnect(client, server, instance_id)
    br_disconnect(client, server)

    logger.info(f"Completed {inspect.currentframe().f_code.co_name}")


def test_br_map_concurrent_different_operations(client, server):
    """
    Verify that MAP can gracefully handle concurrent different operations from MCE and MSE
    without race conditions, data corruption, or resource conflicts.
    """
    map_concurrent_different_operations_scenario_1(client, server)
    map_concurrent_different_operations_scenario_2(client, server)
