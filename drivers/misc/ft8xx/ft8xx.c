/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ftdi_ft800

#include <zephyr/drivers/misc/ft8xx/ft8xx.h>

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/misc/ft8xx/ft8xx_copro.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_common.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_dl.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_memory.h>

#include "ft8xx_drv.h"
#include "ft8xx_host_commands.h"

LOG_MODULE_REGISTER(ft8xx, CONFIG_DISPLAY_LOG_LEVEL);

#define FT8XX_DLSWAP_FRAME 0x02

#define FT8XX_EXPECTED_ID 0x7C

struct ft8xx_config {
	uint16_t vsize;
	uint16_t voffset;
	uint16_t vcycle;
	uint16_t vsync0;
	uint16_t vsync1;
	uint16_t hsize;
	uint16_t hoffset;
	uint16_t hcycle;
	uint16_t hsync0;
	uint16_t hsync1;
	uint8_t pclk;
	uint8_t pclk_pol :1;
	uint8_t cspread  :1;
	uint8_t swizzle  :4;
};

struct ft8xx_data {
	const struct ft8xx_config *config;
	ft8xx_int_callback irq_callback;
};

const static struct ft8xx_config ft8xx_config = {
	.pclk     = DT_INST_PROP(0, pclk),
	.pclk_pol = DT_INST_PROP(0, pclk_pol),
	.cspread  = DT_INST_PROP(0, cspread),
	.swizzle  = DT_INST_PROP(0, swizzle),
	.vsize    = DT_INST_PROP(0, vsize),
	.voffset  = DT_INST_PROP(0, voffset),
	.vcycle   = DT_INST_PROP(0, vcycle),
	.vsync0   = DT_INST_PROP(0, vsync0),
	.vsync1   = DT_INST_PROP(0, vsync1),
	.hsize    = DT_INST_PROP(0, hsize),
	.hoffset  = DT_INST_PROP(0, hoffset),
	.hcycle   = DT_INST_PROP(0, hcycle),
	.hsync0   = DT_INST_PROP(0, hsync0),
	.hsync1   = DT_INST_PROP(0, hsync1),
};

static struct ft8xx_data ft8xx_data = {
	.config = &ft8xx_config,
	.irq_callback = NULL,
};

static void host_command(uint8_t cmd)
{
	int err;

	err = ft8xx_drv_command(cmd);
	__ASSERT(err == 0, "Writing FT8xx command failed");
}

static void wait(void)
{
	k_sleep(K_MSEC(20));
}

static bool verify_chip(void)
{
	uint32_t id = ft8xx_rd32(FT800_REG_ID);

	return (id & 0xff) == FT8XX_EXPECTED_ID;
}

static int ft8xx_init(const struct device *dev)
{
	int ret;
	const struct ft8xx_config *config = dev->config;

	ret = ft8xx_drv_init();
	if (ret < 0) {
		LOG_ERR("FT8xx driver initialization failed with %d", ret);
		return ret;
	}

	/* Reset display controller */
	host_command(CORERST);
	host_command(ACTIVE);
	wait();
	host_command(CLKEXT);
	host_command(CLK48M);
	wait();

	host_command(CORERST);
	host_command(ACTIVE);
	wait();
	host_command(CLKEXT);
	host_command(CLK48M);
	wait();

	if (!verify_chip()) {
		LOG_ERR("FT8xx chip not recognized");
		return -ENODEV;
	}

	/* Disable LCD */
	ft8xx_wr8(FT800_REG_GPIO, 0);
	ft8xx_wr8(FT800_REG_PCLK, 0);

	/* Configure LCD */
	ft8xx_wr16(FT800_REG_HSIZE, config->hsize);
	ft8xx_wr16(FT800_REG_HCYCLE, config->hcycle);
	ft8xx_wr16(FT800_REG_HOFFSET, config->hoffset);
	ft8xx_wr16(FT800_REG_HSYNC0, config->hsync0);
	ft8xx_wr16(FT800_REG_HSYNC1, config->hsync1);
	ft8xx_wr16(FT800_REG_VSIZE, config->vsize);
	ft8xx_wr16(FT800_REG_VCYCLE, config->vcycle);
	ft8xx_wr16(FT800_REG_VOFFSET, config->voffset);
	ft8xx_wr16(FT800_REG_VSYNC0, config->vsync0);
	ft8xx_wr16(FT800_REG_VSYNC1, config->vsync1);
	ft8xx_wr8(FT800_REG_SWIZZLE, config->swizzle);
	ft8xx_wr8(FT800_REG_PCLK_POL, config->pclk_pol);
	ft8xx_wr8(FT800_REG_CSPREAD, config->cspread);

	/* Display initial screen */

	/* Set the initial color */
	ft8xx_wr32(FT800_RAM_DL + 0, FT8XX_CLEAR_COLOR_RGB(0, 0x80, 0));
	/* Clear to the initial color */
	ft8xx_wr32(FT800_RAM_DL + 4, FT8XX_CLEAR(1, 1, 1));
	/* End the display list */
	ft8xx_wr32(FT800_RAM_DL + 8, FT8XX_DISPLAY());
	ft8xx_wr8(FT800_REG_DLSWAP, FT8XX_DLSWAP_FRAME);

	/* Enable LCD */

	/* Enable display bit */
	ft8xx_wr8(FT800_REG_GPIO_DIR, 0x80);
	ft8xx_wr8(FT800_REG_GPIO, 0x80);
	/* Enable backlight */
	ft8xx_wr16(FT800_REG_PWM_HZ, 0x00FA);
	ft8xx_wr8(FT800_REG_PWM_DUTY, 0x10);
	/* Enable LCD signals */
	ft8xx_wr8(FT800_REG_PCLK, config->pclk);

	return 0;
}

DEVICE_DEFINE(ft8xx_spi, "ft8xx_spi", ft8xx_init, NULL,
		&ft8xx_data, &ft8xx_config,
		APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

int ft8xx_get_touch_tag(void)
{
	/* Read FT800_REG_INT_FLAGS to clear IRQ */
	(void)ft8xx_rd8(FT800_REG_INT_FLAGS);

	return (int)ft8xx_rd8(FT800_REG_TOUCH_TAG);
}

void ft8xx_drv_irq_triggered(const struct device *dev, struct gpio_callback *cb,
			      uint32_t pins)
{
	if (ft8xx_data.irq_callback != NULL) {
		ft8xx_data.irq_callback();
	}
}

void ft8xx_register_int(ft8xx_int_callback callback)
{
	if (ft8xx_data.irq_callback != NULL) {
		return;
	}

	ft8xx_data.irq_callback = callback;
	ft8xx_wr8(FT800_REG_INT_MASK, 0x04);
	ft8xx_wr8(FT800_REG_INT_EN, 0x01);
}

void ft8xx_calibrate(struct ft8xx_touch_transform *data)
{
	uint32_t result = 0;

	do {
		ft8xx_copro_cmd_dlstart();
		ft8xx_copro_cmd(FT8XX_CLEAR_COLOR_RGB(0x00, 0x00, 0x00));
		ft8xx_copro_cmd(FT8XX_CLEAR(1, 1, 1));
		ft8xx_copro_cmd_calibrate(&result);
	} while (result == 0);

	data->a = ft8xx_rd32(FT800_REG_TOUCH_TRANSFORM_A);
	data->b = ft8xx_rd32(FT800_REG_TOUCH_TRANSFORM_B);
	data->c = ft8xx_rd32(FT800_REG_TOUCH_TRANSFORM_C);
	data->d = ft8xx_rd32(FT800_REG_TOUCH_TRANSFORM_D);
	data->e = ft8xx_rd32(FT800_REG_TOUCH_TRANSFORM_E);
	data->f = ft8xx_rd32(FT800_REG_TOUCH_TRANSFORM_F);
}

void ft8xx_touch_transform_set(const struct ft8xx_touch_transform *data)
{
	ft8xx_wr32(FT800_REG_TOUCH_TRANSFORM_A, data->a);
	ft8xx_wr32(FT800_REG_TOUCH_TRANSFORM_B, data->b);
	ft8xx_wr32(FT800_REG_TOUCH_TRANSFORM_C, data->c);
	ft8xx_wr32(FT800_REG_TOUCH_TRANSFORM_D, data->d);
	ft8xx_wr32(FT800_REG_TOUCH_TRANSFORM_E, data->e);
	ft8xx_wr32(FT800_REG_TOUCH_TRANSFORM_F, data->f);
}
