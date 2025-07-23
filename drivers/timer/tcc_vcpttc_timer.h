/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TIMER_TCC_VCPTTC_TIMER_H_
#define ZEPHYR_DRIVERS_TIMER_TCC_VCPTTC_TIMER_H_

#define TIMER_INDEX CONFIG_TCC_VCPTTC_TIMER_INDEX

#define TIMER_IRQ            DT_INST_IRQN(0)
#define TIMER_BASE_ADDR      DT_INST_REG_ADDR(0)
#define TIMER_CLOCK_FREQUECY DT_INST_PROP(0, clock_frequency)

#define TICKS_PER_SEC   CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define CYCLES_PER_SEC  TIMER_CLOCK_FREQUECY
#define CYCLES_PER_TICK (CYCLES_PER_SEC / TICKS_PER_SEC)

/* Maximum value of interval counter */
#define VCP_MAX_INTERVAL_COUNT 0xFFFFFFFFU

#define CYCLES_NEXT_MIN 10000
#define CYCLES_NEXT_MAX VCP_MAX_INTERVAL_COUNT

/* Register Map */
#define TMR_OP_EN_CFG    0x000UL
#define TMR_MAIN_CNT_LVD 0x004UL
#define TMR_CMP_VALUE0   0x008UL
#define TMR_CMP_VALUE1   0x00CUL
#define TMR_PSCL_CNT     0x010UL
#define TMR_MAIN_CNT     0x014UL
#define TMR_IRQ_CTRL     0x018UL

/* Configuration Value */
#define TMR_OP_EN_CFG_LDM0_ON         (1UL << 28UL)
#define TMR_OP_EN_CFG_LDM1_ON         (1UL << 29UL)
#define TMR_OP_EN_CFG_OPMODE_FREE_RUN (0UL << 26UL)
#define TMR_OP_EN_CFG_OPMODE_ONE_SHOT (1UL << 26UL)
#define TMR_OP_EN_CFG_LDZERO_OFFSET   25UL
#define TMR_OP_EN_CFG_CNT_EN          (1UL << 24UL)

/**
 * 0: Reading this register to be cleared,
 * 1: Writing a non-zero value to MASKED_IRQ_STATUS to be cleared.
 */
#define TMR_IRQ_CLR_CTRL_WRITE (1UL << 31UL)
#define TMR_IRQ_CLR_CTRL_READ  (0UL << 31UL)
#define TMR_IRQ_MASK_ALL       0x1FUL
#define TMR_IRQ_CTRL_IRQ_EN0   (1UL << 16UL)
#define TMR_IRQ_CTRL_IRQ_EN1   (2UL << 16UL)
#define TMR_IRQ_CTRL_IRQ_EN2   (4UL << 16UL)
#define TMR_IRQ_CTRL_IRQ_EN3   (8UL << 16UL)
#define TMR_IRQ_CTRL_IRQ_EN4   (16UL << 16UL)
#define TMR_IRQ_CTRL_IRQ_ALLEN                                                                     \
	((TMR_IRQ_CTRL_IRQ_EN0) | (TMR_IRQ_CTRL_IRQ_EN1) | (TMR_IRQ_CTRL_IRQ_EN2) |                \
	 (TMR_IRQ_CTRL_IRQ_EN3) | (TMR_IRQ_CTRL_IRQ_EN4))

#define TMR_PRESCALE 11UL
#define TMR_CLK_RATE (12UL * 1000UL * 1000UL)

#define VCP_MAX_INT_VAL 4294967295UL

BUILD_ASSERT(TIMER_CLOCK_FREQUECY == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "Configured system timer frequency does not match the TTC "
	     "clock frequency in the device tree");

BUILD_ASSERT(CYCLES_PER_SEC >= TICKS_PER_SEC,
	     "Timer clock frequency must be greater than the system tick "
	     "frequency");

BUILD_ASSERT((CYCLES_PER_SEC % TICKS_PER_SEC) == 0,
	     "Timer clock frequency is not divisible by the system tick "
	     "frequency");

enum vcp_timer_start_mode {
	TIMER_START_MAINCNT = 0x0UL,
	TIMER_START_ZERO = 0x1UL,
};

enum timer_channel {
	TIMER_CH_0 = 0UL,
	TIMER_CH_1 = 1UL,
	TIMER_CH_2 = 2UL,
	TIMER_CH_3 = 3UL,
	TIMER_CH_4 = 4UL,
	TIMER_CH_5 = 5UL,
	TIMER_CH_6 = 6UL,
	TIMER_CH_7 = 7UL,
	TIMER_CH_8 = 8UL,
	TIMER_CH_9 = 9UL,
	TIMER_CH_MAX = 10UL,
};

enum vcp_timer_counter_mode {
	TIMER_COUNTER_MAIN = 0UL,
	TIMER_COUNTER_COMP0 = 1UL,
	TIMER_COUNTER_COMP1 = 2UL,
	TIMER_COUNTER_SMALL_COMP = 3UL,
};

enum vcp_timer_op_mode {
	TIMER_OP_FREERUN = 0UL,
	TIMER_OP_ONESHOT
};

typedef void (*vcp_timer_handler_fn)(enum timer_channel channel, const void *arg_ptr);

struct vcp_timer_config {
	enum timer_channel cfg_channel;
	enum vcp_timer_start_mode cfg_start_mode;
	enum vcp_timer_op_mode cfg_op_mode;
	enum vcp_timer_counter_mode cfg_counter_mode;
	uint32_t cfg_main_val_usec;
	uint32_t cfg_cmp0_val_usec;
	uint32_t cfg_cmp1_val_usec;
	vcp_timer_handler_fn handler_fn;
	void *arg_ptr;
};

struct timer_resource_table {
	enum timer_channel rsc_table_channel;
	bool rsc_table_used;
	vcp_timer_handler_fn rsc_table_handler;
	void *rsc_table_arg;
};

#endif /* ZEPHYR_DRIVERS_TIMER_TCC_VCPTTC_TIMER_H_ */
