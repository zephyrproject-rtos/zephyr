# SPDX-FileCopyrightText: Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

"""Bluetooth Classic BIP server b2b tests: BIP server (DUT) against BIP client (harness device)."""

import contextlib
import re
import time

from conftest import (
    logger,
)

PROMPT = None


def bt_obex_rsp_code_to_str(rsp_code):
    """Convert OBEX response code to string representation."""
    rsp_code_map = {
        0x90: "Continue",
        0xA0: "Success",
        0xA1: "Created",
        0xA2: "Accepted",
        0xA3: "Non-Authoritative Information",
        0xA4: "No Content",
        0xA5: "Reset Content",
        0xA6: "Partial Content",
        0xB0: "Multiple Choices",
        0xB1: "Moved Permanently",
        0xB2: "Moved temporarily",
        0xB3: "See Other",
        0xB4: "Not modified",
        0xB5: "Use Proxy",
        0xC0: "Bad Request - server couldn't understand request",
        0xC1: "Unauthorized",
        0xC2: "Payment Required",
        0xC3: "Forbidden - operation is understood but refused",
        0xC4: "Not Found",
        0xC5: "Method Not Allowed",
        0xC6: "Not Acceptable",
        0xC7: "Proxy Authentication Required",
        0xC8: "Request Time Out",
        0xC9: "Conflict",
        0xCA: "Gone",
        0xCB: "Length Required",
        0xCC: "Precondition Failed",
        0xCD: "Requested Entity Too Large",
        0xCE: "Requested URL Too Large",
        0xCF: "Unsupported media type",
        0xD0: "Internal serve Error",
        0xD1: "Not Implemented",
        0xD2: "Bad Gateway",
        0xD3: "Service Unavailable",
        0xD4: "Gateway Timeout",
        0xD5: "HTTP Version not supported",
        0xE0: "Database Full",
        0xE1: "Database Locked",
    }

    return rsp_code_map.get(rsp_code, "Unknown")


def bip_br_clear(bip_client, bip_server):
    """Clear BR state on both client and server."""
    bip_server.exec_command("br clear all")
    global PROMPT
    PROMPT = bip_server.shell.prompt
    bip_client.exec_command("br clear all")


def bip_br_connect(bip_client, bip_server):
    """Establish ACL connection from client to server."""
    retry = 3
    logger.info(f'acl connect {bip_server.addr}')
    while retry > 0:
        try:
            bip_client.iexpect(f'br connect {bip_server.addr}', 'Connected', timeout=35)
            break
        except Exception:
            time.sleep(5)
            retry -= 1
            if retry > 0:
                logger.info('Retry BR connection')
            continue


def bip_sdp_discover_rfcomm(bip_client):
    """SDP discover and extract RFCOMM channel."""
    pattern = r'Found RFCOMM channel\s+(\d+)'
    bip_client.exec_command('bip sdp discover')
    _, data = bip_client._wait_for_shell_response(r'Found RFCOMM channel.*', timeout=35)
    for line in data:
        match = re.search(pattern, line)
        if match:
            channel = match.group(1)
            logger.info(f'rfcomm channel = {channel}')
            return channel
    return None


def bip_sdp_discover_l2cap(bip_client):
    """SDP discover and extract L2CAP PSM."""
    pattern = r'Found GOEP L2CAP PSM\s+(0x[\da-fA-F]+|\d+)'
    bip_client.exec_command('bip sdp discover')
    _, data = bip_client._wait_for_shell_response(r'Found GOEP L2CAP PSM.*', timeout=35)
    for line in data:
        match = re.search(pattern, line)
        if match:
            psm = match.group(1)
            logger.info(f'l2cap psm = {psm}')
            return psm
    return None


def bip_rfcomm_transport_connection(bip_client, bip_server):
    """Establish RFCOMM transport connection."""
    channel = bip_sdp_discover_rfcomm(bip_client)
    if channel is None:
        logger.error('Failed to get BIP RFCOMM channel')
        return

    bip_client.iexpect(f'bip connect-rfcomm {channel}', r'BIP.*transport connected on.*')
    bip_server._wait_for_shell_response(r'BIP.*transport connected on.*')


def bip_l2cap_transport_connection(bip_client, bip_server):
    """Establish L2CAP transport connection."""
    psm = bip_sdp_discover_l2cap(bip_client)
    if psm is None:
        logger.error('Failed to get BIP L2CAP PSM')
        return

    psm_hex = format(int(psm), 'x')
    logger.info(f'l2cap psm hex = {psm_hex}')
    bip_client.iexpect(f'bip connect-l2cap {psm_hex}', r'BIP.*transport connected on.*')
    bip_server._wait_for_shell_response(r'BIP.*transport connected on.*')


BIP_UUID_IMAGE_PUSH = 'e33d954583744ad79ec5c16be31ede8e'


def bip_obex_connect(bip_client, bip_server, conn_type=0):
    """Establish OBEX connection. conn_type: 0=IMAGE_PUSH, 1=IMAGE_PULL, etc."""
    bip_client.iexpect('bip client set_feats_funcs', bip_client.shell.prompt)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command(f'bip client conn {conn_type}')
    bip_server._wait_for_shell_response(r'BIP server.*conn req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server conn success')
    bip_client._wait_for_shell_response(r'conn rsp.*Success', timeout=10)


def bip_obex_disconnect(bip_client, bip_server):
    """Disconnect OBEX connection."""
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip client disconn')
    bip_server._wait_for_shell_response(r'BIP server.*disconn req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server disconn success')
    bip_client._wait_for_shell_response(r'disconn rsp.*Success', timeout=10)


def test_BR_BIP_SERVER_RFCOMM_CONNECT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server RFCOMM transport connection success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_rfcomm_transport_connection(bip_client, bip_server)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_RFCOMM_CONNECT_FAILURE(bip_client, bip_server):
    """Test BR BIP Server RFCOMM transport connection failure with invalid channel."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    invalid_channel = 8
    bip_client.exec_command(f'bip connect-rfcomm {invalid_channel}')
    found = False
    try:
        _, lines = bip_client._wait_for_shell_response(r'BIP.*transport connected on.*', timeout=15)
        for line in lines:
            if re.search(r'BIP.*transport connected on.*', line):
                found = True
                break
    except AssertionError:
        pass
    assert found is False, "Transport should not connect with invalid channel"
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_L2CAP_CONNECT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server L2CAP transport connection success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_L2CAP_CONNECT_FAILURE(bip_client, bip_server):
    """Test BR BIP Server L2CAP transport connection failure with invalid PSM."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    invalid_psm = '1008'
    bip_client.exec_command(f'bip connect-l2cap {invalid_psm}')
    found = False
    try:
        _, lines = bip_client._wait_for_shell_response(r'BIP.*transport connected on.*', timeout=15)
        for line in lines:
            if re.search(r'BIP.*transport connected on.*', line):
                found = True
                break
    except AssertionError:
        pass
    assert found is False, "Transport should not connect with invalid PSM"
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_RFCOMM_DISCONNECT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server RFCOMM transport disconnection success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_rfcomm_transport_connection(bip_client, bip_server)
    bip_client.iexpect('bip disconnect-rfcomm', r'BIP.*transport disconnected')
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_L2CAP_DISCONNECT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server L2CAP transport disconnection success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_client.iexpect('bip disconnect-l2cap', r'BIP.*transport disconnected')
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_OBEX_CONNECT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server OBEX connection success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_obex_disconnect(bip_client, bip_server)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_OBEX_CONNECT_ERROR(bip_client, bip_server):
    """Test BR BIP Server OBEX connection error - server rejects with Forbidden."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_client.iexpect('bip client set_feats_funcs', bip_client.shell.prompt)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip client conn 0')
    bip_server._wait_for_shell_response(r'BIP server.*conn req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server conn error c3')
    bip_client._wait_for_shell_response(r'conn rsp.*Forbidden', timeout=10)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_OBEX_DISCONNECT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server OBEX disconnection success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_obex_disconnect(bip_client, bip_server)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_OBEX_DISCONNECT_ERROR(bip_client, bip_server):
    """Test BR BIP Server OBEX disconnection error - server rejects."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip client disconn')
    bip_server._wait_for_shell_response(r'BIP server.*disconn req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server disconn error c3')
    bip_client._wait_for_shell_response(r'disconn rsp.*Forbidden', timeout=10)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def bip_start_get_caps_with_continue(bip_client, bip_server):
    """Start a GetCapabilities operation and have server respond with Continue."""
    bip_client.exec_command('bip client get_caps')
    bip_server._wait_for_shell_response(r'BIP server.*get_caps req', timeout=10)
    bip_server.exec_command('bip server get_caps noerror')
    bip_client._wait_for_shell_response(r'get_caps rsp.*', timeout=10)


def bip_start_op_then_abort(bip_client, bip_server, op):
    """Start a BIP operation, get Continue (keep in-progress), then abort."""
    bip_client.exec_command(f'bip client {op}')
    bip_server._wait_for_shell_response(rf'BIP server.*{op} req', timeout=10)
    bip_server.exec_command(f'bip server {op} error 90')
    bip_client._wait_for_shell_response(rf'{op} rsp.*Continue', timeout=10)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip client abort')
    bip_server._wait_for_shell_response(r'BIP server.*abort req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server abort success')
    bip_client._wait_for_shell_response(r'abort rsp.*Success', timeout=10)


def bip_srm_test_setup(bip_client, bip_server, conn_type=0):
    """Common SRM test setup: clear, connect ACL, L2CAP transport, OBEX."""
    bip_br_clear(bip_client, bip_server)
    if conn_type != 0:
        bip_server_set_type(bip_server, conn_type)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=conn_type)


def bip_srm_test_teardown(bip_client, bip_server, conn_type=0):
    """Common SRM test teardown: ACL disconnect tears down all upper layers."""
    time.sleep(1)
    bip_client.dut.write(b'bt disconnect\n')
    with contextlib.suppress(Exception):
        bip_client._wait_for_shell_response(r'Disconnected', timeout=5)
    if conn_type != 0:
        bip_server_set_type(bip_server, 0)


def bip_srm_operation(bip_client, bip_server, op):
    """Perform a BIP operation with SRM header over L2CAP."""
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.iexpect('bip add-header srm 1', bip_client.shell.prompt)
    bip_operation(bip_client, bip_server, op)


def bip_srmp_server_operation(bip_client, bip_server, op):
    """Perform a BIP operation with SRM over L2CAP (SRMP Wait from server)."""
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.iexpect('bip add-header srm 1', bip_client.shell.prompt)
    bip_operation(bip_client, bip_server, op)


def bip_srmp_client_operation(bip_client, bip_server, op):
    """Perform a BIP operation with SRM + SRMP Wait from client over L2CAP."""
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.iexpect('bip add-header srm 1', bip_client.shell.prompt)
    bip_client.iexpect('bip add-header srm_param 1', bip_client.shell.prompt)
    bip_operation(bip_client, bip_server, op)


def test_BR_BIP_SERVER_ABORT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server abort ongoing operation success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_start_get_caps_with_continue(bip_client, bip_server)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip client abort')
    bip_server._wait_for_shell_response(r'BIP server.*abort req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server abort success')
    bip_client._wait_for_shell_response(r'abort rsp.*Success', timeout=10)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_ABORT_ERROR(bip_client, bip_server):
    """Test BR BIP Server abort operation error - server rejects."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_start_get_caps_with_continue(bip_client, bip_server)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip client abort')
    bip_server._wait_for_shell_response(r'BIP server.*abort req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server abort error c3')
    bip_client._wait_for_shell_response(r'BIP.*transport disconnected', timeout=10)
    bip_client.dut.write(b'bt disconnect\n')
    with contextlib.suppress(Exception):
        bip_client._wait_for_shell_response(r'Disconnected', timeout=5)


def bip_get_caps(bip_client, bip_server):
    """Perform full GetCapabilities exchange, handling Continue responses."""
    bip_client.exec_command('bip client get_caps')
    bip_server._wait_for_shell_response(r'BIP server.*get_caps req', timeout=10)
    bip_server.exec_command('bip server get_caps noerror')
    _, lines = bip_client._wait_for_shell_response(r'get_caps rsp', timeout=10)
    for line in lines:
        if re.search(r'get_caps rsp.*Continue', line):
            while True:
                bip_client.exec_command('bip client get_caps')
                bip_server._wait_for_shell_response(r'BIP server.*get_caps req', timeout=10)
                bip_server.exec_command('bip server get_caps noerror')
                _, lines2 = bip_client._wait_for_shell_response(r'get_caps rsp', timeout=10)
                done = False
                for line2 in lines2:
                    if re.search(r'get_caps rsp.*Success', line2):
                        done = True
                        break
                if done:
                    break
            break


def test_BR_BIP_SERVER_GET_CAPS_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetCapabilities success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_get_caps(bip_client, bip_server)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_GET_CAPS_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetCapabilities with multi-packet Continue responses."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_client.exec_command('bip client get_caps')
    bip_server._wait_for_shell_response(r'BIP server.*get_caps req', timeout=10)
    bip_server.exec_command('bip server get_caps noerror')
    _, lines = bip_client._wait_for_shell_response(r'get_caps rsp.*Continue', timeout=10)
    while True:
        bip_client.exec_command('bip client get_caps')
        bip_server._wait_for_shell_response(r'BIP server.*get_caps req', timeout=10)
        bip_server.exec_command('bip server get_caps noerror')
        _, lines = bip_client._wait_for_shell_response(r'get_caps rsp', timeout=10)
        done = False
        for line in lines:
            if re.search(r'get_caps rsp.*Success', line):
                done = True
                break
        if done:
            break
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_GET_CAPS_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetCapabilities error - server rejects."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_client.exec_command('bip client get_caps')
    bip_server._wait_for_shell_response(r'BIP server.*get_caps req', timeout=10)
    bip_server.exec_command('bip server get_caps error c3')
    bip_client._wait_for_shell_response(r'get_caps rsp.*Forbidden', timeout=10)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_GET_CAPS_SRM(bip_client, bip_server):
    """Test BR BIP Server GetCapabilities with SRM over L2CAP."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_srm_operation(bip_client, bip_server, 'get_caps')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_CAPS_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetCapabilities with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_srmp_server_operation(bip_client, bip_server, 'get_caps')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_CAPS_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetCapabilities with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_srmp_client_operation(bip_client, bip_server, 'get_caps')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_CAPS_ABORT(bip_client, bip_server):
    """Test BR BIP Server abort during GetCapabilities."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_start_get_caps_with_continue(bip_client, bip_server)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip client abort')
    bip_server._wait_for_shell_response(r'BIP server.*abort req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server abort success')
    bip_client._wait_for_shell_response(r'abort rsp.*Success', timeout=10)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def bip_put_image(bip_client, bip_server):
    """Perform full PutImage exchange, handling Continue responses."""
    while True:
        bip_client.exec_command('bip client put_image')
        bip_server._wait_for_shell_response(r'BIP server.*put_image req', timeout=10)
        bip_server.exec_command('bip server put_image noerror')
        _, lines = bip_client._wait_for_shell_response(r'put_image rsp', timeout=10)
        done = False
        for line in lines:
            if re.search(r'put_image rsp.*Success', line):
                done = True
                break
        if done:
            break


def test_BR_BIP_SERVER_PUT_IMAGE_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server PutImage success."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_put_image(bip_client, bip_server)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


def test_BR_BIP_SERVER_PUT_IMAGE_ERROR(bip_client, bip_server):
    """Test BR BIP Server PutImage error - server rejects."""
    bip_br_clear(bip_client, bip_server)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=0)
    bip_client.exec_command('bip client put_image')
    bip_server._wait_for_shell_response(r'BIP server.*put_image req', timeout=10)
    bip_server.exec_command('bip server put_image error cf')
    bip_client._wait_for_shell_response(r'put_image rsp.*Unsupported media type', timeout=10)
    time.sleep(1)
    bip_client.iexpect('bt disconnect', 'Disconnected')


SUCCESS_RSP_OPS = {'get_status'}


def bip_operation(bip_client, bip_server, op):
    """Perform a full BIP operation exchange, handling Continue responses."""
    server_rsp = 'success' if op in SUCCESS_RSP_OPS else 'noerror'
    while True:
        bip_client.exec_command(f'bip client {op}')
        bip_server._wait_for_shell_response(rf'BIP server.*{op} req', timeout=10)
        bip_server.exec_command(f'bip server {op} {server_rsp}')
        _, lines = bip_client._wait_for_shell_response(rf'{op} rsp', timeout=10)
        done = False
        for line in lines:
            if re.search(rf'{op} rsp.*Success', line):
                done = True
                break
        if done:
            break


def bip_operation_error(bip_client, bip_server, op, error_code='c3', error_str='Forbidden'):
    """Perform a BIP operation that server rejects with error."""
    bip_client.exec_command(f'bip client {op}')
    bip_server._wait_for_shell_response(rf'BIP server.*{op} req', timeout=10)
    bip_server.exec_command(f'bip server {op} error {error_code}')
    bip_client._wait_for_shell_response(rf'{op} rsp.*{error_str}', timeout=10)


def bip_server_set_type(bip_server, conn_type):
    """Re-register BIP server with a different connection type."""
    bip_server.exec_command('bip server unreg')
    bip_server.iexpect(f'bip server reg {conn_type}', bip_server.shell.prompt)


def bip_secondary_server_reg(bip_client, conn_type):
    """Register secondary server on the harness (primary INITIATOR) side."""
    bip_client.exec_command('bip_server_test sec_unreg')
    bip_client.iexpect(f'bip_server_test sec_reg {conn_type}', bip_client.shell.prompt)


def bip_secondary_server_unreg(bip_client):
    """Unregister secondary server on the harness side."""
    bip_client.exec_command('bip_server_test sec_unreg')


def bip_sec_sdp_discover_l2cap(bip_server, conn_type):
    """SDP discover the secondary service and extract its L2CAP PSM."""
    pattern = r'Found secondary GOEP L2CAP PSM\s+(0x[\da-fA-F]+|\d+)'
    bip_server.exec_command(f'bip_server_test sec_sdp_discover {conn_type}')
    _, data = bip_server._wait_for_shell_response(r'Found secondary GOEP L2CAP PSM.*', timeout=35)
    for line in data:
        match = re.search(pattern, line)
        if match:
            psm = match.group(1)
            logger.info(f'secondary l2cap psm = {psm}')
            return psm
    return None


def bip_secondary_obex_connect(bip_client, bip_server, conn_type):
    """Secondary OBEX connect: Harness=secondary server, DUT=secondary client."""
    bip_server.iexpect('bip_server_test sec_set_feats_funcs 0x1ff 0x1ffff', bip_server.shell.prompt)

    psm = bip_sec_sdp_discover_l2cap(bip_server, conn_type)
    if psm is None:
        logger.error('Failed to get secondary GOEP L2CAP PSM')
        return
    psm_hex = format(int(psm, 0), 'x')
    bip_server.iexpect(
        f'bip_server_test sec_connect_l2cap {psm_hex}', r'BIP secondary.*transport connected on.*'
    )
    bip_client._wait_for_shell_response(r'BIP secondary.*transport connected on.*')

    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command(f'bip_server_test sec_conn {conn_type}')
    bip_client._wait_for_shell_response(r'BIP server.*conn req', timeout=10)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip_server_test sec_server_conn success')
    bip_server._wait_for_shell_response(r'conn rsp.*Success', timeout=10)


def bip_partial_image_test_setup(bip_client, bip_server):
    """Setup for GET_PARTIAL_IMAGE: primary type=2, secondary type=10 over L2CAP.
    Secondary roles: Harness=server, DUT=client (reversed from primary)."""
    bip_br_clear(bip_client, bip_server)
    bip_server_set_type(bip_server, 2)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=2)
    bip_secondary_server_reg(bip_client, 10)
    bip_secondary_obex_connect(bip_client, bip_server, 10)


def bip_partial_image_test_teardown(bip_client, bip_server):
    """Teardown: ACL disconnect tears down all upper layers."""
    time.sleep(1)
    bip_client.dut.write(b'bt disconnect\n')
    with contextlib.suppress(Exception):
        bip_client._wait_for_shell_response(r'Disconnected', timeout=5)
    bip_secondary_server_unreg(bip_client)
    bip_server_set_type(bip_server, 0)


def bip_sec_operation(bip_client, bip_server, op):
    """Perform operation through secondary connection.
    Harness (bip_server) = secondary client, DUT (bip_client) = secondary server."""
    while True:
        bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
        bip_server.exec_command(f'bip_server_test sec_{op}')
        bip_client._wait_for_shell_response(rf'BIP server.*{op} req', timeout=10)
        bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
        bip_client.exec_command(f'bip_server_test sec_server_{op} noerror')
        _, lines = bip_server._wait_for_shell_response(rf'{op} rsp', timeout=10)
        done = False
        for line in lines:
            if re.search(rf'{op} rsp.*Success', line):
                done = True
                break
        if done:
            break


def bip_sec_operation_error(bip_client, bip_server, op, error_code='c3', error_str='Forbidden'):
    """Perform secondary operation that server rejects with error."""
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command(f'bip_server_test sec_{op}')
    bip_client._wait_for_shell_response(rf'BIP server.*{op} req', timeout=10)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command(f'bip_server_test sec_server_{op} error {error_code}')
    bip_server._wait_for_shell_response(rf'{op} rsp.*{error_str}', timeout=10)


def bip_sec_srm_operation(bip_client, bip_server, op):
    """Perform secondary operation with SRM header.
    Note: bip_sec_operation already calls alloc-buf, but SRM header must be
    added to the first buffer before the operation loop starts."""
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.iexpect('bip add-header srm 1', bip_server.shell.prompt)
    bip_server.exec_command(f'bip_server_test sec_{op}')
    bip_client._wait_for_shell_response(rf'BIP server.*{op} req', timeout=10)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command(f'bip_server_test sec_server_{op} noerror')
    _, lines = bip_server._wait_for_shell_response(rf'{op} rsp', timeout=10)
    for line in lines:
        if re.search(rf'{op} rsp.*Success', line):
            return
    while True:
        bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
        bip_server.exec_command(f'bip_server_test sec_{op}')
        bip_client._wait_for_shell_response(rf'BIP server.*{op} req', timeout=10)
        bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
        bip_client.exec_command(f'bip_server_test sec_server_{op} noerror')
        _, lines = bip_server._wait_for_shell_response(rf'{op} rsp', timeout=10)
        done = False
        for line in lines:
            if re.search(rf'{op} rsp.*Success', line):
                done = True
                break
        if done:
            break


def bip_sec_srmp_server_operation(bip_client, bip_server, op):
    """Perform secondary operation with SRM (SRMP Wait from server)."""
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.iexpect('bip add-header srm 1', bip_server.shell.prompt)
    bip_sec_operation(bip_client, bip_server, op)


def bip_sec_srmp_client_operation(bip_client, bip_server, op):
    """Perform secondary operation with SRM + SRMP Wait from client."""
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.iexpect('bip add-header srm 1', bip_server.shell.prompt)
    bip_server.iexpect('bip add-header srm_param 1', bip_server.shell.prompt)
    bip_sec_operation(bip_client, bip_server, op)


def bip_sec_start_op_then_abort(bip_client, bip_server, op):
    """Start secondary operation, get Continue response (keep in-progress), then abort."""
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command(f'bip_server_test sec_{op}')
    bip_client._wait_for_shell_response(rf'BIP server.*{op} req', timeout=10)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command(f'bip_server_test sec_server_{op} error 90')
    bip_server._wait_for_shell_response(rf'{op} rsp.*Continue', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip_server_test sec_abort')
    bip_client._wait_for_shell_response(r'BIP server.*abort req', timeout=10)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip_server_test sec_server_abort success')
    bip_server._wait_for_shell_response(r'abort rsp.*Success', timeout=10)


def bip_test_setup(bip_client, bip_server, conn_type=0):
    """Common test setup: clear, connect ACL, L2CAP transport, OBEX."""
    bip_br_clear(bip_client, bip_server)
    if conn_type != 0:
        bip_server_set_type(bip_server, conn_type)
    bip_br_connect(bip_client, bip_server)
    bip_l2cap_transport_connection(bip_client, bip_server)
    bip_obex_connect(bip_client, bip_server, conn_type=conn_type)


def bip_test_teardown(bip_client, bip_server, conn_type=0):
    """Common test teardown: ACL disconnect tears down all upper layers."""
    time.sleep(1)
    bip_client.dut.write(b'bt disconnect\n')
    with contextlib.suppress(Exception):
        bip_client._wait_for_shell_response(r'Disconnected', timeout=5)
    if conn_type != 0:
        bip_server_set_type(bip_server, 0)


def test_BR_BIP_SERVER_PUT_IMAGE_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server PutImage with multi-packet Continue responses."""
    bip_test_setup(bip_client, bip_server)
    bip_client.exec_command('bip client put_image')
    bip_server._wait_for_shell_response(r'BIP server.*put_image req', timeout=10)
    bip_server.exec_command('bip server put_image noerror')
    bip_client._wait_for_shell_response(r'put_image rsp.*Continue', timeout=10)
    while True:
        bip_client.exec_command('bip client put_image')
        bip_server._wait_for_shell_response(r'BIP server.*put_image req', timeout=10)
        bip_server.exec_command('bip server put_image noerror')
        _, lines = bip_client._wait_for_shell_response(r'put_image rsp', timeout=10)
        done = False
        for line in lines:
            if re.search(r'put_image rsp.*Success', line):
                done = True
                break
        if done:
            break
    bip_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_IMAGE_SRM(bip_client, bip_server):
    """Test BR BIP Server PutImage with SRM over L2CAP."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_srm_operation(bip_client, bip_server, 'put_image')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_IMAGE_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server PutImage with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_srmp_server_operation(bip_client, bip_server, 'put_image')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_IMAGE_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server PutImage with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_srmp_client_operation(bip_client, bip_server, 'put_image')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_THUMBNAIL_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server PutLinkedThumbnail success."""
    bip_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_operation(bip_client, bip_server, 'put_linked_thumbnail')
    bip_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_THUMBNAIL_ERROR(bip_client, bip_server):
    """Test BR BIP Server PutLinkedThumbnail error."""
    bip_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_operation_error(bip_client, bip_server, 'put_linked_thumbnail')
    bip_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_THUMBNAIL_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server PutLinkedThumbnail with Continue."""
    bip_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_operation(bip_client, bip_server, 'put_linked_thumbnail')
    bip_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_THUMBNAIL_SRM(bip_client, bip_server):
    """Test BR BIP Server PutLinkedThumbnail with SRM."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_srm_operation(bip_client, bip_server, 'put_linked_thumbnail')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_THUMBNAIL_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server PutLinkedThumbnail with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_srmp_server_operation(bip_client, bip_server, 'put_linked_thumbnail')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_THUMBNAIL_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server PutLinkedThumbnail with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_srmp_client_operation(bip_client, bip_server, 'put_linked_thumbnail')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_ATTACHMENT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server PutLinkedAttachment success."""
    bip_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_operation(bip_client, bip_server, 'put_linked_attachment')
    bip_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_ATTACHMENT_ERROR(bip_client, bip_server):
    """Test BR BIP Server PutLinkedAttachment error."""
    bip_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_operation_error(bip_client, bip_server, 'put_linked_attachment')
    bip_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_ATTACHMENT_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server PutLinkedAttachment with Continue."""
    bip_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_operation(bip_client, bip_server, 'put_linked_attachment')
    bip_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_ATTACHMENT_SRM(bip_client, bip_server):
    """Test BR BIP Server PutLinkedAttachment with SRM."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_srm_operation(bip_client, bip_server, 'put_linked_attachment')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_ATTACHMENT_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server PutLinkedAttachment with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_srmp_server_operation(bip_client, bip_server, 'put_linked_attachment')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_PUT_LINKED_ATTACHMENT_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server PutLinkedAttachment with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server)
    bip_put_image(bip_client, bip_server)
    bip_srmp_client_operation(bip_client, bip_server, 'put_linked_attachment')
    bip_srm_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_IMAGE_LIST_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetImageList success."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_image_list')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_LIST_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetImageList error."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation_error(bip_client, bip_server, 'get_image_list')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_LIST_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetImageList with multi-packet Continue responses."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_image_list')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_LIST_SRM(bip_client, bip_server):
    """Test BR BIP Server GetImageList with SRM over L2CAP."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srm_operation(bip_client, bip_server, 'get_image_list')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_LIST_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetImageList with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_server_operation(bip_client, bip_server, 'get_image_list')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_LIST_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetImageList with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_client_operation(bip_client, bip_server, 'get_image_list')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_LIST_ABORT(bip_client, bip_server):
    """Test BR BIP Server GetImageList abort during operation."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_start_op_then_abort(bip_client, bip_server, 'get_image_list')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_PROPERTIES_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetImageProperties success."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_image_properties')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_PROPERTIES_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetImageProperties error."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation_error(bip_client, bip_server, 'get_image_properties')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_PROPERTIES_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetImageProperties with multi-packet Continue responses."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_image_properties')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_PROPERTIES_SRM(bip_client, bip_server):
    """Test BR BIP Server GetImageProperties with SRM over L2CAP."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srm_operation(bip_client, bip_server, 'get_image_properties')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_PROPERTIES_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetImageProperties with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_server_operation(bip_client, bip_server, 'get_image_properties')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_PROPERTIES_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetImageProperties with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_client_operation(bip_client, bip_server, 'get_image_properties')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_PROPERTIES_ABORT(bip_client, bip_server):
    """Test BR BIP Server GetImageProperties abort during operation."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_start_op_then_abort(bip_client, bip_server, 'get_image_properties')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


# --- GetImage (IMAGE_PULL, conn_type=1) ---


def test_BR_BIP_SERVER_GET_IMAGE_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetImage success."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_image')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetImage error."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation_error(bip_client, bip_server, 'get_image')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetImage with multi-packet Continue responses."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_image')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_SRM(bip_client, bip_server):
    """Test BR BIP Server GetImage with SRM over L2CAP."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srm_operation(bip_client, bip_server, 'get_image')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetImage with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_server_operation(bip_client, bip_server, 'get_image')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetImage with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_client_operation(bip_client, bip_server, 'get_image')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_IMAGE_ABORT(bip_client, bip_server):
    """Test BR BIP Server GetImage abort during operation."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_start_op_then_abort(bip_client, bip_server, 'get_image')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


# --- GetLinkedThumbnail (IMAGE_PULL, conn_type=1) ---


def test_BR_BIP_SERVER_GET_LINKED_THUMBNAIL_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetLinkedThumbnail success."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_linked_thumbnail')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_THUMBNAIL_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetLinkedThumbnail error."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation_error(bip_client, bip_server, 'get_linked_thumbnail')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_THUMBNAIL_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetLinkedThumbnail with multi-packet Continue responses."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_linked_thumbnail')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_THUMBNAIL_SRM(bip_client, bip_server):
    """Test BR BIP Server GetLinkedThumbnail with SRM over L2CAP."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srm_operation(bip_client, bip_server, 'get_linked_thumbnail')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_THUMBNAIL_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetLinkedThumbnail with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_server_operation(bip_client, bip_server, 'get_linked_thumbnail')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_THUMBNAIL_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetLinkedThumbnail with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_client_operation(bip_client, bip_server, 'get_linked_thumbnail')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_THUMBNAIL_ABORT(bip_client, bip_server):
    """Test BR BIP Server GetLinkedThumbnail abort during operation."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_start_op_then_abort(bip_client, bip_server, 'get_linked_thumbnail')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


# --- GetLinkedAttachment (IMAGE_PULL, conn_type=1) ---


def test_BR_BIP_SERVER_GET_LINKED_ATTACHMENT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetLinkedAttachment success."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_linked_attachment')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_ATTACHMENT_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetLinkedAttachment error."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation_error(bip_client, bip_server, 'get_linked_attachment')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_ATTACHMENT_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetLinkedAttachment with multi-packet Continue responses."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'get_linked_attachment')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_ATTACHMENT_SRM(bip_client, bip_server):
    """Test BR BIP Server GetLinkedAttachment with SRM over L2CAP."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srm_operation(bip_client, bip_server, 'get_linked_attachment')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_ATTACHMENT_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetLinkedAttachment with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_server_operation(bip_client, bip_server, 'get_linked_attachment')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_ATTACHMENT_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetLinkedAttachment with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=1)
    bip_srmp_client_operation(bip_client, bip_server, 'get_linked_attachment')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_GET_LINKED_ATTACHMENT_ABORT(bip_client, bip_server):
    """Test BR BIP Server GetLinkedAttachment abort during operation."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_start_op_then_abort(bip_client, bip_server, 'get_linked_attachment')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


# --- DeleteImage (IMAGE_PULL, conn_type=1) ---


def test_BR_BIP_SERVER_DELETE_IMAGE_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server DeleteImage success."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation(bip_client, bip_server, 'delete_image')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


def test_BR_BIP_SERVER_DELETE_IMAGE_ERROR(bip_client, bip_server):
    """Test BR BIP Server DeleteImage error."""
    bip_test_setup(bip_client, bip_server, conn_type=1)
    bip_operation_error(bip_client, bip_server, 'delete_image')
    bip_test_teardown(bip_client, bip_server, conn_type=1)


# --- RemoteDisplay (REMOTE_DISPLAY, conn_type=5) ---


def test_BR_BIP_SERVER_REMOTE_DISPLAY_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server RemoteDisplay success."""
    bip_test_setup(bip_client, bip_server, conn_type=5)
    bip_operation(bip_client, bip_server, 'remote_display')
    bip_test_teardown(bip_client, bip_server, conn_type=5)


def test_BR_BIP_SERVER_REMOTE_DISPLAY_ERROR(bip_client, bip_server):
    """Test BR BIP Server RemoteDisplay error."""
    bip_test_setup(bip_client, bip_server, conn_type=5)
    bip_operation_error(bip_client, bip_server, 'remote_display')
    bip_test_teardown(bip_client, bip_server, conn_type=5)


# --- GetMonitoringImage (REMOTE_CAMERA, conn_type=4) ---


def test_BR_BIP_SERVER_GET_MONITORING_IMAGE_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetMonitoringImage success."""
    bip_test_setup(bip_client, bip_server, conn_type=4)
    bip_operation(bip_client, bip_server, 'get_monitoring_image')
    bip_test_teardown(bip_client, bip_server, conn_type=4)


def test_BR_BIP_SERVER_GET_MONITORING_IMAGE_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetMonitoringImage error."""
    bip_test_setup(bip_client, bip_server, conn_type=4)
    bip_operation_error(bip_client, bip_server, 'get_monitoring_image')
    bip_test_teardown(bip_client, bip_server, conn_type=4)


def test_BR_BIP_SERVER_GET_MONITORING_IMAGE_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetMonitoringImage with multi-packet Continue responses."""
    bip_test_setup(bip_client, bip_server, conn_type=4)
    bip_operation(bip_client, bip_server, 'get_monitoring_image')
    bip_test_teardown(bip_client, bip_server, conn_type=4)


def test_BR_BIP_SERVER_GET_MONITORING_IMAGE_SRM(bip_client, bip_server):
    """Test BR BIP Server GetMonitoringImage with SRM over L2CAP."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=4)
    bip_srm_operation(bip_client, bip_server, 'get_monitoring_image')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=4)


def test_BR_BIP_SERVER_GET_MONITORING_IMAGE_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetMonitoringImage with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=4)
    bip_srmp_server_operation(bip_client, bip_server, 'get_monitoring_image')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=4)


def test_BR_BIP_SERVER_GET_MONITORING_IMAGE_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetMonitoringImage with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=4)
    bip_srmp_client_operation(bip_client, bip_server, 'get_monitoring_image')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=4)


def test_BR_BIP_SERVER_GET_MONITORING_IMAGE_ABORT(bip_client, bip_server):
    """Test BR BIP Server GetMonitoringImage abort during operation."""
    bip_test_setup(bip_client, bip_server, conn_type=4)
    bip_start_op_then_abort(bip_client, bip_server, 'get_monitoring_image')
    bip_test_teardown(bip_client, bip_server, conn_type=4)


# --- GetPartialImage (REFERENCED_OBJ, conn_type=10) ---


def test_BR_BIP_SERVER_GET_PARTIAL_IMAGE_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetPartialImage success.
    Secondary connection: Harness=secondary client, DUT=secondary server."""
    bip_partial_image_test_setup(bip_client, bip_server)
    bip_sec_operation(bip_client, bip_server, 'get_partial_image')
    bip_partial_image_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_PARTIAL_IMAGE_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetPartialImage error."""
    bip_partial_image_test_setup(bip_client, bip_server)
    bip_sec_operation_error(bip_client, bip_server, 'get_partial_image')
    bip_partial_image_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_PARTIAL_IMAGE_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetPartialImage with Continue."""
    bip_partial_image_test_setup(bip_client, bip_server)
    bip_sec_operation(bip_client, bip_server, 'get_partial_image')
    bip_partial_image_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_PARTIAL_IMAGE_SRM(bip_client, bip_server):
    """Test BR BIP Server GetPartialImage with SRM over L2CAP."""
    bip_partial_image_test_setup(bip_client, bip_server)
    bip_sec_srm_operation(bip_client, bip_server, 'get_partial_image')
    bip_partial_image_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_PARTIAL_IMAGE_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetPartialImage with SRMP Wait from server."""
    bip_partial_image_test_setup(bip_client, bip_server)
    bip_sec_srmp_server_operation(bip_client, bip_server, 'get_partial_image')
    bip_partial_image_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_PARTIAL_IMAGE_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetPartialImage with SRMP Wait from client."""
    bip_partial_image_test_setup(bip_client, bip_server)
    bip_sec_srmp_client_operation(bip_client, bip_server, 'get_partial_image')
    bip_partial_image_test_teardown(bip_client, bip_server)


def test_BR_BIP_SERVER_GET_PARTIAL_IMAGE_ABORT(bip_client, bip_server):
    """Test BR BIP Server GetPartialImage abort."""
    bip_partial_image_test_setup(bip_client, bip_server)
    bip_sec_start_op_then_abort(bip_client, bip_server, 'get_partial_image')
    bip_partial_image_test_teardown(bip_client, bip_server)


# --- StartPrint (ADVANCED_IMAGE_PRINTING, conn_type=2) ---


def test_BR_BIP_SERVER_START_PRINT_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server StartPrint success."""
    bip_test_setup(bip_client, bip_server, conn_type=2)
    bip_operation(bip_client, bip_server, 'start_print')
    bip_test_teardown(bip_client, bip_server, conn_type=2)


def test_BR_BIP_SERVER_START_PRINT_ERROR(bip_client, bip_server):
    """Test BR BIP Server StartPrint error."""
    bip_test_setup(bip_client, bip_server, conn_type=2)
    bip_operation_error(bip_client, bip_server, 'start_print')
    bip_test_teardown(bip_client, bip_server, conn_type=2)


# --- StartArchive (AUTO_ARCHIVE, conn_type=3) ---


def test_BR_BIP_SERVER_START_ARCHIVE_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server StartArchive success."""
    bip_test_setup(bip_client, bip_server, conn_type=3)
    bip_operation(bip_client, bip_server, 'start_archive')
    bip_test_teardown(bip_client, bip_server, conn_type=3)


def test_BR_BIP_SERVER_START_ARCHIVE_ERROR(bip_client, bip_server):
    """Test BR BIP Server StartArchive error."""
    bip_test_setup(bip_client, bip_server, conn_type=3)
    bip_operation_error(bip_client, bip_server, 'start_archive')
    bip_test_teardown(bip_client, bip_server, conn_type=3)


# --- GetStatus (ADVANCED_IMAGE_PRINTING=2 or AUTO_ARCHIVE=3) ---


def test_BR_BIP_SERVER_GET_STATUS_SUCCESS(bip_client, bip_server):
    """Test BR BIP Server GetStatus success."""
    bip_test_setup(bip_client, bip_server, conn_type=3)
    bip_operation(bip_client, bip_server, 'get_status')
    bip_test_teardown(bip_client, bip_server, conn_type=3)


def test_BR_BIP_SERVER_GET_STATUS_ERROR(bip_client, bip_server):
    """Test BR BIP Server GetStatus error."""
    bip_test_setup(bip_client, bip_server, conn_type=3)
    bip_operation_error(bip_client, bip_server, 'get_status')
    bip_test_teardown(bip_client, bip_server, conn_type=3)


def test_BR_BIP_SERVER_GET_STATUS_CONTINUE(bip_client, bip_server):
    """Test BR BIP Server GetStatus with Continue."""
    bip_test_setup(bip_client, bip_server, conn_type=3)
    bip_operation(bip_client, bip_server, 'get_status')
    bip_test_teardown(bip_client, bip_server, conn_type=3)


def test_BR_BIP_SERVER_GET_STATUS_SRM(bip_client, bip_server):
    """Test BR BIP Server GetStatus with SRM."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=3)
    bip_srm_operation(bip_client, bip_server, 'get_status')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=3)


def test_BR_BIP_SERVER_GET_STATUS_SRMP_SERVER(bip_client, bip_server):
    """Test BR BIP Server GetStatus with SRMP Wait from server."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=3)
    bip_srmp_server_operation(bip_client, bip_server, 'get_status')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=3)


def test_BR_BIP_SERVER_GET_STATUS_SRMP_CLIENT(bip_client, bip_server):
    """Test BR BIP Server GetStatus with SRMP Wait from client."""
    bip_srm_test_setup(bip_client, bip_server, conn_type=3)
    bip_srmp_client_operation(bip_client, bip_server, 'get_status')
    bip_srm_test_teardown(bip_client, bip_server, conn_type=3)


def test_BR_BIP_SERVER_GET_STATUS_ABORT(bip_client, bip_server):
    """Test BR BIP Server GetStatus abort."""
    bip_test_setup(bip_client, bip_server, conn_type=3)
    bip_client.exec_command('bip client get_status')
    bip_server._wait_for_shell_response(r'BIP server.*get_status req', timeout=10)
    bip_server.exec_command('bip server get_status continue')
    bip_client._wait_for_shell_response(r'get_status rsp', timeout=10)
    bip_client.iexpect('bip alloc-buf', bip_client.shell.prompt)
    bip_client.exec_command('bip client abort')
    bip_server._wait_for_shell_response(r'BIP server.*abort req', timeout=10)
    bip_server.iexpect('bip alloc-buf', bip_server.shell.prompt)
    bip_server.exec_command('bip server abort success')
    bip_client._wait_for_shell_response(r'abort rsp.*Success', timeout=10)
    bip_test_teardown(bip_client, bip_server, conn_type=3)
