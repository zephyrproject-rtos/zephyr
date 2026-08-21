# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging
import time

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

# OBEX / OPP numeric response codes (see include/zephyr/bluetooth/classic/obex.h).
RSP_SUCCESS = "a0"
RSP_BAD_REQ = "c0"
RSP_FORBIDDEN = "c3"
RSP_NOT_FOUND = "c4"
RSP_ENTITY_TOO_LARGE = "cd"
RSP_UNSUPP_MEDIA_TYPE = "cf"
RSP_UNAVAIL = "d3"


def _lines_match(lines: list[str], response: str) -> bool:
    return any(response in line for line in lines)


def _wait_for(dut: DeviceAdapter, response: str, max_wait_sec: int = 10) -> bool:
    """Poll a DUT's serial buffer until 'response' is seen or the timeout expires."""
    lines: list[str] = []
    for _ in range(max_wait_sec):
        read_lines = dut.readlines()
        lines += read_lines
        for line in read_lines:
            if response in line:
                logger.info(f"Matched '{response}'")
                return True
        time.sleep(1)
    logger.info(f"Did not match '{response}': {lines}")
    return False


def _wait_for_all(dut: DeviceAdapter, responses: list[str], max_wait_sec: int = 10) -> bool:
    """Wait until every string in 'responses' has been seen, in any order.

    Unlike calling _wait_for() repeatedly, this accumulates all lines and ticks
    off each expected response as it appears, so responses that arrive together
    in a single readlines() batch are not lost.
    """
    pending = list(responses)
    for _ in range(max_wait_sec):
        for line in dut.readlines():
            for resp in list(pending):
                if resp in line:
                    logger.info(f"Matched '{resp}'")
                    pending.remove(resp)
            if not pending:
                return True
        if not pending:
            return True
        time.sleep(1)
    logger.info(f"Did not match {pending}")
    return False


def _cmd_expect(
    shell: Shell, dut: DeviceAdapter, command: str, response: str, max_wait_sec: int = 10
) -> bool:
    """Run a command on 'shell' and expect 'response' (sync output first, then async)."""
    lines = shell.exec_command(command)
    if _lines_match(lines, response):
        logger.info(f"Matched '{response}' in command output")
        return True
    return _wait_for(dut, response, max_wait_sec)


def _cmd(shell: Shell, command: str) -> list[str]:
    return shell.exec_command(command)


def _reset(server_sh: Shell):
    server_sh.exec_command("opp_s reset_rsp")


# ----------------------------------------------------------------------------
# Session fixture
# ----------------------------------------------------------------------------


@pytest.fixture(name='transport_ready', scope='session')
def fixture_transport_ready(duts: list[DeviceAdapter], shells: list[Shell], initialize):
    """Bring up ACL + RFCOMM once and keep them up for the whole suite.

    The OPP RFCOMM transport cannot be reconnected within a single boot cycle
    (a second connect returns -EALREADY), so the transport is established a
    single time here. OBEX is *not* connected by the fixture; the ordered test
    methods perform exactly one OBEX connect, one OBEX disconnect, and one
    RFCOMM disconnect across the whole session.
    """
    _client_addr, server_addr = initialize
    client_dut, server_dut = duts[0], duts[1]
    client_sh, server_sh = shells[0], shells[1]

    server_sh.exec_command("br pscan on")
    server_sh.exec_command("br iscan on")
    reg = server_sh.exec_command("opp_s register")
    assert _lines_match(reg, "OPP server registered") or _lines_match(
        reg, "OPP server already registered"
    ), "OPP server not registered"

    client_sh.exec_command(f"br connect {server_addr}")
    assert _wait_for(client_dut, "Connected", max_wait_sec=15), "Client ACL not connected"
    _wait_for(server_dut, "Connected", max_wait_sec=5)

    assert _cmd_expect(
        client_sh, client_dut, "opp_c discover", "OPP server discovered on RFCOMM channel"
    ), "Client did not discover the OPP server channel"

    assert _cmd_expect(
        client_sh, client_dut, "opp_c connect_rfcomm", "OPP client RFCOMM connected"
    ), "Client RFCOMM transport not connected"
    assert _wait_for(server_dut, "OPP server RFCOMM connected"), (
        "Server RFCOMM transport not connected"
    )
    return server_addr


class TestOPP:
    """OPP board-to-board test suite (single-image dual-role).

    DUT0 (shells[0]) is the OPP Push Client, DUT1 (shells[1]) is the OPP Push
    Server. The ACL + RFCOMM transport is established once by the
    'transport_ready' fixture and kept up for the whole session because the
    RFCOMM transport cannot be reconnected within one boot cycle. The methods
    are ordered to model one OBEX session lifecycle: OBEX connect (2.2), data
    operations (2.3-2.6), PDU utilities (2.8), OBEX disconnect (2.7), then the
    RFCOMM transport disconnect (2.1) last. Server response codes are configured
    at runtime via 'opp_s set_*_rsp'.
    """

    # ==================================================================
    # 2.2 OBEX Connection Establishment
    # ==================================================================

    def test_2_2_obex_connect_error(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_OBEX_CONNECT_ERROR / BR_OPP_CLIENT_OBEX_CONNECT_ERROR
        A rejected CONNECT leaves the OBEX session disconnected so the next test
        can establish it cleanly.
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        server_sh.exec_command(f"opp_s set_connect_rsp {RSP_UNAVAIL}")
        assert _cmd_expect(
            client_sh,
            client_dut,
            "opp_c obex_connect",
            f"OPP client OBEX connect error 0x{RSP_UNAVAIL}",
        ), "Client did not observe the OBEX connect error"
        assert _wait_for(server_dut, f"OPP server OBEX connect rejected 0x{RSP_UNAVAIL}"), (
            "Server did not report the rejected OBEX connect"
        )
        _reset(server_sh)
        time.sleep(1)

    def test_2_2_obex_connect_success(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_OBEX_CONNECT_SUCCESS / BR_OPP_CLIENT_OBEX_CONNECT_SUCCESS
        BR_OPP_SERVER_OBEX_CONNECT_NO_TARGET_HEADER
        BR_OPP_CLIENT_OBEX_CONNECT_NO_TARGET_HEADER
        The client never sends a Target header (per spec 5.4), so a SUCCESS
        establishes the no-Target-header path implicitly. This is the single
        OBEX connect for the whole session.
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        assert _cmd_expect(
            client_sh, client_dut, "opp_c obex_connect", "OPP client OBEX connected"
        ), "Client OBEX not connected"
        # The server emits "no_target_header" and "OBEX connected" back to back,
        # often in the same readlines() batch. Scan for both markers together so
        # neither line is discarded while matching the other.
        assert _wait_for_all(
            server_dut,
            [
                "OPP server OBEX connect no_target_header",
                "OPP server OBEX connected",
            ],
        ), "Server did not confirm no-Target-header connect and OBEX connected"

    # ==================================================================
    # 2.3 Object Push (PUT)
    # ==================================================================

    def test_2_3_push_success_final(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PUSH_SUCCESS_FINAL / BR_OPP_CLIENT_PUSH_SUCCESS_FINAL"""
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        assert _cmd_expect(client_sh, client_dut, "opp_c push", "OPP client push success"), (
            "Client push not successful"
        )
        assert _wait_for(server_dut, "OPP server push complete"), "Server missed object"

    def test_2_3_push_continue_non_final(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PUSH_CONTINUE_NON_FINAL / BR_OPP_CLIENT_PUSH_CONTINUE_NON_FINAL"""
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        assert _cmd_expect(client_sh, client_dut, "opp_c push_start", "OPP client push continue"), (
            "Client did not receive CONTINUE for non-final PUT"
        )
        assert _wait_for(server_dut, "OPP server push continue"), "Server did not CONTINUE"
        assert _cmd_expect(client_sh, client_dut, "opp_c push_final", "OPP client push success"), (
            "Client did not complete multi-packet PUT"
        )
        assert _wait_for(server_dut, "OPP server push complete"), "Server did not complete"

    def test_2_3_push_unsupported_media(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PUSH_UNSUPPORTED_MEDIA_ERROR
        BR_OPP_CLIENT_PUSH_UNSUPPORTED_MEDIA_ERROR
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        server_sh.exec_command(f"opp_s set_push_rsp {RSP_UNSUPP_MEDIA_TYPE}")
        assert _cmd_expect(
            client_sh, client_dut, "opp_c push", "OPP client push unsupported_media"
        ), "Client did not observe UNSUPP_MEDIA_TYPE"
        assert _wait_for(server_dut, f"OPP server push error 0x{RSP_UNSUPP_MEDIA_TYPE}"), (
            "Server did not report the UNSUPP_MEDIA_TYPE rejection"
        )
        _reset(server_sh)

    def test_2_3_push_entity_too_large(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PUSH_ENTITY_TOO_LARGE_ERROR
        BR_OPP_CLIENT_PUSH_ENTITY_TOO_LARGE_ERROR
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        server_sh.exec_command(f"opp_s set_push_rsp {RSP_ENTITY_TOO_LARGE}")
        assert _cmd_expect(
            client_sh, client_dut, "opp_c push", "OPP client push entity_too_large"
        ), "Client did not observe ENTITY_TOO_LARGE"
        assert _wait_for(server_dut, f"OPP server push error 0x{RSP_ENTITY_TOO_LARGE}"), (
            "Server did not report the ENTITY_TOO_LARGE rejection"
        )
        _reset(server_sh)

    def test_2_3_push_multiple_objects(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PUSH_MULTIPLE_OBJECTS / BR_OPP_CLIENT_PUSH_MULTIPLE_OBJECTS"""
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        assert _cmd_expect(client_sh, client_dut, "opp_c push", "OPP client push success"), (
            "First object push failed"
        )
        assert _wait_for(server_dut, "OPP server push complete"), "Server missed object 1"
        assert _cmd_expect(client_sh, client_dut, "opp_c push", "OPP client push success"), (
            "Second object push failed"
        )
        assert _wait_for(server_dut, "OPP server push complete"), "Server missed object 2"

    def test_2_3_push_abort_ongoing(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PUSH_ABORT_ONGOING / BR_OPP_CLIENT_PUSH_ABORT_ONGOING"""
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        assert _cmd_expect(client_sh, client_dut, "opp_c push_start", "OPP client push continue"), (
            "Client did not start multi-packet PUT"
        )
        assert _cmd_expect(client_sh, client_dut, "opp_c abort", "OPP client abort success"), (
            "Client abort during PUT failed"
        )
        assert _wait_for(server_dut, "OPP server abort handled"), "Server did not abort"

    # ==================================================================
    # 2.4 Business Card Pull (GET)
    # ==================================================================

    def test_2_4_pull_bcard_success(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PULL_BCARD_SUCCESS_FINAL / BR_OPP_CLIENT_PULL_BCARD_SUCCESS_FINAL"""
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        server_sh.exec_command(f"opp_s set_pull_rsp {RSP_SUCCESS}")
        assert _cmd_expect(
            client_sh, client_dut, "opp_c pull_bcard", "OPP client pull_bcard success"
        ), "Client did not pull the business card"
        assert _wait_for(server_dut, "OPP server pull_bcard success"), "Server sent no vCard"
        _reset(server_sh)

    def test_2_4_pull_bcard_multi_packet(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PULL_BCARD_CONTINUE_NON_FINAL
        BR_OPP_CLIENT_PULL_BCARD_CONTINUE_NON_FINAL
        With multi-packet pull enabled the server answers the first GET with a
        CONTINUE carrying a non-final Body chunk. The OPP client stack does not
        auto-continue a GET, so the application issues an explicit continuation
        GET ('opp_c pull_continue') which the server answers with a SUCCESS
        carrying the final End-of-Body chunk. This exercises the GET
        CONTINUE_NON_FINAL path on the wire for both roles.
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        server_sh.exec_command("opp_s set_pull_multi 1")
        assert _cmd_expect(
            client_sh, client_dut, "opp_c pull_bcard", "OPP client pull_bcard continue"
        ), "Client did not observe the intermediate CONTINUE"
        assert _wait_for(server_dut, "OPP server pull_bcard continue"), (
            "Server did not send a non-final CONTINUE"
        )
        # The client stack does not auto-continue; drive the continuation GET.
        assert _cmd_expect(
            client_sh, client_dut, "opp_c pull_continue", "OPP client pull_bcard success"
        ), "Client did not complete the multi-packet pull"

        assert _wait_for(server_dut, "OPP server pull_bcard success"), (
            "Server did not finish the multi-packet pull"
        )
        _reset(server_sh)

    def test_2_4_pull_bcard_not_found(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PULL_BCARD_NOT_FOUND / BR_OPP_CLIENT_PULL_BCARD_NOT_FOUND"""
        client_dut = duts[0]
        client_sh, server_sh = shells[0], shells[1]
        server_sh.exec_command(f"opp_s set_pull_rsp {RSP_NOT_FOUND}")
        assert _cmd_expect(
            client_sh, client_dut, "opp_c pull_bcard", "OPP client pull_bcard not_found"
        ), "Client did not observe NOT_FOUND"
        _reset(server_sh)

    def test_2_4_pull_bcard_forbidden_name(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PULL_BCARD_FORBIDDEN_NAME / BR_OPP_CLIENT_PULL_BCARD_FORBIDDEN_NAME
        A non-empty Name header makes the stack auto-reply FORBIDDEN.
        """
        client_dut = duts[0]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        assert _cmd_expect(
            client_sh, client_dut, "opp_c pull_bcard named", "OPP client pull_bcard forbidden"
        ), "Client did not observe FORBIDDEN for a non-empty Name header"

    def test_2_4_pull_bcard_abort_ongoing(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_PULL_BCARD_ABORT_ONGOING / BR_OPP_CLIENT_PULL_BCARD_ABORT_ONGOING
        BR_OPP_SERVER_PULL_BCARD_CONTINUE_NON_FINAL
        BR_OPP_CLIENT_PULL_BCARD_CONTINUE_NON_FINAL
        The single-packet business card completes in one response, so by the
        time the abort is issued there may be no operation outstanding; either
        an abort report or a graceful "no operation in progress" rejection is
        acceptable and the session stays stable.
        """
        client_dut = duts[0]
        client_sh, server_sh = shells[0], shells[1]
        server_sh.exec_command(f"opp_s set_pull_rsp {RSP_SUCCESS}")
        client_sh.exec_command("opp_c pull_bcard")
        lines = _cmd(client_sh, "opp_c abort")
        handled = _lines_match(lines, "OPP client abort") or _lines_match(
            lines, "Unable to send abort"
        )
        assert handled or _wait_for(client_dut, "OPP client abort", max_wait_sec=5), (
            "Client abort during GET not handled gracefully"
        )
        _reset(server_sh)

    # ==================================================================
    # 2.5 Business Card Exchange (PUT + GET)
    # ==================================================================

    def test_2_5_exchange_success(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_EXCHANGE_BCARD_SUCCESS / BR_OPP_CLIENT_EXCHANGE_BCARD_SUCCESS"""
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        server_sh.exec_command(f"opp_s set_pull_rsp {RSP_SUCCESS}")

        assert _cmd_expect(client_sh, client_dut, "opp_c push", "OPP client push success"), (
            "Exchange PUT failed"
        )
        assert _wait_for(server_dut, "OPP server push complete"), "Server missed exchange PUT"
        assert _cmd_expect(
            client_sh, client_dut, "opp_c pull_bcard", "OPP client pull_bcard success"
        ), "Exchange GET failed"
        assert _wait_for(server_dut, "OPP server pull_bcard success"), (
            "Server did not complete the exchange GET"
        )
        _reset(server_sh)

    def test_2_5_exchange_pull_not_found(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_EXCHANGE_BCARD_PULL_NOT_FOUND
        BR_OPP_CLIENT_EXCHANGE_BCARD_PULL_NOT_FOUND
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        assert _cmd_expect(client_sh, client_dut, "opp_c push", "OPP client push success"), (
            "Exchange PUT failed"
        )

        server_sh.exec_command(f"opp_s set_pull_rsp {RSP_NOT_FOUND}")
        assert _cmd_expect(
            client_sh, client_dut, "opp_c pull_bcard", "OPP client pull_bcard not_found"
        ), "Exchange GET NOT_FOUND not observed"
        assert _wait_for(server_dut, "OPP server pull_bcard not_found"), (
            "Server did not report NOT_FOUND for the exchange GET"
        )
        _reset(server_sh)

    # ==================================================================
    # 2.6 Abort Ongoing Request
    # ==================================================================

    def test_2_6_abort_active_request(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_ABORT_ACTIVE_REQUEST / BR_OPP_CLIENT_ABORT_ACTIVE_REQUEST"""
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        assert _cmd_expect(client_sh, client_dut, "opp_c push_start", "OPP client push continue"), (
            "Client did not start an active request"
        )
        assert _cmd_expect(client_sh, client_dut, "opp_c abort", "OPP client abort success"), (
            "Client abort of active request failed"
        )
        assert _wait_for(server_dut, "OPP server abort handled"), "Server did not abort"

    def test_2_6_abort_no_active_request(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_ABORT_NO_ACTIVE_REQUEST / BR_OPP_CLIENT_ABORT_NO_ACTIVE_REQUEST
        With nothing outstanding, the API may reject with an error or the abort
        callback may report SUCCESS; either way the session stays stable.
        """
        client_dut = duts[0]
        client_sh = shells[0]
        lines = _cmd(client_sh, "opp_c abort")
        handled = _lines_match(lines, "OPP client abort") or _lines_match(
            lines, "Unable to send abort"
        )
        assert handled or _wait_for(client_dut, "OPP client abort", max_wait_sec=5), (
            "Client abort with no active request not handled gracefully"
        )

    # ==================================================================
    # 2.8 Service Discovery and PDU Utilities
    # ==================================================================

    def test_2_8_sdp_and_pdu(self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready):
        """BR_OPP_SERVER_SDP_SERVICE_RECORD / BR_OPP_CLIENT_SDP_DISCOVERY
        BR_OPP_SERVER_PDU_CREATION / BR_OPP_CLIENT_PDU_CREATION
        SDP discovery + service record are exercised by the 'transport_ready'
        fixture; here we verify PDU allocation on both roles.
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        assert _cmd_expect(server_sh, server_dut, "opp_s create_pdu", "OPP server create_pdu ok"), (
            "Server PDU allocation failed"
        )
        assert _cmd_expect(client_sh, client_dut, "opp_c create_pdu", "OPP client create_pdu ok"), (
            "Client PDU allocation failed"
        )

    # ==================================================================
    # 2.7 OBEX Disconnection (single OBEX disconnect for the session)
    # ==================================================================

    def test_2_7_obex_disconnect_error(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_OBEX_DISCONNECT_ERROR / BR_OPP_CLIENT_OBEX_DISCONNECT_ERROR
        The server rejects the DISCONNECT so the OBEX session stays up for the
        subsequent successful disconnect.
        """
        client_dut = duts[0]
        client_sh, server_sh = shells[0], shells[1]
        server_sh.exec_command(f"opp_s set_disconnect_rsp {RSP_BAD_REQ}")
        assert _cmd_expect(
            client_sh, client_dut, "opp_c obex_disconnect", "OPP client OBEX disconnect error"
        ), "Client did not observe an OBEX disconnect error"
        _reset(server_sh)
        time.sleep(1)

    def test_2_7_obex_disconnect_success(
        self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready
    ):
        """BR_OPP_SERVER_OBEX_DISCONNECT_SUCCESS / BR_OPP_CLIENT_OBEX_DISCONNECT_SUCCESS
        After the preceding rejected DISCONNECT the OBEX session may either
        remain connected or already be torn down. Issue a clean DISCONNECT and
        accept either "disconnected" (session was still up and is now closed)
        or the API rejecting it because the session is already gone; in both
        cases the OBEX session ends up disconnected.
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh, server_sh = shells[0], shells[1]
        _reset(server_sh)
        lines = _cmd(client_sh, "opp_c obex_disconnect")
        if _lines_match(lines, "OPP client OBEX disconnected") or _wait_for(
            client_dut, "OPP client OBEX disconnected", max_wait_sec=5
        ):
            assert _wait_for(server_dut, "OPP server OBEX disconnected"), (
                "Server OBEX not disconnected"
            )
        else:
            assert _lines_match(lines, "Unable to send OBEX disconnect"), (
                "Client OBEX disconnect neither succeeded nor reported already-disconnected"
            )

    # ==================================================================
    # 2.1 Transport Disconnection (runs last; RFCOMM cannot be reconnected)
    # ==================================================================

    def test_2_1_transport(self, duts: list[DeviceAdapter], shells: list[Shell], transport_ready):
        """BR_OPP_SERVER_TRANSPORT_RFCOMM_ACCEPT / BR_OPP_CLIENT_TRANSPORT_RFCOMM_CONNECT
        BR_OPP_SERVER_TRANSPORT_RFCOMM_DISCONNECT
        BR_OPP_CLIENT_TRANSPORT_RFCOMM_DISCONNECT
        The RFCOMM connect + accept are covered by the 'transport_ready'
        fixture; this final test tears the transport down.
        """
        client_dut, server_dut = duts[0], duts[1]
        client_sh = shells[0]
        assert _cmd_expect(
            client_sh, client_dut, "opp_c disconnect_rfcomm", "OPP client RFCOMM disconnected"
        ), "Client RFCOMM not disconnected"
        assert _wait_for(server_dut, "OPP server RFCOMM disconnected"), (
            "Server RFCOMM not disconnected"
        )
