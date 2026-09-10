/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/drivers/edac.h>
#include <ibecc.h>

#define TEST_ADDRESS1		0x1000
#define TEST_ADDRESS2		0x2000
#define TEST_DATA		0xface
#define TEST_ADDRESS_MASK	INJ_ADDR_BASE_MASK_MASK
#define DURATION		100

ZTEST(ibecc, test_ibecc_driver_initialized)
{
	const struct device *dev;

	LOG_DBG("Test ibecc driver is initialized");

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	zassert_true(device_is_ready(dev), "Device is not ready");
}

K_APPMEM_PARTITION_DEFINE(default_part);

K_APP_BMEM(default_part) static volatile int interrupt;
K_APP_BMEM(default_part) static volatile uint32_t error_type;
K_APP_BMEM(default_part) static volatile uint64_t error_address;
K_APP_BMEM(default_part) static volatile uint16_t error_syndrome;

/* Keep track or correctable and uncorrectable errors */
static unsigned int errors_correctable, errors_uncorrectable;

static void callback(const struct device *d, void *data)
{
	struct ibecc_error *error_data = data;

	interrupt++;
	error_type = error_data->type;
	error_address = error_data->address;
	error_syndrome = error_data->syndrome;
}

ZTEST(ibecc, test_ibecc_api)
{
	const struct device *dev;
	uint64_t value;
	int ret;

	LOG_DBG("Test IBECC API");

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
	zassert_equal(ret, errors_correctable,
		      "Error correctable count does not match");

	ret = edac_errors_uc_get(dev);
	zassert_equal(ret, errors_uncorrectable,
		      "Error uncorrectable count does mot match");

	/* Notification API */

	ret = edac_notify_callback_set(dev, callback);
	zassert_equal(ret, 0, "Error setting notification callback");
}

ZTEST(ibecc, test_ibecc_error_inject_api)
{
	const struct device *dev;
	uint32_t test_value;
	uint64_t val;
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_EDAC_ERROR_INJECT);

	LOG_DBG("Test IBECC Inject API");

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
	LOG_DBG("Mapped 0x%llx to 0x%llx", addr, test_addr);

	test_value = sys_read32(test_addr);
	LOG_DBG("Read value 0x%llx: 0x%x", test_addr, test_value);

	/* Write to this test address some data */
	sys_write32(TEST_DATA, test_addr);
	LOG_DBG("Wrote value 0x%x at 0x%llx", TEST_DATA, test_addr);

	/* Read back, triggering interrupt and notification */
	test_value = sys_read32(test_addr);
	LOG_DBG("Read value 0x%llx: 0x%x", test_addr, test_value);

	/* Wait for interrupt if needed */
	k_busy_wait(USEC_PER_MSEC * DURATION);

	/* Load to local variable to avoid using volatile in assert */
	num_int = interrupt;

	zassert_not_equal(num_int, 0, "Interrupt handler did not execute");
	zassert_equal(num_int, 1,
		      "Interrupt handler executed more than once! (%d)\n",
		      num_int);

	LOG_DBG("Interrupt %d", num_int);
	LOG_DBG("Error: type %u, address 0x%llx, syndrome %u",
		error_type, error_address, error_syndrome);

	/* Check statistic information */

	ret = edac_errors_cor_get(dev);
	zassert_equal(ret, type == EDAC_ERROR_TYPE_DRAM_COR ?
		      errors_cor + 1 : errors_cor,
		      "Incorrect correctable count");
	LOG_DBG("Correctable error count %d", ret);

	errors_correctable = ret;

	ret = edac_errors_uc_get(dev);
	zassert_equal(ret, type == EDAC_ERROR_TYPE_DRAM_UC ?
		      errors_uc + 1 : errors_uc,
		      "Incorrect uncorrectable count");
	LOG_DBG("Uncorrectable error count %d", ret);

	errors_uncorrectable = ret;

	/* Clear */

	ret = edac_inject_set_error_type(dev, 0);
	zassert_equal(ret, 0, "Error setting inject error type");

	ret = edac_inject_set_param1(dev, 0);
	zassert_equal(ret, 0, "Error setting inject address");

	ret = edac_inject_set_param2(dev, 0);
	zassert_equal(ret, 0, "Error setting inject address mask");

	ret = edac_inject_error_trigger(dev);
	zassert_equal(ret, 0, "Error setting ctrl");
}

static void check_values(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	intptr_t address = (intptr_t)p1;
	intptr_t type = (intptr_t)p2;
	intptr_t addr, errtype;

#if defined(CONFIG_USERSPACE)
	LOG_DBG("Test communication in user mode thread");
	zassert_true(k_is_user_context(), "thread left in kernel mode");
#endif

	/* Load to local variables to avoid using volatile in assert */
	addr = error_address;
	errtype = error_type;

	/* Verify page address and error type */
	zassert_equal(addr, address, "Error address wrong");
	zassert_equal(errtype, type, "Error type wrong");
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
	k_thread_user_mode_enter(check_values,
				 (void *)addr,
				 (void *)type,
				 NULL);
#else
	check_values((void *)addr, (void *)type, NULL);
#endif
}

ZTEST(ibecc, test_ibecc_error_inject_test_cor)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_EDAC_ERROR_INJECT);

	LOG_DBG("Test IBECC injection correctable error");

	ibecc_error_inject_test(TEST_ADDRESS1, TEST_ADDRESS_MASK,
				EDAC_ERROR_TYPE_DRAM_COR);
}

ZTEST(ibecc, test_ibecc_error_inject_test_uc)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_EDAC_ERROR_INJECT);

	LOG_DBG("Test IBECC injection uncorrectable error");

	ibecc_error_inject_test(TEST_ADDRESS2, TEST_ADDRESS_MASK,
				EDAC_ERROR_TYPE_DRAM_UC);
}

static void *setup_ibecc(void)
{
#if defined(CONFIG_USERSPACE)
	int ret = k_mem_domain_add_partition(&k_mem_domain_default,
					     &default_part);
	if (ret != 0) {
		LOG_ERR("Failed to add to mem domain (%d)", ret);
		LOG_ERR("Running test setup function second time?");
		ztest_test_fail();
	}
#endif
	return NULL;
}

ZTEST_SUITE(ibecc, NULL, setup_ibecc, NULL, NULL, NULL);
