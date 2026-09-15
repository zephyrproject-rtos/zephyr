/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBH_VENDOR_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBH_VENDOR_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief User callback, registered by usbh_vendor_interrupt_subscribe()
 *
 * @param user_ctx      Pointer to user context
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to buffer
 * @param buf_len       Buffer length
 *
 */
typedef void (*usbh_vendor_callback_t)(void *user_ctx, uint8_t ep_id, void *buf, size_t buf_len);

/**
 * @brief Read data from input bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to output buffer
 * @param buf_len       Buffer length
 *
 */
typedef int (*usbh_vendor_bulk_read_t)(const struct device *dev, uint8_t ep_id, uint8_t *buf,
				       size_t buf_len);

/**
 * @brief Write data to output bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to input buffer
 * @param buf_len       Buffer length
 *
 */
typedef int (*usbh_vendor_bulk_write_t)(const struct device *dev, uint8_t ep_id, const uint8_t *buf,
					size_t buf_len);

/**
 * @brief Write data to output interrupt endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to input buffer
 * @param buf_len       Buffer length
 *
 */
typedef int (*usbh_vendor_interrupt_write_t)(const struct device *dev, uint8_t ep_id,
					     const uint8_t *buf, size_t buf_len);

/**
 * @brief Subscribe callback to input interrupt endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param cb            User callback
 * @param user_ctx      User context
 *
 */
typedef int (*usbh_vendor_interrupt_subscribe_t)(const struct device *dev, uint8_t ep_id,
						 usbh_vendor_callback_t cb, void *user_ctx);

/**
 * @brief Unsubscribe callback from input interrupt endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 *
 */
typedef int (*usbh_vendor_interrupt_unsubscribe_t)(const struct device *dev, uint8_t ep_id);

__subsystem struct usbh_vendor_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief usbh_vendor_bulk_read
	 */
	usbh_vendor_bulk_read_t bulk_read;
	/**
	 * @driver_ops_mandatory @copybrief usbh_vendor_bulk_write
	 */
	usbh_vendor_bulk_write_t bulk_write;

	/**
	 * @driver_ops_optional @copybrief usbh_vendor_interrupt_subscribe
	 */
	usbh_vendor_interrupt_subscribe_t interrupt_subscribe;

	/**
	 * @driver_ops_optional @copybrief usbh_vendor_interrupt_unsubscribe
	 */
	usbh_vendor_interrupt_unsubscribe_t interrupt_unsubscribe;

	/**
	 * @driver_ops_optional @copybrief usbh_vendor_interrupt_write
	 */
	usbh_vendor_interrupt_write_t interrupt_write;
};

/**
 * @brief Read data from input bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to output buffer
 * @param buf_len       Buffer length
 *
 * @return positive number of received bytes, negative errno value on failure.
 */
static inline int usbh_vendor_bulk_read(const struct device *dev, uint8_t ep_id, uint8_t *buf,
					size_t buf_len)
{
	return DEVICE_API_GET(usbh_vendor, dev)->bulk_read(dev, ep_id, buf, buf_len);
}

/**
 * @brief Write data to output bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to input buffer
 * @param buf_len       Buffer length
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_bulk_write(const struct device *dev, uint8_t ep_id,
					 const uint8_t *buf, size_t buf_len)
{
	return DEVICE_API_GET(usbh_vendor, dev)->bulk_write(dev, ep_id, buf, buf_len);
}

/**
 * @brief Subscribe to receiving interrupts
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param cb            Callback to user function
 * @param user_ctx      User context
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_interrupt_subscribe(const struct device *dev, uint8_t ep_id,
						  usbh_vendor_callback_t cb, void *user_ctx)
{
	const struct usbh_vendor_driver_api *api = DEVICE_API_GET(usbh_vendor, dev);

	if (api->interrupt_subscribe == NULL) {
		return -ENOSYS;
	}
	return api->interrupt_subscribe(dev, ep_id, cb, user_ctx);
}

/**
 * @brief Unsubscribe from receiving interrupts
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_interrupt_unsubscribe(const struct device *dev, uint8_t ep_id)
{
	const struct usbh_vendor_driver_api *api = DEVICE_API_GET(usbh_vendor, dev);

	if (api->interrupt_unsubscribe == NULL) {
		return -ENOSYS;
	}
	return api->interrupt_unsubscribe(dev, ep_id);
}

/**
 * @brief Write data to output interrupt endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to input buffer
 * @param buf_len       Buffer length
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_interrupt_write(const struct device *dev, uint8_t ep_id,
					      const uint8_t *buf, size_t buf_len)
{
	const struct usbh_vendor_driver_api *api = DEVICE_API_GET(usbh_vendor, dev);

	if (api->interrupt_write == NULL) {
		return -ENOSYS;
	}
	return api->interrupt_write(dev, ep_id, buf, buf_len);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBH_VENDOR_H_ */
