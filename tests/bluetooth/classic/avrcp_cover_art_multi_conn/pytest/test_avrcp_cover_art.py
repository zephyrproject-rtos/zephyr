# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import logging
import re
import time

logger = logging.getLogger(__name__)

CONNECT_RETRIES = 3

BT_OBEX_VERSION = 0x10
BT_GOEP_MTU = 255


class RspCode:
    CONTINUE = 0x90
    SUCCESS = 0xA0


rsp_code_str = {
    RspCode.CONTINUE: "Continue",
    RspCode.SUCCESS: "Success",
}


def br_pre(init):
    init.exec_command('br clear all')
    init.exec_command('bt clear all')


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


def sdp_discover(init):
    psm = None
    expected_l2cap_uuid = "ATTR UUID 0100 found"
    expected_obex_psm = r"ATTR value \(16-bit\): (?P<psm>\d+)"
    expected_obex_uuid = "ATTR UUID 0008 found"

    init.exec_command('br sdp-find')
    found, lines = init.wait_for_shell_response(expected_obex_uuid)
    assert found is True

    for i in range(2, len(lines)):
        searched = re.search(expected_obex_uuid, lines[i])
        if searched is None:
            continue
        searched = re.search(expected_l2cap_uuid, lines[i - 2])
        if searched is None:
            continue
        searched = re.search(expected_obex_psm, lines[i - 1])
        if searched is None:
            continue
        psm = int(searched.group("psm"))
        break

    assert psm is not None
    return psm


def avrcp_connect(init, rsp):
    init.exec_command('test_avrcp connect')
    found, _ = init.wait_for_shell_response("AVRCP CT connected")
    assert found is True
    found, _ = rsp.wait_for_shell_response("AVRCP TG connected")
    assert found is True


def ca_ct_l2cap_connect(init, rsp, psm):
    init.exec_command(f'test_avrcp_cover_art ct l2cap_connect {psm}')
    found, _ = init.wait_for_shell_response("Cover Art CT L2CAP Connected")
    assert found is True
    found, _ = rsp.wait_for_shell_response("Cover Art TG L2CAP Connected")
    assert found is True


def ca_ct_l2cap_connect_fail(init, psm):
    lines = init.exec_command(f'test_avrcp_cover_art ct l2cap_connect {psm}')
    expected = "L2CAP connect failed"
    found = init.check_shell_response(lines, expected)
    if found is False:
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def ca_ct_l2cap_disconnect(init, rsp):
    init.exec_command('test_avrcp_cover_art ct l2cap_disconnect')
    found, _ = init.wait_for_shell_response("Cover Art CT L2CAP Disconnected")
    assert found is True
    found, _ = rsp.wait_for_shell_response("Cover Art TG L2CAP Disconnected")
    assert found is True


def ca_tg_l2cap_disconnect(init, rsp):
    init.exec_command('test_avrcp_cover_art tg l2cap_disconnect')
    found, _ = init.wait_for_shell_response("Cover Art TG L2CAP Disconnected")
    assert found is True
    found, _ = rsp.wait_for_shell_response("Cover Art CT L2CAP Disconnected")
    assert found is True


def ca_connect(init, rsp, rsp_code):
    init.exec_command('test_avrcp_cover_art ct connect')
    expected = (
        rf"Cover Art TG (?P<addr>\w+) OBEX connect req: "
        rf"version {BT_OBEX_VERSION:02x}, mopl {BT_GOEP_MTU:04x}"
    )
    found, lines = rsp.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is None:
            continue

        rsp.avrcp_info[init.pub_addr] = searched.group("addr")
        break

    cmd = "test_avrcp_cover_art tg connect "
    if rsp_code == RspCode.SUCCESS:
        cmd += 'success'
    elif rsp_code == RspCode.CONTINUE:
        cmd += 'continue'
    else:
        cmd += f'error {rsp_code:X}'
    rsp.exec_command(cmd)

    expected = rf"Cover Art CT (?P<addr>\w+) conn rsp, rsp_code {rsp_code_str[rsp_code]}"
    expected += rf", version {BT_OBEX_VERSION:02x}, mopl {BT_GOEP_MTU:04x}"
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    for line in lines:
        searched = re.search(expected, line)
        if searched is None:
            continue

        init.avrcp_info[rsp.pub_addr] = searched.group("addr")
        break


def ca_cmd_initiate(init, rsp, cmd):
    """Initiate a Cover Art command and wait for initial Continue response."""
    init.exec_command(f'test_avrcp_cover_art ct {cmd}')
    expected = f"Cover Art TG {rsp.avrcp_info[init.pub_addr]} {cmd} req, final true"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror')
    expected = f"Cover Art CT {init.avrcp_info[rsp.pub_addr]} {cmd} rsp, rsp_code Continue"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ca_cmd_complete(init, rsp, cmd):
    """Complete a Cover Art command by continuing until Success response."""
    rsp_code = 'Continue'

    while True:
        init.exec_command(f'test_avrcp_cover_art ct {cmd}')
        expected = f"Cover Art TG {rsp.avrcp_info[init.pub_addr]} {cmd} req, final true"
        found, _ = rsp.wait_for_shell_response(expected)
        assert found is True

        rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror')
        expected = (
            rf"Cover Art CT {init.avrcp_info[rsp.pub_addr]} {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        )
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is None:
                continue

            rsp_code = searched.group("rsp_code")
            if rsp_code != 'Continue':
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'


def test_br_ca_server_concurrent_operations_001(client, server_1, server_2):
    """Verify server handles concurrent operations from multiple clients."""
    br_pre(client)
    br_pre(server_1)
    br_pre(server_2)

    # Client 1 create connection
    br_connect(server_1, client)
    br_select(client, server_1.pub_addr)
    br_security(server_1, client)
    avrcp_connect(server_1, client)
    psm = sdp_discover(server_1)
    ca_ct_l2cap_connect(server_1, client, psm)
    ca_connect(server_1, client, RspCode.SUCCESS)

    # Client 2 create connection
    br_connect(server_2, client)
    br_select(client, server_2.pub_addr)
    br_security(server_2, client)
    avrcp_connect(server_2, client)
    psm = sdp_discover(server_2)
    ca_ct_l2cap_connect(server_2, client, psm)
    ca_connect(server_2, client, RspCode.SUCCESS)

    # Initiate concurrent operations
    br_select(client, server_1.pub_addr)
    ca_cmd_initiate(server_1, client, "get_image_properties")
    br_select(client, server_2.pub_addr)
    ca_cmd_initiate(server_2, client, "get_image_properties")
    br_select(client, server_1.pub_addr)
    ca_cmd_complete(server_1, client, "get_image_properties")
    br_select(client, server_2.pub_addr)
    ca_cmd_complete(server_2, client, "get_image_properties")

    br_disconnect(server_1, client)
    br_disconnect(server_2, client)


def test_br_ca_client_multiple_request_001(client, server_1, server_2):
    """Verify client handles multiple simultaneous requests."""
    br_pre(client)
    br_pre(server_1)
    br_pre(server_2)

    # Create connection with server 1
    br_connect(client, server_1)
    br_select(client, server_1.pub_addr)
    br_security(client, server_1)
    avrcp_connect(client, server_1)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server_1, psm)
    ca_connect(client, server_1, RspCode.SUCCESS)

    # Create connection with server 2
    br_connect(client, server_2)
    br_select(client, server_2.pub_addr)
    br_security(client, server_2)
    avrcp_connect(client, server_2)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server_2, psm)
    ca_connect(client, server_2, RspCode.SUCCESS)

    # Initiate concurrent operations
    br_select(client, server_1.pub_addr)
    ca_cmd_initiate(client, server_1, "get_image_properties")
    br_select(client, server_2.pub_addr)
    ca_cmd_initiate(client, server_2, "get_image_properties")
    br_select(client, server_1.pub_addr)
    ca_cmd_complete(client, server_1, "get_image_properties")
    br_select(client, server_2.pub_addr)
    ca_cmd_complete(client, server_2, "get_image_properties")

    br_disconnect(client, server_2)
    br_disconnect(client, server_1)
