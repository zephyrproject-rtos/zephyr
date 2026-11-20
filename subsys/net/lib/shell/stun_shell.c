/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#include "net_shell_private.h"

#if defined(CONFIG_STUN_SHELL)

#include <errno.h>
#include <string.h>

#include <zephyr/net/stun_client.h>

/* How long "net stun query" waits unless it is told. With one address to ask it
 * is time for the request and three retransmissions of it, which RFC 8489
 * section 6.2.1 has at 0.5, 1.5 and 3.5 s.
 */
#define QUERY_TIMEOUT_MS 5000U

/* Longest DNS name, RFC 1035 section 2.3.4. */
#define SERVER_HOST_MAX_LEN 253U

/* The server "net stun query" asks; an empty name is no server. */
static char server_host[SERVER_HOST_MAX_LEN + 1U] = "";
static uint16_t server_port = STUN_CLIENT_DEFAULT_PORT;

/* Held by the command that reads or writes the server. */
static K_MUTEX_DEFINE(server_lock);

/* Print the server that is set, or that there is none. */
static void print_server(const struct shell *sh)
{
	if (server_host[0] == '\0') {
		PR("No STUN server set\n");
	} else {
		PR("STUN server: %s port %u\n", server_host, server_port);
	}
}

/* With no argument print the server; otherwise check the host name and the
 * port, then store both.
 */
static int stun_server(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t port = STUN_CLIENT_DEFAULT_PORT;
	size_t len = 0U;

	if (argc < 2) {
		print_server(sh);
		return 0;
	}

	len = strlen(argv[1]);
	if (len >= sizeof(server_host)) {
		PR_ERROR("Host name too long, %u characters at most\n",
			 (unsigned int)(sizeof(server_host) - 1U));
		return -EINVAL;
	}

	if (argc > 2) {
		int err = 0;
		unsigned long value = shell_strtoul(argv[2], 10, &err);

		if (err != 0 || value < 1UL || value > UINT16_MAX) {
			PR_ERROR("Invalid port, 1 to 65535 expected\n");
			return -EINVAL;
		}
		port = (uint16_t)value;
	}

	memcpy(server_host, argv[1], len + 1U);
	server_port = port;
	print_server(sh);
	return 0;
}

/* Parse the timeout, run stun_client_simple() against the server that is set
 * and print the mapped address or what went wrong.
 */
static int stun_query(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t timeout_ms = QUERY_TIMEOUT_MS;
	char text[NET_IPV6_ADDR_LEN] = "";
	struct net_sockaddr_storage mapped = {0};
	const struct net_sockaddr_in *mapped4 = NULL;
	int rc = 0;

	if (argc > 1) {
		int err = 0;
		unsigned long value = shell_strtoul(argv[1], 10, &err);

		if (err != 0 || value != (uint32_t)value) {
			PR_ERROR("Invalid timeout, milliseconds expected\n");
			return -EINVAL;
		}
		timeout_ms = (uint32_t)value;
	}

	if (server_host[0] == '\0') {
		PR_ERROR("No STUN server set, use: net stun server <host> [<port>]\n");
		return -ENOEXEC;
	}

	rc = stun_client_simple(server_host, server_port, timeout_ms, &mapped);
	switch (rc) {
	case 0:
		break;
	case -ETIMEDOUT:
		PR_ERROR("No answer from %s port %u\n", server_host, server_port);
		return -ENOEXEC;
	case -EHOSTUNREACH:
		PR_ERROR("Cannot resolve %s\n", server_host);
		return -ENOEXEC;
	case -EPROTO:
		PR_ERROR("%s answered with an error, or with no address\n", server_host);
		return -ENOEXEC;
	default:
		PR_ERROR("STUN query failed (%d)\n", rc);
		return -ENOEXEC;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && mapped.ss_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *mapped6 = net_sin6(net_sad(&mapped));

		(void)net_addr_ntop(NET_AF_INET6, &mapped6->sin6_addr, text, sizeof(text));
		PR("Mapped address: [%s]:%u\n", text, net_ntohs(mapped6->sin6_port));
		return 0;
	}
	mapped4 = net_sin(net_sad(&mapped));
	(void)net_addr_ntop(NET_AF_INET, &mapped4->sin_addr, text, sizeof(text));
	PR("Mapped address: %s:%u\n", text, net_ntohs(mapped4->sin_port));
	return 0;
}

/* Run the command with the server locked, or refuse it if another command
 * holds the lock.
 */
static int run_locked(shell_cmd_handler handler, const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;

	if (k_mutex_lock(&server_lock, K_NO_WAIT) != 0) {
		PR_ERROR("A STUN query is running, try again later\n");
		return -EBUSY;
	}

	rc = handler(sh, argc, argv);
	(void)k_mutex_unlock(&server_lock);
	return rc;
}

#endif /* CONFIG_STUN_SHELL */

/* "net stun server": the command when CONFIG_STUN_SHELL is set, else a hint how
 * to enable it.
 */
static int cmd_net_stun_server(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_STUN_SHELL)
	return run_locked(stun_server, sh, argc, argv);
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_STUN, CONFIG_STUN_CLIENT and CONFIG_STUN_SHELL", "STUN");
	return 0;
#endif /* CONFIG_STUN_SHELL */
}

/* "net stun query": the command when CONFIG_STUN_SHELL is set, else a hint how
 * to enable it.
 */
static int cmd_net_stun_query(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_STUN_SHELL)
	return run_locked(stun_query, sh, argc, argv);
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_STUN, CONFIG_STUN_CLIENT and CONFIG_STUN_SHELL", "STUN");
	return 0;
#endif /* CONFIG_STUN_SHELL */
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	net_cmd_stun,
	SHELL_CMD_ARG(server, NULL,
		      SHELL_HELP("Set the STUN server to ask, or show it. "
				 "An empty name (\"\") unsets it.",
				 "[<host> [<port>]]"),
		      cmd_net_stun_server, 1, 2),
	SHELL_CMD_ARG(query, NULL,
		      SHELL_HELP("Ask the STUN server for the address it sees this device at. "
				 "Waits 5 seconds unless told; 0 waits for all the "
				 "retransmissions, 39.5 seconds at each address with the "
				 "default configuration.",
				 "[<timeout in ms>]"),
		      cmd_net_stun_query, 1, 1),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((net), stun, &net_cmd_stun,
		 "Find out the public address of the device from a STUN server.", NULL, 1, 0);
