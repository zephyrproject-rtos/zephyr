/*
 * Copyright (c) 2026, Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/fuel_gauge/hy4245.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

/* Data flash locations used by the tests */
#define SUBCLASS_CALIBRATION 0x02 /* device specific, skipped by config image programming */
#define SUBCLASS_SHARED      0x03 /* block 1 has device specific bytes 7 and 8 */
#define SUBCLASS_MFG_INFO    0x20

#define BLOCK_SIZE   HY4245_DATA_FLASH_BLOCK_SIZE
#define ELEMENT_SIZE HY4245_CONFIG_IMAGE_ELEMENT_SIZE

struct hy4245_fixture {
	const struct device *dev;
};

static void *hy4245_setup(void)
{
	static ZTEST_DMEM struct hy4245_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(hycon_hy4245);
	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Fuel gauge not found");

	return &fixture;
}

static void hy4245_before(void *f)
{
	struct hy4245_fixture *fixture = f;
	union fuel_gauge_prop_val val = {};

	/* Every test starts with a sealed gauge outside calibration mode */
	if (fuel_gauge_get_prop(fixture->dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE, &val) == 0 &&
	    val.custom_bool) {
		val.custom_bool = false;
		(void)fuel_gauge_set_prop(fixture->dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE, val);
	}
}

static uint32_t get_uint(const struct device *dev, fuel_gauge_prop_t prop)
{
	union fuel_gauge_prop_val val = {};

	zassert_ok(fuel_gauge_get_prop(dev, prop, &val), "reading property %u failed", prop);

	return val.custom_uint;
}

static bool get_bool(const struct device *dev, fuel_gauge_prop_t prop)
{
	union fuel_gauge_prop_val val = {};

	zassert_ok(fuel_gauge_get_prop(dev, prop, &val), "reading property %u failed", prop);

	return val.custom_bool;
}

static int set_bool(const struct device *dev, fuel_gauge_prop_t prop, bool value)
{
	return fuel_gauge_set_prop(dev, prop, (union fuel_gauge_prop_val){.custom_bool = value});
}

static void enter_calibration_mode(const struct device *dev)
{
	zassert_ok(set_bool(dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE, true));
	zassert_true(get_bool(dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE));
}

static void leave_calibration_mode(const struct device *dev)
{
	zassert_ok(set_bool(dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE, false));
	zassert_false(get_bool(dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE));
}

static void read_block(const struct device *dev, uint8_t subclass, uint8_t block, uint8_t *out)
{
	struct hy4245_data_flash_block blk = {
		.subclass = subclass,
		.block = block,
		.len = BLOCK_SIZE,
	};

	zassert_ok(fuel_gauge_get_buffer_prop(dev, HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK, &blk,
					      sizeof(blk)),
		   "reading block 0x%02x/%u failed", subclass, block);
	memcpy(out, blk.data, BLOCK_SIZE);
}

static void write_block(const struct device *dev, uint8_t subclass, uint8_t block,
			const uint8_t *in, uint8_t len)
{
	struct hy4245_data_flash_block blk = {
		.subclass = subclass,
		.block = block,
		.len = len,
	};

	memcpy(blk.data, in, len);
	zassert_ok(fuel_gauge_set_buffer_prop(dev, HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK, &blk,
					      sizeof(blk)),
		   "writing block 0x%02x/%u failed", subclass, block);
}

static void fill_element(uint8_t *image, size_t index, uint8_t subclass, uint8_t block,
			 uint8_t value)
{
	uint8_t *element = &image[index * ELEMENT_SIZE];

	element[0] = subclass;
	element[1] = block;
	memset(&element[2], value, BLOCK_SIZE);
}

ZTEST_USER_F(hy4245, test_get_props__standard_values)
{
	const fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_VOLTAGE_UV,
		FUEL_GAUGE_CURRENT_UA,
		FUEL_GAUGE_AVG_CURRENT_UA,
		FUEL_GAUGE_TEMPERATURE_DK,
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT,
		FUEL_GAUGE_REMAINING_CAPACITY_UAH,
		FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH,
		FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS,
		FUEL_GAUGE_RUNTIME_TO_FULL_MINS,
		FUEL_GAUGE_CHARGE_VOLTAGE_UV,
		FUEL_GAUGE_CHARGE_CURRENT_UA,
		FUEL_GAUGE_DESIGN_CAPACITY,
		FUEL_GAUGE_STATE_OF_HEALTH,
	};
	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)] = {};

	zassert_ok(fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props)));

	/* Fixed readings of the emulator, converted to the API units */
	zassert_equal(vals[0].voltage_uv, 3700000);
	zassert_equal(vals[1].current_ua, -1234000);
	zassert_equal(vals[2].avg_current_ua, -250000);
	zassert_equal(vals[3].temperature_dk, 2981);
	zassert_equal(vals[4].relative_state_of_charge_pct, 55);
	zassert_equal(vals[5].remaining_capacity_uah, 1200000);
	zassert_equal(vals[6].full_charge_capacity_uah, 2400000);
	zassert_equal(vals[7].runtime_to_empty_mins, 300);
	zassert_equal(vals[8].runtime_to_full_mins, 120);
	zassert_equal(vals[9].chg_voltage_uv, 4200000);
	zassert_equal(vals[10].chg_current_ua, 1500000);
	zassert_equal(vals[11].design_cap, 2500);
	zassert_equal(vals[12].state_of_health, 99);
}

ZTEST_USER_F(hy4245, test_unsupported_operations)
{
	union fuel_gauge_prop_val val = {};

	zassert_equal(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CYCLE_COUNT, &val), -ENOTSUP);
	zassert_equal(fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_CUSTOM_BEGIN + 100, &val),
		      -ENOTSUP);
	zassert_equal(fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_VOLTAGE_UV, val), -ENOTSUP);
	zassert_equal(fuel_gauge_battery_cutoff(fixture->dev), -ENOSYS);
}

ZTEST_USER_F(hy4245, test_status_and_version_props)
{
	uint32_t status, flags, cfg_a, checksum;

	zassert_equal(get_uint(fixture->dev, HY4245_FUEL_GAUGE_FIRMWARE_VERSION), 0x2006);
	zassert_equal(get_uint(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_VERSION), 0x0003);
	zassert_equal(get_uint(fixture->dev, HY4245_FUEL_GAUGE_SAFETY_STATUS), 0);
	zassert_equal(get_uint(fixture->dev, HY4245_FUEL_GAUGE_LIFETIME_OVER_TEMPERATURE_MINS), 42);

	flags = get_uint(fixture->dev, HY4245_FUEL_GAUGE_FLAGS);
	zassert_true(flags & HY4245_FLAGS_DSG, "gauge not discharging");
	zassert_false(flags & HY4245_FLAGS_CHG, "gauge charging");

	/* Data sheet defaults of OperationCfgA() */
	cfg_a = get_uint(fixture->dev, HY4245_FUEL_GAUGE_OPERATION_CONFIG_A);
	zassert_true(cfg_a & HY4245_OPERATION_CONFIG_A_TEMPS, "external temperature sensor");
	zassert_true(cfg_a & HY4245_OPERATION_CONFIG_A_SLEEP, "sleep mode");
	zassert_false(cfg_a & HY4245_OPERATION_CONFIG_A_CELL0, "single cell");

	/* Sealed and not in calibration mode after the reset done by the before hook */
	status = get_uint(fixture->dev, HY4245_FUEL_GAUGE_CONTROL_STATUS);
	zassert_true(status & HY4245_CONTROL_STATUS_SS, "gauge not sealed");
	zassert_false(status & HY4245_CONTROL_STATUS_BCA, "calibration mode active");

	/* The data flash checksum is stable while nothing is written */
	enter_calibration_mode(fixture->dev);
	checksum = get_uint(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_CHECKSUM);
	zassert_not_equal(checksum, 0);
	zassert_equal(get_uint(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_CHECKSUM), checksum);
	leave_calibration_mode(fixture->dev);
}

ZTEST_USER_F(hy4245, test_flash_update_enable)
{
	if (!get_bool(fixture->dev, HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE)) {
		zassert_false(get_uint(fixture->dev, HY4245_FUEL_GAUGE_OPERATION_CONFIG_A) &
			      HY4245_OPERATION_CONFIG_A_UPD_EN);
		zassert_ok(set_bool(fixture->dev, HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE, true));
	}
	zassert_true(get_bool(fixture->dev, HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE));
	zassert_true(get_uint(fixture->dev, HY4245_FUEL_GAUGE_OPERATION_CONFIG_A) &
		     HY4245_OPERATION_CONFIG_A_UPD_EN);

	/* Enabling twice is fine, disabling is not possible */
	zassert_ok(set_bool(fixture->dev, HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE, true));
	zassert_equal(set_bool(fixture->dev, HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE, false),
		      -ENOTSUP);
}

ZTEST_USER_F(hy4245, test_calibration_mode_session)
{
	uint32_t status;

	zassert_false(get_bool(fixture->dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE));

	enter_calibration_mode(fixture->dev);
	status = get_uint(fixture->dev, HY4245_FUEL_GAUGE_CONTROL_STATUS);
	zassert_false(status & HY4245_CONTROL_STATUS_SS, "gauge still sealed");
	zassert_true(status & HY4245_CONTROL_STATUS_BCA, "calibration mode not active");

	/* Entering again is idempotent */
	zassert_ok(set_bool(fixture->dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE, true));
	zassert_true(get_bool(fixture->dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE));

	/* Leaving resets the gauge, which seals it again */
	leave_calibration_mode(fixture->dev);
	status = get_uint(fixture->dev, HY4245_FUEL_GAUGE_CONTROL_STATUS);
	zassert_true(status & HY4245_CONTROL_STATUS_SS, "gauge not sealed after reset");
	zassert_false(status & HY4245_CONTROL_STATUS_BCA,
		      "calibration mode still active after reset");
}

ZTEST_USER_F(hy4245, test_reset)
{
	zassert_equal(set_bool(fixture->dev, HY4245_FUEL_GAUGE_RESET, false), -EINVAL);

	enter_calibration_mode(fixture->dev);
	zassert_ok(set_bool(fixture->dev, HY4245_FUEL_GAUGE_RESET, true));
	zassert_false(get_bool(fixture->dev, HY4245_FUEL_GAUGE_CALIBRATION_MODE));
	zassert_true(get_uint(fixture->dev, HY4245_FUEL_GAUGE_CONTROL_STATUS) &
		     HY4245_CONTROL_STATUS_SS);
}

ZTEST_USER_F(hy4245, test_buffer_props_require_calibration_mode)
{
	uint8_t data[BLOCK_SIZE] = {0};
	struct hy4245_data_flash_block blk = {
		.subclass = SUBCLASS_MFG_INFO,
		.block = 0,
		.len = BLOCK_SIZE,
	};
	uint8_t image[ELEMENT_SIZE] = {0};

	zassert_equal(fuel_gauge_get_buffer_prop(fixture->dev,
						 HY4245_FUEL_GAUGE_MANUFACTURER_INFO_A, data,
						 sizeof(data)),
		      -EACCES);
	zassert_equal(fuel_gauge_set_buffer_prop(fixture->dev,
						 HY4245_FUEL_GAUGE_MANUFACTURER_INFO_C, data,
						 sizeof(data)),
		      -EACCES);
	zassert_equal(fuel_gauge_get_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK,
						 &blk, sizeof(blk)),
		      -EACCES);
	zassert_equal(fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_CONFIG_IMAGE,
						 image, sizeof(image)),
		      -EACCES);
	zassert_equal(fuel_gauge_set_buffer_prop(fixture->dev,
						 HY4245_FUEL_GAUGE_CONFIG_IMAGE_VERIFY, image,
						 sizeof(image)),
		      -EACCES);
}

ZTEST_USER_F(hy4245, test_buffer_props_argument_validation)
{
	uint8_t data[BLOCK_SIZE + 1] = {0};
	struct hy4245_data_flash_block blk = {
		.subclass = SUBCLASS_MFG_INFO,
		.block = 0,
		.len = BLOCK_SIZE + 1,
	};

	/* Argument checks come before the calibration mode check */
	zassert_equal(
		fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_CONFIG_IMAGE, NULL, 0),
		-EINVAL);
	zassert_equal(fuel_gauge_get_buffer_prop(fixture->dev,
						 HY4245_FUEL_GAUGE_MANUFACTURER_INFO_A, NULL, 0),
		      -EINVAL);

	enter_calibration_mode(fixture->dev);

	zassert_equal(fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK,
						 &blk, sizeof(blk)),
		      -EINVAL, "block length above the block size accepted");
	blk.len = 0;
	zassert_equal(fuel_gauge_get_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK,
						 &blk, sizeof(blk)),
		      -EINVAL, "zero block length accepted");
	blk.len = BLOCK_SIZE;
	zassert_equal(fuel_gauge_get_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK,
						 &blk, sizeof(blk) - 1),
		      -EINVAL, "wrong structure size accepted");
	zassert_equal(fuel_gauge_set_buffer_prop(fixture->dev,
						 HY4245_FUEL_GAUGE_MANUFACTURER_INFO_B, data,
						 sizeof(data)),
		      -EINVAL, "manufacturer info above the block size accepted");
	zassert_equal(fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_CONFIG_IMAGE, data,
						 ELEMENT_SIZE - 1),
		      -EINVAL, "image length not a multiple of the element size accepted");
	zassert_equal(fuel_gauge_get_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_CONFIG_IMAGE, data,
						 BLOCK_SIZE),
		      -ENOTSUP, "configuration image is write only");

	leave_calibration_mode(fixture->dev);
}

ZTEST_USER_F(hy4245, test_data_flash_block_read_write)
{
	uint8_t pattern[BLOCK_SIZE];
	uint8_t partial[4] = {0xde, 0xad, 0xbe, 0xef};
	uint8_t readback[BLOCK_SIZE];
	uint32_t checksum_before, checksum_after;
	struct hy4245_data_flash_block blk = {
		.subclass = SUBCLASS_MFG_INFO,
		.block = 2,
		.len = 2,
	};

	for (size_t i = 0; i < sizeof(pattern); i++) {
		pattern[i] = 0xa5 ^ (uint8_t)i;
	}

	enter_calibration_mode(fixture->dev);

	checksum_before = get_uint(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_CHECKSUM);

	write_block(fixture->dev, SUBCLASS_MFG_INFO, 2, pattern, BLOCK_SIZE);
	read_block(fixture->dev, SUBCLASS_MFG_INFO, 2, readback);
	zassert_mem_equal(readback, pattern, BLOCK_SIZE, "full block write not read back");

	/* A partial write keeps the rest of the block */
	write_block(fixture->dev, SUBCLASS_MFG_INFO, 2, partial, sizeof(partial));
	read_block(fixture->dev, SUBCLASS_MFG_INFO, 2, readback);
	zassert_mem_equal(readback, partial, sizeof(partial), "partial write not applied");
	zassert_mem_equal(&readback[sizeof(partial)], &pattern[sizeof(partial)],
			  BLOCK_SIZE - sizeof(partial), "partial write clobbered the block");

	/* A partial read only touches the requested bytes */
	memset(blk.data, 0xff, sizeof(blk.data));
	zassert_ok(fuel_gauge_get_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_BLOCK,
					      &blk, sizeof(blk)));
	zassert_mem_equal(blk.data, partial, 2);
	zassert_equal(blk.data[2], 0xff);

	checksum_after = get_uint(fixture->dev, HY4245_FUEL_GAUGE_DATA_FLASH_CHECKSUM);
	zassert_not_equal(checksum_after, checksum_before,
			  "data flash checksum unchanged after a write");

	leave_calibration_mode(fixture->dev);

	/* The written data survives the reset */
	enter_calibration_mode(fixture->dev);
	read_block(fixture->dev, SUBCLASS_MFG_INFO, 2, readback);
	zassert_mem_equal(readback, partial, sizeof(partial), "block lost after reset");
	leave_calibration_mode(fixture->dev);
}

ZTEST_USER_F(hy4245, test_manufacturer_info_blocks)
{
	const uint8_t info_a[16] = "manufacturer  A";
	const uint8_t info_b[BLOCK_SIZE] = "manufacturer info block B, 32 b";
	uint8_t readback[BLOCK_SIZE];
	uint8_t first[4];

	enter_calibration_mode(fixture->dev);

	/* A short buffer is padded with zeros */
	zassert_ok(fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_MANUFACTURER_INFO_A,
					      info_a, sizeof(info_a)));
	zassert_ok(fuel_gauge_get_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_MANUFACTURER_INFO_A,
					      readback, sizeof(readback)));
	zassert_mem_equal(readback, info_a, sizeof(info_a));
	for (size_t i = sizeof(info_a); i < BLOCK_SIZE; i++) {
		zassert_equal(readback[i], 0, "block A not zero padded at %zu", i);
	}

	zassert_ok(fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_MANUFACTURER_INFO_B,
					      info_b, sizeof(info_b)));

	/* A short read returns the first bytes of the block */
	zassert_ok(fuel_gauge_get_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_MANUFACTURER_INFO_B,
					      first, sizeof(first)));
	zassert_mem_equal(first, info_b, sizeof(first));

	/* Manufacturer info block B is data flash subclass 0x20 block 1 */
	read_block(fixture->dev, SUBCLASS_MFG_INFO, 1, readback);
	zassert_mem_equal(readback, info_b, BLOCK_SIZE);

	leave_calibration_mode(fixture->dev);
}

ZTEST_USER_F(hy4245, test_config_image_program_and_verify)
{
	uint8_t image[6 * ELEMENT_SIZE];
	uint8_t calib0[BLOCK_SIZE], calib1[BLOCK_SIZE], mfg_a[BLOCK_SIZE], shared1[BLOCK_SIZE];
	uint8_t readback[BLOCK_SIZE];
	uint8_t expected[BLOCK_SIZE];

	fill_element(image, 0, SUBCLASS_CALIBRATION, 0, 0x11);
	fill_element(image, 1, SUBCLASS_CALIBRATION, 1, 0x22);
	fill_element(image, 2, SUBCLASS_SHARED, 0, 0x33);
	fill_element(image, 3, SUBCLASS_SHARED, 1, 0x44);
	fill_element(image, 4, SUBCLASS_MFG_INFO, 0, 0x55);
	fill_element(image, 5, SUBCLASS_MFG_INFO, 1, 0x66);

	enter_calibration_mode(fixture->dev);

	read_block(fixture->dev, SUBCLASS_CALIBRATION, 0, calib0);
	read_block(fixture->dev, SUBCLASS_CALIBRATION, 1, calib1);
	read_block(fixture->dev, SUBCLASS_MFG_INFO, 0, mfg_a);
	read_block(fixture->dev, SUBCLASS_SHARED, 1, shared1);

	zassert_ok(fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_CONFIG_IMAGE, image,
					      sizeof(image)),
		   "programming the image failed");

	/* Device specific blocks are not programmed */
	read_block(fixture->dev, SUBCLASS_CALIBRATION, 0, readback);
	zassert_mem_equal(readback, calib0, BLOCK_SIZE, "calibration block 0 overwritten");
	read_block(fixture->dev, SUBCLASS_CALIBRATION, 1, readback);
	zassert_mem_equal(readback, calib1, BLOCK_SIZE, "calibration block 1 overwritten");
	read_block(fixture->dev, SUBCLASS_MFG_INFO, 0, readback);
	zassert_mem_equal(readback, mfg_a, BLOCK_SIZE, "manufacturer info A overwritten");

	/* Regular blocks are programmed */
	read_block(fixture->dev, SUBCLASS_SHARED, 0, readback);
	memset(expected, 0x33, BLOCK_SIZE);
	zassert_mem_equal(readback, expected, BLOCK_SIZE, "shared block 0 not programmed");
	read_block(fixture->dev, SUBCLASS_MFG_INFO, 1, readback);
	memset(expected, 0x66, BLOCK_SIZE);
	zassert_mem_equal(readback, expected, BLOCK_SIZE, "manufacturer info B not programmed");

	/* Bytes 7 and 8 of the shared block are preserved */
	read_block(fixture->dev, SUBCLASS_SHARED, 1, readback);
	memset(expected, 0x44, BLOCK_SIZE);
	expected[7] = shared1[7];
	expected[8] = shared1[8];
	zassert_mem_equal(readback, expected, BLOCK_SIZE, "device specific bytes not preserved");

	/* The programmed image verifies, a modified one does not */
	zassert_ok(fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_CONFIG_IMAGE_VERIFY,
					      image, sizeof(image)));
	image[5 * ELEMENT_SIZE + 2 + 3] ^= 0xff;
	zassert_equal(fuel_gauge_set_buffer_prop(fixture->dev,
						 HY4245_FUEL_GAUGE_CONFIG_IMAGE_VERIFY, image,
						 sizeof(image)),
		      -EILSEQ);

	/* Differences in skipped or preserved bytes are ignored by the verification */
	image[5 * ELEMENT_SIZE + 2 + 3] ^= 0xff;
	image[0 * ELEMENT_SIZE + 2 + 1] ^= 0xff;
	image[3 * ELEMENT_SIZE + 2 + 7] ^= 0xff;
	zassert_ok(fuel_gauge_set_buffer_prop(fixture->dev, HY4245_FUEL_GAUGE_CONFIG_IMAGE_VERIFY,
					      image, sizeof(image)));

	leave_calibration_mode(fixture->dev);
}

ZTEST_USER_F(hy4245, test_relearn_commands)
{
	uint32_t flags_before, flags_after;

	/* One-shot commands need an explicit true */
	zassert_equal(set_bool(fixture->dev, HY4245_FUEL_GAUGE_CLEAR_LEARNED, false), -EINVAL);
	zassert_equal(set_bool(fixture->dev, HY4245_FUEL_GAUGE_QUICK_START, false), -EINVAL);

	/* ClearLearned clears the learned flag and nothing else */
	flags_before = get_uint(fixture->dev, HY4245_FUEL_GAUGE_FLAGS);
	zassert_ok(set_bool(fixture->dev, HY4245_FUEL_GAUGE_CLEAR_LEARNED, true));
	flags_after = get_uint(fixture->dev, HY4245_FUEL_GAUGE_FLAGS);
	zassert_false(flags_after & HY4245_FLAGS_LRND, "ClearLearned did not clear LRND");
	zassert_equal(flags_after & ~HY4245_FLAGS_LRND, flags_before & ~HY4245_FLAGS_LRND,
		      "ClearLearned changed other flags");

	zassert_ok(set_bool(fixture->dev, HY4245_FUEL_GAUGE_QUICK_START, true));
}

ZTEST_SUITE(hy4245, NULL, hy4245_setup, hy4245_before, NULL, NULL);
