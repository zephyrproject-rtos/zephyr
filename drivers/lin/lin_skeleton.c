/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Skeleton implementation of LIN controller driver */

#include <soc.h>
#include <zephyr/drivers/lin.h>
#include <zephyr/drivers/lin/transceiver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lin_skeleton, CONFIG_LIN_LOG_LEVEL);

/*
 * Structure for holding controller configuration items that can remain in
 * non-volatile memory. This is usually accessed as
 *   const struct lin_skeleton_config *config = dev->config;
 */
struct lin_skeleton_config {
	struct lin_driver_config common;
};

/*
 * Structure to hold driver data. This is usually accessed as
 *   struct lin_skeleton_data *data = dev->data;
 */
struct lin_skeleton_data {
	struct lin_driver_data common;
	const struct device *dev;
	struct lin_filter rx_filter;
	struct k_work_delayable tx_timeout_work;
	struct k_work_delayable rx_timeout_work;
};

/*
 * Bring the LIN skeleton out of stopped state, allowing it to participate in LIN communication.
 */
static int lin_skeleton_start(const struct device *dev)
{
	const struct lin_skeleton_config *config = dev->config;
	struct lin_skeleton_data *data = dev->data;
	const struct device *phy = config->common.phy;
	int ret;

	/* Check if device is already started */
	if (data->common.started) {
		LOG_ERR("LIN controller is already started");
		return -EALREADY;
	}

	/* Start up the transceiver before enabling the LIN controller */
	if (phy) {
		ret = lin_transceiver_enable(phy, 0);
		if (ret) {
			LOG_ERR("Failed to enable LIN transceiver");
			return ret;
		}
	}

	/* Code to bring the device into the operational state */

	/* Set the device state to started */
	data->common.started = true;

	return 0;
}

/**
 * Bring the LIN skeleton to stopped state, disabling the transceiver and
 * the LIN controller from communicating.
 */
static int lin_skeleton_stop(const struct device *dev)
{
	const struct lin_skeleton_config *config = dev->config;
	struct lin_skeleton_data *data = dev->data;
	const struct device *phy = config->common.phy;
	int ret;

	/* Check if device is already stopped */
	if (!data->common.started) {
		LOG_ERR("LIN controller is already stopped");
		return -EALREADY;
	}

	/* Code to bring the device into the stopped state */

	/* Disable the transceiver after stopping the LIN controller */
	if (phy) {
		ret = lin_transceiver_disable(phy);
		if (ret) {
			LOG_ERR("Failed to disable LIN transceiver");
			return ret;
		}
	}

	/* Set the device state to stopped */
	data->common.started = false;

	return 0;
}

/**
 * Configure the LIN skeleton with the given configuration.
 * The configuration is applied only if the device is not started.
 */
static int lin_skeleton_configure(const struct device *dev, const struct lin_config *config)
{
	struct lin_skeleton_data *data = dev->data;

	/* Check if the device is started */
	if (data->common.started) {
		LOG_ERR("Cannot configure LIN controller while it is started");
		return -EBUSY;
	}

	/* Code to apply the configuration to the LIN controller */

	/* Store the configuration */
	data->common.config = *config;

	return 0;
}

/**
 * Get the current configuration of the LIN skeleton.
 */
static int lin_skeleton_get_config(const struct device *dev, struct lin_config *config)
{
	const struct lin_skeleton_data *data = dev->data;

	/* Copy the current configuration to the provided structure */
	*config = data->common.config;

	return 0;
}

/**
 * Add hardware filter rule for event triggered when receiving LIN header (responder only)
 *
 * In case no rule is set, the default behavior is that the event will be fired when any header is
 * received.
 */
static int lin_skeleton_set_rx_filter(const struct device *dev, const struct lin_filter *filter)
{
	struct lin_skeleton_data *data = dev->data;

	/* Code to set the RX filter for the LIN controller */

	/* Store the RX filter */
	data->rx_filter = *filter;

	return 0;
}

/**
 * Send a LIN header and a LIN response (if any)
 *
 * If the device is commander, the header will always transmitted on the bus, then following with
 * the data field. In case only header should be sent, left msg->data_len as 0
 */
static int lin_skeleton_send(const struct device *dev, const struct lin_msg *msg,
			     k_timeout_t timeout)
{
	struct lin_skeleton_data *data = dev->data;

	if (data->common.config.mode != LIN_MODE_COMMANDER) {
		LOG_ERR("LIN send operation is only allowed in commander mode");
		return -EPERM;
	}

	/* Code to send the LIN header */

	if (msg->data_len) {
		/* Code to send LIN response */
	}

	/* Schedule the timeout delay for transmission */
	k_work_reschedule(&data->tx_timeout_work, timeout);

	return 0;
}

/**
 * Send a LIN header and wait for a response
 *
 * If the device is commander, the header will always transmitted on the bus, then it will monitor
 * the bus to receive the response from responder.
 */
static int lin_skeleton_receive(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout)
{
	struct lin_skeleton_data *data = dev->data;

	if (data->common.config.mode != LIN_MODE_COMMANDER) {
		LOG_ERR("LIN receive operation is only allowed in commander mode");
		return -EPERM;
	}

	/* Code to send the LIN header and receive the LIN response */

	/* Schedule the timeout delay for reception */
	k_work_reschedule(&data->rx_timeout_work, timeout);

	return 0;
}

/**
 * Send a LIN response only (responder mode only)
 *
 * The header should be received before calling this function, typically call this function to
 * response the LIN_EVT_RX_HEADER event.
 *
 * The msg->id field should match the id in the received header.
 */
static int lin_skeleton_response(const struct device *dev, const struct lin_msg *msg,
				 k_timeout_t timeout)
{
	struct lin_skeleton_data *data = dev->data;

	if (data->common.config.mode != LIN_MODE_RESPONDER) {
		LOG_ERR("LIN response operation is only allowed in responder mode");
		return -EPERM;
	}

	/* Code to send the LIN response */

	/* Schedule the timeout delay for transmission */
	k_work_reschedule(&data->tx_timeout_work, timeout);

	return 0;
}

/**
 * Read a LIN message from the hardware buffer
 *
 * This function is used to read a LIN message that has been received and stored in the hardware
 * buffer. It is typically called in response to a LIN_EVT_RX_DATA event.
 */
static int lin_skeleton_read(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout)
{
	struct lin_skeleton_data *data = dev->data;

	if (data->common.config.mode != LIN_MODE_RESPONDER) {
		LOG_ERR("LIN read operation is only allowed in responder mode");
		return -EPERM;
	}

	/* Code to read the LIN message */

	/* Schedule the timeout delay for reading */
	k_work_reschedule(&data->rx_timeout_work, timeout);

	return 0;
}

/**
 * Send a LIN wakeup pulse signal
 */
static int lin_skeleton_wakeup_send(const struct device *dev)
{
	/* Code to send the LIN wakeup signal */

	return 0;
}

/**
 * Put the LIN device into sleep mode or wake it up.
 *
 * Should be called to put the LIN device into sleep mode in case:
 * - Responder mode: a LIN sleep command was received.
 * - Commander mode: when no bus activity for a certain time. A goto sleep command should be sent to
 * the bus before entering sleep mode.
 */
static int lin_skeleton_enter_sleep(const struct device *dev, bool enable)
{
	if (enable) {
		/* Code to enter sleep mode */
	} else {
		/* Code to exit sleep mode */
	}

	return 0;
}

/**
 * Register callback for LIN events
 *
 * In case of LIN responder node, it is mandatory to assign callback before starting LIN bus because
 * processing sequence is event triggered.
 */
static int lin_skeleton_set_callback(const struct device *dev, lin_event_callback_t callback,
				     void *user_data)
{
	struct lin_driver_data *data = dev->data;
	int key = irq_lock();

	data->callback = callback;
	data->callback_data = user_data;

	irq_unlock(key);
	return 0;
}

static DEVICE_API(lin, lin_skeleton_api) = {
	.configure = lin_skeleton_configure,
	.get_config = lin_skeleton_get_config,
	.start = lin_skeleton_start,
	.stop = lin_skeleton_stop,
	.send = lin_skeleton_send,
	.receive = lin_skeleton_receive,
	.response = lin_skeleton_response,
	.read = lin_skeleton_read,
	.set_callback = lin_skeleton_set_callback,
	.set_rx_filter = lin_skeleton_set_rx_filter,
	.enter_sleep = lin_skeleton_enter_sleep,
	.wakeup_send = lin_skeleton_wakeup_send,
};

static void lin_skeleton_tx_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct lin_skeleton_data *data =
		CONTAINER_OF(dwork, struct lin_skeleton_data, tx_timeout_work);
	lin_event_callback_t callback = data->common.callback;
	void *user_data = data->common.callback_data;

	/* Fire the timeout callback in case no transmission complete event occurs */
	if (callback) {
		struct lin_event event = {
			.type = LIN_EVT_TX_DATA,
			.status = -EAGAIN,
		};

		callback(data->dev, &event, user_data);
	}
}

static void lin_skeleton_rx_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct lin_skeleton_data *data =
		CONTAINER_OF(dwork, struct lin_skeleton_data, rx_timeout_work);
	lin_event_callback_t callback = data->common.callback;
	void *user_data = data->common.callback_data;

	/* Fire the timeout callback in case no reception complete event occurs */
	if (callback) {
		struct lin_event event = {
			.type = LIN_EVT_RX_DATA,
			.status = -EAGAIN,
		};

		callback(data->dev, &event, user_data);
	}
}

/**
 * Device initialization.
 *
 * Do basically hardware preparation and initialize device software elements to make it ready for
 * starting.
 */
static int lin_skeleton_init(const struct device *dev)
{
	struct lin_skeleton_data *data = dev->data;

	/* Code to initialize hardware */

	/* Initialize the work item for transmission timeout handling */
	k_work_init_delayable(&data->tx_timeout_work, lin_skeleton_tx_timeout_handler);

	/* Initialize the work item for reception timeout handling */
	k_work_init_delayable(&data->rx_timeout_work, lin_skeleton_rx_timeout_handler);

	return 0;
}

#define DT_DRV_COMPAT zephyr_lin_skeleton

#define LIN_SKELETON_DEVICE_DEFINE(inst)                                                           \
	static const struct lin_skeleton_config lin_skeleton_config_##inst = {                     \
		.common = LIN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 20000),                           \
	};                                                                                         \
                                                                                                   \
	static struct lin_skeleton_data lin_skeleton_data_##inst = {                               \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, lin_skeleton_init, NULL, &lin_skeleton_data_##inst,            \
			      &lin_skeleton_config_##inst, POST_KERNEL,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &lin_skeleton_api);

DT_INST_FOREACH_STATUS_OKAY(LIN_SKELETON_DEVICE_DEFINE)
