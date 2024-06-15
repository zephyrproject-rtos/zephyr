# Copyright (c) 2024 Vestas Wind Systems A/S
#
# SPDX-License-Identifier: Apache-2.0

"""
Test suites for testing Zephyr CAN <=> host CAN.
"""

import logging
import pytest

import can
from can import BusABC, CanProtocol

# RX/TX timeout in seconds
TIMEOUT = 1.0

logger = logging.getLogger(__name__)

@pytest.mark.parametrize('msg', [
    pytest.param(
        can.Message(arbitration_id=0x10,
                    is_extended_id=False),
        id='std_id_dlc_0'
    ),
    pytest.param(
        can.Message(arbitration_id=0x20,
                    data=[0xaa, 0xbb, 0xcc, 0xdd],
                    is_extended_id=False),
        id='std_id_dlc_4'
    ),
    pytest.param(
        can.Message(arbitration_id=0x30,
                    data=[0xee, 0xff, 0xee, 0xff, 0xee, 0xff, 0xee, 0xff],
                    is_extended_id=True),
        id='ext_id_dlc_8'
    ),
    pytest.param(
        can.Message(arbitration_id=0x40,
                    data=[0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                          0x10, 0x11],
                    is_fd=True, is_extended_id=False),
        id='std_id_fdf_dlc_9'
    ),
    pytest.param(
        can.Message(arbitration_id=0x50,
                    data=[0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                          0x10, 0x11],
                    is_fd=True, bitrate_switch=True, is_extended_id=False),
        id='std_id_fdf_brs_dlc_9'
    ),
])
class TestCanRxTx():
    """
    Class for testing CAN RX/TX between Zephyr DUT and host.
    """

    @staticmethod
    def check_rx(tx: can.Message, rx: can.Message) -> None:
        """Check if received message matches transmitted message."""
        # pylint: disable-next=unused-variable
        __tracebackhide__ = True

        if rx is None:
            pytest.fail('no message received')

        if not tx.equals(rx, timestamp_delta=None, check_channel=False,
                         check_direction=False):
            pytest.fail(f'rx message "{rx}" not equal to tx message "{tx}"')

    @staticmethod
    def skip_if_unsupported(can_dut: BusABC, can_host: BusABC, msg: can.Message) -> None:
        """Skip test if message format is not supported by both DUT and host."""
        if msg.is_fd:
            if can_dut.protocol == CanProtocol.CAN_20:
                pytest.skip('CAN FD not supported by DUT')
            if can_host.protocol == CanProtocol.CAN_20:
                pytest.skip('CAN FD not supported by host')

    def test_dut_to_host(self, can_dut: BusABC, can_host: BusABC, msg: can.Message) -> None:
        """Test DUT to host communication."""
        self.skip_if_unsupported(can_dut, can_host, msg)

        can_dut.send(msg, timeout=TIMEOUT)
        rx = can_host.recv(timeout=TIMEOUT)
        self.check_rx(msg, rx)

    def test_host_to_dut(self, can_dut: BusABC, can_host: BusABC, msg: can.Message) -> None:
        """Test host to DUT communication."""
        self.skip_if_unsupported(can_dut, can_host, msg)

        can_host.send(msg, timeout=TIMEOUT)
        rx = can_dut.recv(timeout=TIMEOUT)
        self.check_rx(msg, rx)
