# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import logging
import re
import time

logger = logging.getLogger(__name__)

BT_OBEX_VERSION = 0x10
BT_GOEP_MTU = 261


class RspCode:
    CONTINUE = 0x90
    SUCCESS = 0xA0
    BAD_REQ = 0xC0
    UNAUTH = 0xC1
    FORBIDDEN = 0xC3
    NOT_FOUND = 0xC4
    NOT_ACCEPT = 0xC6


rsp_code_str = {
    RspCode.CONTINUE: "Continue",
    RspCode.SUCCESS: "Success",
    RspCode.BAD_REQ: "Bad Request - server couldn't understand request",
    RspCode.UNAUTH: "Unauthorized",
    RspCode.FORBIDDEN: "Forbidden - operation is understood but refused",
    RspCode.NOT_FOUND: "Not Found",
    RspCode.NOT_ACCEPT: "Not Acceptable",
}


# ---------------------------------------------------------------------------
# Common setup/teardown helpers
# ---------------------------------------------------------------------------


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
    count = 3
    while count > 0:
        try:
            _br_connect(init, rsp)
            break
        except Exception:
            time.sleep(5)
            count -= 1
            if count > 0:
                logger.info('Retry BR connection')
            continue


def br_disconnect(init, rsp):
    init.exec_command(f'bt disconnect {rsp.pub_addr}')
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


# ---------------------------------------------------------------------------
# L2CAP transport helpers
# ---------------------------------------------------------------------------


def ftp_l2cap_connect(init, rsp):
    """Connect FTP L2CAP transport (init -> rsp)."""
    psm = rsp.psm
    assert psm is not None

    init.exec_command(f'test_ftp client l2cap_connect {psm:x}')
    found, _ = rsp.wait_for_shell_response(r"FTP server L2CAP connected")
    assert found is True
    found, _ = init.wait_for_shell_response(r"FTP client L2CAP connected")
    assert found is True


def ftp_l2cap_disconnect(init, rsp):
    """Disconnect FTP L2CAP transport."""
    rsp.exec_command('test_ftp server l2cap_disconnect')
    found, _ = init.wait_for_shell_response(r"FTP client L2CAP disconnected")
    assert found is True
    found, _ = rsp.wait_for_shell_response(r"FTP server L2CAP disconnected")
    assert found is True


# ---------------------------------------------------------------------------
# RFCOMM transport helpers
# ---------------------------------------------------------------------------


def ftp_rfcomm_connect(init, rsp):
    """Connect FTP RFCOMM transport (init -> rsp)."""
    channel = rsp.channel
    assert channel is not None

    init.exec_command(f'test_ftp client rfcomm_connect {channel:x}')
    found, _ = rsp.wait_for_shell_response(r"FTP server RFCOMM connected")
    assert found is True
    found, _ = init.wait_for_shell_response(r"FTP client RFCOMM connected")
    assert found is True


def ftp_rfcomm_disconnect(init, rsp):
    """Disconnect FTP RFCOMM transport."""
    rsp.exec_command('test_ftp server rfcomm_disconnect')
    found, _ = init.wait_for_shell_response(r"FTP client RFCOMM disconnected")
    assert found is True
    found, _ = rsp.wait_for_shell_response(r"FTP server RFCOMM disconnected")
    assert found is True


# ---------------------------------------------------------------------------
# OBEX session helpers
# ---------------------------------------------------------------------------


def ftp_obex_connect(init, rsp, rsp_code=RspCode.SUCCESS, password=None):
    """OBEX CONNECT handshake."""
    if password:
        init.exec_command(f'test_ftp client connect {password}')
    else:
        init.exec_command('test_ftp client connect')

    expected = (
        rf"FTP server \w+ OBEX connect req, "
        rf"version {BT_OBEX_VERSION:02x}, mopl"
    )
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    expected = [
        (
            rf"FTP client \w+ OBEX connect rsp, rsp_code {rsp_code_str[rsp_code]}"
            rf", version {BT_OBEX_VERSION:02x}, mopl"
        )
    ]

    if rsp_code == RspCode.UNAUTH:
        rsp.exec_command(f'test_ftp server connect unauth {password}')
        expected.append(r"Server requires authentication")
        found, _ = init.wait_for_shell_response(expected)
        assert found is True
    elif rsp_code == RspCode.SUCCESS:
        rsp.exec_command('test_ftp server connect success')
        expected.append(r"Connection established")
        found, _ = init.wait_for_shell_response(expected)
        assert found is True
    else:
        rsp.exec_command(f'test_ftp server connect error {rsp_code:X}')
        found, _ = init.wait_for_shell_response(expected)
        assert found is True


def ftp_obex_disconnect(init, rsp, rsp_code=RspCode.SUCCESS):
    """OBEX DISCONNECT handshake."""
    init.exec_command('test_ftp client disconnect')
    found, _ = rsp.wait_for_shell_response(r"FTP server \w+ OBEX disconnect req")
    assert found is True

    if rsp_code == RspCode.SUCCESS:
        rsp.exec_command('test_ftp server disconnect success')
    else:
        rsp.exec_command(f'test_ftp server disconnect error {rsp_code:X}')

    expected = rf"FTP client \w+ OBEX disconnect rsp, rsp_code {rsp_code_str[rsp_code]}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ftp_get_cmd_initiate(init, rsp, cmd, extra_args=''):
    """Send first GET request and get a CONTINUE response."""
    init.exec_command(f'test_ftp client {cmd}{extra_args}')
    found, _ = rsp.wait_for_shell_response(rf"FTP server \w+ {cmd} req, final true")
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror')
    expected = [
        rf"FTP client \w+ {cmd} rsp, rsp_code Continue",
        r"OBEX BODY: .*",
    ]
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ftp_get_cmd_complete(init, rsp, cmd, extra_args=''):
    """Continue a GET until Success is received."""
    rsp_code = 'Continue'

    while True:
        init.exec_command(f'test_ftp client {cmd}{extra_args}')
        found, _ = rsp.wait_for_shell_response(rf"FTP server \w+ {cmd} req, final true")
        assert found is True

        rsp.exec_command(f'test_ftp server {cmd} noerror')
        expected = [
            rf"FTP client \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)",
            r"OBEX BODY: .*",
        ]
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        pat = rf"FTP client \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(pat, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'


def ftp_get_cmd_no_split(init, rsp, cmd, extra_args=''):
    """GET in a sigle exchange."""
    init.exec_command(f'test_ftp client {cmd}{extra_args}')
    found, _ = rsp.wait_for_shell_response(rf"FTP server \w+ {cmd} req, final true")
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror no_split')
    expected = [
        rf"FTP client \w+ {cmd} rsp, rsp_code Success",
        r"OBEX BODY: .*",
    ]
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ftp_get_cmd_fail(init, rsp, cmd, rsp_code, extra_args=''):
    """Perform a GET that returns an error."""
    init.exec_command(f'test_ftp client {cmd}{extra_args}')
    found, _ = rsp.wait_for_shell_response(rf"FTP server \w+ {cmd} req, final true")
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} error {rsp_code:X}')
    found, _ = init.wait_for_shell_response(
        rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code_str[rsp_code]}"
    )
    assert found is True


def ftp_get_cmd_srm_no_split(init, rsp, cmd, extra_args=''):
    """GET with SRM enabled in a single exchange."""
    init.exec_command(f'test_ftp client {cmd}{extra_args} srm')
    expected = [rf"FTP server \w+ {cmd} req, final true", "OBEX SRM: 0x01"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror no_split')
    expected = [
        rf"FTP client \w+ {cmd} rsp, rsp_code Success",
        r"OBEX BODY: .*",
    ]
    found, _ = init.wait_for_shell_response(expected)
    assert found is True


def ftp_get_cmd_srm_complete(init, rsp, cmd):
    """Continue until Success is received."""
    rsp_code = 'Continue'

    while True:
        rsp.exec_command(f'test_ftp server {cmd} noerror')
        expected = [
            rf"FTP client \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)",
            r"OBEX BODY: .*",
        ]
        found, lines = init.wait_for_shell_response(expected)
        assert found is True

        pat = rf"FTP client \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        for line in lines:
            searched = re.search(pat, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if rsp_code != 'Continue':
            break

    assert rsp_code == 'Success'


def ftp_get_cmd_srm(init, rsp, cmd, extra_args=''):
    """Perform a GET with SRM enabled."""
    rsp_code = 'Continue'

    init.exec_command(f'test_ftp client {cmd}{extra_args} srm')
    expected = [rf"FTP server \w+ {cmd} req, final true", "OBEX SRM: 0x01"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror srm')
    expected = [
        rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code}",
        "OBEX SRM: 0x01",
        r"OBEX BODY: .*",
    ]
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    # Response phase
    ftp_get_cmd_srm_complete(init, rsp, cmd)


def ftp_get_cmd_srm_param(init, rsp, cmd, extra_args=''):
    """Perform a GET with SRM and SRMP wait."""
    rsp_code = 'Continue'

    init.exec_command(f'test_ftp client {cmd}{extra_args} srm srmp')
    expected = [rf"FTP server \w+ {cmd} req, final true", "OBEX SRM: 0x01", "OBEX SRMP: 0x01"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror srm srmp')
    expected = [
        rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code}",
        "OBEX SRM: 0x01",
        "OBEX SRMP: 0x01",
        r"OBEX BODY: .*",
    ]
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    # Client sends a follow-up GET after SRMP wait
    init.exec_command(f'test_ftp client {cmd}{extra_args}')
    found, _ = rsp.wait_for_shell_response(rf"FTP server \w+ {cmd} req, final true")
    assert found is True

    # Response phase
    ftp_get_cmd_srm_complete(init, rsp, cmd)


def ftp_get_cmd_no_final(init, rsp, cmd, extra_args=''):
    """Perform a GET where init sends non-final (chunked) requests until final=true."""
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_ftp client {cmd}{extra_args} chunked_req')
    found, _ = rsp.wait_for_shell_response(rf"FTP server \w+ {cmd} req, final false")
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror')
    found, _ = init.wait_for_shell_response(rf"FTP client \w+ {cmd} rsp, rsp_code Continue")
    assert found is True

    while True:
        init.exec_command(f'test_ftp client {cmd}{extra_args}')
        expected = rf"FTP server \w+ {cmd} req, final (?P<final>\w+)"
        found, lines = rsp.wait_for_shell_response(expected)
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        rsp.exec_command(f'test_ftp server {cmd} noerror')
        expected = rf"FTP client \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
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
    ftp_get_cmd_complete(init, rsp, cmd, extra_args=extra_args)


def ftp_get_cmd_no_final_srm(init, rsp, cmd, extra_args=''):
    """Perform a GET with SRM where init sends non-final (chunked) requests until final=true."""
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_ftp client {cmd}{extra_args} chunked_req srm')
    expected = [rf"FTP server \w+ {cmd} req, final false", "OBEX SRM: 0x01"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror srm')
    expected = [
        rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code}",
        "OBEX SRM: 0x01",
    ]
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_ftp client {cmd}{extra_args}')
        expected = rf"FTP server \w+ {cmd} req, final (?P<final>\w+)"
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
    ftp_get_cmd_srm_complete(init, rsp, cmd)


def ftp_get_cmd_no_final_srm_param(init, rsp, cmd, extra_args=''):
    """Perform a GET with SRM where init sends non-final (chunked) requests
    and server responds with SRMP wait.
    """
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_ftp client {cmd}{extra_args} chunked_req srm')
    expected = [rf"FTP server \w+ {cmd} req, final false", "OBEX SRM: 0x01"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror srm srmp')
    expected = [
        rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code}",
        "OBEX SRM: 0x01",
        "OBEX SRMP: 0x01",
    ]
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while True:
        init.exec_command(f'test_ftp client {cmd}{extra_args}')
        expected = rf"FTP server \w+ {cmd} req, final (?P<final>\w+)"
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
    ftp_get_cmd_srm_complete(init, rsp, cmd)


def ftp_get_cmd_abort(init, rsp, cmd, extra_args=''):
    """Abort a multi-packet GET in progress."""
    ftp_get_cmd_initiate(init, rsp, cmd, extra_args)

    init.exec_command('test_ftp client abort')
    found, _ = rsp.wait_for_shell_response(r"FTP server \w+ abort req")
    assert found is True

    rsp.exec_command('test_ftp server abort success')
    found, _ = init.wait_for_shell_response(r"FTP client \w+ abort rsp, rsp_code Success")
    assert found is True


def ftp_get_cmd_abort_fail(init, rsp, cmd, rsp_code, extra_args=''):
    """Abort a multi-packet GET and server returns error - link drops."""
    ftp_get_cmd_initiate(init, rsp, cmd, extra_args)

    init.exec_command('test_ftp client abort')
    found, _ = rsp.wait_for_shell_response(r"FTP server \w+ abort req")
    assert found is True

    rsp.exec_command(f'test_ftp server abort error {rsp_code:X}')
    found, _ = init.wait_for_shell_response(r"FTP client L2CAP disconnected")
    assert found is True
    found, _ = rsp.wait_for_shell_response(r"FTP server L2CAP disconnected")
    assert found is True


# ---------------------------------------------------------------------------
# PUT-type operation helpers (push_file, delete, rename, copy, set_permission)
# ---------------------------------------------------------------------------


def ftp_put_cmd_initiate(init, rsp, cmd, extra_args=''):
    """Send first PUT and get CONTINUE response."""
    init.exec_command(f'test_ftp client {cmd}{extra_args}')
    expected = [rf"FTP server \w+ {cmd} req, final false", r"OBEX BODY: .*"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror')
    found, _ = init.wait_for_shell_response(rf"FTP client \w+ {cmd} rsp, rsp_code Continue")
    assert found is True


def ftp_put_cmd_complete(init, rsp, cmd, extra_args=''):
    """Continue PUT until Success."""
    rsp_code = 'Continue'
    final = 'false'

    while True:
        init.exec_command(f'test_ftp client {cmd}{extra_args}')
        expected = rf"FTP server \w+ {cmd} req, final (?P<final>\w+)"
        found, lines = rsp.wait_for_shell_response(
            [rf"FTP server \w+ {cmd} req, final (?P<final>\w+)", r"OBEX BODY: .*"]
        )
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        rsp.exec_command(f'test_ftp server {cmd} noerror')
        pat = rf"FTP client \w+ {cmd} rsp, rsp_code (?P<rsp_code>\w+)"
        found, lines = init.wait_for_shell_response(pat)
        assert found is True

        for line in lines:
            searched = re.search(pat, line)
            if searched is not None:
                rsp_code = searched.group("rsp_code")
                break

        if final == 'true':
            break

    assert rsp_code == 'Success'


def ftp_put_cmd_no_split(init, rsp, cmd, extra_args=''):
    """PUT in a single exchange."""
    init.exec_command(f'test_ftp client {cmd}{extra_args} no_split')
    expected = [rf"FTP server \w+ {cmd} req, final true", r"OBEX BODY: .*"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror')
    found, _ = init.wait_for_shell_response(rf"FTP client \w+ {cmd} rsp, rsp_code Success")
    assert found is True


def ftp_put_cmd_fail(init, rsp, cmd, rsp_code, extra_args=''):
    """Perform a PUT that returns an error on the first packet."""
    init.exec_command(f'test_ftp client {cmd}{extra_args}')
    found, _ = rsp.wait_for_shell_response(
        [rf"FTP server \w+ {cmd} req, final \w+", r"OBEX BODY: .*"]
    )
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} error {rsp_code:X}')
    found, _ = init.wait_for_shell_response(
        rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code_str[rsp_code]}"
    )
    assert found is True


def ftp_put_cmd_srm(init, rsp, cmd, extra_args=''):
    """Perform a PUT with SRM enabled (streaming)."""
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_ftp client {cmd}{extra_args} srm')
    expected = [rf"FTP server \w+ {cmd} req, final false", "OBEX SRM: 0x01", r"OBEX BODY: .*"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror srm')
    found, _ = init.wait_for_shell_response(
        [rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code}", "OBEX SRM: 0x01"]
    )
    assert found is True

    while True:
        init.exec_command(f'test_ftp client {cmd}{extra_args}')
        expected = rf"FTP server \w+ {cmd} req, final (?P<final>\w+)"
        found, lines = rsp.wait_for_shell_response(
            [rf"FTP server \w+ {cmd} req, final (?P<final>\w+)", r"OBEX BODY: .*"]
        )
        assert found is True

        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

        if final == 'true':
            break

    rsp.exec_command(f'test_ftp server {cmd} noerror')
    found, _ = init.wait_for_shell_response(rf"FTP client \w+ {cmd} rsp, rsp_code Success")
    assert found is True


def ftp_put_cmd_srm_param(init, rsp, cmd, extra_args=''):
    """Perform a PUT with SRM and SRMP wait."""
    rsp_code = 'Continue'
    final = 'false'

    init.exec_command(f'test_ftp client {cmd}{extra_args} srm')
    expected = [rf"FTP server \w+ {cmd} req, final false", "OBEX SRM: 0x01", r"OBEX BODY: .*"]
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror srm srmp')
    found, _ = init.wait_for_shell_response(
        [rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code}", "OBEX SRM: 0x01", "OBEX SRMP: 0x01"]
    )
    assert found is True

    # First PUT after SRMP wait
    init.exec_command(f'test_ftp client {cmd}{extra_args}')
    expected = rf"FTP server \w+ {cmd} req, final (?P<final>\w+)"
    found, lines = rsp.wait_for_shell_response(
        [rf"FTP server \w+ {cmd} req, final (?P<final>\w+)", r"OBEX BODY: .*"]
    )
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} noerror')
    expected = rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code}"
    found, _ = init.wait_for_shell_response(expected)
    assert found is True

    while final != 'true':
        init.exec_command(f'test_ftp client {cmd}{extra_args}')
        found, lines = rsp.wait_for_shell_response(
            [rf"FTP server \w+ {cmd} req, final (?P<final>\w+)", r"OBEX BODY: .*"]
        )
        assert found is True

        expected = rf"FTP server \w+ {cmd} req, final (?P<final>\w+)"
        for line in lines:
            searched = re.search(expected, line)
            if searched is not None:
                final = searched.group("final")
                break

    rsp.exec_command(f'test_ftp server {cmd} noerror')
    found, _ = init.wait_for_shell_response(rf"FTP client \w+ {cmd} rsp, rsp_code Success")
    assert found is True


def ftp_put_cmd_abort(init, rsp, cmd, extra_args=''):
    """Abort a multi-packet PUT in progress."""
    ftp_put_cmd_initiate(init, rsp, cmd, extra_args)

    init.exec_command('test_ftp client abort')
    found, _ = rsp.wait_for_shell_response(r"FTP server \w+ abort req")
    assert found is True

    rsp.exec_command('test_ftp server abort success')
    found, _ = init.wait_for_shell_response(r"FTP client \w+ abort rsp, rsp_code Success")
    assert found is True


def ftp_put_cmd_abort_fail(init, rsp, cmd, rsp_code, extra_args=''):
    """Abort a multi-packet PUT and server returns error - link drops."""
    ftp_put_cmd_initiate(init, rsp, cmd, extra_args)

    init.exec_command('test_ftp client abort')
    found, _ = rsp.wait_for_shell_response(r"FTP server \w+ abort req")
    assert found is True

    rsp.exec_command(f'test_ftp server abort error {rsp_code:X}')
    found, _ = init.wait_for_shell_response(r"FTP client L2CAP disconnected")
    assert found is True
    found, _ = rsp.wait_for_shell_response(r"FTP server L2CAP disconnected")
    assert found is True


def ftp_set_folder(init, rsp, path, rsp_code):
    """Perform a SetFolder operation."""
    if path == "/":
        flags = "02"  # BT_FTP_SET_FOLDER_FLAGS_ROOT = NO_CREATE
        name = ''  # empty name header for root
    elif path.startswith(".."):
        flags = "03"  # BT_FTP_SET_FOLDER_FLAGS_UP = BACKUP | NO_CREATE
        name = None
    else:
        flags = "02"  # BT_FTP_SET_FOLDER_FLAGS_DOWN = NO_CREATE
        name = path[2:] if path.startswith("./") else path

    init.exec_command(f'test_ftp client set_folder {path}')
    expected = [rf"FTP server \w+ set_folder req, flags {flags}"]
    if name is not None:
        expected.append(f"OBEX Name: {name}")
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    if rsp_code == RspCode.SUCCESS:
        rsp.exec_command('test_ftp server set_folder success')
    else:
        rsp.exec_command(f'test_ftp server set_folder error {rsp_code:X}')

    found, _ = init.wait_for_shell_response(
        rf"FTP client \w+ set_folder rsp, rsp_code {rsp_code_str[rsp_code]}"
    )
    assert found is True


def ftp_manipulate_object(init, rsp, cmd, params: dict):
    """Manipulate Objects(create_folder, delete, rename, copy, set_permission)."""
    name = params.get('name')
    assert name is not None
    init_cmd = f'test_ftp client {cmd} {name}'
    expected = [rf"FTP server \w+ {cmd} req", rf"OBEX Name: {name}"]

    if cmd in ['rename', 'copy']:
        dest_name = params.get('dest_name')
        assert dest_name is not None
        init_cmd += f' {dest_name}'
        expected.append(rf"OBEX DEST Name: {dest_name}")
    elif cmd in ['set_permission']:
        perms = params.get('perms')
        assert perms is not None
        init_cmd += f' 0x{perms:08x}'
        expected.append(rf"OBEX Permissions: 0x{perms:08x}")

    init.exec_command(init_cmd)
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} success')
    found, _ = init.wait_for_shell_response(rf"FTP client \w+ {cmd} rsp, rsp_code Success")
    assert found is True


def ftp_manipulate_object_fail(init, rsp, cmd, params: dict, rsp_code):
    """Manipulate Objects(create_folder, delete, rename, copy, set_permission)
    that returns an error.
    """
    name = params.get('name')
    assert name is not None
    init_cmd = f'test_ftp client {cmd} {name}'
    expected = [rf"FTP server \w+ {cmd} req", rf"OBEX Name: {name}"]

    if cmd in ['rename', 'copy']:
        dest_name = params.get('dest_name')
        assert dest_name is not None
        init_cmd += f' {dest_name}'
        expected.append(rf"OBEX DEST Name: {dest_name}")
    elif cmd in ['set_permission']:
        perms = params.get('perms')
        assert perms is not None
        init_cmd += f' 0x{perms:08x}'
        expected.append(rf"OBEX Permissions: 0x{perms:08x}")

    init.exec_command(init_cmd)
    found, _ = rsp.wait_for_shell_response(expected)
    assert found is True

    rsp.exec_command(f'test_ftp server {cmd} error {rsp_code:X}')
    found, _ = init.wait_for_shell_response(
        rf"FTP client \w+ {cmd} rsp, rsp_code {rsp_code_str[rsp_code]}"
    )
    assert found is True


# ---------------------------------------------------------------------------
# 5.1.1 RFCOMM Transport
# ---------------------------------------------------------------------------


def test_br_ftp_server_rfcomm_connect_success(client, server):
    """BR_FTP_SERVER_RFCOMM_CONNECT_SUCCESS: RFCOMM transport connect success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_rfcomm_connect(server, client)
    ftp_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_rfcomm_connect_error(client, server):
    """BR_FTP_SERVER_RFCOMM_CONNECT_ERROR: RFCOMM connect with invalid channel."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    # Use an invalid/unused channel to trigger connection failure
    lines = server.exec_command('test_ftp client rfcomm_connect ff')
    expected = "RFCOMM connect failed"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(server, client)


def test_br_ftp_server_rfcomm_disconnect_success(client, server):
    """BR_FTP_SERVER_RFCOMM_DISCONNECT_SUCCESS: RFCOMM disconnect success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_rfcomm_connect(server, client)
    ftp_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_rfcomm_disconnect_error(client, server):
    """BR_FTP_SERVER_RFCOMM_DISCONNECT_ERROR: RFCOMM disconnect when not connected."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    # No RFCOMM session; disconnect should fail gracefully
    lines = server.exec_command('test_ftp client rfcomm_disconnect')
    expected = rf"No connected FTP client for conn {client.pub_addr}"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(server, client)


def test_br_ftp_server_rfcomm_reconnect_same_channel(client, server):
    """BR_FTP_SERVER_RFCOMM_RECONNECT_SAME_CHANNEL: Reconnect same channel
    while connected must fail.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_rfcomm_connect(server, client)

    # Attempt second RFCOMM connect on the same channel - must fail
    channel = client.channel
    lines = server.exec_command(f'test_ftp client rfcomm_connect {channel:x}')
    expected = r"FTP client already connected"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True

    ftp_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_rfcomm_reconnect_with_l2cap(client, server):
    """BR_FTP_SERVER_RFCOMM_RECONNECT_WITH_L2CAP: RFCOMM connect while L2CAP active must fail."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_rfcomm_connect(server, client)

    # Attempt L2CAP connect while RFCOMM is active - must fail
    psm = client.psm
    lines = server.exec_command(f'test_ftp client l2cap_connect {psm:x}')
    expected = r"FTP client already connected"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True

    ftp_rfcomm_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.1.2 L2CAP Transport
# ---------------------------------------------------------------------------


def test_br_ftp_server_l2cap_connect_success(client, server):
    """BR_FTP_SERVER_L2CAP_CONNECT_SUCCESS: L2CAP transport connect success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_l2cap_connect_error(client, server):
    """BR_FTP_SERVER_L2CAP_CONNECT_ERROR: L2CAP connect with invalid PSM."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    # Use an invalid PSM to trigger connection failure
    lines = server.exec_command('test_ftp client l2cap_connect ffff')
    expected = "L2CAP connect failed"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(server, client)


def test_br_ftp_server_l2cap_disconnect_success(client, server):
    """BR_FTP_SERVER_L2CAP_DISCONNECT_SUCCESS: L2CAP disconnect success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_l2cap_disconnect_error(client, server):
    """BR_FTP_SERVER_L2CAP_DISCONNECT_ERROR: L2CAP disconnect when not connected."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    lines = server.exec_command('test_ftp client l2cap_disconnect')
    expected = rf"No connected FTP client for conn {client.pub_addr}"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True
    br_disconnect(server, client)


def test_br_ftp_server_l2cap_reconnect_same_psm(client, server):
    """BR_FTP_SERVER_L2CAP_RECONNECT_SAME_PSM: Reconnect same PSM while connected must fail."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)

    # Attempt second L2CAP connect on the same PSM - must fail
    psm = client.psm
    lines = server.exec_command(f'test_ftp client l2cap_connect {psm:x}')
    expected = r"FTP client already connected"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True

    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_l2cap_reconnect_with_rfcomm(client, server):
    """BR_FTP_SERVER_L2CAP_RECONNECT_WITH_RFCOMM: L2CAP connect while RFCOMM active must fail."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)

    # Attempt RFCOMM connect while L2CAP is active - must fail
    channel = client.channel
    lines = server.exec_command(f'test_ftp client rfcomm_connect {channel:x}')
    expected = r"FTP client already connected"
    found = server.check_shell_response(lines, expected)
    if found is False:
        found, _ = server.wait_for_shell_response(expected)
        assert found is True

    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.2 OBEX Connection Establishment
# ---------------------------------------------------------------------------


def test_br_ftp_server_obex_connect_no_auth_success(client, server):
    """BR_FTP_SERVER_OBEX_CONNECT_NO_AUTH_SUCCESS: OBEX connect without authentication."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_obex_connect_no_auth_error(client, server):
    """BR_FTP_SERVER_OBEX_CONNECT_NO_AUTH_ERROR: OBEX connect refused by client."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.FORBIDDEN)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_obex_connect_with_auth_success(client, server):
    """BR_FTP_SERVER_OBEX_CONNECT_WITH_AUTH_SUCCESS: OBEX connect with correct password."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)

    server.exec_command('test_ftp client connect')
    expected = (
        rf"FTP server \w+ OBEX connect req, "
        rf"version {BT_OBEX_VERSION:02x}, mopl"
    )
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Server challenges with password "1234"
    client.exec_command('test_ftp server connect unauth 1234')
    expected = [
        (
            rf"FTP client \w+ OBEX connect rsp, rsp_code {rsp_code_str[RspCode.UNAUTH]}"
            rf", version {BT_OBEX_VERSION:02x}, mopl"
        )
    ]
    expected.append(r"Server requires authentication")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Client re-connects with correct password
    server.exec_command('test_ftp client connect 1234')
    expected = [
        (
            rf"FTP server \w+ OBEX connect req, "
            rf"version {BT_OBEX_VERSION:02x}, mopl"
        )
    ]
    expected.append(r"Authentication succeeded")
    expected.append(r"Client requires authentication")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    client.exec_command('test_ftp server connect success')
    expected = [
        (
            rf"FTP client \w+ OBEX connect rsp, rsp_code {rsp_code_str[RspCode.SUCCESS]}"
            rf", version {BT_OBEX_VERSION:02x}, mopl"
        )
    ]
    expected.append(r"Connection established \(authentication succeeded\)")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_obex_connect_with_auth_error(client, server):
    """BR_FTP_SERVER_OBEX_CONNECT_WITH_AUTH_ERROR: OBEX connect with wrong password."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)

    server.exec_command('test_ftp client connect')
    expected = (
        rf"FTP server \w+ OBEX connect req, "
        rf"version {BT_OBEX_VERSION:02x}, mopl"
    )
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Server challenges with password "1234"
    client.exec_command('test_ftp server connect unauth 1234')
    expected = [
        (
            rf"FTP client \w+ OBEX connect rsp, rsp_code {rsp_code_str[RspCode.UNAUTH]}"
            rf", version {BT_OBEX_VERSION:02x}, mopl"
        )
    ]
    expected.append(r"Server requires authentication")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Client re-connects with wrong password "wrong"
    server.exec_command('test_ftp client connect wrong')
    expected = [
        (
            rf"FTP server \w+ OBEX connect req, "
            rf"version {BT_OBEX_VERSION:02x}, mopl"
        )
    ]
    expected.append(r"Authentication failed: auth response verification failed")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Server rejects (authentication failure -> forbidden)
    client.exec_command(f'test_ftp server connect error {RspCode.FORBIDDEN:X}')
    expected = (
        rf"FTP client \w+ OBEX connect rsp, rsp_code {rsp_code_str[RspCode.FORBIDDEN]}"
        rf", version {BT_OBEX_VERSION:02x}, mopl"
    )
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.3 OBEX Disconnection
# ---------------------------------------------------------------------------


def test_br_ftp_server_obex_disconnect_success(client, server):
    """BR_FTP_SERVER_OBEX_DISCONNECT_SUCCESS: OBEX disconnect success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_obex_disconnect_error(client, server):
    """BR_FTP_SERVER_OBEX_DISCONNECT_ERROR: OBEX disconnect with server error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_obex_disconnect(server, client, RspCode.BAD_REQ)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.5 Pull Folder Listing
# ---------------------------------------------------------------------------


def test_br_ftp_server_pull_folder_listing_success(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_SUCCESS: Single success response."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_no_split(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_error(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_ERROR: Server returns error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_fail(server, client, 'pull_folder_listing', RspCode.FORBIDDEN)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_continue(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_CONTINUE: Multi-packet with CONTINUE."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_initiate(server, client, 'pull_folder_listing')
    ftp_get_cmd_complete(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_no_final(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_NO_FINAL: Client sends non-final GET requests."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_no_final(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_no_final_srm(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_NO_FINAL_SRM: Non-final GET requests with SRM."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_no_final_srm(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_no_final_srmp_wait(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_NO_FINAL_SRMP_WAIT: Non-final GET
    requests with SRM and server SRMP wait.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_no_final_srm_param(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_srm_success(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_SRM_SUCCESS: SRM enabled, single response."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_srm_no_split(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_srm_continue(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_SRM_CONTINUE: SRM enabled multi-packet with Continue."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_srm(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_srmp_wait(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_SRMP_WAIT: SRM + SRMP wait."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_srm_param(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_abort(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_ABORT: Abort during multi-packet transfer."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_abort(server, client, 'pull_folder_listing')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_folder_listing_abort_error(client, server):
    """BR_FTP_SERVER_PULL_FOLDER_LISTING_ABORT_ERROR: Abort during
    multi-packet transfer and server returns error - link drops.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_abort_fail(server, client, 'pull_folder_listing', RspCode.FORBIDDEN)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.6 SetFolder
# ---------------------------------------------------------------------------


def test_br_ftp_server_set_folder_root_success(client, server):
    """BR_FTP_SERVER_SET_FOLDER_ROOT_SUCCESS: Navigate to root success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_set_folder(server, client, '/', RspCode.SUCCESS)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_set_folder_root_error(client, server):
    """BR_FTP_SERVER_SET_FOLDER_ROOT_ERROR: Navigate to root error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_set_folder(server, client, '/', RspCode.FORBIDDEN)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_set_folder_down_success(client, server):
    """BR_FTP_SERVER_SET_FOLDER_DOWN_SUCCESS: Navigate down to child folder success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_set_folder(server, client, 'docs', RspCode.SUCCESS)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_set_folder_down_error(client, server):
    """BR_FTP_SERVER_SET_FOLDER_DOWN_ERROR: Navigate down to nonexistent folder error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_set_folder(server, client, 'nonexistent', RspCode.NOT_FOUND)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_set_folder_up_success(client, server):
    """BR_FTP_SERVER_SET_FOLDER_UP_SUCCESS: Navigate up to parent folder success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_set_folder(server, client, '..', RspCode.SUCCESS)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_set_folder_up_error(client, server):
    """BR_FTP_SERVER_SET_FOLDER_UP_ERROR: Navigate up when already at root error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_set_folder(server, client, '..', RspCode.FORBIDDEN)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.7 CreateFolder
# ---------------------------------------------------------------------------


def test_br_ftp_server_create_folder_success(client, server):
    """BR_FTP_SERVER_CREATE_FOLDER_SUCCESS: Create folder success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'newfolder'}
    ftp_manipulate_object(server, client, 'create_folder', params)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_create_folder_error(client, server):
    """BR_FTP_SERVER_CREATE_FOLDER_ERROR: Create folder error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'readonly'}
    ftp_manipulate_object_fail(server, client, 'create_folder', params, RspCode.FORBIDDEN)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.8 Pull File
# ---------------------------------------------------------------------------


def test_br_ftp_server_pull_file_success(client, server):
    """BR_FTP_SERVER_PULL_FILE_SUCCESS: Pull file single success response."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_no_split(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_error(client, server):
    """BR_FTP_SERVER_PULL_FILE_ERROR: Server returns error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_fail(server, client, 'pull_file', RspCode.NOT_FOUND, ' missing.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_continue(client, server):
    """BR_FTP_SERVER_PULL_FILE_CONTINUE: Multi-packet with CONTINUE."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_initiate(server, client, 'pull_file', ' readme.txt')
    ftp_get_cmd_complete(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_no_final(client, server):
    """BR_FTP_SERVER_PULL_FILE_NO_FINAL: Client sends non-final GET requests."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_no_final(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_no_final_srm(client, server):
    """BR_FTP_SERVER_PULL_FILE_NO_FINAL_SRM: Non-final GET requests with SRM."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_no_final_srm(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_no_final_srmp_wait(client, server):
    """BR_FTP_SERVER_PULL_FILE_NO_FINAL_SRMP_WAIT: Non-final GET requests
    with SRM and server SRMP wait.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_no_final_srm_param(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_srm_success(client, server):
    """BR_FTP_SERVER_PULL_FILE_SRM_SUCCESS: SRM enabled, single response."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_srm_no_split(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_srm_continue(client, server):
    """BR_FTP_SERVER_PULL_FILE_SRM_CONTINUE: SRM enabled multi-packet with Continue."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_srm(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_srmp_wait(client, server):
    """BR_FTP_SERVER_PULL_FILE_SRMP_WAIT: SRM + SRMP wait."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_srm_param(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_abort(client, server):
    """BR_FTP_SERVER_PULL_FILE_ABORT: Abort during multi-packet download."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_abort(server, client, 'pull_file', ' readme.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_pull_file_abort_error(client, server):
    """BR_FTP_SERVER_PULL_FILE_ABORT_ERROR: Abort during pull_file and
    server returns error - link drops.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_get_cmd_abort_fail(server, client, 'pull_file', RspCode.FORBIDDEN, ' readme.txt')
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.9 Push File
# ---------------------------------------------------------------------------


def test_br_ftp_server_push_file_success(client, server):
    """BR_FTP_SERVER_PUSH_FILE_SUCCESS: Push file single PUT success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_put_cmd_no_split(server, client, 'push_file', ' test.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_push_file_error(client, server):
    """BR_FTP_SERVER_PUSH_FILE_ERROR: Server rejects push file."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_put_cmd_fail(server, client, 'push_file', RspCode.FORBIDDEN, ' test.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_push_file_continue(client, server):
    """BR_FTP_SERVER_PUSH_FILE_CONTINUE: Multi-packet PUT with CONTINUE then SUCCESS."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_put_cmd_initiate(server, client, 'push_file', ' test.txt')
    ftp_put_cmd_complete(server, client, 'push_file', ' test.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_push_file_continue_error(client, server):
    """BR_FTP_SERVER_PUSH_FILE_CONTINUE_ERROR: Server error mid-transfer."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_put_cmd_initiate(server, client, 'push_file', ' test.txt')

    # Second PUT - server now returns error
    server.exec_command('test_ftp client push_file test.txt')
    found, _ = client.wait_for_shell_response(r"FTP server \w+ push_file req")
    assert found is True
    client.exec_command(f'test_ftp server push_file error {RspCode.FORBIDDEN:X}')
    found, _ = server.wait_for_shell_response(
        rf"FTP client \w+ push_file rsp, rsp_code {rsp_code_str[RspCode.FORBIDDEN]}"
    )
    assert found is True

    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_push_file_srm(client, server):
    """BR_FTP_SERVER_PUSH_FILE_SRM: SRM enabled multi-packet streaming."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_put_cmd_srm(server, client, 'push_file', ' test.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_push_file_srmp_wait(client, server):
    """BR_FTP_SERVER_PUSH_FILE_SRMP_WAIT: SRM + SRMP wait."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_put_cmd_srm_param(server, client, 'push_file', ' test.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_push_file_abort(client, server):
    """BR_FTP_SERVER_PUSH_FILE_ABORT: Abort during multi-packet upload."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_put_cmd_abort(server, client, 'push_file', ' test.txt')
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_push_file_abort_error(client, server):
    """BR_FTP_SERVER_PUSH_FILE_ABORT_ERROR: Abort during push_file and
    server returns error - link drops.
    """
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    ftp_put_cmd_abort_fail(server, client, 'push_file', RspCode.FORBIDDEN, ' test.txt')
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.10 Delete
# ---------------------------------------------------------------------------


def test_br_ftp_server_delete_success(client, server):
    """BR_FTP_SERVER_DELETE_SUCCESS: Delete file success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'readme.txt'}
    ftp_manipulate_object(server, client, 'delete', params)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_delete_error(client, server):
    """BR_FTP_SERVER_DELETE_ERROR: Delete file error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'protected.txt'}
    ftp_manipulate_object_fail(server, client, 'delete', params, RspCode.FORBIDDEN)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.11 Copy
# ---------------------------------------------------------------------------


def test_br_ftp_server_copy_success(client, server):
    """BR_FTP_SERVER_COPY_SUCCESS: Copy file success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'readme.txt', 'dest_name': 'readme_backup.txt'}
    ftp_manipulate_object(server, client, 'copy', params)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_copy_error(client, server):
    """BR_FTP_SERVER_COPY_ERROR: Copy file error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'protected.txt', 'dest_name': 'dest.txt'}
    ftp_manipulate_object_fail(server, client, 'copy', params, RspCode.FORBIDDEN)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.12 Move/Rename
# ---------------------------------------------------------------------------


def test_br_ftp_server_rename_success(client, server):
    """BR_FTP_SERVER_RENAME_SUCCESS: Move/rename file success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'old.txt', 'dest_name': 'new.txt'}
    ftp_manipulate_object(server, client, 'rename', params)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_rename_error(client, server):
    """BR_FTP_SERVER_RENAME_ERROR: Move/rename file error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'locked.txt', 'dest_name': 'newname.txt'}
    ftp_manipulate_object_fail(server, client, 'rename', params, RspCode.FORBIDDEN)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


# ---------------------------------------------------------------------------
# 5.13 Set Permission
# ---------------------------------------------------------------------------


def test_br_ftp_server_set_permission_success(client, server):
    """BR_FTP_SERVER_SET_PERMISSION_SUCCESS: Set permission success."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'protected.txt', 'perms': 0x070505}
    ftp_manipulate_object(server, client, 'set_permission', params)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)


def test_br_ftp_server_set_permission_error(client, server):
    """BR_FTP_SERVER_SET_PERMISSION_ERROR: Set permission error."""
    br_pre(server, client)
    br_connect(server, client)
    br_security(server, client)
    ftp_l2cap_connect(server, client)
    ftp_obex_connect(server, client, RspCode.SUCCESS)
    params = {'name': 'readonly.txt', 'perms': 0x070505}
    ftp_manipulate_object_fail(server, client, 'set_permission', params, RspCode.FORBIDDEN)
    ftp_obex_disconnect(server, client, RspCode.SUCCESS)
    ftp_l2cap_disconnect(server, client)
    br_disconnect(server, client)
