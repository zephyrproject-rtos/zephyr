/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_FLASH_SPI_FLASH_SFDP_H_
#define DRIVERS_FLASH_SPI_FLASH_SFDP_H_

#include <flash.h>
#include <gpio.h>
#include <misc/util.h>
#include <spi.h>

#ifndef CONFIG_SPI_FLASH_SMRP_ARRAY_SIZE
#define CONFIG_SPI_FLASH_SMRP_ARRAY_SIZE 8
#endif
#ifndef CONFIG_SPI_FLASH_LAYOUTS_ARRAY_SIZE
#define CONFIG_SPI_FLASH_LAYOUTS_ARRAY_SIZE 1
#endif
#ifndef CONFIG_SPI_FLASH_SMPT_SIZE
#define CONFIG_SPI_FLASH_SMPT_SIZE 16
#endif

typedef u32_t dword_t;

union sfdp_header {
	dword_t dwords[2];
	struct {
		dword_t signature;

		dword_t minor_ver:8;
		dword_t major_ver:8;
		dword_t nph:8;
		dword_t:8;
	};
};

union sfdp_parameter_header {
	dword_t dwords[2];
	struct {
		dword_t id_lsb:8;
		dword_t minor_ver:8;
		dword_t major_ver:8;
		dword_t length:8;

		dword_t addr:24;
		dword_t id_msb:8;
	};
};

struct sfdp_erase_type {
	u8_t size;
	u8_t opcode;
};

union sfdp_basic_flash_parameters {
	dword_t dwords[16];
	struct {
		dword_t block_erase_sizes:2;
		dword_t write_granularity:1;
		dword_t volatile_status_register:1;
		dword_t write_enable_opcode_select:1;
		dword_t:3;
		dword_t opcode_erase_4k:8;
		dword_t support_1_1_2_fast_read:1;
		dword_t addr_bytes:2;
		dword_t support_dtr:1;
		dword_t support_1_2_2_fast_read:1;
		dword_t support_1_4_4_fast_read:1;
		dword_t support_1_1_4_fast_read:1;
		dword_t:9;
		/* 1st dword */

		dword_t density;
		/* 2nd dword */

		dword_t fast_read_1_4_4_dummy_clocks:4;
		dword_t fast_read_1_4_4_mode_clocks:4;
		dword_t fast_read_1_4_4_opcode:8;

		dword_t fast_read_1_1_4_dummy_clocks:4;
		dword_t fast_read_1_1_4_mode_clocks:4;
		dword_t fast_read_1_1_4_opcode:8;
		/* 3th dword */

		dword_t fast_read_1_1_2_dummy_clocks:4;
		dword_t fast_read_1_1_2_mode_clocks:4;
		dword_t fast_read_1_1_2_opcode:8;

		dword_t fast_read_1_2_2_dummy_clocks:4;
		dword_t fast_read_1_2_2_mode_clocks:4;
		dword_t fast_read_1_2_2_opcode:8;
		/* 4th dword */

		dword_t support_2_2_2_fast_read:1;
		dword_t:3;
		dword_t support_4_4_4_fast_read:1;
		dword_t:27;
		/* 5th dword */

		dword_t:16;
		dword_t fast_read_2_2_2_dummy_clocks:4;
		dword_t fast_read_2_2_2_mode_clocks:4;
		dword_t fast_read_2_2_2_opcode:8;
		/* 6th dword */

		dword_t:16;
		dword_t fast_read_4_4_4_dummy_clocks:4;
		dword_t fast_read_4_4_4_mode_clocks:4;
		dword_t fast_read_4_4_4_opcode:8;
		/* 7th dword */

		union {
			struct sfdp_erase_type erase_types[4];
			struct {
				dword_t erase_type_1_size:8;
				dword_t erase_type_1_opcode:8;
				dword_t erase_type_2_size:8;
				dword_t erase_type_2_opcode:8;
				/* 8th dword */

				dword_t erase_type_3_size:8;
				dword_t erase_type_3_opcode:8;
				dword_t erase_type_4_size:8;
				dword_t erase_type_4_opcode:8;
				/* 9th dword */
			};
		};

		dword_t erase_mult:4;
		dword_t erase_type_1_typical_time:7;
		dword_t erase_type_2_typical_time:7;
		dword_t erase_type_3_typical_time:7;
		dword_t erase_type_4_typical_time:7;
		/* 10th dword */

		dword_t page_program_mult:4;
		dword_t page_size:4;
		dword_t page_program_time:6;
		dword_t first_byte_program_time:5;
		dword_t additional_byte_program_time:5;
		dword_t chip_erase_time:7;
		dword_t:1;
		/* 11th dword */

		dword_t program_suspend_prohibited_operations:4;
		dword_t erase_suspend_prohibited_operations:4;
		dword_t:1;
		dword_t program_resume_to_suspend_interval:4;
		dword_t suspend_program_max_latency:7;
		dword_t erase_resume_to_suspend_interval:4;
		dword_t suspend_erase_max_latency:7;
		dword_t support_suspend_and_resume:1;
		/* 12th dword */

		dword_t program_resume_opcode:8;
		dword_t program_suspend_opcode:8;
		dword_t erase_resume_opcode:8;
		dword_t erase_suspend_opcode:8;
		/* 13th dword */

		dword_t:2;
		dword_t polling_device_busy:6;
		dword_t exit_deep_powerdown_time:7;
		dword_t exit_deep_powerdown_opcode:8;
		dword_t enter_deep_powerdown_opcode:8;
		dword_t deep_powerdown:1;
		/* 14th dword */

		dword_t disable_4_4_4_mode_sequences:4;
		dword_t enable_4_4_4_mode_sequences:5;
		dword_t support_0_4_4_mode:1;
		dword_t exit_0_4_4_mode_method:6;
		dword_t entry_0_4_4_mode_method:4;
		dword_t qer:3;
		dword_t hold_or_reset_disable:1;
		dword_t:8;
		/* 15th dword */

		dword_t write_enable_sequences:7;
		dword_t:1;
		dword_t soft_reset_and_rescue_sequence_support:6;
		dword_t exit_4_byte_addressing:10;
		dword_t enter_4_byte_addressing:8;
		/* 16th dword */
	};
};

union sfdp_sector_map_parameters {
	dword_t dwords[1];
	struct {
		dword_t sequence_end:1;
		dword_t type:1;
	};

	struct {
		dword_t:8;
		dword_t command:8;
		dword_t read_latency_clocks:4;
		dword_t:2;
		dword_t address_length:2;
		dword_t read_data_mask:8;
	} command;

	struct {
		dword_t:8;
		dword_t id:8;
		dword_t region_count:8;
		dword_t:8;
	} map;
};

union sfdp_sector_map_region_parameters {
	dword_t dwords[1];
	struct {
		dword_t erase_type_1:1;
		dword_t erase_type_2:1;
		dword_t erase_type_3:1;
		dword_t erase_type_4:1;
		dword_t:4;
		dword_t region_size:24;
	};
};

#define SFDP_BFPT_ID 0xff00       /* Basic Flash Parameter Table */
#define SFDP_SECTOR_MAP_ID 0xff81 /* Sector Map Table */
#define SFDP_RPMC_ID 0xff03      /* Replay Protected Monotonic Counters Table */
#define SFDP_FOUR_ADDR_ID 0xff84 /* 4-byte Address Instruction Table */

#define CMD_READ_SFDP 0x5A
#define CMD_CHIP_ERASE 0xC7
#define SFDP_RESERVED_VALUE 0xff

struct spi_flash_data {
	struct device *spi;
	struct spi_config *tem_config;
	struct spi_config config;
#if defined(CONFIG_SPI_FLASH_GPIO_SPI_CS)
	struct spi_cs_control cs;
#endif
	struct k_sem sem;

	u32_t dummy_clocks:4;
	u32_t mode_clocks:4;
	u32_t data_lines:3;
	u32_t address_lines:3;
	u32_t instruction_lines:3;
	u32_t lines:3;
	u32_t status_busy_bit:3;
	u32_t status_busy:1;
	u32_t four_addr:1;
	u32_t quad_enable:1;
	u32_t write_protection_sw:1;

	struct {
		u8_t read;
		u8_t read_status;
		u8_t block_4k_erase;
		u8_t write_enable;
		u8_t program;
	} opcodes;
	struct sfdp_erase_type erase_types[4];

	flash_api_write_protection write_protection;
	u32_t write_protection_flag;

	u64_t flash_size;
	u32_t page_size;

	u8_t smrp_count;
	union sfdp_sector_map_region_parameters
	    smrp[CONFIG_SPI_FLASH_SMRP_ARRAY_SIZE];

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout
	    pages_layouts[CONFIG_SPI_FLASH_LAYOUTS_ARRAY_SIZE];
	size_t pages_layouts_count;
#endif
};

struct spi_flash_init_config {
	dword_t smpt[CONFIG_SPI_FLASH_SMPT_SIZE];
};

int spi_flash_read_sfdp(struct device *dev, uint32_t addr, void *ptr,
			size_t len);
int spi_flash_cmd(struct device *dev, u8_t cmd_id);

#endif /* DRIVERS_FLASH_SPI_FLASH_SFDP_H_ */