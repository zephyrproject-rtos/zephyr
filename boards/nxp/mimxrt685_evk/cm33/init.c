/*
 * Copyright (c) 2020,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <fsl_device_registers.h>

/*
 * OTP word BOOT_CFG1 (OCOTP shadow index 97) - boot-ROM FlexSPI flash
 * reset-pin configuration. Field layout per the RT6xx boot ROM OTP map,
 * cross-checked against mimxrt685_evk (PIO2_12 = 0x314000) and
 * mimxrt595_evk (PIO4_5 = 0x164000):
 *   bit  14    : FLEXSPI_RESET_PIN_ENABLE
 *   bits 17:15 : GPIO port
 *   bits 22:18 : GPIO pin
 */
#define BOOT_CFG1_FLEXSPI_RESET_ENABLE     BIT(14)
#define BOOT_CFG1_FLEXSPI_RESET_PORT_MASK  GENMASK(17, 15)
#define BOOT_CFG1_FLEXSPI_RESET_PIN_MASK   GENMASK(22, 18)
#define BOOT_CFG1_FLEXSPI_RESET_PORT(p)    FIELD_PREP(BOOT_CFG1_FLEXSPI_RESET_PORT_MASK, (p))
#define BOOT_CFG1_FLEXSPI_RESET_PIN(n)     FIELD_PREP(BOOT_CFG1_FLEXSPI_RESET_PIN_MASK, (n))

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
#else
	/* Set shared signal set 0: SCK, WS from Flexcomm1 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(1) |
				SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(1);
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
	 * sys_reboot() issues NVIC_SystemReset (warm reset). On the RT685 the
	 * ROM must toggle the external FlexSPI flash reset pin during boot, or
	 * the flash is left in a non-default mode and re-boot fails. Enable
	 * this via the BOOT_CFG1 OTP shadow (physical fuses left unprogrammed).
	 *
	 * On the EVK the MX25UM51345G Octal SPI flash RESET is connected to
	 * PIO2_12.
	 *
	 * Read-modify-write so that any other BOOT_CFG1 bits already loaded
	 * from programmed fuses are preserved.
	 *
	 * NOTE: this relies on the BOOT_CFG1 shadow word NOT being write-locked.
	 * If the OCOTP shadow-write lock for this word is set (e.g. on a fused/
	 * secured part), the write is ignored by hardware and this workaround
	 * has no effect.
	 */
	uint32_t cfg = OCOTP->OTP_SHADOW[97];

	cfg &= ~(BOOT_CFG1_FLEXSPI_RESET_ENABLE |
		 BOOT_CFG1_FLEXSPI_RESET_PORT_MASK |
		 BOOT_CFG1_FLEXSPI_RESET_PIN_MASK);
	cfg |= BOOT_CFG1_FLEXSPI_RESET_ENABLE |
	       BOOT_CFG1_FLEXSPI_RESET_PORT(2) |   /* PIO2 */
	       BOOT_CFG1_FLEXSPI_RESET_PIN(12);    /* _12  */
	OCOTP->OTP_SHADOW[97] = cfg;
#endif /* CONFIG_REBOOT */
}
