/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_MQTT_H_
#define ZEPHYR_INCLUDE_NET_MQTT_H_

#include <net/mqtt_types.h>
#include <net/net_context.h>
#include <net/net_app.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT library
 * @defgroup mqtt MQTT library
 * @ingroup networking
 * @{
 */

/**
 * MQTT application type
 */
enum mqtt_app {
	/** Publisher and Subscriber application */
	MQTT_APP_PUBLISHER_SUBSCRIBER,
	/** Publisher only application */
	MQTT_APP_PUBLISHER,
	/** Subscriber only application */
	MQTT_APP_SUBSCRIBER,
	/** MQTT Server */
	MQTT_APP_SERVER
};

/**
 * MQTT context structure
 *
 * @details Context structure for the MQTT high-level API with support for QoS.
 *
 * This API is designed for asynchronous operation, so callbacks are
 * executed when some events must be addressed outside the MQTT routines.
 * Those events are triggered by the reception or transmission of MQTT messages
 * and are defined by the MQTT v3.1.1 spec, see:
 *
 * http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
 *
 * For example, assume that Zephyr is operating as a MQTT_APP_SUBSCRIBER, so it
 * may receive the MQTT PUBLISH and MQTT PUBREL (QoS2) messages. Once the
 * messages are parsed and validated, the #publish_rx callback is executed.
 *
 * Internally, the #publish_rx callback must store the #mqtt_publish_msg message
 * when a MQTT PUBLISH msg is received. When a MQTT PUBREL message is received,
 * the application must evaluate if the PUBREL's Packet Identifier matches a
 * previous received MQTT PUBLISH message. In this case, the user may provide a
 * collection of #mqtt_publish_msg structs (array of structs) to store those
 * messages.
 *
 * <b>NOTE: The application (and not the API) is in charge of keeping track of
 * the state of the received and sent messages.</b>
 */
struct mqtt_ctx {
	/** Net app context structure */
	struct net_app_ctx net_app_ctx;
	s32_t net_init_timeout;
	s32_t net_timeout;

	/** Connectivity */
	char *peer_addr_str;
	u16_t peer_port;

#if defined(CONFIG_MQTT_LIB_TLS)
	/** TLS parameters */
	u8_t *request_buf;
	size_t request_buf_len;
	u8_t *personalization_data;
	size_t personalization_data_len;
	char *cert_host;

	/** TLS thread parameters */
	struct k_mem_pool *tls_mem_pool;
	k_thread_stack_t *tls_stack;
	size_t tls_stack_size;

	/** TLS callback */
	net_app_ca_cert_cb_t cert_cb;
	net_app_entropy_src_cb_t entropy_src_cb;

	/** TLS handshake */
	struct k_sem tls_hs_wait;
	s32_t tls_hs_timeout;
#endif

	/** Callback executed when a MQTT CONNACK msg is received and validated.
	 * If this function pointer is not used, must be set to NULL.
	 */
	void (*connect)(struct mqtt_ctx *ctx);

	/** Callback executed when a MQTT DISCONNECT msg is sent.
	 * If this function pointer is not used, must be set to NULL.
	 */
	void (*disconnect)(struct mqtt_ctx *ctx);

	/** Callback executed when a #MQTT_APP_PUBLISHER application receives
	 * a MQTT PUBxxxx msg.
	 * If type is MQTT_PUBACK, MQTT_PUBCOMP or MQTT_PUBREC, this callback
	 * must return 0 if pkt_id matches the packet id of a previously
	 * received MQTT_PUBxxx message. If this callback returns 0, the caller
	 * will continue.
	 * Any other value will stop the QoS handshake and the caller will
	 * return -EINVAL. The application must discard all the messages
	 * already processed.
	 *
	 * <b>Note: this callback must be not NULL</b>
	 *
	 * @param [in] ctx MQTT context
	 * @param [in] pkt_id Packet Identifier for the input MQTT msg
	 * @param [in] type Packet type
	 */
	int (*publish_tx)(struct mqtt_ctx *ctx, u16_t pkt_id,
			  enum mqtt_packet type);

	/** Callback executed when a MQTT_APP_SUBSCRIBER,
	 * MQTT_APP_PUBLISHER_SUBSCRIBER or MQTT_APP_SERVER applications receive
	 * a MQTT PUBxxxx msg. If this callback returns 0, the caller will
	 * continue. If type is MQTT_PUBREL this callback must return 0 if
	 * pkt_id matches the packet id of a previously received MQTT_PUBxxx
	 * message. Any other value will stop the QoS handshake and the caller
	 * will return -EINVAL
	 *
	 * <b>Note: this callback must be not NULL</b>
	 *
	 * @param [in] ctx MQTT context
	 * @param [in] msg Publish message, this parameter is only used
	 *                 when the type is MQTT_PUBLISH
	 * @param [in] pkt_id Packet Identifier for the input msg
	 * @param [in] type Packet type
	 */
	int (*publish_rx)(struct mqtt_ctx *ctx, struct mqtt_publish_msg *msg,
			  u16_t pkt_id, enum mqtt_packet type);

	/** Callback executed when a MQTT_APP_SUBSCRIBER or
	 * MQTT_APP_PUBLISHER_SUBSCRIBER receives the MQTT SUBACK message
	 * If this callback returns 0, the caller will continue. Any other
	 * value will make the caller return -EINVAL.
	 *
	 * <b>Note: this callback must be not NULL</b>
	 *
	 * @param [in] ctx MQTT context
	 * @param [in] pkt_id Packet Identifier for the MQTT SUBACK msg
	 * @param [in] items Number of elements in the qos array
	 * @param [in] qos Array of QoS values
	 */
	int (*subscribe)(struct mqtt_ctx *ctx, u16_t pkt_id,
			 u8_t items, enum mqtt_qos qos[]);

	/** Callback executed when a MQTT_APP_SUBSCRIBER or
	 * MQTT_APP_PUBLISHER_SUBSCRIBER receives the MQTT UNSUBACK message
	 * If this callback returns 0, the caller will continue. Any other value
	 * will make the caller return -EINVAL
	 *
	 * <b>Note: this callback must be not NULL</b>
	 *
	 * @param [in] ctx	MQTT context
	 * @param [in] pkt_id	Packet Identifier for the MQTT SUBACK msg
	 */
	int (*unsubscribe)(struct mqtt_ctx *ctx, u16_t pkt_id);

	/** Callback executed when an incoming message doesn't pass the
	 * validation stage. This callback may be NULL.
	 * The pkt_type variable may be set to MQTT_INVALID, if the parsing
	 * stage is aborted before determining the MQTT msg packet type.
	 *
	 * @param [in] ctx	MQTT context
	 * @param [in] pkt_type	MQTT Packet type
	 */
	void (*malformed)(struct mqtt_ctx *ctx, u16_t pkt_type);

	/* Internal use only */
	int (*rcv)(struct mqtt_ctx *ctx, struct net_pkt *);

	/** Application type, see: enum mqtt_app */
	u8_t app_type;

	/* Clean session is also part of the MQTT CONNECT msg, however app
	 * behavior is influenced by this parameter, so we keep a copy here
	 */
	/** MQTT Clean Session parameter */
	u8_t clean_session:1;

	/** 1 if the MQTT application is connected and 0 otherwise */
	u8_t connected:1;
};

/**
 * Initializes the MQTT context structure
 *
 * @param ctx MQTT context structure
 * @param app_type See enum mqtt_app
 * @retval 0 always
 */
int mqtt_init(struct mqtt_ctx *ctx, enum mqtt_app app_type);

/**
 * Release the MQTT context structure
 *
 * @param ctx MQTT context structure
 * @retval 0 on success, and <0 if error
 */
int mqtt_close(struct mqtt_ctx *ctx);

/**
 * Connect to an MQTT broker
 *
 * @param ctx MQTT context structure
 * @retval 0 on success, and <0 if error
 */

int mqtt_connect(struct mqtt_ctx *ctx);

/**
 * Sends the MQTT CONNECT message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] msg MQTT CONNECT msg
 *
 * @retval 0 on success
 * @retval -EIO
 * @retval -ENOMEM
 * @retval -EINVAL
 */
int mqtt_tx_connect(struct mqtt_ctx *ctx, struct mqtt_connect_msg *msg);

/**
 * Send the MQTT DISCONNECT message
 *
 * @param [in] ctx MQTT context structure
 *
 * @retval 0 on success
 * @retval -EIO
 * @retval -ENOMEM
 * @retval -EINVAL
 */
int mqtt_tx_disconnect(struct mqtt_ctx *ctx);

/**
 * Sends the MQTT PUBACK message with the given packet id
 *
 * @param [in] ctx MQTT context structure
 * @param [in] id MQTT Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int mqtt_tx_puback(struct mqtt_ctx *ctx, u16_t id);

/**
 * Sends the MQTT PUBCOMP message with the given packet id
 *
 * @param [in] ctx MQTT context structure
 * @param [in] id MQTT Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int mqtt_tx_pubcomp(struct mqtt_ctx *ctx, u16_t id);

/**
 * Sends the MQTT PUBREC message with the given packet id
 *
 * @param [in] ctx MQTT context structure
 * @param [in] id MQTT Packet Identifier
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int mqtt_tx_pubrec(struct mqtt_ctx *ctx, u16_t id);

/**
 * Sends the MQTT PUBREL message with the given packet id
 *
 * @param [in] ctx MQTT context structure
 * @param [in] id MQTT Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int mqtt_tx_pubrel(struct mqtt_ctx *ctx, u16_t id);

/**
 * Sends the MQTT PUBLISH message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] msg MQTT PUBLISH msg
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int mqtt_tx_publish(struct mqtt_ctx *ctx, struct mqtt_publish_msg *msg);

/**
 * Sends the MQTT PINGREQ message
 *
 * @param [in] ctx MQTT context structure
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int mqtt_tx_pingreq(struct mqtt_ctx *ctx);

/**
 * Sends the MQTT SUBSCRIBE message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] pkt_id Packet identifier for the MQTT SUBSCRIBE msg
 * @param [in] items Number of elements in 'topics' and 'qos' arrays
 * @param [in] topics Array of 'items' elements containing C strings.
 *                    For example: {"sensors", "lights", "doors"}
 * @param [in] qos Array of 'items' elements containing MQTT QoS values:
 *                 MQTT_QoS0, MQTT_QoS1, MQTT_QoS2. For example for the 'topics'
 *                 array above the following QoS may be used: {MQTT_QoS0,
 *                 MQTT_QoS2, MQTT_QoS1}, indicating that the subscription to
 *                 'lights' must be done with MQTT_QoS2
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int mqtt_tx_subscribe(struct mqtt_ctx *ctx, u16_t pkt_id, u8_t items,
		      const char *topics[], const enum mqtt_qos qos[]);

/**
 * Sends the MQTT UNSUBSCRIBE message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] pkt_id Packet identifier for the MQTT UNSUBSCRIBE msg
 * @param [in] items Number of elements in the 'topics' array
 * @param [in] topics Array of 'items' elements containing C strings
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 * @retval -EIO
 */
int mqtt_tx_unsubscribe(struct mqtt_ctx *ctx, u16_t pkt_id, u8_t items,
			const char *topics[]);


/**
 * Parses and validates the MQTT CONNACK msg
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 * @param [in] clean_session MQTT clean session parameter
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_rx_connack(struct mqtt_ctx *ctx, struct net_buf *rx,
		    int clean_session);

/**
 * Parses and validates the MQTT PUBACK message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_rx_puback(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * Parses and validates the MQTT PUBCOMP message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_rx_pubcomp(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * Parses and validates the MQTT PUBREC message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_rx_pubrec(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * Parses and validates the MQTT PUBREL message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_rx_pubrel(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * Parses the MQTT PINGRESP message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_rx_pingresp(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * Parses the MQTT SUBACK message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_rx_suback(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * Parses the MQTT UNSUBACK message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_rx_unsuback(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * Parses the MQTT PUBLISH message
 *
 * @param [in] ctx MQTT context structure
 * @param [in] rx Data buffer
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 */
int mqtt_rx_publish(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_MQTT_H_ */
