/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread_border_router.h"
#include <zephyr/net/dns_resolve.h>
#include <zephyr/sys/slist.h>
#include "dns_pack.h"
#include <common/code_utils.hpp>
#include <openthread.h>
#include <openthread/platform/dns.h>
#include <openthread/dnssd_server.h>

#define DNS_TIMEOUT (2 * MSEC_PER_SEC)

NET_BUF_POOL_DEFINE(dns_qname_pool, 1,
		    CONFIG_DNS_RESOLVER_MAX_QUERY_LEN,
		    0, NULL);

static struct otInstance *ot_instance_ptr;
static void dns_pkt_recv_cb(struct net_buf *dns_data, size_t buf_len, void *user_data);
static void dns_upstream_resolver_handle_response(struct otbr_msg_ctx *msg_ctx_ptr);
static void dns_resolve_cb(enum dns_resolve_status status,
			   struct dns_addrinfo *info,
			   void *user_data);
static struct query_context *get_query_by_ot_transaction(otPlatDnsUpstreamQuery *txn);
static struct query_context *get_query_by_user_data(void *user_data);
static void remove_query_ctx(struct query_context *ctx);

/* List used for storing concurrent transactions/queries */
static sys_slist_t query_list = SYS_SLIST_STATIC_INIT(&query_list);
struct query_context {
	/** Node structure variable used for list handling */
	sys_snode_t node;
	/** OpenThread upstream query data */
	otPlatDnsUpstreamQuery *transaction;
	/** DNS ID returned by dns resolver; needed for cancelling */
	uint16_t resolve_query_id;
	/**
	 * DNS query ID from OpenThread initial query.
	 * Required to be present in response message.
	 */
	int originated_query_id;
};

K_MEM_SLAB_DEFINE_STATIC(query_slab, sizeof(struct query_context),
			 CONFIG_DNS_NUM_CONCUR_QUERIES, sizeof(void *));

void otPlatDnsStartUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn,
				 const otMessage *aQuery)
{
	OT_UNUSED_VARIABLE(aInstance);
	otError error = OT_ERROR_NONE;
	uint16_t length = otMessageGetLength(aQuery);
	uint8_t buffer[DNS_RESOLVER_MAX_BUF_SIZE] = {0};
	struct dns_msg_t msg = {0};
	struct net_buf *result = NULL;
	enum dns_rr_type qtype;
	enum dns_class qclass;
	char name[DNS_MAX_NAME_SIZE + 1] = {0};
	struct query_context *ctx = NULL;

	result = net_buf_alloc(&dns_qname_pool, K_FOREVER);
	VerifyOrExit(result != NULL, error = OT_ERROR_FAILED);

	(void)memset(result->data, 0, net_buf_tailroom(result));
	result->len = 0U;

	VerifyOrExit(otMessageRead(aQuery, 0, buffer, length) == length);
	msg.msg = buffer;
	msg.msg_size = length;
	msg.query_offset = DNS_MSG_HEADER_SIZE;

	VerifyOrExit(dns_unpack_query(&msg, result, &qtype, &qclass) > 0,
		     error = OT_ERROR_FAILED);

	VerifyOrExit(result->len <= sizeof(name), error = OT_ERROR_FAILED);
	memcpy(name, result->data, result->len);

	VerifyOrExit(k_mem_slab_alloc(&query_slab, (void **)&ctx, K_NO_WAIT) == 0,
		     error = OT_ERROR_FAILED);

	ctx->transaction = aTxn;
	ctx->originated_query_id = dns_unpack_header_id(buffer);

	sys_slist_append(&query_list, &ctx->node);

	VerifyOrExit(dns_get_addr_info(name, qtype, &ctx->resolve_query_id,
				       dns_resolve_cb, (void *)ctx, DNS_TIMEOUT) == 0,
		     error = OT_ERROR_FAILED);

exit:
	net_buf_unref(result);
	if (error != OT_ERROR_NONE && ctx != NULL) {
		remove_query_ctx(ctx);
	}
}

void otPlatDnsCancelUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn)
{
	OT_UNUSED_VARIABLE(aInstance);
	struct query_context *ctx = get_query_by_ot_transaction(aTxn);

	if (ctx != NULL) {
		dns_cancel_addr_info(ctx->resolve_query_id);
		remove_query_ctx(ctx);
	}

	otPlatDnsUpstreamQueryDone(aInstance, aTxn, NULL);
}

bool otPlatDnsIsUpstreamQueryAvailable(otInstance *aInstance)
{
#if defined(CONFIG_DNS_RESOLVER)
	return true;
#else
	return false;
#endif /* CONFIG_DNS_RESOLVER */
}

otError dns_upstream_resolver_init(otInstance *ot_instance)
{
	ot_instance_ptr = ot_instance;
	dns_resolve_enable_packet_forwarding(dns_resolve_get_default(), dns_pkt_recv_cb);
	return OT_ERROR_NONE;
}

static void dns_pkt_recv_cb(struct net_buf *dns_data, size_t buf_len, void *user_data)
{
	OT_UNUSED_VARIABLE(buf_len);
	struct otbr_msg_ctx *req;
	struct query_context *ctx;

	ctx = get_query_by_user_data(user_data);

	if (ctx != NULL) {
		VerifyOrExit(openthread_border_router_allocate_message((void **)&req) ==
			     OT_ERROR_NONE);

		(void)memcpy(req->buffer, dns_data->data, buf_len);
		UNALIGNED_PUT(htons(ctx->originated_query_id), (uint16_t *)(req->buffer));
		req->length = buf_len;
		req->cb = dns_upstream_resolver_handle_response;
		req->user_data = user_data;

		openthread_border_router_post_message(req);
		remove_query_ctx(ctx);
	}

exit:
	return;
}

static void dns_upstream_resolver_handle_response(struct otbr_msg_ctx *msg_ctx_ptr)
{
	otMessageSettings ot_message_settings = {true, OT_MESSAGE_PRIORITY_NORMAL};
	otMessage *ot_message = NULL;
	struct query_context *ctx = (struct query_context *)msg_ctx_ptr->user_data;

	ot_message = otUdpNewMessage(ot_instance_ptr, &ot_message_settings);
	VerifyOrExit(ot_message != NULL);
	VerifyOrExit(otMessageAppend(ot_message, msg_ctx_ptr->buffer,
				     msg_ctx_ptr->length) == OT_ERROR_NONE);
	otPlatDnsUpstreamQueryDone(ot_instance_ptr,
				   ctx->transaction, ot_message);
	ot_message = NULL;
exit:
	if (ot_message != NULL) {
		otMessageFree(ot_message);
	}
}

static void dns_resolve_cb(enum dns_resolve_status status,
			   struct dns_addrinfo *info,
			   void *user_data)
{
	OT_UNUSED_VARIABLE(status);
	OT_UNUSED_VARIABLE(info);
	OT_UNUSED_VARIABLE(user_data);
}

static struct query_context *get_query_by_ot_transaction(otPlatDnsUpstreamQuery *txn)
{
	struct query_context *ctx = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&query_list, ctx, node) {
		if (ctx->transaction == txn) {
			return ctx;
		}
	}

	return ctx;
}

static struct query_context *get_query_by_user_data(void *user_data)
{
	struct query_context *ctx;
	otPlatDnsUpstreamQuery *query = (otPlatDnsUpstreamQuery *)user_data;

	ctx = get_query_by_ot_transaction(query);

	return ctx;
}

static void remove_query_ctx(struct query_context *ctx)
{
	sys_slist_find_and_remove(&query_list, &ctx->node);
	k_mem_slab_free(&query_slab, (void *)ctx);
}
