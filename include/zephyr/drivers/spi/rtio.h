/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_RTIO_H_
#define ZEPHYR_DRIVERS_SPI_RTIO_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driver context for implementing SPI with RTIO
 */
struct spi_rtio {
	struct k_spinlock lock;
	struct rtio *r;
	struct mpsc io_q;
	struct rtio_iodev iodev;
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;
	struct spi_dt_spec dt_spec;
};

/**
 * @brief Statically define a spi_rtio context
 *
 * @param _name Symbolic name of the context
 * @param _sq_sz Submission queue entry pool size
 * @param _cq_sz Completion queue entry pool size
 */
#define SPI_RTIO_DEFINE(_name, _sq_sz, _cq_sz)				       \
	RTIO_DEFINE(CONCAT(_name, _r), _sq_sz, _cq_sz);			       \
	static struct spi_rtio _name = {				       \
		.r = &CONCAT(_name, _r),				       \
	};

/**
 * @brief Copy the tx_bufs and rx_bufs into a set of RTIO requests
 *
 * @param[in] r rtio context
 * @param[in] iodev iodev to transceive with
 * @param[in] tx_bufs transmit buffer set
 * @param[in] rx_bufs receive buffer set
 * @param[out] last_sqe last sqe submitted, NULL if not enough memory
 *
 * @retval Number of submission queue entries
 * @retval -ENOMEM out of memory
 */
int spi_rtio_copy(struct rtio *r,
		  struct rtio_iodev *iodev,
		  const struct spi_buf_set *tx_bufs,
		  const struct spi_buf_set *rx_bufs,
		  struct rtio_sqe **last_sqe);

/**
 * @brief Initialize a SPI RTIO context
 *
 * @param ctx SPI RTIO driver context
 * @param dev SPI bus
 */
void spi_rtio_init(struct spi_rtio *ctx, const struct device *dev);

/**
 * @brief Signal that the current (ctx->txn_curr) submission has been completed
 *
 * @param ctx SPI RTIO driver context
 * @param status Completion status, negative values are errors
 *
 * @retval true Next submission is ready to start
 * @retval false No more submissions to work on
 */
bool spi_rtio_complete(struct spi_rtio *ctx, int status);

/**
 * @brief Submit, atomically, a submission to work on at some point
 *
 * @retval true Next submission is ready to start
 * @retval false No new submission to start or submissions are in progress already
 */
bool spi_rtio_submit(struct spi_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Perform a SPI Transfer (transceive) in a blocking call
 *
 * Provides a compatible API for the existing spi_transceive API by blocking
 * the caller until the operation is complete.
 * For details see @ref spi_transceive.
 */
int spi_rtio_transceive(struct spi_rtio *ctx,
			const struct spi_config *config,
			const struct spi_buf_set *tx_bufs,
			const struct spi_buf_set *rx_bufs);

/**
 * @brief Fallback SPI RTIO submit implementation.
 *
 * Default RTIO SPI implementation for drivers who do no yet have
 * native support. For details, see @ref spi_iodev_submit.
 */
void spi_rtio_iodev_default_submit(const struct device *dev,
				   struct rtio_iodev_sqe *iodev_sqe);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SPI_RTIO_H_ */
