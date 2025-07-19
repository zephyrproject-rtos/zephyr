/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RTIO_REGMAP_H_
#define ZEPHYR_INCLUDE_RTIO_REGMAP_H_

#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A structure to describe a list of not-consecutive memory chunks
 * for RTIO operations.
 */
struct rtio_reg_buf {
	/* Valid pointer to a data buffer */
	uint8_t *bufp;

	/* Length of the buffer in bytes */
	size_t len;
};

/**
 * @brief bus type
 *
 * RTIO works on top of a RTIO enabled bus, Some RTIO ops require
 * a bus-related handling (e.g. rtio_read_regs_async)
 */
typedef enum {
	BUS_I2C,
	BUS_SPI,
	BUS_I3C,
} rtio_bus_type;

/**
 * @brief chceck if bus is SPI
 */
static inline bool rtio_is_spi(rtio_bus_type bus_type)
{
	return (bus_type == BUS_SPI);
}

/**
 * @brief chceck if bus is I2C
 */
static inline bool rtio_is_i2c(rtio_bus_type bus_type)
{
	return (bus_type == BUS_I2C);
}

/**
 * @brief chceck if bus is I3C
 */
static inline bool rtio_is_i3c(rtio_bus_type bus_type)
{
	return (bus_type == BUS_I3C);
}

/*
 * @brief Create a chain of SQEs representing a bus transaction to read a reg.
 *
 * The RTIO-enabled bus driver is isnstrumented to perform bus read ops
 * for each register in the list.
 *
 * Usage:
 *
 * uint8_t regs_list[]  = { reg_addr1, reg_addr2. ... }
 * struct rtio_reg_buf rbuf[] = {{mem_addr_1, mem_len_1},
 *                               {mem_addr_2, mem_len_2},
 *                               ...
 *                              };
 *
 * rtio_read_regs_async(dev,
 *                      regs_list,
 *                      ARRAY_SIZE(regs_list),
 *                      BUS_SPI,
 *                      rbuf,
 *                      sqe,
 *                      iodev,
 *                      rtio,
 *                      op_cb);
 *
 * @param regs pointer to list of registers to be read. Raise proper bit in case of SPI bus
 * @param rtio_bus_type bus type (i2c, spi, i3c)
 * @param rtio_reg_buf buffer of memory chunks
 * @param iodev_sqe IODEV submission for the await op
 * @param iodev IO device
 * @param r RTIO context
 * @param rtio_callback_t callback routine at the end of op
 */
static inline void rtio_read_regs_async(struct rtio *r,
					struct rtio_iodev *iodev,
					uint8_t *regs,
					uint8_t regs_num,
					rtio_bus_type bus_type,
					struct rtio_reg_buf *buf,
					struct rtio_iodev_sqe *iodev_sqe,
					const struct device *dev,
					rtio_callback_t complete_op_cb)
{
	struct rtio_sqe *write_addr;
	struct rtio_sqe *read_reg;
	struct rtio_sqe *complete_op;
	uint8_t i;

	for (i = 0; i < regs_num; i++) {

		write_addr = rtio_sqe_acquire(r);
		read_reg = rtio_sqe_acquire(r);

		if (write_addr == NULL || read_reg == NULL) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			rtio_sqe_drop_all(r);
			return;
		}

		rtio_sqe_prep_tiny_write(write_addr, iodev, RTIO_PRIO_NORM, &regs[i], 1, NULL);
		write_addr->flags = RTIO_SQE_TRANSACTION;
		rtio_sqe_prep_read(read_reg, iodev, RTIO_PRIO_NORM, buf[i].bufp, buf[i].len, NULL);
		read_reg->flags = RTIO_SQE_CHAINED;

		if (rtio_is_i2c(bus_type)) {
			read_reg->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
		} else if (rtio_is_i3c(bus_type)) {
			read_reg->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
		}
	}

	complete_op = rtio_sqe_acquire(r);
	if (complete_op == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		rtio_sqe_drop_all(r);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(complete_op, complete_op_cb, (void *)dev, iodev_sqe);

	rtio_submit(r, 0);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RTIO_REGMAP_H_ */
