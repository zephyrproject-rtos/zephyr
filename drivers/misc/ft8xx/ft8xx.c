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
#include <zephyr/drivers/misc/ft8xx/ft8xx_audio.h>


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
	
	uint32_t chip_id;
	uint32_t chip_type;


	union ft8xx_bus bus;
	const struct ft8xx_bus_io *bus_io;

	struct gpio_dt_spec irq_gpio;
};

struct ft8xx_data {
//	const struct ft8xx_config *config;
	ft8xx_int_callback irq_callback;
	uint chip_id;
	uint chip_type;
	struct ft8xx_memory_map_t *memory_map;
	struct ft8xx_register_address_map_t *register_map;

	struct ft8xx_touch_transform *ctrform;

	static uint16_t reg_cmd_read;
	static uint16_t reg_cmd_write;
};

static struct ft8xx_data ft8xx_data = {
//	.config = &ft8xx_config,
	.irq_callback = NULL,
	.chip_id = 0,
	.chip_type = 0,
	.memory_map = NULL,
	.register_map = NULL,
	.ctrform = NULL,
};

static struct ft8xx_api {
   
    ft8xx_calibrate 		= ft8xx_calibrate, 
    ft8xx_get_transform 	= ft8xx_touch_transform_get,
    ft8xx_set_transform 	= ft8xx_touch_transform_set,
	ft8xx_get_touch_tag		= ft8xx_get_touch_tag,
	ft8xx_register_int		= ft8xx_register_int,
    ft8xx_host_command		= ft8xx_host_command,
    ft8xx_command			= ft8xx_command,

	ft8xx_audio_load			= ,
	ft8xx_audio_play			= ,
	ft8xx_audio_get_status		= ,
	ft8xx_audio_stop			= ,

	ft8xx_audio_synth_start			= ,
	ft8xx_audio_synth_get_status	= ,
	ft8xx_audio_synth_stop 			= ,

	ft8xx_copro_cmd_dlstart			= , 
	ft8xx_copro_cmd_swap 			= ,
	ft8xx_copro_cmd_coldstart		= ,
	ft8xx_copro_cmd_interrupt		= ,
	ft8xx_copro_cmd_append			= ,
	ft8xx_copro_cmd_regread			= ,
	ft8xx_copro_cmd_memwrite		= ,
	ft8xx_copro_cmd_inflate			= ,
	ft8xx_copro_cmd_loadimage		= ,
	ft8xx_copro_cmd_memcrc			= ,
	ft8xx_copro_cmd_memzero			= ,
	ft8xx_copro_cmd_memset			= ,
	ft8xx_copro_cmd_memcpy			= ,
	ft8xx_copro_cmd_button			= ,
	ft8xx_copro_cmd_clock			= ,
	ft8xx_copro_cmd_fgcolor			= ,
	ft8xx_copro_cmd_bgcolor			= ,
	ft8xx_copro_cmd_gradcolor		= ,
	ft8xx_copro_cmd_gauge			= ,
	ft8xx_copro_cmd_gradient		= ,
	ft8xx_copro_cmd_keys			= ,
	ft8xx_copro_cmd_progress		= ,
	ft8xx_copro_cmd_scrollbar		= ,
	ft8xx_copro_cmd_slider			= ,
	ft8xx_copro_cmd_dial			= ,
	ft8xx_copro_cmd_toggle			= ,
	ft8xx_copro_cmd_text			= ,
	ft8xx_copro_cmd_number			= ,
	ft8xx_copro_cmd_setmatrix		= ,
	ft8xx_copro_cmd_getmatrix		= ,
	ft8xx_copro_cmd_getptr			= ,
	ft8xx_copro_cmd_getprops		= ,
	ft8xx_copro_cmd_scale			= ,
	ft8xx_copro_cmd_rotate			= ,
	ft8xx_copro_cmd_translate		= ,
	ft8xx_copro_cmd_calibrate		= ,
	ft8xx_copro_cmd_spinner			= ,
	ft8xx_copro_cmd_screensaver		= ,
	ft8xx_copro_cmd_sketch			= ,
	ft8xx_copro_cmd_stop			= ,
	ft8xx_copro_cmd_setfont			= ,
	ft8xx_copro_cmd_track			= ,
	ft8xx_copro_cmd_snapshot		= ,
	ft8xx_copro_cmd_logo			= ,

};






static void host_command(const struct device *dev, uint8_t cmd)
{
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;
	
	int err;

	err = ft8xx_drv_command(bus, cmd);
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

static bool verify_chip(const struct device *dev)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
	if (config->chip_id <> 0)
	{
		// check custom chipID matches that in device memory
		if (chipid != data->chip_id) 
		{
			return 0;
		}
		else
		{
			// must specify valid chip type if using custom chipID
			if (check_chiptype(config->chip_type))				
				{
					data->chip_type = config->chip_type;
				}
			else
				{
					LOG_ERR("unknown chip type  %d", chip_type);
					return 0;
				}

		}

	}

	switch(data->chip_id) {
		case FT8xx_CHIP_ID_FT800:
			{
			uint32_t id = ft8xx_rd32(bus,FT800_REG_ID);
			data->chip_id = data->chip_type;

			return (id & 0xff) == FT8XX_EXPECTED_ID;					
			}

		case FT8xx_CHIP_ID_FT810:
		case FT8xx_CHIP_ID_FT811:
		case FT8xx_CHIP_ID_FT812:
		case FT8xx_CHIP_ID_FT813:
			{
			uint32_t id = ft8xx_rd32(bus,FT81x_REG_ID);
			data->chip_id = data->chip_type;

			return (id & 0xff) == FT8XX_EXPECTED_ID;					
			}
	}
    
	return 0;
}

static int identify_chip(dev)
{
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	uint32_t id = ft8xx_rd32(bus,FT800_CHIP_ID);

	return id;
}

#ifdef CONFIG_SPI_EXTENDED_MODES
static void set_spi_mode(const struct device *dev)
{
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;	
	const struct spi_dt_spec *spi = config->spi;
	const struct spi_config spi_config = spi->config;
	
	if (spi_config.operation == )
	{
		// try quad spi mode
		 

	}
	else if (spi_config.operation == )
	{
		// try dual spi mode 


	}



}
#endif



static void setup_chip(const struct device *dev)
{
  // assign register maps
		switch(data->chip_type) {
			case FT8xx_CHIP_ID_FT800:
				{
					data->memory_map = 	&ft800_memory_map;		
					data->register_map = &ft800_register_address_map;					
				}

			case FT8xx_CHIP_ID_FT810:
			case FT8xx_CHIP_ID_FT811:
			case FT8xx_CHIP_ID_FT812:
			case FT8xx_CHIP_ID_FT813:
				{
					data->memory_map = 	&ft81x_memory_map;		
					data->register_map = &ft81x_register_address_map;					
				}
		}

}

static int ft8xx_init(const struct device *dev)
{
	int ret;
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	ret = ft8xx_drv_init(dev);
	if (ret < 0) {
		LOG_ERR("FT8xx driver initialization failed with %d", ret);
		return ret;
	}

	/* Reset display controller */
	host_command(bus, CORERST);
	host_command(bus, ACTIVE);
	wait();
	host_command(bus, CLKEXT);
	host_command(bus, CLK48M);
	wait();

	host_command(bus, CORERST);
	host_command(bus, ACTIVE);
	wait();
	host_command(bus, CLKEXT);
	host_command(bus, CLK48M);
	wait();

	data->chipid = identify_chip(dev);

	if (!verify_chip(dev)) {
		LOG_ERR("FT8xx chip not recognized");
		return -ENODEV;
	}

	setup_chip(dev);

#ifdef CONFIG_SPI_EXTENDED_MODES
	set_spi_mode(dev);
#endif

	/* Disable LCD */
	ft8xx_wr8(bus, data->register_map->REG_GPIO, 0);
	ft8xx_wr8(bus, data->register_map->REG_PCLK, 0);

	/* Configure LCD */
	ft8xx_wr16(bus, data->register_map->REG_HSIZE, config->hsize);
	ft8xx_wr16(bus, data->register_map->REG_HCYCLE, config->hcycle);
	ft8xx_wr16(bus, data->register_map->REG_HOFFSET, config->hoffset);
	ft8xx_wr16(bus, data->register_map->REG_HSYNC0, config->hsync0);
	ft8xx_wr16(bus, data->register_map->REG_HSYNC1, config->hsync1);
	ft8xx_wr16(bus, data->register_map->REG_VSIZE, config->vsize);
	ft8xx_wr16(bus, data->register_map->REG_VCYCLE, config->vcycle);
	ft8xx_wr16(bus, data->register_map->REG_VOFFSET, config->voffset);
	ft8xx_wr16(bus, data->register_map->REG_VSYNC0, config->vsync0);
	ft8xx_wr16(bus, data->register_map->REG_VSYNC1, config->vsync1);
	ft8xx_wr8(bus, data->register_map->REG_SWIZZLE, config->swizzle);
	ft8xx_wr8(bus, data->register_map->REG_PCLK_POL, config->pclk_pol);
	ft8xx_wr8(bus, data->register_map->REG_CSPREAD, config->cspread);

	/* Display initial screen */

	/* Set the initial color */
	ft8xx_wr32(bus, data->memory_map->REG_DL + 0, FT8XX_CLEAR_COLOR_RGB(0, 0x80, 0));
	/* Clear to the initial color */
	ft8xx_wr32(bus, data->memory_map->REG_DL + 4, FT8XX_CLEAR(1, 1, 1));
	/* End the display list */
	ft8xx_wr32(bus, data->memory_map->REG_DL + 8, FT8XX_DISPLAY());
	ft8xx_wr8(bus, data->register_map->REG_DLSWAP, FT8XX_DLSWAP_FRAME);

	/* Enable LCD */

	/* Enable display bit */
	ft8xx_wr8(bus, data->register_map->REG_GPIO_DIR, 0x80);
	ft8xx_wr8(bus, data->register_map->REG_GPIO, 0x80);
	/* Enable backlight */
	ft8xx_wr16(bus, data->register_map->REG_PWM_HZ, 0x00FA);
	ft8xx_wr8(bus, data->register_map->REG_PWM_DUTY, 0x10);
	/* Enable LCD signals */
	ft8xx_wr8(bus, data->register_map->REG_PCLK, config->pclk);

	return 0;
}

int ft8xx_get_touch_tag(const struct device *dev)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	/* Read FT800_REG_INT_FLAGS to clear IRQ */
	(void)ft8xx_rd8(bus, data->register_map->REG_INT_FLAGS);

	return (int)ft8xx_rd8(bus, data->register_map->REG_TOUCH_TAG);
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
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	if (ft8xx_data->irq_callback != NULL) {
		return;
	}

	data->irq_callback = callback;
	ft8xx_rd8(bus, data->register_map->REG_INT_MASK, 0x04);
	ft8xx_rd8(bus, data->register_map->REG_INT_EN, 0x01);
}

void ft8xx_calibrate(const struct device *dev, struct ft8xx_touch_transform *trform)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	uint32_t result = 0;

	do {
		ft8xx_copro_cmd_dlstart();
		ft8xx_copro_cmd(dev, FT8XX_CLEAR_COLOR_RGB(0x00, 0x00, 0x00));
		ft8xx_copro_cmd(dev, FT8XX_CLEAR(1, 1, 1));
		ft8xx_copro_cmd_calibrate(&result);
	} while (result == 0);

	ft8xx_touch_transform_get(dev,trform);
}


void ft8xx_touch_transform_get(const struct device *dev, struct ft8xx_touch_transform *trform)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	uint32_t result = 0;

	trform->a = ft8xx_rd32(bus, data->register_map->REG_TOUCH_TRANSFORM_A);
	trform->b = ft8xx_rd32(bus, data->register_map->REG_TOUCH_TRANSFORM_B);
	trform->c = ft8xx_rd32(bus, data->register_map->REG_TOUCH_TRANSFORM_C);
	trform->d = ft8xx_rd32(bus, data->register_map->REG_TOUCH_TRANSFORM_D);
	trform->e = ft8xx_rd32(bus, data->register_map->REG_TOUCH_TRANSFORM_E);
	trform->f = ft8xx_rd32(bus, data->register_map->REG_TOUCH_TRANSFORM_F);
}

void ft8xx_touch_transform_set(const struct device *dev, const struct ft8xx_touch_transform *trform)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	ft8xx_wr32(bus, data->register_map->REG_TOUCH_TRANSFORM_A, trform->a);
	ft8xx_wr32(bus, data->register_map->REG_TOUCH_TRANSFORM_B, trform->b);
	ft8xx_wr32(bus, data->register_map->REG_TOUCH_TRANSFORM_C, trform->c);
	ft8xx_wr32(bus, data->register_map->REG_TOUCH_TRANSFORM_D, trform->d);
	ft8xx_wr32(bus, data->register_map->REG_TOUCH_TRANSFORM_E, trform->e);
	ft8xx_wr32(bus, data->register_map->REG_TOUCH_TRANSFORM_F, trform->f);
}

#define FT8XX_CONFIG(inst)				
	{						
		/* Initializes struct ft8xx_config for an instance on a SPI bus. */
		.bus.spi = SPI_DT_SPEC_INST_GET(	
			inst, FT8XX_SPI_OPERATION, 0),	
		.bus_io = &ft8xx_bus_io_spi,		

		/* Initializes struct ft8xx_config for an GPIO interupt. */
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios);	
	
		/* Initializes struct ft8xx_config for display properties. */
		.pclk     = DT_INST_PROP(inst, pclk),
		.pclk_pol = DT_INST_PROP(inst, pclk_pol),
		.cspread  = DT_INST_PROP(inst, cspread),
		.swizzle  = DT_INST_PROP(inst, swizzle),
		.vsize    = DT_INST_PROP(inst, vsize),
		.voffset  = DT_INST_PROP(inst, voffset),
		.vcycle   = DT_INST_PROP(inst, vcycle),
		.vsync0   = DT_INST_PROP(inst, vsync0),
		.vsync1   = DT_INST_PROP(inst, vsync1),
		.hsize    = DT_INST_PROP(inst, hsize),
		.hoffset  = DT_INST_PROP(inst, hoffset),
		.hcycle   = DT_INST_PROP(inst, hcycle),
		.hsync0   = DT_INST_PROP(inst, hsync0),
		.hsync1   = DT_INST_PROP(inst, hsync1),

		.chip_id 	= DT_INST_PROP_OR(inst, chipid,0),
		.chip_type	= DT_INST_PROP_OR(inst, chip_type,0),

	}

#define FT8XX_DEFINE(inst)						
	static struct ft8xx_data ft8xx_data_##inst;			
	static const struct ft8xx_config ft8xx_config_##inst =	
			    (FT8XX_CONFIG(inst))

	DEVICE_DT_INST_DEFINE(inst, 
					ft8xx_init,
					NULL,
					&ft8xx_data,
					&ft8xx_config,
					APPLICATION,
					CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
					&ft8xx_api);

DT_INST_FOREACH_STATUS_OKAY(FT8XX_DEFINE)