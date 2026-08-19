/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief DHCPv6 server implementation (RFC 8415)
 *
 * A compact DHCPv6 server supporting non-temporary address assignment (IA_NA)
 * and prefix delegation (IA_PD). Bindings are keyed by client DUID and are
 * statically allocated; a single binding can carry both an IA_NA address and
 * an IA_PD prefix (each with its own IA identifier) for the same client.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dhcpv6_server, CONFIG_NET_DHCPV6_SERVER_LOG_LEVEL);

#include <zephyr/net/dhcpv6_server.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/mld.h>
#include <string.h>

#include "dhcpv6_internal.h"
#include "net_private.h"
#include "udp_internal.h"
#include "ipv6.h"

#define PKT_WAIT_TIME K_MSEC(100)

#define DHCPV6_SERVER_PREFERENCE 255

/* How long a binding that was only advertised (in response to a Solicit) is
 * kept before it is reclaimed. A client that is interested in the offer
 * follows up with a Request well within this time, so an unanswered offer
 * must not hold on to a lease for the whole valid lifetime.
 */
#define DHCPV6_SERVER_OFFER_TIMEOUT_MS (60 * MSEC_PER_SEC)

/* All_DHCP_Relay_Agents_and_Servers (ff02::1:2). Clients send Solicit, Request,
 * Renew (etc.) to this group, so the server interface must join it to receive
 * them (RFC 8415 ch. 7.1).
 */
static const struct net_in6_addr all_dhcpv6_ra_and_servers = {
	{ { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0, 0x02 } }
};

struct dhcpv6_server_binding {
	bool in_use;
	uint8_t duid[sizeof(struct net_dhcpv6_duid_raw)];
	uint8_t duid_len;

	bool has_addr;
	uint32_t addr_iaid;
	struct net_in6_addr addr;

	bool has_prefix;
	uint32_t prefix_iaid;
	struct net_in6_addr prefix;
	uint8_t prefix_len;

	int64_t expiry; /* absolute uptime in ms, 0 = never bound */
};

struct dhcpv6_server_ctx {
	bool in_use;
	struct net_if *iface;
	struct net_dhcpv6_server_params params;
	struct net_dhcpv6_duid_storage serverid;
	struct net_conn_handle *conn;
	struct dhcpv6_server_binding bindings[CONFIG_NET_DHCPV6_SERVER_MAX_LEASES];
};

/* A single server instance is supported. */
static struct dhcpv6_server_ctx server_ctx;
static K_MUTEX_DEFINE(server_lock);

/* Parsed representation of an incoming client message. */
struct dhcpv6_server_msg {
	uint8_t type;
	uint8_t tid[DHCPV6_TID_SIZE];

	bool has_clientid;
	uint8_t clientid[sizeof(struct net_dhcpv6_duid_raw)];
	uint8_t clientid_len;

	bool has_serverid;
	uint8_t serverid[sizeof(struct net_dhcpv6_duid_raw)];
	uint8_t serverid_len;

	bool has_ia_na;
	uint32_t ia_na_iaid;

	bool has_ia_pd;
	uint32_t ia_pd_iaid;

	/* Number of IA Address options the client listed in its IAs, and
	 * whether all of them belong to the link the server serves. Used to
	 * answer Confirm, see dhcpv6_server_addr_on_link().
	 */
	uint8_t ia_addr_count;
	bool ia_addrs_on_link;
};

/* What a reply carries in addition to the mandatory Server ID and Client ID. */
struct dhcpv6_server_reply_opts {
	/** Include an IA_NA option with the leased address. */
	bool include_addr;
	/** Include an IA_PD option with the delegated prefix. */
	bool include_prefix;
	/** Include a Preference option. */
	bool include_preference;
	/** Include a top level Status Code option carrying @ref status. */
	bool include_status;
	/** Top level status, only used if @ref include_status is set. */
	uint16_t status;
	/** Status reported in an empty IA_NA when no address is included,
	 *  DHCPV6_STATUS_SUCCESS to leave the IA_NA out altogether.
	 */
	uint16_t ia_na_status;
	/** Status reported in an empty IA_PD when no prefix is included,
	 *  DHCPV6_STATUS_SUCCESS to leave the IA_PD out altogether.
	 */
	uint16_t ia_pd_status;
};

static uint32_t dhcpv6_server_valid_lifetime(void)
{
	if (server_ctx.params.valid_lifetime != 0) {
		return server_ctx.params.valid_lifetime;
	}

	return CONFIG_NET_DHCPV6_SERVER_VALID_LIFETIME;
}

static uint32_t dhcpv6_server_preferred_lifetime(void)
{
	if (server_ctx.params.preferred_lifetime != 0) {
		return server_ctx.params.preferred_lifetime;
	}

	return CONFIG_NET_DHCPV6_SERVER_PREFERRED_LIFETIME;
}

static void dhcpv6_server_generate_serverid(struct net_if *iface)
{
	struct net_linkaddr *lladdr = net_if_get_link_addr(iface);
	struct net_dhcpv6_duid_storage *serverid = &server_ctx.serverid;
	struct dhcpv6_duid_ll *duid_ll =
				(struct dhcpv6_duid_ll *)&serverid->duid.buf;

	memset(serverid, 0, sizeof(*serverid));

	UNALIGNED_PUT(net_htons(DHCPV6_DUID_TYPE_LL),
		      UNALIGNED_MEMBER_ADDR(serverid, duid.type));
	UNALIGNED_PUT(net_htons(DHCPV6_HARDWARE_ETHERNET_TYPE),
		      UNALIGNED_MEMBER_ADDR(duid_ll, hw_type));
	memcpy(duid_ll->ll_addr, lladdr->addr, lladdr->len);

	serverid->length = DHCPV6_DUID_LL_HEADER_SIZE + lladdr->len;
}

static void dhcpv6_server_free_binding(struct dhcpv6_server_binding *b);

/* Reclaim the bindings whose lease has expired. Without this the (statically
 * allocated) pool would be exhausted by the first
 * CONFIG_NET_DHCPV6_SERVER_MAX_LEASES clients until the server is restarted.
 */
static void dhcpv6_server_reap_bindings(void)
{
	int64_t now = k_uptime_get();

	ARRAY_FOR_EACH(server_ctx.bindings, i) {
		struct dhcpv6_server_binding *b = &server_ctx.bindings[i];

		if (!b->in_use || b->expiry == 0 || b->expiry > now) {
			continue;
		}

		NET_DBG("Reclaiming expired DHCPv6 lease %zu", i);

		dhcpv6_server_free_binding(b);
	}
}

static struct dhcpv6_server_binding *
dhcpv6_server_find_binding(const uint8_t *duid, uint8_t duid_len)
{
	ARRAY_FOR_EACH(server_ctx.bindings, i) {
		struct dhcpv6_server_binding *b = &server_ctx.bindings[i];

		if (b->in_use && b->duid_len == duid_len &&
		    memcmp(b->duid, duid, duid_len) == 0) {
			return b;
		}
	}

	return NULL;
}

static struct dhcpv6_server_binding *dhcpv6_server_alloc_binding(void)
{
	ARRAY_FOR_EACH(server_ctx.bindings, i) {
		if (!server_ctx.bindings[i].in_use) {
			memset(&server_ctx.bindings[i], 0,
			       sizeof(server_ctx.bindings[i]));
			server_ctx.bindings[i].in_use = true;
			return &server_ctx.bindings[i];
		}
	}

	return NULL;
}

static void dhcpv6_server_free_binding(struct dhcpv6_server_binding *b)
{
	memset(b, 0, sizeof(*b));
}

/* Derive a unique address/prefix for a binding from the configured pool base,
 * using the binding's slot index as the varying part.
 */
static uint8_t dhcpv6_server_binding_index(struct dhcpv6_server_binding *b)
{
	return (uint8_t)(b - &server_ctx.bindings[0]);
}

static void dhcpv6_server_assign_addr(struct dhcpv6_server_binding *b,
				      uint32_t iaid)
{
	uint8_t idx = dhcpv6_server_binding_index(b);

	net_ipaddr_copy(&b->addr, &server_ctx.params.addr);
	/* Vary the lowest byte, offset to avoid the ::0 host id. */
	b->addr.s6_addr[15] += (uint8_t)(idx + 1U);
	b->addr_iaid = iaid;
	b->has_addr = true;
}

static void dhcpv6_server_assign_prefix(struct dhcpv6_server_binding *b,
					uint32_t iaid)
{
	uint8_t idx = dhcpv6_server_binding_index(b);
	uint8_t prefix_len = server_ctx.params.prefix_len;
	uint8_t subnet_byte;

	net_ipaddr_copy(&b->prefix, &server_ctx.params.prefix);

	/* Place the varying subnet id in the last byte of the prefix (e.g.
	 * byte 7 for a /64). prefix_len is guaranteed byte-aligned and in
	 * 8..128 by net_dhcpv6_server_start(), so this stays within the
	 * delegated prefix.
	 */
	subnet_byte = (uint8_t)((prefix_len - 1U) / 8U);
	b->prefix.s6_addr[subnet_byte] += idx;
	b->prefix_len = prefix_len;
	b->prefix_iaid = iaid;
	b->has_prefix = true;
}

static int dhcpv6_server_read_duid(struct net_pkt *pkt, uint16_t length,
				   uint8_t *duid, uint8_t *duid_len)
{
	if (length > sizeof(struct net_dhcpv6_duid_raw)) {
		return -EMSGSIZE;
	}

	if (net_pkt_read(pkt, duid, length) < 0) {
		return -EBADMSG;
	}

	*duid_len = (uint8_t)length;

	return 0;
}

/* The address pool the server hands out from is a single /64 on the link the
 * server runs on, so an address is appropriate for that link exactly when it
 * shares the pool prefix.
 */
#define DHCPV6_SERVER_LINK_PREFIX_LEN 64

static bool dhcpv6_server_addr_on_link(const struct net_in6_addr *addr)
{
	return net_ipv6_is_prefix(addr->s6_addr, server_ctx.params.addr.s6_addr,
				  DHCPV6_SERVER_LINK_PREFIX_LEN);
}

/* Read an IA_NA/IA_PD option. Only the IA identifier is needed to hand out a
 * lease, but the addresses the client lists are inspected as well because
 * Confirm is answered based on them.
 */
static int dhcpv6_server_read_ia(struct net_pkt *pkt, uint16_t length,
				 struct dhcpv6_server_msg *msg, uint32_t *iaid)
{
	uint16_t remaining;

	if (length < DHCPV6_OPTION_IA_NA_HEADER_SIZE) {
		return -EMSGSIZE;
	}

	/* IA identifier, followed by t1 and t2 which the server ignores. */
	if (net_pkt_read_be32(pkt, iaid) < 0 ||
	    net_pkt_skip(pkt, 2 * sizeof(uint32_t)) < 0) {
		return -EBADMSG;
	}

	remaining = length - DHCPV6_OPTION_IA_NA_HEADER_SIZE;

	while (remaining >= DHCPV6_OPTION_HEADER_SIZE) {
		struct net_in6_addr addr;
		uint16_t code;
		uint16_t sub_length;

		if (net_pkt_read_be16(pkt, &code) < 0 ||
		    net_pkt_read_be16(pkt, &sub_length) < 0) {
			return -EBADMSG;
		}

		remaining -= DHCPV6_OPTION_HEADER_SIZE;
		if (sub_length > remaining) {
			return -EBADMSG;
		}

		remaining -= sub_length;

		if (code != DHCPV6_OPTION_CODE_IAADDR ||
		    sub_length < DHCPV6_OPTION_IAADDR_HEADER_SIZE) {
			if (net_pkt_skip(pkt, sub_length) < 0) {
				return -EBADMSG;
			}

			continue;
		}

		if (net_pkt_read(pkt, &addr, sizeof(addr)) < 0 ||
		    net_pkt_skip(pkt, sub_length - sizeof(addr)) < 0) {
			return -EBADMSG;
		}

		if (!dhcpv6_server_addr_on_link(&addr)) {
			msg->ia_addrs_on_link = false;
		}

		if (msg->ia_addr_count < UINT8_MAX) {
			msg->ia_addr_count++;
		}
	}

	/* Trailing bytes that are too short to hold an option header. */
	if (remaining > 0U && net_pkt_skip(pkt, remaining) < 0) {
		return -EBADMSG;
	}

	return 0;
}

static int dhcpv6_server_parse(struct net_pkt *pkt,
			       struct dhcpv6_server_msg *msg)
{
	memset(msg, 0, sizeof(*msg));

	/* Cleared again by the first address that is not on this link. */
	msg->ia_addrs_on_link = true;

	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, NET_IPV6UDPH_LEN) < 0) {
		return -EMSGSIZE;
	}

	if (net_pkt_read_u8(pkt, &msg->type) < 0 ||
	    net_pkt_read(pkt, msg->tid, sizeof(msg->tid)) < 0) {
		return -EMSGSIZE;
	}

	while (true) {
		uint16_t code;
		uint16_t length;

		if (net_pkt_read_be16(pkt, &code) < 0) {
			break;
		}

		if (net_pkt_read_be16(pkt, &length) < 0) {
			break;
		}

		switch (code) {
		case DHCPV6_OPTION_CODE_CLIENTID:
			if (dhcpv6_server_read_duid(pkt, length, msg->clientid,
						    &msg->clientid_len) < 0) {
				return -EBADMSG;
			}
			msg->has_clientid = true;
			break;

		case DHCPV6_OPTION_CODE_SERVERID:
			if (dhcpv6_server_read_duid(pkt, length, msg->serverid,
						    &msg->serverid_len) < 0) {
				return -EBADMSG;
			}
			msg->has_serverid = true;
			break;

		case DHCPV6_OPTION_CODE_IA_NA:
			if (dhcpv6_server_read_ia(pkt, length, msg,
						  &msg->ia_na_iaid) < 0) {
				return -EBADMSG;
			}
			msg->has_ia_na = true;
			break;

		case DHCPV6_OPTION_CODE_IA_PD:
			if (dhcpv6_server_read_ia(pkt, length, msg,
						  &msg->ia_pd_iaid) < 0) {
				return -EBADMSG;
			}
			msg->has_ia_pd = true;
			break;

		default:
			if (net_pkt_skip(pkt, length) < 0) {
				return -EMSGSIZE;
			}
			break;
		}
	}

	return 0;
}

static int dhcpv6_server_add_option_header(struct net_pkt *pkt, uint16_t code,
					   uint16_t length)
{
	if (net_pkt_write_be16(pkt, code) < 0 ||
	    net_pkt_write_be16(pkt, length) < 0) {
		return -ENOBUFS;
	}

	return 0;
}

static int dhcpv6_server_add_duid(struct net_pkt *pkt, uint16_t code,
				  const uint8_t *duid, uint8_t duid_len)
{
	if (dhcpv6_server_add_option_header(pkt, code, duid_len) < 0 ||
	    net_pkt_write(pkt, duid, duid_len) < 0) {
		return -ENOBUFS;
	}

	return 0;
}

static int dhcpv6_server_add_ia_na(struct net_pkt *pkt,
				   struct dhcpv6_server_binding *b)
{
	uint32_t valid = dhcpv6_server_valid_lifetime();
	uint32_t preferred = dhcpv6_server_preferred_lifetime();
	uint16_t iaaddr_len = DHCPV6_OPTION_HEADER_SIZE +
			      DHCPV6_OPTION_IAADDR_HEADER_SIZE;
	uint16_t ia_na_len = DHCPV6_OPTION_IA_NA_HEADER_SIZE + iaaddr_len;

	if (dhcpv6_server_add_option_header(pkt, DHCPV6_OPTION_CODE_IA_NA,
					    ia_na_len) < 0) {
		return -ENOBUFS;
	}

	if (net_pkt_write_be32(pkt, b->addr_iaid) < 0 ||
	    net_pkt_write_be32(pkt, valid / 2U) < 0 ||        /* T1 */
	    net_pkt_write_be32(pkt, (valid * 4U) / 5U) < 0) { /* T2 */
		return -ENOBUFS;
	}

	if (dhcpv6_server_add_option_header(pkt, DHCPV6_OPTION_CODE_IAADDR,
					    DHCPV6_OPTION_IAADDR_HEADER_SIZE) < 0) {
		return -ENOBUFS;
	}

	if (net_pkt_write(pkt, &b->addr, sizeof(b->addr)) < 0 ||
	    net_pkt_write_be32(pkt, preferred) < 0 ||
	    net_pkt_write_be32(pkt, valid) < 0) {
		return -ENOBUFS;
	}

	return 0;
}

static int dhcpv6_server_add_ia_pd(struct net_pkt *pkt,
				   struct dhcpv6_server_binding *b)
{
	uint32_t valid = dhcpv6_server_valid_lifetime();
	uint32_t preferred = dhcpv6_server_preferred_lifetime();
	uint16_t iaprefix_len = DHCPV6_OPTION_HEADER_SIZE +
				DHCPV6_OPTION_IAPREFIX_HEADER_SIZE;
	uint16_t ia_pd_len = DHCPV6_OPTION_IA_PD_HEADER_SIZE + iaprefix_len;

	if (dhcpv6_server_add_option_header(pkt, DHCPV6_OPTION_CODE_IA_PD,
					    ia_pd_len) < 0) {
		return -ENOBUFS;
	}

	if (net_pkt_write_be32(pkt, b->prefix_iaid) < 0 ||
	    net_pkt_write_be32(pkt, valid / 2U) < 0 ||        /* T1 */
	    net_pkt_write_be32(pkt, (valid * 4U) / 5U) < 0) { /* T2 */
		return -ENOBUFS;
	}

	if (dhcpv6_server_add_option_header(pkt, DHCPV6_OPTION_CODE_IAPREFIX,
					    DHCPV6_OPTION_IAPREFIX_HEADER_SIZE) < 0) {
		return -ENOBUFS;
	}

	if (net_pkt_write_be32(pkt, preferred) < 0 ||
	    net_pkt_write_be32(pkt, valid) < 0 ||
	    net_pkt_write_u8(pkt, b->prefix_len) < 0 ||
	    net_pkt_write(pkt, &b->prefix, sizeof(b->prefix)) < 0) {
		return -ENOBUFS;
	}

	return 0;
}

static int dhcpv6_server_add_status(struct net_pkt *pkt, uint16_t status)
{
	/* The status message is optional (RFC 8415 ch. 21.13) and is left out
	 * here to keep the reply small.
	 */
	if (dhcpv6_server_add_option_header(pkt, DHCPV6_OPTION_CODE_STATUS_CODE,
					    DHCPV6_OPTION_STATUS_CODE_HEADER_SIZE) < 0 ||
	    net_pkt_write_be16(pkt, status) < 0) {
		return -ENOBUFS;
	}

	return 0;
}

/* Add an IA_NA/IA_PD option that carries no address or prefix but tells the
 * client why, RFC 8415 ch. 18.3.2.
 */
static int dhcpv6_server_add_ia_status(struct net_pkt *pkt, uint16_t code,
				       uint32_t iaid, uint16_t status)
{
	uint16_t status_len = DHCPV6_OPTION_HEADER_SIZE +
			      DHCPV6_OPTION_STATUS_CODE_HEADER_SIZE;
	uint16_t ia_len = DHCPV6_OPTION_IA_NA_HEADER_SIZE + status_len;

	if (dhcpv6_server_add_option_header(pkt, code, ia_len) < 0) {
		return -ENOBUFS;
	}

	if (net_pkt_write_be32(pkt, iaid) < 0 ||
	    net_pkt_write_be32(pkt, 0U) < 0 ||  /* T1 */
	    net_pkt_write_be32(pkt, 0U) < 0) {  /* T2 */
		return -ENOBUFS;
	}

	return dhcpv6_server_add_status(pkt, status);
}

static int dhcpv6_server_add_preference(struct net_pkt *pkt, uint8_t value)
{
	if (dhcpv6_server_add_option_header(pkt, DHCPV6_OPTION_CODE_PREFERENCE,
					    DHCPV6_OPTION_PREFERENCE_SIZE) < 0 ||
	    net_pkt_write_u8(pkt, value) < 0) {
		return -ENOBUFS;
	}

	return 0;
}

static int dhcpv6_server_send_reply(struct net_if *iface,
				    const struct net_in6_addr *dst,
				    enum dhcpv6_msg_type reply_type,
				    struct dhcpv6_server_msg *req,
				    struct dhcpv6_server_binding *b,
				    struct dhcpv6_server_reply_opts *opts)
{
	size_t ia_status_size = DHCPV6_OPTION_HEADER_SIZE +
				DHCPV6_OPTION_IA_NA_HEADER_SIZE +
				DHCPV6_OPTION_HEADER_SIZE +
				DHCPV6_OPTION_STATUS_CODE_HEADER_SIZE;
	const struct net_in6_addr *src;
	struct net_pkt *pkt;
	size_t msg_size;
	int ret = -ENOBUFS;

	src = net_if_ipv6_get_ll(iface, NET_ADDR_PREFERRED);
	if (src == NULL) {
		src = net_if_ipv6_select_src_addr(iface, dst);
	}

	msg_size = DHCPV6_HEADER_SIZE;
	msg_size += DHCPV6_OPTION_HEADER_SIZE + server_ctx.serverid.length;
	msg_size += DHCPV6_OPTION_HEADER_SIZE + req->clientid_len;
	if (opts->include_preference) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE +
			    DHCPV6_OPTION_PREFERENCE_SIZE;
	}
	if (opts->include_status) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE +
			    DHCPV6_OPTION_STATUS_CODE_HEADER_SIZE;
	}
	if (opts->include_addr) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE +
			    DHCPV6_OPTION_IA_NA_HEADER_SIZE +
			    DHCPV6_OPTION_HEADER_SIZE +
			    DHCPV6_OPTION_IAADDR_HEADER_SIZE;
	} else if (opts->ia_na_status != DHCPV6_STATUS_SUCCESS) {
		msg_size += ia_status_size;
	}
	if (opts->include_prefix) {
		msg_size += DHCPV6_OPTION_HEADER_SIZE +
			    DHCPV6_OPTION_IA_PD_HEADER_SIZE +
			    DHCPV6_OPTION_HEADER_SIZE +
			    DHCPV6_OPTION_IAPREFIX_HEADER_SIZE;
	} else if (opts->ia_pd_status != DHCPV6_STATUS_SUCCESS) {
		msg_size += ia_status_size;
	}

	pkt = net_pkt_alloc_with_buffer(iface, msg_size, NET_AF_INET6,
					NET_IPPROTO_UDP, PKT_WAIT_TIME);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	if (net_ipv6_create(pkt, src, dst) < 0 ||
	    net_udp_create(pkt, net_htons(DHCPV6_SERVER_PORT),
			   net_htons(DHCPV6_CLIENT_PORT)) < 0) {
		goto fail;
	}

	if (net_pkt_write_u8(pkt, reply_type) < 0 ||
	    net_pkt_write(pkt, req->tid, sizeof(req->tid)) < 0) {
		goto fail;
	}

	if (dhcpv6_server_add_duid(pkt, DHCPV6_OPTION_CODE_SERVERID,
				   (uint8_t *)&server_ctx.serverid.duid,
				   server_ctx.serverid.length) < 0) {
		goto fail;
	}

	if (dhcpv6_server_add_duid(pkt, DHCPV6_OPTION_CODE_CLIENTID,
				   req->clientid, req->clientid_len) < 0) {
		goto fail;
	}

	if (opts->include_preference &&
	    dhcpv6_server_add_preference(pkt, DHCPV6_SERVER_PREFERENCE) < 0) {
		goto fail;
	}

	if (opts->include_status &&
	    dhcpv6_server_add_status(pkt, opts->status) < 0) {
		goto fail;
	}

	if (opts->include_addr) {
		if (dhcpv6_server_add_ia_na(pkt, b) < 0) {
			goto fail;
		}
	} else if (opts->ia_na_status != DHCPV6_STATUS_SUCCESS) {
		if (dhcpv6_server_add_ia_status(pkt, DHCPV6_OPTION_CODE_IA_NA,
						req->ia_na_iaid,
						opts->ia_na_status) < 0) {
			goto fail;
		}
	}

	if (opts->include_prefix) {
		if (dhcpv6_server_add_ia_pd(pkt, b) < 0) {
			goto fail;
		}
	} else if (opts->ia_pd_status != DHCPV6_STATUS_SUCCESS) {
		if (dhcpv6_server_add_ia_status(pkt, DHCPV6_OPTION_CODE_IA_PD,
						req->ia_pd_iaid,
						opts->ia_pd_status) < 0) {
			goto fail;
		}
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, NET_IPPROTO_UDP);

	ret = net_send_data(pkt);
	if (ret < 0) {
		goto fail;
	}

	return 0;

fail:
	net_pkt_unref(pkt);

	return ret;
}

static bool dhcpv6_server_for_us(struct dhcpv6_server_msg *msg)
{
	if (!msg->has_serverid) {
		return false;
	}

	return msg->serverid_len == server_ctx.serverid.length &&
	       memcmp(msg->serverid, &server_ctx.serverid.duid,
		      server_ctx.serverid.length) == 0;
}

/* Look up (or create) the binding for the client that sent @p msg. A binding
 * created for a Solicit is not committed, i.e. it only holds the offer until
 * the client confirms it with a Request.
 */
static struct dhcpv6_server_binding *
dhcpv6_server_bind(struct dhcpv6_server_msg *msg, bool committed)
{
	struct dhcpv6_server_binding *b;

	b = dhcpv6_server_find_binding(msg->clientid, msg->clientid_len);
	if (b == NULL) {
		b = dhcpv6_server_alloc_binding();
		if (b == NULL) {
			NET_WARN("No free DHCPv6 lease available");
			return NULL;
		}

		memcpy(b->duid, msg->clientid, msg->clientid_len);
		b->duid_len = msg->clientid_len;
	}

	if (msg->has_ia_na && server_ctx.params.offer_addr) {
		dhcpv6_server_assign_addr(b, msg->ia_na_iaid);
	}

	if (msg->has_ia_pd && server_ctx.params.offer_prefix) {
		dhcpv6_server_assign_prefix(b, msg->ia_pd_iaid);
	}

	if (committed) {
		b->expiry = k_uptime_get() +
			    (int64_t)dhcpv6_server_valid_lifetime() * MSEC_PER_SEC;
	} else {
		/* Only advertised so far, keep the binding just long enough for
		 * the client to come back with a Request.
		 */
		b->expiry = k_uptime_get() + DHCPV6_SERVER_OFFER_TIMEOUT_MS;
	}

	return b;
}

/* Tell the client why a requested IA is not part of the reply, RFC 8415
 * ch. 18.3.2.
 */
static void dhcpv6_server_set_ia_status(struct dhcpv6_server_msg *msg,
					struct dhcpv6_server_reply_opts *opts)
{
	/* Either the pool is exhausted or the server is not configured to hand
	 * out this kind of resource. Both are reported the same way, the client
	 * only needs to know that it is not getting one.
	 */
	if (msg->has_ia_na && !opts->include_addr) {
		opts->ia_na_status = DHCPV6_STATUS_NO_ADDR_AVAIL;
	}

	if (msg->has_ia_pd && !opts->include_prefix) {
		opts->ia_pd_status = DHCPV6_STATUS_NO_PREFIX_AVAIL;
	}
}

/* Tell the client that the IAs it wants to extend are not known to this
 * server, RFC 8415 ch. 18.3.4.
 */
static void dhcpv6_server_set_no_binding_status(struct dhcpv6_server_msg *msg,
						struct dhcpv6_server_reply_opts *opts)
{
	if (msg->has_ia_na) {
		opts->ia_na_status = DHCPV6_STATUS_NO_BINDING;
	}

	if (msg->has_ia_pd) {
		opts->ia_pd_status = DHCPV6_STATUS_NO_BINDING;
	}
}

enum dhcpv6_server_action {
	/** Do not answer the message. */
	DHCPV6_SERVER_ACTION_DROP,
	/** Hand out (or extend) the lease and reply with the IAs. */
	DHCPV6_SERVER_ACTION_GRANT,
	/** Reply with empty IAs carrying the NoBinding status. */
	DHCPV6_SERVER_ACTION_NO_BINDING,
};

/* Decide how a Renew or Rebind is answered. Both ask to extend a lease, but
 * they differ in what the server owes a client it has no binding for.
 */
static enum dhcpv6_server_action
dhcpv6_server_extend_action(uint8_t type, bool have_binding)
{
	if (have_binding) {
		return DHCPV6_SERVER_ACTION_GRANT;
	}

	if (type == DHCPV6_MSG_TYPE_RENEW) {
		/* RFC 8415 ch. 18.3.4: the client asked this very server to
		 * extend a lease it does not have, so tell it so instead of
		 * silently creating a new one. The client then falls back to
		 * Solicit/Request.
		 */
		return DHCPV6_SERVER_ACTION_NO_BINDING;
	}

	/* Rebind, RFC 8415 ch. 18.3.5: with no record of the client, the
	 * server would have to tell from the client's addresses whether they
	 * belong to this link. It may then either invalidate them or silently
	 * discard the message; the latter is chosen here, the client recovers
	 * by soliciting a new lease.
	 */
	return DHCPV6_SERVER_ACTION_DROP;
}

/* Hand out a lease for the IAs in the message and reply with what could be
 * assigned.
 */
static void dhcpv6_server_grant(struct net_if *iface,
				const struct net_in6_addr *src,
				struct dhcpv6_server_msg *msg)
{
	struct dhcpv6_server_reply_opts opts = { 0 };
	struct dhcpv6_server_binding *b;

	b = dhcpv6_server_bind(msg, true);
	if (b != NULL) {
		opts.include_addr = b->has_addr;
		opts.include_prefix = b->has_prefix;
	}

	dhcpv6_server_set_ia_status(msg, &opts);

	(void)dhcpv6_server_send_reply(iface, src, DHCPV6_MSG_TYPE_REPLY, msg,
				       b, &opts);
}

static void dhcpv6_server_handle(struct net_if *iface,
				 const struct net_in6_addr *src,
				 struct dhcpv6_server_msg *msg)
{
	struct dhcpv6_server_reply_opts opts = { 0 };
	struct dhcpv6_server_binding *b;
	enum dhcpv6_server_action action;

	if (!msg->has_clientid) {
		NET_DBG("Client message without Client ID, ignoring");
		return;
	}

	/* Make the leases that have expired available again before handing
	 * out a new one.
	 */
	dhcpv6_server_reap_bindings();

	switch (msg->type) {
	case DHCPV6_MSG_TYPE_SOLICIT:
		opts.include_preference = true;

		b = dhcpv6_server_bind(msg, false);
		if (b != NULL) {
			opts.include_addr = b->has_addr;
			opts.include_prefix = b->has_prefix;
		}

		dhcpv6_server_set_ia_status(msg, &opts);

		(void)dhcpv6_server_send_reply(iface, src,
					       DHCPV6_MSG_TYPE_ADVERTISE, msg, b,
					       &opts);
		break;

	case DHCPV6_MSG_TYPE_REQUEST:
		if (!dhcpv6_server_for_us(msg)) {
			return;
		}

		dhcpv6_server_grant(iface, src, msg);
		break;

	case DHCPV6_MSG_TYPE_RENEW:
		if (!dhcpv6_server_for_us(msg)) {
			return;
		}
		__fallthrough;
	case DHCPV6_MSG_TYPE_REBIND:
		/* Rebind is multicast and carries no Server ID. */
		b = dhcpv6_server_find_binding(msg->clientid, msg->clientid_len);
		action = dhcpv6_server_extend_action(msg->type, b != NULL);

		if (action == DHCPV6_SERVER_ACTION_DROP) {
			NET_DBG("No binding to extend, ignoring message");
			return;
		}

		if (action == DHCPV6_SERVER_ACTION_GRANT) {
			dhcpv6_server_grant(iface, src, msg);
			break;
		}

		dhcpv6_server_set_no_binding_status(msg, &opts);

		(void)dhcpv6_server_send_reply(iface, src,
					       DHCPV6_MSG_TYPE_REPLY, msg, NULL,
					       &opts);
		break;

	case DHCPV6_MSG_TYPE_CONFIRM:
		/* Confirm asks whether the addresses the client already has are
		 * still appropriate for the link it is attached to. A server
		 * that cannot answer that question - which includes a Confirm
		 * that lists no addresses at all - stays quiet rather than
		 * guessing, RFC 8415 ch. 18.3.3.
		 */
		if (msg->ia_addr_count == 0U) {
			NET_DBG("Confirm without addresses, not replying");
			return;
		}

		/* The Reply carries the status and nothing else, the client
		 * only needs to know whether it may keep its addresses.
		 */
		opts.include_status = true;
		opts.status = msg->ia_addrs_on_link ? DHCPV6_STATUS_SUCCESS :
						      DHCPV6_STATUS_NOT_ON_LINK;

		(void)dhcpv6_server_send_reply(iface, src,
					       DHCPV6_MSG_TYPE_REPLY, msg, NULL,
					       &opts);
		break;

	case DHCPV6_MSG_TYPE_RELEASE:
	case DHCPV6_MSG_TYPE_DECLINE:
		if (!dhcpv6_server_for_us(msg)) {
			return;
		}

		b = dhcpv6_server_find_binding(msg->clientid, msg->clientid_len);

		/* The reply tells the client whether the server knew about the
		 * binding at all, RFC 8415 ch. 18.3.7 and 18.3.8.
		 */
		opts.include_status = true;
		opts.status = (b != NULL) ? DHCPV6_STATUS_SUCCESS :
					    DHCPV6_STATUS_NO_BINDING;

		/* Reply before freeing so that the client's DUID is still
		 * available for the echoed Client ID option.
		 */
		(void)dhcpv6_server_send_reply(iface, src,
					       DHCPV6_MSG_TYPE_REPLY, msg, b,
					       &opts);

		if (b != NULL) {
			dhcpv6_server_free_binding(b);
		}
		break;

	default:
		NET_DBG("Unhandled DHCPv6 message type %d", msg->type);
		break;
	}
}

static enum net_verdict dhcpv6_server_input(struct net_conn *conn,
					    struct net_pkt *pkt,
					    union net_ip_header *ip_hdr,
					    union net_proto_header *proto_hdr,
					    void *user_data)
{
	struct dhcpv6_server_msg msg;
	struct net_if *iface;
	struct net_in6_addr src;

	ARG_UNUSED(conn);
	ARG_UNUSED(proto_hdr);
	ARG_UNUSED(user_data);

	if (pkt == NULL || ip_hdr == NULL || ip_hdr->ipv6 == NULL) {
		return NET_DROP;
	}

	iface = net_pkt_iface(pkt);

	k_mutex_lock(&server_lock, K_FOREVER);

	if (!server_ctx.in_use || server_ctx.iface != iface) {
		k_mutex_unlock(&server_lock);
		return NET_DROP;
	}

	if (dhcpv6_server_parse(pkt, &msg) < 0) {
		NET_DBG("Failed to parse DHCPv6 client message");
		k_mutex_unlock(&server_lock);
		return NET_DROP;
	}

	net_ipv6_addr_copy_raw(src.s6_addr, ip_hdr->ipv6->src);

	dhcpv6_server_handle(iface, &src, &msg);

	k_mutex_unlock(&server_lock);

	net_pkt_unref(pkt);

	return NET_OK;
}

static void dhcpv6_server_join_mcast(struct net_if *iface)
{
	int ret;

	if (net_if_ipv6_maddr_lookup(&all_dhcpv6_ra_and_servers, &iface) !=
	    NULL) {
		return;
	}

	ret = net_ipv6_mld_join(iface, &all_dhcpv6_ra_and_servers);
	if (ret == -ENOTSUP) {
		/* MLD disabled: still register the address locally so that
		 * messages sent to it are accepted by the input path.
		 */
		(void)net_if_ipv6_maddr_add(iface, &all_dhcpv6_ra_and_servers);
	} else if (ret < 0 && ret != -ENETDOWN && ret != -EALREADY) {
		NET_ERR("Cannot join All_DHCP_Relay_Agents_and_Servers for "
			"iface %d (%d)", net_if_get_by_iface(iface), ret);
	}
}

static void dhcpv6_server_leave_mcast(struct net_if *iface)
{
	if (net_if_ipv6_maddr_lookup(&all_dhcpv6_ra_and_servers, &iface) ==
	    NULL) {
		return;
	}

	if (net_ipv6_mld_leave(iface, &all_dhcpv6_ra_and_servers) == -ENOTSUP) {
		(void)net_if_ipv6_maddr_rm(iface, &all_dhcpv6_ra_and_servers);
	}
}

int net_dhcpv6_server_start(struct net_if *iface,
			    struct net_dhcpv6_server_params *params)
{
	struct net_sockaddr unspec_addr = { 0 };
	int ret;

	if (iface == NULL || params == NULL) {
		return -EINVAL;
	}

	if (!params->offer_addr && !params->offer_prefix) {
		return -EINVAL;
	}

	/* The prefix allocator derives per-client subnets by varying a whole
	 * byte of the base prefix (see dhcpv6_server_assign_prefix()), so it
	 * only supports byte-aligned prefix lengths. A non-byte-aligned length
	 * would alter bits outside the delegated prefix and could hand out
	 * prefixes outside the configured pool, so reject it here.
	 */
	if (params->offer_prefix &&
	    (params->prefix_len < 8U || params->prefix_len > 128U ||
	     (params->prefix_len % 8U) != 0U)) {
		NET_ERR("Unsupported IA_PD prefix length %u (must be a multiple "
			"of 8, 8..128)", params->prefix_len);
		return -EINVAL;
	}

	/* Both allocators derive per-client resources by adding the binding
	 * index to a single byte of the configured base, so make sure that the
	 * whole pool fits in that byte and cannot wrap into an address or
	 * prefix that was never configured.
	 */
	if (params->offer_addr &&
	    (uint32_t)params->addr.s6_addr[15] +
	    CONFIG_NET_DHCPV6_SERVER_MAX_LEASES > UINT8_MAX) {
		NET_ERR("IA_NA pool base %s does not have room for %d leases",
			net_sprint_ipv6_addr(&params->addr),
			CONFIG_NET_DHCPV6_SERVER_MAX_LEASES);
		return -EINVAL;
	}

	if (params->offer_prefix &&
	    (uint32_t)params->prefix.s6_addr[(params->prefix_len - 1U) / 8U] +
	    CONFIG_NET_DHCPV6_SERVER_MAX_LEASES - 1U > UINT8_MAX) {
		NET_ERR("IA_PD pool base %s/%u does not have room for %d "
			"delegations", net_sprint_ipv6_addr(&params->prefix),
			params->prefix_len,
			CONFIG_NET_DHCPV6_SERVER_MAX_LEASES);
		return -EINVAL;
	}

	k_mutex_lock(&server_lock, K_FOREVER);

	if (server_ctx.in_use) {
		k_mutex_unlock(&server_lock);
		return -EALREADY;
	}

	memset(&server_ctx, 0, sizeof(server_ctx));
	server_ctx.iface = iface;
	server_ctx.params = *params;

	dhcpv6_server_generate_serverid(iface);

	net_ipaddr_copy(&net_sin6(&unspec_addr)->sin6_addr,
			net_ipv6_unspecified_address());
	unspec_addr.sa_family = NET_AF_INET6;

	ret = net_udp_register(NET_AF_INET6, NULL, &unspec_addr, 0,
			       DHCPV6_SERVER_PORT, NULL, dhcpv6_server_input,
			       NULL, &server_ctx.conn);
	if (ret < 0) {
		NET_ERR("Failed to register DHCPv6 server listener (%d)", ret);
		k_mutex_unlock(&server_lock);
		return ret;
	}

	server_ctx.in_use = true;

	k_mutex_unlock(&server_lock);

	/* Join the All_DHCP_Relay_Agents_and_Servers group outside server_lock:
	 * net_ipv6_mld_join() transmits an MLD report whose TX path takes
	 * interface locks, so it must not run while holding another lock.
	 */
	dhcpv6_server_join_mcast(iface);

	NET_DBG("DHCPv6 server started on iface %d", net_if_get_by_iface(iface));

	return 0;
}

int net_dhcpv6_server_stop(struct net_if *iface)
{
	k_mutex_lock(&server_lock, K_FOREVER);

	if (!server_ctx.in_use || server_ctx.iface != iface) {
		k_mutex_unlock(&server_lock);
		return -ENOENT;
	}

	if (server_ctx.conn != NULL) {
		(void)net_udp_unregister(server_ctx.conn);
	}

	memset(&server_ctx, 0, sizeof(server_ctx));

	k_mutex_unlock(&server_lock);

	/* Leave the group outside server_lock for the same reason as the join
	 * in net_dhcpv6_server_start() (net_ipv6_mld_leave() transmits).
	 */
	dhcpv6_server_leave_mcast(iface);

	NET_DBG("DHCPv6 server stopped on iface %d", net_if_get_by_iface(iface));

	return 0;
}

int net_dhcpv6_server_foreach_lease(struct net_if *iface,
				    net_dhcpv6_server_lease_cb_t cb,
				    void *user_data)
{
	int ret = 0;

	if (cb == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&server_lock, K_FOREVER);

	if (!server_ctx.in_use ||
	    (iface != NULL && server_ctx.iface != iface)) {
		ret = -ENOENT;
		goto out;
	}

	/* Do not report leases that have already expired. */
	dhcpv6_server_reap_bindings();

	ARRAY_FOR_EACH(server_ctx.bindings, i) {
		struct dhcpv6_server_binding *b = &server_ctx.bindings[i];
		struct net_dhcpv6_server_lease lease;

		if (!b->in_use) {
			continue;
		}

		lease.duid = b->duid;
		lease.duid_len = b->duid_len;
		lease.has_addr = b->has_addr;
		lease.addr = b->addr;
		lease.has_prefix = b->has_prefix;
		lease.prefix = b->prefix;
		lease.prefix_len = b->prefix_len;
		lease.expiry = b->expiry;

		cb(server_ctx.iface, &lease, user_data);
	}

out:
	k_mutex_unlock(&server_lock);

	return ret;
}
