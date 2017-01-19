/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AIO comparator test cases header file
 */

#ifndef __TEST_AIO_H__
#define __TEST_AIO_H__

#include <zephyr.h>
#include <aio_comparator.h>
#include <gpio.h>
#include <ztest.h>

#define AIO_CMP_DEV_NAME	CONFIG_AIO_COMPARATOR_0_NAME
#define GPIO_DEV_NAME		CONFIG_GPIO_QMSI_0_NAME

void test_aio_callback_rise(void);
void test_aio_callback_rise_disable(void);

#if defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD)
#define PIN_OUT	17 /* GPIO17_I2S_RWS */
#define PIN_IN	15 /* GPIO_SS_AIN_15 */
#endif

#endif /* __TEST_AIO_H__ */
