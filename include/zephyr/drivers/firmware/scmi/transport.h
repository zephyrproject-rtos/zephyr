/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_transport
 * @brief Header file for the SCMI Transport Layer.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_TRANSPORT_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_TRANSPORT_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

/**
 * @brief SCMI Transport Layer abstraction and definitions
 * @defgroup scmi_transport Transport
 * @ingroup scmi_interface
 * @{
 */

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
	/** channel private data */
	void *data;
	/**
	 * callback function. This is meant to be set by
	 * the SCMI core and should be called by the SCMI
	 * transport layer driver whenever a reply has
	 * been received.
	 */
	scmi_channel_cb cb;
	/** @cond INTERNAL_HIDDEN */
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

	/** is the channel ready to be used by a protocol? */
	bool ready;
	/** @endcond */
};

/**
 * @struct scmi_transport_api
 * @brief SCMI transport driver operations
 *
 * This structure contains the set of operations to be implemented by
 * all transport drivers.
 */
struct scmi_transport_api {
	/**
	 * @brief Initialize the transport driver
	 *
	 * This operation can be left unimplemented if the driver
	 * requires no initialization.
	 *
	 * @note this operation is optional.
	 *
	 * @param transport transport device
	 *
	 * @retval 0 is successful
	 * @retval <0 negative errno code if failure
	 */
	int (*init)(const struct device *transport);

	/**
	 * @brief Send a message to the platform
	 *
	 * Used to send a message to the platform over a given TX channel.
	 *
	 * @param transport transport device
	 * @param chan channel used to send the message
	 * @param msg message to send
	 *
	 * @retval 0 if successful
	 * @retval <0 negative errno code if failure
	 */
	int (*send_message)(const struct device *transport,
			    struct scmi_channel *chan,
			    struct scmi_message *msg);

	/**
	 * @brief Prepare a channel for communication
	 *
	 * Perform any sort of initialization required by a channel
	 * to be able to send or receive data.
	 *
	 * @param transport transport device
	 * @param chan channel to prepare
	 * @param tx true if channel is TX, false if channel is RX
	 *
	 * @retval 0 if successful
	 * @retval <0 negative errno code if failure
	 */
	int (*setup_chan)(const struct device *transport,
			  struct scmi_channel *chan,
			  bool tx);

	/**
	 * @brief Read a message from the platform
	 *
	 * Used to read/receive a message from the platform over a given
	 * RX channel.
	 *
	 * @param transport transport device
	 * @param chan channel used to receive the message
	 * @param msg message to receive
	 *
	 * @retval 0 if successful
	 * @retval <0 negative errno code if failure
	 */
	int (*read_message)(const struct device *transport,
			    struct scmi_channel *chan,
			    struct scmi_message *msg);

	/**
	 * @brief Check if a TX channel is free
	 *
	 * Used to check if a TX channel allows sending data to the
	 * platform. If a message was previously sent to the platform,
	 * it is assumed that this function will indicate the availability
	 * of the message's reply.
	 *
	 * @param transport device
	 * @param chan TX channel to query
	 *
	 * @retval 0 if successful
	 * @retval <0 negative errno code if failure
	 */
	bool (*channel_is_free)(const struct device *transport,
				struct scmi_channel *chan);

	/**
	 * @brief Request a channel dynamically
	 *
	 * If @kconfig{CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS}
	 * is enabled, this operation will be used to dynamically request
	 * a channel and bind it to a given protocol. Otherwise, operation
	 * can be left unimplemented.
	 *
	 * @note this operation is optional
	 *
	 * @param transport transport device
	 * @param proto ID of the protocol for which the channel is requested
	 * @param tx true if channel is TX, false if channel is RX
	 *
	 * @retval pointer to the channel bound to given protocol if successful
	 * @retval NULL if failure
	 */
	struct scmi_channel *(*request_channel)(const struct device *transport,
						uint32_t proto, bool tx);
};

/** @cond INTERNAL_HIDDEN */

/**
 * See @ref scmi_transport_api::request_channel for more details.
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
 * See @ref scmi_transport_api::init for more details.
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
 * See @ref scmi_transport_api::setup_chan for more details.
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
 * See @ref scmi_transport_api::send_message for more details.
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
 * See @ref scmi_transport_api::read_message for more details.
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
 * See @ref scmi_transport_api::channel_is_free for more details.
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

/** @endcond */

/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_TRANSPORT_H_ */
