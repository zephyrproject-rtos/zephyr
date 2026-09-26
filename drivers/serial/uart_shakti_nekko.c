/*
 * Copyright (c) 2025 AIFoundry
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aifoundry_shakti_uart_v1

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define UART_BAUD         0x00
#define UART_TX           0x08
#define UART_RX           0x10
#define UART_STATUS       0x18
#define UART_DELAY        0x20
#define UART_CONTROL      0x28
#define UART_IEN          0x30
#define UART_RX_THRESHOLD 0x40

#define STATUS_TX_FULL      BIT(1)
#define STATUS_RX_NOT_EMPTY BIT(2)
#define STATUS_PARITY_ERROR BIT(4)
#define STATUS_OVERRUN      BIT(5)
#define STATUS_FRAME_ERROR  BIT(6)
#define STATUS_BREAK_ERROR  BIT(7)

#define CONTROL_CHARSIZE GENMASK(10, 5)
#define BAUD_OVERSAMPLE  16U

struct shakti_nekko_config {
	DEVICE_MMIO_ROM;
	uint16_t divisor;
};

struct shakti_nekko_data {
	DEVICE_MMIO_RAM;
};

static int shakti_nekko_poll_in(const struct device *dev, unsigned char *c)
{
	mem_addr_t base = DEVICE_MMIO_GET(dev);

	if ((sys_read32(base + UART_STATUS) & STATUS_RX_NOT_EMPTY) == 0U) {
		return -1;
	}

	*c = (unsigned char)sys_read32(base + UART_RX);
	return 0;
}

static void shakti_nekko_poll_out(const struct device *dev, unsigned char c)
{
	mem_addr_t base = DEVICE_MMIO_GET(dev);

	while ((sys_read32(base + UART_STATUS) & STATUS_TX_FULL) != 0U) {
	}

	sys_write32(c, base + UART_TX);
}

static int shakti_nekko_err_check(const struct device *dev)
{
	uint32_t status = sys_read32(DEVICE_MMIO_GET(dev) + UART_STATUS);
	int errors = 0;

	if ((status & STATUS_PARITY_ERROR) != 0U) {
		errors |= UART_ERROR_PARITY;
	}
	if ((status & STATUS_OVERRUN) != 0U) {
		errors |= UART_ERROR_OVERRUN;
	}
	if ((status & STATUS_FRAME_ERROR) != 0U) {
		errors |= UART_ERROR_FRAMING;
	}
	if ((status & STATUS_BREAK_ERROR) != 0U) {
		errors |= UART_BREAK;
	}

	return errors;
}

static int shakti_nekko_init(const struct device *dev)
{
	const struct shakti_nekko_config *cfg = dev->config;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	mem_addr_t base = DEVICE_MMIO_GET(dev);

	sys_write32(0U, base + UART_IEN);
	sys_write32(cfg->divisor, base + UART_BAUD);
	sys_write32(FIELD_PREP(CONTROL_CHARSIZE, 8U), base + UART_CONTROL);
	sys_write32(0U, base + UART_DELAY);
	sys_write32(0U, base + UART_RX_THRESHOLD);

	return 0;
}

static DEVICE_API(uart, shakti_nekko_api) = {
	.poll_in = shakti_nekko_poll_in,
	.poll_out = shakti_nekko_poll_out,
	.err_check = shakti_nekko_err_check,
};

#define SHAKTI_NEKKO_DIVISOR(n)                                                                    \
	(DT_INST_PROP(n, current_speed) == 0U                                                      \
		 ? 0U                                                                              \
		 : DT_INST_PROP(n, clock_frequency) /                                              \
			   (BAUD_OVERSAMPLE * (uint64_t)DT_INST_PROP(n, current_speed)))

#define SHAKTI_NEKKO_INIT(n)                                                                       \
	BUILD_ASSERT(!DT_INST_PROP(n, hw_flow_control), "Hardware flow control is not supported"); \
	BUILD_ASSERT(DT_INST_PROP(n, current_speed) > 0U, "Baud rate must be nonzero");            \
	BUILD_ASSERT(SHAKTI_NEKKO_DIVISOR(n) > 0U && SHAKTI_NEKKO_DIVISOR(n) <= UINT16_MAX,        \
		     "Baud divisor must be in the range 1..65535");                                \
	static struct shakti_nekko_data shakti_nekko_data_##n;                                     \
	static const struct shakti_nekko_config shakti_nekko_config_##n = {                        \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.divisor = SHAKTI_NEKKO_DIVISOR(n),                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, shakti_nekko_init, NULL, &shakti_nekko_data_##n,                  \
			      &shakti_nekko_config_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &shakti_nekko_api);

DT_INST_FOREACH_STATUS_OKAY(SHAKTI_NEKKO_INIT)
