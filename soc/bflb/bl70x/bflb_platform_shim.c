/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Platform shims for the BL702 BLE controller binary blob.
 *
 * The blob directly calls bl_irq_*, bl_timer_*, bl_wireless_*,
 * BL702_Delay_*, GLB_*, UART_* functions. This file provides
 * Zephyr-compatible implementations.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(bt_bflb_shim, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);
#include <stdint.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/arch/riscv/irq.h>
#include <clic.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <ef_data_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

static void (*irq_handler_table[CONFIG_NUM_IRQS])(void);
static void *irq_ctx_table[CONFIG_NUM_IRQS];

/*
 * Generic IRQ dispatcher — bridges the blob's handler table to Zephyr's
 * dynamic IRQ system. When the blob registers a handler via bl_irq_register()
 * and enables it via bl_irq_enable(), Zephyr dispatches through this function.
 */
static void bl_irq_dispatcher(const void *arg)
{
	int irqnum = (int)(uintptr_t)arg;

	if (irqnum >= 0 && irqnum < CONFIG_NUM_IRQS && irq_handler_table[irqnum]) {
		irq_handler_table[irqnum]();
	}
}

/*
 * MSOFT handler — the BLE blob sets IRQ 3 pending to request a deferred
 * context switch (FreeRTOS portYIELD). Zephyr handles rescheduling
 * automatically, so we just clear the pending bit.
 */
static void msoft_irq_handler(const void *arg)
{
	ARG_UNUSED(arg);
	sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + RISCV_IRQ_MSOFT);
}

/*
 * bflb_ble_irq_setup — Register MSOFT handler before BLE controller init.
 *
 * The BLE blob's FreeRTOS port triggers MSOFT for context switching.
 * Since we replace the ROM FreeRTOS with Zephyr shims, the MSOFT handler
 * is never set up by the FreeRTOS port init code. Register it here.
 */
void bflb_ble_irq_setup(void)
{
	sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + RISCV_IRQ_MSOFT);
	irq_connect_dynamic(RISCV_IRQ_MSOFT, 1, msoft_irq_handler, NULL, 0);
	irq_enable(RISCV_IRQ_MSOFT);
}

void bl_irq_register_with_ctx(int irqnum, void *handler, void *ctx)
{
	if (irqnum < 0 || irqnum >= CONFIG_NUM_IRQS) {
		return;
	}
	irq_handler_table[irqnum] = handler;
	irq_ctx_table[irqnum] = ctx;
}

void bl_irq_register(int irqnum, void *handler)
{
	bl_irq_register_with_ctx(irqnum, handler, NULL);
}

void bl_irq_unregister(int irqnum, void *handler)
{
	ARG_UNUSED(handler);
	if (irqnum < 0 || irqnum >= CONFIG_NUM_IRQS) {
		return;
	}
	irq_handler_table[irqnum] = NULL;
	irq_ctx_table[irqnum] = NULL;
}

void bl_irq_handler_get(int irqnum, void **handler)
{
	if (irqnum < 0 || irqnum >= CONFIG_NUM_IRQS) {
		*handler = NULL;
		return;
	}
	*handler = (void *)irq_handler_table[irqnum];
}

void bl_irq_ctx_get(int irqnum, void **ctx)
{
	if (irqnum < 0 || irqnum >= CONFIG_NUM_IRQS) {
		*ctx = NULL;
		return;
	}
	*ctx = irq_ctx_table[irqnum];
}

/*
 * bl_irq_enable — Enable an interrupt through Zephyr's dynamic IRQ system.
 *
 * The blob calls bl_irq_register() then bl_irq_enable() to set up IRQ
 * handlers. We must register through Zephyr's irq_connect_dynamic() so
 * the handler appears in Zephyr's ISR table. Directly writing CLIC INTIE
 * would bypass Zephyr's dispatch and cause "spurious interrupt" crashes.
 */
void bl_irq_enable(unsigned int source)
{
	if (source < CONFIG_NUM_IRQS && source != RISCV_IRQ_MSOFT) {
		irq_connect_dynamic(source, 1, bl_irq_dispatcher, (void *)(uintptr_t)source, 0);
	}
	irq_enable(source);
}

void bl_irq_disable(unsigned int source)
{
	irq_disable(source);
}

void bl_irq_pending_set(unsigned int source)
{
	if (source < CONFIG_NUM_IRQS) {
		sys_write8(1, CLIC_HART0_ADDR + CLIC_INTIP + source);
	}
}

void bl_irq_pending_clear(unsigned int source)
{
	if (source < CONFIG_NUM_IRQS) {
		sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + source);
	}
}

int bl_irq_save(void)
{
	uint32_t oldstat;

	__asm volatile("csrrc %0, mstatus, %1" : "=r"(oldstat) : "r"(8));
	return oldstat;
}

void bl_irq_restore(int flags)
{
	__asm volatile("csrw mstatus, %0" : : "r"(flags));
}

/* Timer shims */

void bl_timer_init(void)
{
	/* mtimer is already initialized by Zephyr */
}

uint64_t bl_timer_now_us64(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

/* Delay functions — symbol names are required by the blob */

void BL702_Delay_US(uint32_t cnt)
{
	k_busy_wait(cnt);
}

void BL702_Delay_MS(uint32_t cnt)
{
	k_busy_wait(cnt * 1000);
}

/* Wireless MAC address shims */

static uint8_t wireless_mac_addr[8];
static int8_t wireless_default_tx_power;

int bl_wireless_mac_addr_set(uint8_t mac[8])
{
	memcpy(wireless_mac_addr, mac, 8);
	return 0;
}

int bl_wireless_mac_addr_get(uint8_t mac[8])
{
	memcpy(mac, wireless_mac_addr, 8);
	return 0;
}

void bl_wireless_default_tx_power_set(int8_t power)
{
	wireless_default_tx_power = power;
}

int8_t bl_wireless_default_tx_power_get(void)
{
	return wireless_default_tx_power;
}

/* Power offset storage */
static int8_t wireless_power_offset_zigbee[16];
static int8_t wireless_power_offset_ble[4];

int bl_wireless_power_offset_set(int8_t poweroffset_zigbee[16], int8_t poweroffset_ble[4])
{
	memcpy(wireless_power_offset_zigbee, poweroffset_zigbee, 16);
	memcpy(wireless_power_offset_ble, poweroffset_ble, 4);
	return 0;
}

int bl_wireless_power_offset_get(int8_t poweroffset_zigbee[16], int8_t poweroffset_ble[4])
{
	memcpy(poweroffset_zigbee, wireless_power_offset_zigbee, 16);
	memcpy(poweroffset_ble, wireless_power_offset_ble, 4);
	return 0;
}

/* Capcode calibration storage */
#define MAX_CAPCODE_TABLE_SIZE 30

static int8_t capcode_temp[MAX_CAPCODE_TABLE_SIZE];
static int8_t capcode_offset[MAX_CAPCODE_TABLE_SIZE];
static uint8_t capcode_size;

int bl_wireless_capcode_offset_table_set(int8_t temp[], int8_t offset[], uint8_t size)
{
	if (size > MAX_CAPCODE_TABLE_SIZE) {
		return -1;
	}
	memcpy(capcode_temp, temp, size);
	memcpy(capcode_offset, offset, size);
	capcode_size = size;
	return 0;
}

int bl_wireless_capcode_offset_table_get(int8_t temp[], int8_t offset[], uint8_t *size)
{
	memcpy(temp, capcode_temp, capcode_size);
	memcpy(offset, capcode_offset, capcode_size);
	*size = capcode_size;
	return 0;
}

int8_t bl_wireless_capcode_offset_get(int8_t temp)
{
	for (int i = 1; i < capcode_size; i++) {
		if (temp < capcode_temp[i]) {
			return capcode_offset[i - 1];
		}
	}

	if (capcode_size > 0) {
		return capcode_offset[capcode_size - 1];
	}
	return 0;
}

static uint8_t power_tcal_en;
static uint8_t capcode_tcal_en;

void bl_wireless_power_tcal_en_set(uint8_t en)
{
	power_tcal_en = en;
}

uint8_t bl_wireless_power_tcal_en_get(void)
{
	return power_tcal_en;
}

void bl_wireless_capcode_tcal_en_set(uint8_t en)
{
	capcode_tcal_en = en;
}

uint8_t bl_wireless_capcode_tcal_en_get(void)
{
	return capcode_tcal_en;
}

/* Flash config stub */

/*
 * The BLE blob calls bl_flash_get_flashCfg() to get a pointer to the
 * flash configuration structure. On BL702 this is used during sleep/wake
 * to reconfigure flash. We provide a minimal stub that returns a static
 * config structure. The blob only reads a few fields from it.
 */
static uint8_t flash_cfg_buf[256];

void *bl_flash_get_flashCfg(void)
{
	return flash_cfg_buf;
}

/* GLB register shims */

/*
 * GLB_Get_Root_CLK_Sel — Read root clock source selection
 * Returns: 0=RC32M, 1=XTAL, 2=PLL/DLL
 *
 * The vendor SDK collapses both PLL variants (RC32M-based and XTAL-based)
 * into a single GLB_ROOT_CLK_DLL (2) return value.
 */
uint32_t GLB_Get_Root_CLK_Sel(void)
{
	uint32_t clk = clock_bflb_get_root_clock();

	/* Map BFLB_MAIN_CLOCK_PLL_RC32M (2) and PLL_XTAL (3) to DLL (2) */
	return (clk >= 2) ? 2 : clk;
}

/*
 * GLB_Set_UART_CLK — Stub: do not let the blob reconfigure UART clocks
 * behind Zephyr's clock control driver.
 */
uint32_t GLB_Set_UART_CLK(uint8_t enable, uint8_t clk_sel, uint8_t div)
{
	ARG_UNUSED(enable);
	ARG_UNUSED(clk_sel);
	ARG_UNUSED(div);
	return 0;
}

/*
 * GLB_Set_BLE_CLK — Enable/disable BLE peripheral clock
 * Uses GLB_CLK_CFG1 BLE_EN bit and GLB_CGEN_CFG2 CGEN_S301 bit.
 */
uint32_t GLB_Set_BLE_CLK(uint8_t enable)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG1_OFFSET);
	if (enable) {
		tmp |= (1U << GLB_BLE_EN_POS);
	} else {
		tmp &= GLB_BLE_EN_UMSK;
	}
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG1_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);
	if (enable) {
		tmp |= (1U << GLB_CGEN_S3_POS);
	} else {
		tmp &= GLB_CGEN_S3_UMSK;
	}
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	return 0;
}

/* UART stubs */

/*
 * The BLE blob's uart.o references these UART HAL functions for HCI
 * transport over physical UART. With on-chip HCI (bt_onchiphci_*),
 * these are never called. Provide empty stubs to satisfy the linker.
 */

struct uart_cfg {
	uint8_t dummy[64];
};

uint32_t UART_Init(uint8_t uartId, struct uart_cfg *cfg)
{
	ARG_UNUSED(uartId);
	ARG_UNUSED(cfg);
	return 0;
}

uint32_t UART_Enable(uint8_t uartId, uint8_t txEn, uint8_t rxEn)
{
	ARG_UNUSED(uartId);
	ARG_UNUSED(txEn);
	ARG_UNUSED(rxEn);
	return 0;
}

uint32_t UART_Disable(uint8_t uartId, uint8_t txDis, uint8_t rxDis)
{
	ARG_UNUSED(uartId);
	ARG_UNUSED(txDis);
	ARG_UNUSED(rxDis);
	return 0;
}

uint32_t UART_TxFreeRun(uint8_t uartId, uint8_t txFreeRun)
{
	ARG_UNUSED(uartId);
	ARG_UNUSED(txFreeRun);
	return 0;
}

uint32_t UART_GetRxFifoCount(uint8_t uartId)
{
	ARG_UNUSED(uartId);
	return 0;
}

uint32_t UART_IntClear(uint8_t uartId, uint32_t intType)
{
	ARG_UNUSED(uartId);
	ARG_UNUSED(intType);
	return 0;
}

uint32_t UART_IntMask(uint8_t uartId, uint32_t intType, uint32_t intMask)
{
	ARG_UNUSED(uartId);
	ARG_UNUSED(intType);
	ARG_UNUSED(intMask);
	return 0;
}

/* Word-aligned memcpy */

uint32_t *arch_memcpy4(uint32_t *dst, const uint32_t *src, uint32_t n)
{
	const uint32_t *p = src;
	uint32_t *q = dst;

	while (n--) {
		*q++ = *p++;
	}

	return dst;
}

/* Random number shims */

/*
 * The BLE blob calls random() and srandom() for nonce generation.
 * We use sys_rand32_get() which sources from TRNG when available.
 */
long random(void)
{
	return (long)sys_rand32_get();
}

void srandom(unsigned int seed)
{
	ARG_UNUSED(seed);
	/* TRNG-based, no seeding needed */
}

/* Stack info for blob */

/*
 * The blob references stack_base_svc and stack_len_svc for stack
 * overflow checking. Point to the main thread's stack area.
 */
extern char __main_stack_start[];

uint8_t *stack_base_svc = (uint8_t *)__main_stack_start;
uint32_t stack_len_svc = CONFIG_MAIN_STACK_SIZE;

/* BLE assert handlers */

/*
 * The blob calls these on internal assertion failures.
 */
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

/*
 * _rom_patch_hook and heap_ram_size are defined in the BLE blob itself
 * (arch_main.o and rwip.o respectively). Do not redefine them here.
 */
