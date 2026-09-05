/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * xHCI-aligned xHCI transfer/command ring producer (xhci-ring.c: inc_enq,
 * prepare_ring, queue_trb, link TRB).
 */

#ifndef ZEPHYR_USB_XHCI_RING_H
#define ZEPHYR_USB_XHCI_RING_H

#include <stdint.h>

#include <zephyr/sys/slist.h>

#include "xhci_hw.h"

struct xhci_trb;

struct xhci_ring {
	struct xhci_trb *trbs;
	uint32_t num_trbs;
	uint32_t enqueue;
	uint32_t dequeue;
	uint32_t cycle_state;
	sys_slist_t td_list;
};

void xhci_ring_init(struct xhci_ring *ring, struct xhci_trb *trbs, uint32_t num_trbs,
		    uint32_t link_extra_ctl);

void xhci_ring_event_segment_init(struct xhci_ring *ring, struct xhci_trb *trbs, uint32_t num_trbs);

struct xhci_trb *xhci_ring_enqueue(struct xhci_ring *ring, unsigned int *trb_cycle_out);

void xhci_ring_inc_deq(struct xhci_ring *ring);

int xhci_ring_prepare(struct xhci_ring *ring, unsigned int num_trbs_needed);

struct xhci_trb *xhci_ring_queue_trb(struct xhci_ring *ring, unsigned int *trb_cycle_out);

void xhci_ring_td_link_chain_continue(struct xhci_ring *ring);

void xhci_ring_link_trb_begin_td(struct xhci_ring *ring);

#endif /* ZEPHYR_USB_XHCI_RING_H */
