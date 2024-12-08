/*
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_sn.c
 *
 * @brief MQTT-SN Client API Implementation.
 */

#include "mqtt_sn_msg.h"

#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/net/mqtt_sn.h>
LOG_MODULE_REGISTER(net_mqtt_sn, CONFIG_MQTT_SN_LOG_LEVEL);

#define MQTT_SN_NET_BUFS (CONFIG_MQTT_SN_LIB_MAX_PUBLISH)

NET_BUF_POOL_FIXED_DEFINE(mqtt_sn_messages, MQTT_SN_NET_BUFS, CONFIG_MQTT_SN_LIB_MAX_PAYLOAD_SIZE,
			  0, NULL);

struct mqtt_sn_confirmable {
	int64_t last_attempt;
	uint16_t msg_id;
	uint8_t retries;
};

struct mqtt_sn_publish {
	struct mqtt_sn_confirmable con;
	sys_snode_t next;
	struct mqtt_sn_topic *topic;
	uint8_t pubdata[CONFIG_MQTT_SN_LIB_MAX_PAYLOAD_SIZE];
	size_t datalen;
	enum mqtt_sn_qos qos;
	bool retain;
};

enum mqtt_sn_topic_state {
	MQTT_SN_TOPIC_STATE_REGISTERING,
	MQTT_SN_TOPIC_STATE_REGISTERED,
	MQTT_SN_TOPIC_STATE_SUBSCRIBING,
	MQTT_SN_TOPIC_STATE_SUBSCRIBED,
	MQTT_SN_TOPIC_STATE_UNSUBSCRIBING,
};

struct mqtt_sn_topic {
	struct mqtt_sn_confirmable con;
	sys_snode_t next;
	char name[CONFIG_MQTT_SN_LIB_MAX_TOPIC_SIZE];
	size_t namelen;
	uint16_t topic_id;
	enum mqtt_sn_qos qos;
	enum mqtt_sn_topic_type type;
	enum mqtt_sn_topic_state state;
};

struct mqtt_sn_gateway {
	sys_snode_t next;
	char gw_id;
	int64_t adv_timer;
	char addr[CONFIG_MQTT_SN_LIB_MAX_ADDR_SIZE];
	size_t addr_len;
};

K_MEM_SLAB_DEFINE_STATIC(publishes, sizeof(struct mqtt_sn_publish), CONFIG_MQTT_SN_LIB_MAX_PUBLISH,
			 4);
K_MEM_SLAB_DEFINE_STATIC(topics, sizeof(struct mqtt_sn_topic), CONFIG_MQTT_SN_LIB_MAX_TOPICS, 4);
K_MEM_SLAB_DEFINE_STATIC(gateways, sizeof(struct mqtt_sn_gateway), CONFIG_MQTT_SN_LIB_MAX_GATEWAYS,
			 4);

enum mqtt_sn_client_state {
	MQTT_SN_CLIENT_DISCONNECTED,
	MQTT_SN_CLIENT_ACTIVE,
	MQTT_SN_CLIENT_ASLEEP,
	MQTT_SN_CLIENT_AWAKE
};

static void mqtt_sn_set_state(struct mqtt_sn_client *client, enum mqtt_sn_client_state state)
{
	int prev_state = client->state;

	client->state = state;
	LOG_DBG("Client %p state (%d) -> (%d)", client, prev_state, state);
}

#define T_SEARCHGW_MSEC  (CONFIG_MQTT_SN_LIB_T_SEARCHGW * MSEC_PER_SEC)
#define T_GWINFO_MSEC    (CONFIG_MQTT_SN_LIB_T_GWINFO * MSEC_PER_SEC)
#define T_RETRY_MSEC     (CONFIG_MQTT_SN_LIB_T_RETRY * MSEC_PER_SEC)
#define N_RETRY          (CONFIG_MQTT_SN_LIB_N_RETRY)
#define T_KEEPALIVE_MSEC (CONFIG_MQTT_SN_KEEPALIVE * MSEC_PER_SEC)

static uint16_t next_msg_id(void)
{
	static uint16_t msg_id;

	return ++msg_id;
}

static int encode_and_send(struct mqtt_sn_client *client, struct mqtt_sn_param *p,
			   uint8_t broadcast_radius)
{
	int err;

	err = mqtt_sn_encode_msg(&client->tx, p);
	if (err) {
		goto end;
	}

	LOG_HEXDUMP_DBG(client->tx.data, client->tx.len, "Send message");

	if (!client->transport->sendto) {
		LOG_ERR("Can't send: no callback");
		err = -ENOTSUP;
		goto end;
	}

	if (!client->tx.len) {
		LOG_WRN("Can't send: empty");
		err = -ENOMSG;
		goto end;
	}

	if (broadcast_radius) {
		err = client->transport->sendto(client, client->tx.data, client->tx.len, NULL,
						broadcast_radius);
	} else {
		struct mqtt_sn_gateway *gw;

		gw = SYS_SLIST_PEEK_HEAD_CONTAINER(&client->gateway, gw, next);
		if (gw == NULL || gw->addr_len == 0) {
			LOG_WRN("No Gateway Address");
			err = -ENXIO;
			goto end;
		}
		err = client->transport->sendto(client, client->tx.data, client->tx.len, gw->addr,
						gw->addr_len);
	}

end:
	if (err) {
		LOG_ERR("Error during send: %d", err);
	}
	net_buf_simple_reset(&client->tx);

	return err;
}

static void mqtt_sn_con_init(struct mqtt_sn_confirmable *con)
{
	con->last_attempt = 0;
	con->retries = N_RETRY;
	con->msg_id = next_msg_id();
}

static void mqtt_sn_publish_destroy(struct mqtt_sn_client *client, struct mqtt_sn_publish *pub)
{
	sys_slist_find_and_remove(&client->publish, &pub->next);
	k_mem_slab_free(&publishes, (void *)pub);
}

static void mqtt_sn_publish_destroy_all(struct mqtt_sn_client *client)
{
	struct mqtt_sn_publish *pub;
	sys_snode_t *next;

	while ((next = sys_slist_get(&client->publish)) != NULL) {
		pub = SYS_SLIST_CONTAINER(next, pub, next);
		k_mem_slab_free(&publishes, (void *)pub);
	}
}

static struct mqtt_sn_publish *mqtt_sn_publish_create(struct mqtt_sn_data *data)
{
	struct mqtt_sn_publish *pub;

	if (k_mem_slab_alloc(&publishes, (void **)&pub, K_NO_WAIT)) {
		LOG_ERR("Can't create PUB: no free slot");
		return NULL;
	}

	memset(pub, 0, sizeof(*pub));

	if (data && data->data && data->size) {
		if (data->size > sizeof(pub->pubdata)) {
			LOG_ERR("Can't create PUB: Too much data (%zu)", data->size);
			return NULL;
		}

		memcpy(pub->pubdata, data->data, data->size);
		pub->datalen = data->size;
	}

	mqtt_sn_con_init(&pub->con);

	return pub;
}

static struct mqtt_sn_publish *mqtt_sn_publish_find_msg_id(struct mqtt_sn_client *client,
							   uint16_t msg_id)
{
	struct mqtt_sn_publish *pub;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->publish, pub, next) {
		if (pub->con.msg_id == msg_id) {
			return pub;
		}
	}

	return NULL;
}

static struct mqtt_sn_publish *mqtt_sn_publish_find_topic(struct mqtt_sn_client *client,
							  struct mqtt_sn_topic *topic)
{
	struct mqtt_sn_publish *pub;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->publish, pub, next) {
		if (pub->topic == topic) {
			return pub;
		}
	}

	return NULL;
}

static struct mqtt_sn_topic *mqtt_sn_topic_create(struct mqtt_sn_data *name)
{
	struct mqtt_sn_topic *topic;

	if (k_mem_slab_alloc(&topics, (void **)&topic, K_NO_WAIT)) {
		LOG_ERR("Can't create topic: no free slot");
		return NULL;
	}

	memset(topic, 0, sizeof(*topic));

	if (!name || !name->data || !name->size) {
		LOG_ERR("Can't create topic with empty name");
		return NULL;
	}

	if (name->size > sizeof(topic->name)) {
		LOG_ERR("Can't create topic: name too long (%zu)", name->size);
		return NULL;
	}

	memcpy(topic->name, name->data, name->size);
	topic->namelen = name->size;

	mqtt_sn_con_init(&topic->con);

	return topic;
}

static struct mqtt_sn_topic *mqtt_sn_topic_find_name(struct mqtt_sn_client *client,
						     struct mqtt_sn_data *topic_name)
{
	struct mqtt_sn_topic *topic;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->topic, topic, next) {
		if (topic->namelen == topic_name->size &&
		    memcmp(topic->name, topic_name->data, topic_name->size) == 0) {
			return topic;
		}
	}

	return NULL;
}

static struct mqtt_sn_topic *mqtt_sn_topic_find_msg_id(struct mqtt_sn_client *client,
						       uint16_t msg_id)
{
	struct mqtt_sn_topic *topic;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->topic, topic, next) {
		if (topic->con.msg_id == msg_id) {
			return topic;
		}
	}

	return NULL;
}

static void mqtt_sn_topic_destroy(struct mqtt_sn_client *client, struct mqtt_sn_topic *topic)
{
	struct mqtt_sn_publish *pub;

	/* Destroy all pubs referencing this topic */
	while ((pub = mqtt_sn_publish_find_topic(client, topic)) != NULL) {
		LOG_WRN("Destroying publish msg_id %d", pub->con.msg_id);
		mqtt_sn_publish_destroy(client, pub);
	}

	sys_slist_find_and_remove(&client->topic, &topic->next);
}

static void mqtt_sn_topic_destroy_all(struct mqtt_sn_client *client)
{
	struct mqtt_sn_topic *topic;
	struct mqtt_sn_publish *pub;
	sys_snode_t *next;

	while ((next = sys_slist_get(&client->topic)) != NULL) {
		topic = SYS_SLIST_CONTAINER(next, topic, next);
		/* Destroy all pubs referencing this topic */
		while ((pub = mqtt_sn_publish_find_topic(client, topic)) != NULL) {
			LOG_WRN("Destroying publish msg_id %d", pub->con.msg_id);
			mqtt_sn_publish_destroy(client, pub);
		}

		k_mem_slab_free(&topics, (void *)topic);
	}
}

static void mqtt_sn_gw_destroy(struct mqtt_sn_client *client, struct mqtt_sn_gateway *gw)
{
	LOG_DBG("Destroying gateway %d", gw->gw_id);
	sys_slist_find_and_remove(&client->gateway, &gw->next);
	k_mem_slab_free(&gateways, (void *)gw);
}

static void mqtt_sn_gw_destroy_all(struct mqtt_sn_client *client)
{
	struct mqtt_sn_gateway *gw;
	sys_snode_t *next;

	while ((next = sys_slist_get(&client->gateway)) != NULL) {
		gw = SYS_SLIST_CONTAINER(next, gw, next);
		sys_slist_find_and_remove(&client->gateway, next);
		k_mem_slab_free(&gateways, (void *)gw);
	}
}

static struct mqtt_sn_gateway *mqtt_sn_gw_create(uint8_t gw_id, short duration,
						 struct mqtt_sn_data gw_addr)
{
	struct mqtt_sn_gateway *gw;

	LOG_DBG("Free GW slots: %d", k_mem_slab_num_free_get(&gateways));
	if (k_mem_slab_alloc(&gateways, (void **)&gw, K_NO_WAIT)) {
		LOG_WRN("Can't create GW: no free slot");
		return NULL;
	}

	__ASSERT(gw_addr.size < CONFIG_MQTT_SN_LIB_MAX_ADDR_SIZE,
		 "Gateway address is larger than allowed by CONFIG_MQTT_SN_LIB_MAX_ADDR_SIZE");

	memset(gw, 0, sizeof(*gw));
	memcpy(gw->addr, gw_addr.data, gw_addr.size);
	gw->addr_len = gw_addr.size;
	gw->gw_id = gw_id;
	if (duration == -1) {
		gw->adv_timer = duration;
	} else {
		gw->adv_timer =
			k_uptime_get() + (duration * CONFIG_MQTT_SN_LIB_N_ADV * MSEC_PER_SEC);
	}

	return gw;
}

static struct mqtt_sn_gateway *mqtt_sn_gw_find_id(struct mqtt_sn_client *client, uint16_t gw_id)
{
	struct mqtt_sn_gateway *gw;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->gateway, gw, next) {
		if (gw->gw_id == gw_id) {
			return gw;
		}
	}

	return NULL;
}

static void mqtt_sn_disconnect_internal(struct mqtt_sn_client *client)
{
	struct mqtt_sn_evt evt = {.type = MQTT_SN_EVT_DISCONNECTED};

	mqtt_sn_set_state(client, MQTT_SN_CLIENT_DISCONNECTED);
	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}

	/*
	 * Remove all publishes, but keep topics
	 * Topics are removed on deinit or when connect is called with
	 * clean-session = true
	 */
	mqtt_sn_publish_destroy_all(client);

	k_work_cancel_delayable(&client->process_work);
}

static void mqtt_sn_sleep_internal(struct mqtt_sn_client *client)
{
	struct mqtt_sn_evt evt = {.type = MQTT_SN_EVT_DISCONNECTED};

	mqtt_sn_set_state(client, MQTT_SN_CLIENT_ASLEEP);
	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void mqtt_sn_do_subscribe(struct mqtt_sn_client *client, struct mqtt_sn_topic *topic,
				 bool dup)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_SUBSCRIBE};

	if (!client || !topic) {
		return;
	}

	if (client->state != MQTT_SN_CLIENT_ACTIVE) {
		LOG_ERR("Cannot subscribe: not connected");
		return;
	}

	p.params.subscribe.msg_id = topic->con.msg_id;
	p.params.subscribe.qos = topic->qos;
	p.params.subscribe.topic_type = topic->type;
	p.params.subscribe.dup = dup;
	switch (topic->type) {
	case MQTT_SN_TOPIC_TYPE_NORMAL:
		p.params.subscribe.topic.topic_name.data = topic->name;
		p.params.subscribe.topic.topic_name.size = topic->namelen;
		break;
	case MQTT_SN_TOPIC_TYPE_PREDEF:
	case MQTT_SN_TOPIC_TYPE_SHORT:
		p.params.subscribe.topic.topic_id = topic->topic_id;
		break;
	default:
		LOG_ERR("Unexpected topic type %d", topic->type);
		return;
	}

	encode_and_send(client, &p, 0);
}

static void mqtt_sn_do_unsubscribe(struct mqtt_sn_client *client, struct mqtt_sn_topic *topic)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_UNSUBSCRIBE};

	if (!client || !topic) {
		return;
	}

	if (client->state != MQTT_SN_CLIENT_ACTIVE) {
		LOG_ERR("Cannot unsubscribe: not connected");
		return;
	}

	p.params.unsubscribe.msg_id = topic->con.msg_id;
	p.params.unsubscribe.topic_type = topic->type;
	switch (topic->type) {
	case MQTT_SN_TOPIC_TYPE_NORMAL:
		p.params.unsubscribe.topic.topic_name.data = topic->name;
		p.params.unsubscribe.topic.topic_name.size = topic->namelen;
		break;
	case MQTT_SN_TOPIC_TYPE_PREDEF:
	case MQTT_SN_TOPIC_TYPE_SHORT:
		p.params.unsubscribe.topic.topic_id = topic->topic_id;
		break;
	default:
		LOG_ERR("Unexpected topic type %d", topic->type);
		return;
	}

	encode_and_send(client, &p, 0);
}

static void mqtt_sn_do_register(struct mqtt_sn_client *client, struct mqtt_sn_topic *topic)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_REGISTER};

	if (!client || !topic) {
		return;
	}

	if (client->state != MQTT_SN_CLIENT_ACTIVE) {
		LOG_ERR("Cannot register: not connected");
		return;
	}

	p.params.reg.msg_id = topic->con.msg_id;
	switch (topic->type) {
	case MQTT_SN_TOPIC_TYPE_NORMAL:
		LOG_HEXDUMP_INF(topic->name, topic->namelen, "Registering topic");
		p.params.reg.topic.data = topic->name;
		p.params.reg.topic.size = topic->namelen;
		break;
	default:
		LOG_ERR("Unexpected topic type %d", topic->type);
		return;
	}

	encode_and_send(client, &p, 0);
}

static void mqtt_sn_do_publish(struct mqtt_sn_client *client, struct mqtt_sn_publish *pub, bool dup)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_PUBLISH};

	if (!client || !pub) {
		return;
	}

	if (client->state != MQTT_SN_CLIENT_ACTIVE) {
		LOG_ERR("Cannot subscribe: not connected");
		return;
	}

	LOG_INF("Publishing to topic ID %d", pub->topic->topic_id);

	p.params.publish.data.data = pub->pubdata;
	p.params.publish.data.size = pub->datalen;
	p.params.publish.msg_id = pub->con.msg_id;
	p.params.publish.retain = pub->retain;
	p.params.publish.topic_id = pub->topic->topic_id;
	p.params.publish.topic_type = pub->topic->type;
	p.params.publish.qos = pub->qos;
	p.params.publish.dup = dup;

	encode_and_send(client, &p, 0);
}

static void mqtt_sn_do_searchgw(struct mqtt_sn_client *client)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_SEARCHGW};

	p.params.searchgw.radius = CONFIG_MQTT_SN_LIB_BROADCAST_RADIUS;

	encode_and_send(client, &p, CONFIG_MQTT_SN_LIB_BROADCAST_RADIUS);
}

static void mqtt_sn_do_gwinfo(struct mqtt_sn_client *client)
{
	struct mqtt_sn_param response = {.type = MQTT_SN_MSG_TYPE_GWINFO};
	struct mqtt_sn_gateway *gw;
	struct mqtt_sn_data addr;

	gw = SYS_SLIST_PEEK_HEAD_CONTAINER(&client->gateway, gw, next);

	if (gw == NULL || gw->addr_len == 0) {
		LOG_WRN("No Gateway Address");
		return;
	}

	response.params.gwinfo.gw_id = gw->gw_id;
	addr.data = gw->addr;
	addr.size = gw->addr_len;
	response.params.gwinfo.gw_add = addr;

	encode_and_send(client, &response, client->radius_gwinfo);
}

static void mqtt_sn_do_ping(struct mqtt_sn_client *client)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_PINGREQ};

	switch (client->state) {
	case MQTT_SN_CLIENT_ASLEEP:
		/*
		 * From the spec regarding PINGREQ:
		 * ClientId: contains the client id; this field is optional
		 * and is included by a “sleeping” client when it goes to the
		 * “awake” state and is waiting for messages sent by the
		 * server/gateway, see Section 6.14 for further details.
		 */
		p.params.pingreq.client_id.data = client->client_id.data;
		p.params.pingreq.client_id.size = client->client_id.size;
	case MQTT_SN_CLIENT_ACTIVE:
		encode_and_send(client, &p, 0);
		break;
	default:
		LOG_WRN("Can't ping in state %d", client->state);
		break;
	}
}

static int process_pubs(struct mqtt_sn_client *client, int64_t *next_cycle)
{
	struct mqtt_sn_publish *pub, *pubs;
	const int64_t now = k_uptime_get();
	int64_t next_attempt;
	bool dup;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&client->publish, pub, pubs, next) {
		LOG_HEXDUMP_DBG(pub->topic->name, pub->topic->namelen,
				"Processing publish for topic");
		LOG_HEXDUMP_DBG(pub->pubdata, pub->datalen, "Processing publish data");

		if (pub->con.last_attempt == 0) {
			next_attempt = 0;
			dup = false;
		} else {
			next_attempt = pub->con.last_attempt + T_RETRY_MSEC;
			dup = true;
		}

		if (next_attempt <= now) {
			switch (pub->topic->state) {
			case MQTT_SN_TOPIC_STATE_REGISTERING:
			case MQTT_SN_TOPIC_STATE_SUBSCRIBING:
			case MQTT_SN_TOPIC_STATE_UNSUBSCRIBING:
				LOG_INF("Can't publish; topic is not ready");
				break;
			case MQTT_SN_TOPIC_STATE_REGISTERED:
			case MQTT_SN_TOPIC_STATE_SUBSCRIBED:
				if (!pub->con.retries--) {
					LOG_WRN("PUB ran out of retries, disconnecting");
					mqtt_sn_disconnect_internal(client);
					return -ETIMEDOUT;
				}
				mqtt_sn_do_publish(client, pub, dup);
				if (pub->qos == MQTT_SN_QOS_0 || pub->qos == MQTT_SN_QOS_M1) {
					/* We are done, remove this */
					mqtt_sn_publish_destroy(client, pub);
					continue;
				} else {
					/* Wait for ack */
					pub->con.last_attempt = now;
					next_attempt = now + T_RETRY_MSEC;
				}
				break;
			}
		}

		if (next_attempt > now && (*next_cycle == 0 || next_attempt < *next_cycle)) {
			*next_cycle = next_attempt;
		}
	}

	LOG_DBG("next_cycle: %lld", *next_cycle);

	return 0;
}

static int process_topics(struct mqtt_sn_client *client, int64_t *next_cycle)
{
	struct mqtt_sn_topic *topic;
	const int64_t now = k_uptime_get();
	int64_t next_attempt;
	bool dup;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->topic, topic, next) {
		LOG_HEXDUMP_DBG(topic->name, topic->namelen, "Processing topic");

		if (topic->con.last_attempt == 0) {
			next_attempt = 0;
			dup = false;
		} else {
			next_attempt = topic->con.last_attempt + T_RETRY_MSEC;
			dup = true;
		}

		if (next_attempt <= now) {
			switch (topic->state) {
			case MQTT_SN_TOPIC_STATE_SUBSCRIBING:
				if (!topic->con.retries--) {
					LOG_WRN("Topic ran out of retries, disconnecting");
					mqtt_sn_disconnect_internal(client);
					return -ETIMEDOUT;
				}

				mqtt_sn_do_subscribe(client, topic, dup);
				topic->con.last_attempt = now;
				next_attempt = now + T_RETRY_MSEC;
				break;
			case MQTT_SN_TOPIC_STATE_REGISTERING:
				if (!topic->con.retries--) {
					LOG_WRN("Topic ran out of retries, disconnecting");
					mqtt_sn_disconnect_internal(client);
					return -ETIMEDOUT;
				}

				mqtt_sn_do_register(client, topic);
				topic->con.last_attempt = now;
				next_attempt = now + T_RETRY_MSEC;
				break;
			case MQTT_SN_TOPIC_STATE_UNSUBSCRIBING:
				if (!topic->con.retries--) {
					LOG_WRN("Topic ran out of retries, disconnecting");
					mqtt_sn_disconnect_internal(client);
					return -ETIMEDOUT;
				}
				mqtt_sn_do_unsubscribe(client, topic);
				topic->con.last_attempt = now;
				next_attempt = now + T_RETRY_MSEC;
				break;
			case MQTT_SN_TOPIC_STATE_REGISTERED:
			case MQTT_SN_TOPIC_STATE_SUBSCRIBED:
				break;
			}
		}

		if (next_attempt > now && (*next_cycle == 0 || next_attempt < *next_cycle)) {
			*next_cycle = next_attempt;
		}
	}

	LOG_DBG("next_cycle: %lld", *next_cycle);

	return 0;
}

static int process_ping(struct mqtt_sn_client *client, int64_t *next_cycle)
{
	const int64_t now = k_uptime_get();
	struct mqtt_sn_gateway *gw = NULL;
	int64_t next_ping;

	if (client->ping_retries == N_RETRY) {
		/* Last ping was acked */
		next_ping = client->last_ping + T_KEEPALIVE_MSEC;
	} else {
		next_ping = client->last_ping + T_RETRY_MSEC;
	}

	if (next_ping < now) {
		if (!client->ping_retries--) {
			LOG_WRN("Ping ran out of retries");
			mqtt_sn_disconnect_internal(client);
			SYS_SLIST_PEEK_HEAD_CONTAINER(&client->gateway, gw, next);
			LOG_DBG("Removing non-responsive GW 0x%08x", gw->gw_id);
			mqtt_sn_gw_destroy(client, gw);
			return -ETIMEDOUT;
		}

		LOG_DBG("Sending PINGREQ");
		mqtt_sn_do_ping(client);
		client->last_ping = now;
		next_ping = now + T_RETRY_MSEC;
	}

	if (*next_cycle == 0 || next_ping < *next_cycle) {
		*next_cycle = next_ping;
	}

	LOG_DBG("next_cycle: %lld", *next_cycle);

	return 0;
}

static int process_search(struct mqtt_sn_client *client, int64_t *next_cycle)
{
	const int64_t now = k_uptime_get();

	LOG_DBG("ts_searchgw: %lld", client->ts_searchgw);
	LOG_DBG("ts_gwinfo: %lld", client->ts_gwinfo);

	if (client->ts_searchgw != 0 && client->ts_searchgw <= now) {
		LOG_DBG("Sending SEARCHGW");
		mqtt_sn_do_searchgw(client);
		client->ts_searchgw = 0;
	}

	if (client->ts_gwinfo != 0 && client->ts_gwinfo <= now) {
		LOG_DBG("Sending GWINFO");
		mqtt_sn_do_gwinfo(client);
		client->ts_gwinfo = 0;
	}

	if (*next_cycle == 0 || (client->ts_searchgw != 0 && client->ts_searchgw < *next_cycle)) {
		*next_cycle = client->ts_searchgw;
	}
	if (*next_cycle == 0 || (client->ts_gwinfo != 0 && client->ts_gwinfo < *next_cycle)) {
		*next_cycle = client->ts_gwinfo;
	}

	LOG_DBG("next_cycle: %lld", *next_cycle);

	return 0;
}

static int process_advertise(struct mqtt_sn_client *client, int64_t *next_cycle)
{
	const int64_t now = k_uptime_get();
	struct mqtt_sn_gateway *gw;
	struct mqtt_sn_gateway *gw_next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&client->gateway, gw, gw_next, next) {
		LOG_DBG("Checking if GW 0x%02x is old", gw->gw_id);
		if (gw->adv_timer != -1 && gw->adv_timer <= now) {
			LOG_DBG("Removing non-responsive GW 0x%08x", gw->gw_id);
			if (client->gateway.head == &gw->next) {
				mqtt_sn_disconnect(client);
			}
			mqtt_sn_gw_destroy(client, gw);
		}
		if (gw->adv_timer != -1 && (*next_cycle == 0 || gw->adv_timer < *next_cycle)) {
			*next_cycle = gw->adv_timer;
		}
	}
	LOG_DBG("next_cycle: %lld", *next_cycle);

	return 0;
}

static void process_work(struct k_work *wrk)
{
	struct mqtt_sn_client *client;
	struct k_work_delayable *dwork;
	int64_t next_cycle = 0;
	int err;

	dwork = k_work_delayable_from_work(wrk);
	client = CONTAINER_OF(dwork, struct mqtt_sn_client, process_work);

	LOG_DBG("Executing work of client %p in state %d at time %lld", client, client->state,
		k_uptime_get());

	/* Clean up old advertised gateways from list */
	err = process_advertise(client, &next_cycle);
	if (err) {
		return;
	}

	/* Handle GW search process timers */
	err = process_search(client, &next_cycle);
	if (err) {
		return;
	}

	if (client->state == MQTT_SN_CLIENT_ACTIVE) {
		err = process_topics(client, &next_cycle);
		if (err) {
			return;
		}

		err = process_pubs(client, &next_cycle);
		if (err) {
			return;
		}

		err = process_ping(client, &next_cycle);
		if (err) {
			return;
		}
	}

	if (next_cycle > 0) {
		LOG_DBG("next_cycle: %lld", next_cycle);
		k_work_schedule(dwork, K_MSEC(next_cycle - k_uptime_get()));
	}
}

int mqtt_sn_client_init(struct mqtt_sn_client *client, const struct mqtt_sn_data *client_id,
			struct mqtt_sn_transport *transport, mqtt_sn_evt_cb_t evt_cb, void *tx,
			size_t txsz, void *rx, size_t rxsz)
{
	if (!client || !client_id || !transport || !evt_cb || !tx || !rx) {
		return -EINVAL;
	}

	memset(client, 0, sizeof(*client));

	client->client_id.data = client_id->data;
	client->client_id.size = client_id->size;
	client->transport = transport;
	client->evt_cb = evt_cb;

	net_buf_simple_init_with_data(&client->tx, tx, txsz);
	net_buf_simple_reset(&client->tx);

	net_buf_simple_init_with_data(&client->rx, rx, rxsz);
	net_buf_simple_reset(&client->rx);

	k_work_init_delayable(&client->process_work, process_work);

	if (transport->init) {
		transport->init(transport);
	}

	return 0;
}

void mqtt_sn_client_deinit(struct mqtt_sn_client *client)
{
	if (!client) {
		return;
	}

	mqtt_sn_publish_destroy_all(client);
	mqtt_sn_topic_destroy_all(client);
	mqtt_sn_gw_destroy_all(client);

	if (client->transport && client->transport->deinit) {
		client->transport->deinit(client->transport);
	}

	k_work_cancel_delayable(&client->process_work);
}

int mqtt_sn_add_gw(struct mqtt_sn_client *client, uint8_t gw_id, struct mqtt_sn_data gw_addr)
{
	struct mqtt_sn_gateway *gw;

	gw = mqtt_sn_gw_find_id(client, gw_id);

	if (gw != NULL) {
		mqtt_sn_gw_destroy(client, gw);
	}

	gw = mqtt_sn_gw_create(gw_id, -1, gw_addr);
	if (!gw) {
		return -ENOMEM;
	}

	sys_slist_append(&client->gateway, &gw->next);

	return 0;
}

int mqtt_sn_search(struct mqtt_sn_client *client, uint8_t radius)
{
	if (!client) {
		return -EINVAL;
	}
	/* Set SEARCHGW transmission timer */
	client->ts_searchgw = k_uptime_get() + (T_SEARCHGW_MSEC * sys_rand8_get() / 255);
	k_work_schedule(&client->process_work, K_NO_WAIT);
	LOG_DBG("Requested SEARCHGW for time %lld at time %lld", client->ts_searchgw,
		k_uptime_get());

	return 0;
}

int mqtt_sn_connect(struct mqtt_sn_client *client, bool will, bool clean_session)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_CONNECT};

	if (!client) {
		return -EINVAL;
	}

	if (will && (!client->will_msg.data || !client->will_topic.data)) {
		LOG_ERR("will set to true, but no will data in client");
		return -EINVAL;
	}

	if (clean_session) {
		mqtt_sn_topic_destroy_all(client);
	}

	p.params.connect.clean_session = clean_session;
	p.params.connect.will = will;
	p.params.connect.duration = CONFIG_MQTT_SN_KEEPALIVE;
	p.params.connect.client_id.data = client->client_id.data;
	p.params.connect.client_id.size = client->client_id.size;

	client->last_ping = k_uptime_get();

	return encode_and_send(client, &p, 0);
}

int mqtt_sn_disconnect(struct mqtt_sn_client *client)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_DISCONNECT};
	int err;

	if (!client) {
		return -EINVAL;
	}

	p.params.disconnect.duration = 0;

	err = encode_and_send(client, &p, 0);
	mqtt_sn_disconnect_internal(client);

	return err;
}

int mqtt_sn_sleep(struct mqtt_sn_client *client, uint16_t duration)
{
	struct mqtt_sn_param p = {.type = MQTT_SN_MSG_TYPE_DISCONNECT};
	int err;

	if (!client || !duration) {
		return -EINVAL;
	}

	p.params.disconnect.duration = duration;

	err = encode_and_send(client, &p, 0);
	mqtt_sn_sleep_internal(client);

	return err;
}

int mqtt_sn_subscribe(struct mqtt_sn_client *client, enum mqtt_sn_qos qos,
		      struct mqtt_sn_data *topic_name)
{
	struct mqtt_sn_topic *topic;
	int err;

	if (!client || !topic_name || !topic_name->data || !topic_name->size) {
		return -EINVAL;
	}

	if (client->state != MQTT_SN_CLIENT_ACTIVE) {
		LOG_ERR("Cannot subscribe: not connected");
		return -ENOTCONN;
	}

	topic = mqtt_sn_topic_find_name(client, topic_name);
	if (!topic) {
		topic = mqtt_sn_topic_create(topic_name);
		if (!topic) {
			return -ENOMEM;
		}

		topic->qos = qos;
		topic->state = MQTT_SN_TOPIC_STATE_SUBSCRIBING;
		sys_slist_append(&client->topic, &topic->next);
	}

	err = k_work_reschedule(&client->process_work, K_NO_WAIT);
	if (err < 0) {
		return err;
	}

	return 0;
}

int mqtt_sn_unsubscribe(struct mqtt_sn_client *client, enum mqtt_sn_qos qos,
			struct mqtt_sn_data *topic_name)
{
	struct mqtt_sn_topic *topic;
	int err;

	if (!client || !topic_name) {
		return -EINVAL;
	}

	if (client->state != MQTT_SN_CLIENT_ACTIVE) {
		LOG_ERR("Cannot unsubscribe: not connected");
		return -ENOTCONN;
	}

	topic = mqtt_sn_topic_find_name(client, topic_name);
	if (!topic) {
		LOG_HEXDUMP_ERR(topic_name->data, topic_name->size, "Topic not found");
		return -ENOENT;
	}

	topic->state = MQTT_SN_TOPIC_STATE_UNSUBSCRIBING;
	mqtt_sn_con_init(&topic->con);

	err = k_work_reschedule(&client->process_work, K_NO_WAIT);
	if (err < 0) {
		return err;
	}

	return 0;
}

int mqtt_sn_publish(struct mqtt_sn_client *client, enum mqtt_sn_qos qos,
		    struct mqtt_sn_data *topic_name, bool retain, struct mqtt_sn_data *data)
{
	struct mqtt_sn_publish *pub;
	struct mqtt_sn_topic *topic;
	int err;

	if (!client || !topic_name) {
		return -EINVAL;
	}

	if (qos == MQTT_SN_QOS_M1) {
		LOG_ERR("QoS -1 not supported");
		return -ENOTSUP;
	}

	if (client->state != MQTT_SN_CLIENT_ACTIVE) {
		LOG_ERR("Cannot publish: disconnected");
		return -ENOTCONN;
	}

	topic = mqtt_sn_topic_find_name(client, topic_name);
	if (!topic) {
		topic = mqtt_sn_topic_create(topic_name);
		if (!topic) {
			return -ENOMEM;
		}

		topic->qos = qos;
		topic->state = MQTT_SN_TOPIC_STATE_REGISTERING;
		sys_slist_append(&client->topic, &topic->next);
	}

	pub = mqtt_sn_publish_create(data);
	if (!pub) {
		k_work_reschedule(&client->process_work, K_NO_WAIT);
		return -ENOMEM;
	}

	pub->qos = qos;
	pub->retain = retain;
	pub->topic = topic;

	sys_slist_append(&client->publish, &pub->next);

	err = k_work_reschedule(&client->process_work, K_NO_WAIT);
	if (err < 0) {
		return err;
	}

	return 0;
}

static void handle_advertise(struct mqtt_sn_client *client, struct mqtt_sn_param_advertise *p,
			     struct mqtt_sn_data rx_addr)
{
	struct mqtt_sn_evt evt = {.type = MQTT_SN_EVT_ADVERTISE};
	struct mqtt_sn_gateway *gw;

	gw = mqtt_sn_gw_find_id(client, p->gw_id);

	if (gw == NULL) {
		LOG_DBG("Creating GW 0x%02x with duration %d", p->gw_id, p->duration);
		gw = mqtt_sn_gw_create(p->gw_id, p->duration, rx_addr);
		if (!gw) {
			return;
		}
		sys_slist_append(&client->gateway, &gw->next);
	} else {
		LOG_DBG("Updating timer for GW 0x%02x with duration %d", p->gw_id, p->duration);
		gw->adv_timer =
			k_uptime_get() + (p->duration * CONFIG_MQTT_SN_LIB_N_ADV * MSEC_PER_SEC);
	}

	k_work_schedule(&client->process_work, K_NO_WAIT);
	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void handle_searchgw(struct mqtt_sn_client *client, struct mqtt_sn_param_searchgw *p)
{
	struct mqtt_sn_evt evt = {.type = MQTT_SN_EVT_SEARCHGW};

	/* Increment SEARCHGW transmission timestamp if waiting */
	if (client->ts_searchgw != 0) {
		client->ts_searchgw = k_uptime_get() + (T_SEARCHGW_MSEC * sys_rand8_get() / 255);
	}

	/* Set transmission timestamp to respond to SEARCHGW if we have a GW */
	if (sys_slist_len(&client->gateway) > 0) {
		client->ts_gwinfo = k_uptime_get() + (T_GWINFO_MSEC * sys_rand8_get() / 255);
	}
	client->radius_gwinfo = p->radius;
	k_work_schedule(&client->process_work, K_NO_WAIT);

	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void handle_gwinfo(struct mqtt_sn_client *client, struct mqtt_sn_param_gwinfo *p,
			  struct mqtt_sn_data rx_addr)
{
	struct mqtt_sn_evt evt = {.type = MQTT_SN_EVT_GWINFO};
	struct mqtt_sn_gateway *gw;

	/* Clear SEARCHGW and GWINFO transmission if waiting */
	client->ts_searchgw = 0;
	client->ts_gwinfo = 0;
	k_work_schedule(&client->process_work, K_NO_WAIT);

	/* Extract GW info and store */
	if (p->gw_add.size > 0) {
		rx_addr.data = p->gw_add.data;
		rx_addr.size = p->gw_add.size;
	} else {
	}
	gw = mqtt_sn_gw_create(p->gw_id, -1, rx_addr);

	if (!gw) {
		return;
	}

	sys_slist_append(&client->gateway, &gw->next);

	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void handle_connack(struct mqtt_sn_client *client, struct mqtt_sn_param_connack *p)
{
	struct mqtt_sn_evt evt = {.type = MQTT_SN_EVT_CONNECTED};

	if (p->ret_code == MQTT_SN_CODE_ACCEPTED) {
		LOG_INF("MQTT_SN client connected");
		switch (client->state) {
		case MQTT_SN_CLIENT_DISCONNECTED:
		case MQTT_SN_CLIENT_ASLEEP:
		case MQTT_SN_CLIENT_AWAKE:
			mqtt_sn_set_state(client, MQTT_SN_CLIENT_ACTIVE);
			if (client->evt_cb) {
				client->evt_cb(client, &evt);
			}
			client->ping_retries = N_RETRY;
			break;
		default:
			LOG_ERR("Client received CONNACK but was in state %d", client->state);
			return;
		}
	} else {
		LOG_WRN("CONNACK ret code %d", p->ret_code);
		mqtt_sn_disconnect_internal(client);
	}

	k_work_schedule(&client->process_work, K_NO_WAIT);
}

static void handle_willtopicreq(struct mqtt_sn_client *client)
{
	struct mqtt_sn_param response = {.type = MQTT_SN_MSG_TYPE_WILLTOPIC};

	response.params.willtopic.qos = client->will_qos;
	response.params.willtopic.retain = client->will_retain;
	response.params.willtopic.topic.data = client->will_topic.data;
	response.params.willtopic.topic.size = client->will_topic.size;

	encode_and_send(client, &response, 0);
}

static void handle_willmsgreq(struct mqtt_sn_client *client)
{
	struct mqtt_sn_param response = {.type = MQTT_SN_MSG_TYPE_WILLMSG};

	response.params.willmsg.msg.data = client->will_msg.data;
	response.params.willmsg.msg.size = client->will_msg.size;

	encode_and_send(client, &response, 0);
}

static void handle_register(struct mqtt_sn_client *client, struct mqtt_sn_param_register *p)
{
	struct mqtt_sn_param response = {.type = MQTT_SN_MSG_TYPE_REGACK};
	struct mqtt_sn_topic *topic;

	topic = mqtt_sn_topic_create(&p->topic);
	if (!topic) {
		return;
	}

	topic->state = MQTT_SN_TOPIC_STATE_REGISTERED;
	topic->topic_id = p->topic_id;
	topic->type = MQTT_SN_TOPIC_TYPE_NORMAL;

	sys_slist_append(&client->topic, &topic->next);

	response.params.regack.ret_code = MQTT_SN_CODE_ACCEPTED;
	response.params.regack.topic_id = p->topic_id;
	response.params.regack.msg_id = p->msg_id;

	encode_and_send(client, &response, 0);
}

static void handle_regack(struct mqtt_sn_client *client, struct mqtt_sn_param_regack *p)
{
	struct mqtt_sn_topic *topic = mqtt_sn_topic_find_msg_id(client, p->msg_id);

	if (!topic) {
		LOG_ERR("Can't REGACK, no topic found");
		return;
	}

	if (p->ret_code == MQTT_SN_CODE_ACCEPTED) {
		topic->state = MQTT_SN_TOPIC_STATE_REGISTERED;
		topic->topic_id = p->topic_id;
	} else {
		LOG_WRN("Gateway could not register topic ID %u, code %d", p->topic_id,
			p->ret_code);
	}
}

static void handle_publish(struct mqtt_sn_client *client, struct mqtt_sn_param_publish *p)
{
	struct mqtt_sn_param response;
	struct mqtt_sn_evt evt = {.param.publish = {.data = p->data,
						    .topic_id = p->topic_id,
						    .topic_type = p->topic_type},
				  .type = MQTT_SN_EVT_PUBLISH};

	if (p->qos == MQTT_SN_QOS_1) {
		response.type = MQTT_SN_MSG_TYPE_PUBACK;
		response.params.puback.topic_id = p->topic_id;
		response.params.puback.msg_id = p->msg_id;
		response.params.puback.ret_code = MQTT_SN_CODE_ACCEPTED;

		encode_and_send(client, &response, 0);
	} else if (p->qos == MQTT_SN_QOS_2) {
		response.type = MQTT_SN_MSG_TYPE_PUBREC;
		response.params.pubrec.msg_id = p->msg_id;

		encode_and_send(client, &response, 0);
	}

	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void handle_puback(struct mqtt_sn_client *client, struct mqtt_sn_param_puback *p)
{
	struct mqtt_sn_publish *pub = mqtt_sn_publish_find_msg_id(client, p->msg_id);

	if (!pub) {
		LOG_ERR("No matching PUBLISH found for msg id %u", p->msg_id);
		return;
	}

	mqtt_sn_publish_destroy(client, pub);
}

static void handle_pubrec(struct mqtt_sn_client *client, struct mqtt_sn_param_pubrec *p)
{
	struct mqtt_sn_param response = {.type = MQTT_SN_MSG_TYPE_PUBREL};
	struct mqtt_sn_publish *pub = mqtt_sn_publish_find_msg_id(client, p->msg_id);

	if (!pub) {
		LOG_ERR("No matching PUBLISH found for msg id %u", p->msg_id);
		return;
	}

	pub->con.last_attempt = k_uptime_get();
	pub->con.retries = N_RETRY;

	response.params.pubrel.msg_id = p->msg_id;

	encode_and_send(client, &response, 0);
}

static void handle_pubrel(struct mqtt_sn_client *client, struct mqtt_sn_param_pubrel *p)
{
	struct mqtt_sn_param response = {.type = MQTT_SN_MSG_TYPE_PUBCOMP};

	response.params.pubcomp.msg_id = p->msg_id;

	encode_and_send(client, &response, 0);
}

static void handle_pubcomp(struct mqtt_sn_client *client, struct mqtt_sn_param_pubcomp *p)
{
	struct mqtt_sn_publish *pub = mqtt_sn_publish_find_msg_id(client, p->msg_id);

	if (!pub) {
		LOG_ERR("No matching PUBLISH found for msg id %u", p->msg_id);
		return;
	}

	mqtt_sn_publish_destroy(client, pub);
}

static void handle_suback(struct mqtt_sn_client *client, struct mqtt_sn_param_suback *p)
{
	struct mqtt_sn_topic *topic = mqtt_sn_topic_find_msg_id(client, p->msg_id);

	if (!topic) {
		LOG_ERR("No matching SUBSCRIBE found for msg id %u", p->msg_id);
		return;
	}

	if (p->ret_code == MQTT_SN_CODE_ACCEPTED) {
		topic->state = MQTT_SN_TOPIC_STATE_SUBSCRIBED;
		topic->topic_id = p->topic_id;
		topic->qos = p->qos;
	} else {
		LOG_WRN("SUBACK with ret code %d", p->ret_code);
	}
}

static void handle_unsuback(struct mqtt_sn_client *client, struct mqtt_sn_param_unsuback *p)
{
	struct mqtt_sn_topic *topic = mqtt_sn_topic_find_msg_id(client, p->msg_id);

	if (!topic || topic->state != MQTT_SN_TOPIC_STATE_UNSUBSCRIBING) {
		LOG_ERR("No matching UNSUBSCRIBE found for msg id %u", p->msg_id);
		return;
	}

	mqtt_sn_topic_destroy(client, topic);
}

static void handle_pingreq(struct mqtt_sn_client *client)
{
	struct mqtt_sn_param response = {.type = MQTT_SN_MSG_TYPE_PINGRESP};

	encode_and_send(client, &response, 0);
}

static void handle_pingresp(struct mqtt_sn_client *client)
{
	struct mqtt_sn_evt evt = {.type = MQTT_SN_EVT_PINGRESP};

	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}

	if (client->state == MQTT_SN_CLIENT_AWAKE) {
		mqtt_sn_set_state(client, MQTT_SN_CLIENT_ASLEEP);
	}

	client->ping_retries = N_RETRY;
}

static void handle_disconnect(struct mqtt_sn_client *client, struct mqtt_sn_param_disconnect *p)
{
	LOG_INF("Received DISCONNECT");
	mqtt_sn_disconnect_internal(client);
}

static int handle_msg(struct mqtt_sn_client *client, struct mqtt_sn_data rx_addr)
{
	int err;
	struct mqtt_sn_param p;

	err = mqtt_sn_decode_msg(&client->rx, &p);
	if (err) {
		return err;
	}

	LOG_INF("Got message of type %d", p.type);

	switch (p.type) {
	case MQTT_SN_MSG_TYPE_ADVERTISE:
		handle_advertise(client, &p.params.advertise, rx_addr);
		break;
	case MQTT_SN_MSG_TYPE_SEARCHGW:
		handle_searchgw(client, &p.params.searchgw);
		break;
	case MQTT_SN_MSG_TYPE_GWINFO:
		handle_gwinfo(client, &p.params.gwinfo, rx_addr);
		break;
	case MQTT_SN_MSG_TYPE_CONNACK:
		handle_connack(client, &p.params.connack);
		break;
	case MQTT_SN_MSG_TYPE_WILLTOPICREQ:
		handle_willtopicreq(client);
		break;
	case MQTT_SN_MSG_TYPE_WILLMSGREQ:
		handle_willmsgreq(client);
		break;
	case MQTT_SN_MSG_TYPE_REGISTER:
		handle_register(client, &p.params.reg);
		break;
	case MQTT_SN_MSG_TYPE_REGACK:
		handle_regack(client, &p.params.regack);
		break;
	case MQTT_SN_MSG_TYPE_PUBLISH:
		handle_publish(client, &p.params.publish);
		break;
	case MQTT_SN_MSG_TYPE_PUBACK:
		handle_puback(client, &p.params.puback);
		break;
	case MQTT_SN_MSG_TYPE_PUBREC:
		handle_pubrec(client, &p.params.pubrec);
		break;
	case MQTT_SN_MSG_TYPE_PUBREL:
		handle_pubrel(client, &p.params.pubrel);
		break;
	case MQTT_SN_MSG_TYPE_PUBCOMP:
		handle_pubcomp(client, &p.params.pubcomp);
		break;
	case MQTT_SN_MSG_TYPE_SUBACK:
		handle_suback(client, &p.params.suback);
		break;
	case MQTT_SN_MSG_TYPE_UNSUBACK:
		handle_unsuback(client, &p.params.unsuback);
		break;
	case MQTT_SN_MSG_TYPE_PINGREQ:
		handle_pingreq(client);
		break;
	case MQTT_SN_MSG_TYPE_PINGRESP:
		handle_pingresp(client);
		break;
	case MQTT_SN_MSG_TYPE_DISCONNECT:
		handle_disconnect(client, &p.params.disconnect);
		break;
	case MQTT_SN_MSG_TYPE_WILLTOPICRESP:
		break;
	case MQTT_SN_MSG_TYPE_WILLMSGRESP:
		break;
	default:
		LOG_ERR("Unexpected message type %d", p.type);
		break;
	}

	k_work_reschedule(&client->process_work, K_NO_WAIT);

	return 0;
}

int mqtt_sn_input(struct mqtt_sn_client *client)
{
	ssize_t next_frame_size;
	char addr[CONFIG_MQTT_SN_LIB_MAX_ADDR_SIZE];
	struct mqtt_sn_data rx_addr = {.data = addr, .size = CONFIG_MQTT_SN_LIB_MAX_ADDR_SIZE};
	int err;

	if (!client || !client->transport || !client->transport->recvfrom) {
		return -EINVAL;
	}

	if (client->transport->poll) {
		next_frame_size = client->transport->poll(client);
		if (next_frame_size <= 0) {
			return next_frame_size;
		}
	}

	net_buf_simple_reset(&client->rx);

	next_frame_size = client->transport->recvfrom(client, client->rx.data, client->rx.size,
						      (void *)rx_addr.data, &rx_addr.size);
	if (next_frame_size <= 0) {
		return next_frame_size;
	}

	if (next_frame_size > client->rx.size) {
		return -ENOBUFS;
	}

	client->rx.len = next_frame_size;

	LOG_HEXDUMP_DBG(client->rx.data, client->rx.len, "Received data");

	err = handle_msg(client, rx_addr);

	if (err) {
		return err;
	}

	/* Should be zero */
	return -client->rx.len;
}

int mqtt_sn_get_topic_name(struct mqtt_sn_client *client, uint16_t id,
			   struct mqtt_sn_data *topic_name)
{
	struct mqtt_sn_topic *topic;

	if (!client || !topic_name) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&client->topic, topic, next) {
		if (topic->topic_id == id) {
			topic_name->data = (const uint8_t *)topic->name;
			topic_name->size = topic->namelen;
			return 0;
		}
	}
	return -ENOENT;
}
