/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_RENESAS_RX_H_
#define ZEPHYR_DRIVERS_FLASH_RENESAS_RX_H_

#include <zephyr/drivers/flash.h>

enum flash_region {
	CODE_FLASH,
	DATA_FLASH,
};

struct flash_rx_config {
	struct flash_parameters flash_rx_parameters;
};

struct flash_rx_event {
	volatile bool erase_complete;
	volatile bool write_complete;
	volatile bool error;
};

struct flash_rx_data {
	struct flash_rx_event flash_event;
	enum flash_region FlashRegion;
	uint32_t area_address;
	uint32_t area_size;
	struct k_sem transfer_sem;
};

#endif /* ZEPHYR_DRIVERS_FLASH_RENESAS_RX_H_ */
