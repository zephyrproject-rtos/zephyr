# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging
import re
import time

logger = logging.getLogger(__name__)

JUST_WORKS = 0
PASSKEY_ENTRY = 1
PASSKEY_DISPLAY = 2
PASSKEY_CONFIRM = 3

CONNECT_RETRIES = 3


def sm_get_pairing_method(local_sec, local_io_cap, remote_sec, remote_io_cap):
    pairing_method = {
        'display': {
            'display': JUST_WORKS,
            'yesno': JUST_WORKS,
            'input': PASSKEY_ENTRY,
            'none': JUST_WORKS,
        },
        'yesno': {
            'display': JUST_WORKS,
            'yesno': PASSKEY_CONFIRM,
            'input': PASSKEY_ENTRY,
            'none': JUST_WORKS,
        },
        'input': {
            'display': PASSKEY_DISPLAY,
            'yesno': PASSKEY_DISPLAY,
            'input': PASSKEY_ENTRY,
            'none': JUST_WORKS,
        },
        'none': {
            'display': JUST_WORKS,
            'yesno': JUST_WORKS,
            'input': JUST_WORKS,
            'none': JUST_WORKS,
        },
    }
    method = pairing_method[remote_io_cap][local_io_cap]

    if remote_sec < 3 and local_sec < 3:
        method = JUST_WORKS

    return method


def sm_ctkd_pre(handle, del_br_key=True, del_le_key=True, advertise=True, io_cap='yesno'):
    if del_br_key:
        handle.exec_command('br clear all')

    if del_le_key:
        handle.exec_command('bt clear all')

    if handle.io_cap != io_cap:
        handle.io_cap = io_cap
        handle.exec_command('bt auth none')
        handle.exec_command(f'bt auth {io_cap}')

    if advertise is True:
        handle.bt_advertise()

    handle.get_random_address()


def sm_ctkd_reset_board(handle):
    handle.io_cap = 'none'
    handle.reset()
    handle.bt_init()
    handle.bt_connectable()
    handle.bt_discoverable()


def list_count(lines, str):
    return [str in line for line in lines].count(True)


def le_addrs(init, rsp, key_exist):
    """Return the LE addresses of init and rsp: public identity once bonded, else random."""
    if key_exist is True:
        return f'P:{init.pub_addr}', f'P:{rsp.pub_addr}'
    return f'R:{init.rnd_addr}', f'R:{rsp.rnd_addr}'


def sm_ctkd_wait_passkey(handle):
    regex = r"Confirm passkey for .*?: (?P<passkey>\d{6})"
    found, lines = handle.wait_for_shell_response(regex)
    assert found is True, 'no passkey confirmation request'
    for line in lines:
        searched = re.search(regex, line)
        if searched:
            return int(searched.group("passkey"))
    raise AssertionError('passkey not found in the confirmation request')


def sm_ctkd_passkey_confirm(init, rsp):
    passkey_c = sm_ctkd_wait_passkey(init)
    passkey_s = sm_ctkd_wait_passkey(rsp)
    assert passkey_c == passkey_s, f'passkey mismatch: {passkey_c} != {passkey_s}'
    init.exec_command('bt auth-passkey-confirm')
    rsp.exec_command('bt auth-passkey-confirm')


def _sm_ctkd_br_connect(init, rsp):
    # BR connection
    init.exec_command(f'br connect {rsp.pub_addr}')
    found, _ = init.wait_for_shell_response(f'Connected: {rsp.pub_addr}')
    assert found is True
    found, _ = rsp.wait_for_shell_response(f'Connected: {init.pub_addr}')
    assert found is True


def _sm_ctkd_le_connect(init, rsp, key_exist=False):
    init_addr, rsp_addr = le_addrs(init, rsp, key_exist)

    # LE connection
    init.exec_command(f'bt connect {rsp_addr}')
    found, _ = init.wait_for_shell_response(f'Connected: {rsp_addr}')
    assert found is True
    found, _ = rsp.wait_for_shell_response(f'Connected: {init_addr}')
    assert found is True


def sm_ctkd_br_connect(init, rsp):
    for attempt in range(CONNECT_RETRIES):
        try:
            _sm_ctkd_br_connect(init, rsp)
            return
        except AssertionError:
            if attempt == CONNECT_RETRIES - 1:
                raise
            logger.info('Retry BR connection')
            time.sleep(5)


def sm_ctkd_le_connect(init, rsp, key_exist=False):
    for attempt in range(CONNECT_RETRIES):
        try:
            _sm_ctkd_le_connect(init, rsp, key_exist)
            return
        except AssertionError:
            if attempt == CONNECT_RETRIES - 1:
                raise
            logger.info('Retry LE connection')
            time.sleep(5)


def sm_ctkd_br_security(init, rsp, dut_sec, key_exist=False, derive_key=True):
    pairing_method = sm_get_pairing_method(dut_sec, init.io_cap, dut_sec, rsp.io_cap)

    # BR security
    init.exec_command(f'bt security {dut_sec}')
    if key_exist is False:
        if pairing_method == PASSKEY_CONFIRM:
            sm_ctkd_passkey_confirm(init, rsp)
        elif rsp.io_cap != 'none':
            found, _ = rsp.wait_for_shell_response('Confirm pairing')
            assert found is True
            rsp.exec_command('bt auth-pairing-confirm')

    expected = [f"Security changed: {rsp.pub_addr} level {dut_sec}"]
    if key_exist is False:
        expected.append(f"Bonded with {rsp.pub_addr}")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True
    if key_exist is True:
        found = init.check_shell_response(lines, f"Bonded with {rsp.pub_addr}")
        assert found is not key_exist
    else:
        if derive_key is True and list_count(lines, f"Bonded with {rsp.pub_addr}") < 2:
            found, _ = init.wait_for_shell_response(f"Bonded with {rsp.pub_addr}")
            assert found is True

    expected = [f"Security changed: {init.pub_addr} level {dut_sec}"]
    if key_exist is False:
        expected.append(f"Bonded with {init.pub_addr}")
    found, lines = rsp.wait_for_shell_response(expected)
    assert found is True
    if key_exist is True:
        found = rsp.check_shell_response(lines, f"Bonded with {init.pub_addr}")
        assert found is not key_exist
    else:
        if derive_key is True and list_count(lines, f"Bonded with {init.pub_addr}") < 2:
            found, _ = rsp.wait_for_shell_response(f"Bonded with {init.pub_addr}")
            assert found is True


def sm_ctkd_le_security(init, rsp, dut_sec, key_exist=False, derive_key=True):
    pairing_method = sm_get_pairing_method(dut_sec, init.io_cap, dut_sec, rsp.io_cap)

    init_addr, rsp_addr = le_addrs(init, rsp, key_exist)

    # LE security
    init.exec_command(f'bt security {dut_sec}')
    if key_exist is False and pairing_method == PASSKEY_CONFIRM:
        sm_ctkd_passkey_confirm(init, rsp)

    expected = [f"Security changed: {rsp_addr} level {dut_sec}"]
    if key_exist is False:
        expected.append(f"Bonded with P:{rsp.pub_addr}")
    found, lines = init.wait_for_shell_response(expected)
    assert found is True
    if key_exist is True:
        found = init.check_shell_response(lines, f"Bonded with P:{rsp.pub_addr}")
        assert found is not key_exist

    expected = [f"Security changed: {init_addr} level {dut_sec}"]
    if key_exist is False:
        expected.append(f"Bonded with P:{init.pub_addr}")
    found, lines = rsp.wait_for_shell_response(expected)
    assert found is True
    if key_exist is True:
        found = rsp.check_shell_response(lines, f"Bonded with P:{init.pub_addr}")
        assert found is not key_exist


def sm_ctkd_br_disconnect(init, rsp):
    init.exec_command('bt disconnect')
    found, _ = init.wait_for_shell_response(f'Disconnected: {rsp.pub_addr}')
    assert found is True
    found, _ = rsp.wait_for_shell_response(f'Disconnected: {init.pub_addr}')
    assert found is True


def sm_ctkd_le_disconnect(init, rsp):
    init.exec_command(f'bt disconnect P:{rsp.pub_addr}')
    found, _ = init.wait_for_shell_response(f'Disconnected: P:{rsp.pub_addr}')
    assert found is True
    found, _ = rsp.wait_for_shell_response(f'Disconnected: P:{init.pub_addr}')
    assert found is True


def sm_ctkd_select_le_conn(init, rsp, is_rsp=True):
    init.exec_command(f'bt select P:{rsp.pub_addr}')


def sm_ctkd_select_br_conn(init, rsp, is_rsp=True):
    init.exec_command(f'br select {rsp.pub_addr}')


def sm_ctkd_lk_to_ltk(init, rsp, dut_sec):
    sm_ctkd_pre(init, del_br_key=True, del_le_key=True, advertise=False)
    sm_ctkd_pre(rsp, del_br_key=True, del_le_key=True, advertise=True)
    sm_ctkd_br_connect(init, rsp)
    sm_ctkd_br_security(init, rsp, dut_sec=dut_sec, key_exist=False)
    sm_ctkd_le_connect(init, rsp, key_exist=True)
    sm_ctkd_le_security(init, rsp, dut_sec=dut_sec, key_exist=True)
    sm_ctkd_le_disconnect(init, rsp)
    sm_ctkd_br_disconnect(init, rsp)


def sm_ctkd_ltk_to_lk(init, rsp, dut_sec):
    sm_ctkd_pre(init, del_br_key=True, del_le_key=True, advertise=False)
    sm_ctkd_pre(rsp, del_br_key=True, del_le_key=True, advertise=True)
    sm_ctkd_le_connect(init, rsp, key_exist=False)
    sm_ctkd_le_security(init, rsp, dut_sec=dut_sec, key_exist=False)
    sm_ctkd_br_connect(init, rsp)
    sm_ctkd_br_security(init, rsp, dut_sec=dut_sec, key_exist=True)
    sm_ctkd_br_disconnect(init, rsp)
    sm_ctkd_le_disconnect(init, rsp)


def test_sm_ctkd_lk_to_ltk(client, server):
    """To verify that after pairing on BR/EDR, an LE connection can be established and
    encrypted without explicit LE pairing, using an LTK derived from the BR/EDR Link Key."""
    sm_ctkd_lk_to_ltk(client, server, 2)


def test_sm_ctkd_ltk_to_lk(client, server):
    """To verify that after pairing on LE, a BR/EDR connection can be established and
    encrypted without explicit BR/EDR pairing, using a Link Key derived from the LE LTK."""
    sm_ctkd_ltk_to_lk(client, server, 2)


def test_sm_ctkd_lk_to_ltk_persistence(client, server):
    """To verify that an LTK derived from BR/EDR Link Key persists after a power cycle and
    can be used to encrypt an LE connection without re-pairing."""
    sm_ctkd_lk_to_ltk(client, server, 2)
    sm_ctkd_reset_board(client)
    sm_ctkd_reset_board(server)
    sm_ctkd_pre(client, del_br_key=False, del_le_key=False, advertise=False)
    sm_ctkd_pre(server, del_br_key=False, del_le_key=False, advertise=True)
    sm_ctkd_le_connect(client, server, key_exist=True)
    sm_ctkd_le_security(client, server, 2, key_exist=True)
    sm_ctkd_le_disconnect(client, server)


def test_sm_ctkd_ltk_to_lk_persistence(client, server):
    """To verify that a Link Key derived from LE LTK persists after a power cycle and
    can be used to encrypt a BR/EDR connection without re-pairing."""
    sm_ctkd_ltk_to_lk(client, server, 2)
    sm_ctkd_reset_board(client)
    sm_ctkd_reset_board(server)
    sm_ctkd_pre(client, del_br_key=False, del_le_key=False, advertise=False)
    sm_ctkd_pre(server, del_br_key=False, del_le_key=False, advertise=False)
    sm_ctkd_br_connect(client, server)
    sm_ctkd_br_security(client, server, 2, key_exist=True)
    sm_ctkd_br_disconnect(client, server)


def test_sm_ctkd_lk_update_ltk(client, server):
    """To verify that when a BR/EDR Link Key is updated with a higher security level,
    the derived LE LTK is also updated accordingly."""
    sm_ctkd_lk_to_ltk(client, server, 2)
    # Delete BR key
    sm_ctkd_pre(client, del_br_key=True, del_le_key=False, advertise=False)
    sm_ctkd_pre(server, del_br_key=True, del_le_key=False, advertise=True)
    sm_ctkd_br_connect(client, server)
    sm_ctkd_br_security(client, server, 4, key_exist=False)
    sm_ctkd_le_connect(client, server, key_exist=True)
    sm_ctkd_le_security(client, server, 4, key_exist=True)
    sm_ctkd_le_disconnect(client, server)
    sm_ctkd_br_disconnect(client, server)


def test_sm_ctkd_ltk_update_lk(client, server):
    """To verify that when an LE LTK is updated with a higher security level,
    the derived BR/EDR Link Key is also updated accordingly."""
    sm_ctkd_ltk_to_lk(client, server, 2)
    # Delete LE key
    sm_ctkd_pre(client, del_br_key=False, del_le_key=True, advertise=False)
    sm_ctkd_pre(server, del_br_key=False, del_le_key=True, advertise=True)
    sm_ctkd_le_connect(client, server, key_exist=False)
    sm_ctkd_le_security(client, server, 4, key_exist=False)
    sm_ctkd_br_connect(client, server)
    sm_ctkd_br_security(client, server, 4, key_exist=True)
    sm_ctkd_br_disconnect(client, server)
    sm_ctkd_le_disconnect(client, server)


def test_sm_ctkd_001(client, server):
    """To verify that when a high security level (Level 4) pairing is established over LE first,
    a subsequent lower security level (Level 2) pairing over BR/EDR does not downgrade the security
    of the LE connection by overwriting the LTK."""
    sm_ctkd_ltk_to_lk(client, server, 4)
    # Delete BR key
    sm_ctkd_pre(client, del_br_key=True, del_le_key=False, advertise=False)
    sm_ctkd_pre(server, del_br_key=True, del_le_key=False, advertise=True)
    sm_ctkd_br_connect(client, server)
    sm_ctkd_br_security(client, server, 2, key_exist=False, derive_key=False)
    sm_ctkd_le_connect(client, server, key_exist=True)
    sm_ctkd_le_security(client, server, 4, key_exist=True)
    sm_ctkd_le_disconnect(client, server)
    sm_ctkd_br_disconnect(client, server)


def test_sm_ctkd_002(client, server):
    """To verify that when a high security level (Level 4) pairing is established over LE first,
    a subsequent lower security level (Level 2) pairing over BR/EDR does not downgrade the security
    of the LE connection by overwriting the LTK. This test verifies this behavior from the
    rsp's perspective."""
    sm_ctkd_ltk_to_lk(server, client, 4)
    # Delete BR key
    sm_ctkd_pre(client, del_br_key=True, del_le_key=False, advertise=True)
    sm_ctkd_pre(server, del_br_key=True, del_le_key=False, advertise=False)
    sm_ctkd_br_connect(server, client)
    sm_ctkd_br_security(server, client, 2, key_exist=False, derive_key=False)
    sm_ctkd_le_connect(server, client, key_exist=True)
    sm_ctkd_le_security(server, client, 4, key_exist=True)
    sm_ctkd_le_disconnect(server, client)
    sm_ctkd_br_disconnect(server, client)


def test_sm_ctkd_003(client, server):
    """To verify that when a high security level (Level 4) pairing is established over BR first,
    a subsequent lower security level (Level 2) pairing over LE does not downgrade the security
    of the BR connection by overwriting the LK."""
    sm_ctkd_lk_to_ltk(client, server, 4)
    # Delete LE key and change IO Capability to No Input No Output
    sm_ctkd_pre(client, del_br_key=False, del_le_key=True, advertise=False, io_cap='none')
    sm_ctkd_pre(server, del_br_key=False, del_le_key=True, advertise=True, io_cap='none')
    sm_ctkd_le_connect(client, server, key_exist=False)
    sm_ctkd_le_security(client, server, 2, key_exist=False, derive_key=False)
    # Change IO Capability to Display Yes/No
    sm_ctkd_pre(client, del_br_key=False, del_le_key=False, advertise=False, io_cap='yesno')
    sm_ctkd_pre(server, del_br_key=False, del_le_key=False, advertise=False, io_cap='yesno')
    sm_ctkd_br_connect(client, server)
    sm_ctkd_br_security(client, server, 4, key_exist=True)
    sm_ctkd_br_disconnect(client, server)
    sm_ctkd_le_disconnect(client, server)


def test_sm_ctkd_004(client, server):
    """To verify that when a high security level (Level 4) pairing is established over BR/EDR first,
    a subsequent lower security level (Level 2) pairing over LE does not downgrade the security
    of the BR/EDR connection by overwriting the LK. This test verifies this behavior from the
    rsp's perspective."""
    sm_ctkd_lk_to_ltk(server, client, 4)
    # Delete LE key and change IO Capability to No Input No Output
    sm_ctkd_pre(client, del_br_key=False, del_le_key=True, advertise=True, io_cap='none')
    sm_ctkd_pre(server, del_br_key=False, del_le_key=True, advertise=False, io_cap='none')
    sm_ctkd_le_connect(server, client, key_exist=False)
    sm_ctkd_le_security(server, client, 2, key_exist=False, derive_key=False)
    # Change IO Capability to Display Yes/No
    sm_ctkd_pre(client, del_br_key=False, del_le_key=False, advertise=False, io_cap='yesno')
    sm_ctkd_pre(server, del_br_key=False, del_le_key=False, advertise=False, io_cap='yesno')
    sm_ctkd_br_connect(server, client)
    sm_ctkd_br_security(server, client, 4, key_exist=True)
    sm_ctkd_br_disconnect(server, client)
    sm_ctkd_le_disconnect(server, client)


def test_sm_ctkd_005(client, server):
    """To verify that when a BR connection is established firstly, a subsequent BLE connection
    is established secondly, the BR SMP CTKD occur thirdly, the BLE connection can combine with
    the derived LTK."""
    sm_ctkd_pre(client, del_br_key=True, del_le_key=True, advertise=True, io_cap='none')
    sm_ctkd_pre(server, del_br_key=True, del_le_key=True, advertise=False, io_cap='none')
    sm_ctkd_br_connect(server, client)
    sm_ctkd_le_connect(server, client, key_exist=False)
    sm_ctkd_select_br_conn(server, client)
    sm_ctkd_br_security(server, client, 2, key_exist=False, derive_key=True)
    sm_ctkd_select_le_conn(server, client)
    sm_ctkd_le_security(server, client, 2, key_exist=True, derive_key=False)
    sm_ctkd_le_disconnect(server, client)
    sm_ctkd_select_br_conn(server, client)
    sm_ctkd_br_disconnect(server, client)


def test_sm_ctkd_006(client, server):
    """To verify that when a BR connection is established firstly, a subsequent BLE connection
    is established secondly, the BR SMP CTKD occur thirdly, the BLE connection can combine with
    the derived LTK; Then change the IRK and do the above flow again."""
    sm_ctkd_pre(client, del_br_key=True, del_le_key=True, advertise=True, io_cap='none')
    sm_ctkd_pre(server, del_br_key=True, del_le_key=True, advertise=False, io_cap='none')
    sm_ctkd_br_connect(server, client)
    sm_ctkd_le_connect(server, client, key_exist=False)
    sm_ctkd_select_br_conn(server, client)
    sm_ctkd_br_security(server, client, 2, key_exist=False, derive_key=True)
    sm_ctkd_select_le_conn(server, client)
    sm_ctkd_le_security(server, client, 2, key_exist=True, derive_key=False)

    sm_ctkd_le_disconnect(server, client)
    sm_ctkd_select_br_conn(server, client)
    sm_ctkd_br_disconnect(server, client)

    server.exec_command("test_smp delete_irk")
    sm_ctkd_reset_board(server)
    sm_ctkd_pre(client, del_le_key=False, del_br_key=False, advertise=True, io_cap='none')
    sm_ctkd_pre(server, del_le_key=True, del_br_key=True, advertise=False, io_cap='none')
    sm_ctkd_br_connect(server, client)
    # LE connection
    server.exec_command(f'bt connect R:{client.rnd_addr}')
    found, _ = server.wait_for_shell_response(f'Connected: R:{client.rnd_addr}')
    assert found is True
    found, _ = client.wait_for_shell_response(f'Connected: P:{server.pub_addr}')
    assert found is True
    sm_ctkd_select_br_conn(server, client)
    sm_ctkd_br_security(server, client, 2, key_exist=False, derive_key=True)
    sm_ctkd_select_le_conn(server, client)
    sm_ctkd_le_security(server, client, 2, key_exist=True, derive_key=False)

    sm_ctkd_le_disconnect(server, client)
    sm_ctkd_select_br_conn(server, client)
    sm_ctkd_br_disconnect(server, client)


def test_sm_ctkd_role_switch(client, server):
    """To verify that when a BR connection is established firstly, then switch role secondly,
    the BR SMP CTKD occur thirdly, the CTKD works rightly."""
    sm_ctkd_pre(client, del_br_key=True, del_le_key=True, advertise=True, io_cap='none')
    sm_ctkd_pre(server, del_br_key=True, del_le_key=True, advertise=False, io_cap='none')
    sm_ctkd_br_connect(client, server)
    client.exec_command('br switch-role peripheral')
    expected = [re.escape("Role changed (HCI status 0x00)")]
    expected.append("Current role is: peripheral")
    found, _ = client.wait_for_shell_response(expected)
    assert found is True
    expected = [re.escape("Role changed (HCI status 0x00)")]
    expected.append("Current role is: central")
    found, _ = server.wait_for_shell_response(expected)
    assert found is True
    sm_ctkd_br_security(server, client, 2, key_exist=False, derive_key=True)
    sm_ctkd_le_connect(server, client, key_exist=True)
    sm_ctkd_le_security(server, client, 2, key_exist=True, derive_key=False)

    sm_ctkd_le_disconnect(server, client)
    sm_ctkd_br_disconnect(server, client)
