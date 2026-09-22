/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT sifli_sf32lb_lcdc_mipi_dbi

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <register.h>

LOG_MODULE_REGISTER(mipi_dbi_sf32lb, CONFIG_MIPI_DBI_LOG_LEVEL);

#define LCDC_SETTING    offsetof(LCD_IF_TypeDef, SETTING)
#define LCD_CONF        offsetof(LCD_IF_TypeDef, LCD_CONF)
#define LCD_IF_CONF     offsetof(LCD_IF_TypeDef, LCD_IF_CONF)
#define TE_CONF         offsetof(LCD_IF_TypeDef, TE_CONF)
#define TE_CONF2        offsetof(LCD_IF_TypeDef, TE_CONF2)
#define LCD_WR          offsetof(LCD_IF_TypeDef, LCD_WR)
#define LCD_RD          offsetof(LCD_IF_TypeDef, LCD_RD)
#define LCD_SINGLE      offsetof(LCD_IF_TypeDef, LCD_SINGLE)
#define LCD_SPI_IF_CONF offsetof(LCD_IF_TypeDef, SPI_IF_CONF)
#define LCD_STATUS      offsetof(LCD_IF_TypeDef, STATUS)

#define LCD_INTF_SEL_DBI_TYPEB (0U)
#define LCD_INTF_SEL_SPI       (1U)
#define LCD_INTF_SEL_JDI       (4U)
#define LCD_INTF_SEL_DBI_TYPEA (6U)

struct dbi_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	struct sf32lb_clock_dt_spec clock;
};

struct dbi_sf32lb_data {
	const struct mipi_dbi_config *active_config;
};

static inline void wait_busy(const struct device *dev)
{
	const struct dbi_sf32lb_config *config = dev->config;

	while (sys_test_bit(config->base + LCD_SINGLE, LCD_IF_LCD_SINGLE_LCD_BUSY_Pos) ||
	       sys_test_bit(config->base + LCD_STATUS, LCD_IF_STATUS_LCD_BUSY_Pos)) {
	}
}

static void mipi_dbi_sf32lb_spi_sequence(const struct device *dev, bool end)
{
	const struct dbi_sf32lb_config *config = dev->config;

	wait_busy(dev);

	if (end) {
		sys_set_bit(config->base + LCD_SPI_IF_CONF, LCD_IF_SPI_IF_CONF_SPI_CS_AUTO_DIS_Pos);
	} else {
		sys_clear_bit(config->base + LCD_SPI_IF_CONF,
			      LCD_IF_SPI_IF_CONF_SPI_CS_AUTO_DIS_Pos);
	}
}

static void mipi_dbi_sf32lb_send_single_cmd(const struct device *dev, uint32_t addr,
					    uint32_t addr_len)
{
	const struct dbi_sf32lb_config *config = dev->config;
	uint32_t spi_if_conf;

	wait_busy(dev);

	spi_if_conf = sys_read32(config->base + LCD_SPI_IF_CONF);
	spi_if_conf &= ~(LCD_IF_SPI_IF_CONF_RD_LEN_Msk | LCD_IF_SPI_IF_CONF_SPI_RD_MODE_Msk |
			 LCD_IF_SPI_IF_CONF_WR_LEN_Msk);

	if ((addr_len > 0) && (addr_len <= 4)) {
		spi_if_conf |= FIELD_PREP(LCD_IF_SPI_IF_CONF_WR_LEN_Msk, addr_len - 1);

		sys_write32(spi_if_conf, config->base + LCD_SPI_IF_CONF);
		sys_write32(addr, config->base + LCD_WR);
		sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG | LCD_IF_LCD_SINGLE_TYPE,
			    config->base + LCD_SINGLE);
	}
}

static void mipi_dbi_sf32lb_recv_single_data(const struct device *dev, uint8_t *buf, uint32_t len)
{
	const struct dbi_sf32lb_config *config = dev->config;
	uint32_t spi_if_conf;
	uint32_t data;

	wait_busy(dev);

	spi_if_conf = sys_read32(config->base + LCD_SPI_IF_CONF);
	spi_if_conf &= ~(LCD_IF_SPI_IF_CONF_RD_LEN_Msk | LCD_IF_SPI_IF_CONF_SPI_RD_MODE_Msk |
			 LCD_IF_SPI_IF_CONF_WR_LEN_Msk);

	spi_if_conf |= FIELD_PREP(LCD_IF_SPI_IF_CONF_RD_LEN_Msk, len - 1U) |
		       FIELD_PREP(LCD_IF_SPI_IF_CONF_SPI_RD_MODE_Msk, 1U);

	sys_write32(spi_if_conf, config->base + LCD_SPI_IF_CONF);
	sys_write32(LCD_IF_LCD_SINGLE_RD_TRIG, config->base + LCD_SINGLE);

	wait_busy(dev);

	data = sys_read32(config->base + LCD_RD);

	sys_put_le32(data, buf);
}

static void mipi_dbi_sf32lb_type_c_read_bytes(const struct device *dev, uint32_t addr,
					   uint32_t addr_len, uint8_t *buf, uint16_t len)
{
	wait_busy(dev);
	mipi_dbi_sf32lb_spi_sequence(dev, false);
	mipi_dbi_sf32lb_send_single_cmd(dev, addr, addr_len);

	mipi_dbi_sf32lb_recv_single_data(dev, buf, len);
	mipi_dbi_sf32lb_spi_sequence(dev, true);
}

static inline void wait_lcdc_single_busy(const struct device *dev)
{
	const struct dbi_sf32lb_config *config = dev->config;

	while (sys_test_bit(config->base + LCD_SINGLE, LCD_IF_LCD_SINGLE_LCD_BUSY_Pos)) {
	}
}

static void mipi_dbi_sf32lb_write_bytes(const struct device *dev, uint32_t addr, uint16_t addr_len,
					const uint8_t *buf, uint16_t len)
{
	const struct dbi_sf32lb_config *config = dev->config;
	uint32_t spi_if_conf;

	wait_busy(dev);

	mipi_dbi_sf32lb_spi_sequence(dev, 0U == len);

	spi_if_conf = sys_read32(config->base + LCD_SPI_IF_CONF);
	spi_if_conf &= ~(LCD_IF_SPI_IF_CONF_RD_LEN_Msk | LCD_IF_SPI_IF_CONF_SPI_RD_MODE_Msk |
			 LCD_IF_SPI_IF_CONF_WR_LEN_Msk);

	spi_if_conf |= FIELD_PREP(LCD_IF_SPI_IF_CONF_WR_LEN_Msk, addr_len - 1U);

	sys_write32(spi_if_conf, config->base + LCD_SPI_IF_CONF);
	sys_write32(addr, config->base + LCD_WR);
	sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG | LCD_IF_LCD_SINGLE_TYPE, config->base + LCD_SINGLE);

	uint32_t data_len = len;
	uint32_t total_len = len;

	while (data_len > 0) {
		uint32_t v, l;

		/* Convert 0xAA,0xBB,0xCC ->  0x00AABBCC */
		for (v = 0, l = 0; (l < 4) && (data_len > 0); l++) {
			v = (v << 8) | (*buf);
			data_len--;
			buf++;
		}

		wait_lcdc_single_busy(dev);

		total_len -= l;
		mipi_dbi_sf32lb_spi_sequence(dev, (0 == total_len));

		spi_if_conf = sys_read32(config->base + LCD_SPI_IF_CONF);
		spi_if_conf &= ~(LCD_IF_SPI_IF_CONF_WR_LEN_Msk);
		spi_if_conf |= FIELD_PREP(LCD_IF_SPI_IF_CONF_WR_LEN_Msk, l - 1U);

		sys_write32(spi_if_conf, config->base + LCD_SPI_IF_CONF);
		sys_write32(v, config->base + LCD_WR);
		sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG | LCD_IF_LCD_SINGLE_TYPE,
			    config->base + LCD_SINGLE);
	}
}

static int mipi_dbi_sf32lb_freq_config(const struct device *dev,
				       const struct mipi_dbi_config *dbi_config)
{
	const struct dbi_sf32lb_config *config = dev->config;
	uint32_t freq = dbi_config->config.frequency;
	uint32_t lcdc_clk;
	uint32_t pw, pwl, pwh;
	uint32_t lcd_if_conf;
	int ret;

	ret = sf32lb_clock_control_get_rate_dt(&config->clock, &lcdc_clk);
	if (ret < 0) {
		LOG_ERR("Failed to get LCDC clock rate");
		return ret;
	}

	pw = (lcdc_clk + (freq - 1)) / freq;
	pwl = pw / 2;
	pwh = pw - pwl;

	if (pwl < 1) {
		pwl = 1;
	}
	if (pwl > FIELD_GET(LCD_IF_LCD_IF_CONF_PWL_Msk, LCD_IF_LCD_IF_CONF_PWL_Msk)) {
		pwl = FIELD_GET(LCD_IF_LCD_IF_CONF_PWL_Msk, LCD_IF_LCD_IF_CONF_PWL_Msk);
	}

	if (pwh < 1) {
		pwh = 1;
	}
	if (pwh > FIELD_GET(LCD_IF_LCD_IF_CONF_PWH_Msk, LCD_IF_LCD_IF_CONF_PWH_Msk)) {
		pwh = FIELD_GET(LCD_IF_LCD_IF_CONF_PWH_Msk, LCD_IF_LCD_IF_CONF_PWH_Msk);
	}

	lcd_if_conf = sys_read32(config->base + LCD_IF_CONF);
	lcd_if_conf &= ~(LCD_IF_LCD_IF_CONF_PWL_Msk | LCD_IF_LCD_IF_CONF_PWH_Msk);
	lcd_if_conf |= FIELD_PREP(LCD_IF_LCD_IF_CONF_PWL_Msk, pwl) |
		       FIELD_PREP(LCD_IF_LCD_IF_CONF_PWH_Msk, pwh);
	sys_write32(lcd_if_conf, config->base + LCD_IF_CONF);

	return ret;
}

static int mipi_dbi_sf32lb_spi_config(const struct device *dev,
				      const struct mipi_dbi_config *dbi_config)
{
	const struct dbi_sf32lb_config *config = dev->config;
	const struct spi_config *spi_config = &dbi_config->config;
	uint8_t bus_type = dbi_config->mode & 0xFU;
	uint32_t spi_if_conf = 0;
	uint32_t clk_div;
	uint32_t lcdc_clk;
	uint32_t freq = dbi_config->config.frequency;
	int ret;

	sys_clear_bits(config->base + LCD_SPI_IF_CONF, LCD_IF_SPI_IF_CONF_CLK_DIV);

	ret = sf32lb_clock_control_get_rate_dt(&config->clock, &lcdc_clk);
	if (ret < 0) {
		return ret;
	};

	spi_if_conf = LCD_IF_SPI_IF_CONF_SPI_CS_AUTO_DIS | LCD_IF_SPI_IF_CONF_SPI_CLK_AUTO_DIS |
		      LCD_IF_SPI_IF_CONF_SPI_CS_NO_IDLE;

	if (bus_type == MIPI_DBI_MODE_SPI_4WIRE) {
		spi_if_conf |= LCD_IF_SPI_IF_CONF_4LINE_4_DATA_LINE;
	} else if (bus_type == MIPI_DBI_MODE_SPI_3WIRE) {
		spi_if_conf |= LCD_IF_SPI_IF_CONF_3LINE;
	} else {
		return -EINVAL;
	}

	if (spi_config->operation & SPI_MODE_CPOL) {
		spi_if_conf &= ~LCD_IF_SPI_IF_CONF_SPI_CLK_INIT;
	} else {
		spi_if_conf |= LCD_IF_SPI_IF_CONF_SPI_CLK_INIT;
	}

	if (spi_config->operation & SPI_MODE_CPHA) {
		spi_if_conf |= LCD_IF_SPI_IF_CONF_SPI_CLK_POL;
	}

	clk_div = (lcdc_clk + (freq - 1)) / freq;
	if (clk_div < 2) { /* HW NOT support divider 1 */
		clk_div = 2;
	}
	if (clk_div > 0xFF) {
		clk_div = 0xFF;
	}

	spi_if_conf |= FIELD_PREP(LCD_IF_SPI_IF_CONF_CLK_DIV_Msk, clk_div);
	sys_write32(spi_if_conf, config->base + LCD_SPI_IF_CONF);

	return 0;
}

static int mipi_dbi_sf32lb_configure(const struct device *dev,
				     const struct mipi_dbi_config *dbi_config)
{
	const struct dbi_sf32lb_config *config = dev->config;
	struct dbi_sf32lb_data *data = dev->data;
	uint8_t bus_type = dbi_config->mode & 0xFU;
	uint32_t lcd_conf;
	uint32_t lcd_if_conf;
	int ret;

	if (dbi_config == data->active_config) {
		return 0;
	}

	lcd_conf = sys_read32(config->base + LCD_CONF);
	lcd_conf &= ~(LCD_IF_LCD_CONF_LCD_INTF_SEL_Msk | LCD_IF_LCD_CONF_TARGET_LCD_Msk);

	switch (bus_type) {
	case MIPI_DBI_MODE_8080_BUS_16_BIT:
	case MIPI_DBI_MODE_8080_BUS_9_BIT:
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
		lcd_conf |= FIELD_PREP(LCD_IF_LCD_CONF_LCD_INTF_SEL_Msk, LCD_INTF_SEL_DBI_TYPEB);
		lcd_conf |= FIELD_PREP(LCD_IF_LCD_CONF_TARGET_LCD_Msk, 0U);
		lcd_if_conf = sys_read32(config->base + LCD_IF_CONF);
		lcd_if_conf &= ~(LCD_IF_LCD_IF_CONF_TAS_Msk | LCD_IF_LCD_IF_CONF_TAH_Msk);
		lcd_if_conf |= FIELD_PREP(LCD_IF_LCD_IF_CONF_TAS_Msk, 1U) |
			       FIELD_PREP(LCD_IF_LCD_IF_CONF_TAH_Msk, 1U);
		sys_write32(lcd_if_conf, config->base + LCD_IF_CONF);
		mipi_dbi_sf32lb_freq_config(dev, dbi_config);
		break;

	case MIPI_DBI_MODE_SPI_3WIRE:
	case MIPI_DBI_MODE_SPI_4WIRE:
		lcd_conf |= FIELD_PREP(LCD_IF_LCD_CONF_LCD_INTF_SEL_Msk, LCD_INTF_SEL_SPI);
		ret = mipi_dbi_sf32lb_spi_config(dev, dbi_config);
		if (ret < 0) {
			return ret;
		}
		break;

	default:
		return -EINVAL;
	}

	sys_write32(lcd_conf, config->base + LCD_CONF);
	data->active_config = dbi_config;

	return 0;
}

static int mipi_dbi_reset_sf32lb(const struct device *dev, k_timeout_t delay)
{
	const struct dbi_sf32lb_config *config = dev->config;
	uint32_t delay_ms = k_ticks_to_ms_ceil32(delay.ticks);

	sys_clear_bit(config->base + LCD_IF_CONF, LCD_IF_LCD_IF_CONF_LCD_RSTB_Pos);
	k_msleep(delay_ms);
	sys_set_bit(config->base + LCD_IF_CONF, LCD_IF_LCD_IF_CONF_LCD_RSTB_Pos);

	return 0;
}

static int mipi_dbi_sf32lb_8080_cmd_write_bytes(const struct device *dev, uint8_t cmd,
						const uint8_t *data, size_t data_len)
{
	const struct dbi_sf32lb_config *config = dev->config;

	wait_busy(dev);
	sys_write32(cmd, config->base + LCD_WR);
	sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG, config->base + LCD_SINGLE);

	while (data_len > 0) {
		uint8_t v = *data;

		wait_busy(dev);
		sys_write32(v, config->base + LCD_WR);
		sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG | LCD_IF_LCD_SINGLE_TYPE,
			    config->base + LCD_SINGLE);

		data_len--;
		data++;
	}

	return 0;
}

static int mipi_dbi_command_write_sf32lb(const struct device *dev,
					 const struct mipi_dbi_config *dbi_config, uint8_t cmd,
					 const uint8_t *data, size_t len)
{
	uint32_t addr;
	uint8_t bus_type = dbi_config->mode & 0xFU;

	mipi_dbi_sf32lb_configure(dev, dbi_config);

	switch (bus_type) {
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
		return mipi_dbi_sf32lb_8080_cmd_write_bytes(dev, cmd, data, len);
	case MIPI_DBI_MODE_SPI_3WIRE:
	case MIPI_DBI_MODE_SPI_4WIRE:
		mipi_dbi_sf32lb_write_bytes(dev, addr, sizeof(addr), data, len);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mipi_dbi_sf32lb_8080_cmd_read_bytes(const struct device *dev, uint8_t *cmd,
					       size_t num_cmds, uint8_t *data, size_t data_len)
{
	const struct dbi_sf32lb_config *config = dev->config;

	while (num_cmds > 0) {
		wait_busy(dev);
		sys_write32(*cmd, config->base + LCD_WR);
		sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG, config->base + LCD_SINGLE);

		num_cmds--;
		cmd++;
	}

	while (data_len > 0) {
		wait_busy(dev);
		sys_write32(LCD_IF_LCD_SINGLE_RD_TRIG, config->base + LCD_SINGLE);

		wait_busy(dev);
		*data = sys_read8(config->base + LCD_RD);

		data_len--;
		data++;
	}

	return 0;
}

static int mipi_dbi_command_read_sf32lb(const struct device *dev,
					const struct mipi_dbi_config *dbi_config, uint8_t *cmds,
					size_t num_cmds, uint8_t *response, size_t len)
{
	ARG_UNUSED(num_cmds);
	uint32_t addr;
	uint8_t bus_type = dbi_config->mode & 0xFU;

	switch (bus_type) {
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
		mipi_dbi_sf32lb_8080_cmd_read_bytes(dev, cmds, num_cmds, response, len);
	case MIPI_DBI_MODE_SPI_3WIRE:
	case MIPI_DBI_MODE_SPI_4WIRE:
		mipi_dbi_sf32lb_configure(dev, dbi_config);
		mipi_dbi_sf32lb_type_c_read_bytes(dev, addr, sizeof(addr), response, len);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mipi_dbi_write_display_sf32lb(const struct device *dev,
					 const struct mipi_dbi_config *dbi_config,
					 const uint8_t *framebuf,
					 struct display_buffer_descriptor *desc,
					 enum display_pixel_format pixfmt)
{
	ARG_UNUSED(pixfmt);
	const struct dbi_sf32lb_config *config = dev->config;
	int data_len = desc->buf_size;
	const uint8_t *buf = framebuf;
	uint32_t spi_if_conf;
	uint8_t bus_type = dbi_config->mode & 0xFU;

	if (data_len < 0) {
		return 0;
	}

	if (bus_type == MIPI_DBI_MODE_8080_BUS_16_BIT || bus_type == MIPI_DBI_MODE_8080_BUS_9_BIT ||
	    bus_type == MIPI_DBI_MODE_8080_BUS_8_BIT) {
		while (data_len > 0) {
			wait_busy(dev);

			if (bus_type == MIPI_DBI_MODE_8080_BUS_16_BIT && data_len >= 2) {
				uint16_t v = sys_get_le16(buf);

				sys_write32(v, config->base + LCD_WR);
				sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG | LCD_IF_LCD_SINGLE_TYPE,
					    config->base + LCD_SINGLE);
				buf += 2;
				data_len -= 2;
			} else {
				sys_write32(*buf, config->base + LCD_WR);
				sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG | LCD_IF_LCD_SINGLE_TYPE,
					    config->base + LCD_SINGLE);
				buf++;
				data_len--;
			}
		}
	} else if (bus_type == MIPI_DBI_MODE_SPI_3WIRE || bus_type == MIPI_DBI_MODE_SPI_4WIRE) {
		uint32_t total_len = data_len;

		mipi_dbi_sf32lb_configure(dev, dbi_config);

		while (data_len > 0) {
			uint32_t v, l;

			/* Convert 0xAA,0xBB,0xCC ->  0x00AABBCC */
			for (v = 0, l = 0; (l < 4) && (data_len > 0); l++) {
				v = (v << 8) | (*buf);
				data_len--;
				buf++;
			}

			wait_lcdc_single_busy(dev);

			total_len -= l;
			mipi_dbi_sf32lb_spi_sequence(dev, (0 == total_len));

			spi_if_conf = sys_read32(config->base + LCD_SPI_IF_CONF);
			spi_if_conf &= ~(LCD_IF_SPI_IF_CONF_WR_LEN_Msk);
			spi_if_conf |= FIELD_PREP(LCD_IF_SPI_IF_CONF_WR_LEN_Msk, l - 1U);

			sys_write32(spi_if_conf, config->base + LCD_SPI_IF_CONF);
			sys_write32(v, config->base + LCD_WR);
			sys_write32(LCD_IF_LCD_SINGLE_WR_TRIG | LCD_IF_LCD_SINGLE_TYPE,
				    config->base + LCD_SINGLE);
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int mipi_dbi_configure_te_sf32lb(const struct device *dev, uint8_t edge, k_timeout_t delay)
{
	const struct dbi_sf32lb_config *config = dev->config;
	uint32_t delay_us = k_ticks_to_us_ceil32(delay.ticks);
	uint32_t te_conf;

	te_conf = sys_read32(config->base + TE_CONF);

	te_conf &= ~LCD_IF_TE_CONF_FMARK_MODE_Msk;

	if (edge == MIPI_DBI_TE_RISING_EDGE) {
		te_conf |= FIELD_PREP(LCD_IF_TE_CONF_FMARK_POL_Msk, 0);
	} else if (edge == MIPI_DBI_TE_FALLING_EDGE) {
		te_conf |= FIELD_PREP(LCD_IF_TE_CONF_FMARK_POL_Msk, 1);
	} else {
		return -EINVAL;
	}

	te_conf |= FIELD_PREP(LCD_IF_TE_CONF_FMARK_MODE_Msk, 1) |
		   FIELD_PREP(LCD_IF_TE_CONF_ENABLE_Msk, 1);

	sys_write32(delay_us, config->base + TE_CONF2);
	sys_write32(te_conf, config->base + TE_CONF);

	return 0;
}

static DEVICE_API(mipi_dbi, dbi_sf32lb_api) = {
	.reset = mipi_dbi_reset_sf32lb,
	.command_write = mipi_dbi_command_write_sf32lb,
	.command_read = mipi_dbi_command_read_sf32lb,
	.write_display = mipi_dbi_write_display_sf32lb,
	.configure_te = mipi_dbi_configure_te_sf32lb,
};

static int mipi_dbi_init_sf32lb(const struct device *dev)
{
	const struct dbi_sf32lb_config *config = dev->config;
	int err;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	err = sf32lb_clock_control_on_dt(&config->clock);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", err);
		return err;
	}

	sys_set_bit(config->base + LCDC_SETTING, LCD_IF_SETTING_AUTO_GATE_EN_Pos);
	sys_set_bit(config->base + LCD_IF_CONF, LCD_IF_LCD_IF_CONF_LCD_RSTB_Pos);

	return err;
}

#define DBI_SF32LB_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct dbi_sf32lb_data dbi_sf32lb_data_##n;                                         \
                                                                                                   \
	static const struct dbi_sf32lb_config dbi_sf32lb_config_##n = {                            \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.clock = SF32LB_CLOCK_DT_INST_PARENT_SPEC_GET(n),                                  \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_init_sf32lb, NULL, &dbi_sf32lb_data_##n,                 \
			      &dbi_sf32lb_config_##n, POST_KERNEL, CONFIG_MIPI_DBI_INIT_PRIORITY,  \
			      &dbi_sf32lb_api);                                                    \
	BUILD_ASSERT((DT_CHILD_NUM_STATUS_OKAY(DT_INST_PARENT(n)) == 1),                           \
		"LCDC only supports one operating mode");

DT_INST_FOREACH_STATUS_OKAY(DBI_SF32LB_DEFINE)
