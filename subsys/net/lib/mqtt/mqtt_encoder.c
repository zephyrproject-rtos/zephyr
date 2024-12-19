/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_encoder.c
 *
 * @brief Encoding functions needed to create packet to be sent to the broker.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_enc, CONFIG_MQTT_LOG_LEVEL);

#include "mqtt_internal.h"
#include "mqtt_os.h"

static const struct mqtt_utf8 mqtt_3_1_0_proto_desc =
	MQTT_UTF8_LITERAL("MQIsdp");

static const struct mqtt_utf8 mqtt_proto_desc =
	MQTT_UTF8_LITERAL("MQTT");

/** Never changing ping request, needed for Keep Alive. */
static const uint8_t ping_packet[MQTT_FIXED_HEADER_MIN_SIZE] = {
	MQTT_PKT_TYPE_PINGREQ,
	0x00
};

/** Never changing disconnect request. */
static const uint8_t empty_disc_packet[MQTT_FIXED_HEADER_MIN_SIZE] = {
	MQTT_PKT_TYPE_DISCONNECT,
	0x00
};

/**
 * @brief Packs unsigned 8 bit value to the buffer at the offset requested.
 *
 * @param[in] val Value to be packed.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the value.
 */
static int pack_uint8(uint8_t val, struct buf_ctx *buf)
{
	uint8_t *cur = buf->cur;
	uint8_t *end = buf->end;

	if ((end - cur) < sizeof(uint8_t)) {
		return -ENOMEM;
	}

	NET_DBG(">> val:%02x cur:%p, end:%p", val, (void *)cur, (void *)end);

	/* Pack value. */
	cur[0] = val;
	buf->cur = (cur + sizeof(uint8_t));

	return 0;
}

/**
 * @brief Packs unsigned 16 bit value to the buffer at the offset requested.
 *
 * @param[in] val Value to be packed.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if the procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the value.
 */
static int pack_uint16(uint16_t val, struct buf_ctx *buf)
{
	uint8_t *cur = buf->cur;
	uint8_t *end = buf->end;

	if ((end - cur) < sizeof(uint16_t)) {
		return -ENOMEM;
	}

	NET_DBG(">> val:%04x cur:%p, end:%p", val, (void *)cur, (void *)end);

	/* Pack value. */
	sys_put_be16(val, cur);
	buf->cur = (cur + sizeof(uint16_t));

	return 0;
}

/**
 * @brief Packs utf8 string to the buffer at the offset requested.
 *
 * @param[in] str UTF-8 string and its length to be packed.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if the procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the string.
 */
static int pack_utf8_str(const struct mqtt_utf8 *str, struct buf_ctx *buf)
{
	if ((buf->end - buf->cur) < GET_UT8STR_BUFFER_SIZE(str)) {
		return -ENOMEM;
	}

	NET_DBG(">> str_size:%08x cur:%p, end:%p",
		 (uint32_t)GET_UT8STR_BUFFER_SIZE(str), (void *)buf->cur, (void *)buf->end);

	/* Pack length followed by string. */
	(void)pack_uint16(str->size, buf);

	memcpy(buf->cur, str->utf8, str->size);
	buf->cur += str->size;

	return 0;
}

/**
 * @brief Computes and encodes variable length integer.
 *
 * @note The variable length integer is based on algorithm below:
 *
 * @code
 * do
 *            encodedByte = X MOD 128
 *            X = X DIV 128
 *            // if there are more data to encode, set the top bit of this byte
 *            if ( X > 0 )
 *                encodedByte = encodedByte OR 128
 *            endif
 *                'output' encodedByte
 *       while ( X > 0 )
 * @endcode
 *
 * @param[in] value Integer value to encode.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position. May be NULL (in this case function will
 *                   only calculate number of bytes needed).
 *
 * @return Number of bytes needed to encode integer or a negative error code.
 */
static int pack_variable_int(uint32_t value, struct buf_ctx *buf)
{
	int encoded_bytes = 0U;

	NET_DBG(">> value:0x%08x cur:%p, end:%p", value,
		 (buf == NULL) ? 0 : (void *)buf->cur, (buf == NULL) ? 0 : (void *)buf->end);

	do {
		encoded_bytes++;

		if (buf != NULL) {
			if (buf->cur >= buf->end) {
				return -ENOMEM;
			}

			*(buf->cur) = value & MQTT_LENGTH_VALUE_MASK;
		}

		value >>= MQTT_LENGTH_SHIFT;

		if (buf != NULL) {
			if (value > 0) {
				*(buf->cur) |= MQTT_LENGTH_CONTINUATION_BIT;
			}
			buf->cur++;
		}
	} while (value > 0);

	return encoded_bytes;
}

/**
 * @brief Encodes fixed header for the MQTT message and provides pointer to
 *        start of the header.
 *
 * @param[in] message_type Message type containing packet type and the flags.
 *                         Use @ref MQTT_MESSAGES_OPTIONS to construct the
 *                         message_type.
 * @param[in] start  Pointer to the start of the variable header.
 * @param[inout] buf Buffer context used to encode the frame.
 *                   The 5 bytes before the start of the message are assumed
 *                   by the routine to be available to pack the fixed header.
 *                   However, since the fixed header length is variable
 *                   length, the pointer to the start of the MQTT message
 *                   along with encoded fixed header is supplied as output
 *                   parameter if the procedure was successful.
 *                   As output, the pointers will point to beginning and the end
 *                   of the frame.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EMSGSIZE if the message is too big for MQTT.
 */
static uint32_t mqtt_encode_fixed_header(uint8_t message_type, uint8_t *start,
				      struct buf_ctx *buf)
{
	uint32_t length = buf->cur - start;
	uint8_t fixed_header_length;

	if (length > MQTT_MAX_PAYLOAD_SIZE) {
		return -EMSGSIZE;
	}

	NET_DBG("<< msg type:0x%02x length:0x%08x", message_type, length);

	fixed_header_length = pack_variable_int(length, NULL);
	fixed_header_length += sizeof(uint8_t);

	NET_DBG("Fixed header length = %02x", fixed_header_length);

	/* Set the pointer at the start of the frame before encoding. */
	buf->cur = start - fixed_header_length;

	(void)pack_uint8(message_type, buf);
	(void)pack_variable_int(length, buf);

	/* Set the cur pointer back at the start of the frame,
	 * and end pointer to the end of the frame.
	 */
	buf->cur = buf->cur - fixed_header_length;
	buf->end = buf->cur + length + fixed_header_length;

	return 0;
}

/**
 * @brief Encodes a string of a zero length.
 *
 * @param[in] buffer_len Total size of the buffer on which string will be
 *                       encoded. This shall not be zero.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if the procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the binary
 *                 string.
 */
static int zero_len_str_encode(struct buf_ctx *buf)
{
	return pack_uint16(0x0000, buf);
}

#if defined(CONFIG_MQTT_VERSION_5_0)
/**
 * @brief Packs unsigned 32 bit value to the buffer at the offset requested.
 *
 * @param[in] val Value to be packed.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if the procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the value.
 */
static int pack_uint32(uint32_t val, struct buf_ctx *buf)
{
	uint8_t *cur = buf->cur;
	uint8_t *end = buf->end;

	if ((end - cur) < sizeof(uint32_t)) {
		return -ENOMEM;
	}

	/* Pack value. */
	sys_put_be32(val, cur);
	buf->cur = (cur + sizeof(uint32_t));

	return 0;
}

/**
 * @brief Packs binary data to the buffer at the offset requested.
 *
 * @param[in] bin Binary data and its length to be packed.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if the procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the data.
 */
static int pack_bin_data(const struct mqtt_binstr *bin, struct buf_ctx *buf)
{
	if ((buf->end - buf->cur) < GET_BINSTR_BUFFER_SIZE(bin)) {
		return -ENOMEM;
	}

	/* Pack length followed by binary data. */
	(void)pack_uint16(bin->len, buf);

	memcpy(buf->cur, bin->data, bin->len);
	buf->cur += bin->len;

	return 0;
}

static uint8_t get_uint8_property_default(uint8_t prop)
{
	if (prop == MQTT_PROP_REQUEST_PROBLEM_INFORMATION) {
		return 1;
	}

	return 0;
}

static size_t uint8_property_length(uint8_t prop, uint8_t value)
{
	uint8_t prop_default = get_uint8_property_default(prop);

	if (value == prop_default) {
		return 0;
	}

	return sizeof(uint8_t) + sizeof(uint8_t);
}

static int encode_uint8_property(uint8_t prop, uint8_t value, struct buf_ctx *buf)
{
	uint8_t prop_default = get_uint8_property_default(prop);
	int err;

	if (value == prop_default) {
		return 0;
	}

	err = pack_uint8(prop, buf);
	if (err < 0) {
		return err;
	}

	return pack_uint8(value, buf);
}

static size_t uint16_property_length(uint16_t value)
{
	if (value == 0) {
		return 0;
	}

	return sizeof(uint8_t) + sizeof(uint16_t);
}

static int encode_uint16_property(uint8_t prop, uint16_t value,
				  struct buf_ctx *buf)
{
	int err;

	if (value == 0) {
		return 0;
	}

	err = pack_uint8(prop, buf);
	if (err < 0) {
		return err;
	}

	return pack_uint16(value, buf);
}

static size_t uint32_property_length(uint32_t value)
{
	if (value == 0) {
		return 0;
	}

	return sizeof(uint8_t) + sizeof(uint32_t);
}

static int encode_uint32_property(uint8_t prop, uint32_t value,
				  struct buf_ctx *buf)
{
	int err;

	if (value == 0) {
		return 0;
	}

	err = pack_uint8(prop, buf);
	if (err < 0) {
		return err;
	}

	return pack_uint32(value, buf);
}

static size_t var_int_property_length(uint32_t value)
{
	if (value == 0) {
		return 0;
	}

	return sizeof(uint8_t) + (size_t)pack_variable_int(value, NULL);
}

static int encode_var_int_property(uint8_t prop, uint32_t value,
				   struct buf_ctx *buf)
{
	int err;

	if (value == 0) {
		return 0;
	}

	err = pack_uint8(prop, buf);
	if (err < 0) {
		return err;
	}

	return pack_variable_int(value, buf);
}

static size_t string_property_length(const struct mqtt_utf8 *str)
{
	if (str->size == 0) {
		return 0;
	}

	return sizeof(uint8_t) + GET_UT8STR_BUFFER_SIZE(str);
}

static int encode_string_property(uint8_t prop, const struct mqtt_utf8 *str,
				  struct buf_ctx *buf)
{
	int err;

	if (str->size == 0) {
		return 0;
	}

	err = pack_uint8(prop, buf);
	if (err < 0) {
		return err;
	}

	return pack_utf8_str(str, buf);
}

static size_t string_pair_property_length(const struct mqtt_utf8 *name,
					  const struct mqtt_utf8 *value)
{
	if ((name->size == 0) || (value->size == 0)) {
		return 0;
	}

	return sizeof(uint8_t) + GET_UT8STR_BUFFER_SIZE(name) +
	       GET_UT8STR_BUFFER_SIZE(value);
}

static int encode_string_pair_property(uint8_t prop, const struct mqtt_utf8 *name,
				       const struct mqtt_utf8 *value,
				       struct buf_ctx *buf)
{
	int err;

	if ((name->size == 0) || (value->size == 0)) {
		return 0;
	}

	err = pack_uint8(prop, buf);
	if (err < 0) {
		return err;
	}

	err = pack_utf8_str(name, buf);
	if (err < 0) {
		return err;
	}

	return pack_utf8_str(value, buf);
}

static size_t binary_property_length(const struct mqtt_binstr *bin)
{
	if (bin->len == 0) {
		return 0;
	}

	return sizeof(uint8_t) + GET_BINSTR_BUFFER_SIZE(bin);
}

static int encode_binary_property(uint8_t prop, const struct mqtt_binstr *bin,
				  struct buf_ctx *buf)
{
	int err;

	if (bin->len == 0) {
		return 0;
	}

	err = pack_uint8(prop, buf);
	if (err < 0) {
		return err;
	}

	return pack_bin_data(bin, buf);
}

static size_t user_properties_length(const struct mqtt_utf8_pair *user_props)
{
	size_t total_len = 0;
	size_t len;

	for (int i = 0; i < CONFIG_MQTT_USER_PROPERTIES_MAX; i++) {
		len = string_pair_property_length(&user_props[i].name,
						  &user_props[i].value);
		if (len == 0) {
			break;
		}

		total_len += len;
	}

	return total_len;
}

static int encode_user_properties(const struct mqtt_utf8_pair *user_props,
				  struct buf_ctx *buf)
{
	size_t len;
	int err;

	for (int i = 0; i < CONFIG_MQTT_USER_PROPERTIES_MAX; i++) {
		len = string_pair_property_length(&user_props[i].name,
						  &user_props[i].value);

		if (len == 0) {
			break;
		}

		err = encode_string_pair_property(MQTT_PROP_USER_PROPERTY,
						  &user_props[i].name,
						  &user_props[i].value, buf);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static uint32_t connect_properties_length(const struct mqtt_client *client)
{
	return uint32_property_length(client->prop.session_expiry_interval) +
	       uint16_property_length(client->prop.receive_maximum) +
	       uint32_property_length(client->prop.maximum_packet_size) +
	       uint16_property_length(CONFIG_MQTT_TOPIC_ALIAS_MAX) +
	       uint8_property_length(MQTT_PROP_REQUEST_RESPONSE_INFORMATION,
				     client->prop.request_response_info ? 1U : 0U) +
	       uint8_property_length(MQTT_PROP_REQUEST_PROBLEM_INFORMATION,
				     client->prop.request_problem_info ? 1U : 0U) +
	       user_properties_length(client->prop.user_prop) +
	       string_property_length(&client->prop.auth_method) +
	       binary_property_length(&client->prop.auth_data);
}

static int connect_properties_encode(const struct mqtt_client *client,
				     struct buf_ctx *buf)
{
	uint32_t properties_len;
	int err;

	/* Precalculate total properties length */
	properties_len = connect_properties_length(client);
	err = pack_variable_int(properties_len, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint32_property(MQTT_PROP_SESSION_EXPIRY_INTERVAL,
				     client->prop.session_expiry_interval, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint16_property(MQTT_PROP_RECEIVE_MAXIMUM,
				     client->prop.receive_maximum, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint32_property(MQTT_PROP_MAXIMUM_PACKET_SIZE,
				     client->prop.maximum_packet_size, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint16_property(MQTT_PROP_TOPIC_ALIAS_MAXIMUM,
				     CONFIG_MQTT_TOPIC_ALIAS_MAX, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint8_property(MQTT_PROP_REQUEST_RESPONSE_INFORMATION,
				    client->prop.request_response_info ? 1U : 0U,
				    buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint8_property(MQTT_PROP_REQUEST_PROBLEM_INFORMATION,
				    client->prop.request_problem_info ? 1U : 0U,
				    buf);
	if (err < 0) {
		return err;
	}

	err = encode_user_properties(client->prop.user_prop, buf);
	if (err < 0) {
		return err;
	}

	err = encode_string_property(MQTT_PROP_AUTHENTICATION_METHOD,
				     &client->prop.auth_method, buf);
	if (err < 0) {
		return err;
	}

	err = encode_binary_property(MQTT_PROP_AUTHENTICATION_DATA,
				     &client->prop.auth_data, buf);
	if (err < 0) {
		return err;
	}

	return 0;
}

static uint32_t will_properties_length(const struct mqtt_client *client)
{
	return uint32_property_length(client->will_prop.will_delay_interval) +
	       uint8_property_length(MQTT_PROP_PAYLOAD_FORMAT_INDICATOR,
				     client->will_prop.payload_format_indicator) +
	       uint32_property_length(client->will_prop.message_expiry_interval) +
	       string_property_length(&client->will_prop.content_type) +
	       string_property_length(&client->will_prop.response_topic) +
	       binary_property_length(&client->will_prop.correlation_data) +
	       user_properties_length(client->will_prop.user_prop);
}

static int will_properties_encode(const struct mqtt_client *client,
				  struct buf_ctx *buf)
{
	uint32_t properties_len;
	int err;

	/* Precalculate total properties length */
	properties_len = will_properties_length(client);
	err = pack_variable_int(properties_len, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint32_property(MQTT_PROP_WILL_DELAY_INTERVAL,
				     client->will_prop.will_delay_interval, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint8_property(MQTT_PROP_PAYLOAD_FORMAT_INDICATOR,
				    client->will_prop.payload_format_indicator,
				    buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint32_property(MQTT_PROP_MESSAGE_EXPIRY_INTERVAL,
				     client->will_prop.message_expiry_interval,
				     buf);
	if (err < 0) {
		return err;
	}

	err = encode_string_property(MQTT_PROP_CONTENT_TYPE,
				     &client->will_prop.content_type, buf);
	if (err < 0) {
		return err;
	}

	err = encode_string_property(MQTT_PROP_RESPONSE_TOPIC,
				     &client->will_prop.response_topic, buf);
	if (err < 0) {
		return err;
	}

	err = encode_binary_property(MQTT_PROP_CORRELATION_DATA,
				     &client->will_prop.correlation_data, buf);
	if (err < 0) {
		return err;
	}

	err = encode_user_properties(client->will_prop.user_prop, buf);
	if (err < 0) {
		return err;
	}

	return 0;
}

#else
static int connect_properties_encode(const struct mqtt_client *client,
				     struct buf_ctx *buf)
{
	ARG_UNUSED(client);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}

static int will_properties_encode(const struct mqtt_client *client,
				  struct buf_ctx *buf)
{
	ARG_UNUSED(client);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int connect_request_encode(const struct mqtt_client *client,
			   struct buf_ctx *buf)
{
	uint8_t connect_flags = client->clean_session << 1;
	const uint8_t message_type =
			MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_CONNECT, 0, 0, 0);
	const struct mqtt_utf8 *proto_desc;
	uint8_t *connect_flags_pos;
	int err_code;
	uint8_t *start;

	if (client->protocol_version == MQTT_VERSION_3_1_0) {
		proto_desc = &mqtt_3_1_0_proto_desc;
	} else {
		/* MQTT 3.1.1 and newer share the same protocol prefix. */
		proto_desc = &mqtt_proto_desc;
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	NET_HEXDUMP_DBG(proto_desc->utf8, proto_desc->size,
			 "Encoding Protocol Description.");

	err_code = pack_utf8_str(proto_desc, buf);
	if (err_code != 0) {
		return err_code;
	}

	NET_DBG("Encoding Protocol Version %02x.", client->protocol_version);
	err_code = pack_uint8(client->protocol_version, buf);
	if (err_code != 0) {
		return err_code;
	}

	/* Remember position of connect flag and leave one byte for it to
	 * be packed once we determine its value.
	 */
	connect_flags_pos = buf->cur;

	err_code = pack_uint8(0, buf);
	if (err_code != 0) {
		return err_code;
	}

	NET_DBG("Encoding Keep Alive Time %04x.", client->keepalive);
	err_code = pack_uint16(client->keepalive, buf);
	if (err_code != 0) {
		return err_code;
	}

	/* Properties (MQTT 5.0 only) */
	if (mqtt_is_version_5_0(client)) {
		err_code = connect_properties_encode(client, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	NET_HEXDUMP_DBG(client->client_id.utf8, client->client_id.size,
			 "Encoding Client Id.");
	err_code = pack_utf8_str(&client->client_id, buf);
	if (err_code != 0) {
		return err_code;
	}

	/* Pack will topic and QoS */
	if (client->will_topic != NULL) {
		connect_flags |= MQTT_CONNECT_FLAG_WILL_TOPIC;
		/* QoS is always 1 as of now. */
		connect_flags |= ((client->will_topic->qos & 0x03) << 3);
		connect_flags |= client->will_retain << 5;

		/* Will properties (MQTT 5.0 only) */
		if (mqtt_is_version_5_0(client)) {
			err_code = will_properties_encode(client, buf);
			if (err_code != 0) {
				return err_code;
			}
		}

		NET_HEXDUMP_DBG(client->will_topic->topic.utf8,
				 client->will_topic->topic.size,
				 "Encoding Will Topic.");
		err_code = pack_utf8_str(&client->will_topic->topic, buf);
		if (err_code != 0) {
			return err_code;
		}

		if (client->will_message != NULL) {
			NET_HEXDUMP_DBG(client->will_message->utf8,
					 client->will_message->size,
					 "Encoding Will Message.");
			err_code = pack_utf8_str(client->will_message, buf);
			if (err_code != 0) {
				return err_code;
			}
		} else {
			NET_DBG("Encoding Zero Length Will Message.");
			err_code = zero_len_str_encode(buf);
			if (err_code != 0) {
				return err_code;
			}
		}
	}

	/* Pack Username if any. */
	if (client->user_name != NULL) {
		connect_flags |= MQTT_CONNECT_FLAG_USERNAME;

		NET_HEXDUMP_DBG(client->user_name->utf8,
				 client->user_name->size,
				 "Encoding Username.");
		err_code = pack_utf8_str(client->user_name, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	/* Pack Password if any. */
	if (client->password != NULL) {
		connect_flags |= MQTT_CONNECT_FLAG_PASSWORD;

		NET_HEXDUMP_DBG(client->password->utf8,
				 client->password->size,
				 "Encoding Password.");
		err_code = pack_utf8_str(client->password, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	/* Write the flags the connect flags. */
	*connect_flags_pos = connect_flags;

	return mqtt_encode_fixed_header(message_type, start, buf);
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static uint32_t publish_properties_length(const struct mqtt_publish_param *param)
{
	return uint8_property_length(MQTT_PROP_PAYLOAD_FORMAT_INDICATOR,
				     param->prop.payload_format_indicator) +
	       uint32_property_length(param->prop.message_expiry_interval) +
	       uint16_property_length(param->prop.topic_alias) +
	       string_property_length(&param->prop.response_topic) +
	       binary_property_length(&param->prop.correlation_data) +
	       user_properties_length(param->prop.user_prop) +
	       /* Client does not include Subscription Identifier in any case. */
	       string_property_length(&param->prop.content_type);
}

static int publish_properties_encode(const struct mqtt_publish_param *param,
				     struct buf_ctx *buf)
{
	uint32_t properties_len;
	int err;

	/* Precalculate total properties length */
	properties_len = publish_properties_length(param);
	err = pack_variable_int(properties_len, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint8_property(MQTT_PROP_PAYLOAD_FORMAT_INDICATOR,
				    param->prop.payload_format_indicator, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint32_property(MQTT_PROP_MESSAGE_EXPIRY_INTERVAL,
				     param->prop.message_expiry_interval, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint16_property(MQTT_PROP_TOPIC_ALIAS,
				     param->prop.topic_alias, buf);
	if (err < 0) {
		return err;
	}

	err = encode_string_property(MQTT_PROP_RESPONSE_TOPIC,
				     &param->prop.response_topic, buf);
	if (err < 0) {
		return err;
	}

	err = encode_binary_property(MQTT_PROP_CORRELATION_DATA,
				     &param->prop.correlation_data, buf);
	if (err < 0) {
		return err;
	}

	err = encode_user_properties(param->prop.user_prop, buf);
	if (err < 0) {
		return err;
	}

	/* Client does not include Subscription Identifier in any case. */

	err = encode_string_property(MQTT_PROP_CONTENT_TYPE,
				     &param->prop.content_type, buf);
	if (err < 0) {
		return err;
	}

	return 0;
}
#else
static int publish_properties_encode(const struct mqtt_publish_param *param,
				     struct buf_ctx *buf)
{
	ARG_UNUSED(param);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int publish_encode(const struct mqtt_client *client,
		   const struct mqtt_publish_param *param,
		   struct buf_ctx *buf)
{
	const uint8_t message_type = MQTT_MESSAGES_OPTIONS(
			MQTT_PKT_TYPE_PUBLISH, param->dup_flag,
			param->message.topic.qos, param->retain_flag);
	int err_code;
	uint8_t *start;

	/* Message id zero is not permitted by spec. */
	if ((param->message.topic.qos) && (param->message_id == 0U)) {
		return -EINVAL;
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err_code = pack_utf8_str(&param->message.topic.topic, buf);
	if (err_code != 0) {
		return err_code;
	}

	if (param->message.topic.qos) {
		err_code = pack_uint16(param->message_id, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	if (mqtt_is_version_5_0(client)) {
		err_code = publish_properties_encode(param, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	/* Do not copy payload. We move the buffer pointer to ensure that
	 * message length in fixed header is encoded correctly.
	 */
	buf->cur += param->message.payload.len;

	err_code = mqtt_encode_fixed_header(message_type, start, buf);
	if (err_code != 0) {
		return err_code;
	}

	buf->end -= param->message.payload.len;

	return 0;
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static uint32_t common_ack_properties_length(
	const struct mqtt_common_ack_properties *prop)
{
	return user_properties_length(prop->user_prop) +
	       string_property_length(&prop->reason_string);
}

static int common_ack_properties_encode(
	const struct mqtt_common_ack_properties *prop,
	struct buf_ctx *buf)
{
	uint32_t properties_len;
	int err;

	/* Precalculate total properties length */
	properties_len = common_ack_properties_length(prop);
	/* Properties length can be omitted if equal to 0. */
	if (properties_len == 0) {
		return 0;
	}

	err = pack_variable_int(properties_len, buf);
	if (err < 0) {
		return err;
	}

	err = encode_user_properties(prop->user_prop, buf);
	if (err < 0) {
		return err;
	}

	err = encode_string_property(MQTT_PROP_REASON_STRING,
				     &prop->reason_string, buf);
	if (err < 0) {
		return err;
	}

	return 0;
}
#else /* CONFIG_MQTT_VERSION_5_0 */
static uint32_t common_ack_properties_length(
	const struct mqtt_common_ack_properties *prop)
{
	return 0;
}

static int common_ack_properties_encode(
	const struct mqtt_common_ack_properties *prop,
	struct buf_ctx *buf)
{
	ARG_UNUSED(prop);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

static int common_ack_encode(
	uint8_t message_type, uint16_t message_id,  uint8_t reason_code,
	const struct mqtt_common_ack_properties *prop,
	struct buf_ctx *buf)
{
	int err_code;
	uint8_t *start;

	/* Message id zero is not permitted by spec. */
	if (message_id == 0U) {
		return -EINVAL;
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err_code = pack_uint16(message_id, buf);
	if (err_code != 0) {
		return err_code;
	}

	/* For MQTT < 5.0 properties are NULL. */
	if (prop != NULL) {
		/* The Reason Code and Property Length can be omitted if the
		 * Reason Code is 0x00 (Success) and there are no Properties.
		 */
		if (common_ack_properties_length(prop) == 0 &&
		    reason_code == 0) {
			goto out;
		}

		err_code = pack_uint8(reason_code, buf);
		if (err_code != 0) {
			return err_code;
		}

		err_code = common_ack_properties_encode(prop, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

out:
	return mqtt_encode_fixed_header(message_type, start, buf);
}

int publish_ack_encode(const struct mqtt_client *client,
		       const struct mqtt_puback_param *param,
		       struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBACK, 0, 0, 0);
	const struct mqtt_common_ack_properties *prop = NULL;
	uint8_t reason_code = 0;

#if defined(CONFIG_MQTT_VERSION_5_0)
	if (mqtt_is_version_5_0(client)) {
		prop = &param->prop;
		reason_code = param->reason_code;
	}
#endif

	return common_ack_encode(message_type, param->message_id,
				 reason_code, prop, buf);
}

int publish_receive_encode(const struct mqtt_client *client,
			   const struct mqtt_pubrec_param *param,
			   struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBREC, 0, 0, 0);
	const struct mqtt_common_ack_properties *prop = NULL;
	uint8_t reason_code = 0;

#if defined(CONFIG_MQTT_VERSION_5_0)
	if (mqtt_is_version_5_0(client)) {
		prop = &param->prop;
		reason_code = param->reason_code;
	}
#endif

	return common_ack_encode(message_type, param->message_id,
				 reason_code, prop, buf);
}

int publish_release_encode(const struct mqtt_client *client,
			   const struct mqtt_pubrel_param *param,
			   struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBREL, 0, 1, 0);
	const struct mqtt_common_ack_properties *prop = NULL;
	uint8_t reason_code = 0;

#if defined(CONFIG_MQTT_VERSION_5_0)
	if (mqtt_is_version_5_0(client)) {
		prop = &param->prop;
		reason_code = param->reason_code;
	}
#endif

	return common_ack_encode(message_type, param->message_id,
				 reason_code, prop, buf);
}

int publish_complete_encode(const struct mqtt_client *client,
			    const struct mqtt_pubcomp_param *param,
			    struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBCOMP, 0, 0, 0);
	const struct mqtt_common_ack_properties *prop = NULL;
	uint8_t reason_code = 0;

#if defined(CONFIG_MQTT_VERSION_5_0)
	if (mqtt_is_version_5_0(client)) {
		prop = &param->prop;
		reason_code = param->reason_code;
	}
#endif

	return common_ack_encode(message_type, param->message_id,
				 reason_code, prop, buf);
}

static int empty_disconnect_encode(struct buf_ctx *buf)
{
	uint8_t *cur = buf->cur;
	uint8_t *end = buf->end;

	if ((end - cur) < sizeof(empty_disc_packet)) {
		return -ENOMEM;
	}

	memcpy(cur, empty_disc_packet, sizeof(empty_disc_packet));
	buf->end = (cur + sizeof(empty_disc_packet));

	return 0;
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static uint32_t disconnect_properties_length(const struct mqtt_disconnect_param *param)
{
	return uint32_property_length(param->prop.session_expiry_interval) +
	       string_property_length(&param->prop.reason_string) +
	       user_properties_length(param->prop.user_prop) +
	       string_property_length(&param->prop.server_reference);
}

static int disconnect_properties_encode(const struct mqtt_disconnect_param *param,
					struct buf_ctx *buf)
{
	uint32_t properties_len;
	int err;

	/* Precalculate total properties length */
	properties_len = disconnect_properties_length(param);
	/* Disconnect properties length can be omitted if equal to 0. */
	if (properties_len == 0) {
		return 0;
	}

	err = pack_variable_int(properties_len, buf);
	if (err < 0) {
		return err;
	}

	err = encode_uint32_property(MQTT_PROP_SESSION_EXPIRY_INTERVAL,
				     param->prop.session_expiry_interval, buf);
	if (err < 0) {
		return err;
	}

	err = encode_string_property(MQTT_PROP_REASON_STRING,
				     &param->prop.reason_string, buf);
	if (err < 0) {
		return err;
	}

	err = encode_user_properties(param->prop.user_prop, buf);
	if (err < 0) {
		return err;
	}

	err = encode_string_property(MQTT_PROP_SERVER_REFERENCE,
				     &param->prop.server_reference, buf);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int disconnect_5_0_encode(const struct mqtt_disconnect_param *param,
				 struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_DISCONNECT, 0, 0, 0);
	uint8_t *start;
	int err;

	/* The Reason Code and Property Length can be omitted if the Reason Code
	 * is 0x00 (Normal disconnecton) and there are no Properties.
	 */
	if ((param->reason_code == MQTT_DISCONNECT_NORMAL) &&
	    (disconnect_properties_length(param) == 0U)) {
		return empty_disconnect_encode(buf);
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err = pack_uint8(param->reason_code, buf);
	if (err < 0) {
		return err;
	}

	err = disconnect_properties_encode(param, buf);
	if (err != 0) {
		return err;
	}

	err = mqtt_encode_fixed_header(message_type, start, buf);
	if (err != 0) {
		return err;
	}

	return 0;
}
#else
static int disconnect_5_0_encode(const struct mqtt_disconnect_param *param,
				 struct buf_ctx *buf)
{
	ARG_UNUSED(param);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int disconnect_encode(const struct mqtt_client *client,
		      const struct mqtt_disconnect_param *param,
		      struct buf_ctx *buf)
{
	if (!mqtt_is_version_5_0(client) || param == NULL) {
		return empty_disconnect_encode(buf);
	}

	return disconnect_5_0_encode(param, buf);
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static uint32_t subscribe_properties_length(
			const struct mqtt_subscription_list *param)
{
	return var_int_property_length(param->prop.subscription_identifier) +
	       user_properties_length(param->prop.user_prop);
}

static int subscribe_properties_encode(const struct mqtt_subscription_list *param,
				       struct buf_ctx *buf)
{
	uint32_t properties_len;
	int err;

	/* Precalculate total properties length */
	properties_len = subscribe_properties_length(param);
	err = pack_variable_int(properties_len, buf);
	if (err < 0) {
		return err;
	}

	err = encode_var_int_property(MQTT_PROP_SUBSCRIPTION_IDENTIFIER,
				      param->prop.subscription_identifier,
				      buf);
	if (err < 0) {
		return err;
	}

	err = encode_user_properties(param->prop.user_prop, buf);
	if (err < 0) {
		return err;
	}

	return 0;
}
#else
static int subscribe_properties_encode(const struct mqtt_subscription_list *param,
				       struct buf_ctx *buf)
{
	ARG_UNUSED(param);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int subscribe_encode(const struct mqtt_client *client,
		     const struct mqtt_subscription_list *param,
		     struct buf_ctx *buf)
{
	const uint8_t message_type = MQTT_MESSAGES_OPTIONS(
			MQTT_PKT_TYPE_SUBSCRIBE, 0, 1, 0);
	int err_code, i;
	uint8_t *start;

	/* Message id zero is not permitted by spec. */
	if (param->message_id == 0U) {
		return -EINVAL;
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err_code = pack_uint16(param->message_id, buf);
	if (err_code != 0) {
		return err_code;
	}

	if (mqtt_is_version_5_0(client)) {
		err_code = subscribe_properties_encode(param, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	for (i = 0; i < param->list_count; i++) {
		err_code = pack_utf8_str(&param->list[i].topic, buf);
		if (err_code != 0) {
			return err_code;
		}

		err_code = pack_uint8(param->list[i].qos, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	return mqtt_encode_fixed_header(message_type, start, buf);
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static uint32_t unsubscribe_properties_length(
			const struct mqtt_subscription_list *param)
{
	return user_properties_length(param->prop.user_prop);
}

static int unsubscribe_properties_encode(
		const struct mqtt_subscription_list *param, struct buf_ctx *buf)
{
	uint32_t properties_len;
	int err;

	/* Precalculate total properties length */
	properties_len = unsubscribe_properties_length(param);
	err = pack_variable_int(properties_len, buf);
	if (err < 0) {
		return err;
	}

	err = encode_user_properties(param->prop.user_prop, buf);
	if (err < 0) {
		return err;
	}

	return 0;
}
#else
static int unsubscribe_properties_encode(
		const struct mqtt_subscription_list *param, struct buf_ctx *buf)
{
	ARG_UNUSED(param);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int unsubscribe_encode(const struct mqtt_client *client,
		       const struct mqtt_subscription_list *param,
		       struct buf_ctx *buf)
{
	const uint8_t message_type = MQTT_MESSAGES_OPTIONS(
		MQTT_PKT_TYPE_UNSUBSCRIBE, 0, MQTT_QOS_1_AT_LEAST_ONCE, 0);
	int err_code, i;
	uint8_t *start;

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err_code = pack_uint16(param->message_id, buf);
	if (err_code != 0) {
		return err_code;
	}

	if (mqtt_is_version_5_0(client)) {
		err_code = unsubscribe_properties_encode(param, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	for (i = 0; i < param->list_count; i++) {
		err_code = pack_utf8_str(&param->list[i].topic, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	return mqtt_encode_fixed_header(message_type, start, buf);
}

int ping_request_encode(struct buf_ctx *buf)
{
	uint8_t *cur = buf->cur;
	uint8_t *end = buf->end;

	if ((end - cur) < sizeof(ping_packet)) {
		return -ENOMEM;
	}

	memcpy(cur, ping_packet, sizeof(ping_packet));
	buf->end = (cur + sizeof(ping_packet));

	return 0;
}
