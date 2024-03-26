/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/zsai.h>
#include <zephyr/drivers/zsai_infoword.h>
#include <zephyr/drivers/zsai_ioctl.h>

#define ZSAI_NORMAL_DT_NODE DT_NODELABEL(zsai_normal)
#define ZSAI_ERASE_DT_NODE DT_NODELABEL(zsai_erase)

#define ZSAI_SIM_DT_NORMAL_EXISTS DT_NODE_EXISTS(ZSAI_NORMAL_DT_NODE)
#define ZSAI_SIM_DT_ERASE_EXISTS DT_NODE_EXISTS(ZSAI_ERASE_DT_NODE)
#define ZSAI_SIM_DT_PROP(node, prop) DT_PROP(node, prop)
#define ZSAI_SIM_DT_SIZE(node) ZSAI_SIM_DT_PROP(node, simulated_size)
#define ZSAI_SIM_DT_NEEDS_ERASE(node) DT_NODE_HAS_PROP(node, erase_value)
#define ZSAI_SIM_DT_ERASE_VALUE(node) ZSAI_SIM_DT_PROP(node, erase_value)
#define ZSAI_SIM_DT_WBS(node) ZSAI_SIM_DT_PROP(node, write_block_size)
#define ZSAI_SIM_DT_EBS(node) ZSAI_SIM_DT_PROP(node, erase_block_size)

#define FILLER "I've seen things you people wouldn't believe..."

/* Used to do read/write "in the middle" of a device. */
#define DISTANCE	5

/* This is intentional re-definition from the zsia_simulator.c driver
 * source code. No need to make define outside of driver compilation
 * unit.
 */
#define ZSAI_SIM_ERASE_SUPPORT_NEEDED				\
	IS_ENABLED(CONFIG_ZSAI_SIMULATED_DEVICE_WITH_ERASE)

struct zsai_sim_dev_config {
	/* Required by the ZSAI */
	struct zsai_device_generic_config generic;
	uint32_t size;
#if ZSAI_SIM_ERASE_SUPPORT_NEEDED
	/* Currently simulator supports uniform layout only */
	uint32_t erase_block_size;
#endif
	/* Pointer to simulated device buffer */
	uint8_t *buffer;
};

/* Fill size of buffer with predefined pattern starting at offset */
static void fill(void *buffer, off_t offset, size_t size)
{
	const static char filler[] = FILLER;
	uint8_t *p = (uint8_t *)buffer;

	zassert_true(size != 0, "Size should not be 0");
	zassert_true(offset >= 0, "Offset should be positive %ld", offset);

	while (offset < size) {
		size_t copy_size = MIN(sizeof(filler) - 1, size - offset);

		memcpy(&p[offset], filler, copy_size);
		offset += copy_size;
	}
}

/*
 * Check if size of buffer starting at offset is filled with predefined
 * pattern. Note that the buffer needs to be first filled using
 * fill.
 */
static bool check_filling(const void *buffer, off_t offset, size_t size)
{
	const static char filler[] = FILLER;
	const uint8_t *p = (uint8_t *)buffer;

	zassert_true(size != 0, "Size should not be 0");
	zassert_true(offset >= 0, "Offset should be positive %ld", offset);

	while (offset < size) {
		size_t cmp_size = MIN(sizeof(filler) - 1, size - offset);

		if (memcmp(&p[offset], filler, cmp_size) != 0) {
			return false;
		}
		offset += cmp_size;
	}
	return true;
}

/*
 * Fill device buffer with predictive pattern where byte, at each offset of the
 * buffer, is equal to the lowest byte of the offset/index of that byte.
 */
static void mk_predictive(const struct device *dev, off_t offset, size_t size)
{
	const struct zsai_sim_dev_config *cfg =
		(const struct zsai_sim_dev_config *)ZSAI_DEV_CONFIG(dev);

	zassert_true(offset < cfg->size, "Offset past size %d > %d, offset, cfg->size");
	zassert_true(size <= cfg->size, "Device smaller than size requested %d > %d",
		     size, cfg->size);
	zassert_true(cfg->size - offset >= size, "Size too big");
	zassert_true(size != 0, "Size can not be 0");

	size_t last = size + offset - 1;

	while (offset <= last) {
		cfg->buffer[offset] = offset & 0xff;
		++offset;
	}
}

/*
 * Check if buffer, read from a device, is filled with the predictive pattern.
 * offset_on_dev is the offset used for reading from device.
 */
static bool is_predictive_p(const uint8_t *p, off_t offset_on_dev, size_t size)
{
	zassert_true(size != 0, "Size can not be 0");

	for (int i = 0; i < size; i++) {
		if (p[i] != (uint8_t)((offset_on_dev + i) & 0xff)) {
			return false;
		}
	}
	return true;
}

/*
 * Check if device buffer is still filled with predictive values.
 */
static bool is_predictive(const struct device *dev, off_t offset, size_t size)
{
	const struct zsai_sim_dev_config *cfg =
		(const struct zsai_sim_dev_config *)ZSAI_DEV_CONFIG(dev);

	zassert_true(offset < cfg->size, "Offset past size %d > %d, offset, cfg->size");
	zassert_true(size <= cfg->size, "Device smaller than size requested %d > %d",
		     size, cfg->size);
	zassert_true(cfg->size - offset >= size, "Size too big");
	zassert_true(size != 0, "Size can not be 0");

	size_t last = size + offset - 1;

	while (offset <= last) {
		if (cfg->buffer[offset] != (uint8_t)(offset & 0xff)) {
			return false;
		}
		++offset;
	}
	return true;
}

#if ZSAI_SIM_ERASE_SUPPORT_NEEDED
/*
 * Check if device is tilled with "erae-value".
 */
static bool is_erased(const struct device *dev, off_t offset, size_t size)
{
	const struct zsai_sim_dev_config *cfg =
		(const struct zsai_sim_dev_config *)ZSAI_DEV_CONFIG(dev);

	zassert_true(offset < cfg->size, "Offset past size %d > %d, offset, cfg->size");
	zassert_true(size <= cfg->size, "Device smaller than size requested %d > %d",
		     size, cfg->size);
	zassert_true(cfg->size - offset >= size, "Size too big");
	zassert_true(size != 0, "Size can not be 0");

	size_t last = size + offset - 1;

	while (offset <= last) {
		if (cfg->buffer[offset] != (uint8_t)ZSAI_ERASE_VALUE(dev)) {
			return false;
		}
		++offset;
	}
	return true;
}
#endif

ZTEST(zsai_simulator, test_kconfig_vs_dt)
{
	TC_PRINT("=== The Kconfig vs DT\n");
	TC_PRINT(" == Checking normal devce\n");
	if (ZSAI_SIM_DT_NORMAL_EXISTS) {
		zassert_true(IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_NO_ERASE),
			     "Kconfig for non-erase device driver not set");
	} else {
		zassert_false(IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_NO_ERASE),
			      "Kconfig for non-erase driver not expected to be set");
	}

	TC_PRINT(" == Checking erase requiring device\n");
	if (ZSAI_SIM_DT_ERASE_EXISTS) {
		zassert_true(IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE),
			     "Kconfig for erase device driver not set");
	} else {
		zassert_false(IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE),
			      "Kconfig for erase driver not expected to be set");
	}
}

void common_write(const struct device *dev)
{
	uint8_t buffer[64];
	size_t size = 0;
	const struct zsai_sim_dev_config *cfg =
		(const struct zsai_sim_dev_config *)ZSAI_DEV_CONFIG(dev);

	TC_PRINT(" == Setting up the local buffer\n");
	fill(buffer, 0, sizeof(buffer));
	zassert_true(check_filling(buffer, 0, sizeof(buffer)), "Expected buffer ot be initialized");

	TC_PRINT(" == Getting the device size\n");
	zassert_equal(0, zsai_get_size(dev, &size), "Failed to get device size");

	TC_PRINT(" == Setting device buffer of size %ld to 'predictive' values\n", (long)size);
	mk_predictive(dev, 0, size);

	TC_PRINT(" == Testing incorrect/nop write attempts\n");
	zassert_equal(-EINVAL, zsai_write(dev, buffer, size, sizeof(buffer)),
		      "Expected fail on write beyond range");
	zassert_true(is_predictive(dev, 0, size), "Expected device to be untouched");

	zassert_equal(0, zsai_write(dev, buffer, size, 0), "Expected size 0 to be ok");
	zassert_true(is_predictive(dev, 0, size),
		     "Expected rest of the device to be untouched");
	zassert_true(check_filling(buffer, 0, sizeof(buffer)),
		     "Source buffer should not have been modified");

	if (ZSAI_WRITE_BLOCK_SIZE(dev) != 1) {
		TC_PRINT("  = Unaligned write tests for WBS %d\n", ZSAI_WRITE_BLOCK_SIZE(dev));

		zassert_equal(0, (sizeof(buffer) & (ZSAI_WRITE_BLOCK_SIZE(dev) - 1)),
			      "Size of source buffer needs correcting A");
		zassert_true(ZSAI_WRITE_BLOCK_SIZE(dev) <= sizeof(buffer),
			     "Size of source buffer needs correcting B");
		zassert_equal(-EINVAL, zsai_write(dev, buffer, 1, ZSAI_WRITE_BLOCK_SIZE(dev)),
			      "Expected failure at non-wbs aligned offset");
		zassert_true(is_predictive(dev, 0, size),
			     "Expected rest of the device to be untouched");
		zassert_true(check_filling(buffer, 0, sizeof(buffer)),
			     "Source buffer should not have been modified");

		zassert_equal(-EINVAL, zsai_write(dev, buffer, 0, ZSAI_WRITE_BLOCK_SIZE(dev) - 1),
			      "Expected failure with non-wbs aligned size");
		zassert_true(check_filling(buffer, 0, sizeof(buffer)),
			     "Source buffer should not have been modified");
		zassert_true(is_predictive(dev, 0, size),
			     "Expected rest of the device to be untouched");

		zassert_equal(-EINVAL, zsai_write(dev, buffer, 1, ZSAI_WRITE_BLOCK_SIZE(dev) - 1),
			      "Expected failure with de-alignment in offset and size");
		zassert_true(check_filling(buffer, 0, sizeof(buffer)),
			     "Source buffer should not have been modified");
		zassert_true(is_predictive(dev, 0, size),
			     "Expected rest of the device to be untouched");
	}

	TC_PRINT(" == Real write\n");

	TC_PRINT("  = Write at the beginning\n");
	mk_predictive(dev, 0, size);
	zassert_true(is_predictive(dev, 0, size),
		     "Expected device to be initialized to predictive values");

	zassert_equal(0, zsai_write(dev, buffer, 0, sizeof(buffer)), "Failed to write");
	zassert_equal(0, memcmp(cfg->buffer, buffer, sizeof(buffer)),
		      "Failed to match written data");
	zassert_true(check_filling(buffer, 0, sizeof(buffer)),
		     "Source buffer should not have been modified");
	zassert_true(is_predictive(dev, sizeof(buffer), size - sizeof(buffer)),
		     "Expected rest of the device to be untouched");

	TC_PRINT("  = Write somewhere in the middle\n");
	mk_predictive(dev, 0, size);
	zassert_true(is_predictive(dev, 0, size),
		     "Expected device to be initialized to predictive values");
	zassert_equal(0, zsai_write(dev, buffer, DISTANCE * ZSAI_WRITE_BLOCK_SIZE(dev),
				    sizeof(buffer)),
		      "Failed to write buffer\n");
	zassert_true(check_filling(buffer, 0, sizeof(buffer)),
		     "Source buffer should not have been modified");
	zassert_equal(0, memcmp(&((uint8_t *)cfg->buffer)[DISTANCE * ZSAI_WRITE_BLOCK_SIZE(dev)],
		      buffer, sizeof(buffer)),
		     "Written data does not match buffer");
	zassert_true(check_filling(buffer, 0, sizeof(buffer)),
		     "Source buffer should not have been modified");
	zassert_true(is_predictive(dev, 0, DISTANCE * ZSAI_WRITE_BLOCK_SIZE(dev)),
		     "Expected rest of the device to be untouched");
	zassert_true(is_predictive(dev, DISTANCE * ZSAI_WRITE_BLOCK_SIZE(dev) + sizeof(buffer),
				   sizeof(buffer)),
		     "Expected rest of the device to be untouched");

	TC_PRINT("  = Write at the end\n");
	mk_predictive(dev, 0, size);
	zassert_true(is_predictive(dev, 0, size),
		     "Expected device to be initialized to predictive values");

	zassert_equal(0, zsai_write(dev, buffer, size - sizeof(buffer), sizeof(buffer)),
		      "Write failed");
	zassert_equal(0, memcmp(&((uint8_t *)cfg->buffer)[size - sizeof(buffer)],
		      buffer, sizeof(buffer)),
		     "Written data does not match buffer");
	zassert_true(check_filling(buffer, 0, sizeof(buffer)),
		     "Source buffer should not have been modified");
	zassert_true(is_predictive(dev, 0, size - sizeof(buffer)),
		     "Expected rest of the device to be untouched");

}

void common_read(const struct device *dev)
{
	uint8_t buffer[64];
	size_t size = 0;

	TC_PRINT(" == Setting up the local buffer\n");
	fill(buffer, 0, sizeof(buffer));
	zassert_true(check_filling(buffer, 0, sizeof(buffer)), "Expected buffer ot be initialized");

	TC_PRINT(" == Getting the device size\n");
	zassert_equal(0, zsai_get_size(dev, &size), "Failed to get device size");

	TC_PRINT(" == Setting device buffer of size %ld to 'predictive' values\n", (long)size);
	mk_predictive(dev, 0, size);

	TC_PRINT(" == Testing incorrect/nop read attempts\n");
	zassert_equal(-EINVAL, zsai_read(dev, buffer, size, sizeof(buffer)),
		      "Expected fail on write beyond range");
	zassert_true(is_predictive(dev, 0, size), "Expected device to be untouched");
	zassert_true(check_filling(buffer, 0, sizeof(buffer)), "No read should happen");

	zassert_equal(0, zsai_read(dev, buffer, size, 0), "Expected size 0 to be ok");
	zassert_true(is_predictive(dev, 0, size),
		     "Expected rest of the device to be untouched");
	zassert_true(check_filling(buffer, 0, sizeof(buffer)), "No read should happen");

	TC_PRINT(" == Real read\n");
	mk_predictive(dev, 0, size);
	zassert_true(is_predictive(dev, 0, size),
		     "Expected device to be initialized to predictive values");

	memset(buffer, 0, sizeof(buffer));
	zassert_equal(0, zsai_read(dev, buffer, 0, sizeof(buffer)),
		      "Expected read to succeed");
	zassert_true(is_predictive_p(buffer, 0, sizeof(buffer)),
		     "Values in buffer different than expected");
	zassert_true(is_predictive(dev, 0, size),
		     "Expected device to be untouched");

	memset(buffer, 0, sizeof(buffer));
	zassert_equal(0, zsai_read(dev, buffer, DISTANCE, sizeof(buffer)),
		      "Expected read to succeed");
	zassert_true(is_predictive_p(buffer, DISTANCE, sizeof(buffer)),
		     "Values in buffer different than expected");
	zassert_true(is_predictive(dev, 0, size),
		     "Expected device to be untouched");

	memset(buffer, 0, sizeof(buffer));
	zassert_equal(0, zsai_read(dev, buffer, 0, sizeof(buffer) - DISTANCE),
		      "Expected read to succeed");
	zassert_true(is_predictive_p(buffer, 0, sizeof(buffer) - DISTANCE),
		     "Values in buffer different than expected");
	zassert_true(is_predictive(dev, 0, size),
		     "Expected device to be untouched");

	memset(buffer, 0, sizeof(buffer));
	zassert_equal(0, zsai_read(dev, buffer, size - sizeof(buffer), sizeof(buffer)),
		      "Expected read to succeed");
	zassert_true(is_predictive_p(buffer, size - sizeof(buffer), sizeof(buffer)),
		     "Values in buffer different than expected");
	zassert_true(is_predictive(dev, 0, size),
		     "Expected device to be untouched");
}

#if ZSAI_SIM_DT_NORMAL_EXISTS
ZTEST(zsai_simulator, test_normal_device)
{
	const struct device *dev = DEVICE_DT_GET(ZSAI_NORMAL_DT_NODE);
	struct zsai_ioctl_range zipi = { 0 };
	size_t size;

	TC_PRINT("=== Running no erase requiring device tests\n");
	zassert_true(IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_NO_ERASE),
		     "Expected Kconfig HAS_NO_ERAE not set");

	zassert_true(device_is_ready(dev), "ZSAI Normal device init failed");

	mk_predictive(dev, 0, ZSAI_SIM_DT_SIZE(ZSAI_NORMAL_DT_NODE));
	zassert_true(is_predictive(dev, 0, ZSAI_SIM_DT_SIZE(ZSAI_NORMAL_DT_NODE)),
		     "Expected mem reset util to succeed");

	TC_PRINT(" == Get size\n");
	zassert_equal(0, zsai_get_size(dev, &size), "Failed to get device size");
	zassert_true(is_predictive(dev, 0, ZSAI_SIM_DT_SIZE(ZSAI_NORMAL_DT_NODE)),
		     "Expected mem to be untouched");

	TC_PRINT(" == Get DT size\n");
	zassert_equal(ZSAI_SIM_DT_SIZE(ZSAI_NORMAL_DT_NODE), size, "Device size differs from DT");

	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	zassert_false(ZSAI_ERASE_REQUIRED(dev), "The device should not require erase");
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	/* Note that write block size is optional value and will default to 1 if not provided */
	TC_PRINT(" == Get write block size\n");
	zassert_equal(ZSAI_SIM_DT_WBS(ZSAI_NORMAL_DT_NODE), ZSAI_WRITE_BLOCK_SIZE(dev));
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");
	TC_PRINT("  = Write block size is %d\n", ZSAI_WRITE_BLOCK_SIZE(dev));

	zassert_true(ZSAI_SIM_DT_NEEDS_ERASE(ZSAI_NORMAL_DT_NODE) == ZSAI_ERASE_REQUIRED(dev),
		     "Erase required flag does not match DT");

	TC_PRINT(" == Get page info\n");
	zassert_equal(-ENOTSUP, zsai_get_page_info(dev, 0, &zipi));
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	zassert_equal(-ENOTSUP, zsai_erase(dev, 0, 4096), "Expected device to not support erase");
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	mk_predictive(dev, 0, size);
	zassert_true(is_predictive(dev, 0, size), "Expected device buffer to be setup for test");

	TC_PRINT("=== Running common write test set on no erase requiring device\n");
	common_write(dev);
	TC_PRINT("=== Running common read test set on no erase requiring device\n");
	common_read(dev);
}
#endif

#if ZSAI_SIM_DT_ERASE_EXISTS
ZTEST(zsai_simulator, test_erase_device)
{
	const struct device *dev = DEVICE_DT_GET(ZSAI_ERASE_DT_NODE);
	size_t size;
	struct zsai_ioctl_range zipi = { 0 };
	int ret;

	TC_PRINT("=== Running erase requiring device tests\n");
	zassert_true(IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE),
		     "Expected Kconfig HAS_ERASE not set");

	zassert_true(device_is_ready(dev), "ZSAI Erase Requiring device init failed");

	TC_PRINT(" == Get DT size\n");
	mk_predictive(dev, 0, ZSAI_SIM_DT_SIZE(ZSAI_ERASE_DT_NODE));
	zassert_true(is_predictive(dev, 0, ZSAI_SIM_DT_SIZE(ZSAI_ERASE_DT_NODE)),
		     "Expected mem reset util to succeed");

	TC_PRINT(" == Get size\n");
	ret = zsai_get_size(dev, &size);
	zassert_equal(0, ret, "Failed to get device size, ret == %d", ret);
	zassert_equal(ZSAI_SIM_DT_SIZE(ZSAI_ERASE_DT_NODE), size, "Device size differs from DT");
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	TC_PRINT(" == Is erase required\n");
	zassert_true(ZSAI_ERASE_REQUIRED(dev), "The device should not require erase");
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	zassert_true(ZSAI_SIM_DT_NEEDS_ERASE(ZSAI_ERASE_DT_NODE) == ZSAI_ERASE_REQUIRED(dev),
		     "Erase required flag does not match DT");
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	TC_PRINT(" == Get write block size\n");
	zassert_equal(ZSAI_SIM_DT_WBS(ZSAI_ERASE_DT_NODE), ZSAI_WRITE_BLOCK_SIZE(dev));
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");
	TC_PRINT("  = Write block size is %d\n", ZSAI_WRITE_BLOCK_SIZE(dev));

	uint32_t node_val = ZSAI_SIM_DT_ERASE_VALUE(ZSAI_ERASE_DT_NODE);
	uint32_t dev_val = ZSAI_ERASE_VALUE(dev);

	TC_PRINT(" == Get page info\n");
	zassert_equal((uint8_t)node_val, (uint8_t)dev_val, "Incorrect erase value");
	zassert_equal(0, zsai_get_page_info(dev, 1, &zipi));
	zassert_equal(0, zipi.offset, "Page offset not corrected");
	zassert_equal(ZSAI_SIM_DT_EBS(ZSAI_ERASE_DT_NODE), zipi.size, "Page size not correct");
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	TC_PRINT(" == Unaligned address erase\n");
	zassert_equal(-EINVAL, zsai_erase(dev, 1, zipi.size),
		      "Unaligned erase should fail");
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	TC_PRINT(" == Unaligned size erase\n");
	zassert_equal(-EINVAL, zsai_erase(dev, 0, zipi.size - 1),
		      "Unaligned erase should fail");
	zassert_true(is_predictive(dev, 0, size), "Expected mem to be untouched");

	zassert_equal(0, zsai_erase(dev, 0, zipi.size), "Erase of page failed");
	zassert_true(is_erased(dev, 0, zipi.size), "Expected page to be erased");
	zassert_true(is_predictive(dev, zipi.size, size - zipi.size), "Erased beyond range");

	TC_PRINT(" == Erase first page\n");
	mk_predictive(dev, 0, size);

	int tmp_offset = zipi.size;

	zassert_equal(0, zsai_get_page_info(dev, tmp_offset, &zipi),
		      "Expected success on page info");
	zassert_equal(0, zsai_get_page_info(dev, tmp_offset, &zipi));
	zassert_equal(tmp_offset, zipi.offset, "Page offset not correct");
	zassert_equal(ZSAI_SIM_DT_EBS(ZSAI_ERASE_DT_NODE), zipi.size, "Page size not correct");
	zassert_equal(0, zsai_erase(dev, tmp_offset, zipi.size), "Erase of page failed %d", ret);
	zassert_true(is_predictive(dev, 0, tmp_offset), "Erased beyond range");
	zassert_true(is_predictive(dev, zipi.offset + zipi.size, size - zipi.size - zipi.offset),
		     "Erased beyond range");
	zassert_true(is_erased(dev, zipi.offset, zipi.size), "Expected range erased");

	TC_PRINT(" == Erase last page\n");
	mk_predictive(dev, 0, size);
	zassert_equal(0, zsai_get_page_info(dev, size - 1, &zipi));
	zassert_equal(0, zsai_erase(dev, zipi.offset, zipi.size), "Erase of page failed");
	zassert_true(is_predictive(dev, 0, size - zipi.size), "Erased beyond range");
	zassert_true(is_erased(dev, zipi.offset, zipi.size), "Expected range erased");

	TC_PRINT(" == Erase entire device\n");
	mk_predictive(dev, 0, size);
	zassert_equal(0, zsai_erase(dev, 0, size), "Erase of device failed");
	zassert_true(is_erased(dev, 0, size), "Expected device erased");

	TC_PRINT(" == Erase by range\n");
	mk_predictive(dev, 0, size);
	zassert_equal(0, zsai_get_page_info(dev, 0, &zipi),
		      "Expected success on page info");
	zassert_equal(0, zsai_erase_range(dev, &zipi), "Erase of range failed");
	zassert_true(is_erased(dev, zipi.offset, zipi.size), "Expected range erased");
	zassert_true(is_predictive(dev, zipi.offset + zipi.size, size - zipi.size - zipi.offset),
		     "Erased beyond range");

	TC_PRINT(" == Erase nothing\n");
	mk_predictive(dev, 0, size);
	zassert_equal(0, zsai_erase(dev, 0, 0), "No error when nothing to do");

	TC_PRINT("=== Running common write test set on erase-requiring device\n");
	common_write(dev);
	TC_PRINT("=== Running common read test set on erase-requiring device\n");
	common_read(dev);
}
#endif


ZTEST_SUITE(zsai_simulator, NULL, NULL, NULL, NULL, NULL);
