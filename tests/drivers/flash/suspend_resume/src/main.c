/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <jesd216.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#define TEST_AREA_DEV_NODE DT_INST(0, jedec_spi_nor)
#define TEST_AREA_DEVICE   DEVICE_DT_GET(TEST_AREA_DEV_NODE)
#define TEST_AREA_OFFSET   0xff000

#if DT_NODE_HAS_PROP(TEST_AREA_DEV_NODE, size_in_bytes)
#define TEST_AREA_MAX DT_PROP(TEST_AREA_DEV_NODE, size_in_bytes)
#else
#define TEST_AREA_MAX (DT_PROP(TEST_AREA_DEV_NODE, size) / 8)
#endif

#define EXPECTED_SIZE 1024

#if !defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && !defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#error There is no flash device enabled or it is missing Kconfig options
#endif

#define THREAD_STACK_SIZE 2048

struct test_data {
	/* Timestamp of starting flash operation */
	int64_t start;
	/* Timestamp when finished flash operation */
	int64_t end;
	/* Flag indicating flash operation */
	bool is_read;
	/* Pointer to buffer holding flash data */
	uint8_t *buf;
	/* Length of the buffer */
	size_t len;
	/* Flash address of read/write operation */
	off_t addr;
};

struct k_thread child_thread;
K_THREAD_STACK_DEFINE(child_stack, THREAD_STACK_SIZE);

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static struct flash_pages_info page_info;
static uint8_t __aligned(4) expected[EXPECTED_SIZE];
static uint8_t __aligned(4) read_buf[EXPECTED_SIZE];
static uint8_t erase_value;
static bool ebw_required;

static void *flash_driver_setup(void)
{
	int rc;
	uint8_t bfp_len = 0;
	const struct jesd216_bfp *bfp = NULL;

	TC_PRINT("Test will run on device %s\n", flash_dev->name);
	zassert_true(device_is_ready(flash_dev));

#if defined(CONFIG_SPI_NOR_SFDP_DEVICETREE)
	uint8_t data[] = DT_PROP(TEST_AREA_DEV_NODE, sfdp_bfp);

	bfp = (struct jesd216_bfp *)data;
	bfp_len = DT_PROP_LEN(TEST_AREA_DEV_NODE, sfdp_bfp) / 4;
#elif defined(CONFIG_SPI_NOR_SFDP_RUNTIME)
	const struct jesd216_param_header *php;
	union {
		uint8_t raw[JESD216_SFDP_SIZE(2)];
		struct jesd216_sfdp_header sfdp;
	} u_header;

	rc = flash_sfdp_read(flash_dev, 0, &u_header.raw, sizeof(u_header.raw));
	zassert_equal(rc, 0, "Failed SFDP read");
	zassert_equal(jesd216_sfdp_magic(&u_header.sfdp), JESD216_SFDP_MAGIC, "SFDP magic invalid");

	php = u_header.sfdp.phdr;

	for (int i = 0; i < u_header.sfdp.nph; i++, php++) {
		uint16_t id = jesd216_param_id(php);

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			union {
				uint32_t dw[MIN(php->len_dw, 20)];
				struct jesd216_bfp bfp;
			} u_param;

			rc = flash_sfdp_read(flash_dev, jesd216_param_addr(php), &u_param.dw,
					     php->len_dw);

			zassert_equal(rc, 0, "Failed SFDP BFP read");
			bfp = &u_param.bfp;
			bfp_len = php->len_dw;

			break;
		}
	}
#else
#error "Unhandled SFDP choice"
#endif

	/* DW12 and DW13 hold information for suspend/resume */
	zassert_true(bfp_len >= 13, "SFDP doesn't contain information about suspend/resume");

	/* Check if flash supports suspend/resume  */
	uint32_t dw12 = sys_le32_to_cpu(bfp->dw10[2]);

	/* Inverted logic flag: 1 means not supported */
	zassert_false(dw12 & JESD216_SFDP_BFP_DW12_SUSPRESSUP_FLG,
		      "Serial NOR Flash device doesn't support suspend/resume");

	const struct flash_parameters *fparams = flash_get_parameters(flash_dev);

	erase_value = fparams->erase_value;
	ebw_required = flash_params_get_erase_cap(fparams) & FLASH_ERASE_C_EXPLICIT;

	flash_get_page_info_by_offs(flash_dev, TEST_AREA_OFFSET, &page_info);

	/* Check if test region is not empty */
	rc = flash_read(flash_dev, TEST_AREA_OFFSET, read_buf, EXPECTED_SIZE);
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
	if (IS_ENABLED(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && ebw_required) {
		bool is_buf_clear = true;

		for (off_t i = 0; i < EXPECTED_SIZE; i++) {
			if (read_buf[i] != erase_value) {
				is_buf_clear = false;
				break;
			}
		}

		if (!is_buf_clear) {
			/* Erase a nb of pages aligned to the EXPECTED_SIZE */
			rc = flash_erase(flash_dev, page_info.start_offset,
					 (page_info.size *
					  ((EXPECTED_SIZE + page_info.size - 1) / page_info.size)));

			zassert_equal(rc, 0, "Flash memory not properly erased");
		}
	}

	return NULL;
}

static void flash_thread(void *p1, void *p2, void *p3)
{
	struct test_data *data = (struct test_data *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	data->start = k_uptime_get();

	if (data->is_read) {
		flash_read(flash_dev, data->addr, data->buf, data->len);
	} else {
		flash_write(flash_dev, data->addr, data->buf, data->len);
	}

	data->end = k_uptime_get();
}

ZTEST(flash_suspend_resume, test_flash_erase_write)
{
	int rc;
	int64_t end;
	struct test_data thread_data = {
		.buf = expected,
		.addr = page_info.start_offset,
		.len = EXPECTED_SIZE,
		.is_read = false,
	};

	k_thread_create(&child_thread, child_stack, THREAD_STACK_SIZE, flash_thread, &thread_data,
			NULL, NULL, k_thread_priority_get(k_current_get()), 0, K_MSEC(10));

	rc = flash_erase(
		flash_dev, page_info.start_offset,
		(page_info.size * ((EXPECTED_SIZE + page_info.size - 1) / page_info.size)));

	end = k_uptime_get();
	zassert_equal(rc, 0, "Flash memory not properly erased");

	rc = k_thread_join(&child_thread, K_FOREVER);

	zassert_true(thread_data.start < end, "Write thread should be started before erase ends");
	zassert_true(end < thread_data.end, "Write shouldn't be done before erase");

	rc = flash_read(flash_dev, page_info.start_offset, read_buf, EXPECTED_SIZE);

	zassert_equal(rc, 0, "Cannot read flash");
	zassert_equal(strncmp(expected, read_buf, EXPECTED_SIZE), 0, "Flash content differs");
}

ZTEST(flash_suspend_resume, test_flash_write_read)
{
	int rc;
	int64_t end;
	struct test_data thread_data = {
		.buf = read_buf,
		.addr = page_info.start_offset + page_info.size,
		.len = EXPECTED_SIZE,
		.is_read = true,
	};

	/* Erase a nb of pages aligned to the EXPECTED_SIZE */
	rc = flash_erase(
		flash_dev, page_info.start_offset,
		(page_info.size * ((EXPECTED_SIZE + page_info.size - 1) / page_info.size + 1)));

	zassert_equal(rc, 0, "Flash memory not properly erased");

	k_thread_create(&child_thread, child_stack, THREAD_STACK_SIZE, flash_thread, &thread_data,
			NULL, NULL, k_thread_priority_get(k_current_get()), 0, K_MSEC(10));

	rc = flash_write(flash_dev, page_info.start_offset, expected, EXPECTED_SIZE);

	end = k_uptime_get();
	zassert_equal(rc, 0, "Failed writing to flash");

	rc = k_thread_join(&child_thread, K_FOREVER);

	zassert_true(thread_data.start < end, "Read thread should be started before write ends");
	zassert_true(thread_data.end < end, "Read should suspend write");
}

ZTEST(flash_suspend_resume, test_flash_erase_read)
{
	int rc;
	int64_t end;
	struct test_data thread_data = {
		.buf = read_buf,
		.addr = page_info.start_offset + page_info.size,
		.len = EXPECTED_SIZE,
		.is_read = true,
	};

	rc = flash_fill(flash_dev, 0xA5, page_info.start_offset, 2 * EXPECTED_SIZE);

	k_thread_create(&child_thread, child_stack, THREAD_STACK_SIZE, flash_thread, &thread_data,
			NULL, NULL, k_thread_priority_get(k_current_get()), 0, K_MSEC(10));

	/* Erase a nb of pages aligned to the EXPECTED_SIZE */
	rc = flash_erase(
		flash_dev, page_info.start_offset,
		2 * (page_info.size * ((EXPECTED_SIZE + page_info.size - 1) / page_info.size)));

	end = k_uptime_get();
	zassert_equal(rc, 0, "Flash memory not properly erased");

	rc = k_thread_join(&child_thread, K_FOREVER);

	zassert_true(thread_data.start < end, "Read thread should be started before erase ends");
	zassert_true(thread_data.end < end, "Read should suspend erase");
}

ZTEST_SUITE(flash_suspend_resume, NULL, flash_driver_setup, NULL, NULL, NULL);
