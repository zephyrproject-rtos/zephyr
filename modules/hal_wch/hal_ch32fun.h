/*
 * Copyright (c) 2024 Dhiru Kholia
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: See this table for IC family reference,
 * in conjunction with Page 5 of the reference manual:
 * https://www.wch-ic.com/products/productsCenter/mcuInterface?categoryId=70
 */

#ifndef _CH32FUN_H
#define _CH32FUN_H

#if defined(CONFIG_SOC_CH32V003)
#define CH32V003 1
#include <ch32fun.h>
#endif /* defined(CONFIG_SOC_CH32V003) */

#if defined(CONFIG_SOC_SERIES_CH32V00X)
#define CH32V00x 1
#include <ch32fun.h>
#endif /* defined(CONFIG_SOC_SERIES_CH32V00X) */

#if defined(CONFIG_SOC_SERIES_QINGKE_V4B)
#define CH32V20x    1
#define CH32V20x_D6 1
#include <ch32fun.h>
#endif /* defined(CONFIG_SOC_SERIES_QINGKE_V4B) */

#if defined(CONFIG_SOC_SERIES_QINGKE_V4C)
#define CH32V20x 1
#if defined(CONFIG_SOC_CH32V208)
#define CH32V20x_D8W 1
#endif
#include <ch32fun.h>
#endif /* defined(CONFIG_SOC_SERIES_QINGKE_V4C) */

#if defined(CONFIG_SOC_SERIES_QINGKE_V4F)
#define CH32V30x 1
#if defined(CONFIG_SOC_CH32V303)
#define CH32V30x_D8 1
#elif defined(CONFIG_SOC_CH32V307)
#define CH32V30x_D8C 1
#endif
#include <ch32fun.h>
#endif /* defined(CONFIG_SOC_SERIES_QINGKE_V4F) */

#endif
