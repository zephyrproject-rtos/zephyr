/*
 * Copyright (c) 2018-2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is based on SW_DP.c from CMSIS-DAP Source (Revision:    V2.0.0)
 * https://github.com/ARM-software/CMSIS_5/tree/develop/CMSIS/DAP/Firmware
 * Copyright (c) 2013-2017, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */


/* Serial Wire Debug Port interface bit-bang driver */

#define DT_DRV_COMPAT zephyr_swdp_gpio

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/swdp.h>

#include "swdp_ll_pin.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(swdp, CONFIG_DP_DRIVER_LOG_LEVEL);

#define CLOCK_DELAY(swclk_freq, port_write_cycles) \
	((CPU_CLOCK / 2 / swclk_freq) - port_write_cycles)

/*
 * Default SWCLK frequency in Hz.
 * sw_clock can be used to overwrite this default value.
 */
#define SWDP_DEFAULT_SWCLK_FREQUENCY	1000000U

#define DELAY_SLOW_CYCLES		3U

struct sw_config {
	struct gpio_dt_spec clk;
	struct gpio_dt_spec dout;
	struct gpio_dt_spec dio;
	struct gpio_dt_spec dnoe;
	void *dout_reg;
	void *dio_reg;
	void *dnoe_reg;
	struct gpio_dt_spec noe;
	struct gpio_dt_spec reset;
	uint32_t port_write_cycles;
	void *clk_reg;
};

struct sw_cfg_data {
	uint32_t clock_delay;
	uint8_t turnaround;
	bool data_phase;
	bool fast_clock;
};

/*
 * Move A[2:3], RnW, APnDP bits to their position,
 * add start bit, stop bit(6), park bit and parity bit.
 * For example, reading IDCODE would be APnDP=0, RnW=1, A2=0, A3=0.
 * The request would be 0xa5, which is 10100101 in binary.
 *
 * For more information, see:
 * - CMSIS-DAP Command Specification, DAP_Transfer
 * - ARM Debug Interface v5 Architecture Specification
 */
const static uint8_t sw_request_lut[16] = {
	0x81, 0xa3, 0xa5, 0x87, 0xa9, 0x8b, 0x8d, 0xaf,
	0xb1, 0x93, 0x95, 0xb7, 0x99, 0xbb, 0xbd, 0x9f
};

static ALWAYS_INLINE uint32_t sw_get32bit_parity(uint32_t data)
{
	data ^= data >> 16;
	data ^= data >> 8;
	data ^= data >> 4;
	data ^= data >> 2;
	data ^= data >> 1;

	return data & 1U;
}

/* Set SWCLK DAP hardware output pin to high level */
static ALWAYS_INLINE void pin_swclk_set(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	if (FAST_BITBANG_HW_SUPPORT) {
		swdp_ll_pin_set(config->clk_reg, config->clk.pin);
	} else {
		gpio_pin_set_dt(&config->clk, 1);
	}
}

/* Set SWCLK DAP hardware output pin to low level */
static ALWAYS_INLINE void pin_swclk_clr(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	if (FAST_BITBANG_HW_SUPPORT) {
		swdp_ll_pin_clr(config->clk_reg, config->clk.pin);
	} else {
		gpio_pin_set_dt(&config->clk, 0);
	}
}

/* Set the SWDIO DAP hardware output pin to high level */
static ALWAYS_INLINE void pin_swdio_set(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	if (config->dout.port) {
		if (FAST_BITBANG_HW_SUPPORT) {
			swdp_ll_pin_set(config->dout_reg, config->dout.pin);
		} else {
			gpio_pin_set_dt(&config->dout, 1);
		}
	} else {
		if (FAST_BITBANG_HW_SUPPORT) {
			swdp_ll_pin_set(config->dio_reg, config->dio.pin);
		} else {
			gpio_pin_set_dt(&config->dio, 1);
		}
	}
}

/* Set the SWDIO DAP hardware output pin to low level */
static ALWAYS_INLINE void pin_swdio_clr(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	if (config->dout.port) {
		if (FAST_BITBANG_HW_SUPPORT) {
			swdp_ll_pin_clr(config->dout_reg, config->dout.pin);
		} else {
			gpio_pin_set_dt(&config->dout, 0);
		}
	} else {
		if (FAST_BITBANG_HW_SUPPORT) {
			swdp_ll_pin_clr(config->dio_reg, config->dio.pin);
		} else {
			gpio_pin_set_dt(&config->dio, 0);
		}
	}
}

/* Set the SWDIO DAP hardware output pin to bit level */
static ALWAYS_INLINE void pin_swdio_out(const struct device *dev,
					const uint32_t bit)
{
	if (bit & 1U) {
		pin_swdio_set(dev);
	} else {
		pin_swdio_clr(dev);
	}
}

/* Return current level of the SWDIO DAP hardware input pin */
static ALWAYS_INLINE uint32_t pin_swdio_in(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	if (FAST_BITBANG_HW_SUPPORT) {
		return swdp_ll_pin_get(config->dio_reg, config->dio.pin);
	} else {
		return gpio_pin_get_dt(&config->dio);
	}
}

/*
 * Configure the SWDIO DAP hardware to output mode.
 * This is default configuration for every transfer.
 */
static ALWAYS_INLINE void pin_swdio_out_enable(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	if (config->dnoe.port) {
		if (FAST_BITBANG_HW_SUPPORT) {
			swdp_ll_pin_set(config->dnoe_reg, config->dnoe.pin);
		} else {
			gpio_pin_set_dt(&config->dnoe, 1);
		}
	} else {
		if (FAST_BITBANG_HW_SUPPORT) {
			swdp_ll_pin_output(config->dio_reg, config->dio.pin);
		} else {
			gpio_pin_configure_dt(&config->dio, GPIO_OUTPUT_ACTIVE);
		}
	}
}

/*
 * Configure the SWDIO DAP hardware to input mode.
 */
static ALWAYS_INLINE void pin_swdio_out_disable(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	if (config->dnoe.port) {
		if (FAST_BITBANG_HW_SUPPORT) {
			swdp_ll_pin_clr(config->dnoe_reg, config->dnoe.pin);
		} else {
			gpio_pin_set_dt(&config->dnoe, 0);
		}
	} else {
		if (FAST_BITBANG_HW_SUPPORT) {
			swdp_ll_pin_input(config->dio_reg, config->dio.pin);
		} else {
			gpio_pin_configure_dt(&config->dio, GPIO_INPUT);
		}
	}
}

#define SW_CLOCK_CYCLE(dev, delay)			\
	do {						\
		pin_swclk_clr(dev);			\
		pin_delay_asm(delay);			\
		pin_swclk_set(dev);			\
		pin_delay_asm(delay);			\
	} while (0)

#define SW_WRITE_BIT(dev, bit, delay)			\
	do {						\
		pin_swdio_out(dev, bit);		\
		pin_swclk_clr(dev);			\
		pin_delay_asm(delay);			\
		pin_swclk_set(dev);			\
		pin_delay_asm(delay);			\
	} while (0)

#define SW_READ_BIT(dev, bit, delay)			\
	do {						\
		pin_swclk_clr(dev);			\
		pin_delay_asm(delay);			\
		bit = pin_swdio_in(dev);		\
		pin_swclk_set(dev);			\
		pin_delay_asm(delay);			\
	} while (0)

static int sw_output_sequence(const struct device *dev, uint32_t count,
			      const uint8_t *data)
{
	struct sw_cfg_data *sw_data = dev->data;
	unsigned int key;
	uint32_t val = 0; /* current byte */
	uint32_t n = 0; /* bit counter */

	LOG_DBG("writing %u bits", count);
	LOG_HEXDUMP_DBG(data, count, "sequence bit data");
	key = irq_lock();

	pin_swdio_out_enable(dev);
	while (count--) {
		if (n == 0U) {
			val = *data++;
			n = 8U;
		}
		if (val & 1U) {
			pin_swdio_set(dev);
		} else {
			pin_swdio_clr(dev);
		}
		SW_CLOCK_CYCLE(dev, sw_data->clock_delay);
		val >>= 1;
		n--;
	}

	irq_unlock(key);

	return 0;
}

static int sw_input_sequence(const struct device *dev, uint32_t count,
			     uint8_t *data)
{
	struct sw_cfg_data *sw_data = dev->data;
	unsigned int key;
	uint32_t val = 0U; /* current byte */
	uint32_t n = 8U; /* bit counter */
	uint32_t bit;

	LOG_DBG("reading %u bits", count);
	key = irq_lock();

	pin_swdio_out_disable(dev);
	while (count--) {
		if (n == 0U) {
			*data++ = val;
			val = 0;
			n = 8U;
		}
		SW_READ_BIT(dev, bit, sw_data->clock_delay);
		LOG_DBG("Read bit: %d", bit);
		val = (val << 1 | bit);
		n--;
	}

	*data = val; /* write last byte */
	irq_unlock(key);

	return 0;
}

static ALWAYS_INLINE void sw_cycle_turnaround(const struct device *dev)
{
	struct sw_cfg_data *sw_data = dev->data;
	uint32_t n;

	for (n = sw_data->turnaround; n; n--) {
		SW_CLOCK_CYCLE(dev, sw_data->clock_delay);
	}
}

static int sw_transfer(const struct device *dev,
		       const uint8_t request, uint32_t *const data,
		       const uint8_t idle_cycles, uint8_t *const response)
{
	struct sw_cfg_data *sw_data = dev->data;
	unsigned int key;
	uint32_t ack;
	uint32_t bit;
	uint32_t val;
	uint32_t parity = 0;
	uint32_t n;

	pin_swdio_out_enable(dev);

	LOG_DBG("request 0x%02x idle %u", request, idle_cycles);
	if (!(request & SWDP_REQUEST_RnW)) {
		LOG_DBG("write data 0x%08x", *data);
		parity = sw_get32bit_parity(*data);
	}

	key = irq_lock();

	val = sw_request_lut[request & 0xFU];
	for (n = 8U; n; n--) {
		SW_WRITE_BIT(dev, val, sw_data->clock_delay);
		val >>= 1;
	}

	pin_swdio_out_disable(dev);
	sw_cycle_turnaround(dev);

	/* Acknowledge response */
	SW_READ_BIT(dev, bit, sw_data->clock_delay);
	ack = bit << 0;
	SW_READ_BIT(dev, bit, sw_data->clock_delay);
	ack |= bit << 1;
	SW_READ_BIT(dev, bit, sw_data->clock_delay);
	ack |= bit << 2;

	if (ack == SWDP_ACK_OK) {
		/* Data transfer */
		if (request & SWDP_REQUEST_RnW) {
			/* Read data */
			val = 0U;
			for (n = 32U; n; n--) {
				/* Read RDATA[0:31] */
				SW_READ_BIT(dev, bit, sw_data->clock_delay);
				val >>= 1;
				val |= bit << 31;
			}

			/* Read parity bit */
			SW_READ_BIT(dev, bit, sw_data->clock_delay);
			sw_cycle_turnaround(dev);
			pin_swdio_out_enable(dev);

			if ((sw_get32bit_parity(val) ^ bit) & 1U) {
				ack = SWDP_TRANSFER_ERROR;
			}

			if (data) {
				*data = val;
			}

		} else {
			sw_cycle_turnaround(dev);

			pin_swdio_out_enable(dev);
			/* Write data */
			val = *data;
			for (n = 32U; n; n--) {
				SW_WRITE_BIT(dev, val, sw_data->clock_delay);
				val >>= 1;
			}

			/* Write parity bit */
			SW_WRITE_BIT(dev, parity, sw_data->clock_delay);
		}
		/* Idle cycles */
		n = idle_cycles;
		if (n) {
			pin_swdio_out(dev, 0U);
			for (; n; n--) {
				SW_CLOCK_CYCLE(dev, sw_data->clock_delay);
			}
		}

		pin_swdio_out(dev, 1U);
		irq_unlock(key);
		if (request & SWDP_REQUEST_RnW) {
			LOG_DBG("read data 0x%08x", *data);
		}

		if (response) {
			*response = (uint8_t)ack;
		}

		return 0;
	}

	if ((ack == SWDP_ACK_WAIT) || (ack == SWDP_ACK_FAULT)) {
		/* WAIT OR fault response */
		if (sw_data->data_phase) {
			for (n = 32U + 1U + sw_data->turnaround; n; n--) {
				/* Dummy Read RDATA[0:31] + Parity */
				SW_CLOCK_CYCLE(dev, sw_data->clock_delay);
			}
		} else {
			sw_cycle_turnaround(dev);
		}

		pin_swdio_out_enable(dev);
		pin_swdio_out(dev, 1U);
		irq_unlock(key);
		LOG_DBG("Transfer wait or fault");
		if (response) {
			*response = (uint8_t)ack;
		}

		return 0;
	}

	/* Protocol error */
	for (n = sw_data->turnaround + 32U + 1U; n; n--) {
		/* Back off data phase */
		SW_CLOCK_CYCLE(dev, sw_data->clock_delay);
	}

	pin_swdio_out_enable(dev);
	pin_swdio_out(dev, 1U);
	irq_unlock(key);
	LOG_INF("Protocol error");
	if (response) {
		*response = (uint8_t)ack;
	}

	return 0;
}

static int sw_set_pins(const struct device *dev,
		       const uint8_t pins, const uint8_t value)
{
	const struct sw_config *config = dev->config;

	LOG_DBG("pins 0x%02x value 0x%02x", pins, value);

	if (pins & BIT(SWDP_SWCLK_PIN)) {
		if (value & BIT(SWDP_SWCLK_PIN)) {
			gpio_pin_set_dt(&config->clk, 1);
		} else {
			gpio_pin_set_dt(&config->clk, 0);
		}
	}

	if (config->dout_reg != NULL) {
		if (pins & BIT(SWDP_SWDIO_PIN)) {
			if (value & BIT(SWDP_SWDIO_PIN)) {
				gpio_pin_set_dt(&config->dout, 1);
			} else {
				gpio_pin_set_dt(&config->dout, 0);
			}
		}
	} else {
		if (pins & BIT(SWDP_SWDIO_PIN)) {
			if (value & BIT(SWDP_SWDIO_PIN)) {
				gpio_pin_set_dt(&config->dio, 1);
			} else {
				gpio_pin_set_dt(&config->dio, 0);
			}
		}
	}

	if (config->reset.port) {
		if (pins & BIT(SWDP_nRESET_PIN)) {
			if (value & BIT(SWDP_nRESET_PIN)) {
				gpio_pin_set_dt(&config->reset, 1);
			} else {
				gpio_pin_set_dt(&config->reset, 0);
			}
		}
	}

	return 0;
}

static int sw_get_pins(const struct device *dev, uint8_t *const state)
{
	const struct sw_config *config = dev->config;
	uint32_t val;

	if (config->reset.port) {
		val = gpio_pin_get_dt(&config->reset);
		*state = val ? BIT(SWDP_nRESET_PIN) : 0;
	}

	val = gpio_pin_get_dt(&config->dio);
	*state |= val ? BIT(SWDP_SWDIO_PIN) : 0;

	val = gpio_pin_get_dt(&config->clk);
	*state |= val ? BIT(SWDP_SWCLK_PIN) : 0;

	LOG_DBG("pins state 0x%02x", *state);

	return 0;
}

static int sw_set_clock(const struct device *dev, const uint32_t clock)
{
	const struct sw_config *config = dev->config;
	struct sw_cfg_data *sw_data = dev->data;
	uint32_t delay;

	sw_data->fast_clock = false;
	delay = ((CPU_CLOCK / 2U) + (clock - 1U)) / clock;

	if (delay > config->port_write_cycles) {
		delay -= config->port_write_cycles;
		delay = (delay + (DELAY_SLOW_CYCLES - 1U)) / DELAY_SLOW_CYCLES;
	} else {
		delay = 1U;
	}

	sw_data->clock_delay = delay;

	LOG_WRN("cpu_clock %d, delay %d", CPU_CLOCK, sw_data->clock_delay);

	return 0;
}

static int sw_configure(const struct device *dev,
			const uint8_t turnaround, const bool data_phase)
{
	struct sw_cfg_data *sw_data = dev->data;

	sw_data->turnaround = turnaround;
	sw_data->data_phase = data_phase;

	LOG_INF("turnaround %d, data_phase %d",
		sw_data->turnaround, sw_data->data_phase);

	return 0;
}

static int sw_port_on(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	gpio_pin_set_dt(&config->clk, 1);

	if (config->dnoe.port) {
		gpio_pin_set_dt(&config->dnoe, 1);
	}

	if (config->dout.port) {
		gpio_pin_set_dt(&config->dout, 1);
	} else {
		int ret;

		ret = gpio_pin_configure_dt(&config->dio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
	}

	if (config->noe.port) {
		gpio_pin_set_dt(&config->noe, 1);
	}
	if (config->reset.port) {
		gpio_pin_set_dt(&config->reset, 1);
	}

	return 0;
}

static int sw_port_off(const struct device *dev)
{
	const struct sw_config *config = dev->config;

	if (config->dnoe.port) {
		gpio_pin_set_dt(&config->dnoe, 0);
	}

	if (config->dout.port) {
		gpio_pin_set_dt(&config->dout, 0);
	} else {
		int ret;

		ret = gpio_pin_configure_dt(&config->dio, GPIO_INPUT);
		if (ret) {
			return ret;
		}
	}

	if (config->noe.port) {
		gpio_pin_set_dt(&config->noe, 0);
	}
	if (config->reset.port) {
		gpio_pin_set_dt(&config->reset, 1);
	}

	return 0;
}

static int sw_gpio_init(const struct device *dev)
{
	const struct sw_config *config = dev->config;
	struct sw_cfg_data *sw_data = dev->data;
	int ret;

	ret = gpio_pin_configure_dt(&config->clk, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->dio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	if (config->dout.port) {
		ret = gpio_pin_configure_dt(&config->dout, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
	}

	if (config->dnoe.port) {
		ret = gpio_pin_configure_dt(&config->dnoe, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	}

	if (config->noe.port) {
		ret = gpio_pin_configure_dt(&config->noe, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	}

	if (config->reset.port) {
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
	}

	sw_data->turnaround = 1U;
	sw_data->data_phase = false;
	sw_data->fast_clock = false;
	sw_data->clock_delay = CLOCK_DELAY(SWDP_DEFAULT_SWCLK_FREQUENCY,
					   config->port_write_cycles);

	return 0;
}

static struct swdp_api swdp_bitbang_api = {
	.swdp_output_sequence = sw_output_sequence,
	.swdp_input_sequence = sw_input_sequence,
	.swdp_transfer	= sw_transfer,
	.swdp_set_pins	= sw_set_pins,
	.swdp_get_pins	= sw_get_pins,
	.swdp_set_clock	= sw_set_clock,
	.swdp_configure	= sw_configure,
	.swdp_port_on	= sw_port_on,
	.swdp_port_off	= sw_port_off,
};

#define SW_GPIOS_GET_REG(n, gpios)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, gpios),					\
		    (INT_TO_POINTER(DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(n), gpios)))),	\
		    (NULL))

#define SW_DEVICE_DEFINE(n)								\
	BUILD_ASSERT((DT_INST_NODE_HAS_PROP(n, dout_gpios)) ==				\
		     (DT_INST_NODE_HAS_PROP(n, dnoe_gpios)),				\
		     "Either the dout-gpios or dnoe-gpios property is missing.");	\
											\
	static const struct sw_config sw_cfg_##n = {					\
		.clk = GPIO_DT_SPEC_INST_GET(n, clk_gpios),				\
		.clk_reg = SW_GPIOS_GET_REG(n, clk_gpios),				\
		.dio = GPIO_DT_SPEC_INST_GET(n, dio_gpios),				\
		.dio_reg = SW_GPIOS_GET_REG(n, dio_gpios),				\
		.dout = GPIO_DT_SPEC_INST_GET_OR(n, dout_gpios, {0}),			\
		.dout_reg = SW_GPIOS_GET_REG(n, dout_gpios),				\
		.dnoe = GPIO_DT_SPEC_INST_GET_OR(n, dnoe_gpios, {0}),			\
		.dnoe_reg = SW_GPIOS_GET_REG(n, dnoe_gpios),				\
		.noe = GPIO_DT_SPEC_INST_GET_OR(n, noe_gpios, {0}),			\
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),			\
		.port_write_cycles = DT_INST_PROP(n, port_write_cycles),		\
	};										\
											\
	static struct sw_cfg_data sw_data_##n;						\
											\
	DEVICE_DT_INST_DEFINE(n, sw_gpio_init, NULL,					\
			      &sw_data_##n, &sw_cfg_##n,				\
			      POST_KERNEL, CONFIG_DP_DRIVER_INIT_PRIO,			\
			      &swdp_bitbang_api);

DT_INST_FOREACH_STATUS_OKAY(SW_DEVICE_DEFINE)
