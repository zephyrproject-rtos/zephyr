/*
 * Copyright 2020-2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/platform/hooks.h>
#include <zephyr/devicetree.h>
#include <fsl_device_registers.h>

void board_early_init_hook(void)
{
/* flexcomm1 and flexcomm3 are configured to loopback the TX signal to RX */
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2s, okay)) && \
	(DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_i2s, okay)) && \
	CONFIG_I2S

#if IS_ENABLED(CONFIG_I2S_TEST_SEPARATE_DEVICES)
	/*
	 * Internal loopback
	 *
	 * Flexcomm 3 (I2S Tx) provides SCK, WS and DATA (controller) into set #0.
	 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(3) |
		SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(3) |
		SYSCTL1_SHAREDCTRLSET_SHAREDDATASEL(3) |
		SYSCTL1_SHAREDCTRLSET_FC3DATAOUTEN(1);

	/* Flexcomm 1 (I2S Rx) consumes the set. */
	SYSCTL1->FCCTRLSEL[1] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1) |
				SYSCTL1_FCCTRLSEL_DATAINSEL(1);

	/* Flexcomm 3 (I2S Tx) consumes the provided signals internally. */
	SYSCTL1->FCCTRLSEL[3] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1) |
				SYSCTL1_FCCTRLSEL_DATAOUTSEL(1);
#else
	/*
	 * Duplex audio configuration
	 *
	 * Flexcomm 1 (I2S Rx) provides SCK and WS into set #0.
	 * These are sourced from outside pads, being routed to the WM8904.
	 *
	 * When configuring any of the Flexcomms for I2S traffic, configure them
	 * as I2S targets.
	 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(1) |
		SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(1);

	/* Flexcomm 1 (I2S Rx) consumes provided signals internally. */
	SYSCTL1->FCCTRLSEL[1] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
		SYSCTL1_FCCTRLSEL_WSINSEL(1);

	/* Flexcomm 3 (I2S Tx) consumes the set. */
	SYSCTL1->FCCTRLSEL[3] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
		SYSCTL1_FCCTRLSEL_WSINSEL(1);
#endif

#endif
}
