/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SD subsystem
 */

#ifndef ZEPHYR_INCLUDE_SD_SD_H_
#define ZEPHYR_INCLUDE_SD_SD_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief card status. Used interally by subsystem.
 */
enum card_status {
	CARD_UNINITIALIZED = 0, /*!< card has not been initialized */
	CARD_ERROR = 1, /*!< card state is error */
	CARD_INITIALIZED = 2, /*!< card is in valid state */
};

/**
 * @brief card type. Used interally by subsystem.
 */
enum card_type {
	CARD_SDMMC = 0, /*!< SD memory card */
	CARD_SDIO = 1, /*!< SD I/O card */
	CARD_COMBO = 2, /*!< SD memory and I/O card */
};


/**
 * @brief SD card structure
 *
 * This structure is used by the subsystem to track an individual SD
 * device connected to the system. The application may access these
 * fields, but use caution when changing values.
 */
struct sd_card {
	const struct device *sdhc; /*!< SD host controller for card */
	struct sdhc_io bus_io; /*!< Current bus I/O props for SDHC */
	enum sd_voltage card_voltage; /*!< Card signal voltage */
	struct k_mutex lock; /*!< card mutex */
	struct sdhc_host_props host_props; /*!< SDHC host properties */
	uint32_t ocr; /*!< Raw card OCR content */
	struct sd_switch_caps switch_caps; /*!< SD switch capabilities */
	uint32_t num_io; /*!< I/O function count. 0 for SD cards */
	uint32_t relative_addr; /*!< Card relative address */
	uint32_t block_count; /*!< Number of blocks in SD card */
	uint32_t block_size; /*!< SD block size */
	uint32_t sd_version; /*!< SD specification version */
	enum sd_timing_mode card_speed; /*!< Card timing mode */
	enum card_status status; /*!< Card status */
	enum card_type type; /*!< Card type */
	uint32_t flags; /*!< Card flags */
	uint8_t card_buffer[CONFIG_SD_BUFFER_SIZE]
		__aligned(CONFIG_SDHC_BUFFER_ALIGNMENT); /* Card internal buffer */
};

/**
 * @brief Initialize an SD device
 *
 * Initializes an SD device to use with the subsystem. After this call,
 * only the SD card structure is required to access the card.
 * @param sdhc_dev SD host controller device for this card
 * @param card SD card structure for this card
 * @retval 0 card was initialized
 * @retval -ETIMEDOUT: card initialization timed out
 * @retval -EBUSY: card is busy
 * @retval -EIO: IO error while starting card
 */
int sd_init(const struct device *sdhc_dev, struct sd_card *card);

/**
 * @brief checks to see if card is present in the SD slot
 *
 * @param sdhc_dev SD host controller to check for card presence on
 * @retval true card is present
 * @retval false card is not present
 */
bool sd_is_card_present(const struct device *sdhc_dev);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SD_SD_H_ */
