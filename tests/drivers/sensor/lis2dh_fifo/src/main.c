/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

#include "lis2dh.h"
#include "lis2dh_decoder.h"
#include "emul.h"

#if defined(CONFIG_LIS2DH_STREAM)

static const struct device *const devices[] = {
	DEVICE_DT_GET(DT_NODELABEL(accel_i2c)),
	DEVICE_DT_GET(DT_NODELABEL(accel_spi)),
};
static const struct emul *const emulators[] = {
	EMUL_DT_GET(DT_NODELABEL(accel_i2c)),
	EMUL_DT_GET(DT_NODELABEL(accel_spi)),
};

#define SPI_DEV  DEVICE_DT_GET(DT_NODELABEL(accel_spi))
#define SPI_EMUL EMUL_DT_GET(DT_NODELABEL(accel_spi))

SENSOR_DT_STREAM_IODEV(watermark, DT_NODELABEL(accel_spi),
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE});
SENSOR_DT_STREAM_IODEV(watermark_i2c, DT_NODELABEL(accel_i2c),
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE});
SENSOR_DT_STREAM_IODEV(full, DT_NODELABEL(accel_spi),
		       {SENSOR_TRIG_FIFO_FULL, SENSOR_STREAM_DATA_INCLUDE});
SENSOR_DT_STREAM_IODEV(watermark_and_full, DT_NODELABEL(accel_spi),
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE},
		       {SENSOR_TRIG_FIFO_FULL, SENSOR_STREAM_DATA_INCLUDE});
SENSOR_DT_STREAM_IODEV(nop, DT_NODELABEL(accel_spi),
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_NOP});
SENSOR_DT_STREAM_IODEV(watermark_nop_full_include, DT_NODELABEL(accel_spi),
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_NOP},
		       {SENSOR_TRIG_FIFO_FULL, SENSOR_STREAM_DATA_INCLUDE});
SENSOR_DT_STREAM_IODEV(watermark_include_full_nop, DT_NODELABEL(accel_spi),
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE},
		       {SENSOR_TRIG_FIFO_FULL, SENSOR_STREAM_DATA_NOP});
SENSOR_DT_STREAM_IODEV(drop, DT_NODELABEL(accel_spi),
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_DROP});
SENSOR_DT_READ_IODEV(single, DT_NODELABEL(accel_spi), {SENSOR_CHAN_ACCEL_XYZ, 0});
static struct sensor_stream_trigger empty_triggers[1];
static struct sensor_read_config empty_stream_config = {
	.sensor = SPI_DEV,
	.is_streaming = true,
	.triggers = empty_triggers,
	.count = 0U,
	.max = ARRAY_SIZE(empty_triggers),
};
RTIO_IODEV_DEFINE(empty_stream, &__sensor_iodev_api, &empty_stream_config);
RTIO_DEFINE_WITH_MEMPOOL(ctx, 4, 8, 2, 256, 8);

static void expect_pm_state(const struct device *dev, enum pm_device_state expected)
{
	enum pm_device_state state;

	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, expected);
}

static struct rtio_cqe *consume(void)
{
	struct rtio_cqe *cqe = NULL;

	for (int i = 0; i < 100 && cqe == NULL; i++) {
		cqe = rtio_cqe_consume(&ctx);
		if (cqe == NULL) {
			k_sleep(K_MSEC(1));
		}
	}
	zassert_not_null(cqe, "Completion timeout");
	return cqe;
}

static void expect_error(int error)
{
	struct rtio_cqe *cqe = consume();

	zassert_equal(cqe->result, error);
	zassert_equal(cqe->flags, 0U, "Error must not expose a freed buffer");
	rtio_cqe_release(&ctx, cqe);
}

static void wait_pending_on(const struct device *dev)
{
	struct lis2dh_data *data = dev->data;

	for (int i = 0; i < 100; i++) {
		bool pending;

		lis2dh_lock(dev);
		pending = data->streaming_sqe != NULL;
		lis2dh_unlock(dev);
		if (pending) {
			return;
		}
		k_sleep(K_MSEC(1));
	}
	zassert_unreachable("Submission timeout");
}

static void wait_pending(void)
{
	wait_pending_on(SPI_DEV);
}

static struct rtio_sqe *start_on(const struct device *dev, const struct rtio_iodev *iodev)
{
	struct rtio_sqe *handle = NULL;

	zassert_ok(sensor_stream(iodev, &ctx, NULL, &handle));
	wait_pending_on(dev);
	return handle;
}

static struct rtio_sqe *start(const struct rtio_iodev *iodev)
{
	return start_on(SPI_DEV, iodev);
}

static void cancel_on(const struct device *dev, struct rtio_sqe *handle)
{
	zassert_ok(rtio_sqe_cancel(handle));
	expect_error(-ECANCELED);
	zassert_false(lis2dh_fifo_is_active(dev));
	k_sleep(K_MSEC(20));
	zassert_is_null(rtio_cqe_consume(&ctx), "Duplicate completion");
}

static void cancel(struct rtio_sqe *handle)
{
	cancel_on(SPI_DEV, handle);
}

static enum sensor_trigger_type expect_data(unsigned int samples, uint8_t **saved,
					    uint32_t *saved_len)
{
	struct rtio_cqe *cqe = consume();
	uint8_t *buffer = NULL;
	uint32_t len = 0U;
	const struct lis2dh_encoded_header *header;
	enum sensor_trigger_type trigger;

	zassert_ok(cqe->result);
	zassert_ok(rtio_cqe_get_mempool_buffer(&ctx, cqe, &buffer, &len));
	zassert_not_null(buffer);
	header = (const void *)buffer;
	trigger = header->trigger;
	zassert_equal(header->sample_count, samples);
	zassert_true(header->timestamp_ns > 0U);
	zassert_equal(header->shift,
		      lis2dh_encoded_shift(((struct lis2dh_data *)SPI_DEV->data)->scale));
	rtio_cqe_release(&ctx, cqe);
	if (saved != NULL) {
		*saved = buffer;
		*saved_len = len;
	} else {
		rtio_release_buffer(&ctx, buffer, len);
	}
	return trigger;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		struct lis2dh_test_bus *bus = emulators[i]->data;
		const struct sensor_value rate = {.val1 = 100};
		enum pm_device_state state;

		zassert_true(device_is_ready(devices[i]));
		bus->fail_all = false;
		bus->fail_mask = 0U;
		zassert_ok(lis2dh_fifo_stop(devices[i]));
		zassert_ok(pm_device_state_get(devices[i], &state));
		if (state == PM_DEVICE_STATE_SUSPENDED) {
			zassert_ok(pm_device_action_run(devices[i], PM_DEVICE_ACTION_RESUME));
		}
		lis2dh_test_reset(emulators[i]);
		bus->regs[LIS2DH_REG_CTRL1] = LIS2DH_ACCEL_EN_BITS | LIS2DH_ODR_RATE(5);
		zassert_ok(sensor_attr_set(devices[i], SENSOR_CHAN_ACCEL_XYZ,
					   SENSOR_ATTR_SAMPLING_FREQUENCY, &rate));
	}
}

ZTEST(lis2dh_fifo, test_empty_stream_config_is_rejected_without_side_effects)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = NULL;
	uint8_t ctrl3 = bus->regs[LIS2DH_REG_CTRL3];
	uint8_t ctrl5 = bus->regs[LIS2DH_REG_CTRL5];
	uint8_t fifo_ctrl = bus->regs[LIS2DH_REG_FIFO_CTRL];

	zassert_ok(sensor_stream(&empty_stream, &ctx, NULL, &handle));
	zassert_not_null(handle);
	expect_error(-EINVAL);
	zassert_false(lis2dh_fifo_is_busy(SPI_DEV));
	zassert_equal(bus->regs[LIS2DH_REG_CTRL3], ctrl3);
	zassert_equal(bus->regs[LIS2DH_REG_CTRL5], ctrl5);
	zassert_equal(bus->regs[LIS2DH_REG_FIFO_CTRL], fifo_ctrl);
}

ZTEST(lis2dh_fifo, test_invalid_fifo_watermark_is_rejected_without_side_effects)
{
	static const uint8_t invalid_watermarks[] = {0U, LIS2DH_FIFO_MAX_SAMPLES + 1U, UINT8_MAX};
	const struct lis2dh_config *original = SPI_DEV->config;
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	uint8_t ctrl3 = bus->regs[LIS2DH_REG_CTRL3];
	uint8_t ctrl5 = bus->regs[LIS2DH_REG_CTRL5];
	uint8_t fifo_ctrl = bus->regs[LIS2DH_REG_FIFO_CTRL];

	for (size_t i = 0U; i < ARRAY_SIZE(invalid_watermarks); i++) {
		struct lis2dh_config invalid_config = {
			.bus_init = original->bus_init,
			.bus_cfg = original->bus_cfg,
			.gpio_drdy = original->gpio_drdy,
			.gpio_int = original->gpio_int,
			.int1_mode = original->int1_mode,
			.int2_mode = original->int2_mode,
			.hw = original->hw,
			.temperature = original->temperature,
			.fifo_watermark = invalid_watermarks[i],
		};
		struct device invalid_dev = *SPI_DEV;

		invalid_dev.config = &invalid_config;
		bus->operations = 0U;
		zassert_equal(lis2dh_fifo_start(&invalid_dev), -EINVAL, "watermark %u",
			      invalid_watermarks[i]);
		zassert_equal(bus->operations, 0U);
		zassert_false(lis2dh_fifo_is_busy(SPI_DEV));
		zassert_equal(bus->regs[LIS2DH_REG_CTRL3], ctrl3);
		zassert_equal(bus->regs[LIS2DH_REG_CTRL5], ctrl5);
		zassert_equal(bus->regs[LIS2DH_REG_FIFO_CTRL], fifo_ctrl);
	}
}

ZTEST(lis2dh_fifo, test_pm_suspend_resume_round_trip_both_buses)
{
	for (size_t i = 0U; i < ARRAY_SIZE(devices); i++) {
		struct lis2dh_test_bus *bus = emulators[i]->data;
		struct lis2dh_data *data = devices[i]->data;
		uint8_t active_ctrl1 = bus->regs[LIS2DH_REG_CTRL1];

		expect_pm_state(devices[i], PM_DEVICE_STATE_ACTIVE);
		zassert_ok(pm_device_action_run(devices[i], PM_DEVICE_ACTION_SUSPEND));
		expect_pm_state(devices[i], PM_DEVICE_STATE_SUSPENDED);
		zassert_equal(bus->regs[LIS2DH_REG_CTRL1], LIS2DH_SUSPEND);
		zassert_equal(data->reg_ctrl1_active_val, active_ctrl1);

		zassert_ok(pm_device_action_run(devices[i], PM_DEVICE_ACTION_RESUME));
		expect_pm_state(devices[i], PM_DEVICE_STATE_ACTIVE);
		zassert_equal(bus->regs[LIS2DH_REG_CTRL1], active_ctrl1);
	}
}

ZTEST(lis2dh_fifo, test_pm_bus_errors_leave_retryable_state_both_buses)
{
	for (size_t i = 0U; i < ARRAY_SIZE(devices); i++) {
		struct lis2dh_test_bus *bus = emulators[i]->data;
		uint8_t active_ctrl1 = bus->regs[LIS2DH_REG_CTRL1];

		for (unsigned int operation = 0U; operation < 2U; operation++) {
			lis2dh_test_reset(emulators[i]);
			bus->fail_mask = BIT64(operation);
			zassert_equal(pm_device_action_run(devices[i], PM_DEVICE_ACTION_SUSPEND),
				      -EIO, "device %zu suspend operation %u", i, operation);
			expect_pm_state(devices[i], PM_DEVICE_STATE_ACTIVE);
			zassert_equal(bus->regs[LIS2DH_REG_CTRL1], active_ctrl1);
		}

		lis2dh_test_reset(emulators[i]);
		zassert_ok(pm_device_action_run(devices[i], PM_DEVICE_ACTION_SUSPEND));
		for (unsigned int operation = 0U; operation < 2U; operation++) {
			lis2dh_test_reset(emulators[i]);
			bus->fail_mask = BIT64(operation);
			zassert_equal(
				pm_device_action_run(devices[i], PM_DEVICE_ACTION_RESUME), -EIO,
				      "device %zu resume operation %u", i, operation);
			expect_pm_state(devices[i], PM_DEVICE_STATE_SUSPENDED);
			zassert_equal(bus->regs[LIS2DH_REG_CTRL1], LIS2DH_SUSPEND);
		}

		lis2dh_test_reset(emulators[i]);
		zassert_ok(pm_device_action_run(devices[i], PM_DEVICE_ACTION_RESUME));
		expect_pm_state(devices[i], PM_DEVICE_STATE_ACTIVE);
		zassert_equal(bus->regs[LIS2DH_REG_CTRL1], active_ctrl1);
	}
}

ZTEST(lis2dh_fifo, test_pm_suspend_rejected_during_fifo_without_data_loss)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = start(&watermark);
	uint8_t ctrl1 = bus->regs[LIS2DH_REG_CTRL1];
	uint8_t ctrl3 = bus->regs[LIS2DH_REG_CTRL3];
	uint8_t ctrl5 = bus->regs[LIS2DH_REG_CTRL5];
	uint8_t fifo_ctrl = bus->regs[LIS2DH_REG_FIFO_CTRL];

	lis2dh_test_fill(SPI_EMUL, 16U);
	zassert_equal(pm_device_action_run(SPI_DEV, PM_DEVICE_ACTION_SUSPEND), -EBUSY);
	expect_pm_state(SPI_DEV, PM_DEVICE_STATE_ACTIVE);
	zassert_true(lis2dh_fifo_is_active(SPI_DEV));
	zassert_equal(bus->regs[LIS2DH_REG_CTRL1], ctrl1);
	zassert_equal(bus->regs[LIS2DH_REG_CTRL3], ctrl3);
	zassert_equal(bus->regs[LIS2DH_REG_CTRL5], ctrl5);
	zassert_equal(bus->regs[LIS2DH_REG_FIFO_CTRL], fifo_ctrl);
	zassert_equal(bus->regs[LIS2DH_REG_FIFO_SRC] & LIS2DH_FIFO_FSS_MASK, 16U);

	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	zassert_equal(expect_data(16, NULL, NULL), SENSOR_TRIG_FIFO_WATERMARK);
	wait_pending();
	cancel(handle);

	zassert_ok(pm_device_action_run(SPI_DEV, PM_DEVICE_ACTION_SUSPEND));
	expect_pm_state(SPI_DEV, PM_DEVICE_STATE_SUSPENDED);
	zassert_ok(pm_device_action_run(SPI_DEV, PM_DEVICE_ACTION_RESUME));
	expect_pm_state(SPI_DEV, PM_DEVICE_STATE_ACTIVE);
}

ZTEST(lis2dh_fifo, test_full_burst_both_buses)
{
	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		struct lis2dh_test_bus *bus = emulators[i]->data;
		const struct rtio_iodev *iodev = i == 0U ? &watermark_i2c : &watermark;
		struct rtio_sqe *handle = start_on(devices[i], iodev);

		/* Literal datasheet stream encoding; do not self-test the same macro. */
		zassert_equal(bus->regs[LIS2DH_REG_FIFO_CTRL], 0x8f);
		lis2dh_test_fill(emulators[i], 32U);
		zassert_ok(lis2dh_fifo_handle_irq(devices[i]));
		zassert_equal(bus->bursts, 1);
		zassert_equal(bus->last_len, 192);
		zassert_equal(bus->last_cmd, i == 0 ? 0xa8 : 0xe8);
		expect_data(32, NULL, NULL);
		wait_pending_on(devices[i]);
		cancel_on(devices[i], handle);
	}
}

ZTEST(lis2dh_fifo, test_ctrl3_routes_each_fifo_trigger)
{
	static const struct {
		const struct rtio_iodev *iodev;
		uint8_t expected;
	} cases[] = {
		{&watermark, LIS2DH_EN_FIFO_WTM_INT1},
		{&full, LIS2DH_EN_FIFO_OVRN_INT1},
		{&watermark_and_full, LIS2DH_FIFO_INT1_MASK},
	};
	struct lis2dh_test_bus *bus = SPI_EMUL->data;

	for (size_t i = 0U; i < ARRAY_SIZE(cases); i++) {
		struct rtio_sqe *handle = start(cases[i].iodev);

		zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & LIS2DH_FIFO_INT1_MASK,
			      cases[i].expected, "case %zu", i);
		cancel(handle);
		zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & LIS2DH_FIFO_INT1_MASK, 0U,
			      "case %zu", i);
	}
}

ZTEST(lis2dh_fifo, test_ctrl3_rejects_every_non_fifo_int1_route)
{
	static const uint8_t conflicts[] = {
		LIS2DH_EN_CLICK_INT1,
		LIS2DH_EN_IA_INT1,
		LIS2DH_EN_IA2_INT1,
		LIS2DH_EN_DRDY1_INT1,
		LIS2DH_EN_321DA_INT1,
	};

	for (size_t dev_idx = 0U; dev_idx < ARRAY_SIZE(devices); dev_idx++) {
		struct lis2dh_test_bus *bus = emulators[dev_idx]->data;

		for (size_t route_idx = 0U; route_idx < ARRAY_SIZE(conflicts); route_idx++) {
			lis2dh_test_reset(emulators[dev_idx]);
			bus->regs[LIS2DH_REG_CTRL3] = conflicts[route_idx];
			zassert_equal(lis2dh_fifo_start(devices[dev_idx]), -EBUSY,
				      "device %zu route %zu", dev_idx, route_idx);
			zassert_equal(bus->regs[LIS2DH_REG_CTRL3], conflicts[route_idx]);
			zassert_false(lis2dh_fifo_is_busy(devices[dev_idx]));
			bus->regs[LIS2DH_REG_CTRL3] = 0U;
		}
	}
}

ZTEST(lis2dh_fifo, test_start_rollback_every_bus_operation)
{
	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		struct lis2dh_test_bus *bus = emulators[i]->data;
		unsigned int operations;

		lis2dh_test_reset(emulators[i]);
		zassert_ok(lis2dh_fifo_start(devices[i]));
		/* Diagnostic readback is best-effort and tested separately. */
		operations = bus->operations - bus->diagnostic_reads;
		zassert_ok(lis2dh_fifo_stop(devices[i]));
		for (unsigned int fail = 0; fail < operations; fail++) {
			lis2dh_test_reset(emulators[i]);
			bus->fail_mask = BIT64(fail);
			zassert_equal(lis2dh_fifo_start(devices[i]), -EIO, "operation %u", fail);
			zassert_false(lis2dh_fifo_is_busy(devices[i]));
			zassert_equal(bus->regs[LIS2DH_REG_CTRL3], 0);
			zassert_equal(bus->regs[LIS2DH_REG_CTRL5] & LIS2DH_EN_FIFO, 0);
			zassert_equal(bus->regs[LIS2DH_REG_FIFO_CTRL], 0);
			zassert_ok(lis2dh_fifo_stop(devices[i]));
		}
	}
}

ZTEST(lis2dh_fifo, test_debug_readback_is_non_destructive_and_best_effort)
{
	const uint8_t level = COND_CODE_1(CONFIG_LOG, (CONFIG_SENSOR_LOG_LEVEL), (LOG_LEVEL_NONE));
	unsigned int diagnostic_reads = level >= LOG_LEVEL_DBG ? 2U : 0U;

	for (size_t i = 0U; i < ARRAY_SIZE(devices); i++) {
		struct lis2dh_test_bus *bus = emulators[i]->data;
		unsigned int start_operations;

		lis2dh_test_reset(emulators[i]);
		zassert_ok(lis2dh_fifo_start(devices[i]));
		zassert_equal(bus->diagnostic_reads, diagnostic_reads);
		zassert_equal(bus->bursts, 0U, "Diagnostic reads must not consume XYZ");
		start_operations = bus->operations - bus->diagnostic_reads;
		zassert_ok(lis2dh_fifo_stop(devices[i]));

		for (unsigned int fail = 0U; fail < diagnostic_reads; fail++) {
			lis2dh_test_reset(emulators[i]);
			bus->fail_mask = BIT64(start_operations + fail);
			zassert_ok(lis2dh_fifo_start(devices[i]));
			zassert_true(lis2dh_fifo_is_active(devices[i]));
			zassert_equal(bus->diagnostic_reads, diagnostic_reads);
			zassert_equal(bus->bursts, 0U);
			zassert_equal(bus->regs[LIS2DH_REG_FIFO_CTRL], 0x8f);
			bus->fail_mask = 0U;
			zassert_ok(lis2dh_fifo_stop(devices[i]));
		}
	}
}

ZTEST(lis2dh_fifo, test_failed_rollback_and_stop_are_retryable)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct lis2dh_data *data = SPI_DEV->data;
	const struct sensor_value rate = {.val1 = 100};

	lis2dh_test_reset(SPI_EMUL);
	bus->fail_mask = UINT64_MAX << 5;
	zassert_equal(lis2dh_fifo_start(SPI_DEV), -EIO);
	zassert_true(data->fifo_faulted);
	zassert_equal(lis2dh_fifo_start(SPI_DEV), -EBUSY);
	zassert_equal(sensor_attr_set(SPI_DEV, SENSOR_CHAN_ACCEL_XYZ,
				      SENSOR_ATTR_SAMPLING_FREQUENCY, &rate),
		      -EBUSY);
	bus->fail_mask = 0U;
	zassert_ok(lis2dh_fifo_stop(SPI_DEV));
	zassert_false(data->fifo_faulted);
	zassert_ok(lis2dh_fifo_start(SPI_DEV));
	bus->fail_all = true;
	zassert_equal(lis2dh_fifo_stop(SPI_DEV), -EIO);
	zassert_true(data->fifo_faulted);
	bus->fail_all = false;
	zassert_ok(lis2dh_fifo_stop(SPI_DEV));
	zassert_false(lis2dh_fifo_is_busy(SPI_DEV));
}

ZTEST(lis2dh_fifo, test_stream_submission_retries_failed_cleanup)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = NULL;

	lis2dh_test_reset(SPI_EMUL);
	bus->fail_mask = UINT64_MAX << 5;
	zassert_ok(sensor_stream(&watermark, &ctx, NULL, &handle));
	expect_error(-EIO);
	zassert_true(((struct lis2dh_data *)SPI_DEV->data)->fifo_faulted);

	bus->fail_mask = 0U;
	handle = start(&watermark);
	zassert_true(lis2dh_fifo_is_active(SPI_DEV));
	cancel(handle);
}

ZTEST(lis2dh_fifo, test_spi_update_read_error_does_not_write)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct lis2dh_data *data = SPI_DEV->data;

	lis2dh_test_reset(SPI_EMUL);
	bus->fail_mask = BIT64(0);
	zassert_equal(data->hw_tf->update_reg(SPI_DEV, LIS2DH_REG_CTRL5, 0x40, 0x40), -EIO);
	zassert_equal(bus->operations, 1);
	zassert_equal(bus->writes, 0);
}

ZTEST(lis2dh_fifo, test_odr_all_modes)
{
	const int rates[] = {0, 1, 10, 25, 50, 100, 200, 400, 1344, 1620, 5376};
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct lis2dh_data *data = SPI_DEV->data;

	for (int mode = 0; mode < 3; mode++) {
		bool lp = mode == 2;

		bus->regs[LIS2DH_REG_CTRL4] = mode == 1 ? BIT(3) : 0U;
		for (size_t i = 0; i < ARRAY_SIZE(rates); i++) {
			struct sensor_value rate = {.val1 = rates[i]};
			bool valid = rates[i] <= 400 || (lp ? rates[i] != 1344 : rates[i] == 1344);
			int status;

			bus->regs[LIS2DH_REG_CTRL1] = LIS2DH_ACCEL_EN_BITS | (lp ? BIT(3) : 0U);
			status = sensor_attr_set(SPI_DEV, SENSOR_CHAN_ACCEL_XYZ,
						 SENSOR_ATTR_SAMPLING_FREQUENCY, &rate);
			zassert_equal(status, valid ? 0 : -ENOTSUP);
			if (!valid || rates[i] == 0) {
				zassert_equal(lis2dh_fifo_start(SPI_DEV), -EINVAL);
				continue;
			}
			zassert_ok(lis2dh_fifo_start(SPI_DEV));
			zassert_equal(data->fifo_period_ns, UINT64_C(1000000000) / rates[i]);
			zassert_ok(lis2dh_fifo_stop(SPI_DEV));
		}
	}
	bus->regs[LIS2DH_REG_CTRL1] = LIS2DH_ODR_RATE(8);
	zassert_equal(lis2dh_fifo_start(SPI_DEV), -EINVAL);
	const struct sensor_value invalid[] = {
		{.val1 = -1},
		{.val1 = 65537},
		{.val1 = 100, .val2 = 1},
	};

	for (size_t i = 0; i < ARRAY_SIZE(invalid); i++) {
		zassert_equal(sensor_attr_set(SPI_DEV, SENSOR_CHAN_ACCEL_XYZ,
					      SENSOR_ATTR_SAMPLING_FREQUENCY, &invalid[i]),
			      -EINVAL);
	}
}

ZTEST(lis2dh_fifo, test_stream_full_only)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = start(&full);

	zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & 0x06, LIS2DH_EN_FIFO_OVRN_INT1);
	lis2dh_test_fill(SPI_EMUL, 16U);
	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	zassert_equal(bus->bursts, 0);
	zassert_is_null(rtio_cqe_consume(&ctx));
	for (int i = 0; i < 4; i++) {
		lis2dh_test_fill(SPI_EMUL, 32U);
		zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
		expect_data(32, NULL, NULL);
		wait_pending();
	}
	cancel(handle);
}

ZTEST(lis2dh_fifo, test_stream_nop_without_data_action_is_rejected)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = NULL;

	zassert_ok(sensor_stream(&nop, &ctx, NULL, &handle));
	zassert_not_null(handle);
	expect_error(-ENOTSUP);
	zassert_false(lis2dh_fifo_is_active(SPI_DEV));
	zassert_equal(bus->bursts, 0);
	zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & LIS2DH_FIFO_INT1_MASK, 0U);
}

ZTEST(lis2dh_fifo, test_stream_watermark_nop_then_full_include)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = start(&watermark_nop_full_include);

	lis2dh_test_fill(SPI_EMUL, 16U);
	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	zassert_equal(expect_data(0, NULL, NULL), SENSOR_TRIG_FIFO_WATERMARK);
	wait_pending();
	zassert_equal(bus->bursts, 0U);
	zassert_equal(bus->regs[LIS2DH_REG_FIFO_SRC] & LIS2DH_FIFO_FSS_MASK, 16U);
	zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & LIS2DH_FIFO_INT1_MASK,
		      LIS2DH_EN_FIFO_OVRN_INT1);

	lis2dh_test_fill(SPI_EMUL, 32U);
	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	zassert_equal(expect_data(32, NULL, NULL), SENSOR_TRIG_FIFO_FULL);
	wait_pending();
	zassert_equal(bus->bursts, 1U);
	cancel(handle);
}

ZTEST(lis2dh_fifo, test_stream_full_nop_then_watermark_include)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = start(&watermark_include_full_nop);

	lis2dh_test_fill(SPI_EMUL, 32U);
	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	zassert_equal(expect_data(0, NULL, NULL), SENSOR_TRIG_FIFO_FULL);
	wait_pending();
	zassert_equal(bus->bursts, 0U);
	zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & LIS2DH_FIFO_INT1_MASK,
		      LIS2DH_EN_FIFO_WTM_INT1);

	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	zassert_equal(expect_data(32, NULL, NULL), SENSOR_TRIG_FIFO_WATERMARK);
	wait_pending();
	zassert_equal(bus->bursts, 1U);
	cancel(handle);
}

ZTEST(lis2dh_fifo, test_stream_drop_and_drop_write_failure)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = start(&drop);

	lis2dh_test_fill(SPI_EMUL, 16U);
	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	expect_data(0, NULL, NULL);
	wait_pending();
	zassert_equal(bus->bursts, 0);
	zassert_equal(bus->regs[LIS2DH_REG_FIFO_SRC], LIS2DH_FIFO_EMPTY);
	zassert_equal(bus->regs[LIS2DH_REG_FIFO_CTRL], 0x8f);
	lis2dh_test_fill(SPI_EMUL, 16U);
	bus->fail_mask = BIT64(bus->operations + 2U);
	zassert_equal(lis2dh_fifo_handle_irq(SPI_DEV), -EIO);
	expect_error(-EIO);
	zassert_false(lis2dh_fifo_is_active(SPI_DEV));
	ARG_UNUSED(handle);
}

ZTEST(lis2dh_fifo, test_stream_status_and_burst_errors)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;

	for (unsigned int fail = 0; fail < 2; fail++) {
		(void)start(&watermark);
		lis2dh_test_fill(SPI_EMUL, 16U);
		bus->operations = 0;
		bus->fail_mask = BIT64(fail);
		zassert_equal(lis2dh_fifo_handle_irq(SPI_DEV), -EIO);
		expect_error(-EIO);
		zassert_false(lis2dh_fifo_is_active(SPI_DEV));
		bus->fail_mask = 0U;
	}
}

ZTEST(lis2dh_fifo, test_mempool_exhaustion_and_recovery)
{
	uint8_t *held[2];
	uint32_t sizes[2];

	(void)start(&watermark);
	for (int i = 0; i < 2; i++) {
		lis2dh_test_fill(SPI_EMUL, 16U);
		zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
		expect_data(16, &held[i], &sizes[i]);
		wait_pending();
	}
	lis2dh_test_fill(SPI_EMUL, 16U);
	zassert_true(lis2dh_fifo_handle_irq(SPI_DEV) < 0);
	struct rtio_cqe *cqe = consume();

	zassert_true(cqe->result < 0);
	zassert_equal(cqe->flags, 0);
	rtio_cqe_release(&ctx, cqe);
	zassert_false(lis2dh_fifo_is_active(SPI_DEV));
	for (int i = 0; i < 2; i++) {
		rtio_release_buffer(&ctx, held[i], sizes[i]);
	}
	cancel(start(&watermark));
}

K_THREAD_STACK_DEFINE(irq_stack, 4096);
static struct k_thread irq_thread;

static void run_irq(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	(void)lis2dh_fifo_handle_irq(SPI_DEV);
}

ZTEST(lis2dh_fifo, test_cancel_during_active_burst)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct rtio_sqe *handle = start(&watermark);

	bus->block_burst = true;
	lis2dh_test_fill(SPI_EMUL, 16U);
	k_thread_create(&irq_thread, irq_stack, K_THREAD_STACK_SIZEOF(irq_stack), run_irq, NULL,
			NULL, NULL, 0, 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&bus->burst_entered, K_SECONDS(1)));
	zassert_ok(rtio_sqe_cancel(handle));
	k_sem_give(&bus->burst_release);
	zassert_ok(k_thread_join(&irq_thread, K_SECONDS(1)));
	expect_error(-ECANCELED);
	zassert_false(lis2dh_fifo_is_active(SPI_DEV));
	k_sleep(K_MSEC(20));
	zassert_is_null(rtio_cqe_consume(&ctx));
}

ZTEST(lis2dh_fifo, test_stop_completes_once)
{
	(void)start(&watermark);
	zassert_ok(lis2dh_fifo_stop(SPI_DEV));
	expect_error(-ECANCELED);
	zassert_ok(lis2dh_fifo_stop(SPI_DEV));
	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	k_sleep(K_MSEC(20));
	zassert_is_null(rtio_cqe_consume(&ctx));
	cancel(start(&watermark));
}

ZTEST(lis2dh_fifo, test_gpio_level_irq_and_synchronous_read_exclusion)
{
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	const struct lis2dh_config *cfg = SPI_DEV->config;
	struct sensor_value temp;
	struct rtio_sqe *handle = start(&watermark);

	lis2dh_test_fill(SPI_EMUL, 16U);
	zassert_ok(gpio_emul_input_set(cfg->gpio_drdy.port, cfg->gpio_drdy.pin, 1));
	expect_data(16, NULL, NULL);
	wait_pending();
	bus->regs[cfg->temperature.dout_addr + 1] = 7;
	zassert_equal(sensor_sample_fetch(SPI_DEV), -EBUSY);
	cancel(handle);
	bus->regs[LIS2DH_REG_STATUS] = LIS2DH_STATUS_ZYX_DRDY;
	zassert_ok(sensor_sample_fetch(SPI_DEV));
	zassert_ok(sensor_channel_get(SPI_DEV, SENSOR_CHAN_DIE_TEMP, &temp));
	zassert_equal(temp.val1, 7);
}

static void drdy_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trigger);
}

ZTEST(lis2dh_fifo, test_drdy_conflict_and_pending_disable)
{
	static const struct sensor_trigger trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	/* Hold the lock so the deferred DRDY start cannot run until after FIFO start. */
	lis2dh_lock(SPI_DEV);
	zassert_ok(sensor_trigger_set(SPI_DEV, &trigger, drdy_handler));
	zassert_equal(lis2dh_fifo_start(SPI_DEV), -EBUSY);
	zassert_ok(sensor_trigger_set(SPI_DEV, &trigger, NULL));
	zassert_ok(lis2dh_fifo_start(SPI_DEV));
	zassert_equal(sensor_trigger_set(SPI_DEV, &trigger, drdy_handler), -EBUSY);
	lis2dh_unlock(SPI_DEV);
	k_sleep(K_MSEC(20));
	struct lis2dh_test_bus *bus = SPI_EMUL->data;

	zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & LIS2DH_EN_DRDY1_INT1, 0);
	zassert_equal(bus->regs[LIS2DH_REG_FIFO_CTRL], 0x8f);
	zassert_ok(lis2dh_fifo_stop(SPI_DEV));
}

ZTEST(lis2dh_fifo, test_deferred_drdy_start_respects_lifecycle_lock)
{
	static const struct sensor_trigger trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};
	struct lis2dh_test_bus *bus = SPI_EMUL->data;

	/* With FIFO idle, nothing else masks the deferred DRDY start: it must
	 * wait for the lifecycle lock before touching CTRL3/INT1.
	 */
	lis2dh_lock(SPI_DEV);
	zassert_ok(sensor_trigger_set(SPI_DEV, &trigger, drdy_handler));
	k_sleep(K_MSEC(50));
	zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & LIS2DH_EN_DRDY1_INT1, 0,
		      "deferred DRDY start wrote CTRL3 while the lock was held");
	zassert_ok(sensor_trigger_set(SPI_DEV, &trigger, NULL));
	lis2dh_unlock(SPI_DEV);
	k_sleep(K_MSEC(20));
	zassert_equal(bus->regs[LIS2DH_REG_CTRL3] & LIS2DH_EN_DRDY1_INT1, 0);
}

ZTEST(lis2dh_fifo, test_stop_during_executor_handoff)
{
	(void)start(&watermark);
	lis2dh_lock(SPI_DEV);
	lis2dh_test_fill(SPI_EMUL, 16U);
	zassert_ok(lis2dh_fifo_handle_irq(SPI_DEV));
	zassert_ok(lis2dh_fifo_stop(SPI_DEV));
	/* A queued old RTIO iteration must not stop a newly started FIFO. */
	zassert_ok(lis2dh_fifo_start(SPI_DEV));
	lis2dh_unlock(SPI_DEV);
	expect_data(16, NULL, NULL);
	expect_error(-ECANCELED);
	zassert_true(lis2dh_fifo_is_active(SPI_DEV));
	zassert_ok(lis2dh_fifo_stop(SPI_DEV));
	zassert_is_null(rtio_cqe_consume(&ctx));
}

ZTEST(lis2dh_fifo, test_single_read_scale_errors_and_fifo_exclusion)
{
	uint8_t buffer[sizeof(struct lis2dh_encoded_header) + LIS2DH_ENCODED_SAMPLE_SIZE];
	const struct lis2dh_encoded_header *header = (const void *)buffer;
	struct lis2dh_test_bus *bus = SPI_EMUL->data;
	struct lis2dh_data *data = SPI_DEV->data;
	struct sensor_value range;

	sensor_g_to_ms2(16, &range);
	zassert_ok(sensor_attr_set(SPI_DEV, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &range));
	zassert_ok(sensor_read(&single, &ctx, buffer, sizeof(buffer)));
	zassert_equal(header->sample_count, 1);
	zassert_equal(header->is_fifo, 0);
	zassert_equal(header->shift, 8);
	zassert_equal(header->scale, data->scale);
	zassert_true(header->timestamp_ns > 0);
	zassert_equal(sensor_read(&single, &ctx, buffer, 1), -ENOMEM);
	bus->operations = 0;
	bus->fail_mask = BIT64(0);
	zassert_equal(sensor_read(&single, &ctx, buffer, sizeof(buffer)), -EIO);
	bus->fail_mask = 0;
	uint32_t scale = data->scale;

	sensor_g_to_ms2(2, &range);
	bus->operations = 0;
	bus->fail_mask = BIT64(1);
	zassert_equal(
		sensor_attr_set(SPI_DEV, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &range),
		-EIO);
	zassert_equal(data->scale, scale);
	bus->fail_mask = 0;
	zassert_ok(sensor_attr_set(SPI_DEV, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &range));
	zassert_ok(lis2dh_fifo_start(SPI_DEV));
	zassert_equal(sensor_read(&single, &ctx, buffer, sizeof(buffer)), -EBUSY);
	zassert_ok(lis2dh_fifo_stop(SPI_DEV));
}

ZTEST_SUITE(lis2dh_fifo, NULL, NULL, before, NULL, NULL);

#else /* !CONFIG_LIS2DH_STREAM */

/* Keeps the driver's trigger path compiling without the FIFO option. */
ZTEST(lis2dh_fifo, test_build_without_stream_option)
{
	zassert_true(IS_ENABLED(CONFIG_LIS2DH_TRIGGER));
}

ZTEST_SUITE(lis2dh_fifo, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_LIS2DH_STREAM */
