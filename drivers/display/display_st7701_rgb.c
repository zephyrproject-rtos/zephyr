/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7701_rgb

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/display/mipi_display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(st7701_rgb, CONFIG_DISPLAY_LOG_LEVEL);

#define ST7701_SLEEP_OUT_DELAY_MS    120U
#define ST7701_BK1_SPD2_DELAY_MS     20U
#define ST7701_INVERSION_DELAY_MS    120U
#define ST7701_RESET_SETTLE_DELAY_MS 120U
#define ST7701_MADCTL_DEFAULT        0x00U
#define ST7701_COLMOD_DEFAULT        0x66U

static const uint8_t st7701_cmd2_bk0_sel[] = {0xFF, 0x77, 0x01, 0x00, 0x00, 0x10};
static const uint8_t st7701_cmd2_bk0_lneset[] = {0xC0, 0x3B, 0x00};
static const uint8_t st7701_cmd2_bk0_porctrl[] = {0xC1, 0x0D, 0x02};
static const uint8_t st7701_cmd2_bk0_invsel[] = {0xC2, 0x21, 0x08};
static const uint8_t st7701_cmd2_bk0_cd[] = {0xCD, 0x08};
static const uint8_t st7701_cmd2_bk1_sel[] = {0xFF, 0x77, 0x01, 0x00, 0x00, 0x11};
static const uint8_t st7701_cmd2_bkx_sel_none[] = {0xFF, 0x77, 0x01, 0x00, 0x00, 0x00};

/* Command2, BK1 commands */
#define ST7701_CMD2_BK1_VRHS    0xB0
#define ST7701_CMD2_BK1_VCOM    0xB1
#define ST7701_CMD2_BK1_VGHSS   0xB2
#define ST7701_CMD2_BK1_TESTCMD 0xB3
#define ST7701_CMD2_BK1_VGLS    0xB5
#define ST7701_CMD2_BK1_PWCTRL1 0xB7
#define ST7701_CMD2_BK1_PWCTRL2 0xB8
#define ST7701_CMD2_BK1_SPD1    0xC1
#define ST7701_CMD2_BK1_SPD2    0xC2

#define ST7701_VAL_BK1_VRHS    0x60
#define ST7701_VAL_BK1_VCOM    0x30
#define ST7701_VAL_BK1_VGHSS   0x87
#define ST7701_VAL_BK1_TESTCMD 0x80
#define ST7701_VAL_BK1_VGLS    0x49
#define ST7701_VAL_BK1_PWCTRL1 0x85
#define ST7701_VAL_BK1_PWCTRL2 0x21
#define ST7701_VAL_BK1_SPD1    0x78
#define ST7701_VAL_BK1_SPD2    0x78

struct st7701_rgb_config {
	const struct device *mipi_dbi;
	struct mipi_dbi_config dbi_config;
	struct gpio_dt_spec reset_gpio;
	uint16_t width;
	uint16_t height;
	uint16_t reset_delay_ms;
	bool inversion_on;
	uint8_t pvgamctrl[17];
	uint8_t nvgamctrl[17];
	uint8_t gip_e0[4];
	uint8_t gip_e1[12];
	uint8_t gip_e2[13];
	uint8_t gip_e3[5];
	uint8_t gip_e4[3];
	uint8_t gip_e5[17];
	uint8_t gip_e6[5];
	uint8_t gip_e7[3];
	uint8_t gip_e8[17];
	uint8_t gip_eb[8];
	uint8_t gip_ec[3];
	uint8_t gip_ed[17];
};

static int st7701_rgb_command(const struct device *dev, uint8_t cmd, const uint8_t *data,
			      size_t len)
{
	const struct st7701_rgb_config *cfg = dev->config;

	int ret = mipi_dbi_command_write(cfg->mipi_dbi, &cfg->dbi_config, cmd, data, len);

	if (ret != 0) {
		printk("Failed to send command 0x%02X (error %d)\n", cmd, ret);
	}
	return ret;
}

static int st7701_rgb_command_u8(const struct device *dev, uint8_t cmd, uint8_t value)
{
	return st7701_rgb_command(dev, cmd, &value, 1);
}

static int st7701_rgb_write_cmd_array(const struct device *dev, const uint8_t *cmd_and_data,
				      size_t len)
{
	if ((cmd_and_data == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	return st7701_rgb_command(dev, cmd_and_data[0], (len > 1U) ? &cmd_and_data[1] : NULL,
				  len - 1U);
}

static int st7701_rgb_blanking_on(const struct device *dev)
{
	return st7701_rgb_command(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int st7701_rgb_blanking_off(const struct device *dev)
{
	return st7701_rgb_command(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static int st7701_rgb_hw_reset(const struct st7701_rgb_config *cfg)
{
	int ret;

	if (cfg->reset_gpio.port == NULL) {
		return -ENOSYS;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_set_dt(&cfg->reset_gpio, 1);
	if (ret < 0) {
		return ret;
	}
	k_msleep(cfg->reset_delay_ms);

	ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
	if (ret < 0) {
		return ret;
	}

	/* Panel needs time to boot its internal controller after reset release,
	 * before it can reliably accept SPI commands.
	 */
	k_msleep(ST7701_RESET_SETTLE_DELAY_MS);

	return 0;
}

static int st7701_rgb_send_init_sequence(const struct device *dev)
{
	const struct st7701_rgb_config *cfg = dev->config;
	int ret;

	ret = st7701_rgb_command(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_msleep(ST7701_SLEEP_OUT_DELAY_MS);

	ret = st7701_rgb_write_cmd_array(dev, st7701_cmd2_bk0_sel, sizeof(st7701_cmd2_bk0_sel));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, st7701_cmd2_bk0_lneset,
					 sizeof(st7701_cmd2_bk0_lneset));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, st7701_cmd2_bk0_porctrl,
					 sizeof(st7701_cmd2_bk0_porctrl));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, st7701_cmd2_bk0_invsel,
					 sizeof(st7701_cmd2_bk0_invsel));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, st7701_cmd2_bk0_cd, sizeof(st7701_cmd2_bk0_cd));
	if (ret < 0) {
		return ret;
	}

	ret = st7701_rgb_write_cmd_array(dev, cfg->pvgamctrl, sizeof(cfg->pvgamctrl));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->nvgamctrl, sizeof(cfg->nvgamctrl));
	if (ret < 0) {
		return ret;
	}

	ret = st7701_rgb_write_cmd_array(dev, st7701_cmd2_bk1_sel, sizeof(st7701_cmd2_bk1_sel));
	if (ret < 0) {
		return ret;
	}

	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_VRHS, ST7701_VAL_BK1_VRHS);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_VCOM, ST7701_VAL_BK1_VCOM);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_VGHSS, ST7701_VAL_BK1_VGHSS);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_TESTCMD, ST7701_VAL_BK1_TESTCMD);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_VGLS, ST7701_VAL_BK1_VGLS);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_PWCTRL1, ST7701_VAL_BK1_PWCTRL1);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_PWCTRL2, ST7701_VAL_BK1_PWCTRL2);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_SPD1, ST7701_VAL_BK1_SPD1);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, ST7701_CMD2_BK1_SPD2, ST7701_VAL_BK1_SPD2);
	if (ret < 0) {
		return ret;
	}
	k_msleep(ST7701_BK1_SPD2_DELAY_MS);

	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e0, sizeof(cfg->gip_e0));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e1, sizeof(cfg->gip_e1));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e2, sizeof(cfg->gip_e2));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e3, sizeof(cfg->gip_e3));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e4, sizeof(cfg->gip_e4));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e5, sizeof(cfg->gip_e5));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e6, sizeof(cfg->gip_e6));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e7, sizeof(cfg->gip_e7));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_e8, sizeof(cfg->gip_e8));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_eb, sizeof(cfg->gip_eb));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_ec, sizeof(cfg->gip_ec));
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_write_cmd_array(dev, cfg->gip_ed, sizeof(cfg->gip_ed));
	if (ret < 0) {
		return ret;
	}

	ret = st7701_rgb_write_cmd_array(dev, st7701_cmd2_bkx_sel_none,
					 sizeof(st7701_cmd2_bkx_sel_none));
	if (ret < 0) {
		return ret;
	}

	ret = st7701_rgb_command_u8(dev, MIPI_DCS_SET_ADDRESS_MODE, ST7701_MADCTL_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	ret = st7701_rgb_command_u8(dev, MIPI_DCS_SET_PIXEL_FORMAT, ST7701_COLMOD_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (cfg->inversion_on) {
		ret = st7701_rgb_command(dev, MIPI_DCS_ENTER_INVERT_MODE, NULL, 0);
		if (ret < 0) {
			return ret;
		}
		k_msleep(ST7701_INVERSION_DELAY_MS);
	}

	ret = st7701_rgb_blanking_off(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int st7701_rgb_init(const struct device *dev)
{
	const struct st7701_rgb_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->mipi_dbi)) {
		LOG_ERR("MIPI DBI command channel not ready");
		return -ENODEV;
	}

	ret = st7701_rgb_hw_reset(cfg);
	if (ret == -ENOSYS) {
		ret = mipi_dbi_reset(cfg->mipi_dbi, (uint32_t)cfg->reset_delay_ms);
	}

	if ((ret < 0) && (ret != -ENOSYS)) {
		LOG_ERR("Panel reset failed: %d", ret);
		return ret;
	}

	ret = st7701_rgb_send_init_sequence(dev);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("ST7701 panel ready: %ux%u", cfg->width, cfg->height);
	return 0;
}

static DEVICE_API(display, st7701_rgb_api) = {
	.blanking_on = st7701_rgb_blanking_on,
	.blanking_off = st7701_rgb_blanking_off,
};

#define ST7701_RGB_DBI_CFG(inst)                                                                   \
	MIPI_DBI_CONFIG_DT(DT_DRV_INST(inst), SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(9), 0)

#define ST7701_RGB_INST_DEFINE(inst)                                                               \
	static const struct st7701_rgb_config st7701_rgb_cfg_##inst = {                            \
		.mipi_dbi = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(inst))),                              \
		.dbi_config = ST7701_RGB_DBI_CFG(inst),                                            \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.width = DT_PROP(DT_DRV_INST(inst), width),                                        \
		.height = DT_PROP(DT_DRV_INST(inst), height),                                      \
		.reset_delay_ms = DT_PROP(DT_DRV_INST(inst), reset_delay_ms),                      \
		.inversion_on = DT_PROP(DT_DRV_INST(inst), inversion_on),                          \
		.pvgamctrl = DT_PROP(DT_DRV_INST(inst), pvgamctrl),                                \
		.nvgamctrl = DT_PROP(DT_DRV_INST(inst), nvgamctrl),                                \
		.gip_e0 = DT_PROP(DT_DRV_INST(inst), gip_e0),                                      \
		.gip_e1 = DT_PROP(DT_DRV_INST(inst), gip_e1),                                      \
		.gip_e2 = DT_PROP(DT_DRV_INST(inst), gip_e2),                                      \
		.gip_e3 = DT_PROP(DT_DRV_INST(inst), gip_e3),                                      \
		.gip_e4 = DT_PROP(DT_DRV_INST(inst), gip_e4),                                      \
		.gip_e5 = DT_PROP(DT_DRV_INST(inst), gip_e5),                                      \
		.gip_e6 = DT_PROP(DT_DRV_INST(inst), gip_e6),                                      \
		.gip_e7 = DT_PROP(DT_DRV_INST(inst), gip_e7),                                      \
		.gip_e8 = DT_PROP(DT_DRV_INST(inst), gip_e8),                                      \
		.gip_eb = DT_PROP(DT_DRV_INST(inst), gip_eb),                                      \
		.gip_ec = DT_PROP(DT_DRV_INST(inst), gip_ec),                                      \
		.gip_ed = DT_PROP(DT_DRV_INST(inst), gip_ed),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, st7701_rgb_init, NULL, NULL, &st7701_rgb_cfg_##inst,           \
			      POST_KERNEL, CONFIG_ST7701_RGB_INIT_PRIORITY, &st7701_rgb_api);

DT_INST_FOREACH_STATUS_OKAY(ST7701_RGB_INST_DEFINE)
