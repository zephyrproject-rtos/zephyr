/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hal_mock.c
 * @brief Mock implementations of HAL interfaces for ticker unit tests
 *
 * This file provides mock implementations of hardware abstraction layer
 * (HAL) interfaces required by the ticker module for unit testing.
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

/* Global variables for mock control */
static uint32_t mock_cntr_ticks = 0;
static uint32_t mock_cntr_increment = 1;

/**
 * Mock implementation of cntr_cnt_get
 * Returns a simulated counter value for testing
 */
uint32_t cntr_cnt_get(void)
{
	uint32_t current = mock_cntr_ticks;
	mock_cntr_ticks += mock_cntr_increment;
	return current;
}

/**
 * Mock implementation of cntr_cmp_set
 * Sets the compare value for the counter (no-op in mock)
 */
void cntr_cmp_set(uint8_t cmp, uint32_t value)
{
	/* Mock implementation - no actual hardware operation */
	(void)cmp;
	(void)value;
}

/**
 * Mock implementation of cpu_dmb
 * Data memory barrier (no-op in mock)
 */
void cpu_dmb(void)
{
	/* Mock implementation - no actual memory barrier needed in tests */
}

/**
 * Reset mock counter to initial state
 */
void hal_mock_reset(void)
{
	mock_cntr_ticks = 0;
	mock_cntr_increment = 1;
}

/**
 * Set mock counter increment value
 */
void hal_mock_set_increment(uint32_t increment)
{
	mock_cntr_increment = increment;
}

/**
 * Set mock counter ticks value
 */
void hal_mock_set_ticks(uint32_t ticks)
{
	mock_cntr_ticks = ticks;
}

/**
 * Get current mock counter ticks value
 */
uint32_t hal_mock_get_ticks(void)
{
	return mock_cntr_ticks;
}
