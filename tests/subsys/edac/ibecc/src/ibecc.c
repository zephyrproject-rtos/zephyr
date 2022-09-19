/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#include <zephyr/drivers/edac.h>
#include <ibecc.h>

#if defined(CONFIG_EDAC_ERROR_INJECT)
#define TEST_ADDRESS1		0x1000
#define TEST_ADDRESS2		0x2000
#define TEST_DATA		0xface
#define TEST_ADDRESS_MASK	INJ_ADDR_BASE_MASK_MASK
#define DURATION		100
#endif

ZTEST(ibecc, test_ibecc_initialized)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	zassert_true(device_is_ready(dev), "Device is not ready");

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
	const struct device *dev;
	uint64_t value;
	int ret;

	/* Error log API */

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	zassert_true(device_is_ready(dev), "Device is not ready");

	ret = edac_ecc_error_log_get(dev, &value);
	zassert_equal(ret, -ENODATA, "edac_ecc_error_log_get failed");

	ret = edac_ecc_error_log_clear(dev);
	zassert_equal(ret, 0, "edac_ecc_error_log_clear failed");

	ret = edac_parity_error_log_get(dev, &value);
	zassert_equal(ret, -ENODATA, "edac_parity_error_log_get failed");

	ret = edac_parity_error_log_clear(dev);
	zassert_equal(ret, 0, "edac_parity_error_log_clear failed");

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
	const struct device *dev;
	uint32_t test_value;
	uint64_t val;
	int ret;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	zassert_true(device_is_ready(dev), "Device is not ready");

	/* Verify default parameters */

	ret = edac_inject_get_error_type(dev, &test_value);
	zassert_equal(ret, 0, "Error getting error_type");
	zassert_equal(test_value, 0, "Error type not zero");

	ret = edac_inject_get_param1(dev, &val);
	zassert_equal(ret, 0, "Error getting param1");
	zassert_equal(val, 0, "Error param1 is not zero");

	ret = edac_inject_get_param2(dev, &val);
	zassert_equal(ret, 0, "Error getting param2");
	zassert_equal(val, 0, "Error param2 is not zero");

	/* Verify basic Injection API operations */

	/* Set correct value of param1 */
	ret = edac_inject_set_param1(dev, TEST_ADDRESS1);
	zassert_equal(ret, 0, "Error setting inject address");

	/* Try to set incorrect value of param1 with UINT64_MAX */
	ret = edac_inject_set_param1(dev, UINT64_MAX);
	zassert_not_equal(ret, 0, "Error setting invalid param1");

	ret = edac_inject_get_param1(dev, &val);
	zassert_equal(ret, 0, "Error getting param1");
	zassert_equal(val, TEST_ADDRESS1, "Read back value differs");

	/* Set correct value of param2 */
	ret = edac_inject_set_param2(dev, TEST_ADDRESS_MASK);
	zassert_equal(ret, 0, "Error setting inject address mask");

	/* Try to set incorrect value of param2 with UINT64_MAX */
	ret = edac_inject_set_param2(dev, UINT64_MAX);
	zassert_not_equal(ret, 0, "Error setting invalid param1");

	ret = edac_inject_get_param2(dev, &val);
	zassert_equal(ret, 0, "Error getting param2");
	zassert_equal(val, TEST_ADDRESS_MASK, "Read back value differs");

	/* Clearing parameters */

	ret = edac_inject_set_param1(dev, 0);
	zassert_equal(ret, 0, "Error setting inject address");

	ret = edac_inject_get_param1(dev, &val);
	zassert_equal(ret, 0, "Error getting param1");
	zassert_equal(val, 0, "Read back value differs");

	ret = edac_inject_set_param2(dev, 0);
	zassert_equal(ret, 0, "Error setting inject address mask");

	ret = edac_inject_get_param2(dev, &val);
	zassert_equal(ret, 0, "Error getting param2");
	zassert_equal(val, 0, "Read back value differs");
}
#else
static void test_ibecc_error_inject_api(void)
{
	ztest_test_skip();
}
#endif

#if defined(CONFIG_EDAC_ERROR_INJECT)
static void test_inject(const struct device *dev, uint64_t addr, uint64_t mask,
			uint8_t type)
{
	unsigned int errors_cor, errors_uc;
	uint64_t test_addr;
	uint32_t test_value;
	int ret, num_int;

	interrupt = 0;

	/* Test error_trigger() for unset error type */
	ret = edac_inject_error_trigger(dev);
	zassert_equal(ret, 0, "Error setting ctrl");

	errors_cor = edac_errors_cor_get(dev);
	zassert_not_equal(errors_cor, -ENOSYS, "Not implemented error count");

	errors_uc = edac_errors_uc_get(dev);
	zassert_not_equal(errors_uc, -ENOSYS, "Not implemented error count");

	ret = edac_inject_set_param1(dev, addr);
	zassert_equal(ret, 0, "Error setting inject address");

	ret = edac_inject_set_param2(dev, mask);
	zassert_equal(ret, 0, "Error setting inject address mask");

	/* Test correctable error inject */
	ret = edac_inject_set_error_type(dev, type);
	zassert_equal(ret, 0, "Error setting inject error type");

	ret = edac_inject_get_error_type(dev, &test_value);
	zassert_equal(ret, 0, "Error getting error_type");
	zassert_equal(test_value, type, "Read back value differs");

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

	/* Load to local variable to avoid using volatile in assert */
	num_int = interrupt;

	zassert_not_equal(num_int, 0, "Interrupt handler did not execute");
	zassert_equal(num_int, 1,
		      "Interrupt handler executed more than once! (%d)\n",
		      num_int);

	TC_PRINT("Interrupt %d\n", num_int);
	TC_PRINT("Error: type %u, address 0x%llx, syndrome %u\n",
		 error_type, error_address, error_syndrome);

	/* Check statistic information */

	ret = edac_errors_cor_get(dev);
	zassert_equal(ret, type == EDAC_ERROR_TYPE_DRAM_COR ?
		      errors_cor + 1 : errors_cor,
		      "Incorrect correctable count");
	TC_PRINT("Correctable error count %d\n", ret);

	ret = edac_errors_uc_get(dev);
	zassert_equal(ret, type == EDAC_ERROR_TYPE_DRAM_UC ?
		      errors_uc + 1 : errors_uc,
		      "Incorrect uncorrectable count");
	TC_PRINT("Uncorrectable error count %d\n", ret);
}

static int check_values(void *p1, void *p2, void *p3)
{
	intptr_t address = (intptr_t)p1;
	intptr_t type = (intptr_t)p2;
	intptr_t addr, errtype;

#if defined(CONFIG_USERSPACE)
	TC_PRINT("Test communication in user mode thread\n");
	zassert_true(k_is_user_context(), "thread left in kernel mode");
#endif

	/* Load to local variables to avoid using volatile in assert */
	addr = error_address;
	errtype = error_type;

	/* Verify page address and error type */
	zassert_equal(addr, address, "Error address wrong");
	zassert_equal(errtype, type, "Error type wrong");

	return 0;
}

static void ibecc_error_inject_test(uint64_t addr, uint64_t mask, uint64_t type)
{
	const struct device *dev;
	int ret;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	zassert_true(device_is_ready(dev), "Device is not ready");

	ret = edac_notify_callback_set(dev, callback);
	zassert_equal(ret, 0, "Error setting notification callback");

	/* Test injecting correctable error at address TEST_ADDRESS1 */
	test_inject(dev, addr, mask, type);

#if defined(CONFIG_USERSPACE)
	k_thread_user_mode_enter((k_thread_entry_t)check_values,
				 (void *)addr,
				 (void *)type,
				 NULL);
#else
	check_values((void *)addr, (void *)type, NULL);
#endif
}

static void test_ibecc_error_inject_test_cor(void)
{
	ibecc_error_inject_test(TEST_ADDRESS1, TEST_ADDRESS_MASK,
				EDAC_ERROR_TYPE_DRAM_COR);
}

static void test_ibecc_error_inject_test_uc(void)
{
	ibecc_error_inject_test(TEST_ADDRESS2, TEST_ADDRESS_MASK,
				EDAC_ERROR_TYPE_DRAM_UC);
}
#else /* CONFIG_EDAC_ERROR_INJECT */
static void test_ibecc_error_inject_test_cor(void)
{
	ztest_test_skip();
}

static void test_ibecc_error_inject_test_uc(void)
{
	ztest_test_skip();
}
#endif

static void *setup_ibecc(void)
{
#if defined(CONFIG_USERSPACE)
	int ret = k_mem_domain_add_partition(&k_mem_domain_default,
					     &default_part);
	if (ret != 0) {
		TC_PRINT("Failed to add to mem domain (%d)", ret);
		k_oops();
	}
#endif
	return NULL;
}

ZTEST(ibecc, test_ibecc_injection)
{
	test_ibecc_api();
	test_ibecc_error_inject_api();
	test_ibecc_error_inject_test_cor();
	test_ibecc_error_inject_test_uc();
}

ZTEST_SUITE(ibecc, NULL, setup_ibecc, NULL, NULL, NULL);
