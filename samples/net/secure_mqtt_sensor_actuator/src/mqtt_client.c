/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_mqtt, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/data/json.h>
#include <zephyr/random/random.h>

#include "mqtt_client.h"
#include "device.h"

/* Buffers for MQTT client */
static uint8_t rx_buffer[CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE];
static uint8_t tx_buffer[CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE];

/* MQTT payload buffer */
static uint8_t payload_buf[CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE];

/* MQTT broker details */
static struct sockaddr_storage broker;

/* Socket descriptor */
static struct zsock_pollfd fds[1];
static int nfds;

/* JSON payload format */
static const struct json_obj_descr sensor_sample_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct sensor_sample, unit, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct sensor_sample, value, JSON_TOK_NUMBER),
};

/* MQTT connectivity status flag */
bool mqtt_connected;

/* MQTT client ID buffer */
static uint8_t client_id[50];

#if defined(CONFIG_MQTT_LIB_TLS)
#include "tls_config/cert.h"

/* This should match the CN field in the server's CA cert */
#define TLS_SNI_HOSTNAME	CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME
#define APP_CA_CERT_TAG		1

static const sec_tag_t m_sec_tags[] = {
	APP_CA_CERT_TAG,
};

/** Register CA certificate for TLS */
static int tls_init(void)
{
	int rc;

	rc = tls_credential_add(APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));
	if (rc < 0) {
		LOG_ERR("Failed to register public certificate: %d", rc);
		return rc;
	}

	return rc;
}
#endif

static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}
#if defined(CONFIG_MQTT_LIB_TLS)
	else if (client->transport.type == MQTT_TRANSPORT_SECURE) {
		fds[0].fd = client->transport.tls.sock;
	}
#endif

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

/** Initialise the MQTT client ID as the board name with random hex postfix */
static void init_mqtt_client_id(void)
{
	snprintk(client_id, sizeof(client_id), CONFIG_BOARD"_%x", (uint8_t)sys_rand32_get());
}

static inline void on_mqtt_connect(void)
{
	mqtt_connected = true;
	device_write_led(LED_NET, LED_ON);
	LOG_INF("Connected to MQTT broker!");
	LOG_INF("Hostname: %s", CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME);
	LOG_INF("Client ID: %s", client_id);
	LOG_INF("Port: %s", CONFIG_NET_SAMPLE_MQTT_BROKER_PORT);
	LOG_INF("TLS: %s",
		IS_ENABLED(CONFIG_MQTT_LIB_TLS) ? "Enabled" : "Disabled");
}

static inline void on_mqtt_disconnect(void)
{
	mqtt_connected = false;
	clear_fds();
	device_write_led(LED_NET, LED_OFF);
	LOG_INF("Disconnected from MQTT broker");
}

/** Called when an MQTT payload is received.
 *  Reads the payload and calls the commands
 *  handler if a payloads is received on the
 *  command topic
 */
static void on_mqtt_publish(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
	int rc;
	uint8_t payload[CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE];

	rc = mqtt_read_publish_payload(client, payload,
					CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE);
	if (rc < 0) {
		LOG_ERR("Failed to read received MQTT payload [%d]", rc);
		return;
	}
	/* Place null terminator at end of payload buffer */
	payload[rc] = '\0';

	LOG_INF("MQTT payload received!");
	LOG_INF("topic: '%s', payload: %s",
		evt->param.publish.message.topic.topic.utf8, payload);

	/* If the topic is a command, call the command handler  */
	if (strcmp(evt->param.publish.message.topic.topic.utf8,
			CONFIG_NET_SAMPLE_MQTT_SUB_TOPIC_CMD) == 0) {
		device_command_handler(payload);
	}
}

/** Handler for asynchronous MQTT events */
static void mqtt_event_handler(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT Event Connect failed [%d]", evt->result);
			break;
		}
		on_mqtt_connect();
		break;

	case MQTT_EVT_DISCONNECT:
		on_mqtt_disconnect();
		break;

	case MQTT_EVT_PINGRESP:
		LOG_INF("PINGRESP packet");
		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error [%d]", evt->result);
			break;
		}

		LOG_INF("PUBACK packet ID: %u", evt->param.puback.message_id);
		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREC error [%d]", evt->result);
			break;
		}

		LOG_INF("PUBREC packet ID: %u", evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		mqtt_publish_qos2_release(client, &rel_param);
		break;

	case MQTT_EVT_PUBREL:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREL error [%d]", evt->result);
			break;
		}

		LOG_INF("PUBREL packet ID: %u", evt->param.pubrel.message_id);

		const struct mqtt_pubcomp_param rec_param = {
			.message_id = evt->param.pubrel.message_id
		};

		mqtt_publish_qos2_complete(client, &rec_param);
		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBCOMP error %d", evt->result);
			break;
		}

		LOG_INF("PUBCOMP packet ID: %u", evt->param.pubcomp.message_id);
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result == 0x80) {
			LOG_ERR("MQTT SUBACK error [%d]", evt->result);
			break;
		}

		LOG_INF("SUBACK packet ID: %d", evt->param.suback.message_id);
		break;

	case MQTT_EVT_PUBLISH:
		const struct mqtt_publish_param *p = &evt->param.publish;

		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack_param = {
				.message_id = p->message_id
			};
			mqtt_publish_qos1_ack(client, &ack_param);
		} else if (p->message.topic.qos == MQTT_QOS_2_EXACTLY_ONCE) {
			const struct mqtt_pubrec_param rec_param = {
				.message_id = p->message_id
			};
			mqtt_publish_qos2_receive(client, &rec_param);
		}

		on_mqtt_publish(client, evt);

	default:
		break;
	}
}

/** Poll the MQTT socket for received data */
static int poll_mqtt_socket(struct mqtt_client *client, int timeout)
{
	int rc;

	prepare_fds(client);

	if (nfds <= 0) {
		return -EINVAL;
	}

	rc = zsock_poll(fds, nfds, timeout);
	if (rc < 0) {
		LOG_ERR("Socket poll error [%d]", rc);
	}

	return rc;
}

/** Retrieves a sensor sample and encodes it in JSON format */
static int get_mqtt_payload(struct mqtt_binstr *payload)
{
	int rc;
	struct sensor_sample sample;

	rc = device_read_sensor(&sample);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor sample [%d]", rc);
		return rc;
	}

	rc = json_obj_encode_buf(sensor_sample_descr, ARRAY_SIZE(sensor_sample_descr),
					&sample, payload_buf, CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE);
	if (rc != 0) {
		LOG_ERR("Failed to encode JSON object [%d]", rc);
		return rc;
	}

	payload->data = payload_buf;
	payload->len = strlen(payload->data);

	return rc;
}

int app_mqtt_publish(struct mqtt_client *client)
{
	int rc;
	struct mqtt_publish_param param;
	struct mqtt_binstr payload;
	static uint16_t msg_id = 1;
	struct mqtt_topic topic = {
		.topic = {
			.utf8 = CONFIG_NET_SAMPLE_MQTT_PUB_TOPIC,
			.size = strlen(topic.topic.utf8)
		},
		.qos = IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_QOS_0_AT_MOST_ONCE) ? 0 :
			(IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_QOS_1_AT_LEAST_ONCE) ? 1 : 2)
	};

	rc = get_mqtt_payload(&payload);
	if (rc != 0) {
		LOG_ERR("Failed to get MQTT payload [%d]", rc);
	}

	param.message.topic = topic;
	param.message.payload = payload;
	param.message_id = msg_id++;
	param.dup_flag = 0;
	param.retain_flag = 0;

	rc = mqtt_publish(client, &param);
	if (rc != 0) {
		LOG_ERR("MQTT Publish failed [%d]", rc);
	}

	LOG_INF("Published to topic '%s', QoS %d",
			param.message.topic.topic.utf8,
			param.message.topic.qos);

	return rc;
}

int app_mqtt_subscribe(struct mqtt_client *client)
{
	int rc;
	struct mqtt_topic sub_topics[] = {
		{
			.topic = {
				.utf8 = CONFIG_NET_SAMPLE_MQTT_SUB_TOPIC_CMD,
				.size = strlen(sub_topics->topic.utf8)
			},
			.qos = IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_QOS_0_AT_MOST_ONCE) ? 0 :
				(IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_QOS_1_AT_LEAST_ONCE) ? 1 : 2)
		}
	};
	const struct mqtt_subscription_list sub_list = {
		.list = sub_topics,
		.list_count = ARRAY_SIZE(sub_topics),
		.message_id = 5841u
	};

	LOG_INF("Subscribing to %d topic(s)", sub_list.list_count);

	rc = mqtt_subscribe(client, &sub_list);
	if (rc != 0) {
		LOG_ERR("MQTT Subscribe failed [%d]", rc);
	}

	return rc;
}

/** Process incoming MQTT data and keep the connection alive*/
int app_mqtt_process(struct mqtt_client *client)
{
	int rc;

	rc = poll_mqtt_socket(client, mqtt_keepalive_time_left(client));
	if (rc != 0) {
		if (fds[0].revents & ZSOCK_POLLIN) {
			/* MQTT data received */
			rc = mqtt_input(client);
			if (rc != 0) {
				LOG_ERR("MQTT Input failed [%d]", rc);
				return rc;
			}
			/* Socket error */
			if (fds[0].revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR)) {
				LOG_ERR("MQTT socket closed / error");
				return -ENOTCONN;
			}
		}
	} else {
		/* Socket poll timed out, time to call mqtt_live() */
		rc = mqtt_live(client);
		if (rc != 0) {
			LOG_ERR("MQTT Live failed [%d]", rc);
			return rc;
		}
	}

	return 0;
}

void app_mqtt_run(struct mqtt_client *client)
{
	int rc;

	/* Subscribe to MQTT topics */
	app_mqtt_subscribe(client);

	/* Thread will primarily remain in this loop */
	while (mqtt_connected) {
		rc = app_mqtt_process(client);
		if (rc != 0) {
			break;
		}
	}
	/* Gracefully close connection */
	mqtt_disconnect(client, NULL);
}

void app_mqtt_connect(struct mqtt_client *client)
{
	int rc = 0;

	mqtt_connected = false;

	/* Block until MQTT CONNACK event callback occurs */
	while (!mqtt_connected) {
		rc = mqtt_connect(client);
		if (rc != 0) {
			LOG_ERR("MQTT Connect failed [%d]", rc);
			k_msleep(MSECS_WAIT_RECONNECT);
			continue;
		}

		/* Poll MQTT socket for response */
		rc = poll_mqtt_socket(client, MSECS_NET_POLL_TIMEOUT);
		if (rc > 0) {
			mqtt_input(client);
		}

		if (!mqtt_connected) {
			mqtt_abort(client);
		}
	}
}

int app_mqtt_init(struct mqtt_client *client)
{
	int rc;
	uint8_t broker_ip[NET_IPV4_ADDR_LEN];
	struct sockaddr_in *broker4;
	struct addrinfo *result;
	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	/* Resolve IP address of MQTT broker */
	rc = getaddrinfo(CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME,
				CONFIG_NET_SAMPLE_MQTT_BROKER_PORT, &hints, &result);
	if (rc != 0) {
		LOG_ERR("Failed to resolve broker hostname [%s]", gai_strerror(rc));
		return -EIO;
	}
	if (result == NULL) {
		LOG_ERR("Broker address not found");
		return -ENOENT;
	}

	broker4 = (struct sockaddr_in *)&broker;
	broker4->sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	broker4->sin_family = AF_INET;
	broker4->sin_port = ((struct sockaddr_in *)result->ai_addr)->sin_port;
	freeaddrinfo(result);

	/* Log resolved IP address */
	inet_ntop(AF_INET, &broker4->sin_addr.s_addr, broker_ip, sizeof(broker_ip));
	LOG_INF("Connecting to MQTT broker @ %s", broker_ip);

	/* MQTT client configuration */
	init_mqtt_client_id();
	mqtt_client_init(client);
	client->broker = &broker;
	client->evt_cb = mqtt_event_handler;
	client->client_id.utf8 = client_id;
	client->client_id.size = strlen(client->client_id.utf8);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
#if defined(CONFIG_MQTT_LIB_TLS)
	struct mqtt_sec_config *tls_config;

	client->transport.type = MQTT_TRANSPORT_SECURE;

	rc = tls_init();
	if (rc != 0) {
		LOG_ERR("TLS init error");
		return rc;
	}

	tls_config = &client->transport.tls.config;
	tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = m_sec_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
#if defined(CONFIG_MBEDTLS_SERVER_NAME_INDICATION)
	tls_config->hostname = TLS_SNI_HOSTNAME;
#else
	tls_config->hostname = NULL;
#endif /* CONFIG_MBEDTLS_SERVER_NAME_INDICATION */
#endif /* CONFIG_MQTT_LIB_TLS */

	return rc;
}
