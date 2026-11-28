/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_CLIENT_INTERNAL_H_
#define ZEPHYR_NET_TAILSCALE_CLIENT_INTERNAL_H_

#include <time.h>

#include "tailscale_internal.h"

#define TAILSCALE_MIN_VALID_EPOCH INT64_C(1700000000)

int tailscale_realtime_now(struct timespec *now);

struct tailscale_identity {
	uint8_t machine[TAILSCALE_NODE_KEY_SIZE];
	uint8_t node[TAILSCALE_NODE_KEY_SIZE];
	uint8_t disco[TAILSCALE_NODE_KEY_SIZE];
};

#endif /* ZEPHYR_NET_TAILSCALE_CLIENT_INTERNAL_H_ */
