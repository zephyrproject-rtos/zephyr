/*
 * Copyright (c) 2025 Intel Corporation.
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

RTIO_POOL_DEFINE(rpool, 2, 8, 8);

#ifdef CONFIG_USERSPACE
struct k_mem_domain pool_domain;
#endif


ZTEST_USER(rtio_pool, test_rtio_pool_acquire_release)
{
	struct rtio *r = rtio_pool_acquire(&rpool);

	zassert_not_null(r, "expected valid rtio context");

	struct rtio_sqe nop_sqe;
	struct rtio_cqe nop_cqe;

	rtio_sqe_prep_nop(&nop_sqe, NULL, NULL);
	rtio_sqe_copy_in(r, &nop_sqe, 1);
	rtio_submit(r, 1);
	rtio_cqe_copy_out(r, &nop_cqe, 1, K_FOREVER);

	rtio_pool_release(&rpool, r);
}

static void *rtio_pool_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_mem_domain_init(&pool_domain, 0, NULL);
	k_mem_domain_add_partition(&pool_domain, &rtio_partition);
#if Z_LIBC_PARTITION_EXISTS
	k_mem_domain_add_partition(&pool_domain, &z_libc_partition);
#endif /* Z_LIBC_PARTITION_EXISTS */
#endif /* CONFIG_USERSPACE */

	return NULL;
}

static void rtio_pool_before(void *a)
{
	ARG_UNUSED(a);

#ifdef CONFIG_USERSPACE
	k_object_access_grant(&rpool, k_current_get());
#endif /* CONFIG_USERSPACE */
}

ZTEST_SUITE(rtio_pool, NULL, rtio_pool_setup, rtio_pool_before, NULL, NULL);
