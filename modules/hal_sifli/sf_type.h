/*
 * SPDX-FileCopyrightText: 2019-2022 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SF_TYPE_H
#define SF_TYPE_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/timer/system_timer.h>

#define HAL_GetTick             sys_clock_elapsed

#include <bf0_hal.h>

#undef HAL_ASSERT
#define HAL_ASSERT(expr)        __ASSERT_NO_MSG(expr)
#define SF_ASSERT(expr)         __ASSERT_NO_MSG(expr)

#define SF_EOK                          0               /**< There is no error */
#define SF_ERR                          1               /**< A generic error happens */
#define SF_ETIMEOUT                     2               /**< Timed out */
#define SF_EFULL                        3               /**< The resource is full */
#define SF_EEMPTY                       4               /**< The resource is empty */
#define SF_ENOMEM                       5               /**< No memory */
#define SF_ENOSYS                       6               /**< No system */
#define SF_EBUSY                        7               /**< Busy */
#define SF_EIO                          8               /**< IO error */
#define SF_EINTR                        9               /**< Interrupted system call */
#define SF_EINVAL                       10              /**< Invalid argument */

typedef int sf_err_t;


#endif

