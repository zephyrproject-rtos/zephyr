/** @file
 * @brief DHCPv4 client related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(CONFIG_NET_DEBUG_DHCPV4)
#define SYS_LOG_DOMAIN "net/dhcpv4"
#define NET_DEBUG 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include "net_private.h"

#include "udp.h"
#include <net/dhcpv4.h>

struct dhcp_msg {
	uint8_t op;		/* Message type, 1:BOOTREQUEST, 2:BOOTREPLY */
	uint8_t htype;		/* Hardware Address Type */
	uint8_t hlen;		/* Hardware Address length */
	uint8_t hops;		/* used by relay agents when booting via relay
				 * agent, client sets zero
				 */
	uint8_t xid[4];		/* Transaction ID, random number */
	uint16_t secs;		/* Seconds elapsed since client began address
				 * acqusition or renewal process
				 */
	uint16_t flags;		/* Broadcast or Unicast */
	uint8_t ciaddr[4];	/* Client IP Address */
	uint8_t yiaddr[4];	/* your (client) IP address */
	uint8_t siaddr[4];	/* IP address of next server to use in bootstrap
				 * returned in DHCPOFFER, DHCPACK by server
				 */
	uint8_t giaddr[4];	/* Relat agent IP address */
	uint8_t chaddr[16];	/* Client hardware address */
} __packed;

#define SIZE_OF_SNAME	64
#define SIZE_OF_FILE	128

#define DHCPV4_MSG_BROADCAST	0x8000
#define DHCPV4_MSG_UNICAST	0x0000

#define DHCPV4_MSG_BOOT_REQUEST	1
#define DHCPV4_MSG_BOOT_REPLY	2

#define HARDWARE_ETHERNET_TYPE 1
#define HARDWARE_ETHERNET_LEN  6

#define DHCPV4_SERVER_PORT  67
#define DHCPV4_CLIENT_PORT  68

#define DHCPV4_MSG_TYPE_DISCOVER	1
#define DHCPV4_MSG_TYPE_OFFER		2
#define DHCPV4_MSG_TYPE_REQUEST		3
#define DHCPV4_MSG_TYPE_DECLINE		4
#define DHCPV4_MSG_TYPE_ACK		5
#define DHCPV4_MSG_TYPE_NAK		6
#define DHCPV4_MSG_TYPE_RELEASE		7
#define DHCPV4_MSG_TYPE_INFORM		8

#define DHCPV4_OPTIONS_SUBNET_MASK	1
#define DHCPV4_OPTIONS_ROUTER		3
#define DHCPV4_OPTIONS_DNS_SERVER	6
#define DHCPV4_OPTIONS_REQ_IPADDR	50
#define DHCPV4_OPTIONS_LEASE_TIME	51
#define DHCPV4_OPTIONS_MSG_TYPE		53
#define DHCPV4_OPTIONS_SERVER_ID	54
#define DHCPV4_OPTIONS_REQ_LIST		55
#define DHCPV4_OPTIONS_RENEWAL		58
#define DHCPV4_OPTIONS_END		255

/*
 * TODO:
 * 1) Support for UNICAST flag (some dhcpv4 servers will not reply if
 *    DISCOVER message contains BROADCAST FLAG).
 * 2) Support T2(Rebind) timer.
 */

/*
 * In the process of REQUEST and RENEWAL requests, if server fails to respond
 * with ACK, client will try below number of maximum attempts, if it fails to
 * get reply from server, client starts from beginning (broadcasting DISCOVER
 * message).
 * No limit for DISCOVER message request, it sends forever at the moment.
 */
#define DHCPV4_MAX_NUMBER_OF_ATTEMPTS	3

static uint8_t magic_cookie[4] = { 0x63, 0x82, 0x53, 0x63 }; /* RFC 1497 [17] */

static void dhcpv4_timeout(struct nano_work *work);

/*
 * Timeout for Initialization and allocation of network address.
 * Timeout value is from random number between 1-10 seconds.
 * RFC 2131, Chapter 4.4.1.
 */
static inline uint32_t get_dhcpv4_timeout(void)
{
	return 5 * sys_clock_ticks_per_sec; /* TODO Should be between 1-10 */
}

static inline uint32_t get_dhcpv4_renewal_time(struct net_if *iface)
{
	return (iface->dhcpv4.renewal_time ? iface->dhcpv4.renewal_time :
		iface->dhcpv4.lease_time / 2) * sys_clock_ticks_per_sec;
}

static inline void unset_dhcpv4_on_iface(struct net_if *iface)
{
	if (!iface) {
		return;
	}

	iface->dhcpv4.state = NET_DHCPV4_INIT;
	iface->dhcpv4.attempts = 0;
	iface->dhcpv4.lease_time = 0;
	iface->dhcpv4.renewal_time = 0;

	memset(iface->dhcpv4.server_id.s4_addr, 0, 4);
	memset(iface->dhcpv4.server_id.s4_addr, 0, 4);
	memset(iface->dhcpv4.requested_ip.s4_addr, 0, 4);

	nano_delayed_work_cancel(&iface->dhcpv4_timeout);
	nano_delayed_work_cancel(&iface->dhcpv4_t1_timer);
}

/* Add magic cookie to DCHPv4 messages */
static inline bool add_cookie(struct net_buf *buf)
{
	return net_nbuf_write(buf, sizeof(magic_cookie), magic_cookie);
}

/* Add DHCPv4 message type */
static inline bool add_msg_type(struct net_buf *buf, uint8_t type)
{
	uint8_t data[3] = { DHCPV4_OPTIONS_MSG_TYPE, 1, type };

	return net_nbuf_write(buf, sizeof(data), data);
}

/*
 * Add DHCPv5 minimum required options for server to reply.
 * Can be added more if needed.
 */
static inline bool add_req_options(struct net_buf *buf)
{
	uint8_t data[5] = { DHCPV4_OPTIONS_REQ_LIST,
			    3, /* Length */
			    DHCPV4_OPTIONS_SUBNET_MASK,
			    DHCPV4_OPTIONS_ROUTER,
			    DHCPV4_OPTIONS_DNS_SERVER };

	return net_nbuf_write(buf, sizeof(data), data);
}

static inline bool add_server_id(struct net_buf *buf)
{
	struct net_if *iface = net_nbuf_iface(buf);
	uint8_t data;

	data = DHCPV4_OPTIONS_SERVER_ID;
	if (!net_nbuf_write(buf, 1, &data)) {
		return false;
	}

	data = 4;
	if (!net_nbuf_write(buf, 1, &data)) {
		return false;
	}

	if (!net_nbuf_write(buf, 4, iface->dhcpv4.server_id.s4_addr)) {
		return false;
	}

	return true;
}

static inline bool add_req_ipaddr(struct net_buf *buf)
{
	struct net_if *iface = net_nbuf_iface(buf);
	uint8_t data;

	data = DHCPV4_OPTIONS_REQ_IPADDR;
	if (!net_nbuf_write(buf, 1, &data)) {
		return false;
	}

	data = 4;
	if (!net_nbuf_write(buf, 1, &data)) {
		return false;
	}

	if (!net_nbuf_write(buf, 4, iface->dhcpv4.requested_ip.s4_addr)) {
		return false;
	}

	return true;
}

/* Add DHCPv4 Options end, rest of the message can be padded wit zeros */
static inline bool add_end(struct net_buf *buf)
{
	uint8_t data = DHCPV4_OPTIONS_END;

	return net_nbuf_write(buf, 1, &data);
}

/* File is empty ATM */
static inline bool add_file(struct net_buf *buf)
{
	uint8_t len = SIZE_OF_FILE;
	uint8_t data = 0;

	while (len-- > 0) {
		if (!net_nbuf_write(buf, 1, &data)) {
			return false;
		}
	}

	return true;
}

/* SNAME is empty ATM */
static inline bool add_sname(struct net_buf *buf)
{
	uint8_t len = SIZE_OF_SNAME;
	uint8_t data = 0;

	while (len-- > 0) {
		if (!net_nbuf_write(buf, 1, &data)) {
			return false;
		}
	}

	return true;
}

/* Setup IPv4 + UDP header */
static void setup_header(struct net_buf *buf)
{
	struct net_ipv4_hdr *ipv4;
	struct net_udp_hdr *udp;
	uint16_t len;

	ipv4 = NET_IPV4_BUF(buf);
	udp = NET_UDP_BUF(buf);

	len = net_buf_frags_len(buf->frags);

	/* Setup IPv4 header */
	memset(ipv4, 0, sizeof(struct net_ipv4_hdr));

	ipv4->vhl = 0x45;
	ipv4->ttl = 0xFF;
	ipv4->proto = IPPROTO_UDP;
	ipv4->len[0] = len >> 8;
	ipv4->len[1] = (uint8_t)len;
	ipv4->chksum = ~net_calc_chksum_ipv4(buf);

	net_ipaddr_copy(&ipv4->dst, net_if_ipv4_broadcast_addr());

	len -= NET_IPV4H_LEN;
	/* Setup UDP header */
	udp->src_port = htons(DHCPV4_CLIENT_PORT);
	udp->dst_port = htons(DHCPV4_SERVER_PORT);
	udp->len = htons(len);
	udp->chksum = ~net_calc_chksum_udp(buf);
}

/* Prepare initial DHCPv4 message and add options as per message type */
static struct net_buf *prepare_message(struct net_if *iface, uint8_t type)
{
	struct net_buf *buf;
	struct net_buf *frag;
	struct dhcp_msg *msg;

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return NULL;
	}

	frag = net_nbuf_get_reserve_data(net_if_get_ll_reserve(iface, NULL));
	if (!frag) {
		goto fail;
	}

	net_nbuf_set_ll_reserve(buf, net_buf_headroom(frag));
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv4_hdr));

	net_buf_frag_add(buf, frag);

	/* Leave room for IPv4 + UDP headers */
	net_buf_add(buf->frags, NET_IPV4UDPH_LEN);

	if (net_buf_tailroom(frag) < sizeof(struct dhcp_msg)) {
		goto fail;
	}

	msg = (struct dhcp_msg *)(frag->data + NET_IPV4UDPH_LEN);
	memset(msg, 0, sizeof(struct dhcp_msg));

	msg->op = DHCPV4_MSG_BOOT_REQUEST;

	msg->htype = HARDWARE_ETHERNET_TYPE;
	msg->hlen = HARDWARE_ETHERNET_LEN;

	msg->xid[0] = htonl(iface->dhcpv4.xid);
	msg->flags = htons(DHCPV4_MSG_BROADCAST);

	if (iface->dhcpv4.state == NET_DHCPV4_INIT) {
		memset(msg->ciaddr, 0, sizeof(msg->ciaddr));
	} else {
		memcpy(msg->ciaddr, iface->dhcpv4.requested_ip.s4_addr, 4);
	}

	memcpy(msg->chaddr, iface->link_addr.addr, iface->link_addr.len);

	net_buf_add(frag, sizeof(struct dhcp_msg));

	if (!add_sname(buf) ||
	    !add_file(buf) ||
	    !add_cookie(buf) ||
	    !add_msg_type(buf, type)) {
		goto fail;
	}

	return buf;

fail:
	net_nbuf_unref(buf);
	return NULL;
}

/* Prepare DHCPv4 Message request and send it to peer */
static void send_request(struct net_if *iface, bool renewal)
{
	struct net_buf *buf;

	buf = prepare_message(iface, DHCPV4_MSG_TYPE_REQUEST);
	if (!buf) {
		goto fail;
	}

	if (!add_server_id(buf) ||
	    !add_req_ipaddr(buf) ||
	    !add_end(buf)) {
		goto fail;
	}

	setup_header(buf);

	if (net_send_data(buf) < 0) {
		goto fail;
	}

	if (renewal) {
		iface->dhcpv4.state = NET_DHCPV4_RENEWAL;
	} else {
		iface->dhcpv4.state = NET_DHCPV4_REQUEST;
	}

	iface->dhcpv4.attempts++;

	nano_delayed_work_init(&iface->dhcpv4_timeout, dhcpv4_timeout);
	nano_delayed_work_submit(&iface->dhcpv4_timeout, get_dhcpv4_timeout());

	return;

fail:
	NET_DBG("Message preparation failed");

	if (!buf) {
		net_nbuf_unref(buf);
	}
}

/* Prepare DHCPv4 Dicover message and braodcast it */
static void send_discover(struct net_if *iface)
{
	struct net_buf *buf;

	/*iface->dhcpv4.xid++; FIXME handle properly */

	buf = prepare_message(iface, DHCPV4_MSG_TYPE_DISCOVER);
	if (!buf) {
		goto fail;
	}

	if (!add_req_options(buf) ||
	    !add_end(buf)) {
		goto fail;
	}

	setup_header(buf);

	if (net_send_data(buf) < 0) {
		goto fail;
	}

	iface->dhcpv4.state = NET_DHCPV4_DISCOVER;

	nano_delayed_work_init(&iface->dhcpv4_timeout, dhcpv4_timeout);
	nano_delayed_work_submit(&iface->dhcpv4_timeout, get_dhcpv4_timeout());

	return;

fail:
	NET_DBG("Message preparation failed");

	if (!buf) {
		net_nbuf_unref(buf);
	}
}

static void dhcpv4_timeout(struct nano_work *work)
{
	struct net_if *iface = CONTAINER_OF(work, struct net_if,
					    dhcpv4_timeout);

	if (!iface) {
		NET_DBG("Invalid iface");

		return;
	}

	switch (iface->dhcpv4.state) {
	case NET_DHCPV4_DISCOVER:
		/* Failed to get OFFER message, send DISCOVER again */
		send_discover(iface);
		break;
	case NET_DHCPV4_REQUEST:
		/*
		 * Maximum number of renewal attempts failed, so start
		 * from the beginning.
		 */
		if (iface->dhcpv4.attempts >= DHCPV4_MAX_NUMBER_OF_ATTEMPTS) {
			send_discover(iface);
		} else {
			/* Repeat requests until max number of attempts */
			send_request(iface, false);
		}
		break;
	case NET_DHCPV4_RENEWAL:
		if (iface->dhcpv4.attempts >= DHCPV4_MAX_NUMBER_OF_ATTEMPTS) {
			if (!net_if_ipv4_addr_rm(iface,
						 &iface->dhcpv4.requested_ip)) {
				NET_DBG("Failed to remove addr from iface");
			}

			/*
			 * Maximum number of renewal attempts failed, so start
			 * from the beginning.
			 */
			send_discover(iface);
		} else {
			/* Repeat renewal request for max number of attempts */
			send_request(iface, true);
		}
		break;
	default:
		break;
	}
}

static void dhcpv4_t1_timeout(struct nano_work *work)
{
	struct net_if *iface = CONTAINER_OF(work, struct net_if,
					    dhcpv4_t1_timer);

	if (!iface) {
		NET_DBG("Invalid iface");

		return;
	}

	send_request(iface, true);
}

/*
 * Parse DHCPv4 options and retrieve relavant information
 * as per RFC 2132.
 */
static enum net_verdict parse_options(struct net_if *iface, struct net_buf *buf,
				      uint16_t offset, uint8_t *msg_type)
{
	struct net_buf *frag;
	uint8_t cookie[4];
	uint8_t time[4];
	uint8_t length;
	uint8_t type;
	uint16_t pos;
	uint8_t *ptr = &time[0];
	bool end = false;

	frag = net_nbuf_read(buf, offset, &pos, sizeof(magic_cookie),
			     (uint8_t *)cookie);
	if (!frag || memcmp(magic_cookie, cookie, sizeof(magic_cookie))) {

		NET_DBG("Incorrect magic cookie");
		return NET_DROP;
	}

	while (frag) {
		frag = net_nbuf_read(frag, pos, &pos, 1, &type);

		if (type == DHCPV4_OPTIONS_END) {
			end = true;
			return NET_OK;
		}

		if (!frag) {
			return NET_DROP;
		}

		frag = net_nbuf_read(frag, pos, &pos, 1, &length);
		if (!frag) {
			return NET_DROP;
		}

		switch (type) {
		case DHCPV4_OPTIONS_SUBNET_MASK:
			if (length != 4) {
				return NET_DROP;
			}

			frag = net_nbuf_read(frag, pos, &pos, length,
					     iface->ipv4.netmask.s4_addr);
			break;
		case DHCPV4_OPTIONS_LEASE_TIME:
			if (length != 4) {
				return NET_DROP;
			}

			frag = net_nbuf_read(frag, pos, &pos, length, &time[0]);

			iface->dhcpv4.lease_time =
				ntohl(UNALIGNED_GET((uint32_t *)ptr));
			if (!iface->dhcpv4.lease_time) {
				return NET_DROP;
			}

			break;
		case DHCPV4_OPTIONS_RENEWAL:
			if (length != 4) {
				return NET_DROP;
			}

			frag = net_nbuf_read(frag, pos, &pos, length, &time[0]);

			iface->dhcpv4.renewal_time =
				ntohl(UNALIGNED_GET((uint32_t *)ptr));
			if (!iface->dhcpv4.renewal_time) {
				return NET_DROP;
			}

			break;
		case DHCPV4_OPTIONS_SERVER_ID:
			if (length != 4) {
				return NET_DROP;
			}

			frag = net_nbuf_read(frag, pos, &pos, length,
					     iface->dhcpv4.server_id.s4_addr);
			break;
		case DHCPV4_OPTIONS_MSG_TYPE:
			if (length != 1) {
				return NET_DROP;
			}

			frag = net_nbuf_read(frag, pos, &pos, 1, msg_type);
			break;
		default:
			frag = net_nbuf_skip(frag, pos, &pos, length);
			break;
		}

		if (!frag && pos) {
			return NET_DROP;
		}
	}

	if (!end) {
		return NET_DROP;
	}

	return NET_OK;
}

/* TODO: Handles only DHCPv4 OFFER and ACK messages */
static inline void handle_dhcpv4_reply(struct net_if *iface, uint8_t msg_type)
{
	/*
	 * Check for previous state, reason behind this check is, if client
	 * receives multiple OFFER messages, first one will be handled.
	 * Rest of the replies are discarded.
	 */
	if (iface->dhcpv4.state == NET_DHCPV4_DISCOVER) {
		if (msg_type != NET_DHCPV4_OFFER) {
			NET_DBG("Reply not handled %d", msg_type);
			return;
		}

		/* Send DHCPv4 Request Message */
		nano_delayed_work_cancel(&iface->dhcpv4_timeout);
		send_request(iface, false);

	} else if (iface->dhcpv4.state == NET_DHCPV4_REQUEST ||
		   iface->dhcpv4.state == NET_DHCPV4_RENEWAL) {

		if (msg_type != NET_DHCPV4_ACK) {
			NET_DBG("Reply not handled %d", msg_type);
			return;
		}

		nano_delayed_work_cancel(&iface->dhcpv4_timeout);

		switch (iface->dhcpv4.state) {
		case NET_DHCPV4_REQUEST:
			if (!net_if_ipv4_addr_add(iface,
						  &iface->dhcpv4.requested_ip,
						  NET_ADDR_DHCP,
						  iface->dhcpv4.lease_time)) {
				NET_DBG("Failed to add IPv4 addr to iface %p",
					iface);
				return;
			}

			break;
		case NET_DHCPV4_RENEWAL:
			/* TODO: if the renewal is success, update only
			 * vlifetime on iface
			 */
			break;
		default:
			break;
		}

		iface->dhcpv4.attempts = 0;
		iface->dhcpv4.state = NET_DHCPV4_ACK;

		/* Start renewal time */
		nano_delayed_work_init(&iface->dhcpv4_t1_timer,
				       dhcpv4_t1_timeout);

		nano_delayed_work_submit(&iface->dhcpv4_t1_timer,
					 get_dhcpv4_renewal_time(iface));
	}
}

static enum net_verdict net_dhcpv4_input(struct net_conn *conn,
					 struct net_buf *buf,
					 void *user_data)
{
	struct dhcp_msg *msg;
	struct net_buf *frag;
	struct net_if *iface;
	uint8_t	msg_type;
	uint8_t min;
	uint16_t pos;

	if (!conn) {
		NET_DBG("Invalid connection");
		return NET_DROP;
	}

	if (!buf || !buf->frags) {
		NET_DBG("Invalid buffer, no fragments");
		return NET_DROP;
	}

	iface = net_nbuf_iface(buf);
	if (!iface) {
		return NET_DROP;
	}

	frag = buf->frags;
	min = NET_IPV4UDPH_LEN + sizeof(struct dhcp_msg);

	/*
	 * If the message is not DHCP then continue passing to
	 * related handlers.
	 */
	if (net_buf_frags_len(frag) < min) {
		NET_DBG("Input msg is not related to DHCPv4");
		return NET_CONTINUE;

	}

	msg = (struct dhcp_msg *)(frag->data + NET_IPV4UDPH_LEN);

	if (!(msg->op == DHCPV4_MSG_BOOT_REPLY &&
	      iface->dhcpv4.xid == ntohl(msg->xid[0]) &&
	      !memcmp(msg->chaddr, iface->link_addr.addr,
		      iface->link_addr.len))) {

		NET_DBG("Invalid op or xid or chaddr");
		goto drop;
	}

	memcpy(iface->dhcpv4.requested_ip.s4_addr, msg->yiaddr,
	       sizeof(msg->yiaddr));

	/* sname, file are not used at the moment, skip it */
	frag = net_nbuf_skip(frag, min, &pos, SIZE_OF_SNAME + SIZE_OF_FILE);
	if (!frag && pos) {
		goto drop;
	}

	if (parse_options(iface, frag, pos, &msg_type) == NET_DROP) {
		NET_DBG("Invalid Options");
		goto drop;
	}

	net_nbuf_unref(buf);

	handle_dhcpv4_reply(iface, msg_type);

	return NET_OK;

drop:
	return NET_DROP;
}

void net_dhcpv4_start(struct net_if *iface)
{
	int ret;

	iface->dhcpv4.state = NET_DHCPV4_INIT;
	iface->dhcpv4.attempts = 0;
	iface->dhcpv4.lease_time = 0;
	iface->dhcpv4.renewal_time = 0;

	/*
	 * Register UDP input callback on
	 * DHCPV4_SERVER_PORT(67) and DHCPV4_CLIENT_PORT(68) for
	 * all dhcpv4 related incoming packets.
	 */
	ret = net_udp_register(NULL, NULL,
			       DHCPV4_SERVER_PORT,
			       DHCPV4_CLIENT_PORT,
			       net_dhcpv4_input, NULL, NULL);
	if (ret < 0) {
		NET_DBG("UDP callback registration failed");
		return;
	}

	send_discover(iface);
}
