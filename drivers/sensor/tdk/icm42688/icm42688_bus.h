/*
 * Copyright (c) 2025 ZARM, University of Bremen
 * Copyright (c) 2025 Croxel Inc.alignas
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm42688

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42688_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42688_BUS_H_

#include <stdint.h>
#include <zephyr/rtio/rtio.h>

#include "icm42688.h"
#include "icm42688_reg.h"

/**
 * @brief read from one or more ICM42688 registers
 *
 * this functions wraps all logic necessary to read from any of the ICM42688 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param dev ICM42688 device pointer
 * @param reg start address of ICM42688 register(s) to read from
 * @param buf pointer to byte array to read register values to
 * @param len number of bytes to read from the device
 * @return int 0 on success, negative error code otherwise
 */
static inline int icm42688_bus_read(const struct device *dev, uint16_t reg, uint8_t *buf,
				    uint16_t len)
{
	struct icm42688_dev_data *data = dev->data;
	const struct icm42688_dev_cfg *cfg = dev->config;
	struct rtio *ctx = data->rtio_ctx;
	struct rtio_iodev *iodev = data->rtio_iodev;
	struct rtio_sqe *write_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(ctx);
	struct rtio_cqe *cqe;
	int err;

	if (!write_sqe || !read_sqe) {
		return -ENOMEM;
	}

	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	if (cfg->inst_on_bus == ICM42688_BUS_SPI) {
		address = address | REG_SPI_READ_BIT;
	}
	rtio_sqe_prep_tiny_write(write_sqe, iodev, RTIO_PRIO_HIGH, &address, 1, NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_sqe, iodev, RTIO_PRIO_HIGH, buf, len, NULL);
	if (cfg->inst_on_bus == ICM42688_BUS_I2C) {
		read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	err = rtio_submit(ctx, 2);
	if (err) {
		return err;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return err;
}

/**
 * @brief write to a one or more ICM42688 registers
 *
 * this functions wraps all logic necessary to write to any of the ICM42688 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param dev ICM42688 device pointer
 * @param reg start address of ICM42688 register(s) to write to
 * @param buf pointer to byte array to write register values from
 * @param len number of bytes to read from the device
 * @return int 0 on success, negative error code otherwise
 */
static inline int icm42688_bus_write(const struct device *dev, uint16_t reg, const uint8_t *buf,
				     uint16_t len)
{
	struct icm42688_dev_data *data = dev->data;
	const struct icm42688_dev_cfg *cfg = dev->config;
	struct rtio *ctx = data->rtio_ctx;
	struct rtio_iodev *iodev = data->rtio_iodev;
	struct rtio_cqe *cqe;
	int err;

	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	if (cfg->inst_on_bus == ICM42688_BUS_SPI) {
		struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
		struct rtio_sqe *write_buf_sqe = rtio_sqe_acquire(ctx);

		if (!write_reg_sqe || !write_buf_sqe) {
			return -ENOMEM;
		}

		rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_HIGH, &address, 1, NULL);
		write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
		rtio_sqe_prep_write(write_buf_sqe, iodev, RTIO_PRIO_HIGH, buf, len, NULL);
		err = rtio_submit(ctx, 2);
	} else if (cfg->inst_on_bus == ICM42688_BUS_I2C) {
		struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);

		if (!write_reg_sqe) {
			return -ENOMEM;
		}

		uint8_t msg[len + 1];

		msg[0] = address;
		memcpy(&msg[1], buf, len);
		rtio_sqe_prep_write(write_reg_sqe, iodev, RTIO_PRIO_HIGH, msg, sizeof(msg), NULL);
		write_reg_sqe->iodev_flags = RTIO_IODEV_I2C_STOP;
		err = rtio_submit(ctx, 1);
	} else {
		err = -ENODEV;
	}

	if (err) {
		return err;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return err;
}
#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42688_BUS_H_ */
