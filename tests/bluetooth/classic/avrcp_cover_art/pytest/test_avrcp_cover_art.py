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


def br_pre(init, rsp):
    init.exec_command('br clear all')
    rsp.exec_command('br clear all')
    init.exec_command('bt clear all')
    rsp.exec_command('bt clear all')


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
    init.exec_command('avrcp connect')
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
        rf"Cover Art TG \w+ OBEX connect req: "
        rf"version {BT_OBEX_VERSION:02x}, mopl {BT_GOEP_MTU:04x}"
    )
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    cmd = "test_avrcp_cover_art tg connect "
    if rsp_code == RspCode.SUCCESS:
        cmd += 'success'
    elif rsp_code == RspCode.CONTINUE:
        cmd += 'continue'
    else:
        cmd += f'error {rsp_code:X}'
    rsp.exec_command(cmd)

    expected = rf"Cover Art CT \w+ conn rsp, rsp_code {rsp_code_str[rsp_code]}"
    expected += rf", version {BT_OBEX_VERSION:02x}, mopl {BT_GOEP_MTU:04x}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ca_cmd_initiate(init, rsp, cmd):
    """Initiate a Cover Art command and wait for initial Continue response."""
    init.exec_command(f'test_avrcp_cover_art ct {cmd}')
    expected = rf"Cover Art TG \w+ {cmd} req, final true"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror')
    expected = rf"Cover Art CT \w+ {cmd} rsp, rsp_code Continue"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ca_cmd_complete(init, rsp, cmd):
    """Complete a Cover Art command by continuing until Success response."""
    rsp_code = 'Continue'

    while True:
        init.exec_command(f'test_avrcp_cover_art ct {cmd}')
        expected = rf"Cover Art TG \w+ {cmd} req, final true"
        found, _ = rsp.wait_for_shell_response(expected)
        assert found is True

        rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror')
        expected = rf"Cover Art CT \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
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


def ca_cmd(init, rsp, cmd):
    ca_cmd_complete(init, rsp, cmd)


def ca_cmd_continue(init, rsp, cmd):
    ca_cmd_initiate(init, rsp, cmd)
    ca_cmd_complete(init, rsp, cmd)


def ca_cmd_fail(init, rsp, cmd, rsp_code):
    init.exec_command(f'test_avrcp_cover_art ct {cmd}')
    expected = rf"Cover Art TG \w+ {cmd} req, final true"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_avrcp_cover_art tg {cmd} error {rsp_code:X}')
    expected = rf"Cover Art CT \w+ {cmd} rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ca_cmd_final(init, rsp, cmd):
    rsp_code = 'Continue'
    final = 'false'

    # Request phase
    init.exec_command(f'test_avrcp_cover_art ct {cmd} chunked_req true')
    expected = rf"Cover Art TG \w+ {cmd} req, final false"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror')
    expected = rf"Cover Art CT \w+ {cmd} rsp, rsp_code {rsp_code}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_avrcp_cover_art ct {cmd}')
        expected = rf"Cover Art TG \w+ {cmd} req, final (?P<final>\w+)"
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is None:
                continue

            final = searched.group("final")
            if final == 'true':
                break

        rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror')
        expected = rf"Cover Art CT \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is None:
                continue

            rsp_code = searched.group("rsp_code")
            if rsp_code != 'Continue':
                break

        if rsp_code != 'Continue' or final == 'true':
            break

    assert rsp_code == 'Continue' and final == 'true'

    # Response phase
    ca_cmd_complete(init, rsp, cmd)


def ca_cmd_srm(init, rsp, cmd):
    rsp_code = 'Continue'

    init.exec_command(f'test_avrcp_cover_art ct {cmd} srm true')
    expected = [rf"Cover Art TG \w+ {cmd} req, final true"]
    expected.append("SRM value: 01")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror srm true')
    expected = [rf"Cover Art CT \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("SRM value: 01")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror')
        expected = rf"Cover Art CT \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
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


def ca_cmd_srm_param(init, rsp, cmd):
    rsp_code = 'Continue'

    init.exec_command(f'test_avrcp_cover_art ct {cmd} srm true srmp true')
    expected = [rf"Cover Art TG \w+ {cmd} req, final true"]
    expected.append("SRM value: 01")
    expected.append("SRM Parameter value: 01")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror srm true')
    expected = [rf"Cover Art CT \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("SRM value: 01")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    init.exec_command(f'test_avrcp_cover_art ct {cmd}')
    expected = rf"Cover Art TG \w+ {cmd} req, final true"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    while True:
        rsp.exec_command(f'test_avrcp_cover_art tg {cmd} noerror')
        expected = rf"Cover Art CT \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
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


def ca_cmd_abort(init, rsp, cmd):
    ca_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_avrcp_cover_art ct abort')
    expected = r"Cover Art TG \w+ abort req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command('test_avrcp_cover_art tg abort success')
    expected = r"Cover Art CT \w+ abort rsp, rsp_code Success"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ca_cmd_abort_fail(init, rsp, cmd, rsp_code):
    ca_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_avrcp_cover_art ct abort')
    expected = r"Cover Art TG \w+ abort req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_avrcp_cover_art tg abort error {rsp_code:X}')
    expected = r"Cover Art CT L2CAP Disconnected: \w+"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    expected = r"Cover Art TG L2CAP Disconnected: \w+"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def ca_cmd_abort_buffer(init, rsp, cmd):
    ca_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_avrcp_cover_art ct abort true')
    expected = [r"Cover Art TG \w+ abort req"]
    expected.append("T ff L 5 V abort")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command('test_avrcp_cover_art tg abort success')
    expected = r"Cover Art CT \w+ abort rsp, rsp_code Success"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ca_disconnect(init, rsp, rsp_code):
    init.exec_command('test_avrcp_cover_art ct disconnect')
    expected = r"Cover Art TG \w+ disconn req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    cmd = "test_avrcp_cover_art tg disconnect "
    if rsp_code == RspCode.SUCCESS:
        cmd += 'success'
    elif rsp_code == RspCode.CONTINUE:
        cmd += 'continue'
    else:
        cmd += f'error {rsp_code:X}'
    rsp.exec_command(cmd)

    expected = rf"Cover Art CT \w+ disconn rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def test_br_ca_server_l2cap_accept_001(client, server):
    """Verify server can accept L2CAP connection on Cover Art PSM."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    br_disconnect(server, client)


def test_br_ca_client_l2cap_connect_001(client, server):
    """Verify client can initiate L2CAP connection to Cover Art server."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    br_disconnect(client, server)


def test_br_ca_server_l2cap_disconnect_001(client, server):
    """Verify server can disconnect L2CAP connection."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_tg_l2cap_disconnect(client, server)
    br_disconnect(server, client)


def test_br_ca_client_l2cap_disconnect_001(client, server):
    """Verify client can disconnect L2CAP connection."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_ct_l2cap_disconnect(client, server)
    br_disconnect(client, server)


def test_br_ca_server_l2cap_reject_001(client, server):
    """Verify server rejects connection to invalid PSM."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect_fail(server, psm + 1)
    br_disconnect(server, client)


def test_br_ca_client_l2cap_connect_fail_001(client, server):
    """Verify client handles connection failure gracefully."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect_fail(client, psm + 1)
    br_disconnect(client, server)


def test_br_ca_server_l2cap_disconnect_remote_001(client, server):
    """Verify server handles remote disconnection properly."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_ct_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ca_client_l2cap_disconnect_remote_001(client, server):
    """Verify client handles remote disconnection properly."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_tg_l2cap_disconnect(server, client)
    br_disconnect(client, server)


def test_br_ca_server_connect_success_001(client, server):
    """Verify server accepts valid OBEX connection request."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    br_disconnect(server, client)


def test_br_ca_client_connect_success_001(client, server):
    """Verify client can initiate OBEX connection."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    br_disconnect(client, server)


def test_br_ca_server_connect_reject_001(client, server):
    """Verify server can reject OBEX connection."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.FORBIDDEN)
    br_disconnect(server, client)


def test_br_ca_client_connect_reject_001(client, server):
    """Verify client handles OBEX connection rejection."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.FORBIDDEN)
    br_disconnect(client, server)


def test_br_ca_server_get_img_props_success_001(client, server):
    """Verify server can respond to GetImageProperties request successfully."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd(server, client, "get_image_properties")
    br_disconnect(server, client)


def test_br_ca_client_get_img_props_success_001(client, server):
    """Verify client can request image properties successfully."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd(client, server, "get_image_properties")
    br_disconnect(client, server)


def test_br_ca_server_get_img_props_error_001(client, server):
    """Verify server can return error for invalid image handle."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_fail(server, client, "get_image_properties", RspCode.NOT_FOUND)
    br_disconnect(server, client)


def test_br_ca_client_get_img_props_error_001(client, server):
    """Verify client handles error response properly."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_fail(client, server, "get_image_properties", RspCode.NOT_FOUND)
    br_disconnect(client, server)


def test_br_ca_server_get_img_props_continue_001(client, server):
    """Verify server can send continue response for multi-packet properties."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_continue(server, client, "get_image_properties")
    br_disconnect(server, client)


def test_br_ca_client_get_img_props_continue_001(client, server):
    """Verify client handles continue response for multi-packet properties."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_continue(client, server, "get_image_properties")
    br_disconnect(client, server)


def test_br_ca_server_get_img_props_final_false_001(client, server):
    """Verify server handles non-final GetImageProperties request."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_final(server, client, "get_image_properties")
    br_disconnect(server, client)


def test_br_ca_client_get_img_props_final_false_001(client, server):
    """Verify client can send non-final GetImageProperties request."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_final(client, server, "get_image_properties")
    br_disconnect(client, server)


def test_br_ca_server_get_img_props_srm_001(client, server):
    """Verify server supports Single Response Mode for GetImageProperties."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_srm(server, client, "get_image_properties")
    br_disconnect(server, client)


def test_br_ca_client_get_img_props_srm_001(client, server):
    """Verify client supports Single Response Mode for GetImageProperties."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_srm(client, server, "get_image_properties")
    br_disconnect(client, server)


def test_br_ca_server_get_img_props_srm_param_001(client, server):
    """Verify server handles SRM parameters correctly."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_srm_param(server, client, "get_image_properties")
    br_disconnect(server, client)


def test_br_ca_client_get_img_props_srm_param_001(client, server):
    """Verify client can send SRM parameters."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_srm_param(client, server, "get_image_properties")
    br_disconnect(client, server)


def test_br_ca_server_get_image_success_001(client, server):
    """Verify server can respond to GetImage request successfully."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd(server, client, "get_image")
    br_disconnect(server, client)


def test_br_ca_client_get_image_success_001(client, server):
    """Verify client can request image successfully."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd(client, server, "get_image")
    br_disconnect(client, server)


def test_br_ca_server_get_image_error_001(client, server):
    """Verify server can return error for invalid image descriptor."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_fail(server, client, "get_image", RspCode.NOT_ACCEPT)
    br_disconnect(server, client)


def test_br_ca_client_get_image_error_001(client, server):
    """Verify client handles error response properly."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_fail(client, server, "get_image", RspCode.NOT_ACCEPT)
    br_disconnect(client, server)


def test_br_ca_server_get_image_continue_001(client, server):
    """Verify server can send continue response for large images."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_continue(server, client, "get_image")
    br_disconnect(server, client)


def test_br_ca_client_get_image_continue_001(client, server):
    """Verify client handles continue response for large images."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_continue(client, server, "get_image")
    br_disconnect(client, server)


def test_br_ca_server_get_image_final_false_001(client, server):
    """Verify server handles non-final GetImage request."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_final(server, client, "get_image")
    br_disconnect(server, client)


def test_br_ca_client_get_image_final_false_001(client, server):
    """Verify client can send non-final GetImage request."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_final(client, server, "get_image")
    br_disconnect(client, server)


def test_br_ca_server_get_image_srm_001(client, server):
    """Verify server supports Single Response Mode for GetImage."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_srm(server, client, "get_image")
    br_disconnect(server, client)


def test_br_ca_client_get_image_srm_001(client, server):
    """Verify client supports Single Response Mode for GetImage."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_srm(client, server, "get_image")
    br_disconnect(client, server)


def test_br_ca_server_get_image_srm_param_001(client, server):
    """Verify server handles SRM parameters correctly."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_srm_param(server, client, "get_image")
    br_disconnect(server, client)


def test_br_ca_client_get_image_srm_param_001(client, server):
    """Verify client can send SRM parameters."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_srm_param(client, server, "get_image")
    br_disconnect(client, server)


def test_br_ca_server_get_thumb_success_001(client, server):
    """Verify server can respond to GetLinkedThumbnail request successfully."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd(server, client, "get_linked_thumbnail")
    br_disconnect(server, client)


def test_br_ca_client_get_thumb_success_001(client, server):
    """Verify client can request thumbnail successfully."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd(client, server, "get_linked_thumbnail")
    br_disconnect(client, server)


def test_br_ca_server_get_thumb_error_001(client, server):
    """Verify server can return error for handle without thumbnail."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_fail(server, client, "get_linked_thumbnail", RspCode.NOT_FOUND)
    br_disconnect(server, client)


def test_br_ca_client_get_thumb_error_001(client, server):
    """Verify client handles error response properly."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_fail(client, server, "get_linked_thumbnail", RspCode.NOT_FOUND)
    br_disconnect(client, server)


def test_br_ca_server_get_thumb_continue_001(client, server):
    """Verify server can send continue response for large thumbnails."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_continue(server, client, "get_linked_thumbnail")
    br_disconnect(server, client)


def test_br_ca_client_get_thumb_continue_001(client, server):
    """Verify client handles continue response for large thumbnails."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_continue(client, server, "get_linked_thumbnail")
    br_disconnect(client, server)


def test_br_ca_server_get_thumb_final_false_001(client, server):
    """Verify server handles non-final GetLinkedThumbnail request."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_final(server, client, "get_linked_thumbnail")
    br_disconnect(server, client)


def test_br_ca_client_get_thumb_final_false_001(client, server):
    """Verify client can send non-final GetLinkedThumbnail request."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_final(client, server, "get_linked_thumbnail")
    br_disconnect(client, server)


def test_br_ca_server_get_thumb_srm_001(client, server):
    """Verify server supports Single Response Mode for GetLinkedThumbnail."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_srm(server, client, "get_linked_thumbnail")
    br_disconnect(server, client)


def test_br_ca_client_get_thumb_srm_001(client, server):
    """Verify client supports Single Response Mode for GetLinkedThumbnail."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_srm(client, server, "get_linked_thumbnail")
    br_disconnect(client, server)


def test_br_ca_server_get_thumb_srm_param_001(client, server):
    """Verify server handles SRM parameters correctly."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_srm_param(server, client, "get_linked_thumbnail")
    br_disconnect(server, client)


def test_br_ca_client_get_thumb_srm_param_001(client, server):
    """Verify client can send SRM parameters."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_srm_param(client, server, "get_linked_thumbnail")
    br_disconnect(client, server)


def test_br_ca_server_abort_get_props_001(client, server):
    """Verify server can abort GetImageProperties operation."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_abort(server, client, "get_image_properties")
    ca_cmd(server, client, "get_image")
    br_disconnect(server, client)


def test_br_ca_client_abort_get_props_001(client, server):
    """Verify client can abort GetImageProperties operation."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_abort(client, server, "get_image_properties")
    ca_cmd(client, server, "get_image")
    br_disconnect(client, server)


def test_br_ca_server_abort_get_image_001(client, server):
    """Verify server can abort GetImage operation."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_abort(server, client, "get_image")
    ca_cmd(server, client, "get_image_properties")
    br_disconnect(server, client)


def test_br_ca_client_abort_get_image_001(client, server):
    """Verify client can abort GetImage operation."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_abort(client, server, "get_image")
    ca_cmd(client, server, "get_image_properties")
    br_disconnect(client, server)


def test_br_ca_server_abort_get_thumb_001(client, server):
    """Verify server can abort GetLinkedThumbnail operation."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_abort(server, client, "get_linked_thumbnail")
    ca_cmd(server, client, "get_image_properties")
    br_disconnect(server, client)


def test_br_ca_client_abort_get_thumb_001(client, server):
    """Verify client can abort GetLinkedThumbnail operation."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_abort(client, server, "get_linked_thumbnail")
    ca_cmd(client, server, "get_image_properties")
    br_disconnect(client, server)


def test_br_ca_server_abort_error_001(client, server):
    """Verify server can return error for invalid abort request."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_abort_fail(server, client, "get_image", RspCode.NOT_ACCEPT)
    br_disconnect(server, client)


def test_br_ca_client_abort_error_001(client, server):
    """Verify client handles abort error response."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_abort_fail(client, server, "get_image", RspCode.NOT_ACCEPT)
    br_disconnect(client, server)


def test_br_ca_client_abort_buffer_001(client, server):
    """Verify client can create abort request PDU buffer."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_abort_buffer(client, server, "get_image")
    ca_cmd(client, server, "get_image_properties")
    br_disconnect(client, server)


def test_br_ca_server_disconnect_success_001(client, server):
    """Verify server can respond to OBEX disconnect request successfully."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_disconnect(server, client, RspCode.SUCCESS)
    br_disconnect(server, client)


def test_br_ca_client_disconnect_success_001(client, server):
    """Verify client can initiate OBEX disconnect successfully."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_disconnect(client, server, RspCode.SUCCESS)
    br_disconnect(client, server)


def test_br_ca_server_disconnect_error_001(client, server):
    """Verify server can return error for invalid disconnect request."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_disconnect(server, client, RspCode.BAD_REQ)
    br_disconnect(server, client)


def test_br_ca_client_disconnect_error_001(client, server):
    """Verify client handles disconnect error response."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_disconnect(client, server, RspCode.BAD_REQ)
    br_disconnect(client, server)


def test_br_ca_server_disconnect_force_001(client, server):
    """Verify server handles forced disconnection."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    ca_cmd_initiate(server, client, 'get_image')
    ca_ct_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ca_client_disconnect_force_001(client, server):
    """Verify client handles forced disconnection."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    ca_cmd_initiate(client, server, 'get_image')
    ca_tg_l2cap_disconnect(server, client)
    br_disconnect(client, server)


def test_br_ca_server_rapid_connect_disconnect_001(client, server):
    """Verify server handles rapid connection cycles."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    for _ in range(10):
        ca_ct_l2cap_connect(server, client, psm)
        ca_ct_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ca_client_rapid_connect_disconnect_001(client, server):
    """Verify client handles rapid connection cycles."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    for _ in range(10):
        ca_ct_l2cap_connect(client, server, psm)
        ca_ct_l2cap_disconnect(client, server)
    br_disconnect(client, server)


def test_br_ca_server_cb_register_null_001(client):
    """Verify server callback registration handles NULL parameter."""
    lines = client.exec_command('test_avrcp_cover_art tg reg NULL')
    expected = "Failed to reg cover art TG cb"
    found = client.check_shell_response(lines, expected)
    if found is False:
        found, _ = client.wait_for_shell_response(expected)
        assert found is True


def test_br_ca_client_cb_register_null_001(client):
    """Verify client callback registration handles NULL parameter."""
    lines = client.exec_command('test_avrcp_cover_art ct reg NULL')
    expected = "Failed to reg cover art CT cb"
    found = client.check_shell_response(lines, expected)
    if found is False:
        found, _ = client.wait_for_shell_response(expected)
        assert found is True


def test_br_ca_server_pdu_buffer_pool_001(client, server):
    """Verify server PDU buffer creation with custom pool."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    lines = client.exec_command('test_avrcp_cover_art tg alloc_buf')
    expected = r"Allocate tx buffer successfully: \w+"
    found = client.check_shell_response(lines, expected)
    if found is False:
        found, _ = client.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(server, client)


def test_br_ca_client_pdu_buffer_pool_001(client, server):
    """Verify client PDU buffer creation with custom pool."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    lines = client.exec_command('test_avrcp_cover_art ct alloc_buf')
    expected = r"Allocate tx buffer successfully: \w+"
    found = client.check_shell_response(lines, expected)
    if found is False:
        found, _ = client.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(client, server)


def test_br_ca_server_pdu_buffer_null_pool_001(client, server):
    """Verify server PDU buffer creation handles NULL pool."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    ca_connect(server, client, RspCode.SUCCESS)
    lines = client.exec_command('test_avrcp_cover_art tg alloc_buf NULL')
    expected = r"Allocate tx buffer successfully: \w+"
    found = client.check_shell_response(lines, expected)
    if found is False:
        found, _ = client.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(server, client)


def test_br_ca_client_pdu_buffer_null_pool_001(client, server):
    """Verify client PDU buffer creation handles NULL pool."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    ca_connect(client, server, RspCode.SUCCESS)
    lines = client.exec_command('test_avrcp_cover_art ct alloc_buf NULL')
    expected = r"Allocate tx buffer successfully: \w+"
    found = client.check_shell_response(lines, expected)
    if found is False:
        found, _ = client.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(client, server)


def test_br_ca_server_invalid_state_001(client, server):
    """Verify server APIs handle invalid state conditions."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    avrcp_connect(server, client)
    psm = sdp_discover(server)
    ca_ct_l2cap_connect(server, client, psm)
    lines = client.exec_command('test_avrcp_cover_art tg get_image_properties noerror')
    expected = "Fail to send image properties rsp"
    found = client.check_shell_response(lines, expected)
    if found is False:
        found, _ = client.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(server, client)


def test_br_ca_client_invalid_state_001(client, server):
    """Verify client APIs handle invalid state conditions."""
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    avrcp_connect(client, server)
    psm = sdp_discover(client)
    ca_ct_l2cap_connect(client, server, psm)
    lines = client.exec_command('test_avrcp_cover_art ct get_image_properties')
    expected = "Fail to send get_image_properties req"
    found = client.check_shell_response(lines, expected)
    if found is False:
        found, _ = client.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(client, server)
