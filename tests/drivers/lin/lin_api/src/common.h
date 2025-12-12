/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lin.h>
#include <zephyr/kernel.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(master))
#define LIN_MASTER DT_ALIAS(master)
#else
#error "Please set the correct LIN master device"
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(slave))
#define LIN_SLAVE DT_ALIAS(slave)
#else
#error "Please set the correct LIN slave device"
#endif

/**
 * @brief LIN bus configuration parameters
 */
#define LIN_BUS_BAUDRATE            19200U
#define LIN_BUS_BREAK_LEN_MASTER    13U
#define LIN_BUS_BREAK_LEN_SLAVE     11U
#define LIN_BUS_BREAK_DELIMITER_LEN 1U

/**
 * @brief LIN IDs for testing
 */
#define LIN_MASTER_WRITE_ID 0x01
#define LIN_MASTER_READ_ID  0x02

/**
 * @brief Length of test data
 */
#define LIN_TEST_DATA_LEN LIN_MAX_DLEN

/**
 * @brief LIN devices to be used in tests
 */
extern const struct device *const lin_master;
extern const struct device *const lin_slave;
extern const struct device *const master_phy;

/**
 * @brief LIN configurations
 */
extern const struct lin_config master_cfg;
extern const struct lin_config slave_cfg;

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
 * @brief LIN messages for master and slave
 */
extern struct lin_msg master_msg;
extern struct lin_msg slave_msg;

/**
 * @brief Test setup and teardown functions
 */
extern void *test_lin_setup(void);
extern void test_lin_before(void *f);
extern void test_lin_after(void *f);
