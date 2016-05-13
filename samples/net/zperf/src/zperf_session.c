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

#include <zephyr.h>
#include "zperf_session.h"

#define SESSION_MAX 4

static struct session sessions[SESSION_PROTO_END][SESSION_MAX];

/* Get session from a given packet */
struct session *get_session(struct net_buf *buf, enum session_proto proto)
{
	uint16_t port;
	uip_ipaddr_t ip;
	int i = 0;
	struct session *active = NULL;
	struct session *free = NULL;

	if (!buf) {
		printk("Error! null buf detected.\n");
		return NULL;
	}

	if (proto != SESSION_TCP && proto != SESSION_UDP) {
		printk("Error! unsupported proto.\n");
		return NULL;
	}

	/* Get tuple of the remote connection */
	port = NET_BUF_UDP(buf)->srcport;
	uip_ipaddr_copy(&ip, &NET_BUF_IP(buf)->srcipaddr);

	/* Check whether we already have an active session */
	while (active == NULL && i < SESSION_MAX) {
		struct session *ptr = &sessions[proto][i];

		if (ptr->port == port && uip_ipaddr_cmp(&ptr->ip, &ip)) {
			/* We found an active session */
			active = ptr;
		} else if (free == NULL
				&& (ptr->state == STATE_NULL
					|| ptr->state == STATE_COMPLETED)) {
			/* We found a free slot - just in case */
			free = ptr;
		}
		i++;
	}

	/* If no active session then create a new one */
	if (active == NULL && free != NULL) {
		active = free;
		active->port = port;
		uip_ipaddr_copy(&active->ip, &ip);
	}

	return active;
}

void zperf_reset_session_stats(struct session *session)
{
	if (!session)
		return;

	session->counter = 0;
	session->start_time = 0;
	session->next_id = 0;
	session->length = 0;
	session->outorder = 0;
	session->error = 0;
	session->jitter = 0;
	session->last_transit_time = 0;
}

void zperf_session_init(void)
{
	for (int i = 0; i < SESSION_PROTO_END; i++) {
		for (int j = 0; j < SESSION_MAX; j++) {
			sessions[i][j].state = STATE_NULL;
			zperf_reset_session_stats(&(sessions[i][j]));
		}
	}
}
