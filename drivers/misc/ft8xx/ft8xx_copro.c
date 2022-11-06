/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/ft8xx/ft8xx_copro.h>

#include <stdint.h>
#include <string.h>

#include <zephyr/drivers/misc/ft8xx/ft8xx_common.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_memory.h>
#include "ft8xx_drv.h"

#define FT800_RAM_CMD_SIZE 4096UL

enum {
	CMD_DLSTART		= 0xffffff00,
	CMD_SWAP		= 0xffffff01,
	CMD_INTERUPT	= 0xffffff02,
	CMD_TEXT		= 0xffffff0c,
	CMD_CALIBRATE	= 0xffffff15,
	CMD_REGREAD		= 0xFFFFFF19,
	CMD_MEMWRITE	= 0xFFFFFF1A,
	CMD_APPEND		= 0xFFFFFF1E,
	CMD_INFLATE		= 0xFFFFFF22,
	CMD_LOADIMAGE 	= 0xFFFFFF24,	
	CMD_NUMBER		= 0xffffff2e,
	CMD_COLDSTART	= 0xffffff32,
	CMD_MEDIAFIFO 	= 0xFFFFFF39,
	CMD_PLAYVIDEO 	= 0xFFFFFF3A,	
	CMD_INFLATE2	= 0xFFFFFF50,
	CMD_APILEVEL	= 0xFFFFFF63,

} 



static uint16_t ram_cmd_fullness(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;


	return (data->reg_cmd_write - data->reg_cmd_read) % FT800_RAM_CMD_SIZE;
}

static uint16_t ram_cmd_freespace(const struct device *dev)
{

	return (FT800_RAM_CMD_SIZE - 4UL) - ram_cmd_fullness(dev);
}

static void refresh_reg_cmd_read(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;	
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	data->reg_cmd_read = ft8xx_rd32(bus, data->register_map->REG_CMD_READ);
}

static void flush_reg_cmd_write(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	ft8xx_wr32(bus, data->register_map->REG_CMD_WRITE, data->reg_cmd_write);
}

static void increase_reg_cmd_write(const struct device *dev, uint16_t value)
{
	data->reg_cmd_write = (data->reg_cmd_write + value) % FT800_RAM_CMD_SIZE;
}

void ft8xx_copro_cmd(const struct device *dev, uint32_t cmd)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	while (ram_cmd_freespace(dev) < sizeof(cmd)) {
		refresh_reg_cmd_read(dev);
	}

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, cmd);
	increase_reg_cmd_write(dev, sizeof(cmd));

	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_uint(const struct device *dev, uint32_t cmd, uint32_t param1)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(CMD_TEXT) +
				   sizeof(param);

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, cmd);
	increase_reg_cmd_write(dev, sizeof(cmd));

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, param);
	increase_reg_cmd_write(sizeof(x));	

	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_uint_uint(const struct device *dev, uint32_t cmd, uint32_t param, uint32_t param2)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(CMD_TEXT) +
				   sizeof(param) +
				   sizeof(param2);

	// size check?

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, cmd);
	increase_reg_cmd_write(dev, sizeof(cmd));

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, param);
	increase_reg_cmd_write(sizeof(param));	

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, param2);
	increase_reg_cmd_write(sizeof(param2));	


	flush_reg_cmd_write(dev);
}


int ft8xx_copro_cmd_apilevel(const struct device *dev, uint32_t level)
{
	//bt817/8 only
	switch(data->chip_id) {
		case FT8xx_CHIP_ID_BT817:
		case FT8xx_CHIP_ID_BT818:
			{
				ft8xx_copro_cmd_uint(dev, CMD_APILEVEL, level);
				return 0;
			}
	}

	return -ENOTSUP;
}

void ft8xx_copro_cmd_dlstart(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_DLSTART);
}

void ft8xx_copro_cmd_interrupt(const struct device *dev, uint32_t ms)
{
	ft8xx_copro_cmd_uint(dev, CMD_INTERUPT, ms);
}

void ft8xx_copro_cmd_coldstart(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_COLDSTART);
}

void ft8xx_copro_cmd_swap(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_SWAP);
}

int ft8xx_copro_cmd_append(const struct device *dev, uint32_t ptr, 
			uint32_t num )
{
	const struct ft8xx_data *data = dev->data;


	if (!(num % 4) 
	{
		if 	!((ptr+num) > data->memory_map->RAM_G_END))
		{
			ft8xx_copro_cmd_uint_uint(dev, CMD_APPEND, ptr, num);
		}
		else
		{
			LOG_ERR("Outside memory Range");
			return -ENOTSUP;			
		}
	}
	else
	{	
		LOG_ERR("Appended bytes not multiple of 4");
		return -ENOTSUP;
	}
}

void ft8xx_copro_cmd_regread(const struct device *dev, uint32_t result, 
			uint32_t num )
{
	ft8xx_copro_cmd_uint_uint(dev, CMD_REGREAD, ptr, result);
}

void ft8xx_copro_cmd_memwrite(const struct device *dev, uint32_t ptr, uint32_t num,
			uint8_t *src )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t padding_bytes = (4 - (num % 4)) % 4
	const uint16_t cmd_size = sizeof(CMD_MEMWRITE) +
				   sizeof(ptr) +
				   num +
				   padding_bytes;
	
	
	
	if 	(cmd_size > FT800_RAM_CMD_SIZE)
	{
		LOG_ERR("Command Too Large for Command Buffer");
		return -ENOTSUP;			
	}
	else if (ptr+num > data->memory_map->RAM_G_END))
	{
		LOG_ERR("Data Target Outside memory Range");
		return -ENOTSUP;			
	}
	else
	{
		while (ram_cmd_freespace() < cmd_size) {
			refresh_reg_cmd_read();
		}

		ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, CMD_MEMWRITE);
		increase_reg_cmd_write(sizeof(CMD_MEMWRITE));

		ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, ptr);
		increase_reg_cmd_write(sizeof(ptr));

		ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, num);
		increase_reg_cmd_write(sizeof(num));

		(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, src, num);

		uint8_t pad_bytes[];
		for (i = 0; i < padding_bytes; i++) {
			pad_bytes[i] = 0x0;
		}

		(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, pad_bytes, padding_bytes);


		increase_reg_cmd_write(dev, num + padding_bytes);

		flush_reg_cmd_write(dev);
	}

}

void ft8xx_copro_cmd_inflate(const struct device *dev, uint32_t ptr,
			uint32_t *src, uint32_t len )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t padding_bytes = (4 - (len % 4)) % 4
	const uint16_t cmd_size = sizeof(CMD_INFLATE) +
				   sizeof(ptr) +
				   len +
				   padding_bytes;
	
	if 	(cmd_size > FT800_RAM_CMD_SIZE)
	{
		LOG_ERR("Command Too Large for Command Buffer");
		return -ENOTSUP;			
	}
	else
	{
		while (ram_cmd_freespace() < cmd_size) {
			refresh_reg_cmd_read();
		}

		ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, CMD_INFLATE);
		increase_reg_cmd_write(sizeof(CMD_INFLATE));

		ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, ptr);
		increase_reg_cmd_write(sizeof(ptr));

		(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, src, len);

		uint8_t pad_bytes[];
		for (i = 0; i < padding_bytes; i++) {
			pad_bytes[i] = 0x0;
		}

		(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, pad_bytes, padding_bytes);
		increase_reg_cmd_write(dev, num + padding_bytes);

		flush_reg_cmd_write(dev);
	}

}

int ft8xx_copro_cmd_inflate2(const struct device *dev, uint32_t ptr,
			uint32_t options, uint32_t *src, uint32_t len )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t padding_bytes = (4 - (len % 4)) % 4
	const uint16_t cmd_size = sizeof(CMD_INFLATE) +
				   sizeof(ptr) +
				   sizeof(options)
	const uint16_t data_size = len +
				   padding_bytes;


	const bool data_external = (((options & FT81x_OPT_MEDIAFIFO) == FT81x_OPT_MEDIAFIFO) | 
				((options & FT81x_OPT_FLASH) == FT81x_OPT_FLASH)) ;

	//bt81x only
	switch(data->chip_id) {
		case FT8xx_CHIP_ID_BT815:
		case FT8xx_CHIP_ID_BT816:
		case FT8xx_CHIP_ID_BT817:
		case FT8xx_CHIP_ID_BT818:
			{
				break;
			}
		default: 
		{
			return -ENOTSUP;
		}
	}

	if ((!data_external) && (src != 0) && (len == 0) )
	{
				LOG_ERR("Data not included");
				return -ENOTSUP;
	}

	if 	(cmd_size+data_size > FT800_RAM_CMD_SIZE)
	{
		LOG_ERR("Command Too Large for Command Buffer");
		return -ENOTSUP;			
	}
	else
	{
		if (data_external)
		{
			ft8xx_copro_cmd_uint_uint(dev, CMD_INFLATE, ptr, options);
		}
		else
		{
			while (ram_cmd_freespace() < cmd_size+data_size) {
				refresh_reg_cmd_read();
			}

			ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, CMD_INFLATE);
			increase_reg_cmd_write(sizeof(CMD_INFLATE));

			ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, ptr);
			increase_reg_cmd_write(sizeof(ptr));

			ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, options);
			increase_reg_cmd_write(sizeof(options));

			(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, src, len);

			uint8_t pad_bytes[];
			for (i = 0; i < padding_bytes; i++) {
				pad_bytes[i] = 0x0;
			}

			(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, pad_bytes, padding_bytes);

			increase_reg_cmd_write(dev, num + padding_bytes);

			flush_reg_cmd_write(dev);
		}
	}
	return 0;
}

int ft8xx_copro_cmd_loadimage(const struct device *dev, uint32_t ptr,
				uint32_t options, uint32_t *src, uint32_t len )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t padding_bytes = (4 - (len % 4)) % 4
	const uint16_t cmd_size = sizeof(CMD_LOADIMAGE) +
				   sizeof(ptr) +
				   sizeof(options)
	const uint16_t data_size = len +
				   padding_bytes;

	// data not included in command
	const bool data_external = (((options & FT81x_OPT_MEDIAFIFO) == FT81x_OPT_MEDIAFIFO) | 
				((options & FT81x_OPT_FLASH) == FT81x_OPT_FLASH)) ;

	if ((!data_external) && (src != 0) && (len == 0) )
	{
				LOG_ERR("Data not included");
				return -ENOTSUP;
	}

	if ((options & FT81x_OPT_FLASH) == FT81x_OPT_FLASH)) 
	{
		//bt81x only
		switch(data->chip_id) {
			case FT8xx_CHIP_ID_BT815:
			case FT8xx_CHIP_ID_BT816:
			case FT8xx_CHIP_ID_BT817:
			case FT8xx_CHIP_ID_BT818:
				{
					break;
				}
			default: 
			{
				LOG_ERR("FLASH RAM Not supported on this device");
				return -ENOTSUP;
			}
		}
	}

	if ((options & OPT_MEDIAFIFO ) == OPT_MEDIAFIFO )) 
	{
		//ft81x
		switch(data->chip_id) {
			case FT8xx_CHIP_ID_FT811:
			case FT8xx_CHIP_ID_FT812:
			case FT8xx_CHIP_ID_FT813:
			case FT8xx_CHIP_ID_BT815:
			case FT8xx_CHIP_ID_BT816:
			case FT8xx_CHIP_ID_BT817:
			case FT8xx_CHIP_ID_BT818:
				{
					break;
				}
			default: 
			{
				LOG_ERR("MEDIA FIFO Not supported on this device");
				return -ENOTSUP;
			}
		}
	}

	if 	(cmd_size+data_size > FT800_RAM_CMD_SIZE)
	{
		LOG_ERR("Command Too Large for Command Buffer");
		return -ENOTSUP;			
	}
	else
	{
		if (data_external)
		{
			ft8xx_copro_cmd_uint_uint(dev, CMD_LOADIMAGE, ptr, options);
		}
		else
		{
			while (ram_cmd_freespace() < cmd_size+data_size) {
				refresh_reg_cmd_read();
			}

			ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, CMD_LOADIMAGE);
			increase_reg_cmd_write(sizeof(CMD_LOADIMAGE));

			ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, ptr);
			increase_reg_cmd_write(sizeof(ptr));

			ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, options);
			increase_reg_cmd_write(sizeof(options));

			(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, src, len);

			uint8_t pad_bytes[];
			for (i = 0; i < padding_bytes; i++) {
				pad_bytes[i] = 0x0;
			}

			(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, pad_bytes, padding_bytes);

			increase_reg_cmd_write(dev, num + padding_bytes);

			flush_reg_cmd_write(dev);
		}
	}

	return 0;
}

int ft8xx_copro_cmd_mediafifo(const struct device *dev, uint32_t ptr,
				uint32_t size )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;


	if ((options & OPT_MEDIAFIFO ) == OPT_MEDIAFIFO )) 
	{
		//ft81x
		switch(data->chip_id) {
			case FT8xx_CHIP_ID_FT811:
			case FT8xx_CHIP_ID_FT812:
			case FT8xx_CHIP_ID_FT813:
			case FT8xx_CHIP_ID_BT815:
			case FT8xx_CHIP_ID_BT816:
			case FT8xx_CHIP_ID_BT817:
			case FT8xx_CHIP_ID_BT818:
				{
					break;
				}
			default: 
			{
				LOG_ERR("MEDIA FIFO Not supported on this device");
				return -ENOTSUP;
			}
		}
	}
	if ((ptr % 4) <> 0)
	{
	{
		LOG_ERR("FIFO start must be 4 bytes aligned");
		return -ENOTSUP;
	}
	}
	if ((num % 4) <> 0) 
	{
		LOG_ERR("FIFO size not multiple of 4 bytes");
		return -ENOTSUP;
	}

	if 	((ptr+size) > data->memory_map->RAM_G_END))
	{
		LOG_ERR("Outside memory Range");
		return -ENOTSUP;			
	}
			
	ft8xx_copro_cmd_uint_uint(dev, CMD_MEDIAFIFO, ptr, num);
	
	return 0;
}





void ft8xx_copro_cmd_text(const struct device *dev, int16_t x,
			   int16_t y,
			   int16_t font,
			   uint16_t options,
			   const char *s)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t str_bytes = strlen(s) + 1;
	const uint16_t padding_bytes = (4 - (str_bytes % 4)) % 4;
	const uint16_t cmd_size = sizeof(CMD_TEXT) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(font) +
				   sizeof(options) +
				   str_bytes +
				   padding_bytes;

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, CMD_TEXT);
	increase_reg_cmd_write(sizeof(CMD_TEXT));

	ft8xx_wr16(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, font);
	increase_reg_cmd_write(sizeof(font));

	ft8xx_wr16(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, options);
	increase_reg_cmd_write(dev, sizeof(options));

	(void)ft8xx_drv_write(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, s, str_bytes);
	increase_reg_cmd_write(dev, str_bytes + padding_bytes);

	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_number(const struct device *dev, int16_t x,
			     int16_t y,
			     int16_t font,
			     uint16_t options,
			     int32_t n)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(CMD_NUMBER) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(font) +
				   sizeof(options) +
				   sizeof(n);

	while (ram_cmd_freespace(dev) < cmd_size) {
		refresh_reg_cmd_read(dev);
	}



	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, CMD_NUMBER);
	increase_reg_cmd_write(sizeof(CMD_NUMBER));

	ft8xx_wr16(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, font);
	increase_reg_cmd_write(sizeof(font));

	ft8xx_wr16(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, options);
	increase_reg_cmd_write(sizeof(options));

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, n);
	increase_reg_cmd_write(sizeof(n));

	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_calibrate(const struct device *dev, uint32_t *result)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(CMD_CALIBRATE) + sizeof(uint32_t);
	uint32_t result_address;

	while (ram_cmd_freespace(dev) < cmd_size) {
		refresh_reg_cmd_read(dev);
	}

	ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, CMD_CALIBRATE);
	increase_reg_cmd_write(sizeof(CMD_CALIBRATE));

	result_address = data->memory_map->RAM_CMD + data->reg_cmd_write;
	ft8xx_wr32(bus, result_address, 1UL);
	increase_reg_cmd_write(dev, sizeof(uint32_t));

	flush_reg_cmd_write(dev);

	/* Wait until calibration is finished. */
	while (ram_cmd_fullness(dev) > 0) {
		refresh_reg_cmd_read(dev);
	}

	*result = ft8xx_rd32(bus, result_address);
}
