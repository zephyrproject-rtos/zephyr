/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_H__
#define __OS_WRAPPER_H__

/**
 * @brief  Necessary headers
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "platform_autoconf.h"
#include "ameba.h"
#include <zephyr/kernel.h>


/**
 * @brief  General macro definition
 */

#define RTOS_MAX_DELAY    0xFFFFFFFFUL
#define RTOS_MAX_TIMEOUT  0xFFFFFFFFUL
#define RTOS_TICK_RATE_HZ 1000UL

#endif
