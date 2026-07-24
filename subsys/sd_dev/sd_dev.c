/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SD Device Core Implementation
 *
 * This file implements the core SD device subsystem including heap
 * management, device initialization, and enumeration detection.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/drivers/sd_dev.h>
#include <zephyr/sys/time_units.h>

LOG_MODULE_REGISTER(sd_dev, LOG_LEVEL_INF);

/**
 * @name SD Device Heap Management
 * @{
 */

/** SD device heap buffer */
static uint8_t sd_dev_heap_buf[SDEV_HEAP_SIZE];

/** SD device heap instance */
static struct k_heap sd_dev_heap;

/**
 * @brief Initialize SD device heap
 *
 * This function initializes the heap used for SD device allocations.
 */
static inline void sd_dev_heap_init(void)
{
	k_heap_init(&sd_dev_heap, sd_dev_heap_buf, sizeof(sd_dev_heap_buf));
}

void *sd_dev_heap_alloc(size_t size)
{
	return k_heap_alloc(&sd_dev_heap, size, K_NO_WAIT);
}

void sd_dev_heap_free(void *ptr)
{
	k_heap_free(&sd_dev_heap, ptr);
}

/**
 * @}
 */

/**
 * @name SD Device Notification
 * @{
 */

void sd_dev_notify_host_ready(struct sd_dev_card *card)
{
	const struct device *dev = card->dev;

	sd_dev_set_dev_ready(dev);
}

/**
 * @}
 */

/**
 * @name SD Device Enumeration
 * @{
 */

bool sd_dev_card_is_enumed(struct sd_dev_card *card)
{
#ifdef CONFIG_SDIO_DEV
	if (card->card_type == SDIO_DEVICE_CARD) {
		return sdio_dev_is_enumed(card->sdio);
	}
#endif
	return 0;
}

/**
 * @}
 */

/**
 * @name SD Device Initialization
 * @{
 */

int sd_dev_init(const struct device *dev, struct sd_dev_card **scard)
{
	int ret = 0;
	struct sd_dev_card *card = 0;
	const struct sd_dev_cfg *cfg = 0;

	if (!dev) {
		LOG_ERR("Invalid param: card=%p", card);
		return -EINVAL;
	}

	cfg = sd_dev_get_config(dev);

	sd_dev_heap_init();

	card = sd_dev_heap_alloc(sizeof(struct sd_dev_card));
	if (!card) {
		LOG_WRN("sd_dev card alloc memory fail");
		return -ENOMEM;
	}

	card->is_enum = false;
	card->dev = dev;
	card->card_type = cfg->card_type;

	switch (cfg->card_type) {
	case SD_DEVICE_CARD:
		LOG_INF("SD card init not implemented");
		break;
#ifdef CONFIG_SDIO_DEV
	case SDIO_DEVICE_CARD: {
		struct sdio_dev *sdio = 0;
		const struct sdio_dev_cfg *sdio_cfg = &cfg->sdio_cfg;

		sdio = sd_dev_heap_alloc(sizeof(struct sdio_dev));
		if (!sdio) {
			LOG_ERR("sdio alloc failed");
			return -ENOMEM;
		}

		memset(sdio, 0, sizeof(*sdio));

		card->sdio = sdio;
		sdio->card = card;

		ret = sdio_dev_init(sdio, sdio_cfg);
		if (ret) {
			LOG_ERR("sdio_dev_init failed (%d)", ret);
			sd_dev_heap_free(sdio);
			card->sdio = NULL;
			return ret;
		}

		if (card->sdio == NULL) {
			LOG_ERR("%s: sdio is NULL after init", dev->name);
			return -EFAULT;
		}
		break;
	}
#endif /* CONFIG_SDIO_DEV */
	case MMC_DEVICE_CARD:
		LOG_ERR("MMC stack not enabled");
		return -ENOTSUP;

	default:
		LOG_ERR("Unknown SD card type: %d", cfg->card_type);
		return -EINVAL;
	}

	sd_dev_register_rx_cb(dev, sd_dev_rx_dispatch);

	sd_dev_notify_host_ready(card);

	sd_dev_set_state(dev, SDEV_DEVICE_INIT_DONE);

	*scard = card;

	sd_dev_set_card(dev, card);

	return ret;
}

/**
 * @brief Associate an SD card object with a device.
 *
 * Stores the specified SD card pointer in the device private data.
 *
 * @param dev Pointer to the device.
 * @param card Pointer to the SD card object to associate with the device.
 */
void sd_dev_set_card(const struct device *dev, struct sd_dev_card *card)
{
	*(struct sd_dev_card **)dev->data = card;
}

/**
 * @brief Get the SD card object associated with a device.
 *
 * Retrieves the SD card pointer previously associated with the device.
 *
 * @param dev Pointer to the device.
 *
 * @return Pointer to the associated SD card object, or NULL if no card
 *         has been associated with the device.
 */
struct sd_dev_card *sd_dev_get_card(const struct device *dev)
{
	return *(struct sd_dev_card **)dev->data;
}

/**
 * @brief Deinitialize SDEV card device
 *
 * This function deinitializes the SDEV card device by cleaning up all
 * allocated resources, unregistering callbacks, and freeing memory based
 * on the card type (SD, SDIO, or MMC).
 *
 * @param dev Pointer to the device structure
 * @param card Pointer to the SDEV card structure pointer
 *
 * @retval 0 on success
 * @retval -EINVAL if dev or scard pointer is NULL
 */
int sd_dev_deinit(const struct device *dev, struct sd_dev_card *card)
{
	int ret = 0;

	if (!dev || !card) {
		LOG_ERR("Invalid param: dev=%p card=%p", dev, card);
		return -EINVAL;
	}

	/* Verify card belongs to this device */
	if (card->dev != dev) {
		LOG_ERR("Card device mismatch");
		return -EINVAL;
	}

	/* Deinitialize based on card type */
	switch (card->card_type) {
	case SD_DEVICE_CARD:
		LOG_INF("SD card deinit not implemented");
		break;

#ifdef CONFIG_SDIO_DEV
	case SDIO_DEVICE_CARD: {
		struct sdio_dev *sdio = card->sdio;

		if (!sdio) {
			LOG_WRN("SDIO device already NULL");
			break;
		}

		/* Deinitialize SDIO device */
		ret = sdio_dev_deinit(sdio);
		if (ret) {
			LOG_ERR("sdio_dev_deinit failed (%d)", ret);
			/* Continue cleanup despite error */
		}

		/* Free SDIO structure */
		sd_dev_heap_free(sdio);
		card->sdio = NULL;
		break;
	}
#endif /* CONFIG_SDIO_DEV */

	case MMC_DEVICE_CARD:
		LOG_INF("MMC card deinit not implemented");
		break;

	default:
		LOG_WRN("Unknown SD card type: %d", card->card_type);
		break;
	}

	/* Clear card state */
	card->is_enum = false;
	card->dev = NULL;

	/* Free card structure */
	sd_dev_heap_free(card);

	LOG_INF("%s: SDIO software deinit done", dev->name);

	return ret;
}

/**
 * @}
 */
