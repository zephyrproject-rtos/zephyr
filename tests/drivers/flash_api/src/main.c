/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <drivers/flash.h>
#include <device.h>

/* configuration derived from DT */
#ifdef CONFIG_ARCH_POSIX
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_0)
#else
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_sim_0)
#endif /* CONFIG_ARCH_POSIX */
#define FLASH_SIMULATOR_BASE_OFFSET DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_SIMULATOR_ERASE_UNIT DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define FLASH_SIMULATOR_PROG_UNIT DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_SIMULATOR_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

#define FLASH_SIMULATOR_ERASE_VALUE \
		DT_PROP(DT_PARENT(SOC_NV_FLASH_NODE), erase_value)

#define FLASH_SIMULATOR_DEV_NAME DT_LABEL(DT_INST(0, zephyr_sim_flash))

/* Offset between pages */
#define TEST_SIM_FLASH_SIZE FLASH_SIMULATOR_FLASH_SIZE

#define TEST_SIM_FLASH_END (TEST_SIM_FLASH_SIZE +\
			   FLASH_SIMULATOR_BASE_OFFSET)

#define TEST_SIM_PAGE_SIZE FLASH_SIMULATOR_ERASE_UNIT

#define TEST_SIM_PAGE_COUNT (TEST_SIM_FLASH_SIZE / TEST_SIM_PAGE_SIZE)


static const struct device *flash_dev;

/* Get access to the device and erase it ready for testing*/
static void test_init(void)
{
	flash_dev = device_get_binding(FLASH_SIMULATOR_DEV_NAME);

	zassert_true(flash_dev != NULL, "Simulated flash (%s) not found", FLASH_SIMULATOR_DEV_NAME);
}


static void test_page_get(void)
{
	struct flash_page_info pi, pi1;
	const struct flash_parameters *fp;
	int rc = 0;

	zassert_true(flash_dev != NULL,
		     "Simulated flash driver was not found!");

	zassert_equal(TEST_SIM_PAGE_COUNT, flash_get_page_count(flash_dev),
		"Incorrect number of pages");
	zassert_equal(TEST_SIM_FLASH_SIZE, flash_get_size(flash_dev), "Incorrect size of device");
	fp = flash_get_parameters(flash_dev);
	zassert_true(fp != NULL, "Failed to get flash parameters");
	/* Flash simulator is uniform device and the FPF_NON_UNIFORM_LAYOUT is only known flag
	 * right now.
	 */
	zassert_equal(fp->flags, 0, "Expected 0 for flags");
	zassert_equal(fp->max_page_size, TEST_SIM_PAGE_SIZE, "Unexpected page size");
	zassert_equal(fp->erase_value, FLASH_SIMULATOR_ERASE_VALUE, "Unexpected erase value");
	zassert_true(flash_get_page_info(flash_dev, 0, &pi) == 0, "Failed to obtain page info");
	zassert_true(flash_get_page_info(flash_dev, TEST_SIM_PAGE_SIZE / 2, &pi1) == 0,
		"Failed to obtain page info");
	zassert_equal(pi.size, TEST_SIM_PAGE_SIZE, "Obtained page size not equal to configured");
	zassert_equal(pi.offset, 0, "Expected page offset to be equal to 0");
	zassert_equal(pi1.offset, 0,
		"Expected page offset to be 0 for query in the middle of page at offset 0");
	zassert_equal(pi.size, pi1.size, "Expected both obtained sizes to be the same");
	zassert_true(flash_get_page_info(flash_dev, TEST_SIM_PAGE_SIZE, &pi) == 0,
		"Failed to obtain page info");
	zassert_equal(pi.offset, TEST_SIM_PAGE_SIZE, "Expected second page");
	zassert_equal(pi.size, TEST_SIM_PAGE_SIZE, "Obtained page size not equal to configured");

	/* Asking for info on negative page offset */
	rc = flash_get_page_info(flash_dev, -1, &pi);
	zassert_equal(rc, -EINVAL, "Expected failure");
}

void test_main(void)
{
	ztest_test_suite(flash_api,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_page_get)
			);

	ztest_run_test_suite(flash_api);
}
