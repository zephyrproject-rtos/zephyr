/*
 * Copyright (c) 2018-2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TP_H
#define TP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <zephyr/data/json.h>
#include <zephyr/net/net_pkt.h>
#include "connection.h"

#if defined(CONFIG_NET_TEST_PROTOCOL)

#define TP_SEQ 0
#define TP_ACK 1

#define TP_BOOL	1
#define TP_INT	2

enum tp_type { /* Test protocol message type */
	TP_NONE = 0,
	TP_COMMAND,
	TP_CONFIG_REQUEST,
	TP_CONFIG_REPLY,
	TP_INTROSPECT_REQUEST,
	TP_INTROSPECT_REPLY,
	TP_INTROSPECT_MEMORY_REQUEST,
	TP_INTROSPECT_MEMORY_REPLY,
	TP_INTROSPECT_PACKETS_REQUEST,
	TP_INTROSPECT_PACKETS_REPLY,
	TP_DEBUG_STOP,
	TP_DEBUG_STEP,
	TP_DEBUG_CONTINUE,
	TP_DEBUG_RESPONSE,
	TP_DEBUG_BREAKPOINT_ADD,
	TP_DEBUG_BREAKPOINT_DELETE,
	TP_TRACE_ADD,
	TP_TRACE_DELETE
};

extern bool tp_trace;
extern enum tp_type tp_state;

struct tp_msg {
	const char *msg;
};

static const struct json_obj_descr tp_msg_dsc[] = {
	JSON_OBJ_DESCR_PRIM(struct tp_msg, msg, JSON_TOK_STRING)
};

struct tp {
	enum tp_type type;
	const char *msg;
	const char *status;
	const char *state;
	int seq;
	int ack;
	const char *rcv;
	const char *data;
	const char *op;
};

#define json_str(_type, _field) \
	JSON_OBJ_DESCR_PRIM(struct _type, _field, JSON_TOK_STRING)
#define json_num(_type, _field) \
	JSON_OBJ_DESCR_PRIM(struct _type, _field, JSON_TOK_NUMBER)

static const struct json_obj_descr tp_descr[] = {
	json_str(tp, msg),
	json_str(tp, status),
	json_str(tp, state),
	json_num(tp, seq),
	json_num(tp, ack),
	json_str(tp, rcv),
	json_str(tp, data),
	json_str(tp, op),
};

struct tp_entry {
	const char *key;
	const char *value;
};

static const struct json_obj_descr tp_entry_dsc[] = {
	JSON_OBJ_DESCR_PRIM(struct tp_entry, key, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct tp_entry, value, JSON_TOK_STRING),
};

struct tp_new {
	const char *msg;
	struct tp_entry data[10];
	size_t num_entries;
};

static const struct json_obj_descr tp_new_dsc[] = {
	JSON_OBJ_DESCR_PRIM(struct tp_new, msg, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct tp_new, data, 10, num_entries,
				 tp_entry_dsc, ARRAY_SIZE(tp_entry_dsc)),
};

enum net_verdict tp_input(struct net_conn *net_conn,
			  struct net_pkt *pkt,
			  union net_ip_header *ip,
			  union net_proto_header *proto,
			  void *user_data);

char *tp_basename(char *path);
const char *tp_hex_to_str(void *data, size_t len);
size_t tp_str_to_hex(void *buf, size_t bufsize, const char *s);

void _tp_output(sa_family_t af, struct net_if *iface, void *data,
		size_t data_len, const char *file, int line);
#define tp_output(_af, _iface, _data, _data_len)	\
	_tp_output(_af, _iface, _data, _data_len,	\
		   tp_basename(__FILE__), __LINE__)

void tp_pkt_adj(struct net_pkt *pkt, int req_len);

enum tp_type tp_msg_to_type(const char *s);

void *tp_malloc(size_t size, const char *file, int line, const char *func);
void tp_free(void *ptr, const char *file, int line, const char *func);
void *tp_calloc(size_t nmemb, size_t size, const char *file, int line,
		const char *func);
void tp_mem_stat(void);

struct net_buf *tp_nbuf_alloc(struct net_buf_pool *pool, size_t len,
				const char *file, int line, const char *func);
struct net_buf *tp_nbuf_clone(struct net_buf *buf, const char *file, int line,
				const char *func);
void tp_nbuf_unref(struct net_buf *nbuf, const char *file, int line,
			const char *func);
void tp_nbuf_stat(void);
void tp_pkt_alloc(struct net_pkt *pkt,
		  const char *file, int line);

struct net_pkt *tp_pkt_clone(struct net_pkt *pkt, const char *file, int line);
void tp_pkt_unref(struct net_pkt *pkt, const char *file, int line);
void tp_pkt_stat(void);

uint32_t tp_seq_track(int kind, uint32_t *pvalue, int req,
			const char *file, int line, const char *func);
void tp_seq_stat(void);

struct tp *json_to_tp(void *data, size_t data_len);
enum tp_type json_decode_msg(void *data, size_t data_len);
struct tp_new *json_to_tp_new(void *data, size_t data_len);
void tp_encode(struct tp *tp, void *data, size_t *data_len);
void tp_new_to_json(struct tp_new *tp, void *data, size_t *data_len);
void tp_new_find_and_apply(struct tp_new *tp, const char *key, void *value,
				int type);
void tp_out(sa_family_t af, struct net_if *iface, const char *msg,
	    const char *key, const char *value);

bool tp_tap_input(struct net_pkt *pkt);

#else /* else of IS_ENABLED(CONFIG_NET_TEST_PROTOCOL) */

#define tp_tap_input(_pkt) false
#define tp_input(_pkt) false
#define tp_out(args...)

#endif /* end of IS_ENABLED(CONFIG_NET_TEST_PROTOCOL) */

#ifdef __cplusplus
}
#endif

#endif /* TP_H */
