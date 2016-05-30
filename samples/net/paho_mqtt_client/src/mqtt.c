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

#include <string.h>
#include <errno.h>

#include "mqtt.h"
#include "tcp.h"

#include "MQTTPacket.h"
#include "MQTTConnect.h"
#include "MQTTPublish.h"
#include "MQTTSubscribe.h"
#include "MQTTUnsubscribe.h"

/* non thread safe */
#define BUF_SIZE 128
static uint8_t mqtt_buffer[BUF_SIZE];

int mqtt_connect(struct net_context *ctx, char *client_name)
{
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	unsigned char session_present;
	unsigned char conn_ack;
	size_t rx_len;
	size_t tx_len;
	int rc;

	memset(&data, 0x00, sizeof(MQTTPacket_connectData));

	data.MQTTVersion = 4;
	data.clientID.cstring = client_name;
	data.keepAliveInterval = 500;
	data.cleansession = 1;
	data.username.cstring = "zephyr";
	data.password.cstring = "1234";
	data.willFlag = 1;
	data.will.message.cstring = "zephyr_will_msg";
	data.will.retained = 0;
	data.will.topicName.cstring = "zephyr_will_topic";
	data.will.qos = 1;

	rc = MQTTSerialize_connect(mqtt_buffer, BUF_SIZE, &data);
	tx_len = rc;
	rc = rc <= 0 ? -1 : 0;
	if (rc != 0) {
		return rc;
	}

	tcp_tx(ctx, mqtt_buffer, tx_len);

	rc = tcp_rx(ctx, mqtt_buffer, &rx_len, BUF_SIZE);
	if (rc != 0) {
		return -EIO;
	}
	rc = MQTTDeserialize_connack(&session_present, &conn_ack,
				     mqtt_buffer, rx_len);

	return rc == 1 ? conn_ack : -EINVAL;
}

int mqtt_disconnect(struct net_context *ctx)
{
	int tx_len;
	int rc;

	rc = MQTTSerialize_disconnect(mqtt_buffer, BUF_SIZE);
	tx_len = rc;
	rc = rc <= 0 ? -EINVAL : 0;
	if (rc != 0) {
		return -EINVAL;
	}

	tcp_tx(ctx, mqtt_buffer, tx_len);
	return 0;
}

int mqtt_publish(struct net_context *ctx, char *topic, char *msg)
{
	MQTTString topic_str = MQTTString_initializer;
	unsigned char pkt_type;
	unsigned char dup;
	unsigned short pkt_id;
	size_t tx_len;
	size_t rx_len;
	int rc = 0;

	topic_str.cstring = topic;
	rc = MQTTSerialize_publish(mqtt_buffer, BUF_SIZE, 0, 0, 0, 0,
				   topic_str, (unsigned char *)msg,
				   strlen(msg));
	tx_len = rc;
	rc = rc <= 0 ? -EINVAL : 0;
	if (rc != 0) {
		return rc;
	}

	tcp_tx(ctx, mqtt_buffer, tx_len);

	rc = tcp_rx(ctx, mqtt_buffer, &rx_len, BUF_SIZE);
	if (rc != 0) {
		return -EIO;
	}

	rc = MQTTDeserialize_ack(&pkt_type, &dup, &pkt_id, mqtt_buffer,
				 rx_len);
	return rc == 1 ? 0 : -EINVAL;
}

int mqtt_pingreq(struct net_context *ctx)
{
	unsigned char pkt_type;
	unsigned char dup;
	unsigned short pkt_id;
	size_t tx_len;
	size_t rx_len;
	int rc;

	rc = MQTTSerialize_pingreq(mqtt_buffer, BUF_SIZE);

	tx_len = rc;
	rc = rc <= 0 ? -EINVAL : 0;
	if (rc != 0) {
		return -EINVAL;
	}

	tcp_tx(ctx, mqtt_buffer, tx_len);

	rc = tcp_rx(ctx, mqtt_buffer, &rx_len, BUF_SIZE);
	if (rc != 0) {
		return -EIO;
	}

	rc = MQTTDeserialize_ack(&pkt_type, &dup, &pkt_id, mqtt_buffer,
				  rx_len);
	return rc == 1 ? 0 : -EINVAL;
}

int mqtt_publish_read(struct net_context *ctx)
{
	MQTTString topic;
	unsigned char dup;
	unsigned char retained;
	unsigned short msg_id;
	unsigned char *msg;
	size_t rx_len;
	int qos;
	int msg_len;
	int rc;

	rc = tcp_rx(ctx, mqtt_buffer, &rx_len, BUF_SIZE);
	if (rc != 0) {
		return -EIO;
	}

	rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msg_id,
				     &topic, &msg, &msg_len,
				     mqtt_buffer, rx_len);
	rc = rc == 1 ? 0 : -EIO;
	if (rc == 0) {
		printf("\n\tReceived message: [%.*s] %.*s\n\n",
		       topic.lenstring.len, topic.lenstring.data,
		       msg_len, msg);
	}
	return rc;
}


int mqtt_subscribe(struct net_context *ctx, char *topic)
{
	MQTTString topic_str = MQTTString_initializer;
	unsigned short submsg_id;
	size_t rx_len;
	size_t tx_len;
	int msg_id = 1;
	int req_qos = 0;
	int sub_count;
	int granted_qos;
	int rc = 0;

	topic_str.cstring = topic;
	rc = MQTTSerialize_subscribe(mqtt_buffer, BUF_SIZE, 0, msg_id, 1,
				     &topic_str, &req_qos);
	tx_len = rc;
	rc = rc <= 0 ? -EINVAL : 0;
	if (rc != 0) {
		return -EINVAL;
	}

	tcp_tx(ctx, mqtt_buffer, tx_len);

	rc = tcp_rx(ctx, mqtt_buffer, &rx_len, BUF_SIZE);
	if (rc != 0) {
		return -EIO;
	}

	rc = MQTTDeserialize_suback(&submsg_id, 1, &sub_count, &granted_qos,
				    mqtt_buffer, rx_len);

	return rc == 1 ? granted_qos : -EINVAL;
}

