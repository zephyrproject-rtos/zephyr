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
#include <zephyr/drivers/misc/ft8xx/ft8xx_api.h>


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
	uint chip_id;
	uint chip_type;
	ft8xx_memory_map_t *memory_map;
	ft8xx_register_address_map_t *register_map;
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
	.chip_id = 0,
	.chip_type = 0,
	.memory_map = NULL,
	.register_map = NULL,
};

static struct ft8xx_api {
   
   .do_this = generic_do_this,
   
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


static bool check_chiptype(int chiptype) 
{
	switch(data->chip_id) {
		case FT8xx_CHIP_ID_FT800:
		case FT8xx_CHIP_ID_FT810:
		case FT8xx_CHIP_ID_FT811:
		case FT8xx_CHIP_ID_FT812:
		case FT8xx_CHIP_ID_FT813:
			{
				return 1;
			}
		default:
			{
				return 0;
			}
	}

}

static bool verify_chip(struct ft8xx_data *data)
{
	int dt_chipid = DT_INST_PROP_OR(0, chipid,0) 
	if dt_chipid <> 0
	{
		// check custom chipID matches that in device memory
		if (chipid != data->chip_id) 
		{
			return 0;
		}
		else
		{
			// must specify valid chip type if using custom chipID
			int dt_chip_type = DT_INST_PROP_OR(0, chip_type,0);
			if (check_chiptype(st_chip_type))				
				{
					data->chip_type = dt_chip_type;
				}
			else
				{
					LOG_ERR("unknown chip type  %d", chip_type);
					return 0;
				}

		}

	}
	else
		switch(data->chip_id) {
			case FT8xx_CHIP_ID_FT800:
				{
				uint32_t id = ft8xx_rd32(FT800_REG_ID);
				data->chip_id = data->chip_type;

				return (id & 0xff) == FT8XX_EXPECTED_ID;					
				}

			case FT8xx_CHIP_ID_FT810:
			case FT8xx_CHIP_ID_FT811:
			case FT8xx_CHIP_ID_FT812:
			case FT8xx_CHIP_ID_FT813:
				{
				uint32_t id = ft8xx_rd32(FT81x_REG_ID);
				data->chip_id = data->chip_type;

				return (id & 0xff) == FT8XX_EXPECTED_ID;					
				}
		}
    }
	return 0;
}

static int identify_chip(void)
{
	uint32_t id = ft8xx_rd32(FT800_CHIP_ID);

	return id;
}

static void setup_chip(struct ft8xx_data *data)
{
  // assign register maps
		switch(data->chip_type) {
			case FT8xx_CHIP_ID_FT800:
				{
					data->memory_map = 	ft800_memory_map;		
					data->register_map = ft800_register_address_map;					
				}

			case FT8xx_CHIP_ID_FT810:
			case FT8xx_CHIP_ID_FT811:
			case FT8xx_CHIP_ID_FT812:
			case FT8xx_CHIP_ID_FT813:
				{
					data->memory_map = 	ft81x_memory_map;		
					data->register_map = ft81x_register_address_map;					
				}
		}
}

static int ft8xx_init(const struct device *dev)
{
	int ret;
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;


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

	data->chipid = identify_chip(void);

	if (!verify_chip(data)) {
		LOG_ERR("FT8xx chip not recognized");
		return -ENODEV;
	}

	setup_chip(data)





	/* Disable LCD */
	ft8xx_wr8(data->register_map->REG_GPIO, 0);
	ft8xx_wr8(data->register_map->REG_PCLK, 0);

	/* Configure LCD */
	ft8xx_wr16(data->register_map->REG_HSIZE, config->hsize);
	ft8xx_wr16(data->register_map->REG_HCYCLE, config->hcycle);
	ft8xx_wr16(data->register_map->REG_HOFFSET, config->hoffset);
	ft8xx_wr16(data->register_map->REG_HSYNC0, config->hsync0);
	ft8xx_wr16(data->register_map->REG_HSYNC1, config->hsync1);
	ft8xx_wr16(data->register_map->REG_VSIZE, config->vsize);
	ft8xx_wr16(data->register_map->REG_VCYCLE, config->vcycle);
	ft8xx_wr16(data->register_map->REG_VOFFSET, config->voffset);
	ft8xx_wr16(data->register_map->REG_VSYNC0, config->vsync0);
	ft8xx_wr16(data->register_map->REG_VSYNC1, config->vsync1);
	ft8xx_wr8(data->register_map->REG_SWIZZLE, config->swizzle);
	ft8xx_wr8(data->register_map->REG_PCLK_POL, config->pclk_pol);
	ft8xx_wr8(data->register_map->REG_CSPREAD, config->cspread);

	/* Display initial screen */

	/* Set the initial color */
	ft8xx_wr32(data->memory_map->REG_DL + 0, FT8XX_CLEAR_COLOR_RGB(0, 0x80, 0));
	/* Clear to the initial color */
	ft8xx_wr32(data->memory_map->REG_DL + 4, FT8XX_CLEAR(1, 1, 1));
	/* End the display list */
	ft8xx_wr32(data->memory_map->REG_DL + 8, FT8XX_DISPLAY());
	ft8xx_wr8(data->register_map->REG_DLSWAP, FT8XX_DLSWAP_FRAME);

	/* Enable LCD */

	/* Enable display bit */
	ft8xx_wr8(data->register_map->REG_GPIO_DIR, 0x80);
	ft8xx_wr8(data->register_map->REG_GPIO, 0x80);
	/* Enable backlight */
	ft8xx_wr16(data->register_map->REG_PWM_HZ, 0x00FA);
	ft8xx_wr8(data->register_map->REG_PWM_DUTY, 0x10);
	/* Enable LCD signals */
	ft8xx_wr8(data->register_map->REG_PCLK, config->pclk);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, ft8xx_init, NULL, &ft8xx_data, &ft8xx_config,
		      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ft8xx_api);

int ft8xx_get_touch_tag(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;

	/* Read FT800_REG_INT_FLAGS to clear IRQ */
	(void)ft8xx_rd8(data->register_map->REG_INT_FLAGS);

	return (int)ft8xx_rd8(data->register_map->REG_TOUCH_TAG);
}





void ft8xx_drv_irq_triggered(const struct device *dev, struct gpio_callback *cb,
			      uint32_t pins)
{
	const struct ft8xx_data *data = dev->data;

	if (data->irq_callback != NULL) {
		data->irq_callback();
	}
}

void ft8xx_register_int(const struct device *dev, ft8xx_int_callback callback)
{
	const struct ft8xx_data *data = dev->data;

	if (ft8xx_data->irq_callback != NULL) {
		return;
	}

	data->irq_callback = callback;
	ft8xx_wr8(data->register_map->REG_INT_MASK, 0x04);
	ft8xx_wr8(data->register_map->REG_INT_EN, 0x01);
}

void ft8xx_calibrate(const struct device *dev, struct ft8xx_touch_transform *trform)
{
	const struct ft8xx_data *data = dev->data;
	uint32_t result = 0;

	do {
		ft8xx_copro_cmd_dlstart();
		ft8xx_copro_cmd(FT8XX_CLEAR_COLOR_RGB(0x00, 0x00, 0x00));
		ft8xx_copro_cmd(FT8XX_CLEAR(1, 1, 1));
		ft8xx_copro_cmd_calibrate(&result);
	} while (result == 0);

	trform->a = ft8xx_rd32(data->register_map->REG_TOUCH_TRANSFORM_A);
	trform->b = ft8xx_rd32(data->register_map->REG_TOUCH_TRANSFORM_B);
	trform->c = ft8xx_rd32(data->register_map->REG_TOUCH_TRANSFORM_C);
	trform->d = ft8xx_rd32(data->register_map->REG_TOUCH_TRANSFORM_D);
	trform->e = ft8xx_rd32(data->register_map->REG_TOUCH_TRANSFORM_E);
	trform->f = ft8xx_rd32(data->register_map->REG_TOUCH_TRANSFORM_F);
}

void ft8xx_touch_transform_set(const struct device *dev, const struct ft8xx_touch_transform *trform)
{
	const struct ft8xx_data *data = dev->data;

	ft8xx_wr32(data->register_map->REG_TOUCH_TRANSFORM_A, trform->a);
	ft8xx_wr32(data->register_map->REG_TOUCH_TRANSFORM_B, trform->b);
	ft8xx_wr32(data->register_map->REG_TOUCH_TRANSFORM_C, trform->c);
	ft8xx_wr32(data->register_map->REG_TOUCH_TRANSFORM_D, trform->d);
	ft8xx_wr32(data->register_map->REG_TOUCH_TRANSFORM_E, trform->e);
	ft8xx_wr32(data->register_map->REG_TOUCH_TRANSFORM_F, trform->f);
}

