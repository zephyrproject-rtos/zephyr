/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_offload.h
 *
 * @brief MQTT_OFFLOAD Client Implementation
 *
 * @details
 * MQTT_OFFLOAD Client's Application interface is defined in this header.
 * Targets protocol version 1.2.
 *
 * @defgroup mqtt_offload_socket MQTT_OFFLOAD Client library
 * @since 3.3
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_MQTT_OFFLOAD_H_
#define ZEPHYR_INCLUDE_NET_MQTT_OFFLOAD_H_

#include <stddef.h>

#include <zephyr/net_buf.h>
#include <zephyr/types.h>
#include <zephyr/net/mqtt.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief MQTT transport type. */
enum mqtt_offload_transport_type {
	/** Use non secure TCP transport for MQTT connection. */
	MQTT_OFFLOAD_TRANSPORT_NON_SECURE,

#if defined(CONFIG_MQTT_OFFLOAD_LIB_TLS)
	/** Use secure TCP transport (TLS) for MQTT connection. */
	MQTT_OFFLOAD_TRANSPORT_SECURE,
#endif /* CONFIG_MQTT_OFFLOAD_LIB_TLS */

#if defined(CONFIG_MQTT_LIB_CUSTOM_TRANSPORT)
	/** Use custom transport for MQTT connection. */
	MQTT_OFFLOAD_TRANSPORT_CUSTOM,
#endif /* CONFIG_MQTT_LIB_CUSTOM_TRANSPORT */
	MQTT_OFFLOAD_TRANSPORT_COMMON,
	/** Shall not be used as a transport type.
	 *  Indicator of maximum transport types possible.
	 */
	MQTT_OFFLOAD_TRANSPORT_NUM
};

/**
 * Quality of Service. QoS 0-2 work the same as basic MQTT, QoS -1 is an MQTT_OFFLOAD addition.
 * QOS -1 is not supported yet.
 */
enum mqtt_offload_qos {
	MQTT_OFFLOAD_QOS_0, /**< QOS 0 */
	MQTT_OFFLOAD_QOS_1, /**< QOS 1 */
	MQTT_OFFLOAD_QOS_2, /**< QOS 2 */
	MQTT_OFFLOAD_QOS_M1 /**< QOS -1 */
};

/**
 * MQTT_OFFLOAD return codes.
 */
enum mqtt_offload_return_code {
	MQTT_OFFLOAD_CODE_ACCEPTED = 0x00,            /**< Accepted */
	MQTT_OFFLOAD_CODE_REJECTED_CONGESTION = 0x01, /**< Rejected: congestion */
	MQTT_OFFLOAD_CODE_REJECTED_TOPIC_ID = 0x02,   /**< Rejected: Invalid Topic ID */
	MQTT_OFFLOAD_CODE_REJECTED_NOTSUP = 0x03,     /**< Rejected: Not Supported */
	MQTT_OFFLOAD_CODE_REJECTED = 0x04,            /**< Rejected: General Error */
};

/** @brief Abstracts memory buffers. */
struct mqtt_offload_data {
	uint8_t *data; /**< Pointer to data. */
	size_t size;   /**< Size of data, in bytes. */
};

/**
 * @brief Structure to hold MQTT offload buffer configuration.
 *
 * This structure encapsulates the transmit and receive buffers used by the MQTT offload client.
 * It is used to reduce the number of parameters passed to initialization functions and to group
 * related buffer data together for clarity and maintainability.
 */
struct mqtt_offload_buffers {
	/** Pointer to the transmit buffer memory */
	void *tx;
	/** Size of the transmit buffer in bytes */
	size_t txsz;
	/** Pointer to the receive buffer memory */
	void *rx;
	/** Size of the receive buffer in bytes */
	size_t rxsz;
};

/**
 * @brief Initialize memory buffer from C literal string.
 *
 * Use it as follows:
 *
 * struct mqtt_sn_data topic = MQTT_OFFLOAD_DATA_STRING_LITERAL("/zephyr");
 *
 * @param[in] literal Literal string from which to generate mqtt_sn_data object.
 */
#define MQTT_OFFLOAD_DATA_STRING_LITERAL(literal)                                                  \
	((struct mqtt_offload_data){literal, sizeof(literal) - 1})

/**
 * @brief Initialize memory buffer from single bytes.
 *
 * Use it as follows:
 *
 * struct mqtt_sn_data data = MQTT_OFFLOAD_DATA_BYTES(0x13, 0x37);
 */
#define MQTT_OFFLOAD_DATA_BYTES(...)                                                               \
	((struct mqtt_offload_data){(uint8_t[]){__VA_ARGS__}, sizeof((uint8_t[]){__VA_ARGS__})})

/**
 * Event types that can be emitted by the library.
 */
enum mqtt_offload_evt_type {
	MQTT_OFFLOAD_EVT_CONNECTED,       /**< Connected to a gateway */
	MQTT_OFFLOAD_EVT_DISCONNECTED,    /**< Disconnected */
	MQTT_OFFLOAD_EVT_ASLEEP,          /**< Entered ASLEEP state */
	MQTT_OFFLOAD_EVT_AWAKE,           /**< Entered AWAKE state */
	MQTT_OFFLOAD_EVT_PUBLISH,         /**< Received a PUBLISH message */
	MQTT_OFFLOAD_EVT_SUBSCRIBE_ACK,   /**< Received a SUBSCRIBE message */
	MQTT_OFFLOAD_EVT_UNSUBSCRIBE_ACK, /**< Received a UNSUBSCRIBE message */
	MQTT_OFFLOAD_EVT_PUBLISH_ACK,     /**< Received a PUBLISH message */
};

/**
 * Event metadata.
 */
union mqtt_offload_evt_param {
	/** Structure holding publish event details */
	struct {
		/** The topic name associated with the event */
		struct mqtt_offload_data topic;
		/** The payload data associated with the event */
		struct mqtt_offload_data data;
	} publish;
};

/**
 * MQTT_OFFLOAD event structure to be handled by the event callback.
 */
struct mqtt_offload_evt {
	/** Event type */
	enum mqtt_offload_evt_type type;
	/** Event parameters */
	union mqtt_offload_evt_param param;
	/** Event result. 0 or a negative error code (errno.h) indicating
	 *  reason of failure.
	 */
	int result;
};

struct mqtt_offload_client;

/**
 * @brief Asynchronous event notification callback registered by the
 *        application.
 *
 * @param[in] client Identifies the client for which the event is notified.
 * @param[in] evt Event description along with result and associated
 *                parameters (if any).
 */
typedef void (*mqtt_offload_evt_cb_t)(struct mqtt_offload_client *client,
				      const struct mqtt_offload_evt *evt);
/**
 * Transport struct for HL78XX_MQTT based transport.
 */
struct mqtt_offload_transport_mqtt {
	/** Name of the interface that the MQTT client instance should be bound to.
	 *  Leave as NULL if not specified.
	 */
	const char *if_name;
	/** Transport type selection for client instance.
	 *  @ref mqtt_transport_type for possible values. MQTT_TRANSPORT_MAX
	 *  is not a valid type.
	 */
	enum mqtt_offload_transport_type type;

	/** TLS socket transport for MQTT */
	struct {
		/** Socket descriptor. */
		int sock;
#if defined(CONFIG_MQTT_OFFLOAD_LIB_TLS)
		/** TLS configuration. See @ref mqtt_sec_config for
		 *  details.
		 */
		struct mqtt_sec_config config;
#endif /* CONFIG_MQTT_OFFLOAD_LIB_TLS */

	} mqtt;
};

/**
 * Structure describing an MQTT_OFFLOAD client.
 */
struct mqtt_offload_client {
	/** 1-23 character unique client ID */
	struct mqtt_offload_data client_id;
	/** Broker details, for example, address, port. Address type should
	 *  be compatible with transport used.
	 */
	const void *broker;

	/** Topic for Will message.
	 * Must be initialized before connecting with will=true
	 */
	struct mqtt_offload_data will_topic;

	/** Will message.
	 * Must be initialized before connecting with will=true
	 */
	struct mqtt_offload_data will_msg;

	/** Quality of Service for the Will message */
	enum mqtt_offload_qos will_qos;

	/** Flag indicating if the will message should be retained by the broker */
	bool will_retain;

	/** Underlying transport to be used by the client */
	struct mqtt_offload_transport_mqtt *transport;

	/** Buffer for outgoing data */
	struct net_buf_simple tx;

	/** Buffer for incoming data */
	struct net_buf_simple rx;

	/** Buffer for incoming data sender address */
	struct net_buf_simple rx_addr;

	/** Event callback */
	mqtt_offload_evt_cb_t evt_cb;

	/** Message ID for the next message to be sent */
	uint16_t next_msg_id;

	/** List of pending publish messages */
	sys_slist_t publish;

	/** List of registered topics */
	sys_slist_t topic;

	/** Current state of the MQTT_OFFLOAD client */
	int state;

	/** Timestamp of the last ping request */
	int64_t last_ping;

	/** Number of retries for failed ping attempts */
	uint8_t ping_retries;

	/** MQTT protocol version. */
	uint8_t protocol_version;
	/** Work structure for processing MQTT_OFFLOAD events */
	struct k_work process_work;
};

/**
 * @brief Initialize an MQTT offload client instance.
 *
 * This function sets up the MQTT offload client with the provided configuration,
 * including client identity, transport layer, event callback, and buffer settings.
 * It also initializes internal work structures and prepares the client for operation.
 *
 * @param client Pointer to the MQTT offload client instance to initialize.
 * @param client_id Pointer to the structure containing the MQTT client identifier.
 * @param transport Pointer to the transport-specific MQTT implementation.
 * @param evt_cb Callback function to handle MQTT offload events.
 * @param buffers Pointer to a structure containing transmit and receive buffer configurations.
 *
 * @retval 0 on successful initialization.
 * @retval -EINVAL if any required parameter is NULL.
 */
int mqtt_offload_client_init(struct mqtt_offload_client *client,
			     const struct mqtt_offload_data *client_id,
			     struct mqtt_offload_transport_mqtt *transport,
			     mqtt_offload_evt_cb_t evt_cb, struct mqtt_offload_buffers *buffers);

/**
 * @brief Deinitialize the client.
 *
 * This removes all topics and publishes, and also de-inits the transport.
 *
 * @param client        The MQTT_OFFLOAD client to deinitialize.
 */
void mqtt_offload_client_deinit(struct mqtt_offload_client *client);

/**
 * @brief Connect the client.
 *
 * @param client            The MQTT_OFFLOAD client to connect.
 * @param will              Flag indicating if a Will message should be sent.
 * @param clean_session     Flag indicating if a clean session should be started.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_offload_connect(struct mqtt_offload_client *client, bool will, bool clean_session);

/**
 * @brief Disconnect the client.
 *
 * @param client            The MQTT_OFFLOAD client to disconnect.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_offload_disconnect(struct mqtt_offload_client *client);

/**
 * @brief Set the client into sleep state.
 *
 * @param client            The MQTT_OFFLOAD client to be put to sleep.
 * @param duration          Sleep duration (in seconds).
 *
 * @return 0 on success, negative errno code on failure.
 */
int mqtt_offload_sleep(struct mqtt_offload_client *client, uint16_t duration);

/**
 * @brief Subscribe to a given topic.
 *
 * @param client            The MQTT_OFFLOAD client that should subscribe.
 * @param qos               The desired quality of service for the subscription.
 * @param topic_name        The name of the topic to subscribe to.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_offload_subscribe(struct mqtt_offload_client *client, enum mqtt_offload_qos qos,
			   const struct mqtt_offload_data *topic_name);

/**
 * @brief Unsubscribe from a topic.
 *
 * @param client            The MQTT_OFFLOAD client that should unsubscribe.
 * @param topic_name        The name of the topic to unsubscribe from.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_offload_unsubscribe(struct mqtt_offload_client *client,
			     const struct mqtt_offload_data *topic_name);

/**
 * @brief Publish a value.
 *
 * If the topic is not yet registered with the gateway, the library takes care of it.
 *
 * @param client            The MQTT_OFFLOAD client that should publish.
 * @param qos               The desired quality of service for the publish.
 * @param topic_name        The name of the topic to publish to.
 * @param retain            Flag indicating if the message should be retained by the broker.
 * @param data              The data to be published.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_offload_publish(struct mqtt_offload_client *client, enum mqtt_offload_qos qos,
			 const struct mqtt_offload_data *topic_name, bool retain,
			 const struct mqtt_offload_data *data);

/**
 * @brief Check the transport for new incoming data.
 *
 * Call this function periodically, or if you have good reason to believe there is any data.
 * If the client's transport struct contains a poll-function, this function is non-blocking.
 *
 * @param client            The MQTT_OFFLOAD client to check for incoming data.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_offload_input(struct mqtt_offload_client *client);

/**
 * @brief Get topic name by topic ID.
 *
 * @param[in] client The MQTT_OFFLOAD client that uses this topic.
 * @param[in] id Topic identifier.
 * @param[out] topic_name Will be assigned to topic name.
 *
 * @return 0 on success, -ENOENT if topic ID doesn't exist,
 * or -EINVAL on invalid arguments.
 */
int mqtt_offload_get_topic_name(struct mqtt_offload_client *client, uint16_t id,
				struct mqtt_offload_data *topic_name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_MQTT_OFFLOAD_H_ */

/**@}  */
