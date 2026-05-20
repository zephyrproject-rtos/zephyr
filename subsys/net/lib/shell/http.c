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

#if defined(CONFIG_HTTP_SERVER)
static const char *http_protocols_str(const struct http_service_desc *svc)
{
	static char buf[sizeof("HTTPS/1.x, HTTPS/2.0, HTTPS/3.0")];
	const struct http_service_config *cfg = svc->config;
	int pos = 0;
	bool print_sep = false;
	const char *https;

	if (COND_CODE_1(CONFIG_NET_SOCKETS_SOCKOPT_TLS,
			(svc->sec_tag_list != NULL),
			(0))) {
		https = "S";
	} else {
		https = "";
	}

	if (cfg != NULL) {
		if (cfg->http_ver & HTTP_VERSION_1) {
			pos += snprintk(buf + pos, sizeof(buf) - pos, "HTTP%s/1.x", https);
			print_sep = true;
		}

		if (cfg->http_ver & HTTP_VERSION_2) {
			pos += snprintk(buf + pos, sizeof(buf) - pos, "%sHTTP%s/2.0",
					print_sep ? ", " : "", https);
			print_sep = true;
		}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		if ((cfg->http_ver & HTTP_VERSION_3) && svc->sec_tag_list != NULL) {
			pos += snprintk(buf + pos, sizeof(buf) - pos, "%sHTTPS/3.0",
					print_sep ? ", " : "");
		}
#endif
	} else {
		if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_1)) {
			pos += snprintk(buf + pos, sizeof(buf) - pos, "HTTP%s/1.x", https);
			print_sep = true;
		}

		if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_2)) {
			pos += snprintk(buf + pos, sizeof(buf) - pos, "%sHTTP%s/2.0",
					print_sep ? ", " : "", https);
			print_sep = true;
		}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		if (IS_ENABLED(CONFIG_HTTP_SERVER_VERSION_3) && svc->sec_tag_list != NULL) {
			pos += snprintk(buf + pos, sizeof(buf) - pos, "%sHTTPS/3.0",
					print_sep ? ", " : "");
		}
#endif
	}

	return buf;
}
#endif

static int cmd_net_http(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_HTTP_SERVER)
	int res_count = 0, serv_count = 0;

	PR("%-15s\t%-19s\t%s\n",
	   "Host:Port", "Concurrent/Backlog", "Protocols");
	PR("\tResource type\tMethods\t\tEndpoint\n");

	HTTP_SERVICE_FOREACH(svc) {
		char backlog_buf[sizeof("xxxxx/xxxxx")];

		snprintk(backlog_buf, sizeof(backlog_buf), "%zu/%zu",
			 svc->concurrent, svc->backlog);

		PR("\n");
		PR("%s:%d\t%-19s\t%s\n",
		   svc->host == NULL || svc->host[0] == '\0' ?
		   "<any>" : svc->host, svc->port ? *svc->port : 0,
		   backlog_buf,
		   http_protocols_str(svc));

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

/* clang-format off */
SHELL_SUBCMD_SET_CREATE(sub_http, (net, http));

SHELL_SUBCMD_ADD((net, http), services, NULL,
		 SHELL_HELP("Show HTTP services", NULL),
		 cmd_net_http, 1, 0);

SHELL_SUBCMD_ADD((net), http, &sub_http,
		 SHELL_HELP("HTTP commands", NULL),
		 NULL, 1, 0);
/* clang-format on */
