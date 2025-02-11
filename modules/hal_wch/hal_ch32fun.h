/*
 * Copyright (c) 2024 Dhiru Kholia
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CH32FUN_H
#define _CH32FUN_H

#if defined(CONFIG_SOC_CH32V003)
#define CH32V003 1
#include <ch32fun.h>
#endif

#if defined(CONFIG_SOC_SERIES_CH32V00X)
#define CH32V003 1
#include <ch32fun.h>
#endif

#if defined(CONFIG_SOC_SERIES_QINGKE_V4B)
#define CH32V20x 1
#include <ch32fun.h>
#endif

#if defined(CONFIG_SOC_SERIES_QINGKE_V4C)
#define CH32V20x 1
#include <ch32fun.h>
#endif

#if defined(CONFIG_SOC_SERIES_QINGKE_V4F)
#define CH32V30x 1
#if defined(CONFIG_SOC_CH32V303)
#define CH32V30x_D8 1
#endif

#include <ch32fun.h>
#endif

#endif
