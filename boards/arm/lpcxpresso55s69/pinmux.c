/*
 * Copyright (c) 2019,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <fsl_common.h>
#include <fsl_iocon.h>
#include <soc.h>

static int lpcxpresso_55s69_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm6), nxp_lpc_i2s, okay)) && \
		(DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm7), nxp_lpc_i2s, okay)) && \
		CONFIG_I2S
	/*
	 * Flexcomm 6 and 7 are connected to codec on board, and shared signal
	 * sets are used to enable one I2S device to handle RX and one to handle
	 * TX
	 */
	CLOCK_EnableClock(kCLOCK_Sysctl);
	/* Set shared signal set 0 SCK, WS from Transmit I2S - Flexcomm 7 */
	SYSCTL->SHAREDCTRLSET[0] = SYSCTL_SHAREDCTRLSET_SHAREDSCKSEL(7) |
				SYSCTL_SHAREDCTRLSET_SHAREDWSSEL(7);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Data in from Transmit I2S - Flexcomm 7 */
	SYSCTL->SHAREDCTRLSET[0] |= SYSCTL_SHAREDCTRLSET_SHAREDDATASEL(7);
	/* Enable Transmit I2S - Flexcomm 7 for Shared Data Out */
	SYSCTL->SHAREDCTRLSET[0] |= SYSCTL_SHAREDCTRLSET_FC7DATAOUTEN(1);
#endif

	/* Set Receive I2S - Flexcomm 6 SCK, WS from shared signal set 0 */
	SYSCTL->FCCTRLSEL[6] = SYSCTL_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL_FCCTRLSEL_WSINSEL(1);

	/* Set Transmit I2S - Flexcomm 7 SCK, WS from shared signal set 0 */
	SYSCTL->FCCTRLSEL[7] = SYSCTL_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL_FCCTRLSEL_WSINSEL(1);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Receive I2S - Flexcomm 6 Data in from shared signal set 0 */
	SYSCTL->FCCTRLSEL[6] |= SYSCTL_FCCTRLSEL_DATAINSEL(1);
	/* Select Transmit I2S - Flexcomm 7 Data out to shared signal set 0 */
	SYSCTL->FCCTRLSEL[7] |= SYSCTL_FCCTRLSEL_DATAOUTSEL(1);
#endif

#endif

	return 0;
}

SYS_INIT(lpcxpresso_55s69_pinmux_init,  PRE_KERNEL_1, 0);
