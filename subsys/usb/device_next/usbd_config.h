/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_CONFIG_H
#define ZEPHYR_INCLUDE_USBD_CONFIG_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Get configuration descriptor bConfigurationValue value
 *
 * @param[in] cfg_nd Pointer to a configuration node structure
 *
 * @return bConfigurationValue value
 */
static inline uint8_t usbd_config_get_value(const struct usbd_config_node *const cfg_nd)
{
	struct usb_cfg_descriptor *cfg_desc = cfg_nd->desc;

	return cfg_desc->bConfigurationValue;
}

/**
 * @brief Set configuration descriptor bConfigurationValue value
 *
 * @param[in] cfg_nd Pointer to a configuration node structure
 */
static inline void usbd_config_set_value(const struct usbd_config_node *const cfg_nd,
					 const uint8_t value)
{
	struct usb_cfg_descriptor *cfg_desc = cfg_nd->desc;

	cfg_desc->bConfigurationValue = value;
}

/**
 * @brief Get configuration node
 *
 * Get configuration node with desired configuration number.
 *
 * @param[in] ctx    Pointer to USB device support context
 * @param[in] speed  Speed the configuration number applies to
 * @param[in] cfg    Configuration number (bConfigurationValue)
 *
 * @return pointer to configuration node or NULL if does not exist
 */
struct usbd_config_node *usbd_config_get(struct usbd_context *uds_ctx,
					 const enum usbd_speed speed,
					 uint8_t cfg);

/**
 * @brief Get selected configuration node
 *
 * Get configuration node based on configuration selected by the host.
 *
 * @param[in] ctx    Pointer to USB device support context
 *
 * @return pointer to configuration node or NULL if does not exist
 */
struct usbd_config_node *usbd_config_get_current(struct usbd_context *uds_ctx);

/**
 * @brief Check whether a configuration exist
 *
 * @param[in] ctx    Pointer to USB device support context
 * @param[in] speed  Speed at which the configuration should be checked
 * @param[in] cfg    Configuration number (bConfigurationValue)
 *
 * @return True if a configuration exist.
 */
bool usbd_config_exist(struct usbd_context *const uds_ctx,
		       const enum usbd_speed speed,
		       const uint8_t cfg);

/**
 * @brief Setup new USB device configuration
 *
 * This function disables all active endpoints of current configuration
 * and enables all interface alternate 0 endpoints of a new configuration.
 * Determined to be called Set Configuration request.
 *
 * @param[in] ctx     Pointer to USB device support context
 * @param[in] new_cfg New configuration number (bConfigurationValue)
 *
 * @return 0 on success, other values on fail.
 */
int usbd_config_set(struct usbd_context *uds_ctx, uint8_t new_cfg);

#endif /* ZEPHYR_INCLUDE_USBD_CONFIG_H */
