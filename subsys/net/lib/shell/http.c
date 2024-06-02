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

#define IS_BIT_SET(val, bit) (((val >> bit) & (0x1)) != 0)

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

SHELL_SUBCMD_ADD((net), http, NULL,
		 "Show HTTP services.",
		 cmd_net_http, 1, 0);
