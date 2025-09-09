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
#define CONNECT_TIMEOUT_MS CONFIG_SHELL_MQTT_CONNECT_TIMEOUT_MS
#define LISTEN_TIMEOUT_MS CONFIG_SHELL_MQTT_LISTEN_TIMEOUT_MS
#define MQTT_SEND_DELAY_MS K_MSEC(100)
#define PROCESS_INTERVAL K_MSEC(CONFIG_SHELL_MQTT_WORK_DELAY_MS)
#define SHELL_MQTT_WORKQ_STACK_SIZE 2048

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

static void prepare_fds(struct shell_mqtt *sh)
{
	if (sh->mqtt_cli.transport.type == MQTT_TRANSPORT_NON_SECURE) {
		sh->fds[0].fd = sh->mqtt_cli.transport.tcp.sock;
	}

	sh->fds[0].events = ZSOCK_POLLIN;
	sh->nfds = 1;
}

static void clear_fds(struct shell_mqtt *sh)
{
	sh->nfds = 0;
}

/*
 * Upon successful completion, poll() shall return a non-negative value. A positive value indicates
 * the total number of pollfd structures that have selected events (that is, those for which the
 * revents member is non-zero). A value of 0 indicates that the call timed out and no file
 * descriptors have been selected. Upon failure, poll() shall return -1 and set errno to indicate
 * the error.
 */
static int wait(struct shell_mqtt *sh, int timeout)
{
	int rc = 0;

	if (sh->nfds > 0) {
		rc = zsock_poll(sh->fds, sh->nfds, timeout);
		if (rc < 0) {
			LOG_ERR("poll error: %d", errno);
		}
	}

	return rc;
}

/* Query IP address for the broker URL */
static int get_mqtt_broker_addrinfo(struct shell_mqtt *sh)
{
	int rc;
	struct zsock_addrinfo hints = { .ai_family = AF_INET,
					.ai_socktype = SOCK_STREAM,
					.ai_protocol = 0 };

	if (sh->haddr != NULL) {
		zsock_freeaddrinfo(sh->haddr);
	}

	rc = zsock_getaddrinfo(CONFIG_SHELL_MQTT_SERVER_ADDR,
			       STRINGIFY(CONFIG_SHELL_MQTT_SERVER_PORT), &hints, &sh->haddr);
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
static void sh_mqtt_close_and_cleanup(struct shell_mqtt *sh)
{
	/* Initialize to negative value so that the mqtt_abort case can run */
	int rc = -1;

	/* If both network & mqtt connected, mqtt_disconnect will send a
	 * disconnection packet to the broker, it will invoke
	 * mqtt_evt_handler:MQTT_EVT_DISCONNECT if success
	 */
	if ((sh->network_state == SHELL_MQTT_NETWORK_CONNECTED) &&
	    (sh->transport_state == SHELL_MQTT_TRANSPORT_CONNECTED)) {
		rc = mqtt_disconnect(&sh->mqtt_cli, NULL);
	}

	/* If network/mqtt disconnected, or mqtt_disconnect failed, do mqtt_abort */
	if (rc < 0) {
		/* mqtt_abort doesn't send disconnection packet to the broker, but it
		 * makes sure that the MQTT connection is aborted locally and will
		 * always invoke mqtt_evt_handler:MQTT_EVT_DISCONNECT
		 */
		(void)mqtt_abort(&sh->mqtt_cli);
	}

	/* Cleanup socket */
	clear_fds(sh);
}

static void broker_init(struct shell_mqtt *sh)
{
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&sh->broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(CONFIG_SHELL_MQTT_SERVER_PORT);

	net_ipaddr_copy(&broker4->sin_addr, &net_sin(sh->haddr->ai_addr)->sin_addr);
}

static void client_init(struct shell_mqtt *sh)
{
	static struct mqtt_utf8 password;
	static struct mqtt_utf8 username;

	password.utf8 = (uint8_t *)CONFIG_SHELL_MQTT_SERVER_PASSWORD;
	password.size = strlen(CONFIG_SHELL_MQTT_SERVER_PASSWORD);
	username.utf8 = (uint8_t *)CONFIG_SHELL_MQTT_SERVER_USERNAME;
	username.size = strlen(CONFIG_SHELL_MQTT_SERVER_USERNAME);

	mqtt_client_init(&sh->mqtt_cli);

	/* MQTT client configuration */
	sh->mqtt_cli.broker = &sh->broker;
	sh->mqtt_cli.evt_cb = mqtt_evt_handler;
	sh->mqtt_cli.client_id.utf8 = (uint8_t *)sh->device_id;
	sh->mqtt_cli.client_id.size = strlen(sh->device_id);
	sh->mqtt_cli.password = &password;
	sh->mqtt_cli.user_name = &username;
	sh->mqtt_cli.protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	sh->mqtt_cli.rx_buf = sh->buf.rx;
	sh->mqtt_cli.rx_buf_size = sizeof(sh->buf.rx);
	sh->mqtt_cli.tx_buf = sh->buf.tx;
	sh->mqtt_cli.tx_buf_size = sizeof(sh->buf.tx);

	/* MQTT transport configuration */
	sh->mqtt_cli.transport.type = MQTT_TRANSPORT_NON_SECURE;
}

/* Work routine to process MQTT packet and keep alive MQTT connection */
static void sh_mqtt_process_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	struct shell_mqtt *sh = sh_mqtt;
	int rc;
	int64_t remaining = LISTEN_TIMEOUT_MS;
	int64_t start_time = k_uptime_get();

	if (sh->network_state != SHELL_MQTT_NETWORK_CONNECTED) {
		LOG_DBG("%s_work while %s", "process", "network disconnected");
		return;
	}

	/* If context can't be locked, that means net conn cb locked it */
	if (sh_mqtt_context_lock(K_NO_WAIT) != 0) {
		/* In that case we should simply return */
		LOG_DBG("%s_work unable to lock context", "process");
		return;
	}

	if (sh->transport_state != SHELL_MQTT_TRANSPORT_CONNECTED) {
		LOG_DBG("MQTT %s", "not connected");
		goto process_error;
	}

	if (sh->subscribe_state != SHELL_MQTT_SUBSCRIBED) {
		LOG_DBG("%s_work while %s", "process", "MQTT not subscribed");
		goto process_error;
	}

	LOG_DBG("MQTT %s", "Processing");
	/* Listen to the port for a duration defined by LISTEN_TIMEOUT_MS */
	while ((remaining > 0) && (sh->network_state == SHELL_MQTT_NETWORK_CONNECTED) &&
	       (sh->transport_state == SHELL_MQTT_TRANSPORT_CONNECTED) &&
	       (sh->subscribe_state == SHELL_MQTT_SUBSCRIBED)) {
		LOG_DBG("Listening to socket");
		rc = wait(sh, remaining);
		if (rc > 0) {
			LOG_DBG("Process socket for MQTT packet");
			rc = mqtt_input(&sh->mqtt_cli);
			if (rc != 0) {
				LOG_ERR("%s error: %d", "processed: mqtt_input", rc);
				goto process_error;
			}
		} else if (rc < 0) {
			goto process_error;
		}

		LOG_DBG("MQTT %s", "Keepalive");
		rc = mqtt_live(&sh->mqtt_cli);
		if ((rc != 0) && (rc != -EAGAIN)) {
			LOG_ERR("%s error: %d", "mqtt_live", rc);
			goto process_error;
		}

		remaining = LISTEN_TIMEOUT_MS + start_time - k_uptime_get();
	}

	/* Reschedule the process work */
	LOG_DBG("Scheduling %s work", "process");
	(void)sh_mqtt_work_reschedule(&sh->process_dwork, PROCESS_INTERVAL);
	sh_mqtt_context_unlock();
	return;

process_error:
	LOG_DBG("%s: close MQTT, cleanup socket & reconnect", "connect");
	sh_mqtt_close_and_cleanup(sh);
	(void)sh_mqtt_work_reschedule(&sh->connect_dwork, PROCESS_INTERVAL);
	sh_mqtt_context_unlock();
}

static void sh_mqtt_subscribe_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	struct shell_mqtt *sh = sh_mqtt;

	/* Subscribe config information */
	struct mqtt_topic subs_topic = { .topic = { .utf8 = sh->sub_topic,
						    .size = strlen(sh->sub_topic) },
					 .qos = MQTT_QOS_1_AT_LEAST_ONCE };
	const struct mqtt_subscription_list subs_list = { .list = &subs_topic,
							  .list_count = 1U,
							  .message_id = 1U };
	int rc;

	if (sh->network_state != SHELL_MQTT_NETWORK_CONNECTED) {
		LOG_DBG("%s_work while %s", "subscribe", "network disconnected");
		return;
	}

	/* If context can't be locked, that means net conn cb locked it */
	if (sh_mqtt_context_lock(K_NO_WAIT) != 0) {
		/* In that case we should simply return */
		LOG_DBG("%s_work unable to lock context", "subscribe");
		return;
	}

	if (sh->transport_state != SHELL_MQTT_TRANSPORT_CONNECTED) {
		LOG_DBG("%s_work while %s", "subscribe", "transport disconnected");
		goto subscribe_error;
	}

	rc = mqtt_subscribe(&sh->mqtt_cli, &subs_list);
	if (rc == 0) {
		/* Wait for mqtt's connack */
		LOG_DBG("Listening to socket");
		rc = wait(sh, CONNECT_TIMEOUT_MS);
		if (rc > 0) {
			LOG_DBG("Process socket for MQTT packet");
			rc = mqtt_input(&sh->mqtt_cli);
			if (rc != 0) {
				LOG_ERR("%s error: %d", "subscribe: mqtt_input", rc);
				goto subscribe_error;
			}
		} else if (rc < 0) {
			goto subscribe_error;
		}

		/* No suback, fail */
		if (sh->subscribe_state != SHELL_MQTT_SUBSCRIBED) {
			goto subscribe_error;
		}

		LOG_DBG("Scheduling MQTT process work");
		(void)sh_mqtt_work_reschedule(&sh->process_dwork, PROCESS_INTERVAL);
		sh_mqtt_context_unlock();

		LOG_INF("Logs will be published to: %s", sh->pub_topic);
		LOG_INF("Subscribing shell cmds from: %s", sh->sub_topic);

		return;
	}

subscribe_error:
	LOG_DBG("%s: close MQTT, cleanup socket & reconnect", "subscribe");
	sh_mqtt_close_and_cleanup(sh);
	(void)sh_mqtt_work_reschedule(&sh->connect_dwork, PROCESS_INTERVAL);
	sh_mqtt_context_unlock();
}

/* Work routine to connect to MQTT */
static void sh_mqtt_connect_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	struct shell_mqtt *sh = sh_mqtt;
	int rc;

	if (sh->network_state != SHELL_MQTT_NETWORK_CONNECTED) {
		LOG_DBG("%s_work while %s", "connect", "network disconnected");
		return;
	}

	/* If context can't be locked, that means net conn cb locked it */
	if (sh_mqtt_context_lock(K_NO_WAIT) != 0) {
		/* In that case we should simply return */
		LOG_DBG("%s_work unable to lock context", "connect");
		return;
	}

	if (sh->transport_state == SHELL_MQTT_TRANSPORT_CONNECTED) {
		__ASSERT(0, "MQTT shouldn't be already connected");
		LOG_ERR("MQTT shouldn't be already connected");
		goto connect_error;
	}

	/* Resolve the broker URL */
	LOG_DBG("Resolving DNS");
	rc = get_mqtt_broker_addrinfo(sh);
	if (rc != 0) {
		(void)sh_mqtt_work_reschedule(&sh->connect_dwork, PROCESS_INTERVAL);
		sh_mqtt_context_unlock();
		return;
	}

	LOG_DBG("Initializing MQTT client");
	broker_init(sh);
	client_init(sh);

	/* Try to connect to mqtt */
	LOG_DBG("Connecting to MQTT broker");
	rc = mqtt_connect(&sh->mqtt_cli);
	if (rc != 0) {
		LOG_ERR("%s error: %d", "mqtt_connect", rc);
		goto connect_error;
	}

	/* Prepare port config */
	LOG_DBG("Preparing socket");
	prepare_fds(sh);

	/* Wait for mqtt's connack */
	LOG_DBG("Listening to socket");
	rc = wait(sh, CONNECT_TIMEOUT_MS);
	if (rc > 0) {
		LOG_DBG("Process socket for MQTT packet");
		rc = mqtt_input(&sh->mqtt_cli);
		if (rc != 0) {
			LOG_ERR("%s error: %d", "connect: mqtt_input", rc);
			goto connect_error;
		}
	} else if (rc < 0) {
		goto connect_error;
	}

	/* No connack, fail */
	if (sh->transport_state != SHELL_MQTT_TRANSPORT_CONNECTED) {
		goto connect_error;
	}

	LOG_DBG("Scheduling %s work", "subscribe");
	(void)sh_mqtt_work_reschedule(&sh->subscribe_dwork, PROCESS_INTERVAL);
	sh_mqtt_context_unlock();
	return;

connect_error:
	LOG_DBG("%s: close MQTT, cleanup socket & reconnect", "connect");
	sh_mqtt_close_and_cleanup(sh);
	(void)sh_mqtt_work_reschedule(&sh->connect_dwork, PROCESS_INTERVAL);
	sh_mqtt_context_unlock();
}

static int sh_mqtt_publish(struct shell_mqtt *sh, uint8_t *data, uint32_t len)
{
	sh->pub_data.message.payload.data = data;
	sh->pub_data.message.payload.len = len;
	sh->pub_data.message_id++;

	return mqtt_publish(&sh->mqtt_cli, &sh->pub_data);
}

static int sh_mqtt_publish_tx_buf(struct shell_mqtt *sh, bool is_work)
{
	int rc;

	rc = sh_mqtt_publish(sh, &sh->tx_buf.buf[0], sh->tx_buf.len);
	memset(&sh->tx_buf, 0, sizeof(sh->tx_buf));
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
	struct shell_mqtt *sh = sh_mqtt;
	int rc;

	(void)sh_mqtt_context_lock(K_FOREVER);

	rc = sh_mqtt_publish_tx_buf(sh, true);
	if (rc != 0) {
		LOG_DBG("%s: close MQTT, cleanup socket & reconnect", "publish");
		sh_mqtt_close_and_cleanup(sh);
		(void)sh_mqtt_work_reschedule(&sh->connect_dwork, PROCESS_INTERVAL);
	}

	sh_mqtt_context_unlock();
}

static void cancel_dworks_and_cleanup(struct shell_mqtt *sh)
{
	(void)k_work_cancel_delayable(&sh->connect_dwork);
	(void)k_work_cancel_delayable(&sh->subscribe_dwork);
	(void)k_work_cancel_delayable(&sh->process_dwork);
	(void)k_work_cancel_delayable(&sh->publish_dwork);
	sh_mqtt_close_and_cleanup(sh);
}

static void net_disconnect_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	struct shell_mqtt *sh = sh_mqtt;

	LOG_WRN("Network %s", "disconnected");
	sh->network_state = SHELL_MQTT_NETWORK_DISCONNECTED;

	/* Stop all possible work */
	(void)sh_mqtt_context_lock(K_FOREVER);
	cancel_dworks_and_cleanup(sh);
	sh_mqtt_context_unlock();
	/* If the transport was requested, the connect work will be rescheduled
	 * when internet is connected again
	 */
}

/* Network connection event handler */
static void network_evt_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				struct net_if *iface)
{
	struct shell_mqtt *sh = sh_mqtt;

	if ((mgmt_event == NET_EVENT_L4_CONNECTED) &&
	    (sh->network_state == SHELL_MQTT_NETWORK_DISCONNECTED)) {
		LOG_WRN("Network %s", "connected");
		sh->network_state = SHELL_MQTT_NETWORK_CONNECTED;
		(void)sh_mqtt_work_reschedule(&sh->connect_dwork, PROCESS_INTERVAL);
	} else if ((mgmt_event == NET_EVENT_L4_DISCONNECTED) &&
		   (sh->network_state == SHELL_MQTT_NETWORK_CONNECTED)) {
		(void)sh_mqtt_work_submit(&sh->net_disconnected_work);
	}
}

static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
	struct shell_mqtt *sh = sh_mqtt;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			sh->transport_state = SHELL_MQTT_TRANSPORT_DISCONNECTED;
			LOG_ERR("MQTT %s %d", "connect failed", evt->result);
			break;
		}

		sh->transport_state = SHELL_MQTT_TRANSPORT_CONNECTED;
		LOG_WRN("MQTT %s", "client connected!");
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT subscribe: %s", "error");
			sh->subscribe_state = SHELL_MQTT_NOT_SUBSCRIBED;
			break;
		}

		LOG_WRN("MQTT subscribe: %s", "ok");
		sh->subscribe_state = SHELL_MQTT_SUBSCRIBED;
		break;

	case MQTT_EVT_UNSUBACK:
		LOG_DBG("UNSUBACK packet id: %u", evt->param.suback.message_id);
		sh->subscribe_state = SHELL_MQTT_NOT_SUBSCRIBED;
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_WRN("MQTT disconnected: %d", evt->result);
		sh->transport_state = SHELL_MQTT_TRANSPORT_DISCONNECTED;
		sh->subscribe_state = SHELL_MQTT_NOT_SUBSCRIBED;
		break;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *pub = &evt->param.publish;
		uint32_t payload_left;
		size_t size;
		int rc;

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
			size = (size_t)ring_buf_put_claim(&sh->rx_rb, &sh->rx_rb_ptr,
							  payload_left);
			/* Read `size` bytes of payload from mqtt */
			rc = mqtt_read_publish_payload_blocking(client, sh->rx_rb_ptr, size);

			/* errno value, return */
			if (rc < 0) {
				ring_buf_reset(&sh->rx_rb);
				return;
			}

			size = (size_t)rc;
			/* Indicate that `size` bytes of payload has been written into rb */
			(void)ring_buf_put_finish(&sh->rx_rb, size);
			/* Update `payload_left` */
			payload_left -= size;
			/* Tells the shell that we have new data for it */
			sh->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, sh->shell_context);
			/* Arbitrary sleep for the shell to do its thing */
			(void)k_msleep(100);
		}

		/* Shell won't execute the cmds without \r\n */
		while (true) {
			/* Check if rb's free space is enough to fit in \r\n */
			size = ring_buf_space_get(&sh->rx_rb);
			if (size >= sizeof("\r\n")) {
				(void)ring_buf_put(&sh->rx_rb, "\r\n", sizeof("\r\n"));
				break;
			}
			/* Arbitrary sleep for the shell to do its thing */
			(void)k_msleep(100);
		}

		sh->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, sh->shell_context);
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
	struct shell_mqtt *sh = sh_mqtt;

	(void)memset(sh, 0, sizeof(struct shell_mqtt));

	(void)k_mutex_init(&sh->lock);

	if (!shell_mqtt_get_devid(sh->device_id, DEVICE_ID_HEX_MAX_SIZE)) {
		LOG_ERR("Unable to get device identity, using dummy value");
		(void)snprintf(sh->device_id, sizeof("dummy"), "dummy");
	}

	LOG_DBG("Client ID is %s", sh->device_id);

	(void)snprintf(sh->pub_topic, SH_MQTT_TOPIC_TX_MAX_SIZE, "%s" CONFIG_SHELL_MQTT_TOPIC_TX_ID,
		       sh->device_id);
	(void)snprintf(sh->sub_topic, SH_MQTT_TOPIC_RX_MAX_SIZE, "%s" CONFIG_SHELL_MQTT_TOPIC_RX_ID,
		       sh->device_id);

	ring_buf_init(&sh->rx_rb, RX_RB_SIZE, sh->rx_rb_buf);

	LOG_DBG("Initializing shell MQTT backend");

	sh->shell_handler = evt_handler;
	sh->shell_context = context;

	sh->pub_data.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
	sh->pub_data.message.topic.topic.utf8 = (uint8_t *)sh->pub_topic;
	sh->pub_data.message.topic.topic.size =
		strlen(sh->pub_data.message.topic.topic.utf8);
	sh->pub_data.dup_flag = 0U;
	sh->pub_data.retain_flag = 0U;

	/* Initialize the work queue */
	k_work_queue_init(&sh->workq);
	k_work_queue_start(&sh->workq, sh_mqtt_workq_stack,
			   K_KERNEL_STACK_SIZEOF(sh_mqtt_workq_stack), K_PRIO_COOP(7), NULL);
	(void)k_thread_name_set(&sh->workq.thread, "sh_mqtt_workq");
	k_work_init(&sh->net_disconnected_work, net_disconnect_handler);
	k_work_init_delayable(&sh->connect_dwork, sh_mqtt_connect_handler);
	k_work_init_delayable(&sh->subscribe_dwork, sh_mqtt_subscribe_handler);
	k_work_init_delayable(&sh->process_dwork, sh_mqtt_process_handler);
	k_work_init_delayable(&sh->publish_dwork, sh_mqtt_publish_handler);

	LOG_DBG("Initializing listener for network");
	net_mgmt_init_event_callback(&sh->mgmt_cb, network_evt_handler, NET_EVENT_MASK);

	sh->network_state = SHELL_MQTT_NETWORK_DISCONNECTED;
	sh->transport_state = SHELL_MQTT_TRANSPORT_DISCONNECTED;
	sh->subscribe_state = SHELL_MQTT_NOT_SUBSCRIBED;

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	ARG_UNUSED(transport);
	struct shell_mqtt *sh = sh_mqtt;

	/* Not initialized yet */
	if (sh == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(blocking);
	struct shell_mqtt *sh = sh_mqtt;

	/* Not initialized yet */
	if (sh == NULL) {
		return -ENODEV;
	}

	/* Listen for network connection status */
	net_mgmt_add_event_callback(&sh->mgmt_cb);
	conn_mgr_mon_resend_status();

	return 0;
}

static int write_data(const struct shell_transport *transport, const void *data, size_t length,
		      size_t *cnt)
{
	ARG_UNUSED(transport);
	struct shell_mqtt *sh = sh_mqtt;
	int rc = 0;
	struct k_work_sync ws;
	size_t copy_len;

	*cnt = 0;

	/* Not initialized yet */
	if (sh == NULL) {
		return -ENODEV;
	}

	/* Not connected to broker */
	if (sh->transport_state != SHELL_MQTT_TRANSPORT_CONNECTED) {
		goto out;
	}

	(void)k_work_cancel_delayable_sync(&sh->publish_dwork, &ws);

	do {
		if ((sh->tx_buf.len + length - *cnt) > TX_BUF_SIZE) {
			copy_len = TX_BUF_SIZE - sh->tx_buf.len;
		} else {
			copy_len = length - *cnt;
		}

		memcpy(sh->tx_buf.buf + sh->tx_buf.len, (uint8_t *)data + *cnt, copy_len);
		sh->tx_buf.len += copy_len;

		/* Send the data immediately if the buffer is full */
		if (sh->tx_buf.len == TX_BUF_SIZE) {
			rc = sh_mqtt_publish_tx_buf(sh, false);
			if (rc != 0) {
				sh_mqtt_close_and_cleanup(sh);
				(void)sh_mqtt_work_reschedule(&sh->connect_dwork, PROCESS_INTERVAL);
				*cnt = length;
				return rc;
			}
		}

		*cnt += copy_len;
	} while (*cnt < length);

	if (sh->tx_buf.len > 0) {
		(void)sh_mqtt_work_reschedule(&sh->publish_dwork, MQTT_SEND_DELAY_MS);
	}

	/* Inform shell that it is ready for next TX */
	sh->shell_handler(SHELL_TRANSPORT_EVT_TX_RDY, sh->shell_context);

out:
	/* We will always assume that we sent everything */
	*cnt = length;
	return rc;
}

static int read_data(const struct shell_transport *transport, void *data, size_t length,
		     size_t *cnt)
{
	ARG_UNUSED(transport);
	struct shell_mqtt *sh = sh_mqtt;

	/* Not initialized yet */
	if (sh == NULL) {
		return -ENODEV;
	}

	/* Not subscribed yet */
	if (sh->subscribe_state != SHELL_MQTT_SUBSCRIBED) {
		*cnt = 0;
		return 0;
	}

	*cnt = ring_buf_get(&sh->rx_rb, data, length);

	/* Inform the shell if there are still data in the rb */
	if (ring_buf_size_get(&sh->rx_rb) > 0) {
		sh->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, sh->shell_context);
	}

	return 0;
}

const struct shell_transport_api shell_mqtt_transport_api = { .init = init,
							      .uninit = uninit,
							      .enable = enable,
							      .write = write_data,
							      .read = read_data };

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
