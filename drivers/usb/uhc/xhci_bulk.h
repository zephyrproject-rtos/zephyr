/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * xHCI-aligned bulk/interrupt transfer layer (xhci-ring.c: finish_td,
 * queue_bulk_tx prologue, process_bulk_intr_td, giveback + ring sync).
 */

#ifndef ZEPHYR_USB_XHCI_BULK_H
#define ZEPHYR_USB_XHCI_BULK_H

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/sys/slist.h>

#include "xhci_ring.h"

struct xhci_td {
	struct xhci_trb *start_trb;
	unsigned int start_cycle;
	uint8_t trb_count;
	sys_snode_t node;
};

struct xhci_bulk_hcd_ops {
	void *ctx;
	uint8_t slot_id;
	uint32_t (*read_ep_state)(void *ctx, uint8_t dci);
	int (*reset_ep)(void *ctx, uint32_t ep_index);
	int (*stop_ep)(void *ctx, uint32_t ep_index);
	int (*set_tr_dequeue)(void *ctx, uint32_t ep_index, uint64_t deq);
	/*
	 * Platform: align SW/HW ring heads (never regress SW producer — Set TR Dequeue
	 * when HW consumer lags after finish_td).
	 */
	void (*sync_ring_from_hw)(void *ctx, uint8_t dci);
};

uint64_t xhci_bulk_deq_operand(const struct xhci_ring *ring);

void xhci_bulk_finish_td(struct xhci_ring *ring, const struct xhci_td *td, int br);

/*
 * Retire a failed TD from the software ring without re-doorbelling the same TRB
 * (Versal integrated DWC3+xHCI: xHCI TSP soft retry re-rings stale TRBs → COMP=4 loop).
 */
void xhci_bulk_abandon_td(struct xhci_ring *ring, const struct xhci_td *td);

int xhci_bulk_recover_ep(const struct xhci_bulk_hcd_ops *ops, uint8_t dci, struct xhci_ring *ring);

/*
 * xHCI_queue_bulk_tx() prologue: HW ring sync (IN and OUT), then recover
 * endpoint if HALTED/ERROR/STOPPED.
 */
int xhci_bulk_queue_prologue(const struct xhci_bulk_hcd_ops *ops, uint8_t dci,
			     struct xhci_ring *ring, bool bulk_in);

bool xhci_bulk_event_is_interior(const struct xhci_td *td, uint64_t evt_trb_ptr, uint32_t cc,
				 uint64_t ioc_trb_phys);

bool xhci_bulk_event_belongs_to_td(const struct xhci_ring *ring, const struct xhci_td *td,
				   uint64_t evt_trb_ptr);

uint64_t xhci_bulk_td_ioc_phys(const struct xhci_ring *ring, const struct xhci_td *td);

/*
 * xHCI finish_td + optional HW sync after successful completion (both directions).
 */
void xhci_bulk_td_completed(const struct xhci_bulk_hcd_ops *ops, uint8_t dci,
			    struct xhci_ring *ring, const struct xhci_td *td, bool bulk_in, int br);

#endif /* ZEPHYR_USB_XHCI_BULK_H */
