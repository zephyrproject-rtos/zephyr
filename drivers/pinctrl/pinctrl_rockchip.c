/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rockchip_rk3588_pinctrl

#include <errno.h>
#include <stdint.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define RK3588_PMU1_IOC     0x0000U
#define RK3588_PMU2_IOC     0x4000U
#define RK3588_BUS_IOC      0x8000U
#define RK3588_VCCIO1_4_IOC 0x9000U
#define RK3588_VCCIO3_5_IOC 0xa000U
#define RK3588_VCCIO2_IOC   0xb000U
#define RK3588_VCCIO6_IOC   0xc000U
#define RK3588_EMMC_IOC     0xd000U

#define RK3588_PINCTRL_BIAS_COUNT(node_id)                                                         \
	(DT_PROP(node_id, bias_disable) + DT_PROP(node_id, bias_pull_down) +                       \
	 DT_PROP(node_id, bias_pull_up))

#define RK3588_PINCTRL_ASSERT_GROUP(node_id)                                                       \
	BUILD_ASSERT(RK3588_PINCTRL_BIAS_COUNT(node_id) <= 1,                                      \
		     "RK3588 pinctrl group must specify at most one bias property");

#define RK3588_PINCTRL_ASSERT_STATE(node_id, unused)                                               \
	DT_FOREACH_CHILD(node_id, RK3588_PINCTRL_ASSERT_GROUP)

#define RK3588_PINCTRL_ASSERT_CONTROLLER(node_id)                                                  \
	DT_FOREACH_CHILD_VARGS(node_id, RK3588_PINCTRL_ASSERT_STATE, 0)

DT_FOREACH_STATUS_OKAY(rockchip_rk3588_pinctrl, RK3588_PINCTRL_ASSERT_CONTROLLER)

struct rk3588_reg_range {
	uint16_t start_pin;
	uint32_t reg;
};

static int rk3588_find_range(const struct rk3588_reg_range *ranges, size_t range_count, uint8_t pin,
			     uint32_t *reg)
{
	int ret = -EINVAL;

	for (size_t i = range_count; (i > 0U) && (ret < 0); i--) {
		if (pin >= ranges[i - 1U].start_pin) {
			*reg = ranges[i - 1U].reg;
			ret = 0;
		}
	}

	return ret;
}

static uint8_t rk3588_drive_width(uint8_t pin)
{
	uint8_t bank = (uint8_t)(pin / 32U);
	uint8_t local = (uint8_t)(pin % 32U);
	uint8_t width = 0U;

	switch (bank) {
	case 0U:
		width = (local < 12U) ? 2U : 3U;
		break;
	case 1U:
		width = ((local >= 16U) && (local < 30U)) ? 2U : 3U;
		break;
	case 2U:
		if ((local < 4U) || (local >= 24U)) {
			width = 3U;
		} else if (local < 6U) {
			width = 0U; /* GPIO2_A4/A5 have no DS fields. */
		} else {
			width = 2U;
		}
		break;
	case 3U:
		width = 3U;
		break;
	case 4U:
		width = ((local >= 18U) && (local < 23U)) ? 2U : 3U;
		break;
	default:
		break;
	}

	return width;
}

static int rk3588_drive_code(uint8_t ohms, uint8_t width)
{
	struct rk3588_drive_map {
		uint8_t ohms;
		uint8_t code;
	};
	static const struct rk3588_drive_map width_2bit[] = {
		{100U, 0U},
		{33U, 1U},
		{50U, 2U},
		{25U, 3U},
	};
	static const struct rk3588_drive_map width_3bit[] = {
		{100U, 0U}, {33U, 1U}, {50U, 2U}, {66U, 4U}, {25U, 5U}, {40U, 6U},
	};
	const struct rk3588_drive_map *mappings = NULL;
	size_t mapping_count = 0U;
	int ret = -EINVAL;

	switch (width) {
	case 2U:
		mappings = width_2bit;
		mapping_count = ARRAY_SIZE(width_2bit);
		ret = -ENOTSUP;
		break;
	case 3U:
		mappings = width_3bit;
		mapping_count = ARRAY_SIZE(width_3bit);
		ret = -ENOTSUP;
		break;
	default:
		break;
	}

	for (size_t i = 0U; (i < mapping_count) && (ret < 0); i++) {
		if (mappings[i].ohms == ohms) {
			ret = (int)mappings[i].code;
		}
	}

	return ret;
}

static int rk3588_validate_mux(uint8_t pin, uint8_t function)
{
	/*
	 * Valid mux masks use one bit per 4-bit hardware mux value. They are
	 * generated from the RK3588 pinctrl descriptions and independently
	 * checked against the TRM pin function tables.
	 */
	/* clang-format off */
	static const uint32_t rk3588_mux_masks[RK3588_PIN_COUNT] = {
		/* GPIO0 A */ 0x0003U, 0x0007U, 0x0007U, 0x0003U,
			       0x0003U, 0x000fU, 0x0007U, 0x0003U,
		/* GPIO0 B */ 0x0017U, 0x001fU, 0x0007U, 0x0007U,
			       0x0000U, 0x1607U, 0x1607U, 0x1b0bU,
		/* GPIO0 C */ 0x1b0dU, 0x0003U, 0x0003U, 0x0003U,
			       0x161dU, 0x1e17U, 0x3917U, 0x1f07U,
		/* GPIO0 D */ 0x1f07U, 0x3d1fU, 0x3d0bU, 0x0103U,
			       0x3f0fU, 0x2f07U, 0x0007U, 0x0000U,

		/* GPIO1 A */ 0x07b1U, 0x0771U, 0x0f03U, 0x0f21U,
			       0x0121U, 0x0121U, 0x0121U, 0x0995U,
		/* GPIO1 B */ 0x0195U, 0x0195U, 0x0595U, 0x05d5U,
			       0x0595U, 0x0511U, 0x063dU, 0x0e7dU,
		/* GPIO1 C */ 0x0701U, 0x0701U, 0x0f03U, 0x0f03U,
			       0x0b19U, 0x0603U, 0x0a09U, 0x0603U,
		/* GPIO1 D */ 0x0703U, 0x071fU, 0x0f0fU, 0x0f0dU,
			       0x0005U, 0x0109U, 0x0e25U, 0x0e35U,

		/* GPIO2 A */ 0x0007U, 0x0003U, 0x0603U, 0x0603U,
			       0x0000U, 0x0000U, 0x040fU, 0x040fU,
		/* GPIO2 B */ 0x060fU, 0x060fU, 0x0207U, 0x020fU,
			       0x061bU, 0x061bU, 0x0607U, 0x0607U,
		/* GPIO2 C */ 0x0707U, 0x0703U, 0x0703U, 0x0307U,
			       0x051bU, 0x0003U, 0x0000U, 0x0000U,
		/* GPIO2 D */ 0x0007U, 0x0007U, 0x0007U, 0x0007U,
			       0x0603U, 0x0603U, 0x0007U, 0x0007U,

		/* GPIO3 A */ 0x0b2fU, 0x0b3fU, 0x053fU, 0x053fU,
			       0x051fU, 0x0637U, 0x0213U, 0x0813U,
		/* GPIO3 B */ 0x0813U, 0x0c13U, 0x0c0bU, 0x040bU,
			       0x040bU, 0x0e0bU, 0x0e0bU, 0x0323U,
		/* GPIO3 C */ 0x0703U, 0x0513U, 0x0f07U, 0x0f07U,
			       0x0737U, 0x0737U, 0x0133U, 0x0333U,
		/* GPIO3 D */ 0x0f33U, 0x0d33U, 0x0733U, 0x0f33U,
			       0x0579U, 0x0d71U, 0x0000U, 0x0000U,

		/* GPIO4 A */ 0x059fU, 0x059fU, 0x019fU, 0x0497U,
			       0x0797U, 0x079fU, 0x079fU, 0x039fU,
		/* GPIO4 B */ 0x079fU, 0x07dfU, 0x1f9fU, 0x1e9fU,
			       0x0cffU, 0x0df5U, 0x0bf5U, 0x03b5U,
		/* GPIO4 C */ 0x0335U, 0x0b3dU, 0x0d03U, 0x0b07U,
			       0x0f03U, 0x0f03U, 0x0f03U, 0x0000U,
		/* GPIO4 D */ 0x0e27U, 0x0e27U, 0x0627U, 0x0e27U,
			       0x0e27U, 0x0637U, 0x0000U, 0x0000U,
	};
	static const uint32_t function_masks[] = {
		0x0001U, 0x0002U, 0x0004U, 0x0008U, 0x0010U, 0x0020U, 0x0040U, 0x0080U,
		0x0100U, 0x0200U, 0x0400U, 0x0800U, 0x1000U, 0x2000U, 0x4000U, 0x8000U,
	};
	/* clang-format on */
	int ret = 0;

	if (rk3588_mux_masks[pin] == 0U) {
		ret = -ENOTSUP;
	}
	if ((ret == 0) && (function > 15U)) {
		ret = -EINVAL;
	}
	if ((ret == 0) && (function <= 15U) &&
	    ((rk3588_mux_masks[pin] & function_masks[function]) == 0U)) {
		ret = -EINVAL;
	}

	return ret;
}

static int rk3588_validate_pin(const pinctrl_soc_pin_t *pin)
{
	int ret = 0;

	if (pin->pin >= RK3588_PIN_COUNT) {
		ret = -EINVAL;
	} else {
		ret = rk3588_validate_mux(pin->pin, pin->function);
		if (ret == 0) {
			if ((pin->bias < RK3588_PINCTRL_BIAS_NONE) ||
			    (pin->bias > RK3588_PINCTRL_BIAS_PULL_UP)) {
				ret = -EINVAL;
			}
		}

		if ((ret == 0) && (pin->drive_strength_ohms != 0U)) {
			uint8_t width = rk3588_drive_width(pin->pin);
			int drive_code = rk3588_drive_code(pin->drive_strength_ohms, width);

			if (drive_code < 0) {
				ret = drive_code;
			}
		}

		if ((ret == 0) && ((pin->flags & (RK3588_PINCTRL_INPUT_ENABLE |
						  RK3588_PINCTRL_INPUT_DISABLE)) != 0U)) {
			if (((pin->flags & RK3588_PINCTRL_INPUT_ENABLE) != 0U) &&
			    ((pin->flags & RK3588_PINCTRL_INPUT_DISABLE) != 0U)) {
				ret = -EINVAL;
			}
		}

		if ((ret == 0) && ((pin->flags & (RK3588_PINCTRL_SCHMITT_ENABLE |
						  RK3588_PINCTRL_SCHMITT_DISABLE)) != 0U)) {
			if (((pin->flags & RK3588_PINCTRL_SCHMITT_ENABLE) != 0U) &&
			    ((pin->flags & RK3588_PINCTRL_SCHMITT_DISABLE) != 0U)) {
				ret = -EINVAL;
			}
		}
	}

	return ret;
}

static void rk3588_write_field(uintptr_t base, uint32_t reg, uint32_t bit, uint32_t width,
			       uint32_t value)
{
	uint32_t mask = (UINT32_C(1) << width) - 1U;
	uint32_t data;

	mask <<= bit;
	data = (value << bit) & mask;
	sys_write32((mask << 16U) | data, base + (uintptr_t)reg);
}

static int rk3588_set_pull(uint8_t pin, rk3588_pinctrl_bias_t bias, uintptr_t base)
{
	static const struct rk3588_reg_range rk3588_pull_ranges[] = {
		{0U, RK3588_PMU1_IOC + 0x0020U},       {8U, RK3588_PMU1_IOC + 0x0024U},
		{13U, RK3588_PMU2_IOC + 0x0028U},      {16U, RK3588_PMU2_IOC + 0x002cU},
		{24U, RK3588_PMU2_IOC + 0x0030U},      {32U, RK3588_VCCIO1_4_IOC + 0x0110U},
		{40U, RK3588_VCCIO1_4_IOC + 0x0114U},  {48U, RK3588_VCCIO1_4_IOC + 0x0118U},
		{56U, RK3588_VCCIO1_4_IOC + 0x011cU},  {64U, RK3588_EMMC_IOC + 0x0120U},
		{70U, RK3588_VCCIO3_5_IOC + 0x0120U},  {72U, RK3588_VCCIO3_5_IOC + 0x0124U},
		{80U, RK3588_VCCIO3_5_IOC + 0x0128U},  {88U, RK3588_EMMC_IOC + 0x012cU},
		{96U, RK3588_VCCIO3_5_IOC + 0x0130U},  {104U, RK3588_VCCIO3_5_IOC + 0x0134U},
		{112U, RK3588_VCCIO3_5_IOC + 0x0138U}, {120U, RK3588_VCCIO3_5_IOC + 0x013cU},
		{128U, RK3588_VCCIO6_IOC + 0x0140U},   {136U, RK3588_VCCIO6_IOC + 0x0144U},
		{144U, RK3588_VCCIO6_IOC + 0x0148U},   {146U, RK3588_VCCIO3_5_IOC + 0x0148U},
		{152U, RK3588_VCCIO2_IOC + 0x014cU},
	};
	uint32_t reg = 0U;
	int ret = 0;

	if (bias != RK3588_PINCTRL_BIAS_NONE) {
		ret = rk3588_find_range(rk3588_pull_ranges, ARRAY_SIZE(rk3588_pull_ranges), pin,
					&reg);
		if (ret == 0) {
			uint32_t value = 0U;

			switch (bias) {
			case RK3588_PINCTRL_BIAS_DISABLE:
				value = 0U;
				break;
			case RK3588_PINCTRL_BIAS_PULL_DOWN:
				value = 1U;
				break;
			case RK3588_PINCTRL_BIAS_PULL_UP:
				value = 3U;
				break;
			default:
				ret = -EINVAL;
				break;
			}

			if (ret == 0) {
				static const uint32_t bit_mod2[] = {0U, 2U,  4U,  6U,
								    8U, 10U, 12U, 14U};
				uint32_t bit = bit_mod2[pin % 8U];

				rk3588_write_field(base, reg, bit, 2U, value);
			}
		}
	}

	return ret;
}

static int rk3588_set_drive(uint8_t pin, uint8_t ohms, uintptr_t base)
{
	static const struct rk3588_reg_range rk3588_drive_ranges[] = {
		{0U, RK3588_PMU1_IOC + 0x0010U},       {4U, RK3588_PMU1_IOC + 0x0014U},
		{8U, RK3588_PMU1_IOC + 0x0018U},       {12U, RK3588_PMU2_IOC + 0x0014U},
		{16U, RK3588_PMU2_IOC + 0x0018U},      {20U, RK3588_PMU2_IOC + 0x001cU},
		{24U, RK3588_PMU2_IOC + 0x0020U},      {28U, RK3588_PMU2_IOC + 0x0024U},
		{32U, RK3588_VCCIO1_4_IOC + 0x0020U},  {36U, RK3588_VCCIO1_4_IOC + 0x0024U},
		{40U, RK3588_VCCIO1_4_IOC + 0x0028U},  {44U, RK3588_VCCIO1_4_IOC + 0x002cU},
		{48U, RK3588_VCCIO1_4_IOC + 0x0030U},  {52U, RK3588_VCCIO1_4_IOC + 0x0034U},
		{56U, RK3588_VCCIO1_4_IOC + 0x0038U},  {60U, RK3588_VCCIO1_4_IOC + 0x003cU},
		{64U, RK3588_EMMC_IOC + 0x0040U},      {68U, RK3588_VCCIO3_5_IOC + 0x0044U},
		{72U, RK3588_VCCIO3_5_IOC + 0x0048U},  {76U, RK3588_VCCIO3_5_IOC + 0x004cU},
		{80U, RK3588_VCCIO3_5_IOC + 0x0050U},  {84U, RK3588_VCCIO3_5_IOC + 0x0054U},
		{88U, RK3588_EMMC_IOC + 0x0058U},      {92U, RK3588_EMMC_IOC + 0x005cU},
		{96U, RK3588_VCCIO3_5_IOC + 0x0060U},  {100U, RK3588_VCCIO3_5_IOC + 0x0064U},
		{104U, RK3588_VCCIO3_5_IOC + 0x0068U}, {108U, RK3588_VCCIO3_5_IOC + 0x006cU},
		{112U, RK3588_VCCIO3_5_IOC + 0x0070U}, {116U, RK3588_VCCIO3_5_IOC + 0x0074U},
		{120U, RK3588_VCCIO3_5_IOC + 0x0078U}, {124U, RK3588_VCCIO3_5_IOC + 0x007cU},
		{128U, RK3588_VCCIO6_IOC + 0x0080U},   {132U, RK3588_VCCIO6_IOC + 0x0084U},
		{136U, RK3588_VCCIO6_IOC + 0x0088U},   {140U, RK3588_VCCIO6_IOC + 0x008cU},
		{144U, RK3588_VCCIO6_IOC + 0x0090U},   {146U, RK3588_VCCIO3_5_IOC + 0x0090U},
		{148U, RK3588_VCCIO3_5_IOC + 0x0094U}, {152U, RK3588_VCCIO2_IOC + 0x0098U},
		{156U, RK3588_VCCIO2_IOC + 0x009cU},
	};
	uint32_t reg = 0U;
	int ret = 0;

	if (ohms != 0U) {
		ret = rk3588_find_range(rk3588_drive_ranges, ARRAY_SIZE(rk3588_drive_ranges), pin,
					&reg);
		if (ret == 0) {
			uint8_t width = rk3588_drive_width(pin);
			static const uint32_t drive_bits[] = {0U, 4U, 8U, 12U};
			int drive_code = rk3588_drive_code(ohms, width);

			if (drive_code >= 0) {
				uint32_t bit = drive_bits[pin % 4U];

				rk3588_write_field(base, reg, bit, width, (uint32_t)drive_code);
			} else {
				ret = drive_code;
			}
		}
	}

	return ret;
}

static int rk3588_set_input_enable(uint8_t pin, uint8_t flags, uintptr_t base)
{
	static const struct rk3588_reg_range rk3588_ie_ranges[] = {
		{0U, RK3588_PMU1_IOC + 0x0028U},       {8U, RK3588_PMU1_IOC + 0x002cU},
		{13U, RK3588_PMU2_IOC + 0x0034U},      {16U, RK3588_PMU2_IOC + 0x0038U},
		{24U, RK3588_PMU2_IOC + 0x003cU},      {32U, RK3588_VCCIO1_4_IOC + 0x0180U},
		{40U, RK3588_VCCIO1_4_IOC + 0x0184U},  {48U, RK3588_VCCIO1_4_IOC + 0x0188U},
		{56U, RK3588_VCCIO1_4_IOC + 0x018cU},  {64U, RK3588_EMMC_IOC + 0x0190U},
		{70U, RK3588_VCCIO3_5_IOC + 0x0190U},  {72U, RK3588_VCCIO3_5_IOC + 0x0194U},
		{80U, RK3588_VCCIO3_5_IOC + 0x0198U},  {88U, RK3588_EMMC_IOC + 0x019cU},
		{96U, RK3588_VCCIO3_5_IOC + 0x01a0U},  {104U, RK3588_VCCIO3_5_IOC + 0x01a4U},
		{112U, RK3588_VCCIO3_5_IOC + 0x01a8U}, {120U, RK3588_VCCIO3_5_IOC + 0x01acU},
		{128U, RK3588_VCCIO6_IOC + 0x01b0U},   {136U, RK3588_VCCIO6_IOC + 0x01b4U},
		{144U, RK3588_VCCIO6_IOC + 0x01b8U},   {146U, RK3588_VCCIO3_5_IOC + 0x01b8U},
		{152U, RK3588_VCCIO2_IOC + 0x01bcU},
	};
	uint32_t reg = 0U;
	int ret = 0;

	if ((flags & (RK3588_PINCTRL_INPUT_ENABLE | RK3588_PINCTRL_INPUT_DISABLE)) != 0U) {
		ret = rk3588_find_range(rk3588_ie_ranges, ARRAY_SIZE(rk3588_ie_ranges), pin, &reg);
		if (ret == 0) {
			uint32_t value = (((flags & RK3588_PINCTRL_INPUT_ENABLE) != 0U) ? 1U : 0U);

			rk3588_write_field(base, reg, pin % 8U, 1U, value);
		}
	}

	return ret;
}

static int rk3588_set_schmitt(uint8_t pin, uint8_t flags, uintptr_t base)
{
	static const struct rk3588_reg_range rk3588_smt_ranges[] = {
		{0U, RK3588_PMU1_IOC + 0x0030U},       {8U, RK3588_PMU1_IOC + 0x0034U},
		{13U, RK3588_PMU2_IOC + 0x0040U},      {16U, RK3588_PMU2_IOC + 0x0044U},
		{24U, RK3588_PMU2_IOC + 0x0048U},      {32U, RK3588_VCCIO1_4_IOC + 0x0210U},
		{40U, RK3588_VCCIO1_4_IOC + 0x0214U},  {48U, RK3588_VCCIO1_4_IOC + 0x0218U},
		{56U, RK3588_VCCIO1_4_IOC + 0x021cU},  {64U, RK3588_EMMC_IOC + 0x0220U},
		{70U, RK3588_VCCIO3_5_IOC + 0x0220U},  {72U, RK3588_VCCIO3_5_IOC + 0x0224U},
		{80U, RK3588_VCCIO3_5_IOC + 0x0228U},  {88U, RK3588_EMMC_IOC + 0x022cU},
		{96U, RK3588_VCCIO3_5_IOC + 0x0230U},  {104U, RK3588_VCCIO3_5_IOC + 0x0234U},
		{112U, RK3588_VCCIO3_5_IOC + 0x0238U}, {120U, RK3588_VCCIO3_5_IOC + 0x023cU},
		{128U, RK3588_VCCIO6_IOC + 0x0240U},   {136U, RK3588_VCCIO6_IOC + 0x0244U},
		{144U, RK3588_VCCIO6_IOC + 0x0248U},   {146U, RK3588_VCCIO3_5_IOC + 0x0248U},
		{152U, RK3588_VCCIO2_IOC + 0x024cU},
	};
	uint32_t reg = 0U;
	int ret = 0;

	if ((flags & (RK3588_PINCTRL_SCHMITT_ENABLE | RK3588_PINCTRL_SCHMITT_DISABLE)) != 0U) {
		ret = rk3588_find_range(rk3588_smt_ranges, ARRAY_SIZE(rk3588_smt_ranges), pin,
					&reg);
		if (ret == 0) {
			uint32_t value =
				(((flags & RK3588_PINCTRL_SCHMITT_ENABLE) != 0U) ? 1U : 0U);

			rk3588_write_field(base, reg, pin % 8U, 1U, value);
		}
	}

	return ret;
}

static int rk3588_set_mux(uint8_t pin, uint8_t function, uintptr_t base)
{
	static const uint32_t mux_bits[] = {0U, 4U, 8U, 12U};
	static const uint32_t group_offsets[] = {0U, 8U, 16U, 24U};
	static const uint32_t half_offsets[] = {0U, 4U};
	static const uint32_t bank_offsets[] = {0U, RK3588_BUS_IOC + 0x20U, RK3588_BUS_IOC + 0x40U,
						RK3588_BUS_IOC + 0x60U, RK3588_BUS_IOC + 0x80U};
	static const uint32_t route_offsets[] = {
		0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,  0U,  0U,  0U,  0U,  0U,  0U,  0U,
		4U, 4U, 4U, 4U, 8U, 8U, 8U, 8U, 12U, 12U, 12U, 12U, 16U, 16U, 16U, 16U,
	};
	uint8_t bank = (uint8_t)(pin / 32U);
	uint8_t local = (uint8_t)(pin % 32U);
	uint32_t bit = mux_bits[local % 4U];
	uint32_t mux_reg;

	if (bank == 0U) {
		if (local < 12U) {
			mux_reg = group_offsets[local / 8U] + half_offsets[(local % 8U) / 4U];
			rk3588_write_field(base, mux_reg, bit, 4U, function);
		} else {
			uint32_t route_reg = route_offsets[local];

			mux_reg = RK3588_BUS_IOC + 0x0cU + route_reg;
			if (function < 8U) {
				rk3588_write_field(base, RK3588_PMU2_IOC + route_reg, bit, 4U,
						   function);
			} else {
				rk3588_write_field(base, RK3588_PMU2_IOC + route_reg, bit, 4U, 8U);
				rk3588_write_field(base, mux_reg, bit, 4U, function);
			}
		}
	} else {
		mux_reg = bank_offsets[bank];
		mux_reg += group_offsets[local / 8U];
		mux_reg += half_offsets[(local % 8U) / 4U];
		rk3588_write_field(base, mux_reg, bit, 4U, function);
	}

	return 0;
}

ZTESTABLE_STATIC int rk3588_pinctrl_configure_pin(const pinctrl_soc_pin_t *pin, uintptr_t base)
{
	static struct k_spinlock rk3588_ioc_lock;
	int ret = rk3588_validate_pin(pin);

	if (ret == 0) {
		k_spinlock_key_t key = k_spin_lock(&rk3588_ioc_lock);

		ret = rk3588_set_pull(pin->pin, pin->bias, base);
		if (ret == 0) {
			ret = rk3588_set_drive(pin->pin, pin->drive_strength_ohms, base);
		}
		if (ret == 0) {
			ret = rk3588_set_input_enable(pin->pin, pin->flags, base);
		}
		if (ret == 0) {
			ret = rk3588_set_schmitt(pin->pin, pin->flags, base);
		}
		if (ret == 0) {
			ret = rk3588_set_mux(pin->pin, pin->function, base);
		}

		k_spin_unlock(&rk3588_ioc_lock, key);
	}

	return ret;
}

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "RK3588 pinctrl requires exactly one enabled instance");
BUILD_ASSERT(DT_INST_REG_SIZE(0) == 0x10000U,
	     "RK3588 pinctrl requires the complete 64 KiB IOC window");

static mm_reg_t rk3588_ioc_base;

static int rk3588_pinctrl_init(void)
{
	device_map(&rk3588_ioc_base, DT_INST_REG_ADDR(0), DT_INST_REG_SIZE(0), K_MEM_CACHE_NONE);

	return 0;
}

SYS_INIT(rk3588_pinctrl_init, PRE_KERNEL_1, 0);

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int ret = 0;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; (i < pin_cnt) && (ret == 0); i++) {
		ret = rk3588_pinctrl_configure_pin(&pins[i], (uintptr_t)rk3588_ioc_base);
	}

	return ret;
}
