/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

#include "akm09918c.h"
#include "akm09918c_emul.h"
#include "akm09918c_reg.h"

struct akm09918c_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *akm09918c_setup(void)
{
	static struct akm09918c_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(akm09918c)),
		.target = EMUL_DT_GET(DT_NODELABEL(akm09918c)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void akm09918c_before(void *f)
{
	struct akm09918c_fixture *fixture = f;

	akm09918c_emul_reset(fixture->target);
}

ZTEST_SUITE(akm09918c, NULL, akm09918c_setup, akm09918c_before, NULL, NULL);

ZTEST_F(akm09918c, test_fetch_fail_no_ready_data)
{
	uint8_t status = 0;

	akm09918c_emul_set_reg(fixture->target, AKM09918C_REG_ST1, &status, 1);
	zassert_equal(-EBUSY, sensor_sample_fetch(fixture->dev));
}

static void test_fetch_magnetic_field(const struct akm09918c_fixture *fixture,
				      const int16_t magn_percent[3])
{
	struct sensor_value values[3];
	int64_t expect_ugauss;
	int64_t actual_ugauss;
	uint8_t register_buffer[6];

	/* Set the ST1 register to show we have data */
	register_buffer[0] = AKM09918C_ST1_DRDY;
	akm09918c_emul_set_reg(fixture->target, AKM09918C_REG_ST1, register_buffer, 1);

	/* Set the data magn_percent * range */
	for (int i = 0; i < 3; ++i) {
		register_buffer[i * 2 + 1] = (magn_percent[i] >> 8) & GENMASK(7, 0);
		register_buffer[i * 2] = magn_percent[i] & GENMASK(7, 0);
	}
	akm09918c_emul_set_reg(fixture->target, AKM09918C_REG_HXL, register_buffer, 6);

	/* Fetch the data */
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_MAGN_XYZ, values));

	/* Assert the data is within 0.000005 Gauss */
	actual_ugauss = values[0].val1 * INT64_C(1000000) + values[0].val2;
	expect_ugauss = magn_percent[0] * AKM09918C_MICRO_GAUSS_PER_BIT;
	zassert_within(expect_ugauss, actual_ugauss, INT64_C(5),
		       "(X) expected %" PRIi64 " micro-gauss, got %" PRIi64 " micro-gauss",
		       expect_ugauss, actual_ugauss);

	actual_ugauss = values[1].val1 * INT64_C(1000000) + values[1].val2;
	expect_ugauss = magn_percent[1] * AKM09918C_MICRO_GAUSS_PER_BIT;
	zassert_within(expect_ugauss, actual_ugauss, INT64_C(5),
		       "(Y) expected %" PRIi64 " micro-gauss, got %" PRIi64 " micro-gauss",
		       expect_ugauss, actual_ugauss);

	actual_ugauss = values[2].val1 * INT64_C(1000000) + values[2].val2;
	expect_ugauss = magn_percent[2] * AKM09918C_MICRO_GAUSS_PER_BIT;
	zassert_within(expect_ugauss, actual_ugauss, INT64_C(5),
		       "(Z) expected %" PRIi64 " micro-gauss, got %" PRIi64 " micro-gauss",
		       expect_ugauss, actual_ugauss);
}

ZTEST_F(akm09918c, test_fetch_magn)
{
	/* Use (0.25, -0.33..., 0.91) as the factors */
	const int16_t magn_percent[3] = {
		INT16_C(AKM09918C_MAGN_MAX_DATA_REG) / INT16_C(4),
		INT16_C(AKM09918C_MAGN_MIN_DATA_REG) / INT16_C(3),
		(int16_t)(INT16_C(AKM09918C_MAGN_MAX_DATA_REG) * INT32_C(91) / INT32_C(100)),
	};

	test_fetch_magnetic_field(fixture, magn_percent);
}

#ifdef CONFIG_SENSOR_ASYNC_API

RTIO_DEFINE_WITH_MEMPOOL(rtio_ctx, 4, 4, 4, 64, sizeof(void *));
SENSOR_DT_READ_IODEV(iodev, DT_NODELABEL(akm09918c), {SENSOR_CHAN_MAGN_XYZ, 0});

static bool consume_cqe(int32_t *result, void **userdata)
{
	struct rtio_cqe *cqe = rtio_cqe_consume(&rtio_ctx);
	uint8_t *buf = NULL;
	uint32_t buf_len = 0U;

	if (cqe == NULL) {
		return false;
	}

	*result = cqe->result;
	*userdata = cqe->userdata;

	if ((rtio_cqe_get_mempool_buffer(&rtio_ctx, cqe, &buf, &buf_len) == 0) && (buf != NULL)) {
		rtio_release_buffer(&rtio_ctx, buf, buf_len);
	}
	rtio_cqe_release(&rtio_ctx, cqe);

	return true;
}

ZTEST_F(akm09918c, test_async_overlapping_reads)
{
	uint8_t status = AKM09918C_ST1_DRDY;
	void *userdata = NULL;
	int32_t result = 0;
	uint32_t cqe_count = 0U;
	uint32_t first_count = 0U;
	uint32_t second_count = 0U;
	int32_t first_result = -EIO;
	int32_t second_result = 0;

	akm09918c_emul_set_reg(fixture->target, AKM09918C_REG_ST1, &status, 1);

	/* Submit read #2 while read #1's 9 ms measurement delay is in flight */
	zassert_ok(sensor_read_async_mempool(&iodev, &rtio_ctx, UINT_TO_POINTER(1U)));
	k_msleep(1);
	zassert_ok(sensor_read_async_mempool(&iodev, &rtio_ctx, UINT_TO_POINTER(2U)));

	k_msleep(20);

	while (consume_cqe(&result, &userdata)) {
		cqe_count++;
		if (userdata == UINT_TO_POINTER(1U)) {
			first_count++;
			first_result = result;
		} else if (userdata == UINT_TO_POINTER(2U)) {
			second_count++;
			second_result = result;
		} else {
			zassert_unreachable("Unexpected CQE userdata %p (result=%d)", userdata,
					    result);
		}
	}

	zassert_equal(cqe_count, 2U, "Expected 2 CQEs, got %u", cqe_count);
	zassert_equal(first_count, 1U, "Expected read #1 to complete once, got %u", first_count);
	zassert_equal(second_count, 1U, "Expected read #2 to complete once, got %u", second_count);
	zassert_ok(first_result, "Expected read #1 to succeed, got %d", first_result);
	zassert_equal(second_result, -EBUSY, "Expected read #2 to fail with -EBUSY, got %d",
		      second_result);

	/* Re-arm DRDY (emulator clears it on TMPS read) and verify busy flag unlocked */
	akm09918c_emul_set_reg(fixture->target, AKM09918C_REG_ST1, &status, 1);
	zassert_ok(sensor_read_async_mempool(&iodev, &rtio_ctx, UINT_TO_POINTER(3U)));
	k_msleep(20);

	zassert_true(consume_cqe(&result, &userdata), "Expected sequential read #3 to complete");
	zassert_equal(userdata, UINT_TO_POINTER(3U), "Unexpected userdata for read #3");
	zassert_ok(result, "Expected sequential read #3 to succeed, got %d", result);
	zassert_false(consume_cqe(&result, &userdata), "Unexpected extra CQE after read #3");
}

#endif /* CONFIG_SENSOR_ASYNC_API */
