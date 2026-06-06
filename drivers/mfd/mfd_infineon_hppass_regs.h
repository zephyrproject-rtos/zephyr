/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief HPPASS internal register helpers (composed constants, TRM citations).
 *
 * Internal driver header.  Register addresses, accessor macros, and field
 * Pos/Msk constants come from @c cy_device.h, which itself includes
 * @c ip/cyip_hppass.h.  Both are generated from the same source as the
 * device tree files and are the authoritative single source of truth for
 * the register map.
 *
 * What this header provides
 * -------------------------
 *
 *   - Composed constants for register values whose individual fields
 *     are documented in the TRM but whose @e composition is required
 *     for normal operation (e.g. AREF_CTRL init value).
 *   - TRM section citations next to each helper.
 *
 * References
 * ----------
 *
 * HPPASS is shared across multiple Infineon device families. The TRM
 * citations throughout this file refer to the PSOC Control C3 Mainline
 * documentation, which is the reference used to derive the composed
 * constants below:
 *
 *   - <em>PSOC Control C3 Mainline Architecture TRM</em>,
 *     002-39348 Rev. *C
 *   - <em>PSOC Control C3 Mainline Registers TRM</em>,
 *     002-39445 Rev. *B
 */

#ifndef ZEPHYR_DRIVERS_MFD_MFD_INFINEON_HPPASS_REGS_H_
#define ZEPHYR_DRIVERS_MFD_MFD_INFINEON_HPPASS_REGS_H_

#include <cy_device.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Register offsets within the HPPASS @c reg base.  Pair with
 * @c sys_read32() / @c sys_write32() / @c sys_set_bits() /
 * @c sys_clear_bits().  Field @c _Pos / @c _Msk constants continue
 * to come from @c cy_device.h (the PDL register-definition header).
 *
 * Section base offsets within HPPASS (Regs TRM Table 20-1):
 *   SARADC = 0x00070000   (SAR ADC calibration)
 *   ACTRLR = 0x000D0000   (Autonomous Controller)
 *   INFRA  = 0x000E0000   (subsystem infrastructure / AREF / triggers)
 *   MMIO   = 0x000F0000   (top-level MMIO, INTR, trigger routing)
 */

/* SARADC calibration. */
#define IFX_HPPASS_SARADC_CALLIN(n)        (0x00070020U + (n) * 4U)

/* ACTRLR (Autonomous Controller). */
#define IFX_HPPASS_AC_CTRL                 0x000D0000U
#define IFX_HPPASS_AC_CMD_RUN              0x000D0004U
#define IFX_HPPASS_AC_CMD_STATE            0x000D0008U
#define IFX_HPPASS_AC_BLOCK_STATUS         0x000D0010U
#define IFX_HPPASS_AC_STATUS               0x000D0020U
#define IFX_HPPASS_AC_CFG                  0x000D0024U
/* TTCFG[16] array at ACTRLR + 0x800, each entry 0x10 bytes. */
#define IFX_HPPASS_AC_TT_CFG0(n)           (0x000D0800U + (n) * 0x10U + 0x0U)
#define IFX_HPPASS_AC_TT_CFG1(n)           (0x000D0800U + (n) * 0x10U + 0x4U)
#define IFX_HPPASS_AC_TT_CFG2(n)           (0x000D0800U + (n) * 0x10U + 0x8U)
#define IFX_HPPASS_AC_TT_CFG3(n)           (0x000D0800U + (n) * 0x10U + 0xCU)

/* INFRA. */
#define IFX_HPPASS_INFRA_TR_IN_SEL         0x000E0000U
#define IFX_HPPASS_INFRA_HW_TR_MODE        0x000E0004U
#define IFX_HPPASS_INFRA_FW_TR_PULSE       0x000E0008U
#define IFX_HPPASS_INFRA_FW_TR_LEVEL       0x000E000CU
#define IFX_HPPASS_INFRA_CLOCK_STARTUP_DIV 0x000E0020U
#define IFX_HPPASS_INFRA_STARTUP_CFG(n)    (0x000E0030U + (n) * 4U)
#define IFX_HPPASS_INFRA_VDDA_STATUS       0x000E0050U
#define IFX_HPPASS_INFRA_AREF_CTRL         0x000E0E00U

/* MMIO. */
#define IFX_HPPASS_MMIO_INTR               0x000F0040U
#define IFX_HPPASS_MMIO_INTR_MASK          0x000F0048U
#define IFX_HPPASS_MMIO_INTR_MASKED        0x000F004CU
#define IFX_HPPASS_MMIO_TR_LEVEL_CFG       0x000F0050U
#define IFX_HPPASS_MMIO_TR_LEVEL_OUT(n)    (0x000F0060U + (n) * 4U)
#define IFX_HPPASS_MMIO_TR_PULSE_OUT(n)    (0x000F0080U + (n) * 4U)

/*
 * CSG (Comparator Slope Generator): offsets relative to CSG @c reg base.
 * CSG section base within HPPASS is 0x000B0000 (Regs TRM Table 20-1),
 * but the CSG DT node has its own reg property pointing at 0x000B0000
 * so offsets below are relative to that.
 *
 * Regs TRM 002-39445 §20.1.53–20.1.63.
 */
#define IFX_HPPASS_CSG_CTRL                0x00000A00U

/* Per-slice register offsets relative to the slice base (Regs TRM §20.1.42–20.1.51). */
#define IFX_HPPASS_CSG_SLICE_CMP_CFG       0x00U
#define IFX_HPPASS_CSG_SLICE_CMP_STATUS    0x24U

/* DAC register offsets relative to the DAC node base (slice + 0x04). */
#define IFX_HPPASS_CSG_SLICE_DAC_CFG       0x00U
#define IFX_HPPASS_CSG_SLICE_DAC_VAL       0x18U

#define IFX_HPPASS_CSG_CMP_INTR            0x00000B10U
#define IFX_HPPASS_CSG_CMP_INTR_SET        0x00000B14U
#define IFX_HPPASS_CSG_CMP_INTR_MASK       0x00000B18U
#define IFX_HPPASS_CSG_CMP_INTR_MASKED     0x00000B1CU
#define IFX_HPPASS_CSG_VDAC_OUT_BLANK      0x00000B20U

/**
 * @brief Composed value for AREFv2_AREF_CTRL "normal operation, internal
 *        Vref" mode.
 *
 * Per Regs TRM 20.1.83 (HPPASS_INFRA_AREFv2_AREF_CTRL), the following
 * reserved fields must be set to specific values for normal operation:
 *
 *   - VREF_SEL[21:20]      = 0x1 (LOCAL)        - "for normal operation,
 *                                                  set to '1' selection"
 *   - IZTAT_SEL[16]        = 0x1 (LOCAL)        - "for normal operation,
 *                                                  set to '1' selection"
 *   - AREF_BIAS_SCALE[3:2] = 0x1                - "must be set to b01"
 *
 */
#define IFX_HPPASS_AREF_CTRL_NORMAL_INTERNAL                                     \
	(HPPASS_INFRA_AREFV2_AREF_CTRL_ENABLED_Msk |                             \
	 (1 << HPPASS_INFRA_AREFV2_AREF_CTRL_VREF_SEL_Pos) |                   \
	 (1 << HPPASS_INFRA_AREFV2_AREF_CTRL_IZTAT_SEL_Pos) |                  \
	 (1 << HPPASS_INFRA_AREFV2_AREF_CTRL_AREF_BIAS_SCALE_Pos))

/**
 * @brief Number of AC state-transition table (STT) slots.
 *
 * Fixed by silicon (Arch TRM §27.2.4.3): 16 @c TTCFG[n] register groups.
 */
#define IFX_HPPASS_AC_STT_COUNT          16

/**
 * @brief Number of SAR linearity calibration table entries.
 *
 * Fixed by silicon (Arch TRM 27.2.2.9): 16 @c SARADC.CALLIN[n] words,
 * mirrored from SFLASH @c SAR_CAL_LIN_TABLE[].
 */
#define IFX_HPPASS_SAR_CAL_LIN_TABLE_SIZE 16

/**
 * @brief Mask of all sources reported by @c HPPASS_INTR (Regs TRM 20.1.96).
 *
 * Convenience for clearing/masking all sources at once.
 */
#define IFX_HPPASS_INTR_ALL_MSK                                                  \
	(HPPASS_MMIO_HPPASS_INTR_FIFO_OVERFLOW_Msk |                             \
	 HPPASS_MMIO_HPPASS_INTR_FIFO_UNDERFLOW_Msk |                            \
	 HPPASS_MMIO_HPPASS_INTR_RESULT_OVERFLOW_Msk |                           \
	 HPPASS_MMIO_HPPASS_INTR_ENTRY_TR_COLLISION_Msk |                        \
	 HPPASS_MMIO_HPPASS_INTR_ENTRY_HOLD_VIOLATION_Msk |                      \
	 HPPASS_MMIO_HPPASS_INTR_AC_INT_Msk)

/**
 * @brief Encoded @c STATUS field value indicating the AC is running.
 *
 * Per Regs TRM 20.1.68 (HPPASS_ACTRLR_STATUS): @c STATUS[9:8] = @c 0x1.
 */
#define IFX_HPPASS_AC_STATUS_RUNNING                                             \
	(0x1 << HPPASS_ACTRLR_STATUS_STATUS_Pos)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MFD_MFD_INFINEON_HPPASS_REGS_H_ */
