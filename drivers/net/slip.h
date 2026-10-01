/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>

struct slip_config {
	const struct device *uart;
};

struct slip_context {
	bool first;		/* SLIP received it's byte or not after
				 * driver initialization or SLIP_END byte.
				 */
	struct net_pkt *rx;	/* SLIP data is placed into this net_pkt */
	struct net_buf *last;	/* Pointer to last buffer in the list */
	uint8_t *ptr;		/* Where in net_pkt to add data */
	struct net_if *iface;
	uint8_t state;

#if defined(CONFIG_SLIP_STATISTICS)
#define SLIP_STATS(statement)
#else
	uint16_t garbage;
#define SLIP_STATS(statement) statement
#endif
};

void slip_iface_init(struct net_if *iface);
int slip_send(const struct device *dev, struct net_pkt *pkt);
