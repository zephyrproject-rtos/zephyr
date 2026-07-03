/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I3C_RTIO_H_
#define ZEPHYR_DRIVERS_I3C_RTIO_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_I3C_CALLBACK
/**
 * @brief Per-callback closure used by the *_cb async helpers.
 *
 * Allocated from the i3c_rtio context's pool when a *_cb call submits work
 * and freed by the trampoline once the user callback has been invoked.
 */
struct i3c_rtio;
struct i3c_rtio_cb_slot {
	struct i3c_rtio *ctx;
	i3c_callback_t cb;
	void *userdata;
	const struct device *dev;
};
#endif /* CONFIG_I3C_CALLBACK */

/**
 * @brief Driver context for implementing i3c with rtio
 */
struct i3c_rtio {
	struct k_sem lock;
	struct k_spinlock slock;
	struct rtio *r;
	struct mpsc io_q;
	struct rtio_iodev iodev;
	/**
	 * Backing storage for iodev.data so i3c_iodev_submit() can route
	 * sqes built via this ctx's iodev to the owning controller's
	 * iodev_submit. Initialized by i3c_rtio_init().
	 */
	struct i3c_iodev_data iodev_data;
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;
	struct i3c_device_desc *i3c_desc;

#ifdef CONFIG_I3C_CALLBACK
	/*
	 * Callback closure pool for the *_cb async helpers. Sized to
	 * CONFIG_I3C_RTIO_SQ_SIZE — you can't have more outstanding async
	 * transfers than the submission queue can hold. The free bitmap is
	 * managed lock-free via atomic_test_and_set_bit / atomic_clear_bit
	 * so allocation and release are safe from any context (including ISR).
	 */
	struct i3c_rtio_cb_slot cb_slots[CONFIG_I3C_RTIO_SQ_SIZE];
	ATOMIC_DEFINE(cb_slot_used, CONFIG_I3C_RTIO_SQ_SIZE);
#endif
};

/**
 * @brief Statically define an i3c_rtio context
 *
 * @param _name Symbolic name of the context
 * @param _sq_sz Submission queue entry pool size
 * @param _cq_sz Completion queue entry pool size
 */
#define I3C_RTIO_DEFINE(_name, _sq_sz, _cq_sz)		\
	RTIO_DEFINE(CONCAT(_name, _r), _sq_sz, _cq_sz);	\
	static struct i3c_rtio _name = {		\
		.r = &CONCAT(_name, _r),		\
	};

/**
 * @brief Copy an array of i3c_msgs to rtio submissions and a transaction
 *
 * Each sqe carries a pointer to its originating struct i3c_msg in the
 * sqe userdata. Drivers may write back msg->num_xfer and msg->err via this
 * pointer at completion, matching the legacy i3c_transfer() contract. The
 * msgs array must therefore remain valid until the transaction completes.
 *
 * @retval sqe Last sqe setup in the copy
 * @retval NULL Not enough memory to copy the transaction
 */
struct rtio_sqe *i3c_rtio_copy(struct rtio *r, struct rtio_iodev *iodev, struct i3c_msg *msgs,
			       uint8_t num_msgs);

/**
 * @brief Initialize an i3c rtio context
 *
 * @param ctx I3C RTIO driver context
 * @param dev I3C controller device that owns this context. Bound into
 *            ctx->iodev so i3c_iodev_submit() can dispatch sqes built
 *            via ctx->iodev back to this controller's iodev_submit.
 */
void i3c_rtio_init(struct i3c_rtio *ctx, const struct device *dev);

/**
 * @brief Signal that the current (ctx->txn_curr) submission has been completed
 *
 * @param ctx I3C RTIO driver context
 * @param status Completion status, negative values are errors
 *
 * @retval true Next submission is ready to start
 * @retval false No more submissions to work on
 */
bool i3c_rtio_complete(struct i3c_rtio *ctx, int status);

/**
 * @brief Submit, atomically, a submission to work on at some point
 *
 * @retval true Next submission is ready to start
 * @retval false No new submission to start or submissions are in progress already
 */
bool i3c_rtio_submit(struct i3c_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Configure the I3C bus controller
 *
 * Provides a compatible API for the existing i3c_configure API, and blocks the
 * caller until the transfer completes.
 *
 * See i3c_configure().
 */
int i3c_rtio_configure(struct i3c_rtio *ctx, enum i3c_config_type type, void *config);

/**
 * @brief Transfer i3c messages in a blocking call
 *
 * Provides a compatible API for the existing i3c_transfer API, and blocks the caller
 * until the transfer completes.
 *
 * See i3c_transfer().
 */
int i3c_rtio_transfer(struct i3c_rtio *ctx, struct i3c_msg *msgs, uint8_t num_msgs,
			       struct i3c_device_desc *desc);

/**
 * @brief Perform an I3C bus recovery in a blocking call
 *
 * Provides a compatible API for the existing i3c_recover API, and blocks the caller
 * until the process completes.
 *
 * See i3c_recover().
 */
int i3c_rtio_recover(struct i3c_rtio *ctx);

/**
 * @brief Perform an I3C CCC in a blocking call
 *
 * Provides a compatible API for the existing i3c_do_ccc API, and blocks the caller
 * until the process completes.
 *
 * See i3c_do_ccc().
 */
int i3c_rtio_ccc(struct i3c_rtio *ctx, struct i3c_ccc_payload *payload);

#ifdef CONFIG_I3C_CALLBACK
/**
 * @brief Transfer i3c messages asynchronously, invoking @p cb on completion.
 *
 * Non-blocking analog of i3c_rtio_transfer(). The user callback is invoked
 * once the entire transaction completes; the result int passed to @p cb is
 * the same value i3c_rtio_transfer() would return synchronously.
 *
 * The @p msgs array (and the buffers it references) must remain valid until
 * @p cb fires. Each msg's @c num_xfer and @c err fields may be updated by
 * the driver before the callback, matching the i3c_transfer_cb() contract.
 * @p cb must be non-NULL.
 *
 * @retval 0 on successful submission (the transfer is in flight)
 * @retval -ENOMEM if no submission slots or callback closure slots are free
 */
int i3c_rtio_transfer_cb(struct i3c_rtio *ctx, const struct device *dev,
			 struct i3c_msg *msgs, uint8_t num_msgs,
			 struct i3c_device_desc *desc, i3c_callback_t cb,
			 void *userdata);

/**
 * @brief Send a CCC asynchronously, invoking @p cb on completion.
 *
 * Non-blocking analog of i3c_rtio_ccc(). See i3c_rtio_transfer_cb() for the
 * callback invocation contract.
 */
int i3c_rtio_ccc_cb(struct i3c_rtio *ctx, const struct device *dev,
		    struct i3c_ccc_payload *payload, i3c_callback_t cb,
		    void *userdata);
#endif /* CONFIG_I3C_CALLBACK */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRVIERS_I3C_RTIO_H_ */
