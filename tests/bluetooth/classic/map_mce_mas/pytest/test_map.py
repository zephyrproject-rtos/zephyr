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


# def test_br_map_mce_l2cap_connect_success(client, server):
#     """Verify that the MCE can successfully establish an L2CAP connection to the MSE."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_l2cap_connect_failure(client, server):
#     """Verify that the MCE handles L2CAP connection failure correctly."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect_fail(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_l2cap_disconnect_success(client, server):
#     """Verify that the MCE can successfully disconnect an L2CAP connection."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_l2cap_disconnect_failure(client, server):
#     """Verify that the MCE handles L2CAP disconnection errors correctly."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)

#     # Try to disconnect without establishing connection first
#     lines = client.exec_command('test_map mce mas l2cap_disconnect')
#     expected = "No default MCE MAS conn available"
#     found = client.check_shell_response(lines, expected)
#     if found is False:
#         found, _ = client.wait_for_shell_response(expected)
#         assert found is True

#     br_disconnect(client, server)


# def test_br_map_mce_rfcomm_connect_success(client, server):
#     """Verify that the MCE can successfully establish an RFCOMM connection to the MSE."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_rfcomm_connect_failure(client, server):
#     """Verify that the MCE handles RFCOMM connection failure correctly."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect_fail(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_rfcomm_disconnect_success(client, server):
#     """Verify that the MCE can successfully disconnect an RFCOMM connection."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_rfcomm_disconnect_failure(client, server):
#     """Verify that the MCE handles RFCOMM disconnection errors correctly."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)

#     # Try to disconnect without establishing connection first
#     lines = client.exec_command('test_map mce mas rfcomm_disconnect')
#     expected = "No default MCE MAS conn available"
#     found = client.check_shell_response(lines, expected)
#     if found is False:
#         found, _ = client.wait_for_shell_response(expected)
#         assert found is True

#     br_disconnect(client, server)


# def test_br_map_mce_obex_connect_success(client, server):
#     """Verify that the MCE can successfully establish an OBEX connection
#     over the transport layer."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_obex_connect_failure(client, server):
#     """Verify that the MCE handles OBEX connection failure correctly."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.FORBIDDEN)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_obex_disconnect_success(client, server):
#     """Verify that the MCE can successfully disconnect an OBEX connection."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_obex_disconnect_failure(client, server):
#     """Verify that the MCE handles OBEX disconnection errors correctly."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_disconnect(client, server, RspCode.BAD_REQ)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_ntf_reg_success(client, server):
#     """Verify that the MCE can successfully register for notifications."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd(client, server, 'set_ntf_reg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_ntf_reg_failure(client, server):
#     """Verify that the MCE handles SetNotificationRegistration errors correctly."""
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_fail(client, server, 'set_ntf_reg', RspCode.NOT_ACCEPT)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_ntf_reg_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in SetNotificationRegistration
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for SetNotificationRegistration
#     # NotificationStatus (0x0E) = 1 (Notification ON)
#     client.exec_command('test_map add_header app_param notification_status 1')

#     # Send SetNotificationRegistration request
#     cmd = "set_ntf_reg"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final \w+",
#         r"T 0e L 1",  # NotificationStatus (tag 0x0E)
#         r"NotificationStatus: ON",
#         r"OBEX BODY: .*",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # MSE sends response
#     server.exec_command(f'test_map mse mas {cmd} noerror')
#     expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code Success"
#     found, _ = client.wait_for_shell_response(expected)
#     assert found is True

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_folder_root_success(client, server):
#     """
#     Verify that the MCE can successfully navigate to the root folder.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_set_folder(client, server, "/", RspCode.SUCCESS)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_folder_down_success(client, server):
#     """
#     Verify that the MCE can successfully navigate down to a subfolder.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_set_folder(client, server, "/", RspCode.SUCCESS)
#     mas_set_folder(client, server, "telecom", RspCode.SUCCESS)
#     mas_set_folder(client, server, "./msg", RspCode.SUCCESS)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_folder_up_success(client, server):
#     """
#     Verify that the MCE can successfully navigate up to the parent folder.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_set_folder(client, server, "/", RspCode.SUCCESS)
#     mas_set_folder(client, server, "telecom", RspCode.SUCCESS)
#     mas_set_folder(client, server, "msg", RspCode.SUCCESS)
#     mas_set_folder(client, server, "..", RspCode.SUCCESS)
#     mas_set_folder(client, server, "../telecom", RspCode.SUCCESS)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_folder_failure(client, server):
#     """
#     Verify that the MCE handles SetFolder to root errors correctly.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Execute SetFolder to root with error response
#     mas_set_folder(client, server, "/", RspCode.FORBIDDEN)

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_final_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve folder listing
#     with final bit set in a single response.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd(client, server, 'get_folder_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_non_final_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve folder listing
#     with final bit not set (multi-packet transfer).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final(client, server, 'get_folder_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_final_failure(client, server):
#     """
#     Verify that the MCE handles GetFolderListing errors correctly when final bit is set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_fail(client, server, 'get_folder_listing', RspCode.FORBIDDEN)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_non_final_failure(client, server):
#     """
#     Verify that the MCE handles GetFolderListing errors correctly when final bit is not set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final_fail(client, server, 'get_folder_listing', RspCode.FORBIDDEN)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_continue_response(client, server):
#     """
#     Verify that the MCE correctly handles Continue (0x90) response during GetFolderListing.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_continue(client, server, 'get_folder_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_srm_enabled(client, server):
#     """
#     Verify that the MCE can successfully retrieve folder listing using Single Response Mode (SRM).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_srm(client, server, 'get_folder_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_non_final_srm_enabled(client, server):
#     """
#     Verify that the MCE can successfully retrieve folder listing using
#     Single Response Mode (SRM) with final bit not set for multi-packet transfer.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final_srm(client, server, 'get_folder_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_srmp_wait(client, server):
#     """
#     Verify that the MCE correctly handles SRMP Wait indication during GetFolderListing.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_srm_param(client, server, 'get_folder_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_abort_during_transfer(client, server):
#     """
#     Verify that the MCE can abort GetFolderListing operation during multi-packet transfer.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_abort(client, server, 'get_folder_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_abort_failure(client, server):
#     """
#     Verify that the MCE handles abort failure during GetFolderListing operation.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_abort_fail(client, server, 'get_folder_listing', RspCode.FORBIDDEN)
#     br_disconnect(client, server)


# def test_br_map_mce_get_folder_listing_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in GetFolderListing
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Navigate to root folder
#     mas_set_folder(client, server, "/", RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for GetFolderListing
#     # MaxListCount (0x01) = 0x000A (10 entries)
#     client.exec_command('test_map add_header app_param max_list_count 10')

#     # ListStartOffset (0x02) = 0x0005 (start from 6th entry)
#     client.exec_command('test_map add_header app_param list_start_offset 5')

#     # Send GetFolderListing request
#     cmd = "get_folder_listing"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final true",
#         r"T 01 L 2",  # MaxListCount
#         r"MaxListCount: 10",
#         r"T 02 L 2",  # ListStartOffset
#         r"ListStartOffset: 5",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # Wait for MCE to receive response
#     while True:
#         server.exec_command(f'test_map mse mas {cmd} noerror')
#         expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
#         expected.append(r"OBEX BODY: .*")
#         found, lines = client.wait_for_shell_response(expected)
#         assert found is True

#         expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
#         for line in lines:
#             searched = re.search(expected, line)
#             if searched is not None:
#                 rsp_code = searched.group("rsp_code")
#                 break

#         if rsp_code != 'Continue':
#             break

#     assert rsp_code == 'Success'

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_final_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve messages listing
#     with final bit set in a single response.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd(client, server, 'get_msg_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_non_final_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve messages listing
#     with final bit not set (multi-packet transfer).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final(client, server, 'get_msg_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_final_failure(client, server):
#     """
#     Verify that the MCE handles GetMessagesListing errors correctly when final bit is set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_fail(client, server, 'get_msg_listing', RspCode.NOT_FOUND)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_non_final_failure(client, server):
#     """
#     Verify that the MCE handles GetMessagesListing errors correctly when final bit is not set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final_fail(client, server, 'get_msg_listing', RspCode.NOT_FOUND)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_continue_response(client, server):
#     """
#     Verify that the MCE correctly handles Continue (0x90) response during GetMessagesListing.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_continue(client, server, 'get_msg_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_srm_enabled(client, server):
#     """
#     Verify that the MCE can successfully retrieve messages listing using
#     Single Response Mode (SRM).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_srm(client, server, 'get_msg_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_non_final_srm_enabled(client, server):
#     """
#     Verify that the MCE can successfully retrieve messages listing using
#     Single Response Mode (SRM) with final bit not set for multi-packet.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final_srm(client, server, 'get_msg_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_srmp_wait(client, server):
#     """
#     Verify that the MCE correctly handles SRMP Wait indication during GetMessagesListing.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_srm_param(client, server, 'get_msg_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_abort_during_transfer(client, server):
#     """
#     Verify that the MCE can abort GetMessagesListing operation during multi-packet transfer.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_abort(client, server, 'get_msg_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_abort_failure(client, server):
#     """
#     Verify that the MCE handles abort failure during GetMessagesListing operation.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_abort_fail(client, server, 'get_msg_listing', RspCode.FORBIDDEN)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_listing_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in GetMessagesListing
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Navigate to message folder
#     mas_set_folder(client, server, "/", RspCode.SUCCESS)
#     mas_set_folder(client, server, "telecom", RspCode.SUCCESS)
#     mas_set_folder(client, server, "msg", RspCode.SUCCESS)
#     mas_set_folder(client, server, "inbox", RspCode.SUCCESS)

#     # Clean up
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for GetMessagesListing
#     # MaxListCount (0x01) = 0x000A (10 entries)
#     client.exec_command('test_map add_header app_param max_list_count 10')

#     # ListStartOffset (0x02) = 0x0005 (start from 6th entry)
#     client.exec_command('test_map add_header app_param list_start_offset 5')

#     # FilterMessageType (0x03) = 0x0F (all message types)
#     client.exec_command('test_map add_header app_param filter_msg_type 0F')

#     # FilterPeriodBegin (0x04) = "20240101T000000"
#     client.exec_command('test_map add_header app_param filter_period_begin 20240101T000000')

#     # FilterPeriodEnd (0x05) = "20241231T235959"
#     client.exec_command('test_map add_header app_param filter_period_end 20241231T235959')

#     # FilterReadStatus (0x06) = 0x02 (unread only)
#     client.exec_command('test_map add_header app_param filter_read_status 2')

#     # FilterRecipient (0x07) = "recipient@example.com"
#     client.exec_command('test_map add_header app_param filter_recipient recipient@example.com')

#     # FilterOriginator (0x08) = "sender@example.com"
#     client.exec_command('test_map add_header app_param filter_originator sender@example.com')

#     # FilterPriority (0x09) = 0x01 (high priority)
#     client.exec_command('test_map add_header app_param filter_priority 1')

#     # SubjectLength (0x13) = 0x00FF (255 characters)
#     client.exec_command('test_map add_header app_param subject_length 255')

#     # ParameterMask (0x10) = 0x00FFFFFFFF (all parameters)
#     client.exec_command('test_map add_header app_param parameter_mask FFFFFFFF')

#     # ConversationID (0x22) = 128-bit value in hex string format
#     client.exec_command(
#         'test_map add_header app_param conversation_id 00112233445566778899AABBCCDDEEFF'
#     )

#     # FilterMessageHandle (0x24) = 64-bit value in hex string format
#     client.exec_command('test_map add_header app_param filter_msg_handle 0000000000000001')

#     # Send GetMessagesListing request
#     cmd = "get_msg_listing"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final true",
#         r"T 01 L 2",  # MaxListCount
#         r"MaxListCount: 10",
#         r"T 02 L 2",  # ListStartOffset
#         r"ListStartOffset: 5",
#         r"T 03 L 1",  # FilterMessageType
#         r"FilterMessageType: 0x0f",
#         r"SMS_GSM=Y, SMS_CDMA=Y, EMAIL=Y, MMS=Y, IM=N",
#         r"T 04 L 15",  # FilterPeriodBegin (YYYYMMDDTHHmmss = 15 chars)
#         r"FilterPeriodBegin: 20240101T000000",
#         r"T 05 L 15",  # FilterPeriodEnd
#         r"FilterPeriodEnd: 20241231T235959",
#         r"T 06 L 1",  # FilterReadStatus
#         r"FilterReadStatus: 2 \(read_only\)",
#         r"T 07 L 21",  # FilterRecipient (length of "recipient@example.com")
#         r"FilterRecipient: recipient@example.com",
#         r"T 08 L 18",  # FilterOriginator (length of "sender@example.com")
#         r"FilterOriginator: sender@example.com",
#         r"T 09 L 1",  # FilterPriority
#         r"FilterPriority: 1 \(high_priority\)",
#         r"T 13 L 1",  # SubjectLength
#         r"SubjectLength: 255",
#         r"T 10 L 4",  # ParameterMask
#         r"ParameterMask: 0xffffffff",
#         r"T 22 L 32",  # ConversationID
#         r"ConversationID: 00112233445566778899AABBCCDDEEFF",
#         r"T 24 L 16",  # FilterMsgHandle
#         r"FilterMsgHandle: 0000000000000001",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # Wait for MCE to receive response
#     while True:
#         server.exec_command(f'test_map mse mas {cmd} noerror')
#         expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
#         expected.append(r"OBEX BODY: .*")
#         found, lines = client.wait_for_shell_response(expected)
#         assert found is True

#         expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
#         for line in lines:
#             searched = re.search(expected, line)
#             if searched is not None:
#                 rsp_code = searched.group("rsp_code")
#                 break

#         if rsp_code != 'Continue':
#             break

#     assert rsp_code == 'Success'

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_final_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve a message with final bit set in a
#     single response.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd(client, server, 'get_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_non_final_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve a message
#     with final bit not set (multi-packet transfer).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final(client, server, 'get_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_final_failure(client, server):
#     """
#     Verify that the MCE handles GetMessage errors correctly when final bit is set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_fail(client, server, 'get_msg', RspCode.NOT_FOUND)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_non_final_failure(client, server):
#     """
#     Verify that the MCE handles GetMessage errors correctly when final bit is not set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final_fail(client, server, 'get_msg', RspCode.NOT_FOUND)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_continue_response(client, server):
#     """
#     Verify that the MCE correctly handles Continue (0x90) response during GetMessage.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_continue(client, server, 'get_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_srm_enabled(client, server):
#     """
#     Verify that the MCE can successfully retrieve a message using Single Response Mode (SRM).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_srm(client, server, 'get_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_non_final_srm_enabled(client, server):
#     """
#     Verify that the MCE can successfully retrieve a message using
#     Single Response Mode (SRM) with final bit not set for multi-packet transfer.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final_srm(client, server, 'get_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_srmp_wait(client, server):
#     """
#     Verify that the MCE correctly handles SRMP Wait indication during GetMessage.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_srm_param(client, server, 'get_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_abort_during_transfer(client, server):
#     """
#     Verify that the MCE can abort GetMessage operation during multi-packet transfer.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_abort(client, server, 'get_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_abort_failure(client, server):
#     """
#     Verify that the MCE handles abort failure during GetMessage operation.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_abort_fail(client, server, 'get_msg', RspCode.FORBIDDEN)
#     br_disconnect(client, server)


# def test_br_map_mce_get_msg_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in GetMessage
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Navigate to message folder (optional, but good practice)
#     mas_set_folder(client, server, "/", RspCode.SUCCESS)
#     mas_set_folder(client, server, "telecom", RspCode.SUCCESS)
#     mas_set_folder(client, server, "msg", RspCode.SUCCESS)
#     mas_set_folder(client, server, "inbox", RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for GetMessage
#     # Attachment (0x0A) = 0x01 (include attachments)
#     client.exec_command('test_map add_header app_param attachment 1')

#     # Charset (0x14) = 0x01 (UTF-8)
#     client.exec_command('test_map add_header app_param charset 1')

#     # FractionRequest (0x15) = 0x01 (next fraction)
#     client.exec_command('test_map add_header app_param fraction_request 1')

#     # Send GetMessage request
#     cmd = "get_msg"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final true",
#         r"T 0a L 1",  # Attachment (tag 0x0A)
#         r"Attachment: ON",
#         r"T 14 L 1",  # Charset (tag 0x14)
#         r"Charset: UTF-8",
#         r"T 15 L 1",  # FractionRequest (tag 0x15)
#         r"FractionRequest: Next",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # Wait for MCE to receive response
#     while True:
#         server.exec_command(f'test_map mse mas {cmd} noerror')
#         expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
#         expected.append(r"OBEX BODY: .*")
#         found, lines = client.wait_for_shell_response(expected)
#         assert found is True

#         expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
#         for line in lines:
#             searched = re.search(expected, line)
#             if searched is not None:
#                 rsp_code = searched.group("rsp_code")
#                 break

#         if rsp_code != 'Continue':
#             break

#     assert rsp_code == 'Success'

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_msg_status_success(client, server):
#     """
#     Verify that the MCE can successfully set message status (read/deleted).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd(client, server, 'set_msg_status')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_msg_status_failure(client, server):
#     """
#     Verify that the MCE handles SetMessageStatus errors correctly.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_fail(client, server, 'set_msg_status', RspCode.NOT_FOUND)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_msg_status_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in SetMessageStatus
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Navigate to message folder (optional, but good practice)
#     mas_set_folder(client, server, "/", RspCode.SUCCESS)
#     mas_set_folder(client, server, "telecom", RspCode.SUCCESS)
#     mas_set_folder(client, server, "msg", RspCode.SUCCESS)
#     mas_set_folder(client, server, "inbox", RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for SetMessageStatus
#     # StatusIndicator (0x17) = 0x00 (read status)
#     client.exec_command('test_map add_header app_param status_indicator 0')

#     # StatusValue (0x18) = 0x01 (read)
#     client.exec_command('test_map add_header app_param status_value 1')

#     # ExtendedData (0x28) = "0:18;2:486;3:11;"
#     client.exec_command('test_map add_header app_param extended_data 0:18;2:486;3:11;')

#     # Send SetMessageStatus request
#     cmd = "set_msg_status"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final true",
#         r"T 17 L 1",  # StatusIndicator (tag 0x17)
#         r"StatusIndicator: 0 \(ReadStatus\)",
#         r"T 18 L 1",  # StatusValue (tag 0x18)
#         r"StatusValue: Yes",
#         r"T 28 L 16",  # ExtendedData (tag 0x28, length of "0:18;2:486;3:11;")
#         r"ExtendedData: 0:18;2:486;3:11;",
#         r"OBEX BODY: .*",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params, timeout=10)
#     assert found is True, "MSE did not receive all application parameters"

#     server.exec_command(f'test_map mse mas {cmd} noerror')
#     expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code Success"
#     found, _ = client.wait_for_shell_response(expected)
#     assert found is True

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_push_msg_success(client, server):
#     """
#     Verify that the MCE can successfully push a message with final bit set in a single request.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd(client, server, 'push_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_push_msg_failure(client, server):
#     """
#     Verify that the MCE handles PushMessage errors correctly when final bit is not set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_fail(client, server, 'push_msg', RspCode.FORBIDDEN)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_push_msg_continue_response(client, server):
#     """
#     Verify that the MCE correctly handles Continue (0x90) response during PushMessage.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_continue(client, server, 'push_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_push_msg_srm_enabled(client, server):
#     """
#     Verify that the MCE can successfully push a message using Single Response Mode (SRM).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_srm(client, server, 'push_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_push_msg_srmp_wait(client, server):
#     """
#     Verify that the MCE correctly handles SRMP Wait indication during PushMessage.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_srm_param(client, server, 'push_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_push_msg_abort_during_transfer(client, server):
#     """
#     Verify that the MCE can abort PushMessage operation during multi-packet transfer.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_abort(client, server, 'push_msg')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_push_msg_abort_failure(client, server):
#     """
#     Verify that the MCE handles abort failure during PushMessage operation.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_abort_fail(client, server, 'push_msg', RspCode.FORBIDDEN)
#     br_disconnect(client, server)


# def test_br_map_mce_push_msg_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in PushMessage
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Navigate to target folder (e.g., "telecom/msg/outbox")
#     mas_set_folder(client, server, "/", RspCode.SUCCESS)
#     mas_set_folder(client, server, "telecom", RspCode.SUCCESS)
#     mas_set_folder(client, server, "msg", RspCode.SUCCESS)
#     mas_set_folder(client, server, "outbox", RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for PushMessage
#     # Transparent (0x0B) = 0x01 (transparent mode)
#     client.exec_command('test_map add_header app_param transparent 1')

#     # Retry (0x0C) = 0x01 (retry enabled)
#     client.exec_command('test_map add_header app_param retry 1')

#     # Charset (0x14) = 0x01 (UTF-8)
#     client.exec_command('test_map add_header app_param charset 1')

#     # Attachment (0x0A) = 0x01 (include attachments)
#     client.exec_command('test_map add_header app_param attachment 1')

#     # ConversationID (0x22) = 16 bytes conversation ID (32 hex chars)
#     client.exec_command(
#         'test_map add_header app_param conversation_id 00112233445566778899AABBCCDDEEFF'
#     )

#     # MessageHandle (0x2A) = 16 bytes message handle (16 hex chars)
#     client.exec_command('test_map add_header app_param message_handle 0000000000000001')

#     # ModifyText (0x2B) = 0x01 (modify text allowed/prepend)
#     client.exec_command('test_map add_header app_param modify_text 1')

#     # Send PushMessage request
#     cmd = "push_msg"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final false",
#         r"T 0b L 1",  # Transparent (tag 0x0B)
#         r"Transparent: ON",
#         r"T 0c L 1",  # Retry (tag 0x0C)
#         r"Retry: ON",
#         r"T 14 L 1",  # Charset (tag 0x14)
#         r"Charset: UTF-8",
#         r"T 0a L 1",  # Attachment (tag 0x0A)
#         r"Attachment: ON",
#         r"T 22 L 32",  # ConversationID (tag 0x22, 32 hex chars)
#         r"ConversationID: 00112233445566778899AABBCCDDEEFF",
#         r"T 2a L 16",  # MessageHandle (tag 0x2A, 16 hex chars)
#         r"MessageHandle: 0000000000000001",
#         r"T 2b L 1",  # ModifyText (tag 0x2B)
#         r"ModifyText: PREPEND",
#         r"OBEX BODY: .*",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params, timeout=10)
#     assert found is True, "MSE did not receive all application parameters"

#     server.exec_command(f'test_map mse mas {cmd} noerror')
#     expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code Continue"]
#     expected.append("OBEX SRM: 01")
#     found, lines = client.wait_for_shell_response(expected)
#     assert found is True

#     while True:
#         client.exec_command(f'test_map mce mas {cmd}')
#         expected = [rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"]
#         expected.append(r"OBEX BODY: .*")
#         found, lines = server.wait_for_shell_response(expected)
#         assert found is True

#         expected = rf"MSE MAS \w+ {cmd} req, final (?P<final>\w+)"
#         for line in lines:
#             searched = re.search(expected, line)
#             if searched is not None:
#                 final = searched.group("final")
#                 break

#         if final == 'true':
#             break

#     assert final == 'true'

#     server.exec_command(f'test_map mse mas {cmd} noerror')
#     expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code Success"
#     found, _ = client.wait_for_shell_response(expected)
#     assert found is True

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_update_inbox_success(client, server):
#     """
#     Verify that the MCE can successfully request inbox update.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd(client, server, 'update_inbox')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_update_inbox_failure(client, server):
#     """
#     Verify that the MCE handles UpdateInbox errors correctly.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_fail(client, server, 'update_inbox', RspCode.NOT_ACCEPT)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_mas_inst_info_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve MAS instance information.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd(client, server, 'get_mas_inst_info')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_mas_inst_info_failure(client, server):
#     """
#     Verify that the MCE handles GetMASInstanceInformation errors correctly.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_fail(client, server, 'get_mas_inst_info', RspCode.NOT_FOUND)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_mas_inst_info_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in GetMASInstanceInformation
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for GetMASInstanceInformation
#     # MASInstanceID (0x0F) = 0x00 (instance ID 0)
#     client.exec_command('test_map add_header app_param mas_instance_id 0')

#     # Send GetMASInstanceInformation request
#     cmd = "get_mas_inst_info"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final true",
#         r"T 0f L 1",  # MASInstanceID (tag 0x0F)
#         r"MASInstanceID: 0",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # Wait for MCE to receive response
#     while True:
#         server.exec_command(f'test_map mse mas {cmd} noerror')
#         expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
#         expected.append(r"OBEX BODY: .*")
#         found, lines = client.wait_for_shell_response(expected)
#         assert found is True

#         expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
#         for line in lines:
#             searched = re.search(expected, line)
#             if searched is not None:
#                 rsp_code = searched.group("rsp_code")
#                 break

#         if rsp_code != 'Continue':
#             break

#     assert rsp_code == 'Success'

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_owner_status_success(client, server):
#     """
#     Verify that the MCE can successfully set owner status (presence and chat state).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd(client, server, 'set_owner_status')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_owner_status_failure(client, server):
#     """
#     Verify that the MCE handles SetOwnerStatus errors correctly.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_fail(client, server, 'set_owner_status', RspCode.BAD_REQ)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_owner_status_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in SetOwnerStatus
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for SetOwnerStatus
#     # PresenceAvailability (0x1C) = 0x02 (online)
#     client.exec_command('test_map add_header app_param presence_availability 2')

#     # PresenceText (0x1D) = "Available for chat"
#     client.exec_command('test_map add_header app_param presence_text "Available for chat"')

#     # LastActivity (0x1E) = "20240101T120000"
#     client.exec_command('test_map add_header app_param last_activity 20240101T120000')

#     # ChatState (0x21) = 0x02 (active)
#     client.exec_command('test_map add_header app_param chat_state 2')

#     # ConversationID (0x22) = 16 bytes conversation ID (32 hex chars)
#     client.exec_command(
#         'test_map add_header app_param conversation_id 00112233445566778899AABBCCDDEEFF'
#     )

#     # Send SetOwnerStatus request
#     cmd = "set_owner_status"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final \w+",
#         r"T 1c L 1",  # PresenceAvailability (tag 0x1C)
#         r"PresenceAvailability: 2 \(Online\)",
#         r"T 1d L \d+",  # PresenceText (tag 0x1D)
#         r"PresenceText: Available for chat",
#         r"T 1e L 15",  # LastActivity (tag 0x1E)
#         r"LastActivity: 20240101T120000",
#         r"T 21 L 1",  # ChatState (tag 0x21)
#         r"ChatState: 2 \(Active\)",
#         r"T 22 L 32",  # ConversationID (tag 0x22)
#         r"ConversationID: 00112233445566778899AABBCCDDEEFF",
#         r"OBEX BODY: .*",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # MSE sends response
#     server.exec_command(f'test_map mse mas {cmd} noerror')
#     expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code Success"
#     found, lines = client.wait_for_shell_response(expected)
#     assert found is True

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_owner_status_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve owner status.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd(client, server, 'get_owner_status')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_owner_status_failure(client, server):
#     """
#     Verify that the MCE handles GetOwnerStatus errors correctly.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_fail(client, server, 'get_owner_status', RspCode.BAD_REQ)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_owner_status_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in GetOwnerStatus
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for GetOwnerStatus
#     # ConversationID (0x22) = 16 bytes conversation ID (32 hex chars)
#     client.exec_command(
#         'test_map add_header app_param conversation_id 00112233445566778899AABBCCDDEEFF'
#     )

#     # Send GetOwnerStatus request
#     cmd = "get_owner_status"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final true",
#         r"T 22 L 32",  # ConversationID (tag 0x22)
#         r"ConversationID: 00112233445566778899AABBCCDDEEFF",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # Wait for MCE to receive response
#     while True:
#         server.exec_command(f'test_map mse mas {cmd} noerror')
#         expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
#         expected.append(r"OBEX BODY: .*")
#         found, lines = client.wait_for_shell_response(expected)
#         assert found is True

#         expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
#         for line in lines:
#             searched = re.search(expected, line)
#             if searched is not None:
#                 rsp_code = searched.group("rsp_code")
#                 break

#         if rsp_code != 'Continue':
#             break

#     assert rsp_code == 'Success'

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_final_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve conversation listing
#     with final bit set in a single response.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd(client, server, 'get_convo_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_non_final_success(client, server):
#     """
#     Verify that the MCE can successfully retrieve conversation listing
#     with final bit not set (multi-packet transfer).
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final(client, server, 'get_convo_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_final_failure(client, server):
#     """
#     Verify that the MCE handles GetConversationListing errors correctly when final bit is set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_fail(client, server, 'get_convo_listing', RspCode.BAD_REQ)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_non_final_failure(client, server):
#     """
#     Verify that the MCE handles GetConversationListing errors correctly when final bit is not set.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_rfcomm_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final_fail(client, server, 'get_convo_listing', RspCode.BAD_REQ)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_rfcomm_disconnect(client, server)
#     br_disconnect(client, server)


def test_br_map_mce_get_convo_listing_continue_response(client, server):
    """
    Verify that the MCE correctly handles Continue (0x90) response during GetConversationListing.
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mas_rfcomm_connect(client, server)
    mas_connect(client, server, RspCode.SUCCESS)
    mas_get_cmd_continue(client, server, 'get_convo_listing')
    mas_disconnect(client, server, RspCode.SUCCESS)
    mas_rfcomm_disconnect(client, server)
    br_disconnect(client, server)


def test_br_map_mce_get_convo_listing_srm_enabled(client, server):
    """
    Verify that the MCE can successfully retrieve conversation listing using
    Single Response Mode (SRM).
    """
    br_pre(client, server)
    br_connect(client, server)
    br_security(client, server)
    mas_l2cap_connect(client, server)
    mas_connect(client, server, RspCode.SUCCESS)
    mas_get_cmd_srm(client, server, 'get_convo_listing')
    mas_disconnect(client, server, RspCode.SUCCESS)
    mas_l2cap_disconnect(client, server)
    br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_non_final_srm_enabled(client, server):
#     """
#     Verify that the MCE can successfully retrieve conversation listing using
#     Single Response Mode (SRM) with final bit not set for multi-packet transfer.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_final_srm(client, server, 'get_convo_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_srmp_wait(client, server):
#     """
#     Verify that the MCE correctly handles SRMP Wait indication during GetConversationListing.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_srm_param(client, server, 'get_convo_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_abort_during_transfer(client, server):
#     """
#     Verify that the MCE can abort GetConversationListing operation during multi-packet transfer.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_abort(client, server, 'get_convo_listing')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_abort_failure(client, server):
#     """
#     Verify that the MCE handles abort failure during GetConversationListing operation.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_get_cmd_abort_fail(client, server, 'get_convo_listing', RspCode.FORBIDDEN)
#     br_disconnect(client, server)


# def test_br_map_mce_get_convo_listing_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in GetConversationListing
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for GetConversationListing
#     # MaxListCount (0x01) = 0x000A (10 entries)
#     client.exec_command('test_map add_header app_param max_list_count 10')

#     # ListStartOffset (0x02) = 0x0005 (start from 6th entry)
#     client.exec_command('test_map add_header app_param list_start_offset 5')

#     # FilterLastActivityBegin (0x1F) = "20240101T000000"
#     client.exec_command(
#         'test_map add_header app_param filter_last_activity_begin 20240101T000000'
#     )

#     # FilterLastActivityEnd (0x20) = "20241231T235959"
#     client.exec_command('test_map add_header app_param filter_last_activity_end 20241231T235959')

#     # ConvParameterMask (0x26) = 0xFFFFFFFF (all parameters)
#     client.exec_command('test_map add_header app_param convo_parameter_mask FFFFFFFF')

#     # FilterReadStatus (0x06) = 0x02 (read only)
#     client.exec_command('test_map add_header app_param filter_read_status 2')

#     # FilterRecipient (0x07) = "recipient@example.com"
#     client.exec_command('test_map add_header app_param filter_recipient recipient@example.com')

#     # ConversationID (0x22) = 16 bytes conversation ID (32 hex chars)
#     client.exec_command(
#         'test_map add_header app_param conversation_id 00112233445566778899AABBCCDDEEFF'
#     )

#     # Send GetConversationListing request
#     cmd = "get_convo_listing"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final true",
#         r"T 01 L 2",  # MaxListCount
#         r"MaxListCount: 10",
#         r"T 02 L 2",  # ListStartOffset
#         r"ListStartOffset: 5",
#         r"T 1f L 15",  # FilterLastActivityBegin
#         r"FilterLastActivityBegin: 20240101T000000",
#         r"T 20 L 15",  # FilterLastActivityEnd
#         r"FilterLastActivityEnd: 20241231T235959",
#         r"T 26 L 4",  # ConvParameterMask
#         r"ConvoParameterMask: 0xffffffff",
#         r"T 06 L 1",  # FilterReadStatus
#         r"FilterReadStatus: 2 \(read_only\)",
#         r"T 07 L 21",  # FilterRecipient
#         r"FilterRecipient: recipient@example.com",
#         r"T 22 L 32",  # ConversationID
#         r"ConversationID: 00112233445566778899AABBCCDDEEFF",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # Wait for MCE to receive response
#     while True:
#         server.exec_command(f'test_map mse mas {cmd} noerror')
#         expected = [rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"]
#         expected.append(r"OBEX BODY: .*")
#         found, lines = client.wait_for_shell_response(expected)
#         assert found is True

#         expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
#         for line in lines:
#             searched = re.search(expected, line)
#             if searched is not None:
#                 rsp_code = searched.group("rsp_code")
#                 break

#         if rsp_code != 'Continue':
#             break

#     assert rsp_code == 'Success'

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_ntf_filter_success(client, server):
#     """
#     Verify that the MCE can successfully set notification filter.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd(client, server, 'set_ntf_filter')
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_ntf_filter_failure(client, server):
#     """
#     Verify that the MCE handles SetNotificationFilter errors correctly.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)
#     mas_put_cmd_fail(client, server, 'set_ntf_filter', RspCode.BAD_REQ)
#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)


# def test_br_map_mce_set_ntf_filter_param_all(client, server):
#     """
#     Verify that MCE can send all supported application parameters in SetNotificationFilter
#     request and MSE can correctly parse and match these parameters.
#     """
#     br_pre(client, server)
#     br_connect(client, server)
#     br_security(client, server)
#     mas_l2cap_connect(client, server)
#     mas_connect(client, server, RspCode.SUCCESS)

#     # Clean up any existing parameters
#     client.exec_command('test_map add_header app_param clear')
#     server.exec_command('test_map add_header app_param clear')

#     # Add all application parameters for SetNotificationFilter
#     # NotificationFilterMask (0x25) = 0x00007FFF (all notification types enabled)
#     client.exec_command('test_map add_header app_param notification_filter_mask 00007FFF')

#     # Send SetNotificationFilter request
#     cmd = "set_ntf_filter"
#     client.exec_command(f'test_map mce mas {cmd}')

#     # Expected patterns for MSE to verify all parameters are received
#     expected_params = [
#         rf"MSE MAS \w+ {cmd} req, final \w+",
#         r"T 25 L 4",  # NotificationFilterMask (tag 0x25)
#         r"NotificationFilterMask: 0x00007fff",
#         r"OBEX BODY: .*",
#     ]

#     # Wait for MSE to receive and parse the request
#     found, _ = server.wait_for_shell_response(expected_params)
#     assert found is True, "MSE did not receive all application parameters"

#     # MSE sends response
#     server.exec_command(f'test_map mse mas {cmd} noerror')
#     expected = rf"MCE MAS \w+ {cmd} rsp, rsp_code Success"
#     found, _ = client.wait_for_shell_response(expected)
#     assert found is True

#     mas_disconnect(client, server, RspCode.SUCCESS)
#     mas_l2cap_disconnect(client, server)
#     br_disconnect(client, server)
