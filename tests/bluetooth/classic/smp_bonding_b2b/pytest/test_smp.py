# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging
import time

logger = logging.getLogger(__name__)

l2cap_server_psm = 0x1001

CONNECT_RETRIES = 3


def sm_pre(handle):
    handle.exec_command('br clear all')
    handle.exec_command('bt clear all')
    handle.exec_command('bt auth none')


def sm_br_register_l2cap_server(handle, psm, sec):
    handle.exec_command(f"l2cap_br register {format(psm, 'x')}")
    handle.exec_command(f"l2cap_br security {format(psm, 'x')} {sec}")


def _sm_br_connect(client, server):
    client.exec_command(f'br connect {server.addr}')
    found, _ = client.wait_for_shell_response(f'Connected: {server.addr}')
    assert found is True
    found, _ = server.wait_for_shell_response(f'Connected: {client.addr}')
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
    found, _ = client.wait_for_shell_response(f'Disconnected: {server.addr}')
    assert found is True
    found, _ = server.wait_for_shell_response(f'Disconnected: {client.addr}')
    assert found is True


def test_sm_bonding_init_004(client, server):
    # init: general bonding, l2cap client
    # rsp: non-bondable, l2cap server
    # expect: pairing fail.

    dut_sec = 2
    sm_pre(client)
    sm_pre(server)
    sm_br_register_l2cap_server(server, l2cap_server_psm, dut_sec)

    # Responder as l2cap server: non-bondable
    server.exec_command("bt bondable off")

    # Initiator setup: l2cap client: General Bonding
    client.exec_command("bt bondable on")

    # Initiator connect
    sm_br_connect(client, server)

    # Initiator l2cap connect
    client.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}")

    expected = [f"Pairing failed with {server.addr} reason: Pairing not allowed"]
    expected.append(r"Channel \w+ disconnected")
    expected.append(f"Disconnected: {server.addr}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    expected = [f"Pairing failed with {client.addr} reason: Authentication failure"]
    expected.append("Invalid remote bonding requirements")
    expected.append(f"Disconnected: {client.addr}")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True


def test_sm_bonding_init_007(client, server):
    # init: dedicated bonding
    # rsp: non-bondable
    # expect: pairing fail.

    dut_sec = 2
    sm_pre(client)
    sm_pre(server)

    # Responder: non-bondable
    server.exec_command("bt bondable off")

    # Initiator: dedicated Bonding
    client.exec_command("bt bondable on")

    # Initiator connect
    sm_br_connect(client, server)

    # Initiator security
    client.exec_command(f"bt security {dut_sec}")

    expected = [f"Pairing failed with {server.addr} reason: Pairing not allowed"]
    expected.append(f"Disconnected: {server.addr}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    expected = [f"Pairing failed with {client.addr} reason: Authentication failure"]
    expected.append("Invalid remote bonding requirements")
    expected.append(f"Disconnected: {client.addr}")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True


def test_sm_bonding_init_009(client, server):
    # init: dedicated bonding
    # rsp: dedicated bonding
    # expect: bonding success.

    dut_sec = 2
    sm_pre(client)
    sm_pre(server)

    # Responder: dedicated Bonding
    server.exec_command("bt bondable on")

    # Initiator: dedicated Bonding
    client.exec_command("bt bondable on")

    # Initiator connect
    sm_br_connect(client, server)

    # Initiator security
    client.exec_command(f"bt security {dut_sec}")

    expected = [f"Bonded with {server.addr}"]
    expected.append(f"Security changed: {server.addr} level {dut_sec}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    expected = [f"Bonded with {client.addr}"]
    expected.append(f"Security changed: {client.addr} level {dut_sec}")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    # Initiator disconnect
    sm_br_disconnect(client, server)

    # Initiator check bonded
    lines = client.exec_command("br bonds")
    assert client.check_shell_response(lines, f"Remote Identity: {server.addr}")


def test_sm_bonding_rsp_012(client, server):
    # init: dedicated bonding
    # rsp: non-bondable
    # expect: pairing fail.

    # Responder: non-bondable
    dut_sec = 2
    sm_pre(client)
    sm_pre(server)

    client.exec_command("bt bondable off")

    # Initiator: dedicated Bonding
    server.exec_command("bt bondable on")

    # Initiator connect
    sm_br_connect(server, client)

    # Initiator security
    server.exec_command(f"bt security {dut_sec}")

    expected = [f"Pairing failed with {client.addr} reason: Pairing not allowed"]
    expected.append(f"Disconnected: {client.addr}")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    expected = [f"Pairing failed with {server.addr} reason: Authentication failure"]
    expected.append("Invalid remote bonding requirements")
    expected.append(f"Disconnected: {server.addr}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True


def test_sm_bonding_rsp_018(client, server):
    # init: dedicated bonding
    # rsp: dedicated bonding
    # expect: bonding success.

    dut_sec = 2
    sm_pre(client)
    sm_pre(server)

    # Responder: non-bondable
    client.exec_command("bt bondable on")

    # Initiator: dedicated Bonding
    server.exec_command("bt bondable on")

    # Initiator connect
    sm_br_connect(server, client)

    # Initiator security
    server.exec_command(f"bt security {dut_sec}")

    expected = [f"Bonded with {client.addr}"]
    expected.append(f"Security changed: {client.addr} level {dut_sec}")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True

    expected = [f"Bonded with {server.addr}"]
    expected.append(f"Security changed: {server.addr} level {dut_sec}")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True

    # Initiator disconnect
    sm_br_disconnect(server, client)

    # Initiator check bonded
    lines = server.exec_command("br bonds")
    assert server.check_shell_response(lines, f"Remote Identity: {client.addr}")
