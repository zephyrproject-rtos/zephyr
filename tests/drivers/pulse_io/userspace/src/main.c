/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/pulse_io.h>

static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(pulse_io0));

/*
 * Exercise the syscall path from user mode: capability query, channel
 * lifecycle, configure (struct copied in by the handler), and a
 * transmit/receive round-trip (buffers bounds-checked by the handler).
 */
ZTEST_USER(pulse_io_userspace, test_syscall_roundtrip)
{
	struct pulse_io_caps caps;
	struct pulse_io_channel *ch;
	struct pulse_io_config cfg = {
		.mode = PULSE_IO_MODE_SYMBOL,
		.dir = PULSE_IO_DIR_TX,
		.resolution_hz = 1000000,
	};
	struct pulse_symbol tx[] = {
		{.duration = 100, .level = 1},
		{.duration = 200, .level = 0},
	};
	struct pulse_symbol rx[8];
	size_t count = 0;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	zassert_true(caps.modes & PULSE_IO_MODE_SYMBOL);

	zassert_ok(pulse_io_channel_get(dev, 0, &ch));
	zassert_ok(pulse_io_channel_configure(dev, ch, &cfg));

	struct pulse_io_tx_req txr = {.symbols = tx, .count = ARRAY_SIZE(tx)};

	zassert_ok(pulse_io_transmit_sync(dev, ch, &txr, K_MSEC(10)));

	struct pulse_io_rx_req rxr = {.symbols = rx, .capacity = ARRAY_SIZE(rx)};

	zassert_ok(pulse_io_receive_sync(dev, ch, &rxr, &count, K_MSEC(10)));
	zassert_equal(count, ARRAY_SIZE(tx));
	zassert_equal(rx[0].duration, tx[0].duration);

	zassert_ok(pulse_io_stop(dev, ch));
	zassert_ok(pulse_io_channel_release(dev, ch));
}

static void *setup(void)
{
	k_object_access_grant(dev, k_current_get());
	return NULL;
}

ZTEST_SUITE(pulse_io_userspace, NULL, setup, NULL, NULL, NULL);
