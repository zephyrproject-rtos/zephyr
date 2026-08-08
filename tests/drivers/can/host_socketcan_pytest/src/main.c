/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/printk.h>

/*
 * Exercise the CAN controller against a real host CAN bus driven by a pytest
 * test. The twister "can" sidecar creates the host SocketCAN interface and
 * reports its name to the test through the `sidecar_params` fixture, so the
 * test opens a socket on that very bus itself instead of relying on a separate
 * companion process.
 *
 * The guest sends a frame periodically and reports every frame it receives, so
 * the test can check both directions of the bus.
 */

#define TX_CAN_ID 0x123U
#define RX_CAN_ID 0x321U

CAN_MSGQ_DEFINE(rx_msgq, 4);

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

int main(void)
{
	const struct can_frame tx_frame = {
		.id = TX_CAN_ID,
		.dlc = 2U,
		.data = {0xABU, 0xCDU},
	};
	const struct can_filter filter = {
		.id = RX_CAN_ID,
		.mask = CAN_STD_ID_MASK,
	};
	struct can_frame rx_frame;
	int err;

	if (!device_is_ready(can_dev)) {
		printk("CAN device not ready\n");
		return 0;
	}

	err = can_set_mode(can_dev, CAN_MODE_NORMAL);
	if (err != 0) {
		printk("failed to set normal mode (%d)\n", err);
		return 0;
	}

	err = can_start(can_dev);
	if (err != 0) {
		printk("failed to start CAN controller (%d)\n", err);
		return 0;
	}

	err = can_add_rx_filter_msgq(can_dev, &rx_msgq, &filter);
	if (err < 0) {
		printk("failed to add rx filter (%d)\n", err);
		return 0;
	}

	printk("CAN host test ready\n");

	while (true) {
		/* The test may still be binding to the bus, so keep sending
		 * rather than relying on a single frame being observed.
		 */
		(void)can_send(can_dev, &tx_frame, K_MSEC(100), NULL, NULL);

		if (k_msgq_get(&rx_msgq, &rx_frame, K_MSEC(100)) == 0) {
			printk("rx id=0x%x dlc=%u data=", rx_frame.id, rx_frame.dlc);
			for (uint8_t i = 0U; i < rx_frame.dlc; i++) {
				printk("%02x", rx_frame.data[i]);
			}
			printk("\n");
		}
	}

	return 0;
}
