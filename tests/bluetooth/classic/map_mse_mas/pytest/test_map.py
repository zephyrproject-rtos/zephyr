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


def mas_rfcomm_connect(init, rsp):
    channel = rsp.channel
    instance_id = rsp.instance_id
    supported_features = rsp.supported_features

    init.exec_command(
        f'test_map mce mas rfcomm_connect {channel:02X} {instance_id} {supported_features:08X}'
    )
    expected = rf"MCE MAS RFCOMM connected: \w+, addr: {rsp.pub_addr}, instance_id: {instance_id}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True
    expected = rf"MSE MAS RFCOMM connected: \w+, addr: {init.pub_addr}, instance_id: {instance_id}"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def mas_rfcomm_connect_fail(init, rsp):
    channel = 0x1E  # RFCOMM_CHANNEL_END to trigger connection failure
    instance_id = rsp.instance_id
    supported_features = rsp.supported_features

    lines = init.exec_command(
        f'test_map mce mas rfcomm_connect {channel:02X} {instance_id} {supported_features:08X}'
    )
    expected = "MCE MAS RFCOMM disconnected"
    found = init.check_shell_response(lines, expected)
    if found is False:
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def mas_rfcomm_disconnect(init, rsp):
    init.exec_command('test_map mce mas rfcomm_disconnect')
    found, _ = init.wait_for_shell_response("MCE MAS RFCOMM disconnected:")
    assert found is True
    found, _ = rsp.wait_for_shell_response("MSE MAS RFCOMM disconnected:")
    assert found is True


def mse_mas_rfcomm_disconnect(init, rsp):
    init.exec_command('test_map mse mas rfcomm_disconnect')
    found, _ = init.wait_for_shell_response("MSE MAS RFCOMM disconnected:")
    assert found is True
    found, _ = rsp.wait_for_shell_response("MCE MAS RFCOMM disconnected:")
    assert found is True


def mas_l2cap_connect(init, rsp):
    psm = rsp.psm
    instance_id = rsp.instance_id
    supported_features = rsp.supported_features

    init.exec_command(
        f'test_map mce mas l2cap_connect {psm:04X} {instance_id} {supported_features:08X}'
    )
    expected = rf"MCE MAS L2CAP connected: \w+, addr: {rsp.pub_addr}, instance_id: {instance_id}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True
    expected = rf"MSE MAS L2CAP connected: \w+, addr: {init.pub_addr}, instance_id: {instance_id}"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def mas_l2cap_connect_fail(init, rsp):
    psm = rsp.psm + 0x1000  # Invalid PSM to trigger connection failure
    instance_id = rsp.instance_id
    supported_features = rsp.supported_features

    lines = init.exec_command(
        f'test_map mce mas l2cap_connect {psm:04X} {instance_id} {supported_features:08X}'
    )
    expected = "MCE MAS L2CAP disconnected"
    found = init.check_shell_response(lines, expected)
    if found is False:
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def mas_l2cap_disconnect(init, rsp):
    init.exec_command('test_map mce mas l2cap_disconnect')
    found, _ = init.wait_for_shell_response("MCE MAS L2CAP disconnected:")
    assert found is True
    found, _ = rsp.wait_for_shell_response("MSE MAS L2CAP disconnected:")
    assert found is True


def mse_mas_l2cap_disconnect(init, rsp):
    init.exec_command('test_map mse mas l2cap_disconnect')
    found, _ = init.wait_for_shell_response("MSE MAS L2CAP disconnected:")
    assert found is True
    found, _ = rsp.wait_for_shell_response("MCE MAS L2CAP disconnected:")
    assert found is True


def mas_connect(init, rsp, rsp_code):
    init.exec_command('test_map mce mas connect')
    expected = (
        rf"MSE MAS \w+ conn req, "
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

    expected = rf"MCE MAS \w+ conn rsp, rsp_code {rsp_code_str[rsp_code]}"
    expected += rf", version {BT_OBEX_VERSION:02x}, mopl"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_disconnect(init, rsp, rsp_code):
    init.exec_command('test_map mce mas disconnect')
    expected = r"MSE MAS \w+ disconn req"
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

    expected = rf"MCE MAS \w+ disconn rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_get_cmd_initiate(init, rsp, cmd):
    """Initiate a MAP command and wait for initial Continue response."""
    init.exec_command(f'test_map mce mas {cmd}')
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code Continue"]
    expected.append(r"OBEX BODY: .*")
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_get_cmd_complete(init, rsp, cmd):
    """Complete a MAP command by continuing until Success response."""
    rsp_code = 'Continue'

    while True:
        init.exec_command(f'test_map mce mas {cmd}')
        expected = rf"MSE MAS \w+ {cmd} req, final true"
        found, _ = rsp.wait_for_shell_response(expected)
        assert found is True

        rsp.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'


def mas_get_cmd(init, rsp, cmd):
    mas_get_cmd_complete(init, rsp, cmd)


def mas_get_cmd_continue(init, rsp, cmd):
    mas_get_cmd_initiate(init, rsp, cmd)
    mas_get_cmd_complete(init, rsp, cmd)


def mas_get_cmd_fail(init, rsp, cmd, rsp_code):
    init.exec_command(f'test_map mce mas {cmd}')
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} error {rsp_code:X}')
    expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_get_cmd_final(init, rsp, cmd):
    rsp_code = 'Continue'
    final = 'false'

    # Request phase
    init.exec_command(f'test_map mce mas {cmd} chunked_req')
    expected = rf"MSE MAS \w+ {cmd} req, final false"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_map mce mas {cmd}')
        expected = rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        rsp.exec_command(f'test_map mse mas {cmd} noerror')
        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue' or final == 'true':
            break

    assert final == 'true'

    if rsp_code == 'Success':
        expected = r"OBEX BODY: .*"
        found = init.check_shell_response(lines, expected)
        if not found:
            found, _ = init.wait_for_shell_response(expected)
            assert found is True
        return

    assert rsp_code == 'Continue'

    # Response phase
    mas_get_cmd_complete(init, rsp, cmd)


def mas_get_cmd_final_fail(init, rsp, cmd, rsp_code):
    init.exec_command(f'test_map mce mas {cmd} chunked_req')
    expected = rf"MSE MAS \w+ {cmd} req, final false"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} error {rsp_code:X}')
    expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_get_cmd_srm(init, rsp, cmd):
    rsp_code = 'Continue'

    init.exec_command(f'test_map mce mas {cmd}')
    expected = [rf"MSE MAS \w+ {cmd} req, final true"]
    expected.append("OBEX SRM: 01")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        rsp.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'


def mas_get_cmd_final_srm(init, rsp, cmd):
    rsp_code = 'Continue'
    final = 'false'

    # Request phase
    init.exec_command(f'test_map mce mas {cmd} chunked_req')
    expected = [rf"MSE MAS \w+ {cmd} req, final false"]
    expected.append("OBEX SRM: 01")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_map mce mas {cmd}')
        expected = rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    assert final == 'true'

    # Response phase
    while True:
        rsp.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'


def mas_get_cmd_srm_param(init, rsp, cmd):
    rsp_code = 'Continue'

    init.exec_command(f'test_map mce mas {cmd} srmp')
    expected = [rf"MSE MAS \w+ {cmd} req, final true"]
    expected.append("OBEX SRM: 01")
    expected.append("OBEX SRMP: 01")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror srmp')
    expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    expected.append("OBEX SRMP: 01")
    expected.append(r"OBEX BODY: .*")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True

    init.exec_command(f'test_map mce mas {cmd}')
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    while True:
        rsp.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'


def mas_get_cmd_abort(init, rsp, cmd):
    mas_get_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_map mce mas abort')
    expected = r"MSE MAS \w+ abort req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command('test_map mse mas abort success')
    expected = r"MCE MAS \w+ abort rsp, rsp_code Success"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_get_cmd_abort_fail(init, rsp, cmd, rsp_code):
    mas_get_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_map mce mas abort')
    expected = r"MSE MAS \w+ abort req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas abort error {rsp_code:X}')
    expected = r"MCE MAS L2CAP disconnected: \w+"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    expected = r"MSE MAS L2CAP disconnected: \w+"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def mas_put_cmd_initiate(init, rsp, cmd):
    """Initiate a PUT command and wait for initial Continue response."""
    init.exec_command(f'test_map mce mas {cmd}')
    expected = [rf"MSE MAS \w+ {cmd} req, final false"]
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code Continue"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_put_cmd_complete(init, rsp, cmd):
    """Complete a PUT command by continuing until Success response."""
    rsp_code = 'Continue'
    final = 'false'

    while True:
        init.exec_command(f'test_map mce mas {cmd}')
        expected = [rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        rsp.exec_command(f'test_map mse mas {cmd} noerror')
        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
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


def mas_put_cmd(init, rsp, cmd):
    mas_put_cmd_complete(init, rsp, cmd)


def mas_put_cmd_continue(init, rsp, cmd):
    mas_put_cmd_initiate(init, rsp, cmd)
    mas_put_cmd_complete(init, rsp, cmd)


def mas_put_cmd_fail(init, rsp, cmd, rsp_code):
    init.exec_command(f'test_map mce mas {cmd}')
    expected = [rf"MSE MAS \w+ {cmd} req, final \w+"]
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} error {rsp_code:X}')
    expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_put_cmd_srm(init, rsp, cmd):
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_map mce mas {cmd}')
    expected = [rf"MSE MAS \w+ {cmd} req, final false"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_map mce mas {cmd}')
        expected = [rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    assert final == 'true'

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code Success"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_put_cmd_srm_param(init, rsp, cmd):
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_map mce mas {cmd}')
    expected = [rf"MSE MAS \w+ {cmd} req, final false"]
    expected.append("OBEX SRM: 01")
    expected.append(r"OBEX BODY: .*")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror srmp')
    expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code}"]
    expected.append("OBEX SRM: 01")
    expected.append("OBEX SRMP: 01")
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    init.exec_command(f'test_map mce mas {cmd}')
    expected = [rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"]
    expected.append(r"OBEX BODY: .*")
    found, lines = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code {rsp_code}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_map mce mas {cmd}')
        expected = [rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    assert final == 'true'

    rsp.exec_command(f'test_map mse mas {cmd} noerror')
    expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code Success"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_put_cmd_abort(init, rsp, cmd):
    mas_put_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_map mce mas abort')
    expected = r"MSE MAS \w+ abort req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command('test_map mse mas abort success')
    expected = r"MCE MAS \w+ abort rsp, rsp_code Success"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def mas_put_cmd_abort_fail(init, rsp, cmd, rsp_code):
    mas_put_cmd_initiate(init, rsp, cmd)

    init.exec_command('test_map mce mas abort')
    expected = r"MSE MAS \w+ abort req"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_map mse mas abort error {rsp_code:X}')
    expected = r"MCE MAS L2CAP disconnected: \w+"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    expected = r"MSE MAS L2CAP disconnected: \w+"
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True


def mas_set_folder(init, rsp, path, rsp_code):
    if path == "/":
        flags = "2"  # BT_MAP_SET_FOLDER_FLAGS_ROOT = 2
        name = ""
    elif path.startswith(".."):
        flags = "3"  # BT_MAP_SET_FOLDER_FLAGS_UP = 3
        if len(path) > 2:
            if path[2] == '/':
                name = path[3:] if len(path) > 3 else ""  # "../folder" case
            else:
                name = None  # Invalid path like "..folder"
        else:
            name = ""  # ".." case
    else:
        flags = "2"  # BT_MAP_SET_FOLDER_FLAGS_DOWN = 2
        if path.startswith("./"):
            name = path[2:] if len(path) > 2 else None  # "./folder" case
        else:
            name = path  # "folder" case

    assert name is not None
    name_len = len(name) * 2 + 2 if len(name) > 0 else 0

    init.exec_command(f'test_map mce mas set_folder {path}')
    expected = [rf"MSE MAS \w+ set_folder req, flags {flags}"]
    expected.append(f"HI {OBEX_HDR.NAME:02x} Len {name_len}")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    cmd = "test_map mse mas set_folder "
    if rsp_code == RspCode.SUCCESS:
        cmd += 'success'
    else:
        cmd += f'error {rsp_code:X}'
    rsp.exec_command(cmd)

    expected = rf"MCE MAS \w+ set_folder rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def test_br_map_mse_rfcomm_connect_success(client, server):
    """
    BR_MAP_MSE_RFCOMM_CONNECT_SUCCESS

    Verify that the MSE can successfully accept an RFCOMM connection from MCE.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_rfcomm_connect_failure(client, server):
    """
    BR_MAP_MSE_RFCOMM_CONNECT_FAILURE

    Verify that the MSE handles RFCOMM connection rejection correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect_fail(server, client)
    br_disconnect(server, client)


def test_br_map_mse_rfcomm_disconnect_success(client, server):
    """
    BR_MAP_MSE_RFCOMM_DISCONNECT_SUCCESS

    Verify that the MSE can successfully disconnect an RFCOMM connection.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mse_mas_rfcomm_disconnect(client, server)
    br_disconnect(server, client)


def test_br_map_mse_rfcomm_disconnect_failure(client, server):
    """
    BR_MAP_MSE_RFCOMM_DISCONNECT_FAILURE

    Verify that the MSE handles RFCOMM disconnection errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)

    # Try to disconnect without establishing connection first
    lines = server.exec_command('test_map mse mas rfcomm_disconnect')
    expected = "No default MSE MAS conn available"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True

    br_disconnect(server, client)


def test_br_map_mse_l2cap_connect_success(client, server):
    """
    BR_MAP_MSE_L2CAP_CONNECT_SUCCESS

    Verify that the MSE can successfully accept an L2CAP connection from MCE.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_l2cap_connect_failure(client, server):
    """
    BR_MAP_MSE_L2CAP_CONNECT_FAILURE

    Verify that the MSE handles L2CAP connection rejection correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect_fail(server, client)
    br_disconnect(server, client)


def test_br_map_mse_l2cap_disconnect_success(client, server):
    """
    BR_MAP_MSE_L2CAP_DISCONNECT_SUCCESS

    Verify that the MSE can successfully disconnect an L2CAP connection.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mse_mas_l2cap_disconnect(client, server)
    br_disconnect(server, client)


def test_br_map_mse_l2cap_disconnect_failure(client, server):
    """
    BR_MAP_MSE_L2CAP_DISCONNECT_FAILURE

    Verify that the MSE handles L2CAP disconnection errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)

    # Try to disconnect without establishing connection first
    lines = server.exec_command('test_map mse mas l2cap_disconnect')
    expected = "No default MSE MAS conn available"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True

    br_disconnect(server, client)


def test_br_map_mse_obex_connect_success(client, server):
    """
    BR_MAP_MSE_OBEX_CONNECT_SUCCESS

    Verify that the MSE can successfully establish an OBEX connection over the transport layer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_obex_connect_failure(client, server):
    """
    BR_MAP_MSE_OBEX_CONNECT_FAILURE

    Verify that the MSE handles OBEX connection rejection correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.FORBIDDEN)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_obex_disconnect_success(client, server):
    """
    BR_MAP_MSE_OBEX_DISCONNECT_SUCCESS

    Verify that the MSE can successfully handle OBEX disconnection.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_obex_disconnect_failure(client, server):
    """
    BR_MAP_MSE_OBEX_DISCONNECT_FAILURE

    Verify that the MSE handles OBEX disconnection errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_disconnect(server, client, RspCode.BAD_REQ)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_ntf_reg_success(client, server):
    """
    BR_MAP_MSE_SET_NTF_REG_SUCCESS

    Verify that the MSE can successfully handle SetNotificationRegistration request.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd(server, client, 'set_ntf_reg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_ntf_reg_failure(client, server):
    """
    BR_MAP_MSE_SET_NTF_REG_FAILURE

    Verify that the MSE handles invalid SetNotificationRegistration request correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_fail(server, client, 'set_ntf_reg', RspCode.NOT_ACCEPT)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_folder_root_success(client, server):
    """
    BR_MAP_MSE_SET_FOLDER_ROOT_SUCCESS

    Verify that the MSE can successfully navigate to root folder.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_set_folder(server, client, "/", RspCode.SUCCESS)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_folder_down_success(client, server):
    """
    BR_MAP_MSE_SET_FOLDER_DOWN_SUCCESS

    Verify that the MSE can successfully navigate down to a subfolder.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_set_folder(server, client, "/", RspCode.SUCCESS)
    mas_set_folder(server, client, "telecom", RspCode.SUCCESS)
    mas_set_folder(server, client, "./msg", RspCode.SUCCESS)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_folder_up_success(client, server):
    """
    BR_MAP_MSE_SET_FOLDER_UP_SUCCESS

    Verify that the MSE can successfully navigate up to the parent folder.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_set_folder(server, client, "/", RspCode.SUCCESS)
    mas_set_folder(server, client, "telecom", RspCode.SUCCESS)
    mas_set_folder(server, client, "msg", RspCode.SUCCESS)
    mas_set_folder(server, client, "..", RspCode.SUCCESS)
    mas_set_folder(server, client, "../telecom", RspCode.SUCCESS)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_folder_failure(client, server):
    """
    BR_MAP_MSE_SET_FOLDER_FAILURE

    Verify that the MSE handles SetFolder errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)

    # Execute SetFolder to root with error response
    mas_set_folder(server, client, "/", RspCode.FORBIDDEN)

    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_final_success(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_FINAL_SUCCESS

    Verify that the MSE can successfully handle GetFolderListing request
    with final bit set in a single response.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd(server, client, 'get_folder_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_non_final_success(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_NON_FINAL_SUCCESS

    Verify that the MSE can successfully handle GetFolderListing request
    with final bit not set (multi-packet transfer).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final(server, client, 'get_folder_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_final_failure(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_FINAL_FAILURE

    Verify that the MSE handles GetFolderListing errors correctly when final bit is set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_fail(server, client, 'get_folder_listing', RspCode.FORBIDDEN)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_non_final_failure(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_NON_FINAL_FAILURE

    Verify that the MSE handles GetFolderListing errors correctly when final bit is not set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final_fail(server, client, 'get_folder_listing', RspCode.FORBIDDEN)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_continue_response(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_CONTINUE_RESPONSE

    Verify that the MSE correctly handles Continue (0x90) response during GetFolderListing.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_continue(server, client, 'get_folder_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_srm_enabled(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_SRM_ENABLED

    Verify that the MSE can successfully handle GetFolderListing using Single Response Mode (SRM).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_srm(server, client, 'get_folder_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_non_final_srm_enabled(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_NON_FINAL_SRM_ENABLED

    Verify that the MSE can successfully handle GetFolderListing request with
    final bit not set using Single Response Mode (SRM) for multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final_srm(server, client, 'get_folder_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_srmp_wait(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_SRMP_WAIT

    Verify that the MSE correctly handles SRMP Wait indication during GetFolderListing.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_srm_param(server, client, 'get_folder_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_abort_during_transfer(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_ABORT_DURING_TRANSFER

    Verify that the MSE can abort GetFolderListing operation during multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_abort(server, client, 'get_folder_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_abort_failure(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_ABORT_FAILURE

    Verify that the MSE handles abort failure during GetFolderListing operation.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_abort_fail(server, client, 'get_folder_listing', RspCode.FORBIDDEN)
    br_disconnect(server, client)


def test_br_map_mse_get_folder_listing_param_all(client, server):
    """
    BR_MAP_MSE_GET_FOLDER_LISTING_PARAM_ALL

    Verify that MSE can send all supported application parameters in GetFolderListing
    response and MCE can correctly parse and match these parameters.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)

    # Navigate to root folder
    mas_set_folder(server, client, "/", RspCode.SUCCESS)

    # Clean up any existing parameters
    client.exec_command('test_map add_header app_param clear')
    server.exec_command('test_map add_header app_param clear')

    # MCE sends GetFolderListing request
    cmd = "get_folder_listing"
    server.exec_command(f'test_map mce mas {cmd}')

    # MSE receives request
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # MSE adds all application parameters for GetFolderListing response
    # FolderListingSize (0x11) = 0x0005 (5 folders)
    client.exec_command('test_map add_header app_param folder_listing_size 5')

    # MSE sends response with all parameters
    client.exec_command(f'test_map mse mas {cmd} noerror')

    # MCE receives response and verifies all parameters
    expected_params = [
        rf"MCE MAS \w+ {cmd} rsp, rsp_code Continue",
        r"T 11 L 2",  # FolderListingSize (tag 0x11)
        r"FolderListingSize: 5",
        r"OBEX BODY: .*",
    ]

    found, _ = server.wait_for_shell_response(expected_params)
    assert found is True, "MCE did not receive all application parameters"

    while True:
        client.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = server.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'

    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_final_success(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_FINAL_SUCCESS

    Verify that the MSE can successfully handle GetMessagesListing request
    with final bit set in a single response.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd(server, client, 'get_msg_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_non_final_success(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_NON_FINAL_SUCCESS

    Verify that the MSE can successfully handle GetMessagesListing request
    with final bit not set (multi-packet transfer).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final(server, client, 'get_msg_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_final_failure(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_FINAL_FAILURE

    Verify that the MSE handles GetMessagesListing errors correctly when final bit is set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_fail(server, client, 'get_msg_listing', RspCode.NOT_FOUND)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_non_final_failure(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_NON_FINAL_FAILURE

    Verify that the MSE handles GetMessagesListing errors correctly when final bit is not set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final_fail(server, client, 'get_msg_listing', RspCode.NOT_FOUND)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_continue_response(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_CONTINUE_RESPONSE

    Verify that the MSE correctly handles Continue (0x90) response during GetMessagesListing.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_continue(server, client, 'get_msg_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_srm_enabled(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_SRM_ENABLED

    Verify that the MSE can successfully handle GetMessagesListing using Single Response Mode (SRM).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_srm(server, client, 'get_msg_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_non_final_srm_enabled(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_NON_FINAL_SRM_ENABLED

    Verify that the MSE can successfully handle GetMessagesListing request with
    final bit not set using Single Response Mode (SRM) for multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final_srm(server, client, 'get_msg_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_srmp_wait(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_SRMP_WAIT

    Verify that the MSE correctly handles SRMP Wait indication during GetMessagesListing.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_srm_param(server, client, 'get_msg_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_abort_during_transfer(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_ABORT_DURING_TRANSFER

    Verify that the MSE can abort GetMessagesListing operation during multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_abort(server, client, 'get_msg_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_abort_failure(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_ABORT_FAILURE

    Verify that the MSE handles abort failure during GetMessagesListing operation.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_abort_fail(server, client, 'get_msg_listing', RspCode.FORBIDDEN)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_listing_param_all(client, server):
    """
    BR_MAP_MSE_GET_MSG_LISTING_PARAM_ALL

    Verify that MSE can send all supported application parameters in GetMessagesListing
    response and MCE can correctly parse and match these parameters.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)

    # Navigate to message folder
    mas_set_folder(server, client, "/", RspCode.SUCCESS)
    mas_set_folder(server, client, "telecom", RspCode.SUCCESS)
    mas_set_folder(server, client, "msg", RspCode.SUCCESS)
    mas_set_folder(server, client, "inbox", RspCode.SUCCESS)

    # Clean up
    client.exec_command('test_map add_header app_param clear')
    server.exec_command('test_map add_header app_param clear')

    # MCE sends GetMessagesListing request
    cmd = "get_msg_listing"
    server.exec_command(f'test_map mce mas {cmd}')

    # MSE receives request
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # MSE adds all application parameters for GetMessagesListing response
    # MessagesListingSize (0x12) = 0x0032 (50 messages)
    client.exec_command('test_map add_header app_param listing_size 50')

    # NewMessage (0x0D) = 0x01 (Yes, new messages available)
    client.exec_command('test_map add_header app_param new_message 1')

    # MSETime (0x19) = "20240115T120000"
    client.exec_command('test_map add_header app_param mse_time 20240115T120000')

    # DatabaseIdentifier (0x1A) = "0123456789ABCDEF0123456789ABCDEF"
    client.exec_command(
        'test_map add_header app_param database_id 0123456789ABCDEF0123456789ABCDEF'
    )

    # FolderVersionCounter (0x23) = "00112233445566778899AABBCCDDEEFF"
    client.exec_command('test_map add_header app_param folder_ver 00112233445566778899AABBCCDDEEFF')

    # MSE sends response with all parameters
    client.exec_command(f'test_map mse mas {cmd} noerror')

    # MCE receives response and verifies all parameters
    expected_params = [
        rf"MCE MAS \w+ {cmd} rsp, rsp_code Continue",
        r"T 12 L 2",  # ListingSize (tag 0x12)
        r"ListingSize: 50",
        r"T 0d L 1",  # NewMessage (tag 0x0D)
        r"NewMessage: ON",
        r"T 19 L 15",  # MSETime (tag 0x19)
        r"MSETime: 20240115T120000",
        r"T 1a L 32",  # DatabaseIdentifier (tag 0x1A)
        r"DatabaseIdentifier: 0123456789ABCDEF0123456789ABCDEF",
        r"T 23 L 32",  # FolderVersionCounter (tag 0x23)
        r"FolderVersionCounter: 00112233445566778899AABBCCDDEEFF",
        r"OBEX BODY: .*",
    ]

    found, _ = server.wait_for_shell_response(expected_params)
    assert found is True, "MCE did not receive all application parameters"

    while True:
        client.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = server.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'

    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_final_success(client, server):
    """
    BR_MAP_MSE_GET_MSG_FINAL_SUCCESS

    Verify that the MSE can successfully handle GetMessage request
    with final bit set in a single response.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd(server, client, 'get_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_non_final_success(client, server):
    """
    BR_MAP_MSE_GET_MSG_NON_FINAL_SUCCESS

    Verify that the MSE can successfully handle GetMessage request
    with final bit not set (multi-packet transfer).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final(server, client, 'get_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_final_failure(client, server):
    """
    BR_MAP_MSE_GET_MSG_FINAL_FAILURE

    Verify that the MSE handles GetMessage errors correctly when final bit is set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_fail(server, client, 'get_msg', RspCode.NOT_FOUND)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_non_final_failure(client, server):
    """
    BR_MAP_MSE_GET_MSG_NON_FINAL_FAILURE

    Verify that the MSE handles GetMessage errors correctly when final bit is not set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final_fail(server, client, 'get_msg', RspCode.NOT_FOUND)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_continue_response(client, server):
    """
    BR_MAP_MSE_GET_MSG_CONTINUE_RESPONSE

    Verify that the MSE correctly handles Continue (0x90) response during GetMessage.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_continue(server, client, 'get_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_srm_enabled(client, server):
    """
    BR_MAP_MSE_GET_MSG_SRM_ENABLED

    Verify that the MSE can successfully handle GetMessage using Single Response Mode (SRM).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_srm(server, client, 'get_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_non_final_srm_enabled(client, server):
    """
    BR_MAP_MSE_GET_MSG_NON_FINAL_SRM_ENABLED

    Verify that the MSE can successfully handle GetMessage request with
    final bit not set using Single Response Mode (SRM) for multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final_srm(server, client, 'get_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_srmp_wait(client, server):
    """
    BR_MAP_MSE_GET_MSG_SRMP_WAIT

    Verify that the MSE correctly handles SRMP Wait indication during GetMessage.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_srm_param(server, client, 'get_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_abort_during_transfer(client, server):
    """
    BR_MAP_MSE_GET_MSG_ABORT_DURING_TRANSFER

    Verify that the MSE can abort GetMessage operation during multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_abort(server, client, 'get_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_abort_failure(client, server):
    """
    BR_MAP_MSE_GET_MSG_ABORT_FAILURE

    Verify that the MSE handles abort failure during GetMessage operation.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_abort_fail(server, client, 'get_msg', RspCode.FORBIDDEN)
    br_disconnect(server, client)


def test_br_map_mse_get_msg_param_all(client, server):
    """
    BR_MAP_MSE_GET_MSG_PARAM_ALL

    Verify that MSE can send all supported application parameters in GetMessage
    response and MCE can correctly parse and match these parameters.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)

    # Navigate to message folder
    mas_set_folder(server, client, "/", RspCode.SUCCESS)
    mas_set_folder(server, client, "telecom", RspCode.SUCCESS)
    mas_set_folder(server, client, "msg", RspCode.SUCCESS)
    mas_set_folder(server, client, "inbox", RspCode.SUCCESS)

    # Clean up any existing parameters
    client.exec_command('test_map add_header app_param clear')
    server.exec_command('test_map add_header app_param clear')

    # MCE sends GetMessage request
    cmd = "get_msg"
    server.exec_command(f'test_map mce mas {cmd}')

    # MSE receives request
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # MSE adds all application parameters for GetMessage response
    # FractionDeliver (0x16) = 0x01 (Last fraction)
    client.exec_command('test_map add_header app_param fraction_deliver 1')

    # MSE sends response with all parameters
    client.exec_command(f'test_map mse mas {cmd} noerror')

    # MCE receives response and verifies all parameters
    expected_params = [
        rf"MCE MAS \w+ {cmd} rsp, rsp_code Continue",
        r"T 16 L 1",  # FractionDeliver (tag 0x16)
        r"FractionDeliver: Last",
        r"OBEX BODY: .*",
    ]

    found, _ = server.wait_for_shell_response(expected_params)
    assert found is True, "MCE did not receive all application parameters"

    while True:
        client.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = server.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'

    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_msg_status_success(client, server):
    """
    BR_MAP_MSE_SET_MSG_STATUS_SUCCESS

    Verify that the MSE can successfully handle SetMessageStatus request (read/deleted).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd(server, client, 'set_msg_status')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_msg_status_failure(client, server):
    """
    BR_MAP_MSE_SET_MSG_STATUS_FAILURE

    Verify that the MSE handles SetMessageStatus errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_fail(server, client, 'set_msg_status', RspCode.NOT_FOUND)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_push_msg_success(client, server):
    """
    BR_MAP_MSE_PUSH_MSG_SUCCESS

    Verify that the MSE can successfully handle PushMessage request
    with final bit set in a single request.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd(server, client, 'push_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_push_msg_failure(client, server):
    """
    BR_MAP_MSE_PUSH_MSG_FAILURE

    Verify that the MSE handles PushMessage errors correctly when final bit is not set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_fail(server, client, 'push_msg', RspCode.FORBIDDEN)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_push_msg_continue_response(client, server):
    """
    BR_MAP_MSE_PUSH_MSG_CONTINUE_RESPONSE

    Verify that the MSE correctly handles Continue (0x90) response during PushMessage.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_continue(server, client, 'push_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_push_msg_srm_enabled(client, server):
    """
    BR_MAP_MSE_PUSH_MSG_SRM_ENABLED

    Verify that the MSE can successfully handle PushMessage using Single Response Mode (SRM).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_srm(server, client, 'push_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_push_msg_srmp_wait(client, server):
    """
    BR_MAP_MSE_PUSH_MSG_SRMP_WAIT

    Verify that the MSE correctly handles SRMP Wait indication during PushMessage.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_srm_param(server, client, 'push_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_push_msg_abort_during_transfer(client, server):
    """
    BR_MAP_MSE_PUSH_MSG_ABORT_DURING_TRANSFER

    Verify that the MSE can abort PushMessage operation during multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_abort(server, client, 'push_msg')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_push_msg_abort_failure(client, server):
    """
    BR_MAP_MSE_PUSH_MSG_ABORT_FAILURE

    Verify that the MSE handles abort failure during PushMessage operation.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_abort_fail(server, client, 'push_msg', RspCode.FORBIDDEN)
    br_disconnect(server, client)


def test_br_map_mse_update_inbox_success(client, server):
    """
    BR_MAP_MSE_UPDATE_INBOX_SUCCESS

    Verify that the MSE can successfully handle UpdateInbox request.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd(server, client, 'update_inbox')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_update_inbox_failure(client, server):
    """
    BR_MAP_MSE_UPDATE_INBOX_FAILURE

    Verify that the MSE handles UpdateInbox errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_fail(server, client, 'update_inbox', RspCode.NOT_ACCEPT)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_mas_inst_info_success(client, server):
    """
    BR_MAP_MSE_GET_MAS_INST_INFO_SUCCESS

    Verify that the MSE can successfully handle GetMASInstanceInformation request.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd(server, client, 'get_mas_inst_info')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_mas_inst_info_failure(client, server):
    """
    BR_MAP_MSE_GET_MAS_INST_INFO_FAILURE

    Verify that the MSE handles GetMASInstanceInformation errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_fail(server, client, 'get_mas_inst_info', RspCode.NOT_FOUND)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_mas_inst_info_param_all(client, server):
    """
    BR_MAP_MSE_GET_MAS_INST_INFO_PARAM_ALL

    Verify that MSE can send all supported application parameters in GetMASInstanceInformation
    response and MCE can correctly parse and match these parameters.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)

    # Clean up any existing parameters
    client.exec_command('test_map add_header app_param clear')
    server.exec_command('test_map add_header app_param clear')

    # MCE sends GetMASInstanceInformation request
    cmd = "get_mas_inst_info"
    server.exec_command(f'test_map mce mas {cmd}')

    # MSE receives request
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # MSE adds all application parameters for GetMASInstanceInformation response
    # OwnerUCI (0x27) = "+1234567890"
    client.exec_command('test_map add_header app_param owner_uci +1234567890')

    # MSE sends response with all parameters
    client.exec_command(f'test_map mse mas {cmd} noerror')

    # MCE receives response and verifies all parameters
    expected_params = [
        rf"MCE MAS \w+ {cmd} rsp, rsp_code Success",
        r"T 27 L 11",  # OwnerUCI (tag 0x27)
        r"OwnerUCI: \+1234567890",
        r"OBEX BODY: .*",
    ]

    found, _ = server.wait_for_shell_response(expected_params)
    assert found is True, "MCE did not receive all application parameters"

    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_owner_status_success(client, server):
    """
    BR_MAP_MSE_SET_OWNER_STATUS_SUCCESS

    Verify that the MSE can successfully handle SetOwnerStatus request (presence and chat state).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd(server, client, 'set_owner_status')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_owner_status_failure(client, server):
    """
    BR_MAP_MSE_SET_OWNER_STATUS_FAILURE

    Verify that the MSE handles SetOwnerStatus errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_fail(server, client, 'set_owner_status', RspCode.BAD_REQ)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_owner_status_success(client, server):
    """
    BR_MAP_MSE_GET_OWNER_STATUS_SUCCESS

    Verify that the MSE can successfully handle GetOwnerStatus request.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd(server, client, 'get_owner_status')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_owner_status_failure(client, server):
    """
    BR_MAP_MSE_GET_OWNER_STATUS_FAILURE

    Verify that the MSE handles GetOwnerStatus errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_fail(server, client, 'get_owner_status', RspCode.BAD_REQ)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_owner_status_param_all(client, server):
    """
    BR_MAP_MSE_GET_OWNER_STATUS_PARAM_ALL

    Verify that MSE can send all supported application parameters in GetOwnerStatus
    response and MCE can correctly parse and match these parameters.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)

    # Clean up any existing parameters
    client.exec_command('test_map add_header app_param clear')
    server.exec_command('test_map add_header app_param clear')

    cmd = "get_owner_status"
    server.exec_command(f'test_map mce mas {cmd}')

    # MSE receives request
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # MSE adds all application parameters for GetOwnerStatus response
    # PresenceAvailability (0x1C) = 0x02 (Online)
    client.exec_command('test_map add_header app_param presence_availability 2')

    # PresenceText (0x1D) = "Available for chat"
    client.exec_command('test_map add_header app_param presence_text "Available for chat"')

    # LastActivity (0x1E) = "20240115T120000"
    client.exec_command('test_map add_header app_param last_activity 20240115T120000')

    # ChatState (0x21) = 0x02 (Active)
    client.exec_command('test_map add_header app_param chat_state 2')

    # MSE sends response with all parameters
    client.exec_command(f'test_map mse mas {cmd} noerror')

    # MCE receives response and verifies all parameters
    expected_params = [
        rf"MCE MAS \w+ {cmd} rsp, rsp_code Success",
        r"T 1c L 1",  # PresenceAvailability (tag 0x1C)
        r"PresenceAvailability: 2 \(Online\)",
        r"T 1d L 18",  # PresenceText (tag 0x1D)
        r"PresenceText: Available for chat",
        r"T 1e L 15",  # LastActivity (tag 0x1E)
        r"LastActivity: 20240115T120000",
        r"T 21 L 1",  # ChatState (tag 0x21)
        r"ChatState: 2 \(Active\)",
        r"OBEX BODY: .*",
    ]

    found, _ = server.wait_for_shell_response(expected_params)
    assert found is True, "MCE did not receive all application parameters"

    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_final_success(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_FINAL_SUCCESS

    Verify that the MSE can successfully handle GetConversationListing request
    with final bit set in a single response.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd(server, client, 'get_convo_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_non_final_success(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_NON_FINAL_SUCCESS

    Verify that the MSE can successfully handle GetConversationListing request
    with final bit not set (multi-packet transfer).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final(server, client, 'get_convo_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_final_failure(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_FINAL_FAILURE

    Verify that the MSE handles GetConversationListing errors correctly when final bit is set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_fail(server, client, 'get_convo_listing', RspCode.BAD_REQ)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_non_final_failure(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_NON_FINAL_FAILURE

    Verify that the MSE handles GetConversationListing errors correctly when final bit is not set.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final_fail(server, client, 'get_convo_listing', RspCode.BAD_REQ)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_continue_response(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_CONTINUE_RESPONSE

    Verify that the MSE correctly handles Continue (0x90) response during GetConversationListing.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_rfcomm_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_continue(server, client, 'get_convo_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_srm_enabled(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_SRM_ENABLED

    Verify that the MSE can successfully handle GetConversationListing
    using Single Response Mode (SRM).
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_srm(server, client, 'get_convo_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_non_final_srm_enabled(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_NON_FINAL_SRM_ENABLED

    Verify that the MSE can successfully handle GetConversationListing request with
    final bit not set using Single Response Mode (SRM) for multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_final_srm(server, client, 'get_convo_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_srmp_wait(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_SRMP_WAIT

    Verify that the MSE correctly handles SRMP Wait indication during GetConversationListing.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_srm_param(server, client, 'get_convo_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_abort_during_transfer(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_ABORT_DURING_TRANSFER

    Verify that the MSE can abort GetConversationListing operation during multi-packet transfer.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_abort(server, client, 'get_convo_listing')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_abort_failure(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_ABORT_FAILURE

    Verify that the MSE handles abort failure during GetConversationListing operation.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_get_cmd_abort_fail(server, client, 'get_convo_listing', RspCode.FORBIDDEN)
    br_disconnect(server, client)


def test_br_map_mse_get_convo_listing_param_all(client, server):
    """
    BR_MAP_MSE_GET_CONVO_LISTING_PARAM_ALL

    Verify that MSE can send all supported application parameters in GetConversationListing
    response and MCE can correctly parse and match these parameters.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)

    # Clean up
    client.exec_command('test_map add_header app_param clear')
    server.exec_command('test_map add_header app_param clear')

    # MCE sends GetConversationListing request
    cmd = "get_convo_listing"
    server.exec_command(f'test_map mce mas {cmd}')

    # MSE receives request
    expected = rf"MSE MAS \w+ {cmd} req, final true"
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # MSE adds all application parameters for GetConversationListing response
    # ListingSize (0x12) = 0x000A (10 conversations)
    client.exec_command('test_map add_header app_param listing_size 10')

    # MSETime (0x19) = "20240115T120000"
    client.exec_command('test_map add_header app_param mse_time 20240115T120000')

    # DatabaseIdentifier (0x1A) = "0123456789ABCDEF0123456789ABCDEF"
    client.exec_command(
        'test_map add_header app_param database_id 0123456789ABCDEF0123456789ABCDEF'
    )

    # ConversationListingVersionCounter (0x1B) = "FFEEDDCCBBAA99887766554433221100"
    client.exec_command(
        'test_map add_header app_param convo_listing_ver FFEEDDCCBBAA99887766554433221100'
    )

    # MSE sends response with all parameters
    client.exec_command(f'test_map mse mas {cmd} noerror')

    # MCE receives response and verifies all parameters
    expected_params = [
        rf"MCE MAS \w+ {cmd} rsp, rsp_code Continue",
        r"T 12 L 2",  # ListingSize (tag 0x12)
        r"ListingSize: 10",
        r"T 19 L 15",  # MSETime (tag 0x19)
        r"MSETime: 20240115T120000",
        r"T 1a L 32",  # DatabaseIdentifier (tag 0x1A)
        r"DatabaseIdentifier: 0123456789ABCDEF0123456789ABCDEF",
        r"T 1b L 32",  # ConvoListingVersionCounter (tag 0x1B)
        r"ConvoListingVersionCounter: FFEEDDCCBBAA99887766554433221100",
        r"OBEX BODY: .*",
    ]

    found, _ = server.wait_for_shell_response(expected_params)
    assert found is True, "MCE did not receive all application parameters"

    while True:
        client.exec_command(f'test_map mse mas {cmd} noerror')
        expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
        expected.append(r"OBEX BODY: .*")
        found, lines = server.wait_for_shell_response(expected)
        assert found is True

        expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'

    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_ntf_filter_success(client, server):
    """
    BR_MAP_MSE_SET_NTF_FILTER_SUCCESS

    Verify that the MSE can successfully handle SetNotificationFilter request.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd(server, client, 'set_ntf_filter')
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_map_mse_set_ntf_filter_failure(client, server):
    """
    BR_MAP_MSE_SET_NTF_FILTER_FAILURE

    Verify that the MSE handles SetNotificationFilter errors correctly.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    mas_l2cap_connect(server, client)
    mas_connect(server, client, RspCode.SUCCESS)
    mas_put_cmd_fail(server, client, 'set_ntf_filter', RspCode.BAD_REQ)
    mas_disconnect(server, client, RspCode.SUCCESS)
    mas_l2cap_disconnect(server, client)
    br_disconnect(server, client)
