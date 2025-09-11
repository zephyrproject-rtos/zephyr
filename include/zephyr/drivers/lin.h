/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup lin_controller
 * @brief LIN (Local Interconnect Network) driver API
 *
 * This file contains definitions and API for LIN communication.
 */
#ifndef ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_LIN_H
#define ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_LIN_H

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for Local Interconnect Network (LIN) controllers and transceivers
 * @defgroup lin_controller LIN controller
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief LIN frame definitions
 * @{
 */

/**
 * @brief Bit mask for a LIN identifier.
 */
#define LIN_ID_MASK 0x3FU

/**
 * @cond INTERNAL_HIDDEN
 * Internally calculated maximum data length
 */
/** Maximum data length for LIN messages */
#define LIN_MAX_DLEN 8U

/** @endcond */

/** @} */

/**
 * @brief LIN device configuration flags
 * @anchor LIN_CONFIG_FLAGS
 *
 * @{
 */
#define LIN_BUS_CONFLICT_DETECTION BIT(0) /**< LIN bus conflict detection enabled */
#define LIN_BUS_AUTO_SYNC          BIT(1) /**< LIN bus auto synchronization enabled */

/** @} */

/**
 * @brief Provide a type to hold LIN controller configuration flags.
 *
 * @see @ref LIN_CONFIG_FLAGS.
 */
typedef uint32_t lin_config_flags_t;

/**
 * @brief LIN checksum type
 */
enum lin_checksum_type {
	LIN_CHECKSUM_CLASSIC = 0, /**< Classic checksum */
	LIN_CHECKSUM_ENHANCED,    /**< Enhanced checksum */
};

/**
 * @brief LIN event types
 */
enum lin_event_type {
	LIN_EVT_RX_HEADER = 0, /**< Header received event (responder mode only). */
	LIN_EVT_TX_HEADER,     /**< Header transmission complete event (commander mode only) */
	LIN_EVT_RX_DATA,       /**< Data received event. */
	LIN_EVT_TX_DATA,       /**< Data transmission complete event */
	LIN_EVT_TX_WAKEUP,     /**< Transmit wake up complete event */
	LIN_EVT_RX_WAKEUP,     /**< Receive wake up complete event */
	LIN_EVT_ERR,           /**< Error event */
};

/**
 * @brief LIN error flags
 * @anchor LIN_ERR_FLAGS
 */
#define LIN_ERR_BUS_COLLISION    BIT(0) /**< LIN bus collision detected */
#define LIN_ERR_INVALID_CHECKSUM BIT(1) /**< LIN checksum error */
#define LIN_ERR_COUNTER_OVERFLOW BIT(2) /**< LIN counter overflow */
#define LIN_ERR_OVERRUN          BIT(3) /**< LIN overrun error */
#define LIN_ERR_PARITY           BIT(4) /**< LIN parity error */
#define LIN_ERR_FRAMING          BIT(5) /**< LIN framing error */

/**
 * @brief Provide a type to hold LIN error flags.
 *
 * @see @ref LIN_ERR_FLAGS.
 */
typedef uint32_t lin_error_flags_t;

/**
 * @brief LIN operation mode
 */
enum lin_mode {
	LIN_MODE_COMMANDER = 0, /** LIN commander node */
	LIN_MODE_RESPONDER,     /** LIN responder node */
};

/**
 * @brief LIN configuration structure
 */
struct lin_config {
	/** LIN operation mode */
	enum lin_mode mode;
	/** LIN baud rate */
	uint32_t baudrate;
	/** LIN break field bit length */
	size_t break_len;
	/** LIN break delimiter bit length */
	size_t break_delimiter_len;
	/** LIN flags */
	lin_config_flags_t flags;
};

/**
 * @brief LIN message structure
 */
struct lin_msg {
	/** LIN message ID */
	uint8_t id;
	/** Flags for the message */
	uint8_t flags;
	/** Data length for the message */
	uint8_t data_len;
	/** Pointer to the message data */
	void *data;
	/** Checksum type for the message */
	enum lin_checksum_type checksum_type;
};

/**
 * @brief LIN event structure
 *
 * This structure is used to pass LIN events to the application.
 */
struct lin_event {
	/** LIN event type */
	enum lin_event_type type;
	/** LIN status code, if any */
	int status;
	union {
		/** LIN header event context, if any */
		struct {
			/** LIN Protected ID */
			uint8_t pid;
		} header;
		/** LIN data event context, if any */
		struct {
			/** LIN Protected ID */
			uint8_t pid;
			/** Number of data bytes received */
			uint8_t bytes_received;
			/** Received checksum */
			uint8_t checksum;
		} data;
		/** LIN error event flags, if any */
		lin_error_flags_t error_flags;
	};
};

/**
 * @brief LIN event callback function type
 *
 * This function is called when a LIN event occurs.
 *
 * @param dev Pointer to the LIN device structure
 * @param event Pointer to the LIN event structure
 * @param user_data Pointer to arbitrary user data.
 */
typedef void (*lin_event_callback_t)(const struct device *dev, const struct lin_event *event,
				     void *user_data);

/**
 * @brief LIN frame filter configuration (responder mode only)
 *
 * This structure is used to define a LIN frame filter for receiving messages.
 */
struct lin_filter {
	/** LIN PID to match */
	uint8_t primary_pid;
	/** Secondary PID, if any. Set to 0x00 if not used */
	uint8_t secondary_pid;
	/** LIN mask for ID matching */
	uint8_t mask;
};

/**
 * @brief Callback API for starting LIN controller.
 * See @a lin_start() for argument description.
 */
typedef int (*lin_start_t)(const struct device *dev);

/**
 * @brief Callback API for stopping LIN controller.
 * See @a lin_stop() for argument description.
 */
typedef int (*lin_stop_t)(const struct device *dev);

/**
 * @brief Callback API for configuring LIN controller.
 * See @a lin_configure() for argument description.
 */
typedef int (*lin_configure_t)(const struct device *dev, const struct lin_config *config);

/**
 * @brief Callback API for getting LIN configuration.
 * See @a lin_get_config() for argument description.
 */
typedef int (*lin_get_config_t)(const struct device *dev, struct lin_config *config);

/**
 * @brief Callback API for sending LIN message.
 * See @a lin_send() for argument description.
 */
typedef int (*lin_send_t)(const struct device *dev, const struct lin_msg *msg, k_timeout_t timeout);

/**
 * @brief Callback API for receiving LIN message.
 * See @a lin_receive() for argument description.
 */
typedef int (*lin_receive_t)(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout);

/**
 * @brief Callback API for processing LIN response messages.
 * See @a lin_response() for argument description.
 */
typedef int (*lin_response_t)(const struct device *dev, const struct lin_msg *msg,
			      k_timeout_t timeout);

/**
 * @brief Callback API for reading LIN response message
 * See @a lin_read() for argument description.
 */
typedef int (*lin_read_t)(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout);

/**
 * @brief Callback API for sending a LIN wakeup signal.
 * See @a lin_wakeup_send() for argument description.
 */
typedef int (*lin_wakeup_send_t)(const struct device *dev);

/**
 * @brief Callback API for placing the LIN node in bus sleep mode or waking it up.
 * See @a lin_enter_sleep() for argument description.
 */
typedef int (*lin_enter_sleep_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API for setting LIN transmit event callback.
 * See @a lin_set_callback() for argument description.
 */
typedef int (*lin_set_callback_t)(const struct device *dev, lin_event_callback_t callback,
				  void *user_data);

/**
 * @brief Callback API for setting a LIN RX filter.
 * See @a lin_set_rx_filter() for argument description.
 */
typedef int (*lin_set_rx_filter_t)(const struct device *dev, const struct lin_filter *filter);

/**
 * @brief Common LIN driver configuration structure.
 *
 * This structure is used to configure LIN devices and is expected to be the first element in the
 * object pointed to by the config field in the device structure.
 */
struct lin_driver_config {
	/** Pointer to the device structure for the associated LIN transceiver device or NULL. */
	const struct device *phy;
	/** Minimum supported bitrate by the LIN controller/transceiver combination. */
	uint32_t min_bitrate;
	/** Maximum supported bitrate by the LIN controller/transceiver combination. */
	uint32_t max_bitrate;
	/** Initial LIN bitrate. */
	uint32_t bitrate;
};

/**
 * @brief Static initializer for @p lin_driver_config struct
 *
 * @param node_id Devicetree node identifier
 * @param _min_bitrate minimum bitrate supported by the LIN controller
 * @param _max_bitrate maximum bitrate supported by the LIN controller
 */
#define LIN_DT_DRIVER_CONFIG_GET(node_id, _min_bitrate, _max_bitrate)                              \
	{                                                                                          \
		.phy = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(node_id, phys)),                           \
		.min_bitrate = DT_PROP_OR(node_id, min_bitrate, _min_bitrate),                     \
		.max_bitrate = DT_PROP_OR(node_id, max_bitrate, _max_bitrate),                     \
		.bitrate = DT_PROP_OR(node_id, bitrate, CONFIG_LIN_DEFAULT_BITRATE),               \
	}

/**
 * @brief Static initializer for @p lin_driver_config struct from DT_DRV_COMPAT instance
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param _min_bitrate minimum bitrate supported by the LIN controller
 * @param _max_bitrate maximum bitrate supported by the LIN controller
 * @see LIN_DT_DRIVER_CONFIG_GET()
 */
#define LIN_DT_DRIVER_CONFIG_INST_GET(inst, _min_bitrate, _max_bitrate)                            \
	LIN_DT_DRIVER_CONFIG_GET(DT_DRV_INST(inst), _min_bitrate, _max_bitrate)

/**
 * @brief Common LIN driver data structure
 *
 * This structure is used to hold LIN driver state and is expected to be the first element in the
 * object pointed to by the data field in the device structure.
 */
struct lin_driver_data {
	/** Current LIN controller configuration. */
	struct lin_config config;
	/** True if LIN controller is started, false otherwise. */
	bool started;
	/** LIN event callback function pointer or NULL. */
	lin_event_callback_t callback;
	/** User data pointer passed to the callback function. */
	void *callback_data;
};

/**
 * @brief LIN driver API structure
 */
__subsystem struct lin_driver_api {
	lin_start_t start;
	lin_stop_t stop;
	lin_configure_t configure;
	lin_get_config_t get_config;
	lin_set_rx_filter_t set_rx_filter;
	lin_send_t send;
	lin_receive_t receive;
	lin_response_t response;
	lin_read_t read;
	lin_wakeup_send_t wakeup_send;
	lin_enter_sleep_t enter_sleep;
	lin_set_callback_t set_callback;
};

/**
 * @brief Helper function to extract LIN frame identifier from protected identifier (PID).
 *
 * @param pid Protected identifier
 * @return uint8_t Frame identifier
 */
static inline uint8_t lin_get_frame_id(uint8_t pid)
{
	return pid & LIN_ID_MASK;
}

/**
 * @brief Helper function to compute LIN protected identifier (PID) from frame identifier.
 *
 * @param id Frame identifier
 * @return uint8_t Protected identifier
 */
static inline uint8_t lin_compute_pid(uint8_t id)
{
	uint8_t p0 = (FIELD_GET(BIT(0), id) ^ FIELD_GET(BIT(1), id) ^ FIELD_GET(BIT(2), id) ^
		      FIELD_GET(BIT(4), id)) &
		     0x1U;
	uint8_t p1 = ~(FIELD_GET(BIT(1), id) ^ FIELD_GET(BIT(3), id) ^ FIELD_GET(BIT(4), id) ^
		       FIELD_GET(BIT(5), id)) &
		     0x1U;

	return (id & LIN_ID_MASK) | (p0 << 6) | (p1 << 7);
}

/**
 * @brief Helper function to verify LIN protected identifier (PID).
 *
 * @param pid Protected identifier to verify
 * @return true if the PID is valid, false otherwise
 */
static inline bool lin_verify_pid(uint8_t pid)
{
	uint8_t id = lin_get_frame_id(pid);

	return lin_compute_pid(id) == pid;
}

/**
 * @brief Get the LIN transceiver device associated with the LIN controller.
 *
 * This function retrieves the LIN transceiver device associated with the specified LIN controller.
 *
 * @param dev Pointer to the LIN device structure
 * @return Pointer to the LIN transceiver device structure, or NULL if not available
 */
__syscall const struct device *lin_get_transceiver(const struct device *dev);

static const struct device *z_impl_lin_get_transceiver(const struct device *dev)
{
	const struct lin_driver_config *config = dev->config;

	return config->phy;
}

/**
 * @brief Start LIN controller.
 *
 * This function brings the LIN controller to active state, allowing it to participate in
 * LIN communication. It also enables the LIN transceiver (if supported).
 *
 * @param dev Pointer to the LIN device structure
 * @retval 0 if successful.
 * @retval -EALREADY if the LIN controller is already started.
 * @retval negative errno code on failure.
 */
__syscall int lin_start(const struct device *dev);

static inline int z_impl_lin_start(const struct device *dev)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->start == NULL) {
		return -ENOSYS;
	}

	return api->start(dev);
}

/**
 * @brief Stop LIN controller.
 *
 * This function brings the LIN controller into the deactivated state, preventing it from
 * participating in LIN communication. It also disables the LIN transceiver (if supported).
 *
 * @param dev Pointer to the LIN device structure
 * @retval 0 on success
 * @retval -EALREADY if the LIN controller is already stopped.
 * @retval negative errno code on failure
 */
__syscall int lin_stop(const struct device *dev);

static inline int z_impl_lin_stop(const struct device *dev)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->stop == NULL) {
		return -ENOSYS;
	}

	return api->stop(dev);
}

/**
 * @brief Configure LIN controller.
 *
 * This function configures the LIN controller with the given settings.
 *
 * @param dev Pointer to the LIN device structure
 * @param config Pointer to the configuration structure
 * @return 0 on success, negative errno code on failure
 */
__syscall int lin_configure(const struct device *dev, const struct lin_config *config);

static inline int z_impl_lin_configure(const struct device *dev, const struct lin_config *config)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->configure == NULL) {
		return -ENOSYS;
	}

	return api->configure(dev, config);
}

/**
 * @brief Get current LIN configuration.
 *
 * This function retrieves the current configuration of the LIN controller.
 *
 * @param dev Pointer to the LIN device structure
 * @param config Pointer to configuration structure
 * @return 0 on success, negative errno code on failure
 */
__syscall int lin_get_config(const struct device *dev, struct lin_config *config);

static inline int z_impl_lin_get_config(const struct device *dev, struct lin_config *config)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->get_config == NULL) {
		return -ENOSYS;
	}

	return api->get_config(dev, config);
}

/**
 * @brief Send LIN message (commander mode only).
 *
 * This function setup an asynchronous LIN write transaction with a specified LIN identifier.
 * When device work in commander mode:
 * - Send a header with the specified ID.
 * - Send the data payload (optional). Leave the msg->data_len as 0 in case response data is not
 * needed.
 *
 * @param dev Pointer to LIN device structure
 * @param msg Pointer to input LIN message structure
 * @param timeout Timeout waiting to complete transmission
 * @retval 0 on success
 * @retval -EPERM if the device not in commander mode
 * @retval negative errno code on failure
 */
__syscall int lin_send(const struct device *dev, const struct lin_msg *msg, k_timeout_t timeout);

static inline int z_impl_lin_send(const struct device *dev, const struct lin_msg *msg,
				  k_timeout_t timeout)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->send == NULL) {
		return -ENOSYS;
	}

	return api->send(dev, msg, timeout);
}

/**
 * @brief Receive LIN message (commander mode only).
 *
 * This function setup an asynchronous LIN read transaction with a specified LIN identifier.
 * When device work in commander mode:
 * - Send a header with specified ID for data requesting.
 * - Read back data from responder.
 *
 * @param dev Pointer to the LIN device structure
 * @param msg Pointer to the LIN message structure
 * @param timeout Timeout waiting to complete transmission
 * @retval 0 on success
 * @retval -EPERM if the device is not in commander mode
 * @retval negative errno code on failure
 */
__syscall int lin_receive(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout);

static inline int z_impl_lin_receive(const struct device *dev, struct lin_msg *msg,
				     k_timeout_t timeout)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->receive == NULL) {
		return -ENOSYS;
	}

	return api->receive(dev, msg, timeout);
}

/**
 * @brief Respond to a received header (responder mode only).
 *
 * This is an interrupt-driven API for a responder node to respond to a LIN header sent by the
 * commander node. For a responder node, it should be called when a LIN_EVT_RX_HEADER event occurs
 * to publish the response for the received header.
 *
 * Only call this API if a header has been received.
 *
 * @param dev Pointer to the LIN device structure
 * @param msg Pointer to the LIN message structure
 * @param timeout Timeout waiting for the response to complete
 * @retval 0 on success
 * @retval -EPERM if the device is not in responder mode
 * @retval -EFAULT if there is no header received
 * @retval negative errno code on failure
 */
__syscall int lin_response(const struct device *dev, const struct lin_msg *msg,
			   k_timeout_t timeout);

static inline int z_impl_lin_response(const struct device *dev, const struct lin_msg *msg,
				      k_timeout_t timeout)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->response == NULL) {
		return -ENOSYS;
	}

	return api->response(dev, msg, timeout);
}

/**
 * @brief Start data reception (responder mode only)
 *
 * This is an interrupt-driven API that a responder node uses to initiate a non-blocking
 * reception of a LIN message's data. It should be called when a LIN_EVT_RX_HEADER
 * event occurs to receive the response for the corresponding header.
 *
 * Only call this API if a header has been received.
 *
 * @param dev Pointer to the LIN device structure
 * @param msg Pointer to the LIN message structure
 * @param timeout Timeout waiting for reception to complete
 * @retval 0 on success
 * @retval -EPERM if the device is not in responder mode
 * @retval -EFAULT if there is no header received
 * @retval negative errno code on failure
 */
__syscall int lin_read(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout);

static inline int z_impl_lin_read(const struct device *dev, struct lin_msg *msg,
				  k_timeout_t timeout)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->read == NULL) {
		return -ENOSYS;
	}

	return api->read(dev, msg, timeout);
}

/**
 * @brief Send LIN wakeup signal.
 *
 * Drive the bus dominant for the duration required by the LIN specification
 * so sleeping nodes wake up.
 *
 * @param dev LIN device
 * @return 0 on success, negative errno on failure
 */
__syscall int lin_wakeup_send(const struct device *dev);

static inline int z_impl_lin_wakeup_send(const struct device *dev)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->wakeup_send == NULL) {
		return -ENOSYS;
	}

	return api->wakeup_send(dev);
}

/**
 * @brief Enter or exit sleep mode on the LIN device.
 *
 * This function enables or disables sleep mode for the LIN device, depending on the value of @p
 * enable.
 *
 * It is typically called when a wakeup signal or a sleep command is detected on the LIN bus.
 *
 * @param dev Pointer to the LIN device structure.
 * @param enable true to enable sleep mode, false to disable it.
 * @return 0 on success, negative error code on failure.
 */
__syscall int lin_enter_sleep(const struct device *dev, bool enable);

static inline int z_impl_lin_enter_sleep(const struct device *dev, bool enable)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->enter_sleep == NULL) {
		return -ENOSYS;
	}

	return api->enter_sleep(dev, enable);
}

/**
 * @brief Set LIN event callback.
 *
 * This function sets a callback that will be called on LIN events.
 *
 * @param dev Pointer to the LIN device structure.
 * @param callback Pointer to the callback function.
 * @param user_data Pointer to user data passed to the callback.
 * @return 0 on success, negative error code on failure.
 */
__syscall int lin_set_callback(const struct device *dev, lin_event_callback_t callback,
			       void *user_data);

static inline int z_impl_lin_set_callback(const struct device *dev, lin_event_callback_t callback,
					  void *user_data)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->set_callback == NULL) {
		return -ENOSYS;
	}

	return api->set_callback(dev, callback, user_data);
}

/**
 * @brief Set a LIN RX filter (responder mode only).
 *
 * This function sets a header filter for LIN messages based on ID and mask. In case there is no
 * filter set, all incoming messages are accepted.
 *
 * When an accepted header is received, a LIN_EVT_RX_HEADER event is generated.
 *
 * @param dev Pointer to the LIN device structure.
 * @param filter Pointer to the LIN filter structure.
 * @retval 0 on success
 * @retval -EPERM in case device is in commander mode.
 * @retval negative error code on failure.
 */
__syscall int lin_set_rx_filter(const struct device *dev, const struct lin_filter *filter);

static inline int z_impl_lin_set_rx_filter(const struct device *dev,
					   const struct lin_filter *filter)
{
	const struct lin_driver_api *api = DEVICE_API_GET(lin, dev);

	if (api->set_rx_filter == NULL) {
		return -ENOSYS;
	}

	return api->set_rx_filter(dev, filter);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/lin.h>

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_LIN_H */
