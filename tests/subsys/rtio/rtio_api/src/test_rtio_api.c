/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/kobject.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/app_memory/mem_domain.h>
#include <zephyr/sys/util_loops.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/timing/timing.h>
#include <zephyr/rtio/rtio.h>

#include "rtio_iodev_test.h"

/* Repeat tests to ensure they are repeatable */
#define TEST_REPEATS 4

#define MEM_BLK_COUNT 4
#define MEM_BLK_SIZE 16
#define MEM_BLK_ALIGN 4

#define SQE_POOL_SIZE 5
#define CQE_POOL_SIZE 5

/*
 * Purposefully double the block count and half the block size. This leaves the same size mempool,
 * but ensures that allocation is done in larger blocks because the tests assume a larger block
 * size.
 */
RTIO_DEFINE_WITH_MEMPOOL(r_simple, SQE_POOL_SIZE, CQE_POOL_SIZE, MEM_BLK_COUNT * 2,
			 MEM_BLK_SIZE / 2, MEM_BLK_ALIGN);

RTIO_IODEV_TEST_DEFINE(iodev_test_simple);

/**
 * @brief Test the basics of the RTIO API
 *
 * Ensures that we can setup an RTIO context, enqueue a request, and receive
 * a completion event.
 */
void test_rtio_simple_(struct rtio *r)
{
	int res;
	uintptr_t userdata[2] = {0, 1};
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;

	rtio_iodev_test_init(&iodev_test_simple);

	TC_PRINT("setting up single no-op\n");
	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, (struct rtio_iodev *)&iodev_test_simple, &userdata[0]);

	TC_PRINT("submit with wait\n");
	res = rtio_submit(r, 1);
	zassert_ok(res, "Should return ok from rtio_execute");

	cqe = rtio_cqe_consume(r);
	zassert_not_null(cqe, "Expected a valid cqe");
	zassert_ok(cqe->result, "Result should be ok");
	zassert_equal_ptr(cqe->userdata, &userdata[0], "Expected userdata back");
	rtio_cqe_release(r, cqe);
}

ZTEST(rtio_api, test_rtio_simple)
{
	TC_PRINT("rtio simple simple\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_simple_(&r_simple);
	}
}

ZTEST(rtio_api, test_rtio_no_response)
{
	int res;
	uintptr_t userdata[2] = {0, 1};
	struct rtio_sqe *sqe;
	struct rtio_cqe cqe;

	rtio_iodev_test_init(&iodev_test_simple);

	sqe = rtio_sqe_acquire(&r_simple);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, (struct rtio_iodev *)&iodev_test_simple, &userdata[0]);
	sqe->flags |= RTIO_SQE_NO_RESPONSE;

	res = rtio_submit(&r_simple, 0);
	zassert_ok(res, "Should return ok from rtio_execute");

	res = rtio_cqe_copy_out(&r_simple, &cqe, 1, K_MSEC(500));
	zassert_equal(0, res, "Expected no CQEs");
}

RTIO_DEFINE(r_chain, SQE_POOL_SIZE, CQE_POOL_SIZE);

RTIO_IODEV_TEST_DEFINE(iodev_test_chain0);
RTIO_IODEV_TEST_DEFINE(iodev_test_chain1);
struct rtio_iodev *iodev_test_chain[] = {&iodev_test_chain0, &iodev_test_chain1};

/**
 * @brief Test chained requests
 *
 * Ensures that we can setup an RTIO context, enqueue a chained requests,
 * and receive completion events in the correct order given the chained
 * flag and multiple devices where serialization isn't guaranteed.
 */
void test_rtio_chain_(struct rtio *r)
{
	int res;
	uint32_t userdata[4] = {0, 1, 2, 3};
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	uintptr_t cq_count = atomic_get(&r->cq_count);

	for (int i = 0; i < 4; i++) {
		sqe = rtio_sqe_acquire(r);
		zassert_not_null(sqe, "Expected a valid sqe");
		rtio_sqe_prep_nop(sqe, iodev_test_chain[i % 2],
				  &userdata[i]);
		sqe->flags |= RTIO_SQE_CHAINED;
		TC_PRINT("produce %d, sqe %p, userdata %d\n", i, sqe, userdata[i]);
	}

	/* Clear the last one */
	sqe->flags = 0;

	TC_PRINT("submitting\n");

	res = rtio_submit(r, 4);
	TC_PRINT("checking cq\n");
	zassert_ok(res, "Should return ok from rtio_execute");
	zassert_equal(atomic_get(&r->cq_count) - cq_count, 4, "Should have 4 pending completions");

	for (int i = 0; i < 4; i++) {
		cqe = rtio_cqe_consume(r);
		zassert_not_null(cqe, "Expected a valid cqe");
		TC_PRINT("consume %d, cqe %p, userdata %d\n", i, cqe, *(uint32_t *)cqe->userdata);
		zassert_ok(cqe->result, "Result should be ok");

		zassert_equal_ptr(cqe->userdata, &userdata[i], "Expected in order completions");
		rtio_cqe_release(r, cqe);
	}
}

ZTEST(rtio_api, test_rtio_chain)
{
	TC_PRINT("initializing iodev test devices\n");

	for (int i = 0; i < 2; i++) {
		rtio_iodev_test_init(iodev_test_chain[i]);
	}

	TC_PRINT("rtio chain simple\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_chain_(&r_chain);
	}
}

RTIO_DEFINE(r_multi_chain, SQE_POOL_SIZE, CQE_POOL_SIZE);

RTIO_IODEV_TEST_DEFINE(iodev_test_multi0);
RTIO_IODEV_TEST_DEFINE(iodev_test_multi1);
struct rtio_iodev *iodev_test_multi[] = {&iodev_test_multi0, &iodev_test_multi1};

/**
 * @brief Test multiple asynchronous chains against one iodev
 */
void test_rtio_multiple_chains_(struct rtio *r)
{
	int res;
	uintptr_t userdata[4] = {0, 1, 2, 3};
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe = NULL;

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			sqe = rtio_sqe_acquire(r);
			zassert_not_null(sqe, "Expected a valid sqe");
			rtio_sqe_prep_nop(sqe, iodev_test_multi[i],
					  (void *)userdata[i*2 + j]);
			if (j == 0) {
				sqe->flags |= RTIO_SQE_CHAINED;
			} else {
				sqe->flags |= 0;
			}
		}
	}

	TC_PRINT("calling submit from test case\n");
	res = rtio_submit(r, 0);
	zassert_ok(res, "Should return ok from rtio_execute");

	bool seen[4] = { 0 };

	TC_PRINT("waiting for 4 completions\n");
	for (int i = 0; i < 4; i++) {
		TC_PRINT("waiting on completion %d\n", i);

		cqe = rtio_cqe_consume(r);
		while (cqe == NULL) {
			k_sleep(K_MSEC(1));
			cqe = rtio_cqe_consume(r);
		}

		TC_PRINT("consumed cqe %p, result, %d, userdata %lu\n", cqe,
			 cqe->result, (uintptr_t)cqe->userdata);

		zassert_not_null(cqe, "Expected a valid cqe");
		zassert_ok(cqe->result, "Result should be ok");
		seen[(uintptr_t)cqe->userdata] = true;
		if (seen[1]) {
			zassert_true(seen[0], "Should see 0 before 1");
		}
		if (seen[3]) {
			zassert_true(seen[2], "Should see 2 before 3");
		}
		rtio_cqe_release(r, cqe);
	}
}

ZTEST(rtio_api, test_rtio_multiple_chains)
{
	for (int i = 0; i < 2; i++) {
		rtio_iodev_test_init(iodev_test_multi[i]);
	}

	TC_PRINT("rtio multiple chains\n");
	test_rtio_multiple_chains_(&r_multi_chain);
}

#ifdef CONFIG_USERSPACE
struct k_mem_domain rtio_domain;
#endif

RTIO_BMEM uint8_t syscall_bufs[4];

RTIO_DEFINE(r_syscall, SQE_POOL_SIZE, CQE_POOL_SIZE);
RTIO_IODEV_TEST_DEFINE(iodev_test_syscall);

ZTEST_USER(rtio_api, test_rtio_syscalls)
{
	int res;
	struct rtio_sqe sqe = {0};
	struct rtio_cqe cqe = {0};

	struct rtio *r = &r_syscall;

	for (int i = 0; i < 4; i++) {
		TC_PRINT("copying sqe in from stack\n");
		/* Not really legal from userspace! Ugh */
		rtio_sqe_prep_nop(&sqe, &iodev_test_syscall,
				  &syscall_bufs[i]);
		res = rtio_sqe_copy_in(r, &sqe, 1);
		zassert_equal(res, 0, "Expected success copying sqe");
	}

	TC_PRINT("submitting\n");
	res = rtio_submit(r, 4);

	for (int i = 0; i < 4; i++) {
		TC_PRINT("consume %d\n", i);
		res = rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER);
		zassert_equal(res, 1, "Expected success copying cqe");
		zassert_ok(cqe.result, "Result should be ok");
		zassert_equal_ptr(cqe.userdata, &syscall_bufs[i],
				  "Expected in order completions");
	}
}

RTIO_BMEM uint8_t mempool_data[MEM_BLK_SIZE];

static void test_rtio_simple_mempool_(struct rtio *r, int run_count)
{
	int res;
	struct rtio_sqe sqe = {0};
	struct rtio_cqe cqe = {0};

	for (int i = 0; i < MEM_BLK_SIZE; ++i) {
		mempool_data[i] = i + run_count;
	}

	TC_PRINT("setting up single mempool read %p\n", r);
	rtio_sqe_prep_read_with_pool(&sqe, (struct rtio_iodev *)&iodev_test_simple, 0,
				     mempool_data);
	TC_PRINT("Calling rtio_sqe_copy_in()\n");
	res = rtio_sqe_copy_in(r, &sqe, 1);
	zassert_ok(res);

	TC_PRINT("submit with wait\n");
	res = rtio_submit(r, 1);
	zassert_ok(res, "Should return ok from rtio_submit");

	TC_PRINT("Calling rtio_cqe_copy_out\n");
	res = rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER);
	zassert_equal(1, res);
	TC_PRINT("cqe result %d, userdata %p\n", cqe.result, cqe.userdata);
	zassert_ok(cqe.result, "Result should be ok");
	zassert_equal_ptr(cqe.userdata, mempool_data, "Expected userdata back");

	uint8_t *buffer = NULL;
	uint32_t buffer_len = 0;

	TC_PRINT("Calling rtio_cqe_get_mempool_buffer\n");
	zassert_ok(rtio_cqe_get_mempool_buffer(r, &cqe, &buffer, &buffer_len));

	zassert_not_null(buffer, "Expected an allocated mempool buffer");
	zassert_equal(buffer_len, MEM_BLK_SIZE);
	zassert_mem_equal(buffer, mempool_data, MEM_BLK_SIZE, "Data expected to be the same");
	TC_PRINT("Calling rtio_cqe_get_mempool_buffer\n");
	rtio_release_buffer(r, buffer, buffer_len);
}

ZTEST_USER(rtio_api, test_rtio_simple_mempool)
{
	for (int i = 0; i < TEST_REPEATS * 2; i++) {
		test_rtio_simple_mempool_(&r_simple, i);
	}
}

static void test_rtio_simple_cancel_(struct rtio *r)
{
	struct rtio_sqe sqe[SQE_POOL_SIZE];
	struct rtio_cqe cqe;
	struct rtio_sqe *handle;

	rtio_sqe_prep_nop(sqe, (struct rtio_iodev *)&iodev_test_simple, NULL);
	rtio_sqe_copy_in_get_handles(r, sqe, &handle, 1);
	rtio_sqe_cancel(handle);
	TC_PRINT("Submitting 1 to RTIO\n");
	rtio_submit(r, 0);

	/* Check that we don't get a CQE */
	zassert_equal(0, rtio_cqe_copy_out(r, &cqe, 1, K_MSEC(15)));

	/* Check that the SQE pool is empty by filling it all the way */
	for (int i = 0; i < SQE_POOL_SIZE; ++i) {
		rtio_sqe_prep_nop(&sqe[i], (struct rtio_iodev *)&iodev_test_simple, NULL);
	}
	zassert_ok(rtio_sqe_copy_in(r, sqe, SQE_POOL_SIZE));

	/* Since there's no good way to just reset the RTIO context, wait for the nops to finish */
	rtio_submit(r, SQE_POOL_SIZE);
	for (int i = 0; i < SQE_POOL_SIZE; ++i) {
		zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER));
	}
}

ZTEST_USER(rtio_api, test_rtio_simple_cancel)
{
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_simple_cancel_(&r_simple);
	}
}

static void test_rtio_chain_cancel_(struct rtio *r)
{
	struct rtio_sqe sqe[SQE_POOL_SIZE];
	struct rtio_cqe cqe;
	struct rtio_sqe *handle;

	/* Prepare the chain */
	rtio_sqe_prep_nop(&sqe[0], (struct rtio_iodev *)&iodev_test_simple, NULL);
	rtio_sqe_prep_nop(&sqe[1], (struct rtio_iodev *)&iodev_test_simple, NULL);
	sqe[0].flags |= RTIO_SQE_CHAINED;

	/* Copy the chain */
	rtio_sqe_copy_in_get_handles(r, sqe, &handle, 2);
	rtio_sqe_cancel(handle);
	k_msleep(20);
	rtio_submit(r, 0);

	/* Check that we don't get cancelled completion notifications */
	zassert_equal(0, rtio_cqe_copy_out(r, &cqe, 1, K_MSEC(15)));

	/* Check that the SQE pool is empty by filling it all the way */
	for (int i = 0; i < SQE_POOL_SIZE; ++i) {
		rtio_sqe_prep_nop(&sqe[i], (struct rtio_iodev *)&iodev_test_simple, NULL);
	}
	zassert_ok(rtio_sqe_copy_in(r, sqe, SQE_POOL_SIZE));

	/* Since there's no good way to just reset the RTIO context, wait for the nops to finish */
	rtio_submit(r, SQE_POOL_SIZE);
	for (int i = 0; i < SQE_POOL_SIZE; ++i) {
		zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER));
	}

	/* Try cancelling the middle sqe in a chain */
	rtio_sqe_prep_nop(&sqe[0], (struct rtio_iodev *)&iodev_test_simple, NULL);
	rtio_sqe_prep_nop(&sqe[1], (struct rtio_iodev *)&iodev_test_simple, NULL);
	rtio_sqe_prep_nop(&sqe[2], (struct rtio_iodev *)&iodev_test_simple, NULL);
	sqe[0].flags |= RTIO_SQE_CHAINED;
	sqe[1].flags |= RTIO_SQE_CHAINED | RTIO_SQE_CANCELED;

	/* Copy in the first non cancelled sqe */
	rtio_sqe_copy_in_get_handles(r, sqe, &handle, 3);
	rtio_submit(r, 1);

	/* Check that we get one completion no cancellation notifications */
	zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_MSEC(15)));

	/* Check that we get no more completions for the cancelled submissions */
	zassert_equal(0, rtio_cqe_copy_out(r, &cqe, 1, K_MSEC(15)));

	/* Check that the SQE pool is empty by filling it all the way */
	for (int i = 0; i < SQE_POOL_SIZE; ++i) {
		rtio_sqe_prep_nop(&sqe[i], (struct rtio_iodev *)&iodev_test_simple, NULL);
	}
	zassert_ok(rtio_sqe_copy_in(r, sqe, SQE_POOL_SIZE));

	/* Since there's no good way to just reset the RTIO context, wait for the nops to finish */
	rtio_submit(r, SQE_POOL_SIZE);
	for (int i = 0; i < SQE_POOL_SIZE; ++i) {
		zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER));
	}
}

ZTEST_USER(rtio_api, test_rtio_chain_cancel)
{
	TC_PRINT("start test\n");
	k_msleep(20);
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_chain_cancel_(&r_simple);
	}
}

static void test_rtio_transaction_cancel_(struct rtio *r)
{
	struct rtio_sqe sqe[SQE_POOL_SIZE];
	struct rtio_cqe cqe;
	struct rtio_sqe *handle;

	/* Prepare the chain */
	rtio_sqe_prep_nop(&sqe[0], (struct rtio_iodev *)&iodev_test_simple, NULL);
	rtio_sqe_prep_nop(&sqe[1], (struct rtio_iodev *)&iodev_test_simple, NULL);
	sqe[0].flags |= RTIO_SQE_TRANSACTION;

	/* Copy the chain */
	rtio_sqe_copy_in_get_handles(r, sqe, &handle, 2);
	rtio_sqe_cancel(handle);
	TC_PRINT("Submitting 2 to RTIO\n");
	rtio_submit(r, 0);

	/* Check that we don't get a CQE */
	zassert_equal(0, rtio_cqe_copy_out(r, &cqe, 1, K_MSEC(15)));

	/* Check that the SQE pool is empty by filling it all the way */
	for (int i = 0; i < SQE_POOL_SIZE; ++i) {
		rtio_sqe_prep_nop(&sqe[i], (struct rtio_iodev *)&iodev_test_simple, NULL);
	}
	zassert_ok(rtio_sqe_copy_in(r, sqe, SQE_POOL_SIZE));

	/* Since there's no good way to just reset the RTIO context, wait for the nops to finish */
	rtio_submit(r, SQE_POOL_SIZE);
	for (int i = 0; i < SQE_POOL_SIZE; ++i) {
		zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER));
	}
}

ZTEST_USER(rtio_api, test_rtio_transaction_cancel)
{
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_transaction_cancel_(&r_simple);
	}
}

static inline void test_rtio_simple_multishot_(struct rtio *r, int idx)
{
	int res;
	struct rtio_sqe sqe;
	struct rtio_cqe cqe;
	struct rtio_sqe *handle;

	for (int i = 0; i < MEM_BLK_SIZE; ++i) {
		mempool_data[i] = i + idx;
	}

	TC_PRINT("setting up single mempool read\n");
	rtio_sqe_prep_read_multishot(&sqe, (struct rtio_iodev *)&iodev_test_simple, 0,
				     mempool_data);
	TC_PRINT("Calling rtio_sqe_copy_in()\n");
	res = rtio_sqe_copy_in_get_handles(r, &sqe, &handle, 1);
	zassert_ok(res);

	TC_PRINT("submit with wait, handle=%p\n", handle);
	res = rtio_submit(r, 1);
	zassert_ok(res, "Should return ok from rtio_execute");

	TC_PRINT("Calling rtio_cqe_copy_out\n");
	zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER));
	zassert_ok(cqe.result, "Result should be ok but got %d", cqe.result);
	zassert_equal_ptr(cqe.userdata, mempool_data, "Expected userdata back");

	uint8_t *buffer = NULL;
	uint32_t buffer_len = 0;

	TC_PRINT("Calling rtio_cqe_get_mempool_buffer\n");
	zassert_ok(rtio_cqe_get_mempool_buffer(r, &cqe, &buffer, &buffer_len));

	zassert_not_null(buffer, "Expected an allocated mempool buffer");
	zassert_equal(buffer_len, MEM_BLK_SIZE);
	zassert_mem_equal(buffer, mempool_data, MEM_BLK_SIZE, "Data expected to be the same");
	TC_PRINT("Calling rtio_release_buffer\n");
	rtio_release_buffer(r, buffer, buffer_len);

	TC_PRINT("Waiting for next cqe\n");
	zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER));
	zassert_ok(cqe.result, "Result should be ok but got %d", cqe.result);
	zassert_equal_ptr(cqe.userdata, mempool_data, "Expected userdata back");
	rtio_cqe_get_mempool_buffer(r, &cqe, &buffer, &buffer_len);
	rtio_release_buffer(r, buffer, buffer_len);

	TC_PRINT("Canceling %p\n", handle);
	rtio_sqe_cancel(handle);
	/* Flush any pending CQEs */
	while (rtio_cqe_copy_out(r, &cqe, 1, K_MSEC(15)) != 0) {
		rtio_cqe_get_mempool_buffer(r, &cqe, &buffer, &buffer_len);
		rtio_release_buffer(r, buffer, buffer_len);
	}
}

ZTEST_USER(rtio_api, test_rtio_multishot)
{
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_simple_multishot_(&r_simple, i);
	}
}

RTIO_DEFINE(r_transaction, SQE_POOL_SIZE, CQE_POOL_SIZE);

RTIO_IODEV_TEST_DEFINE(iodev_test_transaction0);
RTIO_IODEV_TEST_DEFINE(iodev_test_transaction1);
struct rtio_iodev *iodev_test_transaction[] = {&iodev_test_transaction0, &iodev_test_transaction1};

/**
 * @brief Test transaction requests
 *
 * Ensures that we can setup an RTIO context, enqueue a transaction requests,
 * and receive completion events in the correct order given the transaction
 * flag and multiple devices where serialization isn't guaranteed.
 */
void test_rtio_transaction_(struct rtio *r)
{
	int res;
	uintptr_t userdata[2] = {0, 1};
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	bool seen[2] = { 0 };
	uintptr_t cq_count = atomic_get(&r->cq_count);

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, &iodev_test_transaction0, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, NULL, &userdata[0]);

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, &iodev_test_transaction1, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, NULL,
			  &userdata[1]);

	TC_PRINT("submitting userdata 0 %p, userdata 1 %p\n", &userdata[0], &userdata[1]);
	res = rtio_submit(r, 4);
	TC_PRINT("checking cq, completions available, count at start %lu, current count %lu\n",
		 cq_count, atomic_get(&r->cq_count));
	zassert_ok(res, "Should return ok from rtio_execute");
	zassert_equal(atomic_get(&r->cq_count) - cq_count, 4, "Should have 4 pending completions");

	for (int i = 0; i < 4; i++) {
		TC_PRINT("consume %d\n", i);
		cqe = rtio_cqe_consume(r);
		zassert_not_null(cqe, "Expected a valid cqe");
		zassert_ok(cqe->result, "Result should be ok");
		if (i % 2 == 0) {
			zassert_is_null(cqe->userdata);
			rtio_cqe_release(r, cqe);
			continue;
		}
		uintptr_t idx = *(uintptr_t *)cqe->userdata;

		TC_PRINT("userdata is %p, value %" PRIuPTR "\n", cqe->userdata, idx);
		zassert(idx == 0 || idx == 1, "idx should be 0 or 1");
		seen[idx] = true;
		rtio_cqe_release(r, cqe);
	}

	zassert_true(seen[0], "Should have seen transaction 0");
	zassert_true(seen[1], "Should have seen transaction 1");
}

ZTEST(rtio_api, test_rtio_transaction)
{
	TC_PRINT("initializing iodev test devices\n");

	for (int i = 0; i < 2; i++) {
		rtio_iodev_test_init(iodev_test_transaction[i]);
	}

	TC_PRINT("rtio transaction simple\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_transaction_(&r_transaction);
	}
}

ZTEST(rtio_api, test_rtio_cqe_count_overflow)
{
	/* atomic_t max value as `uintptr_t` */
	const atomic_t max_uval = UINTPTR_MAX;

	/* atomic_t max value as if it were a signed word `intptr_t` */
	const atomic_t max_sval = UINTPTR_MAX >> 1;

	TC_PRINT("initializing iodev test devices\n");

	for (int i = 0; i < 2; i++) {
		rtio_iodev_test_init(iodev_test_transaction[i]);
	}

	TC_PRINT("rtio transaction CQE overflow\n");
	atomic_set(&r_transaction.cq_count, max_uval - 3);
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_transaction_(&r_transaction);
	}

	TC_PRINT("initializing iodev test devices\n");

	for (int i = 0; i < 2; i++) {
		rtio_iodev_test_init(iodev_test_transaction[i]);
	}

	TC_PRINT("rtio transaction CQE overflow\n");
	atomic_set(&r_transaction.cq_count, max_sval - 3);
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_transaction_(&r_transaction);
	}
}


#define THROUGHPUT_ITERS 100000
RTIO_DEFINE(r_throughput, SQE_POOL_SIZE, CQE_POOL_SIZE);

void _test_rtio_throughput(struct rtio *r)
{
	timing_t start_time, end_time;
	struct rtio_cqe *cqe;
	struct rtio_sqe *sqe;

	timing_init();
	timing_start();

	start_time = timing_counter_get();

	for (uint32_t i = 0; i < THROUGHPUT_ITERS; i++) {
		sqe = rtio_sqe_acquire(r);
		rtio_sqe_prep_nop(sqe, NULL, NULL);
		rtio_submit(r, 0);
		cqe = rtio_cqe_consume(r);
		rtio_cqe_release(r, cqe);
	}

	end_time = timing_counter_get();

	uint64_t cycles = timing_cycles_get(&start_time, &end_time);
	uint64_t ns = timing_cycles_to_ns(cycles);

	TC_PRINT("%llu ns for %d iterations, %llu ns per op\n",
		 ns, THROUGHPUT_ITERS, ns/THROUGHPUT_ITERS);
}


ZTEST(rtio_api, test_rtio_throughput)
{
	_test_rtio_throughput(&r_throughput);
}

RTIO_DEFINE(r_callback_chaining, SQE_POOL_SIZE, CQE_POOL_SIZE);
RTIO_IODEV_TEST_DEFINE(iodev_test_callback_chaining0);
static bool cb_no_cqe_run;

/**
 * Callback for testing with
 */
void rtio_callback_chaining_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg0)
{
	TC_PRINT("chaining callback with userdata %p\n", arg0);
}

void rtio_callback_chaining_cb_no_cqe(struct rtio *r, const struct rtio_sqe *sqe, void *arg0)
{
	TC_PRINT("Chaining callback with userdata %p (No CQE)\n", arg0);
	cb_no_cqe_run = true;
}

/**
 * @brief Test callback chaining requests
 *
 * Ensures that we can setup an RTIO context, enqueue a transaction of requests,
 * receive completion events, and catch a callback at the end  in the correct
 * order
 */
void test_rtio_callback_chaining_(struct rtio *r)
{
	int res;
	int32_t userdata[4] = {0, 1, 2, 3};
	int32_t ordering[4] = { -1, -1, -1, -1};
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	uintptr_t cq_count = atomic_get(&r->cq_count);

	rtio_iodev_test_init(&iodev_test_callback_chaining0);

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_callback(sqe, &rtio_callback_chaining_cb, sqe, &userdata[0]);
	sqe->flags |= RTIO_SQE_CHAINED;

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, &iodev_test_callback_chaining0, &userdata[1]);
	sqe->flags |= RTIO_SQE_TRANSACTION;

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, &iodev_test_callback_chaining0, &userdata[2]);
	sqe->flags |= RTIO_SQE_CHAINED;

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_callback_no_cqe(sqe, &rtio_callback_chaining_cb_no_cqe, sqe, NULL);
	sqe->flags |= RTIO_SQE_CHAINED;

	sqe = rtio_sqe_acquire(r);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_callback(sqe, &rtio_callback_chaining_cb, sqe, &userdata[3]);

	TC_PRINT("submitting\n");
	res = rtio_submit(r, 4);
	TC_PRINT("checking cq, completions available, count at start %lu, current count %lu\n",
		 cq_count, atomic_get(&r->cq_count));
	zassert_ok(res, "Should return ok from rtio_execute");
	zassert_equal(atomic_get(&r->cq_count) - cq_count, 4, "Should have 4 pending completions");
	zassert_true(cb_no_cqe_run, "Callback without CQE should have run");

	for (int i = 0; i < 4; i++) {
		TC_PRINT("consume %d\n", i);
		cqe = rtio_cqe_consume(r);
		zassert_not_null(cqe, "Expected a valid cqe");
		zassert_ok(cqe->result, "Result should be ok");

		int32_t idx = *(int32_t *)cqe->userdata;

		TC_PRINT("userdata is %p, value %d\n", cqe->userdata, idx);
		ordering[idx] = i;

		rtio_cqe_release(r, cqe);
	}

	for (int i = 0; i < 4; i++) {
		zassert_equal(ordering[i], i,
			      "Execpted ordering of completions to match submissions");
	}
}

ZTEST(rtio_api, test_rtio_callback_chaining)
{
	test_rtio_callback_chaining_(&r_callback_chaining);
}

static void *rtio_api_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_mem_domain_init(&rtio_domain, 0, NULL);
	k_mem_domain_add_partition(&rtio_domain, &rtio_partition);
#if Z_LIBC_PARTITION_EXISTS
	k_mem_domain_add_partition(&rtio_domain, &z_libc_partition);
#endif /* Z_LIBC_PARTITION_EXISTS */
#endif /* CONFIG_USERSPACE */

	return NULL;
}

static void rtio_api_before(void *a)
{
	ARG_UNUSED(a);

	STRUCT_SECTION_FOREACH(rtio, r)
	{
		struct rtio_cqe cqe;

		while (rtio_cqe_copy_out(r, &cqe, 1, K_MSEC(15))) {
		}
	}

	rtio_iodev_test_init(&iodev_test_simple);
	rtio_iodev_test_init(&iodev_test_syscall);
#ifdef CONFIG_USERSPACE
	k_mem_domain_add_thread(&rtio_domain, k_current_get());
	rtio_access_grant(&r_simple, k_current_get());
	rtio_access_grant(&r_syscall, k_current_get());
	k_object_access_grant(&iodev_test_simple, k_current_get());
	k_object_access_grant(&iodev_test_syscall, k_current_get());
#endif
}

ZTEST_SUITE(rtio_api, NULL, rtio_api_setup, rtio_api_before, NULL, NULL);
