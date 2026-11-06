/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

/*
 * prj.conf requests CONFIG_PCIE_EP_EMUL=y, but this application has no
 * zephyr,pcie-ep-emul devicetree node. The driver's
 * DT_HAS_ZEPHYR_PCIE_EP_EMUL_ENABLED dependency must keep the symbol
 * off so the driver is not compiled with zero instances. If the gate
 * is dropped, the symbol turns on and this assert fails the build.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_PCIE_EP_EMUL),
	     "PCIE_EP_EMUL must stay gated off without a zephyr,pcie-ep-emul node");

int main(void)
{
	return 0;
}
