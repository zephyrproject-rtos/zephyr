/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_sn.h
 *
 * @brief MQTT-SN Client Implementation
 *
 * @details
 * MQTT-SN Client's Application interface is defined in this header.
 * Targets protocol version 1.2.
 *
 * @defgroup mqtt_sn_socket MQTT-SN Client library
 * @since 3.3
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_MQTT_SN_H_
#define ZEPHYR_INCLUDE_NET_MQTT_SN_H_

#include <stddef.h>

#include <zephyr/net_buf.h>
#include <zephyr/types.h>

#include <sys/types.h>

#ifdef CONFIG_MQTT_SN_TRANSPORT_UDP
#include <zephyr/net/net_ip.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Quality of Service. QoS 0-2 work the same as basic MQTT, QoS -1 is an MQTT-SN addition.
 * QOS -1 is not supported yet.
 */
enum mqtt_sn_qos {
	MQTT_SN_QOS_0, /**< QOS 0 */
	MQTT_SN_QOS_1, /**< QOS 1 */
	MQTT_SN_QOS_2, /**< QOS 2 */
	MQTT_SN_QOS_M1 /**< QOS -1 */
};

/**
 * MQTT-SN topic types.
 */
enum mqtt_sn_topic_type {
	/**
	 * Normal topic.
	 * It allows usage of any valid UTF-8 string as a topic name.
	 */
	MQTT_SN_TOPIC_TYPE_NORMAL,
	/**
	 * Pre-defined topic.
	 * It allows usage of a two-byte identifier representing a topic name for
	 * which the corresponding topic name is known in advance by both the client
	 * and the gateway/server.
	 */
	MQTT_SN_TOPIC_TYPE_PREDEF,
	/**
	 * Short topic.
	 * It allows usage of a two-byte string as a topic name.
	 */
	MQTT_SN_TOPIC_TYPE_SHORT
};

/**
 * MQTT-SN return codes.
 */
enum mqtt_sn_return_code {
	MQTT_SN_CODE_ACCEPTED = 0x00, /**< Accepted */
	MQTT_SN_CODE_REJECTED_CONGESTION = 0x01, /**< Rejected: congestion */
	MQTT_SN_CODE_REJECTED_TOPIC_ID = 0x02, /**< Rejected: Invalid Topic ID */
	MQTT_SN_CODE_REJECTED_NOTSUP = 0x03, /**< Rejected: Not Supported */
};

/** @brief Abstracts memory buffers. */
struct mqtt_sn_data {
	const uint8_t *data; /**< Pointer to data. */
	uint16_t size;	     /**< Size of data, in bytes. */
};

/**
 * @brief Initialize memory buffer from C literal string.
 *
 * Use it as follows:
 *
 * struct mqtt_sn_data topic = MQTT_SN_DATA_STRING_LITERAL("/zephyr");
 *
 * @param[in] literal Literal string from which to generate mqtt_sn_data object.
 */
#define MQTT_SN_DATA_STRING_LITERAL(literal) ((struct mqtt_sn_data){literal, sizeof(literal) - 1})

/**
 * @brief Initialize memory buffer from single bytes.
 *
 * Use it as follows:
 *
 * struct mqtt_sn_data data = MQTT_SN_DATA_BYTES(0x13, 0x37);
 */
#define MQTT_SN_DATA_BYTES(...) \
	((struct mqtt_sn_data) { (uint8_t[]){ __VA_ARGS__ }, sizeof((uint8_t[]){ __VA_ARGS__ })})

/**
 * Event types that can be emitted by the library.
 */
enum mqtt_sn_evt_type {
	MQTT_SN_EVT_CONNECTED,	  /**< Connected to a gateway */
	MQTT_SN_EVT_DISCONNECTED, /**< Disconnected */
	MQTT_SN_EVT_ASLEEP,	  /**< Entered ASLEEP state */
	MQTT_SN_EVT_AWAKE,	  /**< Entered AWAKE state */
	MQTT_SN_EVT_PUBLISH,	  /**< Received a PUBLISH message */
	MQTT_SN_EVT_PINGRESP	  /**< Received a PINGRESP */
};

/**
 * Event metadata.
 */
union mqtt_sn_evt_param {
	/** Structure holding publish event details */
	struct {
		/** The payload data associated with the event */
		struct mqtt_sn_data data;
		/** The type of topic for the event */
		enum mqtt_sn_topic_type topic_type;
		/** The identifier for the topic of the event */
		uint16_t topic_id;
	} publish;
};

/**
 * MQTT-SN event structure to be handled by the event callback.
 */
struct mqtt_sn_evt {
	/** Event type */
	enum mqtt_sn_evt_type type;
	/** Event parameters */
	union mqtt_sn_evt_param param;
};

struct mqtt_sn_client;

/**
 * @brief Asynchronous event notification callback registered by the
 *        application.
 *
 * @param[in] client Identifies the client for which the event is notified.
 * @param[in] evt Event description along with result and associated
 *                parameters (if any).
 */
typedef void (*mqtt_sn_evt_cb_t)(struct mqtt_sn_client *client, const struct mqtt_sn_evt *evt);

/**
 * @brief Structure to describe an MQTT-SN transport.
 *
 * MQTT-SN does not require transports to be reliable or to hold a connection.
 * Transports just need to be frame-based, so you can use UDP, ZigBee, or even
 * a simple UART, given some kind of framing protocol is used.
 */
struct mqtt_sn_transport {
	/**
	 * @brief Will be called once on client init to initialize the transport.
	 *
	 * Use this to open sockets or similar. May be NULL.
	 */
	int (*init)(struct mqtt_sn_transport *transport);

	/**
	 * @brief Will be called on client deinit
	 *
	 * Use this to close sockets or similar. May be NULL.
	 */
	void (*deinit)(struct mqtt_sn_transport *transport);

	/**
	 * Will be called by the library when it wants to send a message.
	 */
	int (*msg_send)(struct mqtt_sn_client *client, void *buf, size_t sz);

	/**
	 * @brief Will be called by the library when it wants to receive a message.
	 *
	 * Implementations should follow recv conventions.
	 */
	ssize_t (*recv)(struct mqtt_sn_client *client, void *buffer, size_t length);

	/**
	 * @brief Check if incoming data is available.
	 *
	 * If poll() returns a positive number, recv must not block.
	 *
	 * May be NULL, but recv should not block then either.
	 *
	 * @return Positive number if data is available, or zero if there is none.
	 * Negative values signal errors.
	 */
	int (*poll)(struct mqtt_sn_client *client);
};

#ifdef CONFIG_MQTT_SN_TRANSPORT_UDP
/**
 * Transport struct for UDP based transport.
 */
struct mqtt_sn_transport_udp {
	/** Parent struct */
	struct mqtt_sn_transport tp;

	/** Socket FD */
	int sock;

	/** Address of the gateway */
	struct sockaddr gwaddr;
	socklen_t gwaddrlen;
};

#define UDP_TRANSPORT(transport) CONTAINER_OF(transport, struct mqtt_sn_transport_udp, tp)

/**
 * @brief Initialize the UDP transport.
 *
 * @param[in] udp The transport to be initialized
 * @param[in] gwaddr Pre-initialized gateway address
 * @param[in] addrlen Size of the gwaddr structure.
 */
int mqtt_sn_transport_udp_init(struct mqtt_sn_transport_udp *udp, struct sockaddr *gwaddr,
			       socklen_t addrlen);
#endif

/**
 * Structure describing an MQTT-SN client.
 */
struct mqtt_sn_client {
	/** 1-23 character unique client ID */
	struct mqtt_sn_data client_id;

	/** Topic for Will message.
	 * Must be initialized before connecting with will=true
	 */
	struct mqtt_sn_data will_topic;

	/** Will message.
	 * Must be initialized before connecting with will=true
	 */
	struct mqtt_sn_data will_msg;

	/** Quality of Service for the Will message */
	enum mqtt_sn_qos will_qos;

	/** Flag indicating if the will message should be retained by the broker */
	bool will_retain;

	/** Underlying transport to be used by the client */
	struct mqtt_sn_transport *transport;

	/** Buffer for outgoing data */
	struct net_buf_simple tx;

	/** Buffer for incoming data */
	struct net_buf_simple rx;

	/** Event callback */
	mqtt_sn_evt_cb_t evt_cb;

	/** Message ID for the next message to be sent */
	uint16_t next_msg_id;

	/** List of pending publish messages */
	sys_slist_t publish;

	/** List of registered topics */
	sys_slist_t topic;

	/** Current state of the MQTT-SN client */
	int state;

	/** Timestamp of the last ping request */
	int64_t last_ping;

	/** Number of retries for failed ping attempts */
	uint8_t ping_retries;

	/** Delayable work structure for processing MQTT-SN events */
	struct k_work_delayable process_work;
};

/**
 * @brief Initialize a client.
 *
 * @param client        The MQTT-SN client to initialize.
 * @param client_id     The ID to be used by the client.
 * @param transport     The transport to be used by the client.
 * @param evt_cb        The event callback function for the client.
 * @param tx            Pointer to the transmit buffer.
 * @param txsz          Size of the transmit buffer.
 * @param rx            Pointer to the receive buffer.
 * @param rxsz          Size of the receive buffer.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_sn_client_init(struct mqtt_sn_client *client, const struct mqtt_sn_data *client_id,
			struct mqtt_sn_transport *transport, mqtt_sn_evt_cb_t evt_cb, void *tx,
			size_t txsz, void *rx, size_t rxsz);

/**
 * @brief Deinitialize the client.
 *
 * This removes all topics and publishes, and also de-inits the transport.
 *
 * @param client        The MQTT-SN client to deinitialize.
 */
void mqtt_sn_client_deinit(struct mqtt_sn_client *client);

/**
 * @brief Connect the client.
 *
 * @param client            The MQTT-SN client to connect.
 * @param will              Flag indicating if a Will message should be sent.
 * @param clean_session     Flag indicating if a clean session should be started.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_sn_connect(struct mqtt_sn_client *client, bool will, bool clean_session);

/**
 * @brief Disconnect the client.
 *
 * @param client            The MQTT-SN client to disconnect.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_sn_disconnect(struct mqtt_sn_client *client);

/**
 * @brief Set the client into sleep state.
 *
 * @param client            The MQTT-SN client to be put to sleep.
 * @param duration          Sleep duration (in seconds).
 *
 * @return 0 on success, negative errno code on failure.
 */
int mqtt_sn_sleep(struct mqtt_sn_client *client, uint16_t duration);

/**
 * @brief Subscribe to a given topic.
 *
 * @param client            The MQTT-SN client that should subscribe.
 * @param qos               The desired quality of service for the subscription.
 * @param topic_name        The name of the topic to subscribe to.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_sn_subscribe(struct mqtt_sn_client *client, enum mqtt_sn_qos qos,
		      struct mqtt_sn_data *topic_name);

/**
 * @brief Unsubscribe from a topic.
 *
 * @param client            The MQTT-SN client that should unsubscribe.
 * @param qos               The quality of service used when subscribing.
 * @param topic_name        The name of the topic to unsubscribe from.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_sn_unsubscribe(struct mqtt_sn_client *client, enum mqtt_sn_qos qos,
			struct mqtt_sn_data *topic_name);

/**
 * @brief Publish a value.
 *
 * If the topic is not yet registered with the gateway, the library takes care of it.
 *
 * @param client            The MQTT-SN client that should publish.
 * @param qos               The desired quality of service for the publish.
 * @param topic_name        The name of the topic to publish to.
 * @param retain            Flag indicating if the message should be retained by the broker.
 * @param data              The data to be published.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_sn_publish(struct mqtt_sn_client *client, enum mqtt_sn_qos qos,
		    struct mqtt_sn_data *topic_name, bool retain, struct mqtt_sn_data *data);

/**
 * @brief Check the transport for new incoming data.
 *
 * Call this function periodically, or if you have good reason to believe there is any data.
 * If the client's transport struct contains a poll-function, this function is non-blocking.
 *
 * @param client            The MQTT-SN client to check for incoming data.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_sn_input(struct mqtt_sn_client *client);

/**
 * @brief Get topic name by topic ID.
 *
 * @param[in] client The MQTT-SN client that uses this topic.
 * @param[in] id Topic identifier.
 * @param[out] topic_name Will be assigned to topic name.
 *
 * @return 0 on success, -ENOENT if topic ID doesn't exist,
 * or -EINVAL on invalid arguments.
 */
int mqtt_sn_get_topic_name(struct mqtt_sn_client *client, uint16_t id,
			   struct mqtt_sn_data *topic_name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_MQTT_SN_H_ */

/**@}  */
