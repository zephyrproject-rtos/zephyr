/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Backend API for the ITE IT8801 MFD emulator.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_EMUL_ITE_IT8801_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_EMUL_ITE_IT8801_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ITE IT8801 MFD backend emulator APIs
 * @defgroup ite_it8801_emulator_backend ITE IT8801 MFD backend emulator APIs
 * @ingroup io_emulators
 * @{
 */

/** Sentinel value used to disable I2C read/write failure injection. */
#define IT8801_EMUL_NO_FAIL_REG 0xFFFF

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
 */
__subsystem struct it8801_emul_driver_api {
	void (*reset)(const struct emul *target);
	int (*get_reg)(const struct emul *target, uint8_t reg, uint8_t *val);
	int (*set_reg)(const struct emul *target, uint8_t reg, uint8_t val);
	void (*set_read_fail_reg)(const struct emul *target, uint16_t reg);
	void (*set_write_fail_reg)(const struct emul *target, uint16_t reg);
};
/**
 * @endcond
 */

/**
 * @brief Reset the IT8801 emulator to its default state.
 *
 * All internal register state is cleared to zero and any pending failure
 * injection is cancelled.
 *
 * @param target Pointer to the emulator instance.
 */
static inline void it8801_emul_reset(const struct emul *target)
{
	const struct it8801_emul_driver_api *api =
		(const struct it8801_emul_driver_api *)target->backend_api;

	api->reset(target);
}

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
static inline int it8801_emul_get_reg(const struct emul *target, uint8_t reg,
				      uint8_t *val)
{
	const struct it8801_emul_driver_api *api =
		(const struct it8801_emul_driver_api *)target->backend_api;

	return api->get_reg(target, reg, val);
}

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
static inline int it8801_emul_set_reg(const struct emul *target, uint8_t reg,
				      uint8_t val)
{
	const struct it8801_emul_driver_api *api =
		(const struct it8801_emul_driver_api *)target->backend_api;

	return api->set_reg(target, reg, val);
}

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
static inline void it8801_emul_set_read_fail_reg(const struct emul *target,
						 uint16_t reg)
{
	const struct it8801_emul_driver_api *api =
		(const struct it8801_emul_driver_api *)target->backend_api;

	api->set_read_fail_reg(target, reg);
}

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
static inline void it8801_emul_set_write_fail_reg(const struct emul *target,
						  uint16_t reg)
{
	const struct it8801_emul_driver_api *api =
		(const struct it8801_emul_driver_api *)target->backend_api;

	api->set_write_fail_reg(target, reg);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_EMUL_ITE_IT8801_H_ */
