/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for USB Audio Offload Link (UAOL) drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UAOL_H_
#define ZEPHYR_INCLUDE_DRIVERS_UAOL_H_

#include <stdint.h>
#include <zephyr/device.h>

/** @brief UAOL hardware capabilities. */
struct uaol_capabilities {
	uint32_t input_streams;          /**< Number of supported input streams */
	uint32_t output_streams;         /**< Number of supported output streams */
	uint32_t bidirectional_streams;  /**< Number of supported bidir streams */
	uint32_t max_tx_fifo_size;       /**< Max TX FIFO size */
	uint32_t max_rx_fifo_size;       /**< Max RX FIFO size */
};

/** @brief UAOL stream configuration data. */
struct uaol_config {
	uint8_t xhci_bus;                /**< xHCI controller bus */
	uint8_t xhci_device;             /**< xHCI controller device */
	uint8_t xhci_function;           /**< xHCI controller function */
	uint16_t art_divider_m;          /**< USB ART divider multiplication value */
	uint16_t art_divider_n;          /**< USB ART divider division value */
	uint32_t sample_rate;            /**< Audio sample rate (frames per second ) */
	uint32_t channels;               /**< Number of audio channels */
	uint32_t sample_bits;            /**< Audio sample (container) size in bits */
	uint32_t service_interval;       /**< Service interval for PCM stream operation in us */
	uint32_t sio_credit_size;        /**< SIO credit packet size in bytes */
	uint16_t fifo_start_offset;      /**< UAOL FIFO start address offset */
	uint16_t channel_map;            /**< HDA link stream and channels mapping for UAOL FIFO */
};

/** @brief UAOL stream endpoint table entry. */
struct uaol_ep_table_entry {
	uint32_t usb_ep_address     : 5;  /**< USB endpoint number */
	uint32_t device_slot_number : 8;  /**< Slot number */
	uint32_t split_ep           : 1;  /**< Split endpoint flag */
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */
typedef int (*uaol_api_config)(const struct device *dev, int stream, struct uaol_config *config);

typedef int (*uaol_api_start)(const struct device *dev, int stream);

typedef int (*uaol_api_stop)(const struct device *dev, int stream);

typedef int (*uaol_api_program_ep_table)(const struct device *dev, int stream,
					 struct uaol_ep_table_entry entry, bool valid);

typedef int (*uaol_api_get_capabilities)(const struct device *dev, struct uaol_capabilities *caps);

__subsystem struct uaol_driver_api {
	uaol_api_config config;
	uaol_api_start start;
	uaol_api_stop stop;
	uaol_api_program_ep_table program_ep_table;
	uaol_api_get_capabilities get_capabilities;
};
/**
 * @endcond
 */

/**
 * @brief Set UAOL individual stream configuration.
 *
 * @param dev UAOL device instance.
 * @param stream UAOL stream index.
 * @param cfg Configuration to be applied.
 *
 * @return 0 on success, all other values should be treated as error.
 */
static inline int uaol_config(const struct device *dev, int stream, struct uaol_config *cfg)
{
	const struct uaol_driver_api *api = dev->api;

	return api->config(dev, stream, cfg);
}

/**
 * @brief Start UAOL individual stream operation.
 *
 * @param dev UAOL device instance.
 * @param stream UAOL stream index.
 *
 * @return 0 on success, all other values should be treated as error.
 */
static inline int uaol_start(const struct device *dev, int stream)
{
	const struct uaol_driver_api *api = dev->api;

	return api->start(dev, stream);
}

/**
 * @brief Stop UAOL individual stream operation.
 *
 * @param dev UAOL device instance.
 * @param stream UAOL stream index.
 *
 * @return 0 on success, all other values should be treated as error.
 */
static inline int uaol_stop(const struct device *dev, int stream)
{
	const struct uaol_driver_api *api = dev->api;

	return api->stop(dev, stream);
}

/**
 * @brief Program an endpoint table entry for UAOL individual stream.
 *
 * @param dev UAOL device instance.
 * @param stream UAOL stream index.
 * @param entry Data to be stored in EP table.
 * @param valid Flag marking an entry as valid or invalid.
 *
 * @return 0 on success, all other values should be treated as error.
 */
static inline int uaol_program_ep_table(const struct device *dev, int stream,
					struct uaol_ep_table_entry entry, bool valid)
{
	const struct uaol_driver_api *api = dev->api;

	return api->program_ep_table(dev, stream, entry, valid);
}

/**
 * @brief Query UAOL hardware capabilities.
 *
 * @param dev UAOL device instance.
 * @param caps Capabilities data to be returned.
 *
 * @return 0 on success, all other values should be treated as error.
 */
static inline int uaol_get_capabilities(const struct device *dev, struct uaol_capabilities *caps)
{
	const struct uaol_driver_api *api = dev->api;

	return api->get_capabilities(dev, caps);
}

/**
 * @brief Get the HDA link stream ID mapped in HW to the UAOL stream ID.
 *
 * Can be called anytime, e.g., before the device probe.
 *
 * @param dev UAOL device instance.
 * @param uaol_stream_id UAOL stream ID.
 *
 * @return the HDA link stream ID on success, otherwise -1.
 */
int uaol_get_mapped_hda_link_stream_id(const struct device *dev, int uaol_stream_id);

#endif /* ZEPHYR_INCLUDE_DRIVERS_UAOL_H_ */
