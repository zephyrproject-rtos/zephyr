/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Platform shims for the Bouffalo Lab BLE controller binary blob.
 *
 * The controller blob directly calls bl_irq_*, bl_wireless_*, and bl_rtc_*
 * functions (these are NOT weak overrides — they're direct symbol references).
 * This file provides Zephyr implementations of those functions.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <clic.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <pds_reg.h>
#include <hbn_reg.h>
#include <l1c_reg.h>

/*
 * mtime register address from device tree.
 * On BL702L with CLIC_THEAD_SEPARATED_LAYOUT, CLIC_HART0_ADDR points to the
 * CLIC interrupt controller (0x2800000), NOT the timer block. The mtime
 * register lives at a separate address (0x200BFF8).
 */
#define MTIME_BASE DT_REG_ADDR_BY_NAME(DT_NODELABEL(mtimer), mtime)
#define MTIME_LO   (MTIME_BASE)
#define MTIME_HI   (MTIME_BASE + 4)

#define MAX_CAPCODE_TABLE_SIZE 30

void (*handler_list[CONFIG_NUM_IRQS])(void);
void *irq_ctx_table[CONFIG_NUM_IRQS];

static uint8_t wireless_mac_addr[8];
static int8_t wireless_default_tx_power;

static int8_t wireless_power_offset_zigbee[16];
static int8_t wireless_power_offset_ble[4];

static int8_t capcode_temp[MAX_CAPCODE_TABLE_SIZE];
static int8_t capcode_offset[MAX_CAPCODE_TABLE_SIZE];
static uint8_t capcode_size;

static uint8_t power_tcal_en;
static uint8_t capcode_tcal_en;

/*
 * Bridge ISR: dispatches from Zephyr's ISR table to the SDK's handler_list[].
 * The arg encodes the IRQ number so we can look up the right handler.
 */
static void bl_irq_bridge_isr(const void *arg)
{
	int irqnum = (int)(uintptr_t)arg;

	if (irqnum >= 0 && irqnum < CONFIG_NUM_IRQS && handler_list[irqnum]) {
		handler_list[irqnum]();
	}
}

void bl_irq_register_with_ctx(int irqnum, void *handler, void *ctx)
{
	if (irqnum < 0 || irqnum >= CONFIG_NUM_IRQS) {
		return;
	}
	handler_list[irqnum] = handler;
	irq_ctx_table[irqnum] = ctx;

	/* Also register with Zephyr's ISR table so the CLIC dispatcher
	 * can route the interrupt to our bridge function.
	 */
	irq_connect_dynamic(irqnum, 0, bl_irq_bridge_isr, (void *)(uintptr_t)irqnum, 0);
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
	handler_list[irqnum] = NULL;
	irq_ctx_table[irqnum] = NULL;
}

void bl_irq_handler_get(int irqnum, void **handler)
{
	if (irqnum < 0 || irqnum >= CONFIG_NUM_IRQS) {
		*handler = NULL;
		return;
	}
	*handler = (void *)handler_list[irqnum];
}

void bl_irq_ctx_get(int irqnum, void **ctx)
{
	if (irqnum < 0 || irqnum >= CONFIG_NUM_IRQS) {
		*ctx = NULL;
		return;
	}
	*ctx = irq_ctx_table[irqnum];
}

void bl_irq_enable(unsigned int source)
{
	irq_enable(source);
}

void bl_irq_disable(unsigned int source)
{
	irq_disable(source);
}

void bl_irq_pending_set(unsigned int source)
{
	sys_write8(1, CLIC_HART0_ADDR + CLIC_INTIP + source);
}

void bl_irq_pending_clear(unsigned int source)
{
	sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + source);
}

int bl_irq_save(void)
{
	return (int)irq_lock();
}

void bl_irq_restore(int flags)
{
	irq_unlock((unsigned int)flags);
}

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
	uint8_t i;

	for (i = 1U; i < capcode_size; i++) {
		if (temp < capcode_temp[i]) {
			return capcode_offset[i - 1];
		}
	}

	if (capcode_size > 0) {
		return capcode_offset[capcode_size - 1];
	}
	return 0;
}

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

/*
 * The controller blob uses bl_rtc_* for timestamping.
 * Map to the RISC-V machine timer (mtime).
 */
uint64_t bl_rtc_get_counter(void)
{
	uint32_t hi, lo, hi2;

	do {
		hi = sys_read32(MTIME_HI);
		lo = sys_read32(MTIME_LO);
		hi2 = sys_read32(MTIME_HI);
	} while (hi != hi2);

	return ((uint64_t)hi << 32) | lo;
}

uint64_t bl_rtc_get_delta_counter(uint64_t ref)
{
	return bl_rtc_get_counter() - ref;
}

uint32_t bl_rtc_get_aligned_counter(void)
{
	return (uint32_t)bl_rtc_get_counter();
}

/* Busy-wait delay shims, called by the BLE controller blob. */
void BL702L_Delay_US(uint32_t cnt)
{
	k_busy_wait(cnt);
}

void BL702L_Delay_MS(uint32_t cnt)
{
	k_busy_wait(cnt * 1000U);
}

/*
 * Enable/disable the BLE clock gate and BLE module clock.
 * GLB_CLK_CFG1 bit 24 = BLE_EN (module clock)
 * GLB_CGEN_CFG2 bit 4 = CGEN_S301 (clock gate)
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
		tmp |= (1U << GLB_CGEN_S301_POS);
	} else {
		tmp &= GLB_CGEN_S301_UMSK;
	}
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	return 0; /* SUCCESS */
}

/*
 * These functions are used by the BLE controller for 32kHz crystal
 * calibration against the 32MHz clock. They read PDS_XTAL_CNT_32K
 * register fields.
 */

void rom_bl_rtc_trigger_xtal_cnt_32k(void)
{
	uint32_t tmp;

	/* If using internal RC 32K (f32k_sel == 1), skip */
	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	if (((tmp & HBN_F32K_SEL_MSK) >> HBN_F32K_SEL_POS) == 1) {
		return;
	}

	/* Clear done flag */
	tmp = sys_read32(GLB_BASE + GLB_XTAL_DEG_32K_OFFSET);
	tmp |= (1U << GLB_CLR_XTAL_CNT_32K_DONE_POS);
	sys_write32(tmp, GLB_BASE + GLB_XTAL_DEG_32K_OFFSET);

	/* Trigger measurement */
	tmp = sys_read32(GLB_BASE + GLB_XTAL_DEG_32K_OFFSET);
	tmp |= (1U << GLB_XTAL_CNT_32K_SW_TRIG_PS_POS);
	sys_write32(tmp, GLB_BASE + GLB_XTAL_DEG_32K_OFFSET);
}

uint16_t rom_bl_rtc_get_xtal_cnt_32k_counter(void)
{
	uint32_t tmp;

	tmp = sys_read32(PDS_BASE + PDS_XTAL_CNT_32K_OFFSET);

	/* If measurement in progress, wait for done */
	if ((tmp & PDS_XTAL_CNT_32K_PROCESS_MSK) != 0) {
		do {
			tmp = sys_read32(PDS_BASE + PDS_XTAL_CNT_32K_OFFSET);
		} while ((tmp & PDS_XTAL_CNT_32K_DONE_MSK) == 0);
	}

	return (uint16_t)((tmp & PDS_RO_XTAL_CNT_32K_CNT_MSK) >> PDS_RO_XTAL_CNT_32K_CNT_POS);
}

uint32_t rom_bl_rtc_32k_to_32m(uint32_t cycles)
{
	uint32_t tmp;
	uint16_t cnt;
	uint16_t res;

	tmp = sys_read32(PDS_BASE + PDS_XTAL_CNT_32K_OFFSET);
	cnt = (uint16_t)((tmp & PDS_RO_XTAL_CNT_32K_CNT_MSK) >> PDS_RO_XTAL_CNT_32K_CNT_POS);
	res = (uint16_t)((tmp & PDS_RO_XTAL_CNT_32K_RES_MSK) >> PDS_RO_XTAL_CNT_32K_RES_POS);

	return cycles * cnt + cycles * res / 64;
}

/*
 * rom_bl_rtc_* functions — referenced by ROM extension objects
 * (rom_hal_ext.o, rom_thread_port_ext.o, rom_zb_simple_ext.o).
 * These wrap the bl_rtc_* implementations above.
 */

uint64_t rom_bl_rtc_get_counter(void)
{
	return bl_rtc_get_counter();
}

uint64_t rom_bl_rtc_get_delta_counter(uint64_t ref_cnt)
{
	return bl_rtc_get_counter() - ref_cnt;
}

uint64_t rom_bl_rtc_get_aligned_counter(void)
{
	return bl_rtc_get_counter();
}

/*
 * RTC frequency: BL702L uses RC32K (32000 Hz) by default.
 * bl_rtc_frequency is a GP-relative variable set by ble_rwdata.ld.
 * The mtime runs at sys_clock_hw_cycles_per_sec(), but these ROM RTC
 * functions operate on the 32kHz RTC domain counter. Since we map
 * bl_rtc_get_counter() to mtime, we convert using the mtime frequency.
 */
uint32_t rom_bl_rtc_counter_to_ms(uint32_t cnt)
{
	uint32_t freq = sys_clock_hw_cycles_per_sec();

	return (uint32_t)((uint64_t)cnt * 1000 / freq);
}

uint32_t rom_bl_rtc_ms_to_counter(uint32_t ms)
{
	uint32_t freq = sys_clock_hw_cycles_per_sec();

	return (uint32_t)((uint64_t)ms * freq / 1000);
}

/*
 * Simple memory/delay functions referenced by ROM extension objects
 * (rom_lmac154_ext.o, rom_thread_pds_ext.o) and BLE controller blobs.
 */

void *arch_memcpy(void *dst, const void *src, uint32_t n)
{
	return memcpy(dst, src, n);
}

int arch_memcmp(const void *s1, const void *s2, uint32_t n)
{
	return memcmp(s1, s2, n);
}

uint32_t *arch_memcpy4(uint32_t *dst, const uint32_t *src, uint32_t n)
{
	const uint32_t *p = src;
	uint32_t *q = dst;

	while (n--) {
		*q++ = *p++;
	}

	return dst;
}

uint32_t *arch_memset4(uint32_t *dst, const uint32_t val, uint32_t n)
{
	uint32_t *q = dst;

	while (n--) {
		*q++ = val;
	}

	return dst;
}

void arch_delay_ms(uint32_t ms)
{
	k_sleep(K_MSEC(ms));
}
