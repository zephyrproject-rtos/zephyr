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

struct uaol_capabilities {
	uint32_t input_streams;
	uint32_t output_streams;
	uint32_t bidirectional_streams;
	uint32_t max_tx_fifo_size;
	uint32_t max_rx_fifo_size;
};

struct uaol_config {
	uint8_t xhci_bus;
	uint8_t xhci_device;
	uint8_t xhci_function;
	uint16_t art_divider_m;
	uint16_t art_divider_n;
	uint32_t sample_rate;
	uint32_t channels;
	uint32_t sample_bits;
	uint32_t service_interval;
	uint32_t sio_credit_size;
	uint16_t fifo_start_offset;
	uint16_t channel_map;
};

struct uaol_ep_table_entry {
	uint32_t usb_ep_address     : 5;
	uint32_t device_slot_number : 8;
	uint32_t split_ep           : 1;
};

typedef int (*uaol_api_config)(const struct device *dev, int stream, struct uaol_config *config);

typedef int (*uaol_api_start)(const struct device *dev, int stream);

typedef int (*uaol_api_stop)(const struct device *dev, int stream);

typedef int (*uaol_api_program_ep_table)(const struct device *dev, int stream,
					 struct uaol_ep_table_entry entry, bool valid);

typedef int (*uaol_api_get_capabilities)(const struct device *dev, struct uaol_capabilities *caps);

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */
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

#endif /* ZEPHYR_INCLUDE_DRIVERS_UAOL_H_ */
