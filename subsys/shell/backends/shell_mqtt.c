/*
 * Copyright (c) 2022 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell_mqtt.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/drivers/hwinfo.h>

SHELL_MQTT_DEFINE(shell_transport_mqtt);
SHELL_DEFINE(shell_mqtt, "", &shell_transport_mqtt,
	     CONFIG_SHELL_BACKEND_MQTT_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_BACKEND_MQTT_LOG_MESSAGE_QUEUE_TIMEOUT, SHELL_FLAG_OLF_CRLF);

LOG_MODULE_REGISTER(shell_mqtt, CONFIG_SHELL_MQTT_LOG_LEVEL);

#define NET_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONNECT_TIMEOUT_MS 2000
#define LISTEN_TIMEOUT_MS 500
#define MQTT_SEND_DELAY_MS K_MSEC(100)
#define PROCESS_INTERVAL K_SECONDS(2)
#define SHELL_MQTT_WORKQ_STACK_SIZE 2048

#ifdef CONFIG_SHELL_MQTT_SERVER_USERNAME
#define MQTT_USERNAME CONFIG_SHELL_MQTT_SERVER_USERNAME
#else
#define MQTT_USERNAME NULL
#endif /* CONFIG_SHELL_MQTT_SERVER_USERNAME */

#ifdef CONFIG_SHELL_MQTT_SERVER_PASSWORD
#define MQTT_PASSWORD CONFIG_SHELL_MQTT_SERVER_PASSWORD
#else
#define MQTT_PASSWORD NULL
#endif /*SHELL_MQTT_SERVER_PASSWORD */

struct shell_mqtt *sh_mqtt;
K_KERNEL_STACK_DEFINE(sh_mqtt_workq_stack, SHELL_MQTT_WORKQ_STACK_SIZE);

static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt);

static inline int sh_mqtt_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_reschedule_for_queue(&sh_mqtt->workq, dwork, delay);
}

static inline int sh_mqtt_work_submit(struct k_work *work)
{
	return k_work_submit_to_queue(&sh_mqtt->workq, work);
}

/* Lock the context of the shell mqtt */
static inline int sh_mqtt_context_lock(k_timeout_t timeout)
{
	return k_mutex_lock(&sh_mqtt->lock, timeout);
}

/* Unlock the context of the shell mqtt */
static inline void sh_mqtt_context_unlock(void)
{
	(void)k_mutex_unlock(&sh_mqtt->lock);
}

static void sh_mqtt_rx_rb_flush(void)
{
	uint8_t c;
	uint32_t size = ring_buf_size_get(&sh_mqtt->rx_rb);

	while (size > 0) {
		size = ring_buf_get(&sh_mqtt->rx_rb, &c, 1U);
	}
}

bool __weak shell_mqtt_get_devid(char *id, int id_max_len)
{
	uint8_t hwinfo_id[DEVICE_ID_BIN_MAX_SIZE];
	ssize_t length;

	length = hwinfo_get_device_id(hwinfo_id, DEVICE_ID_BIN_MAX_SIZE);
	if (length <= 0) {
		return false;
	}

	(void)memset(id, 0, id_max_len);
	length = bin2hex(hwinfo_id, (size_t)length, id, id_max_len);

	return length > 0;
}

static void prepare_fds(void)
{
	if (sh_mqtt->mqtt_cli.transport.type == MQTT_TRANSPORT_NON_SECURE) {
		sh_mqtt->fds[0].fd = sh_mqtt->mqtt_cli.transport.tcp.sock;
	}

	sh_mqtt->fds[0].events = ZSOCK_POLLIN;
	sh_mqtt->nfds = 1;
}

static void clear_fds(void)
{
	sh_mqtt->nfds = 0;
}

/*
 * Upon successful completion, poll() shall return a non-negative value. A positive value indicates
 * the total number of pollfd structures that have selected events (that is, those for which the
 * revents member is non-zero). A value of 0 indicates that the call timed out and no file
 * descriptors have been selected. Upon failure, poll() shall return -1 and set errno to indicate
 * the error.
 */
static int wait(int timeout)
{
	int rc = 0;

	if (sh_mqtt->nfds > 0) {
		rc = zsock_poll(sh_mqtt->fds, sh_mqtt->nfds, timeout);
		if (rc < 0) {
			LOG_ERR("poll error: %d", errno);
		}
	}

	return rc;
}

/* Query IP address for the broker URL */
static int get_mqtt_broker_addrinfo(void)
{
	int rc;
	struct zsock_addrinfo hints = { .ai_family = AF_INET,
					.ai_socktype = SOCK_STREAM,
					.ai_protocol = 0 };

	if (sh_mqtt->haddr != NULL) {
		zsock_freeaddrinfo(sh_mqtt->haddr);
	}

	rc = zsock_getaddrinfo(CONFIG_SHELL_MQTT_SERVER_ADDR,
			       STRINGIFY(CONFIG_SHELL_MQTT_SERVER_PORT), &hints, &sh_mqtt->haddr);
	if (rc == 0) {
		LOG_INF("DNS%s resolved for %s:%d", "", CONFIG_SHELL_MQTT_SERVER_ADDR,
			CONFIG_SHELL_MQTT_SERVER_PORT);

		return 0;
	}

	LOG_ERR("DNS%s resolved for %s:%d, retrying", " not", CONFIG_SHELL_MQTT_SERVER_ADDR,
		CONFIG_SHELL_MQTT_SERVER_PORT);

	return rc;
}

/* Close MQTT connection properly and cleanup socket */
static void sh_mqtt_close_and_cleanup(void)
{
	/* Initialize to negative value so that the mqtt_abort case can run */
	int rc = -1;

	/* If both network & mqtt connected, mqtt_disconnect will send a
	 * disconnection packet to the broker, it will invoke
	 * mqtt_evt_handler:MQTT_EVT_DISCONNECT if success
	 */
	if ((sh_mqtt->network_state == SHELL_MQTT_NETWORK_CONNECTED) &&
	    (sh_mqtt->transport_state == SHELL_MQTT_TRANSPORT_CONNECTED)) {
		rc = mqtt_disconnect(&sh_mqtt->mqtt_cli);
	}

	/* If network/mqtt disconnected, or mqtt_disconnect failed, do mqtt_abort */
	if (rc < 0) {
		/* mqtt_abort doesn't send disconnection packet to the broker, but it
		 * makes sure that the MQTT connection is aborted locally and will
		 * always invoke mqtt_evt_handler:MQTT_EVT_DISCONNECT
		 */
		(void)mqtt_abort(&sh_mqtt->mqtt_cli);
	}

	/* Cleanup socket */
	clear_fds();
}

static void broker_init(void)
{
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&sh_mqtt->broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(CONFIG_SHELL_MQTT_SERVER_PORT);

	net_ipaddr_copy(&broker4->sin_addr, &net_sin(sh_mqtt->haddr->ai_addr)->sin_addr);
}

static void client_init(void)
{
	static struct mqtt_utf8 password;
	static struct mqtt_utf8 username;

	password.utf8 = (uint8_t *)MQTT_PASSWORD;
	password.size = strlen(MQTT_PASSWORD);
	username.utf8 = (uint8_t *)MQTT_USERNAME;
	username.size = strlen(MQTT_USERNAME);

	mqtt_client_init(&sh_mqtt->mqtt_cli);

	/* MQTT client configuration */
	sh_mqtt->mqtt_cli.broker = &sh_mqtt->broker;
	sh_mqtt->mqtt_cli.evt_cb = mqtt_evt_handler;
	sh_mqtt->mqtt_cli.client_id.utf8 = (uint8_t *)sh_mqtt->device_id;
	sh_mqtt->mqtt_cli.client_id.size = strlen(sh_mqtt->device_id);
	sh_mqtt->mqtt_cli.password = &password;
	sh_mqtt->mqtt_cli.user_name = &username;
	sh_mqtt->mqtt_cli.protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	sh_mqtt->mqtt_cli.rx_buf = sh_mqtt->buf.rx;
	sh_mqtt->mqtt_cli.rx_buf_size = sizeof(sh_mqtt->buf.rx);
	sh_mqtt->mqtt_cli.tx_buf = sh_mqtt->buf.tx;
	sh_mqtt->mqtt_cli.tx_buf_size = sizeof(sh_mqtt->buf.tx);

	/* MQTT transport configuration */
	sh_mqtt->mqtt_cli.transport.type = MQTT_TRANSPORT_NON_SECURE;
}

/* Work routine to process MQTT packet and keep alive MQTT connection */
static void sh_mqtt_process_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	int rc;
	int64_t remaining = LISTEN_TIMEOUT_MS;
	int64_t start_time = k_uptime_get();

	if (sh_mqtt->network_state != SHELL_MQTT_NETWORK_CONNECTED) {
		LOG_DBG("%s_work while %s", "process", "network disconnected");
		return;
	}

	/* If context can't be locked, that means net conn cb locked it */
	if (sh_mqtt_context_lock(K_NO_WAIT) != 0) {
		/* In that case we should simply return */
		LOG_DBG("%s_work unable to lock context", "process");
		return;
	}

	if (sh_mqtt->transport_state != SHELL_MQTT_TRANSPORT_CONNECTED) {
		LOG_DBG("MQTT %s", "not connected");
		goto process_error;
	}

	if (sh_mqtt->subscribe_state != SHELL_MQTT_SUBSCRIBED) {
		LOG_DBG("%s_work while %s", "process", "MQTT not subscribed");
		goto process_error;
	}

	LOG_DBG("MQTT %s", "Processing");
	/* Listen to the port for a duration defined by LISTEN_TIMEOUT_MS */
	while ((remaining > 0) && (sh_mqtt->network_state == SHELL_MQTT_NETWORK_CONNECTED) &&
	       (sh_mqtt->transport_state == SHELL_MQTT_TRANSPORT_CONNECTED) &&
	       (sh_mqtt->subscribe_state == SHELL_MQTT_SUBSCRIBED)) {
		LOG_DBG("Listening to socket");
		rc = wait(remaining);
		if (rc > 0) {
			LOG_DBG("Process socket for MQTT packet");
			rc = mqtt_input(&sh_mqtt->mqtt_cli);
			if (rc != 0) {
				LOG_ERR("%s error: %d", "processed: mqtt_input", rc);
				goto process_error;
			}
		} else if (rc < 0) {
			goto process_error;
		}

		LOG_DBG("MQTT %s", "Keepalive");
		rc = mqtt_live(&sh_mqtt->mqtt_cli);
		if ((rc != 0) && (rc != -EAGAIN)) {
			LOG_ERR("%s error: %d", "mqtt_live", rc);
			goto process_error;
		}

		remaining = LISTEN_TIMEOUT_MS + start_time - k_uptime_get();
	}

	/* Reschedule the process work */
	LOG_DBG("Scheduling %s work", "process");
	(void)sh_mqtt_work_reschedule(&sh_mqtt->process_dwork, K_SECONDS(2));
	sh_mqtt_context_unlock();
	return;

process_error:
	LOG_DBG("%s: close MQTT, cleanup socket & reconnect", "connect");
	sh_mqtt_close_and_cleanup();
	(void)sh_mqtt_work_reschedule(&sh_mqtt->connect_dwork, K_SECONDS(1));
	sh_mqtt_context_unlock();
}

static void sh_mqtt_subscribe_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	/* Subscribe config information */
	struct mqtt_topic subs_topic = { .topic = { .utf8 = sh_mqtt->sub_topic,
						    .size = strlen(sh_mqtt->sub_topic) },
					 .qos = MQTT_QOS_1_AT_LEAST_ONCE };
	const struct mqtt_subscription_list subs_list = { .list = &subs_topic,
							  .list_count = 1U,
							  .message_id = 1U };
	int rc;

	if (sh_mqtt->network_state != SHELL_MQTT_NETWORK_CONNECTED) {
		LOG_DBG("%s_work while %s", "subscribe", "network disconnected");
		return;
	}

	/* If context can't be locked, that means net conn cb locked it */
	if (sh_mqtt_context_lock(K_NO_WAIT) != 0) {
		/* In that case we should simply return */
		LOG_DBG("%s_work unable to lock context", "subscribe");
		return;
	}

	if (sh_mqtt->transport_state != SHELL_MQTT_TRANSPORT_CONNECTED) {
		LOG_DBG("%s_work while %s", "subscribe", "transport disconnected");
		goto subscribe_error;
	}

	rc = mqtt_subscribe(&sh_mqtt->mqtt_cli, &subs_list);
	if (rc == 0) {
		/* Wait for mqtt's connack */
		LOG_DBG("Listening to socket");
		rc = wait(CONNECT_TIMEOUT_MS);
		if (rc > 0) {
			LOG_DBG("Process socket for MQTT packet");
			rc = mqtt_input(&sh_mqtt->mqtt_cli);
			if (rc != 0) {
				LOG_ERR("%s error: %d", "subscribe: mqtt_input", rc);
				goto subscribe_error;
			}
		} else if (rc < 0) {
			goto subscribe_error;
		}

		/* No suback, fail */
		if (sh_mqtt->subscribe_state != SHELL_MQTT_SUBSCRIBED) {
			goto subscribe_error;
		}

		LOG_DBG("Scheduling MQTT process work");
		(void)sh_mqtt_work_reschedule(&sh_mqtt->process_dwork, PROCESS_INTERVAL);
		sh_mqtt_context_unlock();

		LOG_INF("Logs will be published to: %s", sh_mqtt->pub_topic);
		LOG_INF("Subscribing shell cmds from: %s", sh_mqtt->sub_topic);

		return;
	}

subscribe_error:
	LOG_DBG("%s: close MQTT, cleanup socket & reconnect", "subscribe");
	sh_mqtt_close_and_cleanup();
	(void)sh_mqtt_work_reschedule(&sh_mqtt->connect_dwork, K_SECONDS(2));
	sh_mqtt_context_unlock();
}

/* Work routine to connect to MQTT */
static void sh_mqtt_connect_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	int rc;

	if (sh_mqtt->network_state != SHELL_MQTT_NETWORK_CONNECTED) {
		LOG_DBG("%s_work while %s", "connect", "network disconnected");
		return;
	}

	/* If context can't be locked, that means net conn cb locked it */
	if (sh_mqtt_context_lock(K_NO_WAIT) != 0) {
		/* In that case we should simply return */
		LOG_DBG("%s_work unable to lock context", "connect");
		return;
	}

	if (sh_mqtt->transport_state == SHELL_MQTT_TRANSPORT_CONNECTED) {
		__ASSERT(0, "MQTT shouldn't be already connected");
		LOG_ERR("MQTT shouldn't be already connected");
		goto connect_error;
	}

	/* Resolve the broker URL */
	LOG_DBG("Resolving DNS");
	rc = get_mqtt_broker_addrinfo();
	if (rc != 0) {
		(void)sh_mqtt_work_reschedule(&sh_mqtt->connect_dwork, K_SECONDS(1));
		sh_mqtt_context_unlock();
		return;
	}

	LOG_DBG("Initializing MQTT client");
	broker_init();
	client_init();

	/* Try to connect to mqtt */
	LOG_DBG("Connecting to MQTT broker");
	rc = mqtt_connect(&sh_mqtt->mqtt_cli);
	if (rc != 0) {
		LOG_ERR("%s error: %d", "mqtt_connect", rc);
		goto connect_error;
	}

	/* Prepare port config */
	LOG_DBG("Preparing socket");
	prepare_fds();

	/* Wait for mqtt's connack */
	LOG_DBG("Listening to socket");
	rc = wait(CONNECT_TIMEOUT_MS);
	if (rc > 0) {
		LOG_DBG("Process socket for MQTT packet");
		rc = mqtt_input(&sh_mqtt->mqtt_cli);
		if (rc != 0) {
			LOG_ERR("%s error: %d", "connect: mqtt_input", rc);
			goto connect_error;
		}
	} else if (rc < 0) {
		goto connect_error;
	}

	/* No connack, fail */
	if (sh_mqtt->transport_state != SHELL_MQTT_TRANSPORT_CONNECTED) {
		goto connect_error;
	}

	LOG_DBG("Scheduling %s work", "subscribe");
	(void)sh_mqtt_work_reschedule(&sh_mqtt->subscribe_dwork, K_SECONDS(2));
	sh_mqtt_context_unlock();
	return;

connect_error:
	LOG_DBG("%s: close MQTT, cleanup socket & reconnect", "connect");
	sh_mqtt_close_and_cleanup();
	(void)sh_mqtt_work_reschedule(&sh_mqtt->connect_dwork, K_SECONDS(2));
	sh_mqtt_context_unlock();
}

static int sh_mqtt_publish(uint8_t *data, uint32_t len)
{
	sh_mqtt->pub_data.message.payload.data = data;
	sh_mqtt->pub_data.message.payload.len = len;
	sh_mqtt->pub_data.message_id++;

	return mqtt_publish(&sh_mqtt->mqtt_cli, &sh_mqtt->pub_data);
}

static int sh_mqtt_publish_tx_buf(bool is_work)
{
	int rc;

	rc = sh_mqtt_publish(&sh_mqtt->tx_buf.buf[0], sh_mqtt->tx_buf.len);
	memset(&sh_mqtt->tx_buf, 0, sizeof(sh_mqtt->tx_buf));
	if (rc != 0) {
		LOG_ERR("MQTT publish error: %d", rc);
		return rc;
	}

	/* Arbitrary delay to not kill the session */
	if (!is_work) {
		k_sleep(MQTT_SEND_DELAY_MS);
	}

	return rc;
}

static void sh_mqtt_publish_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	int rc;

	(void)sh_mqtt_context_lock(K_FOREVER);

	rc = sh_mqtt_publish_tx_buf(true);
	if (rc != 0) {
		LOG_DBG("%s: close MQTT, cleanup socket & reconnect", "publish");
		sh_mqtt_close_and_cleanup();
		(void)sh_mqtt_work_reschedule(&sh_mqtt->connect_dwork, K_SECONDS(2));
	}

	sh_mqtt_context_unlock();
}

static void cancel_dworks_and_cleanup(void)
{
	(void)k_work_cancel_delayable(&sh_mqtt->connect_dwork);
	(void)k_work_cancel_delayable(&sh_mqtt->subscribe_dwork);
	(void)k_work_cancel_delayable(&sh_mqtt->process_dwork);
	(void)k_work_cancel_delayable(&sh_mqtt->publish_dwork);
	sh_mqtt_close_and_cleanup();
}

static void net_disconnect_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_WRN("Network %s", "disconnected");
	sh_mqtt->network_state = SHELL_MQTT_NETWORK_DISCONNECTED;

	/* Stop all possible work */
	(void)sh_mqtt_context_lock(K_FOREVER);
	cancel_dworks_and_cleanup();
	sh_mqtt_context_unlock();
	/* If the transport was requested, the connect work will be rescheduled
	 * when internet is connected again
	 */
}

/* Network connection event handler */
static void network_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				struct net_if *iface)
{
	if ((mgmt_event == NET_EVENT_L4_CONNECTED) &&
	    (sh_mqtt->network_state == SHELL_MQTT_NETWORK_DISCONNECTED)) {
		LOG_WRN("Network %s", "connected");
		sh_mqtt->network_state = SHELL_MQTT_NETWORK_CONNECTED;
		(void)sh_mqtt_work_reschedule(&sh_mqtt->connect_dwork, K_SECONDS(1));
	} else if ((mgmt_event == NET_EVENT_L4_DISCONNECTED) &&
		   (sh_mqtt->network_state == SHELL_MQTT_NETWORK_CONNECTED)) {
		(void)sh_mqtt_work_submit(&sh_mqtt->net_disconnected_work);
	}
}

static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			sh_mqtt->transport_state = SHELL_MQTT_TRANSPORT_DISCONNECTED;
			LOG_ERR("MQTT %s %d", "connect failed", evt->result);
			break;
		}

		sh_mqtt->transport_state = SHELL_MQTT_TRANSPORT_CONNECTED;
		LOG_WRN("MQTT %s", "client connected!");

		break;
	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT subscribe: %s", "error");
			sh_mqtt->subscribe_state = SHELL_MQTT_NOT_SUBSCRIBED;
			break;
		}

		LOG_WRN("MQTT subscribe: %s", "ok");
		sh_mqtt->subscribe_state = SHELL_MQTT_SUBSCRIBED;
		break;

	case MQTT_EVT_UNSUBACK:
		LOG_DBG("UNSUBACK packet id: %u", evt->param.suback.message_id);
		sh_mqtt->subscribe_state = SHELL_MQTT_NOT_SUBSCRIBED;
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_WRN("MQTT disconnected: %d", evt->result);
		sh_mqtt->transport_state = SHELL_MQTT_TRANSPORT_DISCONNECTED;
		sh_mqtt->subscribe_state = SHELL_MQTT_NOT_SUBSCRIBED;
		break;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *pub = &evt->param.publish;
		uint32_t size, payload_left;

		payload_left = pub->message.payload.len;

		LOG_DBG("MQTT publish received %d, %d bytes", evt->result, payload_left);
		LOG_DBG("   id: %d, qos: %d", pub->message_id, pub->message.topic.qos);
		LOG_DBG("   item: %s", pub->message.topic.topic.utf8);

		/* For MQTT_QOS_0_AT_MOST_ONCE no acknowledgment needed */
		if (pub->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			struct mqtt_puback_param puback = { .message_id = pub->message_id };

			(void)mqtt_publish_qos1_ack(client, &puback);
		}

		while (payload_left > 0) {
			/* Attempt to claim `payload_left` bytes of buffer in rb */
			size = ring_buf_put_claim(&sh_mqtt->rx_rb, &sh_mqtt->rx_rb_ptr,
						  payload_left);
			/* Read `size` bytes of payload from mqtt */
			size = mqtt_read_publish_payload_blocking(client, sh_mqtt->rx_rb_ptr, size);

			/* errno value, return */
			if (size < 0) {
				(void)ring_buf_put_finish(&sh_mqtt->rx_rb, 0U);
				sh_mqtt_rx_rb_flush();
				return;
			}

			/* Indicate that `size` bytes of payload has been written into rb */
			(void)ring_buf_put_finish(&sh_mqtt->rx_rb, size);
			/* Update `payload_left` */
			payload_left -= size;
			/* Tells the shell that we have new data for it */
			sh_mqtt->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_mqtt->shell_context);
			/* Arbitrary sleep for the shell to do its thing */
			(void)k_msleep(100);
		}

		/* Shell won't execute the cmds without \r\n */
		while (true) {
			/* Check if rb's free space is enough to fit in \r\n */
			size = ring_buf_space_get(&sh_mqtt->rx_rb);
			if (size >= sizeof("\r\n")) {
				(void)ring_buf_put(&sh_mqtt->rx_rb, "\r\n", sizeof("\r\n"));
				break;
			}
			/* Arbitrary sleep for the shell to do its thing */
			(void)k_msleep(100);
		}

		sh_mqtt->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_mqtt->shell_context);
		break;
	}

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			break;
		}

		LOG_DBG("PUBACK packet id: %u", evt->param.puback.message_id);
		break;

	case MQTT_EVT_PINGRESP:
		LOG_DBG("PINGRESP packet");
		break;

	default:
		LOG_DBG("MQTT event received %d", evt->type);
		break;
	}
}

static int init(const struct shell_transport *transport, const void *config,
		shell_transport_handler_t evt_handler, void *context)
{
	sh_mqtt = (struct shell_mqtt *)transport->ctx;

	(void)memset(sh_mqtt, 0, sizeof(struct shell_mqtt));

	(void)k_mutex_init(&sh_mqtt->lock);

	if (!shell_mqtt_get_devid(sh_mqtt->device_id, DEVICE_ID_HEX_MAX_SIZE)) {
		LOG_ERR("Unable to get device identity, using dummy value");
		(void)snprintf(sh_mqtt->device_id, sizeof("dummy"), "dummy");
	}

	LOG_DBG("Client ID is %s", sh_mqtt->device_id);

	(void)snprintf(sh_mqtt->pub_topic, SH_MQTT_TOPIC_MAX_SIZE, "%s_tx", sh_mqtt->device_id);
	(void)snprintf(sh_mqtt->sub_topic, SH_MQTT_TOPIC_MAX_SIZE, "%s_rx", sh_mqtt->device_id);

	ring_buf_init(&sh_mqtt->rx_rb, RX_RB_SIZE, sh_mqtt->rx_rb_buf);

	LOG_DBG("Initializing shell MQTT backend");

	sh_mqtt->shell_handler = evt_handler;
	sh_mqtt->shell_context = context;

	sh_mqtt->pub_data.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
	sh_mqtt->pub_data.message.topic.topic.utf8 = (uint8_t *)sh_mqtt->pub_topic;
	sh_mqtt->pub_data.message.topic.topic.size =
		strlen(sh_mqtt->pub_data.message.topic.topic.utf8);
	sh_mqtt->pub_data.dup_flag = 0U;
	sh_mqtt->pub_data.retain_flag = 0U;

	/* Initialize the work queue */
	k_work_queue_init(&sh_mqtt->workq);
	k_work_queue_start(&sh_mqtt->workq, sh_mqtt_workq_stack,
			   K_KERNEL_STACK_SIZEOF(sh_mqtt_workq_stack), K_PRIO_COOP(7), NULL);
	(void)k_thread_name_set(&sh_mqtt->workq.thread, "sh_mqtt_workq");
	k_work_init(&sh_mqtt->net_disconnected_work, net_disconnect_handler);
	k_work_init_delayable(&sh_mqtt->connect_dwork, sh_mqtt_connect_handler);
	k_work_init_delayable(&sh_mqtt->subscribe_dwork, sh_mqtt_subscribe_handler);
	k_work_init_delayable(&sh_mqtt->process_dwork, sh_mqtt_process_handler);
	k_work_init_delayable(&sh_mqtt->publish_dwork, sh_mqtt_publish_handler);

	LOG_DBG("Initializing listener for network");
	net_mgmt_init_event_callback(&sh_mqtt->mgmt_cb, network_evt_handler, NET_EVENT_MASK);

	sh_mqtt->network_state = SHELL_MQTT_NETWORK_DISCONNECTED;
	sh_mqtt->transport_state = SHELL_MQTT_TRANSPORT_DISCONNECTED;
	sh_mqtt->subscribe_state = SHELL_MQTT_NOT_SUBSCRIBED;

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	ARG_UNUSED(transport);

	/* Not initialized yet */
	if (sh_mqtt == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(blocking);

	/* Not initialized yet */
	if (sh_mqtt == NULL) {
		return -ENODEV;
	}

	/* Listen for network connection status */
	net_mgmt_add_event_callback(&sh_mqtt->mgmt_cb);
	net_conn_mgr_resend_status();

	return 0;
}

static int write(const struct shell_transport *transport, const void *data, size_t length,
		 size_t *cnt)
{
	ARG_UNUSED(transport);
	int rc = 0;
	struct k_work_sync ws;
	size_t copy_len;

	*cnt = 0;

	/* Not initialized yet */
	if (sh_mqtt == NULL) {
		return -ENODEV;
	}

	/* Not connected to broker */
	if (sh_mqtt->transport_state != SHELL_MQTT_TRANSPORT_CONNECTED) {
		goto out;
	}

	(void)k_work_cancel_delayable_sync(&sh_mqtt->publish_dwork, &ws);

	do {
		if ((sh_mqtt->tx_buf.len + length - *cnt) > TX_BUF_SIZE) {
			copy_len = TX_BUF_SIZE - sh_mqtt->tx_buf.len;
		} else {
			copy_len = length - *cnt;
		}

		memcpy(sh_mqtt->tx_buf.buf + sh_mqtt->tx_buf.len, (uint8_t *)data + *cnt, copy_len);
		sh_mqtt->tx_buf.len += copy_len;

		/* Send the data immediately if the buffer is full */
		if (sh_mqtt->tx_buf.len == TX_BUF_SIZE) {
			rc = sh_mqtt_publish_tx_buf(false);
			if (rc != 0) {
				sh_mqtt_close_and_cleanup();
				(void)sh_mqtt_work_reschedule(&sh_mqtt->connect_dwork,
							      K_SECONDS(2));
				*cnt = length;
				return rc;
			}
		}

		*cnt += copy_len;
	} while (*cnt < length);

	if (sh_mqtt->tx_buf.len > 0) {
		(void)sh_mqtt_work_reschedule(&sh_mqtt->publish_dwork, MQTT_SEND_DELAY_MS);
	}

	/* Inform shell that it is ready for next TX */
	sh_mqtt->shell_handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_mqtt->shell_context);

out:
	/* We will always assume that we sent everything */
	*cnt = length;
	return rc;
}

static int read(const struct shell_transport *transport, void *data, size_t length, size_t *cnt)
{
	ARG_UNUSED(transport);

	/* Not initialized yet */
	if (sh_mqtt == NULL) {
		return -ENODEV;
	}

	/* Not subscribed yet */
	if (sh_mqtt->subscribe_state != SHELL_MQTT_SUBSCRIBED) {
		*cnt = 0;
		return 0;
	}

	*cnt = ring_buf_get(&sh_mqtt->rx_rb, data, length);

	/* Inform the shell if there are still data in the rb */
	if (ring_buf_size_get(&sh_mqtt->rx_rb) > 0) {
		sh_mqtt->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_mqtt->shell_context);
	}

	return 0;
}

const struct shell_transport_api shell_mqtt_transport_api = { .init = init,
							      .uninit = uninit,
							      .enable = enable,
							      .write = write,
							      .read = read };

static int enable_shell_mqtt(void)
{

	bool log_backend = CONFIG_SHELL_MQTT_INIT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_MQTT_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
				       CONFIG_LOG_MAX_LEVEL :
				       CONFIG_SHELL_MQTT_INIT_LOG_LEVEL;
	static const struct shell_backend_config_flags cfg_flags = {
		.insert_mode = 0,
		.echo = 0,
		.obscure = 0,
		.mode_delete = 0,
		.use_colors = 0,
		.use_vt100 = 0,
	};

	return shell_init(&shell_mqtt, NULL, cfg_flags, log_backend, level);
}

/* Function is used for testing purposes */
const struct shell *shell_backend_mqtt_get_ptr(void)
{
	return &shell_mqtt;
}

SYS_INIT(enable_shell_mqtt, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
