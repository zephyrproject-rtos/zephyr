# Copyright (c) 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

"""Two default Bluetooth shell images, driven through their consoles.

The images are tests/bluetooth/shell built for nrf52_bsim/native with the
upstream xterm-native-shell configuration, as in the bluetooth.native_shell
twister scenario: Host, Controller and radio, with the console on stdio.
"""

import re

import pytest
import pytest_asyncio
from bsworld.devices import ProcessInstance
from zephyr_images import add_image


async def initialize_shell(device: ProcessInstance) -> None:
    """Initialize the Bluetooth Host after the shell's startup banner."""
    await device.wait_log(r"Before any Bluetooth commands")
    await device.shell.run("bt init sync")
    await device.wait_log(r"Bluetooth initialized")


async def shell_identity(device: ProcessInstance) -> str:
    await device.shell.run("bt id-show")
    line = await device.wait_log(r"\*0: [PR]:[0-9A-F:]+")
    return re.search(r"\*0: ([PR]:[0-9A-F:]+)", line)[1]


async def connect(peripheral, central):
    """Connect the Central to the Peripheral's connectable advertising."""
    address = await shell_identity(peripheral)
    await peripheral.shell.run("bt advertise on identity")
    await peripheral.wait_log("Advertising started")
    await central.shell.run(f"bt connect {address}")
    await central.wait_log(rf"Connected: {re.escape(address)}")
    await peripheral.wait_log(r"Connected: [PR]:[0-9A-F:]+")


async def disconnect(peripheral, central):
    await central.shell.run("bt disconnect")
    # Local termination reports 0x16; the peer sees the remote user's 0x13.
    await central.wait_log(r"Disconnected: .*reason 0x16")
    await peripheral.wait_log(r"Disconnected: .*reason 0x13")


async def notify_and_verify(peripheral, central, payload: bytes) -> None:
    """Subscribe to the shell's test vendor service and check one notification.

    The default shell redraws its prompt inside hexdumps, so the notification
    is checked by its length rather than its bytes.
    """
    await central.shell.run("gatt discover")
    value_handle = ccc_handle = None
    while True:
        line = await central.wait_log(r"Descriptor .* found: handle|Discover complete")
        if "Discover complete" in line:
            break
        uuid, handle = re.search(r"Descriptor (\S+) found: handle ([0-9a-f]+)", line).groups()
        if uuid.endswith("56789abcdef5"):
            value_handle = int(handle, 16)
        elif uuid == "2902" and value_handle is not None and ccc_handle is None:
            ccc_handle = int(handle, 16)
    assert value_handle is not None and ccc_handle is not None

    await central.shell.run(f"gatt subscribe {ccc_handle:x} {value_handle:x}")
    await central.wait_log("Subscribed")
    # Reading the CCC on the same ATT bearer confirms notifications are
    # enabled before the server sends, without timing sleeps.
    await central.shell.run(f"gatt read {ccc_handle:x}")
    await central.wait_log("Read complete: err 0x00 length 2")
    await peripheral.shell.run(f"gatt notify {value_handle:x} {payload.hex()}")
    await central.wait_log(rf"Notification: value_handle {value_handle}, length {len(payload)}")


@pytest_asyncio.fixture
async def shell_pair(topo):
    """Return two started and initialized shells as (peripheral, central)."""
    peripheral = add_image(topo, "shell", label="peripheral", seed=41, shell=True)
    central = add_image(topo, "shell", label="central", seed=42, shell=True)
    topo.set_channel_attenuation(default_db=45)
    await topo.start()
    for device in (peripheral, central):
        await initialize_shell(device)
    return peripheral, central


@pytest.mark.parametrize("active", [False, True], ids=["passive", "active"])
async def test_scan(shell_pair, active):
    """Passive scanning reports non-connectable advertising; active scanning
    obtains the SCAN_RSP that carries the device name."""
    peripheral, central = shell_pair
    address = re.escape(await shell_identity(peripheral))
    if active:
        # Legacy connectable advertising carries the device name in SCAN_RSP,
        # reported as event type 4 with the SR flag set.
        await peripheral.shell.run("bt advertise on identity")
        pattern = rf"\[DEVICE\]: {address}, AD evt type 4, .*test shell C:1 S:1 D:0 SR:1"
    else:
        # The shell keeps its name in SCAN_RSP, so "nconn" sends ADV_SCAN_IND:
        # scannable, but reported as not connectable.
        await peripheral.shell.run("bt advertise nconn identity")
        pattern = rf"\[DEVICE\]: {address}, AD evt type 2, .*C:0 S:1 D:0 SR:0"
    await peripheral.wait_log("Advertising started")
    await central.shell.run("bt scan on" if active else "bt scan passive")
    await central.wait_log(pattern)
    await central.shell.run("bt scan off")
    await central.wait_log("Scan successfully stopped")


async def test_encrypted_gatt(shell_pair):
    """Connect, encrypt the link and receive a notification over it."""
    peripheral, central = shell_pair
    await peripheral.shell.run("gatt register")
    await peripheral.wait_log("Registered test vendor service")
    await connect(peripheral, central)
    await central.shell.run("bt security 2")
    for device in (central, peripheral):
        await device.wait_log(r"Security changed: .* level 2")
    await notify_and_verify(peripheral, central, b"Zephyr")
    await disconnect(peripheral, central)
