/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic MSI proxy API for MSI-X endpoints.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_MSI_PROXY_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_MSI_PROXY_H_

#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/types.h>

/**
 * @brief MSI proxy
 * @defgroup msi_proxy MSI proxy
 * @since 4.6
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 *
 * An MSI proxy is a platform block that decodes MSI/MSI-X posted writes
 * from an endpoint and raises a host interrupt. It is used when the
 * endpoint's MSI target address is not decoded by a GIC ITS.
 *
 * A channel is the proxy-local vector number, i.e. the value the proxy
 * uses to select one of its outputs. It is not the endpoint vector and
 * not the host interrupt number; the endpoint driver maps its own
 * vectors to channels through its interrupt-map.
 *
 * msi_proxy_compose() returns the complete message to write to an MSI-X
 * table entry for a channel. The encoding is defined by the proxy.
 *
 * msi_proxy_enable() and msi_proxy_disable() may be called from thread
 * or ISR context. Implementations must serialize concurrent calls for
 * different channels. The device must be ready before any function is
 * called.
 */

/** @brief Complete MSI message for one proxy channel. */
struct msi_proxy_message {
	/** Address the endpoint must write to, as seen by the endpoint. */
	uint64_t address;
	/** Value to write to the message data field. */
	uint32_t data;
};

/** @brief MSI doorbell window exposed by a proxy. */
struct msi_proxy_window {
	/** PCIe bus base address of the window. */
	uint64_t bus_addr;
	/** CPU physical base address of the window. */
	uint64_t cpu_addr;
	/**
	 * Size in bytes required by the proxy. The consumer may map a larger
	 * power-of-two region that covers this range and must validate the
	 * alignment and granularity supported by its controller.
	 */
	size_t size;
};

/**
 * @brief Callback type for composing the message of a channel.
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
 * @brief Callback type for retrieving the proxy doorbell window.
 *
 * @param dev MSI proxy device.
 * @param window Filled with the window description.
 *
 * @retval 0 The window was returned.
 * @retval -ENOTSUP The proxy has no doorbell window.
 */
typedef int (*msi_proxy_get_window_t)(const struct device *dev, struct msi_proxy_window *window);

/**
 * @brief Callback type for unmasking a host-side MSI path.
 *
 * @param dev MSI proxy device.
 * @param channel Proxy-local channel number.
 *
 * @retval 0 The host path is enabled.
 * @retval -EINVAL The channel is out of range.
 */
typedef int (*msi_proxy_enable_t)(const struct device *dev, unsigned int channel);

/**
 * @brief Callback type for masking a host-side MSI path.
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
	/** @brief See @ref msi_proxy_get_window(). */
	msi_proxy_get_window_t get_window;
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
static inline int msi_proxy_compose(const struct device *dev, unsigned int channel,
				    struct msi_proxy_message *message)
{
	return DEVICE_API_GET(msi_proxy, dev)->compose(dev, channel, message);
}

/**
 * @brief Get the proxy doorbell window.
 *
 * @param dev MSI proxy device.
 * @param window Filled with the window description.
 *
 * @retval 0 The window was returned.
 * @retval -ENOTSUP The proxy has no doorbell window.
 */
static inline int msi_proxy_get_window(const struct device *dev, struct msi_proxy_window *window)
{
	return DEVICE_API_GET(msi_proxy, dev)->get_window(dev, window);
}

/**
 * @brief Unmask the host-side MSI path for a channel.
 *
 * @param dev MSI proxy device.
 * @param channel Proxy-local channel number.
 *
 * @retval 0 The host path is enabled.
 * @retval -EINVAL The channel is out of range.
 */
static inline int msi_proxy_enable(const struct device *dev, unsigned int channel)
{
	return DEVICE_API_GET(msi_proxy, dev)->enable(dev, channel);
}

/**
 * @brief Mask the host-side MSI path for a channel.
 *
 * @param dev MSI proxy device.
 * @param channel Proxy-local channel number.
 */
static inline void msi_proxy_disable(const struct device *dev, unsigned int channel)
{
	DEVICE_API_GET(msi_proxy, dev)->disable(dev, channel);
}

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_MSI_PROXY_H_ */
