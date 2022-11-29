/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_PCA9420_I2C_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_PCA9420_I2C_H_

#define PCA9420_MODE_OFFSET_MASK 0x1E00
#define PCA9420_MODE_OFFSET_SHIFT 0x9

/*
 * PMIC mode offset macro. This macro will encode a PMIC mode offset into
 * a given PMIC mode. When selecting a mode for the regulator, all registers
 * will have an offset applied (vsel, ilim, enable), so that the new mode
 * can be configured. This macro encodes that offset for a given mode.
 * @param off: Offset to apply to PMIC config regs when selecting mode
 */
#define PCA9420_MODE_OFFSET(off) \
	((off << PCA9420_MODE_OFFSET_SHIFT) & PCA9420_MODE_OFFSET_MASK)


/*
 * PMIC mode selection multi-register flag. This flag indicates that the
 * mode selection register for this mode requires an offset applied to it.
 * When this flag is set for a given mode, the value passed as the PMIC
 * mode selector will be written into the register:
 * <modesel-reg> + <pmic mode offset value>. If the flag is not passed,
 * the value of the PMIC mode selector will be written directly to
 * <modesel-reg> with no offset applied.
 */
#define PCA9420_MODE_FLAG_MODESEL_MULTI_REG 0x100

/*
 * PMIC flags mask. These bits are used to indicate features or requirements
 * of PMIC modes.
 */
#define PCA9420_MODE_FLAGS_MASK 0xF00

#define PCA9420_MODE_SELECTOR_MASK 0xFF

/*
 * PMIC mode selector macro. This macro should be passed a value to be written
 * to the PMIC modesel-reg, in order to select a given mode. When the regulator
 * driver mode is switched, this value will be written to <modesel-reg>, or
 * <modesel-reg> + <pmic mode offset value> if MODESEL_MULTI_REG flag is set.
 * this will switch the PMIC to the new mode, which can then be configured with
 * the voltage and current limit setting APIs.
 * @param mode: mode selection value, to be written to modesel-reg
 */
#define PCA9420_MODE_SELECTOR(mode) (mode & PCA9420_MODE_SELECTOR_MASK)


/*
 * PMIC mode macro. This macro should be used to create a PMIC mode definition
 * for each PMIC mode. Each mode encodes the offset that must be applied to
 * the regulator's configuration registers to manage the specific mode, as well
 * as the value to write to the regulators' <modesel-reg>. Finally, the flags
 * field controls specific behaviors of the PMIC mode, which are defined
 * with the pattern PCA9420_MODE_FLAG_xxx.
 *
 * @param offset: offset to apply to regulator configuration registers to
 *	configure the target PMIC mode's voltage and current
 * @param flags: pmic mode flags, used to indicate specific behaviors of
 *	a given mode.
 * @param selection_val: selection value, this is the actual value to be
 *	written to the pmic's <modesel-reg> to select the given mode.
 */

#define PCA9420_MODE(offset, flags, selection_val) \
	PCA9420_MODE_OFFSET(offset) | \
	(flags & PCA9420_MODE_FLAGS_MASK) | \
	PCA9420_MODE_SELECTOR(selection_val)

#define PCA9420_MODECFG0 \
	(PCA9420_MODE(0x0, 0x0, 0x0)) /* ModeCfg 0, selected via I2C */
#define PCA9420_MODECFG1 \
	(PCA9420_MODE(0x4, 0x0, 0x8)) /* ModeCfg 1, selected via I2C */
#define PCA9420_MODECFG2 \
	(PCA9420_MODE(0x8, 0x0, 0x10)) /* ModeCfg 2, selected via I2C */
#define PCA9420_MODECFG3 \
	(PCA9420_MODE(0xC, 0x0, 0x18)) /* ModeCfg 3, selected via I2C */

/* ModeCfg 0, selected via PIN */
#define PCA9420_MODECFG0_PIN \
	(PCA9420_MODE(0x0, PCA9420_MODE_FLAG_MODESEL_MULTI_REG, 0x40))
/* ModeCfg 1, selected via PIN */
#define PCA9420_MODECFG1_PIN \
	(PCA9420_MODE(0x4, PCA9420_MODE_FLAG_MODESEL_MULTI_REG, 0x40))
/* ModeCfg 2, selected via PIN */
#define PCA9420_MODECFG2_PIN \
	(PCA9420_MODE(0x8, PCA9420_MODE_FLAG_MODESEL_MULTI_REG, 0x40))
/* ModeCfg 3, selected via PIN */
#define PCA9420_MODECFG3_PIN \
	(PCA9420_MODE(0xC, PCA9420_MODE_FLAG_MODESEL_MULTI_REG, 0x40))

/** @brief Top level system ctrl 3 */
#define PCA9420_TOP_CNTL3     0x0CU

/** @brief Mode configuration for mode 0_0 */
#define PCA9420_MODECFG_0_0          0x22U

/** @brief I2C Mode control mask */
#define PCA9420_TOP_CNTL3_MODE_I2C_MASK 0x18U

/*
 * @brief Mode control selection mask. When this bit is set, the external
 * PMIC pins MODESEL0 and MODESEL1 can be used to select the active mode
 */
#define PCA9420_MODECFG_0_MODE_CTRL_SEL_MASK 0x40U

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_PCA9420_I2C_H_*/
