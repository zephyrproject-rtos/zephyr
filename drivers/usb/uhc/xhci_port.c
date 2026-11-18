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
