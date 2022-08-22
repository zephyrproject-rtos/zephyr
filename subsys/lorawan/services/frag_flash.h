/*
 * Copyright (c) 2022 tado GmbH
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_SERVICES_FRAG_FLASH_H_
#define ZEPHYR_SUBSYS_LORAWAN_SERVICES_FRAG_FLASH_H_

#include <stdint.h>

/**
 * Initialize flash driver and prepare partition for new firmware image.
 *
 * This function mass-erases the flash partition and may take a while to return.
 *
 * @returns 0 for success, otherwise negative error code
 */
int frag_flash_init(void);

/**
 * Write received data fragment to flash
 *
 * This function is called by FragDecoder from LoRaMAC-node stack.
 *
 * @param addr Flash address relative to start of slot
 * @param data Data buffer
 * @param size Number of bytes in the buffer
 *
 * @returns 0 for success, otherwise negative error code
 */
int8_t frag_flash_write(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * Read back data from flash
 *
 * This function is called by FragDecoder from LoRaMAC-node stack.
 *
 * @param addr Flash address relative to start of slot
 * @param data Data buffer
 * @param size Number of bytes in the buffer
 *
 * @returns 0 for success, otherwise negative error code
 */
int8_t frag_flash_read(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * Finalize flashing after sufficient fragments have been received.
 *
 * After this call the new firmware is ready to be checked and booted.
 */
void frag_flash_finish(void);

#endif /* ZEPHYR_SUBSYS_LORAWAN_SERVICES_FRAG_FLASH_H_ */
