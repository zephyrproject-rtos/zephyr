/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * Copyright (c) 2026 Dev It Wise
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/pulse_io.h>

#define CELL_PERIOD_TICKS 13U
#define FRAME_CELLS       24U

static const struct device *pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pulse_io0));
static struct pulse_io_channel *pwm_chan;

static const struct pulse_io_config cell_tx_cfg = {
	.resolution_hz = 10000000U,
	.cell_period_ticks = CELL_PERIOD_TICKS,
	.mode = PULSE_IO_MODE_CELL,
	.dir = PULSE_IO_DIR_TX,
};

static void pulse_io_nrf_pwm_after(void *fixture)
{
	struct pulse_io_channel *held = pwm_chan;

	ARG_UNUSED(fixture);

	if (held == NULL) {
		return;
	}

	pwm_chan = NULL;
	zassert_ok(pulse_io_channel_release(pwm_dev, held));
}

ZTEST(pulse_io_nrf_pwm, test_capabilities)
{
	struct pulse_io_caps caps;

	zassert_ok(pulse_io_get_capabilities(pwm_dev, &caps));
	zassert_equal(caps.modes, PULSE_IO_MODE_CELL);
	zassert_true(caps.supports_tx);
	zassert_false(caps.supports_rx);
	zassert_equal(caps.num_channels, 1U);
	zassert_equal(caps.tx_channel_mask, BIT(0));
	zassert_equal(caps.rx_channel_mask, 0U);
	zassert_equal(caps.tx_max_streaming, CONFIG_PULSE_IO_NRF_PWM_CELLS);
	zassert_equal(caps.cell_duty_bits, 15U);
}

ZTEST(pulse_io_nrf_pwm, test_channel_index)
{
	zassert_equal(pulse_io_channel_get(pwm_dev, 1, &pwm_chan), -ENODEV);
}

ZTEST(pulse_io_nrf_pwm, test_configure_refusals)
{
	struct pulse_io_config cfg = {
		.resolution_hz = 1000U,
		.cell_period_ticks = 262U,
		.mode = PULSE_IO_MODE_SYMBOL,
		.dir = PULSE_IO_DIR_TX,
	};

	zassert_ok(pulse_io_channel_get(pwm_dev, 0, &pwm_chan));
	zassert_equal(pulse_io_channel_configure(pwm_dev, pwm_chan, &cfg), -ENOTSUP);

	cfg.mode = PULSE_IO_MODE_CELL;
	cfg.dir = PULSE_IO_DIR_RX;
	zassert_equal(pulse_io_channel_configure(pwm_dev, pwm_chan, &cfg), -ENOTSUP);

	cfg.dir = PULSE_IO_DIR_TX;
	cfg.carrier_en = true;
	zassert_equal(pulse_io_channel_configure(pwm_dev, pwm_chan, &cfg), -ENOTSUP);

	cfg.carrier_en = false;
	cfg.cell_period_ticks = 263U;
	zassert_equal(pulse_io_channel_configure(pwm_dev, pwm_chan, &cfg), -EINVAL);

	cfg.cell_period_ticks = 262U;
	zassert_ok(pulse_io_channel_configure(pwm_dev, pwm_chan, &cfg));
}

ZTEST(pulse_io_nrf_pwm, test_transmit_cells)
{
	struct pulse_cell cells[FRAME_CELLS];
	struct pulse_io_tx_req req = {.cells = cells, .count = ARRAY_SIZE(cells)};

	for (size_t i = 0U; i < ARRAY_SIZE(cells); i++) {
		cells[i] = (struct pulse_cell){.duty = ((i % 2U) == 0U) ? 4U : 9U};
	}

	zassert_ok(pulse_io_channel_get(pwm_dev, 0, &pwm_chan));
	zassert_ok(pulse_io_channel_configure(pwm_dev, pwm_chan, &cell_tx_cfg));
	zassert_ok(pulse_io_transmit_sync(pwm_dev, pwm_chan, &req, K_MSEC(10)));

	cells[FRAME_CELLS - 1U].duty = CELL_PERIOD_TICKS + 1U;
	zassert_equal(pulse_io_transmit_sync(pwm_dev, pwm_chan, &req, K_MSEC(10)), -EINVAL);

	cells[FRAME_CELLS - 1U].duty = 9U;
	zassert_ok(pulse_io_transmit_sync(pwm_dev, pwm_chan, &req, K_MSEC(10)));
}

ZTEST(pulse_io_nrf_pwm, test_transmit_refusals)
{
	static struct pulse_cell oversized[CONFIG_PULSE_IO_NRF_PWM_CELLS + 1U];
	struct pulse_io_tx_req req = {.cells = oversized};

	zassert_ok(pulse_io_channel_get(pwm_dev, 0, &pwm_chan));
	zassert_ok(pulse_io_channel_configure(pwm_dev, pwm_chan, &cell_tx_cfg));

	req.count = 0U;
	zassert_equal(pulse_io_transmit_sync(pwm_dev, pwm_chan, &req, K_MSEC(10)), -EINVAL);

	req.count = ARRAY_SIZE(oversized);
	zassert_equal(pulse_io_transmit_sync(pwm_dev, pwm_chan, &req, K_MSEC(10)), -ENOMEM);

	req.count = 1U;
	req.loop_count = 1U;
	zassert_equal(pulse_io_transmit_sync(pwm_dev, pwm_chan, &req, K_MSEC(10)), -ENOTSUP);
}

ZTEST(pulse_io_nrf_pwm, test_receive)
{
	struct pulse_cell cells[1];
	struct pulse_io_rx_req req = {.cells = cells, .capacity = ARRAY_SIZE(cells)};
	size_t count = 0U;

	zassert_ok(pulse_io_channel_get(pwm_dev, 0, &pwm_chan));
	zassert_equal(pulse_io_receive_sync(pwm_dev, pwm_chan, &req, &count, K_MSEC(10)), -ENOTSUP);
}

ZTEST_SUITE(pulse_io_nrf_pwm, NULL, NULL, NULL, pulse_io_nrf_pwm_after, NULL);
