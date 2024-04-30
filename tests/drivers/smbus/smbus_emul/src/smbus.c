/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/smbus.h>

#include "emul.h"

#define PERIPH_ADDR	0x10

static uint8_t mock_sys_in8(io_port_t port)
{
	return emul_in8(port);
}

static void mock_sys_out8(uint8_t data, io_port_t port)
{
	emul_out8(data, port);
}

static uint32_t mock_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	return emul_pci_read(reg);
}

#if defined(PCIE_CONF_WRITE)
static void mock_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	emul_pci_write(bdf, reg, data);
}

#define pcie_conf_write(bdf, reg, val)	mock_conf_write(bdf, reg, val)
#endif /* PCIE_CONF_WRITE */

/* Redefine PCIE access */
#define pcie_conf_read(bdf, reg)	mock_conf_read(bdf, reg)

/* Redefine sys_in function */
#define sys_in8(port)			mock_sys_in8(port)
#define sys_out8(data, port)		mock_sys_out8(data, port)

#define CONFIG_SMBUS_INTEL_PCH_ACCESS_IO
#define device_map(a, b, c, d)
#define pcie_probe(bdf, id)	1
#define pcie_set_cmd(a, b, c)

#define SMBUS_EMUL	"smbus_emul"

#ifdef PERIPHERAL_INT
#define CONFIG_SMBUS_INTEL_PCH_SMBALERT		1
#define CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY	1
#endif

#include "intel_pch_smbus.c"

void run_isr(enum emul_isr_type type)
{
	const struct device *const dev = device_get_binding(SMBUS_EMUL);

	switch (type) {
	case EMUL_SMBUS_INTR:
		emul_set_io(emul_get_io(PCH_SMBUS_HSTS) |
			    PCH_SMBUS_HSTS_INTERRUPT, PCH_SMBUS_HSTS);
		break;
	case EMUL_SMBUS_SMBALERT:
		emul_set_io(emul_get_io(PCH_SMBUS_HSTS) |
			    PCH_SMBUS_HSTS_SMB_ALERT, PCH_SMBUS_HSTS);
		break;
	case EMUL_SMBUS_HOST_NOTIFY:
		emul_set_io(emul_get_io(PCH_SMBUS_SSTS)|
			    PCH_SMBUS_SSTS_HNS, PCH_SMBUS_SSTS);
		peripheral_handle_host_notify();
		break;
	default:
		break;
	}

	smbus_isr(dev);
}

static void config_function(const struct device *dev)
{
	TC_PRINT("Emulator device configuration\n");
}
static struct pch_data smbus_data;
/* Zero initialized, dummy device does not care about pcie ids */
static struct pcie_dev pcie_params;
static struct pch_config pch_config_data = {
	.config_func = config_function,
	.pcie = &pcie_params,
};

DEVICE_DEFINE(dummy_driver, SMBUS_EMUL, &pch_smbus_init,
	      NULL, &smbus_data, &pch_config_data, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);

ZTEST(test_smbus_emul, test_byte)
{
	const struct device *const dev = device_get_binding(SMBUS_EMUL);
	uint8_t snd_byte, rcv_byte;
	int ret;

	zassert_not_null(dev, "Device not found");

	ret = smbus_quick(dev, PERIPH_ADDR, 1);
	zassert_ok(ret, "SMBus Quick failed");

	snd_byte = sys_rand8_get();

	ret = smbus_byte_write(dev, PERIPH_ADDR, snd_byte);
	zassert_ok(ret, "SMBus Byte Write failed");

	ret = smbus_byte_read(dev, PERIPH_ADDR, &rcv_byte);
	zassert_ok(ret, "SMBus Byte Read failed");

	zassert_equal(snd_byte, rcv_byte, "Data mismatch");

	ret = smbus_byte_data_write(dev, PERIPH_ADDR, 0, snd_byte);
	zassert_ok(ret, "SMBus Byte Data Write failed");

	ret = smbus_byte_data_read(dev, PERIPH_ADDR, 0, &rcv_byte);
	zassert_ok(ret, "SMBus Byte Data Read failed");

	zassert_equal(snd_byte, rcv_byte, "Data mismatch");
}

ZTEST(test_smbus_emul, test_word)
{
	const struct device *const dev = device_get_binding(SMBUS_EMUL);
	uint16_t snd_word, rcv_word;
	uint8_t snd_byte;
	int ret;

	zassert_not_null(dev, "Device not found");

	snd_word = sys_rand16_get();

	ret = smbus_word_data_write(dev, PERIPH_ADDR, 0, snd_word);
	zassert_ok(ret, "SMBus Word Data Write failed");

	ret = smbus_word_data_read(dev, PERIPH_ADDR, 0, &rcv_word);
	zassert_ok(ret, "SMBus Byte Data Read failed");

	zassert_equal(snd_word, rcv_word, "Data mismatch");

	/* Test 2 byte writes following word read */

	snd_byte = sys_rand8_get();

	ret = smbus_byte_data_write(dev, PERIPH_ADDR, 0, snd_byte);
	zassert_ok(ret, "SMBus Byte Data Write failed");
	ret = smbus_byte_data_write(dev, PERIPH_ADDR, 1, snd_byte);
	zassert_ok(ret, "SMBus Byte Data Write failed");

	ret = smbus_word_data_read(dev, PERIPH_ADDR, 0, &rcv_word);
	zassert_ok(ret, "SMBus Byte Data Read failed");

	zassert_equal(snd_byte << 8 | snd_byte, rcv_word, "Data mismatch");
}

ZTEST(test_smbus_emul, test_proc_call)
{
	const struct device *const dev = device_get_binding(SMBUS_EMUL);
	uint16_t snd_word, rcv_word;
	int ret;

	zassert_not_null(dev, "Device not found");

	snd_word = sys_rand16_get();
	zassert_not_equal(snd_word, 0, "Random number generator misconfgured");

	ret = smbus_pcall(dev, PERIPH_ADDR, 0x0, snd_word, &rcv_word);
	zassert_ok(ret, "SMBus Proc Call failed");

	/* Our emulated Proc Call swaps bytes */
	zassert_equal(snd_word, BSWAP_16(rcv_word), "Data mismatch");
}

ZTEST(test_smbus_emul, test_block)
{
	const struct device *const dev = device_get_binding(SMBUS_EMUL);
	uint8_t snd_block[SMBUS_BLOCK_BYTES_MAX];
	uint8_t rcv_block[SMBUS_BLOCK_BYTES_MAX];
	uint8_t snd_count, rcv_count;
	int ret;

	zassert_not_null(dev, "Device not found");

	sys_rand_get(snd_block, sizeof(snd_block));

	snd_count = sizeof(snd_block);

	ret = smbus_block_write(dev, PERIPH_ADDR, 0, snd_count, snd_block);
	zassert_ok(ret, "SMBUS write block failed, ret %d", ret);

	ret = smbus_block_read(dev, PERIPH_ADDR, 0, &rcv_count, rcv_block);
	zassert_ok(ret, "SMBUS read block failed, ret %d", ret);

	zassert_equal(snd_count, rcv_count, "Block count differs");
	zassert_true(!memcmp(snd_block, rcv_block, rcv_count),
		     "Data mismatch");
}

ZTEST(test_smbus_emul, test_block_pcall)
{
	const struct device *const dev = device_get_binding(SMBUS_EMUL);
	uint8_t snd_block[SMBUS_BLOCK_BYTES_MAX];
	uint8_t rcv_block[SMBUS_BLOCK_BYTES_MAX];
	uint8_t snd_count, rcv_count;
	int ret;

	zassert_not_null(dev, "Device not found");

	sys_rand_get(snd_block, sizeof(snd_block));

	snd_count = SMBUS_BLOCK_BYTES_MAX / 2;
	ret = smbus_block_pcall(dev, PERIPH_ADDR, 0, snd_count, snd_block,
				&rcv_count, rcv_block);
	zassert_ok(ret, "SMBUS block pcall failed, ret %d", ret);
	zassert_equal(snd_count, rcv_count, "Block count differs");

	/**
	 * Verify that our emulated peripheral swapped bytes in the block
	 * buffer
	 */
	for (int i = 0; i < rcv_count; i++) {
		zassert_equal(snd_block[i], rcv_block[rcv_count - (i + 1)],
			      "Data mismatch, not swapped");

	}
}

/* SMBALERT handling */

/* False by default */
bool smbalert_handled;

static void smbalert_cb(const struct device *dev, struct smbus_callback *cb,
			uint8_t addr)
{
	LOG_DBG("SMBALERT callback");

	smbalert_handled = true;
}

struct smbus_callback smbalert_callback = {
	.handler = smbalert_cb,
	.addr = PERIPH_ADDR,
};

/* Host Notify handling */

/* False by default */
bool notify_handled;

static void notify_cb(const struct device *dev, struct smbus_callback *cb,
		      uint8_t addr)
{
	LOG_DBG("Notify callback");

	notify_handled = true;
}

struct smbus_callback notify_callback = {
	.handler = notify_cb,
	.addr = PERIPH_ADDR,
};

/* Setup peripheral SMBus device on a bus */

struct smbus_peripheral peripheral = {
	.addr = PERIPH_ADDR,
	.smbalert = true,
	.host_notify = true,
};

ZTEST(test_smbus_emul, test_alert)
{
	const struct device *const dev = device_get_binding(SMBUS_EMUL);
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_SMBUS_INTEL_PCH_SMBALERT);

	zassert_not_null(dev, "Device not found");

	/* Try to remove not existing callback */
	ret = smbus_smbalert_remove_cb(dev, &smbalert_callback);
	zassert_equal(ret, -ENOENT, "Callback remove failed");

	/* Set callback */
	ret = smbus_smbalert_set_cb(dev, &smbalert_callback);
	zassert_ok(ret, "Callback set failed");

	/* Emulate SMBus alert from peripheral device */
	peripheral_clear_smbalert(&peripheral);
	smbalert_handled = false;

	/* Run without configure smbalert */
	run_isr(EMUL_SMBUS_SMBALERT);

	/* Wait for delayed work handled */
	k_sleep(K_MSEC(100));

	/* Verify that smbalert is NOT handled */
	zassert_false(smbalert_handled, "smbalert is not handled");

	/* Now enable smbalert */
	ret = smbus_configure(dev, SMBUS_MODE_CONTROLLER | SMBUS_MODE_SMBALERT);
	zassert_ok(ret, "Configure failed");

	/* Emulate SMBus alert again */
	run_isr(EMUL_SMBUS_SMBALERT);

	/* Wait for delayed work handled */
	k_sleep(K_MSEC(100));

	/* Verify that smbalert is not handled */
	zassert_true(smbalert_handled, "smbalert is not handled");
}

ZTEST(test_smbus_emul, test_host_notify)
{
	const struct device *const dev = device_get_binding(SMBUS_EMUL);
	int ret;

	Z_TEST_SKIP_IFNDEF(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY);

	zassert_not_null(dev, "Device not found");

	/* Try to remove not existing callback */
	ret = smbus_host_notify_remove_cb(dev, &notify_callback);
	zassert_equal(ret, -ENOENT, "Callback remove failed");

	/* Set callback */
	ret = smbus_host_notify_set_cb(dev, &notify_callback);
	zassert_ok(ret, "Callback set failed");

	/* Emulate SMBus alert from peripheral device */
	notify_handled = false;

	/* Run without configuring Host Notify */
	run_isr(EMUL_SMBUS_HOST_NOTIFY);

	/* Wait for delayed work handled */
	k_sleep(K_MSEC(100));

	/* Verify that smbalert is NOT handled */
	zassert_false(notify_handled, "smbalert is not handled");

	/* Now enable smbalert */
	ret = smbus_configure(dev,
			      SMBUS_MODE_CONTROLLER | SMBUS_MODE_HOST_NOTIFY);
	zassert_ok(ret, "Configure failed");

	/* Emulate SMBus alert again */
	run_isr(EMUL_SMBUS_HOST_NOTIFY);

	/* Wait for delayed work handled */
	k_sleep(K_MSEC(100));

	/* Verify that smbalert is handled */
	zassert_true(notify_handled, "smbalert is not handled");
}

/* Test setup function */
static void *smbus_emul_setup(void)
{
	emul_register_smbus_peripheral(&peripheral);

	return NULL;
}

ZTEST_SUITE(test_smbus_emul, NULL, smbus_emul_setup, NULL, NULL, NULL);
