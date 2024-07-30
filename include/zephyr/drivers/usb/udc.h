/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New USB device controller (UDC) driver API
 */

#ifndef ZEPHYR_INCLUDE_UDC_H
#define ZEPHYR_INCLUDE_UDC_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usb_ch9.h>

/**
 * @brief Maximum packet size of control endpoint supported by the controller.
 */
enum udc_mps0 {
	UDC_MPS0_8,
	UDC_MPS0_16,
	UDC_MPS0_32,
	UDC_MPS0_64,
};

/**
 * USB device controller capabilities
 *
 * This structure is mainly intended for the USB device stack.
 */
struct udc_device_caps {
	/** USB high speed capable controller */
	uint32_t hs : 1;
	/** Controller supports USB remote wakeup */
	uint32_t rwup : 1;
	/** Controller performs status OUT stage automatically */
	uint32_t out_ack : 1;
	/** Controller expects device address to be set before status stage */
	uint32_t addr_before_status : 1;
	/** Controller can detect the state change of USB supply VBUS.*/
	uint32_t can_detect_vbus : 1;
	/** Maximum packet size for control endpoint */
	enum udc_mps0 mps0 : 2;
};

/**
 * @brief USB device actual speed
 */
enum udc_bus_speed {
	/** Device is probably not connected */
	UDC_BUS_UNKNOWN,
	/** Device is connected to a full speed bus */
	UDC_BUS_SPEED_FS,
	/** Device is connected to a high speed bus  */
	UDC_BUS_SPEED_HS,
	/** Device is connected to a super speed bus */
	UDC_BUS_SPEED_SS,
};

/**
 * USB device controller endpoint capabilities
 */
struct udc_ep_caps {
	/** Maximum packet size of the endpoint buffer */
	uint32_t mps : 16;
	/** Control transfer capable endpoint (for completeness) */
	uint32_t control : 1;
	/** Interrupt transfer capable endpoint */
	uint32_t interrupt : 1;
	/** Bulk transfer capable endpoint */
	uint32_t bulk : 1;
	/** ISO transfer capable endpoint */
	uint32_t iso : 1;
	/** IN transfer capable endpoint */
	uint32_t in : 1;
	/** OUT transfer capable endpoint */
	uint32_t out : 1;
};

/**
 * USB device controller endpoint status
 */
struct udc_ep_stat {
	/** Endpoint is enabled */
	uint32_t enabled : 1;
	/** Endpoint is halted (returning STALL PID) */
	uint32_t halted : 1;
	/** Last submitted PID is DATA1 */
	uint32_t data1 : 1;
	/** If double buffering is supported, last used buffer is odd */
	uint32_t odd : 1;
	/** Endpoint is busy */
	uint32_t busy : 1;
};

/**
 * USB device controller endpoint configuration
 *
 * This structure is mandatory for configuration and management of endpoints.
 * It is not exposed to higher layer and is used only by internal part
 * of UDC API and driver.
 */
struct udc_ep_config {
	/** Endpoint requests FIFO */
	struct k_fifo fifo;
	/** Endpoint capabilities */
	struct udc_ep_caps caps;
	/** Endpoint status */
	struct udc_ep_stat stat;
	/** Endpoint address */
	uint8_t addr;
	/** Endpoint attributes */
	uint8_t attributes;
	/** Maximum packet size */
	uint16_t mps;
	/** Polling interval */
	uint8_t interval;
};


/**
 * @brief USB device controller event types
 */
enum udc_event_type {
	/** VBUS ready event. Signals that VBUS is in stable condition. */
	UDC_EVT_VBUS_READY,
	/** VBUS removed event. Signals that VBUS is below the valid range. */
	UDC_EVT_VBUS_REMOVED,
	/** Device resume event */
	UDC_EVT_RESUME,
	/** Device suspended event */
	UDC_EVT_SUSPEND,
	/** Port reset detected */
	UDC_EVT_RESET,
	/** Start of Frame event */
	UDC_EVT_SOF,
	/** Endpoint request result event */
	UDC_EVT_EP_REQUEST,
	/**
	 * Non-correctable error event, requires attention from higher
	 * levels or application.
	 */
	UDC_EVT_ERROR,
};

/**
 * USB device controller event
 *
 * Common structure for all events that originate from
 * the UDC driver and are passed to higher layer using
 * message queue and a callback (udc_event_cb_t) provided
 * by higher layer during controller initialization (udc_init).
 */
struct udc_event {
	/** Event type */
	enum udc_event_type type;
	union {
		/** Event value */
		uint32_t value;
		/** Event status value, if any */
		int status;
		/** Pointer to request used only for UDC_EVT_EP_REQUEST */
		struct net_buf *buf;
	};
	/** Pointer to device struct */
	const struct device *dev;
};

/**
 * UDC endpoint buffer info
 *
 * This structure is mandatory for all UDC request.
 * It contains the meta data about the request and is stored in
 * user_data array of net_buf structure for each request.
 */
struct udc_buf_info {
	/** Endpoint to which request is associated */
	uint8_t ep;
	/** Flag marks setup transfer */
	unsigned int setup : 1;
	/** Flag marks data stage of setup transfer */
	unsigned int data : 1;
	/** Flag marks status stage of setup transfer */
	unsigned int status : 1;
	/** Flag marks ZLP at the end of a transfer */
	unsigned int zlp : 1;
	/** Flag marks request buffer claimed by the controller (TBD) */
	unsigned int claimed : 1;
	/** Flag marks request buffer is queued (TBD) */
	unsigned int queued : 1;
	/** Transfer owner (usually pointer to a class instance) */
	void *owner;
	/** Transfer result, 0 on success, other values on error */
	int err;
} __packed;

/**
 * @typedef udc_event_cb_t
 * @brief Callback to submit UDC event to higher layer.
 *
 * At the higher level, the event is to be inserted into a message queue.
 * (TBD) Maybe it is better to provide a pointer to k_msgq passed during
 * initialization.
 *
 * @param[in] dev      Pointer to device struct of the driver instance
 * @param[in] event    Point to event structure
 *
 * @return 0 on success, all other values should be treated as error.
 */
typedef int (*udc_event_cb_t)(const struct device *dev,
			      const struct udc_event *const event);

/**
 * @brief UDC driver API
 * This is the mandatory API any USB device controller driver needs to expose
 * with exception of:
 *   device_speed(), test_mode() are only required for HS controllers
 */
struct udc_api {
	enum udc_bus_speed (*device_speed)(const struct device *dev);
	int (*ep_enqueue)(const struct device *dev,
			  struct udc_ep_config *const cfg,
			  struct net_buf *const buf);
	int (*ep_dequeue)(const struct device *dev,
			  struct udc_ep_config *const cfg);
	int (*ep_set_halt)(const struct device *dev,
			   struct udc_ep_config *const cfg);
	int (*ep_clear_halt)(const struct device *dev,
			     struct udc_ep_config *const cfg);
	int (*ep_try_config)(const struct device *dev,
			     struct udc_ep_config *const cfg);
	int (*ep_enable)(const struct device *dev,
			 struct udc_ep_config *const cfg);
	int (*ep_disable)(const struct device *dev,
			  struct udc_ep_config *const cfg);
	int (*host_wakeup)(const struct device *dev);
	int (*set_address)(const struct device *dev,
			   const uint8_t addr);
	int (*test_mode)(const struct device *dev,
			 const uint8_t mode, const bool dryrun);
	int (*enable)(const struct device *dev);
	int (*disable)(const struct device *dev);
	int (*init)(const struct device *dev);
	int (*shutdown)(const struct device *dev);
	int (*lock)(const struct device *dev);
	int (*unlock)(const struct device *dev);
};

/**
 * Controller is initialized by udc_init() and can generate the VBUS events,
 * if capable, but shall not be recognizable by host.
 */
#define UDC_STATUS_INITIALIZED		0
/**
 * Controller is enabled and all API functions are available,
 * controller is recognizable by host.
 */
#define UDC_STATUS_ENABLED		1
/** Controller is suspended by the host */
#define UDC_STATUS_SUSPENDED		2

/**
 * Common UDC driver data structure
 *
 * Mandatory structure for each UDC controller driver.
 * To be implemented as device's private data (device->data).
 */
struct udc_data {
	/** LUT for endpoint management */
	struct udc_ep_config *ep_lut[32];
	/** Controller capabilities */
	struct udc_device_caps caps;
	/** Driver access mutex */
	struct k_mutex mutex;
	/** Callback to submit an UDC event to upper layer */
	udc_event_cb_t event_cb;
	/** USB device controller status */
	atomic_t status;
	/** Internal used Control Sequence Stage */
	int stage;
	/** Pointer to buffer containing setup packet */
	struct net_buf *setup;
	/** Driver private data */
	void *priv;
};

/**
 * @brief New USB device controller (UDC) driver API
 * @defgroup udc_api USB device controller driver API
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Checks whether the controller is initialized.
 *
 * @param[in] dev      Pointer to device struct of the driver instance
 *
 * @return true if controller is initialized, false otherwise
 */
static inline bool udc_is_initialized(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return atomic_test_bit(&data->status, UDC_STATUS_INITIALIZED);
}

/**
 * @brief Checks whether the controller is enabled.
 *
 * @param[in] dev      Pointer to device struct of the driver instance
 *
 * @return true if controller is enabled, false otherwise
 */
static inline bool udc_is_enabled(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return atomic_test_bit(&data->status, UDC_STATUS_ENABLED);
}

/**
 * @brief Checks whether the controller is suspended.
 *
 * @param[in] dev      Pointer to device struct of the driver instance
 *
 * @return true if controller is suspended, false otherwise
 */
static inline bool udc_is_suspended(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return atomic_test_bit(&data->status, UDC_STATUS_SUSPENDED);
}

/**
 * @brief Initialize USB device controller
 *
 * Initialize USB device controller and control IN/OUT endpoint.
 * After initialization controller driver should be able to detect
 * power state of the bus and signal power state changes.
 *
 * @param[in] dev      Pointer to device struct of the driver instance
 * @param[in] event_cb Event callback from the higher layer (USB device stack)
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EINVAL on parameter error (no callback is passed)
 * @retval -EALREADY already initialized
 */
int udc_init(const struct device *dev, udc_event_cb_t event_cb);

/**
 * @brief Enable USB device controller
 *
 * Enable powered USB device controller and allow host to
 * recognize and enumerate the device.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not initialized
 * @retval -EALREADY already enabled
 */
int udc_enable(const struct device *dev);

/**
 * @brief Disable USB device controller
 *
 * Disable enabled USB device controller.
 * The driver should continue to detect power state changes.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EALREADY already disabled
 */
int udc_disable(const struct device *dev);

/**
 * @brief Poweroff USB device controller
 *
 * Shut down the controller completely to reduce energy consumption
 * or to change the role of the controller.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EALREADY controller is not initialized
 */
int udc_shutdown(const struct device *dev);

/**
 * @brief Get USB device controller capabilities
 *
 * Obtain the capabilities of the controller
 * such as full speed (FS), high speed (HS), and more.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return USB device controller capabilities.
 */
static inline struct udc_device_caps udc_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->caps;
}

/**
 * @brief Get actual USB device speed
 *
 * The function should be called after the reset event to determine
 * the actual bus speed.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return USB device controller capabilities.
 */
enum udc_bus_speed udc_device_speed(const struct device *dev);

/**
 * @brief Set USB device address.
 *
 * Set address of enabled USB device.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] addr   USB device address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not enabled (or not initialized)
 */
static inline int udc_set_address(const struct device *dev, const uint8_t addr)
{
	const struct udc_api *api = dev->api;
	int ret;

	if (!udc_is_enabled(dev)) {
		return -EPERM;
	}

	api->lock(dev);
	ret = api->set_address(dev, addr);
	api->unlock(dev);

	return ret;
}

/**
 * @brief Enable Test Mode.
 *
 * For compliance testing, high-speed controllers must support test modes.
 * A particular test is enabled by a SetFeature(TEST_MODE) request.
 * To disable a test mode, device needs to be power cycled.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] mode   Test mode
 * @param[in] dryrun Verify that a particular mode can be enabled, but do not
 *                   enable test mode
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENOTSUP Test mode is not supported
 */
static inline int udc_test_mode(const struct device *dev,
				const uint8_t mode, const bool dryrun)
{
	const struct udc_api *api = dev->api;
	int ret;

	if (!udc_is_enabled(dev)) {
		return -EPERM;
	}

	if (api->test_mode != NULL) {
		api->lock(dev);
		ret = api->test_mode(dev, mode, dryrun);
		api->unlock(dev);
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

/**
 * @brief Initiate host wakeup procedure.
 *
 * Initiate host wakeup. Only possible when the bus is suspended.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not enabled (or not initialized)
 */
static inline int udc_host_wakeup(const struct device *dev)
{
	const struct udc_api *api = dev->api;
	int ret;

	if (!udc_is_enabled(dev)) {
		return -EPERM;
	}

	api->lock(dev);
	ret = api->host_wakeup(dev);
	api->unlock(dev);

	return ret;
}

/**
 * @brief Try an endpoint configuration.
 *
 * Try an endpoint configuration based on endpoint descriptor.
 * This function may modify wMaxPacketSize descriptor fields
 * of the endpoint. All properties of the descriptor,
 * such as direction, and transfer type, should be set correctly.
 * If wMaxPacketSize value is zero, it will be
 * updated to maximum buffer size of the endpoint.
 *
 * @param[in] dev        Pointer to device struct of the driver instance
 * @param[in] ep         Endpoint address (same as bEndpointAddress)
 * @param[in] attributes Endpoint attributes (same as bmAttributes)
 * @param[in] mps        Maximum packet size (same as wMaxPacketSize)
 * @param[in] interval   Polling interval (same as bInterval)
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EINVAL on wrong parameter
 * @retval -ENOTSUP endpoint configuration not supported
 * @retval -ENODEV no endpoints available
 */
int udc_ep_try_config(const struct device *dev,
		      const uint8_t ep,
		      const uint8_t attributes,
		      uint16_t *const mps,
		      const uint8_t interval);

/**
 * @brief Configure and enable endpoint.
 *
 * Configure and make an endpoint ready for use.
 * Valid for all endpoints except control IN/OUT.
 *
 * @param[in] dev        Pointer to device struct of the driver instance
 * @param[in] ep         Endpoint address (same as bEndpointAddress)
 * @param[in] attributes Endpoint attributes (same as bmAttributes)
 * @param[in] mps        Maximum packet size (same as wMaxPacketSize)
 * @param[in] interval   Polling interval (same as bInterval)
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EINVAL on wrong parameter (control IN/OUT endpoint)
 * @retval -EPERM controller is not initialized
 * @retval -ENODEV endpoint configuration not found
 * @retval -EALREADY endpoint is already enabled
 */
int udc_ep_enable(const struct device *dev,
		  const uint8_t ep,
		  const uint8_t attributes,
		  const uint16_t mps,
		  const uint8_t interval);

/**
 * @brief Disable endpoint.
 *
 * Valid for all endpoints except control IN/OUT.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EINVAL on wrong parameter (control IN/OUT endpoint)
 * @retval -ENODEV endpoint configuration not found
 * @retval -EALREADY endpoint is already disabled
 * @retval -EPERM controller is not initialized
 */
int udc_ep_disable(const struct device *dev, const uint8_t ep);

/**
 * @brief Halt endpoint
 *
 * Valid for all endpoints.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found
 * @retval -ENOTSUP not supported (e.g. isochronous endpoint)
 * @retval -EPERM controller is not enabled
 */
int udc_ep_set_halt(const struct device *dev, const uint8_t ep);

/**
 * @brief Clear endpoint halt
 *
 * Valid for all endpoints.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found
 * @retval -ENOTSUP not supported (e.g. isochronous endpoint)
 * @retval -EPERM controller is not enabled
 */
int udc_ep_clear_halt(const struct device *dev, const uint8_t ep);

/**
 * @brief Queue USB device controller request
 *
 * Add request to the queue. If the queue is empty, the request
 * buffer can be claimed by the controller immediately.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] buf    Pointer to UDC request buffer
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found
 * @retval -EACCES endpoint is not enabled (TBD)
 * @retval -EBUSY request can not be queued
 * @retval -EPERM controller is not initialized
 */
int udc_ep_enqueue(const struct device *dev, struct net_buf *const buf);

/**
 * @brief Remove all USB device controller requests from endpoint queue
 *
 * UDC_EVT_EP_REQUEST event will be generated when the driver
 * releases claimed buffer, no new requests will be claimed,
 * all requests in the queue will passed as chained list of
 * the event variable buf. The endpoint queue is empty after that.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found
 * @retval -EACCES endpoint is not disabled
 * @retval -EPERM controller is not initialized
 */
int udc_ep_dequeue(const struct device *dev, const uint8_t ep);

/**
 * @brief Allocate UDC request buffer
 *
 * Allocate a new buffer from common request buffer pool.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 * @param[in] size   Size of the request buffer
 *
 * @return pointer to allocated request or NULL on error.
 */
struct net_buf *udc_ep_buf_alloc(const struct device *dev,
				 const uint8_t ep,
				 const size_t size);

/**
 * @brief Free UDC request buffer
 *
 * Put the buffer back into the request buffer pool.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] buf    Pointer to UDC request buffer
 *
 * @return 0 on success, all other values should be treated as error.
 */
int udc_ep_buf_free(const struct device *dev, struct net_buf *const buf);

/**
 * @brief Set ZLP flag in requests metadata.
 *
 * The controller should send a ZLP at the end of the transfer.
 *
 * @param[in] buf    Pointer to UDC request buffer
 */
static inline void udc_ep_buf_set_zlp(struct net_buf *const buf)
{
	struct udc_buf_info *bi;

	__ASSERT_NO_MSG(buf);
	bi = (struct udc_buf_info *)net_buf_user_data(buf);
	if (USB_EP_DIR_IS_IN(bi->ep)) {
		bi->zlp = 1;
	}
}

/**
 * @brief Get requests metadata.
 *
 * @param[in] buf    Pointer to UDC request buffer
 *
 * @return pointer to metadata structure.
 */
static inline struct udc_buf_info *udc_get_buf_info(const struct net_buf *const buf)
{
	__ASSERT_NO_MSG(buf);
	return (struct udc_buf_info *)net_buf_user_data(buf);
}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_UDC_H */
