/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Emulator for the MAXM86161 Optical Biosensor
 */

#ifndef MAXM86161_EMUL_H_
#define MAXM86161_EMUL_H_

#include <stdint.h>

/**
 * @brief Set a register value in the emulated MAXM86161.
 *
 * @param data_ptr Pointer to the emulator data (from emul->data).
 * @param reg Register address.
 * @param value Value to write.
 * @return 0 on success, negative errno on failure.
 */
int maxm86161_mock_set_register(void *data_ptr, uint8_t reg, uint8_t value);

/**
 * @brief Get a register value from the emulated MAXM86161.
 *
 * @param data_ptr Pointer to the emulator data (from emul->data).
 * @param reg Register address.
 * @param value Pointer to store the read value.
 * @return 0 on success, negative errno on failure.
 */
int maxm86161_mock_get_register(void *data_ptr, uint8_t reg, uint8_t *value);

/**
 * @brief Inject an I2C bus fault for transactions touching @p reg.
 *
 * Once armed, any read or write transaction whose starting register address
 * equals @p reg returns @p err (a negative errno such as -EIO) instead of
 * completing. Used to exercise the driver's bus-error propagation paths.
 *
 * @param data_ptr Pointer to the emulator data (from emul->data).
 * @param reg Register address that should fault, or 0 with @p err == 0 to
 *            clear any previously-armed fault (see maxm86161_mock_clear_fault()).
 * @param err Negative errno to return, or 0 to disarm.
 * @return 0 on success.
 */
int maxm86161_mock_set_fault(void *data_ptr, uint8_t reg, int err);

/**
 * @brief Clear any armed I2C bus fault.
 *
 * @param data_ptr Pointer to the emulator data (from emul->data).
 * @return 0 on success.
 */
int maxm86161_mock_clear_fault(void *data_ptr);

/**
 * @brief Enable or disable emulation of self-clearing bits.
 *
 * By default the emulator auto-clears the SYSTEM_CONTROL reset bit and the
 * DIE_TEMP_CONFIG TEMP_EN bit after a write, mimicking the real device so that
 * driver polling loops terminate. Disabling this lets tests exercise the
 * timeout / "measurement not complete" branches by keeping those bits set.
 *
 * @param data_ptr Pointer to the emulator data (from emul->data).
 * @param enable true to auto-clear self-clearing bits (default), false to hold.
 * @return 0 on success.
 */
int maxm86161_mock_set_selfclear(void *data_ptr, bool enable);

#endif /* MAXM86161_EMUL_H_ */
