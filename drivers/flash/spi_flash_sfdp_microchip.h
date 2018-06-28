/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_FLASH_SPI_FLASH_SFDP_MICROCHIP_H_
#define DRIVERS_FLASH_SPI_FLASH_SFDP_MICROCHIP_H_

#include "spi_flash_sfdp.h"

union sfdp_microchip_parameters {
	dword_t dwords[24];
	struct {
		/* Identification */
		dword_t manufacturer_id:8;
		dword_t memory_type:8;
		dword_t device_id:8;
		dword_t:8;

		/* Interface */
		dword_t interfaces_supported:3;
		dword_t supports_enable_quad:1;
		dword_t supports_hold_reset_function:3;
		dword_t supports_software_reset:1;
		dword_t supports_quad_reset:1;
		dword_t:2;
		dword_t byte_program_or_page_program:3;
		dword_t program_erase_suspend_supported:1;
		dword_t deep_power_down_mode_supported:1;
		dword_t otp_capable_supported:1;
		dword_t supports_block_group_protect:1;
		dword_t supports_independent_block_protect:1;
		dword_t supports_independent_non_volatile_lock:1;
		dword_t:4;
		dword_t:8;

		dword_t minimum_supply_voltage:16;
		dword_t maximum_supply_voltage:16;

		dword_t byte_program_typical_time:8;
		dword_t:8;
		dword_t page_program_typical_time:8;
		dword_t sector_or_block_erase_typical_time:8;

		dword_t chip_erase_typical_time:8;
		dword_t byte_program_max_time:8;
		dword_t:8;
		dword_t page_program_max_time:8;

		dword_t sector_or_block_erase_max_time:8;
		dword_t chip_erase_max_time:8;
		dword_t program_security_id_max_time:8;
		dword_t write_protection_enable_latency_max_time:8;

		dword_t write_suspend_latency_max_time:8;
		dword_t to_deep_power_down_max_time:8;
		dword_t deep_power_down_to_standby_mode_max_time:8;
		dword_t:8;

		dword_t:8;
		dword_t:8;
		dword_t:8;
		dword_t:8;

		/* Supported Instructions */
		dword_t nop_opcode:8;
		dword_t reset_enable_opcode:8;
		dword_t reset_memory_opcode:8;
		dword_t enable_quad_opcode:8;

		dword_t reset_quad_opcode:8;
		dword_t read_status_reg_opcode:8;
		dword_t write_status_reg_opcode:8;
		dword_t read_configuration_reg_opcode:8;

		dword_t write_enable_opcode:8;
		dword_t write_disable_opcode:8;
		dword_t byte_program_or_page_program_opcode:8;
		dword_t spi_quad_page_program_opcode:8;

		dword_t suspends_program_or_erase_opcode:8;
		dword_t resumes_program_or_erase_opcode:8;
		dword_t read_block_protection_reg_opcode:8;
		dword_t write_block_protection_reg_opcode:8;

		dword_t lock_down_block_protection_reg_opcode:8;
		dword_t non_volatile_write_lock_down_reg_opcode:8;
		dword_t global_block_protection_unlock_opcode:8;
		dword_t read_security_id_opcode:8;

		dword_t program_user_security_id_area_opcode:8;
		dword_t lockout_security_id_programming_opcode:8;
		dword_t set_burst_length_opcode:8;
		dword_t jedec_id_opcode:8;

		dword_t quad_j_id_opcode:8;
		dword_t sfdp_opcode:8;
		dword_t deep_power_down_mode_opcode:8;
		dword_t release_depp_power_down_opcode:8;

		dword_t read_burst_with_1_4_4_dummy_clocks:4;
		dword_t read_burst_with_1_4_4_mode_clocks:4;
		dword_t read_burst_with_1_4_4_opcode:8;
		dword_t read_burst_with_4_4_4_dummy_clocks:4;
		dword_t read_burst_with_4_4_4_mode_clocks:4;
		dword_t read_burst_with_4_4_4_opcode:8;

		dword_t read_1_1_1_dummy_clocks:4;
		dword_t read_1_1_1_mode_clocks:4;
		dword_t read_1_1_1_opcode:8;
		dword_t fast_read_1_1_1_dummy_clocks:4;
		dword_t fast_read_1_1_1_mode_clocks:4;
		dword_t fast_read_1_1_1_opcode:8;

		dword_t:8;
		dword_t:8;
		dword_t:8;
		dword_t:8;

		/* Security ID */
		dword_t security_id_size_in_bytes:16;
		dword_t:8;
		dword_t:8;

		/* Memory Organization/Block Protection Bit Mapping */
		struct {
			dword_t type:8;
			dword_t number:8;
			dword_t bit_start:8;
			dword_t bit_end:8;
		} sections[5];
	};
};

#define SFDP_MICROCHIP_ID 0x01bf /*  Microchip (Vendor) Parameter Table */

int spi_flash_sfdp_microchip(struct device *dev,
			     struct spi_flash_init_config *init_config,
			     u32_t addr, u8_t len);

#endif /* DRIVERS_FLASH_SPI_FLASH_SFDP_MICROCHIP_H_ */