/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host vendor class driver API
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBH_VENDOR_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBH_VENDOR_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB host vendor class driver API
 * @defgroup usbh_vendor_api USB host vendor class driver API
 * @ingroup usb
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @brief User callback, registered by usbh_vendor_intr_subscribe()
 *
 * The buffer is owned and freed by the vendor driver. It is valid only
 * until the callback returns, so the callback must copy any data it
 * needs to keep (e.g. with memcpy()).
 *
 * @param user_ctx      Pointer to user context
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to buffer
 * @param buf_len       Buffer length
 * @param err           0 on success, negative errno value on failure
 *
 */
typedef void (*usbh_vendor_intr_callback_t)(void *user_ctx, uint8_t ep_id, void *buf,
					    size_t buf_len, int err);

/**
 * @brief Completion callback of usbh_vendor_bulk_write() and usbh_vendor_intr_write()
 *
 * @param user_ctx      Pointer to user context
 * @param ep_id         Endpoint ID
 * @param err           0 on success, negative errno value on failure
 *
 */
typedef void (*usbh_vendor_write_callback_t)(void *user_ctx, uint8_t ep_id, int err);

/**
 * @brief Completion callback of usbh_vendor_bulk_read()
 *
 * The buffer is owned and freed by the vendor driver. It is valid only
 * until the callback returns, so the callback must copy any data it
 * needs to keep (e.g. with memcpy()).
 *
 * @param user_ctx      Pointer to user context
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to buffer
 * @param buf_len       Buffer length
 * @param err           0 on success, negative errno value on failure
 *
 */
typedef void (*usbh_vendor_read_callback_t)(void *user_ctx, uint8_t ep_id, void *buf,
					    size_t buf_len, int err);

/**
 * @brief Read data from input bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf_len       Buffer length
 * @param cb            Callback
 * @param user_ctx      Callback context
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_vendor_bulk_read_t)(const struct device *dev, uint8_t ep_id, size_t buf_len,
				       usbh_vendor_read_callback_t cb, void *user_ctx);

/**
 * @brief Write data to output bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to input buffer
 * @param buf_len       Buffer length
 * @param cb            Callback
 * @param user_ctx      Callback context
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_vendor_bulk_write_t)(const struct device *dev, uint8_t ep_id, const uint8_t *buf,
					size_t buf_len, usbh_vendor_write_callback_t cb,
					void *user_ctx);

/**
 * @brief Cancel pending transfer on endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_vendor_cancel_t)(const struct device *dev, uint8_t ep_id);

/**
 * @brief Write data to output interrupt endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to input buffer
 * @param buf_len       Buffer length
 * @param cb            Callback
 * @param user_ctx      Callback context
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_vendor_intr_write_t)(const struct device *dev, uint8_t ep_id, const uint8_t *buf,
					size_t buf_len, usbh_vendor_write_callback_t cb,
					void *user_ctx);

/**
 * @brief Subscribe callback to input interrupt endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param cb            User callback
 * @param user_ctx      User context
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_vendor_intr_subscribe_t)(const struct device *dev, uint8_t ep_id,
					    usbh_vendor_intr_callback_t cb, void *user_ctx);

/**
 * @brief Unsubscribe callback from input interrupt endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_vendor_intr_unsubscribe_t)(const struct device *dev, uint8_t ep_id);

/**
 * @brief USB host vendor class driver API structure
 *
 * Bulk operations are mandatory, interrupt operations are optional.
 */
__subsystem struct usbh_vendor_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief usbh_vendor_bulk_read
	 */
	usbh_vendor_bulk_read_t bulk_read;
	/**
	 * @driver_ops_mandatory @copybrief usbh_vendor_cancel_bulk_read
	 */
	usbh_vendor_cancel_t cancel_bulk_read;

	/**
	 * @driver_ops_mandatory @copybrief usbh_vendor_bulk_write
	 */
	usbh_vendor_bulk_write_t bulk_write;

	/**
	 * @driver_ops_mandatory @copybrief usbh_vendor_cancel_bulk_write
	 */
	usbh_vendor_cancel_t cancel_bulk_write;

	/**
	 * @driver_ops_optional @copybrief usbh_vendor_intr_subscribe
	 */
	usbh_vendor_intr_subscribe_t intr_subscribe;

	/**
	 * @driver_ops_optional @copybrief usbh_vendor_intr_unsubscribe
	 */
	usbh_vendor_intr_unsubscribe_t intr_unsubscribe;

	/**
	 * @driver_ops_optional @copybrief usbh_vendor_intr_write
	 */
	usbh_vendor_intr_write_t intr_write;

	/**
	 * @driver_ops_optional @copybrief usbh_vendor_cancel_intr_write
	 */
	usbh_vendor_cancel_t cancel_intr_write;
};

/**
 * @brief Read data from input bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf_len       Buffer length
 * @param cb            Callback
 * @param user_ctx      Callback context
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_bulk_read(const struct device *dev, uint8_t ep_id, size_t buf_len,
					usbh_vendor_read_callback_t cb, void *user_ctx)
{
	return DEVICE_API_GET(usbh_vendor, dev)->bulk_read(dev, ep_id, buf_len, cb, user_ctx);
}

/**
 * @brief Cancel read operation to bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_cancel_bulk_read(const struct device *dev, uint8_t ep_id)
{
	return DEVICE_API_GET(usbh_vendor, dev)->cancel_bulk_read(dev, ep_id);
}

/**
 * @brief Write data to output bulk endpoint
 *
 * Starts an asynchronous write to bulk endpoint. The driver copies the input data.
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to input buffer
 * @param buf_len       Buffer length
 * @param cb            Callback
 * @param user_ctx      Callback context
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_bulk_write(const struct device *dev, uint8_t ep_id,
					 const uint8_t *buf, size_t buf_len,
					 usbh_vendor_write_callback_t cb, void *user_ctx)
{
	return DEVICE_API_GET(usbh_vendor, dev)->bulk_write(dev, ep_id, buf, buf_len, cb, user_ctx);
}

/**
 * @brief Cancel write operation to bulk endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_cancel_bulk_write(const struct device *dev, uint8_t ep_id)
{
	return DEVICE_API_GET(usbh_vendor, dev)->cancel_bulk_write(dev, ep_id);
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
static inline int usbh_vendor_intr_subscribe(const struct device *dev, uint8_t ep_id,
					     usbh_vendor_intr_callback_t cb, void *user_ctx)
{
	const struct usbh_vendor_driver_api *api = DEVICE_API_GET(usbh_vendor, dev);

	if (api->intr_subscribe == NULL) {
		return -ENOSYS;
	}
	return api->intr_subscribe(dev, ep_id, cb, user_ctx);
}

/**
 * @brief Unsubscribe from receiving interrupts
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_intr_unsubscribe(const struct device *dev, uint8_t ep_id)
{
	const struct usbh_vendor_driver_api *api = DEVICE_API_GET(usbh_vendor, dev);

	if (api->intr_unsubscribe == NULL) {
		return -ENOSYS;
	}
	return api->intr_unsubscribe(dev, ep_id);
}

/**
 * @brief Write data to output interrupt endpoint
 *
 * Starts an asynchronous write to interrupt endpoint. The driver copies the input data.
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 * @param buf           Pointer to input buffer
 * @param buf_len       Buffer length
 * @param cb            Callback
 * @param user_ctx      Callback context
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_intr_write(const struct device *dev, uint8_t ep_id,
					 const uint8_t *buf, size_t buf_len,
					 usbh_vendor_write_callback_t cb, void *user_ctx)
{
	const struct usbh_vendor_driver_api *api = DEVICE_API_GET(usbh_vendor, dev);

	if (api->intr_write == NULL) {
		return -ENOSYS;
	}
	return api->intr_write(dev, ep_id, buf, buf_len, cb, user_ctx);
}

/**
 * @brief Cancel write operation to interrupt endpoint
 *
 * @param dev           Pointer to device
 * @param ep_id         Endpoint ID
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_vendor_cancel_intr_write(const struct device *dev, uint8_t ep_id)
{
	const struct usbh_vendor_driver_api *api = DEVICE_API_GET(usbh_vendor, dev);

	if (api->cancel_intr_write == NULL) {
		return -ENOSYS;
	}
	return api->cancel_intr_write(dev, ep_id);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBH_VENDOR_H_ */
