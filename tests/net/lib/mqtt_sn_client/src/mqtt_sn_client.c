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
static const struct mqtt_sn_data client2_id = MQTT_SN_DATA_STRING_LITERAL("zephyr2");
static const uint8_t gw_id = 12;
static const struct mqtt_sn_data gw_addr = MQTT_SN_DATA_STRING_LITERAL("gw1");

static uint8_t tx[255];
static uint8_t rx[255];

static struct msg_send_data {
	int called;
	size_t msg_sz;
	int ret;
	const void *dest_addr;
	size_t addrlen;
	struct mqtt_sn_client *client;
} msg_send_data;

struct k_sem mqtt_sn_tx_sem;
struct k_sem mqtt_sn_rx_sem;
struct k_sem mqtt_sn_cb_sem;

int mqtt_sn_data_cmp(struct mqtt_sn_data data1, struct mqtt_sn_data data2)
{
	return data1.size == data2.size && strncmp(data1.data, data2.data, data1.size);
}

static int msg_sendto(struct mqtt_sn_client *client, void *buf, size_t sz, const void *dest_addr,
		      size_t addrlen)
{
	msg_send_data.called++;
	msg_send_data.msg_sz = sz;
	msg_send_data.client = client;
	msg_send_data.dest_addr = dest_addr;
	msg_send_data.addrlen = addrlen;

	k_sem_give(&mqtt_sn_tx_sem);

	return msg_send_data.ret;
}

static void assert_msg_send(int called, size_t msg_sz, const struct mqtt_sn_data *dest_addr)
{
	zassert_equal(msg_send_data.called, called, "msg_send called %d times instead of %d",
		      msg_send_data.called, called);
	zassert_equal(msg_send_data.msg_sz, msg_sz, "msg_sz is %zu instead of %zu",
		      msg_send_data.msg_sz, msg_sz);
	if (dest_addr != NULL) {
		zassert_equal(mqtt_sn_data_cmp(*dest_addr,
					       *((struct mqtt_sn_data *)msg_send_data.dest_addr)),
			      0, "Addresses incorrect");
	}

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

	k_sem_give(&mqtt_sn_cb_sem);
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
	const void *src_addr;
	size_t addrlen;
} recvfrom_data;

static ssize_t tp_recvfrom(struct mqtt_sn_client *client, void *buffer, size_t length,
			   void *src_addr, size_t *addrlen)
{
	if (recvfrom_data.data && recvfrom_data.sz > 0 && length >= recvfrom_data.sz) {
		memcpy(buffer, recvfrom_data.data, recvfrom_data.sz);
		memcpy(src_addr, recvfrom_data.src_addr, recvfrom_data.addrlen);
		*addrlen = recvfrom_data.addrlen;

		k_sem_give(&mqtt_sn_rx_sem);
	}

	return recvfrom_data.sz;
}

int tp_poll(struct mqtt_sn_client *client)
{
	return recvfrom_data.sz;
}

static ZTEST_BMEM struct mqtt_sn_client mqtt_clients[8];
static ZTEST_BMEM struct mqtt_sn_client *mqtt_client;

static void setup(void *f)
{
	ARG_UNUSED(f);
	static ZTEST_BMEM size_t i;

	mqtt_client = &mqtt_clients[i++];

	transport = (struct mqtt_sn_transport){
		.init = tp_init, .sendto = msg_sendto, .recvfrom = tp_recvfrom, .poll = tp_poll};
	tp_initialized = false;

	memset(&evt_cb_data, 0, sizeof(evt_cb_data));
	memset(&msg_send_data, 0, sizeof(msg_send_data));
	memset(&recvfrom_data, 0, sizeof(recvfrom_data));
	k_sem_init(&mqtt_sn_tx_sem, 0, 1);
	k_sem_init(&mqtt_sn_rx_sem, 0, 1);
	k_sem_init(&mqtt_sn_cb_sem, 0, 1);
}

static int input(struct mqtt_sn_client *client, void *buf, size_t sz,
		 const struct mqtt_sn_data *src_addr)
{
	recvfrom_data.data = buf;
	recvfrom_data.sz = sz;
	recvfrom_data.src_addr = src_addr->data;
	recvfrom_data.addrlen = src_addr->size;

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

	err = mqtt_sn_add_gw(client, gw_id, gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");
	zassert_false(sys_slist_is_empty(&client->gateway), "GW not saved.");

	err = mqtt_sn_connect(client, false, false);
	zassert_equal(err, 0, "unexpected error %d");
	assert_msg_send(1, 12, &gw_addr);
	zassert_equal(client->state, 0, "Wrong state");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");

	err = input(client, connack, sizeof(connack), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(client->state, 1, "Wrong state");
	zassert_equal(evt_cb_data.called, 1, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_CONNECTED, "Wrong event");
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_handle_advertise)
{
	static uint8_t advertise[] = {5, 0x00, 0x0c, 0, 1};
	static uint8_t connack[] = {3, 0x05, 0x00};
	int err;

	err = mqtt_sn_client_init(mqtt_client, &client_id, &transport, evt_cb, tx, sizeof(tx), rx,
				  sizeof(rx));
	zassert_equal(err, 0, "unexpected error %d");

	err = input(mqtt_client, advertise, sizeof(advertise), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_false(sys_slist_is_empty(&mqtt_client->gateway), "GW not saved.");
	zassert_equal(evt_cb_data.called, 1, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_ADVERTISE, "Wrong event");

	err = input(mqtt_client, advertise, sizeof(advertise), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_false(sys_slist_is_empty(&mqtt_client->gateway), "GW not saved.");
	zassert_equal(sys_slist_len(&mqtt_client->gateway), 1, "Too many Gateways stored.");
	zassert_equal(evt_cb_data.called, 2, "Unexpected event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_ADVERTISE, "Wrong event");

	err = mqtt_sn_connect(mqtt_client, false, false);
	zassert_equal(err, 0, "unexpected error %d");
	assert_msg_send(1, 12, &gw_addr);
	zassert_equal(mqtt_client->state, 0, "Wrong state");
	zassert_equal(evt_cb_data.called, 2, "Unexpected event");

	err = input(mqtt_client, connack, sizeof(connack), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(mqtt_client->state, 1, "Wrong state");
	zassert_equal(evt_cb_data.called, 3, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_CONNECTED, "Wrong event");

	err = k_sem_take(&mqtt_sn_cb_sem, K_NO_WAIT);
	err = k_sem_take(&mqtt_sn_cb_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Timed out waiting for callback.");

	zassert_true(sys_slist_is_empty(&mqtt_client->gateway), "GW not cleared on timeout");
	zassert_equal(evt_cb_data.called, 4, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_DISCONNECTED, "Wrong event");
	zassert_equal(mqtt_client->state, 0, "Wrong state");

	mqtt_sn_client_deinit(mqtt_client);
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_add_gw)
{
	int err;

	err = mqtt_sn_client_init(mqtt_client, &client_id, &transport, evt_cb, tx, sizeof(tx), rx,
				  sizeof(rx));
	zassert_equal(err, 0, "unexpected error %d");

	err = mqtt_sn_add_gw(mqtt_client, gw_id, gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_false(sys_slist_is_empty(&mqtt_client->gateway), "GW not saved.");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");

	mqtt_sn_client_deinit(mqtt_client);
}

/* Test send SEARCHGW and GW response */
static ZTEST(mqtt_sn_client, test_mqtt_sn_search_gw)
{
	int err;
	static uint8_t gwinfo[3];

	gwinfo[0] = 3;
	gwinfo[1] = 0x02;
	gwinfo[2] = gw_id;

	err = mqtt_sn_client_init(mqtt_client, &client_id, &transport, evt_cb, tx, sizeof(tx), rx,
				  sizeof(rx));
	zassert_equal(err, 0, "unexpected error %d");

	err = k_sem_take(&mqtt_sn_tx_sem, K_NO_WAIT);
	err = mqtt_sn_search(mqtt_client, 1);
	zassert_equal(err, 0, "unexpected error %d");

	err = k_sem_take(&mqtt_sn_tx_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Timed out waiting for callback.");

	assert_msg_send(1, 3, NULL);
	zassert_equal(mqtt_client->state, 0, "Wrong state");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");

	err = input(mqtt_client, gwinfo, sizeof(gwinfo), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_false(sys_slist_is_empty(&mqtt_client->gateway), "GW not saved.");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_GWINFO, "Wrong event");

	mqtt_sn_client_deinit(mqtt_client);
}

/* Test send SEARCHGW and peer response */
static ZTEST(mqtt_sn_client, test_mqtt_sn_search_peer)
{
	int err;
	static uint8_t gwinfo[3 + 3];

	gwinfo[0] = 3 + gw_addr.size;
	gwinfo[1] = 0x02;
	gwinfo[2] = gw_id;
	memcpy(&gwinfo[3], gw_addr.data, 3);

	err = mqtt_sn_client_init(mqtt_client, &client_id, &transport, evt_cb, tx, sizeof(tx), rx,
				  sizeof(rx));
	zassert_equal(err, 0, "unexpected error %d");

	err = k_sem_take(&mqtt_sn_tx_sem, K_NO_WAIT);
	err = mqtt_sn_search(mqtt_client, 1);
	zassert_equal(err, 0, "unexpected error %d");

	err = k_sem_take(&mqtt_sn_tx_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Timed out waiting for callback.");

	assert_msg_send(1, 3, NULL);
	zassert_equal(mqtt_client->state, 0, "Wrong state");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");

	err = input(mqtt_client, gwinfo, sizeof(gwinfo), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_false(sys_slist_is_empty(&mqtt_client->gateway), "GW not saved.");
	zassert_equal(evt_cb_data.called, 1, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_GWINFO, "Wrong event");

	mqtt_sn_client_deinit(mqtt_client);
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_respond_searchgw)
{
	int err;
	static uint8_t searchgw[] = {3, 0x01, 1};

	err = mqtt_sn_client_init(mqtt_client, &client_id, &transport, evt_cb, tx, sizeof(tx), rx,
				  sizeof(rx));
	zassert_equal(err, 0, "unexpected error %d");

	err = mqtt_sn_add_gw(mqtt_client, gw_id, gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_false(sys_slist_is_empty(&mqtt_client->gateway), "GW not saved.");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");

	err = k_sem_take(&mqtt_sn_tx_sem, K_NO_WAIT);
	err = input(mqtt_client, searchgw, sizeof(searchgw), &client2_id);
	zassert_equal(err, 0, "unexpected error %d");

	err = k_sem_take(&mqtt_sn_tx_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Timed out waiting for callback.");

	zassert_equal(evt_cb_data.called, 1, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_SEARCHGW, "Wrong event");
	assert_msg_send(1, 3 + gw_addr.size, NULL);

	mqtt_sn_client_deinit(mqtt_client);
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_connect_no_will)
{
	mqtt_sn_connect_no_will(mqtt_client);
	mqtt_sn_client_deinit(mqtt_client);
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_connect_will)
{
	static uint8_t willtopicreq[] = {2, 0x06};
	static uint8_t willmsgreq[] = {2, 0x08};
	static uint8_t connack[] = {3, 0x05, 0x00};

	int err;

	err = mqtt_sn_client_init(mqtt_client, &client_id, &transport, evt_cb, tx, sizeof(tx), rx,
				  sizeof(rx));
	zassert_equal(err, 0, "unexpected error %d");

	err = mqtt_sn_add_gw(mqtt_client, gw_id, gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_false(sys_slist_is_empty(&mqtt_client->gateway), "GW not saved.");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");

	mqtt_client->will_topic = MQTT_SN_DATA_STRING_LITERAL("topic");
	mqtt_client->will_msg = MQTT_SN_DATA_STRING_LITERAL("msg");

	err = mqtt_sn_connect(mqtt_client, true, false);
	zassert_equal(err, 0, "unexpected error %d");
	assert_msg_send(1, 12, &gw_addr);
	zassert_equal(mqtt_client->state, 0, "Wrong state");

	err = input(mqtt_client, willtopicreq, sizeof(willtopicreq), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(mqtt_client->state, 0, "Wrong state");
	assert_msg_send(1, 8, &gw_addr);

	err = input(mqtt_client, willmsgreq, sizeof(willmsgreq), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(mqtt_client->state, 0, "Wrong state");
	zassert_equal(evt_cb_data.called, 0, "Unexpected event");
	assert_msg_send(1, 5, &gw_addr);

	err = input(mqtt_client, connack, sizeof(connack), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	zassert_equal(mqtt_client->state, 1, "Wrong state");
	zassert_equal(evt_cb_data.called, 1, "NO event");
	zassert_equal(evt_cb_data.last_evt.type, MQTT_SN_EVT_CONNECTED, "Wrong event");

	mqtt_sn_client_deinit(mqtt_client);
}

static ZTEST(mqtt_sn_client, test_mqtt_sn_publish_qos0)
{
	struct mqtt_sn_data data = MQTT_SN_DATA_STRING_LITERAL("Hello, World!");
	struct mqtt_sn_data topic = MQTT_SN_DATA_STRING_LITERAL("zephyr");
	/* registration ack with topic ID 0x1A1B, msg ID 0x0001, return code accepted */
	uint8_t regack[] = {7, 0x0B, 0x1A, 0x1B, 0x00, 0x01, 0};
	int err;

	mqtt_sn_connect_no_will(mqtt_client);
	err = k_sem_take(&mqtt_sn_tx_sem, K_NO_WAIT);
	err = mqtt_sn_publish(mqtt_client, MQTT_SN_QOS_0, &topic, false, &data);
	zassert_equal(err, 0, "Unexpected error %d", err);

	assert_msg_send(0, 0, NULL);

	/* Expect a REGISTER to be sent */
	err = k_sem_take(&mqtt_sn_tx_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Timed out waiting for callback.");
	assert_msg_send(1, 12, &gw_addr);
	err = input(mqtt_client, regack, sizeof(regack), &gw_addr);
	zassert_equal(err, 0, "unexpected error %d");
	err = k_sem_take(&mqtt_sn_tx_sem, K_NO_WAIT);
	assert_msg_send(0, 0, NULL);
	err = k_sem_take(&mqtt_sn_tx_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Timed out waiting for callback.");
	assert_msg_send(1, 20, &gw_addr);

	zassert_true(sys_slist_is_empty(&mqtt_client->publish), "Publish not empty");
	zassert_false(sys_slist_is_empty(&mqtt_client->topic), "Topic empty");

	mqtt_sn_client_deinit(mqtt_client);
}

ZTEST_SUITE(mqtt_sn_client, NULL, NULL, setup, NULL, NULL);
