/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
