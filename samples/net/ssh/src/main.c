/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ssh_sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/ssh/common.h>
#include <zephyr/net/ssh/server.h>
#include <zephyr/net/ssh/client.h>
#include <zephyr/net/net_config.h>

#if defined(CONFIG_NET_SAMPLE_SSH_USE_PASSWORD)
#define SSH_PASSWORD CONFIG_NET_SAMPLE_SSH_PASSWORD
#else
#define SSH_PASSWORD NULL
#endif

#define KEY_SIZE_BITS 2048

NET_CONFIG_SSH_KEY_BUF_DEFINE_STATIC(my_key_buf, KEY_SIZE_BITS);

/* We store the our private key to slot 0 and public key to slot 1 */

NET_CONFIG_SSH_PRIV_KEY_DEFINE_STATIC(my_priv_key_config,
				      my_key_buf, sizeof(my_key_buf),
				      "id_rsa", 0,
				      SSH_HOST_KEY_TYPE_RSA, KEY_SIZE_BITS);

NET_CONFIG_SSH_PUB_KEY_DEFINE_STATIC(my_pub_key_config,
				     my_key_buf, sizeof(my_key_buf),
				     "authorized_key_0", 1);

NET_CONFIG_SSH_PASSWORD_AUTH_DEFINE_STATIC(my_password_auth,
					   CONFIG_NET_SAMPLE_SSH_USERNAME,
					   SSH_PASSWORD);

static bool connected;
static struct net_mgmt_event_callback mgmt_cb;

/* Protects the transport and channel bookkeeping below. It is updated from the
 * SSH server/client threads (the event callbacks) and read from the shell
 * thread (the sample commands).
 */
static K_MUTEX_DEFINE(sample_lock);

#if defined(CONFIG_SSH_SERVER) || defined(CONFIG_SSH_CLIENT)
/* Store a transport into the first free slot of an array. Returns the slot
 * index, or -1 if the array is full. If the transport is already stored its
 * existing index is returned.
 */
static int transport_track(struct ssh_transport **arr, size_t count,
			   struct ssh_transport *transport)
{
	int free_slot = -1;

	for (size_t i = 0; i < count; i++) {
		if (arr[i] == transport) {
			return (int)i;
		}

		if (arr[i] == NULL && free_slot < 0) {
			free_slot = (int)i;
		}
	}

	if (free_slot >= 0) {
		arr[free_slot] = transport;
	}

	return free_slot;
}

static void transport_untrack(struct ssh_transport **arr, size_t count,
			      struct ssh_transport *transport)
{
	for (size_t i = 0; i < count; i++) {
		if (arr[i] == transport) {
			arr[i] = NULL;
		}
	}
}
#endif /* CONFIG_SSH_SERVER || CONFIG_SSH_CLIENT */

#if defined(CONFIG_SSH_SERVER)
/* Transports of the clients connected to our server. */
static struct ssh_transport *server_transport[CONFIG_SSH_SERVER_MAX_CLIENTS];

static int server_transport_event_handler(struct ssh_transport *transport,
					  const struct ssh_transport_event *event,
					  void *user_data);

/* The server transport configuration. This must not be allocated from the
 * stack as it is used by the SSH server until it is unregistered.
 */
static struct ssh_transport_conf server_transport_conf = {
	.cb = server_transport_event_handler,
};

static int server_transport_event_handler(struct ssh_transport *transport,
					  const struct ssh_transport_event *event,
					  void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event->type) {
	case SSH_TRANSPORT_EVENT_CHANNEL_OPEN:
		k_mutex_lock(&sample_lock, K_FOREVER);
		if (transport_track(server_transport, ARRAY_SIZE(server_transport),
				    transport) < 0) {
			LOG_WRN("No free server transport slots");
		}
		k_mutex_unlock(&sample_lock);
		break;

	case SSH_TRANSPORT_EVENT_CLOSED:
		k_mutex_lock(&sample_lock, K_FOREVER);
		transport_untrack(server_transport, ARRAY_SIZE(server_transport),
				  transport);
		k_mutex_unlock(&sample_lock);
		break;

	default:
		break;
	}

	return 0;
}
#endif /* CONFIG_SSH_SERVER */

#if defined(CONFIG_SSH_CLIENT)
/* Transports of our client connections. The connection itself is managed by
 * the SSH shell ("net ssh start"/"net ssh stop"); here we just record the
 * authenticated transports so that a channel can be opened on them.
 */
static struct ssh_transport *client_transport[CONFIG_SSH_CLIENT_MAX_CLIENTS];

/* Channels we have opened on our client connections. */
static struct ssh_channel *client_channels[CONFIG_SSH_MAX_CHANNELS];

static int client_transport_event_handler(struct ssh_transport *transport,
					  const struct ssh_transport_event *event,
					  void *user_data);

/* The client transport configuration. This must not be allocated from the
 * stack as it is used by the SSH client until it is unregistered.
 */
static struct ssh_transport_conf client_transport_conf = {
	.cb = client_transport_event_handler,
};

static int client_transport_event_handler(struct ssh_transport *transport,
					  const struct ssh_transport_event *event,
					  void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event->type) {
	case SSH_TRANSPORT_EVENT_AUTHENTICATE_RESULT:
		if (!event->authenticate_result.success) {
			break;
		}

		/* The connection is authenticated, so remember the transport
		 * to be able to open a channel on it later.
		 */
		k_mutex_lock(&sample_lock, K_FOREVER);
		if (transport_track(client_transport, ARRAY_SIZE(client_transport),
				    transport) < 0) {
			LOG_WRN("No free client transport slots");
		}
		k_mutex_unlock(&sample_lock);
		break;

	case SSH_TRANSPORT_EVENT_CLOSED:
		k_mutex_lock(&sample_lock, K_FOREVER);
		transport_untrack(client_transport, ARRAY_SIZE(client_transport),
				  transport);
		k_mutex_unlock(&sample_lock);
		break;

	default:
		break;
	}

	return 0;
}

static int channel_track(struct ssh_channel *channel)
{
	int free_slot = -1;

	for (size_t i = 0; i < ARRAY_SIZE(client_channels); i++) {
		if (client_channels[i] == channel) {
			return (int)i;
		}

		if (client_channels[i] == NULL && free_slot < 0) {
			free_slot = (int)i;
		}
	}

	if (free_slot >= 0) {
		client_channels[free_slot] = channel;
	}

	return free_slot;
}

static void channel_untrack(struct ssh_channel *channel)
{
	for (size_t i = 0; i < ARRAY_SIZE(client_channels); i++) {
		if (client_channels[i] == channel) {
			client_channels[i] = NULL;
		}
	}
}

/* Channel event callback. The channel is created with "sample open" and
 * removed with "sample close".
 */
static int channel_event_handler(struct ssh_channel *channel,
				 const struct ssh_channel_event *event,
				 void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event->type) {
	case SSH_CHANNEL_EVENT_OPEN_RESULT: {
		int i;

		k_mutex_lock(&sample_lock, K_FOREVER);
		i = channel_track(channel);
		k_mutex_unlock(&sample_lock);

		if (i < 0) {
			LOG_WRN("No free channel slots");
			return 0;
		}

		LOG_INF("Channel %d opened", i);
		break;
	}

	case SSH_CHANNEL_EVENT_RX_DATA_READY: {
		uint8_t buf[64];
		int len;

		while ((len = ssh_channel_read(channel, buf, sizeof(buf))) > 0) {
			LOG_INF("Channel RX: %.*s", len, buf);
		}
		break;
	}

	case SSH_CHANNEL_EVENT_CLOSED:
		k_mutex_lock(&sample_lock, K_FOREVER);
		channel_untrack(channel);
		k_mutex_unlock(&sample_lock);

		LOG_INF("Channel closed");
		break;

	default:
		break;
	}

	return 0;
}

static int cmd_sample_open(const struct shell *sh, size_t argc, char *argv[])
{
	struct ssh_transport *transport = NULL;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Use the first authenticated client connection. Establish one first
	 * with the SSH shell, e.g. "net ssh start <user>@<host>".
	 */
	k_mutex_lock(&sample_lock, K_FOREVER);
	for (size_t i = 0; i < ARRAY_SIZE(client_transport); i++) {
		if (client_transport[i] != NULL) {
			transport = client_transport[i];
			break;
		}
	}
	k_mutex_unlock(&sample_lock);

	if (transport == NULL) {
		shell_error(sh, "No client connection, use \"net ssh start\" first");
		return -ENOTCONN;
	}

	ret = ssh_transport_channel_open(transport, channel_event_handler, NULL);
	if (ret < 0) {
		shell_error(sh, "Failed to open SSH channel: %d", ret);
		return ret;
	}

	shell_print(sh, "Channel open requested");

	return 0;
}

static int cmd_sample_close(const struct shell *sh, size_t argc, char *argv[])
{
	struct ssh_channel *channel;
	int ch;
	int err = 0;

	ARG_UNUSED(argc);

	ch = shell_strtol(argv[1], 10, &err);
	if (err != 0 || ch < 0 || ch >= (int)ARRAY_SIZE(client_channels)) {
		shell_error(sh, "Invalid channel number: %s", argv[1]);
		return -EINVAL;
	}

	k_mutex_lock(&sample_lock, K_FOREVER);
	channel = client_channels[ch];
	k_mutex_unlock(&sample_lock);

	if (channel == NULL) {
		shell_error(sh, "Channel %d not open", ch);
		return -ENOENT;
	}

	err = ssh_channel_close(channel);
	if (err < 0) {
		shell_error(sh, "Failed to close channel %d: %d", ch, err);
		return err;
	}

	shell_print(sh, "Channel %d close requested", ch);

	return 0;
}
#endif /* CONFIG_SSH_CLIENT */

static int cmd_sample_info(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Network %sconnected", connected ? "" : "dis");

	k_mutex_lock(&sample_lock, K_FOREVER);

#if defined(CONFIG_SSH_SERVER)
	for (size_t i = 0; i < ARRAY_SIZE(server_transport); i++) {
		if (server_transport[i] != NULL) {
			shell_print(sh, "Server transport %zu: %p", i,
				    (void *)server_transport[i]);
		}
	}
#endif

#if defined(CONFIG_SSH_CLIENT)
	for (size_t i = 0; i < ARRAY_SIZE(client_transport); i++) {
		if (client_transport[i] != NULL) {
			shell_print(sh, "Client transport %zu: %p", i,
				    (void *)client_transport[i]);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(client_channels); i++) {
		if (client_channels[i] != NULL) {
			shell_print(sh, "Channel %zu: %p", i,
				    (void *)client_channels[i]);
		}
	}
#endif

	k_mutex_unlock(&sample_lock);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sample_commands,
#if defined(CONFIG_SSH_CLIENT)
	SHELL_CMD(open, NULL,
		  SHELL_HELP("Create a channel on the client connection", ""),
		  cmd_sample_open),
	SHELL_CMD_ARG(close, NULL,
		      SHELL_HELP("Remove a channel", "<channel>"),
		      cmd_sample_close, 2, 0),
#endif /* CONFIG_SSH_CLIENT */
	SHELL_CMD(info, NULL,
		  SHELL_HELP("List connections and channels", ""),
		  cmd_sample_info),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sample, &sample_commands,
		   "Sample application commands", NULL);

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");
		connected = true;
		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network disconnected");
		connected = false;
		return;
	}
}

int main(void)
{
	int ret;

	LOG_INF("SSH sample for %s", CONFIG_BOARD_TARGET);

#if defined(CONFIG_SSH_SERVER)
	ret = ssh_server_register_transport_callback(&server_transport_conf);
	if (ret < 0) {
		LOG_WRN("Failed to register SSH server transport callback: %d", ret);
	}
#endif

#if defined(CONFIG_SSH_CLIENT)
	ret = ssh_client_register_transport_callback(&client_transport_conf);
	if (ret < 0) {
		LOG_WRN("Failed to register SSH client transport callback: %d", ret);
	}
#endif

	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		net_mgmt_init_event_callback(&mgmt_cb,
					     event_handler, EVENT_MASK);
		net_mgmt_add_event_callback(&mgmt_cb);
		conn_mgr_mon_resend_status();
	}

#if defined(CONFIG_SSH_SERVER)
	ret = net_config_init_sshd(NULL, &my_priv_key_config,
				   &my_pub_key_config, &my_password_auth, 0);
	if (ret != 0) {
		LOG_ERR("Failed to initialize SSH server");
		return 0;
	}
#endif /* CONFIG_SSH_SERVER */

	k_sleep(K_FOREVER);
}
