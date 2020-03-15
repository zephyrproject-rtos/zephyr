/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(aws_mqtt, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <net/socket.h>
#include <net/mqtt.h>
#include <net/net_config.h>
#include <net/net_event.h>
#include <net/sntp.h>
#include <stdio.h>
#include <sys/printk.h>
#include <string.h>
#include <errno.h>
#include "dhcp.h"
#include <time.h>
#include <inttypes.h>
#include "certificates.h"
#include "config.h"

#define APP_CA_CERT_TAG 1
#define APP_PRIVATE_SERVER_KEY_TAG 2

#define RC_STR(rc) ((rc) == 0 ? "OK" : "ERROR")
#define PRINT_RESULT(func, rc) \
	LOG_INF("%s: %d <%s>", (func), rc, RC_STR(rc))

static sec_tag_t m_sec_tags[] = {
    APP_CA_CERT_TAG,
    APP_PRIVATE_SERVER_KEY_TAG,
};


/* Buffers for MQTT client. */
static u8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
static u8_t tx_buffer[APP_MQTT_BUFFER_SIZE];
static bool connected;
static u64_t next_alive;
struct zsock_addrinfo *haddr;
static struct sockaddr_storage broker;
static struct mqtt_client client_ctx;


static void broker_init(void)
{
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(CONFIG_AWS_PORT);

	net_ipaddr_copy(&broker4->sin_addr,
			&net_sin(haddr->ai_addr)->sin_addr);

}

void mqtt_evt_handler(struct mqtt_client *const client,
		const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
		case MQTT_EVT_CONNACK:
			if (evt->result != 0) {
				LOG_ERR("MQTT connect failed %d", evt->result);
				break;
			}

			connected = true;
			LOG_INF("MQTT client connected!");
			break;

		case MQTT_EVT_DISCONNECT:
			LOG_INF("MQTT client disconnected %d", evt->result);
			connected = false;
			break;

		case MQTT_EVT_PUBACK:
			if (evt->result != 0) {
				LOG_ERR("MQTT PUBACK error %d", evt->result);
				break;
			}

			LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);

			break;

		case MQTT_EVT_PUBREC:
			if (evt->result != 0) {
				LOG_ERR("MQTT PUBREC error %d", evt->result);
				break;
			}

			LOG_INF("PUBREC packet id: %u", evt->param.pubrec.message_id);

			const struct mqtt_pubrel_param rel_param = {
				.message_id = evt->param.pubrec.message_id
			};
			err = mqtt_publish_qos2_release(client, &rel_param);
			if (err != 0) {
				LOG_ERR("Failed to send MQTT PUBREL: %d", err);
			}

			break;

		case MQTT_EVT_PUBCOMP:
			if (evt->result != 0) {
				LOG_ERR("MQTT PUBCOMP error %d", evt->result);
				break;
			}

			LOG_INF("PUBCOMP packet id: %u",
					evt->param.pubcomp.message_id);

			break;

		default:
			break;
	}
}

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);
	broker_init();

	client->password = NULL;
	client->user_name = NULL;

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (u8_t *)MQTT_CLIENTID;
	client->client_id.size = strlen(MQTT_CLIENTID);
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	client->transport.type = MQTT_TRANSPORT_SECURE;
	struct mqtt_sec_config *tls_config = &client->transport.tls.config;
	tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = m_sec_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
	tls_config->hostname = CONFIG_AWS_HOSTNAME;
	client->transport.type = MQTT_TRANSPORT_SECURE;
}

static int wait_for_input(int timeout)
{
        int res;
        struct zsock_pollfd fds[1] = {
                [0] = {.fd = client_ctx.transport.tls.sock,
                      .events = ZSOCK_POLLIN,
                      .revents = 0},
        };

        res = zsock_poll(fds, 1, timeout);
        if (res < 0) {
                LOG_ERR("poll read event error");
                return -errno;
        }
        return res;
}

static int try_to_connect(struct mqtt_client *client)
{
	int rc;

	LOG_DBG("attempting to connect...");

	client_init(client);
	rc = mqtt_connect(client);
	if (rc != 0) {
		PRINT_RESULT("mqtt_connect", rc);
	}else {
		if (wait_for_input(5000) > 0) {
			mqtt_input(client);
			if (!connected) {
				LOG_INF("Aborting connection\n");
				mqtt_abort(client);
			}
		}
	}
	if (connected) {
		return 0;
	}
	return -EINVAL;
}

static void publish(struct mqtt_client *client, enum mqtt_qos qos)
{
        char payload[] = "{id=123}";
        char topic[] = "/things/";
        u8_t len = strlen(topic);
        struct mqtt_publish_param param;
	char pub_msg[64];
	int err;

        param.message.topic.qos = qos;
        param.message.topic.topic.utf8 = (u8_t *)topic;
        param.message.topic.topic.size = len;
        param.message.payload.data = pub_msg;
        param.message.payload.len = strlen(payload);
        param.message_id = sys_rand32_get();
        param.dup_flag = 0U;
        param.retain_flag = 0U;


	mqtt_live(client);
	next_alive = k_uptime_get() + ALIVE_TIME;

        while (1) {
                LOG_INF("Publishing data");
                sprintf(pub_msg, "%s: %d\n", "payload", param.message_id);
                param.message.payload.len = strlen(pub_msg);
                err = mqtt_publish(client, &param);
                if (err) {
                        LOG_ERR("could not publish, error %d", err);
                        break;
                }

                /* idle and process messages */
                while (k_uptime_get() < next_alive) {
                        LOG_INF("... idling ...");
                        if (wait_for_input(5000) > 0) {
                                mqtt_input(client);
                        }
                }
                mqtt_live(client);
                next_alive += ALIVE_TIME;
        }
}

static int tls_init(void)
{
        int err = -EINVAL;

        err = tls_credential_add(APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
                                 amazon_certificate, sizeof(amazon_certificate));
        if (err < 0) {
                LOG_ERR("Failed to register ca certificate: %d", err);
                return err;
        }

        err = tls_credential_add(APP_PRIVATE_SERVER_KEY_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
                                 private_key, sizeof(private_key));
        if (err < 0) {
                LOG_ERR("Failed to register private key: %d", err);
                return err;
        }

        err = tls_credential_add(APP_PRIVATE_SERVER_KEY_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE,
                                 server_cert, sizeof(server_cert));
        if (err < 0) {
                LOG_ERR("Failed to register server certificate: %d", err);
                return err;
        }
        return err;
}

void mqtt_startup(char *hostname, int port)
{
	struct mqtt_client *client = &client_ctx;
	int err, cnt;	
	static struct zsock_addrinfo hints;
	int res = 0;
	int rc;


	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	cnt = 0;
	while ((err = getaddrinfo(CONFIG_AWS_HOSTNAME, "8883", &hints,
					&haddr)) && cnt < 3) {
		LOG_ERR("Unable to get address for broker, retrying");
		cnt++;
	}
	if (err != 0) {
		LOG_ERR("Unable to get address for broker, error %d",
				res);
		return;
	}
	LOG_INF("DNS resolved for %s:%d", CONFIG_AWS_HOSTNAME, CONFIG_AWS_PORT);
	rc = try_to_connect(client);
	PRINT_RESULT("try_to_connect", rc);
	if (rc == 0) {
		publish(&client_ctx, MQTT_QOS_1_AT_LEAST_ONCE);
	}

}

void main(void)
{
        int rc;

	app_dhcpv4_startup();
        rc = tls_init();
	if (rc) {
		return;
	}
	mqtt_startup(CONFIG_AWS_HOSTNAME, CONFIG_AWS_PORT);	
}
