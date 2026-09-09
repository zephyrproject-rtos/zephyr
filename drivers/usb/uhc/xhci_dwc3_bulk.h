/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DWC3+xHCI platform glue for the xHCI-aligned bulk layer.
 */

#ifndef ZEPHYR_USB_XHCI_DWC3_BULK_H
#define ZEPHYR_USB_XHCI_DWC3_BULK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

struct uhc_dwc3_data;
struct xhci_ring;
struct xhci_td;
struct xhci_ep_ctx;
struct xhci_trb;

uint8_t uhc_dwc3_slot_id_get(struct uhc_dwc3_data *priv);
uint8_t *uhc_dwc3_dev_ctx_get(struct uhc_dwc3_data *priv);
const struct device *uhc_dwc3_device_get(struct uhc_dwc3_data *priv);
mm_reg_t uhc_dwc3_dwc3_base_get(struct uhc_dwc3_data *priv);
mm_reg_t uhc_dwc3_op_base_get(struct uhc_dwc3_data *priv);
uint8_t uhc_dwc3_max_ports_get(struct uhc_dwc3_data *priv);
struct xhci_ring *uhc_dwc3_ep_bulk_ring(struct uhc_dwc3_data *priv, uint8_t dci);
struct xhci_td *uhc_dwc3_bulk_urb_td(struct uhc_dwc3_data *priv, uint8_t dci);
struct xhci_ep_ctx *uhc_dwc3_output_ep_ctx(struct uhc_dwc3_data *priv, unsigned int dci);

uint32_t uhc_dwc3_xhci_read_ep_state(struct uhc_dwc3_data *priv, uint8_t dci);
int uhc_dwc3_xhci_reset_ep(struct uhc_dwc3_data *priv, uint32_t ep_index);
int uhc_dwc3_xhci_stop_ep_ring(struct uhc_dwc3_data *priv, uint32_t ep_index);
int uhc_dwc3_xhci_set_tr_dequeue(struct uhc_dwc3_data *priv, uint32_t ep_index, uint64_t deq);

void xhci_dwc3_bulk_align_ring(struct uhc_dwc3_data *priv, uint8_t dci);

void xhci_dwc3_bulk_sync_ring_from_hw(struct uhc_dwc3_data *priv, uint8_t dci);

int xhci_dwc3_bulk_queue_prologue(struct uhc_dwc3_data *priv, uint8_t dci);
int xhci_dwc3_bulk_abort_td(struct uhc_dwc3_data *priv, uint8_t dci);

int xhci_dwc3_bulk_recover_ep(struct uhc_dwc3_data *priv, uint8_t dci);

void xhci_dwc3_bulk_td_giveback(struct uhc_dwc3_data *priv, uint8_t dci, bool dir_in, int br,
				uint32_t req_len);

uint8_t uhc_dwc3_bulk_urb_xfer_ep(struct uhc_dwc3_data *priv, uint8_t dci);

#endif /* ZEPHYR_USB_XHCI_DWC3_BULK_H */
