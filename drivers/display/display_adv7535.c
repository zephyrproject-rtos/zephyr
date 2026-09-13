/*
 * Copyright (C) 2026, Savoir-faire Linux, Inc.
 * Author: Paolo Wattebled <paolo.wattebled@savoirfairelinux.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adv7535

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adv7535, CONFIG_DISPLAY_LOG_LEVEL);

BUILD_ASSERT(CONFIG_ADV7535_INIT_PRIORITY > CONFIG_MIPI_DSI_INIT_PRIORITY,
	     "ADV7535 must initialize after the MIPI DSI host");

#define ADV7535_MAIN_POWER            0x41
#define ADV7535_MAIN_POWER_DOWN       BIT(6)
#define ADV7535_MAIN_CEC_ADDR         0xe1
#define ADV7535_MAIN_POWER2           0xd6
#define ADV7535_MAIN_HPD_OVERRIDE     BIT(6)
#define ADV7535_CEC_OUTPUT            0x03
#define ADV7535_CEC_OUTPUT_DISABLE    0x0b
#define ADV7535_CEC_OUTPUT_ENABLE     0x89
#define ADV7535_CEC_TIMING_GEN        0x27
#define ADV7535_CEC_TIMING_GEN_RESET  0xcb
#define ADV7535_CEC_TIMING_GEN_RUN    0x8b
#define ADV7535_CEC_TEST_MODE         0x55
#define ADV7535_CEC_DSI_RESET         0x26
#define ADV7535_CEC_DSI_RESET_ASSERT  0x18
#define ADV7535_CEC_DSI_RESET_RELEASE 0x38
#define ADV7535_TIMING_MAX            0xfff

struct adv7535_config {
	struct i2c_dt_spec main;
	struct i2c_dt_spec cec;
	const struct device *mipi_dsi;
	uint8_t data_lanes;
	uint8_t pixel_format;
	struct mipi_dsi_timings timings;
};

struct adv7535_reg {
	bool cec;
	uint8_t reg;
	uint8_t value;
};

static const struct adv7535_reg adv7535_fixed_init[] = {
	{false, 0x16, 0x20}, {false, 0x9a, 0xe0}, {false, 0xba, 0x70}, {false, 0xde, 0x82},
	{false, 0xe4, 0x40}, {false, 0xe5, 0x80}, {true, 0x15, 0xd0},  {true, 0x17, 0xd0},
	{true, 0x24, 0x20},  {true, 0x57, 0x11},  {true, 0x05, 0xc8},
};

static const struct adv7535_reg adv7535_video_init[] = {
	{false, 0xaf, 0x16}, {false, 0x55, 0x10}, {false, 0x56, 0x28}, {false, 0x40, 0x80},
	{false, 0x4c, 0x04}, {false, 0x49, 0x00}, {false, 0x4a, 0x80}, {true, 0xbe, 0x3d},
};

static int adv7535_write(const struct i2c_dt_spec *map, uint8_t reg, uint8_t value)
{
	int ret = i2c_reg_write_byte_dt(map, reg, value);

	if (ret < 0) {
		LOG_ERR("I2C 0x%02x reg 0x%02x write failed: %d", map->addr, reg, ret);
	}

	return ret;
}

static int adv7535_update(const struct i2c_dt_spec *map, uint8_t reg, uint8_t mask, uint8_t value)
{
	int ret = i2c_reg_update_byte_dt(map, reg, mask, value);

	if (ret < 0) {
		LOG_ERR("I2C 0x%02x reg 0x%02x update failed: %d", map->addr, reg, ret);
	}

	return ret;
}

static int adv7535_write_regs(const struct adv7535_config *cfg, const struct adv7535_reg *regs,
			      size_t count)
{
	for (size_t i = 0; i < count; i++) {
		const struct i2c_dt_spec *map = regs[i].cec ? &cfg->cec : &cfg->main;
		int ret = adv7535_write(map, regs[i].reg, regs[i].value);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int adv7535_write_timing(const struct i2c_dt_spec *cec, uint8_t reg, uint16_t value)
{
	uint16_t encoded = value << 4;
	int ret;

	ret = adv7535_write(cec, reg, encoded >> 8);
	if (ret == 0) {
		ret = adv7535_write(cec, reg + 1, encoded & 0xff);
	}

	return ret;
}

static void adv7535_disable(const struct adv7535_config *cfg)
{
	(void)adv7535_write(&cfg->cec, ADV7535_CEC_OUTPUT, ADV7535_CEC_OUTPUT_DISABLE);
	(void)adv7535_update(&cfg->main, ADV7535_MAIN_POWER, ADV7535_MAIN_POWER_DOWN,
			     ADV7535_MAIN_POWER_DOWN);
}

static int adv7535_program(const struct adv7535_config *cfg)
{
	const struct mipi_dsi_timings *t = &cfg->timings;
	const struct {
		uint8_t reg;
		uint16_t value;
	} timings[] = {
		{0x28, t->hactive + t->hfp + t->hsync + t->hbp},
		{0x2a, t->hsync},
		{0x2c, t->hfp},
		{0x2e, t->hbp},
		{0x30, t->vactive + t->vfp + t->vsync + t->vbp},
		{0x32, t->vsync},
		{0x34, t->vfp},
		{0x36, t->vbp},
	};
	int ret;

	ret = adv7535_write(&cfg->main, ADV7535_MAIN_CEC_ADDR, cfg->cec.addr << 1);
	if (ret < 0) {
		return ret;
	}
	ret = adv7535_write(&cfg->cec, ADV7535_CEC_OUTPUT, ADV7535_CEC_OUTPUT_DISABLE);
	if (ret < 0) {
		return ret;
	}
	ret = adv7535_update(&cfg->main, ADV7535_MAIN_POWER, ADV7535_MAIN_POWER_DOWN, 0);
	if (ret < 0) {
		return ret;
	}
	ret = adv7535_update(&cfg->main, ADV7535_MAIN_POWER2, ADV7535_MAIN_HPD_OVERRIDE,
			     ADV7535_MAIN_HPD_OVERRIDE);
	if (ret < 0) {
		return ret;
	}
	ret = adv7535_write_regs(cfg, adv7535_fixed_init, ARRAY_SIZE(adv7535_fixed_init));
	if (ret < 0) {
		return ret;
	}
	ret = adv7535_write(&cfg->cec, 0x1c, cfg->data_lanes << 4);
	if (ret < 0) {
		return ret;
	}
	ret = adv7535_write_regs(cfg, adv7535_video_init, ARRAY_SIZE(adv7535_video_init));
	if (ret < 0) {
		return ret;
	}
	ret = adv7535_write(&cfg->cec, ADV7535_CEC_DSI_RESET, ADV7535_CEC_DSI_RESET_ASSERT);
	if (ret < 0) {
		return ret;
	}
	k_msleep(200);
	ret = adv7535_write(&cfg->cec, ADV7535_CEC_DSI_RESET, ADV7535_CEC_DSI_RESET_RELEASE);

	if (ret < 0) {
		return ret;
	}
	ret = adv7535_write(&cfg->cec, 0x16, 0x18);
	if (ret < 0) {
		return ret;
	}
	for (size_t i = 0; i < ARRAY_SIZE(timings); i++) {
		ret = adv7535_write_timing(&cfg->cec, timings[i].reg, timings[i].value);
		if (ret < 0) {
			return ret;
		}
	}
	ret = adv7535_write(&cfg->cec, ADV7535_CEC_TIMING_GEN, ADV7535_CEC_TIMING_GEN_RESET);
	ret = ret ?: adv7535_write(&cfg->cec, ADV7535_CEC_TIMING_GEN, ADV7535_CEC_TIMING_GEN_RUN);
	return ret
		       ?: adv7535_write(&cfg->cec, ADV7535_CEC_TIMING_GEN,
					ADV7535_CEC_TIMING_GEN_RESET);
}

static int adv7535_init(const struct device *dev)
{
	const struct adv7535_config *cfg = dev->config;
	struct mipi_dsi_device mdev = {
		.data_lanes = cfg->data_lanes,
		.timings = cfg->timings,
		.pixfmt = cfg->pixel_format,
		.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
			      MIPI_DSI_MODE_VIDEO_HSE,
	};
	uint8_t id_msb;
	uint8_t id_lsb;
	uint16_t id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->main) || !device_is_ready(cfg->mipi_dsi)) {
		LOG_ERR("I2C bus or MIPI DSI host not ready");
		return -ENODEV;
	}

	ret = adv7535_write(&cfg->main, ADV7535_MAIN_CEC_ADDR, cfg->cec.addr << 1);
	ret = ret ?: i2c_reg_read_byte_dt(&cfg->cec, 0x00, &id_msb);
	ret = ret ?: i2c_reg_read_byte_dt(&cfg->cec, 0x01, &id_lsb);
	if (ret < 0) {
		LOG_ERR("chip identification failed: %d", ret);
		return ret;
	}

	id = ((uint16_t)id_msb << 8) | id_lsb;
	if (id != 0x7533 && id != 0x7535) {
		LOG_ERR("unexpected chip ID 0x%04x", id);
		return -ENODEV;
	}

	ret = adv7535_program(cfg);
	if (ret < 0) {
		goto fail;
	}

	ret = adv7535_write(&cfg->cec, ADV7535_CEC_OUTPUT, ADV7535_CEC_OUTPUT_ENABLE);
	ret = ret ?: adv7535_write(&cfg->cec, ADV7535_CEC_TEST_MODE, 0x00);
	if (ret < 0) {
		goto fail;
	}

	ret = mipi_dsi_attach(cfg->mipi_dsi, 0U, &mdev);
	if (ret < 0) {
		LOG_ERR("MIPI DSI attach failed: %d", ret);
		goto fail;
	}
	LOG_INF("ADV753x ID 0x%04x, %ux%u RGB888, %u lanes", id, cfg->timings.hactive,
		cfg->timings.vactive, cfg->data_lanes);
	return 0;

fail:
	adv7535_disable(cfg);
	return ret;
}

#define ADV7535_DEFINE(inst)                                                                       \
	BUILD_ASSERT(DT_INST_PROP(inst, data_lanes) == 4, "ADV7535 requires 4 lanes");             \
	BUILD_ASSERT(DT_INST_PROP(inst, pixel_format) == MIPI_DSI_PIXFMT_RGB888,                   \
		     "ADV7535 requires RGB888");                                                   \
	BUILD_ASSERT(DT_PROP(DT_PARENT(DT_INST_PHANDLE(inst, display_timing)), width) * 9U ==      \
			     DT_PROP(DT_PARENT(DT_INST_PHANDLE(inst, display_timing)), height) *   \
				     16U,                                                          \
		     "ADV7535 requires a 16:9 active area");                                       \
	BUILD_ASSERT(DT_INST_PROP(inst, adi_addr_cec) <= 0x7f,                                     \
		     "ADV7535 CEC address must be seven-bit");                                     \
	BUILD_ASSERT(DT_INST_PROP(inst, adi_addr_cec) != DT_INST_REG_ADDR(inst),                   \
		     "ADV7535 main and CEC addresses must differ");                                \
	BUILD_ASSERT(                                                                              \
		DT_PROP(DT_PARENT(DT_INST_PHANDLE(inst, display_timing)), width) +                 \
				DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing, hsync_len) + \
				DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,              \
						   hfront_porch) +                                 \
				DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,              \
						   hback_porch) <=                                 \
			ADV7535_TIMING_MAX,                                                        \
		"ADV7535 horizontal total exceeds 12 bits");                                       \
	BUILD_ASSERT(                                                                              \
		DT_PROP(DT_PARENT(DT_INST_PHANDLE(inst, display_timing)), height) +                \
				DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing, vsync_len) + \
				DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,              \
						   vfront_porch) +                                 \
				DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,              \
						   vback_porch) <=                                 \
			ADV7535_TIMING_MAX,                                                        \
		"ADV7535 vertical total exceeds 12 bits");                                         \
	static const struct adv7535_config adv7535_config_##inst = {                               \
		.main = I2C_DT_SPEC_INST_GET(inst),                                                \
		.cec = {.bus = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(inst))),                           \
			.addr = DT_INST_PROP(inst, adi_addr_cec)},                                 \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_PHANDLE(inst, mipi_dsi)),                        \
		.data_lanes = DT_INST_PROP(inst, data_lanes),                                      \
		.pixel_format = DT_INST_PROP(inst, pixel_format),                                  \
		.timings =                                                                         \
			{                                                                          \
				.hactive = DT_PROP(                                                \
					DT_PARENT(DT_INST_PHANDLE(inst, display_timing)), width),  \
				.hfp = DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,       \
							  hfront_porch),                           \
				.hbp = DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,       \
							  hback_porch),                            \
				.hsync = DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,     \
							    hsync_len),                            \
				.vactive = DT_PROP(                                                \
					DT_PARENT(DT_INST_PHANDLE(inst, display_timing)), height), \
				.vfp = DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,       \
							  vfront_porch),                           \
				.vbp = DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,       \
							  vback_porch),                            \
				.vsync = DT_PROP_BY_PHANDLE(DT_DRV_INST(inst), display_timing,     \
							    vsync_len),                            \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, adv7535_init, NULL, NULL, &adv7535_config_##inst, POST_KERNEL, \
			      CONFIG_ADV7535_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(ADV7535_DEFINE)
