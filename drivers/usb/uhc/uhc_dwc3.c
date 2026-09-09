/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal DWC3 xHCI USB Host Controller driver for Zephyr.
 *
 * Supports control and bulk transfers on a single port.
 * Based on xHCI 1.x, DWC3 programming guides, and the AMD BURST host flow in
 * `xusb_host_example.c` (Versal / ZynqMP reference).
 */

#define DT_DRV_COMPAT snps_dwc3

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/clock.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/net_buf.h>

#include "uhc_common.h"
#include "dwc3_regs.h"
#include "dwc3_xilinx.h"
#include "dwc3_host.h"
#include "xhci_dwc3_priv.h"
#include "xhci_dwc3_internal.h"
#include "xhci_hw.h"
#include "xhci_ring.h"
#include "xhci_bulk.h"
#include "xhci_dwc3_bulk.h"
#include "xhci_dma.h"
#include "xhci_dwc3_log.h"

LOG_MODULE_REGISTER(uhc_dwc3, CONFIG_UHC_DRIVER_LOG_LEVEL);

static int uhc_dwc3_lock(const struct device *dev);
static int uhc_dwc3_unlock(const struct device *dev);

static uint32_t xhci_bulk_bytes_to_64k_boundary(uint64_t buf_phys, uint32_t remaining)
{
	uint32_t offset = (uint32_t)(buf_phys & 0xffffULL);
	uint32_t room = XHCI_TRB_64K_BOUND - offset;

	return (remaining < room) ? remaining : room;
}

/* xHCI 1.0+ TD SIZE field (xhci_td_remainder, non-MTK path). */
static uint32_t xhci_bulk_td_size_remainder(uint32_t transferred_before, uint32_t trb_chunk_len,
					    uint32_t td_total_len, uint32_t maxp,
					    bool more_trbs_coming)
{
	if (!more_trbs_coming || (transferred_before == 0U && trb_chunk_len == 0U) ||
	    trb_chunk_len == td_total_len) {
		return 0U;
	}

	if (maxp == 0U) {
		return 0U;
	}

	/* xHCI_td_remainder (xHCI 6.4.1 / 4.9.2). */
	uint32_t total_packets = (td_total_len + maxp - 1U) / maxp;
	uint32_t done_packets = (transferred_before + trb_chunk_len) / maxp;

	if (total_packets > done_packets) {
		return total_packets - done_packets;
	}

	return 0U;
}
static uint32_t xhci_bulk_next_chunk_len(const void *dma_base, uint32_t run, uint32_t remaining,
					 uint16_t ep_mps_sz, bool dir_in)
{
	uint64_t chunk_phys = xhci_dma_addr((uint8_t *)dma_base + run);
	uint32_t chunk = xhci_bulk_bytes_to_64k_boundary(chunk_phys, remaining);

	if (dir_in && (chunk_phys & 63ULL) != 0ULL && ep_mps_sz > 0U) {
		chunk = MIN(chunk, (uint32_t)ep_mps_sz);
	}

	return chunk;
}

static uint32_t xhci_bulk_count_trbs_for_td(const void *dma_base, uint32_t td_len,
					    uint16_t ep_mps_sz, bool dir_in)
{
	uint32_t needed = 0U;
	uint32_t run = 0U;
	uint32_t rem = td_len;
	while (rem > 0U) {
		uint32_t chunk = xhci_bulk_next_chunk_len(dma_base, run, rem, ep_mps_sz, dir_in);

		run += chunk;
		rem -= chunk;
		needed++;
	}

	return needed;
}
/* -------------------------------------------------------------------------- */
/* Bulk TRB publish helpers (ep_enqueue path)                                 */
/* -------------------------------------------------------------------------- */

static void xhci_bulk_flush_td_chain(struct xhci_ring *ring, struct xhci_trb *start_trb,
				     uint8_t trb_count)
{
	uint64_t seg;
	uint32_t idx;

	if (ring->trbs == NULL || start_trb == NULL || trb_count == 0U) {
		return;
	}

	seg = xhci_dma_addr(ring->trbs);
	idx = (uint32_t)((xhci_dma_addr(start_trb) - seg) / 16U);

	for (uint8_t t = 0U; t < trb_count; t++) {
		uint32_t ti = idx + (uint32_t)t;

		if (ti >= ring->num_trbs - 1U) {
			break;
		}
		xhci_flush_bulk_td(ring, &ring->trbs[ti]);
	}
}

static void xhci_bulk_giveback_first_trb(struct uhc_dwc3_data *priv, struct xhci_ring *ring,
					 struct xhci_trb *start_trb, unsigned int start_cycle,
					 uint8_t trb_count, uint32_t slot_id, uint32_t dci)
{
	xhci_bulk_flush_td_chain(ring, start_trb, trb_count);

	barrier_dmem_fence_full();
	if (start_cycle != 0U) {
		start_trb->control |= XHCI_TRB_CYCLE;
	} else {
		start_trb->control &= ~XHCI_TRB_CYCLE;
	}
	dwc3_dma_flush_aligned(start_trb, sizeof(*start_trb));

	ring_ep_doorbell(priv, slot_id, dci);
}

static int dwc3_ep_sync_after_clear_feature_cb(const struct device *dev, struct usb_device *udev)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	const struct uhc_dwc3_config *cfg = dev->config;
	/*
	 * Integrated DWC3+xHCI platforms may report RUNNING in the output
	 * context after CLEAR_FEATURE(HALT) while the pipe is STOPPED.
	 */
	const bool force_drop_add =
		xhci_plat_quirk(&cfg->xhci_plat, XHCI_QUIRK_BULK_FORCE_DROP_ADD);

	if (udev == NULL || udev->cfg_desc == NULL) {
		return -EINVAL;
	}

	UHC_DWC3_BULK_FLOW_INF("uhc_flow: bulk sync after HALT clear addr=%u force_drop_add=%d",
			       (unsigned int)udev->addr, force_drop_add ? 1 : 0);

	return xhci_bulk_eps_reconfigure_drop_add(priv, udev, force_drop_add);
}
static int uhc_dwc3_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_dwc3_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static int uhc_dwc3_init(const struct device *dev)
{
	const struct uhc_dwc3_config *cfg = dev->config;
	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	int ret;

	priv->dev = dev;
	k_sem_init(&priv->cmd_sem, 0, 1);
	k_sem_init(&priv->xfer_sem, 0, 1);
	for (unsigned int dci = 0U; dci < 32U; dci++) {
		priv->bulk_expect_ioc_trb_phys[dci] = 0ULL;
		priv->bulk_td_trb_count[dci] = 0U;
		priv->bulk_xfer_result[dci] = 0;
		priv->bulk_xfer_length[dci] = 0U;
		priv->bulk_xfer_comp_code[dci] = 0U;
	}
	k_mutex_init(&priv->evt_mutex);
	k_work_init(&priv->event_work, xhci_event_work_handler);

	DEVICE_MMIO_NAMED_MAP(dev, core, K_MEM_CACHE_NONE);
	if (cfg->usb2_wrapper_present) {
		DEVICE_MMIO_NAMED_MAP(dev, usb2_wrapper, K_MEM_CACHE_NONE);
	}

	ret = dwc3_host_burst_init(dev);
	if (ret != 0) {
		LOG_ERR("DWC3 BURST host init failed: %d", ret);
		return ret;
	}

	ret = xhci_reset(priv, uhc_dwc3_core_mmio(dev));
	if (ret != 0) {
		LOG_ERR("xHCI capability/CNR probe failed: %d", ret);
		return ret;
	}

	ret = xhci_setup(priv);
	if (ret != 0) {
		LOG_ERR("xHCI ring/context setup failed: %d", ret);
		return ret;
	}

	priv->slot_id = 0U;
	priv->port_speed = 0U;
	priv->root_connect_submitted = false;
	priv->root_port = 1U;
	priv->write_64_hi_lo = xhci_plat_quirk(&cfg->xhci_plat, XHCI_QUIRK_WRITE_64_HI_LO);

	LOG_DBG("MMIO and xHCI data structures initialized");
	return 0;
}

/*
 * xHCI_start() after RUN: IMOD=0, IMAN=0, then enable IMAN.IE for IRQ events.
 */

static int uhc_dwc3_enable(const struct device *dev)
{
	const struct uhc_dwc3_config *cfg = dev->config;
	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	int ret;

	if (priv->op_base == 0U) {
		LOG_ERR("not initialized");
		return -ENODEV;
	}

	ret = xhci_start(priv);
	if (ret != 0) {
		LOG_ERR("xHCI RUN failed: %d", ret);
		return ret;
	}

	dwc3_host_susphy_post_run(dev, false);

	xhci_post_start_interrupter0(priv);

	cfg->irq_enable_func(dev);

	LOG_DBG("xHCI controller running with interrupts enabled");

	xhci_poll_boot_connected_device(priv, dev);

	return 0;
}

static int uhc_dwc3_disable(const struct device *dev)
{
	const struct uhc_dwc3_config *cfg = dev->config;
	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	uint32_t cmd;
	int timeout;

	if (priv->op_base == 0U) {
		return 0;
	}

	if (priv->slot_id != 0U) {
		xhci_teardown_active_slot(priv);
	}

	priv->root_connect_submitted = false;
	priv->steady_after_configure_ep = false;
	priv->root_port = 1U;

	cmd = xhci_readl(priv->op_base, XHCI_OP_USBCMD);
	cmd &= ~(XHCI_USBCMD_RUN | XHCI_USBCMD_INTE | XHCI_USBCMD_HSEE);
	xhci_writel(priv->op_base, XHCI_OP_USBCMD, cmd);

	timeout = 1000;
	while (!(xhci_readl(priv->op_base, XHCI_OP_USBSTS) & XHCI_USBSTS_HCH) && timeout > 0) {
		k_busy_wait(100);
		timeout--;
	}

	cfg->irq_disable_func(dev);
	return 0;
}

static int uhc_dwc3_shutdown(const struct device *dev)
{
	return uhc_dwc3_disable(dev);
}

static void uhc_dwc3_free_dev(const struct device *dev)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);

	if (priv->op_base == 0U) {
		return;
	}

	priv->steady_after_configure_ep = false;
	xhci_teardown_active_slot(priv);
}

static int uhc_dwc3_bus_reset(const struct device *dev)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	const struct uhc_dwc3_config *cfg = dev->config;
	uint8_t connect_rp = priv->root_port != 0U ? priv->root_port : 1U;
	uint32_t port_off =
		XHCI_OP_PORTSC_BASE + ((uint32_t)connect_rp - 1U) * XHCI_PORT_REGS_STRIDE;
	uint32_t portsc;
	int timeout;
	int ret;

	if (priv->op_base == 0U) {
		return -EIO;
	}

	const bool cfg_ep_steady = priv->steady_after_configure_ep;

	if (cfg_ep_steady) {
		LOG_WRN("bus_reset after Configure Endpoint (unexpected in steady state; "
			"expect reset only on new enumeration or disconnect)");
	}
	priv->steady_after_configure_ep = false;

	/*
	 * Disconnect clears steady_after_configure_ep but left slot_id set; the next
	 * bus_reset must Disable Slot and reset rings so ENABLE_SLOT runs again.
	 * If this reset is the "unexpected during steady" path, keep the slot.
	 */
	if (priv->slot_id != 0U && !cfg_ep_steady) {
		xhci_teardown_active_slot(priv);
	}

	UHC_DWC3_DBG("bus_reset: enter CSC port %u HC N_PORTS=%u %d", (unsigned int)connect_rp,
		     (unsigned int)priv->max_ports, port_off);

	portsc = xhci_readl(priv->op_base, port_off);
	if (!(portsc & XHCI_PORTSC_PP)) {
		portsc = xhci_port_state_to_neutral(portsc) | XHCI_PORTSC_PP;
		xhci_writel(priv->op_base, port_off, portsc);
		k_busy_wait(20000);
		portsc = xhci_readl(priv->op_base, port_off);
	}

	if (!(portsc & XHCI_PORTSC_CCS)) {
		LOG_WRN("no device on root port %u", connect_rp);
		return -ENODEV;
	}

	priv->port_speed = XHCI_PORTSC_SPEED(portsc);

	/*
	 * FS/HS enumeration uses the USB2 companion PORTSC when the HC exposes
	 * two roots (SS=port1, USB2=port2). CSC/PSCE may still say "port 1" while
	 * PR/PED advance on port 2 — polling port 1 then looks "stuck" at 0x…6f1.
	 */
	uint8_t reset_rp = connect_rp;

	if (priv->max_ports >= 2U && priv->port_speed < XHCI_SPEED_SUPER) {
		reset_rp = (uint8_t)dwc3_usb2_root_port(priv, cfg);
		port_off = XHCI_OP_PORTSC_BASE + ((uint32_t)reset_rp - 1U) * XHCI_PORT_REGS_STRIDE;
		if (reset_rp != connect_rp) {
			UHC_DWC3_DBG("bus_reset: USB2 reset on port %u (CSC was %u)",
				     (unsigned int)reset_rp, (unsigned int)connect_rp);
			portsc = xhci_readl(priv->op_base, port_off);
		}
	}

	priv->root_port = reset_rp;

	if (priv->port_speed < XHCI_SPEED_SUPER) {
		/* Clear stale change bits before asserting a new reset (W1C). */
		portsc = xhci_readl(priv->op_base, port_off);
		{
			uint32_t w1c = portsc & XHCI_PORTSC_W1C_MASK;

			if (w1c != 0U) {
				xhci_writel(priv->op_base, port_off,
					    xhci_port_state_to_neutral(portsc) | w1c);
				portsc = xhci_readl(priv->op_base, port_off);
			}
		}

		/* Assert PR (USB2 hot reset). */
		portsc = xhci_readl(priv->op_base, port_off);
		portsc = xhci_port_state_to_neutral(portsc) | XHCI_PORTSC_PR;
		xhci_writel(priv->op_base, port_off, portsc);
		UHC_DWC3_DBG("bus_reset: wait PR clear (~2s)");

		timeout = 2000;
		while (timeout > 0) {
			k_busy_wait(1000);
			portsc = xhci_readl(priv->op_base, port_off);

			if (((unsigned int)timeout % 250U) == 0U) {
				UHC_DWC3_DBG("bus_reset: PORTSC=0x%08x left=%d PR=%u", portsc,
					     timeout, (portsc & XHCI_PORTSC_PR) ? 1U : 0U);
			}

			if ((portsc & XHCI_PORTSC_PR) == 0U) {
				UHC_DWC3_DBG("bus_reset: PR cleared PORTSC=0x%08x", portsc);
				break;
			}

			timeout--;
		}

		if (timeout == 0) {
			LOG_ERR("bus_reset: timeout waiting PR clear");
			return -ETIMEDOUT;
		}

		k_busy_wait(20000);

		UHC_DWC3_DBG("bus_reset: after recovery delay PORTSC=0x%08x CCS=%u PED=%u", portsc,
			     (portsc & XHCI_PORTSC_CCS) != 0U ? 1U : 0U,
			     (portsc & XHCI_PORTSC_PED) != 0U ? 1U : 0U);

		if (portsc & XHCI_PORTSC_PRC) {
			UHC_DWC3_DBG("bus_reset: clearing PRC");
			uint32_t tmp = xhci_readl(priv->op_base, port_off);
			uint32_t w1c = tmp & XHCI_PORTSC_W1C_MASK;

			if (w1c != 0U) {
				xhci_writel(priv->op_base, port_off,
					    xhci_port_state_to_neutral(tmp) | w1c);
			}
		}

		if ((xhci_readl(priv->op_base, port_off) & XHCI_PORTSC_PR) != 0U) {
			UHC_DWC3_DBG("bus_reset: wait PR clear after W1C (~200 ms)");
			timeout = 200;
			while (timeout > 0) {
				k_msleep(1);
				portsc = xhci_readl(priv->op_base, port_off);
				if (((unsigned int)timeout % 50U) == 0U) {
					UHC_DWC3_DBG("bus_reset: PORTSC=0x%08x left=%d PR=%u",
						     portsc, timeout,
						     (portsc & XHCI_PORTSC_PR) != 0U ? 1U : 0U);
				}
				if ((portsc & XHCI_PORTSC_PR) == 0U) {
					break;
				}
				timeout--;
			}
		}
		portsc = xhci_readl(priv->op_base, port_off);
		UHC_DWC3_DBG("bus_reset: wait PED (~500 ms) PORTSC=0x%08x", portsc);

		if (portsc & XHCI_PORTSC_PED) {
			UHC_DWC3_DBG("bus_reset: PED already set, skipping wait");
		} else {
			timeout = 500;
			while (timeout > 0) {
				k_msleep(1);
				portsc = xhci_readl(priv->op_base, port_off);
				if (((unsigned int)timeout % 100U) == 0U) {
					UHC_DWC3_DBG("bus_reset: PORTSC=0x%08x left=%d PED=%u",
						     portsc, timeout,
						     (portsc & XHCI_PORTSC_PED) != 0U ? 1U : 0U);
				}
				if (portsc & XHCI_PORTSC_PED) {
					break;
				}
				timeout--;
			}
			if (!(portsc & XHCI_PORTSC_PED)) {
				LOG_ERR("USB2 PED not set after reset (PORTSC=0x%08x)", portsc);
				return -EBUSY;
			}
		}
	} else {
		/* SuperSpeed: warm reset (WR) per xHCI 1.x port reset. */
		portsc = xhci_readl(priv->op_base, port_off);
		{
			uint32_t w1c = portsc & XHCI_PORTSC_W1C_MASK;

			if (w1c != 0U) {
				xhci_writel(priv->op_base, port_off,
					    xhci_port_state_to_neutral(portsc) | w1c);
				portsc = xhci_readl(priv->op_base, port_off);
			}
		}

		portsc = xhci_port_state_to_neutral(xhci_readl(priv->op_base, port_off)) |
			 XHCI_PORTSC_WR;
		xhci_writel(priv->op_base, port_off, portsc);
		UHC_DWC3_DBG("bus_reset: wait SS warm reset (WR) clear (~2s)");

		timeout = 2000;
		while (timeout > 0) {
			k_busy_wait(1000);
			portsc = xhci_readl(priv->op_base, port_off);
			if ((portsc & XHCI_PORTSC_WR) == 0U) {
				break;
			}
			timeout--;
		}

		if (timeout == 0) {
			LOG_ERR("bus_reset: timeout waiting SS WR clear");
			return -ETIMEDOUT;
		}

		portsc = xhci_readl(priv->op_base, port_off);
		if (portsc & XHCI_PORTSC_W1C_MASK) {
			xhci_writel(priv->op_base, port_off,
				    xhci_port_state_to_neutral(portsc) |
					    (portsc & XHCI_PORTSC_W1C_MASK));
		}

		timeout = 500;
		while (timeout > 0) {
			k_msleep(1);
			portsc = xhci_readl(priv->op_base, port_off);
			if (portsc & XHCI_PORTSC_PED) {
				break;
			}
			timeout--;
		}

		if (!(portsc & XHCI_PORTSC_PED)) {
			LOG_ERR("SS PED not set after warm reset (PORTSC=0x%08x)", portsc);
			return -EBUSY;
		}
	}

	portsc = xhci_readl(priv->op_base, port_off);
	priv->port_speed = XHCI_PORTSC_SPEED(portsc);

	/*
	 * Slot context speed encoding differs from PORTSC speed field;
	 * match xusb_host_example SlotCtxSpeed().
	 */
	uint8_t slot_speed;

	UHC_DWC3_DBG("bus_reset: slot_speed %u", priv->port_speed);
	switch (priv->port_speed) {
	case XHCI_SPEED_FULL:
		slot_speed = 1U;
		break;
	case XHCI_SPEED_LOW:
		slot_speed = 2U;
		break;
	case XHCI_SPEED_HIGH:
		slot_speed = 3U;
		break;
	case XHCI_SPEED_SUPER:
		slot_speed = 4U;
		break;
	default:
		slot_speed = 3U;
		break;
	}

	if (priv->slot_id == 0U) {
		UHC_DWC3_DBG("bus_reset: calling enable_slot");
		ret = xhci_enable_slot(priv);
		if (ret != 0) {
			return ret;
		}
		UHC_DWC3_DBG("bus_reset: enable_slot done");
	}

	/* Address Device with BSR=1 (xHCI_enable_device / xHCI 4.3.2).
	 * xhci_address_device_initial() blocks on command completion internally.
	 */
	UHC_DWC3_DBG("bus_reset: Address Device (BSR=1)");
	ret = xhci_address_device_initial(priv, reset_rp, slot_speed);
	if (ret != 0) {
		return ret;
	}

	UHC_DWC3_DBG("bus_reset() done — usbh will run GET_DESCRIPTOR next");

	return 0;
}

static int uhc_dwc3_sof_enable(const struct device *dev)
{
	/* SOF is automatically generated when xHCI runs */
	return 0;
}

static int uhc_dwc3_bus_suspend(const struct device *dev)
{
	/* TODO: implement port suspend via PORTSC PLS */
	return -ENOTSUP;
}

static int uhc_dwc3_bus_resume(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* TODO: PORTSC PLS resume when suspend is implemented. No-op is correct
	 * when the root port was not suspended (typical after bus reset + SOF).
	 */
	return 0;
}

static int uhc_dwc3_assign_address(const struct device *dev, struct usb_device *udev,
				   uint8_t *addr_out)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	uint8_t hc_addr;
	int ret;

	ARG_UNUSED(udev);

	if (addr_out == NULL) {
		return -EINVAL;
	}

	xhci_ep0_ring_sync_from_hw(priv);
	ret = xhci_address_device_set_address_bsr0(priv);
	if (ret != 0) {
		return ret;
	}

	dwc3_dma_invalidate(priv->dev_ctx, 2048);
	hc_addr = (uint8_t)(((const struct xhci_slot_ctx *)priv->dev_ctx)->dev_state & 0xffU);
	if (hc_addr == 0U) {
		LOG_ERR("Address Device (BSR=0): HC returned USB address 0");
		return -EIO;
	}

	*addr_out = hc_addr;
	return 0;
}

static int uhc_dwc3_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);

	UHC_DWC3_DBG("ep_enqueue ep=0x%02x", xfer->ep);

	uhc_xfer_append(dev, xfer);

	/* Host stack uses EP0 as 0x00 (OUT) or 0x80 (IN); index is always 0. */
	if (USB_EP_GET_IDX(xfer->ep) == 0U) {
		struct xhci_ring *ring = &priv->ep0_ring;
		struct usb_setup_packet *setup = (struct usb_setup_packet *)xfer->setup_pkt;
		uint16_t wLength = sys_le16_to_cpu(setup->wLength);
		bool has_data = (wLength > 0);
		bool dir_in = (setup->bmRequestType & 0x80) != 0;

		xhci_ep0_ring_sync_from_hw(priv);

		if (!xhci_ep0_ring_verify_dequeue_matches_sw(priv)) {
			LOG_WRN("EP0 HW/SW dequeue mismatch before TD "
				"(bmReqType=0x%02x bReq=0x%02x) — abort enqueue",
				setup->bmRequestType, setup->bRequest);
			xfer->err = -EIO;
			uhc_xfer_return(dev, xfer, xfer->err);
			return 0;
		}

		/* Control transfer: Setup → [Data] → Status */
		/* IN data stage: discard stale cache lines before the device DMAs in. */
		if (has_data && xfer->buf && dir_in) {
			dwc3_dma_invalidate(xfer->buf->data, wLength);
		}

		xhci_ring_link_trb_begin_td(ring);

		/* Setup TRB */
		unsigned int pcs_setup;
		struct xhci_trb *trb_setup = xhci_ring_enqueue(ring, &pcs_setup);

		memcpy(&trb_setup->param_lo, xfer->setup_pkt, 8);
		trb_setup->status = XHCI_TRB_LEN(8) | XHCI_TRB_INTR_TARGET(0);
		{
			uint16_t mps = priv->ep0_max_packet;
			uint32_t td_sz;
			uint32_t trt;

			if (mps == 0U) {
				mps = 64U;
			}

			td_sz = ep0_td_size_packets_after_setup(wLength, mps);
			trt = XHCI_TRB_TRT_NO_DATA;
			if (has_data) {
				trt = dir_in ? XHCI_TRB_TRT_IN : XHCI_TRB_TRT_OUT;
			}

			trb_setup->status |= XHCI_TRB_TD_SIZE(td_sz);

			/* xHCI §4.11.2 / xHCI: CHAIN on every TRB except the last (IOC). */
			trb_setup->control = XHCI_TRB_TYPE(XHCI_TRB_SETUP) | XHCI_TRB_IDT |
					     XHCI_TRB_TRT(trt) | XHCI_TRB_CHAIN |
					     (pcs_setup ? XHCI_TRB_CYCLE : 0);

			UHC_DWC3_DBG("ep0 ctrl ep=0x%02x type=0x%02x req=0x%02x wVal=0x%04x "
				     "wLen=%u ep0_mps=%u/%u setup_TD_SIZE=%u enq=%u cyc=%u",
				     xfer->ep, setup->bmRequestType, setup->bRequest,
				     sys_le16_to_cpu(setup->wValue), wLength, priv->ep0_max_packet,
				     mps, td_sz, (unsigned int)ring->enqueue,
				     (unsigned int)pcs_setup);
		}

		xhci_ring_td_link_chain_continue(ring);

		struct xhci_trb *trb_data = NULL;

		if (has_data && xfer->buf) {
			unsigned int pcs_data;
			uint64_t buf_phys = xhci_dma_addr(xfer->buf->data);

			trb_data = xhci_ring_enqueue(ring, &pcs_data);
			trb_data->param_lo = (uint32_t)buf_phys;
			trb_data->param_hi = (uint32_t)(buf_phys >> 32);
			trb_data->status = XHCI_TRB_LEN(wLength) | XHCI_TRB_TD_SIZE(0) |
					   XHCI_TRB_INTR_TARGET(0);
			trb_data->control = XHCI_TRB_TYPE(XHCI_TRB_DATA) |
					    (dir_in ? XHCI_TRB_DIR_IN : 0) | XHCI_TRB_CHAIN |
					    (pcs_data ? XHCI_TRB_CYCLE : 0);

			xhci_ring_td_link_chain_continue(ring);
		}

		/* Status TRB (last in TD: no CHAIN; IOC) */
		unsigned int pcs_status;
		struct xhci_trb *trb_status = xhci_ring_enqueue(ring, &pcs_status);

		trb_status->param_lo = 0;
		trb_status->param_hi = 0;
		trb_status->status = XHCI_TRB_TD_SIZE(0) | XHCI_TRB_INTR_TARGET(0);
		/* xHCI_queue_ctrl_tx: Status IN unless data stage was device-to-host. */
		uint32_t status_dir = has_data ? (dir_in ? 0U : XHCI_TRB_DIR_IN) : XHCI_TRB_DIR_IN;

		trb_status->control = XHCI_TRB_TYPE(XHCI_TRB_STATUS) | XHCI_TRB_IOC | status_dir |
				      (pcs_status ? XHCI_TRB_CYCLE : 0);

		xhci_ring_td_link_chain_continue(ring);

		if (has_data && xfer->buf && !dir_in) {
			dwc3_dma_flush(xfer->buf->data, wLength);
		}

		/* TRBs + link TRB must reach RAM before EP doorbell (cf. xhci_send_command). */
		xhci_flush_ep0_td(ring, trb_setup, trb_data, trb_status);

		UHC_DWC3_DBG("TD built trbs=%d status_DIR_IN=%u enq=%u cyc=%u",
			     (trb_data != NULL) ? 3 : 2,
			     (unsigned int)((status_dir & XHCI_TRB_DIR_IN) != 0U),
			     (unsigned int)ring->enqueue, (unsigned int)(ring->cycle_state & 1U));

		UHC_DWC3_DBG("EP submit: ep=0x%02x ep_id=%u ring=ep0", xfer->ep,
			     (unsigned int)XHCI_DCI_DEFAULT_CONTROL);
		/*
		 * xfer_sem is capped at 1. A completion that runs when no thread is
		 * waiting only increments count if count < limit; a second completion
		 * is then a no-op. Reset immediately before each EP0 doorbell so this TD
		 * always pairs with a fresh take (helps GET_DESCRIPTOR(CONFIGURATION)
		 * after SET_CONFIGURATION on integrated DWC3+xHCI).
		 */
		k_sem_reset(&priv->xfer_sem);

		priv->ep0_active_xfer = xfer;

		/* EP0: DCI 1 (not 0) */
		ring_ep_doorbell(priv, priv->slot_id, XHCI_DCI_DEFAULT_CONTROL);

		/* Wait for completion */
		if (dwc3_xfer_sem_take_ep0(dev, priv, K_MSEC(5000)) != 0) {
			struct xhci_ep_ctx *ep0_dbg;

			LOG_ERR("EP0 xfer timeout — no transfer event? "
				"(has_data=%d dir_in=%d; check DB ep_id vs ep0 ring)",
				has_data, dir_in);

			dwc3_dma_invalidate(priv->dev_ctx, 2048);
			ep0_dbg = xhci_slot_output_ep_ctx(priv, 1);
			LOG_ERR("EP0 timeout diag: ctx deq=0x%016llx ep_info=0x%08x "
				"STATE=%u | sw_enq=%u sw_cyc=%u link_CHAIN=%u | slot=%u "
				"doorbell_dci=%u",
				(unsigned long long)ep0_dbg->deq, ep0_dbg->ep_info,
				(unsigned int)xhci_ep_ctx_ep_state(ep0_dbg->ep_info),
				(unsigned int)ring->enqueue, (unsigned int)(ring->cycle_state & 1U),
				(unsigned int)((ring->trbs[ring->num_trbs - 1U].control &
						XHCI_TRB_CHAIN) != 0U),
				(unsigned int)priv->slot_id,
				(unsigned int)XHCI_DCI_DEFAULT_CONTROL);
			LOG_ERR("EP0 timeout: ep_info2=0x%08x tx_info=0x%08x", ep0_dbg->ep_info2,
				ep0_dbg->tx_info);
			(void)xhci_cancel_ep_xfer(priv, XHCI_DCI_DEFAULT_CONTROL, xfer, -ETIMEDOUT);
			if (priv->ep0_active_xfer == xfer) {
				xfer->err = -ETIMEDOUT;
			} else {
				xfer->err = priv->xfer_result;
			}
		} else {
			xfer->err = priv->xfer_result;
			if (xfer->buf && dir_in && priv->xfer_result == 0) {
				uint32_t lenfield = priv->xfer_length & 0xffffffU;
				uint32_t got = xhci_in_bytes_from_event(wLength, lenfield,
									priv->xfer_comp_code);

				dwc3_dma_invalidate(xfer->buf->data, got);
				net_buf_add(xfer->buf, got);
			}
		}

		priv->ep0_active_xfer = NULL;

		if (xfer->err == 0) {
			UHC_DWC3_DBG("EP0 xfer OK COMP=%u lenfield=%u", priv->xfer_comp_code,
				     (unsigned int)(priv->xfer_length & 0xffffffU));
		}

		/* FS: xhci_check_maxpacket if bMaxPacketSize0 != initial guess (64) */
		if (xfer->err == 0 && has_data && dir_in && xfer->buf != NULL &&
		    setup->bRequest == USB_SREQ_GET_DESCRIPTOR &&
		    USB_REQTYPE_GET_TYPE(setup->bmRequestType) == USB_REQTYPE_TYPE_STANDARD &&
		    wLength >= 8U && wLength <= 64U &&
		    ((setup->wValue >> 8) & 0xffU) == USB_DESC_DEVICE &&
		    (setup->wValue & 0xffU) == 0U && xfer->buf->len >= 8U) {
			uint8_t mps0 = xfer->buf->data[7];

			if (priv->port_speed == XHCI_SPEED_FULL && mps0 != 64U && mps0 != 0U &&
			    (mps0 == 8U || mps0 == 16U || mps0 == 32U)) {
				(void)xhci_evaluate_ep0_mps(priv, mps0);
			}
			xhci_ep0_verify_mps_matches(priv, mps0, "GET_DESCRIPTOR(8) vs EP0 ctx");
		}

		/*
		 * HS (and FS after 8-byte read): re-commit EP0 via Evaluate Context after
		 * full device descriptor. FS path may have already updated MPS from byte 7;
		 * issuing Evaluate again with current ep0_max_packet refreshes the xHC.
		 */
		if (xfer->err == 0 && has_data && dir_in && xfer->buf != NULL &&
		    setup->bRequest == USB_SREQ_GET_DESCRIPTOR &&
		    USB_REQTYPE_GET_TYPE(setup->bmRequestType) == USB_REQTYPE_TYPE_STANDARD &&
		    wLength >= 18U && wLength <= 64U &&
		    ((setup->wValue >> 8) & 0xffU) == USB_DESC_DEVICE &&
		    (setup->wValue & 0xffU) == 0U && xfer->buf->len >= 18U) {
			int er = xhci_evaluate_ep0_mps(priv, priv->ep0_max_packet);

			if (er != 0) {
				LOG_WRN("Evaluate Context after GET_DESCRIPTOR(18) failed: %d", er);
			}
		}

		uhc_xfer_return(dev, xfer, xfer->err);
	} else {
		/* Bulk / interrupt transfer (EP1+). EP0 must use control path. */
		const struct uhc_dwc3_config *cfg = dev->config;

		__ASSERT(USB_EP_GET_IDX(xfer->ep) != 0U,
			 "EP0 is control-only; host uses 0x00/0x80");

		bool dir_in = USB_EP_DIR_IS_IN(xfer->ep);
		uint32_t len = 0U;

		if (xfer->buf != NULL) {
			len = (uint32_t)xfer->buf->len;
			if (len == 0U) {
				len = (uint32_t)net_buf_frags_len(xfer->buf);
			}
			/*
			 * Bulk IN: host stack often submits with buf->len == 0 (no net_buf_add
			 * before doorbell). frags_len is 0 for a single empty fragment; use the
			 * buffer's usable capacity. Completion then net_buf_add(got) — do not
			 * pre-fill len to the max or completion would double-count.
			 */
			if (len == 0U && dir_in) {
				len = (uint32_t)net_buf_max_len(xfer->buf);
			}
		}

		/* xHCI DCI: EP n OUT = 2n, EP n IN = 2n+1 (n >= 1) */
		uint8_t ep_num = USB_EP_GET_IDX(xfer->ep);
		uint8_t dci = ep_num * 2U + (dir_in ? 1U : 0U);
		struct xhci_ring *ring = &priv->ep_bulk_rings[dci];

		if (ring->trbs == NULL) {
			LOG_ERR("DCI %u ring not configured (Configure Endpoint first)",
				(unsigned int)dci);
			xfer->err = -ENODEV;
			uhc_xfer_return(dev, xfer, xfer->err);
			return 0;
		}

		if (len == 0U) {
			LOG_ERR("bulk/interrupt xfer len=0 (ep=0x%02x)", xfer->ep);
			xfer->err = -EINVAL;
			uhc_xfer_return(dev, xfer, xfer->err);
			return 0;
		}

		if (priv->bulk_active_xfer[dci] != NULL) {
			LOG_ERR("bulk DCI%u: xfer already active (ep=0x%02x)", (unsigned int)dci,
				xfer->ep);
			xfer->err = -EBUSY;
			uhc_xfer_return(dev, xfer, xfer->err);
			return 0;
		}

		if (xhci_dwc3_bulk_queue_prologue(priv, dci) != 0) {
			LOG_ERR("bulk DCI%u: queue prologue failed (ep=0x%02x)", (unsigned int)dci,
				xfer->ep);
			xfer->err = -EIO;
			uhc_xfer_return(dev, xfer, xfer->err);
			return 0;
		}

		{
			uint32_t ep_st = uhc_dwc3_xhci_read_ep_state(priv, dci);

			UHC_DWC3_BULK_FLOW_INF("uhc_flow: bulk enqueue DCI%u ep=0x%02x EP_STATE=%u",
					       (unsigned int)dci, xfer->ep, ep_st);

			if (ep_st == XHCI_EP_CTX_EP_STATE_STOPPED) {
				LOG_ERR("bulk DCI%u STOPPED before queue — recover",
					(unsigned int)dci);
				if (xhci_dwc3_bulk_recover_ep(priv, dci) != 0) {
					xfer->err = -EIO;
					uhc_xfer_return(dev, xfer, xfer->err);
					return 0;
				}
				ep_st = uhc_dwc3_xhci_read_ep_state(priv, dci);
			}

			if (ep_st != XHCI_EP_CTX_EP_STATE_RUNNING && xfer->udev != NULL &&
			    xfer->udev->cfg_desc != NULL) {
				const bool force_drop_add = xhci_plat_quirk(
					&cfg->xhci_plat, XHCI_QUIRK_BULK_FORCE_DROP_ADD);

				LOG_WRN("bulk DCI%u EP_STATE=%u — %s Configure "
					"Endpoint drop+add",
					(unsigned int)dci, ep_st,
					force_drop_add ? "force" : "skip");
				if (force_drop_add && xhci_bulk_eps_reconfigure_drop_add(
							      priv, xfer->udev, true) == 0) {
					(void)xhci_dwc3_bulk_queue_prologue(priv, dci);
					ep_st = uhc_dwc3_xhci_read_ep_state(priv, dci);
					UHC_DWC3_BULK_FLOW_INF("uhc_flow: bulk enqueue DCI%u "
							       "after force drop+add EP_STATE=%u",
							       (unsigned int)dci, ep_st);
				}
			}

			if (ep_st != XHCI_EP_CTX_EP_STATE_RUNNING) {
				LOG_ERR("bulk DCI%u EP_STATE=%u — reject queue", (unsigned int)dci,
					ep_st);
				xfer->err = -EIO;
				uhc_xfer_return(dev, xfer, xfer->err);
				return 0;
			}
		}

		uint32_t trb_len = len;
		uint32_t trb_dma_len = len;
		void *out_dma_ptr = xfer->buf->data;
		bool bulk_in_staging = false;
		uint8_t *bulk_in_bounce = NULL;

		if (!dir_in) {
			uint64_t ap = xhci_dma_addr(out_dma_ptr);

			if ((ap & 63ULL) != 0ULL) {
				if (trb_len > (uint32_t)CONFIG_UHC_DWC3_BULK_OUT_STAGING_BUFSZ) {
					LOG_ERR("bulk OUT staging: trb_len %u > STAGING_BUFSZ %u",
						(unsigned int)trb_len,
						(unsigned int)
							CONFIG_UHC_DWC3_BULK_OUT_STAGING_BUFSZ);
					xfer->err = -E2BIG;
					uhc_xfer_return(dev, xfer, xfer->err);
					return 0;
				}
				memcpy(priv->bulk_out_staging, xfer->buf->data, len);
				out_dma_ptr = priv->bulk_out_staging;
				UHC_DWC3_DBG(
					"bulk OUT 64-byte DMA staging (was phys 0x%016llx len=%u)",
					(unsigned long long)ap, (unsigned int)len);
			}
		}

		if (dir_in) {
			uint64_t ap = xhci_dma_addr(xfer->buf->data);
			const bool bulk_in_misaligned = (ap & 63ULL) != 0ULL;
			/*
			 * Some integrated wrappers require driver-owned bounce
			 * buffers for bulk IN (direct net_buf DMA is unsafe).
			 */
			const bool bulk_in_force_staging =
				xhci_plat_quirk(&cfg->xhci_plat, XHCI_QUIRK_BULK_IN_STAGING);

			if (bulk_in_force_staging || bulk_in_misaligned) {
				if (len > 64U) {
					if (len > UHC_DWC3_BULK_IN_DATABUF_SZ) {
						LOG_ERR("bulk IN staging: len %u > databuf %u",
							(unsigned int)len,
							(unsigned int)UHC_DWC3_BULK_IN_DATABUF_SZ);
						xfer->err = -E2BIG;
						uhc_xfer_return(dev, xfer, xfer->err);
						return 0;
					}
					bulk_in_bounce = priv->bulk_in_databuf;
				} else {
					bulk_in_bounce = priv->bulk_in_smallbuf;
				}
				out_dma_ptr = bulk_in_bounce;
				bulk_in_staging = true;
				UHC_DWC3_DBG("bulk IN DMA staging %s "
					     "(user phys 0x%016llx len=%u)",
					     bulk_in_force_staging ? "platform" : "misaligned",
					     (unsigned long long)ap, (unsigned int)len);
			}
		}

		uint16_t ep_mps_sz = USB_MPS_EP_SIZE(xfer->mps);

		if (ep_mps_sz == 0U) {
			ep_mps_sz = 512U;
		}

		uint32_t trbs_needed =
			xhci_bulk_count_trbs_for_td(out_dma_ptr, trb_len, ep_mps_sz, dir_in);

		if (xhci_ring_prepare(ring, trbs_needed) != 0) {
			LOG_ERR("bulk TD needs %u TRBs (64 KiB boundary); ring=%u",
				(unsigned int)trbs_needed, (unsigned int)ring->num_trbs);
			xfer->err = -E2BIG;
			uhc_xfer_return(dev, xfer, xfer->err);
			return 0;
		}

		xhci_ring_link_trb_begin_td(ring);

		priv->bulk_urb[dci].td.start_trb = &ring->trbs[ring->enqueue];
		priv->bulk_urb[dci].td.node.next = NULL;
		sys_slist_append(&ring->td_list, &priv->bulk_urb[dci].td.node);

		struct xhci_trb *last_trb = NULL;
		unsigned int bulk_td_start_cycle = ring->cycle_state & 1U;
		struct xhci_trb *bulk_td_start_trb = &ring->trbs[ring->enqueue];
		bool bulk_td_first_trb = true;

		if (ring->enqueue != ring->dequeue) {
			LOG_WRN("bulk DCI%u enqueue/dequeue mismatch before TD "
				"(enq=%u deq=%u) — prologue bug",
				(unsigned int)dci, (unsigned int)ring->enqueue,
				(unsigned int)ring->dequeue);
		}

		{
			uint32_t run = 0U;
			uint32_t remaining = trb_len;
			unsigned int bulk_trb_seq = 0U;

			while (remaining > 0U) {
				uint32_t chunk = xhci_bulk_next_chunk_len(
					out_dma_ptr, run, remaining, ep_mps_sz, dir_in);
				uint64_t chunk_phys = xhci_dma_addr((uint8_t *)out_dma_ptr + run);
				bool more = (chunk < remaining);
				unsigned int pcs;
				struct xhci_trb *t = xhci_ring_queue_trb(ring, &pcs);

				xhci_ring_td_link_chain_continue(ring);

				last_trb = t;

				uint32_t td_rem = xhci_bulk_td_size_remainder(run, chunk, trb_len,
									      ep_mps_sz, more);

				t->param_lo = (uint32_t)chunk_phys;
				t->param_hi = (uint32_t)(chunk_phys >> 32);
				t->status = XHCI_TRB_LEN(chunk) | XHCI_TRB_TD_SIZE(td_rem) |
					    XHCI_TRB_INTR_TARGET(0);

				uint32_t ctl = XHCI_TRB_TYPE(XHCI_TRB_NORMAL);

				if (more) {
					ctl |= XHCI_TRB_CHAIN;
				} else {
					ctl |= XHCI_TRB_IOC;
				}
				if (dir_in) {
					ctl |= XHCI_TRB_ISP;
				}

				t->control = ctl;
				t->control &= ~XHCI_TRB_CYCLE;

				run += chunk;
				remaining -= chunk;

				if (xfer->buf && !dir_in && chunk > 0U) {
					dwc3_dma_flush_aligned(
						(uint8_t *)out_dma_ptr + (run - chunk), chunk);
				}

				dwc3_dma_flush_aligned(t, sizeof(*t));
				dwc3_dma_flush_aligned(&ring->trbs[ring->num_trbs - 1U],
						       sizeof(struct xhci_trb));
				if (!bulk_td_first_trb) {
					t->control |= (pcs != 0U) ? XHCI_TRB_CYCLE : 0U;
					xhci_flush_bulk_td(ring, t);
				}
				bulk_td_first_trb = false;

				{
					char ctx[48];
					unsigned int idx = bulk_trb_seq++;

					snprintk(ctx, sizeof(ctx), "bulk %s #%u chunk=%u rem_td=%u",
						 dir_in ? "IN" : "OUT", idx, (unsigned int)chunk,
						 (unsigned int)remaining);
					xhci_dbg_log_normal_trb(t, ctx);
					if (trbs_needed > 1U && dir_in && (idx == 0U || !more)) {
						UHC_DWC3_DBG("bulk IN multi-TRB TRB%u: len=%u "
							     "TD_SIZE=%u CH=%u IOC=%u ISP=%u",
							     idx, (unsigned int)chunk,
							     (unsigned int)td_rem,
							     (t->control & XHCI_TRB_CHAIN) ? 1U
											   : 0U,
							     (t->control & XHCI_TRB_IOC) ? 1U : 0U,
							     (t->control & XHCI_TRB_ISP) ? 1U : 0U);
					}
				}
			}
		}

		if (xfer->buf && dir_in && len > 0U) {
			/*
			 * Before the xHC DMA-writes into the buffer: flush+invalidate so no
			 * dirty/stale cache lines cause COMP_USB_TRANSACTION_ERROR or corrupt data.
			 * (Invalidate-only was insufficient on some AArch64 + device-memory paths.)
			 */
			if (bulk_in_staging && bulk_in_bounce != NULL) {
				dwc3_dma_prep_rx(bulk_in_bounce, trb_dma_len);
			} else {
				dwc3_dma_prep_rx(xfer->buf->data, trb_dma_len);
			}
		}

		if (trbs_needed > 1U) {
			UHC_DWC3_DBG("bulk TD split: %u TRBs (total=%u start_phys=0x%016llx)",
				     (unsigned int)trbs_needed, (unsigned int)trb_len,
				     (unsigned long long)xhci_dma_addr(out_dma_ptr));
			if (dir_in) {
				UHC_DWC3_DBG("bulk IN multi-TRB: TD_SIZE xhci_td_remainder; ISP=%s",
					     "all Normals");
			}
		}

		if (!dir_in) {
			dwc3_dma_flush_aligned(out_dma_ptr, trb_dma_len);
		}

		UHC_DWC3_DBG("EP submit: ep=0x%02x DCI=%u ring=per-ep", xfer->ep,
			     (unsigned int)dci);

#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
		if (!dir_in && last_trb != NULL) {
			uint32_t dw3 = last_trb->control;
			uint32_t trb_type = XHCI_TRB_FIELD_TO_TYPE(dw3);
			unsigned int ioc = (dw3 & XHCI_TRB_IOC) ? 1U : 0U;
			unsigned int isp = (dw3 & XHCI_TRB_ISP) ? 1U : 0U;
			unsigned int cyc = (dw3 & XHCI_TRB_CYCLE) ? 1U : 0U;
			unsigned int next_pcs = (unsigned int)(ring->cycle_state & 1U);

			UHC_DWC3_DBG("bulk OUT last Normal TRB DW3=0x%08x decode: "
				     "TRB_TYPE=%u (Normal=%u) IOC=%u ISP=%u CYCLE=%u | "
				     "next_slot_pcs=%u trbs_in_td=%u",
				     dw3, trb_type, (unsigned int)XHCI_TRB_NORMAL, ioc, isp, cyc,
				     next_pcs, (unsigned int)trbs_needed);
		}
#endif

		/* in-flight URB: one in-flight TD per DCI; completion in event path. */
		priv->bulk_urb[dci].td.trb_count =
			(trbs_needed > 255U) ? 255U : (uint8_t)trbs_needed;
		priv->bulk_urb[dci].td.start_trb = bulk_td_start_trb;
		priv->bulk_urb[dci].td.start_cycle = bulk_td_start_cycle;

		if (last_trb != NULL) {
			priv->bulk_expect_ioc_trb_phys[dci] =
				xhci_bulk_td_ioc_phys(ring, &priv->bulk_urb[dci].td);
			priv->bulk_td_trb_count[dci] = priv->bulk_urb[dci].td.trb_count;
		} else {
			priv->bulk_expect_ioc_trb_phys[dci] = 0ULL;
			priv->bulk_td_trb_count[dci] = 0U;
		}

		if (last_trb != NULL) {
			xhci_flush_bulk_td(ring, last_trb);
		}

		priv->bulk_active_xfer[dci] = xfer;
		priv->bulk_urb[dci].xfer = xfer;
		priv->bulk_urb[dci].req_len = len;
		priv->bulk_urb[dci].trb_dma_len = trb_dma_len;
		priv->bulk_urb[dci].dir_in = dir_in;
		priv->bulk_urb[dci].in_staging = bulk_in_staging;

		xhci_bulk_giveback_first_trb(priv, ring, bulk_td_start_trb, bulk_td_start_cycle,
					     priv->bulk_urb[dci].td.trb_count, priv->slot_id, dci);

		/* async UHC submit(): async — finish_td + giveback in event handler. */
		return 0;
	}

	return 0;
}

/*
 * Host stack: SET_CONFIGURATION, full configuration descriptor read, and
 * parse_configuration_descriptor() (interfaces + ep_in/ep_out) complete
 * before this runs. Issue xHCI Configure Endpoint and program per-DCI rings.
 */
static int uhc_dwc3_add_endpoints(const struct device *dev, struct usb_device *const udev)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	struct usb_cfg_descriptor *cfg = udev->cfg_desc;
	struct xhci_slot_ctx *slot;
	int err;

	if (cfg == NULL) {
		return -EINVAL;
	}

	dwc3_dma_invalidate(priv->dev_ctx, sizeof(priv->dev_ctx));
	slot = (struct xhci_slot_ctx *)priv->dev_ctx;

	if ((uint8_t)(slot->dev_state & 0xffU) != udev->addr) {
		LOG_WRN("device configured: slot USB addr %#x != udev->addr %u",
			(unsigned int)(slot->dev_state & 0xffU), udev->addr);
	}

	UHC_DWC3_DBG("device configured: addr=%u speed=%u cfg=%u", udev->addr,
		     (unsigned int)udev->speed, cfg->bConfigurationValue);

	for (unsigned int i = 0; i < cfg->bNumInterfaces && i < UHC_INTERFACES_MAX; i++) {
		struct usb_desc_header *dhp = udev->ifaces[i].dhp;

		if (dhp == NULL || dhp->bDescriptorType != USB_DESC_INTERFACE) {
			continue;
		}

		UHC_DWC3_DBG("  if %u alt %u class %u sub %u proto %u eps %u",
			     ((struct usb_if_descriptor *)dhp)->bInterfaceNumber,
			     ((struct usb_if_descriptor *)dhp)->bAlternateSetting,
			     ((struct usb_if_descriptor *)dhp)->bInterfaceClass,
			     ((struct usb_if_descriptor *)dhp)->bInterfaceSubClass,
			     ((struct usb_if_descriptor *)dhp)->bInterfaceProtocol,
			     ((struct usb_if_descriptor *)dhp)->bNumEndpoints);
	}

	for (unsigned int n = 1U; n < 16U; n++) {
		struct usb_ep_descriptor *epd;

		epd = udev->ep_out[n].desc;
		if (epd != NULL) {
			UHC_DWC3_DBG(
				"  ep OUT idx %u addr 0x%02x type %u mps %u mult %u interval %u", n,
				epd->bEndpointAddress,
				(unsigned int)(epd->bmAttributes & USB_EP_TRANSFER_TYPE_MASK),
				(unsigned int)USB_MPS_EP_SIZE(epd->wMaxPacketSize),
				(unsigned int)USB_MPS_ADDITIONAL_TRANSACTIONS(epd->wMaxPacketSize),
				(unsigned int)epd->bInterval);
		}

		epd = udev->ep_in[n].desc;
		if (epd != NULL) {
			UHC_DWC3_DBG(
				"  ep IN  idx %u addr 0x%02x type %u mps %u mult %u interval %u", n,
				epd->bEndpointAddress,
				(unsigned int)(epd->bmAttributes & USB_EP_TRANSFER_TYPE_MASK),
				(unsigned int)USB_MPS_EP_SIZE(epd->wMaxPacketSize),
				(unsigned int)USB_MPS_ADDITIONAL_TRANSACTIONS(epd->wMaxPacketSize),
				(unsigned int)epd->bInterval);
		}
	}

	err = xhci_dwc3_configure_non_ep0(priv, udev);
	if (err) {
		LOG_ERR("Configure Endpoint failed: %d", err);
		return err;
	}

	return 0;
}

static uint8_t xhci_xfer_ep_dci(const struct uhc_transfer *xfer)
{
	if (USB_EP_GET_IDX(xfer->ep) == 0U) {
		return XHCI_DCI_DEFAULT_CONTROL;
	}

	return xhci_usb_ep_addr_to_dci(xfer->ep);
}

/*
 * xHCI_urb_dequeue() subset: Stop Endpoint, drain events, wake waiter with
 * -ECONNRESET (or caller-supplied err on timeout path).
 */
int xhci_cancel_ep_xfer(struct uhc_dwc3_data *priv, uint8_t dci, struct uhc_transfer *xfer, int err)
{
	uint32_t ep_index = (dci == XHCI_DCI_DEFAULT_CONTROL) ? 0U : (uint32_t)dci - 1U;
	int stop_ret;

	if (priv->slot_id == 0U) {
		return -ENODEV;
	}

	if (dci == XHCI_DCI_DEFAULT_CONTROL) {
		if (priv->ep0_active_xfer != xfer) {
			return -ENOENT;
		}
	} else if (dci >= 32U || priv->bulk_active_xfer[dci] != xfer) {
		return -ENOENT;
	}

	stop_ret = xhci_cmd_stop_ep_ring(priv, ep_index);
	if (stop_ret != 0) {
		LOG_WRN("cancel DCI%u Stop EP failed %d — continue cleanup", (unsigned int)dci,
			stop_ret);
	}

	for (unsigned int d = 0U; d < 32U; d++) {
		(void)k_mutex_lock(&priv->evt_mutex, K_FOREVER);
		xhci_process_events_nolock(priv);
		(void)k_mutex_unlock(&priv->evt_mutex);
		k_busy_wait(100);
	}

	if (dci == XHCI_DCI_DEFAULT_CONTROL) {
		priv->xfer_result = err;
		priv->ep0_active_xfer = NULL;
		xhci_ep0_ring_sync_from_hw(priv);
		k_sem_give(&priv->xfer_sem);
	} else {
		priv->bulk_expect_ioc_trb_phys[dci] = 0ULL;
		priv->bulk_td_trb_count[dci] = 0U;
		xhci_dwc3_bulk_sync_ring_from_hw(priv, dci);
		(void)xhci_dwc3_bulk_recover_ep(priv, dci);
		xhci_bulk_giveback_urb(priv, dci, err, 0, 0);
	}

	return 0;
}

static int uhc_dwc3_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	uint8_t dci = xhci_xfer_ep_dci(xfer);

	ARG_UNUSED(dev);

	return xhci_cancel_ep_xfer(priv, dci, (struct uhc_transfer *)xfer, -ECONNRESET);
}

static int dwc3_eps_verify_steady_cb(const struct device *dev, struct usb_device *udev)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);

	if (udev == NULL || !uhc_is_enabled(dev)) {
		return -EINVAL;
	}

	dwc3_dma_invalidate(priv->dev_ctx, 2048);
	return xhci_dwc3_bulk_output_eps_steady(priv, udev);
}

static bool dwc3_post_configure_steady_cb(const struct device *dev)
{
	struct uhc_dwc3_data *priv = uhc_get_private(dev);

	if (!uhc_is_enabled(dev)) {
		return false;
	}

	return priv->steady_after_configure_ep;
}
/* -------------------------------------------------------------------------- */
/* Bulk layer exports (xhci_dwc3_bulk.c)                                      */
/* -------------------------------------------------------------------------- */

uint8_t uhc_dwc3_slot_id_get(struct uhc_dwc3_data *priv)
{
	return priv->slot_id;
}

uint8_t *uhc_dwc3_dev_ctx_get(struct uhc_dwc3_data *priv)
{
	return priv->dev_ctx;
}

const struct device *uhc_dwc3_device_get(struct uhc_dwc3_data *priv)
{
	return priv->dev;
}

mm_reg_t uhc_dwc3_dwc3_base_get(struct uhc_dwc3_data *priv)
{
	if (priv == NULL || priv->dev == NULL) {
		return 0U;
	}

	return uhc_dwc3_core_mmio(priv->dev);
}

mm_reg_t uhc_dwc3_op_base_get(struct uhc_dwc3_data *priv)
{
	return priv != NULL ? priv->op_base : 0U;
}

uint8_t uhc_dwc3_max_ports_get(struct uhc_dwc3_data *priv)
{
	return priv != NULL ? priv->max_ports : 0U;
}

struct xhci_ring *uhc_dwc3_ep_bulk_ring(struct uhc_dwc3_data *priv, uint8_t dci)
{
	return &priv->ep_bulk_rings[dci];
}

struct xhci_td *uhc_dwc3_bulk_urb_td(struct uhc_dwc3_data *priv, uint8_t dci)
{
	return &priv->bulk_urb[dci].td;
}

struct xhci_ep_ctx *uhc_dwc3_output_ep_ctx(struct uhc_dwc3_data *priv, unsigned int dci)
{
	return xhci_slot_output_ep_ctx(priv, dci);
}

uint32_t uhc_dwc3_xhci_read_ep_state(struct uhc_dwc3_data *priv, uint8_t dci)
{
	struct xhci_ep_ctx *ep;

	dwc3_dma_invalidate(priv->dev_ctx, 2048);
	ep = xhci_slot_output_ep_ctx(priv, dci);
	return xhci_ep_ctx_ep_state(ep->ep_info);
}

int uhc_dwc3_xhci_reset_ep(struct uhc_dwc3_data *priv, uint32_t ep_index)
{
	return xhci_send_command(priv, 0, 0, 0,
				 XHCI_TRB_TYPE(XHCI_TRB_RESET_EP) |
					 XHCI_TRB_SLOT_ID(priv->slot_id) |
					 XHCI_TRB_EP_INDEX_FOR_CMD(ep_index));
}

int uhc_dwc3_xhci_stop_ep_ring(struct uhc_dwc3_data *priv, uint32_t ep_index)
{
	return xhci_cmd_stop_ep_ring(priv, ep_index);
}

uint8_t uhc_dwc3_bulk_urb_xfer_ep(struct uhc_dwc3_data *priv, uint8_t dci)
{
	if (priv == NULL || dci >= 32U || priv->bulk_urb[dci].xfer == NULL) {
		return 0U;
	}

	return priv->bulk_urb[dci].xfer->ep;
}

int uhc_dwc3_xhci_set_tr_dequeue(struct uhc_dwc3_data *priv, uint32_t ep_index, uint64_t deq)
{
	return xhci_cmd_set_tr_dequeue_deq(priv, ep_index, deq);
}

static DEVICE_API(uhc, uhc_dwc3_api) = {
	.lock = uhc_dwc3_lock,
	.unlock = uhc_dwc3_unlock,
	.init = uhc_dwc3_init,
	.enable = uhc_dwc3_enable,
	.disable = uhc_dwc3_disable,
	.shutdown = uhc_dwc3_shutdown,
	.bus_reset = uhc_dwc3_bus_reset,
	.sof_enable = uhc_dwc3_sof_enable,
	.bus_suspend = uhc_dwc3_bus_suspend,
	.bus_resume = uhc_dwc3_bus_resume,
	.ep_enqueue = uhc_dwc3_ep_enqueue,
	.ep_dequeue = uhc_dwc3_ep_dequeue,
	.add_endpoints = uhc_dwc3_add_endpoints,
	.eps_verify_steady = dwc3_eps_verify_steady_cb,
	.ep_sync_after_clear_feature = dwc3_ep_sync_after_clear_feature_cb,
	.post_configure_steady = dwc3_post_configure_steady_cb,
	.assign_address = uhc_dwc3_assign_address,
	.free_dev = uhc_dwc3_free_dev,
};

/* -------------------------------------------------------------------------- */
/* Device instantiation                                                       */
/* -------------------------------------------------------------------------- */

static int uhc_dwc3_preinit(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	k_mutex_init(&data->mutex);

	return 0;
}

#define UHC_DWC3_USB2_WRAPPER_ROM_INIT(n)                                                          \
	COND_CODE_1(DT_INST_REG_HAS_NAME(n, usb2_wrapper), \
		    (DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(usb2_wrapper, DT_DRV_INST(n))), \
		    (.usb2_wrapper = {0}))

#define UHC_DWC3_DEVICE_DEFINE(n)                                                                  \
	static void uhc_dwc3_irq_enable_##n(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uhc_dwc3_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static void uhc_dwc3_irq_disable_##n(const struct device *dev)                             \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(n));                                                      \
	}                                                                                          \
                                                                                                   \
	static const struct uhc_dwc3_config uhc_dwc3_cfg_##n = {                                   \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(core, DT_DRV_INST(n)),                          \
		UHC_DWC3_USB2_WRAPPER_ROM_INIT(n),                                                 \
		.usb2_wrapper_present = DT_INST_REG_HAS_NAME(n, usb2_wrapper),                     \
		.boot_probe_ports_min = (uint8_t)DT_INST_PROP_OR(n, zephyr_boot_probe_ports, 0),   \
		.usb2_poll_port = (uint8_t)DT_INST_PROP_OR(n, zephyr_usb2_poll_port, 0),           \
		.xilinx = DWC3_XILINX_CONFIG_INIT(n),                                              \
		.xhci_plat = XHCI_PLAT_CONFIG_INIT(n),                                             \
		.irq_enable_func = uhc_dwc3_irq_enable_##n,                                        \
		.irq_disable_func = uhc_dwc3_irq_disable_##n,                                      \
	};                                                                                         \
                                                                                                   \
	static struct uhc_dwc3_data uhc_dwc3_priv_##n;                                             \
                                                                                                   \
	static struct uhc_data uhc_dwc3_data_##n = {                                               \
		.mutex = Z_MUTEX_INITIALIZER(uhc_dwc3_data_##n.mutex),                             \
		.priv = &uhc_dwc3_priv_##n,                                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uhc_dwc3_preinit, NULL, &uhc_dwc3_data_##n, &uhc_dwc3_cfg_##n,    \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uhc_dwc3_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_DWC3_DEVICE_DEFINE)
