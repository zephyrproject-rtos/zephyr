/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* for strtok_r() */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <getopt.h>
#include <string.h>

#include <zephyr/sys/sys_getopt.h>
#include <zephyr/net/socketutils.h>
#include <zephyr/shell/shell_ssh.h>
#include <zephyr/net/ssh/common.h>
#include <zephyr/net/ssh/server.h>
#include <zephyr/net/ssh/client.h>

#include "net_shell_private.h"

#if defined(CONFIG_SSH_SHELL)

#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 32
#define MAX_ADDRESS_LEN 64
#define MAX_HOSTKEY_LEN 32

#if defined(CONFIG_SSH_MAX_HOST_KEYS)
#define MAX_HOST_KEYS CONFIG_SSH_MAX_HOST_KEYS
#else
#define MAX_HOST_KEYS 1
#endif

struct ssh_params {
	char username[MAX_USERNAME_LEN + 1];
	char password[MAX_PASSWORD_LEN + 1];
	char address[MAX_ADDRESS_LEN + 1];
	int instance;
	int hostkey_idx;
	int auth_key_idx[MAX_HOST_KEYS];
	int auth_key_count;
};

#if defined(CONFIG_SSH_CLIENT)
static void shell_bypass_password(const struct shell *sh,
				  uint8_t *data, size_t len,
				  void *user_data)
{
	struct shell_ssh_context *ctx = user_data;
	struct shell_ssh_passwd *password = &ctx->password;

	while (len--) {
		uint8_t c = *data++;

		if (c == '\r' || c == '\n' ||
		    password->len >= sizeof(password->buf) - 1) {
			password->buf[password->len] = 0;
			shell_set_bypass(sh, NULL, NULL);

			if (ctx->transport != NULL) {
				const char *username;

				username = ssh_transport_client_user_name(ctx->transport);

				(void)ssh_transport_auth_password(ctx->transport,
								  username,
								  password->buf);
			}

			mbedtls_platform_zeroize(password->buf, sizeof(password->buf));
			password->len = 0;

			ctx->transport = NULL;
			break;
		}

		password->buf[password->len++] = c;
	}
}

static void shell_bypass_ssh_shell(const struct shell *sh,
				   uint8_t *data, size_t len,
				   void *user_data)
{
	struct shell_ssh_context *ctx = user_data;

	if (ctx->channel == NULL) {
		return;
	}

	(void)ssh_channel_write(ctx->channel, data, len);
}

static int ssh_client_channel_event_callback(struct ssh_channel *channel,
					     const struct ssh_channel_event *event,
					     void *user_data)
{
	struct shell_ssh_context *ctx = user_data;
	const struct shell *sh = ctx->sh;

	switch (event->type) {
	case SSH_CHANNEL_EVENT_OPEN_RESULT: {
		LOG_DBG("Client channel opened");
		return ssh_channel_request_shell(channel);
	}

	case SSH_CHANNEL_EVENT_REQUEST_RESULT:
		LOG_DBG("Client channel shell request complete");
		ctx->channel = channel;
		shell_set_bypass(sh, shell_bypass_ssh_shell, ctx);
		__fallthrough;

	case SSH_CHANNEL_EVENT_RX_DATA_READY: {
#define MAX_BUF_LEN 128
		uint8_t buf[MAX_BUF_LEN];

		while (true) {
			int len;

			len = ssh_channel_read(channel, buf, sizeof(buf));
			if (len <= 0) {
				return len;
			}

			shell_fprintf(sh, SHELL_NORMAL, "%.*s", len, buf);
		}

		break;
	}

	case SSH_CHANNEL_EVENT_TX_DATA_READY:
		break;

	case SSH_CHANNEL_EVENT_RX_STDERR_DATA_READY: {
		uint8_t buf[MAX_BUF_LEN];

		while (true) {
			int len;

			len = ssh_channel_read_stderr(channel, buf, sizeof(buf));
			if (len <= 0) {
				return len;
			}

			shell_fprintf(sh, SHELL_ERROR, "%.*s", len, buf);
		}

		break;
	}

	case SSH_CHANNEL_EVENT_EOF:
		LOG_DBG("Client channel EOF");
		break;

	case SSH_CHANNEL_EVENT_CLOSED:
		LOG_DBG("Client channel closed");
		shell_set_bypass(sh, NULL, NULL);
		ctx->channel = NULL;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int shell_ssh_client_transport_event_callback(struct ssh_transport *transport,
						     const struct ssh_transport_event *event,
						     void *user_data)
{
	struct shell_ssh_context *ctx = user_data;
	const struct shell *sh = ctx->sh;

	ctx->transport = transport;

	switch (event->type) {
	case SSH_TRANSPORT_EVENT_SERVICE_ACCEPTED: {
		const char *username;

		username = ssh_transport_client_user_name(transport);

		shell_set_bypass(sh, shell_bypass_password, ctx);
		shell_fprintf(sh, SHELL_INFO, "%s's password: ", username);

		return 0;

	}

	case SSH_TRANSPORT_EVENT_AUTHENTICATE_RESULT:
		if (event->authenticate_result.success) {
			return ssh_transport_channel_open(transport,
							  ssh_client_channel_event_callback,
							  ctx);
		}

		return -EPERM;

	default:
		return -EINVAL;
	}

	return -EINVAL;
}
#endif /* CONFIG_SSH_CLIENT */

#if defined(CONFIG_SSH_SERVER)
static int sshd_parse_args_to_params(const struct shell *sh,
				     int argc, char *argv[],
				     bool is_start,
				     struct ssh_params *config)
{
	struct sys_getopt_state *state;
	int option_index = 1;
	int opt;
	const char *short_options;
	const struct sys_getopt_option *long_options;

	static const char start_short_options[] = "a:b:i:k:p:u:h";
	static const struct sys_getopt_option start_long_options[] = {
		{ "authorized-keys", sys_getopt_required_argument, 0, 'a' },
		{ "bind-address", sys_getopt_required_argument, 0, 'b' },
		{ "instance", sys_getopt_required_argument, 0, 'i' },
		{ "hostkey", sys_getopt_required_argument, 0, 'k' },
		{ "password", sys_getopt_required_argument, 0, 'p' },
		{ "username", sys_getopt_required_argument, 0, 'u' },
		{ "help", sys_getopt_no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	static const char stop_short_options[] = "i:h";
	static const struct sys_getopt_option stop_long_options[] = {
		{ "instance", sys_getopt_required_argument, 0, 'i' },
		{ "help", sys_getopt_no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	if (is_start) {
		short_options = start_short_options;
		long_options = start_long_options;
	} else {
		short_options = stop_short_options;
		long_options = stop_long_options;
	}

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options,
				      &option_index)) != -1) {
		state = sys_getopt_state_get();

		if (is_start) {
			/* Only start command supports all options */
			switch (opt) {
			case 'a': {
				/* Parse comma-separated list of authorized key indices */
				char *token, *saveptr;
				size_t auth_key_idx = 0;

				token = strtok_r(state->optarg, ",", &saveptr);
				while (token != NULL && auth_key_idx < MAX_HOST_KEYS) {
					int idx, err = 0;

					idx = shell_strtol(token, 10, &err);
					if (err != 0) {
						PR_WARNING("Invalid auth key index \"%s\" (%d)\n",
							   token, err);
						return -EINVAL;
					}

					config->auth_key_idx[auth_key_idx] = idx;
					auth_key_idx++;
					token = strtok_r(NULL, ",", &saveptr);
				}

				config->auth_key_count = auth_key_idx;
				break;
			}
			case 'b':
				strncpy(config->address, state->optarg, MAX_ADDRESS_LEN);
				config->address[MAX_ADDRESS_LEN] = '\0';
				break;
			case 'i': {
				int err = 0;
				int instance;

				instance = shell_strtol(state->optarg, 10, &err);
				if (err != 0) {
					PR_WARNING("Invalid instance \"%s\" (%d)\n",
						   state->optarg, err);
					return -EINVAL;
				}

				config->instance = instance;
				break;
			}
			case 'k': {
				char hostkey[MAX_HOSTKEY_LEN + 1];
				int err = 0;
				int idx;

				strncpy(hostkey, state->optarg, MAX_HOSTKEY_LEN);
				hostkey[MAX_HOSTKEY_LEN] = '\0';

				idx = shell_strtol(hostkey, 10, &err);
				if (err != 0) {
					PR_WARNING("Invalid host key index \"%s\" (%d)\n",
						   hostkey, err);
					return -EINVAL;
				}

				config->hostkey_idx = idx;
				break;
			}
			case 'p':
				strncpy(config->password, state->optarg, MAX_PASSWORD_LEN);
				config->password[MAX_PASSWORD_LEN] = '\0';
				break;
			case 'u':
				strncpy(config->username, state->optarg, MAX_USERNAME_LEN);
				config->username[MAX_USERNAME_LEN] = '\0';
				break;
			case 'h':
			case '?':
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			default:
				PR_ERROR("Invalid option %c\n", state->optopt);
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
		} else {
			/* Stop command, only instance option is supported for now. */
			switch (opt) {
			case 'i': {
				int err = 0;
				int instance;

				instance = shell_strtol(state->optarg, 10, &err);
				if (err != 0) {
					PR_WARNING("Invalid instance \"%s\" (%d)\n",
						   state->optarg, err);
					return -EINVAL;
				}

				config->instance = instance;
				break;
			}
			case 'h':
			case '?':
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			default:
				PR_ERROR("Invalid option %c\n", state->optopt);
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
		}
	}

	return 0;
}
#endif /* CONFIG_SSH_SERVER */

#if defined(CONFIG_SSH_CLIENT)
static int ssh_parse_args_to_params(const struct shell *sh,
				    int argc, char *argv[],
				    bool is_start,
				    struct ssh_params *config)
{
	struct sys_getopt_state *state = NULL;
	int option_index = 1;
	int opt;
	const char *short_options;
	const struct sys_getopt_option *long_options;

	static const char start_short_options[] = "i:k:h";
	static const struct sys_getopt_option start_long_options[] = {
		{ "instance", sys_getopt_required_argument, 0, 'i' },
		{ "hostkey", sys_getopt_required_argument, 0, 'k' },
		{ "help", sys_getopt_no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	static const char stop_short_options[] = "i:h";
	static const struct sys_getopt_option stop_long_options[] = {
		{ "instance", sys_getopt_required_argument, 0, 'i' },
		{ "help", sys_getopt_no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	if (is_start) {
		short_options = start_short_options;
		long_options = start_long_options;
	} else {
		short_options = stop_short_options;
		long_options = stop_long_options;
	}

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options,
				      &option_index)) != -1) {
		state = sys_getopt_state_get();

		if (is_start) {
			/* Only start command supports all options */
			switch (opt) {
			case 'i': {
				int err = 0;
				int instance;

				instance = shell_strtol(state->optarg, 10, &err);
				if (err != 0) {
					PR_WARNING("Invalid instance \"%s\" (%d)\n",
						   state->optarg, err);
					return -EINVAL;
				}

				config->instance = instance;
				break;
			}
			case 'k': {
				char hostkey[MAX_HOSTKEY_LEN + 1];
				int err = 0;
				int idx;

				strncpy(hostkey, state->optarg, MAX_HOSTKEY_LEN);
				hostkey[MAX_HOSTKEY_LEN] = '\0';

				idx = shell_strtol(hostkey, 10, &err);
				if (err != 0) {
					PR_WARNING("Invalid host key index \"%s\" (%d)\n",
						   hostkey, err);
					return -EINVAL;
				}

				config->hostkey_idx = idx;
				break;
			}
			case 'h':
			case '?':
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			default:
				PR_ERROR("Invalid option %c\n", state->optopt);
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
		} else {
			/* Stop command, only instance option is supported for now. */
			switch (opt) {
			case 'i': {
				int err = 0;
				int instance;

				instance = shell_strtol(state->optarg, 10, &err);
				if (err != 0) {
					PR_WARNING("Invalid instance \"%s\" (%d)\n",
						   state->optarg, err);
					return -EINVAL;
				}

				config->instance = instance;
				break;
			}
			case 'h':
			case '?':
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			default:
				PR_ERROR("Invalid option %c\n", state->optopt);
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
		}
	}

	if (is_start && sys_getopt_optind < argc) {
		/* The first non-option argument is the destination address */
		strncpy(config->address, argv[sys_getopt_optind], MAX_ADDRESS_LEN);
		config->address[MAX_ADDRESS_LEN] = '\0';
	}

	return 0;
}
#endif /* CONFIG_SSH_CLIENT */
#endif /* CONFIG_SSH_SHELL */

static int cmd_sshd_start(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_SSH_SHELL)
#if defined(CONFIG_SSH_SERVER)
	struct ssh_params params = { 0 };
	struct net_sockaddr_storage bind_addr;
	struct ssh_server *sshd;
	int server_instance, hostkey_idx;
	int family;
	int ret;

	params.instance = -1;
	params.hostkey_idx = -1;

	ret = sshd_parse_args_to_params(sh, argc, argv, true, &params);
	if (ret < 0) {
		return ret;
	}

	if (params.instance < 0) {
		server_instance = 0; /* Default instance */
	} else {
		server_instance = params.instance;
	}

	if (params.hostkey_idx < 0) {
		hostkey_idx = 0; /* Default host key index */
	} else {
		hostkey_idx = params.hostkey_idx;
	}

	memset(&bind_addr, 0, sizeof(bind_addr));
	net_sin(net_sad(&bind_addr))->sin_port = htons(CONFIG_SSH_PORT);

	if (params.address[0] == '\0') {
		if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
			family = NET_AF_INET6; /* Use IPv6 to support both IPv4 and IPv6 */
		} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
			family = NET_AF_INET; /* Default to IPv4 */
		} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
			family = NET_AF_INET6; /* Default to IPv6 */
		} else {
			PR_ERROR("No IP stack enabled\n");
			return -ENOTSUP;
		}

		/* Bind to all interfaces */
		bind_addr.ss_family = family;
	} else {
		bool st;

		st = net_ipaddr_parse(params.address, strlen(params.address), net_sad(&bind_addr));
		if (!st) {
			PR_ERROR("Failed to parse bind address \"%s\"\n", params.address);
			return -EINVAL;
		}
	}

	sshd = ssh_server_instance(server_instance);
	if (sshd == NULL) {
		PR_ERROR("Failed to get SSH server instance\n");
		return -EINVAL;
	}

	ret = ssh_server_start(sshd,
			       net_sad(&bind_addr),
			       hostkey_idx,
			       params.username[0] != '\0' ? params.username : NULL,
			       params.password[0] != '\0' ? params.password : NULL,
			       params.auth_key_idx,
			       params.auth_key_count,
			       shell_sshd_event_callback,
			       shell_sshd_transport_event_callback,
			       NULL);
	if (ret != 0) {
		PR_ERROR("Failed to start SSH server: %d\n", ret);
	}

	return ret;
#else
	PR_INFO("SSH %s support is not enabled. Set %s to enable it.\n",
		"server", "CONFIG_SSH_SERVER");
	return -ENOTSUP;
#endif /* CONFIG_SSH_SERVER */
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_SSH_SHELL", "SSH");
	return 0;
#endif /* CONFIG_SSH_SHELL */
}

static int cmd_sshd_stop(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_SSH_SHELL)
#if defined(CONFIG_SSH_SERVER)
	struct ssh_params params = { 0 };
	struct ssh_server *sshd;
	int server_instance;
	int ret;

	params.instance = -1;

	ret = sshd_parse_args_to_params(sh, argc, argv, false, &params);
	if (ret < 0) {
		return ret;
	}

	if (params.instance < 0) {
		server_instance = 0; /* Default instance */
	} else {
		server_instance = params.instance;
	}

	sshd = ssh_server_instance(server_instance);
	if (sshd == NULL) {
		PR_ERROR("Failed to get SSH %s instance\n", "server");
		return -ENOENT;
	}

	ret = ssh_server_stop(sshd);
	if (ret < 0) {
		PR_ERROR("Failed to stop SSH %s: %d\n", "server", ret);
	}

	return ret;
#else
	PR_INFO("SSH %s support is not enabled. Set %s to enable it.\n",
		"server", "CONFIG_SSH_SERVER");
	return -ENOTSUP;
#endif /* CONFIG_SSH_SERVER */
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_SSH_SHELL", "SSH");
	return 0;
#endif /* CONFIG_SSH_SHELL */
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_sshd,
	SHELL_CMD_ARG(start, NULL,
		      SHELL_HELP("Start ssh server so peer can connect to us",
				 "[-k <host-key-idx>] [-b <bind-address>[:<port>]] "
				 "[-i <instance>]\n"
				 "[-p <password>] [-a <auth-key-idx>,<auth-key-idx>,...]\n"
				 "If port is omitted, then the default value "
				 STRINGIFY(CONFIG_SSH_PORT) " is used.\n"
				 "The bind address can be IPv4/6 address. "
				 "IPv6 address should be enclosed in brackets, "
				 "e.g. [2001:db8::1] if port is also given\n"
				 "If the bind address is omitted, then the server "
				 "will bind to all interfaces.\n"
				 "If instance is omitted, then the default value 0 is used.\n"
				 "The password is optional, if not set, then password\n"
				 "authentication will be disabled."),
		      cmd_sshd_start, 1, 10),
	SHELL_CMD_ARG(stop, NULL,
		      SHELL_HELP("Stop ssh server",
				 "[-i <instance>]\n"
				 "If instance is omitted, then the default value 0 is used."),
		      cmd_sshd_stop, 1, 3),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), sshd, &net_cmd_sshd,
		 "SSH server commands", NULL, 1, 1);

static int cmd_ssh_start(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_SSH_SHELL)
#if defined(CONFIG_SSH_CLIENT)
	struct ssh_params params = { 0 };
	struct net_sockaddr_storage dest_addr;
	struct ssh_client *sshc;
	static struct shell_ssh_context ctx;
	int client_instance, hostkey_idx;
	char *dst, *username;
	int ret;
	bool st;

	params.instance = -1;
	params.hostkey_idx = -1;

	ctx.sh = sh;

	ret = ssh_parse_args_to_params(sh, argc, argv, true, &params);
	if (ret < 0) {
		return ret;
	}

	if (params.instance < 0) {
		client_instance = 0; /* Default instance */
	} else {
		client_instance = params.instance;
	}

	hostkey_idx = params.hostkey_idx;

	memset(&dest_addr, 0, sizeof(dest_addr));
	net_sin(net_sad(&dest_addr))->sin_port = htons(CONFIG_SSH_PORT);

	if (params.address[0] == '\0') {
		PR_ERROR("Destination address is required\n");
		return -EINVAL;
	}

	dst = strstr(params.address, "@");
	if (dst == NULL) {
		PR_ERROR("Invalid destination format, expected <user>@<address>\n");
		return -EINVAL;
	}

	*dst = '\0'; /* Split user and address */
	username = params.address;

	st = net_ipaddr_parse(dst + 1, strlen(dst + 1), net_sad(&dest_addr));
	if (!st) {
		/* Address parsing failed, try to resolve it as a hostname */
		struct zsock_addrinfo *addr;
		struct zsock_addrinfo addrinfo_hint = {
			.ai_family = NET_AF_UNSPEC,
			.ai_socktype = NET_SOCK_STREAM,
			.ai_protocol = NET_IPPROTO_TCP,
		};

		ret = net_getaddrinfo_addr_str(dst + 1,
					       STRINGIFY(CONFIG_SSH_PORT),
					       &addrinfo_hint,
					       &addr);
		if (ret != 0) {
			PR_ERROR("Failed to parse destination address \"%s\"\n",
				 params.address);
			return -EINVAL;
		}

		memcpy(&dest_addr, addr->ai_addr, addr->ai_addrlen);
		zsock_freeaddrinfo(addr);
	}

	sshc = ssh_client_instance(client_instance);
	if (sshc == NULL) {
		PR_ERROR("Failed to get SSH client instance\n");
		return -ENOENT;
	}

	ret = ssh_client_start(sshc,
			       username,
			       net_sad(&dest_addr),
			       hostkey_idx,
			       shell_ssh_client_transport_event_callback,
			       &ctx);
	if (ret != 0) {
		PR_ERROR("Failed to start SSH client: %d\n", ret);
	} else {
		PR_INFO("SSH client started on instance %d\n", client_instance);
	}

	return ret;
#else
	PR_INFO("SSH %s support is not enabled. Set %s to enable it.\n",
		"client", "CONFIG_SSH_CLIENT");
	return -ENOTSUP;
#endif /* CONFIG_SSH_CLIENT */
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_SSH_SHELL", "SSH");
	return 0;
#endif /* CONFIG_SSH_SHELL */
}

static int cmd_ssh_stop(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_SSH_SHELL)
#if defined(CONFIG_SSH_CLIENT)
	struct ssh_params params = { 0 };
	struct ssh_client *sshc;
	int client_instance;
	int ret;

	params.instance = -1;

	ret = ssh_parse_args_to_params(sh, argc, argv, false, &params);
	if (ret < 0) {
		return ret;
	}

	if (params.instance < 0) {
		client_instance = 0; /* Default instance */
	} else {
		client_instance = params.instance;
	}

	sshc = ssh_client_instance(client_instance);
	if (sshc == NULL) {
		PR_ERROR("Failed to get SSH %s instance\n", "client");
		return -ENOENT;
	}

	ret = ssh_client_stop(sshc);
	if (ret < 0) {
		PR_ERROR("Failed to stop SSH %s: %d\n", "client", ret);
	}

	return ret;
#else
	PR_INFO("SSH %s support is not enabled. Set %s to enable it.\n",
		"client", "CONFIG_SSH_CLIENT");
	return -ENOTSUP;
#endif /* CONFIG_SSH_CLIENT */
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_SSH_SHELL", "SSH");
	return 0;
#endif /* CONFIG_SSH_SHELL */
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ssh,
	SHELL_CMD_ARG(start, NULL,
		      SHELL_HELP("Connect to ssh peer",
				 "[-k <host-key-idx>] [-i <instance>] <destination>[:<port>]\n"
				 "If port is omitted, then the default value "
				 STRINGIFY(CONFIG_SSH_PORT) " is used.\n"
				 "The destination address can be either a hostname or "
				 "IPv4/6 address. "
				 "IPv6 address should be enclosed in brackets, "
				 "e.g. [2001:db8::1] if port is also given\n"
				 "If instance is omitted, then the default value 0 is used."),
		      cmd_ssh_start, 2, 6),
	SHELL_CMD_ARG(stop, NULL,
		      SHELL_HELP("Stop ssh client connection",
				 "[-i <instance>]\n"
				 "If instance is omitted, then the default value 0 is used."),
		      cmd_ssh_stop, 1, 3),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), ssh, &net_cmd_ssh,
		 "SSH client commands", NULL, 1, 1);
