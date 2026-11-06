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


def mns_rfcomm_connect(init, rsp):
    channel = rsp.channel
    supported_features = rsp.supported_features

    init.exec_command(f'test_map mse mns rfcomm_connect {channel:02X} {supported_features:08X}')
    expected = rf"MSE MNS RFCOMM connected: \w+, addr: {rsp.pub_addr}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True
    expected = rf"MCE MNS RFCOMM connected: \w+, addr: {init.pub_addr}"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def mns_rfcomm_connect_fail(init, rsp):
    channel = 0x1E  # RFCOMM_CHANNEL_END to trigger connection failure
    supported_features = rsp.supported_features

    lines = init.exec_command(
        f'test_map mse mns rfcomm_connect {channel:02X} {supported_features:08X}'
    )
    expected = "MSE MNS RFCOMM disconnected"
    found = init.check_shell_response(lines, expected)
    if found is False:
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def mns_rfcomm_disconnect(init, rsp):
    init.exec_command('test_map mse mns rfcomm_disconnect')
    found, _ = init.wait_for_shell_response("MSE MNS RFCOMM disconnected:")
    assert found is True
    found, _ = rsp.wait_for_shell_response("MCE MNS RFCOMM disconnected:")
    assert found is True


def mns_l2cap_connect(init, rsp):
    psm = rsp.psm
    supported_features = rsp.supported_features

    init.exec_command(f'test_map mse mns l2cap_connect {psm:04X} {supported_features:08X}')
    expected = rf"MSE MNS L2CAP connected: \w+, addr: {rsp.pub_addr}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True
    expected = rf"MCE MNS L2CAP connected: \w+, addr: {init.pub_addr}"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def mns_l2cap_connect_fail(init, rsp):
    psm = rsp.psm + 0x1000  # Invalid PSM to trigger connection failure
    supported_features = rsp.supported_features

    lines = init.exec_command(f'test_map mse mns l2cap_connect {psm:04X} {supported_features:08X}')
    expected = "MSE MNS L2CAP disconnected"
    found = init.check_shell_response(lines, expected)
    if found is False:
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def mns_l2cap_disconnect(init, rsp):
    init.exec_command('test_map mse mns l2cap_disconnect')
    found, _ = init.wait_for_shell_response("MSE MNS L2CAP disconnected:")
    assert found is True
    found, _ = rsp.wait_for_shell_response("MCE MNS L2CAP disconnected:")
    assert found is True


def mns_connect(init, rsp, rsp_code):
    init.exec_command('test_map mse mns connect')
    expected = (
        rf"MCE MNS \w+ conn req, "
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

    expected = rf"MSE MNS \w+ conn rsp, rsp_code {rsp_code_str[rsp_code]}"
    expected += rf", version {BT_OBEX_VERSION:02x}, mopl"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mns_disconnect(init, rsp, rsp_code):
    init.exec_command('test_map mse mns disconnect')
    expected = r"MCE MNS \w+ disconn req"
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

    expected = rf"MSE MNS \w+ disconn rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mns_put_cmd_initiate(init, rsp, cmd):
    """Initiate a PUT command and wait for initial Continue response."""
    init.exec_command(f'test_map mse mns {cmd}')
    expected = [rf"MCE MNS \w+ {cmd} req, final false"]
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mce mns {cmd} noerror')
    expected = rf"MSE MNS \w+ {cmd} rsp, rsp_code Continue"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mns_put_cmd_complete(init, rsp, cmd):
    """Complete a PUT command by continuing until Success response."""
    rsp_code = 'Continue'
    final = 'false'

    while True:
        init.exec_command(f'test_map mse mns {cmd}')
        expected = [rf"MCE MNS \w+ {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MNS \w+ {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        rsp.exec_command(f'test_map mce mns {cmd} noerror')
        expected = rf"MSE MNS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
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


def mns_put_cmd(init, rsp, cmd):
    mns_put_cmd_complete(init, rsp, cmd)


def mns_put_cmd_continue(init, rsp, cmd):
    mns_put_cmd_initiate(init, rsp, cmd)
    mns_put_cmd_complete(init, rsp, cmd)


def mns_put_cmd_fail(init, rsp, cmd, rsp_code):
    init.exec_command(f'test_map mse mns {cmd}')
    expected = [rf"MCE MNS \w+ {cmd} req, final \w+"]
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mce mns {cmd} error {rsp_code:X}')
    expected = rf"MSE MNS \w+ {cmd} rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mns_put_cmd_srm(init, rsp, cmd):
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_map mse mns {cmd}')
    expected = [rf"MCE MNS \w+ {cmd} req, final \w+"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mce mns {cmd} noerror')
    expected = [rf"MSE MNS \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_map mse mns {cmd}')
        expected = [rf"MCE MNS \w+ {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MNS \w+ {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    assert final == 'true'

    rsp.exec_command(f'test_map mce mns {cmd} noerror')
    expected = rf"MSE MNS \w+ {cmd} rsp, rsp_code Success"
    found, lines = init.wait_for_shell_response(expected)
    assert found is True


def mns_put_cmd_srm_param(init, rsp, cmd):
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_map mse mns {cmd}')
    expected = [rf"MCE MNS \w+ {cmd} req, final \w+"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mce mns {cmd} noerror srmp')
    expected = [rf"MSE MNS \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    expected.append("OBEX SRMP: 01")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_map mse mns {cmd}')
        expected = [rf"MCE MNS \w+ {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MNS \w+ {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    assert final == 'true'

    rsp.exec_command(f'test_map mce mns {cmd} noerror')
    expected = rf"MSE MNS \w+ {cmd} rsp, rsp_code Success"
    found, lines = init.wait_for_shell_response(expected)
    assert found is True


def mns_put_cmd_abort(init, rsp, cmd):
    mns_put_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_map mse mns abort')
    expected = r"MCE MNS \w+ abort req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command('test_map mce mns abort success')
    expected = r"MSE MNS \w+ abort rsp, rsp_code Success"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mns_put_cmd_abort_fail(init, rsp, cmd, rsp_code):
    mns_put_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_map mse mns abort')
    expected = r"MCE MNS \w+ abort req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mce mns abort error {rsp_code:X}')
    expected = r"MSE MNS L2CAP disconnected: \w+"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    expected = r"MCE MNS L2CAP disconnected: \w+"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def test_br_map_mse_mns_rfcomm_connect_success(client, server):
    """
    BR_MAP_MSE_MNS_RFCOMM_CONNECT_SUCCESS

    Verify that MSE MNS can successfully initiate an RFCOMM connection to MCE MNS server.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_rfcomm_connect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_rfcomm_disconnect_success(client, server):
    """
    BR_MAP_MSE_MNS_RFCOMM_DISCONNECT_SUCCESS

    Verify that MSE MNS can successfully disconnect an RFCOMM connection.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_rfcomm_connect(client, server)
    mns_rfcomm_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_l2cap_connect_success(client, server):
    """
    BR_MAP_MSE_MNS_L2CAP_CONNECT_SUCCESS

    Verify that MSE MNS can successfully initiate an L2CAP connection to MCE MNS server.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_l2cap_disconnect_success(client, server):
    """
    BR_MAP_MSE_MNS_L2CAP_DISCONNECT_SUCCESS

    Verify that MSE MNS can successfully disconnect an L2CAP connection.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    mns_l2cap_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_obex_connect_success(client, server):
    """
    BR_MAP_MSE_MNS_OBEX_CONNECT_SUCCESS

    Verify that MSE MNS can successfully send an OBEX CONNECT request and receive success response.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_rfcomm_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_rfcomm_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_obex_connect_error(client, server):
    """
    BR_MAP_MSE_MNS_OBEX_CONNECT_ERROR

    Verify that MSE MNS can handle OBEX CONNECT request rejection from MCE MNS server.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_rfcomm_connect(client, server)
    mns_connect(client, server, RspCode.FORBIDDEN)
    mns_rfcomm_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_obex_disconnect_success(client, server):
    """
    BR_MAP_MSE_MNS_OBEX_DISCONNECT_SUCCESS

    Verify that MSE MNS can successfully send an OBEX DISCONNECT request
    and receive success response.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_l2cap_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_obex_disconnect_error(client, server):
    """
    BR_MAP_MSE_MNS_OBEX_DISCONNECT_ERROR

    Verify that MSE MNS can handle OBEX DISCONNECT request error response from MCE MNS server.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_disconnect(client, server, RspCode.FORBIDDEN)
    mns_l2cap_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_sendevent_success(client, server):
    """
    BR_MAP_MSE_MNS_SENDEVENT_SUCCESS

    Verify that MSE MNS can successfully send a complete SendEvent request
    in a single packet and receive success response.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_rfcomm_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_put_cmd(client, server, 'send_event')
    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_rfcomm_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_sendevent_error(client, server):
    """
    BR_MAP_MSE_MNS_SENDEVENT_ERROR

    Verify that MSE MNS can handle SendEvent request error response from MCE MNS server.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_rfcomm_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_put_cmd_fail(client, server, 'send_event', RspCode.BAD_REQ)
    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_rfcomm_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_sendevent_continue(client, server):
    """
    BR_MAP_MSE_MNS_SENDEVENT_CONTINUE

    Verify that MSE MNS can handle multi-packet SendEvent transfer with CONTINUE responses.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_rfcomm_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_put_cmd_continue(client, server, 'send_event')
    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_rfcomm_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_sendevent_srm_enabled(client, server):
    """
    BR_MAP_MSE_MNS_SENDEVENT_SRM_ENABLED

    Verify that MSE MNS can successfully send multi-packet SendEvent
    with Single Response Mode (SRM) enabled.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_put_cmd_srm(client, server, 'send_event')
    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_l2cap_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_sendevent_srm_wait(client, server):
    """
    BR_MAP_MSE_MNS_SENDEVENT_SRM_WAIT

    Verify that MSE MNS can handle SRMP Wait parameter during SRM-enabled SendEvent operation.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_put_cmd_srm_param(client, server, 'send_event')
    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_l2cap_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_sendevent_abort_during_transfer(client, server):
    """
    BR_MAP_MSE_MNS_SENDEVENT_ABORT_DURING_TRANSFER

    Verify that MSE MNS can successfully abort an ongoing multi-packet SendEvent operation.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_put_cmd_abort(client, server, 'send_event')
    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_l2cap_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mse_mns_sendevent_abort_failure(client, server):
    """
    BR_MAP_MSE_MNS_SENDEVENT_ABORT_FAILURE

    Verify that MSE MNS can handle ABORT request error response during SendEvent operation.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)
    mns_put_cmd_abort_fail(client, server, 'send_event', RspCode.FORBIDDEN)
    br_disconnect(client, server)


def test_br_map_mse_mns_sendevent_param_all(client, server):
    """
    BR_MAP_MSE_MNS_SENDEVENT_PARAM_ALL

    Verify that MSE MNS can send all supported application parameters in SendEvent
    request and MCE MNS can correctly parse and match these parameters.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mns_l2cap_connect(client, server)
    mns_connect(client, server, RspCode.SUCCESS)

    # Clean up any existing parameters
    client.exec_command('test_map add_header app_param clear')
    server.exec_command('test_map add_header app_param clear')

    # MSE MNS adds all application parameters for SendEvent request
    # MASInstanceID (0x0F) = 0x00 (Instance 0)
    client.exec_command('test_map add_header app_param mas_instance_id 0')

    # Send SendEvent request
    cmd = "send_event"
    client.exec_command(f'test_map mse mns {cmd}')

    # Expected patterns for MCE MNS to verify all parameters are received
    expected_params = [
        rf"MCE MNS \w+ {cmd} req, final false",
        r"T 0f L 1",  # MASInstanceID (tag 0x0F)
        r"MASInstanceID: 0",
        r"OBEX BODY: .*",
    ]

    # Wait for MCE MNS to receive and parse the request
    found, _ = server.wait_for_shell_response(expected_params, timeout=10)
    assert found is True, "MCE MNS did not receive all application parameters"

    # MCE MNS sends Continue response
    server.exec_command(f'test_map mce mns {cmd} noerror')
    expected = [rf"MSE MNS \w+ {cmd} rsp, rsp_code Continue"]
    expected.append("OBEX SRM: 01")
    found, lines = client.wait_for_shell_response(expected)
    assert found is True

    # Continue sending event data until final packet
    while True:
        client.exec_command(f'test_map mse mns {cmd}')
        expected = [rf"MCE MNS \w+ {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = server.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MNS \w+ {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    assert final == 'true'

    # MCE MNS sends Success response
    server.exec_command(f'test_map mce mns {cmd} noerror')
    expected = rf"MSE MNS \w+ {cmd} rsp, rsp_code Success"
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    mns_disconnect(client, server, RspCode.SUCCESS)
    mns_l2cap_disconnect(client, server)
    br_disconnect(client, server)
