/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nrf91_slm.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(nrf91_slm, CONFIG_MODEM_LOG_LEVEL);

static void nrf91_slm_chat_on_xgetaddrinfo(struct modem_chat *chat, char **argv, uint16_t argc,
					   void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;
	char buf[NET_IPV6_ADDR_LEN + 1];
	char *start;
	char *end;
	uintptr_t len;
	int ret;

	if (argc != 2) {
		return;
	}

	start = strchr(argv[1], '\"');
	if (!start) {
		LOG_ERR("malformed DNS response!!");
		return;
	}

	end = strchr(start + 1, '\"');
	if (!end) {
		LOG_ERR("malformed DNS response!!");
		return;
	}

	len = end - start - 1;
	memcpy(buf, start + 1, len);
	buf[len] = '\0';

	ret = net_addr_pton(data->dns_result.ai_family, buf,
			    &((struct sockaddr_in *)&data->dns_result_addr)->sin_addr);

	if (ret < 0) {
		LOG_ERR("failed to convert address (%d)", ret);
		memset(&data->dns_result, 0, sizeof(data->dns_result));
		memset(&data->dns_result_addr, 0, sizeof(data->dns_result_addr));
		return;
	}
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCH_DEFINE(abort_match, "ERROR", "", NULL);
MODEM_CHAT_MATCH_DEFINE(xgetaddrinfo_match, "#XGETADDRINFO: ", "", nrf91_slm_chat_on_xgetaddrinfo);

/* AT#XGETADDRINFO=<hostname>[,<family>] */
static int nrf91_slm_xgetaddrinfo(struct nrf91_slm_data *data, const char *hostname, int family)
{
	struct modem_chat_script script;
	struct modem_chat_script_chat script_chats[2];
	char request[sizeof("AT#XGETADDRINF=##,#") + NET_IPV6_ADDR_LEN];
	int ret;

	ret = snprintk(request, sizeof(request), "AT#XGETADDRINFO=\"%s\",%d", hostname, family);
	if (ret < 0) {
		return ret;
	}

	modem_chat_script_chat_init(&script_chats[0]);
	modem_chat_script_chat_set_request(&script_chats[0], request);
	modem_chat_script_chat_set_response_matches(&script_chats[0], &xgetaddrinfo_match, 1);

	modem_chat_script_chat_init(&script_chats[1]);
	modem_chat_script_chat_set_request(&script_chats[1], "");
	modem_chat_script_chat_set_response_matches(&script_chats[1], &ok_match, 1);

	modem_chat_script_init(&script);
	modem_chat_script_set_name(&script, "xgetaddrinfo");
	modem_chat_script_set_script_chats(&script, script_chats, 2);
	modem_chat_script_set_abort_matches(&script, &abort_match, 1);
	modem_chat_script_set_timeout(&script, 120000);

	return modem_chat_run_script(&data->chat, &script);
}

int nrf91_slm_getaddrinfo(struct nrf91_slm_data *data, const char *node, const char *service,
			  const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{
	uint32_t port = 0;
	int ret;

	/* Modem is not attached to the network. */
	if (data->state != NRF91_SLM_STATE_CARRIER_ON) {
		LOG_ERR("modem currently not attached to the network!");
		return DNS_EAI_AGAIN;
	}

	/* init result */
	memset(&data->dns_result, 0, sizeof(data->dns_result));
	memset(&data->dns_result_addr, 0, sizeof(data->dns_result_addr));

	/* Currently only support IPv4. */
	data->dns_result.ai_family = AF_INET;
	data->dns_result_addr.sa_family = AF_INET;
	data->dns_result.ai_addr = &data->dns_result_addr;
	data->dns_result.ai_addrlen = sizeof(data->dns_result_addr);
	data->dns_result.ai_canonname = data->dns_result_canonname;
	data->dns_result_canonname[0] = '\0';

	if (service) {
		port = atoi(service);
		if (port < 1 || port > USHRT_MAX) {
			return DNS_EAI_SERVICE;
		}
	}

	if (port > 0 && data->dns_result.ai_family == AF_INET) {
		net_sin(&data->dns_result_addr)->sin_port = htons(port);
	}

	/* Check if node is an IP address */
	if (net_addr_pton(data->dns_result.ai_family, node,
			  &((struct sockaddr_in *)&data->dns_result_addr)->sin_addr) == 0) {
		*res = &data->dns_result;
		return 0;
	}

	/* user flagged node as numeric host, but we failed net_addr_pton */
	if (hints && hints->ai_flags & AI_NUMERICHOST) {
		return DNS_EAI_NONAME;
	}

	ret = k_mutex_lock(&data->chat_lock, K_SECONDS(1));
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	ret = nrf91_slm_xgetaddrinfo(data, node, AF_INET);
	k_mutex_unlock(&data->chat_lock);

	if (ret < 0) {
		errno = -ret;
		return DNS_EAI_SYSTEM;
	}

	*res = (struct zsock_addrinfo *)&data->dns_result;
	return 0;
}

void nrf91_slm_freeaddrinfo(struct nrf91_slm_data *data, struct zsock_addrinfo *res)
{
	ARG_UNUSED(data);
	ARG_UNUSED(res);
}
