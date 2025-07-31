/*
 * Copyright (c) 2025 Xtooltech
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hardware CRC Engine Test Application
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <string.h>

LOG_MODULE_REGISTER(crc_test, LOG_LEVEL_INF);

/* Test data */
static const uint8_t test_data[] = "123456789";
static const uint8_t test_data2[] = "The quick brown fox jumps over the lazy dog";

/* Expected CRC values for "123456789" */
#define EXPECTED_CRC16_CCITT  0x29B1
#define EXPECTED_CRC16        0xBB3D
#define EXPECTED_CRC32        0xCBF43926

static void test_crc16_ccitt(const struct device *crc_dev)
{
	struct crc_config config = {
		.type = CRC_POLY_TYPE_CRC16_CCITT,
		.seed = 0xFFFF,
		.reflect_input = false,
		.reflect_output = false,
		.complement_input = false,
		.complement_output = false,
	};
	uint32_t result;
	int ret;

	printk("\n=== CRC-16-CCITT Test ===\n");

	ret = crc_configure(crc_dev, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure CRC-16-CCITT: %d", ret);
		return;
	}

	/* Single computation */
	result = crc_compute(crc_dev, test_data, sizeof(test_data) - 1);
	printk("CRC-16-CCITT of '%s': 0x%04X (expected 0x%04X) %s\n",
	       test_data, result & 0xFFFF, EXPECTED_CRC16_CCITT,
	       (result & 0xFFFF) == EXPECTED_CRC16_CCITT ? "PASS" : "FAIL");

	/* Incremental computation */
	crc_reset(crc_dev);
	crc_append(crc_dev, test_data, 5);
	crc_append(crc_dev, test_data + 5, 4);
	result = crc_get_result(crc_dev);
	printk("CRC-16-CCITT incremental: 0x%04X %s\n",
	       result & 0xFFFF,
	       (result & 0xFFFF) == EXPECTED_CRC16_CCITT ? "PASS" : "FAIL");
}

static void test_crc16(const struct device *crc_dev)
{
	struct crc_config config = {
		.type = CRC_POLY_TYPE_CRC16,
		.seed = 0x0000,
		.reflect_input = true,
		.reflect_output = true,
		.complement_input = false,
		.complement_output = false,
	};
	uint32_t result;
	int ret;

	printk("\n=== CRC-16 Test ===\n");

	ret = crc_configure(crc_dev, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure CRC-16: %d", ret);
		return;
	}

	result = crc_compute(crc_dev, test_data, sizeof(test_data) - 1);
	printk("CRC-16 of '%s': 0x%04X (expected 0x%04X) %s\n",
	       test_data, result & 0xFFFF, EXPECTED_CRC16,
	       (result & 0xFFFF) == EXPECTED_CRC16 ? "PASS" : "FAIL");
}

static void test_crc32(const struct device *crc_dev)
{
	struct crc_config config = {
		.type = CRC_POLY_TYPE_CRC32,
		.seed = 0xFFFFFFFF,
		.reflect_input = true,
		.reflect_output = true,
		.complement_input = false,
		.complement_output = true,
	};
	uint32_t result;
	int ret;

	printk("\n=== CRC-32 Test ===\n");

	ret = crc_configure(crc_dev, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure CRC-32: %d", ret);
		return;
	}

	/* Test with standard test vector */
	result = crc_compute(crc_dev, test_data, sizeof(test_data) - 1);
	printk("CRC-32 of '%s': 0x%08X (expected 0x%08X) %s\n",
	       test_data, result, EXPECTED_CRC32,
	       result == EXPECTED_CRC32 ? "PASS" : "FAIL");

	/* Test with longer data */
	result = crc_compute(crc_dev, test_data2, sizeof(test_data2) - 1);
	printk("CRC-32 of '%s': 0x%08X\n", test_data2, result);
}

static void benchmark_crc(const struct device *crc_dev)
{
	struct crc_config config = {
		.type = CRC_POLY_TYPE_CRC32,
		.seed = 0xFFFFFFFF,
		.reflect_input = true,
		.reflect_output = true,
		.complement_input = false,
		.complement_output = true,
	};
	static uint8_t large_buffer[4096];
	uint32_t start_time, end_time, elapsed_us;
	uint32_t result;
	int ret;

	printk("\n=== CRC Performance Test ===\n");

	/* Fill buffer with test pattern */
	for (int i = 0; i < sizeof(large_buffer); i++) {
		large_buffer[i] = i & 0xFF;
	}

	ret = crc_configure(crc_dev, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure CRC for benchmark: %d", ret);
		return;
	}

	/* Measure CRC-32 performance */
	start_time = k_cycle_get_32();
	result = crc_compute(crc_dev, large_buffer, sizeof(large_buffer));
	end_time = k_cycle_get_32();

	elapsed_us = k_cyc_to_us_floor32(end_time - start_time);
	
	printk("CRC-32 of %d bytes: 0x%08X\n", sizeof(large_buffer), result);
	printk("Time: %u us\n", elapsed_us);
	printk("Throughput: %u MB/s\n",
	       (uint32_t)((sizeof(large_buffer) * 1000000ULL) / elapsed_us / 1024 / 1024));
}

int main(void)
{
	const struct device *crc_dev;

	printk("\nHardware CRC Engine Test\n");
	printk("========================\n");

	/* Get CRC device */
	crc_dev = DEVICE_DT_GET(DT_NODELABEL(crc));
	if (!device_is_ready(crc_dev)) {
		LOG_ERR("CRC device not ready");
		return -1;
	}

	LOG_INF("CRC device ready");

	/* Run tests */
	test_crc16_ccitt(crc_dev);
	test_crc16(crc_dev);
	test_crc32(crc_dev);
	benchmark_crc(crc_dev);

	printk("\nAll tests completed!\n");

	return 0;
}