/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/ft8xx/ft8xx_copro.h>

#include <stdint.h>
#include <string.h>
#include <time.h>


#include <zephyr/drivers/misc/ft8xx/ft8xx_common.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_memory.h>
#include "ft8xx_drv.h"

#define FT800_RAM_CMD_SIZE 4096UL

#define BPP_ARGB1555 		16
#define BPP_L1 				1
#define BPP_L2				2
#define BPP_L4				4
#define BPP_L8 				8
#define BPP_RGB332			8
#define BPP_ARGB2			8
#define BPP_ARGB4			16
#define BPP_ARGB8			32
#define BPP_RGB565			16
#define BPP_PALETTED565		8
#define BPP_PALETTED4444	8
#define BPP_PALETTED8		8



enum ft81x_commands{
	CMD_DLSTART				= 0xffffff00,
	CMD_SWAP				= 0xffffff01,
	CMD_INTERUPT			= 0xffffff02,
	CMD_BGCOLOR				= 0xFFFFFF09,	
	CMD_FGCOLOR				= 0xFFFFFF0A,
	CMD_GRAGIENT 			= 0xFFFFFF0B,		
	CMD_TEXT				= 0xffffff0C,
	CMD_BUTTON				= 0xFFFFFF0D,
	CMD_PROGRESS			= 0xFFFFFF0F,
	CMD_SLIDER				= 0xffffff10,		
	CMD_SCROLLBAR 			= 0xFFFFFF11,	
	CMD_TOGGLE				= 0xffffff12,	
	CMD_GAUGE				= 0xFFFFFF13,	
	CMD_CLOCK				= 0xFFFFFF14,	
	CMD_CALIBRATE			= 0xffffff15,
	CMD_SPINNER				= 0xFFFFFF16,
	CMD_STOP				= 0xFFFFFF17,	
	CMD_MEMCRC				= 0xFFFFFF18,
	CMD_REGREAD				= 0xFFFFFF19,
	CMD_MEMWRITE			= 0xFFFFFF1A,
	CMD_MEMZERO				= 0xFFFFFF1C,
	CMD_APPEND				= 0xFFFFFF1E,
	CMD_SNAPSHOT			= 0xFFFFFF1F,

	CMD_BITMAP_TRANSFORM	= 0xFFFFFF21,
	CMD_INFLATE				= 0xFFFFFF22,
	CMD_GETPTR 				= 0xFFFFFF23,	
	CMD_LOADIMAGE 			= 0xFFFFFF24,
	CMD_GETPROPS			= 0xFFFFFF25,	
	CMD_LOADIDENTITY 		= 0xffffff26,
	CMD_TRANSLATE			= 0xFFFFFF27,	
	CMD_SCALE				= 0xFFFFFF28,
	CMD_ROTATE				= 0xFFFFFF29,
	CMD_SETMATRIX			= 0xffffff2A,
	CMD_SETFONT				= 0xFFFFFF2B,
	CMD_TRACK				= 0xFFFFFF2C,	
	CMD_DIAL				= 0xffffff2d,		
	CMD_NUMBER				= 0xffffff2e,
	CMD_SCREENSAVER			= 0xFFFFFF2F,
	CMD_SKETCH				= 0xFFFFFF30,	
	CMD_LOGO				= 0xFFFFFF31,
	CMD_COLDSTART			= 0xffffff32,
	CMD_GETMATRIX			= 0xffffff33,	
	CMD_GRADCOLOR			= 0xFFFFFF34,
	CMD_SETROTATE			= 0xFFFFFF36,
	CMD_SNAPSHOT2			= 0xFFFFFF37,
	CMD_MEDIAFIFO 			= 0xFFFFFF39,
	CMD_PLAYVIDEO 			= 0xFFFFFF3A,
	CMD_SETFONT2			= 0xFFFFFF3B,
	CMD_SETSCRATCH			= 0xffffff3c,


	CMD_ROMFONT 			= 0xffffff3f,	
	CMD_VIDEOSTART  		= 0xFFFFFF40,
	CMD_VIDEOFRAME  		= 0xFFFFFF41,
	CMD_SYNC				= 0xFFFFFF42,
	CMD_SETBITMAP			= 0xFFFFFF43,
	CMD_FLASHERASE			= 0xFFFFFF44,
	CMD_FLASHEWRTITE 		= 0xFFFFFF45,
	CMD_FLASHEREAD			= 0xFFFFFF46,
	CMD_FLASHUPDATE 		= 0xFFFFFF47,
	CMD_FLASHDETACH 		= 0xFFFFFF48,
	CMD_FLASHATTACH 		= 0xFFFFFF49,
	CMD_FLASHFAST			= 0xFFFFFF4A,
	CMD_FLASHSPIDESEL 		= 0xFFFFFF4B,
	CMD_FLASHSPITX			= 0xFFFFFF4C,
	CMD_FLASHSPIRX 			= 0xFFFFFF4D,
	CMD_CLEARCACHE 			= 0xFFFFFF4F,
	CMD_INFLATE2			= 0xFFFFFF50,
	CMD_ROTATEAROUND 		= 0xFFFFFF51,
	CMD_RESETFONTS			= 0xFFFFFF52,
	CMD_ANIMSTART			= 0xFFFFFF53,
	CMD_ANIMSTOP			= 0xFFFFFF54,
	CMD_ANIMXY 				= 0xFFFFFF55,
	CMD_ANIMDRAW			= 0xFFFFFF56,
	CMD_GRADIENTA			= 0xFFFFFF57,
	CMD_FILLWIDTH			= 0xFFFFFF58,
	CMD_APPENDF				= 0xFFFFFF59,
	CMD_ANIMFRAME			= 0xFFFFFF5A,

	CMD_VIDEOSTARTF			= 0xFFFFFF5F,
	CMD_CALIBRATESUB 		= 0xFFFFFF60,
	CMD_TESTCARD			= 0xFFFFFF61,
	CMD_HSF 				= 0xFFFFFF62,
	CMD_APILEVEL			= 0xFFFFFF63,
	CMD_GETIMAGE 			= 0xFFFFFF64,
	CMD_WAIT				= 0xFFFFFF65,
	CMD_RETURN				= 0xFFFFFF66,
	CMD_CALLLIST			= 0xFFFFFF67,
	CMD_NEWLIST				= 0xFFFFFF68,
	CMD_ENDLIST				= 0xFFFFFF69,
	CMD_PCLKFREQ			= 0xFFFFFF6A,
	CMD_FONTCACHE			= 0xFFFFFF6B,
	CMD_FONTCACHEQUERY 		= 0xFFFFFF6C,
	CMD_ANIMFRAMERAM 		= 0xFFFFFF6D,
	CMD_ANIMSTARTRAM 		= 0xFFFFFF6E,
	CMD_RUNANIM				= 0xFFFFFF6F,
	CMD_FLASHEPROGRAM 		= 0xFFFFFF70,

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

static uint16_t reg_cmdb_space(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;	
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	if (data->register_map->REG_CMDB_SPACE)
	{
		// 32bit register 12bits of data highest 20 bits reserved
		return ft8xx_rd16(bus, data->register_map->REG_CMDB_SPACE) & 0x0FFF;
	}

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

static void refresh_reg_cmd_write(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;	
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	data->reg_cmd_write = ft8xx_rd32(bus, data->register_map->REG_CMD_WRITE);
}


static void wait_cmd_freespace(const struct device *dev, uint16_t free_space)
{
	if (data->memory_map->REG_CMDB_SPACE)
	{
		while (reg_cmdb_space(dev) < free_space) {
			//sleep? timeout?
		}
	}
	else 
	{
		while (ram_cmd_freespace(dev) < free_space) {
			refresh_reg_cmd_read(dev);
		}
	}

}

static int wait_cmd_freespace_timeout(const struct device *dev, uint16_t free_space, uint32_t timeout)
{
	struct time_t start; 

	if (timeout != 0)
	{
		start = clock() * 1000 / CLOCKS_PER_SEC;
	}	

	if (data->memory_map->REG_CMDB_SPACE)
	{
		while (reg_cmdb_space(dev) < free_space) {
			// k_sleep(K_MSEC(1)) //sleep? 
			if ( clock() * 1000 / CLOCKS_PER_SEC - start > timeout )
				{
				return 1;	
				}			
		}
	}
	else 
	{
		while (ram_cmd_freespace(dev) < free_space) {
			refresh_reg_cmd_read(dev);
			if ( clock() * 1000 / CLOCKS_PER_SEC - start > timeout )
				{
				return 1;	
				}				
		}
	}

}

void ft8xx_copro_cmd(const struct device *dev, uint32_t cmd)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	wait_cmd_freespace(dev, cmd_size);

	// use command append register where possible
	if (data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wr32(bus, data->memory_map->REG_CMDB_WRITE, cmd);		
	}
	else 
	{
		ft8xx_wr32(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, cmd);				
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_uint(const struct device *dev, uint32_t cmd, uint32_t param1)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(cmd) +
				   sizeof(param);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;
	uint32_t target_address;

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,cmd);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer);

	}
	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_uint_uint(const struct device *dev, uint32_t cmd, uint32_t param, uint32_t param2)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(cmd) +
				   sizeof(param) +
				   sizeof(param2);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;
	uint32_t target_address;

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,cmd);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param2);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}

void ft8xx_copro_cmd_uint_uint_uint(const struct device *dev, uint32_t cmd, uint32_t param, uint32_t param2, uint32_t param3)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(cmd) +
				   sizeof(param) +
				   sizeof(param2)+
				   sizeof(param3);


	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;
	uint32_t target_address;

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,cmd);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param2);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param3);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_uint_uint_uint_uint(const struct device *dev, uint32_t cmd, uint32_t param, uint32_t param2, uint32_t param3, uint32_t param4)
{
	const struct ft8xx_data *data = dev->data;
	const struct ft8xx_config *config = dev->config;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(cmd) +
				   sizeof(param) +
				   sizeof(param2)+
				   sizeof(param3)+
				   sizeof(param4);


	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;
	uint32_t target_address;

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,cmd);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param2);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param3);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,param4);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer);
	}

	increase_reg_cmd_write(dev, cmd_size);
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

void ft8xx_copro_cmd_regread(const struct device *dev, uint32_t ptr,
			uint32_t *result )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint_uint(dev, CMD_REGREAD, ptr, 0);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*result = ft8xx_rd32(dev, data->memory_map->RAM_CMD +(fifo_offset + 8) % 4096) )

}

void ft8xx_copro_cmd_memwrite(const struct device *dev, uint32_t ptr, uint32_t num,
			uint8_t *src )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t padding_bytes = (4 - (num % 4)) % 4
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(ptr);
	
	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;
	
	if 	(cmd_size+num+padding_bytes > FT800_RAM_CMD_SIZE)
	{
		// automatic splitting?
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
		memset(buffer, 0, cmd_size);

		wait_cmd_freespace(dev, cmd_size);

		offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_MEMWRITE);
		offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,ptr);
		offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,num);
	
		// use command append register where possible
		if (!data->memory_map->REG_CMDB_WRITE)
		{
			ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, src, num, padding_bytes);

		}
		else
		{
			ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, src, num, padding_bytes);
		}

		increase_reg_cmd_write(dev, cmd_size+num + padding_bytes);
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
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(ptr);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	   
	
	if 	(cmd_size > FT800_RAM_CMD_SIZE)
	{
		LOG_ERR("Command Too Large for Command Buffer");
		return -ENOTSUP;			
	}
	else
	{
		memset(buffer, 0, cmd_size);

		wait_cmd_freespace(dev, cmd_size);

		offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_INFLATE);
		offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,ptr);
		
		// use command append register where possible
		if (!data->memory_map->REG_CMDB_WRITE)
		{
			ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, src, len, padding_bytes);

		}
		else
		{
			ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, src, len, padding_bytes);
		}

		increase_reg_cmd_write(dev, cmd_size+len + padding_bytes);
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
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(ptr) +
				   sizeof(options);
	const uint16_t data_size = len +
				   padding_bytes;


	const bool data_external = (((options & FT81x_OPT_MEDIAFIFO) == FT81x_OPT_MEDIAFIFO) | 
				((options & FT81x_OPT_FLASH) == FT81x_OPT_FLASH)) ;

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

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
			ft8xx_copro_cmd_uint_uint(dev, CMD_INFLATE2, ptr, options);
		}
		else
		{
			memset(buffer, 0, cmd_size);

			wait_cmd_freespace(dev, cmd_size);

			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_INFLATE2);
			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,ptr);
			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,options);
			
			// use command append register where possible
			if (!data->memory_map->REG_CMDB_WRITE)
			{
				ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, src, len, padding_bytes);

			}
			else
			{
				ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, src, len, padding_bytes);
			}

			increase_reg_cmd_write(dev, cmd_size+len + padding_bytes);
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
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(ptr) +
				   sizeof(options);
	const uint16_t data_size = len +
				   padding_bytes;

	// data not included in command
	const bool data_external = (((options & FT81x_OPT_MEDIAFIFO) == FT81x_OPT_MEDIAFIFO) | 
				((options & FT81x_OPT_FLASH) == FT81x_OPT_FLASH)) ;

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

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
			memset(buffer, 0, cmd_size);

			wait_cmd_freespace(dev, cmd_size);

			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_LOADIMAGE);
			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,ptr);
			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,options);
			
			// use command append register where possible
			if (!data->memory_map->REG_CMDB_WRITE)
			{
				ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, src, len, padding_bytes);

			}
			else
			{
				ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, src, len, padding_bytes);
			}

			increase_reg_cmd_write(dev, cmd_size+len + padding_bytes);
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

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

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
	if ((ptr % 4) != 0)
	{
	{
		LOG_ERR("FIFO start must be 4 bytes aligned");
		return -ENOTSUP;
	}
	}
	if ((num % 4) != 0) 
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

int ft8xx_copro_cmd_playvideo(const struct device *dev, uint32_t options, 
				uint32_t *src, uint32_t len) ;
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t padding_bytes = (4 - (len % 4)) % 4
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(ptr) +
				   sizeof(options)
	const uint16_t data_size = len +
				   padding_bytes;


	const bool data_external = (((options & FT81x_OPT_MEDIAFIFO) == FT81x_OPT_MEDIAFIFO) || 
				((options & FT81x_OPT_FLASH) == FT81x_OPT_FLASH)) ;

	int options_filtered = options;

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
			LOG_ERR("Video Playback Not supported on this device");
			return -ENOTSUP;
		}
	}

	if ((options & FT81x_OPT_FLASH ) == FT81x_OPT_FLASH ))
	{
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
			LOG_ERR("Flash Video Playback Not supported on this device");
			return -ENOTSUP;
		}

	}
	// filter unsupported options
	switch(data->chip_id) {
		case FT8xx_CHIP_ID_FT811:
		case FT8xx_CHIP_ID_FT812:
		case FT8xx_CHIP_ID_FT813:
		{
		
			if ((options_filtered & FT81x_OPT_OVERLAY ) == FT81x_OPT_OVERLAY ))
			{
				options_filtered &= options ~(FT81x_OPT_OVERLAY))
				LOG_NOTICE("OPT_OVERLAY not suppored so ignored")

			}

			if ((options_filtered & OPT_NODL ) == OPT_NODL ))
			{
				options_filtered &= options ~(OPT_NODL ))
				LOG_NOTICE("OPT_NODL not suppored so ignored")

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
			ft8xx_copro_cmd_uint_uint(dev, CMD_PLAYVIDEO, ptr, options);
		}
		else
		{
			memset(buffer, 0, cmd_size);

			wait_cmd_freespace(dev, cmd_size);

			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_PLAYVIDEO);
			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,ptr);
			offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,options);
			
			// use command append register where possible
			if (!data->memory_map->REG_CMDB_WRITE)
			{
				ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, src, len, padding_bytes);

			}
			else
			{
				ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, src, len, padding_bytes);
			}

			increase_reg_cmd_write(dev, cmd_size+len + padding_bytes);
			flush_reg_cmd_write(dev);


		}
	}
	
	return 0;
}

void ft8xx_copro_cmd_videostart(const struct device) ;
{
	ft8xx_copro_cmd(dev, CMD_VIDEOSTART);
}

void ft8xx_copro_cmd_videoframe(const struct device, uint32_t dst,
				uint32_t prt)
{
	ft8xx_copro_cmd_uint_uint(dev, CMD_VIDEOFRAME, dst, ptr);
}

void ft8xx_copro_cmd_memcrc(const struct device *dev, uint32_t ptr,
				uint32_t num, uint32_t *result );
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_MEMCRC, ptr, num, 0);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*result = ft8xx_rd32(dev, data->memory_map->RAM_CMD +(fifo_offset + 12) % 4096) )

}

void ft8xx_copro_cmd_memzero(const struct device *dev, uint32_t ptr,
				uint32_t num)
{
	ft8xx_copro_cmd_uint_uint(dev, CMD_MEMZERO, ptr, num);
}

void ft8xx_copro_cmd_memset(const struct device *dev, uint32_t ptr,
				uint32_t value, uint32_t num)
{
	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_MEMSET, ptr, value, num);
}

void ft8xx_copro_cmd_memcpy(const struct device *dev, uint32_t dest, 
				uint32_t src, uint32_t num)
{
	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_MEMCPY, dest, src, num);
}

void ft8xx_copro_cmd_button(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				int16_t font,
				uint16_t options,
				const char* s );
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t str_bytes = strlen(s) + 1;
	const uint16_t padding_bytes = (4 - (str_bytes % 4)) % 4;
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(w) +
				   sizeof(h) +
				   sizeof(font) +
				   sizeof(options);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	bool unicode = 0;


	//bt81x
	switch(data->chip_id) {
		case FT8xx_CHIP_ID_BT815:
		case FT8xx_CHIP_ID_BT816:
		case FT8xx_CHIP_ID_BT817:
		case FT8xx_CHIP_ID_BT818:
			{
				unicode = 1;
				break;
			}
		default:
		{
			unicode = 0;
			//does this need any handling?
			//test if string utf8 and strip non ascii unicode chars?
		}
	}

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_BUTTON);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,h);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,font);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, s, str_bytes, padding_bytes);

	}
	else
	{
		ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, s, str_bytes, padding_bytes);
	}

	increase_reg_cmd_write(dev, cmd_size+str_bytes + padding_bytes);
	flush_reg_cmd_write(dev);

}

void ft8xx_copro_cmd_clock(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t r,
				uint16_t options,
				uint16_t h,
				uint16_t m,
				uint16_t s,
				uint16_t ms )
{

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(r) +
				   sizeof(options) +
				   sizeof(h) +
				   sizeof(m) +
				   sizeof(s) +
				   sizeof(ms);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,CMD_CLOCK);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,r);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,h);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,m);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,s);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,ms);


	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}

void ft8xx_copro_cmd_fgcolor(const struct device *dev, uint32_t c);
{

	ft8xx_copro_cmd_uint(dev, CMD_FGCOLOR, (c & 0x00FFFFFF));

}

void ft8xx_copro_cmd_bgcolor(const struct device *dev, uint32_t c);
{

	ft8xx_copro_cmd_uint(dev, CMD_BGCOLOR, (c & 0x00FFFFFF));

}

void ft8xx_copro_cmd_gradcolor(const struct device *dev, uint32_t c);
{

	ft8xx_copro_cmd_uint(dev, CMD_GRADCOLOR, (c & 0x00FFFFFF));

}

void ft8xx_copro_cmd_gauge(const struct device *dev, int16_t x,
			   int16_t y,
			   int16_t r,
			   uint16_t options,
			   uint16_t major,
			   uint16_t minor,
			   uint16_t val,
			   uint16_t range)
{

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(r) +
				   sizeof(options) +
				   sizeof(major) +
				   sizeof(minor) +
				   sizeof(val) +
				   sizeof(range);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,CMD_GAUGE);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,r);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,major);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,minor);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,val);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,range);


	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_gradient(const struct device *dev, int16_t x0,
			   int16_t y0,
			   uint32_t rgb0,
			   int16_t x1,
			   int16_t y1,
			   uint32_t rgb1);
{

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x0) +
				   sizeof(y0) +
				   sizeof(rgb0) +
				   sizeof(x1) +
				   sizeof(y1) +
				   sizeof(rgb1);


	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,CMD_GRADIENT);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x0);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y0);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,(rbg0 & 0x00FFFFFF));
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,x1);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,y1);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,(rgb1 & 0x00FFFFFF));

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);


}

void ft8xx_copro_cmd_gradientA(const struct device *dev, int16_t x0,
			   int16_t y0,
			   uint32_t argb0,
			   int16_t x1,
			   int16_t y1,
			   uint32_t argb1);
{

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x0) +
				   sizeof(y0) +
				   sizeof(argb0) +
				   sizeof(x1) +
				   sizeof(y1) +
				   sizeof(argb1);


	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,CMD_GRADIENTA);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x0);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y0);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,(arbg0));
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,x1);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,y1);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,(argb1));

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);


}

void ft8xx_copro_cmd_keys(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				int16_t font,
				uint16_t options,
				const char* s );
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(w) +
				   sizeof(h) +
				   sizeof(font) +
				   sizeof(options);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	bool unicode = 0;


	//bt81x
	switch(data->chip_id) {
		case FT8xx_CHIP_ID_BT815:
		case FT8xx_CHIP_ID_BT816:
		case FT8xx_CHIP_ID_BT817:
		case FT8xx_CHIP_ID_BT818:
			{
				unicode = 1;
				break;
			}
		default:
		{
			unicode = 0;
			//does this need any handling?
			//test if string utf8 and strip non ascii unicode chars?
		}
	}

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_KEYS);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,h);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,font);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, s, str_bytes, padding_bytes);

	}
	else
	{
		ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, s, str_bytes, padding_bytes);
	}

	increase_reg_cmd_write(dev, cmd_size+str_bytes + padding_bytes);
	flush_reg_cmd_write(dev);

}

void ft8xx_copro_cmd_progress(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				uint16_t options,
				uint16_t val,
				uint16_t range);
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(w) +
				   sizeof(h) +
				   sizeof(options)+
				   sizeof(val) +
				   sizeof(range) +
				   sizeof(uint16_t) ; // pad to multiple of 4 bytes;


	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_PROGRESS);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,h);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,val);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,range);


	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_scrollbar(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				uint16_t options,
				uint16_t val,
				uint16_t size,
				uint16_t range)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(w) +
				   sizeof(h) +
				   sizeof(options)+
				   sizeof(val) +
				   sizeof(size) +
				   sizeof(range);


	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_SCROLLBAR);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,h);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,val);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,size);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,range);


	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_slider(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				uint16_t options,
				uint16_t val,
				uint16_t range);
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) +
					sizeof(x) +
					sizeof(y) +
					sizeof(w) +
					sizeof(h) +
					sizeof(options)+
					sizeof(val) +
					sizeof(range) +
					sizeof(uint16_t) ; // pad to multiple of 4 bytes


	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_SLIDER);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,h);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,val);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,range);


	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_dial(const struct device *dev, int16_t x,
				int16_t y,
				int16_t w,
				int16_t r,
				uint16_t options,
				uint16_t val)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(r) +
				   sizeof(options) +
				   sizeof(val);


	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_DIAL);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,r);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,val);	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);


}
void ft8xx_copro_cmd_toggle(const struct device *dev, int16_t x,
			   int16_t y,
			   int16_t w,
			   int16_t font,
			   uint16_t options,
			   uint16_t state,
			   const char *s)
{
const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t str_bytes = strlen(s) + 1;
	const uint16_t padding_bytes = (4 - (str_bytes % 4)) % 4;
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(font) +
				   sizeof(options);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	

	bool unicode = 0;


	//bt81x
	switch(data->chip_id) {
		case FT8xx_CHIP_ID_BT815:
		case FT8xx_CHIP_ID_BT816:
		case FT8xx_CHIP_ID_BT817:
		case FT8xx_CHIP_ID_BT818:
			{
				unicode = 1;
				break;
			}
		default:
		{
			unicode = 0;
			//does this need any handling?
			//test if string utf8 and strip non ascii unicode chars?
		}
	}

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_TOGGLE);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,font);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,state);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, s, str_bytes, padding_bytes);

	}
	else
	{
		ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, s, str_bytes, padding_bytes);
	}

	increase_reg_cmd_write(dev, cmd_size+str_bytes + padding_bytes);
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_fillwidth(const struct device *dev, uint32_t s)
{

	ft8xx_copro_cmd_uint(dev, CMD_FILLWIDTH, s);

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
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(font) +
				   sizeof(options);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	

	bool unicode = 0;


	//bt81x
	switch(data->chip_id) {
		case FT8xx_CHIP_ID_BT815:
		case FT8xx_CHIP_ID_BT816:
		case FT8xx_CHIP_ID_BT817:
		case FT8xx_CHIP_ID_BT818:
			{
				unicode = 1;
				break;
			}
		default:
		{
			unicode = 0;
			//does this need any handling?
			//test if string utf8 and strip non ascii unicode chars?
		}
	}

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_TEXT);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,font);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, s, str_bytes, padding_bytes);

	}
	else
	{
		ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, s, str_bytes, padding_bytes);
	}

	increase_reg_cmd_write(dev, cmd_size+str_bytes + padding_bytes);
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

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(font) +
				   sizeof(options) +
				   sizeof(n);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;				   

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_NUMBER);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,font);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,options);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,n);	
	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size+str_bytes + padding_bytes);
	flush_reg_cmd_write(dev);

}

void ft8xx_copro_cmd_loadidentity(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_LOADIDENTITY);

}


void ft8xx_copro_cmd_setmatrix(const struct device *dev)
{

	ft8xx_copro_cmd(dev, CMD_GETMATRIX);

}

void ft8xx_copro_cmd_getmatrix(const struct device *dev, const struct ft8xx_bitmap_transform_matrix_t *matrix)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t data_size =	sizeof(matrix->a) +
				   sizeof(matrix->b) +
				   sizeof(matrix->c) +
				   sizeof(matrix->d) +
				   sizeof(matrix->e) +
				   sizeof(matrix->f);

	const uint16_t cmd_size = sizeof(uint32_t) + data_size;

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;		

	uint8_t read_data[data_size]; 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_GETMATRIX);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,0);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,0);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,0);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,0);	
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,0);	
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,0);	
	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	increase_reg_cmd_write(dev, cmd_size+str_bytes + padding_bytes);
	flush_reg_cmd_write(dev);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	ft8xx_rdblock(dev, data->memory_map->RAM_CMD +(fifo_offset + 4) % 4096), read_data , data_size);

	matrix->a = sys_le32_to_cpu(*(uint32_t *)(read_data));
	matrix->b = sys_le32_to_cpu(*(uint32_t *)(read_data+sizeof(uint32_t)));
	matrix->c = sys_le32_to_cpu(*(uint32_t *)(read_data+sizeof(uint32_t)*2));
	matrix->d = sys_le32_to_cpu(*(uint32_t *)(read_data+sizeof(uint32_t)*3));
	matrix->e = sys_le32_to_cpu(*(uint32_t *)(read_data+sizeof(uint32_t)*4));
	matrix->f = sys_le32_to_cpu(*(uint32_t *)(read_data+sizeof(uint32_t)*5));


}

void ft8xx_copro_cmd_getptr(const struct device *dev, uint32_t *result );
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint(dev, CMD_GETPTR, 0);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*result = ft8xx_rd32(dev, data->memory_map->RAM_CMD +(fifo_offset + 4) % 4096) )

}

void ft8xx_copro_cmd_getprops(const struct device *dev, uint32_t *ptr, uint32_t *width, 
			uint32_t *height );
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t data_size =	sizeof(uint32_t) +
				   sizeof(uint32_t) +
				   sizeof(uint32_t);

	const uint16_t cmd_size = sizeof(uint32_t) + data_size;

	uint8_t read_data[data_size]; 

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint(dev, CMD_GETPTR, 0);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	ft8xx_rdblock(dev, data->memory_map->RAM_CMD +(fifo_offset + sizeof(uint32_t)) % 4096), read_data , data_size);

	*ptr = sys_le32_to_cpu(*(uint32_t *)(read_data));
	*width = sys_le32_to_cpu(*(uint32_t *)(read_data+sizeof(uint32_t)));
	*height = sys_le32_to_cpu(*(uint32_t *)(read_data+sizeof(uint32_t)*2));

}

void ft8xx_copro_cmd_scale(const struct device *dev, int32_t sx, int32_t sy )
{
	uint32_t sx_u = ((union { int32_t i; uint32_t u; }){ .i = sx }).u;
	uint32_t sy_u = ((union { int32_t i; uint32_t u; }){ .i = sy }).u;

	ft8xx_copro_cmd_uint_uint(dev, CMD_SCALE, sx_u,sy_u);

}

void ft8xx_copro_cmd_rotate(const struct device *dev, int32_t a )
{
	uint32_t a_u = ((union { int32_t i; uint32_t u; }){ .i = sx }).u;

	ft8xx_copro_cmd_uint_(dev, CMD_ROTATE, a_u);

}

void ft8xx_copro_cmd_rotatearound(const struct device *dev, int32_t x, int32_t y,
			int32_t a,
			int32_t s)
{
	uint32_t x_u = ((union { int32_t i; uint32_t u; }){ .i = sx }).u;
	uint32_t y_u = ((union { int32_t i; uint32_t u; }){ .i = sx }).u;
	uint32_t a_u = ((union { int32_t i; uint32_t u; }){ .i = sx }).u;
	uint32_t s_u = ((union { int32_t i; uint32_t u; }){ .i = sx }).u;


	ft8xx_copro_cmd_uint_uint_uint_uint(dev, CMD_ROTATEAROUND, x_u, y_u, a_u, s_u);

}

void ft8xx_copro_cmd_translate(const struct device *dev, int32_t x, int32_t y)
{
	uint32_t x_u = ((union { int32_t i; uint32_t u; }){ .i = sx }).u;
	uint32_t y_u = ((union { int32_t i; uint32_t u; }){ .i = sx }).u;

	ft8xx_copro_cmd_uint_uint(dev, CMD_TRANSLATE, x_u, y_u);

}

void ft8xx_copro_cmd_calibrate(const struct device *dev, uint32_t *result)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint(dev, CMD_CALIBRATE, 1);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*result = ft8xx_rd32(bus, data->memory_map->RAM_CMD +(fifo_offset + 4) % 4096) )

}

void ft8xx_copro_cmd_calibratesub(const struct device *dev, uint16_t x, uint16_t y,
			uint16_t w,
			uint16_t h,
			uint32_t *result)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) + 
					sizeof(x) +
					sizeof(y) +
					sizeof(w) +
					sizeof(h) +
					sizeof(uint32_t);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;		

	uint8_t read_data[data_size]; 

	//bt817/8 only
	switch(data->chip_id) {
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

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_CALIBRATESUB);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,h);	
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,0);	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*result = ft8xx_rd32(dev, data->memory_map->RAM_CMD +(fifo_offset + (sizeof(uint32_t)*2+sizeof(uint16_t)*4) % 4096) )
}

int ft8xx_copro_cmd_setrotation(const struct device *dev, uint32_t r)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	//ft81x or later
	if(data->chip_id) < FT8xx_CHIP_ID_FT810
	{
			return -ENOTSUP;
	}

	ft8xx_copro_cmd_uint(dev, CMD_SETROTATE, r);

}

void ft8xx_copro_cmd_spinner(const struct device *dev, int16_t x, int16_t y,
			uint16_t style,
			uint16_t scale)
{
	const uint16_t cmd_size = sizeof(uint32_t) + 
					sizeof(x) +
					sizeof(y) +
					sizeof(style) +
					sizeof(scale);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;		

	uint8_t read_data[data_size]; 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_SPINNER);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,style);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,scale);	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}

void ft8xx_copro_cmd_screensaver(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_SCREENSAVER);
}


void ft8xx_copro_cmd_sketch(const struct device *dev, int16_t x,
			int16_t y,
			uint16_t w,
			uint16_t h,
			uint32_t ptr,
			uint16_t format)
{
	const uint16_t cmd_size = sizeof(uint32_t) + 
					sizeof(x) +
					sizeof(y) +
					sizeof(w) +
					sizeof(h) +
					sizeof(ptr) +
					sizeof(format)+
					sizeof(uint16_t) ; // pad to multiple of 4 bytes

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;		

	uint8_t read_data[data_size]; 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_SKETCH);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,h);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,ptr);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,format);	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);
	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}

void ft8xx_copro_cmd_stop(const struct device *dev);
{
	ft8xx_copro_cmd(dev, CMD_STOP);
}


int ft8xx_copro_cmd_setfont(const struct device *dev, uint32_t font, uint32_t ptr)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	ft8xx_copro_cmd_uint_uint(dev, CMD_SETFONT,font,ptr);

}

int ft8xx_copro_cmd_setfont2(const struct device *dev, uint32_t font, uint32_t ptr,
				uint32_t firstchar)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	//ft81x or later
	if(data->chip_id) < FT8xx_CHIP_ID_FT810
	{
			return -ENOTSUP;
	}

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_SETFONT2,font,ptr);

}

void ft8xx_copro_cmd_setscratch(const struct device *dev, uint32_t handle)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	//ft81x or later
	if(data->chip_id) < FT8xx_CHIP_ID_FT810
	{
			return -ENOTSUP;
	}

	ft8xx_copro_cmd_uint(dev, CMD_SETSCRATCH, handle);
}

void ft8xx_copro_cmd_romfont(const struct device *dev, uint32_t font,
			uint32_t romslot)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	//ft81x or later
	if(data->chip_id) < FT8xx_CHIP_ID_FT810
	{
			return -ENOTSUP;
	}

	ft8xx_copro_cmd_uint_uint(dev, CMD_ROMFONT, font, romslot);
}

void ft8xx_copro_cmd_resetfonts(const struct device *dev)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	//ft81x or later
	if(data->chip_id) < FT8xx_CHIP_ID_FT810
	{
			return -ENOTSUP;
	}

	ft8xx_copro_cmd(dev, CMD_RESETFONTS);
}

void ft8xx_copro_cmd_track(const struct device *dev, int16_t x,
			int16_t y,
			int16_t w,
			int16_t h,
			int16_t tag);
{
	const uint16_t cmd_size = sizeof(uint32_t) + 
					sizeof(x) +
					sizeof(y) +
					sizeof(w) +
					sizeof(h) +
					sizeof(tag) +
					sizeof(uint16_t) ; // pad to multiple of 4 bytes

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;		

	uint8_t read_data[data_size]; 

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_TRACK);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,h);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,tag);
	//offset = ft8xx_append_block_int16(buffer,cmd_size,offset,0);	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);
	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}

void ft8xx_copro_cmd_snapshot(const struct device *dev, uint32_t ptr)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	uint16_t r_hsize = ft8xx_rd16(bus,data->memory_map->REG_HSIZE);
	uint16_t r_vsize = ft8xx_rd16(bus,data->memory_map->REG_VSIZE);


	size = (BPP_ARGB4 * r_hsize * r_vsize ) /8; 


	if (ptr+size > data->memory_map->RAM_G_END))
	{
		LOG_ERR("Snapshot Target Outside memory Range");
		return -ENOTSUP;			
	}

	ft8xx_copro_cmd_uint(dev, CMD_SNAPSHOT,ptr);

}


int ft8xx_copro_cmd_snapshot2(const struct device *dev,  uint32_t fmt, uint32_t ptr,
			int16_t x,
			int16_t y,
			int16_t w,
			int16_t h)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) + 
					sizeof(fmt) +
					sizeof(ptr) +
					sizeof(x) +
					sizeof(y) +
					sizeof(w) +
					sizeof(h); 

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;		

	uint8_t read_data[data_size]; 

	//ft81x or later
	if(data->chip_id) < FT8xx_CHIP_ID_FT810
	{
			return -ENOTSUP;
	}

	uint_32_t size

	switch (fmt) 	{
		case FT81x_FORMAT_RGB565:
		{
			size = (BPP_RGB565 * w * h)/8; 
			break;
		} 
		
		case FT81x_FORMAT_ARGB4:
		{
			size = (BPP_ARGB4 * w * h)/8; 
			break;
		} 

		case FT81x_FORMAT_ARGB8:
		{
			size = (BPP_ARGB8 * w * h)/8; 
			break;
		} 

		default 
		{
			return -ENOTSUP;
		}
	}

	if (ptr+size > data->memory_map->RAM_G_END))
	{
		LOG_ERR("Snapshot Target Outside memory Range");
		return -ENOTSUP;			
	}

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_SNAPSHOT2);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,fmt);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,ptr);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,w);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,h);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);
	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);
}


int ft8xx_copro_cmd_setbitmap(const struct device *dev,  uint32_t source,
			uint16_t fmt,
			uint16_t width,
			uint16_t height )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) + 
					sizeof(source) +
					sizeof(fmt) +
					sizeof(width) +
					sizeof(height) +
					sizeof(uint16_t); // pad to multiple of 4 bytes

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;		

	uint8_t read_data[data_size]; 

	//ft81x or later
	if(data->chip_id) < FT8xx_CHIP_ID_FT810
	{
			return -ENOTSUP;
	}

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_SETBITMAP);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,source);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,fmt);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,width);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,height);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);
	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}

int ft8xx_copro_cmd_logo(const struct device *dev)
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
			return -ENOTSUP;
		}
	}

	ft8xx_copro_cmd_uint(dev, CMD_SNAPSHOT,ptr);
}

int ft8xx_copro_cmd_flasherase(const struct device *dev)
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
			return -ENOTSUP;
		}
	}	
		
	ft8xx_copro_cmd_uint(dev, CMD_FLASHERASE,ptr);
}

void ft8xx_copro_cmd_flashwrite(const struct device *dev, uint32_t ptr,
			uint32_t num, uint8_t *src)
{

	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t alignment = 256;
	const uint16_t padding_bytes = (alignment - (num % alignment)) % alignment;
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(ptr);
	
	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;
	
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

	if (ptr % alignment)
	{
		LOG_ERR("Bad Flash block alignment, start address must be multiple of 256");
		return -ENOTSUP;		
	}
	if (num % alignment)
	{
		LOG_INFO("Flash block size not multiple of 256, padding by %d bytes", padding_bytes);
	}

	if 	(cmd_size+num+padding_bytes > FT800_RAM_CMD_SIZE)
	{
		// automatic splitting?
		LOG_ERR("Command Too Large for Command Buffer");
		return -ENOTSUP;			
	}
	else if (ptr+num > data->flash_size))
	{
		LOG_ERR("Data Target Outside memory Range");
		return -ENOTSUP;			
	}
	else
	{
		memset(buffer, 0, cmd_size);

		wait_cmd_freespace(dev, cmd_size);

		offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_FLASHWRITE);
		offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,ptr);
		offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,num);
	
		// use command append register where possible
		if (!data->memory_map->REG_CMDB_WRITE)
		{
			ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, src, num, padding_bytes);

		}
		else
		{
			ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, src, num, padding_bytes);
		}

		increase_reg_cmd_write(dev, cmd_size+num + padding_bytes);
		flush_reg_cmd_write(dev);
	}


}

int ft8xx_copro_cmd_flashprogram(const struct device *dev, uint32_t dest, uint32_t src, 
			uint32_t num)
{

	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t ram_alignment = 4;
	const uint16_t flash_alignment = 4096;

	//const uint16_t padding_bytes = (flash_alignment - (num % flash_alignment)) % flash_alignment;
	
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
	if (src % ram_alignment)
	{
		LOG_ERR("RAM block alignment, start address must be multiple of %d",ram_alignment);
		return -ENOTSUP;		
	}
	if (dest % flash_alignment)
	{
		LOG_ERR("Bad Flash block alignment, start address must be multiple of %d",flash_alignment);
		return -ENOTSUP;		
	}
	if (src+num > data->memory_map->RAM_G_END))
	{
		LOG_ERR("Data Source Outside RAM Range");
		return -ENOTSUP;			
	}	
	if (dest+num > data->flash_size))
	{
		LOG_ERR("Data Target Outside Flash memory Range");
		return -ENOTSUP;			
	}	
	if (num % flash_alignment)
	{
		LOG_DEBUG("Flash block size not multiple of 4096");
		return -ENOTSUP;	
	}

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_FLASHPROGRAM, dest, src, num);

}

int ft8xx_copro_cmd_flashread(const struct device *dev, uint32_t dest, uint32_t src, 
			uint32_t num)
{

	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t ram_alignment = 4;
	const uint16_t flash_alignment = 64;

	const uint16_t padding_bytes = (ram_alignment - (num % ram_alignment)) % ram_alignment;
	
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
	if (src % ram_alignment)
	{
		LOG_ERR("RAM block alignment, start address must be multiple of %d",ram_alignment);
		return -ENOTSUP;		
	}
	if (dest % flash_alignment)
	{
		LOG_ERR("Bad Flash block alignment, start address must be multiple of %d",flash_alignment);
		return -ENOTSUP;		
	}
	if (dest+num+padding_bytes > data->memory_map->RAM_G_END))
	{
		LOG_ERR("Data Target Outside RAM Range");
		return -ENOTSUP;			
	}	
	if (src+num+padding_bytes > data->flash_size))
	{
		LOG_ERR("Data Source Outside Flash memory Range");
		return -ENOTSUP;			
	}	
	if (num % ram_alignment)
	{
		LOG_DEBUG("Ram block size not multiple of 4, padding by %d bytes", padding_bytes);
	}

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_FLASHREAD, dest, src, num+padding_bytes);

}

int ft8xx_copro_cmd_appendf(const struct device *dev, uint32_t ptr,
			uint32_t num)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t ram_alignment = 4;
	const uint16_t flash_alignment = 64;
	
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
	if (ptr % flash_alignment)
	{
		LOG_ERR("Bad Flash block alignment, start address must be multiple of %d",flash_alignment);
		return -ENOTSUP;		
	}
	if (ptr+num > data->flash_size))
	{
		LOG_ERR("Data Target Outside Flash memory Range");
		return -ENOTSUP;			
	}	
	if (num % ram_alignment)
	{
		LOG_ERR("block size not multiple of 4");
	}

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_APPENDF, ptr, num);
}


int ft8xx_copro_cmd_flashupdate(const struct device *dev, uint32_t dest, uint32_t src, 
			uint32_t num)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t ram_alignment = 4;
	const uint16_t flash_alignment = 4096;

	//const uint16_t padding_bytes = (flash_alignment - (num % flash_alignment)) % flash_alignment;
	
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
	if (src % ram_alignment)
	{
		LOG_ERR("RAM block alignment, start address must be multiple of %d",ram_alignment);
		return -ENOTSUP;		
	}
	if (dest % flash_alignment)
	{
		LOG_ERR("Bad Flash block alignment, start address must be multiple of %d",flash_alignment);
		return -ENOTSUP;		
	}
	if (src+num > data->memory_map->RAM_G_END))
	{
		LOG_ERR("Data Source Outside RAM Range");
		return -ENOTSUP;			
	}	
	if (dest+num > data->flash_size))
	{
		LOG_ERR("Data Target Outside Flash memory Range");
		return -ENOTSUP;			
	}	
	if (num % flash_alignment)
	{
		LOG_DEBUG("Flash block size not multiple of 4096");
		return -ENOTSUP;
	}

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_FLASHUPDATE, dest, src, num);

}

int ft8xx_copro_cmd_flashdetach(const struct device *dev)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	ft8xx_copro_cmd_(dev, CMD_FLASHDETACH);
}

int ft8xx_copro_cmd_flashattach(const struct device *dev)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	ft8xx_copro_cmd(dev, CMD_FLASHATTACH);
}

int ft8xx_copro_cmd_flashfast(const struct device *dev, uint32_t *result)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint(dev, CMD_FLASHFAST, 0);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*result = ft8xx_rd32(dev, data->memory_map->RAM_CMD +(fifo_offset + 4) % 4096) )

}

int ft8xx_copro_cmd_flashspidesel(const struct device *dev, uint32_t *result)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	ft8xx_copro_cmd(dev, CMD_FLASHSPIDESEL);

}

int ft8xx_copro_cmd_flashspitx(const struct device *dev, uint32_t num,
			uint8_t *src )
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(num);

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
	
	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_FLASHSPITX);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock_dual(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size, src, num, 0);

	}
	else
	{
		ft8xx_wrblock_dual(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size, src, num, 0);
	}

	increase_reg_cmd_write(dev, cmd_size+num + padding_bytes);
	flush_reg_cmd_write(dev);

}

int ft8xx_copro_cmd_flashspirx(const struct device *dev, uint32_t ptr, uint32_t num)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	ft8xx_copro_cmd_uint_uint(dev, CMD_FLASHSPIRX,ptr,num);

}

int ft8xx_copro_cmd_clearcache(const struct device *dev)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	ft8xx_copro_cmd(dev, CMD_CLEARCACHE);

}

int ft8xx_copro_cmd_flashsource(const struct device *dev, uint32_t ptr)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	if (ptr % 64)
	{
		LOG_ERR("Bad Flash block alignment, start address must be multiple of 64");
		return -ENOTSUP;		
	}

	ft8xx_copro_cmd_uint(dev, CMD_FLASHSOURCE, ptr);

}

int ft8xx_copro_cmd_videostartf(const struct device *dev)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	ft8xx_copro_cmd(dev, CMD_VIDEOSTARTF);

}

int ft8xx_copro_cmd_animstart(const struct device *dev, int32_t ch,
			uint32_t aoptr,
			uint32_t loop)
{

	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	uint32_t ch_u = ((union { int32_t i; uint32_t u; }){ .i = ch }).u;


	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_ANIMSTART,ch_u,aoptr,loop);
}

int ft8xx_copro_cmd_animstartram(const struct device *dev, int32_t ch,
			uint32_t aoptr,
			uint32_t loop)
{

	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;
	
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

	if (aoptr % 64)
	{
		LOG_ERR("RAM block alignment, start address must be multiple of 64");
		return -ENOTSUP;		
	}	

	uint32_t ch_u = ((union { int32_t i; uint32_t u; }){ .i = ch }).u;


	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_ANIMSTART,ch_u,aoptr,loop);
}

int ft8xx_copro_cmd_animstartram(const struct device *dev, uint32_t waitmask,
			uint32_t play)
{
	const struct ft8xx_data *data = dev->data;
	
	//bt817/8 only
	switch(data->chip_id) {
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

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_ANIMSTARTRAM, waitmask, play);
}

int ft8xx_copro_cmd_animxy(const struct device *dev, int32_t ch, int16_t x,
			int16_t y )
{
	const struct ft8xx_data *data = dev->data;

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(ch) +
				   sizeof(x) +
				   sizeof(y);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 


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




	uint32_t ch_u = ((union { int32_t i; uint32_t u; }){ .i = ch }).u;

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_ANIMXY);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,ch);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer);

	}
	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}

int ft8xx_copro_cmd_animdraw(const struct device *dev, int32_t ch)
{

	const struct ft8xx_data *data = dev->data;
	
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

	uint32_t ch_u = ((union { int32_t i; uint32_t u; }){ .i = ch }).u;


	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_ANIMDRAW,ch_u);
}

int ft8xx_copro_cmd_animframe(const struct device *dev, int16_t x,
			int16_t y, 
			uint32_t aoptr,
			uint32_t frame)
{
	const struct ft8xx_data *data = dev->data;

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(aoptr) +
				   sizeof(frame);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 


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
	uint32_t ch_u = ((union { int32_t i; uint32_t u; }){ .i = ch }).u;

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_ANIMFRAME);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,aoptr);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,frame);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer);

	}
	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}


int ft8xx_copro_cmd_animframeram(const struct device *dev, int16_t x,
			int16_t y, 
			uint32_t aoptr,
			uint32_t frame)
{
	const struct ft8xx_data *data = dev->data;

	const uint16_t cmd_size = sizeof(uint32_t) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(aoptr) +
				   sizeof(frame);

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;	 


	//bt816/7 only
	switch(data->chip_id) {

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

	if (aoptr % 64)
	{
		LOG_ERR("RAM block alignment, start address must be multiple of 64");
		return -ENOTSUP;		
	}	

	uint32_t ch_u = ((union { int32_t i; uint32_t u; }){ .i = ch }).u;

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_ANIMFRAMERAM);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,x);
	offset = ft8xx_append_block_int16(buffer,cmd_size,offset,y);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,aoptr);
	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,frame);

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer);

	}
	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

}


int ft8xx_copro_cmd_sync(const struct device *dev)
{

	const struct ft8xx_data *data = dev->data;
	
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

	uint32_t ch_u = ((union { int32_t i; uint32_t u; }){ .i = ch }).u;


	ft8xx_copro_cmd_uint(dev, CMD_SYNC,ch_u);
}

int ft8xx_copro_cmd_bitmap_transform(const struct device *dev, int32_t x0,
			int32_t y0,
			int32_t x1,
			int32_t y1,
			int32_t x2,
			int32_t y2,
			int32_t tx0,
			int32_t ty0,
			int32_t tx1,
			int32_t ty1,
			int32_t tx2,
			int32_t ty2,
			uint16_t *result)
{
	const struct ft8xx_config *config = dev->config;
	const struct ft8xx_data *data = dev->data;
	union ft8xx_bus *bus = config->bus;

	const uint16_t cmd_size = sizeof(uint32_t) + 
					sizeof(x0) +
					sizeof(y0) +
					sizeof(x1) +
					sizeof(y1) +
					sizeof(x2) +
					sizeof(y2) +
					sizeof(tx0) +
					sizeof(ty0) +
					sizeof(tx1) +
					sizeof(ty1) +
					sizeof(tx2) +
					sizeof(ty2) +
					sizeof(uint16_t) +
					sizeof(uint16_t); // pad to multiple of 4 bytes;

	uint8_t buffer[cmd_size]; 
	uint32_t offset = 0;		

	uint8_t read_data[data_size]; 

	//bt817/8 only
	switch(data->chip_id) {
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

	memset(buffer, 0, cmd_size);

	wait_cmd_freespace(dev, cmd_size);

	offset = ft8xx_append_block_uint32(buffer,cmd_size,offset,CMD_BITMAP_TRANSFORM);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,x0);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,y0);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,x1);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,y1);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,x2);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,y2);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,tx0);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,ty0);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,tx1);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,ty1);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,tx2);
	offset = ft8xx_append_block_int32(buffer,cmd_size,offset,ty2);
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,0);	
	offset = ft8xx_append_block_uint16(buffer,cmd_size,offset,0);	

	// use command append register where possible
	if (!data->memory_map->REG_CMDB_WRITE)
	{
		ft8xx_wrblock(bus, data->memory_map->REG_CMDB_WRITE, buffer, cmd_size);

	}
	else
	{
		ft8xx_wrblock(bus, data->memory_map->RAM_CMD + data->reg_cmd_write, buffer, cmd_size);
	}

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	increase_reg_cmd_write(dev, cmd_size);
	flush_reg_cmd_write(dev);

	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*result = ft8xx_rd16(dev, data->memory_map->RAM_CMD +(fifo_offset + (sizeof(uint32_t)*12 % 4096) )


}

int ft8xx_copro_cmd_testcard(const struct device *dev)
{

	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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

	ft8xx_copro_cmd(dev, CMD_TESTCARD);
}

int ft8xx_copro_cmd_wait(const struct device *dev, uint32_t us)
{

	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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

	ft8xx_copro_cmd_uint(dev, CMD_WAIT, us);
}

int ft8xx_copro_cmd_newlist(const struct device *dev, uint32_t a)
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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

	ft8xx_copro_cmd_uint(dev, CMD_NEWLIST, a);	
}

int ft8xx_copro_cmd_endlist(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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

	ft8xx_copro_cmd(dev, CMD_ENDLIST);	
}

int ft8xx_copro_cmd_calllist(const struct device *dev, uint32_t a)
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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

	ft8xx_copro_cmd_uint(dev, CMD_CALLLIST, a);	
}

int ft8xx_copro_cmd_return(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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

	ft8xx_copro_cmd(dev, CMD_RETURN);	
}

int ft8xx_copro_cmd_fontcache(const struct device *dev, uint32_t font,
			int32_t ptr,
			uint32_t num )
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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


	if (ptr % 64)
	{
		LOG_ERR("Bad block alignment, start address must be multiple of 64");
		return -ENOTSUP;		
	}


	if (num % 4)
	{
		LOG_ERR("Bad block alignment, must be multiple of 4");
		return -ENOTSUP;		
	}

	if (num >= (16 * 1024))
	{
		LOG_ERR("Bad cache size, must be at least 16 Kbytes");
		return -ENOTSUP;		
	}

	if 	((ptr+num) > data->memory_map->RAM_G_END))
	{
		LOG_ERR("Cache is Outside memory Range");
		return -ENOTSUP;			
	}		


	uint32_t ptr_u = ((union { int32_t i; uint32_t u; }){ .i = ptr }).u;

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_FONTCACHE,font, ptr_u, num);	
}

int ft8xx_copro_cmd_fontcachequery(const struct device *dev, uint32_t *total,
			int32_t *used)
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint_uint(dev, CMD_FONTCACHEQUERY, 0, 0);	


	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*total = ft8xx_rd16(dev, data->memory_map->RAM_CMD +(fifo_offset + (sizeof(uint32_t)) % 4096) )
	*used = ft8xx_rd16(dev, data->memory_map->RAM_CMD +(fifo_offset + ((sizeof(uint32_t)*2) % 4096) )


}

int ft8xx_copro_cmd_fontcachequery(const struct device *dev, uint32_t *source,
			int32_t *fmt,
			int32_t *w,
			int32_t *h,
			int32_t *palette)
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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

	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint_uint(dev, CMD_GETIMAGE, 0, 0);	


	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*source = ft8xx_rd16(dev, data->memory_map->RAM_CMD +(fifo_offset + (sizeof(uint32_t)) % 4096) )
	*fmt = ft8xx_rd16(dev, data->memory_map->RAM_CMD +(fifo_offset + ((sizeof(uint32_t))*2) % 4096) )
	*w = ft8xx_rd16(dev, data->memory_map->RAM_CMD +(fifo_offset + ((sizeof(uint32_t))*3) % 4096) )
	*h = ft8xx_rd16(dev, data->memory_map->RAM_CMD +(fifo_offset + ((sizeof(uint32_t))*4) % 4096) )
	*palette = ft8xx_rd16(dev, data->memory_map->RAM_CMD +(fifo_offset + ((sizeof(uint32_t))*5) % 4096) )


}


int ft8xx_copro_cmd_hsf(const struct device *dev, uint32_t w)
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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


	ft8xx_copro_cmd_uint(dev, CMD_HSF, w);	

}

int ft8xx_copro_cmd_pclkfreq(const struct device *dev, uint32_t ftarget,
			int32_t rounding,
			uint32_t factual )
{
	const struct ft8xx_data *data = dev->data;
	
	//bt81x only
	switch(data->chip_id) {
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


	uint16_t fifo_offset = ft8xx_rd16(bus,data->memory_map->REG_CMD_WRITE);

	ft8xx_copro_cmd_uint_uint_uint(dev, CMD_PCLKFREQ, ftarget,rounding,0);	


	while (ft8xx_rd16(bus,data->memory_map->REG_CMD_READ) != fifo_offset)
	{
		// sleep?
	}	

	*fmt = ft8xx_rd32(dev, data->memory_map->RAM_CMD +(fifo_offset + ((sizeof(uint32_t))*3) % 4096) )


}