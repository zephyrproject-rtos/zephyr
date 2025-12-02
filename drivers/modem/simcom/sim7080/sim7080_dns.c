/*
 * Copyright (C) 2025 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT simcom_sim7080

#include <zephyr/logging/log.h>
#include <zephyr/net/offloaded_netdev.h>
LOG_MODULE_REGISTER(modem_simcom_sim7080_dns, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/drivers/modem/simcom-sim7080.h>
#include "sim7080.h"

static struct zsock_addrinfo dns_result;
static struct sockaddr dns_result_addr;
static char dns_result_canonname[DNS_MAX_NAME_SIZE + 1];

/*
 * Parses the dns response from the modem.
 *
 * Response on success:
 * +CDNSGIP: 1,<domain name>,<IPv4>[,<IPv6>]
 *
 * Response on failure:
 * +CDNSGIP: 0,<err>
 */
MODEM_CMD_DEFINE(on_cmd_cdnsgip)
{
	int state;
	char ips[256];
	size_t out_len;
	int ret = -1;

	state = atoi(argv[0]);
	if (state == 0) {
		LOG_ERR("DNS lookup failed with error %s", argv[1]);
		goto exit;
	}

	/* Offset to skip the leading " */
	out_len = net_buf_linearize(ips, sizeof(ips) - 1, data->rx_buf, 1, len);
	ips[out_len] = '\0';

	/* find trailing " */
	char *ipv4 = strstr(ips, "\"");

	if (!ipv4) {
		LOG_ERR("Malformed DNS response!!");
		goto exit;
	}

	*ipv4 = '\0';
	net_addr_pton(dns_result.ai_family, ips,
			  &((struct sockaddr_in *)&dns_result_addr)->sin_addr);
	ret = 0;

exit:
	k_sem_give(&mdata.sem_dns);
	return ret;
}

/*
 * Perform a dns lookup.
 */
static int offload_getaddrinfo(const char *node, const char *service,
				   const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{
	struct modem_cmd cmd[] = { MODEM_CMD("+CDNSGIP: ", on_cmd_cdnsgip, 2U, ",") };
	char sendbuf[sizeof("AT+CDNSGIP=\"\",##,#####") + 128];
	uint32_t port = 0;
	int ret;

	/* Modem is not attached to the network. */
	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return DNS_EAI_AGAIN;
	}

	/* init result */
	(void)memset(&dns_result, 0, sizeof(dns_result));
	(void)memset(&dns_result_addr, 0, sizeof(dns_result_addr));

	/* Currently only support IPv4. */
	dns_result.ai_family = AF_INET;
	dns_result_addr.sa_family = AF_INET;
	dns_result.ai_addr = &dns_result_addr;
	dns_result.ai_addrlen = sizeof(dns_result_addr);
	dns_result.ai_canonname = dns_result_canonname;
	dns_result_canonname[0] = '\0';

	if (service) {
		port = atoi(service);
		if (port < 1 || port > USHRT_MAX) {
			return DNS_EAI_SERVICE;
		}
	}

	if (port > 0U) {
		if (dns_result.ai_family == AF_INET) {
			net_sin(&dns_result_addr)->sin_port = htons(port);
		}
	}

	/* Check if node is an IP address */
	if (net_addr_pton(dns_result.ai_family, node,
			  &((struct sockaddr_in *)&dns_result_addr)->sin_addr) == 0) {
		*res = &dns_result;
		return 0;
	}

	/* user flagged node as numeric host, but we failed net_addr_pton */
	if (hints && hints->ai_flags & AI_NUMERICHOST) {
		return DNS_EAI_NONAME;
	}

	ret = snprintk(sendbuf, sizeof(sendbuf), "AT+CDNSGIP=\"%s\",%u,%u", node,
				mdata.dns.recount, mdata.dns.timeout);
	if (ret < 0) {
		LOG_ERR("Formatting dns query failed");
		return ret;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), sendbuf,
				 &mdata.sem_dns, MDM_DNS_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	*res = (struct zsock_addrinfo *)&dns_result;
	return 0;
}

/*
 * Free addrinfo structure.
 */
static void offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	/* No need to free static memory. */
	ARG_UNUSED(res);
}

/*
 * DNS vtable.
 */
const struct socket_dns_offload offload_dns_ops = {
	.getaddrinfo = offload_getaddrinfo,
	.freeaddrinfo = offload_freeaddrinfo,
};

int mdm_sim7080_dns_set_lookup_params(uint8_t recount, uint16_t timeout)
{
	if (recount > SIM7080_DNS_MAX_RECOUNT || timeout > SIM7080_DNS_MAX_TIMEOUT_MS) {
		return -EINVAL;
	}

	mdata.dns.recount = recount;
	mdata.dns.timeout = timeout;

	return 0;
}

void mdm_sim7080_dns_get_lookup_params(uint8_t *recount, uint16_t *timeout)
{
	if (recount) {
		*recount = mdata.dns.recount;
	}

	if (timeout) {
		*timeout = mdata.dns.timeout;
	}
}
