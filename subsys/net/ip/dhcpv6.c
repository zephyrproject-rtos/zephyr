/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief DHCPv6 client implementation
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dhcpv6, CONFIG_NET_DHCPV6_LOG_LEVEL);

#include <zephyr/net/dhcpv6.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/math_extras.h>

#include "dhcpv6_internal.h"
#include "ipv6.h"
#include "net_private.h"
#include "udp_internal.h"

/* Maximum number of options client can request. */
#define DHCPV6_MAX_OPTION_REQUEST 2

struct dhcpv6_options_include {
	bool clientid : 1;
	bool serverid : 1;
	bool elapsed_time : 1;
	bool ia_na : 1;
	bool iaaddr : 1;
	bool ia_pd : 1;
	bool iaprefix : 1;
	uint16_t oro[DHCPV6_MAX_OPTION_REQUEST];
};

static K_MUTEX_DEFINE(lock);

/* All_DHCP_Relay_Agents_and_Servers (ff02::1:2) */
static const struct in6_addr all_dhcpv6_ra_and_servers = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
							       0, 0, 0, 0, 0, 0x01, 0, 0x02 } } };

static sys_slist_t dhcpv6_ifaces = SYS_SLIST_STATIC_INIT(&dhcpv6_ifaces);
static struct k_work_delayable dhcpv6_timeout_work;
static struct net_mgmt_event_callback dhcpv6_mgmt_cb;

const char *net_dhcpv6_state_name(enum net_dhcpv6_state state)
{
	static const char * const name[] = {
		"disabled",
		"init",
		"soliciting",
		"requesting",
		"confirming",
		"renewing",
		"rebinding",
		"information requesting",
		"bound",
	};

	__ASSERT_NO_MSG(state >= 0 && state < sizeof(name));
	return name[state];
}

static void dhcpv6_generate_tid(struct net_if *iface)
{
	sys_rand_get(iface->config.dhcpv6.tid, sizeof(iface->config.dhcpv6.tid));
}

static void dhcvp6_update_deadlines(struct net_if *iface, int64_t now,
				    uint32_t t1, uint32_t t2,
				    uint32_t preferred_lifetime,
				    uint32_t valid_lifetime)
{
	uint64_t t1_abs, t2_abs, expire_abs;

	/* In case server does not set T1/T2 values, the time choice is left to
	 * the client discretion.
	 * Here, we use recommendations for the servers, where it's advised to
	 * set T1/T2 as 0.5 and 0.8 of the preferred lifetime.
	 */
	if (t1 == 0 && t2 == 0) {
		if (preferred_lifetime == DHCPV6_INFINITY) {
			t1 = DHCPV6_INFINITY;
			t2 = DHCPV6_INFINITY;
		} else {
			t1 = preferred_lifetime * 0.5;
			t2 = preferred_lifetime * 0.8;
		}
	} else if (t1 == 0) {
		if (t2 == DHCPV6_INFINITY) {
			t1 = DHCPV6_INFINITY;
		} else {
			t1 = t2 * 0.625; /* 0.5 / 0.8 */
		}
	} else if (t2 == 0) {
		if (t1 == DHCPV6_INFINITY) {
			t2 = DHCPV6_INFINITY;
		} else {
			t2 = t1 * 1.6; /* 0.8 / 0.5 */
			/* Overflow check. */
			if (t2 < t1) {
				t2 = DHCPV6_INFINITY;
			}
		}
	} else if (t1 >= t2) {
		NET_ERR("Invalid T1(%u)/T2(%u) values.", t1, t2);
		return;
	}

	if (t1 == DHCPV6_INFINITY ||
	    u64_add_overflow(now, 1000ULL * t1, &t1_abs)) {
		t1_abs = UINT64_MAX;
	}

	if (t2 == DHCPV6_INFINITY ||
	    u64_add_overflow(now, 1000ULL * t2, &t2_abs)) {
		t2_abs = UINT64_MAX;
	}

	if (valid_lifetime == DHCPV6_INFINITY ||
	    u64_add_overflow(now, 1000ULL * valid_lifetime, &expire_abs)) {
		expire_abs = UINT64_MAX;
	}

	if (iface->config.dhcpv6.t1 > t1_abs) {
		iface->config.dhcpv6.t1 = t1_abs;
	}

	if (iface->config.dhcpv6.t2 > t2_abs) {
		iface->config.dhcpv6.t2 = t2_abs;
	}

	if (iface->config.dhcpv6.expire < expire_abs) {
		iface->config.dhcpv6.expire = expire_abs;
	}
}

static void dhcpv6_set_timeout(struct net_if *iface, uint64_t timeout)
{
	int64_t now = k_uptime_get();

	NET_DBG("sched dhcpv6 timeout iface=%p timeout=%llums", iface, timeout);

	if (u64_add_overflow(now, timeout, &iface->config.dhcpv6.timeout)) {
		iface->config.dhcpv6.timeout = UINT64_MAX;
	}
}

static void dhcpv6_reschedule(void)
{
	k_work_reschedule(&dhcpv6_timeout_work, K_NO_WAIT);
}

static int randomize_timeout(int multiplier, int timeout)
{
	int factor;

	/* DHCPv6 RFC8415, ch. 15. the randomization factor should be a random
	 * number between -0.1 nand +0.1. As we operate on integers here, we
	 * scale it to -100 and +100, and divide the result by 1000.
	 */
	factor = (int)(sys_rand32_get() % 201) - 100;

	return (multiplier * timeout) + ((factor * timeout) / 1000);
}

static int dhcpv6_initial_retransmit_time(int init_retransmit_time)
{
	/* DHCPv6 RFC8415, ch. 15. Retransmission time for the first msg. */
	return randomize_timeout(1, init_retransmit_time);
}

static uint32_t dhcpv6_next_retransmit_time(int prev_retransmit_time,
					    int max_retransmit_time)
{
	int retransmit_time;

	/* DHCPv6 RFC8415, ch. 15. Retransmission time for the subsequent msg. */
	retransmit_time = randomize_timeout(2, prev_retransmit_time);

	if (max_retransmit_time == 0) {
		return retransmit_time;
	}

	if (retransmit_time > max_retransmit_time) {
		retransmit_time = randomize_timeout(1, max_retransmit_time);
	}

	return retransmit_time;
}

/* DHCPv6 packet encoding functions */

static int dhcpv6_add_header(struct net_pkt *pkt, enum dhcpv6_msg_type type,
			     uint8_t *tid)
{
	int ret;

	ret = net_pkt_write_u8(pkt, type);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write(pkt, tid, DHCPV6_TID_SIZE);

	return ret;
}

static int dhcpv6_add_option_header(struct net_pkt *pkt,
				    enum dhcpv6_option_code code,
				    uint16_t length)
{
	int ret;

	ret = net_pkt_write_be16(pkt, code);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be16(pkt, length);

	return ret;
}

static int dhcpv6_add_option_clientid(struct net_pkt *pkt,
				      struct net_dhcpv6_duid_storage *clientid)
{
	int ret;

	ret = dhcpv6_add_option_header(pkt, DHCPV6_OPTION_CODE_CLIENTID,
				       clientid->length);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write(pkt, &clientid->duid, clientid->length);

	return ret;
}

static int dhcpv6_add_option_serverid(struct net_pkt *pkt,
				      struct net_dhcpv6_duid_storage *serverid)
{
	int ret;

	ret = dhcpv6_add_option_header(pkt, DHCPV6_OPTION_CODE_SERVERID,
				       serverid->length);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write(pkt, &serverid->duid, serverid->length);

	return ret;
}


static int dhcpv6_add_option_elapsed_time(struct net_pkt *pkt, uint64_t since)
{
	uint64_t elapsed;
	int ret;

	ret = dhcpv6_add_option_header(pkt, DHCPV6_OPTION_CODE_ELAPSED_TIME,
				       DHCPV6_OPTION_ELAPSED_TIME_SIZE);
	if (ret < 0) {
		return ret;
	}

	/* Elapsed time should be expressed in hundredths of a second. */
	elapsed = (k_uptime_get() - since) / 10ULL;
	if (elapsed > 0xFFFF) {
		elapsed = 0xFFFF;
	}

	ret = net_pkt_write_be16(pkt, (uint16_t)elapsed);

	return ret;
}

static int dhcpv6_add_option_ia_na(struct net_pkt *pkt, struct dhcpv6_ia_na *ia_na,
				   bool include_addr)
{
	uint16_t optlen;
	int ret;

	if (!include_addr) {
		optlen = DHCPV6_OPTION_IA_NA_HEADER_SIZE;
	} else {
		optlen = DHCPV6_OPTION_IA_NA_HEADER_SIZE +
			 DHCPV6_OPTION_HEADER_SIZE +
			 DHCPV6_OPTION_IAADDR_HEADER_SIZE;
	}

	ret = dhcpv6_add_option_header(pkt, DHCPV6_OPTION_CODE_IA_NA, optlen);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_na->iaid);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_na->t1);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_na->t2);
	if (ret < 0) {
		return ret;
	}

	if (!include_addr) {
		return 0;
	}

	ret = dhcpv6_add_option_header(pkt, DHCPV6_OPTION_CODE_IAADDR,
				       DHCPV6_OPTION_IAADDR_HEADER_SIZE);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write(pkt, &ia_na->iaaddr.addr, sizeof(ia_na->iaaddr.addr));
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_na->iaaddr.preferred_lifetime);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_na->iaaddr.valid_lifetime);

	return ret;
}

static int dhcpv6_add_option_ia_pd(struct net_pkt *pkt, struct dhcpv6_ia_pd *ia_pd,
				   bool include_prefix)
{
	uint16_t optlen;
	int ret;

	if (!include_prefix) {
		optlen = DHCPV6_OPTION_IA_PD_HEADER_SIZE;
	} else {
		optlen = DHCPV6_OPTION_IA_PD_HEADER_SIZE +
			 DHCPV6_OPTION_HEADER_SIZE +
			 DHCPV6_OPTION_IAPREFIX_HEADER_SIZE;
	}

	ret = dhcpv6_add_option_header(pkt, DHCPV6_OPTION_CODE_IA_PD,
				       optlen);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_pd->iaid);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_pd->t1);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_pd->t2);
	if (ret < 0) {
		return ret;
	}

	if (!include_prefix) {
		return 0;
	}

	ret = dhcpv6_add_option_header(pkt, DHCPV6_OPTION_CODE_IAPREFIX,
				       DHCPV6_OPTION_IAPREFIX_HEADER_SIZE);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_pd->iaprefix.preferred_lifetime);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, ia_pd->iaprefix.valid_lifetime);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_u8(pkt, ia_pd->iaprefix.prefix_len);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write(pkt, &ia_pd->iaprefix.prefix,
			    sizeof(ia_pd->iaprefix.prefix));

	return ret;
}

static int dhcpv6_add_option_oro(struct net_pkt *pkt, uint16_t *codes,
				 int code_cnt)
{
	int ret;

	ret = dhcpv6_add_option_header(pkt, DHCPV6_OPTION_CODE_ORO,
				       sizeof(uint16_t) * code_cnt);
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < code_cnt; i++) {
		ret = net_pkt_write_be16(pkt, codes[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

static size_t dhcpv6_calculate_message_size(struct dhcpv6_options_include *options)
{
	size_t msg_size = sizeof(struct dhcpv6_msg_hdr);
	uint8_t oro_cnt = 0;

	if (options->clientid) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE;
		msg_size += sizeof(struct net_dhcpv6_duid_storage);
	}

	if (options->serverid) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE;
		msg_size += sizeof(struct net_dhcpv6_duid_storage);
	}

	if (options->elapsed_time) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE;
		msg_size += DHCPV6_OPTION_ELAPSED_TIME_SIZE;
	}

	if (options->ia_na) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE;
		msg_size += DHCPV6_OPTION_IA_NA_HEADER_SIZE;
	}

	if (options->iaaddr) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE;
		msg_size += DHCPV6_OPTION_IAADDR_HEADER_SIZE;
	}

	if (options->ia_pd) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE;
		msg_size += DHCPV6_OPTION_IA_PD_HEADER_SIZE;
	}

	if (options->iaprefix) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE;
		msg_size += DHCPV6_OPTION_IAPREFIX_HEADER_SIZE;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(options->oro); i++) {
		if (options->oro[i] == 0) {
			break;
		}

		oro_cnt++;
	}

	if (oro_cnt > 0) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE;
		msg_size += oro_cnt * sizeof(uint16_t);
	}

	return msg_size;
}

static int dhcpv6_add_options(struct net_if *iface, struct net_pkt *pkt,
			      struct dhcpv6_options_include *options)
{
	uint8_t oro_cnt = 0;
	int ret;

	if (options->clientid) {
		ret = dhcpv6_add_option_clientid(
				pkt, &iface->config.dhcpv6.clientid);
		if (ret < 0) {
			goto fail;
		}
	}

	if (options->serverid) {
		ret = dhcpv6_add_option_serverid(
				pkt, &iface->config.dhcpv6.serverid);
		if (ret < 0) {
			goto fail;
		}
	}

	if (options->elapsed_time) {
		ret = dhcpv6_add_option_elapsed_time(
				pkt, iface->config.dhcpv6.exchange_start);
		if (ret < 0) {
			goto fail;
		}
	}

	if (options->ia_na) {
		struct dhcpv6_ia_na ia_na = {
			.iaid = iface->config.dhcpv6.addr_iaid,
		};

		if (options->iaaddr) {
			memcpy(&ia_na.iaaddr.addr, &iface->config.dhcpv6.addr,
			       sizeof(ia_na.iaaddr.addr));
		}

		ret = dhcpv6_add_option_ia_na(pkt, &ia_na, options->iaaddr);
		if (ret < 0) {
			goto fail;
		}
	}

	if (options->ia_pd) {
		struct dhcpv6_ia_pd ia_pd = {
			.iaid = iface->config.dhcpv6.prefix_iaid,
		};

		if (options->iaprefix) {
			memcpy(&ia_pd.iaprefix.prefix, &iface->config.dhcpv6.prefix,
			       sizeof(ia_pd.iaprefix.prefix));
			ia_pd.iaprefix.prefix_len = iface->config.dhcpv6.prefix_len;
		}

		ret = dhcpv6_add_option_ia_pd(pkt, &ia_pd, options->iaprefix);
		if (ret < 0) {
			goto fail;
		}
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(options->oro); i++) {
		if (options->oro[i] == 0) {
			break;
		}

		oro_cnt++;
	}

	if (oro_cnt > 0) {
		ret = dhcpv6_add_option_oro(pkt, options->oro, oro_cnt);
		if (ret < 0) {
			goto fail;
		}
	}

	return 0;

fail:
	return ret;
}

static struct net_pkt *dhcpv6_create_message(struct net_if *iface,
					     enum dhcpv6_msg_type msg_type,
					     struct dhcpv6_options_include *options)
{
	struct in6_addr *local_addr;
	struct net_pkt *pkt;
	size_t msg_size;

	local_addr = net_if_ipv6_get_ll(iface, NET_ADDR_ANY_STATE);
	if (local_addr == NULL) {
		NET_ERR("No LL address");
		return NULL;
	}

	msg_size = dhcpv6_calculate_message_size(options);

	pkt = net_pkt_alloc_with_buffer(iface, msg_size, AF_INET6,
					IPPROTO_UDP, K_FOREVER);
	if (pkt == NULL) {
		return NULL;
	}

	if (net_ipv6_create(pkt, local_addr, &all_dhcpv6_ra_and_servers) < 0 ||
	    net_udp_create(pkt, htons(DHCPV6_CLIENT_PORT),
			   htons(DHCPV6_SERVER_PORT)) < 0) {
		goto fail;
	}

	dhcpv6_generate_tid(iface);

	if (dhcpv6_add_header(pkt, msg_type, iface->config.dhcpv6.tid) < 0) {
		goto fail;
	}

	if (dhcpv6_add_options(iface, pkt, options) < 0) {
		goto fail;
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_UDP);

	return pkt;

fail:
	net_pkt_unref(pkt);

	return NULL;
}

static int dhcpv6_send_solicit(struct net_if *iface)
{
	int ret;
	struct net_pkt *pkt;
	struct dhcpv6_options_include options = {
		.clientid = true,
		.elapsed_time = true,
		.ia_na = iface->config.dhcpv6.params.request_addr,
		.ia_pd = iface->config.dhcpv6.params.request_prefix,
		.oro = { DHCPV6_OPTION_CODE_SOL_MAX_RT },
	};

	pkt = dhcpv6_create_message(iface, DHCPV6_MSG_TYPE_SOLICIT, &options);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

static int dhcpv6_send_request(struct net_if *iface)
{
	int ret;
	struct net_pkt *pkt;
	struct dhcpv6_options_include options = {
		.clientid = true,
		.serverid = true,
		.elapsed_time = true,
		.ia_na = iface->config.dhcpv6.params.request_addr,
		.ia_pd = iface->config.dhcpv6.params.request_prefix,
		.oro = { DHCPV6_OPTION_CODE_SOL_MAX_RT },
	};

	pkt = dhcpv6_create_message(iface, DHCPV6_MSG_TYPE_REQUEST, &options);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

static int dhcpv6_send_renew(struct net_if *iface)
{
	int ret;
	struct net_pkt *pkt;
	struct dhcpv6_options_include options = {
		.clientid = true,
		.serverid = true,
		.elapsed_time = true,
		.ia_na = iface->config.dhcpv6.params.request_addr,
		.iaaddr = iface->config.dhcpv6.params.request_addr,
		.ia_pd = iface->config.dhcpv6.params.request_prefix,
		.iaprefix = iface->config.dhcpv6.params.request_prefix,
		.oro = { DHCPV6_OPTION_CODE_SOL_MAX_RT },
	};

	pkt = dhcpv6_create_message(iface, DHCPV6_MSG_TYPE_RENEW, &options);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

static int dhcpv6_send_rebind(struct net_if *iface)
{
	int ret;
	struct net_pkt *pkt;
	struct dhcpv6_options_include options = {
		.clientid = true,
		.elapsed_time = true,
		.ia_na = iface->config.dhcpv6.params.request_addr,
		.iaaddr = iface->config.dhcpv6.params.request_addr,
		.ia_pd = iface->config.dhcpv6.params.request_prefix,
		.iaprefix = iface->config.dhcpv6.params.request_prefix,
		.oro = { DHCPV6_OPTION_CODE_SOL_MAX_RT },
	};

	pkt = dhcpv6_create_message(iface, DHCPV6_MSG_TYPE_REBIND, &options);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

static int dhcpv6_send_confirm(struct net_if *iface)
{
	int ret;
	struct net_pkt *pkt;
	struct dhcpv6_options_include options = {
		.clientid = true,
		.elapsed_time = true,
		.ia_na = true,
		.iaaddr = true,
	};

	pkt = dhcpv6_create_message(iface, DHCPV6_MSG_TYPE_CONFIRM, &options);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

/* DHCPv6 packet parsing functions */

static int dhcpv6_parse_option_clientid(struct net_pkt *pkt, uint16_t length,
					struct net_dhcpv6_duid_storage *clientid)
{
	struct net_dhcpv6_duid_raw duid;
	int ret;

	if (length > sizeof(struct net_dhcpv6_duid_raw)) {
		NET_ERR("DUID too large to handle");
		return -EMSGSIZE;
	}

	ret = net_pkt_read(pkt, &duid, length);
	if (ret < 0) {
		return ret;
	}

	clientid->length = length;
	memcpy(&clientid->duid, &duid, length);

	return 0;
}

static int dhcpv6_parse_option_serverid(struct net_pkt *pkt, uint16_t length,
					struct net_dhcpv6_duid_storage *serverid)
{
	struct net_dhcpv6_duid_raw duid;
	int ret;

	if (length > sizeof(struct net_dhcpv6_duid_raw)) {
		NET_ERR("DUID too large to handle");
		return -EMSGSIZE;
	}

	ret = net_pkt_read(pkt, &duid, length);
	if (ret < 0) {
		return ret;
	}

	serverid->length = length;
	memcpy(&serverid->duid, &duid, length);

	return 0;
}

static int dhcpv6_parse_option_preference(struct net_pkt *pkt, uint16_t length,
					  uint8_t *preference)
{
	if (length != DHCPV6_OPTION_PREFERENCE_SIZE) {
		return -EBADMSG;
	}

	if (net_pkt_read_u8(pkt, preference) < 0) {
		return -EBADMSG;
	}

	return 0;
}

static int dhcpv6_parse_option_status_code(struct net_pkt *pkt,
					   uint16_t length, uint16_t *status)
{
	int ret;

	if (length < DHCPV6_OPTION_STATUS_CODE_HEADER_SIZE) {
		NET_ERR("Invalid IAADDR option size");
		return -EMSGSIZE;
	}

	ret = net_pkt_read_be16(pkt, status);
	if (ret < 0) {
		return ret;
	}

	NET_DBG("status code %d", *status);

	length -= DHCPV6_OPTION_STATUS_CODE_HEADER_SIZE;
	if (length > 0) {
		/* Ignore status message */
		ret = net_pkt_skip(pkt, length);
	}

	return ret;
}

static int dhcpv6_parse_option_iaaddr(struct net_pkt *pkt, uint16_t length,
				      struct dhcpv6_iaaddr *iaaddr)
{
	int ret;

	if (length < DHCPV6_OPTION_IAADDR_HEADER_SIZE) {
		NET_ERR("Invalid IAADDR option size");
		return -EMSGSIZE;
	}

	ret = net_pkt_read(pkt, &iaaddr->addr, sizeof(iaaddr->addr));
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read_be32(pkt, &iaaddr->preferred_lifetime);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read_be32(pkt, &iaaddr->valid_lifetime);
	if (ret < 0) {
		return ret;
	}

	/* DHCPv6 RFC8415, ch. 21.6 The client MUST discard any addresses for
	 * which the preferred lifetime is greater than the valid lifetime.
	 */
	if (iaaddr->preferred_lifetime > iaaddr->valid_lifetime) {
		return -EBADMSG;
	}

	NET_DBG("addr %s preferred_lifetime %d valid_lifetime %d",
		net_sprint_ipv6_addr(&iaaddr->addr), iaaddr->preferred_lifetime,
		iaaddr->valid_lifetime);

	iaaddr->status = DHCPV6_STATUS_SUCCESS;

	length -= DHCPV6_OPTION_IAADDR_HEADER_SIZE;
	while (length > 0) {
		uint16_t code, sublen;

		ret = net_pkt_read_be16(pkt, &code);
		if (ret < 0) {
			return ret;
		}

		ret = net_pkt_read_be16(pkt, &sublen);
		if (ret < 0) {
			return ret;
		}

		switch (code) {
		case DHCPV6_OPTION_CODE_STATUS_CODE:
			ret = dhcpv6_parse_option_status_code(pkt, sublen,
							      &iaaddr->status);
			if (ret < 0) {
				return ret;
			}

			break;
		default:
			NET_DBG("Unexpected option %d length %d", code, sublen);

			ret = net_pkt_skip(pkt, sublen);
			if (ret < 0) {
				return ret;
			}

			break;
		}

		length -= (sublen + 4);
	}

	return 0;
}

static int dhcpv6_parse_option_ia_na(struct net_pkt *pkt, uint16_t length,
				     struct dhcpv6_ia_na *ia_na)
{
	int ret;

	if (length < DHCPV6_OPTION_IA_NA_HEADER_SIZE) {
		NET_ERR("Invalid IA_NA option size");
		return -EMSGSIZE;
	}

	ret = net_pkt_read_be32(pkt, &ia_na->iaid);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read_be32(pkt, &ia_na->t1);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read_be32(pkt, &ia_na->t2);
	if (ret < 0) {
		return ret;
	}

	/* DHCPv6 RFC8415, ch. 21.4 If a client receives an IA_NA with T1
	 * greater than T2 and both T1 and T2 are greater than 0, the client
	 * discards the IA_NA option and processes the remainder of the message
	 * as though the server had not included the invalid IA_NA option.
	 */
	if (ia_na->t1 != 0 && ia_na->t2 != 0 && ia_na->t1 > ia_na->t2) {
		return -ENOENT;
	}

	NET_DBG("iaid %d t1 %d t2 %d", ia_na->iaid, ia_na->t1, ia_na->t2);

	/* In case there's no IAADDR option, make this visible be setting
	 * error status. If the option is present, option parser will overwrite
	 * the value.
	 */
	ia_na->iaaddr.status = DHCPV6_STATUS_NO_ADDR_AVAIL;
	ia_na->status = DHCPV6_STATUS_SUCCESS;

	length -= DHCPV6_OPTION_IA_NA_HEADER_SIZE;
	while (length > 0) {
		uint16_t code, sublen;

		ret = net_pkt_read_be16(pkt, &code);
		if (ret < 0) {
			return ret;
		}

		ret = net_pkt_read_be16(pkt, &sublen);
		if (ret < 0) {
			return ret;
		}

		switch (code) {
		case DHCPV6_OPTION_CODE_IAADDR:
			ret = dhcpv6_parse_option_iaaddr(pkt, sublen,
							 &ia_na->iaaddr);
			if (ret < 0) {
				return ret;
			}

			break;

		case DHCPV6_OPTION_CODE_STATUS_CODE:
			ret = dhcpv6_parse_option_status_code(pkt, sublen,
							      &ia_na->status);
			if (ret < 0) {
				return ret;
			}

			break;

		default:
			NET_DBG("Unexpected option %d length %d", code, sublen);

			ret = net_pkt_skip(pkt, sublen);
			if (ret < 0) {
				return ret;
			}

			break;
		}

		length -= (sublen + 4);
	}

	return 0;
}

static int dhcpv6_parse_option_iaprefix(struct net_pkt *pkt, uint16_t length,
					struct dhcpv6_iaprefix *iaprefix)
{
	int ret;

	if (length < DHCPV6_OPTION_IAPREFIX_HEADER_SIZE) {
		NET_ERR("Invalid IAPREFIX option size");
		return -EMSGSIZE;
	}

	ret = net_pkt_read_be32(pkt, &iaprefix->preferred_lifetime);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read_be32(pkt, &iaprefix->valid_lifetime);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read_u8(pkt, &iaprefix->prefix_len);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read(pkt, &iaprefix->prefix, sizeof(iaprefix->prefix));
	if (ret < 0) {
		return ret;
	}

	/* DHCPv6 RFC8415, ch. 21.22 The client MUST discard any prefixes for
	 * which the preferred lifetime is greater than the valid lifetime.
	 */
	if (iaprefix->preferred_lifetime > iaprefix->valid_lifetime) {
		return -EBADMSG;
	}

	NET_DBG("prefix %s/%u preferred_lifetime %d valid_lifetime %d",
		net_sprint_ipv6_addr(&iaprefix->prefix), iaprefix->prefix_len,
		iaprefix->preferred_lifetime, iaprefix->valid_lifetime);

	iaprefix->status = DHCPV6_STATUS_SUCCESS;

	length -= DHCPV6_OPTION_IAPREFIX_HEADER_SIZE;
	while (length > 0) {
		uint16_t code, sublen;

		ret = net_pkt_read_be16(pkt, &code);
		if (ret < 0) {
			return ret;
		}

		ret = net_pkt_read_be16(pkt, &sublen);
		if (ret < 0) {
			return ret;
		}

		switch (code) {
		case DHCPV6_OPTION_CODE_STATUS_CODE:
			ret = dhcpv6_parse_option_status_code(pkt, sublen,
							      &iaprefix->status);
			if (ret < 0) {
				return ret;
			}

			break;
		default:
			NET_DBG("Unexpected option %d length %d", code, sublen);

			ret = net_pkt_skip(pkt, sublen);
			if (ret < 0) {
				return ret;
			}

			break;
		}

		length -= (sublen + 4);
	}

	return 0;
}

static int dhcpv6_parse_option_ia_pd(struct net_pkt *pkt, uint16_t length,
				     struct dhcpv6_ia_pd *ia_pd)
{
	int ret;

	if (length < DHCPV6_OPTION_IA_PD_HEADER_SIZE) {
		NET_ERR("Invalid IA_PD option size");
		return -EMSGSIZE;
	}

	ret = net_pkt_read_be32(pkt, &ia_pd->iaid);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read_be32(pkt, &ia_pd->t1);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_read_be32(pkt, &ia_pd->t2);
	if (ret < 0) {
		return ret;
	}

	/* DHCPv6 RFC8415, ch. 21.21 If a client receives an IA_PD with T1
	 * greater than T2 and both T1 and T2 are greater than 0, the client
	 * discards the IA_PD option and processes the remainder of the message
	 * as though the server had not included the IA_PD option.
	 */
	if (ia_pd->t1 != 0 && ia_pd->t2 != 0 && ia_pd->t1 > ia_pd->t2) {
		return -ENOENT;
	}

	NET_DBG("iaid %d t1 %d t2 %d", ia_pd->iaid, ia_pd->t1, ia_pd->t2);

	/* In case there's no IAPREFIX option, make this visible be setting
	 * error status. If the option is present, option parser will overwrite
	 * the value.
	 */
	ia_pd->iaprefix.status = DHCPV6_STATUS_NO_PREFIX_AVAIL;
	ia_pd->status = DHCPV6_STATUS_SUCCESS;

	length -= DHCPV6_OPTION_IA_PD_HEADER_SIZE;
	while (length > 0) {
		uint16_t code, sublen;

		ret = net_pkt_read_be16(pkt, &code);
		if (ret < 0) {
			return ret;
		}

		ret = net_pkt_read_be16(pkt, &sublen);
		if (ret < 0) {
			return ret;
		}

		switch (code) {
		case DHCPV6_OPTION_CODE_IAPREFIX:
			ret = dhcpv6_parse_option_iaprefix(pkt, sublen,
							   &ia_pd->iaprefix);
			if (ret < 0) {
				return ret;
			}

			break;

		case DHCPV6_OPTION_CODE_STATUS_CODE:
			ret = dhcpv6_parse_option_status_code(pkt, sublen,
							      &ia_pd->status);
			if (ret < 0) {
				return ret;
			}

			break;
		default:
			NET_DBG("Unexpected option %d length %d", code, sublen);

			ret = net_pkt_skip(pkt, sublen);
			if (ret < 0) {
				return ret;
			}

			break;
		}

		length -= (sublen + 4);
	}

	return 0;
}

static int dhcpv6_find_option(struct net_pkt *pkt, enum dhcpv6_option_code opt_code,
			      uint16_t *opt_len)
{
	uint16_t length;
	uint16_t code;
	int ret;

	while (net_pkt_read_be16(pkt, &code) == 0) {
		if (net_pkt_read_be16(pkt, &length) < 0) {
			return -EBADMSG;
		}

		if (code == opt_code) {
			*opt_len = length;
			return 0;
		}

		ret = net_pkt_skip(pkt, length);
		if (ret < 0) {
			return ret;
		}
	}

	return -ENOENT;
}

static int dhcpv6_find_clientid(struct net_pkt *pkt,
				struct net_dhcpv6_duid_storage *clientid)
{
	struct net_pkt_cursor backup;
	uint16_t length;
	int ret;

	net_pkt_cursor_backup(pkt, &backup);

	ret = dhcpv6_find_option(pkt, DHCPV6_OPTION_CODE_CLIENTID, &length);
	if (ret == 0) {
		ret = dhcpv6_parse_option_clientid(pkt, length, clientid);
	}

	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

static int dhcpv6_find_serverid(struct net_pkt *pkt,
				struct net_dhcpv6_duid_storage *serverid)
{
	struct net_pkt_cursor backup;
	uint16_t length;
	int ret;

	net_pkt_cursor_backup(pkt, &backup);

	ret = dhcpv6_find_option(pkt, DHCPV6_OPTION_CODE_SERVERID, &length);
	if (ret == 0) {
		ret = dhcpv6_parse_option_serverid(pkt, length, serverid);
	}

	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

static int dhcpv6_find_server_preference(struct net_pkt *pkt,
					 uint8_t *preference)
{
	struct net_pkt_cursor backup;
	uint16_t length;
	int ret;

	net_pkt_cursor_backup(pkt, &backup);

	ret = dhcpv6_find_option(pkt, DHCPV6_OPTION_CODE_PREFERENCE, &length);
	if (ret == 0) {
		ret = dhcpv6_parse_option_preference(pkt, length, preference);
	} else if (ret == -ENOENT) {
		/* In case no preference option is present, default to 0.
		 * DHCPv6 RFC8415, ch. 18.2.1.
		 */
		*preference = 0;
		ret = 0;
	}

	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

static int dhcpv6_find_ia_na(struct net_pkt *pkt, struct dhcpv6_ia_na *ia_na)
{
	struct net_pkt_cursor backup;
	uint16_t length;
	int ret;

	net_pkt_cursor_backup(pkt, &backup);

	ret = dhcpv6_find_option(pkt, DHCPV6_OPTION_CODE_IA_NA, &length);
	if (ret == 0) {
		ret = dhcpv6_parse_option_ia_na(pkt, length, ia_na);
	}

	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

static int dhcpv6_find_ia_pd(struct net_pkt *pkt, struct dhcpv6_ia_pd *ia_pd)
{
	struct net_pkt_cursor backup;
	uint16_t length;
	int ret;

	net_pkt_cursor_backup(pkt, &backup);

	ret = dhcpv6_find_option(pkt, DHCPV6_OPTION_CODE_IA_PD, &length);
	if (ret == 0) {
		ret = dhcpv6_parse_option_ia_pd(pkt, length, ia_pd);
	}

	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

static int dhcpv6_find_status_code(struct net_pkt *pkt, uint16_t *status)
{
	struct net_pkt_cursor backup;
	uint16_t length;
	int ret;

	net_pkt_cursor_backup(pkt, &backup);

	ret = dhcpv6_find_option(pkt, DHCPV6_OPTION_CODE_STATUS_CODE, &length);
	if (ret == 0) {
		ret = dhcpv6_parse_option_status_code(pkt, length, status);
	} else if (ret == -ENOENT) {
		/* In case no status option is present, default to success.
		 * DHCPv6 RFC8415, ch. 21.13.
		 */
		*status = DHCPV6_STATUS_SUCCESS;
		ret = 0;
	}

	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

/* DHCPv6 state changes */

static void dhcpv6_enter_init(struct net_if *iface)
{
	uint32_t timeout;

	/* RFC8415 requires to wait a random period up to 1 second before
	 * sending the initial solicit/information request/confirm.
	 */
	timeout = sys_rand32_get() % DHCPV6_SOL_MAX_DELAY;

	dhcpv6_set_timeout(iface, timeout);
}

static void dhcpv6_enter_soliciting(struct net_if *iface)
{
	iface->config.dhcpv6.retransmit_timeout =
		dhcpv6_initial_retransmit_time(DHCPV6_SOL_TIMEOUT);
	iface->config.dhcpv6.retransmissions = 0;
	iface->config.dhcpv6.server_preference = -1;
	iface->config.dhcpv6.exchange_start = k_uptime_get();

	(void)dhcpv6_send_solicit(iface);
	dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);
}

static void dhcpv6_enter_requesting(struct net_if *iface)
{
	iface->config.dhcpv6.retransmit_timeout =
		dhcpv6_initial_retransmit_time(DHCPV6_REQ_TIMEOUT);
	iface->config.dhcpv6.retransmissions = 0;
	iface->config.dhcpv6.exchange_start = k_uptime_get();

	(void)dhcpv6_send_request(iface);
	dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);
}

static void dhcpv6_enter_renewing(struct net_if *iface)
{
	iface->config.dhcpv6.retransmit_timeout =
		dhcpv6_initial_retransmit_time(DHCPV6_REN_TIMEOUT);
	iface->config.dhcpv6.retransmissions = 0;
	iface->config.dhcpv6.exchange_start = k_uptime_get();

	(void)dhcpv6_send_renew(iface);
	dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);
}

static void dhcpv6_enter_rebinding(struct net_if *iface)
{
	iface->config.dhcpv6.retransmit_timeout =
		dhcpv6_initial_retransmit_time(DHCPV6_REB_TIMEOUT);
	iface->config.dhcpv6.retransmissions = 0;
	iface->config.dhcpv6.exchange_start = k_uptime_get();

	(void)dhcpv6_send_rebind(iface);
	dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);
}

static void dhcpv6_enter_confirming(struct net_if *iface)
{
	iface->config.dhcpv6.retransmit_timeout =
		dhcpv6_initial_retransmit_time(DHCPV6_CNF_TIMEOUT);
	iface->config.dhcpv6.retransmissions = 0;
	iface->config.dhcpv6.exchange_start = k_uptime_get();

	(void)dhcpv6_send_confirm(iface);
	dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);
}

static void dhcpv6_enter_bound(struct net_if *iface)
{
	iface->config.dhcpv6.timeout = iface->config.dhcpv6.t1;

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_DHCP_BOUND, iface,
					&iface->config.dhcpv6,
					sizeof(iface->config.dhcpv6));
}

static void dhcpv6_enter_state(struct net_if *iface, enum net_dhcpv6_state state)
{
	iface->config.dhcpv6.state = state;

	NET_DBG("enter state=%s",
		net_dhcpv6_state_name(iface->config.dhcpv6.state));

	switch (iface->config.dhcpv6.state) {
	case NET_DHCPV6_DISABLED:
		break;
	case NET_DHCPV6_INIT:
		return dhcpv6_enter_init(iface);
	case NET_DHCPV6_SOLICITING:
		return dhcpv6_enter_soliciting(iface);
	case NET_DHCPV6_REQUESTING:
		return dhcpv6_enter_requesting(iface);
	case NET_DHCPV6_CONFIRMING:
		return dhcpv6_enter_confirming(iface);
	case NET_DHCPV6_RENEWING:
		return dhcpv6_enter_renewing(iface);
	case NET_DHCPV6_REBINDING:
		return dhcpv6_enter_rebinding(iface);
	case NET_DHCPV6_INFO_REQUESTING:
		break;
	case NET_DHCPV6_BOUND:
		return dhcpv6_enter_bound(iface);
	}
}

/* DHCPv6 input processing */

static int dhcpv6_handle_advertise(struct net_if *iface, struct net_pkt *pkt,
				   uint8_t *tid)
{
	struct net_dhcpv6_duid_storage duid = { 0 };
	struct dhcpv6_ia_pd ia_pd = { 0 };
	struct dhcpv6_ia_na ia_na = { 0 };
	uint8_t server_preference = 0;
	uint16_t status = 0;
	int ret;

	if (iface->config.dhcpv6.state != NET_DHCPV6_SOLICITING) {
		return -EINVAL;
	}

	/* Verify client ID. */
	ret = dhcpv6_find_clientid(pkt, &duid);
	if (ret < 0) {
		NET_ERR("Client ID missing");
		return ret;
	}

	if (iface->config.dhcpv6.clientid.length != duid.length ||
	    memcmp(&iface->config.dhcpv6.clientid.duid, &duid.duid,
		   iface->config.dhcpv6.clientid.length) != 0) {
		NET_ERR("Client ID mismatch");
		return -EBADMSG;
	}

	/* Verify server ID is present. */
	memset(&duid, 0, sizeof(duid));
	ret = dhcpv6_find_serverid(pkt, &duid);
	if (ret < 0) {
		NET_ERR("Server ID missing");
		return ret;
	}

	/* Verify TID. */
	if (memcmp(iface->config.dhcpv6.tid, tid,
		   sizeof(iface->config.dhcpv6.tid)) != 0) {
		NET_INFO("TID mismatch");
		return -EBADMSG;
	}

	/* Verify status code. */
	ret = dhcpv6_find_status_code(pkt, &status);
	if (ret < 0) {
		return ret;
	}

	if (status != DHCPV6_STATUS_SUCCESS) {
		/* Ignore. */
		return 0;
	}

	/* TODO Process SOL_MAX_RT/INF_MAX_RT options. */

	/* Verify server preference. */
	ret = dhcpv6_find_server_preference(pkt, &server_preference);
	if (ret < 0) {
		return ret;
	}

	if ((int16_t)server_preference < iface->config.dhcpv6.server_preference) {
		/* Ignore. */
		return 0;
	}

	/* Find/verify address. */
	if (iface->config.dhcpv6.params.request_addr) {
		ret = dhcpv6_find_ia_na(pkt, &ia_na);
		if (ret < 0) {
			NET_ERR("Address missing");
			return ret;
		}

		if (ia_na.status != DHCPV6_STATUS_SUCCESS ||
		    ia_na.iaaddr.status != DHCPV6_STATUS_SUCCESS) {
			/* Ignore. */
			return 0;
		}
	}

	/* Find/verify prefix. */
	if (iface->config.dhcpv6.params.request_prefix) {
		ret = dhcpv6_find_ia_pd(pkt, &ia_pd);
		if (ret < 0) {
			NET_ERR("Prefix missing");
			return ret;
		}

		if (ia_pd.status != DHCPV6_STATUS_SUCCESS ||
		    ia_pd.iaprefix.status != DHCPV6_STATUS_SUCCESS) {
			/* Ignore. */
			return 0;
		}
	}

	/* Valid advertisement received, store received offer. */
	memcpy(&iface->config.dhcpv6.serverid, &duid,
	       sizeof(iface->config.dhcpv6.serverid));
	iface->config.dhcpv6.server_preference = server_preference;

	/* DHCPv6 RFC8415, ch. 18.2.1, if client received Advertise
	 * message with maximum preference, or after the first
	 * retransmission period, it should proceed with the exchange,
	 * w/o further wait.
	 */
	if (server_preference == DHCPV6_MAX_SERVER_PREFERENCE ||
	    iface->config.dhcpv6.retransmissions > 0) {
		/* Reschedule immediately */
		dhcpv6_enter_state(iface, NET_DHCPV6_REQUESTING);
		dhcpv6_reschedule();
	}

	return 0;
}

static int dhcpv6_handle_reply(struct net_if *iface, struct net_pkt *pkt,
			       uint8_t *tid)
{
	struct net_dhcpv6_duid_storage duid = { 0 };
	struct dhcpv6_ia_pd ia_pd = { 0 };
	struct dhcpv6_ia_na ia_na = { 0 };
	int64_t now = k_uptime_get();
	uint16_t status = 0;
	bool rediscover = false;
	int ret;

	if (iface->config.dhcpv6.state != NET_DHCPV6_REQUESTING &&
	    iface->config.dhcpv6.state != NET_DHCPV6_CONFIRMING &&
	    iface->config.dhcpv6.state != NET_DHCPV6_RENEWING &&
	    iface->config.dhcpv6.state != NET_DHCPV6_REBINDING) {
		return -EINVAL;
	}

	/* Verify client ID. */
	ret = dhcpv6_find_clientid(pkt, &duid);
	if (ret < 0) {
		NET_ERR("Client ID missing");
		return ret;
	}

	if (iface->config.dhcpv6.clientid.length != duid.length ||
	    memcmp(&iface->config.dhcpv6.clientid.duid, &duid.duid,
		   iface->config.dhcpv6.clientid.length) != 0) {
		NET_ERR("Client ID mismatch");
		return -EBADMSG;
	}

	/* Verify server ID is present. */
	memset(&duid, 0, sizeof(duid));
	ret = dhcpv6_find_serverid(pkt, &duid);
	if (ret < 0) {
		NET_ERR("Server ID missing");
		return ret;
	}

	/* Verify TID. */
	if (memcmp(iface->config.dhcpv6.tid, tid,
		   sizeof(iface->config.dhcpv6.tid)) != 0) {
		NET_INFO("TID mismatch");
		return -EBADMSG;
	}

	/* TODO Process SOL_MAX_RT/INF_MAX_RT options. */

	/* Verify status code. */
	ret = dhcpv6_find_status_code(pkt, &status);
	if (ret < 0) {
		return ret;
	}

	if (status == DHCPV6_STATUS_UNSPEC_FAIL) {
		/* Ignore and try again later. */
		return 0;
	}

	/* DHCPv6 RFC8415, ch. 18.2.10.1.  If the client receives a NotOnLink
	 * status from the server in response to (...) Request, the client can
	 * either reissue the message without specifying any addresses or
	 * restart the DHCP server discovery process.
	 *
	 * Restart discovery for our case.
	 */
	if (iface->config.dhcpv6.state == NET_DHCPV6_REQUESTING &&
	    status == DHCPV6_STATUS_NOT_ON_LINK) {
		rediscover = true;
		goto out;
	}

	/* In case of Confirm Reply, status success indicates the client can
	 * still use the address.
	 */
	if (iface->config.dhcpv6.state == NET_DHCPV6_CONFIRMING) {
		if (status != DHCPV6_STATUS_SUCCESS) {
			rediscover = true;
		}

		goto out;
	}

	/* Find/verify address. */
	if (iface->config.dhcpv6.params.request_addr) {
		ret = dhcpv6_find_ia_na(pkt, &ia_na);
		if (ret < 0) {
			NET_ERR("Address missing");
			return ret;
		}

		if (iface->config.dhcpv6.addr_iaid != ia_na.iaid) {
			return -EBADMSG;
		}
	}

	/* Find/verify prefix. */
	if (iface->config.dhcpv6.params.request_prefix) {
		ret = dhcpv6_find_ia_pd(pkt, &ia_pd);
		if (ret < 0) {
			NET_ERR("Prefix missing");
			return ret;
		}

		if (iface->config.dhcpv6.prefix_iaid != ia_pd.iaid) {
			return -EBADMSG;
		}
	}

	/* Valid response received, store received data. */
	iface->config.dhcpv6.t1 = UINT64_MAX;
	iface->config.dhcpv6.t2 = UINT64_MAX;
	iface->config.dhcpv6.expire = now;

	if (iface->config.dhcpv6.params.request_addr) {
		struct net_if_addr *ifaddr;

		if (ia_na.status == DHCPV6_STATUS_NO_ADDR_AVAIL ||
		    ia_na.iaaddr.status == DHCPV6_STATUS_NO_ADDR_AVAIL ||
		    ia_na.iaaddr.valid_lifetime == 0) {
			/* Remove old lease. */
			net_if_ipv6_addr_rm(iface, &iface->config.dhcpv6.addr);
			memset(&iface->config.dhcpv6.addr, 0, sizeof(struct in6_addr));
			rediscover = true;
			goto prefix;
		}

		/* TODO On nobiding (renew/rebind) go to requesting */

		if (!net_ipv6_addr_cmp(&iface->config.dhcpv6.addr,
				       net_ipv6_unspecified_address()) &&
		    !net_ipv6_addr_cmp(&iface->config.dhcpv6.addr,
				       &ia_na.iaaddr.addr)) {
			/* Remove old lease. */
			net_if_ipv6_addr_rm(iface, &iface->config.dhcpv6.addr);
		}

		memcpy(&iface->config.dhcpv6.addr, &ia_na.iaaddr.addr,
		       sizeof(iface->config.dhcpv6.addr));

		dhcvp6_update_deadlines(iface, now, ia_na.t1, ia_na.t2,
					ia_na.iaaddr.preferred_lifetime,
					ia_na.iaaddr.valid_lifetime);

		ifaddr = net_if_ipv6_addr_lookup_by_iface(iface, &ia_na.iaaddr.addr);
		if (ifaddr != NULL) {
			net_if_ipv6_addr_update_lifetime(
				ifaddr, ia_na.iaaddr.valid_lifetime);
		} else if (net_if_ipv6_addr_add(iface, &ia_na.iaaddr.addr, NET_ADDR_DHCP,
						ia_na.iaaddr.valid_lifetime) == NULL) {
			NET_ERR("Failed to configure DHCPv6 address");
			net_dhcpv6_stop(iface);
			return -EFAULT;
		}
	}

prefix:
	if (iface->config.dhcpv6.params.request_prefix) {
		struct net_if_ipv6_prefix *ifprefix;

		if (ia_pd.status == DHCPV6_STATUS_NO_PREFIX_AVAIL ||
		    ia_pd.iaprefix.status == DHCPV6_STATUS_NO_PREFIX_AVAIL ||
		    ia_pd.iaprefix.valid_lifetime == 0) {
			/* Remove old lease. */
			net_if_ipv6_prefix_rm(iface, &iface->config.dhcpv6.prefix,
					      iface->config.dhcpv6.prefix_len);
			memset(&iface->config.dhcpv6.prefix, 0, sizeof(struct in6_addr));
			iface->config.dhcpv6.prefix_len = 0;
			rediscover = true;
			goto out;
		}

		if (!net_ipv6_addr_cmp(&iface->config.dhcpv6.prefix,
				       net_ipv6_unspecified_address()) &&
		    (!net_ipv6_addr_cmp(&iface->config.dhcpv6.prefix,
					&ia_pd.iaprefix.prefix) ||
		     iface->config.dhcpv6.prefix_len != ia_pd.iaprefix.prefix_len)) {
			/* Remove old lease. */
			net_if_ipv6_prefix_rm(iface, &iface->config.dhcpv6.prefix,
					      iface->config.dhcpv6.prefix_len);
		}

		iface->config.dhcpv6.prefix_len = ia_pd.iaprefix.prefix_len;

		memcpy(&iface->config.dhcpv6.prefix, &ia_pd.iaprefix.prefix,
		       sizeof(iface->config.dhcpv6.prefix));

		dhcvp6_update_deadlines(iface, now, ia_pd.t1, ia_pd.t2,
					ia_pd.iaprefix.preferred_lifetime,
					ia_pd.iaprefix.valid_lifetime);

		ifprefix = net_if_ipv6_prefix_lookup(iface, &ia_pd.iaprefix.prefix,
						     ia_pd.iaprefix.prefix_len);
		if (ifprefix != NULL) {
			net_if_ipv6_prefix_set_timer(ifprefix, ia_pd.iaprefix.valid_lifetime);
		} else if (net_if_ipv6_prefix_add(iface, &ia_pd.iaprefix.prefix,
						  ia_pd.iaprefix.prefix_len,
						  ia_pd.iaprefix.valid_lifetime) == NULL) {
			NET_ERR("Failed to configure DHCPv6 prefix");
			net_dhcpv6_stop(iface);
			return -EFAULT;
		}
	}

out:
	if (rediscover) {
		dhcpv6_enter_state(iface, NET_DHCPV6_SOLICITING);
	} else {
		dhcpv6_enter_state(iface, NET_DHCPV6_BOUND);
	}

	dhcpv6_reschedule();

	return 0;
}

static int dhcpv6_handle_reconfigure(struct net_if *iface, struct net_pkt *pkt)
{
	/* Reconfigure not supported yet. */
	return -ENOTSUP;
}

static enum net_verdict dhcpv6_input(struct net_conn *conn,
				     struct net_pkt *pkt,
				     union net_ip_header *ip_hdr,
				     union net_proto_header *proto_hdr,
				     void *user_data)
{
	struct net_if *iface;
	uint8_t msg_type;
	uint8_t tid[DHCPV6_TID_SIZE];
	int ret;

	if (!conn) {
		NET_ERR("Invalid connection");
		return NET_DROP;
	}

	if (!pkt) {
		NET_ERR("Invalid packet");
		return NET_DROP;
	}

	iface = net_pkt_iface(pkt);
	if (!iface) {
		NET_ERR("No interface");
		return NET_DROP;
	}

	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, NET_IPV6UDPH_LEN)) {
		NET_ERR("Missing IPv6/UDP header");
		return NET_DROP;
	}

	if (net_pkt_read_u8(pkt, &msg_type) < 0) {
		NET_ERR("Missing message type");
		return NET_DROP;
	}

	if (net_pkt_read(pkt, tid, sizeof(tid)) < 0) {
		NET_ERR("Missing transaction ID");
		return NET_DROP;
	}

	NET_DBG("Received DHCPv6 packet [type=%d, tid=0x%02x%02x%02x]",
		msg_type, tid[0], tid[1], tid[2]);

	switch (msg_type) {
	case DHCPV6_MSG_TYPE_ADVERTISE:
		ret = dhcpv6_handle_advertise(iface, pkt, tid);
		break;
	case DHCPV6_MSG_TYPE_REPLY:
		ret = dhcpv6_handle_reply(iface, pkt, tid);
		break;
	case DHCPV6_MSG_TYPE_RECONFIGURE:
		ret = dhcpv6_handle_reconfigure(iface, pkt);
		break;
	case DHCPV6_MSG_TYPE_SOLICIT:
	case DHCPV6_MSG_TYPE_REQUEST:
	case DHCPV6_MSG_TYPE_CONFIRM:
	case DHCPV6_MSG_TYPE_RENEW:
	case DHCPV6_MSG_TYPE_REBIND:
	case DHCPV6_MSG_TYPE_RELEASE:
	case DHCPV6_MSG_TYPE_DECLINE:
	case DHCPV6_MSG_TYPE_INFORMATION_REQUEST:
	case DHCPV6_MSG_TYPE_RELAY_FORW:
	case DHCPV6_MSG_TYPE_RELAY_REPL:
	default:
		goto drop;
	}

	if (ret < 0) {
		goto drop;
	}

	net_pkt_unref(pkt);

	return NET_OK;

drop:
	return NET_DROP;
}

/* DHCPv6 timer management */

static uint64_t dhcpv6_timeleft(struct net_if *iface, int64_t now)
{
	uint64_t timeout = iface->config.dhcpv6.timeout;

	if (timeout > now) {
		return timeout - now;
	}

	return 0;
}

static uint64_t dhcpv6_manage_timers(struct net_if *iface, int64_t now)
{
	uint64_t timeleft = dhcpv6_timeleft(iface, now);

	NET_DBG("iface %p state=%s timeleft=%llu", iface,
		net_dhcpv6_state_name(iface->config.dhcpv6.state), timeleft);

	if (timeleft != 0U) {
		return iface->config.dhcpv6.timeout;
	}

	if (!net_if_is_up(iface)) {
		/* An interface is down, the registered event handler will
		 * restart DHCP procedure when the interface is back up.
		 */
		return UINT64_MAX;
	}

	switch (iface->config.dhcpv6.state) {
	case NET_DHCPV6_DISABLED:
		break;
	case NET_DHCPV6_INIT: {
		bool have_addr = false;
		bool have_prefix = false;

		if (iface->config.dhcpv6.params.request_addr &&
			!net_ipv6_addr_cmp(&iface->config.dhcpv6.addr,
					net_ipv6_unspecified_address())) {
			have_addr = true;
		}

		if (iface->config.dhcpv6.params.request_prefix &&
			!net_ipv6_addr_cmp(&iface->config.dhcpv6.prefix,
					net_ipv6_unspecified_address())) {
			have_prefix = true;
		}

		if ((have_addr || have_prefix) && now < iface->config.dhcpv6.expire) {
			/* Try to confirm the address/prefix. In case
			 * prefix is requested, Rebind is used with
			 * Confirm timings.
			 */
			iface->config.dhcpv6.expire = now + DHCPV6_CNF_MAX_RD;

			if (!iface->config.dhcpv6.params.request_prefix) {
				dhcpv6_enter_state(iface, NET_DHCPV6_CONFIRMING);
			} else {
				dhcpv6_enter_state(iface, NET_DHCPV6_REBINDING);
			}
		} else {
			dhcpv6_enter_state(iface, NET_DHCPV6_SOLICITING);
		}

		return iface->config.dhcpv6.timeout;
	}
	case NET_DHCPV6_SOLICITING:
		if (iface->config.dhcpv6.server_preference >= 0) {
			dhcpv6_enter_state(iface, NET_DHCPV6_REQUESTING);
			return iface->config.dhcpv6.timeout;
		}

		iface->config.dhcpv6.retransmissions++;
		iface->config.dhcpv6.retransmit_timeout =
			dhcpv6_next_retransmit_time(
				iface->config.dhcpv6.retransmit_timeout,
				DHCPV6_SOL_MAX_RT);

		(void)dhcpv6_send_solicit(iface);
		dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);

		return iface->config.dhcpv6.timeout;
	case NET_DHCPV6_REQUESTING:
		if (iface->config.dhcpv6.retransmissions >= DHCPV6_REQ_MAX_RC) {
			/* Back to soliciting. */
			dhcpv6_enter_state(iface, NET_DHCPV6_SOLICITING);
			return iface->config.dhcpv6.timeout;
		}

		iface->config.dhcpv6.retransmissions++;
		iface->config.dhcpv6.retransmit_timeout =
			dhcpv6_next_retransmit_time(
				iface->config.dhcpv6.retransmit_timeout,
				DHCPV6_REQ_MAX_RT);

		(void)dhcpv6_send_request(iface);
		dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);

		return iface->config.dhcpv6.timeout;
	case NET_DHCPV6_CONFIRMING:
		if (now >= iface->config.dhcpv6.expire) {
			dhcpv6_enter_state(iface, NET_DHCPV6_SOLICITING);
			return iface->config.dhcpv6.timeout;
		}

		iface->config.dhcpv6.retransmissions++;
		iface->config.dhcpv6.retransmit_timeout =
			dhcpv6_next_retransmit_time(
				iface->config.dhcpv6.retransmit_timeout,
				DHCPV6_CNF_MAX_RT);

		(void)dhcpv6_send_confirm(iface);
		dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);

		if (iface->config.dhcpv6.timeout > iface->config.dhcpv6.expire) {
			iface->config.dhcpv6.timeout = iface->config.dhcpv6.expire;
		}

		return iface->config.dhcpv6.timeout;
	case NET_DHCPV6_RENEWING:
		if (now >= iface->config.dhcpv6.t2) {
			dhcpv6_enter_state(iface, NET_DHCPV6_REBINDING);
			return iface->config.dhcpv6.timeout;
		}

		iface->config.dhcpv6.retransmissions++;
		iface->config.dhcpv6.retransmit_timeout =
			dhcpv6_next_retransmit_time(
				iface->config.dhcpv6.retransmit_timeout,
				DHCPV6_REN_MAX_RT);

		(void)dhcpv6_send_renew(iface);
		dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);

		if (iface->config.dhcpv6.timeout > iface->config.dhcpv6.t2) {
			iface->config.dhcpv6.timeout = iface->config.dhcpv6.t2;
		}

		return iface->config.dhcpv6.timeout;
	case NET_DHCPV6_REBINDING:
		if (now >= iface->config.dhcpv6.expire) {
			dhcpv6_enter_state(iface, NET_DHCPV6_SOLICITING);
			return iface->config.dhcpv6.timeout;
		}

		iface->config.dhcpv6.retransmissions++;
		iface->config.dhcpv6.retransmit_timeout =
			dhcpv6_next_retransmit_time(
				iface->config.dhcpv6.retransmit_timeout,
				DHCPV6_REB_MAX_RT);

		(void)dhcpv6_send_rebind(iface);
		dhcpv6_set_timeout(iface, iface->config.dhcpv6.retransmit_timeout);

		if (iface->config.dhcpv6.timeout > iface->config.dhcpv6.expire) {
			iface->config.dhcpv6.timeout = iface->config.dhcpv6.expire;
		}

		return iface->config.dhcpv6.timeout;
	case NET_DHCPV6_INFO_REQUESTING:
		break;
	case NET_DHCPV6_BOUND:
		dhcpv6_enter_state(iface, NET_DHCPV6_RENEWING);
		return iface->config.dhcpv6.timeout;
	}

	return UINT64_MAX;
}

static void dhcpv6_timeout(struct k_work *work)
{
	uint64_t timeout_update = UINT64_MAX;
	int64_t now = k_uptime_get();
	struct net_if_dhcpv6 *current, *next;

	ARG_UNUSED(work);

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dhcpv6_ifaces, current, next, node) {
		struct net_if *iface = CONTAINER_OF(
			CONTAINER_OF(current, struct net_if_config, dhcpv6),
			struct net_if, config);
		uint64_t next_timeout;

		next_timeout = dhcpv6_manage_timers(iface, now);
		if (next_timeout < timeout_update) {
			timeout_update = next_timeout;
		}
	}

	k_mutex_unlock(&lock);

	if (timeout_update != UINT64_MAX) {
		if (now > timeout_update) {
			timeout_update = 0ULL;
		} else {
			timeout_update -= now;
		}

		NET_DBG("Waiting for %llums", timeout_update);
		k_work_reschedule(&dhcpv6_timeout_work, K_MSEC(timeout_update));
	}
}

static void dhcpv6_iface_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t mgmt_event, struct net_if *iface)
{
	sys_snode_t *node = NULL;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE(&dhcpv6_ifaces, node) {
		if (node == &iface->config.dhcpv6.node) {
			break;
		}
	}

	if (node == NULL) {
		goto out;
	}

	if (mgmt_event == NET_EVENT_IF_DOWN) {
		NET_DBG("Interface %p going down", iface);
		dhcpv6_set_timeout(iface, UINT64_MAX);
	} else if (mgmt_event == NET_EVENT_IF_UP) {
		NET_DBG("Interface %p coming up", iface);
		dhcpv6_enter_state(iface, NET_DHCPV6_INIT);
	}

	dhcpv6_reschedule();

out:
	k_mutex_unlock(&lock);
}

static void dhcpv6_generate_client_duid(struct net_if *iface)
{
	struct net_linkaddr *lladdr = net_if_get_link_addr(iface);
	struct net_dhcpv6_duid_storage *clientid = &iface->config.dhcpv6.clientid;
	struct dhcpv6_duid_ll *duid_ll =
				(struct dhcpv6_duid_ll *)&clientid->duid.buf;

	memset(clientid, 0, sizeof(*clientid));

	UNALIGNED_PUT(htons(DHCPV6_DUID_TYPE_LL), &clientid->duid.type);
	UNALIGNED_PUT(htons(DHCPV6_HARDWARE_ETHERNET_TYPE), &duid_ll->hw_type);
	memcpy(duid_ll->ll_addr, lladdr->addr, lladdr->len);

	clientid->length = DHCPV6_DUID_LL_HEADER_SIZE + lladdr->len;
}

/* DHCPv6 public API */

void net_dhcpv6_start(struct net_if *iface, struct net_dhcpv6_params *params)
{
	k_mutex_lock(&lock, K_FOREVER);

	if (iface->config.dhcpv6.state != NET_DHCPV6_DISABLED) {
		NET_ERR("DHCPv6 already running on iface %p, state %s", iface,
			net_dhcpv6_state_name(iface->config.dhcpv6.state));
		goto out;
	}

	if (!params->request_addr && !params->request_prefix) {
		NET_ERR("Information Request not supported yet");
		goto out;
	}

	net_mgmt_event_notify(NET_EVENT_IPV6_DHCP_START, iface);

	NET_DBG("Starting DHCPv6 on iface %p", iface);

	iface->config.dhcpv6.params = *params;

	if (sys_slist_is_empty(&dhcpv6_ifaces)) {
		net_mgmt_add_event_callback(&dhcpv6_mgmt_cb);
	}

	sys_slist_append(&dhcpv6_ifaces, &iface->config.dhcpv6.node);

	if (params->request_addr) {
		iface->config.dhcpv6.addr_iaid = net_if_get_by_iface(iface);
	}

	if (params->request_prefix) {
		iface->config.dhcpv6.prefix_iaid = net_if_get_by_iface(iface);
	}

	dhcpv6_generate_client_duid(iface);
	dhcpv6_enter_state(iface, NET_DHCPV6_INIT);
	dhcpv6_reschedule();

out:
	k_mutex_unlock(&lock);
}

void net_dhcpv6_stop(struct net_if *iface)
{
	k_mutex_lock(&lock, K_FOREVER);

	switch (iface->config.dhcpv6.state) {
	case NET_DHCPV6_DISABLED:
		NET_INFO("DHCPv6 already disabled on iface %p", iface);
		break;

	case NET_DHCPV6_INIT:
	case NET_DHCPV6_SOLICITING:
	case NET_DHCPV6_REQUESTING:
	case NET_DHCPV6_CONFIRMING:
	case NET_DHCPV6_RENEWING:
	case NET_DHCPV6_REBINDING:
	case NET_DHCPV6_INFO_REQUESTING:
	case NET_DHCPV6_BOUND:
		NET_DBG("Stopping DHCPv6 on iface %p, state %s", iface,
			net_dhcpv6_state_name(iface->config.dhcpv6.state));

		(void)dhcpv6_enter_state(iface, NET_DHCPV6_DISABLED);

		sys_slist_find_and_remove(&dhcpv6_ifaces,
					  &iface->config.dhcpv6.node);

		if (sys_slist_is_empty(&dhcpv6_ifaces)) {
			(void)k_work_cancel_delayable(&dhcpv6_timeout_work);
			net_mgmt_del_event_callback(&dhcpv6_mgmt_cb);
		}

		break;
	}

	net_mgmt_event_notify(NET_EVENT_IPV6_DHCP_STOP, iface);

	k_mutex_unlock(&lock);
}

void net_dhcpv6_restart(struct net_if *iface)
{
	struct net_dhcpv6_params params = iface->config.dhcpv6.params;

	net_dhcpv6_stop(iface);
	net_dhcpv6_start(iface, &params);
}

int net_dhcpv6_init(void)
{
	struct sockaddr unspec_addr;
	int ret;

	net_ipaddr_copy(&net_sin6(&unspec_addr)->sin6_addr,
			net_ipv6_unspecified_address());
	unspec_addr.sa_family = AF_INET6;

	ret = net_udp_register(AF_INET6, NULL, &unspec_addr,
			       DHCPV6_SERVER_PORT, DHCPV6_CLIENT_PORT,
			       NULL, dhcpv6_input, NULL, NULL);
	if (ret < 0) {
		NET_DBG("UDP callback registration failed");
		return ret;
	}

	k_work_init_delayable(&dhcpv6_timeout_work, dhcpv6_timeout);
	net_mgmt_init_event_callback(&dhcpv6_mgmt_cb, dhcpv6_iface_event_handler,
				     NET_EVENT_IF_DOWN | NET_EVENT_IF_UP);

	return 0;
}
