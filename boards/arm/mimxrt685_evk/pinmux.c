/*
 * Copyright (c) 2020,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iopctl.h>
#include <soc.h>

static int mimxrt685_evk_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

/* flexcomm1 and flexcomm3 are configured to loopback the TX signal to RX */
#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2s, okay)) && \
	(DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_i2s, okay)) && \
	CONFIG_I2S

	/* Set shared signal set 0 SCK, WS from Transmit I2S - Flexcomm3 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(3) |
				SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(3);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Data in from Transmit I2S - Flexcomm 3 */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_SHAREDDATASEL(3);
	/* Enable Transmit I2S - Flexcomm 3 for Shared Data Out */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_FC3DATAOUTEN(1);
#endif

	/* Set Receive I2S - Flexcomm 1 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

	/* Set Transmit I2S - Flexcomm 3 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[3] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Receive I2S - Flexcomm 1 Data in from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] |= SYSCTL1_FCCTRLSEL_DATAINSEL(1);
	/* Select Transmit I2S - Flexcomm 3 Data out to shared signal set 0 */
	SYSCTL1->FCCTRLSEL[3] |= SYSCTL1_FCCTRLSEL_DATAOUTSEL(1);
#endif

#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(red_pwm_led), okay)
	uint32_t port0_pin31_config = (
			IOPCTL_PIO_FUNC3 |
			/* Disable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_DI |
			/* Enable pull-down function */
			IOPCTL_PIO_PULLDOWN_EN |
			/* Disable input buffer function */
			IOPCTL_PIO_INBUF_DI |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Normal drive */
			IOPCTL_PIO_FULLDRIVE_DI |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is disabled */
			IOPCTL_PIO_PSEDRAIN_DI |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT0 PIN31 (coords: B3) is configured as SCT0_OUT6 */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 31U, port0_pin31_config);
#endif
	return 0;
}

/* priority set to CONFIG_PINMUX_INIT_PRIORITY value */
SYS_INIT(mimxrt685_evk_pinmux_init, PRE_KERNEL_1, 45);
