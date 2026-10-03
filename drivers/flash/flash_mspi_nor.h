/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_MSPI_NOR_H__
#define __FLASH_MSPI_NOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include "jesd216.h"
#include "spi_nor.h"

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(supply_gpios)
#define WITH_SUPPLY_GPIO 1
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define WITH_RESET_GPIO 1
#endif
#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(initial_soft_reset)
#define WITH_SOFT_RESET 1
#endif
#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(has_dpd)
#define WITH_DPD 1
#endif
#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_runtime_sfdp)
#define WITH_RUNTIME_SFDP 1
#endif

#define CMD_EXTENSION_NONE    0
#define CMD_EXTENSION_SAME    1
#define CMD_EXTENSION_INVERSE 2

#define OCTAL_ENABLE_REQ_NONE 0
#define OCTAL_ENABLE_REQ_S2B3 1

#define ENTER_4BYTE_ADDR_NONE  0
#define ENTER_4BYTE_ADDR_B7    1
#define ENTER_4BYTE_ADDR_06_B7 2

#define USES_4BYTE_ADDR(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_OCTAL || \
	 DT_INST_PROP(inst, use_4byte_addressing))

#define DEFAULT_CMD_INFO(inst) { \
	.pp_cmd = USES_4BYTE_ADDR(inst) \
		? SPI_NOR_CMD_PP_4B \
		: SPI_NOR_CMD_PP, \
	.read_cmd = USES_4BYTE_ADDR(inst) \
		  ? SPI_NOR_CMD_READ_FAST_4B \
		  : SPI_NOR_CMD_READ_FAST, \
	.read_mode_bit_cycles = 0, \
	.read_dummy_cycles = 8, \
	.uses_4byte_addr = USES_4BYTE_ADDR(inst), \
	.cmd_extension = CMD_EXTENSION_NONE, \
	.sfdp_addr_4 = false, \
	.sfdp_dummy_20 = false, \
	.rdsr_addr_4 = false, \
	.rdsr_dummy = 0, \
	.rdid_addr_4 = false, \
	.rdid_dummy = 0, }

#define DEFAULT_ERASE_TYPES_DEFINE(inst) \
	static const struct jesd216_erase_type \
	dev##inst##_erase_types[JESD216_NUM_ERASE_TYPES] = \
		{{ .cmd = SPI_NOR_CMD_SE, \
		   .exp = 0x0C }}; \
	static const struct jesd216_erase_type \
	dev##inst##_erase_types_4b[JESD216_NUM_ERASE_TYPES] = \
		{{ .cmd = SPI_NOR_CMD_SE_4B, \
		   .exp = 0x0C }}

#define DEFAULT_ERASE_TYPES(inst) \
	USES_4BYTE_ADDR(inst) ? dev##inst##_erase_types_4b \
			      : dev##inst##_erase_types

#define DEFAULT_SWITCH_INFO(inst) { \
	.quad_enable_req = DT_INST_ENUM_IDX_OR(inst, quad_enable_requirements, \
					       JESD216_DW15_QER_VAL_NONE), \
	.octal_enable_req = OCTAL_ENABLE_REQ_NONE, \
	.enter_4byte_addr = ENTER_4BYTE_ADDR_NONE }

#define FLASH_SIZE_INST(inst) (DT_INST_PROP_OR(inst, size, 0) / 8)

#define FLASH_PAGE_SIZE_INST(inst) \
	DT_INST_PROP_OR(inst, page_size, \
		COND_CODE_1(DT_INST_PROP(inst, use_runtime_sfdp), \
			(0), \
			(SPI_NOR_PAGE_SIZE)))

struct flash_mspi_nor_cmd_info {
	uint8_t read_cmd;
	uint8_t read_mode_bit_cycles : 3;
	uint8_t read_dummy_cycles    : 5;
	uint8_t pp_cmd;
	bool    uses_4byte_addr : 1;
	/* BFP, 18th DWORD, bits 30-29 */
	uint8_t cmd_extension   : 2;
	/* xSPI Profile 1.0 (ID FF05), 1st DWORD: */
	/* - Read SFDP command address bytes: 4 (true) or 3 */
	bool    sfdp_addr_4     : 1;
	/* - Read SDFP command dummy cycles: 20 (true) or 8 */
	bool    sfdp_dummy_20   : 1;
	/* - Read Status Register command address bytes: 4 (true) or 0 */
	bool    rdsr_addr_4     : 1;
	/* - Read Status Register command dummy cycles: 0, 4, or 8 */
	uint8_t rdsr_dummy      : 4;
	/* - Read JEDEC ID command parameters; not sure where to get their
	 *   values from, but since for many flash chips they are the same
	 *   as for RDSR, those are taken as defaults, see DEFAULT_CMD_INFO()
	 */
	bool    rdid_addr_4     : 1;
	uint8_t rdid_dummy      : 4;
};

struct flash_mspi_nor_switch_info {
	uint8_t quad_enable_req  : 3;
	uint8_t octal_enable_req : 3;
	uint8_t enter_4byte_addr : 2;
};

struct flash_mspi_nor_config {
	const struct device *bus;
	uint32_t packet_data_limit;
	uint32_t flash_size;
	uint32_t erase_block_size;
	uint16_t page_size;
	struct mspi_dev_id mspi_id;
	struct mspi_dev_cfg mspi_nor_cfg;
	struct mspi_dev_cfg mspi_control_cfg;
#if defined(CONFIG_MSPI_MEMMAP)
	struct mspi_memmap_cfg memmap_cfg;
#endif
#if defined(WITH_SUPPLY_GPIO)
	struct gpio_dt_spec supply;
#endif
#if defined(WITH_RESET_GPIO)
	struct gpio_dt_spec reset;
	uint32_t reset_pulse_us;
#endif
	uint32_t reset_recovery_us;
	uint32_t transfer_timeout;
#if defined(WITH_DPD)
	uint32_t t_enter_dpd_us;
	uint32_t t_exit_dpd_us;
	uint32_t t_dpdd_us;
#endif
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];
	struct flash_mspi_nor_quirks *quirks;
	const struct jesd216_erase_type *default_erase_types;
	struct flash_mspi_nor_cmd_info default_cmd_info;
	struct flash_mspi_nor_switch_info default_switch_info;
	uint32_t read_freq;
	enum mspi_io_mode read_io_mode;
	uint32_t write_freq;
	enum mspi_io_mode write_io_mode;
	bool jedec_id_specified  : 1;
	bool rx_dummy_specified  : 1;
	bool multiperipheral_bus : 1;
	bool multi_io_cmd        : 1;
	bool single_io_addr      : 1;
	bool initial_soft_reset  : 1;
	bool has_dpd             : 1;
	bool uses_runtime_sfdp   : 1;
	enum mspi_xfer_mode control_xfer_mode;
	enum mspi_xfer_mode data_xfer_mode;
};

struct flash_mspi_nor_data {
#if defined(CONFIG_MULTITHREADING)
	struct k_sem acquired;
#endif
	struct mspi_xfer_packet packet;
	struct mspi_xfer xfer;
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	struct flash_mspi_nor_cmd_info cmd_info;
	struct flash_mspi_nor_switch_info switch_info;
	const struct mspi_dev_cfg *last_applied_cfg;
#if defined(WITH_DPD)
	uint32_t enter_dpd_cycle;
#endif
#if defined(WITH_RUNTIME_SFDP)
	uint32_t flash_size;
	uint16_t page_size;
#endif
#if defined(WITH_RUNTIME_SFDP) && defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	bool chip_initialized;
	const struct mspi_dev_cfg *read_cfg;
	struct mspi_dev_cfg mspi_dev_read_cfg;
	const struct mspi_dev_cfg *write_cfg;
	struct mspi_dev_cfg mspi_dev_write_cfg;
};

#ifdef __cplusplus
}
#endif

#endif /*__FLASH_MSPI_NOR_H__*/
