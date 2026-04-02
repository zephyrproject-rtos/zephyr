/*
 * Copyright (c) 2022-2024 Libre Solar Technologies GmbH
 * Copyright (c) 2022-2024 tado GmbH
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
 * @param frag_size Fragment size used for this session
 *
 * @returns 0 for success, otherwise negative error code
 */
int frag_flash_init(uint32_t frag_size);

/**
 * Write received data fragment to flash.
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
 * Read back data from flash.
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
 * Start caching fragments in RAM.
 *
 * Coded/redundant fragments may be overwritten with future fragments,
 * so we have to cache them in RAM instead of flash.
 *
 * This function must be called once all uncoded fragments have been received.
 */
void frag_flash_use_cache(void);

/**
 * Finalize flashing after sufficient fragments have been received.
 *
 * This call will also write cached fragments to flash.
 *
 * After this call the new firmware is ready to be checked and booted.
 */
void frag_flash_finish(void);

#endif /* ZEPHYR_SUBSYS_LORAWAN_SERVICES_FRAG_FLASH_H_ */
