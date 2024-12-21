/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the SCMI transport layer drivers
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_TRANSPORT_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_TRANSPORT_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

struct scmi_message;
struct scmi_channel;

/**
 * @typedef scmi_channel_cb
 *
 * @brief Callback function for message replies
 *
 * This function should be called by the transport layer
 * driver whenever a reply to a previously sent message
 * has been received. Its purpose is to notifying the SCMI
 * core of the reply's arrival so that proper action can
 * be taken.
 *
 * @param chan pointer to SCMI channel on which the reply
 * arrived
 */
typedef void (*scmi_channel_cb)(struct scmi_channel *chan);

/**
 * @struct scmi_channel
 * @brief SCMI channel structure
 *
 * An SCMI channel is a medium through which a protocol
 * is able to transmit/receive messages. Each of the SCMI
 * channels is represented by a `struct scmi_channel`.
 */
struct scmi_channel {
	/**
	 * channel lock. This is meant to be initialized
	 * and used only by the SCMI core to assure that
	 * only one protocol can send/receive messages
	 * through a channel at a given moment.
	 */
	struct k_mutex lock;
	/**
	 * binary semaphore. This is meant to be initialized
	 * and used only by the SCMI core. Its purpose is to
	 * signal that a reply has been received.
	 */
	struct k_sem sem;
	/** channel private data */
	void *data;
	/**
	 * callback function. This is meant to be set by
	 * the SCMI core and should be called by the SCMI
	 * transport layer driver whenever a reply has
	 * been received.
	 */
	scmi_channel_cb cb;
	/** is the channel ready to be used by a protocol? */
	bool ready;
};

struct scmi_transport_api {
	int (*init)(const struct device *transport);
	int (*send_message)(const struct device *transport,
			    struct scmi_channel *chan,
			    struct scmi_message *msg);
	int (*setup_chan)(const struct device *transport,
			  struct scmi_channel *chan,
			  bool tx);
	int (*read_message)(const struct device *transport,
			    struct scmi_channel *chan,
			    struct scmi_message *msg);
	bool (*channel_is_free)(const struct device *transport,
				struct scmi_channel *chan);
	struct scmi_channel *(*request_channel)(const struct device *transport,
						uint32_t proto, bool tx);
};

/**
 * @brief Request an SCMI channel dynamically
 *
 * Whenever the SCMI transport layer driver doesn't support
 * static channel allocation, the SCMI core will try to bind
 * a channel to a protocol dynamically using this function.
 * Note that no setup needs to be performed on the channel
 * in this function as the core will also call the channel
 * setup() function.
 *
 * @param transport pointer to the device structure for the
 * transport layer
 * @param proto ID of the protocol for which the core is
 * requesting the channel
 * @param tx true if the channel is TX, false if RX
 *
 * @retval pointer to SCMI channel that's to be bound
 * to the protocol
 * @retval NULL if operation was not successful
 */
static inline struct scmi_channel *
scmi_transport_request_channel(const struct device *transport,
			       uint32_t proto, bool tx)
{
	const struct scmi_transport_api *api =
		(const struct scmi_transport_api *)transport->api;

	if (api->request_channel) {
		return api->request_channel(transport, proto, tx);
	}

	return NULL;
}

/**
 * @brief Perform initialization for the transport layer driver
 *
 * The transport layer driver can't be initialized directly
 * (i.e via a call to its init() function) during system initialization.
 * This is because the macro used to define an SCMI transport places
 * `scmi_core_transport_init()` in the init section instead of the
 * driver's init() function. As such, `scmi_core_transport_init()`
 * needs to call this function to perfrom transport layer driver
 * initialization if required.
 *
 * This operation is optional.
 *
 * @param transport pointer to the device structure for the
 * transport layer
 *
 * @retval 0 if successful
 * @retval negative errno code if failure
 */
static inline int scmi_transport_init(const struct device *transport)
{
	const struct scmi_transport_api *api =
		(const struct scmi_transport_api *)transport->api;

	if (api->init) {
		return api->init(transport);
	}

	return 0;
}

/**
 * @brief Setup an SCMI channel
 *
 * Before being able to send/receive messages, an SCMI channel needs
 * to be prepared, which is what this function does. If it returns
 * successfully, an SCMI protocol will be able to use this channel
 * to send/receive messages.
 *
 * @param transport pointer to the device structure for the
 * transport layer
 * @param chan pointer to SCMI channel to be prepared
 * @param tx true if the channel is TX, false if RX
 *
 * @retval 0 if successful
 * @retval negative errno code if failure
 */
static inline int scmi_transport_setup_chan(const struct device *transport,
					    struct scmi_channel *chan,
					    bool tx)
{
	const struct scmi_transport_api *api =
		(const struct scmi_transport_api *)transport->api;

	if (!api || !api->setup_chan) {
		return -ENOSYS;
	}

	return api->setup_chan(transport, chan, tx);
}

/**
 * @brief Send an SCMI channel
 *
 * Send an SCMI message using given SCMI channel. This function is
 * not allowed to block.
 *
 * @param transport pointer to the device structure for the
 * transport layer
 * @param chan pointer to SCMI channel on which the message
 * is to be sent
 * @param msg pointer to message the caller wishes to send
 *
 * @retval 0 if successful
 * @retval negative errno code if failure
 */
static inline int scmi_transport_send_message(const struct device *transport,
					      struct scmi_channel *chan,
					      struct scmi_message *msg)
{
	const struct scmi_transport_api *api =
		(const struct scmi_transport_api *)transport->api;

	if (!api || !api->send_message) {
		return -ENOSYS;
	}

	return api->send_message(transport, chan, msg);
}

/**
 * @brief Read an SCMI message
 *
 * @param transport pointer to the device structure for the
 * transport layer
 * @param chan pointer to SCMI channel on which the message
 * is to be read
 * @param msg pointer to message the caller wishes to read
 *
 * @retval 0 if successful
 * @retval negative errno code if failure
 */
static inline int scmi_transport_read_message(const struct device *transport,
					      struct scmi_channel *chan,
					      struct scmi_message *msg)
{
	const struct scmi_transport_api *api =
		(const struct scmi_transport_api *)transport->api;

	if (!api || !api->read_message) {
		return -ENOSYS;
	}

	return api->read_message(transport, chan, msg);
}

/**
 * @brief Check if an SCMI channel is free
 *
 * @param transport pointer to the device structure for
 * the transport layer
 * @param chan pointer to SCMI channel the query is to be
 * performed on
 *
 * @retval 0 if successful
 * @retval negative errno code if failure
 */
static inline bool scmi_transport_channel_is_free(const struct device *transport,
						  struct scmi_channel *chan)
{
	const struct scmi_transport_api *api =
		(const struct scmi_transport_api *)transport->api;

	if (!api || !api->channel_is_free) {
		return -ENOSYS;
	}

	return api->channel_is_free(transport, chan);
}

/**
 * @brief Perfrom SCMI core initialization
 *
 * @param transport pointer to the device structure for
 * the transport layer
 *
 * @retval 0 if successful
 * @retval negative errno code if failure
 */
int scmi_core_transport_init(const struct device *transport);

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_TRANSPORT_H_ */
