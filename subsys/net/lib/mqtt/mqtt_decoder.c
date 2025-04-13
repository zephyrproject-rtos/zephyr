/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_decoder.c
 *
 * @brief Decoder functions needed for decoding packets received from the
 *        broker.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_dec, CONFIG_MQTT_LOG_LEVEL);

#include "mqtt_internal.h"
#include "mqtt_os.h"

/**
 * @brief Unpacks unsigned 8 bit value from the buffer from the offset
 *        requested.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] val Memory where the value is to be unpacked.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_uint8(struct buf_ctx *buf, uint8_t *val)
{
	uint8_t *cur = buf->cur;
	uint8_t *end = buf->end;

	NET_DBG(">> cur:%p, end:%p", (void *)cur, (void *)end);

	if ((end - cur) < sizeof(uint8_t)) {
		return -EINVAL;
	}

	*val = cur[0];
	buf->cur = (cur + sizeof(uint8_t));

	NET_DBG("<< val:%02x", *val);

	return 0;
}

/**
 * @brief Unpacks unsigned 16 bit value from the buffer from the offset
 *        requested.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] val Memory where the value is to be unpacked.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_uint16(struct buf_ctx *buf, uint16_t *val)
{
	uint8_t *cur = buf->cur;
	uint8_t *end = buf->end;

	NET_DBG(">> cur:%p, end:%p", (void *)cur, (void *)end);

	if ((end - cur) < sizeof(uint16_t)) {
		return -EINVAL;
	}

	*val = sys_get_be16(cur);
	buf->cur = (cur + sizeof(uint16_t));

	NET_DBG("<< val:%04x", *val);

	return 0;
}

/**
 * @brief Unpacks utf8 string from the buffer from the offset requested.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] str Pointer to a string that will hold the string location
 *                 in the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_utf8_str(struct buf_ctx *buf, struct mqtt_utf8 *str)
{
	uint16_t utf8_strlen;
	int err_code;

	NET_DBG(">> cur:%p, end:%p", (void *)buf->cur, (void *)buf->end);

	err_code = unpack_uint16(buf, &utf8_strlen);
	if (err_code != 0) {
		return err_code;
	}

	if ((buf->end - buf->cur) < utf8_strlen) {
		return -EINVAL;
	}

	str->size = utf8_strlen;
	/* Zero length UTF8 strings permitted. */
	if (utf8_strlen) {
		/* Point to right location in buffer. */
		str->utf8 = buf->cur;
		buf->cur += utf8_strlen;
	} else {
		str->utf8 = NULL;
	}

	NET_DBG("<< str_size:%08x", (uint32_t)GET_UT8STR_BUFFER_SIZE(str));

	return 0;
}

/**
 * @brief Unpacks binary string from the buffer from the offset requested with
 *        the length provided.
 *
 * @param[in] length Binary string length.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] str Pointer to a binary string that will hold the binary string
 *                 location in the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_raw_data(uint32_t length, struct buf_ctx *buf,
			   struct mqtt_binstr *str)
{
	NET_DBG(">> cur:%p, end:%p", (void *)buf->cur, (void *)buf->end);

	if ((buf->end - buf->cur) < length) {
		return -EINVAL;
	}

	str->len = length;

	/* Zero length binary strings are permitted. */
	if (length > 0) {
		str->data = buf->cur;
		buf->cur += length;
	} else {
		str->data = NULL;
	}

	NET_DBG("<< bin len:%08x", GET_BINSTR_BUFFER_SIZE(str));

	return 0;
}

int unpack_variable_int(struct buf_ctx *buf, uint32_t *val)
{
	uint8_t shift = 0U;
	int bytes = 0;

	*val = 0U;
	do {
		if (bytes >= MQTT_MAX_LENGTH_BYTES) {
			return -EINVAL;
		}

		if (buf->cur >= buf->end) {
			return -EAGAIN;
		}

		*val += ((uint32_t)*(buf->cur) & MQTT_LENGTH_VALUE_MASK)
								<< shift;
		shift += MQTT_LENGTH_SHIFT;
		bytes++;
	} while ((*(buf->cur++) & MQTT_LENGTH_CONTINUATION_BIT) != 0U);

	if (*val > MQTT_MAX_PAYLOAD_SIZE) {
		return -EINVAL;
	}

	NET_DBG("variable int:0x%08x", *val);

	return bytes;
}

int fixed_header_decode(struct buf_ctx *buf, uint8_t *type_and_flags,
			uint32_t *length)
{
	int err_code;

	err_code = unpack_uint8(buf, type_and_flags);
	if (err_code != 0) {
		return err_code;
	}

	err_code = unpack_variable_int(buf, length);
	if (err_code < 0) {
		return err_code;
	}

	return 0;
}

#if defined(CONFIG_MQTT_VERSION_5_0)
/**
 * @brief Unpacks unsigned 32 bit value from the buffer from the offset
 *        requested.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] val Memory where the value is to be unpacked.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_uint32(struct buf_ctx *buf, uint32_t *val)
{
	uint8_t *cur = buf->cur;
	uint8_t *end = buf->end;

	NET_DBG(">> cur:%p, end:%p", (void *)cur, (void *)end);

	if ((end - cur) < sizeof(uint32_t)) {
		return -EINVAL;
	}

	*val = sys_get_be32(cur);
	buf->cur = (cur + sizeof(uint32_t));

	NET_DBG("<< val:%08x", *val);

	return 0;
}

/**
 * @brief Unpacks binary string from the buffer from the offset requested.
 *        Binary string length is decoded from the first two bytes of the buffer.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] bin Pointer to a binary string that will hold the binary string
 *                 location in the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_binary_data(struct buf_ctx *buf, struct mqtt_binstr *bin)
{
	uint16_t len;
	int err;

	NET_DBG(">> cur:%p, end:%p", (void *)buf->cur, (void *)buf->end);

	err = unpack_uint16(buf, &len);
	if (err != 0) {
		return err;
	}

	if ((buf->end - buf->cur) < len) {
		return -EINVAL;
	}

	bin->len = len;
	/* Zero length binary strings are permitted. */
	if (len > 0) {
		bin->data = buf->cur;
		buf->cur += len;
	} else {
		bin->data = NULL;
	}

	NET_DBG("<< bin len:%08x", GET_BINSTR_BUFFER_SIZE(bin));

	return 0;
}

struct property_decoder {
	void *data;
	bool *found;
	uint8_t type;
};

int decode_uint32_property(struct property_decoder *prop,
			   uint32_t *remaining_len,
			   struct buf_ctx *buf)
{
	uint32_t *value = prop->data;

	if (*remaining_len < sizeof(uint32_t)) {
		return -EINVAL;
	}

	if (unpack_uint32(buf, value) < 0) {
		return -EINVAL;
	}

	*remaining_len -= sizeof(uint32_t);
	*prop->found = true;

	return 0;
}

int decode_uint16_property(struct property_decoder *prop,
			   uint32_t *remaining_len,
			   struct buf_ctx *buf)
{
	uint16_t *value = prop->data;

	if (*remaining_len < sizeof(uint16_t)) {
		return -EINVAL;
	}

	if (unpack_uint16(buf, value) < 0) {
		return -EINVAL;
	}

	*remaining_len -= sizeof(uint16_t);
	*prop->found = true;

	return 0;
}

int decode_uint8_property(struct property_decoder *prop,
			   uint32_t *remaining_len,
			   struct buf_ctx *buf)
{
	uint8_t *value = prop->data;

	if (*remaining_len < sizeof(uint8_t)) {
		return -EINVAL;
	}

	if (unpack_uint8(buf, value) < 0) {
		return -EINVAL;
	}

	*remaining_len -= sizeof(uint8_t);
	*prop->found = true;

	return 0;
}

int decode_string_property(struct property_decoder *prop,
			   uint32_t *remaining_len,
			   struct buf_ctx *buf)
{
	struct mqtt_utf8 *str = prop->data;

	if (unpack_utf8_str(buf, str) < 0) {
		return -EINVAL;
	}

	if (*remaining_len < sizeof(uint16_t) + str->size) {
		return -EINVAL;
	}

	*remaining_len -= sizeof(uint16_t) + str->size;
	*prop->found = true;

	return 0;
}

int decode_binary_property(struct property_decoder *prop,
			   uint32_t *remaining_len,
			   struct buf_ctx *buf)
{
	struct mqtt_binstr *bin = prop->data;

	if (unpack_binary_data(buf, bin) < 0) {
		return -EINVAL;
	}

	if (*remaining_len < sizeof(uint16_t) + bin->len) {
		return -EINVAL;
	}

	*remaining_len -= sizeof(uint16_t) + bin->len;
	*prop->found = true;

	return 0;
}

int decode_user_property(struct property_decoder *prop,
			   uint32_t *remaining_len,
			   struct buf_ctx *buf)
{
	struct mqtt_utf8_pair *user_prop = prop->data;
	struct mqtt_utf8_pair *chosen = NULL;
	struct mqtt_utf8_pair temp = { 0 };
	size_t prop_len;

	if (unpack_utf8_str(buf, &temp.name) < 0) {
		return -EINVAL;
	}

	if (unpack_utf8_str(buf, &temp.value) < 0) {
		return -EINVAL;
	}

	prop_len = (2 * sizeof(uint16_t)) + temp.name.size + temp.value.size;
	if (*remaining_len < prop_len) {
		return -EINVAL;
	}

	*remaining_len -= prop_len;
	*prop->found = true;

	for (int i = 0; i < CONFIG_MQTT_USER_PROPERTIES_MAX; i++) {
		if (user_prop[i].name.utf8 == NULL) {
			chosen = &user_prop[i];
			break;
		}
	}

	if (chosen == NULL) {
		NET_DBG("Cannot parse all user properties, ignore excess");
	} else {
		memcpy(chosen, &temp, sizeof(struct mqtt_utf8_pair));
	}

	return 0;
}

int decode_sub_id_property(struct property_decoder *prop,
			   uint32_t *remaining_len,
			   struct buf_ctx *buf)
{
	uint32_t *sub_id_array = prop->data;
	uint32_t *chosen = NULL;
	uint32_t value;
	int bytes;

	bytes = unpack_variable_int(buf, &value);
	if (bytes < 0) {
		return -EINVAL;
	}

	if (*remaining_len < bytes) {
		return -EINVAL;
	}

	*remaining_len -= bytes;
	*prop->found = true;

	for (int i = 0; i < CONFIG_MQTT_SUBSCRIPTION_ID_PROPERTIES_MAX; i++) {
		if (sub_id_array[i] == 0) {
			chosen = &sub_id_array[i];
			break;
		}
	}

	if (chosen == NULL) {
		NET_DBG("Cannot parse all subscription id properties, ignore excess");
	} else {
		*chosen = value;
	}

	return 0;
}

static int properties_decode(struct property_decoder *prop, uint8_t cnt,
			     struct buf_ctx *buf)
{
	uint32_t properties_len;
	int bytes;
	int err;

	bytes = unpack_variable_int(buf, &properties_len);
	if (bytes < 0) {
		return -EINVAL;
	}

	bytes += (int)properties_len;

	while (properties_len > 0) {
		struct property_decoder *current_prop = NULL;
		uint8_t type;

		/* Decode property type */
		err = unpack_uint8(buf, &type);
		if (err < 0) {
			return -EINVAL;
		}

		properties_len--;

		/* Search if the property is supported in the provided property
		 * array.
		 */
		for (int i = 0; i < cnt; i++) {
			if (type == prop[i].type) {
				current_prop = &prop[i];
			}
		}

		if (current_prop == NULL) {
			NET_DBG("Unsupported property %u", type);
			return -EBADMSG;
		}

		/* Decode property value. */
		switch (type) {
		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
			err = decode_uint32_property(current_prop,
						     &properties_len, buf);
			break;
		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_TOPIC_ALIAS:
			err = decode_uint16_property(current_prop,
						     &properties_len, buf);
			break;
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUBSCRIPTION_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER_AVAILABLE:
		case MQTT_PROP_SHARED_SUBSCRIPTION_AVAILABLE:
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
			err = decode_uint8_property(current_prop,
						    &properties_len, buf);
			break;
		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_REASON_STRING:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_SERVER_REFERENCE:
		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_CONTENT_TYPE:
			err = decode_string_property(current_prop,
						     &properties_len, buf);
			break;
		case MQTT_PROP_USER_PROPERTY:
			err = decode_user_property(current_prop,
						   &properties_len, buf);
			break;
		case MQTT_PROP_AUTHENTICATION_DATA:
		case MQTT_PROP_CORRELATION_DATA:
			err = decode_binary_property(current_prop,
						     &properties_len, buf);
			break;
		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			err = decode_sub_id_property(current_prop,
						     &properties_len, buf);
			break;
		default:
			err = -ENOTSUP;
		}

		if (err < 0) {
			return err;
		}
	}

	return bytes;
}

static int connack_properties_decode(struct buf_ctx *buf,
				     struct mqtt_connack_param *param)
{
	struct property_decoder prop[] = {
		{
			&param->prop.session_expiry_interval,
			&param->prop.rx.has_session_expiry_interval,
			MQTT_PROP_SESSION_EXPIRY_INTERVAL
		},
		{
			&param->prop.receive_maximum,
			&param->prop.rx.has_receive_maximum,
			MQTT_PROP_RECEIVE_MAXIMUM
		},
		{
			&param->prop.maximum_qos,
			&param->prop.rx.has_maximum_qos,
			MQTT_PROP_MAXIMUM_QOS
		},
		{
			&param->prop.retain_available,
			&param->prop.rx.has_retain_available,
			MQTT_PROP_RETAIN_AVAILABLE
		},
		{
			&param->prop.maximum_packet_size,
			&param->prop.rx.has_maximum_packet_size,
			MQTT_PROP_MAXIMUM_PACKET_SIZE
		},
		{
			&param->prop.assigned_client_id,
			&param->prop.rx.has_assigned_client_id,
			MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER
		},
		{
			&param->prop.topic_alias_maximum,
			&param->prop.rx.has_topic_alias_maximum,
			MQTT_PROP_TOPIC_ALIAS_MAXIMUM
		},
		{
			&param->prop.reason_string,
			&param->prop.rx.has_reason_string,
			MQTT_PROP_REASON_STRING
		},
		{
			&param->prop.user_prop,
			&param->prop.rx.has_user_prop,
			MQTT_PROP_USER_PROPERTY
		},
		{
			&param->prop.wildcard_sub_available,
			&param->prop.rx.has_wildcard_sub_available,
			MQTT_PROP_WILDCARD_SUBSCRIPTION_AVAILABLE
		},
		{
			&param->prop.subscription_ids_available,
			&param->prop.rx.has_subscription_ids_available,
			MQTT_PROP_SUBSCRIPTION_IDENTIFIER_AVAILABLE
		},
		{
			&param->prop.shared_sub_available,
			&param->prop.rx.has_shared_sub_available,
			MQTT_PROP_SHARED_SUBSCRIPTION_AVAILABLE
		},
		{
			&param->prop.server_keep_alive,
			&param->prop.rx.has_server_keep_alive,
			MQTT_PROP_SERVER_KEEP_ALIVE
		},
		{
			&param->prop.response_information,
			&param->prop.rx.has_response_information,
			MQTT_PROP_RESPONSE_INFORMATION
		},
		{
			&param->prop.server_reference,
			&param->prop.rx.has_server_reference,
			MQTT_PROP_SERVER_REFERENCE
		},
		{
			&param->prop.auth_method,
			&param->prop.rx.has_auth_method,
			MQTT_PROP_AUTHENTICATION_METHOD
		},
		{
			&param->prop.auth_data,
			&param->prop.rx.has_auth_data,
			MQTT_PROP_AUTHENTICATION_DATA
		}
	};

	return properties_decode(prop, ARRAY_SIZE(prop), buf);
}
#else
static int connack_properties_decode(struct buf_ctx *buf,
				     struct mqtt_connack_param *param)
{
	ARG_UNUSED(param);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int connect_ack_decode(const struct mqtt_client *client, struct buf_ctx *buf,
		       struct mqtt_connack_param *param)
{
	int err_code;
	uint8_t flags, ret_code;

	err_code = unpack_uint8(buf, &flags);
	if (err_code != 0) {
		return err_code;
	}

	err_code = unpack_uint8(buf, &ret_code);
	if (err_code != 0) {
		return err_code;
	}

	param->return_code = ret_code;

	if (client->protocol_version == MQTT_VERSION_3_1_0) {
		goto out;
	}

	param->session_present_flag = flags & MQTT_CONNACK_FLAG_SESSION_PRESENT;
	NET_DBG("[CID %p]: session_present_flag: %d", client,
		param->session_present_flag);

	if (mqtt_is_version_5_0(client)) {
		err_code = connack_properties_decode(buf, param);
		if (err_code < 0) {
			return err_code;
		}
	}

out:
	return 0;
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static int publish_properties_decode(struct buf_ctx *buf,
				     struct mqtt_publish_param *param)
{
	struct property_decoder prop[] = {
		{
			&param->prop.payload_format_indicator,
			&param->prop.rx.has_payload_format_indicator,
			MQTT_PROP_PAYLOAD_FORMAT_INDICATOR
		},
		{
			&param->prop.message_expiry_interval,
			&param->prop.rx.has_message_expiry_interval,
			MQTT_PROP_MESSAGE_EXPIRY_INTERVAL
		},
		{
			&param->prop.topic_alias,
			&param->prop.rx.has_topic_alias,
			MQTT_PROP_TOPIC_ALIAS
		},
		{
			&param->prop.response_topic,
			&param->prop.rx.has_response_topic,
			MQTT_PROP_RESPONSE_TOPIC
		},
		{
			&param->prop.correlation_data,
			&param->prop.rx.has_correlation_data,
			MQTT_PROP_CORRELATION_DATA
		},
		{
			&param->prop.user_prop,
			&param->prop.rx.has_user_prop,
			MQTT_PROP_USER_PROPERTY
		},
		{
			&param->prop.subscription_identifier,
			&param->prop.rx.has_subscription_identifier,
			MQTT_PROP_SUBSCRIPTION_IDENTIFIER
		},
		{
			&param->prop.content_type,
			&param->prop.rx.has_content_type,
			MQTT_PROP_CONTENT_TYPE
		}
	};

	return properties_decode(prop, ARRAY_SIZE(prop), buf);
}

static int publish_topic_alias_check(struct mqtt_client *client,
				     struct mqtt_publish_param *param)
{
	uint16_t alias = param->prop.topic_alias;
	struct mqtt_utf8 *topic_str = &param->message.topic.topic;
	struct mqtt_topic_alias *topic_alias;

	/* Topic alias must not exceed configured CONFIG_MQTT_TOPIC_ALIAS_MAX */
	if (alias > CONFIG_MQTT_TOPIC_ALIAS_MAX) {
		set_disconnect_reason(client, MQTT_DISCONNECT_TOPIC_ALIAS_INVALID);
		return -EBADMSG;
	}

	if (topic_str->size == 0) {
		/* In case there's no topic, topic alias must be set */
		if (alias == 0) {
			return -EBADMSG;
		}

		/* Topic alias number corresponds to the aliases array index. */
		topic_alias = &client->internal.topic_aliases[alias - 1];

		/* In case topic alias has not been configured yet, report an error. */
		if (topic_alias->topic_size == 0) {
			return -EBADMSG;
		}

		topic_str->utf8 = topic_alias->topic_buf;
		topic_str->size = topic_alias->topic_size;

		return 0;
	}

	/* If topic is present and topic alias is set, configure the alias locally. */
	if (alias > 0) {
		topic_alias = &client->internal.topic_aliases[alias - 1];

		if (topic_str->size > ARRAY_SIZE(topic_alias->topic_buf)) {
			NET_ERR("Topic too long (%d bytes) to store, increase "
				"CONFIG_MQTT_TOPIC_ALIAS_STRING_MAX",
				topic_str->size);
			set_disconnect_reason(client, MQTT_DISCONNECT_IMPL_SPECIFIC_ERROR);
			return -ENOMEM;
		}

		memcpy(topic_alias->topic_buf, topic_str->utf8, topic_str->size);
		topic_alias->topic_size = topic_str->size;
	}

	return 0;
}
#else
static int publish_properties_decode(struct buf_ctx *buf,
				     struct mqtt_publish_param *param)
{
	ARG_UNUSED(param);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}

static int publish_topic_alias_check(struct mqtt_client *client,
				     struct mqtt_publish_param *param)
{
	ARG_UNUSED(client);
	ARG_UNUSED(param);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int publish_decode(struct mqtt_client *client, uint8_t flags,
		   uint32_t var_length, struct buf_ctx *buf,
		   struct mqtt_publish_param *param)
{
	int err_code;
	uint32_t var_header_length;

	param->dup_flag = flags & MQTT_HEADER_DUP_MASK;
	param->retain_flag = flags & MQTT_HEADER_RETAIN_MASK;
	param->message.topic.qos = ((flags & MQTT_HEADER_QOS_MASK) >> 1);

	err_code = unpack_utf8_str(buf, &param->message.topic.topic);
	if (err_code != 0) {
		return err_code;
	}

	var_header_length = param->message.topic.topic.size + sizeof(uint16_t);

	if (param->message.topic.qos > MQTT_QOS_0_AT_MOST_ONCE) {
		err_code = unpack_uint16(buf, &param->message_id);
		if (err_code != 0) {
			return err_code;
		}

		var_header_length += sizeof(uint16_t);
	}

	if (mqtt_is_version_5_0(client)) {
		err_code = publish_properties_decode(buf, param);
		if (err_code < 0) {
			return err_code;
		}

		/* Add parsed properties length */
		var_header_length += err_code;

		err_code = publish_topic_alias_check(client, param);
		if (err_code < 0) {
			return err_code;
		}
	}

	if (var_length < var_header_length) {
		NET_ERR("Corrupted PUBLISH message, header length (%u) larger "
			 "than total length (%u)", var_header_length,
			 var_length);
		return -EINVAL;
	}

	param->message.payload.data = NULL;
	param->message.payload.len = var_length - var_header_length;

	return 0;
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static int common_ack_properties_decode(struct buf_ctx *buf,
					struct mqtt_common_ack_properties *prop)
{
	struct property_decoder prop_dec[] = {
		{
			&prop->reason_string,
			&prop->rx.has_reason_string,
			MQTT_PROP_REASON_STRING
		},
		{
			&prop->user_prop,
			&prop->rx.has_user_prop,
			MQTT_PROP_USER_PROPERTY
		}
	};

	return properties_decode(prop_dec, ARRAY_SIZE(prop_dec), buf);
}
#else
static int common_ack_properties_decode(struct buf_ctx *buf,
					struct mqtt_common_ack_properties *prop)
{
	ARG_UNUSED(prop);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

static int common_pub_ack_decode(struct buf_ctx *buf, uint16_t *message_id,
				 uint8_t *reason_code,
				 struct mqtt_common_ack_properties *prop)
{
	size_t remaining_len;
	int err;

	err = unpack_uint16(buf, message_id);
	if (err < 0) {
		return err;
	}

	remaining_len = buf->end - buf->cur;

	/* For MQTT < 5.0 properties are NULL. */
	if (prop != NULL && reason_code != NULL) {
		if (remaining_len > 0) {
			err = unpack_uint8(buf, reason_code);
			if (err < 0) {
				return err;
			}
		}

		if (remaining_len > 1) {
			err = common_ack_properties_decode(buf, prop);
			if (err < 0) {
				return err;
			}
		}
	}

	return 0;
}

int publish_ack_decode(const struct mqtt_client *client, struct buf_ctx *buf,
		       struct mqtt_puback_param *param)
{
	struct mqtt_common_ack_properties *prop = NULL;
	uint8_t *reason_code = NULL;

#if defined(CONFIG_MQTT_VERSION_5_0)
	if (mqtt_is_version_5_0(client)) {
		prop = &param->prop;
		reason_code = &param->reason_code;
	}
#endif

	return common_pub_ack_decode(buf, &param->message_id, reason_code, prop);
}

int publish_receive_decode(const struct mqtt_client *client, struct buf_ctx *buf,
			   struct mqtt_pubrec_param *param)
{
	struct mqtt_common_ack_properties *prop = NULL;
	uint8_t *reason_code = NULL;

#if defined(CONFIG_MQTT_VERSION_5_0)
	if (mqtt_is_version_5_0(client)) {
		prop = &param->prop;
		reason_code = &param->reason_code;
	}
#endif

	return common_pub_ack_decode(buf, &param->message_id, reason_code, prop);
}

int publish_release_decode(const struct mqtt_client *client, struct buf_ctx *buf,
			   struct mqtt_pubrel_param *param)
{
	struct mqtt_common_ack_properties *prop = NULL;
	uint8_t *reason_code = NULL;

#if defined(CONFIG_MQTT_VERSION_5_0)
	if (mqtt_is_version_5_0(client)) {
		prop = &param->prop;
		reason_code = &param->reason_code;
	}
#endif

	return common_pub_ack_decode(buf, &param->message_id, reason_code, prop);
}

int publish_complete_decode(const struct mqtt_client *client, struct buf_ctx *buf,
			    struct mqtt_pubcomp_param *param)
{
	struct mqtt_common_ack_properties *prop = NULL;
	uint8_t *reason_code = NULL;

#if defined(CONFIG_MQTT_VERSION_5_0)
	if (mqtt_is_version_5_0(client)) {
		prop = &param->prop;
		reason_code = &param->reason_code;
	}
#endif

	return common_pub_ack_decode(buf, &param->message_id, reason_code, prop);
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static int suback_properties_decode(struct buf_ctx *buf,
				    struct mqtt_suback_param *param)
{
	return common_ack_properties_decode(buf, &param->prop);
}
#else
static int suback_properties_decode(struct buf_ctx *buf,
				    struct mqtt_suback_param *param)
{
	ARG_UNUSED(param);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int subscribe_ack_decode(const struct mqtt_client *client, struct buf_ctx *buf,
			 struct mqtt_suback_param *param)
{
	int err_code;

	err_code = unpack_uint16(buf, &param->message_id);
	if (err_code != 0) {
		return err_code;
	}

	if (mqtt_is_version_5_0(client)) {
		err_code = suback_properties_decode(buf, param);
		if (err_code < 0) {
			return err_code;
		}
	}

	return unpack_raw_data(buf->end - buf->cur, buf, &param->return_codes);
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static int unsuback_5_0_decode(struct buf_ctx *buf,
			       struct mqtt_unsuback_param *param)
{
	int err;

	err = common_ack_properties_decode(buf, &param->prop);
	if (err < 0) {
		return err;
	}

	return unpack_raw_data(buf->end - buf->cur, buf, &param->reason_codes);
}
#else
static int unsuback_5_0_decode(struct buf_ctx *buf,
			       struct mqtt_unsuback_param *param)
{
	ARG_UNUSED(param);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */

int unsubscribe_ack_decode(const struct mqtt_client *client, struct buf_ctx *buf,
			   struct mqtt_unsuback_param *param)
{
	int err;

	err = unpack_uint16(buf, &param->message_id);
	if (err < 0) {
		return 0;
	}

	if (mqtt_is_version_5_0(client)) {
		return unsuback_5_0_decode(buf, param);
	}

	return 0;
}

#if defined(CONFIG_MQTT_VERSION_5_0)
static int disconnect_properties_decode(struct buf_ctx *buf,
					struct mqtt_disconnect_param *param)
{
	struct property_decoder prop[] = {
		{
			&param->prop.session_expiry_interval,
			&param->prop.rx.has_session_expiry_interval,
			MQTT_PROP_SESSION_EXPIRY_INTERVAL
		},
		{
			&param->prop.reason_string,
			&param->prop.rx.has_reason_string,
			MQTT_PROP_REASON_STRING
		},
		{
			&param->prop.user_prop,
			&param->prop.rx.has_user_prop,
			MQTT_PROP_USER_PROPERTY
		},
		{
			&param->prop.server_reference,
			&param->prop.rx.has_server_reference,
			MQTT_PROP_SERVER_REFERENCE
		}
	};

	return properties_decode(prop, ARRAY_SIZE(prop), buf);
}

int disconnect_decode(const struct mqtt_client *client, struct buf_ctx *buf,
		      struct mqtt_disconnect_param *param)
{

	size_t remaining_len;
	uint8_t reason_code;
	int err;

	if (!mqtt_is_version_5_0(client)) {
		return -ENOTSUP;
	}

	remaining_len = buf->end - buf->cur;

	if (remaining_len > 0) {
		err = unpack_uint8(buf, &reason_code);
		if (err < 0) {
			return err;
		}

		param->reason_code = reason_code;
	}

	if (remaining_len > 1) {
		err = disconnect_properties_decode(buf, param);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int auth_properties_decode(struct buf_ctx *buf,
				  struct mqtt_auth_param *param)
{
	struct property_decoder prop[] = {
		{
			&param->prop.auth_method,
			&param->prop.rx.has_auth_method,
			MQTT_PROP_AUTHENTICATION_METHOD
		},
		{
			&param->prop.auth_data,
			&param->prop.rx.has_auth_data,
			MQTT_PROP_AUTHENTICATION_DATA
		},
		{
			&param->prop.reason_string,
			&param->prop.rx.has_reason_string,
			MQTT_PROP_REASON_STRING
		},
		{
			&param->prop.user_prop,
			&param->prop.rx.has_user_prop,
			MQTT_PROP_USER_PROPERTY
		}
	};

	return properties_decode(prop, ARRAY_SIZE(prop), buf);
}

int auth_decode(const struct mqtt_client *client, struct buf_ctx *buf,
		struct mqtt_auth_param *param)
{
	size_t remaining_len;
	uint8_t reason_code;
	int err;

	if (!mqtt_is_version_5_0(client)) {
		return -ENOTSUP;
	}

	remaining_len = buf->end - buf->cur;

	if (remaining_len > 0) {
		err = unpack_uint8(buf, &reason_code);
		if (err < 0) {
			return err;
		}

		param->reason_code = reason_code;
	}

	if (remaining_len > 1) {
		err = auth_properties_decode(buf, param);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}
#endif /* CONFIG_MQTT_VERSION_5_0 */
