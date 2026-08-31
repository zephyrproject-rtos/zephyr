/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mediatek_mt8188_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>

#include "pinctrl_mtk_common.h"

#define PINCTRL_BASE_ADDR DT_INST_REG_ADDR(0)

#define PINCTRL_OFFSET_MODE_0  0x0300
#define PINCTRL_OFFSET_MODE_1  0x0310
#define PINCTRL_OFFSET_MODE_2  0x0320
#define PINCTRL_OFFSET_MODE_3  0x0330
#define PINCTRL_OFFSET_MODE_4  0x0340
#define PINCTRL_OFFSET_MODE_5  0x0350
#define PINCTRL_OFFSET_MODE_6  0x0360
#define PINCTRL_OFFSET_MODE_7  0x0370
#define PINCTRL_OFFSET_MODE_8  0x0380
#define PINCTRL_OFFSET_MODE_9  0x0390
#define PINCTRL_OFFSET_MODE_10 0x03a0
#define PINCTRL_OFFSET_MODE_11 0x03b0
#define PINCTRL_OFFSET_MODE_12 0x03c0
#define PINCTRL_OFFSET_MODE_13 0x03d0
#define PINCTRL_OFFSET_MODE_14 0x03e0
#define PINCTRL_OFFSET_MODE_15 0x03f0
#define PINCTRL_OFFSET_MODE_16 0x0400
#define PINCTRL_OFFSET_MODE_17 0x0410
#define PINCTRL_OFFSET_MODE_18 0x0420
#define PINCTRL_OFFSET_MODE_19 0x0430
#define PINCTRL_OFFSET_MODE_20 0x0440
#define PINCTRL_OFFSET_MODE_21 0x0450
#define PINCTRL_OFFSET_MODE_22 0x0460

static const pinctrl_mtk_config_t pinctrl_mtk_config = {
	.max_pin = 177,
	.max_func = 8,
};

static const uint32_t pin_to_mode_offset_map[] = {
	/*   0 -   7 */ PINCTRL_OFFSET_MODE_0,
	/*   8 -  15 */ PINCTRL_OFFSET_MODE_1,
	/*  16 -  23 */ PINCTRL_OFFSET_MODE_2,
	/*  24 -  31 */ PINCTRL_OFFSET_MODE_3,
	/*  32 -  39 */ PINCTRL_OFFSET_MODE_4,
	/*  40 -  47 */ PINCTRL_OFFSET_MODE_5,
	/*  48 -  55 */ PINCTRL_OFFSET_MODE_6,
	/*  56 -  63 */ PINCTRL_OFFSET_MODE_7,
	/*  64 -  71 */ PINCTRL_OFFSET_MODE_8,
	/*  72 -  79 */ PINCTRL_OFFSET_MODE_9,
	/*  80 -  87 */ PINCTRL_OFFSET_MODE_10,
	/*  88 -  95 */ PINCTRL_OFFSET_MODE_11,
	/*  96 - 103 */ PINCTRL_OFFSET_MODE_12,
	/* 104 - 111 */ PINCTRL_OFFSET_MODE_13,
	/* 112 - 119 */ PINCTRL_OFFSET_MODE_14,
	/* 120 - 127 */ PINCTRL_OFFSET_MODE_15,
	/* 128 - 135 */ PINCTRL_OFFSET_MODE_16,
	/* 136 - 143 */ PINCTRL_OFFSET_MODE_17,
	/* 144 - 151 */ PINCTRL_OFFSET_MODE_18,
	/* 152 - 159 */ PINCTRL_OFFSET_MODE_19,
	/* 160 - 167 */ PINCTRL_OFFSET_MODE_20,
	/* 168 - 175 */ PINCTRL_OFFSET_MODE_21,
	/* 176 - 176 */ PINCTRL_OFFSET_MODE_22,
	/* 177 - 183 - Reserved */
};

static uint32_t to_mode_offset(uint16_t pin);
static uint32_t to_mode_shift(uint16_t pin);
static uint32_t to_mode_mask(uint16_t pin);

static uint32_t to_mode_offset(uint16_t pin)
{
	/* Each 32 bit MODE register controls 8 pins. */
	return pin_to_mode_offset_map[pin / 8];
}

static uint32_t to_mode_shift(uint16_t pin)
{
	return (pin % 8) * 4;
}

static uint32_t to_mode_mask(uint16_t pin)
{
	return 0xf << to_mode_shift(pin);
}

static int pinctrl_set_func(uint16_t pin, uint16_t func)
{
	uint32_t val;
	uint32_t offset;

	if ((pin >= pinctrl_mtk_config.max_pin) || (func >= pinctrl_mtk_config.max_func)) {
		return -EINVAL;
	}

	offset = to_mode_offset(pin);

	/* Read the existing function value and replace */
	/* with the new function value.                 */
	val = sys_read32(PINCTRL_BASE_ADDR + offset);
	val &= ~(to_mode_mask(pin));
	val |= ((uint32_t)func) << to_mode_shift(pin);

	sys_write32(val, (PINCTRL_BASE_ADDR + offset));

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	while (pin_cnt > 0) {
		int ret;

		ret = pinctrl_set_func(pins->pin, pins->func);
		if (ret != 0) {
			return ret;
		}

		pins++;
		pin_cnt--;
	}

	return 0;
}
