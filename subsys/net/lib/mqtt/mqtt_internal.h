/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_internal.h
 *
 * @brief Function and data structures internal to MQTT module.
 */

#ifndef MQTT_INTERNAL_H_
#define MQTT_INTERNAL_H_

#include <stdint.h>
#include <string.h>

#include <zephyr/net/mqtt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Keep alive time for MQTT (in seconds). Sending of Ping Requests to
 *        keep the connection alive are governed by this value.
 */
#define MQTT_KEEPALIVE CONFIG_MQTT_KEEPALIVE

/**@brief Clean session on every connect (1) or keep subscriptions and messages
 *        between connects (0)
 */
#define MQTT_CLEAN_SESSION (IS_ENABLED(CONFIG_MQTT_CLEAN_SESSION) ? 1U : 0U)

/**@brief Minimum mandatory size of fixed header. */
#define MQTT_FIXED_HEADER_MIN_SIZE 2

/**@brief Maximum size of the fixed header. Remaining length size is 4 in this
 *        case.
 */
#define MQTT_FIXED_HEADER_MAX_SIZE 5

/**@brief MQTT Control Packet Types. */
#define MQTT_PKT_TYPE_CONNECT     0x10
#define MQTT_PKT_TYPE_CONNACK     0x20
#define MQTT_PKT_TYPE_PUBLISH     0x30
#define MQTT_PKT_TYPE_PUBACK      0x40
#define MQTT_PKT_TYPE_PUBREC      0x50
#define MQTT_PKT_TYPE_PUBREL      0x60
#define MQTT_PKT_TYPE_PUBCOMP     0x70
#define MQTT_PKT_TYPE_SUBSCRIBE   0x80
#define MQTT_PKT_TYPE_SUBACK      0x90
#define MQTT_PKT_TYPE_UNSUBSCRIBE 0xA0
#define MQTT_PKT_TYPE_UNSUBACK    0xB0
#define MQTT_PKT_TYPE_PINGREQ     0xC0
#define MQTT_PKT_TYPE_PINGRSP     0xD0
#define MQTT_PKT_TYPE_DISCONNECT  0xE0

/**@brief MQTT Property Types. */
#define MQTT_PROP_PAYLOAD_FORMAT_INDICATOR          0x01
#define MQTT_PROP_MESSAGE_EXPIRY_INTERVAL           0x02
#define MQTT_PROP_CONTENT_TYPE                      0x03
#define MQTT_PROP_RESPONSE_TOPIC                    0x08
#define MQTT_PROP_CORRELATION_DATA                  0x09
#define MQTT_PROP_SUBSCRIPTION_IDENTIFIER           0x0B
#define MQTT_PROP_SESSION_EXPIRY_INTERVAL           0x11
#define MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER        0x12
#define MQTT_PROP_SERVER_KEEP_ALIVE                 0x13
#define MQTT_PROP_AUTHENTICATION_METHOD             0x15
#define MQTT_PROP_AUTHENTICATION_DATA               0x16
#define MQTT_PROP_REQUEST_PROBLEM_INFORMATION       0x17
#define MQTT_PROP_WILL_DELAY_INTERVAL               0x18
#define MQTT_PROP_REQUEST_RESPONSE_INFORMATION      0x19
#define MQTT_PROP_RESPONSE_INFORMATION              0x1A
#define MQTT_PROP_SERVER_REFERENCE                  0x1C
#define MQTT_PROP_REASON_STRING                     0x1F
#define MQTT_PROP_RECEIVE_MAXIMUM                   0x21
#define MQTT_PROP_TOPIC_ALIAS_MAXIMUM               0x22
#define MQTT_PROP_TOPIC_ALIAS                       0x23
#define MQTT_PROP_MAXIMUM_QOS                       0x24
#define MQTT_PROP_RETAIN_AVAILABLE                  0x25
#define MQTT_PROP_USER_PROPERTY                     0x26
#define MQTT_PROP_MAXIMUM_PACKET_SIZE               0x27
#define MQTT_PROP_WILDCARD_SUBSCRIPTION_AVAILABLE   0x28
#define MQTT_PROP_SUBSCRIPTION_IDENTIFIER_AVAILABLE 0x29
#define MQTT_PROP_SHARED_SUBSCRIPTION_AVAILABLE     0x2A

/**@brief Masks for MQTT header flags. */
#define MQTT_HEADER_DUP_MASK     0x08
#define MQTT_HEADER_QOS_MASK     0x06
#define MQTT_HEADER_RETAIN_MASK  0x01

/**@brief Masks for MQTT header flags. */
#define MQTT_CONNECT_FLAG_CLEAN_SESSION   0x02
#define MQTT_CONNECT_FLAG_WILL_TOPIC      0x04
#define MQTT_CONNECT_FLAG_WILL_RETAIN     0x20
#define MQTT_CONNECT_FLAG_PASSWORD        0x40
#define MQTT_CONNECT_FLAG_USERNAME        0x80

#define MQTT_CONNACK_FLAG_SESSION_PRESENT 0x01

/**@brief Maximum payload size of MQTT packet. */
#define MQTT_MAX_PAYLOAD_SIZE 0x0FFFFFFF

/**@brief Computes total size needed to pack a UTF8 string. */
#define GET_UT8STR_BUFFER_SIZE(STR) (sizeof(uint16_t) + (STR)->size)

/**@brief Computes total size needed to pack a binary stream. */
#define GET_BINSTR_BUFFER_SIZE(STR) (sizeof(uint16_t) + (STR)->len)

/**@brief Sets MQTT Client's state with one indicated in 'STATE'. */
#define MQTT_SET_STATE(CLIENT, STATE) ((CLIENT)->internal.state |= (STATE))

/**@brief Sets MQTT Client's state exclusive to 'STATE'. */
#define MQTT_SET_STATE_EXCLUSIVE(CLIENT, STATE) \
					((CLIENT)->internal.state = (STATE))

/**@brief Verifies if MQTT Client's state is set with one indicated in 'STATE'.
 */
#define MQTT_HAS_STATE(CLIENT, STATE) ((CLIENT)->internal.state & (STATE))

/**@brief Reset 'STATE' in MQTT Client's state. */
#define MQTT_RESET_STATE(CLIENT, STATE) ((CLIENT)->internal.state &= ~(STATE))

/**@brief Initialize MQTT Client's state. */
#define MQTT_STATE_INIT(CLIENT) ((CLIENT)->internal.state = MQTT_STATE_IDLE)

/**@brief Computes the first byte of MQTT message header based on message type,
 *        duplication flag, QoS and  the retain flag.
 */
#define MQTT_MESSAGES_OPTIONS(TYPE, DUP, QOS, RETAIN) \
	(((TYPE)      & 0xF0)  | \
	 (((DUP) << 3) & 0x08) | \
	 (((QOS) << 1) & 0x06) | \
	 ((RETAIN) & 0x01))

#define MQTT_MAX_LENGTH_BYTES 4
#define MQTT_LENGTH_VALUE_MASK 0x7F
#define MQTT_LENGTH_CONTINUATION_BIT 0x80
#define MQTT_LENGTH_SHIFT 7

/**@brief Check if the input pointer is NULL, if so it returns -EINVAL. */
#define NULL_PARAM_CHECK(param) \
	do { \
		if ((param) == NULL) { \
			return -EINVAL; \
		} \
	} while (false)

#define NULL_PARAM_CHECK_VOID(param) \
	do { \
		if ((param) == NULL) { \
			return; \
		} \
	} while (false)

/** Buffer context to iterate over buffer. */
struct buf_ctx {
	uint8_t *cur;
	uint8_t *end;
};

/**@brief MQTT States. */
enum mqtt_state {
	/** Idle state, implying the client entry in the table is unused/free.
	 */
	MQTT_STATE_IDLE                 = 0x00000000,

	/** TCP Connection has been requested, awaiting result of the request.
	 */
	MQTT_STATE_TCP_CONNECTING       = 0x00000001,

	/** TCP Connection successfully established. */
	MQTT_STATE_TCP_CONNECTED        = 0x00000002,

	/** MQTT Connection successful. */
	MQTT_STATE_CONNECTED            = 0x00000004,
};

static inline int mqtt_is_version_5_0(const struct mqtt_client *client)
{
	return (IS_ENABLED(CONFIG_MQTT_VERSION_5_0) &&
		client->protocol_version == MQTT_VERSION_5_0);
}

/**@brief Notify application about MQTT event.
 *
 * @param[in] client Identifies the client for which event occurred.
 * @param[in] evt MQTT event.
 */
void event_notify(struct mqtt_client *client, const struct mqtt_evt *evt);

/**@brief Handles MQTT messages received from the peer.
 *
 * @param[in] client Identifies the client for which the data was received.

 * @return 0 if the procedure is successful, an error code otherwise.
 */
int mqtt_handle_rx(struct mqtt_client *client);

/**@brief Constructs/encodes Connect packet.
 *
 * @param[in] client Identifies the client for which the procedure is requested.
 *                   All information required for creating the packet like
 *                   client id, clean session flag, retain session flag etc are
 *                   assumed to be populated for the client instance when this
 *                   procedure is requested.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int connect_request_encode(const struct mqtt_client *client,
			   struct buf_ctx *buf);

/**@brief Constructs/encodes Publish packet.
 *
 * @param[in] param Publish message parameters.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_encode(const struct mqtt_publish_param *param, struct buf_ctx *buf);

/**@brief Constructs/encodes Publish Ack packet.
 *
 * @param[in] param Publish Ack message parameters.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_ack_encode(const struct mqtt_puback_param *param,
		       struct buf_ctx *buf);

/**@brief Constructs/encodes Publish Receive packet.
 *
 * @param[in] param Publish Receive message parameters.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_receive_encode(const struct mqtt_pubrec_param *param,
			   struct buf_ctx *buf);

/**@brief Constructs/encodes Publish Release packet.
 *
 * @param[in] param Publish Release message parameters.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_release_encode(const struct mqtt_pubrel_param *param,
			   struct buf_ctx *buf);

/**@brief Constructs/encodes Publish Complete packet.
 *
 * @param[in] param Publish Complete message parameters.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_complete_encode(const struct mqtt_pubcomp_param *param,
			    struct buf_ctx *buf);

/**@brief Constructs/encodes Disconnect packet.
 *
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int disconnect_encode(struct buf_ctx *buf);

/**@brief Constructs/encodes Subscribe packet.
 *
 * @param[in] param Subscribe message parameters.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int subscribe_encode(const struct mqtt_subscription_list *param,
		     struct buf_ctx *buf);

/**@brief Constructs/encodes Unsubscribe packet.
 *
 * @param[in] param Unsubscribe message parameters.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int unsubscribe_encode(const struct mqtt_subscription_list *param,
		       struct buf_ctx *buf);

/**@brief Constructs/encodes Ping Request packet.
 *
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *                       As output points to the beginning and end of
 *                       the frame.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int ping_request_encode(struct buf_ctx *buf);

/**@brief Decode MQTT Packet Type and Length in the MQTT fixed header.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] type_and_flags Message type and flags.
 * @param[out] length Length of variable header and payload in the MQTT message.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int fixed_header_decode(struct buf_ctx *buf, uint8_t *type_and_flags,
			uint32_t *length);

/**@brief Decode MQTT Connect Ack packet.
 *
 * @param[in] client MQTT client for which packet is decoded.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] param Pointer to buffer for decoded Connect Ack parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int connect_ack_decode(const struct mqtt_client *client, struct buf_ctx *buf,
		       struct mqtt_connack_param *param);

/**@brief Decode MQTT Publish packet.
 *
 * @param[in] flags Byte containing message type and flags.
 * @param[in] var_length Length of the variable part of the message.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] param Pointer to buffer for decoded Publish parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_decode(uint8_t flags, uint32_t var_length, struct buf_ctx *buf,
		   struct mqtt_publish_param *param);

/**@brief Decode MQTT Publish Ack packet.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] param Pointer to buffer for decoded Publish Ack parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_ack_decode(struct buf_ctx *buf, struct mqtt_puback_param *param);

/**@brief Decode MQTT Publish Receive packet.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] param Pointer to buffer for decoded Publish Receive parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_receive_decode(struct buf_ctx *buf,
			   struct mqtt_pubrec_param *param);

/**@brief Decode MQTT Publish Release packet.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] param Pointer to buffer for decoded Publish Release parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_release_decode(struct buf_ctx *buf,
			   struct mqtt_pubrel_param *param);

/**@brief Decode MQTT Publish Complete packet.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] param Pointer to buffer for decoded Publish Complete parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int publish_complete_decode(struct buf_ctx *buf,
			    struct mqtt_pubcomp_param *param);

/**@brief Decode MQTT Subscribe packet.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] param Pointer to buffer for decoded Subscribe parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int subscribe_ack_decode(struct buf_ctx *buf,
			 struct mqtt_suback_param *param);

/**@brief Decode MQTT Unsubscribe packet.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] param Pointer to buffer for decoded Unsubscribe parameters.
 *
 * @return 0 if the procedure is successful, an error code otherwise.
 */
int unsubscribe_ack_decode(struct buf_ctx *buf,
			   struct mqtt_unsuback_param *param);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_INTERNAL_H_ */
