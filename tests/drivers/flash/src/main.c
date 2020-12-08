/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/flash.h>
#include <devicetree.h>
#include <storage/flash_map.h>
#include <nrfx_nvmc.h>

#if (CONFIG_NORDIC_QSPI_NOR - 0)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, nordic_qspi_nor))
#define FLASH_TEST_REGION_OFFSET 0xff000
#define TEST_AREA_MAX DT_PROP(DT_INST(0, nordic_qspi_nor), size)
#else

/* SoC embedded NVM */
#define FLASH_DEVICE DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
#define FLASH_TEST_REGION_OFFSET FLASH_AREA_OFFSET(image_1_nonsecure)
#define TEST_AREA_MAX (FLASH_TEST_REGION_OFFSET +\
		       FLASH_AREA_SIZE(image_1_nonsecure))
#else
#define FLASH_TEST_REGION_OFFSET FLASH_AREA_OFFSET(image_1)
#define TEST_AREA_MAX (FLASH_TEST_REGION_OFFSET + FLASH_AREA_SIZE(image_1))
#endif

#endif

#define EXPECTED_SIZE	256
#define CANARY		0xff

static const struct device *flash_dev;
static struct flash_pages_info page_info;
static uint8_t __aligned(4) expected[EXPECTED_SIZE];

static void test_setup(void)
{
	int rc;

	flash_dev = device_get_binding(FLASH_DEVICE);
	const struct flash_parameters *qspi_flash_parameters =
			flash_get_parameters(flash_dev);

	/* For tests purposes use page (in nrf_qspi_nor page = 64 kB) */
	flash_get_page_info_by_offs(flash_dev, FLASH_TEST_REGION_OFFSET,
				    &page_info);

	/* Check if test region is not empty */
	uint8_t buf[EXPECTED_SIZE];

	rc = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET,
			buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	/* Fill test buffer with random data */
	for (int i = 0; i < EXPECTED_SIZE; i++) {
		expected[i] = i;
	}

	/* Check if tested region fits in flash */
	zassert_true((FLASH_TEST_REGION_OFFSET + EXPECTED_SIZE) < TEST_AREA_MAX,
		     "Test area exceeds flash size");

	/* Check if flash is cleared */
	bool is_buf_clear = true;

	for (off_t i = 0; i < EXPECTED_SIZE; i++) {
		if (buf[i] != qspi_flash_parameters->erase_value) {
			is_buf_clear = false;
			break;
		}
	}

	if (!is_buf_clear) {
		/* erase page */
		rc = flash_erase(flash_dev, page_info.start_offset,
				 page_info.size);
		zassert_equal(rc, 0, "Flash memory not properly erased");
	}
}

static void test_read_unaligned_address(void)
{
	int rc;
	rc = flash_write(flash_dev,
			 page_info.start_offset,
			 expected, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot write to flash");

	#if !defined(CONFIG_NORDIC_QSPI_NOR)
	uint8_t buf[EXPECTED_SIZE];
	/* read buffer length*/
	for (off_t len = 0; len < 25; len++) {
		/* address offset */
		for (off_t ad_o = 0; ad_o < 4; ad_o++) {
			/* buffer offset; leave space for buffer guard */
			for (off_t buf_o = 1; buf_o < 5; buf_o++) {
				/* buffer overflow protection */
				buf[buf_o - 1] = CANARY;
				buf[buf_o + len] = CANARY;
				memset(buf + buf_o, 0, len);
				rc = flash_read(flash_dev,
						page_info.start_offset + ad_o,
						buf + buf_o, len);
				zassert_equal(rc, 0, "Cannot read flash");
				zassert_equal(memcmp(buf + buf_o,
						expected + ad_o,
						len),
					0, "Flash read failed at len=%d, "
					"ad_o=%d, buf_o=%d", len, ad_o, buf_o);
				/* check buffer guards */
				zassert_equal(buf[buf_o - 1], CANARY,
					"Buffer underflow at len=%d, "
					"ad_o=%d, buf_o=%d", len, ad_o, buf_o);
				zassert_equal(buf[buf_o + len], CANARY,
					"Buffer overflow at len=%d, "
					"ad_o=%d, buf_o=%d", len, ad_o, buf_o);
			}
		}
	}
	#endif

}

static void test_erase(void)
{
	int rc;
	uint8_t buf[EXPECTED_SIZE];
	const struct flash_parameters *fp = flash_get_parameters(flash_dev);

	rc = flash_write(flash_dev,
			 page_info.start_offset,
			 expected, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot write to flash");

	rc = flash_erase(flash_dev, page_info.start_offset,
				 page_info.size);
	zassert_equal(rc, 0, "Cannot erase flash");

	rc = flash_read(flash_dev, page_info.start_offset,
			buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	for (off_t i = 0; i < EXPECTED_SIZE; i++) {
		zassert_equal(buf[i], fp->erase_value,
				"Flash memory not properly erased");
	}
}

static void test_access(void)
{
	int rc;

	rc = flash_write_protection_set(flash_dev, true);
	zassert_equal(rc, 0, NULL);

	/* nRF5x write_protection is no-operation */
	#if defined(CONFIG_NORDIC_QSPI_NOR)
	rc = flash_write(flash_dev, page_info.start_offset,
				 expected, EXPECTED_SIZE);
	zassert_equal(rc, -EACCES, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, page_info.start_offset,
			 4);
	zassert_equal(rc, -EACCES, "Unexpected error code (%d)", rc);
	#endif
}

static void test_write_unaligned(void)
{
	int rc;
	uint8_t data[4] = {0};

	rc = flash_write(flash_dev, page_info.start_offset + 1,
				 data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, page_info.start_offset + 1,
				 data, 3);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);
}

static void test_out_of_bounds(void)
{
	int rc;
	uint8_t data[8] = {0};
	size_t flash_size = 0;
	size_t erase_unit_size = 0;

	rc = flash_write_protection_set(flash_dev, false);
	#if defined(CONFIG_NORDIC_QSPI_NOR)
	flash_size = TEST_AREA_MAX;
	/* QSPI erase must be sector-aligned */
	erase_unit_size = 0x1000U;
	#else
	flash_size = nrfx_nvmc_flash_size_get();
	/* nRF5x erase can only be done per page */
	erase_unit_size = nrfx_nvmc_flash_page_size_get();
	#endif

	rc = flash_write(flash_dev, -1,
				 data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, flash_size,
				 data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, flash_size - 4,
				 data, 8);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, page_info.start_offset,
				 data, flash_size);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, page_info.start_offset,
				 data, erase_unit_size);
	zassert_equal(rc, 0, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, flash_size, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, flash_size - 4, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, flash_size - erase_unit_size,
					2*erase_unit_size);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, page_info.start_offset,
				 flash_size);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, -1,
				 data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, flash_size,
				 data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, flash_size - 4,
				 data, 8);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, page_info.start_offset,
				 data, flash_size);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);
}

void test_main(void)
{
	ztest_test_suite(flash_driver_test,
		ztest_unit_test(test_setup),
		ztest_unit_test(test_read_unaligned_address),
		ztest_unit_test(test_erase),
		ztest_unit_test(test_access),
		ztest_unit_test(test_write_unaligned),
		ztest_unit_test(test_out_of_bounds)
	);

	ztest_run_test_suite(flash_driver_test);
}
