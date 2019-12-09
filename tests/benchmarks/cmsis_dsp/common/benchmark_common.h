/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_BENCHMARK_CMSIS_DSP_COMMON_BENCHMARK_COMMON_H_
#define ZEPHYR_BENCHMARK_CMSIS_DSP_COMMON_BENCHMARK_COMMON_H_

#include <ztest.h>
#include <zephyr.h>

#if defined(CONFIG_CPU_CORTEX_M_HAS_DWT)
/* Use cycle counting on the Cortex-M devices that support DWT */

#include <arch/arm/cortex_m/cmsis.h>

#define BENCHMARK_BEGIN()                   \
    uint32_t irq_key;                       \
    irq_key = irq_lock();                   \
    DWT->CYCCNT = 0;                        \
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

#define BENCHMARK_END()                     \
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;   \
    irq_unlock(irq_key);

#define BENCHMARK_TIME      (DWT->CYCCNT)

#define BENCHMARK_TYPE      "Processor Cycles"

#else
/* Use system timer clock on other systems */

#define BENCHMARK_BEGIN()               \
    uint32_t irq_key;                   \
    volatile uint32_t begin_timestamp;  \
    volatile uint32_t end_timestamp;    \
    irq_key = irq_lock();               \
    begin_timestamp = k_cycle_get_32();

#define BENCHMARK_END()                 \
    end_timestamp = k_cycle_get_32();   \
    irq_unlock(irq_key);

#define BENCHMARK_TIME      (end_timestamp - begin_timestamp)

#define BENCHMARK_TYPE      "System Timer Cycles"

#endif

#endif /* ZEPHYR_BENCHMARK_CMSIS_DSP_COMMON_BENCHMARK_COMMON_H_ */
