/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>

/* Warning: The test has been written for testing boards with single
 * instance of Flash Simulator device only.
 */

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

#if (defined(CONFIG_ARCH_POSIX) || defined(CONFIG_BOARD_QEMU_X86))
static const struct device *const flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
#else
static const struct device *const flash_dev = DEVICE_DT_GET(DT_NODELABEL(sim_flash_controller));
#endif
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

/* ret < 0 is errno; ret == 1 is bad value in flash */
static int test_check_erase(const struct device *dev, off_t offset, size_t size)
{
	uint8_t buf[FLASH_SIMULATOR_PROG_UNIT];
	int rc;
	int i = 0;

	BUILD_ASSERT(sizeof(buf) >= FLASH_SIMULATOR_PROG_UNIT);

	i = 0;
	while (i < size) {
		size_t chunk = MIN(size - i, sizeof(buf));
		/* The memset is done to set buf to something else than
		 * FLASH_SIMULATOR_ERASE_VALUE, as we are trying to chek
		 * whether that is what is now in the memory.
		 */
		memset(buf, ~FLASH_SIMULATOR_ERASE_VALUE, sizeof(buf));
		rc = flash_read(flash_dev, offset + i,
				buf, chunk);
		if (rc < 0) {
			TC_PRINT("Unexpected flash_read fail @ %ld",
				 (long)(offset + i));
			return rc;
		}
		do {
			if ((uint8_t)buf[i & (sizeof(buf) - 1)] !=
			    (uint8_t)FLASH_SIMULATOR_ERASE_VALUE) {
				TC_PRINT("Flash not erased at %ld\n",
					 (long)(offset + i));
				return 1;
			}
			++i;
			--chunk;
		} while (chunk);
	}
	return 0;
}

/* Get access to the device and erase it ready for testing*/
static void test_init(void)
{
	zassert_true(device_is_ready(flash_dev),
		     "Simulated flash device not ready");
}

ZTEST(flash_sim_api, test_read)
{
	off_t i;
	int rc;

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_erase should succeed");


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

	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_erase should succeed");


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

ZTEST(flash_sim_api, test_write_read_erase)
{
	test_write_read();
	test_erase();
}


ZTEST(flash_sim_api, test_out_of_bounds)
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

ZTEST(flash_sim_api, test_align)
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

#if defined(CONFIG_FLASH_SIMULATOR_DOUBLE_WRITES) &&	\
	defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
ZTEST(flash_sim_api, test_double_write)
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
	zassert_equal(0, rc, "flash_write should succeed");

	rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
				 &data, sizeof(data));
	zassert_equal(-EIO, rc, "Unexpected error code (%d)", rc);
}
#endif

#if !defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
ZTEST(flash_sim_api, test_ramlike)
{
	/* Within code below there is assumption that the src size is
	 * equal or greater than the FLASH_SIMULATOR_PROG_UNIT
	 * (write-block-size) of device.
	 */
	const uint8_t src[] = "Hello world! This is test string";
	uint8_t buf[FLASH_SIMULATOR_PROG_UNIT];
	/* Round up to next write-block-size */
	int max = (sizeof(src) + FLASH_SIMULATOR_PROG_UNIT - 1) &
		  ~(FLASH_SIMULATOR_PROG_UNIT - 1);
	int rc;
	int i = 0;
	int is = 0;	/* The index within src */

	BUILD_ASSERT(sizeof(src) >= FLASH_SIMULATOR_PROG_UNIT);

	/* Scrub memory with something constant */
	memset(buf, FLASH_SIMULATOR_ERASE_VALUE, sizeof(buf));
	while (i < max) {
		rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET + i,
				 buf, sizeof(buf));
		zassert_equal(0, rc, "flash_write should succeed");
		i += sizeof(buf);
	}

	/* Check the scrubbing */
	zassert_equal(0, test_check_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET, max),
		      "Area not erased");

	/* Now write something new */
	i = 0;
	while (i < max) {
		do {
			buf[i & (sizeof(buf) - 1)] = src[is];
			++i;
			++is;

			/* Continue writing from the beginning of the src */
			if (is >= sizeof(src)) {
				is = 0;
			}
		} while (i & (sizeof(buf) - 1));
		rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET	+ i - sizeof(buf),
				 buf, sizeof(buf));
		zassert_equal(0, rc, "flash_write should succeed");
	}

	/* Check the write */
	i = 0;
	is = 0;
	while (i < max) {
		rc = flash_read(flash_dev, FLASH_SIMULATOR_BASE_OFFSET + i,
				buf, sizeof(buf));
		zassert_equal(0, rc, "flash_read should succeed");
		do {
			zassert_equal((uint8_t)src[is], (uint8_t)buf[i & (sizeof(buf) - 1)],
				      "Expected src and buf to match at index %d\n", i);
			++i;
			++is;
			/* Src has wrapped around */
			if (is >= sizeof(src)) {
				is = 0;
			}
		} while (i & (sizeof(buf) - 1));
		zassert_equal(0, rc, "Unexpected value read");
	}

	/* Because we are checking random access writes, we are now going to
	 * write binary not of the same data we have just written. If this would be
	 * program-erase type memory, where you can only change from erase value
	 * to opposite, such write would render incorrect data in memory,
	 * but this is random access device so we should get exactly what we write.
	 */
	i = 0;
	is = 0;
	while (i < max) {
		do {
			buf[i & (sizeof(buf) - 1)] = ~src[is];
			++i;
			++is;

			/* Continue writing from the beginning of the src */
			if (is >= sizeof(src)) {
				is = 0;
			}
		} while (i & (sizeof(buf) - 1));
		rc = flash_write(flash_dev, FLASH_SIMULATOR_BASE_OFFSET	+ i - sizeof(buf),
				 buf, sizeof(buf));
		zassert_equal(0, rc, "flash_write should succeed");
	}

	/* Check the write */
	i = 0;
	is = 0;
	while (i < max) {
		rc = flash_read(flash_dev, FLASH_SIMULATOR_BASE_OFFSET + i,
				buf, sizeof(buf));
		zassert_equal(0, rc, "flash_read should succeed");
		do {
			zassert_equal((uint8_t)~src[is], (uint8_t)buf[i & (sizeof(buf) - 1)],
				      "Expected src and buf to match at index %d\n", i);
			++i;
			++is;
			/* Src has wrapped around */
			if (is >= sizeof(src)) {
				is = 0;
			}
		} while (i & (sizeof(buf) - 1));
		zassert_equal(0, rc, "Unexpected value read");
	}
}
#endif

ZTEST(flash_sim_api, test_get_erase_value)
{
	const struct flash_parameters *fp = flash_get_parameters(flash_dev);

	zassert_equal(fp->erase_value, FLASH_SIMULATOR_ERASE_VALUE,
		      "Expected erase value %x",
		      FLASH_SIMULATOR_ERASE_VALUE);
}

ZTEST(flash_sim_api, test_flash_fill)
{
	off_t i;
	int rc;
	uint8_t buf[FLASH_SIMULATOR_PROG_UNIT];
#if defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_erase should succeed");
#else
	rc = flash_fill(flash_dev, FLASH_SIMULATOR_ERASE_VALUE,
			FLASH_SIMULATOR_BASE_OFFSET,
			FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_fill should succeed");
#endif
	size_t size = FLASH_SIMULATOR_FLASH_SIZE;

	zassert_equal(0, test_check_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
					  FLASH_SIMULATOR_FLASH_SIZE),
		      "Area not erased");

	rc = flash_fill(flash_dev, 0x55,
			FLASH_SIMULATOR_BASE_OFFSET,
			FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_fill should succeed");

	/* Check if fill did work on both type of devices */
	i = 0;
	size = FLASH_SIMULATOR_FLASH_SIZE;
	while (i < FLASH_SIMULATOR_FLASH_SIZE) {
		size_t chunk = MIN(size, FLASH_SIMULATOR_PROG_UNIT);

		memset(buf, FLASH_SIMULATOR_ERASE_VALUE, sizeof(buf));
		rc = flash_read(flash_dev, FLASH_SIMULATOR_BASE_OFFSET + i,
				buf, chunk);
		zassert_equal(0, rc, "flash_read should succeed at offset %d", i);
		do {
			zassert_equal((uint8_t)buf[i & (sizeof(buf) - 1)], 0x55,
				      "Unexpected value at offset %d\n", i);
			++i;
			--size;
			--chunk;
		} while (chunk);
	}
}

ZTEST(flash_sim_api, test_flash_flatten)
{
	int rc;
#if defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
	rc = flash_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_erase should succeed");
#else
	rc = flash_fill(flash_dev, FLASH_SIMULATOR_ERASE_VALUE,
			FLASH_SIMULATOR_BASE_OFFSET,
			FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_fill should succeed");
#endif
	/* Now fill the device with anything */
	rc = flash_fill(flash_dev, 0xaa,
			FLASH_SIMULATOR_BASE_OFFSET,
			FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_fill should succeed");

	/* And erase */
	rc = flash_flatten(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			   FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_flatten should succeed");

#if !defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
	/* In case of RAM like devices, the erase may not be
	 * provided, in such case flash_flatten calls flash_fill
	 * with erase_value.
	 */
	struct device other;
	struct flash_driver_api api;

	memcpy(&other, flash_dev, sizeof(other));
	other.api = &api;
	memcpy(&api, flash_dev->api, sizeof(api));
	api.erase = NULL;

	/* Now fill the device with anything */
	rc = flash_fill(flash_dev, 0xaa,
			FLASH_SIMULATOR_BASE_OFFSET,
			FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_fill should succeed");

	/* The erase is not implemented */
	rc = flash_erase(&other, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(-ENOSYS, rc, "Expected not implemented");

	rc = flash_flatten(&other, FLASH_SIMULATOR_BASE_OFFSET,
				  FLASH_SIMULATOR_FLASH_SIZE);
	zassert_equal(0, rc, "flash_flatten should succeed");

	test_check_erase(flash_dev, FLASH_SIMULATOR_BASE_OFFSET,
			 FLASH_SIMULATOR_FLASH_SIZE);

#endif
}

#include <zephyr/drivers/flash/flash_simulator.h>

ZTEST(flash_sim_api, test_get_mock)
{
#ifdef CONFIG_ARCH_POSIX
	ztest_test_skip();
#else
	size_t mock_size;
	void *mock_ptr;

	mock_ptr = flash_simulator_get_memory(flash_dev, &mock_size);

	zassert_true(mock_ptr != NULL,
		     "Expected mock_flash address, got NULL.");
	zassert_equal(mock_size, FLASH_SIMULATOR_FLASH_SIZE,
		     "Expected mock_flash size %d, got %d",
		      FLASH_SIMULATOR_FLASH_SIZE, mock_size);
#endif
}

void *flash_sim_setup(void)
{
	test_init();

	return NULL;
}

ZTEST_SUITE(flash_sim_api, NULL, flash_sim_setup, NULL, NULL, NULL);
