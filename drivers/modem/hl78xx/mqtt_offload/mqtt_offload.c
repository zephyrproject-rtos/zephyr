/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_offload.c
 *
 * @brief MQTT Offload Client API Implementation.
 */

#include "mqtt_offload_msg.h"

#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/net/mqtt_offload.h>
#include "mqtt_offload_transport.h"

#define MQTT_OFFLOAD_NET_BUFS (CONFIG_MQTT_OFFLOAD_LIB_MAX_PUBLISH)
#define T_RETRY_MSEC          (CONFIG_MQTT_OFFLOAD_LIB_T_RETRY * MSEC_PER_SEC)
#define N_RETRY               (CONFIG_MQTT_OFFLOAD_LIB_N_RETRY)
#define T_KEEPALIVE_MSEC      (CONFIG_MQTT_OFFLOAD_KEEPALIVE * MSEC_PER_SEC)

LOG_MODULE_REGISTER(net_mqtt_offload, CONFIG_MQTT_OFFLOAD_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mqtt_offload_messages, MQTT_OFFLOAD_NET_BUFS,
			  CONFIG_MQTT_OFFLOAD_LIB_MAX_PAYLOAD_SIZE, 0, NULL);
/* RX thread work queue */
K_KERNEL_STACK_DEFINE(mqtt_offload_stack, 8196);

static struct k_work_q mqtt_offload_workq;

/**
 * A struct to track attempts for actions that require acknowledgment,
 * i.e. topic registering, subscribing, or unsubscribing.
 */
struct mqtt_offload_confirmable {
	int64_t last_attempt;
	uint16_t msg_id;
	uint8_t retries;
};

struct mqtt_offload_publish {
	struct mqtt_offload_confirmable con;
	sys_snode_t next;
	struct mqtt_offload_topic *topic;
	uint8_t pubdata[CONFIG_MQTT_OFFLOAD_LIB_MAX_PAYLOAD_SIZE];
	size_t datalen;
	enum mqtt_offload_qos qos;
	bool retain;
};

enum mqtt_offload_topic_state {
	MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBE,     /*!< Topic requested to subscribe */
	MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBING,   /*!< Topic in progress of subscribing */
	MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBED,    /*!< Topic subscribed */
	MQTT_OFFLOAD_TOPIC_STATE_UNSUBSCRIBE,   /*!< Topic requested to unsubscribe */
	MQTT_OFFLOAD_TOPIC_STATE_UNSUBSCRIBING, /*!< Topic in progress of unsubscribing */
	MQTT_OFFLOAD_TOPIC_STATE_PUBLISH,       /*!< Topic requested to be published */
	MQTT_OFFLOAD_TOPIC_STATE_PUBLISHING,    /*!< Topic in progress of publishing */
};

struct mqtt_offload_topic {
	struct mqtt_offload_confirmable con;
	sys_snode_t next;
	char name[CONFIG_MQTT_OFFLOAD_LIB_MAX_TOPIC_SIZE];
	size_t namelen;
	enum mqtt_offload_qos qos;
	enum mqtt_offload_topic_state state;
};

K_MEM_SLAB_DEFINE_STATIC(publishes, sizeof(struct mqtt_offload_publish),
			 CONFIG_MQTT_OFFLOAD_LIB_MAX_PUBLISH, 4);
K_MEM_SLAB_DEFINE_STATIC(topics, sizeof(struct mqtt_offload_topic),
			 CONFIG_MQTT_OFFLOAD_LIB_MAX_TOPICS, 4);

enum mqtt_offload_client_state {
	MQTT_OFFLOAD_CLIENT_DISCONNECTED,
	MQTT_OFFLOAD_CLIENT_ACTIVE,
	MQTT_OFFLOAD_CLIENT_ASLEEP,
	MQTT_OFFLOAD_CLIENT_AWAKE
};

const char *mqtt_offload_client_state_str(enum mqtt_offload_client_state state)
{
	switch (state) {
	case MQTT_OFFLOAD_CLIENT_DISCONNECTED:
		return "DISCONNECTED";
	case MQTT_OFFLOAD_CLIENT_ACTIVE:
		return "ACTIVE";
	case MQTT_OFFLOAD_CLIENT_ASLEEP:
		return "ASLEEP";
	case MQTT_OFFLOAD_CLIENT_AWAKE:
		return "AWAKE";
	default:
		return "UNKNOWN";
	}
}

static void handle_connack(struct mqtt_offload_client *client,
			   struct mqtt_offload_param_connack *p);
static void handle_suback(struct mqtt_offload_client *client, struct mqtt_offload_param_suback *p);
static void handle_unsuback(struct mqtt_offload_client *client,
			    struct mqtt_offload_param_unsuback *p);
static void handle_puback(struct mqtt_offload_client *client, struct mqtt_offload_param_puback *p);

static void mqtt_offload_set_state(struct mqtt_offload_client *client,
				   enum mqtt_offload_client_state state)
{
	LOG_DBG("Client %p state (%s) -> (%s)", client,
		mqtt_offload_client_state_str(client->state), mqtt_offload_client_state_str(state));

	client->state = state;
}

static uint16_t next_msg_id(void)
{
	static uint16_t msg_id;

	return ++msg_id;
}

static int encode_and_send(struct mqtt_offload_client *client, struct mqtt_offload_param *p,
			   uint8_t broadcast_radius)
{
	int err;

	err = mqtt_offload_encode_msg(&client->tx, p);
	if (err) {
		goto end;
	}

	LOG_HEXDUMP_DBG(client->tx.data, client->tx.len, "Send message");

	if (!client->tx.len) {
		LOG_WRN("Can't send: empty");
		err = -ENOMSG;
		goto end;
	}

	err = mqtt_transport_write(client, client->tx.data, client->tx.len, NULL, 0);

end:
	if (err) {
		LOG_ERR("Error during send: %d", err);
	}
	net_buf_simple_reset(&client->tx);

	return err;
}

static void mqtt_offload_con_init(struct mqtt_offload_confirmable *con)
{
	con->last_attempt = 0;
	con->retries = N_RETRY;
	con->msg_id = next_msg_id();
}

static void mqtt_offload_publish_destroy(struct mqtt_offload_client *client,
					 struct mqtt_offload_publish *pub)
{
	sys_slist_find_and_remove(&client->publish, &pub->next);
	k_mem_slab_free(&publishes, (void *)pub);
}

static void mqtt_offload_publish_destroy_all(struct mqtt_offload_client *client)
{
	struct mqtt_offload_publish *pub;
	sys_snode_t *next;

	while ((next = sys_slist_get(&client->publish)) != NULL) {
		pub = SYS_SLIST_CONTAINER(next, pub, next);
		k_mem_slab_free(&publishes, (void *)pub);
	}
}

static struct mqtt_offload_publish *
mqtt_offload_publish_create(const struct mqtt_offload_data *data)
{
	struct mqtt_offload_publish *pub;

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

	mqtt_offload_con_init(&pub->con);

	return pub;
}

static struct mqtt_offload_publish *
mqtt_offload_publish_find_by_msg_id(struct mqtt_offload_client *client, uint16_t msg_id)
{
	struct mqtt_offload_publish *pub;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->publish, pub, next) {
		if (pub->con.msg_id == msg_id) {
			return pub;
		}
	}

	return NULL;
}

static struct mqtt_offload_publish *
mqtt_offload_publish_find_by_topic(struct mqtt_offload_client *client,
				   struct mqtt_offload_topic *topic)
{
	struct mqtt_offload_publish *pub;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->publish, pub, next) {
		if (pub->topic == topic) {
			return pub;
		}
	}

	return NULL;
}

static struct mqtt_offload_topic *mqtt_offload_topic_create(const struct mqtt_offload_data *name)
{
	struct mqtt_offload_topic *topic;

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

	mqtt_offload_con_init(&topic->con);

	return topic;
}

static struct mqtt_offload_topic *
mqtt_offload_topic_find_by_name(struct mqtt_offload_client *client,
				const struct mqtt_offload_data *topic_name)
{
	struct mqtt_offload_topic *topic;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->topic, topic, next) {
		if (topic->namelen == topic_name->size &&
		    memcmp(topic->name, topic_name->data, topic_name->size) == 0) {
			return topic;
		}
	}

	return NULL;
}

static struct mqtt_offload_topic *
mqtt_offload_topic_find_by_msg_id(struct mqtt_offload_client *client, uint16_t msg_id)
{
	struct mqtt_offload_topic *topic;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->topic, topic, next) {
		if (topic->con.msg_id == msg_id) {
			return topic;
		}
	}

	return NULL;
}

static void mqtt_offload_topic_destroy(struct mqtt_offload_client *client,
				       struct mqtt_offload_topic *topic)
{
	struct mqtt_offload_publish *pub;

	/* Destroy all pubs referencing this topic */
	while ((pub = mqtt_offload_publish_find_by_topic(client, topic)) != NULL) {
		LOG_WRN("Destroying publish msg_id %d", pub->con.msg_id);
		mqtt_offload_publish_destroy(client, pub);
	}

	sys_slist_find_and_remove(&client->topic, &topic->next);
}

static void mqtt_offload_topic_destroy_all(struct mqtt_offload_client *client)
{
	struct mqtt_offload_topic *topic;
	struct mqtt_offload_publish *pub;
	sys_snode_t *next;

	while ((next = sys_slist_get(&client->topic)) != NULL) {
		topic = SYS_SLIST_CONTAINER(next, topic, next);
		/* Destroy all pubs referencing this topic */
		while ((pub = mqtt_offload_publish_find_by_topic(client, topic)) != NULL) {
			LOG_WRN("Destroying publish msg_id %d", pub->con.msg_id);
			mqtt_offload_publish_destroy(client, pub);
		}

		k_mem_slab_free(&topics, (void *)topic);
	}
}

static void mqtt_offload_disconnect_internal(struct mqtt_offload_client *client)
{
	struct mqtt_offload_evt evt = {.type = MQTT_OFFLOAD_EVT_DISCONNECTED};

	mqtt_offload_set_state(client, MQTT_OFFLOAD_CLIENT_DISCONNECTED);
	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}

	/*
	 * Remove all publishes, but keep topics
	 * Topics are removed on deinit or when connect is called with
	 * clean-session = true
	 */
	mqtt_offload_publish_destroy_all(client);

	k_work_cancel(&client->process_work);
}

static void mqtt_offload_sleep_internal(struct mqtt_offload_client *client)
{
	struct mqtt_offload_evt evt = {.type = MQTT_OFFLOAD_EVT_DISCONNECTED};

	mqtt_offload_set_state(client, MQTT_OFFLOAD_CLIENT_ASLEEP);
	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

/**
 * @brief Internal function to send a SUBSCRIBE message for a topic.
 *
 * @param client
 * @param topic
 * @param dup DUP flag - see HL78XX-MQTT spec
 */
static void mqtt_offload_do_subscribe(struct mqtt_offload_client *client,
				      struct mqtt_offload_topic *topic, bool dup)
{
	int err = 0;
	struct mqtt_offload_param p = {.type = MQTT_OFFLOAD_MSG_TYPE_SUBSCRIBE};
	struct mqtt_offload_param_suback suback;

	if (!client || !topic) {
		return;
	}

	if (client->state != MQTT_OFFLOAD_CLIENT_ACTIVE) {
		LOG_ERR("Cannot subscribe: not connected");
		return;
	}
	/* Message id zero is not permitted by spec. */
	if ((p.params.subscribe.qos) && (p.params.subscribe.msg_id == 0U)) {
		LOG_ERR("%s Cannot subscribe: msg_id is zero", __func__);
		return;
	}
	p.params.subscribe.msg_id = topic->con.msg_id;
	p.params.subscribe.qos = topic->qos;
	p.params.subscribe.topic_name.data = topic->name;
	p.params.subscribe.topic_name.size = topic->namelen;

	err = encode_and_send(client, &p, 0);
	if (err == 0) {
		suback.msg_id = topic->con.msg_id;
		suback.ret_code = MQTT_OFFLOAD_CODE_ACCEPTED;
	} else {
		suback.ret_code = MQTT_OFFLOAD_CODE_REJECTED_TOPIC_ID;
	}
	handle_suback(client, &suback);
}

/**
 * @brief Internal function to send an UNSUBSCRIBE message for a topic.
 *
 * @param client
 * @param topic
 */
static void mqtt_offload_do_unsubscribe(struct mqtt_offload_client *client,
					struct mqtt_offload_topic *topic)
{
	int err = 0;
	struct mqtt_offload_param p = {.type = MQTT_OFFLOAD_MSG_TYPE_UNSUBSCRIBE};
	struct mqtt_offload_param_unsuback unsuback;

	if (!client || !topic) {
		return;
	}

	if (client->state != MQTT_OFFLOAD_CLIENT_ACTIVE) {
		LOG_ERR("Cannot unsubscribe: not connected");
		return;
	}

	p.params.unsubscribe.msg_id = topic->con.msg_id;
	p.params.unsubscribe.topic.topic_name.data = topic->name;
	p.params.unsubscribe.topic.topic_name.size = topic->namelen;

	err = encode_and_send(client, &p, 0);
	if (err == 0) {
		unsuback.msg_id = topic->con.msg_id;
		unsuback.ret_code = MQTT_OFFLOAD_CODE_ACCEPTED;
	} else {
		unsuback.ret_code = MQTT_OFFLOAD_CODE_REJECTED_TOPIC_ID;
	}
	handle_unsuback(client, &unsuback);
}

/**
 * @brief Internal function to send a PUBLISH message.
 *
 * Note that this function does not do sanity checks regarding the pub's topic.
 *
 * @param client
 * @param pub
 * @param dup DUP flag - see HL78XX-MQTT spec.
 */
static void mqtt_offload_do_publish(struct mqtt_offload_client *client,
				    struct mqtt_offload_publish *pub, bool dup)
{
	int err = 0;
	struct mqtt_offload_param p = {.type = MQTT_OFFLOAD_MSG_TYPE_PUBLISH};
	struct mqtt_offload_param_puback puback;

	if (!client || !pub) {
		return;
	}

	if (client->state != MQTT_OFFLOAD_CLIENT_ACTIVE) {
		LOG_ERR("Cannot PUBLISH: not connected");
		return;
	}
	/* Message id zero is not permitted by spec. */
	if ((p.params.publish.qos) && (p.params.publish.msg_id == 0U)) {
		return;
	}
	LOG_INF("Publishing to msg_id %d, retain %d, qos "
		"%d, dup %d",
		pub->con.msg_id, pub->retain, pub->qos, dup);
	LOG_HEXDUMP_DBG(pub->topic->name, pub->topic->namelen, "Publish topic");
	LOG_HEXDUMP_DBG(pub->pubdata, pub->datalen, "Publish data");
	p.params.publish.data.data = pub->pubdata;
	p.params.publish.data.size = pub->datalen;
	p.params.publish.msg_id = pub->con.msg_id;
	p.params.publish.retain = pub->retain;
	p.params.publish.topic.data = pub->topic->name;
	p.params.publish.topic.size = pub->topic->namelen;
	p.params.publish.qos = pub->qos;

	err = encode_and_send(client, &p, 0);

	if (err == 0) {
		LOG_DBG("Published");
		puback.ret_code = MQTT_OFFLOAD_CODE_ACCEPTED;
	} else {
		LOG_ERR("MQTT publish error: %d", err);
		puback.ret_code = MQTT_OFFLOAD_CODE_REJECTED;
	}

	puback.msg_id = pub->con.msg_id;
	handle_puback(client, &puback);
}

/**
 * @brief Process all publish tasks in the queue.
 *
 * @param client
 * @param next_cycle will be set to the time when the next action is required
 *
 * @retval 0 on success
 * @retval -ETIMEDOUT when a publish task ran out of retries
 */
static int process_pubs(struct mqtt_offload_client *client, int64_t *next_cycle)
{
	struct mqtt_offload_publish *pub, *pubs;
	const int64_t now = k_uptime_get();
	int64_t next_attempt;
	bool dup; /* dup flag if message is resent */

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

		/* Check if action is due */
		if (next_attempt <= now) {
			switch (pub->topic->state) {
			case MQTT_OFFLOAD_TOPIC_STATE_PUBLISH:
				if (!pub->con.retries--) {
					LOG_WRN("PUB ran out of retries, disconnecting");
					mqtt_offload_disconnect_internal(client);
					return -ETIMEDOUT;
				}
				pub->topic->state = MQTT_OFFLOAD_TOPIC_STATE_PUBLISHING;
				__fallthrough;
			case MQTT_OFFLOAD_TOPIC_STATE_PUBLISHING:
				mqtt_offload_do_publish(client, pub, dup);
				if (pub->qos == MQTT_OFFLOAD_QOS_0 ||
				    pub->qos == MQTT_OFFLOAD_QOS_M1) {
					/* We are done, remove this */
					mqtt_offload_publish_destroy(client, pub);
					continue;
				} else {
					/* Wait for ack */
					pub->con.last_attempt = now;
					next_attempt = now + T_RETRY_MSEC;
				}
				break;
			default:
				LOG_INF("Can't publish; topic is not ready");
				break;
			}
		}

		/* Remember time when next action is due */
		if (next_attempt > now && (*next_cycle == 0 || next_attempt < *next_cycle)) {
			*next_cycle = next_attempt;
		}
	}

	LOG_DBG("next_cycle: %lld", *next_cycle);

	return 0;
}

static bool topic_action_due(struct mqtt_offload_topic *topic, int64_t now, int64_t *next_attempt,
			     bool *dup)
{
	if (topic->con.last_attempt == 0) {
		*next_attempt = 0;
		*dup = false;
	} else {
		*next_attempt = topic->con.last_attempt + T_RETRY_MSEC;
		*dup = true;
	}
	return (*next_attempt <= now);
}

static bool has_pending_subscriptions(struct mqtt_offload_client *client)
{
	struct mqtt_offload_topic *t;

	SYS_SLIST_FOR_EACH_CONTAINER(&client->topic, t, next) {
		if (t->state == MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBING ||
		    t->state == MQTT_OFFLOAD_TOPIC_STATE_UNSUBSCRIBING) {
			return true;
		}
	}
	return false;
}

static int handle_subscribe(struct mqtt_offload_client *client, struct mqtt_offload_topic *topic,
			    int64_t now, bool dup)
{
	if (!topic->con.retries--) {
		LOG_WRN("Topic ran out of retries, disconnecting");
		mqtt_offload_disconnect_internal(client);
		return -ETIMEDOUT;
	}
	mqtt_offload_do_subscribe(client, topic, dup);
	topic->con.last_attempt = now;
	return 0;
}

static int handle_unsubscribe(struct mqtt_offload_client *client, struct mqtt_offload_topic *topic,
			      int64_t now)
{
	if (!topic->con.retries--) {
		LOG_WRN("Topic ran out of retries, disconnecting");
		mqtt_offload_disconnect_internal(client);
		return -ETIMEDOUT;
	}
	mqtt_offload_do_unsubscribe(client, topic);
	topic->con.last_attempt = now;
	return 0;
}

static int handle_subscribe_state(struct mqtt_offload_client *client,
				  struct mqtt_offload_topic *topic, int64_t now, bool dup,
				  bool *subscribing, int64_t *next_attempt)
{
	if (*subscribing) {
		return 0; /* Skip if already subscribing */
	}

	topic->state = MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBING;
	LOG_INF("Topic subscription now in progress");

	if (handle_subscribe(client, topic, now, dup) < 0) {
		return -ETIMEDOUT;
	}

	*subscribing = true;
	*next_attempt = now + T_RETRY_MSEC;
	return 0;
}

static int handle_subscribing_state(struct mqtt_offload_client *client,
				    struct mqtt_offload_topic *topic, int64_t now, bool dup,
				    bool *subscribing, int64_t *next_attempt)
{
	if (handle_subscribe(client, topic, now, dup) < 0) {
		return -ETIMEDOUT;
	}
	*subscribing = true;
	*next_attempt = now + T_RETRY_MSEC;
	return 0;
}

static int handle_unsubscribe_state(struct mqtt_offload_client *client,
				    struct mqtt_offload_topic *topic, int64_t now,
				    bool *subscribing, int64_t *next_attempt)
{
	if (*subscribing) {
		return 0; /* Skip if already unsubscribing */
	}

	topic->state = MQTT_OFFLOAD_TOPIC_STATE_UNSUBSCRIBING;
	LOG_INF("Topic unsubscription now in progress");

	if (handle_unsubscribe(client, topic, now) < 0) {
		return -ETIMEDOUT;
	}

	*subscribing = true;
	*next_attempt = now + T_RETRY_MSEC;
	return 0;
}

static int handle_unsubscribing_state(struct mqtt_offload_client *client,
				      struct mqtt_offload_topic *topic, int64_t now,
				      bool *subscribing, int64_t *next_attempt)
{
	if (handle_unsubscribe(client, topic, now) < 0) {
		return -ETIMEDOUT;
	}
	*subscribing = true;
	*next_attempt = now + T_RETRY_MSEC;
	return 0;
}

static int process_topics(struct mqtt_offload_client *client, int64_t *next_cycle)
{
	struct mqtt_offload_topic *topic;
	const int64_t now = k_uptime_get();
	int64_t next_attempt;
	bool dup;
	bool subscribing = has_pending_subscriptions(client);

	SYS_SLIST_FOR_EACH_CONTAINER(&client->topic, topic, next) {
		LOG_HEXDUMP_DBG(topic->name, topic->namelen, "Processing topic");

		if (!topic_action_due(topic, now, &next_attempt, &dup)) {
			goto update_next_cycle;
		}

		int err = 0;

		switch (topic->state) {
		case MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBE:
			err = handle_subscribe_state(client, topic, now, dup, &subscribing,
						     &next_attempt);
			break;

		case MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBING:
			err = handle_subscribing_state(client, topic, now, dup, &subscribing,
						       &next_attempt);
			break;

		case MQTT_OFFLOAD_TOPIC_STATE_UNSUBSCRIBE:
			err = handle_unsubscribe_state(client, topic, now, &subscribing,
						       &next_attempt);
			break;

		case MQTT_OFFLOAD_TOPIC_STATE_UNSUBSCRIBING:
			err = handle_unsubscribing_state(client, topic, now, &subscribing,
							 &next_attempt);
			break;

		default:
			break;
		}

		if (err < 0) {
			return err;
		}

update_next_cycle:
		if (next_attempt > now && (*next_cycle == 0 || next_attempt < *next_cycle)) {
			*next_cycle = next_attempt;
		}
	}

	LOG_DBG("next_cycle: %lld", *next_cycle);
	return 0;
}

/**
 * @brief Housekeeping task that is called by workqueue item
 *
 * @param wrk The work item
 */
static void process_work(struct k_work *wrk)
{
	struct mqtt_offload_client *client;
	int64_t next_cycle = 0;
	int err;

	client = CONTAINER_OF(wrk, struct mqtt_offload_client, process_work);

	LOG_DBG("Executing work of client %p in state %d at time %lld", client, client->state,
		k_uptime_get());

	if (client->state == MQTT_OFFLOAD_CLIENT_ACTIVE) {
		err = process_topics(client, &next_cycle);
		if (err) {
			return;
		}

		err = process_pubs(client, &next_cycle);
		if (err) {
			return;
		}
	}

	if (next_cycle > 0) {
		LOG_DBG("next_cycle: %lld", next_cycle);
	}
}

int mqtt_offload_client_init(struct mqtt_offload_client *client,
			     const struct mqtt_offload_data *client_id,
			     struct mqtt_offload_transport_mqtt *transport,
			     mqtt_offload_evt_cb_t evt_cb, struct mqtt_offload_buffers *buffers)
{
	int ret = 0;

	if (!client || !client_id || !transport || !evt_cb || !buffers) {
		return -EINVAL;
	}

	memset(client, 0, sizeof(*client));

	client->client_id.data = client_id->data;
	client->client_id.size = client_id->size;
	client->transport = transport;
	client->evt_cb = evt_cb;

	net_buf_simple_init_with_data(&client->tx, buffers->tx, buffers->txsz);
	net_buf_simple_reset(&client->tx);

	net_buf_simple_init_with_data(&client->rx, buffers->rx, buffers->rxsz);
	net_buf_simple_reset(&client->rx);
	/* Initialize work queue and event handling */
	k_work_queue_start(&mqtt_offload_workq, mqtt_offload_stack,
			   K_KERNEL_STACK_SIZEOF(mqtt_offload_stack), K_PRIO_COOP(7), NULL);

	k_work_init(&client->process_work, process_work);

	return ret;
}

void mqtt_offload_client_deinit(struct mqtt_offload_client *client)
{
	if (!client) {
		return;
	}

	mqtt_offload_publish_destroy_all(client);
	mqtt_offload_topic_destroy_all(client);

	k_work_cancel(&client->process_work);
}

int mqtt_offload_connect(struct mqtt_offload_client *client, bool will, bool clean_session)
{
	struct mqtt_offload_param p = {.type = MQTT_OFFLOAD_MSG_TYPE_CONNECT};
	struct mqtt_offload_param_connack connack;
	int err;

	if (!client || !client->transport) {
		return -EINVAL;
	}

	if (will && (!client->will_msg.data || !client->will_topic.data)) {
		LOG_ERR("will set to true, but no will data in client");
		return -EINVAL;
	}

	if (clean_session) {
		LOG_WRN("%d Clean session established, all topics will be removed", __LINE__);
		mqtt_offload_topic_destroy_all(client);
	}

	p.params.connect.clean_session = clean_session;
	p.params.connect.will = will;
	p.params.connect.qos = client->will_qos;
	p.params.connect.duration = CONFIG_MQTT_OFFLOAD_KEEPALIVE;
	p.params.connect.client_id.data = client->client_id.data;
	p.params.connect.client_id.size = client->client_id.size;

	client->last_ping = k_uptime_get();

	err = mqtt_transport_connect(client, &p);

	if (err == 0) {
		connack.ret_code = MQTT_OFFLOAD_CODE_ACCEPTED;
	} else {
		connack.ret_code = MQTT_OFFLOAD_CODE_REJECTED_CONGESTION;
	}

	handle_connack(client, &connack);
	return 0;
}

int mqtt_offload_disconnect(struct mqtt_offload_client *client)
{
	struct mqtt_offload_param p = {.type = MQTT_OFFLOAD_MSG_TYPE_DISCONNECT};
	int err;

	if (!client) {
		return -EINVAL;
	}

	p.params.disconnect.duration = 0;

	err = mqtt_transport_close(client);
	if (err == 0) {
		LOG_DBG("Disconnected");
	} else {
		LOG_ERR("Failed to disconnect");
	}
	mqtt_offload_disconnect_internal(client);

	return err;
}

int mqtt_offload_sleep(struct mqtt_offload_client *client, uint16_t duration)
{
	struct mqtt_offload_param p = {.type = MQTT_OFFLOAD_MSG_TYPE_DISCONNECT};
	int err;

	if (!client || !duration) {
		return -EINVAL;
	}

	p.params.disconnect.duration = duration;

	err = encode_and_send(client, &p, 0);
	mqtt_offload_sleep_internal(client);

	return err;
}

int mqtt_offload_subscribe(struct mqtt_offload_client *client, enum mqtt_offload_qos qos,
			   const struct mqtt_offload_data *topic_name)
{
	struct mqtt_offload_topic *topic;
	int err;

	if (!client || !topic_name || !topic_name->data || !topic_name->size) {
		return -EINVAL;
	}
	if (client->state != MQTT_OFFLOAD_CLIENT_ACTIVE) {
		LOG_ERR("Cannot subscribe: not connected");
		return -ENOTCONN;
	}
	topic = mqtt_offload_topic_find_by_name(client, topic_name);
	if (!topic) {
		topic = mqtt_offload_topic_create(topic_name);
		if (!topic) {
			return -ENOMEM;
		}
		topic->qos = qos;
		topic->state = MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBE;
		sys_slist_append(&client->topic, &topic->next);
	}

	err = k_work_submit_to_queue(&mqtt_offload_workq, &client->process_work);
	if (err < 0) {
		return err;
	}

	return 0;
}

int mqtt_offload_unsubscribe(struct mqtt_offload_client *client,
			     const struct mqtt_offload_data *topic_name)
{
	struct mqtt_offload_topic *topic;
	int err;

	if (!client || !topic_name) {
		return -EINVAL;
	}

	if (client->state != MQTT_OFFLOAD_CLIENT_ACTIVE) {
		LOG_ERR("Cannot unsubscribe: not connected");
		return -ENOTCONN;
	}

	topic = mqtt_offload_topic_find_by_name(client, topic_name);
	if (!topic) {
		LOG_HEXDUMP_ERR(topic_name->data, topic_name->size, "Topic not found");
		return -ENOENT;
	}

	if (topic->state != MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBED) {
		LOG_ERR("Cannot unsubscribe: not subscribed");
		return -EAGAIN;
	}

	topic->state = MQTT_OFFLOAD_TOPIC_STATE_UNSUBSCRIBE;
	mqtt_offload_con_init(&topic->con);

	err = k_work_submit_to_queue(&mqtt_offload_workq, &client->process_work);
	if (err < 0) {
		return err;
	}

	return 0;
}

int mqtt_offload_publish(struct mqtt_offload_client *client, enum mqtt_offload_qos qos,
			 const struct mqtt_offload_data *topic_name, bool retain,
			 const struct mqtt_offload_data *data)
{
	struct mqtt_offload_publish *pub;
	struct mqtt_offload_topic *topic;
	int err;

	if (!client || !topic_name) {
		return -EINVAL;
	}

	if (qos == MQTT_OFFLOAD_QOS_M1) {
		LOG_ERR("QoS -1 not supported");
		return -ENOTSUP;
	}

	if (client->state != MQTT_OFFLOAD_CLIENT_ACTIVE) {
		LOG_ERR("Cannot publish: disconnected");
		return -ENOTCONN;
	}
	topic = mqtt_offload_topic_find_by_name(client, topic_name);
	if (!topic) {
		topic = mqtt_offload_topic_create(topic_name);
		if (!topic) {
			return -ENOMEM;
		}
		topic->qos = qos;
		topic->state = MQTT_OFFLOAD_TOPIC_STATE_PUBLISH;
		sys_slist_append(&client->topic, &topic->next);
	}
	pub = mqtt_offload_publish_create(data);
	if (!pub) {
		k_work_submit_to_queue(&mqtt_offload_workq, &client->process_work);
		return -ENOMEM;
	}
	pub->qos = qos;
	pub->retain = retain;
	pub->topic = topic;

	sys_slist_append(&client->publish, &pub->next);

	err = k_work_submit_to_queue(&mqtt_offload_workq, &client->process_work);
	if (err < 0) {
		return err;
	}

	return 0;
}

static void handle_willtopicreq(struct mqtt_offload_client *client)
{
	struct mqtt_offload_param response = {.type = MQTT_OFFLOAD_MSG_TYPE_WILLTOPIC};

	response.params.willtopic.qos = client->will_qos;
	response.params.willtopic.retain = client->will_retain;
	response.params.willtopic.topic.data = client->will_topic.data;
	response.params.willtopic.topic.size = client->will_topic.size;

	encode_and_send(client, &response, 0);
}

static void handle_willmsgreq(struct mqtt_offload_client *client)
{
	struct mqtt_offload_param response = {.type = MQTT_OFFLOAD_MSG_TYPE_WILLMSG};

	response.params.willmsg.msg.data = client->will_msg.data;
	response.params.willmsg.msg.size = client->will_msg.size;

	encode_and_send(client, &response, 0);
}

static void handle_connack(struct mqtt_offload_client *client, struct mqtt_offload_param_connack *p)
{
	struct mqtt_offload_evt evt = {.type = MQTT_OFFLOAD_EVT_CONNECTED};

	if (p->ret_code == MQTT_OFFLOAD_CODE_ACCEPTED) {
		LOG_INF("HL78XX_MQTT client connected");
		switch (client->state) {
		case MQTT_OFFLOAD_CLIENT_DISCONNECTED:
		case MQTT_OFFLOAD_CLIENT_ASLEEP:
		case MQTT_OFFLOAD_CLIENT_AWAKE:
			mqtt_offload_set_state(client, MQTT_OFFLOAD_CLIENT_ACTIVE);
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
		mqtt_offload_disconnect_internal(client);
	}

	k_work_submit_to_queue(&mqtt_offload_workq, &client->process_work);
}

static void handle_publish(struct mqtt_offload_client *client, struct mqtt_offload_param_publish *p)
{
	struct mqtt_offload_evt evt = {.param.publish = {.topic = p->topic, .data = p->data},
				       .type = MQTT_OFFLOAD_EVT_PUBLISH};

	LOG_DBG("Received PUBLISH to topic %*.s, retain %d, qos %d, msg id %d, data: %*.s",
		p->topic.size, p->topic.data, p->retain, p->qos, p->msg_id, p->data.size,
		p->data.data);

	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void handle_puback(struct mqtt_offload_client *client, struct mqtt_offload_param_puback *p)
{
	struct mqtt_offload_publish *pub = mqtt_offload_publish_find_by_msg_id(client, p->msg_id);
	struct mqtt_offload_evt evt = {.type = MQTT_OFFLOAD_EVT_PUBLISH_ACK, .result = p->ret_code};

	if (!pub) {
		LOG_ERR("No matching PUBLISH found for msg id %u", p->msg_id);
		return;
	}

	mqtt_offload_publish_destroy(client, pub);

	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void handle_suback(struct mqtt_offload_client *client, struct mqtt_offload_param_suback *p)
{
	struct mqtt_offload_topic *topic = mqtt_offload_topic_find_by_msg_id(client, p->msg_id);
	struct mqtt_offload_evt evt = {.type = MQTT_OFFLOAD_EVT_SUBSCRIBE_ACK,
				       .result = p->ret_code};

	if (!topic) {
		LOG_ERR("No matching SUBSCRIBE found for msg id %u", p->msg_id);
		return;
	}

	if (p->ret_code == MQTT_OFFLOAD_CODE_ACCEPTED) {
		topic->state = MQTT_OFFLOAD_TOPIC_STATE_SUBSCRIBED;
		topic->qos = p->qos;
	} else {
		LOG_WRN("SUBACK with ret code %d", p->ret_code);
	}

	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void handle_unsuback(struct mqtt_offload_client *client,
			    struct mqtt_offload_param_unsuback *p)
{
	struct mqtt_offload_topic *topic = mqtt_offload_topic_find_by_msg_id(client, p->msg_id);
	struct mqtt_offload_evt evt = {.type = MQTT_OFFLOAD_EVT_UNSUBSCRIBE_ACK,
				       .result = p->ret_code};

	if (!topic || topic->state != MQTT_OFFLOAD_TOPIC_STATE_UNSUBSCRIBING) {
		LOG_ERR("No matching UNSUBSCRIBE found for msg id %u", p->msg_id);
		return;
	}

	mqtt_offload_topic_destroy(client, topic);

	if (client->evt_cb) {
		client->evt_cb(client, &evt);
	}
}

static void handle_disconnect(struct mqtt_offload_client *client,
			      struct mqtt_offload_param_disconnect *p)
{
	LOG_INF("Received DISCONNECT");
	mqtt_offload_disconnect_internal(client);
}

static int handle_msg(struct mqtt_offload_client *client, struct mqtt_offload_data rx_addr)
{
	int err;
	struct mqtt_offload_param p;

	err = mqtt_offload_decode_msg(&client->rx, &p);
	if (err) {
		return err;
	}

	LOG_INF("Got message of type %d", p.type);

	switch (p.type) {
	case MQTT_OFFLOAD_MSG_TYPE_WILLTOPICREQ:
		handle_willtopicreq(client);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_WILLMSGREQ:
		handle_willmsgreq(client);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_PUBLISH:
		handle_publish(client, &p.params.publish);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_PUBACK:
		handle_puback(client, &p.params.puback);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_SUBACK:
		handle_suback(client, &p.params.suback);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_UNSUBACK:
		handle_unsuback(client, &p.params.unsuback);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_DISCONNECT:
		handle_disconnect(client, &p.params.disconnect);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_WILLTOPICRESP:
		break;
	case MQTT_OFFLOAD_MSG_TYPE_WILLMSGRESP:
		break;
	default:
		LOG_ERR("Unexpected message type %d", p.type);
		break;
	}

	k_work_submit_to_queue(&mqtt_offload_workq, &client->process_work);

	return 0;
}

int mqtt_offload_input(struct mqtt_offload_client *client)
{
	ssize_t next_frame_size;
	char addr[CONFIG_MQTT_OFFLOAD_LIB_MAX_ADDR_SIZE];
	struct mqtt_offload_data rx_addr = {.data = addr,
					    .size = CONFIG_MQTT_OFFLOAD_LIB_MAX_ADDR_SIZE};
	int err;

	if (!client || !client->transport) {
		return -EINVAL;
	}

	next_frame_size = mqtt_transport_poll(client);
	if (next_frame_size <= 0) {
		return next_frame_size;
	}

	net_buf_simple_reset(&client->rx);

	next_frame_size = mqtt_transport_read(client, client->rx.data, client->rx.size,
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
	LOG_DBG("%d Processed HL78XX-MQTT message %d", __LINE__, client->rx.len);
	/* Should be zero */
	return -client->rx.len;
}
