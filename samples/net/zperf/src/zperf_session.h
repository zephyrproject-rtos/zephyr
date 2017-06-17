/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZPERF_SESSION_H
#define __ZPERF_SESSION_H

#include <linker/sections.h>
#include <toolchain.h>

#include <zephyr.h>
#include <misc/printk.h>

#include <net/net_ip.h>
#include <net/net_core.h>

#include "zperf.h"
#include "zperf_internal.h"
#include "shell_utils.h"

/* Type definition */
enum state {
	STATE_NULL, /* Session has not yet started */
	STATE_ONGOING, /* 1st packet has been received, last packet not yet */
	STATE_LAST_PACKET_RECEIVED, /* Last packet has been received */
	STATE_COMPLETED /* Session completed, stats pkt can be sent if needed */
};

enum session_proto {
	SESSION_UDP = 0,
	SESSION_TCP = 1,
	SESSION_PROTO_END
};

struct session {
	/* Tuple */
	u16_t port;

	struct net_addr ip;

	enum state state;

	/* Stat data */
	u32_t counter;
	u32_t next_id;
	u32_t outorder;
	u32_t error;
	u64_t length;
	u32_t start_time;
	u32_t last_time;
	s32_t jitter;
	s32_t last_transit_time;

	/* Stats packet*/
	struct zperf_server_hdr stat;
};

struct session *get_session(struct net_pkt *pkt, enum session_proto proto);
void zperf_session_init(void);
void zperf_reset_session_stats(struct session *session);

#endif /* __ZPERF_SESSION_H */
