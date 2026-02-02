/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/lin.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "common.h"

ZTEST_DMEM const struct device *const lin_commander = DEVICE_DT_GET(LIN_COMMANDER);
ZTEST_DMEM const struct device *const lin_responder = DEVICE_DT_GET(LIN_RESPONDER);
ZTEST_DMEM const struct device *const commander_phy =
	DEVICE_DT_GET_OR_NULL(DT_PHANDLE(LIN_COMMANDER, phys));

ZTEST_DMEM const struct lin_config commander_cfg = {
	.mode = LIN_MODE_COMMANDER,
	.baudrate = LIN_BUS_BAUDRATE,
	.break_len = LIN_BUS_BREAK_LEN_COMMANDER,
	.break_delimiter_len = LIN_BUS_BREAK_DELIMITER_LEN,
	.flags = 0,
};

ZTEST_DMEM const struct lin_config responder_cfg = {
	.mode = LIN_MODE_RESPONDER,
	.baudrate = LIN_BUS_BAUDRATE,
	.break_len = LIN_BUS_BREAK_LEN_RESPONDER,
	.break_delimiter_len = LIN_BUS_BREAK_DELIMITER_LEN,
	.flags = IS_ENABLED(CONFIG_LIN_AUTO_SYNCHRONIZATION) ? LIN_BUS_AUTO_SYNC : 0,
};

ZTEST_DMEM const uint8_t lin_test_data[LIN_TEST_DATA_LEN] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
};

ZTEST_DMEM uint8_t rx_buffer[LIN_TEST_DATA_LEN];
ZTEST_DMEM struct k_sem transmission_completed;
ZTEST_DMEM struct lin_msg commander_msg;
ZTEST_DMEM struct lin_msg responder_msg;

void *test_lin_setup(void)
{
	k_sem_init(&transmission_completed, 0, 1);
	return NULL;
}

void test_lin_before(void *f)
{
	ARG_UNUSED(f);
	k_sem_reset(&transmission_completed);
	memset(rx_buffer, 0, sizeof(rx_buffer));
}

void test_lin_after(void *f)
{
	ARG_UNUSED(f);
	lin_stop(lin_commander);
	lin_stop(lin_responder);
}
