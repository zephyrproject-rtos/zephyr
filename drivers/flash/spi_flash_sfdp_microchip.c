/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "SPI Flash"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_FLASH_LEVEL
#include <logging/sys_log.h>

#include "spi_flash_sfdp_microchip.h"
#include <errno.h>

static int spi_flash_microchip_write_protection_set(struct device *dev,
						    bool enable)
{
	struct spi_flash_data *const data = dev->driver_data;
	int r;

	if (!enable) {
		r = spi_flash_cmd(dev, data->write_protection_flag);
		if (r) {
			return r;
		}
		/* Here globally unlocks write protection and then uses software
		 * write protection
		 * Because there is no global locking operation
		 * Todo a full hardware write protection operation
		 */
		data->write_protection = NULL;
		data->write_protection_sw = enable;
	}
	return 0;
}

int spi_flash_sfdp_microchip(struct device *dev,
			     struct spi_flash_init_config *init_config,
			     u32_t addr, u8_t len)
{
	struct spi_flash_data *data = dev->driver_data;
	union sfdp_microchip_parameters mp;
	int r;

	if (len != ARRAY_SIZE(mp.dwords)) {
		SYS_LOG_ERR("Wrong microchip parameters size");
		return -ENODEV;
	}

	r = spi_flash_read_sfdp(dev, addr, &mp, len * 4);
	if (r != 0) {
		return r;
	}

	if (mp.manufacturer_id != 0xBF) {
		SYS_LOG_ERR("Wrong microchip manufacturer id");
		return -ENODEV;
	}

	if (mp.memory_type != 0x26) {
		SYS_LOG_ERR("Go to do about microchip memory type : %2x",
			    mp.memory_type);
		return -ENODEV;
	}

	if (mp.device_id != 0x42 && mp.device_id != 0x41) {
		SYS_LOG_WRN("Manufacturer : microchip, "
			    "memory type : %2x, "
			    "device id : %2x, "
			    "Unverified device may not work "
			    "properly",
			    mp.memory_type, mp.device_id);
	}

	data->config.frequency = MHZ(80);
	if (data->data_lines == 1) {
		if (mp.fast_read_1_1_1_opcode != SFDP_RESERVED_VALUE) {
			data->opcodes.read = mp.fast_read_1_1_1_opcode;
			data->dummy_clocks = mp.fast_read_1_1_1_dummy_clocks;
			data->mode_clocks = mp.fast_read_1_1_1_mode_clocks;
		} else if (mp.read_1_1_1_opcode != SFDP_RESERVED_VALUE) {
			data->opcodes.read = mp.read_1_1_1_opcode;
			data->dummy_clocks = mp.read_1_1_1_dummy_clocks;
			data->mode_clocks = mp.read_1_1_1_mode_clocks;
			data->config.frequency = MHZ(40);
		} else {
			SYS_LOG_ERR("Wrong read mode : 1-1-1");
			return -ENODEV;
		}
	}

	if (mp.global_block_protection_unlock_opcode != SFDP_RESERVED_VALUE) {
		data->write_protection =
		    spi_flash_microchip_write_protection_set;
		data->write_protection_flag =
		    mp.global_block_protection_unlock_opcode;
	}

	data->write_protection_sw = true;
	data->opcodes.program = mp.byte_program_or_page_program_opcode;

	return 0;
}