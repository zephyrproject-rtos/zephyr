/** @file
 * @brief DHCPv4 server implementation
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(net_dhcpv4_server, CONFIG_NET_DHCPV4_SERVER_LOG_LEVEL);

#include "dhcpv4_internal.h"
#include "net_private.h"
#include "../../l2/ethernet/arp.h"

#define DHCPV4_OPTIONS_MSG_TYPE_SIZE 3
#define DHCPV4_OPTIONS_IP_LEASE_TIME_SIZE 6
#define DHCPV4_OPTIONS_SERVER_ID_SIZE 6
#define DHCPV4_OPTIONS_SUBNET_MASK_SIZE 6
#define DHCPV4_OPTIONS_CLIENT_ID_MIN_SIZE 2

#define ADDRESS_RESERVED_TIMEOUT K_SECONDS(5)
#define ADDRESS_PROBE_TIMEOUT K_MSEC(CONFIG_NET_DHCPV4_SERVER_ICMP_PROBE_TIMEOUT)

#if (CONFIG_NET_DHCPV4_SERVER_ICMP_PROBE_TIMEOUT > 0)
#define DHCPV4_SERVER_ICMP_PROBE 1
#endif

/* RFC 1497 [17] */
static const uint8_t magic_cookie[4] = { 0x63, 0x82, 0x53, 0x63 };

#define DHCPV4_MAX_PARAMETERS_REQUEST_LEN 16

struct dhcpv4_parameter_request_list {
	uint8_t list[DHCPV4_MAX_PARAMETERS_REQUEST_LEN];
	uint8_t count;
};

struct dhcpv4_server_probe_ctx {
	struct net_icmp_ctx icmp_ctx;
	struct dhcp_msg discovery;
	struct dhcpv4_parameter_request_list params;
	struct dhcpv4_addr_slot *slot;
};

struct dhcpv4_server_ctx {
	struct net_if *iface;
	int sock;
	struct k_work_delayable timeout_work;
	struct dhcpv4_addr_slot addr_pool[CONFIG_NET_DHCPV4_SERVER_ADDR_COUNT];
	struct in_addr server_addr;
	struct in_addr netmask;
#if defined(DHCPV4_SERVER_ICMP_PROBE)
	struct dhcpv4_server_probe_ctx probe_ctx;
#endif
};

static struct dhcpv4_server_ctx server_ctx[CONFIG_NET_DHCPV4_SERVER_INSTANCES];
static struct zsock_pollfd fds[CONFIG_NET_DHCPV4_SERVER_INSTANCES];
static K_MUTEX_DEFINE(server_lock);

static void dhcpv4_server_timeout_recalc(struct dhcpv4_server_ctx *ctx)
{
	k_timepoint_t next = sys_timepoint_calc(K_FOREVER);
	k_timeout_t timeout;

	for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
		struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

		if (slot->state == DHCPV4_SERVER_ADDR_RESERVED ||
		    slot->state == DHCPV4_SERVER_ADDR_ALLOCATED) {
			if (sys_timepoint_cmp(slot->expiry, next) < 0) {
				next = slot->expiry;
			}
		}
	}

	timeout = sys_timepoint_timeout(next);

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		LOG_DBG("No more addresses, canceling timer");
		k_work_cancel_delayable(&ctx->timeout_work);
	} else {
		k_work_reschedule(&ctx->timeout_work, timeout);
	}
}

/* Option parsing. */

static uint8_t *dhcpv4_find_option(uint8_t *data, size_t datalen,
				   uint8_t *optlen, uint8_t opt_code)
{
	uint8_t *opt = NULL;

	while (datalen > 0) {
		uint8_t code;
		uint8_t len;

		code = *data;

		/* Two special cases (fixed sized options) */
		if (code == 0) {
			data++;
			datalen--;
			continue;
		}

		if (code == DHCPV4_OPTIONS_END) {
			break;
		}

		/* Length field should now follow. */
		if (datalen < 2) {
			break;
		}

		len = *(data + 1);

		if (datalen < len + 2) {
			break;
		}

		if (code == opt_code) {
			/* Found the option. */
			opt = data + 2;
			*optlen = len;
			break;
		}

		data += len + 2;
		datalen -= len + 2;
	}

	return opt;
}

static int dhcpv4_find_message_type_option(uint8_t *data, size_t datalen,
					   uint8_t *msgtype)
{
	uint8_t *opt;
	uint8_t optlen;

	opt = dhcpv4_find_option(data, datalen, &optlen,
				 DHCPV4_OPTIONS_MSG_TYPE);
	if (opt == NULL) {
		return -ENOENT;
	}

	if (optlen != 1) {
		return -EINVAL;
	}

	*msgtype = *opt;

	return 0;
}

static int dhcpv4_find_server_id_option(uint8_t *data, size_t datalen,
					struct in_addr *server_id)
{
	uint8_t *opt;
	uint8_t optlen;

	opt = dhcpv4_find_option(data, datalen, &optlen,
				 DHCPV4_OPTIONS_SERVER_ID);
	if (opt == NULL) {
		return -ENOENT;
	}

	if (optlen != sizeof(struct in_addr)) {
		return -EINVAL;
	}

	memcpy(server_id, opt, sizeof(struct in_addr));

	return 0;
}

static int dhcpv4_find_client_id_option(uint8_t *data, size_t datalen,
					uint8_t *client_id, uint8_t *len)
{
	uint8_t *opt;
	uint8_t optlen;

	opt = dhcpv4_find_option(data, datalen, &optlen,
				 DHCPV4_OPTIONS_CLIENT_ID);
	if (opt == NULL) {
		return -ENOENT;
	}

	if (optlen < DHCPV4_OPTIONS_CLIENT_ID_MIN_SIZE) {
		return -EINVAL;
	}

	if (optlen > *len) {
		LOG_ERR("Not enough memory for DHCPv4 client identifier.");
		return -ENOMEM;
	}

	memcpy(client_id, opt, optlen);
	*len = optlen;

	return 0;
}

static int dhcpv4_find_requested_ip_option(uint8_t *data, size_t datalen,
					   struct in_addr *requested_ip)
{
	uint8_t *opt;
	uint8_t optlen;

	opt = dhcpv4_find_option(data, datalen, &optlen,
				 DHCPV4_OPTIONS_REQ_IPADDR);
	if (opt == NULL) {
		return -ENOENT;
	}

	if (optlen != sizeof(struct in_addr)) {
		return -EINVAL;
	}

	memcpy(requested_ip, opt, sizeof(struct in_addr));

	return 0;
}

static int dhcpv4_find_ip_lease_time_option(uint8_t *data, size_t datalen,
					    uint32_t *lease_time)
{
	uint8_t *opt;
	uint8_t optlen;

	opt = dhcpv4_find_option(data, datalen, &optlen,
				 DHCPV4_OPTIONS_LEASE_TIME);
	if (opt == NULL) {
		return -ENOENT;
	}

	if (optlen != sizeof(uint32_t)) {
		return -EINVAL;
	}

	*lease_time = sys_get_be32(opt);

	return 0;
}

static int dhcpv4_find_parameter_request_list_option(
				uint8_t *data, size_t datalen,
				struct dhcpv4_parameter_request_list *params)
{
	uint8_t *opt;
	uint8_t optlen;

	opt = dhcpv4_find_option(data, datalen, &optlen,
				 DHCPV4_OPTIONS_REQ_LIST);
	if (opt == NULL) {
		return -ENOENT;
	}

	if (optlen > sizeof(params->list)) {
		/* Best effort here, copy as much as we can. */
		optlen = sizeof(params->list);
	}

	memcpy(params->list, opt, optlen);
	params->count = optlen;

	return 0;
}

/* Option encoding. */

static uint8_t *dhcpv4_encode_magic_cookie(uint8_t *buf, size_t *buflen)
{
	if (buf == NULL || *buflen < SIZE_OF_MAGIC_COOKIE) {
		return NULL;
	}

	memcpy(buf, magic_cookie, SIZE_OF_MAGIC_COOKIE);

	*buflen -= SIZE_OF_MAGIC_COOKIE;

	return buf + SIZE_OF_MAGIC_COOKIE;
}

static uint8_t *dhcpv4_encode_ip_lease_time_option(uint8_t *buf, size_t *buflen,
						   uint32_t lease_time)
{
	if (buf == NULL || *buflen < DHCPV4_OPTIONS_IP_LEASE_TIME_SIZE) {
		return NULL;
	}

	buf[0] = DHCPV4_OPTIONS_LEASE_TIME;
	buf[1] = sizeof(lease_time);
	sys_put_be32(lease_time, &buf[2]);

	*buflen -= DHCPV4_OPTIONS_IP_LEASE_TIME_SIZE;

	return buf + DHCPV4_OPTIONS_IP_LEASE_TIME_SIZE;
}

static uint8_t *dhcpv4_encode_message_type_option(uint8_t *buf, size_t *buflen,
						  uint8_t msgtype)
{
	if (buf == NULL || *buflen < DHCPV4_OPTIONS_MSG_TYPE_SIZE) {
		return NULL;
	}

	buf[0] = DHCPV4_OPTIONS_MSG_TYPE;
	buf[1] = 1;
	buf[2] = msgtype;

	*buflen -= DHCPV4_OPTIONS_MSG_TYPE_SIZE;

	return buf + DHCPV4_OPTIONS_MSG_TYPE_SIZE;
}

static uint8_t *dhcpv4_encode_server_id_option(uint8_t *buf, size_t *buflen,
					       struct in_addr *server_id)
{
	if (buf == NULL || *buflen < DHCPV4_OPTIONS_SERVER_ID_SIZE) {
		return NULL;
	}

	buf[0] = DHCPV4_OPTIONS_SERVER_ID;
	buf[1] = sizeof(struct in_addr);
	memcpy(&buf[2], server_id->s4_addr, sizeof(struct in_addr));

	*buflen -= DHCPV4_OPTIONS_SERVER_ID_SIZE;

	return buf + DHCPV4_OPTIONS_SERVER_ID_SIZE;
}

static uint8_t *dhcpv4_encode_subnet_mask_option(uint8_t *buf, size_t *buflen,
						 struct in_addr *mask)
{
	if (buf == NULL || *buflen < DHCPV4_OPTIONS_SUBNET_MASK_SIZE) {
		return NULL;
	}

	buf[0] = DHCPV4_OPTIONS_SUBNET_MASK;
	buf[1] = sizeof(struct in_addr);
	memcpy(&buf[2], mask->s4_addr, sizeof(struct in_addr));

	*buflen -= DHCPV4_OPTIONS_SUBNET_MASK_SIZE;

	return buf + DHCPV4_OPTIONS_SUBNET_MASK_SIZE;
}

static uint8_t *dhcpv4_encode_end_option(uint8_t *buf, size_t *buflen)
{
	if (buf == NULL || *buflen < 1) {
		return NULL;
	}

	buf[0] = DHCPV4_OPTIONS_END;

	*buflen -= 1;

	return buf + 1;
}

/* Response handlers. */

static uint8_t *dhcpv4_encode_header(uint8_t *buf, size_t *buflen,
				     struct dhcp_msg *msg,
				     struct in_addr *yiaddr)
{
	struct dhcp_msg *reply_msg = (struct dhcp_msg *)buf;

	if (buf == NULL || *buflen < sizeof(struct dhcp_msg)) {
		return NULL;
	}

	reply_msg->op = DHCPV4_MSG_BOOT_REPLY;
	reply_msg->htype = msg->htype;
	reply_msg->hlen = msg->hlen;
	reply_msg->hops = 0;
	reply_msg->xid = msg->xid;
	reply_msg->secs = 0;
	reply_msg->flags = msg->flags;
	memcpy(reply_msg->ciaddr, msg->ciaddr, sizeof(reply_msg->ciaddr));
	if (yiaddr != NULL) {
		memcpy(reply_msg->yiaddr, yiaddr, sizeof(struct in_addr));
	} else {
		memset(reply_msg->yiaddr, 0, sizeof(reply_msg->ciaddr));
	}
	memset(reply_msg->siaddr, 0, sizeof(reply_msg->siaddr));
	memcpy(reply_msg->giaddr, msg->giaddr, sizeof(reply_msg->giaddr));
	memcpy(reply_msg->chaddr, msg->chaddr, sizeof(reply_msg->chaddr));

	*buflen -= sizeof(struct dhcp_msg);

	return buf + sizeof(struct dhcp_msg);
}

static uint8_t *dhcpv4_encode_string(uint8_t *buf, size_t *buflen, char *str,
				     size_t max_len)
{
	if (buf == NULL || *buflen < max_len) {
		return NULL;
	}

	memset(buf, 0, max_len);

	if (str == NULL) {
		goto out;
	}

	strncpy(buf, str, max_len - 1);

 out:
	*buflen -= max_len;

	return buf + max_len;
}

static uint8_t *dhcpv4_encode_sname(uint8_t *buf, size_t *buflen, char *sname)
{
	return dhcpv4_encode_string(buf, buflen, sname, SIZE_OF_SNAME);
}

static uint8_t *dhcpv4_encode_file(uint8_t *buf, size_t *buflen, char *file)
{
	return dhcpv4_encode_string(buf, buflen, file, SIZE_OF_FILE);
}

static uint8_t *dhcpv4_encode_requested_params(
				uint8_t *buf, size_t *buflen,
				struct dhcpv4_server_ctx *ctx,
				struct dhcpv4_parameter_request_list *params)
{
	for (uint8_t i = 0; i < params->count; i++) {
		switch (params->list[i]) {
		case DHCPV4_OPTIONS_SUBNET_MASK:
			buf = dhcpv4_encode_subnet_mask_option(
				buf, buflen, &ctx->netmask);
			if (buf == NULL) {
				goto out;
			}
			break;

		/* Others - just ignore. */
		default:
			break;
		}
	}

out:
	return buf;
}

static int dhcpv4_send(struct dhcpv4_server_ctx *ctx, enum net_dhcpv4_msg_type type,
		       uint8_t *reply, size_t len, struct dhcp_msg *msg,
		       struct in_addr *yiaddr)
{
	struct sockaddr_in dst_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(DHCPV4_CLIENT_PORT),
	};
	struct in_addr giaddr; /* Relay agent address */
	struct in_addr ciaddr; /* Client address */
	int ret;

	memcpy(&giaddr, msg->giaddr, sizeof(giaddr));
	memcpy(&ciaddr, msg->ciaddr, sizeof(ciaddr));

	/* Select destination address as described in ch. 4.1. */
	if (!net_ipv4_is_addr_unspecified(&giaddr)) {
		/* If the 'giaddr' field in a DHCP message from a client is
		 * non-zero, the server sends any return messages to the
		 * 'DHCP server' port on the BOOTP relay agent whose address
		 * appears in 'giaddr'.
		 */
		dst_addr.sin_addr = giaddr;
		dst_addr.sin_port = htons(DHCPV4_SERVER_PORT);
	} else if (type == NET_DHCPV4_MSG_TYPE_NAK) {
		/* In all cases, when 'giaddr' is zero, the server broadcasts
		 * any DHCPNAK messages to 0xffffffff.
		 */
		dst_addr.sin_addr = *net_ipv4_broadcast_address();
	} else if (!net_ipv4_is_addr_unspecified(&ciaddr)) {
		/* If the 'giaddr' field is zero and the 'ciaddr' field is
		 * nonzero, then the server unicasts DHCPOFFER and DHCPACK
		 * messages to the address in 'ciaddr'.
		 */
		dst_addr.sin_addr = ciaddr;
	} else if (ntohs(msg->flags) & DHCPV4_MSG_BROADCAST) {
		/* If 'giaddr' is zero and 'ciaddr' is zero, and the broadcast
		 * bit is set, then the server broadcasts DHCPOFFER and DHCPACK
		 * messages to 0xffffffff.
		 */
		dst_addr.sin_addr = *net_ipv4_broadcast_address();
	} else if (yiaddr != NULL) {
		/* If the broadcast bit is not set and 'giaddr' is zero and
		 * 'ciaddr' is zero, then the server unicasts DHCPOFFER and
		 * DHCPACK messages to the client's hardware address and 'yiaddr'
		 * address.
		 */
		struct net_eth_addr hwaddr;

		memcpy(&hwaddr, msg->chaddr, sizeof(hwaddr));
		net_arp_update(ctx->iface, yiaddr, &hwaddr, false, true);
		dst_addr.sin_addr = *yiaddr;
	} else {
		NET_ERR("Unspecified destination address.");
		return -EDESTADDRREQ;
	}

	ret = zsock_sendto(ctx->sock, reply, len, 0, (struct sockaddr *)&dst_addr,
			   sizeof(dst_addr));
	if (ret < 0) {
		return -errno;
	}

	return 0;
}

static int dhcpv4_send_offer(struct dhcpv4_server_ctx *ctx, struct dhcp_msg *msg,
			     struct in_addr *addr, uint32_t lease_time,
			     struct dhcpv4_parameter_request_list *params)
{
	uint8_t reply[NET_IPV4_MTU];
	uint8_t *buf = reply;
	size_t buflen = sizeof(reply);
	size_t reply_len = 0;
	int ret;

	buf = dhcpv4_encode_header(buf, &buflen, msg, addr);
	buf = dhcpv4_encode_sname(buf, &buflen, NULL);
	buf = dhcpv4_encode_file(buf, &buflen, NULL);
	buf = dhcpv4_encode_magic_cookie(buf, &buflen);
	buf = dhcpv4_encode_ip_lease_time_option(buf, &buflen, lease_time);
	buf = dhcpv4_encode_message_type_option(buf, &buflen,
						NET_DHCPV4_MSG_TYPE_OFFER);
	buf = dhcpv4_encode_server_id_option(buf, &buflen, &ctx->server_addr);
	buf = dhcpv4_encode_requested_params(buf, &buflen, ctx, params);
	buf = dhcpv4_encode_end_option(buf, &buflen);

	if (buf == NULL) {
		LOG_ERR("Failed to encode %s message", "Offer");
		return -ENOMEM;
	}

	reply_len = sizeof(reply) - buflen;

	ret = dhcpv4_send(ctx, NET_DHCPV4_MSG_TYPE_OFFER, reply, reply_len,
			  msg, addr);
	if (ret < 0) {
		LOG_ERR("Failed to send %s message, %d", "Offer", ret);
		return ret;
	}

	return 0;
}

static int dhcpv4_send_ack(struct dhcpv4_server_ctx *ctx, struct dhcp_msg *msg,
			   struct in_addr *addr, uint32_t lease_time,
			   struct dhcpv4_parameter_request_list *params,
			   bool inform)
{
	uint8_t reply[NET_IPV4_MTU];
	uint8_t *buf = reply;
	size_t buflen = sizeof(reply);
	size_t reply_len = 0;
	int ret;

	buf = dhcpv4_encode_header(buf, &buflen, msg, inform ? NULL : addr);
	buf = dhcpv4_encode_sname(buf, &buflen, NULL);
	buf = dhcpv4_encode_file(buf, &buflen, NULL);
	buf = dhcpv4_encode_magic_cookie(buf, &buflen);
	if (!inform) {
		buf = dhcpv4_encode_ip_lease_time_option(buf, &buflen, lease_time);
	}
	buf = dhcpv4_encode_message_type_option(buf, &buflen,
						NET_DHCPV4_MSG_TYPE_ACK);
	buf = dhcpv4_encode_server_id_option(buf, &buflen, &ctx->server_addr);
	buf = dhcpv4_encode_requested_params(buf, &buflen, ctx, params);
	buf = dhcpv4_encode_end_option(buf, &buflen);

	if (buf == NULL) {
		LOG_ERR("Failed to encode %s message", "ACK");
		return -ENOMEM;
	}

	reply_len = sizeof(reply) - buflen;

	ret = dhcpv4_send(ctx, NET_DHCPV4_MSG_TYPE_ACK, reply, reply_len, msg,
			  addr);
	if (ret < 0) {
		LOG_ERR("Failed to send %s message, %d", "ACK", ret);
		return ret;
	}

	return 0;
}

static int dhcpv4_send_nak(struct dhcpv4_server_ctx *ctx, struct dhcp_msg *msg)
{
	uint8_t reply[NET_IPV4_MTU];
	uint8_t *buf = reply;
	size_t buflen = sizeof(reply);
	size_t reply_len = 0;
	int ret;

	buf = dhcpv4_encode_header(buf, &buflen, msg, NULL);
	buf = dhcpv4_encode_sname(buf, &buflen, NULL);
	buf = dhcpv4_encode_file(buf, &buflen, NULL);
	buf = dhcpv4_encode_magic_cookie(buf, &buflen);
	buf = dhcpv4_encode_message_type_option(buf, &buflen,
						NET_DHCPV4_MSG_TYPE_NAK);
	buf = dhcpv4_encode_server_id_option(buf, &buflen, &ctx->server_addr);
	buf = dhcpv4_encode_end_option(buf, &buflen);

	if (buf == NULL) {
		LOG_ERR("Failed to encode %s message", "NAK");
		return -ENOMEM;
	}

	reply_len = sizeof(reply) - buflen;

	ret = dhcpv4_send(ctx, NET_DHCPV4_MSG_TYPE_NAK, reply, reply_len, msg,
			  NULL);
	if (ret < 0) {
		LOG_ERR("Failed to send %s message, %d", "NAK", ret);
		return ret;
	}

	return 0;
}

/* Message handlers. */

static int dhcpv4_get_client_id(struct dhcp_msg *msg, uint8_t *options,
				uint8_t optlen, struct dhcpv4_client_id *client_id)
{
	int ret;

	client_id->len = sizeof(client_id->buf);

	ret = dhcpv4_find_client_id_option(options, optlen, client_id->buf,
					   &client_id->len);
	if (ret == 0) {
		return 0;
	}

	/* No Client Id option or too long to use, fallback to hardware address. */
	if (msg->hlen > sizeof(msg->chaddr)) {
		LOG_ERR("Malformed chaddr length.");
		return -EINVAL;
	}

	client_id->buf[0] = msg->htype;
	client_id->buf[1] = msg->hlen;
	memcpy(client_id->buf + 2, msg->chaddr, msg->hlen);
	client_id->len = msg->hlen + 2;

	return 0;
}

static uint32_t dhcpv4_get_lease_time(uint8_t *options, uint8_t optlen)
{
	uint32_t lease_time;

	if (dhcpv4_find_ip_lease_time_option(options, optlen,
					     &lease_time) == 0) {
		return lease_time;
	}

	return CONFIG_NET_DHCPV4_SERVER_ADDR_LEASE_TIME;
}

#if defined(DHCPV4_SERVER_ICMP_PROBE)
static int dhcpv4_probe_address(struct dhcpv4_server_ctx *ctx,
				 struct dhcpv4_addr_slot *slot)
{
	struct sockaddr_in dest_addr = {
		.sin_family = AF_INET,
		.sin_addr = slot->addr,
	};
	int ret;

	ret = net_icmp_send_echo_request(&ctx->probe_ctx.icmp_ctx, ctx->iface,
					 (struct sockaddr *)&dest_addr,
					 NULL, ctx);
	if (ret < 0) {
		LOG_ERR("Failed to send ICMP probe");
	}

	return ret;
}

static int echo_reply_handler(struct net_icmp_ctx *icmp_ctx,
			      struct net_pkt *pkt,
			      struct net_icmp_ip_hdr *ip_hdr,
			      struct net_icmp_hdr *icmp_hdr,
			      void *user_data)
{
	struct dhcpv4_server_ctx *ctx = user_data;
	struct dhcpv4_server_probe_ctx *probe_ctx = &ctx->probe_ctx;
	struct dhcpv4_addr_slot *new_slot = NULL;
	struct in_addr peer_addr;

	ARG_UNUSED(icmp_ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(ip_hdr);
	ARG_UNUSED(icmp_hdr);

	k_mutex_lock(&server_lock, K_FOREVER);

	if (probe_ctx->slot == NULL) {
		goto out;
	}

	if (ip_hdr->family != AF_INET) {
		goto out;
	}

	net_ipv4_addr_copy_raw((uint8_t *)&peer_addr, ip_hdr->ipv4->src);
	if (!net_ipv4_addr_cmp(&peer_addr, &probe_ctx->slot->addr)) {
		goto out;
	}

	LOG_DBG("Got ICMP probe response, blocking address %s",
		net_sprint_ipv4_addr(&probe_ctx->slot->addr));

	probe_ctx->slot->state = DHCPV4_SERVER_ADDR_DECLINED;

	/* Try to find next free address */
	for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
		struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

		if (slot->state == DHCPV4_SERVER_ADDR_FREE) {
			new_slot = slot;
			break;
		}
	}

	if (new_slot == NULL) {
		LOG_DBG("No more free addresses to assign, ICMP probing stopped");
		probe_ctx->slot = NULL;
		dhcpv4_server_timeout_recalc(ctx);
		goto out;
	}

	if (dhcpv4_probe_address(ctx, new_slot) < 0) {
		probe_ctx->slot = NULL;
		dhcpv4_server_timeout_recalc(ctx);
		goto out;
	}

	new_slot->state = DHCPV4_SERVER_ADDR_RESERVED;
	new_slot->expiry = sys_timepoint_calc(ADDRESS_PROBE_TIMEOUT);
	new_slot->client_id.len = probe_ctx->slot->client_id.len;
	memcpy(new_slot->client_id.buf, probe_ctx->slot->client_id.buf,
	       new_slot->client_id.len);
	new_slot->lease_time = probe_ctx->slot->lease_time;

	probe_ctx->slot = new_slot;

	dhcpv4_server_timeout_recalc(ctx);

out:
	k_mutex_unlock(&server_lock);

	return 0;
}

static int dhcpv4_server_probing_init(struct dhcpv4_server_ctx *ctx)
{
	return net_icmp_init_ctx(&ctx->probe_ctx.icmp_ctx,
				 NET_ICMPV4_ECHO_REPLY, 0,
				 echo_reply_handler);
}

static void dhcpv4_server_probing_deinit(struct dhcpv4_server_ctx *ctx)
{
	(void)net_icmp_cleanup_ctx(&ctx->probe_ctx.icmp_ctx);
}

static int dhcpv4_server_probe_setup(struct dhcpv4_server_ctx *ctx,
				     struct dhcpv4_addr_slot *slot,
				     struct dhcp_msg *msg,
				     struct dhcpv4_parameter_request_list *params)
{
	int ret;

	if (ctx->probe_ctx.slot != NULL) {
		return -EBUSY;
	}

	ret = dhcpv4_probe_address(ctx, slot);
	if (ret < 0) {
		return ret;
	}

	ctx->probe_ctx.slot = slot;
	ctx->probe_ctx.discovery = *msg;
	ctx->probe_ctx.params = *params;

	return 0;
}

static void dhcpv4_server_probe_timeout(struct dhcpv4_server_ctx *ctx,
					struct dhcpv4_addr_slot *slot)
{
	/* Probe timer expired, send offer. */
	ctx->probe_ctx.slot = NULL;

	(void)net_arp_clear_pending(ctx->iface, &slot->addr);

	if (dhcpv4_send_offer(ctx, &ctx->probe_ctx.discovery, &slot->addr,
			      slot->lease_time, &ctx->probe_ctx.params) < 0) {
		slot->state = DHCPV4_SERVER_ADDR_FREE;
		return;
	}

	slot->expiry = sys_timepoint_calc(ADDRESS_RESERVED_TIMEOUT);
}

static bool dhcpv4_server_is_slot_probed(struct dhcpv4_server_ctx *ctx,
					 struct dhcpv4_addr_slot *slot)
{
	return (ctx->probe_ctx.slot == slot);
}
#else /* defined(DHCPV4_SERVER_ICMP_PROBE) */
#define dhcpv4_server_probing_init(...) (0)
#define dhcpv4_server_probing_deinit(...)
#define dhcpv4_server_probe_setup(...) (-ENOTSUP)
#define dhcpv4_server_probe_timeout(...)
#define dhcpv4_server_is_slot_probed(...) (false)
#endif /* defined(DHCPV4_SERVER_ICMP_PROBE) */

static void dhcpv4_handle_discover(struct dhcpv4_server_ctx *ctx,
				   struct dhcp_msg *msg, uint8_t *options,
				   uint8_t optlen)
{
	struct dhcpv4_parameter_request_list params = { 0 };
	struct dhcpv4_addr_slot *selected = NULL;
	struct dhcpv4_client_id client_id;
	bool probe = false;
	int ret;

	ret = dhcpv4_get_client_id(msg, options, optlen, &client_id);
	if (ret < 0) {
		return;
	}

	(void)dhcpv4_find_parameter_request_list_option(options, optlen, &params);

	/* Address pool and address selection algorithm as
	 * described in 4.3.1
	 */

	/* 1. Check for current bindings */
	for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
		struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

		if ((slot->state == DHCPV4_SERVER_ADDR_RESERVED ||
		     slot->state == DHCPV4_SERVER_ADDR_ALLOCATED) &&
		    slot->client_id.len == client_id.len &&
		    memcmp(slot->client_id.buf, client_id.buf,
			    client_id.len) == 0) {
			if (slot->state == DHCPV4_SERVER_ADDR_RESERVED &&
			    dhcpv4_server_is_slot_probed(ctx, slot)) {
				LOG_DBG("ICMP probing in progress, ignore Discovery");
				return;
			}

			/* Got match in current bindings. */
			selected = slot;
			break;
		}
	}

	/* 2. Skipped, for now expired/released entries are forgotten. */

	/* 3. Check Requested IP Address option. */
	if (selected == NULL) {
		struct in_addr requested_ip;

		ret = dhcpv4_find_requested_ip_option(options, optlen,
						      &requested_ip);
		if (ret == 0) {
			for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
				struct dhcpv4_addr_slot *slot =
							&ctx->addr_pool[i];

				if (net_ipv4_addr_cmp(&slot->addr,
						      &requested_ip) &&
				    slot->state == DHCPV4_SERVER_ADDR_FREE) {
					/* Requested address is free. */
					selected = slot;
					probe = true;
					break;
				}
			}
		}
	}

	/* 4. Allocate new address from pool, if available. */
	if (selected == NULL) {
		struct in_addr giaddr;

		memcpy(&giaddr, msg->giaddr, sizeof(giaddr));
		if (!net_ipv4_is_addr_unspecified(&giaddr)) {
			/* Only addresses in local subnet supproted for now. */
			return;
		}

		for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
			struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

			if (slot->state == DHCPV4_SERVER_ADDR_FREE) {
				/* Requested address is free. */
				selected = slot;
				probe = true;
				break;
			}
		}
	}

	if (selected == NULL) {
		LOG_ERR("No free address found in address pool");
	} else {
		uint32_t lease_time = dhcpv4_get_lease_time(options, optlen);

		if (IS_ENABLED(DHCPV4_SERVER_ICMP_PROBE) && probe) {
			if (dhcpv4_server_probe_setup(ctx, selected,
						      msg, &params) < 0) {
				/* Probing context already in use or failed to
				 * send probe, ignore Discovery for now and wait
				 * for retransmission.
				 */
				return;
			}

			selected->expiry =
				sys_timepoint_calc(ADDRESS_PROBE_TIMEOUT);
		} else {
			if (dhcpv4_send_offer(ctx, msg, &selected->addr,
					      lease_time, &params) < 0) {
				return;
			}

			selected->expiry =
				sys_timepoint_calc(ADDRESS_RESERVED_TIMEOUT);
		}

		LOG_DBG("DHCPv4 processing Discover - reserved %s",
			net_sprint_ipv4_addr(&selected->addr));

		selected->state = DHCPV4_SERVER_ADDR_RESERVED;
		selected->client_id.len = client_id.len;
		memcpy(selected->client_id.buf, client_id.buf, client_id.len);
		selected->lease_time = lease_time;
		dhcpv4_server_timeout_recalc(ctx);
	}
}

static void dhcpv4_handle_request(struct dhcpv4_server_ctx *ctx,
				  struct dhcp_msg *msg, uint8_t *options,
				  uint8_t optlen)
{
	struct dhcpv4_parameter_request_list params = { 0 };
	struct dhcpv4_addr_slot *selected = NULL;
	struct dhcpv4_client_id client_id;
	struct in_addr requested_ip, server_id, ciaddr, giaddr;
	int ret;

	memcpy(&ciaddr, msg->ciaddr, sizeof(ciaddr));
	memcpy(&giaddr, msg->giaddr, sizeof(giaddr));

	if (!net_ipv4_is_addr_unspecified(&giaddr)) {
		/* Only addresses in local subnet supported for now. */
		return;
	}

	ret = dhcpv4_get_client_id(msg, options, optlen, &client_id);
	if (ret < 0) {
		/* Failed to obtain Client ID, ignore. */
		return;
	}

	(void)dhcpv4_find_parameter_request_list_option(options, optlen, &params);

	ret = dhcpv4_find_server_id_option(options, optlen, &server_id);
	if (ret == 0) {
		/* Server ID present, Request generated during SELECTING. */
		if (!net_ipv4_addr_cmp(&ctx->server_addr, &server_id)) {
			/* Not for us, ignore. */
			return;
		}

		ret = dhcpv4_find_requested_ip_option(options, optlen,
						      &requested_ip);
		if (ret < 0) {
			/* Requested IP missing, ignore. */
			return;
		}

		if (!net_ipv4_is_addr_unspecified(&ciaddr)) {
			/* ciaddr MUST be zero */
			return;
		}

		for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
			struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

			if (net_ipv4_addr_cmp(&slot->addr, &requested_ip) &&
			    slot->client_id.len == client_id.len &&
			    memcmp(slot->client_id.buf, client_id.buf,
				   client_id.len) == 0 &&
			    slot->state == DHCPV4_SERVER_ADDR_RESERVED) {
				selected = slot;
				break;
			}
		}

		if (selected == NULL) {
			LOG_ERR("No valid slot found for DHCPv4 Request");
		} else {
			uint32_t lease_time = dhcpv4_get_lease_time(options, optlen);

			if (dhcpv4_send_ack(ctx, msg, &selected->addr, lease_time,
					    &params, false) < 0) {
				return;
			}

			LOG_DBG("DHCPv4 processing Request - allocated %s",
				net_sprint_ipv4_addr(&selected->addr));

			selected->lease_time = lease_time;
			selected->expiry = sys_timepoint_calc(
							K_SECONDS(lease_time));
			selected->state = DHCPV4_SERVER_ADDR_ALLOCATED;
			dhcpv4_server_timeout_recalc(ctx);
		}

		return;
	}

	/* No server ID option - check requested address. */
	ret = dhcpv4_find_requested_ip_option(options, optlen, &requested_ip);
	if (ret == 0) {
		/* Requested IP present, Request generated during INIT-REBOOT. */
		if (!net_ipv4_is_addr_unspecified(&ciaddr)) {
			/* ciaddr MUST be zero */
			return;
		}

		if (!net_if_ipv4_addr_mask_cmp(ctx->iface, &requested_ip)) {
			/* Wrong subnet. */
			dhcpv4_send_nak(ctx, msg);
		}

		for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
			struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

			if (slot->client_id.len == client_id.len &&
			    memcmp(slot->client_id.buf, client_id.buf,
				   client_id.len) == 0 &&
			    (slot->state == DHCPV4_SERVER_ADDR_RESERVED ||
			     slot->state == DHCPV4_SERVER_ADDR_ALLOCATED)) {
				selected = slot;
				break;
			}
		}

		if (selected != NULL) {
			if (net_ipv4_addr_cmp(&selected->addr, &requested_ip)) {
				uint32_t lease_time = dhcpv4_get_lease_time(
								options, optlen);

				if (dhcpv4_send_ack(ctx, msg, &selected->addr,
						    lease_time, &params,
						    false) < 0) {
					return;
				}

				selected->lease_time = lease_time;
				selected->expiry = sys_timepoint_calc(
							K_SECONDS(lease_time));
				dhcpv4_server_timeout_recalc(ctx);
			} else {
				dhcpv4_send_nak(ctx, msg);
			}
		}

		/* No notion of the client, remain silent. */
		return;
	}

	/* Neither server ID or requested IP set, Request generated during
	 * RENEWING or REBINDING.
	 */

	if (!net_if_ipv4_addr_mask_cmp(ctx->iface, &ciaddr)) {
		/* Wrong subnet. */
		dhcpv4_send_nak(ctx, msg);
	}

	for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
		struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

		if (net_ipv4_addr_cmp(&slot->addr, &ciaddr)) {
			selected = slot;
			break;
		}
	}

	if (selected != NULL) {
		if (selected->state == DHCPV4_SERVER_ADDR_ALLOCATED &&
		    selected->client_id.len == client_id.len &&
		    memcmp(selected->client_id.buf, client_id.buf,
			   client_id.len) == 0) {
			uint32_t lease_time = dhcpv4_get_lease_time(
								options, optlen);

			if (dhcpv4_send_ack(ctx, msg, &ciaddr, lease_time,
					    &params, false) < 0) {
				return;
			}

			selected->lease_time = lease_time;
			selected->expiry = sys_timepoint_calc(
							K_SECONDS(lease_time));
			dhcpv4_server_timeout_recalc(ctx);
		} else {
			dhcpv4_send_nak(ctx, msg);
		}
	}
}

static void dhcpv4_handle_decline(struct dhcpv4_server_ctx *ctx,
				  struct dhcp_msg *msg, uint8_t *options,
				  uint8_t optlen)
{
	struct dhcpv4_client_id client_id;
	struct in_addr requested_ip, server_id;
	int ret;

	ret = dhcpv4_find_server_id_option(options, optlen, &server_id);
	if (ret < 0) {
		/* No server ID, ignore. */
		return;
	}

	if (!net_ipv4_addr_cmp(&ctx->server_addr, &server_id)) {
		/* Not for us, ignore. */
		return;
	}

	ret = dhcpv4_get_client_id(msg, options, optlen, &client_id);
	if (ret < 0) {
		/* Failed to obtain Client ID, ignore. */
		return;
	}

	ret = dhcpv4_find_requested_ip_option(options, optlen,
						&requested_ip);
	if (ret < 0) {
		/* Requested IP missing, ignore. */
		return;
	}

	LOG_ERR("Received DHCPv4 Decline for %s (address already in use)",
		net_sprint_ipv4_addr(&requested_ip));

	for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
		struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

		if (net_ipv4_addr_cmp(&slot->addr, &requested_ip) &&
		    slot->client_id.len == client_id.len &&
		    memcmp(slot->client_id.buf, client_id.buf,
				client_id.len) == 0 &&
		    (slot->state == DHCPV4_SERVER_ADDR_RESERVED ||
		     slot->state == DHCPV4_SERVER_ADDR_ALLOCATED)) {
			slot->state = DHCPV4_SERVER_ADDR_DECLINED;
			slot->expiry = sys_timepoint_calc(K_FOREVER);
			dhcpv4_server_timeout_recalc(ctx);
			break;
		}
	}
}

static void dhcpv4_handle_release(struct dhcpv4_server_ctx *ctx,
				  struct dhcp_msg *msg, uint8_t *options,
				  uint8_t optlen)
{
	struct dhcpv4_client_id client_id;
	struct in_addr ciaddr, server_id;
	int ret;

	ret = dhcpv4_find_server_id_option(options, optlen, &server_id);
	if (ret < 0) {
		/* No server ID, ignore. */
		return;
	}

	if (!net_ipv4_addr_cmp(&ctx->server_addr, &server_id)) {
		/* Not for us, ignore. */
		return;
	}

	ret = dhcpv4_get_client_id(msg, options, optlen, &client_id);
	if (ret < 0) {
		/* Failed to obtain Client ID, ignore. */
		return;
	}

	memcpy(&ciaddr, msg->ciaddr, sizeof(ciaddr));

	for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
		struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

		if (net_ipv4_addr_cmp(&slot->addr, &ciaddr) &&
		    slot->client_id.len == client_id.len &&
		    memcmp(slot->client_id.buf, client_id.buf,
				client_id.len) == 0 &&
		    (slot->state == DHCPV4_SERVER_ADDR_RESERVED ||
		     slot->state == DHCPV4_SERVER_ADDR_ALLOCATED)) {
			LOG_DBG("DHCPv4 processing Release - %s",
				net_sprint_ipv4_addr(&slot->addr));

			slot->state = DHCPV4_SERVER_ADDR_FREE;
			slot->expiry = sys_timepoint_calc(K_FOREVER);
			dhcpv4_server_timeout_recalc(ctx);
			break;
		}
	}
}

static void dhcpv4_handle_inform(struct dhcpv4_server_ctx *ctx,
				 struct dhcp_msg *msg, uint8_t *options,
				 uint8_t optlen)
{
	struct dhcpv4_parameter_request_list params = { 0 };

	(void)dhcpv4_find_parameter_request_list_option(options, optlen, &params);
	(void)dhcpv4_send_ack(ctx, msg, (struct in_addr *)msg->ciaddr, 0,
			      &params, true);
}

/* Server core. */

static void dhcpv4_server_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct dhcpv4_server_ctx *ctx =
		CONTAINER_OF(dwork, struct dhcpv4_server_ctx, timeout_work);

	k_mutex_lock(&server_lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
		struct dhcpv4_addr_slot *slot = &ctx->addr_pool[i];

		if ((slot->state == DHCPV4_SERVER_ADDR_RESERVED ||
		     slot->state == DHCPV4_SERVER_ADDR_ALLOCATED) &&
		     sys_timepoint_expired(slot->expiry)) {
			if (slot->state == DHCPV4_SERVER_ADDR_RESERVED &&
			    dhcpv4_server_is_slot_probed(ctx, slot)) {
				dhcpv4_server_probe_timeout(ctx, slot);
			} else {
				LOG_DBG("Address %s expired",
					net_sprint_ipv4_addr(&slot->addr));
				slot->state = DHCPV4_SERVER_ADDR_FREE;
			}
		}
	}

	dhcpv4_server_timeout_recalc(ctx);

	k_mutex_unlock(&server_lock);
}

static void dhcpv4_process_data(struct dhcpv4_server_ctx *ctx, uint8_t *data,
				size_t datalen)
{
	struct dhcp_msg *msg;
	uint8_t msgtype;
	int ret;

	if (datalen < sizeof(struct dhcp_msg)) {
		LOG_DBG("DHCPv4 server malformed message");
		return;
	}

	msg = (struct dhcp_msg *)data;

	if (msg->op != DHCPV4_MSG_BOOT_REQUEST) {
		/* Silently drop messages other than BOOTREQUEST */
		return;
	}

	data += sizeof(struct dhcp_msg);
	datalen -= sizeof(struct dhcp_msg);

	/* Skip server hostname/filename/option cookie */
	if (datalen < (SIZE_OF_SNAME + SIZE_OF_FILE + SIZE_OF_MAGIC_COOKIE)) {
		return;
	}

	data += SIZE_OF_SNAME + SIZE_OF_FILE + SIZE_OF_MAGIC_COOKIE;
	datalen -= SIZE_OF_SNAME + SIZE_OF_FILE + SIZE_OF_MAGIC_COOKIE;

	/* Search options for DHCP message type. */
	ret = dhcpv4_find_message_type_option(data, datalen, &msgtype);
	if (ret < 0) {
		LOG_ERR("No message type option");
		return;
	}

	k_mutex_lock(&server_lock, K_FOREVER);

	switch (msgtype) {
	case NET_DHCPV4_MSG_TYPE_DISCOVER:
		dhcpv4_handle_discover(ctx, msg, data, datalen);
		break;
	case NET_DHCPV4_MSG_TYPE_REQUEST:
		dhcpv4_handle_request(ctx, msg, data, datalen);
		break;
	case NET_DHCPV4_MSG_TYPE_DECLINE:
		dhcpv4_handle_decline(ctx, msg, data, datalen);
		break;
	case NET_DHCPV4_MSG_TYPE_RELEASE:
		dhcpv4_handle_release(ctx, msg, data, datalen);
		break;
	case NET_DHCPV4_MSG_TYPE_INFORM:
		dhcpv4_handle_inform(ctx, msg, data, datalen);
		break;

	case NET_DHCPV4_MSG_TYPE_OFFER:
	case NET_DHCPV4_MSG_TYPE_ACK:
	case NET_DHCPV4_MSG_TYPE_NAK:
	default:
		/* Ignore server initiated and unknown message types. */
		break;
	}

	k_mutex_unlock(&server_lock);
}

static void dhcpv4_server_cb(struct k_work *work)
{
	struct net_socket_service_event *evt =
		CONTAINER_OF(work, struct net_socket_service_event, work);
	struct dhcpv4_server_ctx *ctx = NULL;
	uint8_t recv_buf[NET_IPV4_MTU];
	int ret;

	for (int i = 0; i < ARRAY_SIZE(server_ctx); i++) {
		if (server_ctx[i].sock == evt->event.fd) {
			ctx = &server_ctx[i];
			break;
		}
	}

	if (ctx == NULL) {
		LOG_ERR("No DHCPv4 server context found for given FD.");
		return;
	}

	if (evt->event.revents & ZSOCK_POLLERR) {
		LOG_ERR("DHCPv4 server poll revents error");
		net_dhcpv4_server_stop(ctx->iface);
		return;
	}

	if (!(evt->event.revents & ZSOCK_POLLIN)) {
		return;
	}

	ret = zsock_recvfrom(evt->event.fd, recv_buf, sizeof(recv_buf),
			     ZSOCK_MSG_DONTWAIT, NULL, 0);
	if (ret < 0) {
		if (errno == EAGAIN) {
			return;
		}

		LOG_ERR("DHCPv4 server recv error, %d", errno);
		net_dhcpv4_server_stop(ctx->iface);
		return;
	}

	dhcpv4_process_data(ctx, recv_buf, ret);
}

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(dhcpv4_server, NULL, dhcpv4_server_cb,
				      CONFIG_NET_DHCPV4_SERVER_INSTANCES);

int net_dhcpv4_server_start(struct net_if *iface, struct in_addr *base_addr)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr = INADDR_ANY_INIT,
		.sin_port = htons(DHCPV4_SERVER_PORT),
	};
	struct ifreq ifreq = { 0 };
	int ret, sock = -1, slot = -1;
	const struct in_addr *server_addr;
	struct in_addr netmask;

	if (iface == NULL || base_addr == NULL) {
		return -EINVAL;
	}

	if (!net_if_ipv4_addr_mask_cmp(iface, base_addr)) {
		LOG_ERR("Address pool does not belong to the interface subnet.");
		return -EINVAL;
	}

	server_addr = net_if_ipv4_select_src_addr(iface, base_addr);
	if (server_addr == NULL) {
		LOG_ERR("Failed to obtain a valid server address.");
		return -EINVAL;
	}

	if ((htonl(server_addr->s_addr) >= htonl(base_addr->s_addr)) &&
	    (htonl(server_addr->s_addr) <
	     htonl(base_addr->s_addr) + CONFIG_NET_DHCPV4_SERVER_ADDR_COUNT)) {
		LOG_ERR("Address pool overlaps with server address.");
		return -EINVAL;
	}

	netmask = net_if_ipv4_get_netmask(iface);
	if (net_ipv4_is_addr_unspecified(&netmask)) {
		LOG_ERR("Failed to obtain subnet mask.");
		return -EINVAL;
	}

	k_mutex_lock(&server_lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(server_ctx); i++) {
		if (server_ctx[i].iface != NULL) {
			if (server_ctx[i].iface == iface) {
				LOG_ERR("DHCPv4 server instance already running.");
				ret = -EALREADY;
				goto error;
			}
		} else {
			if (slot < 0) {
				slot = i;
			}
		}
	}

	if (slot < 0) {
		LOG_ERR("No free DHCPv4 server intance.");
		ret = -ENOMEM;
		goto error;
	}

	ret = net_if_get_name(iface, ifreq.ifr_name, sizeof(ifreq.ifr_name));
	if (ret < 0) {
		LOG_ERR("Failed to obtain interface name.");
		goto error;
	}

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		ret = errno;
		LOG_ERR("Failed to create DHCPv4 server socket, %d", ret);
		goto error;
	}

	ret = zsock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifreq,
			       sizeof(ifreq));
	if (ret < 0) {
		ret = errno;
		LOG_ERR("Failed to bind DHCPv4 server socket with interface, %d",
			ret);
		goto error;
	}

	ret = zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		ret = errno;
		LOG_ERR("Failed to bind DHCPv4 server socket, %d", ret);
		goto error;
	}

	fds[slot].fd = sock;
	fds[slot].events = ZSOCK_POLLIN;

	server_ctx[slot].iface = iface;
	server_ctx[slot].sock = sock;
	server_ctx[slot].server_addr = *server_addr;
	server_ctx[slot].netmask = netmask;

	k_work_init_delayable(&server_ctx[slot].timeout_work,
			      dhcpv4_server_timeout);

	LOG_DBG("Started DHCPv4 server, address pool:");
	for (int i = 0; i < ARRAY_SIZE(server_ctx[slot].addr_pool); i++) {
		server_ctx[slot].addr_pool[i].state = DHCPV4_SERVER_ADDR_FREE;
		server_ctx[slot].addr_pool[i].addr.s_addr =
					htonl(ntohl(base_addr->s_addr) + i);

		LOG_DBG("\t%2d: %s", i,
			net_sprint_ipv4_addr(
				&server_ctx[slot].addr_pool[i].addr));
	}

	ret = dhcpv4_server_probing_init(&server_ctx[slot]);
	if (ret < 0) {
		LOG_ERR("Failed to register probe handler, %d", ret);
		goto cleanup;
	}

	ret = net_socket_service_register(&dhcpv4_server, fds, ARRAY_SIZE(fds),
					  NULL);
	if (ret < 0) {
		LOG_ERR("Failed to register socket service, %d", ret);
		dhcpv4_server_probing_deinit(&server_ctx[slot]);
		goto cleanup;
	}

	k_mutex_unlock(&server_lock);

	return 0;

cleanup:
	memset(&server_ctx[slot], 0, sizeof(server_ctx[slot]));
	fds[slot].fd = -1;

error:
	if (sock >= 0) {
		(void)zsock_close(sock);
	}

	k_mutex_unlock(&server_lock);

	return ret;
}

int net_dhcpv4_server_stop(struct net_if *iface)
{
	struct k_work_sync sync;
	int slot = -1;
	int ret = 0;
	bool service_stop = true;

	if (iface == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&server_lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(server_ctx); i++) {
		if (server_ctx[i].iface == iface) {
			slot = i;
			break;
		}
	}

	if (slot < 0) {
		ret = -ENOENT;
		goto out;
	}

	fds[slot].fd = -1;
	(void)zsock_close(server_ctx[slot].sock);

	dhcpv4_server_probing_deinit(&server_ctx[slot]);
	k_work_cancel_delayable_sync(&server_ctx[slot].timeout_work, &sync);

	memset(&server_ctx[slot], 0, sizeof(server_ctx[slot]));

	for (int i = 0; i < ARRAY_SIZE(fds); i++) {
		if (fds[i].fd >= 0) {
			service_stop = false;
			break;
		}
	}

	if (service_stop) {
		ret = net_socket_service_unregister(&dhcpv4_server);
	} else {
		ret = net_socket_service_register(&dhcpv4_server, fds,
						  ARRAY_SIZE(fds), NULL);
	}

out:
	k_mutex_unlock(&server_lock);

	return ret;
}

static void dhcpv4_server_foreach_lease_on_ctx(struct dhcpv4_server_ctx *ctx,
						 net_dhcpv4_lease_cb_t cb,
						 void *user_data)
{
	for (int i = 0; i < ARRAY_SIZE(ctx->addr_pool); i++) {
		struct dhcpv4_addr_slot *addr = &ctx->addr_pool[i];

		if (addr->state != DHCPV4_SERVER_ADDR_FREE) {
			cb(ctx->iface, addr, user_data);
		}
	}
}

int net_dhcpv4_server_foreach_lease(struct net_if *iface,
				    net_dhcpv4_lease_cb_t cb,
				    void *user_data)
{
	int slot = -1;
	int ret = 0;

	k_mutex_lock(&server_lock, K_FOREVER);

	if (iface == NULL) {
		for (int i = 0; i < ARRAY_SIZE(server_ctx); i++) {
			if (server_ctx[i].iface != NULL) {
				dhcpv4_server_foreach_lease_on_ctx(
						&server_ctx[i], cb, user_data);
			}
		}

		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(server_ctx); i++) {
		if (server_ctx[i].iface == iface) {
			slot = i;
			break;
		}
	}

	if (slot < 0) {
		ret = -ENOENT;
		goto out;
	}

	dhcpv4_server_foreach_lease_on_ctx(&server_ctx[slot], cb, user_data);

out:
	k_mutex_unlock(&server_lock);

	return ret;
}

void net_dhcpv4_server_init(void)
{
	for (int i = 0; i < ARRAY_SIZE(fds); i++) {
		fds[i].fd = -1;
	}
}
