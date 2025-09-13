/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_client.c */

#include "common.h"
#include "creds/creds.h"

#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mqtt_offload.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt_client, LOG_LEVEL_DBG);

#define MAX_RETRIES         10u
#define BACKOFF_EXP_BASE_MS 1000u
#define BACKOFF_EXP_MAX_MS  60000u
#define BACKOFF_CONST_MS    5000u

#define MQTT_BROKER_PORT CONFIG_MQTT_PORT
#if defined(CONFIG_MQTT_OFFLOAD_LIB_TLS)

#define TLS_TAG_DEVICE_CERTIFICATE  1
#define TLS_TAG_DEVICE_PRIVATE_KEY  1
#define TLS_TAG_MQTT_CA_CERTIFICATE 2
#endif /* CONFIG_MQTT_OFFLOAD_LIB_TLS */

static void process_thread(void);

K_THREAD_DEFINE(mqtt_offload_thread_id, STACK_SIZE, process_thread, NULL, NULL, NULL,
		THREAD_PRIORITY, IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0, -1);

static struct mqtt_offload_client mqtt_client;
static struct mqtt_offload_transport_mqtt tp;
static struct mqtt_offload_data client_id =
	MQTT_OFFLOAD_DATA_STRING_LITERAL(CONFIG_MQTT_THING_NAME);
static bool subscribed;
static int64_t ts;
static struct mqtt_offload_data topic_p =
	MQTT_OFFLOAD_DATA_STRING_LITERAL(CONFIG_MQTT_PUBLISH_TOPIC);
static struct mqtt_offload_data topic_s =
	MQTT_OFFLOAD_DATA_STRING_LITERAL(CONFIG_MQTT_SUBSCRIBE_TOPIC);

static uint8_t tx_buf[CONFIG_NET_SAMPLE_MQTT_OFFLOAD_BUFFER_SIZE];
static uint8_t rx_buf[CONFIG_NET_SAMPLE_MQTT_OFFLOAD_BUFFER_SIZE];

static bool mqtt_offload_connected;
static struct sockaddr_in mqtt_broker;
#if defined(CONFIG_MQTT_OFFLOAD_LIB_TLS)
static const sec_tag_t sec_tls_tags[] = {
	TLS_TAG_DEVICE_CERTIFICATE,
	TLS_TAG_MQTT_CA_CERTIFICATE,
};

static int setup_credentials(void)
{
	int ret;

	ret = tls_credential_add(TLS_TAG_DEVICE_CERTIFICATE, TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 public_cert, public_cert_len);
	if (ret < 0) {
		LOG_ERR("Failed to add device certificate: %d", ret);
		goto exit;
	}

	ret = tls_credential_add(TLS_TAG_DEVICE_PRIVATE_KEY, TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key, private_key_len);
	if (ret < 0) {
		LOG_ERR("Failed to add device private key: %d", ret);
		goto exit;
	}

	ret = tls_credential_add(TLS_TAG_MQTT_CA_CERTIFICATE, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_cert, ca_cert_len);
	if (ret < 0) {
		LOG_ERR("Failed to add device private key: %d", ret);
		goto exit;
	}

exit:
	return ret;
}
#endif /* CONFIG_MQTT_OFFLOAD_LIB_TLS */

static void evt_cb(struct mqtt_offload_client *client, const struct mqtt_offload_evt *evt)
{
	switch (evt->type) {
	case MQTT_OFFLOAD_EVT_CONNECTED: /* Connected to <broker> */
		LOG_INF("MQTT-OFFLOAD event EVT_CONNECTED");
		mqtt_offload_connected = true;
		break;
	case MQTT_OFFLOAD_EVT_DISCONNECTED: /* Disconnected */
		LOG_INF("MQTT-OFFLOAD event EVT_DISCONNECTED");
		mqtt_offload_connected = false;
		break;
	case MQTT_OFFLOAD_EVT_ASLEEP: /* Entered ASLEEP state */
		LOG_INF("MQTT-OFFLOAD event EVT_ASLEEP");
		break;
	case MQTT_OFFLOAD_EVT_AWAKE: /* Entered AWAKE state */
		LOG_INF("MQTT-OFFLOAD event EVT_AWAKE");
		break;
	case MQTT_OFFLOAD_EVT_PUBLISH: /* Received a PUBLISH message */
		LOG_INF("MQTT-OFFLOAD event EVT_PUBLISH");
		LOG_HEXDUMP_INF(evt->param.publish.topic.data, evt->param.publish.topic.size,
				"Published topic");
		LOG_HEXDUMP_INF(evt->param.publish.data.data, evt->param.publish.data.size,
				"Published data");
		LOG_DBG("%d %d", __LINE__, evt->param.publish.data.size);
		break;
	case MQTT_OFFLOAD_EVT_SUBSCRIBE_ACK: /* Received a SUBSCRIBE ACK message */
		LOG_INF("MQTT-OFFLOAD event EVT_SUBSCRIBE_ACK");
		subscribed = true;
		break;
	case MQTT_OFFLOAD_EVT_UNSUBSCRIBE_ACK: /* Received a UNSUBSCRIBE ACK message */
		LOG_INF("MQTT-OFFLOAD event EVT_UNSUBSCRIBE_ACK");
		subscribed = false;
		break;
	case MQTT_OFFLOAD_EVT_PUBLISH_ACK: /* Received a PUBLISH ACK message */
		if (evt->result != 0) {
			LOG_ERR("MQTT-OFFLOAD event EVT_PUBLISH_ACK failed");
		} else {
			LOG_INF("MQTT-OFFLOAD event EVT_PUBLISH_ACK success");
		}
		break;
	default:
		break;
	}
}

static int do_work(void)
{
	char out[20];
	struct mqtt_offload_data pubdata = {.data = out};
	const int64_t now = k_uptime_get();
	int err;

	err = mqtt_offload_input(&mqtt_client);
	if (err < 0) {
		LOG_ERR("failed: input: %d", err);
		return err;
	}

	if (mqtt_offload_connected && !subscribed) {
		err = mqtt_offload_subscribe(&mqtt_client, MQTT_OFFLOAD_QOS_0, &topic_s);
		if (err < 0) {
			return err;
		}
	}

	if (now - ts > 50000 && mqtt_offload_connected) {
		LOG_INF("Publishing timestamp");

		ts = now;

		err = snprintk(out, sizeof(out), "%" PRIi64, ts);
		if (err < 0) {
			LOG_ERR("failed: snprintf");
			return err;
		}

		pubdata.size = MIN(sizeof(out), err);

		err = mqtt_offload_publish(&mqtt_client, MQTT_OFFLOAD_QOS_0, &topic_p, false,
					   &pubdata);
		if (err < 0) {
			LOG_ERR("failed: publish: %d", err);
			return err;
		}
	}

	return 0;
}

static void mqtt_client_setup(void)
{
	int err = 0;
	struct mqtt_offload_buffers buffers;

#if defined(CONFIG_MQTT_OFFLOAD_LIB_TLS)
	setup_credentials();
	/* setup TLS */
	tp.type = MQTT_OFFLOAD_TRANSPORT_SECURE;
	struct mqtt_sec_config *const tls_config = &tp.mqtt.config;

	tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = sec_tls_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(sec_tls_tags);
	tls_config->hostname = CONFIG_MQTT_ENDPOINT;
	tls_config->cert_nocopy = TLS_CERT_NOCOPY_NONE;
#else
	tp.type = MQTT_TRANSPORT_NON_SECURE;
#endif
	buffers.tx = tx_buf;
	buffers.txsz = sizeof(tx_buf);
	buffers.rx = rx_buf;
	buffers.rxsz = sizeof(rx_buf);

	err = mqtt_offload_client_init(&mqtt_client, &client_id, &tp, evt_cb, &buffers);

	__ASSERT(err == 0, "mqtt_offload_client_init() failed %d", err);

	mqtt_client.broker = &mqtt_broker;

	mqtt_client.protocol_version = MQTT_VERSION_3_1_1;
}

struct backoff_context {
	uint16_t retries_count;
	uint16_t max_retries;

#if defined(CONFIG_MQTT_EXPONENTIAL_BACKOFF)
	uint32_t attempt_max_backoff; /* ms */
	uint32_t max_backoff;         /* ms */
#endif
};

static void backoff_context_init(struct backoff_context *bo)
{
	__ASSERT_NO_MSG(bo != NULL);

	bo->retries_count = 0u;
	bo->max_retries = MAX_RETRIES;

#if defined(CONFIG_MQTT_EXPONENTIAL_BACKOFF)
	bo->attempt_max_backoff = BACKOFF_EXP_BASE_MS;
	bo->max_backoff = BACKOFF_EXP_MAX_MS;
#endif
}

/* https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/ */
static void backoff_get_next(struct backoff_context *bo, uint32_t *next_backoff_ms)
{
	__ASSERT_NO_MSG(bo != NULL);
	__ASSERT_NO_MSG(next_backoff_ms != NULL);

#if defined(CONFIG_MQTT_EXPONENTIAL_BACKOFF)
	if (bo->retries_count <= bo->max_retries) {
		*next_backoff_ms = sys_rand32_get() % (bo->attempt_max_backoff + 1u);

		/* Calculate max backoff for the next attempt (~ 2**attempt) */
		bo->attempt_max_backoff = MIN(bo->attempt_max_backoff * 2u, bo->max_backoff);
		bo->retries_count++;
	}
#else
	*next_backoff_ms = BACKOFF_CONST_MS;
#endif
}

static int mqtt_client_try_connect(void)
{
	int ret = 0;
	uint32_t backoff_ms;
	struct backoff_context bo;

	backoff_context_init(&bo);

	while (bo.retries_count <= bo.max_retries) {
		LOG_INF("Connecting client");
		ret = mqtt_offload_connect(&mqtt_client, false, true);
		__ASSERT(ret == 0, "mqtt_offload_connect() failed %d", ret);
		if (ret == 0) {
			goto exit;
		}

		backoff_get_next(&bo, &backoff_ms);

		LOG_ERR("Failed to connect: %d backoff delay: %u ms", ret, backoff_ms);
		k_msleep(backoff_ms);
	}

exit:
	return ret;
}

int mqtt_client_loop(struct pollfd *fds)
{
	int rc;

	mqtt_client_setup();

	rc = mqtt_client_try_connect();
	if (rc != 0) {
		return rc;
	}

	fds->fd = tp.mqtt.sock;
	fds->events = POLLIN;
	return 0;
}

static int resolve_broker_addr(struct sockaddr_in *broker)
{
	int ret;
	struct addrinfo *ai = NULL;

	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
	};
	char port_string[6] = {0};

	sprintf(port_string, "%d", MQTT_BROKER_PORT);
	ret = getaddrinfo(CONFIG_MQTT_ENDPOINT, port_string, &hints, &ai);
	if (ret == 0) {
		char addr_str[INET_ADDRSTRLEN];

		memcpy(broker, ai->ai_addr, MIN(ai->ai_addrlen, sizeof(struct sockaddr_storage)));

		inet_ntop(AF_INET, &broker->sin_addr, addr_str, sizeof(addr_str));
		LOG_INF("Resolved: %s:%u", addr_str, htons(broker->sin_port));
	} else {
		LOG_ERR("failed to resolve hostname err = %d (errno = %d)", ret, errno);
	}

	freeaddrinfo(ai);

	return ret;
}

static void process_thread(void)
{
	int err;
	struct pollfd fds;

	resolve_broker_addr(&mqtt_broker);

	err = mqtt_client_loop(&fds);
	if (err != 0) {
		goto cleanup;
	}

	while (err == 0) {
		k_sleep(K_MSEC(500));

		err = do_work();
	}

cleanup:
	mqtt_offload_disconnect(&mqtt_client);

	close(fds.fd);
	fds.fd = -1;
	LOG_ERR("Exiting thread: %d", err);
}

void start_thread(void)
{
	int rc;
#if defined(CONFIG_USERSPACE)
	rc = k_mem_domain_add_thread(&app_domain, udp_thread_id);
	if (rc < 0) {
		LOG_ERR("Failed: k_mem_domain_add_thread() %d", rc);
		return;
	}
#endif
	rc = k_thread_name_set(mqtt_offload_thread_id, "mqtt_offload");
	if (rc < 0 && rc != -ENOSYS) {
		LOG_ERR("Failed: k_thread_name_set() %d", rc);
		return;
	}

	k_thread_start(mqtt_offload_thread_id);

	rc = k_thread_join(mqtt_offload_thread_id, K_FOREVER);
	if (rc != 0) {
		LOG_ERR("Failed: k_thread_join() %d", rc);
	}
}
