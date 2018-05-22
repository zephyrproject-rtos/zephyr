/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "rpl-br/coap"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>

#include <net/net_app.h>
#include <net/coap.h>
#include <net/udp.h>

#include "rpl.h"
#include "net_private.h"

#include "config.h"

#define MY_COAP_PORT 0xC0AB
#define PEER_COAP_PORT htons(0x1633) /* 5683 */

#define PKT_WAIT_TIME K_SECONDS(1)
#define RESPONSE_TIME  K_SECONDS(3)

#define MAX_COAP_REQUEST_ATTEMPTS	5
#define MAX_COAP_REQUESTS		20

struct coap_request {
	struct k_delayed_work timer; /* Timer to retransmit messages */
	struct sockaddr_in6 peer; /* Peer CoAP server socket address */
	u16_t id; /* Message id sent */
	u8_t code; /* Expecting reply code */
	u8_t count; /* Number of trails */
	bool used; /* Entry used or not */
	enum coap_request_type type;
	coap_reply_cb_t cb;
	void *user_data;
};

static struct net_context *coap;
static struct coap_request requests[MAX_COAP_REQUESTS];

static const char * const led_uri_path[] = { "led", NULL };
static const char * const rpl_obs_path[] = { "rpl-obs", NULL };

static void get_from_ip_addr(struct coap_packet *cpkt,
			     struct sockaddr_in6 *from)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(cpkt->pkt, &hdr);
	if (!udp_hdr) {
		return;
	}

	net_ipaddr_copy(&from->sin6_addr, &NET_IPV6_HDR(cpkt->pkt)->src);
	from->sin6_port = udp_hdr->src_port;
	from->sin6_family = AF_INET6;
}

static struct coap_request *
get_coap_request_by_type(const struct sockaddr_in6 *peer,
			 enum coap_request_type type)
{
	u8_t i;

	for (i = 0; i < MAX_COAP_REQUESTS; i++) {
		if (!requests[i].used) {
			continue;
		}

		if (requests[i].type == type &&
		    net_ipv6_addr_cmp(&requests[i].peer.sin6_addr,
				      &peer->sin6_addr)) {
			return &requests[i];
		}
	}

	return NULL;
}

static struct coap_request *
get_coap_request_by_id(const struct sockaddr_in6 *peer, u16_t id)
{
	u8_t i;

	for (i = 0; i < MAX_COAP_REQUESTS; i++) {
		if (!requests[i].used) {
			continue;
		}

		if (requests[i].id == id &&
		    net_ipv6_addr_cmp(&requests[i].peer.sin6_addr,
				      &peer->sin6_addr)) {
			return &requests[i];
		}
	}

	return NULL;
}

static struct coap_request *
get_coap_request_by_addr(const struct in6_addr *peer)
{
	u8_t i;

	for (i = 0; i < MAX_COAP_REQUESTS; i++) {
		if (!requests[i].used) {
			continue;
		}

		if (net_ipv6_addr_cmp(&requests[i].peer.sin6_addr, peer)) {
			return &requests[i];
		}
	}

	return NULL;
}

static struct coap_request *get_free_coap_request(void)
{
	int i;

	for (i = 0; i < MAX_COAP_REQUESTS; i++) {
		if (!requests[i].used) {
			return &requests[i];
		}
	}

	return NULL;
}

static void clear_coap_request(struct coap_request *request)
{
	if (!request->used) {
		return;
	}

	request->type = COAP_REQ_NONE;
	request->id = 0;
	request->code = 0;
	request->count = 0;
	request->used = false;
	request->cb = NULL;
	request->user_data = NULL;

	k_delayed_work_cancel(&request->timer);
}

static bool toggle_led(const struct sockaddr_in6 *peer, u16_t id)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct coap_packet request;
	const char * const *p;
	int r;

	pkt = net_pkt_get_tx(coap, PKT_WAIT_TIME);
	if (!pkt) {
		NET_ERR("Ran out of network packets");
		return false;
	}

	frag = net_pkt_get_data(coap, PKT_WAIT_TIME);
	if (!frag) {
		NET_ERR("Ran out of network buffers");
		goto end;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&request, pkt, 1, COAP_TYPE_NON_CON,
			     0, NULL, COAP_METHOD_POST, id);
	if (r < 0) {
		NET_ERR("Failed to initialize CoAP packet");
		goto end;
	}

	for (p = led_uri_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			NET_ERR("Unable add option to request.\n");
			goto end;
		}
	}

	r = net_context_sendto(pkt, (const struct sockaddr *)peer,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		NET_ERR("Cannot send data to peer (%d)", r);
		goto end;
	}

	return true;

end:
	net_pkt_unref(pkt);
	return false;
}

static bool set_rpl_observer(const struct sockaddr_in6 *peer, u16_t id)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct coap_packet request;
	const char * const *p;
	int r;

	pkt = net_pkt_get_tx(coap, PKT_WAIT_TIME);
	if (!pkt) {
		NET_ERR("Ran out of network packets");
		return false;
	}

	frag = net_pkt_get_data(coap, PKT_WAIT_TIME);
	if (!frag) {
		NET_ERR("Ran out of network buffers");
		goto end;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&request, pkt, 1, COAP_TYPE_CON,
			     8, coap_next_token(),
			     COAP_METHOD_GET, id);
	if (r < 0) {
		NET_ERR("Failed to initialize CoAP packet");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
	if (r < 0) {
		NET_ERR("Unable add option to request");
		goto end;
	}

	for (p = rpl_obs_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			NET_ERR("Unable add option to request");
			goto end;
		}
	}

	r = net_context_sendto(pkt, (const struct sockaddr *)peer,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		NET_ERR("Cannot send data to peer (%d)", r);
		goto end;
	}

	return true;

end:
	net_pkt_unref(pkt);
	return false;
}

static void request_timeout(struct k_work *work)
{
	struct coap_request *request = CONTAINER_OF(work,
						    struct coap_request,
						    timer);

	if (request->type != COAP_REQ_RPL_OBS) {
		return;
	}

	/* Check if number of CoAP requests to this peer reached max or not. */
	if (request->count >= MAX_COAP_REQUEST_ATTEMPTS) {
			clear_coap_request(request);
		return;
	}

	request->count++;

	set_rpl_observer(&request->peer, request->id);

	k_delayed_work_submit(&request->timer, RESPONSE_TIME);
}

static void add_nbr_to_topology(struct in6_addr *nbr)
{
	if (topology.nodes[0].used) {
		return;
	}

	topology.nodes[0].id = 1;
	topology.nodes[0].used = true;
	snprintk(topology.nodes[0].label, sizeof(topology.nodes[0].label),
		 "NBR");
	net_ipaddr_copy(&topology.nodes[0].addr, nbr);
}

static void add_node_to_topology(struct in6_addr *node)
{
	u8_t i;

	/* BR takes 'id : 1', so node's id starts from 2 */
	for (i = 1; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		if (topology.nodes[i].used) {
			continue;
		}

		topology.nodes[i].id = i + 1;
		topology.nodes[i].used = true;

		snprintk(topology.nodes[i].label,
			 sizeof(topology.nodes[i].label),
			 "N%d", topology.nodes[i].id);

		net_ipaddr_copy(&topology.nodes[i].addr, node);
		break;
	}
}

static void update_node_topology(struct in6_addr *node,
				 struct in6_addr *parent,
				 u16_t rank)
{
	u8_t i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		if (!topology.nodes[i].used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(&topology.nodes[i].addr,
				       node)) {
			continue;
		}

		topology.nodes[i].rank = rank;
		net_ipaddr_copy(&topology.nodes[i].parent, parent);
		break;
	}
}

static void remove_node_from_topology(struct in6_addr *node)
{
	u8_t i;

	for (i = 1; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		if (!topology.nodes[i].used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(&topology.nodes[i].addr,
				       node)) {
			continue;
		}

		topology.nodes[i].used = false;
	}
}

#define COAP_REPLY_PARENT strlen("parent-")
#define COAP_REPLY_RANK strlen("rank-")
#define COAP_REPLY_NONE strlen("None")
#define COAP_REPLY_RANK_VALUE 6

#define MAX_PAYLOAD_SIZE (NET_IPV6_ADDR_LEN + \
			  COAP_REPLY_PARENT + \
			  COAP_REPLY_RANK   + \
			  COAP_REPLY_RANK_VALUE)

static void node_obs_reply(struct coap_packet *response, void *user_data)
{
	char payload[MAX_PAYLOAD_SIZE];
	char parent_str[NET_IPV6_ADDR_LEN + 1];
	char rank_str[COAP_REPLY_RANK_VALUE + 1];
	struct sockaddr_in6 from;
	struct in6_addr parent;
	struct net_buf *frag;
	u16_t offset;
	u16_t len;
	u16_t rank;
	u8_t i;

	frag = coap_packet_get_payload(response, &offset, &len);
	if (!frag && offset == 0xffff) {
		NET_ERR("Error while getting payload");
		return;
	}

	if (!len) {
		NET_ERR("Invalid response");
		return;
	}

	frag = net_frag_read(frag, offset, &offset, len, (u8_t *) payload);
	if (!frag && offset == 0xffff) {
		return;
	}

	if (strncmp(payload, "parent-", COAP_REPLY_PARENT)) {
		return;
	}

	if (!strncmp(payload + COAP_REPLY_PARENT, "None", COAP_REPLY_NONE)) {
		return;
	}

	i = 0;
	offset = COAP_REPLY_PARENT;
	while (payload[offset] != '\n') {
		parent_str[i++] = payload[offset++];
	}

	parent_str[i] = '\0';

	i++;
	if (strncmp(payload + COAP_REPLY_PARENT + i, "rank-",
		    COAP_REPLY_RANK)) {
		return;
	}

	if (!strncmp(payload + COAP_REPLY_PARENT + i, "None",
		     COAP_REPLY_NONE)) {
		return;
	}

	offset = COAP_REPLY_PARENT + i + COAP_REPLY_RANK;
	i = 0;
	while (offset < len) {
		rank_str[i++] = payload[offset++];
	}

	if (i > strlen(rank_str)) {
		return;
	}

	rank_str[i] = '\0';

	if (net_addr_pton(AF_INET6, parent_str, &parent) < 0) {
		NET_ERR("Failed to convert parent address");
		return;
	}

	get_from_ip_addr(response, &from);
	rank = atoi(rank_str);

	update_node_topology(&from.sin6_addr, &parent, rank);
}

static void pkt_receive(struct net_context *context,
			struct net_pkt *pkt,
			int status,
			void *user_data)
{
	struct coap_option options[4] = { 0 };
	struct coap_packet response;
	struct coap_request *coap_req;
	struct sockaddr_in6 from;
	u16_t id;
	u8_t type;
	u8_t code;
	u8_t opt_num = 4;
	int r;

	r = coap_packet_parse(&response, pkt, options, opt_num);
	if (r < 0) {
		NET_ERR("Invalid data received (%d)\n", r);
		net_pkt_unref(pkt);
		return;
	}

	type = coap_header_get_type(&response);
	code = coap_header_get_code(&response);
	id  = coap_header_get_id(&response);
	get_from_ip_addr(&response, &from);

	if (type != COAP_TYPE_ACK) {
		NET_ERR("Invalid response, type %d", type);
		net_pkt_unref(pkt);
		return;
	}

	NET_DBG("Received %d bytes coap payload",
		net_pkt_appdatalen(pkt) - response.hdr_len - response.opt_len);

	coap_req = get_coap_request_by_id(&from, id);
	if (!coap_req) {
		net_pkt_unref(pkt);
		return;
	}

	if (code != coap_req->code) {
		NET_ERR("Invalid response, code %d", code);
		goto end;
	}

	if (coap_req->type == COAP_REQ_RPL_OBS) {
		node_obs_reply(&response, coap_req->user_data);
	}

	if (coap_req->cb) {
		coap_req->cb(&response, coap_req->user_data);
	}

end:
	clear_coap_request(coap_req);
	net_pkt_unref(pkt);
}

void coap_remove_node_from_topology(struct in6_addr *peer)
{
	struct coap_request *request;

	request = get_coap_request_by_addr(peer);
	if (request) {
		clear_coap_request(request);
	}

	remove_node_from_topology(peer);
}

void coap_send_request(struct in6_addr *peer_addr,
		       enum coap_request_type type,
		       coap_reply_cb_t cb,
		       void *user_data)
{
	struct sockaddr_in6 peer;
	struct coap_request *request;

	peer.sin6_family = AF_INET6;
	peer.sin6_port = PEER_COAP_PORT;
	net_ipaddr_copy(&peer.sin6_addr, peer_addr);

	if (type == COAP_REQ_TOGGLE_LED) {
		toggle_led(&peer, coap_next_id());
		return;
	}

	if (type != COAP_REQ_RPL_OBS) {
		return;
	}

	/* Check for request has been sent already or not. If request has been
	 * sent already then timer will run until number of requests reach
	 * MAX_COAP_REQUEST_ATTEMPTS.
	 */
	request = get_coap_request_by_type(&peer, type);
	if (request) {
		return;
	}

	request = get_free_coap_request();
	if (!request) {
		NET_ERR("Failed to get free coap request");
		return;
	}

	request->peer.sin6_family = peer.sin6_family;
	request->peer.sin6_port = peer.sin6_port;
	net_ipaddr_copy(&request->peer.sin6_addr, &peer.sin6_addr);

	request->id = coap_next_id();
	request->count = 1;
	request->type = type;
	request->cb = cb;
	request->user_data = user_data;
	request->used = true;
	request->code = COAP_RESPONSE_CODE_CONTENT;

	add_node_to_topology(peer_addr);
	set_rpl_observer(&request->peer, request->id);

	k_delayed_work_init(&request->timer, request_timeout);
	k_delayed_work_submit(&request->timer, RESPONSE_TIME);
}

int coap_init(void)
{
	struct net_if *iface = NULL;

	static struct sockaddr_in6 my_addr = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(MY_COAP_PORT)
			};
	u8_t i;
	int r;

	iface = net_if_get_ieee802154();
	if (!iface) {
		NET_ERR("No IEEE 802.15.4 network interface found.");
		return -EINVAL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (iface->config.ip.ipv6->unicast[i].is_used) {
			break;
		}
	}

	if (i >= NET_IF_MAX_IPV6_ADDR) {
		return -EINVAL;
	}

	net_ipaddr_copy(&my_addr.sin6_addr,
			&iface->config.ip.ipv6->unicast[i].address.in6_addr);

	add_nbr_to_topology(&my_addr.sin6_addr);

	r = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &coap);
	if (r < 0) {
		NET_ERR("Could not get UDP context");
		return r;
	}

	r = net_context_bind(coap, (struct sockaddr *) &my_addr,
			     sizeof(my_addr));
	if (r < 0) {
		NET_ERR("Could not bind to the context");
		return r;
	}

	r = net_context_recv(coap, pkt_receive, 0, NULL);
	if (r < 0) {
		NET_ERR("Could not set recv callback in the context");
		return r;
	}

	return 0;
}
