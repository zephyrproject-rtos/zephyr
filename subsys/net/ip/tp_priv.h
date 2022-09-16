/*
 * Copyright (c) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TP_PRIV_H
#define TP_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/net/net_pkt.h>

#define tp_dbg(fmt, args...) printk("%s: " fmt "\n", __func__, ## args)
#define tp_err(fmt, args...) do {				\
	printk("%s: Error: " fmt "\n", __func__, ## args);	\
	k_oops();						\
} while (0)

#define tp_assert(cond, fmt, args...) do {			\
	if ((cond) == false) {					\
		printk("%s: Assertion failed: %s, " fmt "\n",	\
			__func__, #cond, ## args);		\
		k_oops();					\
	}							\
} while (0)

#define is(_a, _b) (strcmp((_a), (_b)) == 0)
#define ip_get(_x) ((struct net_ipv4_hdr *) net_pkt_ip_data((_x)))

#define TP_MEM_HEADER_COOKIE 0xAAAAAAAA
#define TP_MEM_FOOTER_COOKIE 0xBBBBBBBB

struct tp_mem {
	sys_snode_t next;
	const char *file;
	int line;
	const char *func;
	size_t size;
	uint32_t *footer;
	uint32_t header;
	uint8_t mem[];
};

struct tp_nbuf {
	sys_snode_t next;
	struct net_buf *nbuf;
	const char *file;
	int line;
};

struct tp_pkt {
	sys_snode_t next;
	struct net_pkt *pkt;
	const char *file;
	int line;
};

struct tp_seq {
	sys_snode_t next;
	const char *file;
	int line;
	const char *func;
	int kind;
	int req;
	uint32_t value;
	uint32_t old_value;
	int of;
};

#ifdef __cplusplus
}
#endif

#endif /* TP_PRIV_H */
