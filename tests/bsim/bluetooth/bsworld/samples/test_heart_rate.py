# Copyright (c) 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

"""samples/bluetooth/peripheral_hr and central_hr against each other."""

from zephyr_images import add_image


async def test_central_hr_receives_peripheral_hr_notifications(topo):
    """The two samples find each other, connect and stream heart rate."""
    peripheral = add_image(topo, "peripheral_hr", label="peripheral_hr", seed=41)
    central = add_image(topo, "central_hr", label="central_hr", seed=42)
    topo.set_channel_attenuation(default_db=45)
    await topo.start()

    await peripheral.wait_log(r"\bConnected$")
    await central.wait_log(r"\[SUBSCRIBED\]")
    await peripheral.wait_log(r"HRS notification status changed: enabled")
    for _ in range(3):
        # Flags octet plus an 8-bit heart rate value.
        await central.wait_log(r"\[NOTIFICATION\] data 0x[0-9a-f]+ length 2")
