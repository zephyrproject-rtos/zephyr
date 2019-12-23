/** @file
 * @brief DHCPv4 client related functions
 */

/*
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2018 Vincent van der Locht
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_dhcpv4, CONFIG_NET_DHCPV4_LOG_LEVEL);

#include <errno.h>
#include <inttypes.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include "net_private.h"

#include <net/udp.h>
#include "udp_internal.h"
#include <net/dhcpv4.h>
#include <net/dns_resolve.h>

#include "dhcpv4.h"
#include "ipv4.h"

#define PKT_WAIT_TIME K_SECONDS(1)

static sys_slist_t dhcpv4_ifaces;
static struct k_delayed_work timeout_work;

static struct net_mgmt_event_callback mgmt4_cb;

/* RFC 1497 [17] */
static const u8_t magic_cookie[4] = { 0x63, 0x82, 0x53, 0x63 };

static const char *dhcpv4_msg_type_name(enum dhcpv4_msg_type msg_type)
	__attribute__((unused));

static const char *dhcpv4_msg_type_name(enum dhcpv4_msg_type msg_type)
{
	static const char * const name[] = {
		"discover",
		"offer",
		"request",
		"decline",
		"ack",
		"nak",
		"release",
		"inform"
	};

	__ASSERT_NO_MSG(msg_type >= 1 && msg_type <= sizeof(name));
	return name[msg_type - 1];
}

/* Add magic cookie to DCHPv4 messages */
static inline bool dhcpv4_add_cookie(struct net_pkt *pkt)
{
	if (net_pkt_write(pkt, (void *)magic_cookie,
			  ARRAY_SIZE(magic_cookie))) {
		return false;
	}

	return true;
}

/* Add a an option with the form OPTION LENGTH VALUE.  */
static bool dhcpv4_add_option_length_value(struct net_pkt *pkt, u8_t option,
					   u8_t size, const void *value)
{
	if (net_pkt_write_u8(pkt, option) ||
	    net_pkt_write_u8(pkt, size) ||
	    net_pkt_write(pkt, value, size)) {
		return false;
	}

	return true;
}

/* Add DHCPv4 message type */
static bool dhcpv4_add_msg_type(struct net_pkt *pkt, u8_t type)
{
	return dhcpv4_add_option_length_value(pkt, DHCPV4_OPTIONS_MSG_TYPE,
					      1, &type);
}

/* Add DHCPv4 minimum required options for server to reply.
 * Can be added more if needed.
 */
static bool dhcpv4_add_req_options(struct net_pkt *pkt)
{
	static u8_t data[3] = { DHCPV4_OPTIONS_SUBNET_MASK,
				DHCPV4_OPTIONS_ROUTER,
				DHCPV4_OPTIONS_DNS_SERVER };

	return dhcpv4_add_option_length_value(pkt, DHCPV4_OPTIONS_REQ_LIST,
					      ARRAY_SIZE(data), data);
}

static bool dhcpv4_add_server_id(struct net_pkt *pkt,
				 const struct in_addr *addr)
{
	return dhcpv4_add_option_length_value(pkt, DHCPV4_OPTIONS_SERVER_ID,
					      4, addr->s4_addr);
}

static bool dhcpv4_add_req_ipaddr(struct net_pkt *pkt,
				  const struct in_addr *addr)
{
	return dhcpv4_add_option_length_value(pkt, DHCPV4_OPTIONS_REQ_IPADDR,
					      4, addr->s4_addr);
}

/* Add DHCPv4 Options end, rest of the message can be padded wit zeros */
static inline bool dhcpv4_add_end(struct net_pkt *pkt)
{
	if (net_pkt_write_u8(pkt, DHCPV4_OPTIONS_END)) {
		return false;
	}

	return true;
}

/* File is empty ATM */
static inline bool dhcpv4_add_file(struct net_pkt *pkt)
{
	if (net_pkt_memset(pkt, 0, SIZE_OF_FILE)) {
		return false;
	}

	return true;
}

/* SNAME is empty ATM */
static inline bool dhcpv4_add_sname(struct net_pkt *pkt)
{
	if (net_pkt_memset(pkt, 0, SIZE_OF_SNAME)) {
		return false;
	}

	return true;
}

/* Create DHCPv4 message and add options as per message type */
static struct net_pkt *dhcpv4_create_message(struct net_if *iface, u8_t type,
					     const struct in_addr *ciaddr,
					     const struct in_addr *src_addr,
					     const struct in_addr *server_addr,
					     bool server_id, bool requested_ip)
{
	NET_PKT_DATA_ACCESS_DEFINE(dhcp_access, struct dhcp_msg);
	const struct in_addr *addr;
	size_t size = DHCPV4_MESSAGE_SIZE;
	struct net_pkt *pkt;
	struct dhcp_msg *msg;

	if (src_addr == NULL) {
		addr = net_ipv4_unspecified_address();
	} else {
		addr = src_addr;
	}

	if (server_id) {
		size += DHCPV4_OLV_MSG_SERVER_ID;
	}

	if (requested_ip) {
		size +=  DHCPV4_OLV_MSG_REQ_IPADDR;
	}

	if (type == DHCPV4_MSG_TYPE_DISCOVER) {
		size +=  DHCPV4_OLV_MSG_REQ_LIST;
	}

	pkt = net_pkt_alloc_with_buffer(iface, size, AF_INET,
					IPPROTO_UDP, K_FOREVER);

	net_pkt_set_ipv4_ttl(pkt, 0xFF);

	if (net_ipv4_create(pkt, addr, server_addr) ||
	    net_udp_create(pkt, htons(DHCPV4_CLIENT_PORT),
			   htons(DHCPV4_SERVER_PORT))) {
		goto fail;
	}

	msg = (struct dhcp_msg *)net_pkt_get_data(pkt, &dhcp_access);

	(void)memset(msg, 0, sizeof(struct dhcp_msg));

	msg->op    = DHCPV4_MSG_BOOT_REQUEST;
	msg->htype = HARDWARE_ETHERNET_TYPE;
	msg->hlen  = HARDWARE_ETHERNET_LEN;
	msg->xid   = htonl(iface->config.dhcpv4.xid);
	msg->flags = htons(DHCPV4_MSG_BROADCAST);

	if (ciaddr) {
		/* The ciaddr field was zero'd out above, if we are
		 * asked to send a ciaddr then fill it in now
		 * otherwise leave it as all zeros.
		 */
		memcpy(msg->ciaddr, ciaddr, 4);
	}

	memcpy(msg->chaddr, net_if_get_link_addr(iface)->addr,
	       net_if_get_link_addr(iface)->len);

	if (net_pkt_set_data(pkt, &dhcp_access)) {
		goto fail;
	}

	if (!dhcpv4_add_sname(pkt) ||
	    !dhcpv4_add_file(pkt) ||
	    !dhcpv4_add_cookie(pkt) ||
	    !dhcpv4_add_msg_type(pkt, type)) {
		goto fail;
	}

	if ((server_id &&
	     !dhcpv4_add_server_id(pkt, &iface->config.dhcpv4.server_id)) ||
	    (requested_ip &&
	     !dhcpv4_add_req_ipaddr(pkt, &iface->config.dhcpv4.requested_ip))) {
		goto fail;
	}

	if (type == DHCPV4_MSG_TYPE_DISCOVER && !dhcpv4_add_req_options(pkt)) {
		goto fail;
	}

	if (!dhcpv4_add_end(pkt)) {
		goto fail;
	}

	net_pkt_cursor_init(pkt);

	net_ipv4_finalize(pkt, IPPROTO_UDP);

	return pkt;

fail:
	NET_DBG("Message creation failed");
	net_pkt_unref(pkt);

	return NULL;
}

/* Prepare DHCPv4 Message request and send it to peer */
static u32_t dhcpv4_send_request(struct net_if *iface)
{
	const struct in_addr *server_addr = net_ipv4_broadcast_address();
	const struct in_addr *ciaddr = NULL;
	const struct in_addr *src_addr = NULL;
	bool with_server_id = false;
	bool with_requested_ip = false;
	struct net_pkt *pkt;
	u32_t timeout;

	iface->config.dhcpv4.xid++;

	switch (iface->config.dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_BOUND:
		/* Not possible */
		NET_ASSERT_INFO(0, "Invalid state %s",
			net_dhcpv4_state_name(iface->config.dhcpv4.state));
		break;
	case NET_DHCPV4_REQUESTING:
		with_server_id = true;
		with_requested_ip = true;
		break;
	case NET_DHCPV4_RENEWING:
		/* Since we have an address populate the ciaddr field.
		 */
		ciaddr = &iface->config.dhcpv4.requested_ip;

		/* UNICAST the DHCPREQUEST */
		src_addr = ciaddr;
		server_addr = &iface->config.dhcpv4.server_id;

		/* RFC2131 4.4.5 Client MUST NOT include server
		 * identifier in the DHCPREQUEST.
		 */
		break;
	case NET_DHCPV4_REBINDING:
		/* Since we have an address populate the ciaddr field.
		 */
		ciaddr = &iface->config.dhcpv4.requested_ip;
		src_addr = ciaddr;

		break;
	}

	pkt = dhcpv4_create_message(iface, DHCPV4_MSG_TYPE_REQUEST,
				    ciaddr, src_addr, server_addr,
				    with_server_id, with_requested_ip);
	if (!pkt) {
		goto fail;
	}

	if (net_send_data(pkt) < 0) {
		goto fail;
	}

	timeout = DHCPV4_INITIAL_RETRY_TIMEOUT << iface->config.dhcpv4.attempts;

	iface->config.dhcpv4.attempts++;

	NET_DBG("send request dst=%s xid=0x%x ciaddr=%s%s%s timeout=%us",
		log_strdup(net_sprint_ipv4_addr(server_addr)),
		iface->config.dhcpv4.xid,
		ciaddr ?
		log_strdup(net_sprint_ipv4_addr(ciaddr)) : "<unknown>",
		with_server_id ? " +server-id" : "",
		with_requested_ip ? " +requested-ip" : "",
		timeout);

	iface->config.dhcpv4.timer_start = k_uptime_get();
	iface->config.dhcpv4.request_time = timeout;

	return timeout;

fail:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	return UINT32_MAX;
}

/* Prepare DHCPv4 Discover message and broadcast it */
static u32_t dhcpv4_send_discover(struct net_if *iface)
{
	struct net_pkt *pkt;
	u32_t timeout;

	iface->config.dhcpv4.xid++;

	pkt = dhcpv4_create_message(iface, DHCPV4_MSG_TYPE_DISCOVER,
				    NULL, NULL, net_ipv4_broadcast_address(),
				    false, false);
	if (!pkt) {
		goto fail;
	}

	if (net_send_data(pkt) < 0) {
		goto fail;
	}

	timeout = DHCPV4_INITIAL_RETRY_TIMEOUT << iface->config.dhcpv4.attempts;

	iface->config.dhcpv4.attempts++;

	NET_DBG("send discover xid=0x%x timeout=%us",
		iface->config.dhcpv4.xid, timeout);

	iface->config.dhcpv4.timer_start = k_uptime_get();
	iface->config.dhcpv4.request_time = timeout;

	return timeout;

fail:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	return iface->config.dhcpv4.xid %
			(CONFIG_NET_DHCPV4_INITIAL_DELAY_MAX -
			 DHCPV4_INITIAL_DELAY_MIN) +
			DHCPV4_INITIAL_DELAY_MIN;
}

static void dhcpv4_update_timeout_work(u32_t timeout)
{
	if (!k_delayed_work_remaining_get(&timeout_work) ||
	    K_SECONDS(timeout) <
	    k_delayed_work_remaining_get(&timeout_work)) {
		k_delayed_work_cancel(&timeout_work);
		k_delayed_work_submit(&timeout_work, K_SECONDS(timeout));
	}
}

static void dhcpv4_enter_selecting(struct net_if *iface)
{
	iface->config.dhcpv4.attempts = 0U;

	iface->config.dhcpv4.lease_time = 0U;
	iface->config.dhcpv4.renewal_time = 0U;
	iface->config.dhcpv4.rebinding_time = 0U;

	iface->config.dhcpv4.state = NET_DHCPV4_SELECTING;
	NET_DBG("enter state=%s",
		net_dhcpv4_state_name(iface->config.dhcpv4.state));
}

static bool dhcpv4_check_timeout(s64_t start, u32_t time, s64_t timeout)
{
	start += K_SECONDS(time);
	if (start < 0) {
		start = -start;
	}

	if (start > timeout) {
		return false;
	}

	return true;
}

static bool dhcpv4_request_timedout(struct net_if *iface, s64_t timeout)
{
	return dhcpv4_check_timeout(iface->config.dhcpv4.timer_start,
				    iface->config.dhcpv4.request_time,
				    timeout);
}

static bool dhcpv4_renewal_timedout(struct net_if *iface, s64_t timeout)
{
	if (!dhcpv4_check_timeout(iface->config.dhcpv4.timer_start,
				  iface->config.dhcpv4.renewal_time,
				  timeout)) {
		return false;
	}

	iface->config.dhcpv4.state = NET_DHCPV4_RENEWING;
	NET_DBG("enter state=%s",
		net_dhcpv4_state_name(iface->config.dhcpv4.state));
	iface->config.dhcpv4.attempts = 0U;

	return true;
}

static bool dhcpv4_rebinding_timedout(struct net_if *iface, s64_t timeout)
{
	if (!dhcpv4_check_timeout(iface->config.dhcpv4.timer_start,
				  iface->config.dhcpv4.rebinding_time,
				  timeout)) {
		return false;
	}

	iface->config.dhcpv4.state = NET_DHCPV4_REBINDING;
	NET_DBG("enter state=%s",
		net_dhcpv4_state_name(iface->config.dhcpv4.state));
	iface->config.dhcpv4.attempts = 0U;

	return true;
}

static void dhcpv4_enter_requesting(struct net_if *iface)
{
	iface->config.dhcpv4.attempts = 0U;
	iface->config.dhcpv4.state = NET_DHCPV4_REQUESTING;
	NET_DBG("enter state=%s",
		net_dhcpv4_state_name(iface->config.dhcpv4.state));

	dhcpv4_update_timeout_work(dhcpv4_send_request(iface));
}

static void dhcpv4_enter_bound(struct net_if *iface)
{
	u32_t renewal_time;
	u32_t rebinding_time;

	renewal_time = iface->config.dhcpv4.renewal_time;
	if (!renewal_time) {
		/* The default renewal time rfc2131 4.4.5 */
		renewal_time = iface->config.dhcpv4.lease_time / 2U;
		iface->config.dhcpv4.renewal_time = renewal_time;
	}

	rebinding_time = iface->config.dhcpv4.rebinding_time;
	if (!rebinding_time) {
		/* The default rebinding time rfc2131 4.4.5 */
		rebinding_time = iface->config.dhcpv4.lease_time * 875U / 1000;
		iface->config.dhcpv4.rebinding_time = rebinding_time;
	}

	iface->config.dhcpv4.state = NET_DHCPV4_BOUND;
	NET_DBG("enter state=%s renewal=%us rebinding=%us",
		net_dhcpv4_state_name(iface->config.dhcpv4.state),
		renewal_time, rebinding_time);

	iface->config.dhcpv4.timer_start = k_uptime_get();
	iface->config.dhcpv4.request_time = MIN(renewal_time, rebinding_time);

	dhcpv4_update_timeout_work(iface->config.dhcpv4.request_time);
}

static u32_t dhcph4_manage_timers(struct net_if *iface, s64_t timeout)
{
	NET_DBG("iface %p state=%s", iface,
		net_dhcpv4_state_name(iface->config.dhcpv4.state));

	if (!dhcpv4_request_timedout(iface, timeout)) {
		return iface->config.dhcpv4.request_time;
	}

	switch (iface->config.dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
		break;
	case NET_DHCPV4_INIT:
		dhcpv4_enter_selecting(iface);
		/* Fall through, as discover msg needs to be sent */
	case NET_DHCPV4_SELECTING:
		/* Failed to get OFFER message, send DISCOVER again */
		return dhcpv4_send_discover(iface);
	case NET_DHCPV4_REQUESTING:
		/* Maximum number of renewal attempts failed, so start
		 * from the beginning.
		 */
		if (iface->config.dhcpv4.attempts >=
					DHCPV4_MAX_NUMBER_OF_ATTEMPTS) {
			NET_DBG("too many attempts, restart");
			dhcpv4_enter_selecting(iface);
			return dhcpv4_send_discover(iface);
		}

		return dhcpv4_send_request(iface);
	case NET_DHCPV4_BOUND:
		if (dhcpv4_renewal_timedout(iface, timeout) ||
		    dhcpv4_rebinding_timedout(iface, timeout)) {
			return dhcpv4_send_request(iface);
		}

		return MIN(iface->config.dhcpv4.renewal_time,
			   iface->config.dhcpv4.rebinding_time);
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
		if (iface->config.dhcpv4.attempts >=
					DHCPV4_MAX_NUMBER_OF_ATTEMPTS) {
			NET_DBG("too many attempts, restart");

			if (!net_if_ipv4_addr_rm(iface,
					 &iface->config.dhcpv4.requested_ip)) {
				NET_DBG("Failed to remove addr from iface");
			}

			/* Maximum number of renewal attempts failed, so start
			 * from the beginning.
			 */
			dhcpv4_enter_selecting(iface);
			return dhcpv4_send_discover(iface);
		} else {
			return dhcpv4_send_request(iface);
		}
	}

	return UINT32_MAX;
}

static void dhcpv4_timeout(struct k_work *work)
{
	u32_t timeout_update = UINT32_MAX - 1;
	s64_t timeout = k_uptime_get();
	struct net_if_dhcpv4 *current, *next;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dhcpv4_ifaces, current, next, node) {
		struct net_if *iface = CONTAINER_OF(
			CONTAINER_OF(current, struct net_if_config, dhcpv4),
			struct net_if, config);
		u32_t next_timeout;

		next_timeout = dhcph4_manage_timers(iface, timeout);
		if (next_timeout < timeout_update) {
			timeout_update = next_timeout;
		}
	}

	if (timeout_update != UINT32_MAX) {
		NET_DBG("Waiting for %us", timeout_update);

		k_delayed_work_submit(&timeout_work,
				      K_SECONDS(timeout_update));
	}
}

/* Parse DHCPv4 options and retrieve relavant information
 * as per RFC 2132.
 */
static bool dhcpv4_parse_options(struct net_pkt *pkt,
				 struct net_if *iface,
				 enum dhcpv4_msg_type *msg_type)
{
	u8_t cookie[4];
	u8_t length;
	u8_t type;

	if (net_pkt_read(pkt, cookie, sizeof(cookie)) ||
	    memcmp(magic_cookie, cookie, sizeof(magic_cookie))) {
		NET_DBG("Incorrect magic cookie");
		return false;
	}

	while (!net_pkt_read_u8(pkt, &type)) {
		if (type == DHCPV4_OPTIONS_END) {
			NET_DBG("options_end");
			return true;
		}

		if (net_pkt_read_u8(pkt, &length)) {
			NET_ERR("option parsing, bad length");
			return false;
		}

		switch (type) {
		case DHCPV4_OPTIONS_SUBNET_MASK: {
			struct in_addr netmask;

			if (length != 4U) {
				NET_ERR("options_subnet_mask, bad length");
				return false;
			}

			if (net_pkt_read(pkt, netmask.s4_addr, length)) {
				NET_ERR("options_subnet_mask, short packet");
				return false;
			}

			net_if_ipv4_set_netmask(iface, &netmask);
			NET_DBG("options_subnet_mask %s",
				log_strdup(net_sprint_ipv4_addr(&netmask)));
			break;
		}
		case DHCPV4_OPTIONS_ROUTER: {
			struct in_addr router;

			/* Router option may present 1 or more
			 * addresses for routers on the clients
			 * subnet.  Routers should be listed in order
			 * of preference.  Hence we choose the first
			 * and skip the rest.
			 */
			if (length % 4 != 0U || length < 4) {
				NET_ERR("options_router, bad length");
				return false;
			}

			if (net_pkt_read(pkt, router.s4_addr, 4) ||
			    net_pkt_skip(pkt, length - 4U)) {
				NET_ERR("options_router, short packet");
				return false;
			}

			NET_DBG("options_router: %s",
				log_strdup(net_sprint_ipv4_addr(&router)));
			net_if_ipv4_set_gw(iface, &router);
			break;
		}
#if defined(CONFIG_DNS_RESOLVER)
		case DHCPV4_OPTIONS_DNS_SERVER: {
			int i;
			struct dns_resolve_context *ctx;
			struct sockaddr_in dns;
			const struct sockaddr *dns_servers[] = {
				(struct sockaddr *)&dns, NULL
			};
			int status;

			/* DNS server option may present 1 or more
			 * addresses. Each 4 bytes in length. DNS
			 * servers should be listed in order
			 * of preference.  Hence we choose the first
			 * and skip the rest.
			 */
			if (length % 4 != 0U) {
				NET_ERR("options_dns, bad length");
				return false;
			}

			(void)memset(&dns, 0, sizeof(dns));

			if (net_pkt_read(pkt, dns.sin_addr.s4_addr, 4) ||
			    net_pkt_skip(pkt, length - 4U)) {
				NET_ERR("options_dns, short packet");
				return false;
			}

			ctx = dns_resolve_get_default();
			for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
				if (!ctx->queries[i].cb) {
					continue;
				}

				dns_resolve_cancel(ctx, ctx->queries[i].id);
			}
			dns_resolve_close(ctx);

			dns.sin_family = AF_INET;
			status = dns_resolve_init(ctx, NULL, dns_servers);
			if (status < 0) {
				NET_DBG("options_dns, failed to set "
					"resolve address: %d", status);
				return false;
			}

			break;
		}
#endif
		case DHCPV4_OPTIONS_LEASE_TIME:
			if (length != 4U) {
				NET_ERR("options_lease_time, bad length");
				return false;
			}

			if (net_pkt_read_be32(
				    pkt, &iface->config.dhcpv4.lease_time) ||
			    !iface->config.dhcpv4.lease_time) {
				NET_ERR("options_lease_time, wrong value");
				return false;
			}

			NET_DBG("options_lease_time: %u",
				iface->config.dhcpv4.lease_time);

			break;
		case DHCPV4_OPTIONS_RENEWAL:
			if (length != 4U) {
				NET_DBG("options_renewal, bad length");
				return false;
			}

			if (net_pkt_read_be32(
				    pkt, &iface->config.dhcpv4.renewal_time) ||
			    !iface->config.dhcpv4.renewal_time) {
				NET_DBG("options_renewal, wrong value");
				return false;
			}

			NET_DBG("options_renewal: %u",
				iface->config.dhcpv4.renewal_time);

			break;
		case DHCPV4_OPTIONS_REBINDING:
			if (length != 4U) {
				NET_DBG("options_rebinding, bad length");
				return false;
			}

			if (net_pkt_read_be32(
				    pkt,
				    &iface->config.dhcpv4.rebinding_time) ||
			    !iface->config.dhcpv4.rebinding_time) {
				NET_DBG("options_rebinding, wrong value");
				return false;
			}

			NET_DBG("options_rebinding: %u",
				iface->config.dhcpv4.rebinding_time);

			break;
		case DHCPV4_OPTIONS_SERVER_ID:
			if (length != 4U) {
				NET_DBG("options_server_id, bad length");
				return false;
			}

			if (net_pkt_read(
				    pkt,
				    iface->config.dhcpv4.server_id.s4_addr,
				    length)) {
				NET_DBG("options_server_id, read err");
				return false;
			}

			NET_DBG("options_server_id: %s",
				log_strdup(net_sprint_ipv4_addr(
					   &iface->config.dhcpv4.server_id)));
			break;
		case DHCPV4_OPTIONS_MSG_TYPE: {
			if (length != 1U) {
				NET_DBG("options_msg_type, bad length");
				return false;
			}

			if (net_pkt_read_u8(pkt, (u8_t *)msg_type)) {
				NET_DBG("options_msg_type, read err");
				return false;
			}

			break;
		}
		default:
			NET_DBG("option unknown: %d", type);

			if (net_pkt_skip(pkt, length)) {
				NET_DBG("option unknown, skip err");
				return false;
			}

			break;
		}
	}

	/* Invalid case: Options without DHCPV4_OPTIONS_END. */
	return false;
}

static inline void dhcpv4_handle_msg_offer(struct net_if *iface)
{
	switch (iface->config.dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
	case NET_DHCPV4_BOUND:
		break;
	case NET_DHCPV4_SELECTING:
		dhcpv4_enter_requesting(iface);
		break;
	}
}

static void dhcpv4_handle_msg_ack(struct net_if *iface)
{
	switch (iface->config.dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_BOUND:
		break;
	case NET_DHCPV4_REQUESTING:
		NET_INFO("Received: %s",
			 log_strdup(net_sprint_ipv4_addr(
					 &iface->config.dhcpv4.requested_ip)));

		if (!net_if_ipv4_addr_add(iface,
					  &iface->config.dhcpv4.requested_ip,
					  NET_ADDR_DHCP,
					  iface->config.dhcpv4.lease_time)) {
			NET_DBG("Failed to add IPv4 addr to iface %p", iface);
			return;
		}

		dhcpv4_enter_bound(iface);
		break;

	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
		/* TODO: If the renewal is success, update only
		 * vlifetime on iface.
		 */
		dhcpv4_enter_bound(iface);
		break;
	}
}

static void dhcpv4_handle_msg_nak(struct net_if *iface)
{
	switch (iface->config.dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_BOUND:
		break;
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_REBINDING:
		/* Restart the configuration process. */
		dhcpv4_enter_selecting(iface);
		break;
	}
}

static void dhcpv4_handle_reply(struct net_if *iface,
				enum dhcpv4_msg_type msg_type)
{
	NET_DBG("state=%s msg=%s",
		net_dhcpv4_state_name(iface->config.dhcpv4.state),
		dhcpv4_msg_type_name(msg_type));

	switch (msg_type) {
	case DHCPV4_MSG_TYPE_OFFER:
		dhcpv4_handle_msg_offer(iface);
		break;
	case DHCPV4_MSG_TYPE_ACK:
		dhcpv4_handle_msg_ack(iface);
		break;
	case DHCPV4_MSG_TYPE_NAK:
		dhcpv4_handle_msg_nak(iface);
		break;
	default:
		NET_DBG("ignore message");
		break;
	}
}

static enum net_verdict net_dhcpv4_input(struct net_conn *conn,
					 struct net_pkt *pkt,
					 union net_ip_header *ip_hdr,
					 union net_proto_header *proto_hdr,
					 void *user_data)
{
	NET_PKT_DATA_ACCESS_DEFINE(dhcp_access, struct dhcp_msg);
	enum dhcpv4_msg_type msg_type = 0;
	struct dhcp_msg *msg;
	struct net_if *iface;

	if (!conn) {
		NET_DBG("Invalid connection");
		return NET_DROP;
	}

	if (!pkt) {
		NET_DBG("Invalid packet");
		return NET_DROP;
	}

	iface = net_pkt_iface(pkt);
	if (!iface) {
		NET_DBG("no iface");
		return NET_DROP;
	}

	/* If the message is not DHCP then continue passing to
	 * related handlers.
	 */
	if (net_pkt_get_len(pkt) < NET_IPV4UDPH_LEN + sizeof(struct dhcp_msg)) {
		NET_DBG("Input msg is not related to DHCPv4");
		return NET_CONTINUE;

	}

	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, NET_IPV4UDPH_LEN)) {
		return NET_DROP;
	}

	msg = (struct dhcp_msg *)net_pkt_get_data(pkt, &dhcp_access);
	if (!msg) {
		return NET_DROP;
	}

	NET_DBG("Received dhcp msg [op=0x%x htype=0x%x hlen=%u xid=0x%x "
		"secs=%u flags=0x%x chaddr=%s",
		msg->op, msg->htype, msg->hlen, ntohl(msg->xid),
		msg->secs, msg->flags,
		log_strdup(net_sprint_ll_addr(msg->chaddr, 6)));
	NET_DBG("  ciaddr=%d.%d.%d.%d",
		msg->ciaddr[0], msg->ciaddr[1], msg->ciaddr[2], msg->ciaddr[3]);
	NET_DBG("  yiaddr=%d.%d.%d.%d",
		msg->yiaddr[0], msg->yiaddr[1], msg->yiaddr[2], msg->yiaddr[3]);
	NET_DBG("  siaddr=%d.%d.%d.%d",
		msg->siaddr[0], msg->siaddr[1], msg->siaddr[2], msg->siaddr[3]);
	NET_DBG("  giaddr=%d.%d.%d.%d]",
		msg->giaddr[0], msg->giaddr[1], msg->giaddr[2], msg->giaddr[3]);

	if (!(msg->op == DHCPV4_MSG_BOOT_REPLY &&
	      iface->config.dhcpv4.xid == ntohl(msg->xid) &&
	      !memcmp(msg->chaddr, net_if_get_link_addr(iface)->addr,
		      net_if_get_link_addr(iface)->len))) {

		NET_DBG("Unexpected op (%d), xid (%x vs %x) or chaddr",
			msg->op, iface->config.dhcpv4.xid, ntohl(msg->xid));
		return NET_DROP;
	}

	memcpy(iface->config.dhcpv4.requested_ip.s4_addr,
	       msg->yiaddr, sizeof(msg->yiaddr));

	net_pkt_acknowledge_data(pkt, &dhcp_access);

	/* SNAME, FILE are not used at the moment, skip it */
	if (net_pkt_skip(pkt, SIZE_OF_SNAME + SIZE_OF_FILE)) {
		NET_DBG("short packet while skipping sname");
		return NET_DROP;
	}

	if (!dhcpv4_parse_options(pkt, iface, &msg_type)) {
		return NET_DROP;
	}

	net_pkt_unref(pkt);

	dhcpv4_handle_reply(iface, msg_type);

	return NET_OK;
}

static void dhcpv4_iface_event_handler(struct net_mgmt_event_callback *cb,
				       u32_t mgmt_event, struct net_if *iface)
{
	sys_snode_t *node = NULL;

	SYS_SLIST_FOR_EACH_NODE(&dhcpv4_ifaces, node) {
		if (node == &iface->config.dhcpv4.node) {
			break;
		}
	}

	if (node == NULL) {
		return;
	}

	if (mgmt_event == NET_EVENT_IF_DOWN) {
		NET_DBG("Interface %p going down", iface);

		if (iface->config.dhcpv4.state == NET_DHCPV4_BOUND) {
			iface->config.dhcpv4.attempts = 0U;
			iface->config.dhcpv4.state = NET_DHCPV4_RENEWING;
			NET_DBG("enter state=%s", net_dhcpv4_state_name(
					iface->config.dhcpv4.state));
		}
	} else if (mgmt_event == NET_EVENT_IF_UP) {
		NET_DBG("Interface %p coming up", iface);

		/* We should not call dhcpv4_send_request() directly here as
		 * the CONFIG_NET_MGMT_EVENT_STACK_SIZE is not large
		 * enough. Instead we can force a request timeout
		 * which will then call dhcpv4_send_request() automatically.
		 */
		iface->config.dhcpv4.timer_start = k_uptime_get() - 1;
		iface->config.dhcpv4.request_time = 0U;

		k_delayed_work_cancel(&timeout_work);
		k_delayed_work_submit(&timeout_work, K_NO_WAIT);
	}
}

const char *net_dhcpv4_state_name(enum net_dhcpv4_state state)
{
	static const char * const name[] = {
		"disabled",
		"init",
		"selecting",
		"requesting",
		"renewing",
		"rebinding",
		"bound",
	};

	__ASSERT_NO_MSG(state >= 0 && state < sizeof(name));
	return name[state];
}

void net_dhcpv4_start(struct net_if *iface)
{
	u32_t timeout;
	u32_t entropy;

	switch (iface->config.dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
		iface->config.dhcpv4.state = NET_DHCPV4_INIT;
		NET_DBG("iface %p state=%s", iface,
			net_dhcpv4_state_name(iface->config.dhcpv4.state));

		iface->config.dhcpv4.attempts = 0U;
		iface->config.dhcpv4.lease_time = 0U;
		iface->config.dhcpv4.renewal_time = 0U;

		iface->config.dhcpv4.server_id.s_addr = 0U;
		iface->config.dhcpv4.requested_ip.s_addr = 0U;

		/* We need entropy for both an XID and a random delay
		 * before sending the initial discover message.
		 */
		entropy = sys_rand32_get();

		/* A DHCP client MUST choose xid's in such a way as to
		 * minimize the change of using and xid identical to
		 * one used by another client.  Choose a random xid st
		 * startup and increment it on each new request.
		 */
		iface->config.dhcpv4.xid = entropy;


		/* RFC2131 4.1.1 requires we wait a random period
		 * between 1 and 10 seconds before sending the initial
		 * discover.
		 */
		timeout = entropy %
				(CONFIG_NET_DHCPV4_INITIAL_DELAY_MAX -
				  DHCPV4_INITIAL_DELAY_MIN) +
				DHCPV4_INITIAL_DELAY_MIN;

		NET_DBG("wait timeout=%us", timeout);

		if (sys_slist_is_empty(&dhcpv4_ifaces)) {
			net_mgmt_add_event_callback(&mgmt4_cb);
		}

		sys_slist_append(&dhcpv4_ifaces,
				 &iface->config.dhcpv4.node);

		iface->config.dhcpv4.timer_start = k_uptime_get();
		iface->config.dhcpv4.request_time = timeout;

		dhcpv4_update_timeout_work(timeout);

		break;
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_REBINDING:
	case NET_DHCPV4_BOUND:
		break;
	}
}

void net_dhcpv4_stop(struct net_if *iface)
{
	switch (iface->config.dhcpv4.state) {
	case NET_DHCPV4_DISABLED:
		break;

	case NET_DHCPV4_RENEWING:
	case NET_DHCPV4_BOUND:
		if (!net_if_ipv4_addr_rm(iface,
					 &iface->config.dhcpv4.requested_ip)) {
			NET_DBG("Failed to remove addr from iface");
		}

		/* Fall through */
	case NET_DHCPV4_INIT:
	case NET_DHCPV4_SELECTING:
	case NET_DHCPV4_REQUESTING:
	case NET_DHCPV4_REBINDING:
		iface->config.dhcpv4.state = NET_DHCPV4_DISABLED;
		NET_DBG("state=%s",
			net_dhcpv4_state_name(iface->config.dhcpv4.state));

		sys_slist_find_and_remove(&dhcpv4_ifaces,
					  &iface->config.dhcpv4.node);

		if (sys_slist_is_empty(&dhcpv4_ifaces)) {
			k_delayed_work_cancel(&timeout_work);
			net_mgmt_del_event_callback(&mgmt4_cb);
		}

		break;
	}
}

int net_dhcpv4_init(void)
{
	struct sockaddr local_addr;
	int ret;

	NET_DBG("");

	net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
			net_ipv4_unspecified_address());
	local_addr.sa_family = AF_INET;

	/* Register UDP input callback on
	 * DHCPV4_SERVER_PORT(67) and DHCPV4_CLIENT_PORT(68) for
	 * all dhcpv4 related incoming packets.
	 */
	ret = net_udp_register(AF_INET, NULL, &local_addr,
			       DHCPV4_SERVER_PORT,
			       DHCPV4_CLIENT_PORT,
			       net_dhcpv4_input, NULL, NULL);
	if (ret < 0) {
		NET_DBG("UDP callback registration failed");
		return ret;
	}

	k_delayed_work_init(&timeout_work, dhcpv4_timeout);

	/* Catch network interface UP or DOWN events and renew the address
	 * if interface is coming back up again.
	 */
	net_mgmt_init_event_callback(&mgmt4_cb, dhcpv4_iface_event_handler,
					 NET_EVENT_IF_DOWN | NET_EVENT_IF_UP);

	return 0;
}
