/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_smbus_basic
 * @{
 * @defgroup t_smbus
 * @brief TestPurpose: verify SMBUS can read and write
 * @}
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/smbus.h>

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(smbus0), okay),
	     "SMBus node is disabled!");

/* Qemu q35 has default emulated EEPROM-like devices */
#define QEMU_SMBUS_EEPROM_ADDR	0x50
#define QEMU_SMBUS_EEPROM_SIZE	256

/**
 * The test is run in userspace only if CONFIG_USERSPACE option is
 * enabled, otherwise it is the same as ZTEST()
 */
ZTEST_USER(test_smbus_qemu, test_smbus_api_read_write)
{
	int ret;

	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(smbus0));

	zassert_true(device_is_ready(dev), "Device is not ready");

	/* Test SMBus quick */
	ret = smbus_quick(dev, QEMU_SMBUS_EEPROM_ADDR, SMBUS_MSG_WRITE);
	zassert_ok(ret, "SMBUS Quick W failed, ret %d", ret);

	ret = smbus_quick(dev, QEMU_SMBUS_EEPROM_ADDR, SMBUS_MSG_READ);
	zassert_ok(ret, "SMBUS Quick R failed, ret %d", ret);

	/* Test SMBus Read / Write Byte Data */
	for (uint16_t addr = 0; addr < QEMU_SMBUS_EEPROM_SIZE; addr++) {
		/* Randomize buffer */
		uint8_t send_byte = (uint8_t)addr;
		uint8_t recv_byte;

		ret = smbus_byte_data_write(dev, QEMU_SMBUS_EEPROM_ADDR, addr,
					    send_byte);
		zassert_ok(ret, "SMBUS write byte data failed, ret %d", ret);

		ret = smbus_byte_data_read(dev, QEMU_SMBUS_EEPROM_ADDR, addr,
					   &recv_byte);
		zassert_ok(ret, "SMBUS read byte data failed, ret %d", ret);

		zassert_equal(send_byte, recv_byte, "SMBUS data compare fail");
	}

	/* Test SMBus Read / Write Word Data */
	for (uint16_t addr = 0; addr < QEMU_SMBUS_EEPROM_SIZE; addr += 2) {
		uint16_t send_word = addr;
		uint16_t recv_word;

		ret = smbus_word_data_write(dev, QEMU_SMBUS_EEPROM_ADDR, addr,
					    send_word);
		zassert_ok(ret, "SMBUS write word data failed, ret %d", ret);

		ret = smbus_word_data_read(dev, QEMU_SMBUS_EEPROM_ADDR, addr,
					   &recv_word);
		zassert_ok(ret, "SMBUS read word data failed, ret %d", ret);

		zassert_equal(send_word, recv_word, "SMBUS data compare fail");
	}

	/* Test SMBus Read / Write Byte on special Qemu SMBus peripheral */
	for (uint16_t addr = 0; addr < QEMU_SMBUS_EEPROM_SIZE; addr++) {
		/* Randomize buffer */
		uint8_t send_byte = (uint8_t)addr;
		uint8_t recv_byte;

		/* Write byte data to EEPROM device */
		ret = smbus_byte_data_write(dev, QEMU_SMBUS_EEPROM_ADDR, addr,
					    send_byte);
		zassert_ok(ret, "SMBUS write byte data failed, ret %d", ret);

		/**
		 * Reading is done through executing two consequitive
		 * operations: write, which sets offset, followed by read, which
		 * reads data from given offset
		 */

		ret = smbus_byte_write(dev, QEMU_SMBUS_EEPROM_ADDR, addr);
		zassert_ok(ret, "SMBUS write byte failed, ret %d", ret);

		ret = smbus_byte_read(dev, QEMU_SMBUS_EEPROM_ADDR, &recv_byte);
		zassert_ok(ret, "SMBUS read byte failed, ret %d", ret);
		zassert_equal(send_byte, recv_byte, "SMBUS data compare fail");
	}

	/**
	 * The Qemu SMBus implementation does not always correctly
	 * emulate SMBus Block protocol, however it is good enough
	 * to test Block Write followed by Block Read
	 */

	/* Test SMBus Block Write / Block Read */
	for (uint16_t addr = 0; addr < QEMU_SMBUS_EEPROM_SIZE;
	     addr += SMBUS_BLOCK_BYTES_MAX) {
		uint8_t send_block[SMBUS_BLOCK_BYTES_MAX];
		uint8_t recv_block[SMBUS_BLOCK_BYTES_MAX];
		uint8_t count;

		/* Poor man randomize ;) */
		for (int i = 0; i < sizeof(send_block); i++) {
			send_block[i] = addr + i;
		}
		ret = smbus_block_write(dev, QEMU_SMBUS_EEPROM_ADDR, addr,
					sizeof(send_block), send_block);
		zassert_ok(ret, "SMBUS write block failed, ret %d", ret);

		ret = smbus_block_read(dev, QEMU_SMBUS_EEPROM_ADDR, addr,
				       &count, recv_block);
		zassert_ok(ret, "SMBUS read block failed, ret %d", ret);

		zassert_equal(count, SMBUS_BLOCK_BYTES_MAX,
			      "Read wrong %d of bytes", count);

		zassert_true(!memcmp(send_block, recv_block, count),
			     "Read / Write data differs");
	}
}

/* Setup is needed for userspace access */
static void *smbus_test_setup(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(smbus0));

	zassert_true(device_is_ready(dev), "Device is not ready");

	k_object_access_grant(dev, k_current_get());

	return NULL;
}

ZTEST_SUITE(test_smbus_qemu, NULL, smbus_test_setup, NULL, NULL, NULL);
