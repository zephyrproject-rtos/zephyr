/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt.h
 *
 * @defgroup mqtt_socket MQTT Client library
 * @ingroup networking
 * @{
 * @brief MQTT Client Implementation
 *
 * @details
 * MQTT Client's Application interface is defined in this header.
 *
 * @note The implementation assumes TCP module is enabled.
 *
 * @note By default the implementation uses MQTT version 3.1.1.
 */

#ifndef ZEPHYR_INCLUDE_NET_MQTT_H_
#define ZEPHYR_INCLUDE_NET_MQTT_H_

#include <stddef.h>

#include <zephyr.h>
#include <zephyr/types.h>
#include <net/tls_credentials.h>
#include <net/net_ip.h>
#include <sys/mutex.h>
#include <net/websocket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT Asynchronous Events notified to the application from the module
 *        through the callback registered by the application.
 */
enum mqtt_evt_type {
	/** Acknowledgment of connection request. Event result accompanying
	 *  the event indicates whether the connection failed or succeeded.
	 */
	MQTT_EVT_CONNACK,

	/** Disconnection Event. MQTT Client Reference is no longer valid once
	 *  this event is received for the client.
	 */
	MQTT_EVT_DISCONNECT,

	/** Publish event received when message is published on a topic client
	 *  is subscribed to.
	 *
	 * @note PUBLISH event structure only contains payload size, the payload
	 *       data parameter should be ignored. Payload content has to be
	 *       read manually with @ref mqtt_read_publish_payload function.
	 */
	MQTT_EVT_PUBLISH,

	/** Acknowledgment for published message with QoS 1. */
	MQTT_EVT_PUBACK,

	/** Reception confirmation for published message with QoS 2. */
	MQTT_EVT_PUBREC,

	/** Release of published message with QoS 2. */
	MQTT_EVT_PUBREL,

	/** Confirmation to a publish release message with QoS 2. */
	MQTT_EVT_PUBCOMP,

	/** Acknowledgment to a subscribe request. */
	MQTT_EVT_SUBACK,

	/** Acknowledgment to a unsubscribe request. */
	MQTT_EVT_UNSUBACK
};

/** @brief MQTT version protocol level. */
enum mqtt_version {
	MQTT_VERSION_3_1_0 = 3, /**< Protocol level for 3.1.0. */
	MQTT_VERSION_3_1_1 = 4  /**< Protocol level for 3.1.1. */
};

/** @brief MQTT Quality of Service types. */
enum mqtt_qos {
	/** Lowest Quality of Service, no acknowledgment needed for published
	 *  message.
	 */
	MQTT_QOS_0_AT_MOST_ONCE = 0x00,

	/** Medium Quality of Service, if acknowledgment expected for published
	 *  message, duplicate messages permitted.
	 */
	MQTT_QOS_1_AT_LEAST_ONCE = 0x01,

	/** Highest Quality of Service, acknowledgment expected and message
	 *  shall be published only once. Message not published to interested
	 *  parties unless client issues a PUBREL.
	 */
	MQTT_QOS_2_EXACTLY_ONCE  = 0x02
};

/** @brief MQTT CONNACK return codes. */
enum mqtt_conn_return_code {
	/** Connection accepted. */
	MQTT_CONNECTION_ACCEPTED                = 0x00,

	/** The Server does not support the level of the MQTT protocol
	 * requested by the Client.
	 */
	MQTT_UNACCEPTABLE_PROTOCOL_VERSION      = 0x01,

	/** The Client identifier is correct UTF-8 but not allowed by the
	 *  Server.
	 */
	MQTT_IDENTIFIER_REJECTED                = 0x02,

	/** The Network Connection has been made but the MQTT service is
	 *  unavailable.
	 */
	MQTT_SERVER_UNAVAILABLE                 = 0x03,

	/** The data in the user name or password is malformed. */
	MQTT_BAD_USER_NAME_OR_PASSWORD          = 0x04,

	/** The Client is not authorized to connect. */
	MQTT_NOT_AUTHORIZED                     = 0x05
};

/** @brief MQTT SUBACK return codes. */
enum mqtt_suback_return_code {
	/** Subscription with QoS 0 succeeded. */
	MQTT_SUBACK_SUCCESS_QoS_0 = 0x00,

	/** Subscription with QoS 1 succeeded. */
	MQTT_SUBACK_SUCCESS_QoS_1 = 0x01,

	/** Subscription with QoS 2 succeeded. */
	MQTT_SUBACK_SUCCESS_QoS_2 = 0x02,

	/** Subscription for a topic failed. */
	MQTT_SUBACK_FAILURE = 0x80
};

/** @brief Abstracts UTF-8 encoded strings. */
struct mqtt_utf8 {
	u8_t *utf8;             /**< Pointer to UTF-8 string. */
	u32_t size;             /**< Size of UTF string, in bytes. */
};

/** @brief Abstracts binary strings. */
struct mqtt_binstr {
	u8_t *data;             /**< Pointer to binary stream. */
	u32_t len;              /**< Length of binary stream. */
};

/** @brief Abstracts MQTT UTF-8 encoded topic that can be subscribed
 *         to or published.
 */
struct mqtt_topic {
	/** Topic on to be published or subscribed to. */
	struct mqtt_utf8 topic;

	/** Quality of service requested for the subscription.
	 *  @ref mqtt_qos for details.
	 */
	u8_t qos;
};

/** @brief Parameters for a publish message. */
struct mqtt_publish_message {
	struct mqtt_topic topic;     /**< Topic on which data was published. */
	struct mqtt_binstr payload; /**< Payload on the topic published. */
};

/** @brief Parameters for a connection acknowledgment (CONNACK). */
struct mqtt_connack_param {
	/** The Session Present flag enables a Client to establish whether
	 *  the Client and Server have a consistent view about whether there
	 *  is already stored Session state.
	 */
	u8_t session_present_flag;

	/** The appropriate non-zero Connect return code indicates if the Server
	 *  is unable to process a connection request for some reason.
	 */
	enum mqtt_conn_return_code return_code;
};

/** @brief Parameters for MQTT publish acknowledgment (PUBACK). */
struct mqtt_puback_param {
	u16_t message_id;
};

/** @brief Parameters for MQTT publish receive (PUBREC). */
struct mqtt_pubrec_param {
	u16_t message_id;
};

/** @brief Parameters for MQTT publish release (PUBREL). */
struct mqtt_pubrel_param {
	u16_t message_id;
};

/** @brief Parameters for MQTT publish complete (PUBCOMP). */
struct mqtt_pubcomp_param {
	u16_t message_id;
};

/** @brief Parameters for MQTT subscription acknowledgment (SUBACK). */
struct mqtt_suback_param {
	u16_t message_id;
	struct mqtt_binstr return_codes;
};

/** @brief Parameters for MQTT unsubscribe acknowledgment (UNSUBACK). */
struct mqtt_unsuback_param {
	u16_t message_id;
};

/** @brief Parameters for a publish message. */
struct mqtt_publish_param {
	/** Messages including topic, QoS and its payload (if any)
	 *  to be published.
	 */
	struct mqtt_publish_message message;

	/** Message id used for the publish message. Redundant for QoS 0. */
	u16_t message_id;

	/** Duplicate flag. If 1, it indicates the message is being
	 *  retransmitted. Has no meaning with QoS 0.
	 */
	u8_t dup_flag : 1;

	/** Retain flag. If 1, the message shall be stored persistently
	 *  by the broker.
	 */
	u8_t retain_flag : 1;
};

/** @brief List of topics in a subscription request. */
struct mqtt_subscription_list {
	/** Array containing topics along with QoS for each. */
	struct mqtt_topic *list;

	/** Number of topics in the subscription list */
	u16_t list_count;

	/** Message id used to identify subscription request. */
	u16_t message_id;
};

/**
 * @brief Defines event parameters notified along with asynchronous events
 *        to the application.
 */
union mqtt_evt_param {
	/** Parameters accompanying MQTT_EVT_CONNACK event. */
	struct mqtt_connack_param connack;

	/** Parameters accompanying MQTT_EVT_PUBLISH event.
	 *
	 * @note PUBLISH event structure only contains payload size, the payload
	 *       data parameter should be ignored. Payload content has to be
	 *       read manually with @ref mqtt_read_publish_payload function.
	 */
	struct mqtt_publish_param publish;

	/** Parameters accompanying MQTT_EVT_PUBACK event. */
	struct mqtt_puback_param puback;

	/** Parameters accompanying MQTT_EVT_PUBREC event. */
	struct mqtt_pubrec_param pubrec;

	/** Parameters accompanying MQTT_EVT_PUBREL event. */
	struct mqtt_pubrel_param pubrel;

	/** Parameters accompanying MQTT_EVT_PUBCOMP event. */
	struct mqtt_pubcomp_param pubcomp;

	/** Parameters accompanying MQTT_EVT_SUBACK event. */
	struct mqtt_suback_param suback;

	/** Parameters accompanying MQTT_EVT_UNSUBACK event. */
	struct mqtt_unsuback_param unsuback;
};

/** @brief Defines MQTT asynchronous event notified to the application. */
struct mqtt_evt {
	/** Identifies the event. */
	enum mqtt_evt_type type;

	/** Contains parameters (if any) accompanying the event. */
	union mqtt_evt_param param;

	/** Event result. 0 or a negative error code (errno.h) indicating
	 *  reason of failure.
	 */
	int result;
};

struct mqtt_client;

/**
 * @brief Asynchronous event notification callback registered by the
 *        application.
 *
 * @param[in] client Identifies the client for which the event is notified.
 * @param[in] evt Event description along with result and associated
 *                parameters (if any).
 */
typedef void (*mqtt_evt_cb_t)(struct mqtt_client *client,
			      const struct mqtt_evt *evt);

/** @brief TLS configuration for secure MQTT transports. */
struct mqtt_sec_config {
	/** Indicates the preference for peer verification. */
	int peer_verify;

	/** Indicates the number of entries in the cipher list. */
	u32_t cipher_count;

	/** Indicates the list of ciphers to be used for the session.
	 *  May be NULL to use the default ciphers.
	 */
	int *cipher_list;

	/** Indicates the number of entries in the sec tag list. */
	u32_t sec_tag_count;

	/** Indicates the list of security tags to be used for the session. */
	sec_tag_t *sec_tag_list;

	/** Peer hostname for ceritificate verification.
	 *  May be NULL to skip hostname verification.
	 */
	char *hostname;
};

/** @brief MQTT transport type. */
enum mqtt_transport_type {
	/** Use non secure TCP transport for MQTT connection. */
	MQTT_TRANSPORT_NON_SECURE,

#if defined(CONFIG_MQTT_LIB_TLS)
	/** Use secure TCP transport (TLS) for MQTT connection. */
	MQTT_TRANSPORT_SECURE,
#endif /* CONFIG_MQTT_LIB_TLS */

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	/** Use non secure Websocket transport for MQTT connection. */
	MQTT_TRANSPORT_NON_SECURE_WEBSOCKET,
#if defined(CONFIG_MQTT_LIB_TLS)
	/** Use secure Websocket transport (TLS) for MQTT connection. */
	MQTT_TRANSPORT_SECURE_WEBSOCKET,
#endif
#endif /* CONFIG_MQTT_LIB_WEBSOCKET */

	/** Shall not be used as a transport type.
	 *  Indicator of maximum transport types possible.
	 */
	MQTT_TRANSPORT_NUM
};

/** @brief MQTT transport specific data. */
struct mqtt_transport {
	/** Transport type selection for client instance.
	 *  @ref mqtt_transport_type for possible values. MQTT_TRANSPORT_MAX
	 *  is not a valid type.
	 */
	enum mqtt_transport_type type;

	union {
		/* TCP socket transport for MQTT */
		struct {
			/** Socket descriptor. */
			int sock;
		} tcp;

#if defined(CONFIG_MQTT_LIB_TLS)
		/* TLS socket transport for MQTT */
		struct {
			/** Socket descriptor. */
			int sock;

			/** TLS configuration. See @ref mqtt_sec_config for
			 *  details.
			 */
			struct mqtt_sec_config config;
		} tls;
#endif /* CONFIG_MQTT_LIB_TLS */
	};

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	/** Websocket transport for MQTT */
	struct {
		/** Websocket configuration. */
		struct websocket_request config;

		/** Socket descriptor */
		int sock;

		/** Websocket timeout */
		s32_t timeout;
	} websocket;
#endif

#if defined(CONFIG_SOCKS)
	struct {
		struct sockaddr addr;
		socklen_t addrlen;
	} proxy;
#endif
};

/** @brief MQTT internal state. */
struct mqtt_internal {
	/** Internal. Mutex to protect access to the client instance. */
	struct sys_mutex mutex;

	/** Internal. Wall clock value (in milliseconds) of the last activity
	 *  that occurred. Needed for periodic PING.
	 */
	u32_t last_activity;

	/** Internal. Client's state in the connection. */
	u32_t state;

	/** Internal.  Packet length read so far. */
	u32_t rx_buf_datalen;

	/** Internal. Remaining payload length to read. */
	u32_t remaining_payload;
};

/**
 * @brief MQTT Client definition to maintain information relevant to the
 *        client.
 */
struct mqtt_client {
	/** MQTT client internal state. */
	struct mqtt_internal internal;

	/** MQTT transport configuration and data. */
	struct mqtt_transport transport;

	/** Unique client identification to be used for the connection. */
	struct mqtt_utf8 client_id;

	/** Broker details, for example, address, port. Address type should
	 *  be compatible with transport used.
	 */
	const void *broker;

	/** User name (if any) to be used for the connection. NULL indicates
	 *  no user name.
	 */
	struct mqtt_utf8 *user_name;

	/** Password (if any) to be used for the connection. Note that if
	 *  password is provided, user name shall also be provided. NULL
	 *  indicates no password.
	 */
	struct mqtt_utf8 *password;

	/** Will topic and QoS. Can be NULL. */
	struct mqtt_topic *will_topic;

	/** Will message. Can be NULL. Non NULL value valid only if will topic
	 *  is not NULL.
	 */
	struct mqtt_utf8 *will_message;

	/** Application callback registered with the module to get MQTT events.
	 */
	mqtt_evt_cb_t evt_cb;

	/** Receive buffer used for MQTT packet reception in RX path. */
	u8_t *rx_buf;

	/** Size of receive buffer. */
	u32_t rx_buf_size;

	/** Transmit buffer used for creating MQTT packet in TX path. */
	u8_t *tx_buf;

	/** Size of transmit buffer. */
	u32_t tx_buf_size;

	/** Keepalive interval for this client in seconds.
	 *  Default is CONFIG_MQTT_KEEPALIVE.
	 */
	u16_t keepalive;

	/** MQTT protocol version. */
	u8_t protocol_version;

	/** Will retain flag, 1 if will message shall be retained persistently.
	 */
	u8_t will_retain : 1;

	/** Clean session flag indicating a fresh (1) or a retained session (0).
	 *  Default is 1.
	 */
	u8_t clean_session : 1;
};

/**
 * @brief Initializes the client instance.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 *
 * @note Shall be called to initialize client structure, before setting any
 *       client parameters and before connecting to broker.
 */
void mqtt_client_init(struct mqtt_client *client);

#if defined(CONFIG_SOCKS)
/*
 * @brief Set proxy server details
 *
 * @param[in] client Client instance for which the procedure is requested,
 *                   Shall not be NULL.
 * @param[in] proxy_addr Proxy server address.
 * @param[in] addrlen Proxy server address length.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 *
 * @note Must be called before calling mqtt_connect().
 */
int mqtt_client_set_proxy(struct mqtt_client *client,
			  struct sockaddr *proxy_addr,
			  socklen_t addrlen);
#endif

/**
 * @brief API to request new MQTT client connection.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 *
 * @note This memory is assumed to be resident until mqtt_disconnect is called.
 * @note Any subsequent changes to parameters like broker address, user name,
 *       device id, etc. have no effect once MQTT connection is established.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 *
 * @note Default protocol revision used for connection request is 3.1.1. Please
 *       set client.protocol_version = MQTT_VERSION_3_1_0 to use protocol 3.1.0.
 * @note
 *       @rst
 *          Please modify :option:`CONFIG_MQTT_KEEPALIVE` time to override
 *          default of 1 minute.
 *       @endrst
 */
int mqtt_connect(struct mqtt_client *client);

/**
 * @brief API to publish messages on topics.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[in] param Parameters to be used for the publish message.
 *                  Shall not be NULL.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish(struct mqtt_client *client,
		 const struct mqtt_publish_param *param);

/**
 * @brief API used by client to send acknowledgment on receiving QoS1 publish
 *        message. Should be called on reception of @ref MQTT_EVT_PUBLISH with
 *        QoS level @ref MQTT_QOS_1_AT_LEAST_ONCE.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[in] param Identifies message being acknowledged.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish_qos1_ack(struct mqtt_client *client,
			  const struct mqtt_puback_param *param);

/**
 * @brief API used by client to send acknowledgment on receiving QoS2 publish
 *        message. Should be called on reception of @ref MQTT_EVT_PUBLISH with
 *        QoS level @ref MQTT_QOS_2_EXACTLY_ONCE.
 *
 * @param[in] client Identifies client instance for which the procedure is
 *                   requested. Shall not be NULL.
 * @param[in] param Identifies message being acknowledged.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish_qos2_receive(struct mqtt_client *client,
			      const struct mqtt_pubrec_param *param);

/**
 * @brief API used by client to request release of QoS2 publish message.
 *        Should be called on reception of @ref MQTT_EVT_PUBREC.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[in] param Identifies message being released.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish_qos2_release(struct mqtt_client *client,
			      const struct mqtt_pubrel_param *param);

/**
 * @brief API used by client to send acknowledgment on receiving QoS2 publish
 *        release message. Should be called on reception of
 *        @ref MQTT_EVT_PUBREL.
 *
 * @param[in] client Identifies client instance for which the procedure is
 *                   requested. Shall not be NULL.
 * @param[in] param Identifies message being completed.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish_qos2_complete(struct mqtt_client *client,
			       const struct mqtt_pubcomp_param *param);

/**
 * @brief API to request subscription of one or more topics on the connection.
 *
 * @param[in] client Identifies client instance for which the procedure
 *                   is requested. Shall not be NULL.
 * @param[in] param Subscription parameters. Shall not be NULL.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_subscribe(struct mqtt_client *client,
		   const struct mqtt_subscription_list *param);

/**
 * @brief API to request unsubscription of one or more topics on the connection.
 *
 * @param[in] client Identifies client instance for which the procedure is
 *                   requested. Shall not be NULL.
 * @param[in] param Parameters describing topics being unsubscribed from.
 *                  Shall not be NULL.
 *
 * @note QoS included in topic description is unused in this API.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_unsubscribe(struct mqtt_client *client,
		     const struct mqtt_subscription_list *param);

/**
 * @brief API to send MQTT ping. The use of this API is optional, as the library
 *        handles the connection keep-alive on it's own, see @ref mqtt_live.
 *
 * @param[in] client Identifies client instance for which procedure is
 *                   requested.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_ping(struct mqtt_client *client);

/**
 * @brief API to disconnect MQTT connection.
 *
 * @param[in] client Identifies client instance for which procedure is
 *                   requested.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_disconnect(struct mqtt_client *client);

/**
 * @brief API to abort MQTT connection. This will close the corresponding
 *        transport without closing the connection gracefully at the MQTT level
 *        (with disconnect message).
 *
 * @param[in] client Identifies client instance for which procedure is
 *                   requested.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_abort(struct mqtt_client *client);

/**
 * @brief This API should be called periodically for the client to be able
 *        to keep the connection alive by sending Ping Requests if need be.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 *
 * @note  Application shall ensure that the periodicity of calling this function
 *        makes it possible to respect the Keep Alive time agreed with the
 *        broker on connection. @ref mqtt_connect for details on Keep Alive
 *        time.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_live(struct mqtt_client *client);

/**
 * @brief Receive an incoming MQTT packet. The registered callback will be
 *        called with the packet content.
 *
 * @note In case of PUBLISH message, the payload has to be read separately with
 *       @ref mqtt_read_publish_payload function. The size of the payload to
 *       read is provided in the publish event structure.
 *
 * @note This is a non-blocking call.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_input(struct mqtt_client *client);

/**
 * @brief Read the payload of the received PUBLISH message. This function should
 *        be called within the MQTT event handler, when MQTT PUBLISH message is
 *        notified.
 *
 * @note This is a non-blocking call.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[out] buffer Buffer where payload should be stored.
 * @param[in] length Length of the buffer, in bytes.
 *
 * @return Number of bytes read or a negative error code (errno.h) indicating
 *         reason of failure.
 */
int mqtt_read_publish_payload(struct mqtt_client *client, void *buffer,
			      size_t length);

/**
 * @brief Blocking version of @ref mqtt_read_publish_payload function.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[out] buffer Buffer where payload should be stored.
 * @param[in] length Length of the buffer, in bytes.
 *
 * @return Number of bytes read or a negative error code (errno.h) indicating
 *         reason of failure.
 */
int mqtt_read_publish_payload_blocking(struct mqtt_client *client, void *buffer,
				       size_t length);

/**
 * @brief Blocking version of @ref mqtt_read_publish_payload function which
 *        runs until the required number of bytes are read.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[out] buffer Buffer where payload should be stored.
 * @param[in] length Number of bytes to read.
 *
 * @return 0 if success, otherwise a negative error code (errno.h) indicating
 *         reason of failure.
 */
int mqtt_readall_publish_payload(struct mqtt_client *client, u8_t *buffer,
				 size_t length);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_MQTT_H_ */

/**@}  */
