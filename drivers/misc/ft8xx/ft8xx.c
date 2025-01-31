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

#include "ft8xx_dev_data.h"
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

static void host_command(const struct device *dev, uint8_t cmd)
{
	int err;

	err = ft8xx_drv_command(dev, cmd);
	__ASSERT(err == 0, "Writing FT8xx command failed");
}

static void wait(void)
{
	k_sleep(K_MSEC(20));
}

static bool verify_chip(const struct device *dev)
{
	uint32_t id = ft8xx_rd32(dev, FT800_REG_ID);

	return (id & 0xff) == FT8XX_EXPECTED_ID;
}

static int ft8xx_init(const struct device *dev)
{
	int ret;
	const struct ft8xx_config *config = dev->config;
	struct ft8xx_data *data = dev->data;

	data->ft8xx_dev = dev;

	ret = ft8xx_drv_init(dev);
	if (ret < 0) {
		LOG_ERR("FT8xx driver initialization failed with %d", ret);
		return ret;
	}

	/* Reset display controller */
	host_command(dev, CORERST);
	host_command(dev, ACTIVE);
	wait();
	host_command(dev, CLKEXT);
	host_command(dev, CLK48M);
	wait();

	host_command(dev, CORERST);
	host_command(dev, ACTIVE);
	wait();
	host_command(dev, CLKEXT);
	host_command(dev, CLK48M);
	wait();

	if (!verify_chip(dev)) {
		LOG_ERR("FT8xx chip not recognized");
		return -ENODEV;
	}

	/* Disable LCD */
	ft8xx_wr8(dev, FT800_REG_GPIO, 0);
	ft8xx_wr8(dev, FT800_REG_PCLK, 0);

	/* Configure LCD */
	ft8xx_wr16(dev, FT800_REG_HSIZE, config->hsize);
	ft8xx_wr16(dev, FT800_REG_HCYCLE, config->hcycle);
	ft8xx_wr16(dev, FT800_REG_HOFFSET, config->hoffset);
	ft8xx_wr16(dev, FT800_REG_HSYNC0, config->hsync0);
	ft8xx_wr16(dev, FT800_REG_HSYNC1, config->hsync1);
	ft8xx_wr16(dev, FT800_REG_VSIZE, config->vsize);
	ft8xx_wr16(dev, FT800_REG_VCYCLE, config->vcycle);
	ft8xx_wr16(dev, FT800_REG_VOFFSET, config->voffset);
	ft8xx_wr16(dev, FT800_REG_VSYNC0, config->vsync0);
	ft8xx_wr16(dev, FT800_REG_VSYNC1, config->vsync1);
	ft8xx_wr8(dev, FT800_REG_SWIZZLE, config->swizzle);
	ft8xx_wr8(dev, FT800_REG_PCLK_POL, config->pclk_pol);
	ft8xx_wr8(dev, FT800_REG_CSPREAD, config->cspread);

	/* Display initial screen */

	/* Set the initial color */
	ft8xx_wr32(dev, FT800_RAM_DL + 0, FT8XX_CLEAR_COLOR_RGB(0, 0x80, 0));
	/* Clear to the initial color */
	ft8xx_wr32(dev, FT800_RAM_DL + 4, FT8XX_CLEAR(1, 1, 1));
	/* End the display list */
	ft8xx_wr32(dev, FT800_RAM_DL + 8, FT8XX_DISPLAY());
	ft8xx_wr8(dev, FT800_REG_DLSWAP, FT8XX_DLSWAP_FRAME);

	/* Enable LCD */

	/* Enable display bit */
	ft8xx_wr8(dev, FT800_REG_GPIO_DIR, 0x80);
	ft8xx_wr8(dev, FT800_REG_GPIO, 0x80);
	/* Enable backlight */
	ft8xx_wr16(dev, FT800_REG_PWM_HZ, 0x00FA);
	ft8xx_wr8(dev, FT800_REG_PWM_DUTY, 0x10);
	/* Enable LCD signals */
	ft8xx_wr8(dev, FT800_REG_PCLK, config->pclk);

	return 0;
}

int ft8xx_get_touch_tag(const struct device *dev)
{
	/* Read FT800_REG_INT_FLAGS to clear IRQ */
	(void)ft8xx_rd8(dev, FT800_REG_INT_FLAGS);

	return (int)ft8xx_rd8(dev, FT800_REG_TOUCH_TAG);
}

void ft8xx_drv_irq_triggered(const struct device *gpio_port, struct gpio_callback *cb,
			     uint32_t pins)
{
	struct ft8xx_data *ft8xx_data = CONTAINER_OF(cb, struct ft8xx_data, irq_cb_data);
	const struct device *ft8xx_dev = ft8xx_data->ft8xx_dev;

	if (ft8xx_data->irq_callback != NULL) {
		ft8xx_data->irq_callback(ft8xx_dev, ft8xx_data->irq_callback_ud);
	}
}

void ft8xx_register_int(const struct device *dev, ft8xx_int_callback callback, void *user_data)
{
	struct ft8xx_data *ft8xx_data = dev->data;

	if (ft8xx_data->irq_callback != NULL) {
		return;
	}

	ft8xx_data->irq_callback = callback;
	ft8xx_data->irq_callback_ud = user_data;
	ft8xx_wr8(dev, FT800_REG_INT_MASK, 0x04);
	ft8xx_wr8(dev, FT800_REG_INT_EN, 0x01);
}

void ft8xx_calibrate(const struct device *dev, struct ft8xx_touch_transform *data)
{
	uint32_t result = 0;

	do {
		ft8xx_copro_cmd_dlstart(dev);
		ft8xx_copro_cmd(dev, FT8XX_CLEAR_COLOR_RGB(0x00, 0x00, 0x00));
		ft8xx_copro_cmd(dev, FT8XX_CLEAR(1, 1, 1));
		ft8xx_copro_cmd_calibrate(dev, &result);
	} while (result == 0);

	data->a = ft8xx_rd32(dev, FT800_REG_TOUCH_TRANSFORM_A);
	data->b = ft8xx_rd32(dev, FT800_REG_TOUCH_TRANSFORM_B);
	data->c = ft8xx_rd32(dev, FT800_REG_TOUCH_TRANSFORM_C);
	data->d = ft8xx_rd32(dev, FT800_REG_TOUCH_TRANSFORM_D);
	data->e = ft8xx_rd32(dev, FT800_REG_TOUCH_TRANSFORM_E);
	data->f = ft8xx_rd32(dev, FT800_REG_TOUCH_TRANSFORM_F);
}

void ft8xx_touch_transform_set(const struct device *dev, const struct ft8xx_touch_transform *data)
{
	ft8xx_wr32(dev, FT800_REG_TOUCH_TRANSFORM_A, data->a);
	ft8xx_wr32(dev, FT800_REG_TOUCH_TRANSFORM_B, data->b);
	ft8xx_wr32(dev, FT800_REG_TOUCH_TRANSFORM_C, data->c);
	ft8xx_wr32(dev, FT800_REG_TOUCH_TRANSFORM_D, data->d);
	ft8xx_wr32(dev, FT800_REG_TOUCH_TRANSFORM_E, data->e);
	ft8xx_wr32(dev, FT800_REG_TOUCH_TRANSFORM_F, data->f);
}

#define FT8XX_DEVICE(idx)                                                                          \
const static struct ft8xx_config ft8xx_##idx##_config = {                                          \
	.pclk     = DT_INST_PROP(idx, pclk),                                                       \
	.pclk_pol = DT_INST_PROP(idx, pclk_pol),                                                   \
	.cspread  = DT_INST_PROP(idx, cspread),                                                    \
	.swizzle  = DT_INST_PROP(idx, swizzle),                                                    \
	.vsize    = DT_INST_PROP(idx, vsize),                                                      \
	.voffset  = DT_INST_PROP(idx, voffset),                                                    \
	.vcycle   = DT_INST_PROP(idx, vcycle),                                                     \
	.vsync0   = DT_INST_PROP(idx, vsync0),                                                     \
	.vsync1   = DT_INST_PROP(idx, vsync1),                                                     \
	.hsize    = DT_INST_PROP(idx, hsize),                                                      \
	.hoffset  = DT_INST_PROP(idx, hoffset),                                                    \
	.hcycle   = DT_INST_PROP(idx, hcycle),                                                     \
	.hsync0   = DT_INST_PROP(idx, hsync0),                                                     \
	.hsync1   = DT_INST_PROP(idx, hsync1),                                                     \
};                                                                                                 \
static struct ft8xx_data ft8xx_##idx##_data = {                                                    \
	.ft8xx_dev = NULL,                                                                         \
	.irq_callback = NULL,                                                                      \
	.irq_callback_ud = NULL,                                                                   \
												   \
	.spi = SPI_DT_SPEC_INST_GET(idx, SPI_WORD_SET(8) | SPI_OP_MODE_MASTER, 0),                 \
	.irq_gpio = GPIO_DT_SPEC_INST_GET(idx, irq_gpios),                                         \
};                                                                                                 \
DEVICE_DT_INST_DEFINE(idx, ft8xx_init, NULL, &ft8xx_##idx##_data, &ft8xx_##idx##_config,           \
		      POST_KERNEL, CONFIG_FT800_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(FT8XX_DEVICE)
