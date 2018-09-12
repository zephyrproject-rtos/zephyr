/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/mqtt.h>

#include <net/net_context.h>

#include <misc/printk.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_NET_L2_BT)
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#endif

#include "config.h"

#define CONN_TRIES 20

/* Container for some structures used by the MQTT publisher app. */
struct mqtt_client_ctx {
	/**
	 * The connect message structure is only used during the connect
	 * stage. Developers must set some msg properties before calling the
	 * mqtt_tx_connect routine. See below.
	 */
	struct mqtt_connect_msg connect_msg;
	/**
	 * This is the message that will be received by the server
	 * (MQTT broker).
	 */
	struct mqtt_publish_msg pub_msg;

	/**
	 * This is the MQTT application context variable.
	 */
	struct mqtt_ctx mqtt_ctx;

	/**
	 * This variable will be passed to the connect callback, declared inside
	 * the mqtt context struct. If not used, it could be set to NULL.
	 */
	void *connect_data;

	/**
	 * This variable will be passed to the disconnect callback, declared
	 * inside the mqtt context struct. If not used, it could be set to NULL.
	 */
	void *disconnect_data;

	/**
	 * This variable will be passed to the publish_tx callback, declared
	 * inside the mqtt context struct. If not used, it could be set to NULL.
	 */
	void *publish_data;
};

/* The mqtt client struct */
static struct mqtt_client_ctx client_ctx;

/* This routine sets some basic properties for the network context variable */
static int network_setup(void);

#if defined(CONFIG_MQTT_LIB_TLS)

#include "test_certs.h"

/* TLS */
#define TLS_SNI_HOSTNAME "localhost"
#define TLS_REQUEST_BUF_SIZE 1500
#define TLS_PRIVATE_DATA "Zephyr TLS mqtt publisher"

static u8_t tls_request_buf[TLS_REQUEST_BUF_SIZE];

NET_STACK_DEFINE(mqtt_tls_stack, tls_stack,
		CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

NET_APP_TLS_POOL_DEFINE(tls_mem_pool, 30);

int setup_cert(struct net_app_ctx *ctx, void *cert)
{
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	mbedtls_ssl_conf_psk(&ctx->tls.mbedtls.conf,
			client_psk, sizeof(client_psk),
			(const unsigned char *)client_psk_id,
			sizeof(client_psk_id) - 1);
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	{
		mbedtls_x509_crt *ca_cert = cert;
		int ret;

		ret = mbedtls_x509_crt_parse_der(ca_cert,
				ca_certificate,
				sizeof(ca_certificate));
		if (ret != 0) {
			NET_ERR("mbedtls_x509_crt_parse_der failed "
					"(-0x%x)", -ret);
			return ret;
		}

		/* mbedtls_x509_crt_verify() should be called to verify the
		 * cerificate in the real cases
		 */

		mbedtls_ssl_conf_ca_chain(&ctx->tls.mbedtls.conf,
				ca_cert, NULL);

		mbedtls_ssl_conf_authmode(&ctx->tls.mbedtls.conf,
				MBEDTLS_SSL_VERIFY_REQUIRED);

		mbedtls_ssl_conf_cert_profile(&ctx->tls.mbedtls.conf,
				&mbedtls_x509_crt_profile_default);
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return 0;
}
#endif

/* The signature of this routine must match the connect callback declared at
 * the mqtt.h header.
 */
static void connect_cb(struct mqtt_ctx *mqtt_ctx)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	printk("[%s:%d]", __func__, __LINE__);

	if (client_ctx->connect_data) {
		printk(" user_data: %s",
		       (const char *)client_ctx->connect_data);
	}

	printk("\n");
}

/* The signature of this routine must match the disconnect callback declared at
 * the mqtt.h header.
 */
static void disconnect_cb(struct mqtt_ctx *mqtt_ctx)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	printk("[%s:%d]", __func__, __LINE__);

	if (client_ctx->disconnect_data) {
		printk(" user_data: %s",
		       (const char *)client_ctx->disconnect_data);
	}

	printk("\n");
}

/**
 * The signature of this routine must match the publish_tx callback declared at
 * the mqtt.h header.
 *
 * NOTE: we have two callbacks for MQTT Publish related stuff:
 *	- publish_tx, for publishers
 *	- publish_rx, for subscribers
 *
 * Applications must keep a "message database" with pkt_id's. So far, this is
 * not implemented here. For example, if we receive a PUBREC message with an
 * unknown pkt_id, this routine must return an error, for example -EINVAL or
 * any negative value.
 */
static int publish_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_id,
		      enum mqtt_packet type)
{
	struct mqtt_client_ctx *client_ctx;
	const char *str;
	int rc = 0;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	switch (type) {
	case MQTT_PUBACK:
		str = "MQTT_PUBACK";
		break;
	case MQTT_PUBCOMP:
		str = "MQTT_PUBCOMP";
		break;
	case MQTT_PUBREC:
		str = "MQTT_PUBREC";
		break;
	default:
		rc = -EINVAL;
		str = "Invalid MQTT packet";
	}

	printk("[%s:%d] <%s> packet id: %u", __func__, __LINE__, str, pkt_id);

	if (client_ctx->publish_data) {
		printk(", user_data: %s",
		       (const char *)client_ctx->publish_data);
	}

	printk("\n");

	return rc;
}

/**
 * The signature of this routine must match the malformed callback declared at
 * the mqtt.h header.
 */
static void malformed_cb(struct mqtt_ctx *mqtt_ctx, u16_t pkt_type)
{
	printk("[%s:%d] pkt_type: %u\n", __func__, __LINE__, pkt_type);
}

static char *get_mqtt_payload(enum mqtt_qos qos)
{
#if APP_BLUEMIX_TOPIC
	static char payload[30];

	snprintk(payload, sizeof(payload), "{d:{temperature:%d}}",
		 (u8_t)sys_rand32_get());
#else
	static char payload[] = "DOORS:OPEN_QoSx";

	payload[strlen(payload) - 1] = '0' + qos;
#endif

	return payload;
}

static char *get_mqtt_topic(void)
{
#if APP_BLUEMIX_TOPIC
	return "iot-2/type/"BLUEMIX_DEVTYPE"/id/"BLUEMIX_DEVID
	       "/evt/"BLUEMIX_EVENT"/fmt/"BLUEMIX_FORMAT;
#else
	return "sensors";
#endif
}

static void prepare_mqtt_publish_msg(struct mqtt_publish_msg *pub_msg,
				     enum mqtt_qos qos)
{
	/* MQTT message payload may be anything, we we use C strings */
	pub_msg->msg = get_mqtt_payload(qos);
	/* Payload's length */
	pub_msg->msg_len = strlen(client_ctx.pub_msg.msg);
	/* MQTT Quality of Service */
	pub_msg->qos = qos;
	/* Message's topic */
	pub_msg->topic = get_mqtt_topic();
	pub_msg->topic_len = strlen(client_ctx.pub_msg.topic);
	/* Packet Identifier, always use different values */
	pub_msg->pkt_id = sys_rand32_get();
}

#define RC_STR(rc)	((rc) == 0 ? "OK" : "ERROR")

#define PRINT_RESULT(func, rc)	\
	printk("[%s:%d] %s: %d <%s>\n", __func__, __LINE__, \
	       (func), rc, RC_STR(rc))

/* In this routine we block until the connected variable is 1 */
static int try_to_connect(struct mqtt_client_ctx *client_ctx)
{
	int i = 0;

	while (i++ < APP_CONNECT_TRIES && !client_ctx->mqtt_ctx.connected) {
		int rc;

		rc = mqtt_tx_connect(&client_ctx->mqtt_ctx,
				     &client_ctx->connect_msg);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_connect", rc);
		if (rc != 0) {
			continue;
		}
	}

	if (client_ctx->mqtt_ctx.connected) {
		return 0;
	}

	return -EINVAL;
}

static void publisher(void)
{
	int i, rc;

	/* Set everything to 0 and later just assign the required fields. */
	(void)memset(&client_ctx, 0x00, sizeof(client_ctx));

	/* connect, disconnect and malformed may be set to NULL */
	client_ctx.mqtt_ctx.connect = connect_cb;

	client_ctx.mqtt_ctx.disconnect = disconnect_cb;
	client_ctx.mqtt_ctx.malformed = malformed_cb;

	client_ctx.mqtt_ctx.net_init_timeout = APP_NET_INIT_TIMEOUT;
	client_ctx.mqtt_ctx.net_timeout = APP_TX_RX_TIMEOUT;

	client_ctx.mqtt_ctx.peer_addr_str = SERVER_ADDR;
	client_ctx.mqtt_ctx.peer_port = SERVER_PORT;

#if defined(CONFIG_MQTT_LIB_TLS)
	/** TLS setup */
	client_ctx.mqtt_ctx.request_buf = tls_request_buf;
	client_ctx.mqtt_ctx.request_buf_len = TLS_REQUEST_BUF_SIZE;
	client_ctx.mqtt_ctx.personalization_data = TLS_PRIVATE_DATA;
	client_ctx.mqtt_ctx.personalization_data_len = strlen(TLS_PRIVATE_DATA);
	client_ctx.mqtt_ctx.cert_host = TLS_SNI_HOSTNAME;
	client_ctx.mqtt_ctx.tls_mem_pool = &tls_mem_pool;
	client_ctx.mqtt_ctx.tls_stack = tls_stack;
	client_ctx.mqtt_ctx.tls_stack_size = K_THREAD_STACK_SIZEOF(tls_stack);
	client_ctx.mqtt_ctx.cert_cb = setup_cert;
	client_ctx.mqtt_ctx.entropy_src_cb = NULL;
#endif

	/* Publisher apps TX the MQTT PUBLISH msg */
	client_ctx.mqtt_ctx.publish_tx = publish_cb;

	/* The connect message will be sent to the MQTT server (broker).
	 * If clean_session here is 0, the mqtt_ctx clean_session variable
	 * will be set to 0 also. Please don't do that, set always to 1.
	 * Clean session = 0 is not yet supported.
	 */
	client_ctx.connect_msg.client_id = MQTT_CLIENTID;
	client_ctx.connect_msg.client_id_len = strlen(MQTT_CLIENTID);
	client_ctx.connect_msg.clean_session = 1;

	client_ctx.connect_data = "CONNECTED";
	client_ctx.disconnect_data = "DISCONNECTED";
	client_ctx.publish_data = "PUBLISH";

	rc = network_setup();
	PRINT_RESULT("network_setup", rc);
	if (rc < 0) {
		return;
	}

	rc = mqtt_init(&client_ctx.mqtt_ctx, MQTT_APP_PUBLISHER);
	PRINT_RESULT("mqtt_init", rc);
	if (rc != 0) {
		return;
	}

	for (i = 0; i < CONN_TRIES; i++) {
		rc = mqtt_connect(&client_ctx.mqtt_ctx);
		PRINT_RESULT("mqtt_connect", rc);
		if (!rc) {
			goto connected;
		}
	}

	goto exit_app;

connected:

	rc = try_to_connect(&client_ctx);
	PRINT_RESULT("try_to_connect", rc);
	if (rc != 0) {
		goto exit_app;
	}

	i = 0;
	while (i++ < APP_MAX_ITERATIONS) {
		rc = mqtt_tx_pingreq(&client_ctx.mqtt_ctx);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_pingreq", rc);

		prepare_mqtt_publish_msg(&client_ctx.pub_msg, MQTT_QoS0);
		rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_publish", rc);

		prepare_mqtt_publish_msg(&client_ctx.pub_msg, MQTT_QoS1);
		rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_publish", rc);

		prepare_mqtt_publish_msg(&client_ctx.pub_msg, MQTT_QoS2);
		rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
		k_sleep(APP_SLEEP_MSECS);
		PRINT_RESULT("mqtt_tx_publish", rc);
	}

	rc = mqtt_tx_disconnect(&client_ctx.mqtt_ctx);
	PRINT_RESULT("mqtt_tx_disconnect", rc);

exit_app:

	mqtt_close(&client_ctx.mqtt_ctx);

	printk("\nBye!\n");
}

#if defined(CONFIG_NET_L2_BT)
static bool bt_connected;

static
void bt_connect_cb(struct bt_conn *conn, u8_t err)
{
	bt_connected = true;
}

static
void bt_disconnect_cb(struct bt_conn *conn, u8_t reason)
{
	bt_connected = false;
	printk("bt disconnected (reason %u)\n", reason);
}

static
struct bt_conn_cb bt_conn_cb = {
	.connected = bt_connect_cb,
	.disconnected = bt_disconnect_cb,
};
#endif

static int network_setup(void)
{

#if defined(CONFIG_NET_L2_BT)
	const char *progress_mark = "/-\\|";
	int i = 0;
	int rc;

	rc = bt_enable(NULL);
	if (rc) {
		printk("bluetooth init failed\n");
		return rc;
	}

	bt_conn_cb_register(&bt_conn_cb);

	printk("\nwaiting for bt connection: ");
	while (bt_connected == false) {
		k_sleep(250);
		printk("%c\b", progress_mark[i]);
		i = (i + 1) % (sizeof(progress_mark) - 1);
	}
	printk("\n");
#endif

	return 0;
}

void main(void)
{
	publisher();
}
