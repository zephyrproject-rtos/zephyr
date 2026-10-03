/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>

/*
 * Exercise the CAN controller against a real host CAN bus. QEMU connects its
 * emulated controller to a host SocketCAN interface, which the twister "can"
 * sidecar creates along with a companion that echoes every frame back onto the
 * bus. QEMU's CAN models do not self-receive in loopback mode, so the test runs
 * in normal mode and relies on the companion echo for the round trip.
 *
 * Run by hand with a vcan interface and, for example:
 *
 *   python3 can_echo.py <iface>
 */

#define TEST_CAN_ID  0x123U
#define RX_TIMEOUT   K_MSEC(1000)

CAN_MSGQ_DEFINE(rx_msgq, 4);

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

ZTEST(can_host_socketcan, test_frame_echoed_by_host)
{
	const struct can_frame tx_frame = {
		.id = TEST_CAN_ID,
		.dlc = 2U,
		.data = {0xABU, 0xCDU},
	};
	const struct can_filter filter = {
		.id = TEST_CAN_ID,
		.mask = CAN_STD_ID_MASK,
	};
	struct can_frame rx_frame;
	bool received = false;
	int filter_id;
	int err;

	filter_id = can_add_rx_filter_msgq(can_dev, &rx_msgq, &filter);
	zassert_true(filter_id >= 0, "failed to add rx filter (%d)", filter_id);

	/* The host companion may still be binding to the bus when the test
	 * starts (native_sim boots in milliseconds), so keep sending until the
	 * echo comes back rather than relying on a single frame.
	 */
	for (int i = 0; i < 50 && !received; i++) {
		err = can_send(can_dev, &tx_frame, RX_TIMEOUT, NULL, NULL);
		zassert_ok(err, "failed to send CAN frame (%d)", err);

		if (k_msgq_get(&rx_msgq, &rx_frame, K_MSEC(100)) == 0) {
			received = true;
		}
	}

	zassert_true(received, "did not receive the host-echoed frame");
	zassert_equal(rx_frame.id, TEST_CAN_ID, "unexpected id 0x%x", rx_frame.id);
	zassert_equal(rx_frame.dlc, tx_frame.dlc, "unexpected dlc %u", rx_frame.dlc);
	zassert_mem_equal(rx_frame.data, tx_frame.data, tx_frame.dlc, "payload mismatch");

	can_remove_rx_filter(can_dev, filter_id);
}

static void *can_host_socketcan_setup(void)
{
	zassert_true(device_is_ready(can_dev), "CAN device not ready");
	zassert_ok(can_set_mode(can_dev, CAN_MODE_NORMAL), "failed to set normal mode");
	zassert_ok(can_start(can_dev), "failed to start CAN controller");
	return NULL;
}

ZTEST_SUITE(can_host_socketcan, NULL, can_host_socketcan_setup, NULL, NULL, NULL);
