/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_CANBUS_CANOPEN_COMMON_SELF_RECEPTION_H_
#define ZEPHYR_TESTS_SUBSYS_CANBUS_CANOPEN_COMMON_SELF_RECEPTION_H_

#include <stdbool.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define SELF_RECEPTION_ID      0x7FFU
#define SELF_RECEPTION_TIMEOUT K_MSEC(100)

static void self_reception_rx(const struct device *dev, struct can_frame *frame, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(frame);

	k_sem_give(user_data);
}

static void self_reception_tx(const struct device *dev, int error, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(error);
	ARG_UNUSED(user_data);
}

static bool canopen_test_self_reception(const struct device *dev)
{
	const struct can_filter filter = {
		.id = SELF_RECEPTION_ID,
		.mask = CAN_STD_ID_MASK,
	};
	const struct can_frame frame = {
		.id = SELF_RECEPTION_ID,
	};
	struct k_sem rx;
	int filter_id;
	int err;

	k_sem_init(&rx, 0, 1);
	filter_id = can_add_rx_filter(dev, self_reception_rx, &rx, &filter);
	zassert_true(filter_id >= 0);
	zassert_ok(can_start(dev));

	err = can_send(dev, &frame, SELF_RECEPTION_TIMEOUT, self_reception_tx, NULL);
	if (err == 0) {
		err = k_sem_take(&rx, SELF_RECEPTION_TIMEOUT);
	}

	zassert_ok(can_stop(dev));
	can_remove_rx_filter(dev, filter_id);

	return err == 0;
}

#endif /* ZEPHYR_TESTS_SUBSYS_CANBUS_CANOPEN_COMMON_SELF_RECEPTION_H_ */
