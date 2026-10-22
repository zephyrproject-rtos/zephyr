/*
 * SPDX-FileCopyrightText: Copyright 2026 EXALT Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/m95p32_flash_api_extensions.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/time_units.h>

#include <errno.h>
#include <string.h>

#define M95P32_ID_READ_SIZE 16U
#define M95P32_ID_WRITE_SIZE 16U
#define M95P32_ID_BOUNDARY_OFFSET (M95P32_CUSTOMER_ID_PAGE_OFFSET - 8U)
#define M95P32_ID_BOUNDARY_SIZE 16U
#define M95P32_STANDARD_PAGE_PROGRAM_TEST_OFFSET 0x3e0000U
#define M95P32_BUFFER_LOAD_TEST_OFFSET 0x3f0000U
#define M95P32_WRITE_TEST_SIZE      (64U * 1024U)

static uint8_t main_write_data[M95P32_WRITE_TEST_SIZE];
static uint8_t main_read_data[M95P32_WRITE_TEST_SIZE];

static void print_bytes(const char *label, const uint8_t *data, size_t length)
{
	printk("%s", label);
	for (size_t i = 0; i < length; ++i) {
		printk(" %02x", data[i]);
	}
	printk("\n");
}

static void print_test_section(const char *title)
{
	printk("\n==================== %s ====================\n", title);
}

static int read_id_page(const struct device *dev, off_t offset, void *data,
			size_t length)
{
	/* Pass typed parameters to the generic Flash API extension entry point. */
	const struct flash_m95p32_ex_op_read_id_page_in op_in = {
		.offset = offset,
		.length = length,
	};

	return flash_ex_op(dev, FLASH_M95P32_EX_OP_READ_ID_PAGE,
			   (uintptr_t)&op_in, data);
}

static int write_id_page(const struct device *dev, off_t offset,
			 const void *data, size_t length)
{
	const struct flash_m95p32_ex_op_write_id_page_in op_in = {
		.offset = offset,
		.data = data,
		.length = length,
	};

	return flash_ex_op(dev, FLASH_M95P32_EX_OP_WRITE_ID_PAGE,
			   (uintptr_t)&op_in, NULL);
}

static int page_program_with_buffer_load(const struct device *dev, off_t offset,
					 const void *data, size_t length)
{
	const struct flash_m95p32_ex_op_page_program_with_buffer_load_in op_in = {
		.offset = offset,
		.data = data,
		.length = length,
	};

	return flash_ex_op(dev, FLASH_M95P32_EX_OP_PAGE_PROGRAM_WITH_BUFFER_LOAD,
			   (uintptr_t)&op_in, NULL);
}

/* Exercise factory/customer identification pages and reject an invalid write. */
static int test_identification_page(const struct device *dev)
{
	const uint8_t expected_id[] = { 0x20, 0x00, 0x16 };
	const uint8_t expected_customer_data[M95P32_ID_WRITE_SIZE] = {
		'M', '9', '5', 'P', '3', '2', '-', 'e', 'x', '-', 'o', 'p',
		'-', 't', 'e', 's',
	};
	uint8_t id_data[M95P32_ID_READ_SIZE];
	uint8_t customer_data_before[M95P32_ID_WRITE_SIZE];
	uint8_t customer_data[M95P32_ID_WRITE_SIZE];
	uint8_t cleared_customer_data[M95P32_ID_WRITE_SIZE];
	uint8_t boundary_data[M95P32_ID_BOUNDARY_SIZE];
	struct flash_m95p32_ex_op_write_id_page_in boundary_write = {
		.offset = M95P32_ID_AREA_SIZE - 8U,
		.data = expected_customer_data,
		.length = sizeof(expected_customer_data),
	};
	int rc;

	print_test_section("READ: Factory identification page");
	rc = read_id_page(dev, 0, id_data, sizeof(id_data));
	if (rc < 0) {
		printk("RDID failed: %d\n", rc);
		return rc;
	}

	print_bytes("Identification page [0x000..0x00f]:", id_data,
		    sizeof(id_data));
	if (memcmp(id_data, expected_id, sizeof(expected_id)) != 0) {
		printk("Unexpected identification bytes\n");
		return -EIO;
	}

	print_test_section("READ: Customer identification page before clear");
	rc = read_id_page(dev, M95P32_CUSTOMER_ID_PAGE_OFFSET,
			  customer_data_before, sizeof(customer_data_before));
	if (rc < 0) {
		printk("Customer ID page read before WRID failed: %d\n", rc);
		return rc;
	}
	print_bytes("Customer identification page before WRID [0x200..0x20f]:",
		    customer_data_before, sizeof(customer_data_before));

	print_test_section("WRITE: Clear customer identification page");
	memset(cleared_customer_data, 0xff, sizeof(cleared_customer_data));
	print_bytes("Clearing customer identification page [0x200..0x20f]:",
		    cleared_customer_data, sizeof(cleared_customer_data));
	rc = write_id_page(dev, M95P32_CUSTOMER_ID_PAGE_OFFSET,
			   cleared_customer_data, sizeof(cleared_customer_data));
	if (rc < 0) {
		printk("Customer ID page clear failed: %d\n", rc);
		return rc;
	}

	print_test_section("READ: Customer identification page after clear");
	rc = read_id_page(dev, M95P32_CUSTOMER_ID_PAGE_OFFSET, customer_data,
			  sizeof(customer_data));
	if (rc < 0) {
		printk("Customer ID page read after clear failed: %d\n", rc);
		return rc;
	}
	print_bytes("Customer identification page after clear [0x200..0x20f]:",
		    customer_data, sizeof(customer_data));
	if (memcmp(customer_data, cleared_customer_data,
		   sizeof(customer_data)) != 0) {
		printk("Customer identification page clear data mismatch\n");
		return -EIO;
	}

	print_test_section("WRITE: Customer identification page");
	print_bytes("Writing customer identification page [0x200..0x20f]:",
		    expected_customer_data, sizeof(expected_customer_data));

	rc = write_id_page(dev, M95P32_CUSTOMER_ID_PAGE_OFFSET,
			   expected_customer_data, sizeof(expected_customer_data));
	if (rc < 0) {
		printk("WRID failed: %d\n", rc);
		return rc;
	}

	print_test_section("READ: Customer identification page after WRID");
	rc = read_id_page(dev, M95P32_CUSTOMER_ID_PAGE_OFFSET, customer_data,
			  sizeof(customer_data));
	if (rc < 0) {
		printk("Customer ID page read failed: %d\n", rc);
		return rc;
	}
	if (memcmp(customer_data, expected_customer_data,
		   sizeof(customer_data)) != 0) {
		printk("Customer identification page data mismatch\n");
		return -EIO;
	}
	print_bytes("Customer identification page after WRID [0x200..0x20f]:",
		    customer_data, sizeof(customer_data));

	print_test_section("READ: Identification page boundary");
	rc = read_id_page(dev, M95P32_ID_BOUNDARY_OFFSET, boundary_data,
			  sizeof(boundary_data));
	if (rc < 0) {
		printk("Cross-page RDID failed: %d\n", rc);
		return rc;
	}
	print_bytes("Identification page boundary [0x1f8..0x207]:",
		    boundary_data, sizeof(boundary_data));
	if (memcmp(&boundary_data[8], expected_customer_data, 8U) != 0) {
		printk("Cross-page identification data mismatch\n");
		return -EIO;
	}

	/* Writes are restricted to the customer identification page. */
	print_test_section("WRITE: Cross-page protection");
	rc = flash_ex_op(dev, FLASH_M95P32_EX_OP_WRITE_ID_PAGE,
			 (uintptr_t)&boundary_write, NULL);
	if (rc != -EINVAL) {
		printk("Cross-page WRID was not rejected: %d\n", rc);
		return -EIO;
	}
	printk("Cross-page WRID protection passed\n");

	printk("Identification page read/write passed\n");
	return 0;
}

static int verify_main_array_write(const struct device *dev, off_t offset,
				   const char *operation)
{
	int rc;

	rc = flash_read(dev, offset, main_read_data,
			sizeof(main_read_data));
	if (rc < 0) {
		printk("%s read failed: %d\n", operation, rc);
		return rc;
	}
	if (memcmp(main_read_data, main_write_data, sizeof(main_read_data)) != 0) {
		printk("%s data mismatch\n", operation);
		return -EIO;
	}

	printk("%s data verification passed\n", operation);
	return 0;
}

/* Compare regular programming with page program using buffer-load mode. */
static int test_write_performance(const struct device *dev)
{
	uint32_t standard_start;
	uint32_t standard_cycles;
	uint32_t standard_us;
	uint32_t buffer_load_start;
	uint32_t buffer_load_cycles;
	uint32_t buffer_load_us;
	uint32_t speedup_x100;
	uint32_t time_saved_x10;
	int rc;

	for (size_t i = 0; i < sizeof(main_write_data); ++i) {
		main_write_data[i] = (uint8_t)(i ^ 0x5aU);
	}

	print_test_section("PREPARE: Erase write benchmark regions");
	rc = flash_erase(dev, M95P32_STANDARD_PAGE_PROGRAM_TEST_OFFSET,
			 M95P32_WRITE_TEST_SIZE);
	if (rc < 0) {
		printk("Standard-page-program region erase failed: %d\n", rc);
		return rc;
	}
	rc = flash_erase(dev, M95P32_BUFFER_LOAD_TEST_OFFSET,
			 M95P32_WRITE_TEST_SIZE);
	if (rc < 0) {
		printk("Buffer-load region erase failed: %d\n", rc);
		return rc;
	}

	print_test_section("WRITE: Standard page program");
	standard_start = k_cycle_get_32();
	rc = flash_write(dev, M95P32_STANDARD_PAGE_PROGRAM_TEST_OFFSET, main_write_data,
			 sizeof(main_write_data));
	standard_cycles = k_cycle_get_32() - standard_start;
	if (rc < 0) {
		printk("Standard page program failed: %d\n", rc);
		return rc;
	}
	rc = verify_main_array_write(dev, M95P32_STANDARD_PAGE_PROGRAM_TEST_OFFSET,
				    "Standard page program");
	if (rc < 0) {
		return rc;
	}

	print_test_section("WRITE: Page program with buffer load");
	buffer_load_start = k_cycle_get_32();
	rc = page_program_with_buffer_load(dev, M95P32_BUFFER_LOAD_TEST_OFFSET,
					   main_write_data, sizeof(main_write_data));
	buffer_load_cycles = k_cycle_get_32() - buffer_load_start;
	if (rc < 0) {
		printk("Page program with buffer load failed: %d\n", rc);
		return rc;
	}
	rc = verify_main_array_write(dev, M95P32_BUFFER_LOAD_TEST_OFFSET,
				    "Page program with buffer load");
	if (rc < 0) {
		return rc;
	}

	/* Time only the write calls; verification reads are deliberately excluded. */
	print_test_section("RESULT: Write performance");
	standard_us = k_cyc_to_us_floor32(standard_cycles);
	buffer_load_us = k_cyc_to_us_floor32(buffer_load_cycles);
	speedup_x100 = (uint32_t)(((uint64_t)standard_us * 100U) / buffer_load_us);
	time_saved_x10 = (uint32_t)(((uint64_t)(standard_us - buffer_load_us) * 1000U) /
				    standard_us);
	printk("Standard page program (%u bytes): %u cycles, %u.%06u s\n",
		M95P32_WRITE_TEST_SIZE, standard_cycles, standard_us / USEC_PER_SEC,
		standard_us % USEC_PER_SEC);
	printk("Page program with buffer load (%u bytes): %u cycles, %u.%06u s\n",
		M95P32_WRITE_TEST_SIZE, buffer_load_cycles,
		buffer_load_us / USEC_PER_SEC, buffer_load_us % USEC_PER_SEC);
	printk("Page-program-with-buffer-load speedup: %u.%02ux "
		"(%u.%u%% less write time)\n", speedup_x100 / 100U,
		speedup_x100 % 100U, time_saved_x10 / 10U, time_saved_x10 % 10U);

	return 0;
}

int main(void)
{
	const struct device *flash_dev = DEVICE_DT_GET(DT_ALIAS(flash0));
	int rc;

	if (!device_is_ready(flash_dev)) {
		printk("%s: device not ready\n", flash_dev->name);
		return 1;
	}

	printk("M95P32 extended operation test\n");

	rc = test_identification_page(flash_dev);
	if (rc < 0) {
		return 1;
	}

	rc = test_write_performance(flash_dev);
	if (rc < 0) {
		return 1;
	}

	printk("M95P32 extended operation test passed\n");
	return 0;
}
