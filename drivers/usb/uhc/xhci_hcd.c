/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DWC3 xHCI host — controller init, reset, and boot port poll
 */

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
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/net_buf.h>

#include "uhc_common.h"
#include "dwc3_regs.h"
#include "dwc3_host.h"
#include "xhci_dwc3_priv.h"
#include "xhci_dwc3_internal.h"
#include "xhci_hw.h"
#include "xhci_ring.h"
#include "xhci_bulk.h"
#include "xhci_dwc3_bulk.h"
#include "xhci_dma.h"

LOG_MODULE_DECLARE(uhc_dwc3, CONFIG_UHC_DRIVER_LOG_LEVEL);

unsigned int dwc3_usb2_root_port(const struct uhc_dwc3_data *priv,
				 const struct uhc_dwc3_config *cfg)
{
	unsigned int p;

	if (cfg->usb2_poll_port != 0U) {
		p = (unsigned int)cfg->usb2_poll_port;
	} else if (priv->max_ports >= 2U) {
		p = 2U;
	} else {
		p = 1U;
	}
	if (p > priv->max_ports) {
		p = (unsigned int)priv->max_ports;
	}
	if (p < 1U) {
		p = 1U;
	}
	return p;
}

void dwc3_build_port_poll_order(unsigned int *order, unsigned int *n_order, unsigned int maxp,
				unsigned int usb2_first)
{
	unsigned int n = 0U;

	if (maxp < 1U || maxp > 32U) {
		*n_order = 0U;
		return;
	}

	if (usb2_first >= 1U && usb2_first <= maxp) {
		order[n++] = usb2_first;
	}
	for (unsigned int p = 1U; p <= maxp; p++) {
		if (p != usb2_first) {
			order[n++] = p;
		}
	}
	*n_order = n;
}
/* -------------------------------------------------------------------------- */
/* xHCI initialization                                                        */
/* -------------------------------------------------------------------------- */

/*
 * xHCI_reset() operational path — halt, HCRST, wait HCRST clear, wait CNR clear.
 */
static int xhci_op_reset_full(mm_reg_t op_base)
{
	uint32_t cmd, sts;
	int timeout;

	/* Halt: clear RUN if controller not already halted */
	sts = xhci_readl(op_base, XHCI_OP_USBSTS);
	if ((sts & XHCI_USBSTS_HCH) == 0U) {
		cmd = xhci_readl(op_base, XHCI_OP_USBCMD);
		cmd &= ~XHCI_USBCMD_RUN;
		xhci_writel(op_base, XHCI_OP_USBCMD, cmd);
	}

	timeout = 10000;
	while (timeout > 0) {
		sts = xhci_readl(op_base, XHCI_OP_USBSTS);
		if ((sts & XHCI_USBSTS_HCH) != 0U) {
			break;
		}
		k_busy_wait(10);
		timeout--;
	}
	if ((sts & XHCI_USBSTS_HCH) == 0U) {
		LOG_ERR("xHCI: halt (HCH) timeout");
		return -ETIMEDOUT;
	}

	cmd = xhci_readl(op_base, XHCI_OP_USBCMD);
	cmd |= XHCI_USBCMD_HCRST;
	xhci_writel(op_base, XHCI_OP_USBCMD, cmd);

	timeout = 10000;
	while (timeout > 0) {
		cmd = xhci_readl(op_base, XHCI_OP_USBCMD);
		if ((cmd & XHCI_USBCMD_HCRST) == 0U) {
			break;
		}
		k_busy_wait(10);
		timeout--;
	}
	if ((cmd & XHCI_USBCMD_HCRST) != 0U) {
		LOG_ERR("xHCI: HCRST bit clear timeout");
		return -ETIMEDOUT;
	}

	timeout = 2000;
	while ((xhci_readl(op_base, XHCI_OP_USBSTS) & XHCI_USBSTS_CNR) && timeout > 0) {
		k_busy_wait(100);
		timeout--;
	}
	if (timeout == 0) {
		LOG_ERR("xHCI: CNR did not clear after HCRST");
		return -ETIMEDOUT;
	}

	return 0;
}

int xhci_reset(struct uhc_dwc3_data *priv, mm_reg_t xhci_base)
{
	uint32_t cap, hcs1, hcc1;
	uint8_t caplength;
	int ret;

	/* Read capability registers */
	cap = xhci_readl(xhci_base, XHCI_CAP_CAPBASE);
	caplength = XHCI_CAPLENGTH(cap);
	LOG_DBG("xHCI version: 0x%04x, caplength: %u", XHCI_HCIVERSION(cap), caplength);

	priv->op_base = xhci_base + caplength;

	hcs1 = xhci_readl(xhci_base, XHCI_CAP_HCSPARAMS1);
	priv->max_slots = XHCI_HCS1_MAX_SLOTS(hcs1);
	priv->max_ports = XHCI_HCS1_MAX_PORTS(hcs1);

	if (priv->max_slots > XHCI_MAX_DEVSLOTS) {
		priv->max_slots = XHCI_MAX_DEVSLOTS;
	}

	{
		uint32_t hcs2 = xhci_readl(xhci_base, XHCI_CAP_HCSPARAMS2);

		priv->max_scratchpad = XHCI_HCS2_MAX_SCRATCHPAD(hcs2);
	}

	hcc1 = xhci_readl(xhci_base, XHCI_CAP_HCCPARAMS1);
	priv->ctx_bytes = XHCI_CTX_BYTES(hcc1);

	uint32_t dboff = xhci_readl(xhci_base, XHCI_CAP_DBOFF) & XHCI_DBOFF_MASK;

	priv->db_base = xhci_base + dboff;

	uint32_t rtsoff = xhci_readl(xhci_base, XHCI_CAP_RTSOFF) & XHCI_RTSOFF_MASK;

	priv->rt_base = xhci_base + rtsoff;

	LOG_DBG("xHCI: max_slots=%u max_ports=%u scratchpads=%u ctx_bytes=%u", priv->max_slots,
		priv->max_ports, priv->max_scratchpad, priv->ctx_bytes);

	/*
	 * Full operational reset (xhci_reset): after DWC3 BURST HCRST,
	 * repeat halt + HCRST + wait so USBCMD/CRCR/CNR match xHCI init.
	 */
	ret = xhci_op_reset_full(priv->op_base);
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("xHCI: controller ready (operational reset)");
	return 0;
}

int xhci_setup(struct uhc_dwc3_data *priv)
{
	/*
	 * CONFIG: xhci_lowlevel_init — merge max slots with existing CONFIG
	 * (preserve non-slot fields).
	 */
	{
		uint32_t cfg = xhci_readl(priv->op_base, XHCI_OP_CONFIG);

		cfg = (cfg & ~XHCI_CONFIG_SLOTS_MASK) | (priv->max_slots & XHCI_CONFIG_SLOTS_MASK);
		xhci_writel(priv->op_base, XHCI_OP_CONFIG, cfg);
	}

	/* Initialize DCBAA */
	memset(priv->dcbaa, 0, sizeof(priv->dcbaa));

	if (priv->max_scratchpad > 0U) {
		if (priv->max_scratchpad > XHCI_MAX_SCRATCHPADS) {
			LOG_ERR("xHCI: need %u scratchpads (driver max %u)", priv->max_scratchpad,
				XHCI_MAX_SCRATCHPADS);
			return -ENOTSUP;
		}

		for (uint32_t i = 0; i < priv->max_scratchpad; i++) {
			priv->scratchpad_table[i] = xhci_dma_addr(&priv->scratchpad_bufs[i][0]);
		}

		dwc3_dma_flush(priv->scratchpad_table, sizeof(uint64_t) * priv->max_scratchpad);
		dwc3_dma_flush(priv->scratchpad_bufs, 4096U * priv->max_scratchpad);

		priv->dcbaa[0] = xhci_dma_addr(priv->scratchpad_table);
	}

	/* xhci_mem_init: DCBAAP before CRCR */
	xhci_writeq(priv, priv->op_base, XHCI_OP_DCBAAP, xhci_dma_addr(priv->dcbaa));

	/* Command ring — CRCR RMW preserves low control bits (xhci-mem.c) */
	xhci_ring_init(&priv->cmd_ring, priv->cmd_trbs, XHCI_CMD_RING_SIZE, 0U);
	{
		uint64_t crcr = xhci_readq(priv->op_base, XHCI_OP_CRCR);
		uint64_t addr = xhci_dma_addr(priv->cmd_trbs);

		crcr = (crcr & XHCI_CRCR_PRESERVE_MASK) | (addr & ~XHCI_CRCR_PRESERVE_MASK) |
		       (uint64_t)priv->cmd_ring.cycle_state;
		xhci_writeq(priv, priv->op_base, XHCI_OP_CRCR, crcr);
	}

	/* Event ring — linear segment only (see xhci_ring_event_segment_init) */
	xhci_ring_event_segment_init(&priv->evt_ring, priv->evt_trbs, XHCI_EVT_RING_SIZE);

	/* Set up ERST (single segment) */
	priv->erst[0].seg_addr = xhci_dma_addr(priv->evt_trbs);
	priv->erst[0].seg_size = XHCI_EVT_RING_SIZE;
	priv->erst[0].rsvd = 0;

	mm_reg_t ir0 = priv->rt_base + XHCI_RT_IR0;
	uint64_t deq = xhci_dma_addr(priv->evt_trbs);

	/*
	 * Interrupter 0 — xhci_mem_init: ERDP (no EHB), ERSTSZ RMW,
	 * ERSTBA RMW. No IMOD/IMAN here; xhci_lowlevel_init clears those after RUN.
	 */
	xhci_writeq(priv, ir0, XHCI_IR_ERDP, deq & ~XHCI_ERST_PTR_MASK);

	{
		uint32_t ersz = xhci_readl(ir0, XHCI_IR_ERSTSZ);

		ersz &= XHCI_ERST_SIZE_MASK;
		ersz |= 1U; /* ERST_NUM_SEGS */
		xhci_writel(ir0, XHCI_IR_ERSTSZ, ersz);
	}

	{
		uint64_t erstba = xhci_readq(ir0, XHCI_IR_ERSTBA);

		erstba &= XHCI_ERST_PTR_MASK;
		erstba |= xhci_dma_addr(priv->erst) & ~XHCI_ERST_PTR_MASK;
		xhci_writeq(priv, ir0, XHCI_IR_ERSTBA, erstba);
	}

	/*  zero DNCTRL to avoid spurious device events */
	xhci_writel(priv->op_base, XHCI_OP_DNCTRL, 0U);

	/* Initialize EP0 ring */
	xhci_ring_init(&priv->ep0_ring, priv->ep0_trbs, XHCI_EP0_RING_SIZE, 0U);

	/* Per-DCI bulk/interrupt rings: xhci_ring_init() in Configure Endpoint. */

	dwc3_dma_flush(priv->dcbaa, sizeof(priv->dcbaa));
	dwc3_dma_flush(priv->cmd_trbs, sizeof(priv->cmd_trbs));
	dwc3_dma_flush(priv->evt_trbs, sizeof(priv->evt_trbs));
	dwc3_dma_flush(priv->erst, sizeof(priv->erst));
	dwc3_dma_flush(priv->ep0_trbs, sizeof(priv->ep0_trbs));

	LOG_DBG("xHCI: data structures initialized");
	return 0;
}

int xhci_start(struct uhc_dwc3_data *priv)
{
	uint32_t cmd;
	int timeout;

	/*
	 * RUN + USBCMD INTE (USBCMD INTE): host must be allowed to signal
	 * interrupts when the event ring is updated. omits INTE because polling builds
	 * polls; without INTE the IRQ line never asserts for port/status events.
	 */
	cmd = xhci_readl(priv->op_base, XHCI_OP_USBCMD);
	cmd |= XHCI_USBCMD_RUN | XHCI_USBCMD_INTE;
	xhci_writel(priv->op_base, XHCI_OP_USBCMD, cmd);

	timeout = 10000;
	while (timeout > 0) {
		if ((xhci_readl(priv->op_base, XHCI_OP_USBSTS) & XHCI_USBSTS_HCH) == 0U) {
			break;
		}
		k_busy_wait(10);
		timeout--;
	}
	if ((xhci_readl(priv->op_base, XHCI_OP_USBSTS) & XHCI_USBSTS_HCH) != 0U) {
		LOG_ERR("xHCI: controller still halted after RUN");
		return -EIO;
	}

	LOG_DBG("xHCI: controller running");
	return 0;
}
void xhci_post_start_interrupter0(struct uhc_dwc3_data *priv)
{
	mm_reg_t ir0 = priv->rt_base + XHCI_RT_IR0;

	xhci_writel(ir0, XHCI_IR_IMOD, 0);
	xhci_writel(ir0, XHCI_IR_IMAN, 0);
	xhci_writel(ir0, XHCI_IR_IMAN, XHCI_IMAN_IE | XHCI_IMAN_IP);
}
void xhci_poll_boot_connected_device(struct uhc_dwc3_data *priv, const struct device *dev)
{
	const struct uhc_dwc3_config *cfg = dev->config;
	uint32_t portsc;
	uint32_t port_off = XHCI_OP_PORTSC_BASE;
	unsigned int probe_ports;
	unsigned int poll_order[32];
	unsigned int poll_n;
	bool found;

	if (priv->max_ports < 1U) {
		return;
	}

	/*
	 * HCSPARAMS1 N_PORTS is authoritative: only ports 1..N_PORTS have real
	 * PORTSC at op_base + 0x400 + (n-1)*0x10. Reading beyond that is often RAZ
	 * (reads 0) — not "USB2 PP off". E.g. Versal VCK190: N_PORTS=1, the single
	 * PORTSC (abs. 0xFE200420 with caplen 0x20) carries USB2 traffic; there is
	 * no second register despite some DWC3 integrations exposing SS+USB2.
	 * zephyr,boot-probe-ports may request more; cap to N_PORTS (log if capped).
	 */
	probe_ports = priv->max_ports;

	if (cfg->boot_probe_ports_min > probe_ports) {
		probe_ports = cfg->boot_probe_ports_min;
	}
	if (probe_ports < 1U) {
		return;
	}
	if (probe_ports > 32U) {
		probe_ports = 32U;
	}
	if (probe_ports > priv->max_ports) {
		LOG_INF("xHCI: boot probe min %u > HC N_PORTS=%u — using %u only", probe_ports,
			priv->max_ports, priv->max_ports);
		probe_ports = priv->max_ports;
	}

	/*
	 * Visit USB2 root port first (port 2 when N_PORTS>=2, else 1), then others.
	 */
	dwc3_build_port_poll_order(poll_order, &poll_n, probe_ports,
				   dwc3_usb2_root_port(priv, cfg));

	found = false;

	for (unsigned int i = 0U; i < poll_n; i++) {
		unsigned int p = poll_order[i];

		port_off = XHCI_OP_PORTSC_BASE + (p - 1U) * XHCI_PORT_REGS_STRIDE;

		portsc = xhci_readl(priv->op_base, port_off);
		if (!(portsc & XHCI_PORTSC_PP)) {
			portsc = xhci_port_state_to_neutral(portsc) | XHCI_PORTSC_PP;

			xhci_writel(priv->op_base, port_off, portsc);
		}

		for (int wait = 0; wait < 200; wait++) {
			portsc = xhci_readl(priv->op_base, port_off);
			if ((portsc & XHCI_PORTSC_PP) && (portsc & XHCI_PORTSC_CCS)) {
				found = true;
				break;
			}
			k_busy_wait(1000);
		}

		if (found) {
			priv->root_port = (uint8_t)p;
			LOG_DBG("xHCI: boot poll CCS on root port %u", p);
			break;
		}
	}

	cfg->irq_disable_func(dev);
	xhci_process_events(priv);

	if (priv->root_connect_submitted) {
		cfg->irq_enable_func(dev);
		return;
	}

	if (!found) {
		uint32_t port1_sc = xhci_readl(priv->op_base, XHCI_OP_PORTSC_BASE);
		uint32_t port2_sc = 0U;

		if (probe_ports >= 2U) {
			port2_sc = xhci_readl(priv->op_base,
					      XHCI_OP_PORTSC_BASE + XHCI_PORT_REGS_STRIDE);
		}

		/*
		 * PORTSC[1]: typically SuperSpeed root port; PORTSC[2]: USB2 companion.
		 * PP+no CCS on [1] with FS/HS on the cable is normal — check [2] for CCS.
		 * Warn only if PP never stuck on port 1 (possible board power / VBUS).
		 */
		if ((port1_sc & XHCI_PORTSC_PP) != 0U) {
			if (probe_ports >= 2U) {
				LOG_INF("xHCI: boot poll: no device at boot (ports 1..%u, "
					"PORTSC[1]=0x%08x PORTSC[2]=0x%08x)",
					probe_ports, port1_sc, port2_sc);
			} else {
				LOG_INF("xHCI: boot poll: no device at boot (ports 1..%u, "
					"PORTSC[1]=0x%08x)",
					probe_ports, port1_sc);
			}
		} else {
			if (probe_ports >= 2U) {
				LOG_WRN("xHCI: boot poll: no CCS and port power off (ports 1..%u, "
					"PORTSC[1]=0x%08x PORTSC[2]=0x%08x)",
					probe_ports, port1_sc, port2_sc);
			} else {
				LOG_WRN("xHCI: boot poll: no CCS and port power off (ports 1..%u, "
					"PORTSC[1]=0x%08x)",
					probe_ports, port1_sc);
			}
		}
		cfg->irq_enable_func(dev);
		return;
	}

	portsc = xhci_readl(priv->op_base, port_off);
	if (portsc & XHCI_PORTSC_CSC) {
		xhci_writel(priv->op_base, port_off,
			    xhci_port_state_to_neutral(portsc) | XHCI_PORTSC_CSC);
		portsc = xhci_readl(priv->op_base, port_off);
	}

	priv->port_speed = XHCI_PORTSC_SPEED(portsc);
	LOG_INF("xHCI: boot-connected device (speed %u)", priv->port_speed);

	priv->root_connect_submitted = true;
	(void)uhc_submit_event(priv->dev, xhci_port_speed_to_connect_event(priv->port_speed), 0);

	cfg->irq_enable_func(dev);
}
