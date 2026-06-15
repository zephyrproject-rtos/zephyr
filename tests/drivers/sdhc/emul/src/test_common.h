/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <zephyr/drivers/sdhc.h>

/* eMMC EXT_CSD byte offsets (JEDEC JESD84-B51) */
#define EXT_CSD_SEC_COUNT	212
#define EXT_CSD_BUS_WIDTH	183
#define EXT_CSD_HS_TIMING	185
#define EXT_CSD_STRUCTURE	192

/**
 * @brief Bring the emulated card to TRANSFER state.
 *
 * Auto-detects SD vs eMMC personality and runs the correct init sequence.
 * Returns the RCA assigned by CMD3.
 */
uint32_t sdhc_init_to_transfer(const struct device *dev);

/**
 * @brief Query card state via CMD13.
 * @return State nibble (0-15) or negative errno on failure.
 */
int sdhc_get_card_state(const struct device *dev, uint32_t rca);

/**
 * @brief Send CMD55 then an application-specific command.
 */
int sdhc_acmd(const struct device *dev, uint32_t rca,
	      uint8_t opcode, uint32_t arg,
	      struct sdhc_command *cmd_out,
	      struct sdhc_data *data);

/**
 * @brief High-level block write helper.
 * @return 0 on success, negative errno otherwise.
 */
int sdhc_write_blocks(const struct device *dev, uint32_t block_addr,
		      uint8_t *buf, uint32_t n_blocks, size_t *bytes_xfered);

/**
 * @brief High-level block read helper.
 * @return 0 on success, negative errno otherwise.
 */
int sdhc_read_blocks(const struct device *dev, uint32_t block_addr,
		     uint8_t *buf, uint32_t n_blocks, size_t *bytes_xfered);

#endif
