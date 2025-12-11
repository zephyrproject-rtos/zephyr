/*
 * Copyright (c) 2023 Tokita, Hiroshi <tokita.hiroshi@fujitsu.com>
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>

#define DT_DRV_COMPAT raspberrypi_pico_pio

struct pio_rpi_pico_config {
	PIO pio;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const struct reset_dt_spec reset;
};

int pio_rpi_pico_allocate_sm(const struct device *dev, size_t *sm)
{
	const struct pio_rpi_pico_config *config = dev->config;
	int retval;

	retval = pio_claim_unused_sm(config->pio, false);
	if (retval < 0) {
		return -EBUSY;
	}

	*sm = (size_t)retval;
	return 0;
}

static int pio_rpi_pico_init(const struct device *dev)
{
	const struct pio_rpi_pico_config *config = dev->config;
	int ret;

	ret = clock_control_on(config->clk_dev, config->clk_id);
	if (ret < 0) {
		return ret;
	}

	ret = reset_line_toggle_dt(&config->reset);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define RPI_PICO_PIO_INIT(idx)                                                                     \
	static const struct pio_rpi_pico_config pio_rpi_pico_config_##idx = {                      \
		.pio = (PIO)DT_INST_REG_ADDR(idx),                                                 \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                                \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(0, clocks, 0, clk_id),        \
		.reset = RESET_DT_SPEC_INST_GET(idx),                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &pio_rpi_pico_init, NULL, NULL, &pio_rpi_pico_config_##idx,     \
			      PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(RPI_PICO_PIO_INIT)
