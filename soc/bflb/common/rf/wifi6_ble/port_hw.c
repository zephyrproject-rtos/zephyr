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
#include <zephyr/drivers/otp.h>
#include <stdint.h>
#include <string.h>

#include <bflb_soc.h>
#include <glb_reg.h>
#include <pds_reg.h>

#define BTBLE_DT_NODE		DT_COMPAT_GET_ANY_STATUS_OKAY(bflb_bt_hci)
#define BTBLE_IRQ_DM_NUM	DT_IRQ_BY_NAME(BTBLE_DT_NODE, dm, irq)
#define BTBLE_IRQ_DM_PRI	DT_IRQ_BY_NAME(BTBLE_DT_NODE, dm, priority)
#define BTBLE_IRQ_BT_NUM	DT_IRQ_BY_NAME(BTBLE_DT_NODE, bt, irq)
#define BTBLE_IRQ_BT_PRI	DT_IRQ_BY_NAME(BTBLE_DT_NODE, bt, priority)
#define BTBLE_IRQ_BLE_NUM	DT_IRQ_BY_NAME(BTBLE_DT_NODE, ble, irq)
#define BTBLE_IRQ_BLE_PRI	DT_IRQ_BY_NAME(BTBLE_DT_NODE, ble, priority)

#if defined(CONFIG_SOC_SERIES_BL61X)

/* GLB AHB MCU software reset bit positions */
#define GLB_AHB_MCU_SW_BTDM 8
#define GLB_AHB_MCU_SW_PDS  46

/* Bits per software reset register bank (CFG0/CFG1/CFG2) */
#define SWRST_BITS_PER_REG 32U

/* eFuse register offset for device info */
#define EFUSE_DEV_INFO_OFFSET 0x1CU
#define EFUSE_DEV_VERSION_POS 24U
#define EFUSE_DEV_VERSION_MSK 0x0FU

#endif

typedef void (*btblecontroller_isr_handler)(void);

static btblecontroller_isr_handler btblecontroller_ble_isr_func;
static btblecontroller_isr_handler btblecontroller_bt_isr_func;
static btblecontroller_isr_handler btblecontroller_dm_isr_func;

static void btblecontroller_ble_isr(const void *arg)
{
	__ASSERT_NO_MSG(btblecontroller_ble_isr_func != NULL);
	ARG_UNUSED(arg);
	btblecontroller_ble_isr_func();
}

static void btblecontroller_bt_isr(const void *arg)
{
	__ASSERT_NO_MSG(btblecontroller_bt_isr_func != NULL);
	ARG_UNUSED(arg);
	btblecontroller_bt_isr_func();
}

static void btblecontroller_dm_isr(const void *arg)
{
	__ASSERT_NO_MSG(btblecontroller_dm_isr_func != NULL);
	ARG_UNUSED(arg);
	btblecontroller_dm_isr_func();
}

/*
 * btblecontroller_ble_irq_init — Register and enable the BLE interrupt
 */
void btblecontroller_ble_irq_init(void *handler)
{
	IRQ_CONNECT(BTBLE_IRQ_BLE_NUM, BTBLE_IRQ_BLE_PRI, btblecontroller_ble_isr, NULL, 0);
	btblecontroller_ble_isr_func = handler;
	irq_enable(BTBLE_IRQ_BLE_NUM);
}

void btblecontroller_bt_irq_init(void *handler)
{
	IRQ_CONNECT(BTBLE_IRQ_BT_NUM, BTBLE_IRQ_BT_PRI, btblecontroller_bt_isr, NULL, 0);
	btblecontroller_bt_isr_func = handler;
	irq_enable(BTBLE_IRQ_BT_NUM);
}

void btblecontroller_dm_irq_init(void *handler)
{
	IRQ_CONNECT(BTBLE_IRQ_DM_NUM, BTBLE_IRQ_DM_PRI, btblecontroller_dm_isr, NULL, 0);
	btblecontroller_dm_isr_func = handler;
	irq_enable(BTBLE_IRQ_DM_NUM);
}

void btblecontroller_ble_irq_enable(uint8_t enable)
{
	if (enable) {
		irq_enable(BTBLE_IRQ_BLE_NUM);
	} else {
		irq_disable(BTBLE_IRQ_BLE_NUM);
	}
}

void btblecontroller_bt_irq_enable(uint8_t enable)
{
	if (enable) {
		irq_enable(BTBLE_IRQ_BT_NUM);
	} else {
		irq_disable(BTBLE_IRQ_BT_NUM);
	}
}

void btblecontroller_dm_irq_enable(uint8_t enable)
{
	if (enable) {
		irq_enable(BTBLE_IRQ_DM_NUM);
	} else {
		irq_disable(BTBLE_IRQ_DM_NUM);
	}
}

void btblecontroller_sys_reset(void)
{
	sys_reboot(0);
}

uint64_t bflb_mtimer_get_time_us(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

uint64_t btblecontroller_mtimer_get_time_us(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
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

void btblecontroller_pds_trim_rc32m(void)
{
	/* Stub */
}

void btblecontroller_rf_restore(void)
{
	/* No PDS support initially — no RF restore needed */
}

#if defined(CONFIG_SOC_SERIES_BL61X)

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

uint8_t btblecontrolller_get_chip_version(void)
{
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	uint32_t dev_info;

	if (otp_read(efuse, EFUSE_DEV_INFO_OFFSET, &dev_info, sizeof(uint32_t)) < 0) {
		return 0;
	}

	return (dev_info >> EFUSE_DEV_VERSION_POS) & EFUSE_DEV_VERSION_MSK;
}

#endif
