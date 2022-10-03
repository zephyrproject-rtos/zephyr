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

struct ft8xx_memory_map_t
{
	uintptr_t RAM_G,
	uintptr_t ROM_CHIPID,    
	uintptr_t ROM_FONT,      
	uintptr_t ROM_FONT_ADDR, 
	uintptr_t RAM_DL,        
	uintptr_t RAM_PAL,      // not present on ft81x 
	uintptr_t REG,           
	uintptr_t RAM_CMD,        

}

struct ft8xx_register_address_map_t
{  
	uintptr_t REG_ID         
	uintptr_t REG_FRAMES     
	uintptr_t REG_CLOCK      
	uintptr_t REG_FREQUENCY  
	uintptr_t REG_RENDERMODE 
	uintptr_t REG_SNAPY      
	uintptr_t REG_SNAPSHOT   
	uintptr_t REG_CPURESET   
	uintptr_t REG_TAP_CRC    
	uintptr_t REG_TAP_MASK   
	uintptr_t REG_HCYCLE     
	uintptr_t REG_HOFFSET    
	uintptr_t REG_HSIZE      
	uintptr_t REG_HSYNC0     
	uintptr_t REG_HSYNC1     
	uintptr_t REG_VCYCLE     
	uintptr_t REG_VOFFSET    
	uintptr_t REG_VSIZE      
	uintptr_t REG_VSYNC0     
	uintptr_t REG_VSYNC1     
	uintptr_t REG_DLSWAP     
	uintptr_t REG_ROTATE     
	uintptr_t REG_OUTBITS    
	uintptr_t REG_DITHER     
	uintptr_t REG_SWIZZLE    
	uintptr_t REG_CSPREAD    
	uintptr_t REG_PCLK_POL   
	uintptr_t REG_PCLK       
	uintptr_t REG_TAG_X      
	uintptr_t REG_TAG_Y      
	uintptr_t REG_TAG        
	uintptr_t REG_VOL_PB     
	uintptr_t REG_VOL_SOUND  
	uintptr_t REG_SOUND      
	uintptr_t REG_PLAY       
	uintptr_t REG_GPIO_DIR   
	uintptr_t REG_GPIO       
	uintptr_t REG_GPIOX_DIR  
	uintptr_t REG_GPIOX      
	uintptr_t REG_INT_FLAGS  
	uintptr_t REG_INT_EN     
	uintptr_t REG_INT_MASK   
	uintptr_t REG_PLAYBACK_START 
	uintptr_t REG_PLAYBACK_LENGTH 
	uintptr_t REG_PLAYBACK_READPTR 
	uintptr_t REG_PLAYBACK_FREQ 
	uintptr_t REG_PLAYBACK_FORMAT 
	uintptr_t REG_PLAYBACK_LOOP 
	uintptr_t REG_PLAYBACK_PLAY 
	uintptr_t REG_PWM_HZ     
	uintptr_t REG_PWM_DUTY   
	uintptr_t REG_MACRO_0    
	uintptr_t REG_MACRO_1    
	uintptr_t REG_CMD_READ   
	uintptr_t REG_CMD_WRITE  
	uintptr_t REG_CMD_DL     
	uintptr_t REG_TOUCH_MODE 
	uintptr_t REG_TOUCH_ADC_MODE 
	uintptr_t REG_TOUCH_CHARGE 
	uintptr_t REG_TOUCH_SETTLE 
	uintptr_t REG_TOUCH_OVERSAMPLE 
	uintptr_t REG_TOUCH_RZTHRESH 
	uintptr_t REG_TOUCH_RAW_XY 
	uintptr_t REG_TOUCH_RZ   
	uintptr_t REG_TOUCH_SCREEN_XY 
	uintptr_t REG_TOUCH_TAG_XY 
	uintptr_t REG_TOUCH_TAG  
	uintptr_t REG_TOUCH_TRANSFORM_A 
	uintptr_t REG_TOUCH_TRANSFORM_B 
	uintptr_t REG_TOUCH_TRANSFORM_C 
	uintptr_t REG_TOUCH_TRANSFORM_D 
	uintptr_t REG_TOUCH_TRANSFORM_E 
	uintptr_t REG_TOUCH_TRANSFORM_F 
	uintptr_t REG_TOUCH_CONFIG
	uintptr_t REG_CTOUCH_TOUCH4_X
	uintptr_t REG_BIST_EN
	uintptr_t REG_TRIM
	uintptr_t REG_ANA_COMP 
	uintptr_t REG_SPI_WIDTH  
	uintptr_t REG_TOUCH_DIRECT_XY 
	uintptr_t REG_TOUCH_DIRECT_Z1Z2
	uintptr_t REG_DATESTAMP
	uintptr_t REG_CMDB_SPACE 
	uintptr_t REG_CMDB_WRITE 
	uintptr_t REG_TRACKER    
	uintptr_t REG_TRACKER1   
	uintptr_t REG_TRACKER2   
	uintptr_t REG_TRACKER3   
	uintptr_t REG_TRACKER4   
	uintptr_t REG_MEDIAFIFO_READ 
	uintptr_t REG_MEDIAFIFO_WRITE 
	     
}

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

static bool setup_chip(struct ft8xx_data *data)
{






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
