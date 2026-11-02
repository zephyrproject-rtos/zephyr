/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ssh/client.h>
#include <zephyr/net/ssh/keygen.h>
#include <zephyr/net/ssh/server.h>
#include <zephyr/shell/shell_ssh.h>
#include <zephyr/ztest.h>

#define LOOPBACK_ADDR   "127.0.0.1"
#define USER_NAME       "test"
#define PASSWORD        "secret"
#define HOST_KEY_INDEX  0
#define SESSION_TIMEOUT K_SECONDS(10)

struct client_state {
	struct k_sem shell_reply;
	struct k_sem rx_data;
	struct ssh_channel *channel;
	int replies;
	bool shell_ok;
};

static struct client_state client;

static int client_channel_cb(struct ssh_channel *channel, const struct ssh_channel_event *event,
			     void *user_data)
{
	struct client_state *state = user_data;

	switch (event->type) {
	case SSH_CHANNEL_EVENT_OPEN_RESULT:
		return ssh_channel_request_shell(channel);

	case SSH_CHANNEL_EVENT_REQUEST_RESULT:
		/* ssh_channel_request_shell() sends a pty-req before the shell
		 * request, both wanting a reply, so the second reply is the
		 * result of the shell request.
		 */
		state->replies++;
		if (state->replies == 2) {
			state->channel = channel;
			state->shell_ok = event->channel_request_result.success;
			k_sem_give(&state->shell_reply);
		}
		break;

	case SSH_CHANNEL_EVENT_RX_DATA_READY:
		/* Received data is never read, so the server runs out of
		 * channel window once the client buffer is full.
		 */
		k_sem_give(&state->rx_data);
		break;

	default:
		break;
	}

	return 0;
}

static int client_transport_cb(struct ssh_transport *transport,
			       const struct ssh_transport_event *event, void *user_data)
{
	ARG_UNUSED(user_data);

	switch (event->type) {
	case SSH_TRANSPORT_EVENT_SERVICE_ACCEPTED:
		return ssh_transport_auth_password(transport, USER_NAME, PASSWORD);

	case SSH_TRANSPORT_EVENT_AUTHENTICATE_RESULT:
		if (!event->authenticate_result.success) {
			k_sem_give(&client.shell_reply);
			return -EPERM;
		}

		return ssh_transport_channel_open(transport, client_channel_cb, &client);

	default:
		break;
	}

	return 0;
}

static void server_addr(struct net_sockaddr_in *addr)
{
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = NET_AF_INET;
	addr->sin_port = net_htons(CONFIG_SSH_PORT);
	zassert_equal(zsock_inet_pton(NET_AF_INET, LOOPBACK_ADDR, &addr->sin_addr), 1);
}

static bool open_shell_session(struct ssh_client *sshc)
{
	struct net_sockaddr_in addr;

	server_addr(&addr);

	client.channel = NULL;
	client.replies = 0;
	client.shell_ok = false;
	k_sem_reset(&client.shell_reply);
	k_sem_reset(&client.rx_data);

	zassert_ok(ssh_client_start(sshc, USER_NAME, (struct net_sockaddr *)&addr, -1,
				    client_transport_cb, NULL));
	zassert_ok(k_sem_take(&client.shell_reply, SESSION_TIMEOUT),
		   "No reply to the shell request");

	return client.shell_ok;
}

static void close_session(struct ssh_client *sshc)
{
	zassert_ok(ssh_client_stop(sshc));

	/* Let the server handle the disconnect and the shell thread exit */
	k_sleep(K_SECONDS(1));
}

/* A client that disconnects as soon as its shell request is accepted
 * closes the channel before the server shell has printed its prompt.
 */
ZTEST(shell_ssh, test_close_before_shell_output)
{
	struct ssh_client *sshc = ssh_client_instance(0);

	zassert_true(open_shell_session(sshc), "Shell session failed");
	close_session(sshc);

	zexpect_true(open_shell_session(sshc), "Shell session after a closed one failed");
	close_session(sshc);
}

/* A client that stops reading and then disconnects must not leave the
 * server shell instance unusable for the next session.
 */
ZTEST(shell_ssh, test_close_while_output_pending)
{
	static const char cmd[] = "help\r";
	struct ssh_client *sshc = ssh_client_instance(0);

	zassert_true(open_shell_session(sshc), "First shell session failed");

	/* The shell discards input that arrives before it has started, so
	 * wait for its prompt first.
	 */
	zassert_ok(k_sem_take(&client.rx_data, SESSION_TIMEOUT), "No shell prompt");

	/* The output is longer than the channel window of the client and
	 * the TX buffer of the server together, and the client never reads
	 * it, so the server shell thread ends up waiting for room to write.
	 */
	for (int i = 0; i < 8; i++) {
		zassert_equal(ssh_channel_write(client.channel, cmd, sizeof(cmd) - 1),
			      sizeof(cmd) - 1);
	}

	k_sleep(K_SECONDS(1));
	close_session(sshc);

	zexpect_true(open_shell_session(sshc), "Shell session after a stalled client failed");
	close_session(sshc);
}

static void *suite_setup(void)
{
	struct ssh_server *sshd = ssh_server_instance(0);
	struct net_sockaddr_in addr;

	zassert_not_null(sshd);
	zassert_not_null(ssh_client_instance(0));

	k_sem_init(&client.shell_reply, 0, 1);
	k_sem_init(&client.rx_data, 0, 1);

	zassert_ok(ssh_keygen(HOST_KEY_INDEX, SSH_HOST_KEY_TYPE_RSA, 2048),
		   "Failed to generate the host key");

	server_addr(&addr);
	zassert_ok(ssh_server_start(sshd, (struct net_sockaddr *)&addr, HOST_KEY_INDEX, USER_NAME,
				    PASSWORD, NULL, 0, shell_sshd_event_callback,
				    shell_sshd_transport_event_callback, NULL));
	k_sleep(K_MSEC(10));

	return NULL;
}

static void suite_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	zexpect_ok(ssh_server_stop(ssh_server_instance(0)));
}

ZTEST_SUITE(shell_ssh, NULL, suite_setup, NULL, NULL, suite_teardown);
