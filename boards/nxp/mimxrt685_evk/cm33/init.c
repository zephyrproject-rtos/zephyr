/*
 * Copyright (c) 2020,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <fsl_device_registers.h>

void board_early_init_hook(void)
{

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

#ifdef CONFIG_REBOOT
	/*
	 * The sys_reboot API calls NVIC_SystemReset. On the RT685, the warm
	 * reset will not complete correctly unless the ROM toggles the
	 * flash reset pin. We can control this behavior using the OTP shadow
	 * register for OPT word BOOT_CFG1
	 *
	 * Set FLEXSPI_RESET_PIN_ENABLE=1, FLEXSPI_RESET_PIN= PIO2_12
	 */
	 OCOTP->OTP_SHADOW[97] = 0x314000;
#endif /* CONFIG_REBOOT */
}
