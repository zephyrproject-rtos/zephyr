/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/byteorder.h>
#include <synopsys/usb_dwc2_hw.h>
#include <synopsys/dwc2_core.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwc2_core, LOG_LEVEL_DBG);


int dwc2_core_soft_reset(const struct device *dev, void *const dev_base,
			 int (*is_phy_clk_off)(const struct device *dev))
{
	struct usb_dwc2_reg *const base = dev_base;
	mem_addr_t grstctl_reg = (mem_addr_t)&base->grstctl;
	mem_addr_t gsnpsid_reg = (mem_addr_t)&base->gsnpsid;
	k_timepoint_t timepoint;
	uint32_t gsnpsid;
	uint32_t grstctl;
	uint32_t gsnpsid_rev;

	/* Check AHB master idle state before starting reset */
	timepoint = sys_timepoint_calc(K_MSEC(100));
	while (!(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_AHBIDLE)) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for AHB idle timeout, GRSTCTL 0x%08x",
				sys_read32(grstctl_reg));
			return -ETIMEDOUT;
		}
	}

	/*
	 * Load GSNPSID before reset. On some cores it may not be readable
	 * after reset is asserted.
	 */
	gsnpsid = sys_read32(gsnpsid_reg);
	gsnpsid_rev = usb_dwc2_get_gsnpsid_rev(gsnpsid);

	/* Apply Core Soft Reset */
	sys_set_bits(grstctl_reg, USB_DWC2_GRSTCTL_CSFTRST);

	if (gsnpsid_rev < usb_dwc2_get_gsnpsid_rev(USB_DWC2_GSNPSID_REV_4_20A)) {
		/*
		 * Before v4.20a, CSFTRST is self-clearing.
		 * Wait until hardware clears it.
		 */
		timepoint = sys_timepoint_calc(K_MSEC(100));
		while (sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_CSFTRST) {
			k_busy_wait(1);

			if (sys_timepoint_expired(timepoint)) {
				LOG_ERR("Wait for CSR clear timeout, GRSTCTL 0x%08x",
					sys_read32(grstctl_reg));
				return -ETIMEDOUT;
			}

			if (is_phy_clk_off(dev)) {
				LOG_ERR("Core soft reset stuck, PHY clock is off");
				return -EIO;
			}
		}

		/* Hardware requires at least 3 PHY clocks after CSFTRST clears. */
		k_busy_wait(1);
	} else {
		/*
		 * From v4.20a, CSFTRST is write-only/reset-controlled.
		 * Wait for CSFTRSTDONE, then clear it.
		 */
		timepoint = sys_timepoint_calc(K_MSEC(100));
		while (!(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_CSFTRSTDONE)) {
			k_busy_wait(1);

			if (sys_timepoint_expired(timepoint)) {
				LOG_ERR("Wait for CSR done timeout, GRSTCTL 0x%08x",
					sys_read32(grstctl_reg));
				return -ETIMEDOUT;
			}

			if (is_phy_clk_off(dev)) {
				LOG_ERR("Core soft reset stuck, PHY clock is off");
				return -EIO;
			}
		}

		/* Clear CSFTRST, and write 1 to CSFTRSTDONE to clear the done indication. */
		grstctl = sys_read32(grstctl_reg);
		grstctl &= ~USB_DWC2_GRSTCTL_CSFTRST;
		grstctl |= USB_DWC2_GRSTCTL_CSFTRSTDONE;
		sys_write32(grstctl, grstctl_reg);
	}

	/* Wait for AHB master idle again after reset */
	timepoint = sys_timepoint_calc(K_MSEC(100));
	while (!(sys_read32(grstctl_reg) & USB_DWC2_GRSTCTL_AHBIDLE)) {
		k_busy_wait(1);

		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("Wait for AHB idle after reset timeout, GRSTCTL 0x%08x",
				sys_read32(grstctl_reg));
			return -ETIMEDOUT;
		}
	}

	LOG_DBG("DWC2 core reset done");

	return 0;
}
