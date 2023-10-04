/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/shell/shell.h>


static int cmd_list(const struct shell *sh, size_t argc, char **argv)
{
	int count = 0;

	ARG_UNUSED(argv);

	if (argc > 1) {
		return -EINVAL;
	}

	shell_print(sh, "     Name             State            Endpoint");
	COAP_SERVICE_FOREACH(service) {
		shell_print(sh,
			    "[%2d] %-16s %-16s %s:%d",
			    ++count,
			    service->name,
			    service->data->sock_fd < 0 ? "INACTIVE" : "ACTIVE",
			    service->host != NULL ? service->host : "",
			    *service->port);
	}

	if (count == 0) {
		shell_print(sh, "No services available");
		return -ENOENT;
	}

	return 0;
}

static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	int ret = -ENOENT;

	if (argc != 2) {
		shell_error(sh, "Usage: start <service>");
		return -EINVAL;
	}

	COAP_SERVICE_FOREACH(service) {
		if (strcmp(argv[1], service->name) == 0) {
			ret = coap_service_start(service);
			break;
		}
	}

	if (ret < 0) {
		shell_error(sh, "Failed to start service (%d)", ret);
	}

	return ret;
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	int ret = -ENOENT;

	if (argc != 2) {
		shell_error(sh, "Usage: stop <service>");
		return -EINVAL;
	}

	COAP_SERVICE_FOREACH(service) {
		if (strcmp(argv[1], service->name) == 0) {
			ret = coap_service_stop(service);
			break;
		}
	}

	if (ret < 0) {
		shell_error(sh, "Failed to stop service (%d)", ret);
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_coap_service,
	SHELL_CMD(start, NULL,
		  "Start a CoAP Service\n"
		  "Usage: start <service>",
		  cmd_start),
	SHELL_CMD(stop, NULL,
		  "Stop a CoAP Service\n"
		  "Usage: stop <service>",
		  cmd_stop),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(coap_service, &sub_coap_service, "CoAP Service commands", cmd_list);
