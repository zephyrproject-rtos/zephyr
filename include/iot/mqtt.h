/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MQTT_H_
#define _MQTT_H_

#include <iot/mqtt_types.h>
#include <net/net_context.h>

/**
 * @brief mqtt_app	MQTT application type
 */
enum mqtt_app {
	/** Publisher only application */
	MQTT_APP_PUBLISHER,
	/** Subscriber only application */
	MQTT_APP_SUBSCRIBER,
	/** Publisher and Subscriber application */
	MQTT_APP_PUBLISHER_SUBSCRIBER,
	/** MQTT Server */
	MQTT_APP_SERVER
};

/**
 * @brief struct mqtt_ctx	MQTT context structure
 * @details
 * Context structure for the MQTT high-level API with support for QoS.
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
 * previous received MQTT PUBLISH message. In this case, the #publish_rx_data
 * may be used to refer to a collection of #mqtt_publish_msg structs (array of
 * structs).
 *
 * <b>NOTE: The application (and not the API) is in charge of keeping track of
 * the state of the received and sent messages.</b>
 */
struct mqtt_ctx {
	/** IP stack context structure */
	struct net_context *net_ctx;
	/** Network timeout for tx and rx routines */
	uint32_t net_timeout;

	/** Callback executed when a MQTT CONNACK msg is received and validated.
	 * If this function pointer is not used, must be set to NULL.
	 * The argument for the routine pointed to by this pointer
	 * is represented by #connect_data.
	 */
	void (*connect)(void *data);

	/** Data passed to the #connect callback. */
	void *connect_data;

	/** Callback executed when a MQTT DISCONNECT msg is sent.
	 * If this function pointer is not used, must be set to NULL.
	 * The argument for the routine pointed to by this pointer
	 * is represented by #disconnect_data.
	 */
	void (*disconnect)(void *data);

	/** Data passed to the #disconnect callback. */
	void *disconnect_data;

	/** Callback executed when a #MQTT_APP_PUBLISHER application receives
	 * a MQTT PUBxxxx msg.
	 *
	 * <b>Note: this callback must be not NULL</b>
	 *
	 * @param [in] data	User provided data, represented by
	 *			#publish_tx_data
	 * @param [in] pkt_id	Packet Identifier for the input MQTT msg
	 * @param [in] type	Packet type
	 * @return		If this callback returns 0, the caller will
	 *			continue.
	 *
	 * @return		If type is MQTT_PUBACK, MQTT_PUBCOMP or
	 *			MQTT_PUBREC, this callback must return 0 if
	 *			pkt_id matches the packet id of a previously
	 *			received MQTT_PUBxxx message.
	 *
	 * @return		<b>Note: the application must discard all the
	 *			messages already processed</b>
	 *
	 * @return		Any other value will stop the QoS handshake
	 *			and the caller will return -EINVAL
	 */
	int (*publish_tx)(void *data, uint16_t pkt_id, enum mqtt_packet type);

	/** Data passed to the #publish_tx callback */
	void *publish_tx_data;

	/** Callback executed when a MQTT_APP_SUBSCRIBER,
	 * MQTT_APP_PUBLISHER_SUBSCRIBER or MQTT_APP_SERVER application receive
	 * a MQTT PUBxxxx msg.
	 *
	 * <b>Note: this callback must be not NULL</b>
	 *
	 * @param [in] data	User provided data, represented by
	 *			#publish_rx_data
	 * @param [in] msg	Publish message, this parameter is only used
	 *			when the type is MQTT_PUBLISH
	 * @param [in] pkt_id	Packet Identifier for the input msg
	 * @param [in] type	Packet type
	 * @return		If this callback returns 0, the caller will
	 *			continue.
	 *
	 * @return		If type is MQTT_PUBREL this callback must return
	 *			0 if pkt_id matches the packet id of a
	 *			previously received MQTT_PUBxxx message.
	 *
	 * @return		<b>Note: the application must discard all the
	 *			messages already processed</b>
	 *
	 * @return		Any other value will stop the QoS handshake
	 *			and the caller will return -EINVAL
	 */
	int (*publish_rx)(void *data, struct mqtt_publish_msg *msg,
			  uint16_t pkt_id, enum mqtt_packet type);

	/** Data passed to the #publish_rx callback */
	void *publish_rx_data;

	/** Callback executed when a MQTT_APP_SUBSCRIBER or
	 * MQTT_APP_PUBLISHER_SUBSCRIBER receives the MQTT SUBACK message
	 *
	 * <b>Note: this callback must be not NULL</b>
	 *
	 * @param [in] data	User provided data, represented by
	 *			#subscribe_data
	 * @param [in] pkt_id	Packet Identifier for the MQTT SUBACK msg
	 * @param [in] items	Number of elements in the qos array
	 * @param [in] qos	Array of QoS values
	 * @return		If this callback returns 0, the caller will
	 *			continue
	 * @return		Any other value will make the caller return
	 *			-EINVAL
	 */
	int (*subscribe)(void *data, uint16_t pkt_id,
			 uint8_t items, enum mqtt_qos qos[]);

	/** Data passed to the #subscribe callback */
	void *subscribe_data;

	/** Callback executed when a MQTT_APP_SUBSCRIBER or
	 * MQTT_APP_PUBLISHER_SUBSCRIBER receives the MQTT UNSUBACK message
	 *
	 * <b>Note: this callback must be not NULL</b>
	 * @param [in] data	User provided data, represented by
	 *			#unsubscribe_data
	 * @param [in] pkt_id	Packet Identifier for the MQTT SUBACK msg
	 * @return		If this callback returns 0, the caller will
	 *			continue
	 * @return		Any other value will make the caller return
	 *			-EINVAL
	 */
	int (*unsubscribe)(void *data, uint16_t pkt_id);

	/** Data passed to the #unsubscribe callback */
	void *unsubscribe_data;

	/* Clean session is also part of the MQTT CONNECT msg, however app
	 * behavior is influenced by this parameter, so we keep a copy here
	 */
	/** MQTT Clean Session parameter */
	uint8_t clean_session:1;

	/** 1 if the MQTT application is connected and 0 otherwise */
	uint8_t connected:1;

	/** Application type */
	enum mqtt_app app_type;
};

/**
 * @brief mqtt_tx_connect	Sends the MQTT CONNECT message
 * @param [in] ctx		MQTT context structure
 * @param [in] msg		MQTT CONNECT msg
 * @return			0 on success
 * @return			-EIO on network error
 * @return			-ENOMEM if no data/tx buffer is available
 * @return			-EINVAL if invalid data was passed to this
 *				routine
 */
int mqtt_tx_connect(struct mqtt_ctx *ctx, struct mqtt_connect_msg *msg);

/**
 * @brief mqtt_tx_disconnect	Send the MQTT DISCONNECT message
 * @param [in] ctx		MQTT context structure
 * @return			0 on success
 * @return			-EIO on network error
 * @return			-ENOMEM if no data/tx buffer is available
 * @return			-EINVAL if invalid data was passed to this
 *				routine
 */
int mqtt_tx_disconnect(struct mqtt_ctx *ctx);

/**
 * @brief mqtt_tx_puback	Sends the MQTT PUBACK message with the given
 *				packet id
 * @param [in] ctx		MQTT context structure
 * @param [in] id		MQTT Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed to
 *				this routine
 * @return			-ENOMEM if a tx buffer is not available
 * @return			-EIO on network error
 */
int mqtt_tx_puback(struct mqtt_ctx *ctx, uint16_t id);

/**
 * @brief mqtt_tx_pubcomp	Sends the MQTT PUBCOMP message with the given
 *				packet id
 * @param [in] ctx		MQTT context structure
 * @param [in] id		MQTT Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed to
 *				this routine
 * @return			-ENOMEM if a tx buffer is not available
 * @return			-EIO on network error
 */
int mqtt_tx_pubcomp(struct mqtt_ctx *ctx, uint16_t id);

/**
 * @brief mqtt_tx_pubrec	Sends the MQTT PUBREC message with the given
 *				packet id
 * @param [in] ctx		MQTT context structure
 * @param [in] id		MQTT Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed to
 *				this routine
 * @return			-ENOMEM if a tx buffer is not available
 * @return			-EIO on network error
 */
int mqtt_tx_pubrec(struct mqtt_ctx *ctx, uint16_t id);

/**
 * @brief mqtt_tx_pubrel	Sends the MQTT PUBREL message with the given
 *				packet id
 * @param [in] ctx		MQTT context structure
 * @param [in] id		MQTT Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed to
 *				this routine
 * @return			-ENOMEM if a tx buffer is not available
 * @return			-EIO on network error
 */
int mqtt_tx_pubrel(struct mqtt_ctx *ctx, uint16_t id);

/**
 * @brief mqtt_tx_publish	Sends the MQTT PUBLISH message
 * @param [in] ctx		MQTT context structure
 * @param [in] msg		MQTT PUBLISH msg
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed to
 *				this routine
 * @return			-ENOMEM if a tx buffer is not available
 * @return			-EIO on network error
 */
int mqtt_tx_publish(struct mqtt_ctx *ctx, struct mqtt_publish_msg *msg);

/**
 * @brief mqtt_tx_pingreq	Sends the MQTT PINGREQ message
 * @param [in] ctx		MQTT context structure
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed to
 *				this routine
 * @return			-ENOMEM if a tx buffer is not available
 * @return			-EIO on network error
 */
int mqtt_tx_pingreq(struct mqtt_ctx *ctx);

/**
 * @brief mqtt_tx_subscribe	Sends the MQTT SUBSCRIBE message
 * @param [in] ctx		MQTT context structure
 * @param [in] pkt_id		Packet identifier for the MQTT SUBSCRIBE msg
 * @param [in] items		Number of elements in 'topics' and 'qos' arrays
 * @param [in] topics		Array of 'items' elements containing C strings.
 *				For example: {"sensors", "lights", "doors"}
 * @param [in] qos		Array of 'items' elements containing MQTT QoS
 *				values: MQTT_QoS0, MQTT_QoS1, MQTT_QoS2. For
 *				example for the 'topics' array above the
 *				following QoS may be used:
 *				{MQTT_QoS0, MQTT_QoS2, MQTT_QoS1}, indicating
 *				that the subscription to 'lights' must be done
 *				with MQTT_QoS2
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed to
 *				this routine
 * @return			-ENOMEM if a tx buffer is not available
 * @return			-EIO on network error
 */
int mqtt_tx_subscribe(struct mqtt_ctx *ctx, uint16_t pkt_id, int items,
		      const char *topics[], enum mqtt_qos qos[]);

/**
 * @brief mqtt_tx_unsubscribe	Sends the MQTT UNSUBSCRIBE message
 * @param [in] ctx		MQTT context structure
 * @param [in] pkt_id		Packet identifier for the MQTT UNSUBSCRIBE msg
 * @param [in] items		Number of elements in the 'topics' array
 * @param [in] topics		Array of 'items' elements containing C strings
 * @return
 */
int mqtt_tx_unsubscribe(struct mqtt_ctx *ctx, uint16_t pkt_id, int items,
			const char *topics[]);

int mqtt_rx_connack(struct mqtt_ctx *ctx, struct net_buf *rx,
		    int clean_session);

/**
 * @brief mqtt_rx_puback	Parses and validates the MQTT PUBACK message
 * @param [in] ctx		MQTT context structure
 * @param [in] rx		RX buffer from the IP stack
 * @return			0 on success
 * @return			-EINVAL on error
 */
int mqtt_rx_puback(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * @brief mqtt_rx_pubcomp	Parses and validates the MQTT PUBCOMP message
 * @param [in] ctx		MQTT context structure
 * @param [in] rx		RX buffer from the IP stack
 * @return			0 on success
 * @return			-EINVAL on error
 */
int mqtt_rx_pubcomp(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * @brief mqtt_rx_pubrec	Parses and validates the MQTT PUBREC message
 * @param [in] ctx		MQTT context structure
 * @param [in] rx		RX buffer from the IP stack
 * @return			0 on success
 * @return			-EINVAL on error
 */
int mqtt_rx_pubrec(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * @brief mqtt_rx_pubrel	Parses and validates the MQTT PUBREL message
 * @details			rx is an RX buffer from the IP stack
 * @param [in] ctx		MQTT context structure
 * @param [in] rx		RX buffer from the IP stack
 * @return			0 on success
 * @return			-EINVAL on error
 */
int mqtt_rx_pubrel(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * @brief mqtt_rx_pingresp	Parses the MQTT PINGRESP message
 * @param [in] ctx		MQTT context structure
 * @param [in] rx		RX buffer from the IP stack
 * @return			0 on success
 * @return			-EINVAL on error
 */
int mqtt_rx_pingresp(struct mqtt_ctx *ctx, struct net_buf *rx);

/**
 * @brief mqtt_rx_suback	Parses the MQTT SUBACK message
 * @param [in] ctx		MQTT context structure
 * @param [in] rx		RX buffer from the IP stack
 * @return			0 on success
 * @return			-EINVAL on error
 */
int mqtt_rx_suback(struct mqtt_ctx *ctx, struct net_buf *rx);

#endif
