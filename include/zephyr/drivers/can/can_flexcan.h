/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FlexCAN driver timing limits and devicetree helper macros
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FLEXCAN_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FLEXCAN_H_

#include <zephyr/drivers/can.h>

/**
 * @defgroup can_flexcan FlexCAN Driver API
 * @ingroup can_interface
 * @{
 */

/**
 * @defgroup can_flexcan_timing_limits FlexCAN Timing Limits
 * @{
 */

/*
 * FlexCAN CTRL1 Register Timing Limits (CAN Classic)
 *
 * Used for FlexCAN instances that only support CAN classic with
 * bit timing configuration via the CTRL1 register.
 *
 * Register field limits (physical values, not register values):
 * - SJW (Sync Jump Width): 1 - 4
 * - PROP_SEG (Propagation Segment): 1 - 8
 * - PHASE_SEG1 (Phase Segment 1): 1 - 8
 * - PHASE_SEG2 (Phase Segment 2): 2 - 8
 * - PRESCALER (Prescaler): 1 - 256
 */

/** CTRL1 timing minimum limits initializer */
#define CAN_FLEXCAN_CTRL1_TIMING_MIN_INITIALIZER \
	{ \
		.sjw = 1, \
		.prop_seg = 1, \
		.phase_seg1 = 1, \
		.phase_seg2 = 2, \
		.prescaler = 1 \
	}

/** CTRL1 timing maximum limits initializer */
#define CAN_FLEXCAN_CTRL1_TIMING_MAX_INITIALIZER \
	{ \
		.sjw = 4, \
		.prop_seg = 8, \
		.phase_seg1 = 8, \
		.phase_seg2 = 8, \
		.prescaler = 256 \
	}

/*
 * FlexCAN CBT/FDCBT Register Timing Limits (CAN Classic and CAN FD)
 *
 * Used for FlexCAN instances that support CAN classic and CAN FD with
 * CBT (CAN Bit Timing) register for arbitration phase and FDCBT
 * (CAN FD Bit Timing) register for data phase.
 *
 * Arbitration Phase (CBT Register):
 * - SJW: 1 - 32
 * - PROP_SEG: 1 - 64
 * - PHASE_SEG1: 1 - 32
 * - PHASE_SEG2: 2 - 32
 * - PRESCALER: 1 - 1024
 *
 * Data Phase (FDCBT Register):
 * - SJW: 1 - 8
 * - PROP_SEG: 1 - 31
 * - PHASE_SEG1: 1 - 8
 * - PHASE_SEG2: 2 - 8
 * - PRESCALER: 1 - 1024
 */

/** CBT arbitration phase timing minimum limits initializer */
#define CAN_FLEXCAN_CBT_TIMING_MIN_INITIALIZER \
	{ \
		.sjw = 1, \
		.prop_seg = 1, \
		.phase_seg1 = 1, \
		.phase_seg2 = 2, \
		.prescaler = 1 \
	}

/** CBT arbitration phase timing maximum limits initializer */
#define CAN_FLEXCAN_CBT_TIMING_MAX_INITIALIZER \
	{ \
		.sjw = 32, \
		.prop_seg = 64, \
		.phase_seg1 = 32, \
		.phase_seg2 = 32, \
		.prescaler = 1024 \
	}

/** FDCBT data phase timing minimum limits initializer */
#define CAN_FLEXCAN_FDCBT_TIMING_DATA_MIN_INITIALIZER \
	{ \
		.sjw = 1, \
		.prop_seg = 1, \
		.phase_seg1 = 1, \
		.phase_seg2 = 2, \
		.prescaler = 1 \
	}

/** FDCBT data phase timing maximum limits initializer */
#define CAN_FLEXCAN_FDCBT_TIMING_DATA_MAX_INITIALIZER \
	{ \
		.sjw = 8, \
		.prop_seg = 31, \
		.phase_seg1 = 8, \
		.phase_seg2 = 8, \
		.prescaler = 1024 \
	}

/*
 * FlexCAN ENCBT/EDCBT Register Timing Limits (CAN Classic and CAN FD)
 *
 * Used for FlexCAN instances that support CAN classic and CAN FD with
 * enhanced bit timing registers ENCBT (Enhanced Nominal CAN Bit Timing)
 * and EDCBT (Enhanced Data Phase CAN Bit Timing).
 *
 * These registers provide extended timing ranges and DO NOT have a separate
 * propagation segment. The propagation delay is merged into PHASE_SEG1.
 *
 * Arbitration Phase (ENCBT Register):
 * - SJW: 1 - 128
 * - PROP_SEG: 0 (no independent propagation segment)
 * - PHASE_SEG1: 2 - 256
 * - PHASE_SEG2: 2 - 128
 * - PRESCALER: 1 - 1024
 *
 * Data Phase (EDCBT Register):
 * - SJW: 1 - 16
 * - PROP_SEG: 0 (no independent propagation segment)
 * - PHASE_SEG1: 2 - 32
 * - PHASE_SEG2: 2 - 16
 * - PRESCALER: 1 - 1024
 */

/** ENCBT arbitration phase timing minimum limits initializer */
#define CAN_FLEXCAN_ENCBT_TIMING_MIN_INITIALIZER \
	{ \
		.sjw = 1, \
		.prop_seg = 0, \
		.phase_seg1 = 2, \
		.phase_seg2 = 2, \
		.prescaler = 1 \
	}

/** ENCBT arbitration phase timing maximum limits initializer */
#define CAN_FLEXCAN_ENCBT_TIMING_MAX_INITIALIZER \
	{ \
		.sjw = 128, \
		.prop_seg = 0, \
		.phase_seg1 = 256, \
		.phase_seg2 = 128, \
		.prescaler = 1024 \
	}

/** EDCBT data phase timing minimum limits initializer */
#define CAN_FLEXCAN_EDCBT_TIMING_DATA_MIN_INITIALIZER \
	{ \
		.sjw = 1, \
		.prop_seg = 0, \
		.phase_seg1 = 2, \
		.phase_seg2 = 2, \
		.prescaler = 1 \
	}

/** EDCBT data phase timing maximum limits initializer */
#define CAN_FLEXCAN_EDCBT_TIMING_DATA_MAX_INITIALIZER \
	{ \
		.sjw = 16, \
		.prop_seg = 0, \
		.phase_seg1 = 32, \
		.phase_seg2 = 16, \
		.prescaler = 1024 \
	}

/** @} */

/**
 * @defgroup can_flexcan_timing_regs FlexCAN Timing Registers Index
 * @{
 */

/** CTRL1 timing register index */
#define CAN_FLEXCAN_TIMING_REGISTERS_CTRL1	0
/** CBT timing register index */
#define CAN_FLEXCAN_TIMING_REGISTERS_CBT	1
/** ENCBT timing register index */
#define CAN_FLEXCAN_TIMING_REGISTERS_ENCBT	2

/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FLEXCAN_H_ */
