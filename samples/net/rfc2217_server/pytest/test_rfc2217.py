# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Drive the sample's UART on native_sim through its RFC 2217 server.

The server serves uart_1, a pseudoterminal of the host, over offloaded
sockets: pyserial's rfc2217:// client on 127.0.0.1 on one side, the
pseudoterminal on the other.
"""

import re
import time

import pytest
import serial
from twister_harness import DeviceAdapter


def server(dut: DeviceAdapter) -> tuple[str, str]:
    """The pseudoterminal of uart_1 and the server's URL."""
    lines = dut.readlines_until(regex=r"Serving \S+ on TCP port \d+", timeout=10)
    pty = next(
        m.group(1)
        for line in lines
        if (m := re.search(r"uart_1 connected to pseudotty: (\S+)", line))
    )
    port = re.search(r"TCP port (\d+)", lines[-1]).group(1)

    return pty, f"rfc2217://127.0.0.1:{port}"


def test_data_and_settings(dut: DeviceAdapter):
    pty, url = server(dut)
    data = bytes(range(256))

    with (
        serial.Serial(pty, timeout=5) as uart,
        serial.serial_for_url(url, baudrate=115200, timeout=5) as client,
    ):
        # Both ways, with 0xff (IAC) in the data
        client.write(data)
        assert uart.read(len(data)) == data
        uart.write(data)
        assert client.read(len(data)) == data

        # Settings the server acknowledges, or pyserial raises
        client.baudrate = 9600
        client.parity = serial.PARITY_EVEN
        client.stopbits = serial.STOPBITS_TWO

        client.dtr = False
        client.send_break(0.1)
        client.write(b"after the settings")
        assert uart.read(18) == b"after the settings"

    dut.readlines_until(regex="DTR off", timeout=5)
    dut.readlines_until(regex="BREAK off", timeout=5)


def test_new_client_replaces(dut: DeviceAdapter):
    pty, url = server(dut)

    with (
        serial.Serial(pty, timeout=5) as uart,
        serial.serial_for_url(url, timeout=5) as first,
        serial.serial_for_url(url, timeout=5) as second,
    ):
        uart.write(b"to the second")
        assert second.read(13) == b"to the second"

        # The server closed the first client's connection
        deadline = time.monotonic() + 5
        with pytest.raises(serial.SerialException):
            while time.monotonic() < deadline:
                first.read(1)
