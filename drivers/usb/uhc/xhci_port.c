/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * xHCI root-port physical reset (PORTSC PR/WR and completion polling).
 */

#include <errno.h>

#include "xhci_dwc3_internal.h"

LOG_MODULE_DECLARE(uhc_dwc3, CONFIG_UHC_DRIVER_LOG_LEVEL);

int xhci_root_port_reset_preamble(struct uhc_dwc3_data *priv, const struct uhc_dwc3_config *cfg,
				  uint8_t *reset_rp_out, uint32_t *port_off_out)
{
	uint8_t connect_rp = priv->root_port != 0U ? priv->root_port : 1U;
	uint32_t port_off =
		XHCI_OP_PORTSC_BASE + ((uint32_t)connect_rp - 1U) * XHCI_PORT_REGS_STRIDE;
	uint32_t portsc;
	uint8_t reset_rp;

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
	reset_rp = connect_rp;

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

	*reset_rp_out = reset_rp;
	*port_off_out = port_off;

	return 0;
}

int xhci_root_port_reset(struct uhc_dwc3_data *priv, uint32_t port_off)
{
	uint32_t portsc;
	int timeout;

	if (priv == NULL || priv->op_base == 0U) {
		return -EIO;
	}

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

	return 0;
}
