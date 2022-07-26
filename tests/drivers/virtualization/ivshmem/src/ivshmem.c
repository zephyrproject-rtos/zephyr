/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

#include <zephyr/drivers/virtualization/ivshmem.h>

void test_ivshmem_plain(void)
{
	const struct device *ivshmem;
	uintptr_t mem;
	size_t size;
	uint32_t id;
	uint32_t *area_to_rw;
	uint16_t vectors;
	int ret;

	ivshmem = DEVICE_DT_GET_ONE(qemu_ivshmem);
	zassert_true(device_is_ready(ivshmem), "ivshmem device is not ready");

	size = ivshmem_get_mem(ivshmem, &mem);
	zassert_not_equal(size, 0, "Size cannot not be 0");
	zassert_not_null((void *)mem, "Shared memory cannot be null");

	id = ivshmem_get_id(ivshmem);
	zassert_equal(id, 0, "ID should be 0 on ivshmem-plain");

	area_to_rw = (uint32_t *)mem;

	*area_to_rw = 8108; /* some data */
	zassert_equal(*area_to_rw, 8108,
		      "Could not r/w to the shared memory");

	/* Quickly verifying that non-plain features return proper code */
	vectors = ivshmem_get_vectors(ivshmem);
	zassert_equal(vectors, 0, "ivshmem-plain cannot have vectors");

	ret = z_impl_ivshmem_int_peer(ivshmem, 0, 0);
	zassert_equal(ret, -ENOSYS,
		      "interrupting peers should not be supported");

	ret = ivshmem_register_handler(ivshmem, NULL, 0);
	zassert_equal(ret, -ENOSYS,
		      "registering handlers should not be supported");
}

#ifdef CONFIG_USERSPACE
void test_is_usermode(void)
{
	zassert_true(k_is_user_context(), "thread left in kernel mode");
}

void test_quit_kernel(void)
{
	k_thread_user_mode_enter((k_thread_entry_t)test_is_usermode,
				 NULL, NULL, NULL);
}
#else
void test_quit_kernel(void)
{ }
#endif /* CONFIG_USERSPACE */

void test_main(void)
{
	ztest_test_suite(test_ivshmem,
			 ztest_unit_test(test_quit_kernel),
			 ztest_unit_test(test_ivshmem_plain));
	ztest_run_test_suite(test_ivshmem);
}
