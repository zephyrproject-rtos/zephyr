/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2S_RTIO_H_
#define ZEPHYR_DRIVERS_I2S_RTIO_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>

/**
 * @brief Driver context for implementing I2S with RTIO
 */
struct i2s_rtio {
	struct k_spinlock lock;
	struct mpsc io_q;
	struct rtio_iodev_sqe *curr;
	struct mpsc prev_q;
#if CONFIG_I2S_RTIO_LEGACY
	struct rtio *r;
	struct k_sem r_lock;
	struct rtio_iodev iodev;
	struct i2s_iodev_data data;
	enum i2s_dir dir;
#endif
};

/**
 * @brief Statically define an i2s_rtio context
 *
 * @param _name Symbolic name of the context
 */
#define I2S_RTIO_DEFINE(_name)									\
	IF_ENABLED(										\
		CONFIG_I2S_RTIO_LEGACY,								\
		(										\
			RTIO_DEFINE(								\
				CONCAT(_name, _r),						\
				CONFIG_I2S_RTIO_LEGACY_MAX_BLOCK_COUNT,				\
				CONFIG_I2S_RTIO_LEGACY_MAX_BLOCK_COUNT				\
			);									\
		)										\
	)											\
												\
	static struct i2s_rtio _name = {							\
		IF_ENABLED(									\
			CONFIG_I2S_RTIO_LEGACY,							\
			(									\
				.r = &CONCAT(_name, _r),					\
			)									\
		)										\
	}

/**
 * @brief Initialize a I2S RTIO context
 *
 * @param ctx I2S RTIO driver context
 * @param dev I2S bus
 */
void i2s_rtio_init(struct i2s_rtio *ctx, const struct device *dev);

/**
 * @brief Signal that the next submission is ready to be queued up
 *
 * @details I2S hardware typically supports quing up buffers (submissions) while a submission is
 *          currently being executed. This function will "peek" the IO queue (io_q), and if a
 *          successive submission exists, set it as the current submission to be queued up in
 *          hardware, while the currently executing submission completes.
 *
 *          Once a submission completes, call @ref i2s_rtio_complete.
 *
 * @note A call to this function must be followed by a call to @ref i2s_rtio_complete once the
 *       submission completes.
 *
 * @note When peeking the IO queue, if the next submission does not belong to the same iodev as
 *       the currently executing submission, this function will return false as the stream must
 *       be reconfigured to match the new iodev.
 *
 * @param[in] ctx I2S RTIO driver context
 *
 * @retval true Next submission is ready to be queued up
 * @retval false Current submission is last submission of stream
 */
bool i2s_rtio_continue(struct i2s_rtio *ctx);

/**
 * @brief Signal that a previous submission has been completed
 *
 * @param[in] ctx I2S RTIO driver context
 * @param[in] status Completion status, negative values are errors
 *
 * @retval true Next submission is ready to start
 * @retval false No more submissions to work on
 */
bool i2s_rtio_complete(struct i2s_rtio *ctx, int status);

/**
 * @brief Submit, atomically, a submission to work on at some point
 *
 * @retval true Next submission is ready to start
 * @retval false No new submission to start or submissions are in progress already
 */
bool i2s_rtio_submit(struct i2s_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief I2S RTIO wrapper for @ref i2s_configure
 *
 * For details see @ref i2s_configure.
 */
int i2s_rtio_configure(struct i2s_rtio *ctx,
		       enum i2s_dir dir,
		       const struct i2s_config *cfg);

/**
* @brief I2S RTIO wrapper for @ref i2s_config_get
*
* For details see @ref i2s_config_get.
*/
const struct i2s_config *i2s_rtio_config_get(struct i2s_rtio *ctx,
					     enum i2s_dir dir);

/**
* @brief I2S RTIO wrapper for @ref i2s_read
*
* For details see @ref i2s_read.
*/
int i2s_rtio_read(struct i2s_rtio *ctx,
		  void **mem_block,
		  size_t *size);

/**
* @brief I2S RTIO wrapper for @ref i2s_write
*
* For details see @ref i2s_write.
*/
int i2s_rtio_write(struct i2s_rtio *ctx,
		   void *mem_block,
		   size_t size);

/**
* @brief I2S RTIO wrapper for @ref i2s_trigger
*
* For details see @ref i2s_trigger.
*/
int i2s_rtio_trigger(struct i2s_rtio *ctx,
		     enum i2s_dir dir,
		     enum i2s_trigger_cmd cmd);

#endif /* ZEPHYR_DRIVERS_I2S_RTIO_H_ */
