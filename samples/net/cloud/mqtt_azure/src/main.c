/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt_azure, LOG_LEVEL_DBG);

#include <zephyr/zephyr.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include <zephyr/sys/printk.h>
#include <zephyr/random/rand32.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "test_certs.h"

/* Buffers for MQTT client. */
static uint8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
static uint8_t tx_buffer[APP_MQTT_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client_ctx;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

#if defined(CONFIG_SOCKS)
static struct sockaddr socks5_proxy;
#endif

/* Socket Poll */
static struct zsock_pollfd fds[1];
static int nfds;

static bool mqtt_connected;

static struct k_work_delayable pub_message;
#if defined(CONFIG_NET_DHCPV4)
static struct k_work_delayable check_network_conn;

/* Network Management events */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

static struct net_mgmt_event_callback l4_mgmt_cb;
#endif

#if defined(CONFIG_DNS_RESOLVER)
static struct zsock_addrinfo hints;
static struct zsock_addrinfo *haddr;
#endif

static K_SEM_DEFINE(mqtt_start, 0, 1);

/* Application TLS configuration details */
#define TLS_SNI_HOSTNAME CONFIG_SAMPLE_CLOUD_AZURE_HOSTNAME
#define APP_CA_CERT_TAG 1

static sec_tag_t m_sec_tags[] = {
	APP_CA_CERT_TAG,
};

static uint8_t topic[] = "devices/" MQTT_CLIENTID "/messages/devicebound/#";
static struct mqtt_topic subs_topic;
static struct mqtt_subscription_list subs_list;

static void mqtt_event_handler(struct mqtt_client *const client,
			       const struct mqtt_evt *evt);

static int tls_init(void)
{
	int err;

	err = tls_credential_add(APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

	return err;
}

static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_SECURE) {
		fds[0].fd = client->transport.tls.sock;
	}

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

static int wait(int timeout)
{
	int rc = -EINVAL;

	if (nfds <= 0) {
		return rc;
	}

	rc = zsock_poll(fds, nfds, timeout);
	if (rc < 0) {
		LOG_ERR("poll error: %d", errno);
		return -errno;
	}

	return rc;
}

static void broker_init(void)
{
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(SERVER_PORT);

#if defined(CONFIG_DNS_RESOLVER)
	net_ipaddr_copy(&broker4->sin_addr,
			&net_sin(haddr->ai_addr)->sin_addr);
#else
	zsock_inet_pton(AF_INET, SERVER_ADDR, &broker4->sin_addr);
#endif

#if defined(CONFIG_SOCKS)
	struct sockaddr_in *proxy4 = (struct sockaddr_in *)&socks5_proxy;

	proxy4->sin_family = AF_INET;
	proxy4->sin_port = htons(SOCKS5_PROXY_PORT);
	zsock_inet_pton(AF_INET, SOCKS5_PROXY_ADDR, &proxy4->sin_addr);
#endif
}

static void client_init(struct mqtt_client *client)
{
	static struct mqtt_utf8 password;
	static struct mqtt_utf8 username;
	struct mqtt_sec_config *tls_config;

	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_event_handler;

	client->client_id.utf8 = (uint8_t *)MQTT_CLIENTID;
	client->client_id.size = strlen(MQTT_CLIENTID);

	password.utf8 = (uint8_t *)CONFIG_SAMPLE_CLOUD_AZURE_PASSWORD;
	password.size = strlen(CONFIG_SAMPLE_CLOUD_AZURE_PASSWORD);

	client->password = &password;

	username.utf8 = (uint8_t *)CONFIG_SAMPLE_CLOUD_AZURE_USERNAME;
	username.size = strlen(CONFIG_SAMPLE_CLOUD_AZURE_USERNAME);

	client->user_name = &username;

	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
	client->transport.type = MQTT_TRANSPORT_SECURE;

	tls_config = &client->transport.tls.config;

	tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = m_sec_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
	tls_config->hostname = TLS_SNI_HOSTNAME;

#if defined(CONFIG_SOCKS)
	mqtt_client_set_proxy(client, &socks5_proxy,
			      socks5_proxy.sa_family == AF_INET ?
			      sizeof(struct sockaddr_in) :
			      sizeof(struct sockaddr_in6));
#endif
}

static void mqtt_event_handler(struct mqtt_client *const client,
			       const struct mqtt_evt *evt)
{
	struct mqtt_puback_param puback;
	uint8_t data[33];
	int len;
	int bytes_read;

	switch (evt->type) {
	case MQTT_EVT_SUBACK:
		LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);
		break;

	case MQTT_EVT_UNSUBACK:
		LOG_INF("UNSUBACK packet id: %u", evt->param.suback.message_id);
		break;

	case MQTT_EVT_CONNACK:
		if (evt->result) {
			LOG_ERR("MQTT connect failed %d", evt->result);
			break;
		}

		mqtt_connected = true;
		LOG_DBG("MQTT client connected!");
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_DBG("MQTT client disconnected %d", evt->result);

		mqtt_connected = false;
		clear_fds();
		break;

	case MQTT_EVT_PUBACK:
		if (evt->result) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			break;
		}

		LOG_DBG("PUBACK packet id: %u\n", evt->param.puback.message_id);
		break;

	case MQTT_EVT_PUBLISH:
		len = evt->param.publish.message.payload.len;

		LOG_INF("MQTT publish received %d, %d bytes", evt->result, len);
		LOG_INF(" id: %d, qos: %d", evt->param.publish.message_id,
			evt->param.publish.message.topic.qos);

		while (len) {
			bytes_read = mqtt_read_publish_payload(&client_ctx,
					data,
					len >= sizeof(data) - 1 ?
					sizeof(data) - 1 : len);
			if (bytes_read < 0 && bytes_read != -EAGAIN) {
				LOG_ERR("failure to read payload");
				break;
			}

			data[bytes_read] = '\0';
			LOG_INF("   payload: %s", log_strdup(data));
			len -= bytes_read;
		}

		puback.message_id = evt->param.publish.message_id;
		mqtt_publish_qos1_ack(&client_ctx, &puback);
		break;

	default:
		LOG_DBG("Unhandled MQTT event %d", evt->type);
		break;
	}
}

static void subscribe(struct mqtt_client *client)
{
	int err;

	/* subscribe */
	subs_topic.topic.utf8 = topic;
	subs_topic.topic.size = strlen(topic);
	subs_list.list = &subs_topic;
	subs_list.list_count = 1U;
	subs_list.message_id = 1U;

	err = mqtt_subscribe(client, &subs_list);
	if (err) {
		LOG_ERR("Failed on topic %s", topic);
	}
}

static int publish(struct mqtt_client *client, enum mqtt_qos qos)
{
	char payload[] = "{id=123}";
	char topic[] = "devices/" MQTT_CLIENTID "/messages/events/";
	uint8_t len = strlen(topic);
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (uint8_t *)topic;
	param.message.topic.topic.size = len;
	param.message.payload.data = payload;
	param.message.payload.len = strlen(payload);
	param.message_id = sys_rand32_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(client, &param);
}

static void poll_mqtt(void)
{
	int rc;

	while (mqtt_connected) {
		rc = wait(SYS_FOREVER_MS);
		if (rc > 0) {
			mqtt_input(&client_ctx);
		}
	}
}

/* Random time between 10 - 15 seconds
 * If you prefer to have this value more than CONFIG_MQTT_KEEPALIVE,
 * then keep the application connection live by calling mqtt_live()
 * in regular intervals.
 */
static uint8_t timeout_for_publish(void)
{
	return (10 + sys_rand32_get() % 5);
}

static void publish_timeout(struct k_work *work)
{
	int rc;

	if (!mqtt_connected) {
		return;
	}

	rc = publish(&client_ctx, MQTT_QOS_1_AT_LEAST_ONCE);
	if (rc) {
		LOG_ERR("mqtt_publish ERROR");
		goto end;
	}

	LOG_DBG("mqtt_publish OK");
end:
	k_work_reschedule(&pub_message, K_SECONDS(timeout_for_publish()));
}

static int try_to_connect(struct mqtt_client *client)
{
	uint8_t retries = 3U;
	int rc;

	LOG_DBG("attempting to connect...");

	while (retries--) {
		client_init(client);

		rc = mqtt_connect(client);
		if (rc) {
			LOG_ERR("mqtt_connect failed %d", rc);
			continue;
		}

		prepare_fds(client);

		rc = wait(APP_SLEEP_MSECS);
		if (rc < 0) {
			mqtt_abort(client);
			return rc;
		}

		mqtt_input(client);

		if (mqtt_connected) {
			subscribe(client);
			k_work_reschedule(&pub_message,
					  K_SECONDS(timeout_for_publish()));
			return 0;
		}

		mqtt_abort(client);

		wait(10 * MSEC_PER_SEC);
	}

	return -EINVAL;
}

#if defined(CONFIG_DNS_RESOLVER)
static int get_mqtt_broker_addrinfo(void)
{
	int retries = 3;
	int rc = -EINVAL;

	while (retries--) {
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = 0;

		rc = zsock_getaddrinfo(CONFIG_SAMPLE_CLOUD_AZURE_HOSTNAME, "8883",
				       &hints, &haddr);
		if (rc == 0) {
			LOG_INF("DNS resolved for %s:%d",
			CONFIG_SAMPLE_CLOUD_AZURE_HOSTNAME,
			CONFIG_SAMPLE_CLOUD_AZURE_SERVER_PORT);

			return 0;
		}

		LOG_ERR("DNS not resolved for %s:%d, retrying",
			CONFIG_SAMPLE_CLOUD_AZURE_HOSTNAME,
			CONFIG_SAMPLE_CLOUD_AZURE_SERVER_PORT);
	}

	return rc;
}
#endif

static void connect_to_cloud_and_publish(void)
{
	int rc = -EINVAL;

#if defined(CONFIG_NET_DHCPV4)
	while (true) {
		k_sem_take(&mqtt_start, K_FOREVER);
#endif
#if defined(CONFIG_DNS_RESOLVER)
		rc = get_mqtt_broker_addrinfo();
		if (rc) {
			return;
		}
#endif
		rc = try_to_connect(&client_ctx);
		if (rc) {
			return;
		}

		poll_mqtt();
#if defined(CONFIG_NET_DHCPV4)
	}
#endif
}

/* DHCP tries to renew the address after interface is down and up.
 * If DHCPv4 address renewal is success, then it doesn't generate
 * any event. We have to monitor this way.
 * If DHCPv4 attempts exceeds maximum number, it will delete iface
 * address and attempts for new request. In this case we can rely
 * on IPV4_ADDR_ADD event.
 */
#if defined(CONFIG_NET_DHCPV4)
static void check_network_connection(struct k_work *work)
{
	struct net_if *iface;

	if (mqtt_connected) {
		return;
	}

	iface = net_if_get_default();
	if (!iface) {
		goto end;
	}

	if (iface->config.dhcpv4.state == NET_DHCPV4_BOUND) {
		k_sem_give(&mqtt_start);
		return;
	}

	LOG_INF("waiting for DHCP to acquire addr");

end:
	k_work_reschedule(&check_network_conn, K_SECONDS(3));
}
#endif

#if defined(CONFIG_NET_DHCPV4)
static void abort_mqtt_connection(void)
{
	if (mqtt_connected) {
		mqtt_connected = false;
		mqtt_abort(&client_ctx);
		k_work_cancel_delayable(&pub_message);
	}
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & L4_EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		/* Wait for DHCP to be back in BOUND state */
		k_work_reschedule(&check_network_conn, K_SECONDS(3));

		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		abort_mqtt_connection();
		k_work_cancel_delayable(&check_network_conn);

		return;
	}
}
#endif

void main(void)
{
	int rc;

	LOG_DBG("Waiting for network to setup...");

	rc = tls_init();
	if (rc) {
		return;
	}

	k_work_init_delayable(&pub_message, publish_timeout);

#if defined(CONFIG_NET_DHCPV4)
	k_work_init_delayable(&check_network_conn, check_network_connection);

	net_mgmt_init_event_callback(&l4_mgmt_cb, l4_event_handler,
				     L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_mgmt_cb);
#endif

	connect_to_cloud_and_publish();
}
