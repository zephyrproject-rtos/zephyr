/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NATS_H
#define __NATS_H

#include <zephyr/types.h>
#include <net/net_context.h>

struct nats_msg {
	const char *subject;
	const char *sid;
	const char *reply_to;
	const char *payload;
	size_t payload_len;
};

struct nats {
	struct net_context *conn;

	int (*on_auth_required)(const struct nats *nats,
				char *user, size_t *user_len,
				char *pass, size_t *pass_len);
	int (*on_message)(const struct nats *nats,
			  const struct nats_msg *msg);
};

int nats_connect(struct nats *nats, struct sockaddr *addr, socklen_t addrlen);
int nats_disconnect(struct nats *nats);

int nats_subscribe(const struct nats *nats,
		   const char *subject, size_t subject_len,
		   const char *queue_group, size_t queue_group_len,
		   const char *sid, size_t sid_len);
int nats_unsubscribe(const struct nats *nats,
		     const char *sid, size_t sid_len,
		     size_t max_msgs);

int nats_publish(const struct nats *nats,
		 const char *subject, size_t subject_len,
		 const char *reply_to, size_t reply_to_len,
		 const char *payload, size_t payload_len);

#endif /* __NATS_H */
