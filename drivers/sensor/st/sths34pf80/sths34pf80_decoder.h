/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Raw frame format and decoder of the STHS34PF80 sensor driver.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_DECODER_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/**
 * @brief Index of every value the driver can report.
 *
 * Used as the bit number in sths34pf80_rtio_data::valid.
 */
enum sths34pf80_idx {
	STHS34PF80_IDX_OBJECT,
	STHS34PF80_IDX_AMBIENT,
	STHS34PF80_IDX_OBJECT_COMP,
	STHS34PF80_IDX_PRESENCE,
	STHS34PF80_IDX_MOTION,
	STHS34PF80_IDX_TEMP_SHOCK,
	STHS34PF80_IDX_PRESENCE_FLAG,
	STHS34PF80_IDX_MOTION_FLAG,
	STHS34PF80_IDX_TEMP_SHOCK_FLAG,
	STHS34PF80_IDX_COUNT,
};

/*
 * The output registers are read in two blocks, so that no undocumented
 * register is touched:
 *   block 1: FUNC_STATUS (0x25), TOBJECT (0x26-0x27), TAMBIENT (0x28-0x29)
 *   block 2: TOBJ_COMP (0x38-0x39), TPRESENCE (0x3A-0x3B), TMOTION (0x3C-0x3D),
 *            TAMB_SHOCK (0x3E-0x3F)
 * The I2C register address auto-increments, so each block is a single read.
 */
#define STHS34PF80_RTIO_REG_BLOCK1 0x25
#define STHS34PF80_RTIO_LEN_BLOCK1 5
#define STHS34PF80_RTIO_REG_BLOCK2 0x38
#define STHS34PF80_RTIO_LEN_BLOCK2 8

/**
 * @brief Hold the raw frame written by submit() and consumed by the decoder.
 *
 * The block buffers hold the bytes exactly as read from the sensor; they are
 * parsed by the decoder.
 */
struct sths34pf80_rtio_data {
	uint64_t timestamp;                         /**< Timestamp of the read, in ns. */
	uint16_t valid;                             /**< Bitmask of requested values. */
	uint8_t block1[STHS34PF80_RTIO_LEN_BLOCK1]; /**< Raw bytes of register block 1. */
	uint8_t block2[STHS34PF80_RTIO_LEN_BLOCK2]; /**< Raw bytes of register block 2. */
};

/**
 * @brief Map a sensor channel to an enum sths34pf80_idx value.
 *
 * @param chan Sensor channel.
 *
 * @return The matching enum sths34pf80_idx value.
 * @retval -ENOTSUP if the channel is not supported by this driver.
 */
int sths34pf80_chan_to_idx(enum sensor_channel chan);

/**
 * @brief Get the decoder API of the driver.
 *
 * @param dev     Sensor device instance (unused).
 * @param decoder Pointer set to the decoder API.
 *
 * @retval 0 always.
 */
int sths34pf80_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_STHS34PF80_STHS34PF80_DECODER_H_ */
