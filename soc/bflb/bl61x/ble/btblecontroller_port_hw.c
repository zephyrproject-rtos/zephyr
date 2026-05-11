/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hardware port for the Bouffalo Lab BLE controller binary blob on BL61x.
 * Overrides the weak implementations in the precompiled library.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/syscon.h>
#include <stdint.h>
#include <string.h>

#include <bflb_soc.h>
#include <glb_reg.h>
#include <pds_reg.h>

/* BLE IRQ number: IRQ_NUM_BASE(16) + 56 = 72 */
#define BLE_IRQN 72

/* BT IRQ number: IRQ_NUM_BASE(16) + 46 = 62 */
#define BT_IRQN 62

/* DM IRQ number: IRQ_NUM_BASE(16) + 45 = 61 */
#define DM_IRQN 61

/* GLB AHB MCU software reset bit positions */
#define GLB_AHB_MCU_SW_BTDM 8
#define GLB_AHB_MCU_SW_PDS  46

/* Bits per software reset register bank (CFG0/CFG1/CFG2) */
#define SWRST_BITS_PER_REG 32U

/* eFuse register offsets for RC32M trim */
#define EFUSE_RC32M_TRIM_EN_OFFSET   0x78U
#define EFUSE_RC32M_TRIM_CODE_OFFSET 0x7CU
#define EFUSE_RC32M_TRIM_EN_BIT      1U
#define EFUSE_RC32M_TRIM_PARITY_BIT  0U
#define EFUSE_RC32M_TRIM_CODE_POS    4U
#define EFUSE_RC32M_TRIM_CODE_MSK    0xFFU

/* eFuse register offset for device info */
#define EFUSE_DEV_INFO_OFFSET 0x1CU
#define EFUSE_DEV_VERSION_POS 24U
#define EFUSE_DEV_VERSION_MSK 0x0FU

/*
 * btblecontroller_ble_irq_init — Register and enable the BLE interrupt
 */
void btblecontroller_ble_irq_init(void *handler)
{
	irq_connect_dynamic(BLE_IRQN, 0, (void (*)(const void *))handler, NULL, 0);
	irq_enable(BLE_IRQN);
}

void btblecontroller_bt_irq_init(void *handler)
{
	irq_connect_dynamic(BT_IRQN, 0, (void (*)(const void *))handler, NULL, 0);
	irq_enable(BT_IRQN);
}

void btblecontroller_dm_irq_init(void *handler)
{
	irq_connect_dynamic(DM_IRQN, 0, (void (*)(const void *))handler, NULL, 0);
	irq_enable(DM_IRQN);
}

void btblecontroller_ble_irq_enable(uint8_t enable)
{
	if (enable) {
		irq_enable(BLE_IRQN);
	} else {
		irq_disable(BLE_IRQN);
	}
}

void btblecontroller_bt_irq_enable(uint8_t enable)
{
	if (enable) {
		irq_enable(BT_IRQN);
	} else {
		irq_disable(BT_IRQN);
	}
}

void btblecontroller_dm_irq_enable(uint8_t enable)
{
	if (enable) {
		irq_enable(DM_IRQN);
	} else {
		irq_disable(DM_IRQN);
	}
}

/*
 * btblecontroller_enable_ble_clk — Enable/disable BLE peripheral clock
 *
 * BL61x: GLB_CGEN_CFG2 (offset 0x588) bit 10 (GLB_CGEN_S3_BT_BLE2)
 */
void btblecontroller_enable_ble_clk(uint8_t enable)
{
	unsigned int key = irq_lock();
	uint32_t tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	if (enable != 0U) {
		tmp |= GLB_CGEN_S3_BT_BLE2_MSK;
	} else {
		tmp &= GLB_CGEN_S3_BT_BLE2_UMSK;
	}
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG2_OFFSET);
	irq_unlock(key);
}

void btblecontroller_rf_restore(void)
{
	/* No PDS support initially — no RF restore needed */
}

/*
 * btblecontroller_efuse_read_mac — Read BLE MAC address from eFuse
 */
int btblecontroller_efuse_read_mac(uint8_t mac[6])
{
	uint8_t id[8] = {0};
	const size_t mac_len = 6U;

	if (hwinfo_get_device_id(id, mac_len) != (ssize_t)mac_len) {
		memset(mac, 0, mac_len);
		return -1;
	}
	memcpy(mac, id, mac_len);
	return 0;
}

/*
 * GLB_AHB_MCU_Software_Reset — Toggle a software reset bit
 *
 * The reset is in GLB_SWRST_CFG0/1/2 registers at offsets 0x540/0x544/0x548.
 * Bit is selected by swrst value: 0-31 → CFG0, 32-63 → CFG1, 64-95 → CFG2.
 * Sequence: clear bit → set bit → clear bit (pulse).
 */
static void glb_ahb_mcu_software_reset(uint8_t swrst)
{
	uint32_t reg_addr;
	uint32_t bit;
	uint32_t tmp;

	if (swrst < SWRST_BITS_PER_REG) {
		bit = swrst;
		reg_addr = GLB_BASE + GLB_SWRST_CFG0_OFFSET;
	} else if (swrst < (2U * SWRST_BITS_PER_REG)) {
		bit = swrst - SWRST_BITS_PER_REG;
		reg_addr = GLB_BASE + GLB_SWRST_CFG1_OFFSET;
	} else {
		bit = swrst - (2U * SWRST_BITS_PER_REG);
		reg_addr = GLB_BASE + GLB_SWRST_CFG2_OFFSET;
	}

	tmp = sys_read32(reg_addr);
	tmp &= ~(1U << bit);
	sys_write32(tmp, reg_addr);

	tmp = sys_read32(reg_addr);
	tmp |= (1U << bit);
	sys_write32(tmp, reg_addr);

	tmp = sys_read32(reg_addr);
	tmp &= ~(1U << bit);
	sys_write32(tmp, reg_addr);
}

void btblecontroller_software_btdm_reset(void)
{
	glb_ahb_mcu_software_reset(GLB_AHB_MCU_SW_BTDM);
}

void btblecontroller_software_pds_reset(void)
{
	glb_ahb_mcu_software_reset(GLB_AHB_MCU_SW_PDS);
}

/*
 * btblecontroller_pds_trim_rc32m — Trim RC32M oscillator via PDS registers
 *
 * Matches SDK's PDS_Trim_RC32M() sequence:
 * 1. Read trim code from efuse (en @ 0x78 bit1, value @ 0x7C bits[11:4])
 * 2. Validate parity
 * 3. Enable ext code in CTRL0, write code to CTRL2, select ext code
 */
void btblecontroller_pds_trim_rc32m(void)
{
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	uint32_t ef_en, ef_code;
	uint8_t en, parity, code;
	uint32_t tmp;

	if (syscon_read_reg(efuse, EFUSE_RC32M_TRIM_EN_OFFSET, &ef_en) < 0 ||
	    syscon_read_reg(efuse, EFUSE_RC32M_TRIM_CODE_OFFSET, &ef_code) < 0) {
		return;
	}

	en = (ef_en >> EFUSE_RC32M_TRIM_EN_BIT) & 1U;
	parity = (ef_en >> EFUSE_RC32M_TRIM_PARITY_BIT) & 1U;
	code = (ef_code >> EFUSE_RC32M_TRIM_CODE_POS) & EFUSE_RC32M_TRIM_CODE_MSK;

	if (en == 0U) {
		return;
	}

	if ((__builtin_popcount(code) & 1U) != parity) {
		return;
	}

	/* Enable ext code in CTRL0 */
	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL0_OFFSET);
	tmp |= PDS_RC32M_EXT_CODE_EN_MSK;
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL0_OFFSET);
	k_busy_wait(2);

	/* Write trim code to CTRL2 */
	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL2_OFFSET);
	tmp &= PDS_RC32M_CODE_FR_EXT2_UMSK;
	tmp |= ((uint32_t)code << PDS_RC32M_CODE_FR_EXT2_POS);
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL2_OFFSET);

	/* Select ext code */
	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL2_OFFSET);
	tmp |= PDS_RC32M_EXT_CODE_SEL_MSK;
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL2_OFFSET);
	k_busy_wait(1);
}

uint8_t btblecontrolller_get_chip_version(void)
{
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	uint32_t dev_info;

	if (syscon_read_reg(efuse, EFUSE_DEV_INFO_OFFSET, &dev_info) < 0) {
		return 0;
	}

	return (dev_info >> EFUSE_DEV_VERSION_POS) & EFUSE_DEV_VERSION_MSK;
}

void btblecontroller_sys_reset(void)
{
	sys_reboot(0);
}

void btblecontroller_puts(const char *str)
{
	ARG_UNUSED(str);
}

int btblecontroller_printf(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
	return 0;
}

int mfg_media_read_macaddr_with_lock(uint8_t mac[6], uint8_t reload)
{
	ARG_UNUSED(reload);
	return btblecontroller_efuse_read_mac(mac);
}
