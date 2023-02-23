/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/mqtt_sn.h>
#include <zephyr/sys/util.h> /* for ARRAY_SIZE */
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <mqtt_sn_msg.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

static const struct mqtt_sn_data client_id = MQTT_SN_DATA_STRING_LITERAL("zephyr");

static uint8_t tx[255];
static uint8_t rx[255];

static struct msg_send_data {
	int called;
	size_t msg_sz;
	int ret;
	struct mqtt_sn_client *client;
} msg_send_data;

static int msg_send(struct mqtt_sn_client *client, void *buf, size_t sz)
{
	msg_send_data.called++;
	msg_send_data.msg_sz = sz;
	msg_send_data.client = client;

	return msg_send_data.ret;
}

static void assert_msg_send(int called, size_t msg_sz)
{
	zassert_equal(msg_send_data.called, called, "msg_send called %d times instead of %d",
		      msg_send_data.called, called);
	zassert_equal(msg_send_data.msg_sz, msg_sz, "msg_sz is %zu instead of %zu",
		      msg_send_data.msg_sz, msg_sz);

	memset(&msg_send_data, 0, sizeof(msg_send_data));
}

static struct {
	struct mqtt_sn_evt last_evt;
	int called;
} evt_cb_data;

static void evt_cb(struct mqtt_sn_client *client, const struct mqtt_sn_evt *evt)
{
	memcpy(&evt_cb_data.last_evt, evt, sizeof(*evt));
	evt_cb_data.called++;
}

static bool tp_initialized;
static struct mqtt_sn_transport transport;

static int tp_init(struct mqtt_sn_transport *tp)
{
	tp_initialized = true;
	return 0;
}

static struct {
	void *data;
	ssize_t sz;
} recv_data;

static ssize_t tp_recv(struct mqtt_sn_client *client, void *buffer, size_t length)
{
	if (recv_data.data && recv_data.sz > 0 && length >= recv_data.sz) {
		memcpy(buffer, recv_data.data, recv_data.sz);
	}

	return recv_data.sz;
}

int tp_poll(struct mqtt_sn_client *client)
{
	return recv_data.sz;
}

static ZTEST_BMEM struct mqtt_sn_client clients[3];
static ZTEST_BMEM struct mqtt_sn_client *client;

static void setup(void *f)
{
	ARG_UNUSED(f);
	static ZTEST_BMEM size_t i;

	client = &clients[i++];

	transport = (struct mqtt_sn_transport){
		.init = tp_init, .msg_send = msg_send, .recv = tp_recv, .poll = tp_poll};
	tp_initialized = false;

	memset(&evt_cb_data, 0, sizeof(evt_cb_data));
	memset(&msg_send_data, 0, sizeof(msg_send_data));
	memset(&recv_data, 0, sizeof(recv_data));
}

static int input(struct mqtt_sn_client *client, void *buf, size_t sz)
{
	recv_data.data = buf;
	recv_data.sz = sz;

	return mqtt_sn_input(client);
}

static void mqtt_sn_connect_no_will(struct mqtt_sn_client *client)
{
	/* connack with return code accepted */
	static uint8_t connack[] = {3, 0x05, 0x00};
	int err;

	err = mqtt_sn_client_init(client, &client_id, &transport, evt_cb, tx, sizeof(tx), rx,
				  sizeof(rx));
	zassert_equal(err, 0, "unexpected error %d");
	zassert_true(tp_initialized, "Transport not initialized");

	err = mqtt_sn_connect(client, false, false);
	zassert_equal(err, 0, "unexpected error %d");
	assert_msg_send(1, 12);
	zassert_equal(client->state, 0, "Wrong state");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");

	err = input(client, connack, sizeof(connack));
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(client->state, 1, "Wrong state");
	zassert_equal(evt_cb_data.called, 1, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_CONNECTED, "Wrong event");
	k_sleep(K_MSEC(10));
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_connect_no_will)
{

	mqtt_sn_connect_no_will(client);
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_connect_will)
{
	static uint8_t willtopicreq[] = {2, 0x06};
	static uint8_t willmsgreq[] = {2, 0x08};
	static uint8_t connack[] = {3, 0x05, 0x00};

	int err;

	err = mqtt_sn_client_init(client, &client_id, &transport, evt_cb, tx, sizeof(tx), rx,
				  sizeof(rx));
	zassert_equal(err, 0, "unexpected error %d");

	client->will_topic = MQTT_SN_DATA_STRING_LITERAL("topic");
	client->will_msg = MQTT_SN_DATA_STRING_LITERAL("msg");

	err = mqtt_sn_connect(client, true, false);
	zassert_equal(err, 0, "unexpected error %d");
	assert_msg_send(1, 12);
	zassert_equal(client->state, 0, "Wrong state");

	err = input(client, willtopicreq, sizeof(willtopicreq));
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(client->state, 0, "Wrong state");
	assert_msg_send(1, 8);

	err = input(client, willmsgreq, sizeof(willmsgreq));
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(client->state, 0, "Wrong state");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");
	assert_msg_send(1, 5);

	err = input(client, connack, sizeof(connack));
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(client->state, 1, "Wrong state");
	zassert_equal(evt_cb_data.called, 1, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_CONNECTED, "Wrong event");
	k_sleep(K_MSEC(10));
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_publish_qos0)
{
	struct mqtt_sn_data data = MQTT_SN_DATA_STRING_LITERAL("Hello, World!");
	struct mqtt_sn_data topic = MQTT_SN_DATA_STRING_LITERAL("zephyr");
	/* registration ack with topic ID 0x1A1B, msg ID 0x0001, return code accepted */
	uint8_t regack[] = {7, 0x0B, 0x1A, 0x1B, 0x00, 0x01, 0};
	int err;

	mqtt_sn_connect_no_will(client);
	err = mqtt_sn_publish(client, MQTT_SN_QOS_0, &topic, false, &data);
	zassert_equal(err, 0, "Unexpected error %d", err);

	assert_msg_send(0, 0);
	k_sleep(K_MSEC(10));
	/* Expect a REGISTER to be sent */
	assert_msg_send(1, 12);
	err = input(client, regack, sizeof(regack));
	zassert_equal(err, 0, "unexpected error %d");
	assert_msg_send(0, 0);
	k_sleep(K_MSEC(10));
	assert_msg_send(1, 20);

	zassert_true(sys_slist_is_empty(&client->publish), "Publish not empty");
	zassert_false(sys_slist_is_empty(&client->topic), "Topic empty");
}

ZTEST_SUITE(mqtt_sn_client, NULL, NULL, setup, NULL, NULL);
