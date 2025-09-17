/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SOC_FLASH_RENESAS_RX_H_
#define ZEPHYR_DRIVERS_SOC_FLASH_RENESAS_RX_H_

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
	/* Indicates which flash area is being accessed (code or data region). */
	enum flash_region FlashRegion;
	/**
	 * Flash address of FlashRegion
	 * Renesas RX supports two flash regions: CODE and DATA. These regions are
	 * located at different memory addresses and have distinct flash maps.
	 * This field identifies which FlashRegion is in use, allowing
	 * region-specific behavior to be applied
	 */
	uint32_t area_address;
	/* Size of the FlashRegion. */
	uint32_t area_size;
	struct k_sem transfer_sem;
};

#endif /* ZEPHYR_DRIVERS_SOC_FLASH_RENESAS_RX_H_ */
