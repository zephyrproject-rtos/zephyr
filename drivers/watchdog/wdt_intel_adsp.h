/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef ZEPHYR_DRIVERS_WATCHDOG_WDT_INTEL_ADSP_H_
#define ZEPHYR_DRIVERS_WATCHDOG_WDT_INTEL_ADSP_H_

/*
 * Get register offset for core
 */
#define DSPBRx_OFFSET(x)	(0x0020 * (x))

/*
 * DSPCxWDTCS
 * DSP Core Watch Dog Timer Control & Status
 *
 * Offset: 04h
 * Block: DSPBRx
 *
 * This register controls the DSP Core watch dog timer policy.
 */
#define DSPCxWDTCS		0x0004

/*
 * Pause Code
 * type: WO, rst: 00b, rst domain: nan
 *
 * FW write 76h as the code to set the PAUSED bit. Other value are ignored and has no effect.
 */
#define DSPCxWDTCS_PCODE	GENMASK(7, 0)
#define DSPCxWDTCS_PCODE_VALUE	0x76

/*
 * Paused
 * type: RW/1C, rst: 0b, rst domain: DSPLRST
 *
 * When set, it pauses the watch dog timer.  Set when 76h is written to the PCODE
 * field.  Clear when FW writes a 1 to the bit.
 */
#define DSPCxWDTCS_PAUSED	BIT(8)

/*
 * Second Time Out Reset Enable
 * type: RW/1S, rst: 0b, rst domain: DSPLRST
 *
 * When set, it allow the DSP Core reset to take place upon second time out of the
 * watch dog timer. Clear when DSPCCTL.CPA = 0.  Set when FW writes a 1 to the bit.
 */
#define DSPCxWDTCS_STORE	BIT(9)

/*
 * DSPCxWDTIPPTR
 * DSP Core Watch Dog Timer IP Pointer
 *
 * Offset: 08h
 * Block: DSPBRx
 *
 * This register provides the pointer to the DSP Core watch dog timer IP registers.
 */
#define DSPCxWDTIPPTR		0x0008

/*
 * IP Pointer
 * type: RO, rst: 07 8300h + 100h * x, rst domain: nan
 *
 * This field contains the offset to the IP.
 */
#define DSPCxWDTIPPTR_PTR	GENMASK(20, 0)

/*
 * IP Version
 * type: RO, rst: 000b, rst domain: nan
 *
 * This field indicates the version of the IP.
 */
#define DSPCxWDTIPPTR_VER	GENMASK(23, 21)

/**
 * @brief Set pause signal
 *
 * Sets the pause signal to stop the watchdog timing
 *
 * @param base Device base address.
 * @param core Core ID
 */
static inline void intel_adsp_wdt_pause(uint32_t base, const uint32_t core)
{
	const uint32_t reg_addr = base + DSPCxWDTCS + DSPBRx_OFFSET(core);
	uint32_t control;

	control = sys_read32(reg_addr);
	control &= DSPCxWDTCS_STORE;
	control |= FIELD_PREP(DSPCxWDTCS_PCODE, DSPCxWDTCS_PCODE_VALUE);
	sys_write32(control, reg_addr);
}

/**
 * @brief Clear pause signal
 *
 * Clears the pause signal to resume the watchdog timing
 *
 * @param base Device base address.
 * @param core Core ID
 */
static inline void intel_adsp_wdt_resume(uint32_t base, const uint32_t core)
{
	const uint32_t reg_addr = base + DSPCxWDTCS + DSPBRx_OFFSET(core);
	uint32_t control;

	control = sys_read32(reg_addr);
	control &= DSPCxWDTCS_STORE;
	control |= DSPCxWDTCS_PAUSED;
	sys_write32(control, reg_addr);
}

/**
 * @brief Second Time Out Reset Enable
 *
 * When set, it allow the DSP Core reset to take place upon second time out of the watchdog timer.
 *
 * @param base Device base address.
 * @param core Core ID
 */
static inline void intel_adsp_wdt_reset_set(uint32_t base, const uint32_t core, const bool enable)
{
	sys_write32(enable ? DSPCxWDTCS_STORE : 0, base + DSPCxWDTCS + DSPBRx_OFFSET(core));
}

/*
 * Second Time Out Reset Enable
 * type: RW/1S, rst: 0b, rst domain: DSPLRST
 *
 * When set, it allow the DSP Core reset to take place upon second time out of the
 * watch dog timer. Clear when DSPCCTL.CPA = 0.  Set when FW writes a 1 to the bit.
 */
#define DSPCxWDTCS_STORE BIT(9)

/**
 * @brief Get watchdog IP pointer for specified core.
 *
 * Returns the base address of the watchdog IP
 *
 * @param base Device base address.
 * @param core Core ID
 */
static inline uint32_t intel_adsp_wdt_pointer_get(uint32_t base, const uint32_t core)
{
	return FIELD_GET(DSPCxWDTIPPTR_PTR, sys_read32(base + DSPCxWDTIPPTR + DSPBRx_OFFSET(core)));
}

/**
 * @brief Get watchdog version
 *
 * Returns the version of the watchdog IP
 *
 * @param base Device base address.
 * @param core Core ID
 */
static inline uint32_t intel_adsp_wdt_version_get(uint32_t base, const uint32_t core)
{
	return FIELD_GET(DSPCxWDTIPPTR_VER, sys_read32(base + DSPCxWDTIPPTR + DSPBRx_OFFSET(core)));
}

#endif /* ZEPHYR_DRIVERS_WATCHDOG_WDT_INTEL_ADSP_H_ */
