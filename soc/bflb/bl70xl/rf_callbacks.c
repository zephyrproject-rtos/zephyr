/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RF library callbacks required by libbl702l_rf.a.
 * These are called by the RF driver during calibration and reset events.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/arch.h>
#include <stdint.h>

/* Forward declarations from bl702l_rf.h */
extern void rf_set_bz_mode(uint8_t mode);

/* From bl702l_phy.h */
extern void bz_phy_set_tx_power_offset(int8_t poweroffset_zigbee[16], int8_t poweroffset_ble[4]);

/* From bl_wireless shims */
extern int bl_wireless_power_offset_get(int8_t poweroffset_zigbee[16], int8_t poweroffset_ble[4]);

/* Mode constants from bl702l_rf.h */
#define MODE_BLE_ONLY 0
#define MODE_ZB_ONLY  1
#define MODE_BZ_COEX  2

/*
 * rf_reset_done_callback — Called after RF reset completes.
 * Set operating mode and apply power offsets.
 */
void rf_reset_done_callback(void)
{
	int8_t po_zigbee[16];
	int8_t po_ble[4];

	/* TODO: select MODE_ZB_ONLY when 802.15.4 driver is added */
	rf_set_bz_mode(MODE_BLE_ONLY);
	bl_wireless_power_offset_get(po_zigbee, po_ble);
	bz_phy_set_tx_power_offset(po_zigbee, po_ble);
}

/*
 * rf_full_cal_start_callback — Called before RF full calibration.
 * The vendor implementation checks for DMA conflicts with the calibration
 * memory area. For now, we trust that no DMA is active during init.
 */
void rf_full_cal_start_callback(uint32_t addr, uint32_t size)
{
	/* No DMA conflict checking needed during early init */
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	/*
	 * RF calibration uses double-precision float (__muldf3) which accesses
	 * FP CSRs (frrm). With FPU_SHARING the FPU starts disabled per-thread
	 * and relies on an illegal-instruction trap to lazy-enable it. The
	 * BL70XL doesn't populate mtval with the faulting instruction encoding,
	 * breaking the trap handler's FP instruction decoder. Pre-enable the
	 * FPU here to avoid the trap entirely.
	 */
	__asm__ volatile("csrs mstatus, %0" ::"r"(MSTATUS_FS_INIT));
}
