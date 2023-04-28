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
#include <zephyr/rtio/rtio_spsc.h>
#include <zephyr/rtio/rtio_mpsc.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/rtio_executor_simple.h>
#include <zephyr/rtio/rtio_executor_concurrent.h>

#include "rtio_iodev_test.h"

/* Repeat tests to ensure they are repeatable */
#define TEST_REPEATS 4

#define MEM_BLK_COUNT 4
#define MEM_BLK_SIZE 16
#define MEM_BLK_ALIGN 4

RTIO_EXECUTOR_SIMPLE_DEFINE(simple_exec_simp);
/*
 * Purposefully double the block count and half the block size. This leaves the same size mempool,
 * but ensures that allocation is done in larger blocks because the tests assume a larger block
 * size.
 */
RTIO_DEFINE_WITH_MEMPOOL(r_simple_simp, (struct rtio_executor *)&simple_exec_simp, 4, 4,
			 MEM_BLK_COUNT * 2, MEM_BLK_SIZE / 2, MEM_BLK_ALIGN);

RTIO_EXECUTOR_CONCURRENT_DEFINE(simple_exec_con, 1);
RTIO_DEFINE_WITH_MEMPOOL(r_simple_con, (struct rtio_executor *)&simple_exec_con, 4, 4,
			 MEM_BLK_COUNT * 2, MEM_BLK_SIZE / 2, MEM_BLK_ALIGN);

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
	sqe = rtio_spsc_acquire(r->sq);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, (struct rtio_iodev *)&iodev_test_simple, &userdata[0]);

	TC_PRINT("submit with wait\n");
	res = rtio_submit(r, 1);
	zassert_ok(res, "Should return ok from rtio_execute");

	cqe = rtio_spsc_consume(r->cq);
	zassert_not_null(cqe, "Expected a valid cqe");
	zassert_ok(cqe->result, "Result should be ok");
	zassert_equal_ptr(cqe->userdata, &userdata[0], "Expected userdata back");
	rtio_spsc_release(r->cq);
}

ZTEST(rtio_api, test_rtio_simple)
{
	TC_PRINT("rtio simple simple\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_simple_(&r_simple_simp);
	}
	TC_PRINT("rtio simple concurrent\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_simple_(&r_simple_con);
	}
}

RTIO_EXECUTOR_SIMPLE_DEFINE(chain_exec_simp);
RTIO_DEFINE(r_chain_simp, (struct rtio_executor *)&chain_exec_simp, 4, 4);

RTIO_EXECUTOR_CONCURRENT_DEFINE(chain_exec_con, 1);
RTIO_DEFINE(r_chain_con, (struct rtio_executor *)&chain_exec_con, 4, 4);

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

	for (int i = 0; i < 4; i++) {
		sqe = rtio_spsc_acquire(r->sq);
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
	zassert_equal(rtio_spsc_consumable(r->cq), 4, "Should have 4 pending completions");

	for (int i = 0; i < 4; i++) {
		cqe = rtio_spsc_consume(r->cq);
		zassert_not_null(cqe, "Expected a valid cqe");
		TC_PRINT("consume %d, cqe %p, userdata %d\n", i, cqe, *(uint32_t *)cqe->userdata);
		zassert_ok(cqe->result, "Result should be ok");

		zassert_equal_ptr(cqe->userdata, &userdata[i], "Expected in order completions");
		rtio_spsc_release(r->cq);
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
		test_rtio_chain_(&r_chain_simp);
	}
	TC_PRINT("rtio chain concurrent\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_chain_(&r_chain_con);
	}
}

RTIO_EXECUTOR_SIMPLE_DEFINE(multi_exec_simp);
RTIO_DEFINE(r_multi_simp, (struct rtio_executor *)&multi_exec_simp, 4, 4);

RTIO_EXECUTOR_CONCURRENT_DEFINE(multi_exec_con, 2);
RTIO_DEFINE(r_multi_con, (struct rtio_executor *)&multi_exec_con, 4, 4);

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
	struct rtio_cqe *cqe;

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			sqe = rtio_spsc_acquire(r->sq);
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
		cqe = rtio_spsc_consume(r->cq);

		while (cqe == NULL) {
			k_sleep(K_MSEC(1));
			cqe = rtio_spsc_consume(r->cq);
		}

		zassert_not_null(cqe, "Expected a valid cqe");
		TC_PRINT("result %d, would block is %d, inval is %d\n",
			 cqe->result, -EWOULDBLOCK, -EINVAL);
		zassert_ok(cqe->result, "Result should be ok");
		seen[(uintptr_t)cqe->userdata] = true;
		if (seen[1]) {
			zassert_true(seen[0], "Should see 0 before 1");
		}
		if (seen[3]) {
			zassert_true(seen[2], "Should see 2 before 3");
		}
		rtio_spsc_release(r->cq);
	}
}

ZTEST(rtio_api, test_rtio_multiple_chains)
{
	for (int i = 0; i < 2; i++) {
		rtio_iodev_test_init(iodev_test_multi[i]);
	}

	TC_PRINT("rtio multiple simple\n");
	test_rtio_multiple_chains_(&r_multi_simp);
	TC_PRINT("rtio_multiple concurrent\n");
	test_rtio_multiple_chains_(&r_multi_con);
}

#ifdef CONFIG_USERSPACE
struct k_mem_domain rtio_domain;
#endif

RTIO_BMEM uint8_t syscall_bufs[4];

RTIO_EXECUTOR_SIMPLE_DEFINE(syscall_simple);
RTIO_DEFINE(r_syscall, (struct rtio_executor *)&syscall_simple, 4, 4);
RTIO_IODEV_TEST_DEFINE(iodev_test_syscall);

void rtio_syscall_test(void *p1, void *p2, void *p3)
{
	int res;
	struct rtio_sqe sqe;
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

#ifdef CONFIG_USERSPACE
ZTEST(rtio_api, test_rtio_syscalls_usermode)
{

	TC_PRINT("syscalls from user mode test\n");
	TC_PRINT("test iodev init\n");
	rtio_iodev_test_init(&iodev_test_syscall);
	TC_PRINT("mem domain add current\n");
	k_mem_domain_add_thread(&rtio_domain, k_current_get());
	TC_PRINT("rtio access grant\n");
	rtio_access_grant(&r_syscall, k_current_get());
	TC_PRINT("rtio iodev access grant, ptr %p\n", &iodev_test_syscall);
	k_object_access_grant(&iodev_test_syscall, k_current_get());
	TC_PRINT("user mode enter\n");
	k_thread_user_mode_enter(rtio_syscall_test, NULL, NULL, NULL);
}
#endif /* CONFIG_USERSPACE */

RTIO_BMEM uint8_t mempool_data[MEM_BLK_SIZE];

static void test_rtio_simple_mempool_(struct rtio *r, int run_count)
{
	int res;
	struct rtio_sqe sqe;
	struct rtio_cqe cqe;

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
	zassert_ok(res, "Should return ok from rtio_execute");

	TC_PRINT("Calling rtio_cqe_copy_out\n");
	zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER));
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

static void rtio_simple_mempool_test(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	TC_PRINT("rtio simple mempool simple\n");
	for (int i = 0; i < TEST_REPEATS * 2; i++) {
		test_rtio_simple_mempool_(&r_simple_simp, i);
	}
	TC_PRINT("rtio simple mempool concurrent\n");
	for (int i = 0; i < TEST_REPEATS * 2; i++) {
		test_rtio_simple_mempool_(&r_simple_con, i);
	}
}

ZTEST(rtio_api, test_rtio_simple_mempool)
{
	rtio_iodev_test_init(&iodev_test_simple);
#ifdef CONFIG_USERSPACE
	k_mem_domain_add_thread(&rtio_domain, k_current_get());
	rtio_access_grant(&r_simple_simp, k_current_get());
	rtio_access_grant(&r_simple_con, k_current_get());
	k_object_access_grant(&iodev_test_simple, k_current_get());
	k_thread_user_mode_enter(rtio_simple_mempool_test, NULL, NULL, NULL);
#else
	rtio_simple_mempool_test(NULL, NULL, NULL);
#endif
}

static void test_rtio_simple_cancel_(struct rtio *r, int i)
{
	struct rtio_sqe sqe;
	struct rtio_cqe cqe;
	struct rtio_sqe *handle[1];

	rtio_sqe_prep_nop(&sqe, (struct rtio_iodev *)&iodev_test_simple, NULL);
	rtio_sqe_copy_in_get_handles(r, &sqe, handle, 1);
	rtio_sqe_cancel(handle[0]);
	rtio_submit(r, 1);

	zassert_equal(1, rtio_cqe_copy_out(r, &cqe, 1, K_FOREVER));
	zassert_equal(-ECANCELED, cqe.result);
}

static void rtio_simple_cancel_test(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	TC_PRINT("rtio simple cancel simple\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_simple_cancel_(&r_simple_simp, i);
	}
	TC_PRINT("rtio simple cancel concurrent\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_simple_cancel_(&r_simple_con, i);
	}
}

ZTEST(rtio_api, test_rtio_simple_cancel)
{
	rtio_iodev_test_init(&iodev_test_simple);
#ifdef CONFIG_USERSPACE
	k_mem_domain_add_thread(&rtio_domain, k_current_get());
	rtio_access_grant(&r_simple_simp, k_current_get());
	rtio_access_grant(&r_simple_con, k_current_get());
	k_object_access_grant(&iodev_test_simple, k_current_get());
	k_thread_user_mode_enter(rtio_simple_cancel_test, NULL, NULL, NULL);
#else
	rtio_simple_cancel_test(NULL, NULL, NULL);
#endif
}


ZTEST(rtio_api, test_rtio_syscalls)
{
	TC_PRINT("test iodev init\n");
	rtio_iodev_test_init(&iodev_test_syscall);
	TC_PRINT("syscalls from kernel mode\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		rtio_syscall_test(NULL, NULL, NULL);
	}
}

RTIO_EXECUTOR_SIMPLE_DEFINE(transaction_exec_simp);
RTIO_DEFINE(r_transaction_simp, (struct rtio_executor *)&transaction_exec_simp, 4, 4);

RTIO_EXECUTOR_CONCURRENT_DEFINE(transaction_exec_con, 2);
RTIO_DEFINE(r_transaction_con, (struct rtio_executor *)&transaction_exec_con, 4, 4);

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

	sqe = rtio_spsc_acquire(r->sq);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, &iodev_test_transaction0, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;

	sqe = rtio_spsc_acquire(r->sq);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, NULL,
			  &userdata[0]);


	sqe = rtio_spsc_acquire(r->sq);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, &iodev_test_transaction1, NULL);
	sqe->flags |= RTIO_SQE_TRANSACTION;

	sqe = rtio_spsc_acquire(r->sq);
	zassert_not_null(sqe, "Expected a valid sqe");
	rtio_sqe_prep_nop(sqe, NULL,
			  &userdata[1]);

	TC_PRINT("submitting userdata 0 %p, userdata 1 %p\n", &userdata[0], &userdata[1]);
	res = rtio_submit(r, 4);
	TC_PRINT("checking cq, completions available %lu\n", rtio_spsc_consumable(r->cq));
	zassert_ok(res, "Should return ok from rtio_execute");
	zassert_equal(rtio_spsc_consumable(r->cq), 4, "Should have 4 pending completions");

	for (int i = 0; i < 4; i++) {
		TC_PRINT("consume %d\n", i);
		cqe = rtio_spsc_consume(r->cq);
		zassert_not_null(cqe, "Expected a valid cqe");
		zassert_ok(cqe->result, "Result should be ok");
		if (i % 2 == 0) {
			zassert_is_null(cqe->userdata);
			rtio_spsc_release(r->cq);
			continue;
		}
		uintptr_t idx = *(uintptr_t *)cqe->userdata;

		TC_PRINT("userdata is %p, value %" PRIuPTR "\n", cqe->userdata, idx);
		zassert(idx == 0 || idx == 1, "idx should be 0 or 1");
		seen[idx] = true;
		rtio_spsc_release(r->cq);
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
		test_rtio_transaction_(&r_transaction_simp);
	}
	TC_PRINT("rtio transaction concurrent\n");
	for (int i = 0; i < TEST_REPEATS; i++) {
		test_rtio_transaction_(&r_transaction_con);
	}
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

ZTEST_SUITE(rtio_api, NULL, rtio_api_setup, NULL, NULL, NULL);
