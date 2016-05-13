/*
 * Copyright (c) 2015 Intel Corporation
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
#ifndef __ZPERF_SESSION_H
#define __ZPERF_SESSION_H

#include <misc/printk.h>
#include <net/net_core.h>
#include <net/net_socket.h>
#include <net/ip_buf.h>
#include <net/net_ip.h>
#include <sections.h>
#include <toolchain.h>
#include <zephyr.h>

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
	uint16_t port;
	uip_ipaddr_t ip;

	enum state state;

	/* Stat data */
	uint32_t counter;
	uint32_t next_id;
	uint32_t outorder;
	uint32_t error;
	uint64_t length;
	uint32_t start_time;
	uint32_t last_time;
	int32_t jitter;
	int32_t last_transit_time;

	/* Stats packet*/
	struct zperf_server_hdr stat;
};

struct session *get_session(struct net_buf *buf, enum session_proto proto);
void zperf_session_init(void);
void zperf_reset_session_stats(struct session *session);

#endif /* __ZPERF_SESSION_H */
