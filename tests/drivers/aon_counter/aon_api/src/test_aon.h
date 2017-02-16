/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AON cases header file
 *
 * Header file for AON cases
 */

#ifndef __TEST_AON_H__
#define __TEST_AON_H__

#include <counter.h>
#include <zephyr.h>
#include <ztest.h>

#define AON_COUNTER CONFIG_AON_COUNTER_QMSI_DEV_NAME
#define AON_TIMER CONFIG_AON_TIMER_QMSI_DEV_NAME

void test_aon_counter(void);
void test_aon_periodic_timer(void);

#endif /* __TEST_AON_H__ */
