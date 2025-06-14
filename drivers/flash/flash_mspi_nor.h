/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright (c) 2025 Tenstorrent AI ULC
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

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define WITH_RESET_GPIO 1
#endif

/* Flash chip specific quirks */
struct flash_mspi_nor_quirks {
	/* Called after switching to default IO mode. */
	int (*post_switch_mode)(const struct device *dev);
};

/* Flash data. Stored in ROM unless CONFIG_FLASH_MSPI_NOR_RUNTIME_PROBE is enabled */
struct flash_mspi_device_data {
	const struct flash_mspi_nor_cmds *jedec_cmds;
	const struct flash_mspi_nor_quirks *quirks;
	struct mspi_dev_cfg dev_cfg;
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];
	uint8_t dw15_qer;
	uint32_t flash_size;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

struct flash_mspi_nor_config {
	const struct device *bus;
	struct mspi_dev_id mspi_id;
	struct mspi_dev_cfg mspi_nor_init_cfg;
	enum mspi_dev_cfg_mask mspi_nor_cfg_mask;
#if defined(CONFIG_MSPI_XIP)
	struct mspi_xip_cfg xip_cfg;
#endif
#if defined(WITH_RESET_GPIO)
	struct gpio_dt_spec reset;
	uint32_t reset_pulse_us;
	uint32_t reset_recovery_us;
#endif
#if !defined(CONFIG_FLASH_MSPI_NOR_RUNTIME_PROBE)
	struct flash_mspi_device_data flash_data;
#endif
};

#if defined(CONFIG_FLASH_MSPI_NOR_RUNTIME_PROBE)
#define FLASH_DATA(dev) (((struct flash_mspi_nor_data *)(dev->data))->flash_data)
#else
#define FLASH_DATA(dev) (((const struct flash_mspi_nor_config *)(dev->config))->flash_data)
#endif

struct flash_mspi_nor_data {
	struct k_sem acquired;
	struct mspi_xfer_packet packet;
	struct mspi_xfer xfer;
	struct mspi_dev_cfg *curr_cfg;
#if defined(CONFIG_FLASH_MSPI_NOR_RUNTIME_PROBE)
	struct flash_mspi_device_data flash_data;
#endif
};

struct flash_mspi_nor_cmd {
	enum mspi_xfer_direction dir;
	uint32_t cmd;
	uint16_t tx_dummy;
	uint16_t rx_dummy;
	uint8_t cmd_length;
	uint8_t addr_length;
	bool force_single;
};

struct flash_mspi_nor_cmds {
	struct flash_mspi_nor_cmd id;
	struct flash_mspi_nor_cmd write_en;
	struct flash_mspi_nor_cmd read;
	struct flash_mspi_nor_cmd status;
	struct flash_mspi_nor_cmd config;
	struct flash_mspi_nor_cmd page_program;
	struct flash_mspi_nor_cmd sector_erase;
	struct flash_mspi_nor_cmd chip_erase;
	struct flash_mspi_nor_cmd sfdp;
};

struct flash_mspi_nor_devs {
	uint8_t jedec_id[JESD216_READ_ID_LEN];
	const struct mspi_dev_cfg dev_cfg;
	const struct flash_mspi_nor_cmds jedec_cmds;
	const struct flash_mspi_nor_quirks quirks;
	uint8_t dw15_qer;
	uint32_t flash_size;
	uint32_t page_size;
};

extern struct flash_mspi_nor_devs mspi_nor_devs[];
extern const uint32_t mspi_nor_devs_count;
extern const struct flash_mspi_nor_cmds commands_single;
extern const struct flash_mspi_nor_cmds commands_quad;
extern const struct flash_mspi_nor_cmds commands_octal;

void flash_mspi_command_set(const struct device *dev, const struct flash_mspi_nor_cmd *cmd);

#ifdef __cplusplus
}
#endif

#endif /*__FLASH_MSPI_NOR_H__*/
