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

#include <stdbool.h>
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

/** Failure count that makes every access to the register fail. */
#define IT8801_EMUL_FAIL_ALWAYS (-1)

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

/** Number of keyboard scan-out (KSO) lines modelled by the emulator. */
#define IT8801_EMUL_KSO_COUNT 23
/** Number of keyboard scan-in (KSI) lines modelled by the emulator. */
#define IT8801_EMUL_KSI_COUNT 8

/**
 * @brief Reset the IT8801 emulator to its default state.
 *
 * All internal register state is cleared to zero, the vendor ID registers
 * are re-seeded, every key is released and any pending failure injection is
 * cancelled. The SMB_INT# alert line is not touched.
 *
 * Resetting while a driver is running also discards the configuration it
 * wrote, so only call this before the driver under test is initialized.
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
 * @brief Configure the emulator to fail I2C reads of a specific register.
 *
 * The next @p count I2C reads of @p reg return -EIO, after which the
 * register reads normally again. A failed read fills the read buffer with
 * 0xFF, as the data line of an idle I2C bus is pulled high. Replaces any
 * read failure configured earlier.
 *
 * @param target Pointer to the emulator instance.
 * @param reg    Register address that should trigger read failures.
 * @param count  Number of reads to fail, @ref IT8801_EMUL_FAIL_ALWAYS to fail
 *               every read, or 0 to disable read failure injection.
 */
void it8801_emul_set_read_fail(const struct emul *target, uint8_t reg, int count);

/**
 * @brief Configure the emulator to fail I2C writes of a specific register.
 *
 * The next @p count I2C writes of @p reg return -EIO without changing the
 * register, after which it is written normally again. Replaces any write
 * failure configured earlier.
 *
 * @param target Pointer to the emulator instance.
 * @param reg    Register address that should trigger write failures.
 * @param count  Number of writes to fail, @ref IT8801_EMUL_FAIL_ALWAYS to fail
 *               every write, or 0 to disable write failure injection.
 */
void it8801_emul_set_write_fail(const struct emul *target, uint8_t reg, int count);

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
 * @brief Press or release a key in the emulated keyboard matrix.
 *
 * The key connects scan-out line @p kso to scan-in line @p ksi. While its
 * scan-out line is driven, a pressed key pulls the scan-in line low, which
 * the driver observes through the keyboard scan in data register.
 *
 * A falling edge on a scan-in line that is enabled in the keyboard scan in
 * interrupt enable register, with the gather KSI interrupt enabled, latches
 * the line into the keyboard scan in edge event register and drives the
 * MFD's irq-gpios line low through the GPIO emulator. The line is released
 * once the driver clears every pending edge event (write-1-to-clear).
 *
 * @param target  Pointer to the emulator instance.
 * @param kso     Scan-out line of the key (0 to IT8801_EMUL_KSO_COUNT - 1).
 * @param ksi     Scan-in line of the key (0 to IT8801_EMUL_KSI_COUNT - 1).
 * @param pressed True to press the key, false to release it.
 *
 * @retval 0 On success.
 * @retval -EINVAL If a line is out of range or the MFD has no keyboard node.
 */
int it8801_emul_set_key(const struct emul *target, uint8_t kso, uint8_t ksi, bool pressed);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_EMUL_ITE_IT8801_H_ */
