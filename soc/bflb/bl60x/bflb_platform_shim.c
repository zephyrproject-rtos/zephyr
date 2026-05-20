/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/riscv/irq.h>

LOG_MODULE_REGISTER(bt_bflb_shim, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

#include <clic.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

typedef void (*bflb_irq_cb_t)(int irq, void *arg);

static bflb_irq_cb_t irq_handler_table[CONFIG_NUM_IRQS];
static void *irq_ctx_table[CONFIG_NUM_IRQS];

static void bl_irq_dispatcher(const void *arg)
{
	int irqnum = (int)(uintptr_t)arg;

	if (irqnum >= 0 && irqnum < CONFIG_NUM_IRQS && irq_handler_table[irqnum] != NULL) {
		irq_handler_table[irqnum](irqnum, irq_ctx_table[irqnum]);
	}
}

static void msoft_irq_handler(const void *arg)
{
	ARG_UNUSED(arg);
	sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + RISCV_IRQ_MSOFT);
}

void bflb_ble_irq_setup(void)
{
	sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + RISCV_IRQ_MSOFT);
	irq_connect_dynamic(RISCV_IRQ_MSOFT, 1, msoft_irq_handler, NULL, 0);
	irq_enable(RISCV_IRQ_MSOFT);
}

int bflb_irq_attach(int irq, void *isr, void *arg)
{
	if (irq < 0 || irq >= CONFIG_NUM_IRQS) {
		return -1;
	}
	irq_handler_table[irq] = (bflb_irq_cb_t)isr;
	irq_ctx_table[irq] = arg;
	return 0;
}

void bflb_irq_enable(int irq)
{
	if (irq >= 0 && irq < CONFIG_NUM_IRQS && irq != RISCV_IRQ_MSOFT) {
		irq_connect_dynamic(irq, 1, bl_irq_dispatcher, (void *)(uintptr_t)irq, 0);
	}
	irq_enable(irq);
}

void bflb_irq_disable(int irq)
{
	irq_disable(irq);
}

void bflb_irq_clear_pending(int irq)
{
	if (irq >= 0 && irq < CONFIG_NUM_IRQS) {
		sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + irq);
	}
}

uint64_t bl_timer_now_us64(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

void BL602_Delay_US(uint32_t cnt)
{
	k_busy_wait(cnt);
}

void BL602_Delay_MS(uint32_t cnt)
{
	k_busy_wait(cnt * USEC_PER_MSEC);
}

int mfg_media_read_macaddr_with_lock(uint8_t mac[6], int reload)
{
	const size_t mac_len = 6U;

	ARG_UNUSED(reload);

	if (hwinfo_get_device_id(mac, mac_len) != (ssize_t)mac_len) {
		memset(mac, 0, mac_len);
		return -1;
	}

	sys_mem_swap(mac, mac_len);
	return 0;
}

/* Root clock source IDs expected by the BLE blob */
#define GLB_ROOT_CLK_RC32M 0U
#define GLB_ROOT_CLK_XTAL  1U
#define GLB_ROOT_CLK_PLL   2U

uint32_t GLB_Get_Root_CLK_Sel(void)
{
	uint32_t clk = clock_bflb_get_root_clock();

	return (clk >= GLB_ROOT_CLK_PLL) ? GLB_ROOT_CLK_PLL : clk;
}

uint32_t GLB_Set_UART_CLK(uint8_t enable, uint8_t clk_sel, uint8_t div)
{
	ARG_UNUSED(enable);
	ARG_UNUSED(clk_sel);
	ARG_UNUSED(div);
	return 0;
}

uint32_t GLB_Set_BLE_CLK(uint8_t enable)
{
	unsigned int key = irq_lock();
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG1_OFFSET);
	if (enable != 0U) {
		tmp |= (1U << GLB_BLE_EN_POS);
	} else {
		tmp &= GLB_BLE_EN_UMSK;
	}
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG1_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);
	if (enable != 0U) {
		tmp |= (1U << GLB_CGEN_S3_POS);
	} else {
		tmp &= GLB_CGEN_S3_UMSK;
	}
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	irq_unlock(key);
	return 0;
}

int32_t TrapNetCounter;

extern char __main_stack_start[];
uint8_t *stack_base_svc = (uint8_t *)__main_stack_start;
uint32_t stack_len_svc = CONFIG_MAIN_STACK_SIZE;

long random(void)
{
	return (long)sys_rand32_get();
}

void srandom(unsigned int seed)
{
	ARG_UNUSED(seed);
}

void dbg_test_print(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
}

void ble_assert_err(const char *file, int line, uint32_t param0, uint32_t param1)
{
	printk("BLE ASSERT ERR: %s:%d (0x%x, 0x%x)\n", file, line, param0, param1);
	k_panic();
}

void ble_assert_param(const char *file, int line, uint32_t param0, uint32_t param1)
{
	LOG_ERR("BLE ASSERT PARAM: %s:%d (0x%x, 0x%x)", file, line, param0, param1);
}

void ble_assert_warn(const char *file, int line, uint32_t param0, uint32_t param1)
{
	LOG_WRN("BLE ASSERT WARN: %s:%d (0x%x, 0x%x)", file, line, param0, param1);
}
