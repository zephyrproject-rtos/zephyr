/*
 * Copyright (c) 2026 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "icm45686_stream.c"

int icm45686_prep_reg_read_rtio_async(const struct icm45686_bus *bus, uint8_t reg, uint8_t *buf,
				      size_t size, struct rtio_sqe **out)
{
	ARG_UNUSED(bus);
	ARG_UNUSED(reg);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);
	ARG_UNUSED(out);
	return -ENOTSUP;
}

int icm45686_prep_reg_write_rtio_async(const struct icm45686_bus *bus, uint8_t reg,
				       const uint8_t *buf, size_t size, struct rtio_sqe **out)
{
	ARG_UNUSED(bus);
	ARG_UNUSED(reg);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);
	ARG_UNUSED(out);
	return -ENOTSUP;
}

ZTEST(icm45686_stream, test_early_irq_without_submission_is_ignored)
{
	static const struct icm45686_config config;
	struct icm45686_data data = {0};
	const struct device dev = {
		.name = "icm45686-test",
		.config = &config,
		.data = &data,
	};

	icm45686_event_handler(&dev);

	zassert_is_null(data.stream.iodev_sqe);
	zassert_equal(atomic_get(&data.stream.state), ICM45686_STREAM_OFF);
}

ZTEST_SUITE(icm45686_stream, NULL, NULL, NULL, NULL, NULL);
