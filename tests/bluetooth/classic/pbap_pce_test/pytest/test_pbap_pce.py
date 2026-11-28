# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code


import re
import time

from conftest import (
    logger,
)

PROMOT = None
mopl = 300


def bt_obex_rsp_code_to_str(rsp_code):
    """Convert OBEX response code to string representation.

    Args:
        rsp_code: OBEX response code (int)

    Returns:
        String representation of the response code
    """
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


BT_PBAP_PULL_PHONEBOOK_TYPE = "x-bt/phonebook"
BT_PBAP_PULL_VCARD_LISTING_TYPE = "x-bt/vcard-listing"
BT_PBAP_PULL_VCARD_ENTRY_TYPE = "x-bt/vcard"


def pbap_br_clear(pbap_pce, pbap_pse):
    pbap_pse.exec_command("br clear all")
    global PROMOT
    PROMOT = pbap_pse.shell.prompt
    pbap_pce.exec_command("br clear all")


def pbap_br_connect(pbap_pce, pbap_pse):
    retry = 3
    logger.info(f'acl connect {pbap_pse.addr}')
    while retry > 0:
        try:
            pbap_pce.iexpect(f'br connect {pbap_pse.addr}', 'Connected', timeout=35)
            break
        except Exception:
            time.sleep(5)
            retry -= 1
            if retry > 0:
                logger.info('Retry BR connection')
            continue


def pbap_pce_get_l2cap_psm(pbap_pce):
    pattern = r'PSE l2cap psm param\s+(0x[\da-fA-F]+)'
    pbap_pce.exec_command('pbap pce sdp_discover')
    _, data = pbap_pce._wait_for_shell_response(r'PSE l2cap psm param.*', timeout=35)
    for line in data:
        match = re.search(pattern, line)
        if match:
            logger.info(f'psm = {match.group(1)}')
            return match.group(1)
    return None


def pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse):
    psm = pbap_pce_get_l2cap_psm(pbap_pce)
    if psm is None:
        logger.error('Failed to get PSE L2CAP PSM')
        return

    pbap_pce.iexpect(f'pbap pce connect_l2cap {psm}', r'PBAP PCE.*l2cap transport connected on.*')
    pbap_pse._wait_for_shell_response(r'PBAP PSE.*l2cap transport connected on.*')


def pbap_pce_create_obex_connection(pbap_pce, pbap_pse):
    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap connect version.*mopl.*', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} success', PROMOT)

    pbap_pce._wait_for_shell_response(
        r'Connection established successfully \(no auth required\)', timeout=5
    )


def test_BR_PBAP_PCE_L2CAP_CONNECT_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE L2CAP connection success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_L2CAP_CONNECT_FAILURE(pbap_pce, pbap_pse):
    """Test BR PBAP PCE L2CAP connection fail."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    psm = pbap_pce_get_l2cap_psm(pbap_pce)
    if psm is None:
        logger.error('Failed to get PSE L2CAP PSM')
        return
    err_psm = hex(int(psm, 16) - 1)
    pbap_pce.iexpect(f'pbap pce connect_l2cap {err_psm}', r'Fail to send transport connect')
    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_L2CAP_DISCONNECT_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE L2CAP disconnection success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap pce disconnect_l2cap', r'PBAP PCE.*l2cap transport disconnected')
    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_OBEX_CONNECT_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE OBEX connection success."""
    # Clear and establish BR connection
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)

    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap connect version.*mopl.*', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} success', PROMOT)

    pbap_pce._wait_for_shell_response(
        r'Connection established successfully \(no auth required\)', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_OBEX_CONNECT_ERROR(pbap_pce, pbap_pse):
    """Test BR PBAP PCE OBEX connection error."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap connect version.*mopl.*', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    error_code = 0xC4
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} error {hex(error_code)}', PROMOT)

    pbap_pce._wait_for_shell_response(r'Connection may have failed', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_OBEX_CONNECT_AUTH_CLIENT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE OBEX connection with client authentication."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    password = 'test1234'
    pbap_pce.iexpect(f'pbap add_header_auth_challenge {password}', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(
        r'PCE authentication required - add auth_response to connect_rsp tx_buf', timeout=5
    )
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap add_header_auth_response {password}', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} success', PROMOT)

    pbap_pce._wait_for_shell_response(r'Connection established with authentication', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_OBEX_CONNECT_AUTH_SERVER(pbap_pce, pbap_pse):
    """Test BR PBAP PCE OBEX connection with server authentication."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    password = 'test1234'
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap connect version.*mopl.*', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap add_header_auth_challenge {password}', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} unauth', PROMOT)

    pbap_pce._wait_for_shell_response(r'PSE requires authentication', timeout=5)
    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect(f'pbap add_header_auth_response {password}', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(r'auth success', timeout=5)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} success', PROMOT)

    pbap_pce._wait_for_shell_response(r'Connection established successfully', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_OBEX_CONNECT_AUTH_MUTUAL(pbap_pce, pbap_pse):
    """Test BR PBAP PCE OBEX connection with mutual authentication."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    password = 'test1234'
    pbap_pce.iexpect(f'pbap add_header_auth_challenge {password}', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(
        r'PCE authentication required - add auth_response to connect_rsp tx_buf', timeout=5
    )
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap add_header_auth_challenge {password}', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} unauth', PROMOT)

    pbap_pce._wait_for_shell_response(r'PSE requires authentication', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect(f'pbap add_header_auth_response {password}', PROMOT)
    pbap_pce.iexpect(f'pbap add_header_auth_challenge {password}', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(r'auth success', timeout=5)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap add_header_auth_response {password}', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} success', PROMOT)

    pbap_pce._wait_for_shell_response(r'Connection established with authentication', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_OBEX_CONNECT_AUTH_FAILURE(pbap_pce, pbap_pse):
    """Test BR PBAP PCE OBEX connection with authentication failure."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    mopl = '48000'
    password_pce = 'test1234'
    password_pse = 'wrong5678'
    pbap_pce.iexpect(f'pbap add_header_auth_challenge {password_pce}', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(
        r'PCE authentication required - add auth_response to connect_rsp tx_buf', timeout=5
    )
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap add_header_auth_response {password_pse}', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} success', PROMOT)

    pbap_pce._wait_for_shell_response(r'Authentication verification failed')

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_OBEX_DISCONNECT_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE OBEX disconnect success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap pce disconnect', PROMOT)
    pbap_pse._wait_for_shell_response(r'pbap disconnect requested by pce', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse disconnect_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'pbap disconnect result Success', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_OBEX_DISCONNECT_ERROR(pbap_pce, pbap_pse):
    """Test BR PBAP PCE OBEX disconnect error."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap pce disconnect', PROMOT)
    pbap_pse._wait_for_shell_response(r'pbap disconnect requested by pce', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    error_code = 0xC4
    pbap_pse.iexpect(f'pbap pse disconnect_rsp error {hex(error_code)}', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap disconnect result {bt_obex_rsp_code_to_str(error_code)}', timeout=5
    )

    pbap_pce.iexpect('pbap pce disconnect_l2cap', r'PBAP PCE.*l2cap transport disconnected')
    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_ABORT_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE abort success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap connect version.*mopl.*', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} success', PROMOT)

    pbap_pce._wait_for_shell_response(r'Connection established successfully', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror srmp', PROMOT)

    pbap_pce._wait_for_shell_response(r'please send pull cmd again')

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce abort', PROMOT)

    pbap_pse._wait_for_shell_response(r'receive abort req')
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse abort_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'abort success')

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_ABORT_ERROR(pbap_pce, pbap_pse):
    """Test BR PBAP PCE abort error."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect(f'pbap pce connect {mopl}', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap connect version.*mopl.*', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(f'pbap pse connect_rsp {mopl} success', PROMOT)

    pbap_pce._wait_for_shell_response(r'Connection established successfully', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror srmp', PROMOT)

    pbap_pce._wait_for_shell_response(r'please send pull cmd again', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce abort', PROMOT)

    pbap_pse._wait_for_shell_response(r'receive abort req', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    error_code = 0xC4
    pbap_pse.iexpect(
        f'pbap pse abort_rsp error {hex(error_code)}', r'PBAP.*l2cap transport disconnected'
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_PHONEBOOK_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull phonebook success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Continue', timeout=5
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_PHONEBOOK_ERROR(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull phonebook error."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    error_code = 0xC4
    pbap_pse.iexpect(f'pbap pse pull_phone_book_rsp error {hex(error_code)}', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result {bt_obex_rsp_code_to_str(error_code)}',
        timeout=5,
    )

    pbap_pce.iexpect('pbap pce disconnect_l2cap', r'PBAP PCE.*l2cap transport disconnected')
    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_PHONEBOOK_CONTINUE(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull phonebook continue."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(
        'pbap pse pull_phone_book_rsp noerror',
        'Keep sending responses continuously until rsp_code is success',
    )

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Continue', timeout=5
    )
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_PHONEBOOK_SRMP_SERVER_INIT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull phonebook with SRMP."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror srmp', PROMOT)

    pbap_pce._wait_for_shell_response(r'get header srmp success', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_PHONEBOOK_SRMP_CLIENT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull phonebook with client SRMP."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf srmp', PROMOT)

    pbap_pse._wait_for_shell_response(r'get header srmp success', timeout=5)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(r'please send pull cmd again', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_PHONEBOOK_ABORT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull phonebook abort."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)

    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_phone_book request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Continue', timeout=5
    )

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce abort', PROMOT)

    pbap_pse._wait_for_shell_response(r'receive abort req', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse abort_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'abort success', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_PHONEBOOK_PARAM_ALL(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull phonebook with all application parameters."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    # Add all application parameters
    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap add_ap MaxListCount 0x000A', PROMOT)
    pbap_pce.iexpect('pbap add_ap ListStartOffset 0x0000', PROMOT)
    pbap_pce.iexpect('pbap add_ap PropertySelector 0x000000000000000F', PROMOT)
    pbap_pce.iexpect('pbap add_ap Format v2.1', PROMOT)
    pbap_pce.iexpect('pbap add_ap vCardSelector 0x0000000000000001', PROMOT)
    pbap_pce.iexpect('pbap add_ap vCardSelectorOperator or', PROMOT)
    pbap_pce.iexpect('pbap add_ap ResetNewMissedCalls', PROMOT)
    pbap_pce.iexpect('pbap pce pull_pb telecom/pb.vcf', PROMOT)

    _, lines = pbap_pse._wait_for_shell_response(r'ResetNewMissedCalls', timeout=10)
    expected_patterns = [
        r'MaxListCount: 0x000a \(10\)',
        r'ListStartOffset: 0x0000 \(0\)',
        r'PropertySelector: 0x000000000000000f',
        r'Format: 0x00 \(vCard 2\.1\)',
        r'vCardSelector: 0x0000000000000001',
        r'vCardSelectorOperator: 0x00 \(OR\)',
        r'ResetNewMissedCalls: 0x01',
    ]

    matched_patterns = []
    for pattern in expected_patterns:
        pattern_matched = False
        for line in lines:
            if re.search(pattern, line):
                matched_patterns.append(pattern)
                pattern_matched = True
                logger.info(f'Pattern matched: {pattern} in line: {line}')
                break
        if not pattern_matched:
            logger.warning(f'Pattern NOT matched: {pattern}')

    assert len(matched_patterns) == len(expected_patterns), (
        f'Only {len(matched_patterns)}/{len(expected_patterns)} patterns matched'
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Continue', timeout=5
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_phone_book_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_PHONEBOOK_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_SETPATH_ROOT_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE setpath to root success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce set_phone_book 0x02', PROMOT)

    pbap_pse._wait_for_shell_response(r'set phonebook to ROOT folder', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse set_phone_book_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'PBAP set phonebook result Success', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_SETPATH_DOWN_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE setpath down success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce set_phone_book 0x02 telecom', PROMOT)

    pbap_pse._wait_for_shell_response(r'set phonebook to children telecom folder', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse set_phone_book_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'PBAP set phonebook result Success', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_SETPATH_UP_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE setpath up success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce set_phone_book 0x02 telecom', PROMOT)

    pbap_pse._wait_for_shell_response(r'set phonebook to children telecom folder', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse set_phone_book_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'PBAP set phonebook result Success', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce set_phone_book 0x03', PROMOT)

    pbap_pse._wait_for_shell_response(r'set phonebook to parent folder', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse set_phone_book_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'PBAP set phonebook result Success', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_SETPATH_ERROR(pbap_pce, pbap_pse):
    """Test BR PBAP PCE setpath error."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce set_phone_book 0x02 tele', PROMOT)

    pbap_pse._wait_for_shell_response(r'set phonebook to children tele folder', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    err_code = 0xC4
    pbap_pse.iexpect(f'pbap pse set_phone_book_rsp error {hex(err_code)}', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'PBAP set phonebook result {bt_obex_rsp_code_to_str(err_code)}', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDLISTING_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard listing success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_listing request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_listing_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Continue', timeout=5
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_listing_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDLISTING_ERROR(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard listing error."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_listing request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    err_code = 0xC4
    pbap_pse.iexpect(f'pbap pse pull_vcard_listing_rsp error {hex(err_code)}', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result {bt_obex_rsp_code_to_str(err_code)}',
        timeout=5,
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDLISTING_CONTINUE(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard listing continue."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_listing request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(
        'pbap pse pull_vcard_listing_rsp noerror',
        'Keep sending responses continuously until rsp_code is success',
    )

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Continue', timeout=5
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_listing_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDLISTING_SRMP_SERVER_INIT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard listing with server SRMP."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_listing request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(
        'pbap pse pull_vcard_listing_rsp noerror srmp',
        'Suspend after sending a single response and await the PCE request',
    )

    pbap_pce._wait_for_shell_response(r'please send pull cmd again', timeout=5)
    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_listing request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_listing_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDLISTING_SRMP_CLIENT_INIT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard listing with client SRMP."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb srmp', PROMOT)

    pbap_pse._wait_for_shell_response(r'get header srmp success', timeout=5)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(
        'pbap pse pull_vcard_listing_rsp noerror',
        'Suspend after sending a single response and await the PCE request',
    )

    pbap_pce._wait_for_shell_response(r'please send pull cmd again', timeout=5)
    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_listing request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_listing_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Success', timeout=5
    )
    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDLISTING_ABORT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard listing abort."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_listing request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_listing_rsp noerror srmp', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Continue', timeout=5
    )

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce abort', PROMOT)

    pbap_pse._wait_for_shell_response(r'receive abort req', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse abort_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'abort success', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDLISTING_PARAM_ALL(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard listing with all application parameters."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    # Add all application parameters for PullvCardListing
    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap add_ap Order alphanumeric', PROMOT)
    pbap_pce.iexpect('pbap add_ap SearchValue John', PROMOT)
    pbap_pce.iexpect('pbap add_ap SearchAttribute name', PROMOT)
    pbap_pce.iexpect('pbap add_ap MaxListCount 0x0014', PROMOT)
    pbap_pce.iexpect('pbap add_ap ListStartOffset 0x0000', PROMOT)
    pbap_pce.iexpect('pbap add_ap vCardSelector 0x0000000000000003', PROMOT)
    pbap_pce.iexpect('pbap add_ap vCardSelectorOperator or', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_listing pb', PROMOT)

    _, lines = pbap_pse._wait_for_shell_response(r'vCardSelectorOperator', timeout=10)
    expected_patterns = [
        r'Order: 0x01 \(Alphanumeric\)',
        r'SearchValue: John',
        r'SearchAttribute: 0x00 \(Name\)',
        r'MaxListCount: 0x0014 \(20\)',
        r'ListStartOffset: 0x0000 \(0\)',
        r'vCardSelector: 0x0000000000000003',
        r'vCardSelectorOperator: 0x00 \(OR\)',
    ]

    matched_patterns = []
    for pattern in expected_patterns:
        pattern_matched = False
        for line in lines:
            if re.search(pattern, line):
                matched_patterns.append(pattern)
                pattern_matched = True
                logger.info(f'Pattern matched: {pattern} in line: {line}')
                break
        if not pattern_matched:
            logger.warning(f'Pattern NOT matched: {pattern}')

    assert len(matched_patterns) == len(expected_patterns), (
        f'Only {len(matched_patterns)}/{len(expected_patterns)} patterns matched'
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_listing_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Continue', timeout=5
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_listing_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_LISTING_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDENTRY_SUCCESS(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard entry success."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_entry request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_entry_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Continue', timeout=5
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_entry_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDENTRY_ERROR(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard entry error."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_entry request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    err_code = 0xC4
    pbap_pse.iexpect(f'pbap pse pull_vcard_entry_rsp error {hex(err_code)}', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result {bt_obex_rsp_code_to_str(err_code)}',
        timeout=5,
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDENTRY_CONTINUE(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard entry continue."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_entry request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(
        'pbap pse pull_vcard_entry_rsp noerror',
        'Keep sending responses continuously until rsp_code is success',
    )

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Continue', timeout=5
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_entry_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDENTRY_SRMP_SERVER_INIT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard entry with server SRMP."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_entry request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(
        'pbap pse pull_vcard_entry_rsp noerror srmp',
        'Suspend after sending a single response and await the PCE request',
    )

    pbap_pce._wait_for_shell_response(r'please send pull cmd again', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_entry request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_entry_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDENTRY_SRMP_CLIENT_INIT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard entry with client SRMP."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf srmp', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_entry request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect(
        'pbap pse pull_vcard_entry_rsp noerror',
        'Suspend after sending a single response and await the PCE request',
    )

    pbap_pce._wait_for_shell_response(r'please send pull cmd again', timeout=5)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_entry request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_entry_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDENTRY_ABORT(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard entry abort."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf', PROMOT)

    pbap_pse._wait_for_shell_response(r'pbap_pse get pull_vcard_entry request', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_entry_rsp noerror srmp', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Continue', timeout=5
    )

    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap pce abort', PROMOT)

    pbap_pse._wait_for_shell_response(r'receive abort req', timeout=10)
    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse abort_rsp success', PROMOT)

    pbap_pce._wait_for_shell_response(r'abort success', timeout=5)

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_PULL_VCARDENTRY_PARAM_ALL(pbap_pce, pbap_pse):
    """Test BR PBAP PCE pull vcard entry with all application parameters."""
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_l2cap_transport_connection(pbap_pce, pbap_pse)
    pbap_pce_create_obex_connection(pbap_pce, pbap_pse)

    # Add all application parameters for PullvCardListing
    pbap_pce.iexpect('pbap alloc_buf', PROMOT)
    pbap_pce.iexpect('pbap add_ap PropertySelector 0x000000000000000F', PROMOT)
    pbap_pce.iexpect('pbap add_ap Format v2.1', PROMOT)
    pbap_pce.iexpect('pbap pce pull_vcard_entry 1.vcf', PROMOT)

    _, lines = pbap_pse._wait_for_shell_response(r'Format', timeout=10)
    expected_patterns = [
        r'PropertySelector: 0x000000000000000f',
        r'Format: 0x00 \(vCard 2\.1\)',
    ]

    matched_patterns = []
    for pattern in expected_patterns:
        pattern_matched = False
        for line in lines:
            if re.search(pattern, line):
                matched_patterns.append(pattern)
                pattern_matched = True
                logger.info(f'Pattern matched: {pattern} in line: {line}')
                break
        if not pattern_matched:
            logger.warning(f'Pattern NOT matched: {pattern}')

    assert len(matched_patterns) == len(expected_patterns), (
        f'Only {len(matched_patterns)}/{len(expected_patterns)} patterns matched'
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_entry_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Continue', timeout=5
    )

    pbap_pse.iexpect('pbap alloc_buf', PROMOT)
    pbap_pse.iexpect('pbap pse pull_vcard_entry_rsp noerror', PROMOT)

    pbap_pce._wait_for_shell_response(
        f'pbap pull {BT_PBAP_PULL_VCARD_ENTRY_TYPE} result Success', timeout=5
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')
