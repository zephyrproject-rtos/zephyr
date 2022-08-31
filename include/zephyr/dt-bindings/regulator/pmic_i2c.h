/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PMIC_I2C_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PMIC_I2C_H_

#define PMIC_MODE_OFFSET_MASK 0x1E00
#define PMIC_MODE_OFFSET_SHIFT 0x9

/*
 * PMIC mode offset macro. This macro will encode a PMIC mode offset into
 * a given PMIC mode. When selecting a mode for the regulator, all registers
 * will have an offset applied (vsel, ilim, enable), so that the new mode
 * can be configured. This macro encodes that offset for a given mode.
 * @param off: Offset to apply to PMIC config regs when selecting mode
 */
#define PMIC_MODE_OFFSET(off) \
	((off << PMIC_MODE_OFFSET_SHIFT) & PMIC_MODE_OFFSET_MASK)


/*
 * PMIC mode selection multi-register flag. This flag indicates that the
 * mode selection register for this mode requires an offset applied to it.
 * When this flag is set for a given mode, the value passed as the PMIC
 * mode selector will be written into the register:
 * <modesel-reg> + <pmic mode offset value>. If the flag is not passed,
 * the value of the PMIC mode selector will be written directly to
 * <modesel-reg> with no offset applied.
 */
#define PMIC_MODE_FLAG_MODESEL_MULTI_REG 0x100

/*
 * PMIC flags mask. These bits are used to indicate features or requirements
 * of PMIC modes.
 */
#define PMIC_MODE_FLAGS_MASK 0xF00

#define PMIC_MODE_SELECTOR_MASK 0xFF

/*
 * PMIC mode selector macro. This macro should be passed a value to be written
 * to the PMIC modesel-reg, in order to select a given mode. When the regulator
 * driver mode is switched, this value will be written to <modesel-reg>, or
 * <modesel-reg> + <pmic mode offset value> if MODESEL_MULTI_REG flag is set.
 * this will switch the PMIC to the new mode, which can then be configured with
 * the voltage and current limit setting APIs.
 * @param mode: mode selection value, to be written to modesel-reg
 */
#define PMIC_MODE_SELECTOR(mode) (mode & PMIC_MODE_SELECTOR_MASK)


/*
 * PMIC mode macro. This macro should be used to create a PMIC mode definition
 * for each PMIC mode. Each mode encodes the offset that must be applied to
 * the regulator's configuration registers to manage the specific mode, as well
 * as the value to write to the regulators' <modesel-reg>. Finally, the flags
 * field controls specific behaviors of the PMIC mode, which are defined
 * with the pattern PMIC_MODE_FLAG_xxx.
 *
 * @param offset: offset to apply to regulator configuration registers to
 *	configure the target PMIC mode's voltage and current
 * @param flags: pmic mode flags, used to indicate specific behaviors of
 *	a given mode.
 * @param selection_val: selection value, this is the actual value to be
 *	written to the pmic's <modesel-reg> to select the given mode.
 */

#define PMIC_MODE(offset, flags, selection_val) \
	PMIC_MODE_OFFSET(offset) | \
	(flags & PMIC_MODE_FLAGS_MASK) | \
	PMIC_MODE_SELECTOR(selection_val)


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PMIC_I2C_H_ */
