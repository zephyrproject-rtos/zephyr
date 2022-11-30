/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/device.h>
#include <zephyr/fpga_manager/fpga_manager.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* Checking the FPGA configuration status */
ZTEST(fpga_manager_stack, test_fpga_status)
{
	int ret = 0;
	void *config_status_buf = k_malloc(FPGA_RECONFIG_STATUS_BUF_SIZE);

	if (!config_status_buf) {
		zassert_equal(ret, -EFAULT, "Failed to get the status");
	}

	ret = fpga_get_status(config_status_buf);
	zassert_equal(ret, 0, "Failed to get the status");
	k_free(config_status_buf);

	ret = fpga_get_status(NULL);
	zassert_equal(ret, -ENOMEM, "Invalid Memory Address");
}

/* Verifying the FPGA configuration*/
ZTEST(fpga_manager_stack, test_fpga_load_file)
{
	int ret;

	ret = fpga_load_file("file", -1);
	zassert_equal(ret, -ENOTSUP, "FPGA configuration not supported");

	ret = fpga_load_file("file", 1);
	zassert_equal(ret, -ENOTSUP, "FPGA configuration not supported");

	ret = fpga_load_file("file", 0);
	zassert_equal(ret, -ENOENT, "Failed to open file");

	ret = fpga_load_file("file", 'A');
	zassert_equal(ret, -ENOTSUP, "Please provide correct configuration type");

}

/* Verifying the FPGA configuration*/
ZTEST(fpga_manager_stack, test_fpga_load)
{
	char *fpga_memory_addr;
	uint32_t fpga_memory_size = 0;
	int ret = 0;

	ret = fpga_get_memory(&fpga_memory_addr, &fpga_memory_size);
	zassert_equal(ret, 0, "Failed to get the memory");

	ret = fpga_load(fpga_memory_addr, 0);
	zassert_equal(ret, -ENOSR, "FPGA configuration failed");

	ret = fpga_load(NULL, fpga_memory_size);
	zassert_equal(ret, -EFAULT, "FPGA configuration failed");

	ret = fpga_load(fpga_memory_addr - 0x100, fpga_memory_size);
	zassert_equal(ret, -EFAULT, "FPGA configuration failed");

	ret = fpga_load(fpga_memory_addr, fpga_memory_size + 0x100);
	zassert_equal(ret, -ENOSR, "FPGA configuration failed");
}

ZTEST_SUITE(fpga_manager_stack, NULL, NULL, NULL, NULL, NULL);
