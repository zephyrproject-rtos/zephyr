/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
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

/* Offset between pages */
#define TEST_SIM_FLASH_SIZE FLASH_SIMULATOR_FLASH_SIZE

#define TEST_SIM_FLASH_END (TEST_SIM_FLASH_SIZE +\
			   FLASH_SIMULATOR_BASE_OFFSET)

#define PATTERN8TO32BIT(pat) \
		(((((((0xff & pat) << 8) | (0xff & pat)) << 8) | \
		   (0xff & pat)) << 8) | (0xff & pat))

static const struct device *flash_dev;
static uint8_t test_read_buf[TEST_SIM_FLASH_SIZE];

static uint32_t p32_inc;

void pattern32_ini(uint32_t val)
{
	p32_inc = val;
}

static uint32_t pattern32_inc(void)
{
	return p32_inc++;
}

static uint32_t pattern32_flat(void)
{
	return p32_inc;
}

static void test_check_pattern32(off_t start, uint32_t (*pattern_gen)(void),
				 size_t size)
{
	off_t off;
	uint32_t val32, r_val32;
	int rc;

	for (off = 0; off < size; off += 4) {
		rc = flash_read(flash_dev, start + off, &r_val32,
				sizeof(r_val32));
		zassert_equal(0, rc, "flash_write should succeed");
		val32 = pattern_gen();
		zassert_equal(val32, r_val32,
			     "flash word at offset 0x%x has value 0x%08x, " \
			     "expected 0x%08x",
			     start + off, r_val32, val32);
	}
}

/* Get access to the device and erase it ready for testing*/
static void test_init(void)
{
	int rc;

	flash_dev = device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);

	zassert_true(flash_dev != NULL,
		     "Simulated flash driver was not found!");

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_erase should succeed");
}

static void test_read(void)
{
	off_t i;
	int rc;

	rc = flash_read(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			test_read_buf, sizeof(test_read_buf));
	zassert_equal(0, rc, "flash_read should succeed");

	for (i = 0; i < sizeof(test_read_buf); i++) {
		zassert_equal(FLASH_SIMULATOR_ERASE_VALUE,
			     test_read_buf[i],
			     "sim flash byte at offset 0x%x has value 0x%08x",
			     i, test_read_buf[i]);
	}
}

static void test_write_read(void)
{
	off_t off;
	uint32_t val32 = 0, r_val32;
	int rc;

	for (off = 0; off < TEST_SIM_FLASH_SIZE; off += 4) {
		rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET +
					    off,
				 &val32, sizeof(val32));
		zassert_equal(0, rc,
			      "flash_write (%d) should succeed at off 0x%x", rc,
			       FLASH_SIMULATOR_BASE_OFFSET + off);
		val32++;
	}

	val32 = 0;

	for (off = 0; off < TEST_SIM_FLASH_SIZE; off += 4) {
		rc = flash_read(flash_dev, FLASH_SIMULATOR_BASE_OFFSET +
					   off,
				&r_val32, sizeof(r_val32));
		zassert_equal(0, rc, "flash_write should succeed");
		zassert_equal(val32, r_val32,
			"flash byte at offset 0x%x has value 0x%08x, expected" \
			" 0x%08x",
			off, r_val32, val32);
		val32++;
	}
}

static void test_erase(void)
{
	int rc;

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET +
			 FLASH_SIMULATOR_ERASE_UNIT,
			 FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(0, rc, "flash_erase should succeed");

	TC_PRINT("Incremental pattern expected\n");
	pattern32_ini(0);
	test_check_pattern32(FLASH_SIMULATOR_BASE_OFFSET, pattern32_inc,
			     FLASH_SIMULATOR_ERASE_UNIT);

	TC_PRINT("Erased area expected\n");
	pattern32_ini(PATTERN8TO32BIT(FLASH_SIMULATOR_ERASE_VALUE));
	test_check_pattern32(FLASH_SIMULATOR_BASE_OFFSET +
			     FLASH_SIMULATOR_ERASE_UNIT, pattern32_flat,
			     FLASH_SIMULATOR_ERASE_UNIT);

	TC_PRINT("Incremental pattern expected\n");
	pattern32_ini(FLASH_SIMULATOR_ERASE_UNIT*2 /
		      FLASH_SIMULATOR_PROG_UNIT);
	test_check_pattern32(FLASH_SIMULATOR_BASE_OFFSET +
			     FLASH_SIMULATOR_ERASE_UNIT*2, pattern32_inc,
			     FLASH_SIMULATOR_ERASE_UNIT*2);
}

static void test_out_of_bounds(void)
{
	int rc;
	uint8_t data[8] = {0};

	rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET - 4,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET - 4,
				 data, 8);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, TEST_SIM_FLASH_END,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, TEST_SIM_FLASH_END - 4,
				 data, 8);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET -
			 FLASH_SIMULATOR_ERASE_UNIT,
			 FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, TEST_SIM_FLASH_END,
			 FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET -
			 FLASH_SIMULATOR_ERASE_UNIT*2,
			 FLASH_SIMULATOR_ERASE_UNIT*2);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, TEST_SIM_FLASH_END -
			 FLASH_SIMULATOR_ERASE_UNIT,
			 FLASH_SIMULATOR_ERASE_UNIT*2);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, FLASH_SIMULATOR_BASE_OFFSET - 4,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, FLASH_SIMULATOR_BASE_OFFSET - 4,
				 data, 8);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, TEST_SIM_FLASH_END,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, TEST_SIM_FLASH_END - 4, data, 8);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);
}

static void test_align(void)
{
	int rc;
	uint8_t data[4] = {0};

	rc = flash_read(flash_dev, FLASH_SIMULATOR_BASE_OFFSET + 1,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET + 1,
				 data, 4);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
				 data, 3);
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET + 1,
			 FLASH_SIMULATOR_ERASE_UNIT);

	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_ERASE_UNIT + 1);

	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);
}

static void test_double_write(void)
{
	int rc;
	/* Test checks behaviour of write when attempting to double write
	 * selected offset. Simulator, prior to write, checks if selected
	 * memory contains erased values and returns -EIO if not; data has
	 * to be initialized to value that will not be equal to erase
	 * value of flash, for this test.
	 */
	uint32_t data = ~(PATTERN8TO32BIT(FLASH_SIMULATOR_ERASE_VALUE));

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(0, rc, "flash_erase should succeed");

	rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
				 &data, sizeof(data));
	zassert_equal(0, rc, "flash_write should succedd");

	rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
				 &data, sizeof(data));
	zassert_equal(-EIO, rc, "Unexpected error code (%d)", rc);
}

static void test_get_erase_value(void)
{
	const struct flash_parameters *fp = flash_get_parameters(flash_dev);

	zassert_equal(fp->erase_value, FLASH_SIMULATOR_ERASE_VALUE,
		      "Expected erase value %x",
		      FLASH_SIMULATOR_ERASE_VALUE);
}

void test_main(void)
{
	ztest_test_suite(flash_sim_api,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_read),
			 ztest_unit_test(test_write_read),
			 ztest_unit_test(test_erase),
			 ztest_unit_test(test_out_of_bounds),
			 ztest_unit_test(test_align),
			 ztest_unit_test(test_get_erase_value),
			 ztest_unit_test(test_double_write));

	ztest_run_test_suite(flash_sim_api);
}
