/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMBIQ_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMBIQ_CLOCK_H_

/* System Clock Source */
#define AMBIQ_CLK_SRC_XTAL      0U
#define AMBIQ_CLK_SRC_LFRC      1U
#define AMBIQ_CLK_SRC_HFRC      2U

/* Supported CPU Frequencies */
#define AMBIQ_CLK_CPU_48M       48U
#define AMBIQ_CLK_CPU_96M       96U

#define AMBIQ_TIMERS_MODULE     0
#define AMBIQ_UART0_MODULE

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMBIQ_CLOCK_H_ */
