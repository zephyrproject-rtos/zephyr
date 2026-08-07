/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
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
#ifdef CONFIG_REBOOT
	/*
	 * sys_reboot() issues NVIC_SystemReset (warm reset). On the RT685 the
	 * ROM must toggle the external FlexSPI flash reset pin during boot, or
	 * the flash is left in a non-default mode and re-boot fails. Enable
	 * this via the BOOT_CFG1 OTP shadow (physical fuses left unprogrammed).
	 *
	 * On the AUD-EVK the MX25U51245G QSPI flash RESET is wired to PIO2_12
	 * (confirmed on the board schematic).
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
