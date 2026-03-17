/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/method.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/http/parser_url.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/sys/sys_getopt.h>

static int cmd_net_http(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_HTTP_SERVER)
	int res_count = 0, serv_count = 0;

	PR("%-15s\t%-12s\n",
	   "Host:Port", "Concurrent/Backlog");
	PR("\tResource type\tMethods\t\tEndpoint\n");

	HTTP_SERVICE_FOREACH(svc) {
		PR("\n");
		PR("%s:%d\t%zu/%zu\n",
		   svc->host == NULL || svc->host[0] == '\0' ?
		   "<any>" : svc->host, svc->port ? *svc->port : 0,
		   svc->concurrent, svc->backlog);

		HTTP_SERVICE_FOREACH_RESOURCE(svc, res) {
			struct http_resource_detail *detail = res->detail;
			const char *detail_type = "<unknown>";
			int method_count = 0;
			bool print_comma;

			switch (detail->type) {
			case HTTP_RESOURCE_TYPE_STATIC:
				detail_type = "static";
				break;
			case HTTP_RESOURCE_TYPE_STATIC_FS:
				detail_type = "static_fs";
				break;
			case HTTP_RESOURCE_TYPE_DYNAMIC:
				detail_type = "dynamic";
				break;
			case HTTP_RESOURCE_TYPE_WEBSOCKET:
				detail_type = "websocket";
				break;
			}

			PR("\t%12s\t", detail_type);

			print_comma = false;

			for (int i = 0; i < NUM_BITS(uint32_t); i++) {
				if (IS_BIT_SET(detail->bitmask_of_supported_http_methods, i)) {
					PR("%s%s", print_comma ? "," : "", http_method_str(i));
					print_comma = true;
					method_count++;
				}
			}

			if (method_count < 2) {
				/* make columns line up better */
				PR("\t");
			}

			PR("\t%s\n", res->resource);
			res_count++;
		}

		serv_count++;
	}

	if (res_count == 0 && serv_count == 0) {
		PR("No HTTP services and resources found.\n");
	} else {
		PR("\n%d service%sand %d resource%sfound.\n",
		   serv_count, serv_count > 1 ? "s " : " ",
		   res_count, res_count > 1 ? "s " : " ");
	}

#else /* CONFIG_HTTP_SERVER */
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_HTTP_SERVER",
		"HTTP information");
#endif

	return 0;
}


#if defined(CONFIG_HTTP_PARSER)

#define HTTP_PORT  "80"
#define HTTPS_PORT "443"
static uint8_t recv_buf[CONFIG_NET_SHELL_HTTP_CLIENT_RECEIVE_BUFFER_SIZE];

static int create_socket(struct zsock_addrinfo *rp, bool is_tls, sec_tag_t sec_tag, int verify)
{
	int sock;

	if (is_tls) {
		sock = zsock_socket(rp->ai_family, rp->ai_socktype, IPPROTO_TLS_1_2);
	} else {
		sock = zsock_socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	}

	if (sock < 0) {
		return -errno;
	}

	if (is_tls) {
		int ret;

		ret = zsock_setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, &sec_tag, sizeof(sec_tag));
		if (ret < 0) {
			ret = -errno;
			zsock_close(sock);
			return ret;
		}

		ret = zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME, recv_buf, sizeof(recv_buf));
		if (ret < 0) {
			ret = -errno;
			zsock_close(sock);
			return ret;
		}

		ret = zsock_setsockopt(sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
		if (ret < 0) {
			ret = -errno;
			zsock_close(sock);
			return ret;
		}
	}

	return sock;
}

static int resolve_and_connect(const struct shell *sh, const char *host, const char *port,
			       bool is_tls, sec_tag_t sec_tag, int verify, int32_t timeout)
{
	int ret;
	int sock;
	struct zsock_addrinfo *res, *rp;
	static const struct zsock_addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
	};

	ret = zsock_getaddrinfo(host, port, &hints, &res);
	if (ret != 0) {
		PR_ERROR("getaddrinfo(%s, %s) failed: %d\n", host, port, ret);
		return -EINVAL;
	}

	for (rp = res; rp != NULL; rp = rp->ai_next) {
		struct net_sockaddr *sa = rp->ai_addr;
		void *addr = sa->sa_family == AF_INET ? (void *)&net_sin(sa)->sin_addr
						      : (void *)&net_sin6(sa)->sin6_addr;
		char *ip = zsock_inet_ntop(sa->sa_family, addr, recv_buf, sizeof(recv_buf));

		PR("Connecting to %s:%s\n", ip, port);

		sock = create_socket(rp, is_tls, sec_tag, verify);
		if (sock < 0) {
			PR_WARNING("create_socket() failed: %d\n", sock);
			continue;
		}

		ret = zsock_connect(sock, rp->ai_addr, rp->ai_addrlen);
		if (ret < 0) {
			PR_WARNING("connect() failed: %d\n", errno);
			zsock_close(sock);
			sock = -1;
			continue;
		}

		zsock_freeaddrinfo(res);
		return sock;
	}

	zsock_freeaddrinfo(res);
	return -1;
}

static void do_get_request(const struct shell *sh, int sock, const char *host, const char *path,
			   bool quiet_mode)
{
	int ret;
	int64_t start_time;
	size_t total = 0;

	memset(recv_buf, 0, sizeof(recv_buf));
	snprintk(recv_buf, sizeof(recv_buf),
		 "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
	ret = zsock_send(sock, recv_buf, strlen(recv_buf), 0);
	if (ret < 0) {
		PR_WARNING("send() failed: %d\n", errno);
		return;
	}
	PR("%s", recv_buf);

	start_time = k_uptime_ticks();

	while (true) {
		ret = zsock_recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
		if (ret < 0) {
			PR_WARNING("recv() failed: %d\n", errno);
			return;
		}
		if (ret == 0) {
			PR("Connection closed by peer\n");
			break;
		}

		total += ret;

		if (!quiet_mode) {
			recv_buf[ret] = '\0';
			PR("%s", recv_buf);
		}
	}

	int64_t end = k_uptime_ticks();
	int64_t elapsed_us = k_ticks_to_us_ceil64(end - start_time);
	int64_t rate = total * 8;

	/* Try to avoid overflow */
	rate *= MSEC_PER_SEC;
	rate /= elapsed_us;

	PR("Total received: %zu bytes\n", total);
	PR("Elapsed time: %lld ms\n", elapsed_us / 1000);
	PR("Throughput: %lld kbps\n", rate);
}

static int cmd_http_get(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	int c;
	int sock = -1;
	int32_t timeout = 10 * MSEC_PER_SEC;
	struct http_parser_url parser;
	sec_tag_t sec_tag = SEC_TAG_TLS_INVALID;
	int verify = TLS_PEER_VERIFY_NONE;
	bool is_tls = false;
	char port[6] = {0};
	char host[128] = {0};
	char path[128] = {'/', 0};
	bool quiet_mode = false;
	struct sys_getopt_state *state = sys_getopt_state_get();

	if (argc < 2) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	state->optind = 1;

	while ((c = sys_getopt(argc, argv, "qt:s:v:")) != -1) {
		switch (c) {
		case 'q':
			quiet_mode = true;
			break;
		case 't':
			timeout = (int32_t)strtoul(state->optarg, NULL, 10) * MSEC_PER_SEC;
			break;
		case 's':
			sec_tag = (sec_tag_t)strtoul(state->optarg, NULL, 10);
			break;
		case 'v':
			verify = atoi(state->optarg);
			break;
		case '?':
		default:
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	char *url = argv[state->optind];

	http_parser_url_init(&parser);
	ret = http_parser_parse_url(url, strlen(url), 0, &parser);
	if (ret < 0) {
		PR_ERROR("Invalid url: %s\n", url);
		return -EINVAL;
	}

	if (parser.field_set & (1 << UF_SCHEMA)) {
		if (strncmp("http", url + parser.field_data[UF_SCHEMA].off,
			    parser.field_data[UF_SCHEMA].len) == 0) {
			strcpy(port, HTTP_PORT);
		} else if (strncmp("https", url + parser.field_data[UF_SCHEMA].off,
				   parser.field_data[UF_SCHEMA].len) == 0) {
			strcpy(port, HTTPS_PORT);
			is_tls = true;
		} else {
			PR_ERROR("Invalid url: %s\n", url);
			return -EINVAL;
		}
	}

	if (is_tls && sec_tag == SEC_TAG_TLS_INVALID) {
		PR_ERROR("TLS sec_tag required for HTTPS\n");
		return -EINVAL;
	}

	if (parser.field_set & (1 << UF_PORT)) {
		if (parser.field_data[UF_PORT].len >= (sizeof(port) - 1)) {
			PR_ERROR("Invalid url: %s\n", url);
			return -EINVAL;
		}
		strncpy(port, url + parser.field_data[UF_PORT].off, parser.field_data[UF_PORT].len);
		port[parser.field_data[UF_PORT].len] = '\0';
	}
	if (parser.field_set & (1 << UF_PATH)) {
		if (parser.field_data[UF_PATH].len >= (sizeof(path) - 1)) {
			PR_ERROR("Invalid url: %s\n", url);
			return -EINVAL;
		}
		strncpy(path, url + parser.field_data[UF_PATH].off, parser.field_data[UF_PATH].len);
		path[parser.field_data[UF_PATH].len] = '\0';
	}

	if (!(parser.field_set & (1 << UF_HOST))) {
		PR_ERROR("Invalid url: %s\n", url);
		return -EINVAL;
	}

	if (parser.field_data[UF_HOST].len >= (sizeof(host) - 1)) {
		PR_ERROR("Invalid url: %s\n", url);
		return -EINVAL;
	}

	strncpy(host, url + parser.field_data[UF_HOST].off, parser.field_data[UF_HOST].len);
	host[parser.field_data[UF_HOST].len] = '\0';

	sock = resolve_and_connect(sh, host, port, is_tls, sec_tag, verify, timeout);
	if (sock < 0) {
		return -ENOEXEC;
	}

	do_get_request(sh, sock, host, path, quiet_mode);

	if (sock >= 0) {
		zsock_close(sock);
	}

	return ret;
}

#else /* CONFIG_HTTP_PARSER */

static int cmd_http_get(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n", "CONFIG_HTTP_PARSER", "HTTP client");
	return 0;
}

#endif /* CONFIG_HTTP_PARSER */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_http,
	SHELL_CMD(get, NULL, "get [-q] [-t <timeout>] [-s <sec_tag>] [-v <verify>] <url>",
		  cmd_http_get),
	SHELL_CMD(services, NULL, "Show HTTP services and resources", cmd_net_http),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((net), http, &sub_http,
		 "HTTP commands",
		 NULL, 1, 0);
