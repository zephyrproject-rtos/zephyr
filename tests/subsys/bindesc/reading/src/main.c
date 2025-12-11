/*
 * Copyright 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/bindesc.h>
#include <zephyr/drivers/flash.h>

#ifdef CONFIG_ARCH_POSIX
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_0)
#else
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_sim_0)
#endif /* CONFIG_ARCH_POSIX */

#if (defined(CONFIG_ARCH_POSIX) || defined(CONFIG_BOARD_QEMU_X86))
static const struct device *const flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
#else
static const struct device *const flash_dev = DEVICE_DT_GET(DT_NODELABEL(sim_flash_controller));
#endif

#define FLASH_SIMULATOR_ERASE_UNIT DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

static __aligned(BINDESC_ALIGNMENT) const uint8_t descriptors[] = {
	0x46, 0x60, 0xa4, 0x7e, 0x5a, 0x3e, 0x86, 0xb9, /* magic */
	0x00, 0x18, 0x06, 0x00, /* tag: 0x1800 (app version string), length: 0x0006 */
	0x31, 0x2e, 0x30, 0x2e, 0x30, 0x00, /* "1.0.0" */
	0x00, 0x00, /* padding */
	0x01, 0x1b, 0x04, 0x00, /* tag: 0x1b01 (compiler name), length: 0x0004 */
	0x47, 0x4e, 0x55, 0x00, /* "GNU" */
	0x02, 0x1b, 0x07, 0x00, /* tag: 0x1b02 (compiler version), length: 0x0007 */
	0x31, 0x32, 0x2e, 0x32, 0x2e, 0x30, 0x00, /* "12.2.0" */
	0x00, /* padding */
	0x00, 0x19, 0x07, 0x00, /* tag: 0x1900 (kernel version string), length: 0x0007 */
	0x33, 0x2e, 0x35, 0x2e, 0x39, 0x39, 0x00, /* "3.5.99" */
	0x00, /* padding */
	0xff, 0xff, 0x00, 0x00, /* tag: 0xffff (descriptors end), length: 0x0000 */
};

static void *test_setup(void)
{
	flash_erase(flash_dev, 0, FLASH_SIMULATOR_ERASE_UNIT);
	flash_write(flash_dev, 0, descriptors, sizeof(descriptors));

	return NULL;
}

static void test_bindesc_read(struct bindesc_handle *handle)
{
	const char *result;

	bindesc_find_str(handle, BINDESC_ID_KERNEL_VERSION_STRING, &result);
	zassert_mem_equal("3.5.99", result, sizeof("3.5.99"));

	bindesc_find_str(handle, BINDESC_ID_APP_VERSION_STRING, &result);
	zassert_mem_equal("1.0.0", result, sizeof("1.0.0"));

	bindesc_find_str(handle, BINDESC_ID_C_COMPILER_NAME, &result);
	zassert_mem_equal("GNU", result, sizeof("GNU"));

	bindesc_find_str(handle, BINDESC_ID_C_COMPILER_VERSION, &result);
	zassert_mem_equal("12.2.0", result, sizeof("12.2.0"));
}

ZTEST(bindesc_read, test_bindesc_read_from_flash)
{
	struct bindesc_handle handle;

	bindesc_open_flash(&handle, 0, flash_dev);

	test_bindesc_read(&handle);
}

ZTEST(bindesc_read, test_bindesc_read_from_ram)
{
	struct bindesc_handle handle;

	bindesc_open_ram(&handle, descriptors, sizeof(descriptors));

	test_bindesc_read(&handle);
}

ZTEST_SUITE(bindesc_read, NULL, test_setup, NULL, NULL, NULL);
