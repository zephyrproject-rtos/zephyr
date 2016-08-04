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

#include "mqtt_pack.h"

#include "MQTTPacket.h"
#include "MQTTConnect.h"
#include "MQTTPublish.h"
#include "MQTTSubscribe.h"
#include "MQTTUnsubscribe.h"

#include <errno.h>
#include <string.h>

/* From MQTTConnectClient.c */
extern int MQTTSerialize_zero(unsigned char *buf, int buflen,
			      unsigned char packettype);

static void str_to_buf(struct app_buf_t *buf, char *str);

int mqtt_msg_topic(struct mqtt_msg_t *msg, char *str)
{
	str_to_buf(&msg->topic, str);

	return 0;
}

int mqtt_msg_payload_str(struct mqtt_msg_t *msg, char *str)
{
	str_to_buf(&msg->payload, str);

	return 0;
}

int mqtt_next_pktid(struct mqtt_client_ctx_t *mqtt_ctx, uint16_t *pkt_id)
{
	*pkt_id = 1 + mqtt_ctx->pkt_id++;

	return 0;
}

int mqtt_pack_msg(struct app_buf_t *buf, enum mqtt_packet type, uint16_t pkt_id,
	      int dup)
{
	int rc;

	switch (type) {
	case MQTT_PUBACK:
		rc = MQTTSerialize_puback(buf->buf, buf->size, pkt_id);
		break;

	case MQTT_PUBCOMP:
		rc = MQTTSerialize_pubcomp(buf->buf, buf->size, pkt_id);
		break;

	case MQTT_PUBREC:
		rc = MQTTSerialize_ack(buf->buf, buf->size, MQTT_PUBREC,
				       dup, pkt_id);
		break;

	case MQTT_PUBREL:
		rc = MQTTSerialize_pubrel(buf->buf, buf->size, dup, pkt_id);
		break;

	default:
		buf->length = 0;
		return -EINVAL;
	}

	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

int mqtt_pack_subscribe(struct app_buf_t *buf, int dup, uint16_t pkt_id,
		       char *topic, enum mqtt_qos qos)
{
	MQTTString topic_str = MQTTString_initializer;
	int rc;

	topic_str.cstring = topic;
	rc = MQTTSerialize_subscribe(buf->buf, buf->size, dup, pkt_id, 1,
				     &topic_str, (int *)&qos);
	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

int mqtt_pack_unsubscribe(struct app_buf_t *buf, int dup, uint16_t pkt_id,
			  char *topic)
{
	MQTTString topic_str = MQTTString_initializer;
	int rc;

	topic_str.cstring = topic;
	rc = MQTTSerialize_unsubscribe(buf->buf, buf->size, dup, pkt_id, 1,
				       &topic_str);
	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

int mqtt_unpack_suback(struct app_buf_t *buf, uint16_t *pkt_id,
			 int *granted_qos)
{
	int sub_count;
	int rc;

	rc = MQTTDeserialize_suback(pkt_id, 1, &sub_count, granted_qos,
				    buf->buf, buf->length);

	if (rc != 1) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_unpack_unsuback(struct app_buf_t *buf, uint16_t *pkt_id)
{
	int rc;

	rc = MQTTDeserialize_unsuback(pkt_id, buf->buf, buf->length);

	if (rc != 1) {
		return -EINVAL;
	}

	return 0;
}

static int paho_mqtt_ctx(MQTTPacket_connectData *paho,
			 struct mqtt_client_ctx_t *mqtt_ctx)
{
	paho->MQTTVersion = mqtt_ctx->proto;
	paho->clientID.cstring = (char *)mqtt_ctx->client_id;
	paho->keepAliveInterval = mqtt_ctx->keep_alive;
	paho->cleansession = mqtt_ctx->clean_session;
	paho->username.cstring = (char *)mqtt_ctx->username;
	paho->password.cstring = (char *)mqtt_ctx->pass;
	paho->willFlag = mqtt_ctx->will_enabled;
	paho->will.message.cstring = (char *)mqtt_ctx->will_payload;
	paho->will.retained = mqtt_ctx->will_retained;
	paho->will.topicName.cstring = (char *)mqtt_ctx->will_topic;
	paho->will.qos = mqtt_ctx->will_qos;

	return 0;
}

int mqtt_pack_connect(struct app_buf_t *buf,
		      struct mqtt_client_ctx_t *mqtt_ctx)
{
	MQTTPacket_connectData paho = MQTTPacket_connectData_initializer;
	int rc;

	paho_mqtt_ctx(&paho, mqtt_ctx);
	rc = MQTTSerialize_connect(buf->buf, buf->size, &paho);
	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

int mqtt_unpack_connack(struct app_buf_t *buf, int *session, int *connect_ack)
{
	uint8_t _session;
	uint8_t _conn_ack;
	int rc;

	rc = MQTTDeserialize_connack(&_session, &_conn_ack,
				     buf->buf, buf->length);
	if (rc != 1) {
		return -EINVAL;
	}

	*session = _session;
	*connect_ack = _conn_ack;

	return 0;
}

int mqtt_pack_disconnect(struct app_buf_t *buf)
{
	int rc;

	rc = MQTTSerialize_disconnect(buf->buf, buf->size);
	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

int mqtt_pack_publish(struct app_buf_t *buf, struct mqtt_msg_t *msg)
{
	MQTTString topic_str = MQTTString_initializer;
	int rc;

	topic_str.cstring = msg->topic.buf;

	rc = MQTTSerialize_publish(buf->buf, buf->size,
				   msg->dup, msg->qos, msg->retained,
				   msg->pkt_id, topic_str,
				   msg->payload.buf, msg->payload.length);
	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

int mqtt_unpack_publish(struct app_buf_t *buf, struct mqtt_msg_t *msg)
{
	MQTTString topic_str = MQTTString_initializer;
	uint8_t dup;
	uint8_t retained;
	int payload_len;
	int qos;
	int rc;

	rc = MQTTDeserialize_publish(&dup, &qos, &retained,
				     &msg->pkt_id, &topic_str,
				     &msg->payload.buf, &payload_len,
				     buf->buf, buf->length);
	if (rc <= 0) {
		return -EINVAL;
	}

	msg->dup = dup;
	msg->qos = qos;
	msg->retained = retained;
	msg->payload.length = payload_len;
	msg->payload.size = msg->payload.length;

	msg->topic.buf = topic_str.lenstring.data;
	msg->topic.length = topic_str.lenstring.len;
	msg->topic.size = msg->topic.length;

	return 0;
}


int mqtt_unpack_ack(struct app_buf_t *buf, uint8_t *pkt_type,
		    uint8_t *dup, uint16_t *rcv_pkt_id)
{
	int rc;

	rc = MQTTDeserialize_ack(pkt_type, dup, rcv_pkt_id, buf->buf,
				 buf->length);
	if (rc != 1) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_pack_pubrel(struct app_buf_t *buf, int dup, uint16_t pkt_id)
{
	int rc;

	rc = MQTTSerialize_pubrel(buf->buf, buf->size, dup, pkt_id);
	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

int mqtt_pack_pingreq(struct app_buf_t *buf)
{
	int rc;

	rc = MQTTSerialize_pingreq(buf->buf, buf->size);
	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

int mqtt_pack_pingresp(struct app_buf_t *buf)
{
	int rc;

	rc =  MQTTSerialize_zero(buf->buf, buf->size, MQTT_PINGRESP);
	if (rc <= 0) {
		return -EINVAL;
	}

	buf->length = rc;

	return 0;
}

static void str_to_buf(struct app_buf_t *buf, char *str)
{
	buf->buf = str;
	buf->length = strlen(str) + 1;
	buf->size = buf->length;
}
