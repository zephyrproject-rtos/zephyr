/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/random/random.h>

#include "mqtt_internal.h"

#define SERVER_ADDR        "::1"
#define SERVER_PORT        1883
#define APP_SLEEP_MSECS    500
#define MQTT_CLIENTID      "zephyr_publisher"
#define BUFFER_SIZE        128
#define BROKER_BUFFER_SIZE 1500
#define TIMEOUT            100

static uint8_t broker_buf[BROKER_BUFFER_SIZE];
static size_t broker_offset;
static uint8_t broker_topic[32];
static uint8_t rx_buffer[BUFFER_SIZE];
static uint8_t tx_buffer[BUFFER_SIZE];
static struct mqtt_client client_ctx;
static struct sockaddr broker;
int s_sock = -1, c_sock = -1;
static struct zsock_pollfd client_fds[1];
static int client_nfds;

static struct mqtt_test_ctx {
	bool connected;
	bool ping_resp_handled;
	bool publish_handled;
	bool puback_handled;
	bool pubcomp_handled;
	bool suback_handled;
	bool unsuback_handled;
	uint16_t msg_id;
	int payload_left;
	const uint8_t *payload;
} test_ctx;

static const uint8_t payload_short[] = "Short payload";
static const uint8_t payload_long[] = LOREM_IPSUM;

static const uint8_t connect_ack_reply[] = {
	MQTT_PKT_TYPE_CONNACK, 0x02, 0, 0,
};

static const uint8_t ping_resp_reply[] = {
	MQTT_PKT_TYPE_PINGRSP, 0,
};

static const uint8_t puback_reply_template[] = {
	MQTT_PKT_TYPE_PUBACK, 0x02, 0, 0,
};

static const uint8_t pubrec_reply_template[] = {
	MQTT_PKT_TYPE_PUBREC, 0x02, 0, 0,
};

static const uint8_t pubcomp_reply_template[] = {
	MQTT_PKT_TYPE_PUBCOMP, 0x02, 0, 0,
};

static const uint8_t suback_reply_template[] = {
	MQTT_PKT_TYPE_SUBACK, 0x03, 0, 0, 0x02,
};

static const uint8_t unsuback_reply_template[] = {
	MQTT_PKT_TYPE_UNSUBACK, 0x02, 0, 0,
};

static const char *get_mqtt_topic(void)
{
	return "sensors";
}

static void prepare_client_fds(struct mqtt_client *client)
{
	client_fds[0].fd = client->transport.tcp.sock;
	client_fds[0].events = ZSOCK_POLLIN;
	client_nfds = 1;
}

static void clear_client_fds(void)
{
	client_nfds = 0;
}

static void client_wait(bool timeout_allowed)
{
	int ret;

	zassert_true(client_nfds > 0, "Client FDS should be set at this point");
	ret = zsock_poll(client_fds, client_nfds, TIMEOUT);
	if (timeout_allowed) {
		zassert_true(ret >= 0, "poll() error, (%d)", ret);
	} else {
		zassert_true(ret > 0, "poll() error, (%d)", ret);
	}
}

static void broker_init(void)
{
	struct sockaddr_in6 *broker6 = net_sin6(&broker);
	struct sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(SERVER_PORT),
		.sin6_addr = IN6ADDR_ANY_INIT,
	};
	int reuseaddr = 1;
	int ret;

	broker6->sin6_family = AF_INET6;
	broker6->sin6_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET6, SERVER_ADDR, &broker6->sin6_addr);

	memset(broker_topic, 0, sizeof(broker_topic));
	broker_offset = 0;

	s_sock = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (s_sock < 0) {
		printk("Failed to create server socket\n");
	}

	ret = zsock_setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
			       sizeof(reuseaddr));
	if (ret < 0) {
		printk("Failed to set SO_REUSEADDR on server socket, %d\n", errno);
	}

	ret = zsock_bind(s_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
	if (ret < 0) {
		printk("Failed to bind server socket, %d\n", errno);
	}

	ret = zsock_listen(s_sock, 1);
	if (ret < 0) {
		printk("Failed to listen on server socket, %d\n", errno);
	}
}

static void broker_destroy(void)
{
	if (s_sock >= 0) {
		zsock_close(s_sock);
		s_sock = -1;
	}

	if (c_sock >= 0) {
		zsock_close(c_sock);
		c_sock = -1;
	}
}

#define LISTEN_SOCK_ID 0
#define CLIENT_SOCK_ID 1

static void test_send_reply(const uint8_t *reply, size_t len)
{
	while (len) {
		ssize_t out_len = zsock_send(c_sock, reply, len, 0);

		zassert_true(out_len > 0, "Broker send failed (%d)", -errno);

		reply = reply + out_len;
		len -= out_len;
	}
}

static uint8_t encode_fixed_hdr(uint8_t *buf, uint8_t type_flags, uint32_t length)
{
	uint8_t bytes = 0U;

	bytes++;
	*buf = type_flags;
	buf++;

	do {
		bytes++;
		*buf = length & MQTT_LENGTH_VALUE_MASK;
		length >>= MQTT_LENGTH_SHIFT;
		if (length > 0) {
			*buf |= MQTT_LENGTH_CONTINUATION_BIT;
		}
		buf++;
	} while (length > 0);

	return bytes;
}

static void broker_validate_packet(uint8_t *buf, size_t length, uint8_t type,
				   uint8_t flags)
{
	switch (type) {
	case MQTT_PKT_TYPE_CONNECT: {
		test_send_reply(connect_ack_reply, sizeof(connect_ack_reply));
		break;
	}
	case MQTT_PKT_TYPE_PUBLISH: {
		uint8_t qos = (flags & MQTT_HEADER_QOS_MASK) >> 1;
		uint8_t reply_ack[sizeof(puback_reply_template)];
		uint16_t topic_len, var_len = 0;
		bool ack = false;

		topic_len = sys_get_be16(buf);

		if (qos == MQTT_QOS_0_AT_MOST_ONCE) {
			var_len = topic_len + 2;
		} else if (qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			ack = true;
			var_len = topic_len + 4;
			memcpy(reply_ack, puback_reply_template, sizeof(reply_ack));
		} else if (qos == MQTT_QOS_2_EXACTLY_ONCE) {
			ack = true;
			var_len = topic_len + 4;
			memcpy(reply_ack, pubrec_reply_template, sizeof(reply_ack));
		} else {
			zassert_unreachable("Invalid qos received");
		}

		zassert_equal(topic_len, strlen(get_mqtt_topic()), "Invalid topic length");
		zassert_mem_equal(buf + 2, get_mqtt_topic(), topic_len, "Invalid topic");
		zassert_equal(length - var_len, strlen(test_ctx.payload),
			      "Invalid payload length");
		zassert_mem_equal(buf + var_len, test_ctx.payload,
				  strlen(test_ctx.payload), "Invalid payload");

		if (ack) {
			/* Copy packet ID. */
			memcpy(reply_ack + 2, buf + topic_len + 2, 2);
			test_send_reply(reply_ack, sizeof(reply_ack));
		}

		if (topic_len == strlen(broker_topic) &&
		    memcmp(buf + 2, broker_topic, topic_len) == 0) {
			uint8_t fixed_hdr[MQTT_FIXED_HEADER_MAX_SIZE];
			uint8_t hdr_len = encode_fixed_hdr(
				fixed_hdr, MQTT_PKT_TYPE_PUBLISH | flags, length);

			/* Send publish back */
			test_send_reply(fixed_hdr, hdr_len);
			test_send_reply(buf, length);
		}

		break;
	}
	case MQTT_PKT_TYPE_PUBACK: {
		uint16_t message_id;

		zassert_equal(length, 2, "Invalid PUBACK length");

		message_id = sys_get_be16(buf);
		zassert_equal(message_id, test_ctx.msg_id,
			      "Invalid packet ID received.");
		break;
	}
	case MQTT_PKT_TYPE_PUBREL: {
		uint8_t reply[sizeof(pubcomp_reply_template)];

		memcpy(reply, pubcomp_reply_template, sizeof(reply));
		memcpy(reply + 2, buf, 2);
		test_send_reply(reply, sizeof(reply));

		break;
	}
	case MQTT_PKT_TYPE_SUBSCRIBE: {
		uint16_t topic_len;
		uint8_t reply[sizeof(suback_reply_template)];

		topic_len = sys_get_be16(buf + 2);
		zassert_true(topic_len <= length - 5, "Invalid topic length");
		zassert_true(topic_len < sizeof(broker_topic),
			     "Topic length too long to handle");
		memcpy(broker_topic, buf + 4, topic_len);
		broker_topic[topic_len] = '\0';

		memcpy(reply, suback_reply_template, sizeof(suback_reply_template));
		/* Copy packet ID. */
		memcpy(reply + 2, buf, 2);
		test_send_reply(reply, sizeof(reply));

		break;
	}
	case MQTT_PKT_TYPE_UNSUBSCRIBE: {
		uint16_t topic_len;
		uint8_t reply[sizeof(unsuback_reply_template)];

		topic_len = sys_get_be16(buf + 2);
		zassert_true(topic_len <= length - 4, "Invalid topic length");
		zassert_mem_equal(broker_topic, buf + 4, topic_len,
			     "Invalid topic received");
		memset(broker_topic, 0, sizeof(broker_topic));

		memcpy(reply, unsuback_reply_template, sizeof(unsuback_reply_template));
		/* Copy packet ID. */
		memcpy(reply + 2, buf, 2);
		test_send_reply(reply, sizeof(reply));

		break;
	}
	case MQTT_PKT_TYPE_PINGREQ: {
		test_send_reply(ping_resp_reply, sizeof(ping_resp_reply));
		break;
	}
	case MQTT_PKT_TYPE_DISCONNECT: {
		zsock_close(c_sock);
		c_sock = -1;
		break;
	}
	default:
		zassert_true(false, "Not yet supported (%02x)", type);
	}
}

static int broker_receive(uint8_t expected_packet)
{
	struct buf_ctx buf;
	size_t bytes_consumed = 0;
	uint8_t type_and_flags, type, flags;
	uint32_t length;
	int ret;

	zassert_false(broker_offset == sizeof(broker_buf), "Cannot fit full payload!");

	ret = zsock_recv(c_sock, broker_buf + broker_offset,
			 sizeof(broker_buf) - broker_offset,
			 ZSOCK_MSG_DONTWAIT);

	if (ret == -1 && errno == EAGAIN) {
		/* EAGAIN expected only if there already was data in the buffer. */
		zassert_true(broker_offset > 0, "Unexpected EAGAIN in broker");
	} else {
		zassert_true(ret > 0, "Broker receive failed (%d)", -errno);
		broker_offset += ret;
	}

	if (broker_offset < MQTT_FIXED_HEADER_MIN_SIZE) {
		return -EAGAIN;
	}

	buf.cur = broker_buf;
	buf.end = broker_buf + broker_offset;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	if (ret == -EAGAIN) {
		return ret;
	}

	zassert_ok(ret, "Failed to decode fixed header (%d)", ret);

	if (length > buf.end - buf.cur) {
		return -EAGAIN;
	}

	bytes_consumed += buf.cur - broker_buf;
	bytes_consumed += length;

	type = type_and_flags & 0xF0;
	flags = type_and_flags & 0x0F;
	zassert_equal(type, expected_packet,
		      "Unexpected packet type received at the broker, (%02x)",
		      type);

	broker_validate_packet(buf.cur, length, type, flags);

	broker_offset -= bytes_consumed;
	if (broker_offset > 0) {
		memmove(broker_buf, broker_buf + bytes_consumed,
			broker_offset);
	}

	return 0;
}

static void broker_process(uint8_t expected_packet)
{
	struct zsock_pollfd fds[2] = {
		{ s_sock, ZSOCK_POLLIN, 0},
		{ c_sock, ZSOCK_POLLIN, 0},
	};
	int ret;

	if (c_sock >= 0 && broker_offset > 0) {
		ret = broker_receive(expected_packet);
		if (ret == 0) {
			goto out;
		}
	}

	while (true) {
		ret = zsock_poll(fds, ARRAY_SIZE(fds), TIMEOUT);
		zassert_true(ret > 0, "Unexpected timeout on poll");

		for (int i = 0; i < ARRAY_SIZE(fds); i++) {
			if (fds[i].fd < 0) {
				continue;
			}

			zassert_false((fds[i].revents & ZSOCK_POLLERR) ||
				      (fds[i].revents & ZSOCK_POLLHUP) ||
				      (fds[i].revents & ZSOCK_POLLNVAL),
				      "Unexpected poll event, (%02x)",
				      fds[i].revents);

			if (!(fds[i].revents & ZSOCK_POLLIN)) {
				continue;
			}

			if (i == LISTEN_SOCK_ID) {
				zassert_equal(c_sock, -1, "Client already connected");
				ret = zsock_accept(s_sock, NULL, NULL);
				zassert_true(ret >= 0, "Accept failed (%d)", -errno);

				c_sock = ret;
				fds[CLIENT_SOCK_ID].fd = c_sock;
			} else {
				ret = broker_receive(expected_packet);
				if (ret == 0) {
					goto out;
				}
			}
		}
	}

out:
	return;
}

static void publish_handler(struct mqtt_client *const client,
			    const struct mqtt_evt *evt)
{
	int ret;
	static uint8_t buf[sizeof(payload_long)];

	zassert_equal(evt->result, 0, "MQTT PUBLISH error: %d", evt->result);
	zassert_equal(test_ctx.payload_left,
		      evt->param.publish.message.payload.len,
		      "Invalid payload length: %d",
		      evt->param.publish.message.payload.len);

	ret = mqtt_readall_publish_payload(client, buf, test_ctx.payload_left);
	zassert_ok(ret, "Error while reading publish payload (%d)", ret);
	zassert_mem_equal(test_ctx.payload, buf,
			  evt->param.publish.message.payload.len,
			  "Invalid payload content");

	test_ctx.payload_left = 0;
	test_ctx.publish_handled = true;
}

static void mqtt_evt_handler(struct mqtt_client *const client,
			     const struct mqtt_evt *evt)
{
	int ret;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		zassert_ok(evt->result, "MQTT connect failed %d", evt->result);
		test_ctx.connected = true;
		break;

	case MQTT_EVT_DISCONNECT:
		test_ctx.connected = false;
		test_ctx.payload_left = -1;

		break;

	case MQTT_EVT_PUBLISH:
		publish_handler(client, evt);

		if (evt->param.publish.message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = evt->param.publish.message_id,
			};

			ret = mqtt_publish_qos1_ack(client, &ack);
			zassert_ok(ret, "Failed to send MQTT PUBACK (%d)", ret);
		}

		if (evt->param.publish.message.topic.qos == MQTT_QOS_2_EXACTLY_ONCE) {
			const struct mqtt_pubrec_param ack = {
				.message_id = evt->param.publish.message_id,
			};

			ret = mqtt_publish_qos2_receive(client, &ack);
			zassert_ok(ret, "Failed to send MQTT PUBREC (%d)", ret);
		}

		break;

	case MQTT_EVT_PUBACK:
		zassert_ok(evt->result, "MQTT PUBACK error %d", evt->result);
		zassert_equal(evt->param.puback.message_id, test_ctx.msg_id,
			      "Invalid packet ID received.");
		test_ctx.puback_handled = true;

		break;

	case MQTT_EVT_PUBREC: {
		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		zassert_ok(evt->result, "MQTT PUBREC error %d", evt->result);
		zassert_equal(evt->param.pubrec.message_id, test_ctx.msg_id,
			      "Invalid packet ID received.");

		ret = mqtt_publish_qos2_release(client, &rel_param);
		zassert_ok(ret, "Failed to send MQTT PUBREL: %d", ret);

		break;
	}

	case MQTT_EVT_PUBCOMP:
		zassert_ok(evt->result, "MQTT PUBCOMP error %d", evt->result);
		zassert_equal(evt->param.pubcomp.message_id, test_ctx.msg_id,
			      "Invalid packet ID received.");
		test_ctx.pubcomp_handled = true;

		break;

	case MQTT_EVT_SUBACK:
		zassert_ok(evt->result, "MQTT SUBACK error %d", evt->result);
		zassert_equal(evt->param.suback.message_id, test_ctx.msg_id,
			      "Invalid packet ID received.");
		test_ctx.suback_handled = true;

		break;

	case MQTT_EVT_UNSUBACK:
		zassert_ok(evt->result, "MQTT UNSUBACK error %d", evt->result);
		zassert_equal(evt->param.unsuback.message_id, test_ctx.msg_id,
			      "Invalid packet ID received.");

		test_ctx.unsuback_handled = true;

		break;

	case MQTT_EVT_PINGRESP:
		test_ctx.ping_resp_handled = true;
		break;

	default:
		zassert_unreachable("Invalid MQTT packet");
		break;
	}
}

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (uint8_t *)MQTT_CLIENTID;
	client->client_id.size = strlen(MQTT_CLIENTID);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
	client->clean_session = true;

	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);
}

static void test_connect(void)
{
	int ret;

	ret = mqtt_connect(&client_ctx);
	zassert_ok(ret, "MQTT client failed to connect (%d)", ret);
	broker_process(MQTT_PKT_TYPE_CONNECT);
	prepare_client_fds(&client_ctx);

	client_wait(false);
	ret = mqtt_input(&client_ctx);
	zassert_ok(ret, "MQTT client input processing failed (%d)", ret);
}

static void test_pingreq(void)
{
	int ret;

	ret = mqtt_ping(&client_ctx);
	zassert_ok(ret, "MQTT client failed to send ping (%d)", ret);
	broker_process(MQTT_PKT_TYPE_PINGREQ);

	client_wait(false);
	ret = mqtt_input(&client_ctx);
	zassert_ok(ret, "MQTT client input processing failed (%d)", ret);
}

static void test_publish(enum mqtt_qos qos)
{
	int ret;
	struct mqtt_publish_param param;

	test_ctx.payload_left = strlen(test_ctx.payload);
	while (test_ctx.msg_id == 0) {
		test_ctx.msg_id = sys_rand16_get();
	}

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (uint8_t *)get_mqtt_topic();
	param.message.topic.topic.size =
			strlen(param.message.topic.topic.utf8);
	param.message.payload.data = (uint8_t *)test_ctx.payload;
	param.message.payload.len = test_ctx.payload_left;
	param.message_id = test_ctx.msg_id;
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	ret = mqtt_publish(&client_ctx, &param);
	zassert_ok(ret, "MQTT client failed to publish (%d)", ret);
	broker_process(MQTT_PKT_TYPE_PUBLISH);

	client_wait(true);
	ret = mqtt_input(&client_ctx);
	zassert_ok(ret, "MQTT client input processing failed (%d)", ret);

	/* Second input handle for expected Publish Complete response. */
	if (qos == MQTT_QOS_2_EXACTLY_ONCE) {
		broker_process(MQTT_PKT_TYPE_PUBREL);
		client_wait(false);
		ret = mqtt_input(&client_ctx);
		zassert_ok(ret, "MQTT client input processing failed (%d)", ret);
	}
}

static void test_subscribe(void)
{
	int ret;
	struct mqtt_topic topic;
	struct mqtt_subscription_list sub;

	while (test_ctx.msg_id == 0) {
		test_ctx.msg_id = sys_rand16_get();
	}

	topic.topic.utf8 = get_mqtt_topic();
	topic.topic.size = strlen(topic.topic.utf8);
	topic.qos = MQTT_QOS_2_EXACTLY_ONCE;
	sub.list = &topic;
	sub.list_count = 1U;
	sub.message_id = test_ctx.msg_id;

	ret = mqtt_subscribe(&client_ctx, &sub);
	zassert_ok(ret, "MQTT client failed to subscribe (%d)", ret);
	broker_process(MQTT_PKT_TYPE_SUBSCRIBE);

	client_wait(false);
	ret = mqtt_input(&client_ctx);
	zassert_ok(ret, "MQTT client input processing failed (%d)", ret);
}

static void test_unsubscribe(void)
{
	int ret;
	struct mqtt_topic topic;
	struct mqtt_subscription_list unsub;

	while (test_ctx.msg_id == 0) {
		test_ctx.msg_id = sys_rand16_get();
	}

	topic.topic.utf8 = get_mqtt_topic();
	topic.topic.size = strlen(topic.topic.utf8);
	unsub.list = &topic;
	unsub.list_count = 1U;
	unsub.message_id = test_ctx.msg_id;

	ret = mqtt_unsubscribe(&client_ctx, &unsub);
	zassert_ok(ret, "MQTT client failed to unsubscribe (%d)", ret);
	broker_process(MQTT_PKT_TYPE_UNSUBSCRIBE);

	client_wait(false);
	ret = mqtt_input(&client_ctx);
	zassert_ok(ret, "MQTT client input processing failed (%d)", ret);
}

static void test_disconnect(void)
{
	int ret;

	ret = mqtt_disconnect(&client_ctx);
	zassert_ok(ret, "MQTT client failed to disconnect (%d)", ret);
	broker_process(MQTT_PKT_TYPE_DISCONNECT);

	client_wait(false);
	ret = mqtt_input(&client_ctx);
	zassert_equal(ret, -ENOTCONN, "Client should no longer be connected");
}

ZTEST(mqtt_client, test_mqtt_connect)
{
	test_connect();
	zassert_true(test_ctx.connected, "MQTT client should be connected");
	test_disconnect();
	zassert_false(test_ctx.connected, "MQTT client should be disconnected");
}

ZTEST(mqtt_client, test_mqtt_ping)
{
	test_connect();
	test_pingreq();
	zassert_true(test_ctx.ping_resp_handled, "MQTT client should handle ping response");
	test_disconnect();
}

ZTEST(mqtt_client, test_mqtt_publish_qos0)
{
	test_ctx.payload = payload_short;

	test_connect();
	test_publish(MQTT_QOS_0_AT_MOST_ONCE);
	zassert_false(test_ctx.puback_handled, "MQTT client should not receive puback");
	zassert_false(test_ctx.pubcomp_handled, "MQTT client should not receive pubcomp");
	test_disconnect();
}

ZTEST(mqtt_client, test_mqtt_publish_qos1)
{
	test_ctx.payload = payload_short;

	test_connect();
	test_publish(MQTT_QOS_1_AT_LEAST_ONCE);
	zassert_true(test_ctx.puback_handled, "MQTT client should receive puback");
	zassert_false(test_ctx.pubcomp_handled, "MQTT client should not receive pubcomp");
	test_disconnect();
}

ZTEST(mqtt_client, test_mqtt_publish_qos2)
{
	test_ctx.payload = payload_short;

	test_connect();
	test_publish(MQTT_QOS_2_EXACTLY_ONCE);
	zassert_false(test_ctx.puback_handled, "MQTT client should not receive puback");
	zassert_true(test_ctx.pubcomp_handled, "MQTT client should receive pubcomp");
	test_disconnect();
}

ZTEST(mqtt_client, test_mqtt_subscribe)
{
	test_connect();
	test_subscribe();
	zassert_true(test_ctx.suback_handled, "MQTT client should receive suback");
	zassert_str_equal(broker_topic, get_mqtt_topic(), "Invalid topic");
	test_unsubscribe();
	zassert_true(test_ctx.unsuback_handled, "MQTT client should receive unsuback");
	zassert_str_equal(broker_topic, "", "Topic should be cleared now");
	test_disconnect();
}

static void test_pubsub(const uint8_t *payload, enum mqtt_qos qos)
{
	int ret;

	test_ctx.payload = payload;

	test_connect();
	test_subscribe();
	test_publish(qos);

	while (test_ctx.payload_left > 0) {
		client_wait(false);
		ret = mqtt_input(&client_ctx);
		zassert_ok(ret, "MQTT client input processing failed (%d)", ret);
	}

	zassert_true(test_ctx.publish_handled, "MQTT client should receive publish");

	if (qos == MQTT_QOS_1_AT_LEAST_ONCE) {
		broker_process(MQTT_PKT_TYPE_PUBACK);
	}

	test_unsubscribe();
	test_disconnect();
}

ZTEST(mqtt_client, test_mqtt_pubsub_short)
{
	test_pubsub(payload_short, MQTT_QOS_0_AT_MOST_ONCE);
	zassert_false(test_ctx.puback_handled, "MQTT client should not receive puback");
}

ZTEST(mqtt_client, test_mqtt_pubsub_long)
{
	test_pubsub(payload_long, MQTT_QOS_1_AT_LEAST_ONCE);
	zassert_true(test_ctx.puback_handled, "MQTT client should receive puback");
}

static void mqtt_tests_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&test_ctx, 0, sizeof(test_ctx));
	broker_init();
	client_init(&client_ctx);
}

static void mqtt_tests_after(void *fixture)
{
	ARG_UNUSED(fixture);

	broker_destroy();
	mqtt_abort(&client_ctx);
	clear_client_fds();
	/* Let the TCP workqueue release TCP contexts. */
	k_msleep(10);
}

ZTEST_SUITE(mqtt_client, NULL, NULL, mqtt_tests_before, mqtt_tests_after, NULL);
