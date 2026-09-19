/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Control functions for the ITE IT8801 MFD emulator.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_EMUL_ITE_IT8801_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_EMUL_ITE_IT8801_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ITE IT8801 MFD emulator control functions
 * @defgroup ite_it8801_emulator ITE IT8801 MFD emulator control functions
 * @ingroup io_emulators
 * @{
 */

/** Sentinel value used to disable I2C read/write failure injection. */
#define IT8801_EMUL_NO_FAIL_REG 0xFFFF

/**
 * @name Keyboard scan-out column-drive states
 *
 * Non-negative return values of it8801_emul_get_driven_column() are the
 * keyboard scan-out (KSO) line driven low; these sentinels report the states
 * where no single line applies.
 * @{
 */
/** No scan-out line is driven (all outputs deasserted high). */
#define IT8801_EMUL_COLUMN_NONE (-1)
/** All scan-out lines are driven (all outputs asserted low). */
#define IT8801_EMUL_COLUMN_ALL  (-2)
/** @} */

/**
 * @brief Reset the IT8801 emulator to its default state.
 *
 * All internal register state is cleared to zero and any pending failure
 * injection is cancelled.
 *
 * @param target Pointer to the emulator instance.
 */
void it8801_emul_reset(const struct emul *target);

/**
 * @brief Read the current value of an emulated register.
 *
 * @param target Pointer to the emulator instance.
 * @param reg    Register address to read (0x00-0xFF).
 * @param val    Pointer to store the register value.
 *
 * @retval 0 On success.
 * @retval -EINVAL If @p val is NULL.
 */
int it8801_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *val);

/**
 * @brief Set the value of an emulated register.
 *
 * Used by test code to seed the emulator with hardware state that the
 * driver-under-test will subsequently observe.
 *
 * @param target Pointer to the emulator instance.
 * @param reg    Register address to write (0x00-0xFF).
 * @param val    Value to write to the register.
 *
 * @retval 0 On success.
 */
int it8801_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t val);

/**
 * @brief Configure the emulator to fail I2C reads targeting a specific register.
 *
 * Subsequent I2C read transactions to the specified register return -EIO.
 * Set to @ref IT8801_EMUL_NO_FAIL_REG to disable failure injection.
 *
 * @param target Pointer to the emulator instance.
 * @param reg    Register address that should trigger read failures, or
 *               @ref IT8801_EMUL_NO_FAIL_REG to clear.
 */
void it8801_emul_set_read_fail_reg(const struct emul *target, uint16_t reg);

/**
 * @brief Configure the emulator to fail I2C writes targeting a specific register.
 *
 * Subsequent I2C write transactions to the specified register return -EIO.
 * Set to @ref IT8801_EMUL_NO_FAIL_REG to disable failure injection.
 *
 * @param target Pointer to the emulator instance.
 * @param reg    Register address that should trigger write failures, or
 *               @ref IT8801_EMUL_NO_FAIL_REG to clear.
 */
void it8801_emul_set_write_fail_reg(const struct emul *target, uint16_t reg);

/**
 * @brief Decode the keyboard scan-out register into a driven-column state.
 *
 * Interprets the keyboard scan out mode control register the way the IT8801
 * hardware would, so tests can assert on the column the driver is driving
 * rather than on the raw register bits.
 *
 * @param target     Pointer to the emulator instance.
 * @param ksomcr_reg Address of the keyboard scan out mode control register.
 *
 * @retval IT8801_EMUL_COLUMN_NONE No scan-out line is driven.
 * @retval IT8801_EMUL_COLUMN_ALL All scan-out lines are driven.
 * @retval line The scan-out (KSO) line driven low for a single column.
 */
int it8801_emul_get_driven_column(const struct emul *target, uint8_t ksomcr_reg);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_EMUL_ITE_IT8801_H_ */
