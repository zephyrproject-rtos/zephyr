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
#include <zephyr/drivers/usb/usb_buf.h>
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
	/**
	 * Controller performs Status OUT stage automatically after Data IN.
	 *
	 * When set, USB stack will not enqueue Status OUT buffer.
	 */
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
	/** High-Bandwidth (interrupt or iso) capable endpoint */
	uint32_t high_bandwidth : 1;
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
	bool enabled;
	/** Endpoint is halted (returning STALL PID) */
	bool halted;
	/** Last submitted PID is DATA1 */
	bool data1;
	/** Endpoint is busy */
	bool busy;
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
	/**
	 * VBUS ready event. Signals that VBUS is in stable condition.
	 * Only controllers with can_detect_vbus set in struct udc_device_caps
	 * generate it. It can be generated while the controller is initialized,
	 * before it is enabled.
	 */
	UDC_EVT_VBUS_READY,
	/**
	 * VBUS removed event. Signals that VBUS is below the valid range.
	 * Generated under the same conditions as UDC_EVT_VBUS_READY.
	 */
	UDC_EVT_VBUS_REMOVED,
	/** Device resume event */
	UDC_EVT_RESUME,
	/** Device suspended event */
	UDC_EVT_SUSPEND,
	/**
	 * Port reset detected. After this event udc_device_speed() returns the
	 * actual bus speed.
	 */
	UDC_EVT_RESET,
	/**
	 * Start of Frame event. Generated only if CONFIG_UDC_ENABLE_SOF is
	 * enabled, once per frame (per microframe for high speed).
	 */
	UDC_EVT_SOF,
	/**
	 * Endpoint request result event. The driver completed a request. The
	 * event carries the buffer, and the result is in udc_buf_info.err.
	 */
	UDC_EVT_EP_REQUEST,
	/**
	 * Non-correctable error event, requires attention from higher
	 * levels or application. The status of the event is a negative errno
	 * value. For example -ENOBUFS means that an endpoint needed a request
	 * and none was queued.
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
	/**
	 * Endpoint to which request is associated. Set by the allocators, a
	 * function or class that uses its own pool sets it before it queues
	 * the request.
	 */
	uint8_t ep;
	/** Flag marks the SETUP stage of a control transfer */
	unsigned int setup : 1;
	/** Flag marks the data stage of a control transfer */
	unsigned int data : 1;
	/** Flag marks the status stage of a control transfer */
	unsigned int status : 1;
	/**
	 * Flag marks ZLP at the end of an IN transfer. ZLPs are used to end
	 * control and bulk transfers. The driver sends a ZLP after the last
	 * data packet and completes the request after the ZLP was sent. The
	 * flag has no meaning for OUT endpoints.
	 */
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
 * @brief Callback to submit UDC event to higher layer.
 *
 * The driver calls the callback to report an event. The callback can be
 * called from interrupt context and must not block.
 *
 * The USB device stack must not lose a UDC_EVT_EP_REQUEST event, because the
 * driver has handed over the buffer with the event and a lost event leaks
 * the buffer. The buffer belongs to the USB device stack after the call, also
 * if the callback returns an error.
 *
 * @param[in] dev      Pointer to device struct of the driver instance
 * @param[in] event    Pointer to event structure
 *
 * @return 0 on success, all other values should be treated as error. The
 *         return value is diagnostic, the driver cannot do anything if the
 *         call fails.
 */
typedef int (*udc_event_cb_t)(const struct device *dev,
			      const struct udc_event *const event);

/**
 * @brief UDC driver API
 *
 * This is the API any USB device controller driver needs to expose, with the
 * exception of device_speed() and test_mode(), which only high-speed
 * controllers need. The common UDC drivers code takes the device lock with
 * lock() around each call of a member, except lock() and unlock() themselves,
 * so the members run with the lock held and in thread context. The checks
 * that the common UDC drivers code makes before it calls a member are listed
 * for each member. The driver must not repeat them.
 */
struct udc_api {
	/**
	 * Get the bus speed. Optional, the common UDC drivers code reports full
	 * speed if it is not set. Called by udc_device_speed(), which has
	 * checked that the controller is enabled.
	 * Return the speed that the bus negotiated, not the speed the
	 * controller is capable of. The value must be valid when the
	 * UDC_EVT_RESET event is generated.
	 */
	enum udc_bus_speed (*device_speed)(const struct device *dev);
	/**
	 * Queue a request. Called by udc_ep_enqueue(), which has checked that
	 * the controller is enabled and that the endpoint exists and is
	 * enabled. The driver puts the request on the endpoint queue and
	 * starts the transfer if the endpoint is idle. An endpoint that is
	 * busy or halted is not an error, the request stays queued. If the
	 * call fails, the request must not be queued.
	 */
	int (*ep_enqueue)(const struct device *dev,
			  struct udc_ep_config *const cfg,
			  struct net_buf *const buf);
	/**
	 * Complete all requests that are queued on the endpoint. Called by
	 * udc_ep_dequeue(), which has checked that the controller is
	 * initialized and the endpoint exists, and calls the member only if
	 * the queue is not empty. Each request gets its own
	 * UDC_EVT_EP_REQUEST event with the result -ECONNABORTED, or with the
	 * real result if the transfer finished on the bus at the same time. A
	 * request must not be completed twice.
	 */
	int (*ep_dequeue)(const struct device *dev,
			  struct udc_ep_config *const cfg);
	/**
	 * Halt the endpoint, it answers with STALL. Called by
	 * udc_ep_set_halt(), which has checked that the controller is enabled
	 * and that the endpoint exists, is enabled and is not isochronous. The
	 * driver sets the halted state of the endpoint after the hardware
	 * call succeeded. The requests that are queued stay queued.
	 */
	int (*ep_set_halt)(const struct device *dev,
			   struct udc_ep_config *const cfg);
	/**
	 * Clear the halt of the endpoint and reset its data toggle. Called by
	 * udc_ep_clear_halt(), which has made the same checks as for
	 * ep_set_halt() and clears the halted state when the driver returns 0.
	 * The driver starts the requests that are queued on the endpoint.
	 */
	int (*ep_clear_halt)(const struct device *dev,
			     struct udc_ep_config *const cfg);
	/**
	 * Reserved for udc_ep_try_config(). The common UDC drivers code does
	 * not call it currently, leave it unset.
	 */
	int (*ep_try_config)(const struct device *dev,
			     struct udc_ep_config *const cfg);
	/**
	 * Enable the endpoint. Called by udc_ep_enable(), which has checked
	 * that the controller is enabled and that the endpoint exists, is not
	 * enabled and that its configuration matches the capabilities of the
	 * endpoint. It has stored attributes, mps and interval in cfg and
	 * cleared the halted state and the data toggle. The driver takes the
	 * resources that the endpoint needs, for example a FIFO or packet
	 * memory, and programs the endpoint. Endpoints can be enabled and
	 * disabled in any order. If the call fails, the driver leaves nothing
	 * behind.
	 */
	int (*ep_enable)(const struct device *dev,
			 struct udc_ep_config *const cfg);
	/**
	 * Disable the endpoint and give back its resources. Called by
	 * udc_ep_disable(), which has checked that the controller is
	 * initialized and that the endpoint exists and is enabled.
	 */
	int (*ep_disable)(const struct device *dev,
			  struct udc_ep_config *const cfg);
	/**
	 * Start remote wakeup signaling. Called by udc_host_wakeup(), which
	 * has checked that the controller is enabled. A controller that cannot
	 * do it returns -ENOTSUP and does not set rwup in struct
	 * udc_device_caps. The driver generates UDC_EVT_RESUME when the
	 * signaling is finished.
	 */
	int (*host_wakeup)(const struct device *dev);
	/**
	 * Set the device address. Called by udc_set_address(), which has
	 * checked that the controller is enabled. See addr_before_status in
	 * struct udc_device_caps.
	 */
	int (*set_address)(const struct device *dev,
			   const uint8_t addr);
	/**
	 * Enable a test mode, or only check if it is supported if dryrun is
	 * set. Optional, only high-speed controllers need it. Called by
	 * udc_test_mode(), which has checked that the controller is enabled
	 * and returns -ENOTSUP if the member is not set.
	 */
	int (*test_mode)(const struct device *dev,
			 const uint8_t mode, const bool dryrun);
	/**
	 * Enable the controller. Called by udc_enable(), which has checked that
	 * the controller is initialized and not enabled. The driver brings up
	 * everything that init() did not: clocks, PHY, configuration,
	 * interrupts and the control endpoints, and last the connection to the
	 * bus. After that the host can detect the device.
	 */
	int (*enable)(const struct device *dev);
	/**
	 * Disable the controller. Called by udc_disable(), which has checked
	 * that the controller is enabled. The device is not visible to the host
	 * any more, the control endpoints are disabled and clocks and PHY are
	 * switched off. The controller can be enabled again without init().
	 */
	int (*disable)(const struct device *dev);
	/**
	 * Initialize the controller. Called by udc_init(), which has checked
	 * the callback and the context and that the controller is not
	 * initialized. Only what the VBUS detection needs should be done here.
	 * The controller, its clocks and its PHY stay off and the host must not
	 * see the device.
	 */
	int (*init)(const struct device *dev);
	/**
	 * Undo init(), the VBUS detection is stopped. Called by udc_shutdown(),
	 * which has checked that the controller is initialized and not enabled.
	 */
	int (*shutdown)(const struct device *dev);
	/**
	 * Lock the device. It cannot fail and must block until it has the
	 * lock. It is called from thread context only.
	 */
	void (*lock)(const struct device *dev);
	/** Unlock the device. */
	void (*unlock)(const struct device *dev);
};

/**
 * Controller is initialized by udc_init() and can generate the VBUS events,
 * if capable, but shall not be recognizable by host.
 * The bit is cleared by udc_shutdown().
 */
#define UDC_STATUS_INITIALIZED		0
/**
 * Controller is enabled and USB device is operational,
 * controller is recognizable by host.
 * The bit is set by udc_enable() and cleared by udc_disable().
 */
#define UDC_STATUS_ENABLED		1
/**
 * Controller is suspended by the host.
 * The bit is set and cleared by the driver, see udc_is_suspended().
 */
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
	/** Callback to submit an UDC event to higher layer */
	udc_event_cb_t event_cb;
	/** Opaque pointer to store higher layer context */
	const void *event_ctx;
	/** USB device controller status */
	atomic_t status;
	/** Driver private data */
	void *priv;
	/** Last cached setup data (not necessarily last received setup data) */
	uint8_t setup[8];
	/** Cached setup data is waiting for USB stack */
	bool setup_pending;
	/** Last cached setup data is valid (8 bytes, CRC OK) */
	bool setup_valid;
};

/**
 * @brief New USB device controller (UDC) driver API
 *
 * @section udc_states Controller states
 *
 * The controller is in one of three states. The description of each call
 * says which state it requires.
 *
 * @code{.unparsed}
 *                   udc_init()                  udc_enable()
 *   +-------------+ ----------> +-------------+ ------------> +---------+
 *   |     not     |             | initialized |               | enabled |
 *   | initialized | <---------- |             | <------------ |         |
 *   +-------------+             +-------------+               +---------+
 *                   udc_shutdown()              udc_disable()
 * @endcode
 *
 * - Not initialized: the initial state, and the state after udc_shutdown().
 * - Initialized: the controller can detect VBUS but is not visible to the
 *   host.
 * - Enabled: Host can detect USB device.
 *
 * The suspended flag is independent of these states. The driver sets it on
 * bus suspend and clears it on resume, see udc_is_suspended().
 *
 * udc_disable() and udc_shutdown() leave the state even if the driver
 * reports an error.
 *
 * @section udc_requests Requests
 *
 * A request is a net_buf whose user data is a struct udc_buf_info. The
 * buffer comes from a pool defined with UDC_BUF_POOL_DEFINE() or
 * UDC_BUF_POOL_VAR_DEFINE(), which provide the alignment and granularity
 * that the controller needs. A function or class implementation can use a
 * pool of its own, or take buffers from the common pool with
 * udc_ep_buf_alloc() and the control allocators. The ep field of
 * udc_buf_info must be set before the request is queued.
 *
 * @code{.unparsed}
 *                    udc_ep_enqueue()     UDC_EVT_EP_REQUEST
 *   USB device stack ------------> driver -------------------> USB device stack
 * @endcode
 *
 * - A request that udc_ep_enqueue() accepted belongs to the driver until the
 *   driver completes it. A request that udc_ep_enqueue() rejected stays with
 *   the caller and no event is generated.
 * - Every accepted request is completed exactly once with a
 *   UDC_EVT_EP_REQUEST event. The event carries the buffer and the result is
 *   in udc_buf_info.err: 0 on success, a negative errno otherwise, see
 *   udc_ep_dequeue() and udc_ep_enqueue().
 * - Requests on an endpoint are completed in the order they were queued.
 * - After the event the USB device stack owns the buffer again. It releases
 *   the buffer with net_buf_unref() or queues it again.
 *
 * @section udc_control Control transfers
 *
 * The UDC driver expects the USB device stack to use one request per stage of
 * a control transfer. The data stage is optional. It is present only if the
 * wLength field of the SETUP packet is not zero. If wLength is zero, there is
 * no data stage and the direction bit of bmRequestType is ignored. The control
 * allocators mark the stage in the udc_buf_info of the request:
 *   - setup: the SETUP stage. The request is queued on control OUT, and the
 *     SETUP packet is added to it, see udc_ep_enqueue().
 *   - data: the data stage. The request is queued on control OUT if the
 *     direction bit of bmRequestType is host to device, and on control IN if
 *     it is device to host.
 *   - status: the status stage. The request is queued on control IN if the
 *     transfer has no data stage or the data stage is host to device, and on
 *     control OUT if the data stage is device to host. A controller that
 *     sends the status OUT stage itself sets out_ack in struct
 *     udc_device_caps, and then the driver does not get a status OUT request.
 *
 * The driver expects the requests in this order:
 *   1. A SETUP buffer. Its queueing tells the driver that the USB device stack
 *      is ready for a new control transfer.
 *   2. After the SETUP buffer is completed, the request for the data stage,
 *      if there is one, and the request for the status stage.
 *   3. The SETUP buffer for the next control transfer, as soon as the last
 *      request of the current transfer is queued. The status stage can still
 *      be in progress at that time.
 *
 * A new SETUP packet from the host cancels the requests of the previous
 * control transfer, see udc_ep_enqueue().
 *
 * @defgroup udc_api USB Device Controller
 * @ingroup usb_interfaces
 * @since 3.3
 * @version 0.1.1
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
	struct udc_data *data = (struct udc_data *)dev->data;

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
	struct udc_data *data = (struct udc_data *)dev->data;

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
	struct udc_data *data = (struct udc_data *)dev->data;

	return atomic_test_bit(&data->status, UDC_STATUS_SUSPENDED);
}

/**
 * @brief Initialize USB device controller
 *
 * Initialize USB device controller.
 * After initialization controller driver should be able to detect
 * power state of the bus and signal power state changes.
 *
 * The callback and the context must not be NULL. udc_init() stores both
 * before the driver is initialized, and they must stay valid until
 * udc_shutdown().
 *
 * @param[in] dev       Pointer to device struct of the driver instance
 * @param[in] event_cb  Event callback from the higher layer (USB device stack)
 * @param[in] event_ctx Opaque pointer to higher layer context
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EINVAL on parameter error (no callback or context is passed)
 * @retval -EALREADY already initialized
 */
int udc_init(const struct device *dev,
	     udc_event_cb_t event_cb, const void *const event_ctx);

/**
 * @brief Enable USB device controller
 *
 * Enable powered USB device controller and allow host to
 * recognize and enumerate the device. The controller must be initialized.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not initialized
 * @retval -EALREADY already enabled
 * @retval -ETIMEDOUT enable operation timed out
 */
int udc_enable(const struct device *dev);

/**
 * @brief Disable USB device controller
 *
 * Disable enabled USB device controller.
 * The driver should continue to detect power state changes.
 * The controller is not enabled after the call, even if the driver reports
 * an error. The call does not complete the requests that are queued on the
 * endpoints.
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
 * or to change the role of the controller. The controller must be disabled.
 * It is not initialized after the call, even if the driver reports an error.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EBUSY controller is enabled
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
	struct udc_data *data = (struct udc_data *)dev->data;

	return data->caps;
}

/**
 * @brief Get actual USB device speed
 *
 * The function should be called after the reset event to determine
 * the actual bus speed. The call requires an enabled controller.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return Actual bus speed, UDC_BUS_UNKNOWN if the controller is not enabled.
 */
enum udc_bus_speed udc_device_speed(const struct device *dev);

/**
 * @brief Set USB device address.
 *
 * Set address of the USB device. The call requires an enabled controller.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] addr   USB device address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not enabled
 */
static inline int udc_set_address(const struct device *dev, const uint8_t addr)
{
	const struct udc_api *api = (const struct udc_api *)dev->api;
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
 * The call requires an enabled controller.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] mode   Test mode
 * @param[in] dryrun Verify that a particular mode can be enabled, but do not
 *                   enable test mode
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not enabled
 * @retval -ENOTSUP Test mode is not supported
 */
static inline int udc_test_mode(const struct device *dev,
				const uint8_t mode, const bool dryrun)
{
	const struct udc_api *api = (const struct udc_api *)dev->api;
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
 * The call requires an enabled controller.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not enabled
 */
static inline int udc_host_wakeup(const struct device *dev)
{
	const struct udc_api *api = (const struct udc_api *)dev->api;
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
 * All properties of the descriptor, such as direction and transfer type,
 * should be set correctly.
 *
 * The call has no requirement on the controller state and does not change
 * the endpoint.
 *
 * If the value of mps is zero and the configuration is supported, the call
 * replaces it with the largest size the endpoint supports for this transfer
 * type, limited to the maximum of the specification for bulk and interrupt
 * endpoints. For control and isochronous endpoints a zero value is not
 * replaced.
 *
 * @param[in] dev        Pointer to device struct of the driver instance
 * @param[in] ep         Endpoint address (same as bEndpointAddress)
 * @param[in] attributes Endpoint attributes (same as bmAttributes)
 * @param[in] mps        Maximum packet size (same as wMaxPacketSize)
 * @param[in] interval   Polling interval (same as bInterval)
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found
 * @retval -ENOTSUP endpoint configuration not supported
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
 * Use it for all endpoints except control IN/OUT. The driver configures and
 * enables the control endpoints in its enable implementation.
 *
 * The call requires an enabled controller and an endpoint that is not
 * enabled. To change the configuration of an enabled endpoint, disable it
 * first with udc_ep_disable().
 *
 * On success the endpoint stores the attributes, the maximum packet size and
 * the interval, and its halt state and data toggle are reset.
 * A failed call does not change whether the endpoint is enabled.
 *
 * @param[in] dev        Pointer to device struct of the driver instance
 * @param[in] ep         Endpoint address (same as bEndpointAddress)
 * @param[in] attributes Endpoint attributes (same as bmAttributes)
 * @param[in] mps        Maximum packet size (same as wMaxPacketSize)
 * @param[in] interval   Polling interval (same as bInterval)
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EINVAL on wrong parameter (control IN/OUT endpoint)
 * @retval -EPERM controller is not enabled
 * @retval -ENODEV endpoint configuration not found or not valid
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
 * Use it for all endpoints except control IN/OUT. The driver configures and
 * enables the control endpoints in its enable implementation and disables
 * them in its disable implementation.
 *
 * The call requires an initialized controller and an endpoint that is
 * enabled. On success the endpoint is disabled.
 * A failed call does not change whether the endpoint is enabled.
 * Requests that are queued on the endpoint are not removed, use
 * udc_ep_dequeue() for that.
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
 * Applies to enabled endpoints, except isochronous endpoints.
 * The call requires an enabled controller.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found or endpoint is not enabled
 * @retval -ENOTSUP not supported (e.g. isochronous endpoint)
 * @retval -EPERM controller is not enabled
 */
int udc_ep_set_halt(const struct device *dev, const uint8_t ep);

/**
 * @brief Clear endpoint halt
 *
 * Applies to enabled endpoints, except isochronous endpoints.
 * The call requires an enabled controller.
 * On success the halt state of the endpoint is cleared and the requests
 * that are queued on the endpoint are restarted.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found or endpoint is not enabled
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
 * The call requires an enabled controller and an enabled endpoint. The
 * endpoint is taken from the ep field of the udc_buf_info of the buffer.
 * The call only queues the request. An endpoint that is busy with another
 * request is not an error.
 *
 * A request that the call accepts belongs to the driver until it is
 * completed with a UDC_EVT_EP_REQUEST event. If the call fails, the request
 * stays with the caller and no event is generated.
 *
 * The host can send a SETUP packet at any time, also when the USB device
 * stack has not queued a SETUP buffer. The USB device stack receives the SETUP
 * packet in a SETUP buffer that it queued with udc_ep_enqueue(). The 8 bytes
 * of the packet are added to the data of the buffer, and the buffer is
 * completed with a UDC_EVT_EP_REQUEST event and result 0.
 *
 * The driver reports every SETUP packet by calling udc_setup_received(),
 * which does the following:
 *   1. All data and status requests that are still queued on the control
 *      endpoints belong to the previous control transfer. Each of them is
 *      completed with a UDC_EVT_EP_REQUEST event and the result -ECONNRESET.
 *      A queued SETUP buffer stays queued.
 *   2. If a SETUP buffer is queued, the 8 bytes are added to it and it is
 *      completed as described above.
 *   3. If no SETUP buffer is queued, the 8 bytes are kept and the SETUP
 *      packet is pending until the USB device stack queues a SETUP buffer. A
 *      newer SETUP packet replaces a pending one.
 *   4. While a SETUP packet is pending, udc_ep_enqueue() does not queue a
 *      request for a control endpoint. It generates the
 *      UDC_EVT_EP_REQUEST event itself, before the call returns:
 *      - For a SETUP buffer, the 8 bytes of the pending SETUP packet are added
 *        to the buffer and the result is 0. The SETUP packet is no longer
 *        pending.
 *      - For a data or status buffer, the result is -ECONNRESET. The buffer
 *        belongs to a control transfer that the host abandoned by sending
 *        the pending SETUP packet. The SETUP packet stays pending until the
 *        USB device stack queues a SETUP buffer.
 * A SETUP packet that is not valid, for example with a CRC error, is
 * reported with udc_setup_received() and a NULL pointer instead of the data.
 * It is handled in the same way, but the SETUP buffer is completed without
 * data.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] buf    Pointer to UDC request buffer
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found or endpoint is not enabled
 * @retval -EPERM controller is not enabled
 */
int udc_ep_enqueue(const struct device *dev, struct net_buf *const buf);

/**
 * @brief Purges control endpoint queues after controller shutdown
 *
 * The call requires a controller that is not initialized, so call it after
 * udc_shutdown(). It releases the requests that are still queued on the
 * control IN and control OUT endpoints. No UDC_EVT_EP_REQUEST event is
 * generated for them. The queues of the other endpoints are not touched.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EBUSY controller is initialized
 */
int udc_purge_queues(const struct device *dev);

/**
 * @brief Determines if endpoint queue is empty
 *
 * Returns true also if the endpoint configuration does not exist. The call
 * has no requirement on the controller state.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return true if endpoint queue is empty, false otherwise
 */
bool udc_ep_queue_is_empty(const struct device *dev, const uint8_t ep);

/**
 * @brief Remove all USB device controller requests from endpoint queue
 *
 * Every request that was queued on the endpoint when the call was made is
 * completed with its own UDC_EVT_EP_REQUEST event, in the order it was
 * queued. The result is -ECONNABORTED, or the real result if the transfer
 * finished on the bus at the same time.
 *
 * The call requires an initialized controller.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint configuration not found
 * @retval -EPERM controller is not initialized
 */
int udc_ep_dequeue(const struct device *dev, const uint8_t ep);

/**
 * @brief Allocate UDC request buffer
 *
 * Allocate a new buffer from common request buffer pool and set the
 * endpoint address in its udc_buf_info. The call does not wait.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 * @param[in] size   Size of the request buffer
 *
 * @return pointer to allocated request or NULL if no buffer is available.
 */
struct net_buf *udc_ep_buf_alloc(const struct device *dev,
				 const uint8_t ep,
				 const size_t size);

/**
 * @brief Allocate UDC control transfer SETUP buffer
 *
 * Allocate a new buffer from common request buffer pool. The buffer has the
 * setup flag set. Its size is the size of the control endpoint.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return pointer to allocated request or NULL if no buffer is available.
 */
struct net_buf *udc_ctrl_setup_alloc(const struct device *dev);

/**
 * @brief Allocate UDC control transfer data stage buffer
 *
 * Allocate a new buffer from common request buffer pool. The buffer has the
 * data flag set. For control OUT the size is rounded up to a multiple of the
 * size of the control endpoint.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Control endpoint address
 * @param[in] size   Size of the request buffer
 *
 * @return pointer to allocated request or NULL if no buffer is available.
 */
struct net_buf *udc_ctrl_data_alloc(const struct device *dev,
				    const uint8_t ep,
				    const size_t size);

/**
 * @brief Allocate UDC control transfer status stage buffer
 *
 * Allocate a new buffer from common request buffer pool. The buffer has the
 * status flag set. For control OUT the size is the size of the control
 * endpoint, for control IN it is zero.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Control endpoint address
 *
 * @return pointer to allocated request or NULL if no buffer is available.
 */
struct net_buf *udc_ctrl_status_alloc(const struct device *dev,
				      const uint8_t ep);

/**
 * @brief Free UDC request buffer
 *
 * Put the buffer back into its pool. The call does the same as
 * net_buf_unref() under the device lock, prefer net_buf_unref().
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
 * The controller should send a ZLP at the end of the transfer. ZLPs are used
 * to end control and bulk transfers. The flag is set only if the request is
 * for an IN endpoint, otherwise the call does nothing.
 *
 * For the data stage of a control read, the flag is set if the host asked for
 * more data than the request holds and the length of the request is a
 * multiple of the size of the control endpoint. Functions and classes set the
 * flag for IN requests where their protocol needs a ZLP to end the transfer.
 *
 * A request with length 0 for an IN endpoint is sent as one ZLP.
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
 * @brief Get pointer to higher layer context
 *
 * The address of the context is passed as an argument to the udc_init()
 * function and is stored in the UDC data.
 *
 * @param[in] dev Pointer to device struct of the driver instance
 *
 * @return Opaque pointer to higher layer context
 */
static inline const void *udc_get_event_ctx(const struct device *dev)
{
	struct udc_data *data = (struct udc_data *)dev->data;

	return data->event_ctx;
}

/**
 * @brief Get endpoint size from UDC endpoint configuration
 *
 * @param[in] cfg Pointer to UDC endpoint configuration
 *
 * @return Endpoint size
 */
static inline uint16_t udc_mps_ep_size(const struct udc_ep_config *const cfg)
{
	return USB_MPS_EP_SIZE(cfg->mps);
}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_UDC_H */
