/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Interactive shell test suite for 'flash' command
 *
 */

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/device.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

/* configuration derived from DT */
#ifdef CONFIG_ARCH_POSIX
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_0)
#else
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_sim_0)
#endif /* CONFIG_ARCH_POSIX */
#define FLASH_SIMULATOR_BASE_OFFSET DT_REG_ADDR(SOC_NV_FLASH_NODE)

/* Test 'flash read' shell command */
static void test_flash_read(void)
{
	/* To keep the test simple, just compare against known data */
	char *const lines[] = {
		"00000000: 41 42 43 44 45 46 47 48  49 4a 4b 4c 4d 4e 4f 50 |ABCDEFGH IJKLMNOP|",
		"00000010: 51 52 53 54 55 56 57 58  59 5a 5b 5c 5d 5e 5f 60 |QRSTUVWX YZ[\\]^_`|",
		"00000020: 61 62 63                                         |abc              |",
	};
	const struct shell *shell = shell_backend_dummy_get_ptr();
	const struct device *flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	const char *buf;
	const int test_base = FLASH_SIMULATOR_BASE_OFFSET;
	const int test_size = 0x24;  /* 32-alignment required */
	uint8_t data[test_size];
	size_t size;
	int ret;
	int i;

	for (i = 0; i < test_size; i++) {
		data[i] = 'A' + i;
	}

	zassert_true(device_is_ready(flash_dev),
		     "Simulated flash driver not ready");

	ret = flash_write(flash_dev, test_base, data, test_size);
	zassert_equal(0, ret, "flash_write() failed: %d", ret);

	ret = shell_execute_cmd(NULL, "flash read 0 23");
	zassert_equal(0, ret, "flash read failed: %d", ret);

	buf = shell_backend_dummy_get_output(shell, &size);
	for (i = 0; i < ARRAY_SIZE(lines); i++) {
		/* buf contains all the bytes that goes through the shell
		 * backend interface including escape codes, NL and CR.
		 * Function strstr finds place in the buffer where interesting
		 * data is located.
		 */
		zassert_true(strstr(buf, lines[i]), "Line: %d not found", i);
	}
}

void test_main(void)
{
	/* Let the shell backend initialize. */
	k_usleep(10);

	ztest_test_suite(shell_flash_test_suite,
			 ztest_unit_test(test_flash_read)
			);

	ztest_run_test_suite(shell_flash_test_suite);
}
