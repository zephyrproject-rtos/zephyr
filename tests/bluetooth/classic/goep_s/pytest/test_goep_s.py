# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import re
import time

from conftest import (
    GOEP_RFCOMM_CHANNEL,
    logger,
)

GOEP_L2CAP_PSM = 11


def goep_br_clear(goep_s, goep_c):
    goep_s.exec_command("br clear all")
    goep_c.exec_command("br clear all")


def goep_br_connect(goep_s, goep_c):
    retry = 3
    logger.info(f'acl connect {goep_c.addr}')
    while retry > 0:
        try:
            goep_s.iexpect(f'br connect {goep_c.addr}', 'Connected', timeout=35)
            break
        except Exception:
            time.sleep(5)
            retry -= 1
            if retry > 0:
                logger.info('Retry BR connection')
            continue


def test_BR_GOEP_SERVER_TRANSPORT_RFCOMM_ACCEPT_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response(r'GOEP.*transport connected')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_TRANSPORT_RFCOMM_DISCONNECT_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response(r'GOEP.*transport connected')

    goep_s.iexpect('goep disconnect-rfcomm', r'GOEP.*transport disconnected')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_OBEX_CONNECT_NO_AUTH_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_OBEX_CONNECT_AUTH_CHALLENGE_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_s.exec_command("goep server reg")

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    pwd = '0000'

    goep_c.exec_command('goep client conn ff')

    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep_s_test add_auth_req {pwd}')
    goep_s.exec_command('goep server conn error ff c1')

    goep_c._wait_for_shell_response(r'Get auth challenge header, should add auth rsp header')
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep_s_test add_auth_rsp {pwd}')
    goep_c.exec_command('goep client conn ff')

    goep_s._wait_for_shell_response(r'Authentication successful')
    goep_s.exec_command('goep server conn success ff')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_OBEX_CONNECT_AUTH_RESPONSE_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_s.exec_command("goep server reg")

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    pwd = '0000'

    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep_s_test add_auth_req {pwd}')
    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response('Get auth challenge header')

    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep_s_test add_auth_rsp {pwd}')
    goep_s.exec_command('goep server conn success ff')

    goep_c._wait_for_shell_response(r'Authentication successful')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_PUT_SUCCESS_FINAl_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    message = "test_message"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_c.exec_command('goep client put true')

    _, lines = goep_s._wait_for_shell_response(f'{message}')
    found = False
    for line in lines:
        if re.search(r"OBEX server.*put req, final true", line):
            found = True
            break
    assert found is True

    goep_s.exec_command('goep server put success')
    goep_c._wait_for_shell_response(r'OBEX client.*put rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_PUT_CONTINUE_NON_FINAL_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    message = "test_message_continue"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_c.exec_command('goep client put false')

    _, lines = goep_s._wait_for_shell_response(f'{message}')
    found = False
    for line in lines:
        if re.search(r"OBEX server.*put req, final false", line):
            found = True
            break
    assert found is True

    goep_s.exec_command('goep server put continue')
    goep_c._wait_for_shell_response(r'OBEX client.*put rsp, rsp_code Continue')

    message = "test_message_final"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_c.exec_command('goep client put true')

    _, lines = goep_s._wait_for_shell_response(f'{message}')
    found = False
    for line in lines:
        if re.search(r"OBEX server.*put req, final true", line):
            found = True
            break
    assert found is True

    goep_s.exec_command('goep server put success')
    goep_c._wait_for_shell_response(r'OBEX client.*put rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_PUT_ERROR_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    message = "test_message"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_c.exec_command('goep client put true')

    _, lines = goep_s._wait_for_shell_response(f'{message}')
    found = False
    for line in lines:
        if re.search(r"OBEX server.*put req, final true", line):
            found = True
            break
    assert found is True

    goep_s.exec_command('goep server put error c3')
    goep_c._wait_for_shell_response(r'OBEX client.*put rsp, rsp_code Forbidden')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_PUT_SRM_ENABLE_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    srm = 1
    message = "test_message"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header srm {srm}')
    goep_c.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_c.exec_command('goep client put true')

    goep_s._wait_for_shell_response(f'srm : {srm}')
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header srm {srm}')
    goep_s.exec_command('goep server put success')
    goep_c._wait_for_shell_response(f'srm : {srm}')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_PUT_SRM_PARAM_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    srm = 1
    srmp = 1
    message = "test_message"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header srm {srm}')
    goep_c.exec_command(f'goep add-header srm_param {srmp}')
    goep_c.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_c.exec_command('goep client put true')

    goep_s._wait_for_shell_response(f'srmp : {srmp}')
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header srm {srm}')
    goep_s.exec_command(f'goep add-header srm_param {srmp}')
    goep_s.exec_command('goep server put success')
    goep_c._wait_for_shell_response(f'srmp : {srmp}')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_GET_SUCCESS_FINAL_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    goep_c.exec_command('goep client get true')

    goep_s._wait_for_shell_response(r'OBEX server.*get req, final true')

    message = "test_get_message"
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_s.exec_command('goep server get success')

    goep_c._wait_for_shell_response(f'end of body : {message}')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_GET_CONTINUE_NON_FINAL_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    goep_c.exec_command('goep client get true')

    goep_s._wait_for_shell_response(r'OBEX server.*get req, final true')

    message = "test_message_continue"
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header body {(message + "\0").encode("utf-8").hex()}')
    goep_s.exec_command('goep server get continue')

    _, lines = goep_c._wait_for_shell_response(f'body : {message}')
    found = False
    for line in lines:
        if re.search(r"rsp_code Continue", line):
            found = True
            break
    assert found is True

    goep_c.exec_command('goep client get true')
    goep_s._wait_for_shell_response(r'OBEX server.*get req, final true')

    message = "test_message_success"
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_s.exec_command('goep server get success')

    _, lines = goep_c._wait_for_shell_response(f'end of body : {message}')
    found = False
    for line in lines:
        if re.search(r"rsp_code Success", line):
            found = True
            break
    assert found is True

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_GET_NOT_FOUND_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    goep_c.exec_command('goep client get true')

    goep_s._wait_for_shell_response(r'OBEX server.*get req, final true')
    goep_s.exec_command('goep server get error c4')

    goep_c._wait_for_shell_response('rsp_code Not Found')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_GET_SRM_ENABLE_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    srm = 1
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header srm {srm}')
    goep_c.exec_command('goep client get true')

    goep_s._wait_for_shell_response(f'srm : {srm}')
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header srm {srm}')
    goep_s.exec_command('goep server get success')
    goep_c._wait_for_shell_response(f'srm : {srm}')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_GET_SRM_PARAM_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    srm = 1
    srmp = 1
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header srm {srm}')
    goep_c.exec_command(f'goep add-header srm_param {srmp}')
    goep_c.exec_command('goep client get true')

    goep_s._wait_for_shell_response(f'srmp : {srmp}')
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header srm {srm}')
    goep_s.exec_command(f'goep add-header srm_param {srmp}')
    goep_s.exec_command('goep server get success')
    goep_c._wait_for_shell_response(f'srmp : {srmp}')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_ACTION_COPY_SUCCESS_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    action_id = 0
    source_name = "source"
    dest_name = "dest"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header action_id {action_id}')
    goep_c.exec_command(
        f'goep add-header name {''.join([f'{ord(char):04x}' for char in (source_name + '\0')])}'
    )
    goep_c.exec_command(
        f'goep add-header dest_name {''.join([f'{ord(char):04x}' for char in (dest_name + '\0')])}'
    )
    goep_c.exec_command('goep client action true')

    _, lines = goep_s._wait_for_shell_response(f'dest name : {dest_name}')
    found_action_id = False
    found_source_name = False
    found_dest_name = False
    for line in lines:
        if f"action_id : {action_id}" in line:
            found_action_id = True
        elif f"name : {source_name}" in line:
            found_source_name = True
        elif f"dest name : {dest_name}" in line:
            found_dest_name = True

    assert found_action_id is True and found_dest_name is True and found_source_name is True

    goep_s.exec_command('goep server action success')
    goep_c._wait_for_shell_response(r'OBEX client.*action rsp, rsp_code Success')
    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_ACTION_MOVE_SUCCESS_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    action_id = 1
    source_name = "source"
    dest_name = "dest"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header action_id {action_id}')
    goep_c.exec_command(
        f'goep add-header name {''.join([f'{ord(char):04x}' for char in (source_name + '\0')])}'
    )
    goep_c.exec_command(
        f'goep add-header dest_name {''.join([f'{ord(char):04x}' for char in (dest_name + '\0')])}'
    )
    goep_c.exec_command('goep client action true')

    _, lines = goep_s._wait_for_shell_response(f'dest name : {dest_name}')
    found_action_id = False
    found_source_name = False
    found_dest_name = False
    for line in lines:
        if f"action_id : {action_id}" in line:
            found_action_id = True
        elif f"name : {source_name}" in line:
            found_source_name = True
        elif f"dest name : {dest_name}" in line:
            found_dest_name = True

    assert found_action_id is True and found_dest_name is True and found_source_name is True

    goep_s.exec_command('goep server action success')
    goep_c._wait_for_shell_response(r'OBEX client.*action rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_ACTION_PERMISSION_SUCCESS_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    action_id = 2
    source_name = "source"
    dest_name = "dest"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header action_id {action_id}')
    goep_c.exec_command(
        f'goep add-header name {''.join([f'{ord(char):04x}' for char in (source_name + '\0')])}'
    )
    goep_c.exec_command(
        f'goep add-header dest_name {''.join([f'{ord(char):04x}' for char in (dest_name + '\0')])}'
    )
    goep_c.exec_command('goep client action true')

    _, lines = goep_s._wait_for_shell_response(f'dest name : {dest_name}')
    found_action_id = False
    found_source_name = False
    found_dest_name = False
    for line in lines:
        if f"action_id : {action_id}" in line:
            found_action_id = True
        elif f"name : {source_name}" in line:
            found_source_name = True
        elif f"dest name : {dest_name}" in line:
            found_dest_name = True

    assert found_action_id is True and found_dest_name is True and found_source_name is True

    goep_s.exec_command('goep server action success')
    goep_c._wait_for_shell_response(r'OBEX client.*action rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_ACTION_NOT_FOUND_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    action_id = 2
    source_name = "source"
    dest_name = "dest"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header action_id {action_id}')
    goep_c.exec_command(
        f'goep add-header name {''.join([f'{ord(char):04x}' for char in (source_name + '\0')])}'
    )
    goep_c.exec_command(
        f'goep add-header dest_name {''.join([f'{ord(char):04x}' for char in (dest_name + '\0')])}'
    )
    goep_c.exec_command('goep client action true')

    _, lines = goep_s._wait_for_shell_response(f'dest name : {dest_name}')
    found_action_id = False
    found_source_name = False
    found_dest_name = False
    for line in lines:
        if f"action_id : {action_id}" in line:
            found_action_id = True
        elif f"name : {source_name}" in line:
            found_source_name = True
        elif f"dest name : {dest_name}" in line:
            found_dest_name = True

    assert found_action_id is True and found_dest_name is True and found_source_name is True

    goep_s.exec_command('goep server action error c4')
    goep_c._wait_for_shell_response(r'OBEX client.*action rsp, rsp_code Not Found')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_ACTION_SRM_ENABLE_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    srm = 1
    action_id = 2
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header srm {srm}')
    goep_c.exec_command(f'goep add-header action_id {action_id}')

    goep_c.exec_command('goep client action true')

    goep_s._wait_for_shell_response(f'srm : {srm}')
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header srm {srm}')
    goep_s.exec_command('goep server action success')
    goep_c._wait_for_shell_response(f'srm : {srm}')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_ACTION_SRM_PARAM_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    srm = 1
    srmp = 1
    action_id = 2
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header srm {srm}')
    goep_c.exec_command(f'goep add-header srm_param {srmp}')
    goep_c.exec_command(f'goep add-header action_id {action_id}')

    goep_c.exec_command('goep client action true')

    goep_s._wait_for_shell_response(f'srmp : {srmp}')
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header srm {srm}')
    goep_s.exec_command(f'goep add-header srm_param {srmp}')
    goep_s.exec_command('goep server action success')
    goep_c._wait_for_shell_response(f'srmp : {srmp}')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_ABORT_ACTIVE_REQUEST_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    message = "test_message_continue"
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header end_body {(message + "\0").encode("utf-8").hex()}')
    goep_c.exec_command('goep client put false')

    goep_s._wait_for_shell_response(f'{message}')

    goep_s.exec_command('goep server put continue')
    goep_c._wait_for_shell_response(r'OBEX client.*put rsp, rsp_code Continue')

    goep_c.exec_command('goep client abort')

    goep_s._wait_for_shell_response(r'OBEX server.*abort req')

    goep_s.exec_command('goep server abort success')
    goep_c._wait_for_shell_response(r'OBEX client.*abort rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_SETPATH_ROOT_SUCCESS_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    goep_c.exec_command('goep client setpath')

    Flag = 2
    _, lines = goep_s._wait_for_shell_response('set path to root')
    found = False
    for line in lines:
        if f'flags {Flag}' in line:
            found = True
            break
    assert found is True

    goep_s.exec_command('goep server setpath success')
    goep_c._wait_for_shell_response(r'OBEX client.*setpath rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_SETPATH_DOWN_SUCCESS_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    child_folder = 'child'
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(
        f'goep add-header name {''.join([f'{ord(char):04x}' for char in (child_folder + '\0')])}'
    )
    goep_c.exec_command('goep client setpath')

    goep_s._wait_for_shell_response(f'set path to child folder : {child_folder}')

    goep_s.exec_command('goep server setpath success')
    goep_c._wait_for_shell_response(r'OBEX client.*setpath rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_SETPATH_UP_SUCCESS_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    goep_c.exec_command('goep client setpath parent')
    goep_s._wait_for_shell_response('set path to parent folder')

    goep_s.exec_command('goep server setpath success')
    goep_c._wait_for_shell_response(r'OBEX client.*setpath rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_SETPATH_NO_CREATE_FAIL_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    child_folder = 'child'
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(
        f'goep add-header name {''.join([f'{ord(char):04x}' for char in (child_folder + '\0')])}'
    )
    goep_c.exec_command('goep client setpath')

    goep_s._wait_for_shell_response(f'set path to child folder : {child_folder}')

    goep_s.exec_command('goep server setpath error c4')
    goep_c._wait_for_shell_response(r'OBEX client.*setpath rsp, rsp_code Not Found')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_SETPATH_FORBIDDEN_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    child_folder = 'child'
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(
        f'goep add-header name {''.join([f'{ord(char):04x}' for char in (child_folder + '\0')])}'
    )
    goep_c.exec_command('goep client setpath')

    goep_s._wait_for_shell_response(f'set path to child folder : {child_folder}')

    goep_s.exec_command('goep server setpath error c3')
    goep_c._wait_for_shell_response(r'OBEX client.*setpath rsp, rsp_code Forbidden')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_OBEX_DISCONNECT_SUCCESS_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    goep_c.exec_command('goep client disconn')
    goep_s._wait_for_shell_response(r'OBEX server.*disconn req')
    goep_s.exec_command('goep server disconn success')
    goep_c._wait_for_shell_response(r'OBEX client.*disconn rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_OBEX_DISCONNECT_WITH_CONN_ID_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-rfcomm {GOEP_RFCOMM_CHANNEL}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response('GOEP.*transport connected')

    target = '1234567812345678'
    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header target {target.encode().hex()}')
    goep_c.exec_command('goep client conn ff')
    goep_s._wait_for_shell_response(r'OBEX server.*conn req, version 10, mopl 00ff')

    conn_id = 1
    goep_s.exec_command('goep alloc-buf')
    goep_s.exec_command(f'goep add-header who {target.encode().hex()}')
    goep_s.exec_command(f'goep add-header conn_id {conn_id}')
    goep_s.exec_command('goep server conn success ff')
    goep_c._wait_for_shell_response(
        r'OBEX client.*conn rsp, rsp_code Success, version 10, mopl 00ff'
    )

    goep_c.exec_command('goep alloc-buf')
    goep_c.exec_command(f'goep add-header who {target.encode().hex()}')
    goep_c.exec_command(f'goep add-header conn_id {conn_id}')
    goep_c.exec_command('goep client disconn')
    goep_s._wait_for_shell_response(f'connection id : {conn_id}')
    goep_s.exec_command('goep server disconn success')
    goep_c._wait_for_shell_response(r'OBEX client.*disconn rsp, rsp_code Success')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_TRANSPORT_L2CAP_ACCEPT_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_s.exec_command('goep_s_test release_goep_app')
    goep_c.exec_command('goep_s_test release_goep_app')

    goep_s.exec_command(f'goep register-l2cap {GOEP_L2CAP_PSM}')

    goep_c.iexpect(f'goep connect-l2cap {GOEP_L2CAP_PSM}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response(r'GOEP.*transport connected')

    goep_s.iexpect('bt disconnect', 'Disconnected')


def test_BR_GOEP_SERVER_TRANSPORT_L2CAP_DISCONNECT_001(goep_s, goep_c):
    goep_br_clear(goep_s, goep_c)

    goep_br_connect(goep_s, goep_c)

    goep_c.iexpect(f'goep connect-l2cap {GOEP_L2CAP_PSM}', r'GOEP.*transport connected')
    goep_s._wait_for_shell_response(r'GOEP.*transport connected')

    goep_c.iexpect('goep disconnect-l2cap', r'GOEP.*transport disconnected')
    goep_s._wait_for_shell_response(r'GOEP.*transport disconnected')

    goep_s.iexpect('bt disconnect', 'Disconnected')
