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


def pbap_pce_get_rfcomm_channel(pbap_pce):
    pattern = r'PSE rfcomm channel param\s+(0x[\da-fA-F]+)'
    pbap_pce.exec_command('pbap pce sdp_discover')
    _, data = pbap_pce._wait_for_shell_response(r'PSE rfcomm channel param.*', timeout=35)
    for line in data:
        match = re.search(pattern, line)
        if match:
            logger.info(f'rfcomm channel = {match.group(1)}')
            return match.group(1)
    return None


def pbap_pce_rfcomm_transport_connection(pbap_pce, pbap_pse):
    channel = pbap_pce_get_rfcomm_channel(pbap_pce)
    if channel is None:
        logger.error('Failed to get PSE RFCOMM channel')
        return

    pbap_pce.iexpect(
        f'pbap pce connect_rfcomm {channel}', r'PBAP PCE.*rfcomm transport connected on.*'
    )
    pbap_pse._wait_for_shell_response(r'PBAP PSE.*rfcomm transport connected on.*')


def test_BR_PBAP_PCE_RFCOMM_CONNECT_SUCCESS(pbap_pce, pbap_pse):
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_rfcomm_transport_connection(pbap_pce, pbap_pse)
    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_RFCOMM_CONNECT_FAILURE(pbap_pce, pbap_pse):
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)

    channel = pbap_pce_get_rfcomm_channel(pbap_pce)
    if channel is None:
        logger.error('Failed to get PSE RFCOMM channel')
        return

    err_channel = hex(int(channel, 16) - 1)
    logger.info(f'Using invalid channel: {err_channel} (valid channel: {channel})')

    pbap_pce.iexpect(
        f'pbap pce connect_rfcomm {err_channel}', r'PBAP PCE.*rfcomm transport disconnected'
    )

    pbap_pce.iexpect('bt disconnect', 'Disconnected')


def test_BR_PBAP_PCE_RFCOMM_DISCONNECT_SUCCESS(pbap_pce, pbap_pse):
    pbap_br_clear(pbap_pce, pbap_pse)
    pbap_br_connect(pbap_pce, pbap_pse)
    pbap_pce_rfcomm_transport_connection(pbap_pce, pbap_pse)

    pbap_pce.iexpect('pbap pce disconnect_rfcomm', r'PBAP PCE.*rfcomm transport disconnected')
    pbap_pce.iexpect('bt disconnect', 'Disconnected')
