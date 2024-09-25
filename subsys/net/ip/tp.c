/*
 * Copyright (c) 2018-2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "tp.h"
#include "tp_priv.h"
#include "ipv4.h"
#include "ipv6.h"
#include "udp_internal.h"

static sys_slist_t tp_mem = SYS_SLIST_STATIC_INIT(&tp_mem);
static sys_slist_t tp_nbufs = SYS_SLIST_STATIC_INIT(&tp_nbufs);
static sys_slist_t tp_pkts = SYS_SLIST_STATIC_INIT(&tp_pkts);
static sys_slist_t tp_seq = SYS_SLIST_STATIC_INIT(&tp_seq);

bool tp_trace;
enum tp_type tp_state = TP_NONE;

char *tp_basename(char *path)
{
	char *filename = strrchr(path, '/');

	return filename ? (filename + 1) : path;
}

size_t tp_str_to_hex(void *buf, size_t bufsize, const char *s)
{
	size_t i, j, len = strlen(s);

	tp_assert((len % 2) == 0, "Invalid string: %s", s);

	for (i = 0, j = 0; i < len; i += 2, j++) {

		uint8_t byte = (s[i] - '0') << 4 | (s[i + 1] - '0');

		((uint8_t *) buf)[j] = byte;
	}

	return j;
}

void *tp_malloc(size_t size, const char *file, int line, const char *func)
{
	struct tp_mem *mem = k_malloc(sizeof(struct tp_mem) + size +
					sizeof(*mem->footer));

	mem->file = file;
	mem->line = line;
	mem->func = func;

	mem->size = size;

	mem->header = TP_MEM_HEADER_COOKIE;

	mem->footer = (void *) ((uint8_t *) &mem->mem + size);
	*mem->footer = TP_MEM_FOOTER_COOKIE;

	sys_slist_append(&tp_mem, (sys_snode_t *) mem);

	return &mem->mem;
}

static void dump(void *data, size_t len)
{
	uint8_t *buf = data;
	size_t i, width = 8;

	for (i = 0; i < len; i++) {

		if ((i % width) == 0) {
			printk("0x%08lx\t", POINTER_TO_INT(buf + i));
		}

		printk("%02x ", buf[i]);

		if (((i + 1) % width) == 0 || i == (len - 1)) {
			printk("\n");
		}
	}
}

void tp_mem_chk(struct tp_mem *mem)
{
	if (mem->header != TP_MEM_HEADER_COOKIE ||
		*mem->footer != TP_MEM_FOOTER_COOKIE) {

		tp_dbg("%s:%d %s() %p size: %zu",
			mem->file, mem->line, mem->func, mem->mem, mem->size);

		dump(&mem->header, sizeof(mem->header));
		dump(mem->mem, mem->size);
		dump(mem->footer, sizeof(*mem->footer));

		tp_assert(mem->header == TP_MEM_HEADER_COOKIE,
				"%s:%d %s() %p Corrupt header cookie: 0x%x",
				mem->file, mem->line, mem->func, mem->mem,
				mem->header);

		tp_assert(*mem->footer == TP_MEM_FOOTER_COOKIE,
				"%s:%d %s() %p Corrupt footer cookie: 0x%x",
				mem->file, mem->line, mem->func, mem->mem,
				*mem->footer);
	}
}

void tp_free(void *ptr, const char *file, int line, const char *func)
{
	struct tp_mem *mem = (void *)((uint8_t *) ptr - sizeof(struct tp_mem));

	tp_mem_chk(mem);

	if (!sys_slist_find_and_remove(&tp_mem, (sys_snode_t *) mem)) {
		tp_assert(false, "%s:%d %s() Invalid free(%p)",
				file, line, func, ptr);
	}

	memset(mem, 0, sizeof(tp_mem) + mem->size + sizeof(*mem->footer));
	k_free(mem);
}

void *tp_calloc(size_t nmemb, size_t size, const char *file, int line,
		const char *func)
{
	size_t bytes = size * nmemb;
	void *ptr = tp_malloc(bytes, file, line, func);

	memset(ptr, 0, bytes);

	return ptr;
}

void tp_mem_stat(void)
{
	struct tp_mem *mem;

	SYS_SLIST_FOR_EACH_CONTAINER(&tp_mem, mem, next) {
		tp_dbg("len=%zu %s:%d", mem->size, mem->file, mem->line);
		tp_mem_chk(mem);
	}
}

struct net_buf *tp_nbuf_alloc(struct net_buf_pool *pool, size_t len,
				const char *file, int line, const char *func)
{
	struct net_buf *nbuf = net_buf_alloc_len(pool, len, K_NO_WAIT);
	struct tp_nbuf *tp_nbuf = k_malloc(sizeof(struct tp_nbuf));

	tp_assert(len, "");
	tp_assert(nbuf, "Out of nbufs");

	tp_dbg("size=%d %p %s:%d %s()", nbuf->size, nbuf, file, line, func);

	tp_nbuf->nbuf = nbuf;
	tp_nbuf->file = file;
	tp_nbuf->line = line;

	sys_slist_append(&tp_nbufs, (sys_snode_t *) tp_nbuf);

	return nbuf;
}

struct net_buf *tp_nbuf_clone(struct net_buf *buf, const char *file, int line,
				const char *func)
{
	struct net_buf *clone = net_buf_clone(buf, K_NO_WAIT);
	struct tp_nbuf *tb = k_malloc(sizeof(struct tp_nbuf));

	tp_dbg("size=%d %p %s:%d %s()", clone->size, clone, file, line, func);

	tb->nbuf = clone;
	tb->file = file;
	tb->line = line;

	sys_slist_append(&tp_nbufs, &tb->next);

	return clone;

}

void tp_nbuf_unref(struct net_buf *nbuf, const char *file, int line,
			const char *func)
{
	bool found = false;
	struct tp_nbuf *tp_nbuf;

	tp_dbg("len=%d %p %s:%d %s()", nbuf->len, nbuf, file, line, func);

	SYS_SLIST_FOR_EACH_CONTAINER(&tp_nbufs, tp_nbuf, next) {
		if (tp_nbuf->nbuf == nbuf) {
			found = true;
			break;
		}
	}

	tp_assert(found, "Invalid %s(%p): %s:%d", __func__, nbuf, file, line);

	sys_slist_find_and_remove(&tp_nbufs, (sys_snode_t *) tp_nbuf);

	net_buf_unref(nbuf);

	k_free(tp_nbuf);
}

void tp_nbuf_stat(void)
{
	struct tp_nbuf *tp_nbuf;

	SYS_SLIST_FOR_EACH_CONTAINER(&tp_nbufs, tp_nbuf, next) {
		tp_dbg("%s:%d len=%d", tp_nbuf->file, tp_nbuf->line,
			tp_nbuf->nbuf->len);
	}
}

void tp_pkt_alloc(struct net_pkt *pkt,
		  const char *file, int line)
{
	struct tp_pkt *tp_pkt = k_malloc(sizeof(struct tp_pkt));

	tp_assert(tp_pkt, "");
	tp_assert(pkt, "");

	tp_pkt->pkt = pkt;
	tp_pkt->file = file;
	tp_pkt->line = line;

	sys_slist_append(&tp_pkts, (sys_snode_t *) tp_pkt);
}

struct net_pkt *tp_pkt_clone(struct net_pkt *pkt, const char *file, int line)
{
	struct tp_pkt *tp_pkt = k_malloc(sizeof(struct tp_pkt));

	pkt = net_pkt_clone(pkt, K_NO_WAIT);

	tp_pkt->pkt = pkt;
	tp_pkt->file = file;
	tp_pkt->line = line;

	sys_slist_append(&tp_pkts, (sys_snode_t *) tp_pkt);

	return pkt;
}

void tp_pkt_unref(struct net_pkt *pkt, const char *file, int line)
{
	bool found = false;
	struct tp_pkt *tp_pkt;

	SYS_SLIST_FOR_EACH_CONTAINER(&tp_pkts, tp_pkt, next) {
		if (tp_pkt->pkt == pkt) {
			found = true;
			break;
		}
	}

	tp_assert(found, "Invalid %s(%p): %s:%d", __func__, pkt, file, line);

	sys_slist_find_and_remove(&tp_pkts, (sys_snode_t *) tp_pkt);

	net_pkt_unref(tp_pkt->pkt);

	k_free(tp_pkt);
}

void tp_pkt_stat(void)
{
	struct tp_pkt *pkt;

	SYS_SLIST_FOR_EACH_CONTAINER(&tp_pkts, pkt, next) {
		tp_dbg("%s:%d %p", pkt->file, pkt->line, pkt->pkt);
	}
}

#define tp_seq_dump(_seq)						\
{									\
	tp_dbg("%s %u->%u (%s%d) %s:%d %s() %s",			\
		(_seq)->kind == TP_SEQ ? "SEQ" : "ACK",			\
		(_seq)->old_value, (_seq)->value,			\
		(_seq)->req >= 0 ? "+" : "", (_seq)->req,		\
		(_seq)->file, (_seq)->line, (_seq)->func,		\
		(_seq)->of ? "OF" : "");				\
}

uint32_t tp_seq_track(int kind, uint32_t *pvalue, int req,
			const char *file, int line, const char *func)
{
	struct tp_seq *seq = k_calloc(1, sizeof(struct tp_seq));

	seq->file = file;
	seq->line = line;
	seq->func = func;

	seq->kind = kind;

	seq->req = req;
	seq->old_value = *pvalue;

	if (req > 0) {
		seq->of = __builtin_uadd_overflow(seq->old_value, seq->req,
							&seq->value);
	} else {
		seq->value += req;
	}

	*pvalue = seq->value;

	sys_slist_append(&tp_seq, (sys_snode_t *) seq);

	tp_seq_dump(seq);

	return seq->value;
}

void tp_seq_stat(void)
{
	struct tp_seq *seq;

	while ((seq = (struct tp_seq *) sys_slist_get(&tp_seq))) {
		tp_seq_dump(seq);
		k_free(seq);
	}
}

enum tp_type tp_msg_to_type(const char *s)
{
	enum tp_type type = TP_NONE;

#define is_tp(_s, _type) do {		\
	if (is(#_type, _s)) {		\
		type = _type;		\
		goto out;		\
	}				\
} while (false)

	is_tp(s, TP_COMMAND);
	is_tp(s, TP_CONFIG_REQUEST);
	is_tp(s, TP_INTROSPECT_REQUEST);
	is_tp(s, TP_DEBUG_STOP);
	is_tp(s, TP_DEBUG_STEP);
	is_tp(s, TP_DEBUG_CONTINUE);

#undef is_tp

out:
	tp_assert(type != TP_NONE, "Invalid message: %s", s);

	return type;
}

static void udp_finalize_pkt(struct net_pkt *pkt)
{
	int ret = 0;

	net_pkt_cursor_init(pkt);

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		ret = net_ipv4_finalize(pkt, IPPROTO_UDP);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		ret = net_ipv6_finalize(pkt, IPPROTO_UDP);
	}

	NET_ASSERT(ret == 0);
}

static int ip_header_add(struct net_pkt *pkt)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		struct in_addr src;
		struct in_addr dst;

		net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &src);
		net_addr_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &dst);

		return net_ipv4_create(pkt, &src, &dst);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		struct in6_addr src;
		struct in6_addr dst;

		net_addr_pton(AF_INET6, CONFIG_NET_CONFIG_MY_IPV6_ADDR, &src);
		net_addr_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR, &dst);

		return net_ipv6_create(pkt, &src, &dst);
	}

	return -EINVAL;
}

static void tp_pkt_send(struct net_pkt *pkt)
{
	net_pkt_ref(pkt);

	if (net_send_data(pkt) < 0) {
		tp_err("net_send_data()");
	}

	tp_pkt_unref(pkt, tp_basename(__FILE__), __LINE__);
}

static struct net_pkt *tp_output_pkt_alloc(sa_family_t af,
					   struct net_if *iface,
					   size_t len,
					   const char *file, int line)
{
	struct tp_pkt *tp_pkt = k_malloc(sizeof(struct tp_pkt));

	tp_assert(tp_pkt, "");

	tp_pkt->pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct net_udp_hdr) + len,
					af, IPPROTO_UDP, K_NO_WAIT);
	tp_assert(tp_pkt->pkt, "");

	tp_pkt->file = file;
	tp_pkt->line = line;

	sys_slist_append(&tp_pkts, (sys_snode_t *) tp_pkt);

	return tp_pkt->pkt;
}

void _tp_output(sa_family_t af, struct net_if *iface, void *data,
		size_t data_len, const char *file, int line)
{
	struct net_pkt *pkt = tp_output_pkt_alloc(af, iface, data_len,
						  file, line);
	int ret;

	ret = ip_header_add(pkt);
	if (ret < 0) {
		goto fail;
	}

	ret = net_udp_create(pkt, htons(4242), htons(4242));
	if (ret < 0) {
		goto fail;
	}

	ret = net_pkt_write(pkt, data, data_len);
	if (ret < 0) {
		goto fail;
	}

	udp_finalize_pkt(pkt);

	NET_ASSERT(net_pkt_get_len(pkt) <= net_if_get_mtu(pkt->iface));

	tp_pkt_send(pkt);

	return;
fail:
	NET_ASSERT(false);
}

struct tp *json_to_tp(void *data, size_t data_len)
{
	static struct tp tp;

	memset(&tp, 0, sizeof(tp));

	if (json_obj_parse(data, data_len, tp_descr, ARRAY_SIZE(tp_descr),
			&tp) < 0) {
		tp_err("json_obj_parse()");
	}

	tp.type = tp_msg_to_type(tp.msg);

	return &tp;
}

void tp_new_find_and_apply(struct tp_new *tp, const char *key, void *value,
				int type)
{
	bool found = false;
	int i;

	for (i = 0; i < tp->num_entries; i++) {
		if (is(key, tp->data[i].key)) {
			found = true;
			break;
		}
	}

	if (found) {
		switch (type) {
		case TP_BOOL: {
			bool new_value, old = *((bool *) value);

			new_value = atoi(tp->data[i].value);
			*((bool *) value) = new_value;
			tp_dbg("%s %d->%d", key, old, new_value);
			break;
		}
		case TP_INT: {
			int new_value, old_value = *((int *) value);

			new_value = atoi(tp->data[i].value);
			*((int *) value) = new_value;
			tp_dbg("%s %d->%d", key, old_value, new_value);
			break;
		}
		default:
			tp_err("Unimplemented");
		}
	}
}

enum tp_type json_decode_msg(void *data, size_t data_len)
{
	int decoded;
	struct tp_msg tp;

	memset(&tp, 0, sizeof(tp));

	decoded = json_obj_parse(data, data_len, tp_msg_dsc,
					ARRAY_SIZE(tp_msg_dsc), &tp);
#if 0
	if ((decoded & 1) == false) { /* TODO: this fails, why? */
		tp_err("json_obj_parse()");
	}
#endif
	tp_dbg("%s", tp.msg);

	return tp.msg ? tp_msg_to_type(tp.msg) : TP_NONE;
}

struct tp_new *json_to_tp_new(void *data, size_t data_len)
{
	static struct tp_new tp;
	int i;

	memset(&tp, 0, sizeof(tp));

	if (json_obj_parse(data, data_len, tp_new_dsc, ARRAY_SIZE(tp_new_dsc),
				&tp) < 0) {
		tp_err("json_obj_parse()");
	}

	tp_dbg("%s", tp.msg);

	for (i = 0; i < tp.num_entries; i++) {
		tp_dbg("%s=%s", tp.data[i].key, tp.data[i].value);
	}

	return &tp;
}

void tp_encode(struct tp *tp, void *data, size_t *data_len)
{
	int error;

	error = json_obj_encode_buf(tp_descr, ARRAY_SIZE(tp_descr), tp,
					data, *data_len);
	if (error) {
		tp_err("json_obj_encode_buf()");
	}

	*data_len = error ? 0 : strlen(data);
}


void tp_new_to_json(struct tp_new *tp, void *data, size_t *data_len)
{
	int error = json_obj_encode_buf(tp_new_dsc, ARRAY_SIZE(tp_new_dsc), tp,
					data, *data_len);
	if (error) {
		tp_err("json_obj_encode_buf()");
	}

	*data_len = error ? 0 : strlen(data);
}

void tp_out(sa_family_t af, struct net_if *iface, const char *msg,
	    const char *key, const char *value)
{
	if (tp_trace) {
		size_t json_len;
		static uint8_t buf[128]; /* TODO: Merge all static buffers and
				       * eventually drop them
				       */
		struct tp_new tp = {
			.msg = msg,
			.data = { { .key = key, .value = value } },
			.num_entries = 1
		};
		json_len = sizeof(buf);
		tp_new_to_json(&tp, buf, &json_len);
		if (json_len) {
			tp_output(af, iface, buf, json_len);
		}
	}
}

bool tp_tap_input(struct net_pkt *pkt)
{
	bool tap = tp_state != TP_NONE;

	if (tap) {
		net_pkt_ref(pkt);
		/* STAILQ_INSERT_HEAD(&tp_q, pkt, stq_next); */
	}

	return tap;
}
