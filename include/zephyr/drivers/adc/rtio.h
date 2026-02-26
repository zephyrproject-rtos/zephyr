/*
 * Copyright (c) 2024 Croxel, Inc.
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ADC_RTIO_H_
#define ZEPHYR_DRIVERS_ADC_RTIO_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driver context for implementing ADC with RTIO
 */
struct adc_rtio {
	struct k_spinlock spinlock;
	struct mpsc io_q;
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;
#if CONFIG_ADC_RTIO_WRAPPER
	struct rtio *r;
	struct rtio_iodev iodev;
	struct k_sem semlock;
	struct adc_dt_spec spec;
#endif /* CONFIG_ADC_RTIO_WRAPPER */
};

/** @cond INTERNAL_HIDDEN */

#define Z_ADC_RTIO_SQE_SIZE \
	COND_CODE_1(CONFIG_ADC_ASYNC, (2), (1))

#define Z_ADC_RTIO_CQE_SIZE 1

#define Z_ADC_RTIO_DEFINE(_name)								\
	RTIO_DEFINE(										\
		CONCAT(_name, _r),								\
		Z_ADC_RTIO_SQE_SIZE,								\
		Z_ADC_RTIO_CQE_SIZE								\
	)

#define Z_ADC_RTIO_GET(_name) \
	&CONCAT(_name, _r)

/** @endcond */

/**
 * @brief Statically define an adc_rtio context
 *
 * @param _name Symbolic name of the context
 */
#define ADC_RTIO_DEFINE(_name)									\
	IF_ENABLED(CONFIG_ADC_RTIO_WRAPPER, (Z_ADC_RTIO_DEFINE(_name);))			\
	static struct adc_rtio _name = {							\
		IF_ENABLED(CONFIG_ADC_RTIO_WRAPPER, (.r = Z_ADC_RTIO_GET(_name),))		\
	};

/**
 * @brief Initialize adc_rtio context
 *
 * @param[in] ctx adc_rtio context
 * @param[in] dev ADC device
 */
void adc_rtio_init(struct adc_rtio *ctx, const struct device *dev);

/**
 * @brief Signal that the current (ctx->txn_curr) submission has been completed
 *
 * @param[in] ctx ADC RTIO driver context
 * @param[in] status Completion status, negative values are errors
 *
 * @retval true Next submission is ready to start
 * @retval false No more submissions to work on
 */
bool adc_rtio_complete(struct adc_rtio *ctx, int status);

/**
 * @brief Submit, atomically, a submission to work on at some point
 *
 * @param[in] ctx ADC RTIO driver context
 * @param[in] iodev_sqe IODEV SQE to submit
 *
 * @retval true Next submission is ready to start
 * @retval false No new submission to start or submissions are in progress already
 */
bool adc_rtio_submit(struct adc_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Perform an ADC read in a blocking call using RTIO
 *
 * Provides a compatible API for the existing adc_read API by blocking
 * the caller until the operation is complete.
 *
 * For details see @ref adc_channel_setup.
 *
 * @param[in] ctx ADC RTIO driver context
 * @param[in] channel_cfg Channel configuration to set up
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int adc_rtio_channel_setup(struct adc_rtio *ctx, const struct adc_channel_cfg *channel_cfg);

/**
 * @brief Perform an ADC read in a blocking call using RTIO
 *
 * Provides a compatible API for the existing adc_read API by blocking
 * the caller until the operation is complete.
 *
 * For details see @ref adc_read.
 *
 * @param[in] ctx ADC RTIO driver context
 * @param[in] sequence Read sequence
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int adc_rtio_read(struct adc_rtio *ctx, const struct adc_sequence *sequence);

/**
 * @brief Perform an ADC read signalling once complete using RTIO
 *
 * Provides a compatible API for the existing adc_read_async API by signalling
 * the caller when the operation is complete.
 *
 * For details see @ref adc_read_async.
 *
 * @param[in] ctx ADC RTIO driver context
 * @param[in] sequence Read sequence
 * @param[in] async Poll signal to raise once complete
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
int adc_rtio_read_async(struct adc_rtio *ctx,
			const struct adc_sequence *sequence,
			struct k_poll_signal *async);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_ADC_RTIO_H_ */
