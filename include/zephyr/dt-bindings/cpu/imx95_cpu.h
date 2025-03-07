/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CPU_IMX95_CPU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CPU_IMX95_CPU_H_

#define CPU_IDX_M33P        0U
#define CPU_IDX_M7P         1U
#define CPU_IDX_A55C0       2U
#define CPU_IDX_A55C1       3U
#define CPU_IDX_A55C2       4U
#define CPU_IDX_A55C3       5U
#define CPU_IDX_A55C4       6U
#define CPU_IDX_A55C5       7U
#define CPU_IDX_A55P        8U
#define CPU_NUM_IDX         9U

#define CPU_SLEEP_MODE_RUN      0U
#define CPU_SLEEP_MODE_WAIT     1U
#define CPU_SLEEP_MODE_STOP     2U
#define CPU_SLEEP_MODE_SUSPEND  3U

/*! Select between GPC or GIC for wakeup */
#define CPU_SLEEP_FLAG_IRQ_MUX    0x1U
/*! Wake A55 CPU during A55 platform wakeup */
#define CPU_SLEEP_FLAG_A55P_WAKE  0x2U

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CPU_IMX95_CPU_H_ */
