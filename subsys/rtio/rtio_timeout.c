/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTIO timeout iodev backing the RTIO_OP_DELAY operation.
 *
 * Delay submissions are handled like any other iodev operation rather than as
 * an executor special case. The "device" here is the system clock: pending
 * delays are kept in a per-instance list sorted by absolute expiration and
 * driven by a single kernel timeout re-armed to the nearest deadline. This
 * keeps the per-SQE footprint to a single k_timepoint_t (no embedded
 * struct _timeout) and reduces the kernel timeout queue to one node per
 * timeout iodev instance.
 */

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

/** Required to access the Timeout Queue APIs, which are used instead of the
 * Timer APIs because of concerns on size: a shared struct _timeout is cheaper
 * than a k_timer, and RTIO applications are sensitive to memory footprint.
 */
#include <../kernel/include/timeout_q.h>

/**
 * @brief Per-instance state for a timeout iodev
 *
 * The sorted list link reuses each iodev_sqe's embedded mpsc_node (@ref
 * rtio_iodev_sqe.q), which is free while the submission is owned by this iodev
 * (it is out of the context submission queue and not yet recycled to the pool).
 */
struct rtio_timeout_iodev_data {
	struct k_spinlock lock;
	struct _timeout timeout;     /* single kernel timeout shared by all pending delays */
	struct rtio_iodev_sqe *head; /* soonest-first sorted list of pending delays */
};

/* Reuse the iodev_sqe's mpsc_node storage as the sorted-list link. */
static inline struct rtio_iodev_sqe *rtio_tq_next(struct rtio_iodev_sqe *iodev_sqe)
{
	struct mpsc_node *node = mpsc_ptr_get(iodev_sqe->q.next);

	return node == NULL ? NULL : CONTAINER_OF(node, struct rtio_iodev_sqe, q);
}

static inline void rtio_tq_set_next(struct rtio_iodev_sqe *iodev_sqe, struct rtio_iodev_sqe *next)
{
	mpsc_ptr_set(iodev_sqe->q.next, next == NULL ? NULL : &next->q);
}

/**
 * @brief Insert a submission into the sorted list
 *
 * @retval true if the submission became the new head (nearest deadline)
 */
static bool rtio_tq_insert(struct rtio_timeout_iodev_data *data, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev_sqe *prev = NULL;
	struct rtio_iodev_sqe *curr = data->head;

	while (curr != NULL &&
	       sys_timepoint_cmp(curr->sqe.delay.expiry, iodev_sqe->sqe.delay.expiry) <= 0) {
		prev = curr;
		curr = rtio_tq_next(curr);
	}

	rtio_tq_set_next(iodev_sqe, curr);

	if (prev == NULL) {
		data->head = iodev_sqe;
		return true;
	}

	rtio_tq_set_next(prev, iodev_sqe);
	return false;
}

/**
 * @brief Detach all expired submissions from the head into a NULL-terminated batch
 */
static struct rtio_iodev_sqe *rtio_tq_pop_expired(struct rtio_timeout_iodev_data *data)
{
	struct rtio_iodev_sqe *batch = NULL;
	struct rtio_iodev_sqe *last = NULL;

	while (data->head != NULL && sys_timepoint_expired(data->head->sqe.delay.expiry)) {
		struct rtio_iodev_sqe *iodev_sqe = data->head;

		data->head = rtio_tq_next(iodev_sqe);
		rtio_tq_set_next(iodev_sqe, NULL);

		if (last == NULL) {
			batch = iodev_sqe;
		} else {
			rtio_tq_set_next(last, iodev_sqe);
		}
		last = iodev_sqe;
	}

	return batch;
}

static void rtio_tq_expired(struct _timeout *timeout);

/**
 * @brief Arm the shared kernel timeout to the current head, or disarm if empty
 *
 * Must be called with @p data->lock held. On SMP, aborting the timeout may fail
 * with -EAGAIN if the handler is in flight on another CPU; the only correct
 * response is to drop the lock and retry, which is done by the caller.
 *
 * @retval true if the arm completed
 * @retval false if the timeout abort needs the caller to drop the lock and retry
 */
static bool rtio_tq_try_arm(struct rtio_timeout_iodev_data *data)
{
	if (z_try_abort_timeout(&data->timeout) == -EAGAIN) {
		return false;
	}

	if (data->head != NULL) {
		z_add_timeout(&data->timeout, rtio_tq_expired,
			      sys_timepoint_timeout(data->head->sqe.delay.expiry));
	}

	return true;
}

/**
 * @brief Kernel timeout handler for the nearest pending delay
 *
 * Runs in the system clock announce context, exactly as the previous per-SQE
 * alarm did. Expired submissions are detached and the timeout re-armed while
 * holding the lock; completions run with the lock released because completing a
 * submission may re-enter rtio_timeout_iodev_submit() (e.g. a chained delay).
 */
static void rtio_tq_expired(struct _timeout *timeout)
{
	struct rtio_timeout_iodev_data *data =
		CONTAINER_OF(timeout, struct rtio_timeout_iodev_data, timeout);
	struct rtio_iodev_sqe *batch;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	batch = rtio_tq_pop_expired(data);

	if (data->head != NULL) {
		z_add_timeout(&data->timeout, rtio_tq_expired,
			      sys_timepoint_timeout(data->head->sqe.delay.expiry));
	}

	k_spin_unlock(&data->lock, key);

	while (batch != NULL) {
		struct rtio_iodev_sqe *iodev_sqe = batch;

		batch = rtio_tq_next(iodev_sqe);
		rtio_tq_set_next(iodev_sqe, NULL);
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void rtio_timeout_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_timeout_iodev_data *data = iodev_sqe->sqe.iodev->data;
	k_spinlock_key_t key;

	iodev_sqe->sqe.delay.expiry = sys_timepoint_calc(iodev_sqe->sqe.delay.timeout);

	key = k_spin_lock(&data->lock);

	if (rtio_tq_insert(data, iodev_sqe)) {
		/* New nearest deadline: re-arm, dropping the lock to retry if the
		 * handler is in flight on another CPU (SMP).
		 */
		while (!rtio_tq_try_arm(data)) {
			k_spin_unlock(&data->lock, key);
			key = k_spin_lock(&data->lock);
		}
	}

	k_spin_unlock(&data->lock, key);
}

static const struct rtio_iodev_api rtio_timeout_iodev_api = {
	.submit = rtio_timeout_iodev_submit,
};

static struct rtio_timeout_iodev_data rtio_timeout_iodev_data_inst;

RTIO_IODEV_DEFINE(rtio_timeout_iodev, &rtio_timeout_iodev_api, &rtio_timeout_iodev_data_inst);
