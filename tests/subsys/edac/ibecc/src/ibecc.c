/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <ztest.h>
#include <tc_util.h>

#include <drivers/edac.h>
#include <ibecc.h>

#define DEVICE_NAME		DT_LABEL(DT_NODELABEL(ibecc))

#if defined(CONFIG_EDAC_ERROR_INJECT)
#define TEST_ADDRESS1		0x1000
#define TEST_ADDRESS2		0x2000
#define TEST_DATA		0xface
#define TEST_ADDRESS_MASK	INJ_ADDR_BASE_MASK_MASK
#define DURATION		100
#endif

const struct device *dev;

static void test_ibecc_initialized(void)
{
	dev = device_get_binding(DEVICE_NAME);
	zassert_not_null(dev, "Device not found");

	TC_PRINT("Test ibecc driver is initialized\n");

}

K_APPMEM_PARTITION_DEFINE(default_part);

K_APP_BMEM(default_part) static volatile int interrupt;
K_APP_BMEM(default_part) static volatile uint32_t error_type;
K_APP_BMEM(default_part) static volatile uint64_t error_address;
K_APP_BMEM(default_part) static volatile uint16_t error_syndrome;

static void callback(const struct device *d, void *data)
{
	struct ibecc_error *error_data = data;

	interrupt++;
	error_type = error_data->type;
	error_address = error_data->address;
	error_syndrome = error_data->syndrome;
}

static void test_ibecc_api(void)
{
	uint64_t ret;

	/* Error log API */

	ret = edac_ecc_error_log_get(dev);
	zassert_equal(ret, 0, "Error log not zero");

	edac_ecc_error_log_clear(dev);

	ret = edac_parity_error_log_get(dev);
	zassert_equal(ret, 0, "Error log not zero");

	edac_parity_error_log_clear(dev);

	/* Error stat API */

	ret = edac_errors_cor_get(dev);
	zassert_equal(ret, 0, "Error correctable count not zero");

	ret = edac_errors_uc_get(dev);
	zassert_equal(ret, 0, "Error uncorrectable count not zero");

	/* Notification API */

	ret = edac_notify_callback_set(dev, callback);
	zassert_equal(ret, 0, "Error setting notification callback");
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static void test_ibecc_error_inject_api(void)
{
	uint64_t val;
	int ret;

	/* Set correct value of param1 */
	ret = edac_inject_set_param1(dev, TEST_ADDRESS1);
	zassert_equal(ret, 0, "Error setting inject address");

	/* Try to set incorrect value of param1 with UINT64_MAX */
	ret = edac_inject_set_param1(dev, UINT64_MAX);
	zassert_not_equal(ret, 0, "Error setting invalid param1");

	val = edac_inject_get_param1(dev);
	zassert_equal(val, TEST_ADDRESS1, "Read back value differs");

	/* Set correct value of param2 */
	ret = edac_inject_set_param2(dev, TEST_ADDRESS_MASK);
	zassert_equal(ret, 0, "Error setting inject address mask");

	/* Try to set incorrect value of param2 with UINT64_MAX */
	ret = edac_inject_set_param2(dev, UINT64_MAX);
	zassert_not_equal(ret, 0, "Error setting invalid param1");

	val = edac_inject_get_param2(dev);
	zassert_equal(val, TEST_ADDRESS_MASK, "Read back value differs");

	/* Clearing parameters */

	ret = edac_inject_set_param1(dev, 0);
	zassert_equal(ret, 0, "Error setting inject address");

	val = edac_inject_get_param1(dev);
	zassert_equal(val, 0, "Read back value differs");

	ret = edac_inject_set_param2(dev, 0);
	zassert_equal(ret, 0, "Error setting inject address mask");

	val = edac_inject_get_param2(dev);
	zassert_equal(val, 0, "Read back value differs");
}
#else
static void test_ibecc_error_inject_api(void)
{
	ztest_test_skip();
}
#endif

#if defined(CONFIG_EDAC_ERROR_INJECT)
static void test_inject(uint64_t addr, uint64_t mask, uint8_t type)
{

	uint64_t test_addr;
	uint32_t test_value;
	int ret;

	interrupt = 0;

	ret = edac_inject_set_param1(dev, addr);
	zassert_equal(ret, 0, "Error setting inject address");

	ret = edac_inject_set_param2(dev, mask);
	zassert_equal(ret, 0, "Error setting inject address mask");

	/* Test correctable error inject */
	ret = edac_inject_set_error_type(dev, type);
	zassert_equal(ret, 0, "Error setting inject error type");

	test_value = edac_inject_get_error_type(dev);
	zassert_equal(test_value, type,
		      "Read back value differs");

	ret = edac_inject_error_trigger(dev);
	zassert_equal(ret, 0, "Error setting ctrl");

	device_map((mm_reg_t *)&test_addr, addr, 0x100, K_MEM_CACHE_NONE);
	TC_PRINT("Mapped 0x%llx to 0x%llx\n", addr, test_addr);

	test_value = sys_read32(test_addr);
	TC_PRINT("Read value 0x%llx: 0x%x\n", test_addr, test_value);

	/* Write to this test address some data */
	sys_write32(TEST_DATA, test_addr);
	TC_PRINT("Wrote value 0x%x at 0x%llx\n", TEST_DATA, test_addr);

	/* Read back, triggering interrupt and notification */
	test_value = sys_read32(test_addr);
	TC_PRINT("Read value 0x%llx: 0x%x\n", test_addr, test_value);

	/* Wait for interrupt if needed */
	k_busy_wait(USEC_PER_MSEC * DURATION);

	zassert_not_equal(interrupt, 0, "Interrupt handler did not execute");
	zassert_equal(interrupt, 1,
		      "Interrupt handler executed more than once! (%d)\n",
		      interrupt);

	TC_PRINT("Interrupt %d\n", interrupt);
	TC_PRINT("ECC Error Log 0x%llx\n", edac_ecc_error_log_get(dev));
	TC_PRINT("Error: type %u, address 0x%llx, syndrome %u\n",
		 error_type, error_address, error_syndrome);
}

static int check_values(void *p1, void *p2, void *p3)
{
	intptr_t address = (intptr_t)p1;
	intptr_t type = (intptr_t)p2;

#if defined(CONFIG_USERSPACE)
	TC_PRINT("Test communication in user mode thread\n");
	zassert_true(_is_user_context(), "thread left in kernel mode");
#endif

	/* Verify page address and error type */
	zassert_equal(error_address, address, "Error address wrong");
	zassert_equal(error_type, type, "Error type wrong");

	return 0;
}

static void test_ibecc_error_inject_test_cor(void)
{
	int ret;

	ret = edac_notify_callback_set(dev, callback);
	zassert_equal(ret, 0, "Error setting notification callback");

	/* Test injecting correctable error at address TEST_ADDRESS1 */
	test_inject(TEST_ADDRESS1, TEST_ADDRESS_MASK, EDAC_ERROR_TYPE_DRAM_COR);

#if defined(CONFIG_USERSPACE)
	k_thread_user_mode_enter((k_thread_entry_t)check_values,
				 (void *)TEST_ADDRESS1,
				 (void *)EDAC_ERROR_TYPE_DRAM_COR,
				 NULL);
#else
	check_values((void *)TEST_ADDRESS1, (void *)EDAC_ERROR_TYPE_DRAM_COR,
		     NULL);
#endif
}

static void test_ibecc_error_inject_test_uc(void)
{
	int ret;

	ret = edac_notify_callback_set(dev, callback);
	zassert_equal(ret, 0, "Error setting notification callback");

	/* Test injecting uncorrectable error at address TEST_ADDRESS2 */
	test_inject(TEST_ADDRESS2, TEST_ADDRESS_MASK, EDAC_ERROR_TYPE_DRAM_UC);

#if defined(CONFIG_USERSPACE)
	k_thread_user_mode_enter((k_thread_entry_t)check_values,
				 (void *)TEST_ADDRESS2,
				 (void *)EDAC_ERROR_TYPE_DRAM_UC,
				 NULL);
#else
	check_values((void *)TEST_ADDRESS2, (void *)EDAC_ERROR_TYPE_DRAM_UC,
		     NULL);
#endif
}
#else
static void test_ibecc_error_inject_test_cor(void)
{
	ztest_test_skip();
}

static void test_ibecc_error_inject_test_uc(void)
{
	ztest_test_skip();
}
#endif

void test_main(void)
{
#if defined(CONFIG_USERSPACE)
	k_mem_domain_add_partition(&k_mem_domain_default, &default_part);
#endif

	ztest_test_suite(ibecc,
			 ztest_unit_test(test_ibecc_initialized),
			 ztest_unit_test(test_ibecc_api),
			 ztest_unit_test(test_ibecc_error_inject_api),
			 ztest_unit_test(test_ibecc_error_inject_test_cor),
			 ztest_unit_test(test_ibecc_error_inject_test_uc)
			);
	ztest_run_test_suite(ibecc);
}
