/*
 * Copyright (c) 2024 Dhiru Kholia
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CH32FUN_H
#define _CH32FUN_H

#if defined(CONFIG_SOC_CH32V003)
#define CH32V003 1
#include <ch32v003fun.h>
#endif

#if defined(CONFIG_SOC_CH32V208)
#define CH32V20x 1
#define CH32V20x_D8W 1
#include <ch32v003fun.h>
#endif

#endif
