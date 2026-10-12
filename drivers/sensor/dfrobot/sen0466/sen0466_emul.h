/*
 * Copyright (c) 2026, Iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_DFROBOT_SEN0466_SEN0466_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_DFROBOT_SEN0466_SEN0466_EMUL_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

/**
 * @brief Set the values the emulator reports in a data frame.
 *
 * @param target Emulator instance.
 * @param concentration Raw concentration, before the decimal places are applied.
 * @param gas_type Gas type byte.
 * @param decimals Decimal places, 0 to 2.
 * @param temp_adc Raw converter reading of the NTC.
 */
void sen0466_emul_set_sample(const struct emul *target, uint16_t concentration, uint8_t gas_type,
			     uint8_t decimals, uint16_t temp_adc);

/**
 * @brief Set the byte the emulator answers a mode change with.
 *
 * @param target Emulator instance.
 * @param ack Acknowledge byte, 1 for success.
 */
void sen0466_emul_set_ack(const struct emul *target, uint8_t ack);

/**
 * @brief Return a fixed frame to every read, bypassing the built-in frames.
 *
 * Use this to replay a frame captured from hardware.
 *
 * @param target Emulator instance.
 * @param frame Nine bytes to return, or NULL to go back to the built-in frames.
 */
void sen0466_emul_set_frame(const struct emul *target, const uint8_t *frame);

/**
 * @brief Return a frame to the next read only.
 *
 * The read after it is answered normally again, which is how a frame the part
 * sent on its own is modelled.
 *
 * @param target Emulator instance.
 * @param frame Nine bytes to return once, or NULL to drop a frame not yet read.
 */
void sen0466_emul_set_next_frame(const struct emul *target, const uint8_t *frame);

#endif /* ZEPHYR_DRIVERS_SENSOR_DFROBOT_SEN0466_SEN0466_EMUL_H_ */
