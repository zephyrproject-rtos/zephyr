/*
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#if defined(CONFIG_NORDIC_QSPI_NOR)
#define TEST_AREA_DEV_NODE	DT_INST(0, nordic_qspi_nor)
#elif defined(CONFIG_SPI_NOR)
#define TEST_AREA_DEV_NODE	DT_INST(0, jedec_spi_nor)
#else
#define TEST_AREA	storage_partition
#endif

/* TEST_AREA is only defined for configurations that realy on
 * fixed-partition nodes.
 */
#ifdef TEST_AREA
#define TEST_AREA_OFFSET	FIXED_PARTITION_OFFSET(TEST_AREA)
#define TEST_AREA_SIZE		FIXED_PARTITION_SIZE(TEST_AREA)
#define TEST_AREA_MAX		(TEST_AREA_OFFSET + TEST_AREA_SIZE)
#define TEST_AREA_DEVICE	FIXED_PARTITION_DEVICE(TEST_AREA)

#elif defined(TEST_AREA_DEV_NODE)

#define TEST_AREA_DEVICE	DEVICE_DT_GET(TEST_AREA_DEV_NODE)
#define TEST_AREA_OFFSET	0xff000

#if DT_NODE_HAS_PROP(TEST_AREA_DEV_NODE, size_in_bytes)
#define TEST_AREA_MAX DT_PROP(TEST_AREA_DEV_NODE, size_in_bytes)
#else
#define TEST_AREA_MAX (DT_PROP(TEST_AREA_DEV_NODE, size) / 8)
#endif

#else
#error "Unsupported configuraiton"
#endif

#define EXPECTED_SIZE	512

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static struct flash_pages_info page_info;
static uint8_t __aligned(4) expected[EXPECTED_SIZE];
static const struct flash_parameters *flash_params;
static uint8_t erase_value;

static void *flash_driver_setup(void)
{
	int rc;

	TC_PRINT("Test will run on device %s\n", flash_dev->name);
	zassert_true(device_is_ready(flash_dev));

	flash_params = flash_get_parameters(flash_dev);
	erase_value = flash_params->erase_value;

	/* For tests purposes use page (in nrf_qspi_nor page = 64 kB) */
	flash_get_page_info_by_offs(flash_dev, TEST_AREA_OFFSET,
				    &page_info);

	/* Check if test region is not empty */
	uint8_t buf[EXPECTED_SIZE];

	rc = flash_read(flash_dev, TEST_AREA_OFFSET,
			buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	/* Fill test buffer with random data */
	for (int i = 0, val = 0; i < EXPECTED_SIZE; i++, val++) {
		/* Skip erase value */
		if (val == erase_value) {
			val++;
		}
		expected[i] = val;
	}

	/* Check if tested region fits in flash */
	zassert_true((TEST_AREA_OFFSET + EXPECTED_SIZE) <= TEST_AREA_MAX,
		     "Test area exceeds flash size");

	/* Check if flash is cleared */
	bool is_buf_clear = true;

	for (off_t i = 0; i < EXPECTED_SIZE; i++) {
		if (buf[i] != erase_value) {
			is_buf_clear = false;
			break;
		}
	}

	if (!is_buf_clear) {
		/* Erase a nb of pages aligned to the EXPECTED_SIZE */
		rc = flash_erase(flash_dev, page_info.start_offset,
				(page_info.size *
				((EXPECTED_SIZE + page_info.size - 1)
				/ page_info.size)));

		zassert_equal(rc, 0, "Flash memory not properly erased");
	}

	return NULL;
}

ZTEST(flash_driver, test_read_unaligned_address)
{
	int rc;
	uint8_t buf[EXPECTED_SIZE];
	const uint8_t canary = erase_value;

	rc = flash_write(flash_dev,
			 page_info.start_offset,
			 expected, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot write to flash");

	/* read buffer length*/
	for (off_t len = 0; len < 25; len++) {
		/* address offset */
		for (off_t ad_o = 0; ad_o < 4; ad_o++) {
			/* buffer offset; leave space for buffer guard */
			for (off_t buf_o = 1; buf_o < 5; buf_o++) {
				/* buffer overflow protection */
				buf[buf_o - 1] = canary;
				buf[buf_o + len] = canary;
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
				zassert_equal(buf[buf_o - 1], canary,
					"Buffer underflow at len=%d, "
					"ad_o=%d, buf_o=%d", len, ad_o, buf_o);
				zassert_equal(buf[buf_o + len], canary,
					"Buffer overflow at len=%d, "
					"ad_o=%d, buf_o=%d", len, ad_o, buf_o);
			}
		}
	}
}

ZTEST_SUITE(flash_driver, NULL, flash_driver_setup, NULL, NULL, NULL);
