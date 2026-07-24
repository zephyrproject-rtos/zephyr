/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lin.h>
#include <zephyr/kernel.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(commander))
#define LIN_COMMANDER DT_ALIAS(commander)
#else
#error "Please set the correct LIN commander device"
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(responder))
#define LIN_RESPONDER DT_ALIAS(responder)
#else
#error "Please set the correct LIN responder device"
#endif

/**
 * @brief LIN bus configuration parameters
 */
#define LIN_BUS_BAUDRATE            19200U
#define LIN_BUS_BREAK_LEN_COMMANDER 13U
#define LIN_BUS_BREAK_LEN_RESPONDER 11U
#define LIN_BUS_BREAK_DELIMITER_LEN 1U

/**
 * @brief LIN IDs for testing
 */
#define LIN_COMMANDER_WRITE_ID 0x01
#define LIN_COMMANDER_READ_ID  0x02

/**
 * @brief Length of test data
 */
#define LIN_TEST_DATA_LEN LIN_MAX_DLEN

/**
 * @brief LIN devices to be used in tests
 */
extern const struct device *const lin_commander;
extern const struct device *const lin_responder;
extern const struct device *const commander_phy;

/**
 * @brief LIN configurations
 */
extern const struct lin_config commander_cfg;
extern const struct lin_config responder_cfg;

/**
 * @brief Test data
 */
extern const uint8_t lin_test_data[LIN_TEST_DATA_LEN];

/**
 * @brief Receive buffer
 */
extern uint8_t rx_buffer[LIN_TEST_DATA_LEN];

/**
 * @brief Semaphore to signal transmission completion
 */
extern struct k_sem transmission_completed;

/**
 * @brief LIN messages for commander and responder
 */
extern struct lin_msg commander_msg;
extern struct lin_msg responder_msg;

/**
 * @brief Test setup and teardown functions
 */
extern void *test_lin_setup(void);
extern void test_lin_before(void *f);
extern void test_lin_after(void *f);
