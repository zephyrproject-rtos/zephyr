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
struct rtio_regs {
	/** Number of registers in the list **/
	size_t rtio_regs_num;

	struct rtio_regs_list {
		/** Register address **/
		uint8_t reg_addr;

		/** Valid pointer to a data buffer **/
		uint8_t *bufp;

		/** Length of the buffer in bytes **/
		size_t len;
	} *rtio_regs_list;
};

/**
 * @brief bus type
 *
 * RTIO works on top of a RTIO enabled bus, Some RTIO ops require
 * a bus-related handling (e.g. rtio_read_regs_async)
 */
typedef enum {
	RTIO_BUS_I2C,
	RTIO_BUS_SPI,
	RTIO_BUS_I3C,
} rtio_bus_type;

/**
 * @brief check if bus is SPI
 * @param bus_type Type of bus (I2C, SPI, I3C)
 * @return true if bus type is SPI
 */
static inline bool rtio_is_spi(rtio_bus_type bus_type)
{
	return (bus_type == RTIO_BUS_SPI);
}

/**
 * @brief check if bus is I2C
 * @param bus_type Type of bus (I2C, SPI, I3C)
 * @return true if bus type is I2C
 */
static inline bool rtio_is_i2c(rtio_bus_type bus_type)
{
	return (bus_type == RTIO_BUS_I2C);
}

/**
 * @brief check if bus is I3C
 * @param bus_type Type of bus (I2C, SPI, I3C)
 * @return true if bus type is I3C
 */
static inline bool rtio_is_i3c(rtio_bus_type bus_type)
{
	return (bus_type == RTIO_BUS_I3C);
}

/*
 * @brief Create a chain of SQEs representing a bus transaction to read a reg.
 *
 * The RTIO-enabled bus driver is instrumented to perform bus read ops
 * for each register in the list.
 *
 * Usage:
 *
 * @code{.c}
 * struct rtio_regs regs;
 * struct rtio_reg_list regs_list[] = {{regs_addr1, mem_addr_1, mem_len_1},
 *                                     {regs_addr2, mem_addr_2, mem_len_2},
 *                                     ...
 *                                    };
 * regs.rtio_regs_list = regs_list;
 * regs.rtio_regs_num = ARRAY_SIZE(regs_list);
 *
 * rtio_read_regs_async(rtio,
 *                      iodev,
 *                      RTIO_BUS_SPI,
 *                      &regs,
 *                      sqe,
 *                      dev,
 *                      op_cb);
 * @endcode
 *
 * @param r RTIO context
 * @param iodev IO device
 * @param bus_type Type of bus (I2C, SPI, I3C)
 * @param regs pointer to list of registers to be read. Raise proper bit in case of SPI bus
 * @param iodev_sqe IODEV submission for the await op
 * @param dev pointer to the device structure
 * @param complete_op_cb callback routine at the end of op
 */
static inline void rtio_read_regs_async(struct rtio *r, struct rtio_iodev *iodev,
					rtio_bus_type bus_type, struct rtio_regs *regs,
					struct rtio_iodev_sqe *iodev_sqe, const struct device *dev,
					rtio_callback_t complete_op_cb)
{
	struct rtio_sqe *write_addr;
	struct rtio_sqe *read_reg;
	struct rtio_sqe *complete_op;

	for (uint8_t i = 0; i < regs->rtio_regs_num; i++) {

		write_addr = rtio_sqe_acquire(r);
		read_reg = rtio_sqe_acquire(r);

		if (write_addr == NULL || read_reg == NULL) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			rtio_sqe_drop_all(r);
			return;
		}

		rtio_sqe_prep_tiny_write(write_addr, iodev, RTIO_PRIO_NORM,
					 &regs->rtio_regs_list[i].reg_addr, 1, NULL);
		write_addr->flags = RTIO_SQE_TRANSACTION;

		rtio_sqe_prep_read(read_reg, iodev, RTIO_PRIO_NORM, regs->rtio_regs_list[i].bufp,
				   regs->rtio_regs_list[i].len, NULL);
		read_reg->flags = RTIO_SQE_CHAINED;

		switch (bus_type) {
		case RTIO_BUS_I2C:
			read_reg->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
			break;
		case RTIO_BUS_I3C:
			read_reg->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
			break;
		case RTIO_BUS_SPI:
		default:
			break;
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
