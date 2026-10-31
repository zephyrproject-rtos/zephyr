/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Private MSI proxy interface between the BCM2712 MIP and the RP1
 * interrupt controller. This is not a public Zephyr API.
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MSI_PROXY_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MSI_PROXY_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

/** @brief Complete MSI message for one proxy channel. */
struct msi_proxy_message {
	/** Address the endpoint must write to, as seen by the endpoint. */
	uint64_t address;
	/** Value to write to the message data field. */
	uint32_t data;
};

/**
 * @brief Compose the MSI message for a channel.
 *
 * @param dev MSI proxy device.
 * @param channel Proxy-local channel number.
 * @param message Filled with the complete MSI message.
 *
 * @retval 0 The message was composed.
 * @retval -EINVAL The channel is out of range.
 */
typedef int (*msi_proxy_compose_t)(const struct device *dev, unsigned int channel,
				   struct msi_proxy_message *message);

/**
 * @brief Unmask the host-side MSI path for a channel.
 *
 * @param dev MSI proxy device.
 * @param channel Proxy-local channel number.
 *
 * @retval 0 The host path is enabled.
 * @retval -EINVAL The channel is out of range.
 */
typedef int (*msi_proxy_enable_t)(const struct device *dev, unsigned int channel);

/**
 * @brief Mask the host-side MSI path for a channel.
 *
 * @param dev MSI proxy device.
 * @param channel Proxy-local channel number.
 */
typedef void (*msi_proxy_disable_t)(const struct device *dev, unsigned int channel);

/** @brief MSI proxy driver API. */
__subsystem struct msi_proxy_driver_api {
	/** @brief See @ref msi_proxy_compose(). */
	msi_proxy_compose_t compose;
	/** @brief See @ref msi_proxy_enable(). */
	msi_proxy_enable_t enable;
	/** @brief See @ref msi_proxy_disable(). */
	msi_proxy_disable_t disable;
};

static inline int msi_proxy_compose(const struct device *dev, unsigned int channel,
				    struct msi_proxy_message *message)
{
	return DEVICE_API_GET(msi_proxy, dev)->compose(dev, channel, message);
}

static inline int msi_proxy_enable(const struct device *dev, unsigned int channel)
{
	return DEVICE_API_GET(msi_proxy, dev)->enable(dev, channel);
}

static inline void msi_proxy_disable(const struct device *dev, unsigned int channel)
{
	DEVICE_API_GET(msi_proxy, dev)->disable(dev, channel);
}

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MSI_PROXY_H_ */
