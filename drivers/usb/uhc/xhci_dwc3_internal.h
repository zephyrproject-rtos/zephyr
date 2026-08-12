/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DWC3 xHCI internal helpers and cross-module declarations.
 */

#ifndef ZEPHYR_USB_XHCI_DWC3_INTERNAL_H
#define ZEPHYR_USB_XHCI_DWC3_INTERNAL_H

#include <stddef.h>
#include <stdint.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/usb/usb_ch9.h>

#include "xhci_dwc3_priv.h"
#include "xhci_dma.h"
#include "xhci_hw.h"
#include "xhci_dwc3_log.h"
#include "xhci_ring.h"

#define XHCI_TRB_64K_BOUND 0x10000U

static inline uint32_t dwc3_readl(mm_reg_t base, uint32_t offset)
{
	return sys_read32(base + offset);
}

static inline void dwc3_writel(mm_reg_t base, uint32_t offset, uint32_t val)
{
	sys_write32(val, base + offset);
}

static inline uint32_t xhci_readl(mm_reg_t base, uint32_t offset)
{
	return sys_read32(base + offset);
}

static inline void xhci_writel(mm_reg_t base, uint32_t offset, uint32_t val)
{
	sys_write32(val, base + offset);
}

static inline void xhci_writeq(const struct uhc_dwc3_data *priv, mm_reg_t base, uint32_t offset,
			       uint64_t val)
{
	if (priv != NULL && priv->write_64_hi_lo) {
		sys_write32((uint32_t)(val >> 32), base + offset + 4);
		sys_write32((uint32_t)val, base + offset);
	} else {
		sys_write32((uint32_t)val, base + offset);
		sys_write32((uint32_t)(val >> 32), base + offset + 4);
	}
}

static inline uint64_t xhci_readq(mm_reg_t base, uint32_t offset)
{
	uint32_t lo = sys_read32(base + offset);
	uint32_t hi = sys_read32(base + offset + 4);

	return ((uint64_t)hi << 32) | lo;
}

static inline void dwc3_dma_prep_rx(void *addr, size_t size)
{
	dwc3_dma_prep_rx_aligned(addr, size);
}

static inline void dwc3_dma_store_release_before_host_fetch(void)
{
	barrier_dsync_fence_full();
}

static inline void xhci_trb_touch_after_flush(const struct xhci_trb *t)
{
	if (t != NULL) {
		(void)*(const volatile uint32_t *)&t->control;
	}
}

static inline void xhci_dma_fence_before_ep_doorbell(const struct xhci_trb *first_trb)
{
	dwc3_dma_store_release_before_host_fetch();
	xhci_trb_touch_after_flush(first_trb);
}

static inline uint64_t xhci_dma_addr(const void *v)
{
	return (uint64_t)k_mem_phys_addr((void *)(uintptr_t)v);
}

static inline struct xhci_ep_ctx *xhci_slot_output_ep_ctx(const struct uhc_dwc3_data *priv,
							  unsigned int dci)
{
	return (struct xhci_ep_ctx *)(priv->dev_ctx + (size_t)dci * (size_t)priv->ctx_bytes);
}

static inline void xhci_flush_bulk_td(struct xhci_ring *ring, struct xhci_trb *t)
{
	dwc3_dma_flush_aligned(t, sizeof(*t));
	dwc3_dma_flush_aligned(&ring->trbs[ring->num_trbs - 1], sizeof(struct xhci_trb));
	xhci_dma_fence_before_ep_doorbell(t);
}

static inline void ring_cmd_doorbell(struct uhc_dwc3_data *priv)
{
	xhci_writel(priv->db_base, 0U, XHCI_DB_HOST);
	(void)xhci_readl(priv->db_base, 0U);
}

static inline void ring_ep_doorbell(struct uhc_dwc3_data *priv, uint32_t slot_id, uint32_t ep_id)
{
	xhci_writel(priv->db_base, slot_id * XHCI_DB_STRIDE, ep_id);
	(void)xhci_readl(priv->db_base, slot_id * XHCI_DB_STRIDE);
}

/* xhci_hcd.c */
unsigned int dwc3_usb2_root_port(const struct uhc_dwc3_data *priv,
				 const struct uhc_dwc3_config *cfg);
void dwc3_build_port_poll_order(unsigned int *order, unsigned int *n_order, unsigned int maxp,
				unsigned int usb2_first);
int xhci_reset(struct uhc_dwc3_data *priv, mm_reg_t xhci_base);
int xhci_setup(struct uhc_dwc3_data *priv);
int xhci_start(struct uhc_dwc3_data *priv);
void xhci_post_start_interrupter0(struct uhc_dwc3_data *priv);
void xhci_poll_boot_connected_device(struct uhc_dwc3_data *priv, const struct device *dev);

/* xhci_cmd.c */
int xhci_send_command_ex(struct uhc_dwc3_data *priv, uint32_t param_lo, uint32_t param_hi,
			 uint32_t status, uint32_t control, bool drain_evt_ring);
int xhci_send_command(struct uhc_dwc3_data *priv, uint32_t param_lo, uint32_t param_hi,
		      uint32_t status, uint32_t control);
int xhci_cmd_configure_endpoint(struct uhc_dwc3_data *priv);
uint8_t xhci_usb_ep_addr_to_dci(uint8_t ep_addr);
struct usb_ep_descriptor *xhci_ep_desc_for_dci(struct usb_device *udev, uint8_t dci);
uint32_t xhci_int_ep_info_field(const struct usb_ep_descriptor *epd, enum usb_device_speed speed);
int xhci_dwc3_bulk_output_eps_steady(struct uhc_dwc3_data *priv, struct usb_device *udev);
void xhci_dwc3_verify_post_configure(struct uhc_dwc3_data *priv, struct usb_device *udev,
				     const uint8_t *dci_has_desc, unsigned int max_dci);
int xhci_evaluate_context_copy_output(struct uhc_dwc3_data *priv, unsigned int max_dci,
				      const uint8_t *dci_has_desc);
void xhci_dwc3_reset_bandwidth_sw(struct uhc_dwc3_data *priv);
int xhci_dwc3_configure_non_ep0(struct uhc_dwc3_data *priv, struct usb_device *udev);
int xhci_cmd_stop_ep_ring(struct uhc_dwc3_data *priv, uint32_t ep_index);
int xhci_cmd_set_tr_dequeue_deq(struct uhc_dwc3_data *priv, uint32_t ep_index, uint64_t deq);
int xhci_bulk_eps_reconfigure_drop_add(struct uhc_dwc3_data *priv, struct usb_device *udev,
				       bool force_drop_add);
int xhci_enable_slot(struct uhc_dwc3_data *priv);
int xhci_disable_slot_cmd(struct uhc_dwc3_data *priv, uint8_t sid);
void xhci_reset_sw_transfer_rings(struct uhc_dwc3_data *priv);
void xhci_teardown_active_slot(struct uhc_dwc3_data *priv);

/* xhci_ep0.c */
void xhci_flush_ep0_td(struct xhci_ring *ring, struct xhci_trb *setup, struct xhci_trb *data,
		       struct xhci_trb *status);
void xhci_copy_ep0_dequeue_into_input_ctx(struct uhc_dwc3_data *priv);
void xhci_ep0_ring_sync_from_hw(struct uhc_dwc3_data *priv);
void xhci_ep0_verify_mps_matches(struct uhc_dwc3_data *priv, uint16_t expect, const char *tag);
bool xhci_ep0_ring_verify_dequeue_matches_sw(struct uhc_dwc3_data *priv);
uint32_t ep0_td_size_packets_after_setup(uint16_t w_length, uint16_t mps0);
int dwc3_xfer_sem_take_ep0(const struct device *dev, struct uhc_dwc3_data *priv,
			   k_timeout_t timeout);
int xhci_address_device_initial(struct uhc_dwc3_data *priv, uint8_t port, uint8_t speed);
int xhci_address_device_set_address_bsr0(struct uhc_dwc3_data *priv);
int xhci_evaluate_ep0_mps(struct uhc_dwc3_data *priv, uint16_t mps);

/* xhci_event.c */
uint32_t xhci_in_bytes_from_event(uint32_t buf_len, uint32_t lenfield, uint32_t cc);
void xhci_bulk_giveback_urb(struct uhc_dwc3_data *priv, uint8_t dci, int br, uint32_t cc,
			    uint32_t lenfield);
void xhci_dbg_log_normal_trb(const struct xhci_trb *t, const char *ctx);
enum uhc_event_type xhci_port_speed_to_connect_event(uint8_t xhci_speed);
void xhci_handle_event(struct uhc_dwc3_data *priv, struct xhci_trb *evt);
int xhci_wait_cmd_sem(struct uhc_dwc3_data *priv, bool drain_evt_ring);
void xhci_process_events_nolock(struct uhc_dwc3_data *priv);
void xhci_process_events(struct uhc_dwc3_data *priv);
void xhci_event_work_handler(struct k_work *work);
void uhc_dwc3_isr(const struct device *dev);

int xhci_cancel_ep_xfer(struct uhc_dwc3_data *priv, uint8_t dci, struct uhc_transfer *xfer,
			int err);

#endif /* ZEPHYR_USB_XHCI_DWC3_INTERNAL_H */
