/*
 * Copyright (c) 2025 Xtooltech
 * SPDX-License-Identifier: Apache-2.0
 * 
 * LPC54S018 External SDRAM Test Application
 * Tests read/write access to external SDRAM via EMC
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(sdram_test, LOG_LEVEL_INF);

/* SDRAM base address and size from device tree */
#define SDRAM_BASE_ADDR  0xA0000000
#define SDRAM_SIZE       (16 * 1024 * 1024)  /* 16MB */

/* Test patterns */
#define PATTERN_AA55     0xAA55AA55
#define PATTERN_55AA     0x55AA55AA
#define PATTERN_INCR     0x01234567
#define PATTERN_DECR     0xFEDCBA98

/* Example: Large buffer placed in SDRAM using Zephyr section attributes */
__attribute__((section(".sdram_data")))
static uint32_t sdram_large_buffer[256 * 1024];  /* 1MB buffer in SDRAM */

/* Example: BSS data in SDRAM */
__attribute__((section(".sdram_bss")))
static uint32_t sdram_work_area[64 * 1024];  /* 256KB work area in SDRAM */

/* LED for status indication */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* Function to test SDRAM with a pattern */
static int test_pattern(uint32_t pattern, const char *pattern_name)
{
	volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE_ADDR;
	uint32_t test_size = SDRAM_SIZE / sizeof(uint32_t);
	uint32_t errors = 0;
	uint32_t i;
	
	LOG_INF("Testing with pattern %s (0x%08X)...", pattern_name, pattern);
	
	/* Write pattern */
	LOG_INF("Writing %d words...", test_size);
	for (i = 0; i < test_size; i++) {
		if (pattern == PATTERN_INCR) {
			sdram[i] = i;
		} else if (pattern == PATTERN_DECR) {
			sdram[i] = ~i;
		} else {
			sdram[i] = pattern;
		}
		
		/* Show progress every 1MB */
		if ((i & 0x3FFFF) == 0) {
			gpio_pin_toggle_dt(&led);
		}
	}
	
	/* Read and verify */
	LOG_INF("Verifying %d words...", test_size);
	for (i = 0; i < test_size; i++) {
		uint32_t expected;
		uint32_t actual = sdram[i];
		
		if (pattern == PATTERN_INCR) {
			expected = i;
		} else if (pattern == PATTERN_DECR) {
			expected = ~i;
		} else {
			expected = pattern;
		}
		
		if (actual != expected) {
			if (errors < 10) {
				LOG_ERR("Error at 0x%08X: expected 0x%08X, got 0x%08X",
					SDRAM_BASE_ADDR + (i * 4), expected, actual);
			}
			errors++;
		}
		
		/* Show progress every 1MB */
		if ((i & 0x3FFFF) == 0) {
			gpio_pin_toggle_dt(&led);
		}
	}
	
	if (errors == 0) {
		LOG_INF("Pattern %s test PASSED", pattern_name);
	} else {
		LOG_ERR("Pattern %s test FAILED with %d errors", pattern_name, errors);
	}
	
	return errors;
}

/* Function to test random access */
static int test_random_access(void)
{
	volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE_ADDR;
	uint32_t test_size = SDRAM_SIZE / sizeof(uint32_t);
	uint32_t errors = 0;
	int i;
	
	LOG_INF("Testing random access...");
	
	/* Test 1000 random locations */
	for (i = 0; i < 1000; i++) {
		uint32_t addr = rand() % test_size;
		uint32_t value = rand();
		
		sdram[addr] = value;
		uint32_t readback = sdram[addr];
		
		if (readback != value) {
			LOG_ERR("Random access error at 0x%08X: wrote 0x%08X, read 0x%08X",
				SDRAM_BASE_ADDR + (addr * 4), value, readback);
			errors++;
		}
		
		if ((i % 100) == 0) {
			gpio_pin_toggle_dt(&led);
		}
	}
	
	if (errors == 0) {
		LOG_INF("Random access test PASSED");
	} else {
		LOG_ERR("Random access test FAILED with %d errors", errors);
	}
	
	return errors;
}

/* Function to measure bandwidth */
static void test_bandwidth(void)
{
	volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE_ADDR;
	uint32_t test_size = 1024 * 1024 / sizeof(uint32_t); /* 1MB */
	uint32_t start_time, end_time;
	uint32_t i;
	
	LOG_INF("Measuring SDRAM bandwidth...");
	
	/* Write bandwidth */
	start_time = k_cycle_get_32();
	for (i = 0; i < test_size; i++) {
		sdram[i] = i;
	}
	end_time = k_cycle_get_32();
	
	uint32_t write_cycles = end_time - start_time;
	uint32_t write_bandwidth = (test_size * 4 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / write_cycles;
	LOG_INF("Write bandwidth: %d MB/s", write_bandwidth / (1024 * 1024));
	
	/* Read bandwidth */
	volatile uint32_t dummy;
	start_time = k_cycle_get_32();
	for (i = 0; i < test_size; i++) {
		dummy = sdram[i];
	}
	end_time = k_cycle_get_32();
	
	uint32_t read_cycles = end_time - start_time;
	uint32_t read_bandwidth = (test_size * 4 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / read_cycles;
	LOG_INF("Read bandwidth: %d MB/s", read_bandwidth / (1024 * 1024));
	
	(void)dummy; /* Avoid unused variable warning */
}

/* Shell commands for interactive testing */
static int cmd_sdram_test(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	
	shell_print(sh, "Starting SDRAM test...");
	
	int total_errors = 0;
	
	total_errors += test_pattern(PATTERN_AA55, "0xAA55AA55");
	total_errors += test_pattern(PATTERN_55AA, "0x55AA55AA");
	total_errors += test_pattern(PATTERN_INCR, "Incrementing");
	total_errors += test_pattern(PATTERN_DECR, "Decrementing");
	total_errors += test_random_access();
	
	if (total_errors == 0) {
		shell_print(sh, "All SDRAM tests PASSED!");
	} else {
		shell_print(sh, "SDRAM tests completed with %d total errors", total_errors);
	}
	
	return 0;
}

static int cmd_sdram_fill(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: sdram fill <value>");
		return -EINVAL;
	}
	
	uint32_t value = strtoul(argv[1], NULL, 0);
	volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE_ADDR;
	uint32_t test_size = SDRAM_SIZE / sizeof(uint32_t);
	
	shell_print(sh, "Filling SDRAM with 0x%08X...", value);
	
	for (uint32_t i = 0; i < test_size; i++) {
		sdram[i] = value;
		if ((i & 0xFFFFF) == 0) {
			shell_print(sh, "Progress: %d%%", (i * 100) / test_size);
		}
	}
	
	shell_print(sh, "SDRAM filled with 0x%08X", value);
	return 0;
}

static int cmd_sdram_dump(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_error(sh, "Usage: sdram dump <offset> <count>");
		return -EINVAL;
	}
	
	uint32_t offset = strtoul(argv[1], NULL, 0);
	uint32_t count = strtoul(argv[2], NULL, 0);
	
	if (offset + count > SDRAM_SIZE) {
		shell_error(sh, "Offset + count exceeds SDRAM size");
		return -EINVAL;
	}
	
	volatile uint32_t *sdram = (volatile uint32_t *)(SDRAM_BASE_ADDR + offset);
	
	for (uint32_t i = 0; i < count; i += 4) {
		shell_print(sh, "0x%08X: %08X %08X %08X %08X",
			    SDRAM_BASE_ADDR + offset + (i * 4),
			    sdram[i], sdram[i+1], sdram[i+2], sdram[i+3]);
	}
	
	return 0;
}

static int cmd_sdram_bandwidth(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	
	test_bandwidth();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sdram_cmds,
	SHELL_CMD(test, NULL, "Run all SDRAM tests", cmd_sdram_test),
	SHELL_CMD(fill, NULL, "Fill SDRAM with value", cmd_sdram_fill),
	SHELL_CMD(dump, NULL, "Dump SDRAM contents", cmd_sdram_dump),
	SHELL_CMD(bandwidth, NULL, "Measure SDRAM bandwidth", cmd_sdram_bandwidth),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sdram, &sdram_cmds, "SDRAM test commands", NULL);

int main(void)
{
	int ret;
	
	LOG_INF("SDRAM test application for LPC54S018");
	LOG_INF("SDRAM base: 0x%08X, size: %d MB", SDRAM_BASE_ADDR, SDRAM_SIZE / (1024 * 1024));
	
	/* Configure LED */
	if (gpio_is_ready_dt(&led)) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	}
	
	/* Wait for SDRAM to initialize */
	k_sleep(K_MSEC(100));
	
	/* Run basic test */
	LOG_INF("Running basic SDRAM test...");
	volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE_ADDR;
	
	/* Simple write/read test */
	sdram[0] = 0x12345678;
	sdram[1] = 0x9ABCDEF0;
	
	if (sdram[0] == 0x12345678 && sdram[1] == 0x9ABCDEF0) {
		LOG_INF("Basic SDRAM access OK!");
		gpio_pin_set_dt(&led, 1);
	} else {
		LOG_ERR("Basic SDRAM access FAILED!");
		LOG_ERR("Wrote: 0x12345678, Read: 0x%08X", sdram[0]);
		LOG_ERR("Wrote: 0x9ABCDEF0, Read: 0x%08X", sdram[1]);
	}
	
	/* Measure bandwidth */
	test_bandwidth();
	
	LOG_INF("Use shell commands for interactive testing:");
	LOG_INF("  sdram test     - Run all tests");
	LOG_INF("  sdram fill     - Fill with pattern");
	LOG_INF("  sdram dump     - Dump memory");
	LOG_INF("  sdram bandwidth - Measure speed");
	
	/* Keep LED blinking to show we're alive */
	while (1) {
		gpio_pin_toggle_dt(&led);
		k_sleep(K_SECONDS(1));
	}
	
	return 0;
}