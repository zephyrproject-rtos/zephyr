# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import logging
import re
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


def test_br_map_mce_multi_conn_operations(client, server_1, server_2):
    """
    BR_MAP_MCE_MULTI_CONN_OPERATIONS

    Verify that MCE can reliably handle message operations and
    event notifications across multiple MSE devices with multiple MAS instances.

    client: Device A (MCE)
    server_1: Device B (MSE)
    server_2: Device C (MSE)
    """
    br_pre(client)
    br_pre(server_1)
    br_pre(server_2)

    # Device A creates connection with Device B including ACL, L2CAP, MAS and MNS OBEX connections.
    mns_conn = False
    br_connect(client, server_1)
    br_security(client, server_1)
    for server_info in server_1.mas_server:
        # Create MAS OBEX connection
        mas_l2cap_connect(client, server_1, server_info['instance_id'])
        mce_mas_select(client, server_info['instance_id'])
        mse_mas_select(server_1, server_info['instance_id'])
        mas_connect(client, server_1, server_info['instance_id'], RspCode.SUCCESS)

        # Send notification registration
        mas_put_cmd(client, server_1, server_info['instance_id'], 'set_ntf_reg')

        # Create MNS OBEX connection
        if not mns_conn:
            mns_conn = True
            mns_l2cap_connect(server_1, client)
            mns_connect(server_1, client, RspCode.SUCCESS)

    # Device A creates connection with Device C including ACL, L2CAP, MAS and MNS OBEX connections.
    mns_conn = False
    br_connect(client, server_2)
    br_select(client, server_2.pub_addr)
    br_security(client, server_2)
    for server_info in server_2.mas_server:
        # Create MAS OBEX connection
        mas_l2cap_connect(client, server_2, server_info['instance_id'])
        mce_mas_select(client, server_info['instance_id'])
        mse_mas_select(server_2, server_info['instance_id'])
        mas_connect(client, server_2, server_info['instance_id'], RspCode.SUCCESS)

        # Send notification registration
        mas_put_cmd(client, server_2, server_info['instance_id'], 'set_ntf_reg')

        # Create MNS OBEX connection
        if not mns_conn:
            mns_conn = True
            mns_l2cap_connect(server_2, client)
            mns_connect(server_2, client, RspCode.SUCCESS)

    # Send GetMessagesListing
    br_select(client, server_1.pub_addr)
    for server_info in server_1.mas_server:
        mce_mas_select(client, server_info['instance_id'])
        mse_mas_select(server_1, server_info['instance_id'])
        mas_get_cmd_srm(client, server_1, server_info['instance_id'], 'get_msg_listing')

    br_select(client, server_2.pub_addr)
    for server_info in server_2.mas_server:
        mce_mas_select(client, server_info['instance_id'])
        mse_mas_select(server_2, server_info['instance_id'])
        mas_get_cmd_srm(client, server_2, server_info['instance_id'], 'get_msg_listing')

    # Send PushMessage
    br_select(client, server_1.pub_addr)
    for server_info in server_1.mas_server:
        mce_mas_select(client, server_info['instance_id'])
        mse_mas_select(server_1, server_info['instance_id'])
        mas_put_cmd_srm(client, server_1, server_info['instance_id'], 'push_msg')

    br_select(client, server_2.pub_addr)
    for server_info in server_2.mas_server:
        mce_mas_select(client, server_info['instance_id'])
        mse_mas_select(server_2, server_info['instance_id'])
        mas_put_cmd_srm(client, server_2, server_info['instance_id'], 'push_msg')

    # Send SendEvent
    br_select(client, server_1.pub_addr)
    mns_put_cmd_srm(server_1, client, 'send_event')

    br_select(client, server_2.pub_addr)
    mns_put_cmd_srm(server_2, client, 'send_event')

    # Cleanup
    mns_conn = True
    br_select(client, server_1.pub_addr)
    for server_info in server_1.mas_server:
        # Disconnect MNS OBEX connection
        if mns_conn:
            mns_conn = False
            mns_disconnect(server_1, client, RspCode.SUCCESS)
            mns_l2cap_disconnect(server_1, client)

        # Disconnect MAS OBEX connection
        mce_mas_select(client, server_info['instance_id'])
        mse_mas_select(server_1, server_info['instance_id'])
        mas_disconnect(client, server_1, server_info['instance_id'], RspCode.SUCCESS)
        mas_l2cap_disconnect(client, server_1, server_info['instance_id'])
    # Disconnect ACL connection
    br_disconnect(client, server_1)

    mns_conn = True
    br_select(client, server_2.pub_addr)
    for server_info in server_2.mas_server:
        # Disconnect MNS OBEX connection
        if mns_conn:
            mns_conn = False
            mns_disconnect(server_2, client, RspCode.SUCCESS)
            mns_l2cap_disconnect(server_2, client)

        # Disconnect MAS OBEX connection
        mce_mas_select(client, server_info['instance_id'])
        mse_mas_select(server_2, server_info['instance_id'])
        mas_disconnect(client, server_2, server_info['instance_id'], RspCode.SUCCESS)
        mas_l2cap_disconnect(client, server_2, server_info['instance_id'])
    # Disconnect ACL connection
    br_disconnect(client, server_2)


def test_br_map_mse_multi_conn_operations(client, server_1, server_2):
    """
    BR_MAP_MSE_MULTI_CONN_OPERATIONS

    Verify that MSE can reliably handle message operations and
    event notifications from multiple MCE devices with multiple MAS instances.

    client: Device A (MSE)
    server_1: Device B (MCE)
    server_2: Device C (MCE)
    """
    br_pre(client)
    br_pre(server_1)
    br_pre(server_2)

    # Device B creates connection with Device A including ACL, L2CAP, MAS and MNS OBEX connections.
    mns_conn = False
    br_connect(server_1, client)
    br_security(server_1, client)
    for server_info in client.mas_server:
        # Create MAS OBEX connection
        mas_l2cap_connect(server_1, client, server_info['instance_id'])
        mce_mas_select(server_1, server_info['instance_id'])
        mse_mas_select(client, server_info['instance_id'])
        mas_connect(server_1, client, server_info['instance_id'], RspCode.SUCCESS)

        # Send notification registration
        mas_put_cmd(server_1, client, server_info['instance_id'], 'set_ntf_reg')

        # Create MNS OBEX connection
        if not mns_conn:
            mns_conn = True
            mns_l2cap_connect(client, server_1)
            mns_connect(client, server_1, RspCode.SUCCESS)

    # Device C creates connection with Device A including ACL, L2CAP, MAS and MNS OBEX connections.
    mns_conn = False
    br_connect(server_2, client)
    br_select(client, server_2.pub_addr)
    br_security(server_2, client)
    for server_info in client.mas_server:
        # Create MAS OBEX connection
        mas_l2cap_connect(server_2, client, server_info['instance_id'])
        mce_mas_select(server_2, server_info['instance_id'])
        mse_mas_select(client, server_info['instance_id'])
        mas_connect(server_2, client, server_info['instance_id'], RspCode.SUCCESS)

        # Send notification registration
        mas_put_cmd(server_2, client, server_info['instance_id'], 'set_ntf_reg')

        # Create MNS OBEX connection
        if not mns_conn:
            mns_conn = True
            mns_l2cap_connect(client, server_2)
            mns_connect(client, server_2, RspCode.SUCCESS)

    # Send GetMessagesListing from Device B
    br_select(client, server_1.pub_addr)
    for server_info in client.mas_server:
        mce_mas_select(server_1, server_info['instance_id'])
        mse_mas_select(client, server_info['instance_id'])
        mas_get_cmd_srm(server_1, client, server_info['instance_id'], 'get_msg_listing')

    # Send GetMessagesListing from Device C
    br_select(client, server_2.pub_addr)
    for server_info in client.mas_server:
        mce_mas_select(server_2, server_info['instance_id'])
        mse_mas_select(client, server_info['instance_id'])
        mas_get_cmd_srm(server_2, client, server_info['instance_id'], 'get_msg_listing')

    # Send PushMessage from Device B
    br_select(client, server_1.pub_addr)
    for server_info in client.mas_server:
        mce_mas_select(server_1, server_info['instance_id'])
        mse_mas_select(client, server_info['instance_id'])
        mas_put_cmd_srm(server_1, client, server_info['instance_id'], 'push_msg')

    # Send PushMessage from Device C
    br_select(client, server_2.pub_addr)
    for server_info in client.mas_server:
        mce_mas_select(server_2, server_info['instance_id'])
        mse_mas_select(client, server_info['instance_id'])
        mas_put_cmd_srm(server_2, client, server_info['instance_id'], 'push_msg')

    # Send SendEvent from Device A to Device B
    br_select(client, server_1.pub_addr)
    mns_put_cmd_srm(client, server_1, 'send_event')

    # Send SendEvent from Device A to Device C
    br_select(client, server_2.pub_addr)
    mns_put_cmd_srm(client, server_2, 'send_event')

    # Cleanup - Disconnect Device B
    mns_conn = True
    br_select(client, server_1.pub_addr)
    for server_info in client.mas_server:
        # Disconnect MNS OBEX connection
        if mns_conn:
            mns_conn = False
            mns_disconnect(client, server_1, RspCode.SUCCESS)
            mns_l2cap_disconnect(client, server_1)

        # Disconnect MAS OBEX connection
        mce_mas_select(server_1, server_info['instance_id'])
        mse_mas_select(client, server_info['instance_id'])
        mas_disconnect(server_1, client, server_info['instance_id'], RspCode.SUCCESS)
        mas_l2cap_disconnect(server_1, client, server_info['instance_id'])
    # Disconnect ACL connection
    br_disconnect(server_1, client)

    # Cleanup - Disconnect Device C
    mns_conn = True
    br_select(client, server_2.pub_addr)
    for server_info in client.mas_server:
        # Disconnect MNS OBEX connection
        if mns_conn:
            mns_conn = False
            mns_disconnect(client, server_2, RspCode.SUCCESS)
            mns_l2cap_disconnect(client, server_2)

        # Disconnect MAS OBEX connection
        mce_mas_select(server_2, server_info['instance_id'])
        mse_mas_select(client, server_info['instance_id'])
        mas_disconnect(server_2, client, server_info['instance_id'], RspCode.SUCCESS)
        mas_l2cap_disconnect(server_2, client, server_info['instance_id'])
    # Disconnect ACL connection
    br_disconnect(server_2, client)
