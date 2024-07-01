/*
 * Copyright (c) 2024 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>

/** Used to validate/control test execution flow */
K_SEM_DEFINE(work_handler_sem_1, 0, 1);
K_SEM_DEFINE(work_handler_sem_2, 0, 1);
K_SEM_DEFINE(work_handler_sem_3, 0, 1);
static int work_handler_called;

static void work_handler(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_sqe *sqe = &iodev_sqe->sqe;
	struct k_sem *sem = (struct k_sem *)sqe->userdata;

	work_handler_called++;
	printk("\t- %s() called!: %d\n", __func__, work_handler_called);

	k_sem_take(sem, K_FOREVER);

	rtio_executor_ok(iodev_sqe, 0);
}

static void dummy_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	rtio_work_req_submit(req, iodev_sqe, work_handler);
}

struct rtio_iodev_api r_iodev_test_api = {
	.submit = dummy_submit,
};

RTIO_IODEV_DEFINE(dummy_iodev, &r_iodev_test_api, NULL);
RTIO_IODEV_DEFINE(dummy_iodev_2, &r_iodev_test_api, NULL);
RTIO_IODEV_DEFINE(dummy_iodev_3, &r_iodev_test_api, NULL);

RTIO_DEFINE(r_test, 3, 3);
RTIO_DEFINE(r_test_2, 3, 3);
RTIO_DEFINE(r_test_3, 3, 3);

static void before(void *unused)
{
	rtio_sqe_drop_all(&r_test);
	rtio_sqe_drop_all(&r_test_2);
	rtio_sqe_drop_all(&r_test_3);

	k_sem_init(&work_handler_sem_1, 0, 1);
	k_sem_init(&work_handler_sem_2, 0, 1);
	k_sem_init(&work_handler_sem_3, 0, 1);

	work_handler_called = 0;
}

static void after(void *unused)
{
}

ZTEST_SUITE(rtio_work, NULL, NULL, before, after, NULL);

ZTEST(rtio_work, test_work_decouples_submission)
{
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;

	sqe = rtio_sqe_acquire(&r_test);
	rtio_sqe_prep_nop(sqe, &dummy_iodev, &work_handler_sem_1);
	sqe->prio = RTIO_PRIO_NORM;

	zassert_equal(0, work_handler_called);
	zassert_equal(0, rtio_work_req_used_count_get());

	zassert_ok(rtio_submit(&r_test, 0));

	zassert_equal(1, work_handler_called);
	zassert_equal(1, rtio_work_req_used_count_get());

	k_sem_give(&work_handler_sem_1);
	zassert_equal(0, rtio_work_req_used_count_get());

	/** Clean-up */
	cqe = rtio_cqe_consume_block(&r_test);
	rtio_cqe_release(&r_test, cqe);
}

ZTEST(rtio_work, test_work_supports_batching_submissions)
{
	struct rtio_sqe *sqe_a;
	struct rtio_sqe *sqe_b;
	struct rtio_sqe *sqe_c;
	struct rtio_cqe *cqe;

	sqe_a = rtio_sqe_acquire(&r_test);
	rtio_sqe_prep_nop(sqe_a, &dummy_iodev, &work_handler_sem_1);
	sqe_a->prio = RTIO_PRIO_NORM;

	sqe_b = rtio_sqe_acquire(&r_test);
	rtio_sqe_prep_nop(sqe_b, &dummy_iodev, &work_handler_sem_2);
	sqe_b->prio = RTIO_PRIO_NORM;

	sqe_c = rtio_sqe_acquire(&r_test);
	rtio_sqe_prep_nop(sqe_c, &dummy_iodev, &work_handler_sem_3);
	sqe_c->prio = RTIO_PRIO_NORM;

	zassert_ok(rtio_submit(&r_test, 0));

	k_sem_give(&work_handler_sem_1);
	k_sem_give(&work_handler_sem_2);
	k_sem_give(&work_handler_sem_3);

	zassert_equal(3, work_handler_called);
	zassert_equal(0, rtio_work_req_used_count_get());

	/** Clean-up */
	cqe = rtio_cqe_consume_block(&r_test);
	rtio_cqe_release(&r_test, cqe);
	cqe = rtio_cqe_consume_block(&r_test);
	rtio_cqe_release(&r_test, cqe);
	cqe = rtio_cqe_consume_block(&r_test);
	rtio_cqe_release(&r_test, cqe);
}

ZTEST(rtio_work, test_work_supports_preempting_on_higher_prio_submissions)
{
	struct rtio_sqe *sqe_a;
	struct rtio_sqe *sqe_b;
	struct rtio_sqe *sqe_c;
	struct rtio_cqe *cqe;

	sqe_a = rtio_sqe_acquire(&r_test);
	rtio_sqe_prep_nop(sqe_a, &dummy_iodev, &work_handler_sem_1);
	sqe_a->prio = RTIO_PRIO_LOW;

	sqe_b = rtio_sqe_acquire(&r_test_2);
	rtio_sqe_prep_nop(sqe_b, &dummy_iodev_2, &work_handler_sem_2);
	sqe_b->prio = RTIO_PRIO_NORM;

	sqe_c = rtio_sqe_acquire(&r_test_3);
	rtio_sqe_prep_nop(sqe_c, &dummy_iodev_3, &work_handler_sem_3);
	sqe_c->prio = RTIO_PRIO_HIGH;

	zassert_ok(rtio_submit(&r_test, 0));
	zassert_ok(rtio_submit(&r_test_2, 0));
	zassert_ok(rtio_submit(&r_test_3, 0));

	zassert_equal(3, work_handler_called);
	zassert_equal(3, rtio_work_req_used_count_get());

	k_sem_give(&work_handler_sem_1);
	k_sem_give(&work_handler_sem_2);
	k_sem_give(&work_handler_sem_3);

	zassert_equal(3, work_handler_called);
	zassert_equal(0, rtio_work_req_used_count_get());

	/** Clean-up */
	cqe = rtio_cqe_consume_block(&r_test);
	rtio_cqe_release(&r_test, cqe);
	cqe = rtio_cqe_consume_block(&r_test_2);
	rtio_cqe_release(&r_test_2, cqe);
	cqe = rtio_cqe_consume_block(&r_test_3);
	rtio_cqe_release(&r_test_3, cqe);
}

ZTEST(rtio_work, test_used_count_keeps_track_of_alloc_items)
{
	struct rtio_work_req *req_a = NULL;
	struct rtio_work_req *req_b = NULL;
	struct rtio_work_req *req_c = NULL;
	struct rtio_work_req *req_d = NULL;
	struct rtio_work_req *req_e = NULL;

	zassert_equal(0, rtio_work_req_used_count_get());

	/** We expect valid items and the count kept track */
	req_a = rtio_work_req_alloc();
	zassert_not_null(req_a);
	zassert_equal(1, rtio_work_req_used_count_get());

	req_b = rtio_work_req_alloc();
	zassert_not_null(req_b);
	zassert_equal(2, rtio_work_req_used_count_get());

	req_c = rtio_work_req_alloc();
	zassert_not_null(req_c);
	zassert_equal(3, rtio_work_req_used_count_get());

	req_d = rtio_work_req_alloc();
	zassert_not_null(req_d);
	zassert_equal(4, rtio_work_req_used_count_get());

	/** This time should not have been able to allocate. */
	req_e = rtio_work_req_alloc();
	zassert_is_null(req_e);
	zassert_equal(4, rtio_work_req_used_count_get());

	/** Flush requests */
	rtio_work_req_submit(req_a, NULL, NULL);
	zassert_equal(3, rtio_work_req_used_count_get());

	rtio_work_req_submit(req_b, NULL, NULL);
	zassert_equal(2, rtio_work_req_used_count_get());

	rtio_work_req_submit(req_c, NULL, NULL);
	zassert_equal(1, rtio_work_req_used_count_get());

	rtio_work_req_submit(req_d, NULL, NULL);
	zassert_equal(0, rtio_work_req_used_count_get());
}
