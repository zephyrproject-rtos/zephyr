/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio/gpio_bl61x_wo.h>

#define UART_PINS_NODE		DT_PATH(wo_uart)

#define UART_STR		"Hello World!"
/* Append line feed for clean output */
#define UART_STR_FED		(UART_STR "\r\n")
/* 8 toggles of data */
#define UART_FRAME_DATA		8
/* 1 toggle for the start bit (low) */
#define UART_FRAME_START	1
/* 1 toggle for the end bit (high) */
#define UART_FRAME_STOP		1
#define UART_FRAME		(UART_FRAME_DATA + UART_FRAME_START + UART_FRAME_STOP)
#define UART_BAUDRATE		115200

static const struct device *default_uart = DEVICE_DT_GET(DT_INST(0, bflb_uart));

#define PIN_MSK_IS_IMPLX(_n, _p, _i) | (1U << (DT_GPIO_PIN_BY_IDX(_n, _p, _i) % BL61X_WO_PIN_CNT))
#define PIN_MSK_IS_IMPL0(_n, _p, _i) (1U << (DT_GPIO_PIN_BY_IDX(_n, _p, _i) % BL61X_WO_PIN_CNT))
#define PIN_MSK_IS_IMPL(_n, _p, _i) \
	COND_CODE_0(_i, (PIN_MSK_IS_IMPL0(_n, _p, _i)), (PIN_MSK_IS_IMPLX(_n, _p, _i)))

#define PIN_MSK_IS(_n) \
	(DT_FOREACH_PROP_ELEM(_n, gpios, PIN_MSK_IS_IMPL))

#define GET_GPIO_AT(_n, _p, _i) \
	GPIO_DT_SPEC_GET_BY_IDX(_n, _p, _i),

static const struct gpio_dt_spec uart_gpios[] = {
	DT_FOREACH_PROP_ELEM(UART_PINS_NODE, gpios, GET_GPIO_AT)
};

/* Callback to repeat data received on UART RX as to view WO output without an additional UART */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(default_uart)) {
		return;
	}

	if (!uart_irq_rx_ready(default_uart)) {
		return;
	}

	while (uart_fifo_read(default_uart, &c, 1) == 1) {
		uart_poll_out(default_uart, c);
	}
}

int bflb_wo_uart_configure(void)
{
	struct bl61x_wo_config cfg = {
		/* Approximate UART baudrate:
		 * with the typical 40MHz crystal setup it can't be exactly 115200 bauds,
		 * as 40000000 / 115200 = 347.22 WO cycles.
		 * but it can be close enough for most adapters (< 0.1 % error)
		 */
		.total_cycles = bl61x_wo_frequency_to_cycles(UART_BAUDRATE, false),
		/* Full length is second part of the cycle */
		.set_cycles = 0,
		.unset_cycles = 0,
		/* Second part of set cycle is high */
		.set_invert = true,
		/* Second part of unset cycle is low */
		.unset_invert = false,
		.park_high = true,
	};

	/* Somehow the cycle count is invalid, abort */
	if (cfg.total_cycles == 0) {
		return -EIO;
	}

	/* Configure timing, pins, and their flags */
	return bl61x_wo_configure_dt(&cfg, uart_gpios, ARRAY_SIZE(uart_gpios));
}

void bflb_wo_uart_write(const char *const data)
{

	const char *ptr = data;
	const uint16_t pin_msk = PIN_MSK_IS(UART_PINS_NODE);
	/* Buffer the size of one UART 'frame' (start bit, data bits, stop bit) */
	uint16_t buf[UART_FRAME];

	while (*ptr != 0) {
		/* Start Bit */
		for (size_t i = 0; i < UART_FRAME_START; i++) {
			buf[i] = 0;
		}
		/* Data bits, iterate over all 8 bits of the byte */
		for (size_t i = 0; i < UART_FRAME_DATA; i++) {
			/* Set pin's ID in the 16-bits mask if the bit should be set */
			buf[i + UART_FRAME_START] = ((*ptr >> i) & 0x1) ? pin_msk : 0;
		}
		/* Stop Bit(s) */
		for (size_t i = 0; i < UART_FRAME_STOP; i++) {
			buf[i + UART_FRAME_DATA + UART_FRAME_START] = pin_msk;
		}
		bl61x_wo_write(buf, UART_FRAME);
		ptr++;
	}
}

int main(void)
{
	int ret;

	if (!device_is_ready(default_uart)) {
		printk("Logging UART device unavailable\n");
		return 0;
	}

	ret = uart_irq_callback_user_data_set(default_uart, serial_cb, NULL);
	if (ret < 0) {
		printk("Error setting Logging UART callback: %d\n", ret);
		return 0;
	}

	ret = bflb_wo_uart_configure();
	if (ret < 0) {
		printk("Couldn't configure Wire Out: %d\n", ret);
		return 0;
	}

	uart_irq_rx_enable(default_uart);

	while (true) {
		bflb_wo_uart_write(UART_STR_FED);
		printf("Sent: %s\n", UART_STR);
		k_msleep(1000);
	}
}
