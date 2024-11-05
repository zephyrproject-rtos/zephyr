/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WATCHDOG_WDT_RTS_RTS5817_H_
#define ZEPHYR_DRIVERS_WATCHDOG_WDT_RTS_RTS5817_H_

/* Bits of R_WTDG_CTRL (0X0048) */

#define WDOG_EN_OFFSET 0
#define WDOG_EN_BITS   1
#define WDOG_EN_MASK   (((1 << 1) - 1) << 0)
#define WDOG_EN        (WDOG_EN_MASK)

#define WDOG_CLR_OFFSET 1
#define WDOG_CLR_BITS   1
#define WDOG_CLR_MASK   (((1 << 1) - 1) << 1)
#define WDOG_CLR        (WDOG_CLR_MASK)

#define WDOG_TIME_OFFSET 2
#define WDOG_TIME_BITS   2
#define WDOG_TIME_MASK   (((1 << 2) - 1) << 2)
#define WDOG_TIME        (WDOG_TIME_MASK)

#define WDOG_REG_RST_EN_OFFSET 4
#define WDOG_REG_RST_EN_BITS   1
#define WDOG_REG_RST_EN_MASK   (((1 << 1) - 1) << 4)
#define WDOG_REG_RST_EN        (WDOG_REG_RST_EN_MASK)

#define WDOG_BUS_RST_EN_OFFSET 5
#define WDOG_BUS_RST_EN_BITS   1
#define WDOG_BUS_RST_EN_MASK   (((1 << 1) - 1) << 5)
#define WDOG_BUS_RST_EN        (WDOG_BUS_RST_EN_MASK)

#define WDOG_INT_EN_OFFSET 6
#define WDOG_INT_EN_BITS   1
#define WDOG_INT_EN_MASK   (((1 << 1) - 1) << 6)
#define WDOG_INT_EN        (WDOG_INT_EN_MASK)

#define WDOG_INT_CLR_OFFSET 7
#define WDOG_INT_CLR_BITS   1
#define WDOG_INT_CLR_MASK   (((1 << 1) - 1) << 7)
#define WDOG_INT_CLR        (WDOG_INT_CLR_MASK)

#define WDOG_INT_OFFSET 8
#define WDOG_INT_BITS   1
#define WDOG_INT_MASK   (((1 << 1) - 1) << 8)
#define WDOG_INT        (WDOG_INT_MASK)

#define WDOG_INT_SEL_OFFSET 9
#define WDOG_INT_SEL_BITS   1
#define WDOG_INT_SEL_MASK   (((1 << 1) - 1) << 9)
#define WDOG_INT_SEL        (WDOG_INT_SEL_MASK)

#define WDOG_CNT_OFFSET 16
#define WDOG_CNT_BITS   16
#define WDOG_CNT_MASK   (((1 << 16) - 1) << 16)
#define WDOG_CNT        (WDOG_CNT_MASK)

#endif /* ZEPHYR_DRIVERS_WATCHDOG_WDT_RTS_RTS5817_H_ */
