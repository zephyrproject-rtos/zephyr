# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging
import time

logger = logging.getLogger(__name__)

l2cap_server_psm = 0x1001

CONNECT_RETRIES = 3


def sm_pre(handle, io_cap):
    handle.exec_command('br clear all')
    handle.exec_command('bt clear all')
    handle.exec_command('bt auth none')
    handle.exec_command(f'bt auth {io_cap}')


def sm_br_register_l2cap_server(handle, psm, sec):
    handle.exec_command(f"l2cap_br register {format(psm, 'x')}")
    handle.exec_command(f"l2cap_br security {format(psm, 'x')} {sec}")


def _sm_br_connect(client, server):
    client.exec_command(f'br connect {server.pub_addr}')
    found, _ = client.wait_for_shell_response(f'Connected: {server.pub_addr}')
    assert found is True
    found, _ = server.wait_for_shell_response(f'Connected: {client.pub_addr}')
    assert found is True


def sm_br_connect(client, server):
    for attempt in range(CONNECT_RETRIES):
        try:
            _sm_br_connect(client, server)
            return
        except AssertionError:
            if attempt == CONNECT_RETRIES - 1:
                raise
            logger.info('Retry BR connection')
            time.sleep(5)


def sm_br_disconnect(client, server):
    client.exec_command('bt disconnect')
    found, _ = client.wait_for_shell_response(f'Disconnected: {server.pub_addr}')
    assert found is True
    found, _ = server.wait_for_shell_response(f'Disconnected: {client.pub_addr}')
    assert found is True


def test_sm_init_020(client, server_1, server_2):
    """Verify that an initiator can handle multiple concurrent
    pairing processes with different devices."""
    dut_sec = 2
    expected = []
    sm_pre(client, 'display')
    sm_pre(server_1, 'display')
    sm_pre(server_2, 'display')

    sm_br_register_l2cap_server(server_1, l2cap_server_psm, dut_sec)
    sm_br_register_l2cap_server(server_2, l2cap_server_psm, dut_sec)

    # Create ACL connection and start pairing
    sm_br_connect(client, server_1)
    client.exec_command(f'bt security {dut_sec}')
    sm_br_connect(client, server_2)
    client.exec_command(f'bt security {dut_sec}')

    found, _ = server_1.wait_for_shell_response('Confirm pairing')
    assert found is True
    server_1.exec_command('bt auth-pairing-confirm')
    found, _ = server_2.wait_for_shell_response('Confirm pairing')
    assert found is True
    server_2.exec_command('bt auth-pairing-confirm')

    expected.clear()
    expected.append(f"Security changed: {server_1.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {server_1.pub_addr}")
    expected.append(f"Security changed: {server_2.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {server_2.pub_addr}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    expected.clear()
    expected.append(f"Security changed: {client.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {client.pub_addr}")
    found, _ = server_1.wait_for_shell_response(expected)
    assert found is True

    expected.clear()
    expected.append(f"Security changed: {client.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {client.pub_addr}")
    found, _ = server_2.wait_for_shell_response(expected)
    assert found is True

    # Create L2CAP connection
    client.exec_command(f"conn_br select {server_1.pub_addr}")
    client.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}")
    found, _ = client.wait_for_shell_response(r"Channel \w+ connected")
    assert found is True
    found, _ = server_1.wait_for_shell_response(r"Channel \w+ connected")
    assert found is True

    client.exec_command(f"conn_br select {server_2.pub_addr}")
    client.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}")
    found, _ = client.wait_for_shell_response(r"Channel \w+ connected")
    assert found is True
    found, _ = server_2.wait_for_shell_response(r"Channel \w+ connected")
    assert found is True

    # Disconnection
    sm_br_disconnect(client, server_2)
    sm_br_disconnect(client, server_1)


def test_sm_init_031(client, server_1, server_2, server_3):
    """Verify that the initiator handles resource constraints correctly
    during multiple simultaneous connections."""
    dut_sec = 2
    expected = []
    sm_pre(client, 'none')
    sm_pre(server_1, 'none')
    sm_pre(server_2, 'none')
    sm_pre(server_3, 'none')

    sm_br_register_l2cap_server(server_1, l2cap_server_psm, dut_sec)
    sm_br_register_l2cap_server(server_2, l2cap_server_psm, dut_sec)

    # Create connection with server 1
    sm_br_connect(client, server_1)
    client.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}")

    expected.clear()
    expected.append(f"Security changed: {server_1.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {server_1.pub_addr}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True
    expected.clear()
    expected.append(f"Security changed: {client.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {client.pub_addr}")
    found, _ = server_1.wait_for_shell_response(expected)
    assert found is True

    # Create connection with server 2
    sm_br_connect(client, server_2)
    client.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}")

    expected.clear()
    expected.append(f"Security changed: {server_2.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {server_2.pub_addr}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True
    expected.clear()
    expected.append(f"Security changed: {client.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {client.pub_addr}")
    found, _ = server_2.wait_for_shell_response(expected)
    assert found is True

    # Create connection with server 3
    lines = client.exec_command(f'br connect {server_3.pub_addr}')
    assert client.check_shell_response(lines, "Connection failed")

    # Disconnection
    sm_br_disconnect(client, server_2)
    sm_br_disconnect(client, server_1)

    # Make sure client can connect to server 3
    sm_br_connect(client, server_3)
    client.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}")

    expected.clear()
    expected.append(f"Security changed: {server_3.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {server_3.pub_addr}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True
    expected.clear()
    expected.append(f"Security changed: {client.pub_addr} level {dut_sec}")
    expected.append(f"Bonded with {client.pub_addr}")
    found, _ = server_3.wait_for_shell_response(expected)
    assert found is True

    # Disconnection
    sm_br_disconnect(client, server_3)
