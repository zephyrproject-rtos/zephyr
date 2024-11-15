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

#if defined(CONFIG_SLIP_TAP)
#define _SLIP_MTU 1500
#else
#define _SLIP_MTU 576
#endif /* CONFIG_SLIP_TAP */

struct slip_context {
	bool init_done;
	bool first;		/* SLIP received it's byte or not after
				 * driver initialization or SLIP_END byte.
				 */
	uint8_t buf[1];		/* SLIP data is read into this buf */
	struct net_pkt *rx;	/* and then placed into this net_pkt */
	struct net_buf *last;	/* Pointer to last buffer in the list */
	uint8_t *ptr;		/* Where in net_pkt to add data */
	struct net_if *iface;
	uint8_t state;

	uint8_t mac_addr[6];
	struct net_linkaddr ll_addr;

#if defined(CONFIG_SLIP_STATISTICS)
#define SLIP_STATS(statement)
#else
	uint16_t garbage;
#define SLIP_STATS(statement) statement
#endif
};

void slip_iface_init(struct net_if *iface);
int slip_init(const struct device *dev);
int slip_send(const struct device *dev, struct net_pkt *pkt);
