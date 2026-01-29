/*
 * Copyright (c) 2026 MASSDRIVER EI (massdirv.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/misc/bflb_wo/bflb_wo.h>

#define BFLB_WO_UART_MAX_FRAME	12
#define BFLB_WO_WS2812_FRAME	24

int bflb_wo_uart(const char *const data, const uint8_t pin,
		 const struct uart_config *const uart_cfg)
{
	int ret;
	const char *ptr = data;
	uint16_t buf[BFLB_WO_UART_MAX_FRAME];
	uint16_t pin_msk = 1U << (pin % BFLB_WO_PIN_CNT);
	uint8_t data_bits;
	struct bflb_wo_config cfg = {
		.pull_down = false,
		.pull_up = true, /* Approximate UART baudrate */
		.total_cycles = bflb_wo_frequency_to_cycles(uart_cfg->baudrate, false),
		.set_cycles = 0,
		.unset_cycles = 0,
		.set_invert = true,
		.unset_invert = false,
		.park_high = true,
		.pins = BFLB_WO_PINS(pin),
	};

	if (cfg.total_cycles == 0) {
		return -EIO;
	}

	if (uart_cfg->parity != UART_CFG_PARITY_NONE) {
		return -ENOTSUP;
	}

	if (uart_cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	if (uart_cfg->stop_bits != UART_CFG_STOP_BITS_1
	    && uart_cfg->stop_bits != UART_CFG_STOP_BITS_2) {
		return -ENOTSUP;
	}

	switch (uart_cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		data_bits = 5;
		break;
	case UART_CFG_DATA_BITS_6:
		data_bits = 6;
		break;
	case UART_CFG_DATA_BITS_7:
		data_bits = 7;
		break;
	case UART_CFG_DATA_BITS_8:
		data_bits = 8;
		break;
	default:
		return -EINVAL;
	}

	ret = bflb_wo_configure(&cfg);
	if (ret != 0) {
		return ret;
	}

	while (*ptr != 0) {
		buf[0] = 0;
		for (size_t i = 0; i < data_bits; i++) {
			buf[i + 1] = ((*ptr >> i) & 0x1) ? pin_msk : 0;
		}
		buf[data_bits + 1] = pin_msk;
		if (uart_cfg->stop_bits == UART_CFG_STOP_BITS_2) {
			buf[data_bits + 2] = pin_msk;
			bflb_wo_write(buf, data_bits + 3);
		} else {
			bflb_wo_write(buf, data_bits + 2);
		}
		ptr++;
	}

	return 0;
}


int bflb_wo_ws2812b(const uint32_t *const rgb, const size_t len, const uint8_t pin)
{
	int ret;
	uint16_t buf[BFLB_WO_WS2812_FRAME];
	uint16_t pin_msk = 1U << (pin % BFLB_WO_PIN_CNT);
	struct bflb_wo_config cfg = {
		.pull_down = false,
		.pull_up = true,
		.total_cycles = bflb_wo_time_to_cycles(1250, false),
		.set_cycles = bflb_wo_time_to_cycles(400, false),
		.unset_cycles = bflb_wo_time_to_cycles(800, false),
		.set_invert = true,
		.unset_invert = true,
		.park_high = false,
		.pins = BFLB_WO_PINS(pin),
	};

	if (cfg.total_cycles == 0) {
		return -EIO;
	}

	ret = bflb_wo_configure(&cfg);
	if (ret != 0) {
		return ret;
	}

	for (size_t i = 0; i < len; i++) {
	        for (size_t j = 0; j < 8; j++) {
			buf[j] = ((rgb[i] >> (15U - j)) & 0x1) ? pin_msk : 0;
			buf[j + 8] = ((rgb[i] >> (23U - j)) & 0x1) ? pin_msk : 0;
			buf[j + 16] = ((rgb[i] >> (7U - j)) & 0x1) ? pin_msk : 0;
		}
		bflb_wo_write(buf, BFLB_WO_WS2812_FRAME);
	}

	return 0;
}
