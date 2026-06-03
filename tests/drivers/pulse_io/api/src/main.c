/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/pulse_io.h>

static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(pulse_io0));

ZTEST(pulse_io_api, test_device_ready)
{
	zassert_true(device_is_ready(dev), "pulse_io device not ready");
}

ZTEST(pulse_io_api, test_capabilities)
{
	struct pulse_io_caps caps;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	zassert_true(caps.modes & PULSE_IO_MODE_SYMBOL);
	zassert_true(caps.supports_tx);
	zassert_true(caps.supports_rx);
	zassert_equal(caps.num_channels, 4);
}

ZTEST(pulse_io_api, test_channel_lifecycle)
{
	struct pulse_io_channel *ch;

	zassert_ok(pulse_io_channel_get(dev, 0, &ch));
	zassert_not_null(ch);
	struct pulse_io_channel *again;

	zassert_equal(pulse_io_channel_get(dev, 0, &again), -EBUSY);
	zassert_ok(pulse_io_channel_release(dev, ch));
	zassert_equal(pulse_io_channel_get(dev, 99, &ch), -ENODEV);
}

ZTEST(pulse_io_api, test_configure_rejects_bad_mode)
{
	struct pulse_io_channel *ch;
	struct pulse_io_config cfg = {
		.mode = (enum pulse_io_mode)0xFF,
		.dir = PULSE_IO_DIR_TX,
		.resolution_hz = 1000000,
	};

	zassert_ok(pulse_io_channel_get(dev, 1, &ch));
	zassert_equal(pulse_io_channel_configure(dev, ch, &cfg), -ENOTSUP);
	zassert_ok(pulse_io_channel_release(dev, ch));
}

ZTEST(pulse_io_api, test_transmit_receive_roundtrip)
{
	struct pulse_io_channel *ch;
	struct pulse_io_config cfg = {
		.mode = PULSE_IO_MODE_SYMBOL,
		.dir = PULSE_IO_DIR_TX,
		.resolution_hz = 1000000,
	};
	struct pulse_symbol tx[] = {
		{.duration = 100, .level = 1},
		{.duration = 200, .level = 0},
		{.duration = 300, .level = 1},
	};
	struct pulse_symbol rx[8];
	size_t count = 0;

	zassert_ok(pulse_io_channel_get(dev, 2, &ch));
	zassert_ok(pulse_io_channel_configure(dev, ch, &cfg));

	struct pulse_io_tx_req txr = {.symbols = tx, .count = ARRAY_SIZE(tx)};

	zassert_ok(pulse_io_transmit_sync(dev, ch, &txr, K_MSEC(10)));

	struct pulse_io_rx_req rxr = {.symbols = rx, .capacity = ARRAY_SIZE(rx)};

	zassert_ok(pulse_io_receive_sync(dev, ch, &rxr, &count, K_MSEC(10)));
	zassert_equal(count, ARRAY_SIZE(tx), "wrong symbol count");

	for (size_t i = 0; i < count; i++) {
		zassert_equal(rx[i].duration, tx[i].duration, "duration[%zu]", i);
		zassert_equal(rx[i].level, tx[i].level, "level[%zu]", i);
	}

	zassert_ok(pulse_io_stop(dev, ch));
	zassert_ok(pulse_io_channel_release(dev, ch));
}

/* Bit template shared by the codec tests: zero = short high, one = long. */
#define CODEC_T 4U
static const struct pulse_io_bit_template codec_tmpl = {
	.zero = {{CODEC_T, 1}, {3 * CODEC_T, 0}},
	.one = {{3 * CODEC_T, 1}, {CODEC_T, 0}},
	.msb_first = true,
};

ZTEST(pulse_io_api, test_encode_exact_layout)
{
	/* 0x80 MSB-first: bit 7 = 1, bits 6..0 = 0. */
	const uint8_t in = 0x80;
	struct pulse_symbol out[8 * 2];
	size_t produced = 0;

	zassert_ok(pulse_io_encode_bytes(&codec_tmpl, &in, 1, out, ARRAY_SIZE(out), &produced));
	zassert_equal(produced, 16, "wrong symbol count");

	/* First bit is a one: long high then short low. */
	zassert_equal(out[0].duration, 3 * CODEC_T);
	zassert_equal(out[0].level, 1);
	zassert_equal(out[1].duration, CODEC_T);
	zassert_equal(out[1].level, 0);

	/* Second bit is a zero: short high then long low. */
	zassert_equal(out[2].duration, CODEC_T);
	zassert_equal(out[2].level, 1);
	zassert_equal(out[3].duration, 3 * CODEC_T);
}

ZTEST(pulse_io_api, test_encode_decode_lsb_first)
{
	struct pulse_io_bit_template lsb = codec_tmpl;

	lsb.msb_first = false;

	const uint8_t msg[] = {0x35, 0xC2};
	struct pulse_symbol syms[sizeof(msg) * 8 * 2];
	uint8_t decoded[sizeof(msg)];
	size_t produced = 0;

	zassert_ok(
		pulse_io_encode_bytes(&lsb, msg, sizeof(msg), syms, ARRAY_SIZE(syms), &produced));
	zassert_ok(pulse_io_decode_bytes(&lsb, CODEC_T - 1U, syms, produced, decoded,
					 sizeof(decoded), &produced));
	zassert_equal(produced, sizeof(msg));
	zassert_mem_equal(decoded, msg, sizeof(msg), "LSB-first round-trip mismatch");
}

ZTEST(pulse_io_api, test_decode_rejects_garbage)
{
	struct pulse_symbol bad[2] = {
		{.duration = 999, .level = 1}, /* matches neither one nor zero */
		{.duration = 999, .level = 0},
	};
	uint8_t out[1];
	size_t produced = 0;

	zassert_equal(pulse_io_decode_bytes(&codec_tmpl, 1, bad, 16, out, sizeof(out), &produced),
		      -EILSEQ, "garbage should be rejected");
}

ZTEST(pulse_io_api, test_codec_arg_errors)
{
	uint8_t in = 0xAA;
	struct pulse_symbol out[16];
	size_t produced = 0;

	/* NULL args. */
	zassert_equal(pulse_io_encode_bytes(NULL, &in, 1, out, ARRAY_SIZE(out), &produced),
		      -EINVAL);
	/* Output buffer too small (need 16, give 4). */
	zassert_equal(pulse_io_encode_bytes(&codec_tmpl, &in, 1, out, 4, &produced), -ENOMEM);
}

ZTEST(pulse_io_api, test_cell_mode_config)
{
	struct pulse_io_channel *ch;
	struct pulse_io_config cfg = {
		.mode = PULSE_IO_MODE_CELL,
		.dir = PULSE_IO_DIR_TX,
		.resolution_hz = 1000000,
	};

	zassert_ok(pulse_io_channel_get(dev, 0, &ch));

	/* CELL with zero period is rejected. */
	cfg.cell_period_ticks = 0;
	zassert_equal(pulse_io_channel_configure(dev, ch, &cfg), -EINVAL);

	/* CELL with a valid period is accepted. */
	cfg.cell_period_ticks = 20000;
	zassert_ok(pulse_io_channel_configure(dev, ch, &cfg));

	zassert_ok(pulse_io_channel_release(dev, ch));
}

ZTEST(pulse_io_api, test_transmit_sync_rejects_infinite_loop)
{
	struct pulse_io_channel *ch;
	struct pulse_io_config cfg = {
		.mode = PULSE_IO_MODE_SYMBOL,
		.dir = PULSE_IO_DIR_TX,
		.resolution_hz = 1000000,
	};
	struct pulse_symbol sym = {.duration = 100, .level = 1};
	struct pulse_io_tx_req req = {
		.symbols = &sym,
		.count = 1,
		.loop_count = UINT32_MAX, /* infinite */
	};

	zassert_ok(pulse_io_channel_get(dev, 1, &ch));
	zassert_ok(pulse_io_channel_configure(dev, ch, &cfg));
	zassert_equal(pulse_io_transmit_sync(dev, ch, &req, K_MSEC(10)), -ENOTSUP,
		      "infinite loop must be rejected by sync transmit");
	zassert_ok(pulse_io_channel_release(dev, ch));
}

ZTEST_SUITE(pulse_io_api, NULL, NULL, NULL, NULL, NULL);
