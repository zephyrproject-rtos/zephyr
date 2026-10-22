/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>

#include "i2c_target.h"

LOG_MODULE_REGISTER(smbus_api_test, LOG_LEVEL_INF);

/*
 * Functional coverage for smbus_ite_it51xxx.c against a real it51xxx_evb:
 *   host   = smbus2 (channel C)
 *   target = i2c0   (channel A), driven directly by i2c_target.c
 *
 * Test groups:
 *   - protocol coverage: one test per SMBus API entry point
 *   - PEC: positive (write + read) and negative (corrupted read PEC)
 *   - Host Notify: callback registration mechanics
 *   - error paths: address NACK, invalid block params, target NACK
 */

static const struct device *const host = DEVICE_DT_GET(DT_ALIAS(smbus_host));
static const struct device *const target_bus = DEVICE_DT_GET(DT_ALIAS(i2c_target));

/* Address with no target registered on the bus, for NACK/error-path tests */
#define SMBUS_TEST_UNUSED_ADDR 0x70

#define SMBUS_HOST_NOTIFY_MAX_PORT 2

static uint8_t pec_calc(const uint8_t *bytes, size_t len)
{
	uint8_t crc = 0;

	return crc8_ccitt(crc, bytes, len);
}

/* Suite setup / before/ after / teardown */
static void *smbus_api_setup(void)
{
	int ret;

	zassert_true(device_is_ready(host), "SMBus host %s not ready", host->name);

	ret = test_target_start(target_bus);
	zassert_equal(ret, 0, "test_target_start failed: %d", ret);

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER);
	zassert_equal(ret, 0, "smbus_configure(CONTROLLER) failed: %d", ret);

	return NULL;
}

static void smbus_api_before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_target_reset();
}

static void smbus_api_after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Force config back to plain controller mode (no PEC, no Host Notify) after every test */
	(void)smbus_configure(host, SMBUS_MODE_CONTROLLER);
}

static void smbus_api_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)test_target_stop();
}

ZTEST_SUITE(smbus_api, NULL, smbus_api_setup, smbus_api_before, smbus_api_after,
	    smbus_api_teardown);

/* Protocol coverage test */
ZTEST(smbus_api, test_quick_write)
{
	int ret = smbus_quick(host, SMBUS_TEST_TARGET_ADDR, SMBUS_MSG_WRITE);

	zassert_equal(ret, 0, "smbus_quick(WRITE) failed: %d", ret);
	/* Quick has no data */
	zassert_equal(test_target_write_len(), 0, "unexpected data on Quick command");
}

ZTEST(smbus_api, test_byte_write)
{
	int ret = smbus_byte_write(host, SMBUS_TEST_TARGET_ADDR, 0xa5);

	zassert_equal(ret, 0, "smbus_byte_write failed: %d", ret);
	zassert_equal(test_target_write_len(), 1, "wrong byte count");
	zassert_equal(test_target_write_buf()[0], 0xa5, "wrong byte value");
}

ZTEST(smbus_api, test_byte_read)
{
	int ret;
	uint8_t preload[] = {0x5a};
	uint8_t val = 0;

	test_target_set_read_data(preload, sizeof(preload));

	ret = smbus_byte_read(host, SMBUS_TEST_TARGET_ADDR, &val);

	zassert_equal(ret, 0, "smbus_byte_read failed: %d", ret);
	zassert_equal(val, 0x5a, "wrong byte value read back");
}

ZTEST(smbus_api, test_byte_data_write)
{
	int ret = smbus_byte_data_write(host, SMBUS_TEST_TARGET_ADDR, 0x10, 0x77);

	zassert_equal(ret, 0, "smbus_byte_data_write failed: %d", ret);
	zassert_equal(test_target_write_len(), 2, "wrong byte count");
	zassert_equal(test_target_write_buf()[0], 0x10, "wrong command byte");
	zassert_equal(test_target_write_buf()[1], 0x77, "wrong data byte");
}

ZTEST(smbus_api, test_byte_data_read)
{
	int ret;
	uint8_t preload[] = {0x99};
	uint8_t val = 0;

	test_target_set_read_data(preload, sizeof(preload));

	ret = smbus_byte_data_read(host, SMBUS_TEST_TARGET_ADDR, 0x20, &val);

	zassert_equal(ret, 0, "smbus_byte_data_read failed: %d", ret);
	zassert_equal(val, 0x99, "wrong data byte read back");
	zassert_equal(test_target_write_len(), 1, "wrong command-phase length");
	zassert_equal(test_target_write_buf()[0], 0x20, "wrong command byte");
}

ZTEST(smbus_api, test_word_data_write)
{
	int ret = smbus_word_data_write(host, SMBUS_TEST_TARGET_ADDR, 0x30, 0x1234);

	zassert_equal(ret, 0, "smbus_word_data_write failed: %d", ret);
	zassert_equal(test_target_write_len(), 3, "wrong byte count");
	zassert_equal(test_target_write_buf()[0], 0x30, "wrong command byte");
	zassert_equal(test_target_write_buf()[1], 0x34, "wrong LSB");
	zassert_equal(test_target_write_buf()[2], 0x12, "wrong MSB");
}

ZTEST(smbus_api, test_word_data_read)
{
	int ret;
	uint16_t word = 0;
	/* LSB, MSB -> 0x5678 */
	uint8_t preload[] = {0x78, 0x56};

	test_target_set_read_data(preload, sizeof(preload));

	ret = smbus_word_data_read(host, SMBUS_TEST_TARGET_ADDR, 0x40, &word);

	zassert_equal(ret, 0, "smbus_word_data_read failed: %d", ret);
	zassert_equal(word, 0x5678, "wrong word read back");
}

ZTEST(smbus_api, test_pcall)
{
	int ret;
	uint16_t recv = 0;
	/* LSB, MSB -> 0xbeef */
	uint8_t preload[] = {0xef, 0xbe};

	test_target_set_read_data(preload, sizeof(preload));

	ret = smbus_pcall(host, SMBUS_TEST_TARGET_ADDR, 0x50, 0xcafe, &recv);

	zassert_equal(ret, 0, "smbus_pcall failed: %d", ret);
	zassert_equal(recv, 0xbeef, "wrong process-call response");
	zassert_equal(test_target_write_len(), 3, "wrong write-phase length");
	zassert_equal(test_target_write_buf()[0], 0x50, "wrong command byte");
	zassert_equal(test_target_write_buf()[1], 0xfe, "wrong sent LSB");
	zassert_equal(test_target_write_buf()[2], 0xca, "wrong sent MSB");
}

ZTEST(smbus_api, test_block_write)
{
	int ret;
	uint8_t block[SMBUS_BLOCK_BYTES_MAX - 1];

	for (int i = 0; i < SMBUS_BLOCK_BYTES_MAX - 1; i++) {
		block[i] = (uint8_t)(0xc0 + i);
	}

	ret = smbus_block_write(host, SMBUS_TEST_TARGET_ADDR, 0x60, sizeof(block), block);

	zassert_equal(ret, 0, "smbus_block_write failed: %d", ret);
	/* Wire order: [cmd, count, data...] */
	zassert_equal(test_target_write_len(), 2 + sizeof(block), "wrong byte count");
	zassert_equal(test_target_write_buf()[0], 0x60, "wrong command byte");
	zassert_equal(test_target_write_buf()[1], sizeof(block), "wrong byte-count field");
	zassert_mem_equal(&test_target_write_buf()[2], block, sizeof(block), "wrong block data");
}

ZTEST(smbus_api, test_block_read)
{
	int ret;
	/* [count, data...] as the SMBus Block Read wire format expects */
	uint8_t preload[SMBUS_BLOCK_BYTES_MAX];
	uint8_t out_buf[SMBUS_BLOCK_BYTES_MAX];
	uint8_t out_count = 0;

	preload[0] = SMBUS_BLOCK_BYTES_MAX - 1;
	for (int i = 1; i < SMBUS_BLOCK_BYTES_MAX; i++) {
		preload[i] = (uint8_t)(0xa0 + i);
	}
	test_target_set_read_data(preload, sizeof(preload));

	ret = smbus_block_read(host, SMBUS_TEST_TARGET_ADDR, 0x70, &out_count, out_buf);

	zassert_equal(ret, 0, "smbus_block_read failed: %d", ret);
	zassert_equal(out_count, SMBUS_BLOCK_BYTES_MAX - 1, "wrong reported byte count");
	zassert_mem_equal(out_buf, &preload[1], SMBUS_BLOCK_BYTES_MAX - 1,
			  "wrong block data read back");
	/* Write phase of Block Read is just the command byte */
	zassert_equal(test_target_write_len(), 1, "wrong command-phase length");
	zassert_equal(test_target_write_buf()[0], 0x70, "wrong command byte");
}

ZTEST(smbus_api, test_block_pcall_not_supported)
{
	int ret;
	uint8_t snd[] = {0x01};
	uint8_t rcv[SMBUS_BLOCK_BYTES_MAX];
	uint8_t rcv_count = 0;

	ret = smbus_block_pcall(host, SMBUS_TEST_TARGET_ADDR, 0x80, sizeof(snd), snd, &rcv_count,
				rcv);

	/* smbus_it51xxx_block_pcall() is not currently supported */
	zassert_equal(ret, -ENOSYS, "expected -ENOSYS, got %d", ret);
}

/* PEC test */
ZTEST(smbus_api, test_pec_byte_data_write)
{
	int ret;
	uint8_t wire[3];
	uint8_t expected_pec;

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER | SMBUS_MODE_PEC);
	zassert_equal(ret, 0, "smbus_configure(PEC) failed: %d", ret);

	ret = smbus_byte_data_write(host, SMBUS_TEST_TARGET_ADDR, 0x11, 0x22);
	zassert_equal(ret, 0, "smbus_byte_data_write (PEC) failed: %d", ret);

	/* The PEC is calculated for comparison with the PEC obtained after actual transmission */
	wire[0] = (uint8_t)(SMBUS_TEST_TARGET_ADDR << 1) | 0;
	wire[1] = 0x11;
	wire[2] = 0x22;
	expected_pec = pec_calc(wire, sizeof(wire));
	LOG_INF("Byte data write: expected pec=0x%02x", expected_pec);

	zassert_equal(test_target_write_len(), 3, "expected cmd+data+PEC");
	zassert_equal(test_target_write_buf()[2], expected_pec,
		      "PEC mismatch: hw=0x%02x expected=0x%02x", test_target_write_buf()[2],
		      expected_pec);

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER);
	zassert_equal(ret, 0, "smbus_configure(restore) failed: %d", ret);
}

ZTEST(smbus_api, test_pec_byte_data_read)
{
	int ret;
	uint8_t wire[4];
	uint8_t expected_pec;
	uint8_t preload[2];
	uint8_t val = 0;

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER | SMBUS_MODE_PEC);
	zassert_equal(ret, 0, "smbus_configure(PEC) failed: %d", ret);

	/* The PEC is calculated for comparison with the PEC obtained after actual transmission */
	wire[0] = (uint8_t)(SMBUS_TEST_TARGET_ADDR << 1) | 0;
	wire[1] = 0x12;
	wire[2] = (uint8_t)(SMBUS_TEST_TARGET_ADDR << 1) | 1;
	wire[3] = 0x9b;
	expected_pec = pec_calc(wire, sizeof(wire));
	LOG_INF("Byte data read: expected pec=0x%02x", expected_pec);

	preload[0] = wire[3];
	preload[1] = expected_pec;
	test_target_set_read_data(preload, sizeof(preload));

	ret = smbus_byte_data_read(host, SMBUS_TEST_TARGET_ADDR, 0x12, &val);

	zassert_equal(ret, 0, "smbus_byte_data_read (PEC) failed: %d", ret);
	zassert_equal(val, 0x9b, "wrong data byte");

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER);
	zassert_equal(ret, 0, "smbus_configure(restore) failed: %d", ret);
}

ZTEST(smbus_api, test_pec_byte_data_read_corrupt)
{
	int ret;
	/* Deliberately wrong PEC byte */
	uint8_t preload[2] = {0x9b, 0x00};
	uint8_t val = 0;

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER | SMBUS_MODE_PEC);
	zassert_equal(ret, 0, "smbus_configure(PEC) failed: %d", ret);

	test_target_set_read_data(preload, sizeof(preload));

	ret = smbus_byte_data_read(host, SMBUS_TEST_TARGET_ADDR, 0x12, &val);

	zassert_equal(ret, -EIO, "expected -EIO on PEC mismatch, got %d", ret);

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER);
	zassert_equal(ret, 0, "smbus_configure(restore) failed: %d", ret);
}

ZTEST(smbus_api, test_pec_block_read)
{
	int ret;
	uint8_t wire[6];
	uint8_t expected_pec;
	uint8_t preload[4];
	uint8_t out_buf[SMBUS_BLOCK_BYTES_MAX];
	uint8_t len = 0;

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER | SMBUS_MODE_PEC);
	zassert_equal(ret, 0, "smbus_configure(PEC) failed: %d", ret);

	/* The PEC is calculated for comparison with the PEC obtained after actual transmission */
	wire[0] = (uint8_t)(SMBUS_TEST_TARGET_ADDR << 1) | 0;
	wire[1] = 0x12;
	wire[2] = (uint8_t)(SMBUS_TEST_TARGET_ADDR << 1) | 1;
	wire[3] = 0x02;
	wire[4] = 0x1f;
	wire[5] = 0x1e;
	expected_pec = pec_calc(wire, sizeof(wire));
	LOG_INF("Byte data read: expected pec=0x%02x", expected_pec);

	preload[0] = wire[3];
	preload[1] = wire[4];
	preload[2] = wire[5];
	preload[3] = expected_pec;
	test_target_set_read_data(preload, sizeof(preload));

	ret = smbus_block_read(host, SMBUS_TEST_TARGET_ADDR, 0x12, &len, out_buf);
	zassert_equal(ret, 0, "smbus_byte_data_read (PEC) failed: %d", ret);
	zassert_equal(len, 2, "wrong data len");
	/* The comparison data does not include length and PEC data */
	zassert_mem_equal(out_buf, &preload[1], sizeof(preload) - 2, "wrong block data read back");

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER);
	zassert_equal(ret, 0, "smbus_configure(restore) failed: %d", ret);
}

/* Host Notify test */
static void host_notify_cb_handler(const struct device *dev, struct smbus_callback *cb,
				   uint8_t addr)
{
	ARG_UNUSED(cb);

	LOG_INF("%s: Host Notify fired, addr=0x%02x", dev->name, addr);
}

ZTEST(smbus_api, test_host_notify_cb_register)
{
	struct smbus_callback cb = {
		.handler = host_notify_cb_handler,
		.addr = SMBUS_TEST_TARGET_ADDR,
	};
	int ret;

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER | SMBUS_MODE_HOST_NOTIFY);
#if DT_PROP(DT_ALIAS(smbus_host), port_num) > SMBUS_HOST_NOTIFY_MAX_PORT
	zassert_equal(ret, -EIO,
		      "expected -EIO enabling Host Notify on an unsupported port, got %d", ret);
#else
	zassert_equal(ret, 0, "smbus_configure(HOST_NOTIFY) failed: %d", ret);
#endif

	ret = smbus_host_notify_set_cb(host, &cb);
	zassert_equal(ret, 0, "smbus_host_notify_set_cb failed: %d", ret);

	ret = smbus_host_notify_remove_cb(host, &cb);
	zassert_equal(ret, 0, "smbus_host_notify_remove_cb failed: %d", ret);

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER);
	zassert_equal(ret, 0, "smbus_configure(restore) failed: %d", ret);
}

/* SMBALERT# test */
static void smbalert_cb_handler(const struct device *dev, struct smbus_callback *cb, uint8_t addr)
{
	ARG_UNUSED(cb);

	LOG_INF("%s: SMBALERT# fired, addr=0x%02x", dev->name, addr);
}

ZTEST(smbus_api, test_smbalert_cb_register)
{
	struct smbus_callback cb = {
		.handler = smbalert_cb_handler,
		.addr = SMBUS_TEST_TARGET_ADDR,
	};
	int ret;
	uint32_t config = 0;

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER | SMBUS_MODE_SMBALERT);
	zassert_equal(ret, 0, "smbus_configure(SMBALERT) failed: %d", ret);

	ret = smbus_get_config(host, &config);
	zassert_equal(ret, 0, "smbus_get_config failed: %d", ret);
	zassert_true(config & SMBUS_MODE_SMBALERT, "SMBALERT bit not reflected in config");

	ret = smbus_smbalert_set_cb(host, &cb);
	zassert_equal(ret, 0, "smbus_smbalert_set_cb failed: %d", ret);

	ret = smbus_smbalert_remove_cb(host, &cb);
	zassert_equal(ret, 0, "smbus_smbalert_remove_cb failed: %d", ret);

	ret = smbus_configure(host, SMBUS_MODE_CONTROLLER);
	zassert_equal(ret, 0, "smbus_configure(restore) failed: %d", ret);
}

/* Generic API-contract / error paths */
ZTEST(smbus_api, test_get_config_null)
{
	int ret = smbus_get_config(host, NULL);

	zassert_equal(ret, -EIO, "expected -EIO for NULL config pointer, got %d", ret);
}

ZTEST(smbus_api, test_error_address_nack)
{
	/* No target registered at this address: expect a NACK -> -EIO */
	int ret = smbus_byte_write(host, SMBUS_TEST_UNUSED_ADDR, 0x00);

	zassert_equal(ret, -EIO, "expected -EIO addressing an absent peripheral, got %d", ret);
}

ZTEST(smbus_api, test_error_block_write_invalid_count)
{
	uint8_t buf[1] = {0};
	int ret;

	/* count == 0 and count > SMBUS_BLOCK_BYTES_MAX */
	ret = smbus_block_write(host, SMBUS_TEST_TARGET_ADDR, 0x60, 0, buf);
	zassert_equal(ret, -EINVAL, "expected -EINVAL for count=0, got %d", ret);

	ret = smbus_block_write(host, SMBUS_TEST_TARGET_ADDR, 0x60, SMBUS_BLOCK_BYTES_MAX + 1, buf);
	zassert_equal(ret, -EINVAL, "expected -EINVAL for count>max, got %d", ret);
}

ZTEST(smbus_api, test_error_block_write_null_buf)
{
	/*
	 * A valid count with a NULL buffer passes the subsystem-level check
	 * but is rejected by the driver itself.
	 */
	int ret = smbus_block_write(host, SMBUS_TEST_TARGET_ADDR, 0x60, 4, NULL);

	zassert_equal(ret, -EIO, "expected -EIO for NULL buffer, got %d", ret);
}
