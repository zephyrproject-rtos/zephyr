/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public test-accessor API for the SDHC emulator driver.
 *
 * These functions allow test code to manipulate the internal state
 * of the emulated SDHC host controller (fault injection, card
 * presence, SDIO IRQ triggering).  They are not part of the
 * stable Zephyr API and are intended only for ztest use.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SDHC_SDHC_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SDHC_SDHC_EMUL_H_

#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set fault injection on a specific CMD index.
 *
 * When a command with opcode == @p cmd_index is received by the
 * emulator, it will return -EIO instead of processing the command.
 *
 * @param dev    SDHC emulator device.
 * @param cmd_index  CMD opcode to inject the fault on (0-63).
 */
void sdhc_emul_set_fault(const struct device *dev, uint8_t cmd_index);

/**
 * @brief Clear any active fault injection.
 *
 * Restores normal operation so that all commands are processed.
 *
 * @param dev  SDHC emulator device.
 */
void sdhc_emul_clear_fault(const struct device *dev);

/**
 * @brief Set the virtual card presence state.
 *
 * @param dev      SDHC emulator device.
 * @param present  true if the card should appear inserted.
 */
void sdhc_emul_set_card_present(const struct device *dev, bool present);

/**
 * @brief Trigger an SDIO interrupt for a given function.
 *
 * Sets the interrupt pending bit for SDIO function @p fn in the
 * CCCR and fires the registered interrupt callback.
 *
 * @param dev  SDHC emulator device.
 * @param fn   SDIO function number (1-7).
 */
void sdhc_emul_trigger_sdio_irq(const struct device *dev, uint8_t fn);

/**
 * @brief Get a pointer to the emulated card storage.
 *
 * Useful for test verification of written data without issuing
 * additional read commands.
 *
 * @param dev  SDHC emulator device.
 *
 * @return Pointer to the start of the block storage buffer.
 */
uint8_t *sdhc_emul_get_storage(const struct device *dev);

/**
 * @brief Get the number of blocks in the emulated card.
 *
 * @param dev  SDHC emulator device.
 *
 * @return Number of blocks.
 */
uint32_t sdhc_emul_get_block_count(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SDHC_SDHC_EMUL_H_ */
