/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZPERF_SESSION_H
#define __ZPERF_SESSION_H

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>

#include "zperf_internal.h"

/* Type definition */
enum state {
	STATE_NULL, /* Session has not yet started */
	STATE_STARTING, /* Session is starting */
	STATE_ONGOING, /* 1st packet has been received, last packet not yet */
	STATE_LAST_PACKET_RECEIVED, /* Last packet has been received */
	STATE_COMPLETED /* Session completed, stats pkt can be sent if needed */
};

struct session {
	int id;

	/* Tuple for UDP */
	uint16_t port;
	struct net_addr ip;

	enum state state;
	enum session_proto proto;

	/* Stat data */
	uint32_t counter;
	uint32_t next_id;
	uint32_t outorder;
	uint32_t error;
	uint64_t length;
	int64_t start_time;
	uint32_t last_time;
	int32_t jitter;
	int32_t last_transit_time;

	/* Stats packet*/
	struct zperf_server_hdr stat;

#ifdef CONFIG_ZPERF_SESSION_PER_THREAD
	struct zperf_results result;
	struct zperf_async_upload_context async_upload_ctx;
	struct zperf_work *zperf;
	bool in_progress; /* is this session finished or not */
	bool wait_for_start; /* wait until the user starts the sessions */
#endif /* CONFIG_ZPERF_SESSION_PER_THREAD */
};

typedef void (*session_cb_t)(struct session *ses, enum session_proto proto,
			     void *user_data);

struct session *get_session(const struct sockaddr *addr,
			    enum session_proto proto);
struct session *get_free_session(const struct sockaddr *addr,
				 enum session_proto proto);
void zperf_session_init(void);
void zperf_reset_session_stats(struct session *session);
/* Reset all sessions for a given protocol. */
void zperf_session_reset(enum session_proto proto);
void zperf_session_foreach(enum session_proto proto, session_cb_t cb,
			   void *user_data);

#endif /* __ZPERF_SESSION_H */
