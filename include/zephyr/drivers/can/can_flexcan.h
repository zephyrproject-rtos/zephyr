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
 * @cond INTERNAL_HIDDEN
 *
 * Hardware register definitions for internal driver use only.
 */

/* CTRL1 register offset */
#define CAN_FLEXCAN_CTRL1		0x4
/* CTRL1 prescaler division factor field */
#define CAN_FLEXCAN_CTRL1_PRESDIV	GENMASK(31, 24)
/* CTRL1 resync jump width field */
#define CAN_FLEXCAN_CTRL1_RJW		GENMASK(23, 22)
/* CTRL1 phase segment 1 field */
#define CAN_FLEXCAN_CTRL1_PSEG1		GENMASK(21, 19)
/* CTRL1 phase segment 2 field */
#define CAN_FLEXCAN_CTRL1_PSEG2		GENMASK(18, 16)
/* CTRL1 propagation segment field */
#define CAN_FLEXCAN_CTRL1_PROPSEG	GENMASK(2, 0)


/* CBT register offset */
#define CAN_FLEXCAN_CBT			0x50
/* CBT bit timing format enable */
#define CAN_FLEXCAN_CBT_BTF		BIT(31)
/* CBT extended prescaler division factor field */
#define CAN_FLEXCAN_CBT_EPRESDIV	GENMASK(30, 21)
/* CBT extended resync jump width field */
#define CAN_FLEXCAN_CBT_ERJW		GENMASK(20, 16)
/* CBT extended propagation segment field */
#define CAN_FLEXCAN_CBT_EPROPSEG	GENMASK(15, 10)
/* CBT extended phase segment 1 field */
#define CAN_FLEXCAN_CBT_EPSEG1		GENMASK(9, 5)
/* CBT extended phase segment 2 field */
#define CAN_FLEXCAN_CBT_EPSEG2		GENMASK(4, 0)


/* FDCBT register offset */
#define CAN_FLEXCAN_FDCBT		0xC04
/* FDCBT fast prescaler division factor field */
#define CAN_FLEXCAN_FDCBT_FPRESDIV	GENMASK(29, 20)
/* FDCBT fast resync jump width field */
#define CAN_FLEXCAN_FDCBT_FRJW		GENMASK(18, 16)
/* FDCBT fast propagation segment field */
#define CAN_FLEXCAN_FDCBT_FPROPSEG	GENMASK(14, 10)
/* FDCBT fast phase segment 1 field */
#define CAN_FLEXCAN_FDCBT_FPSEG1	GENMASK(7, 5)
/* FDCBT fast phase segment 2 field */
#define CAN_FLEXCAN_FDCBT_FPSEG2	GENMASK(2, 0)


/* FDCTRL register offset */
#define CAN_FLEXCAN_FDCTRL		0xC00
/* FDCTRL transceiver delay compensation enable */
#define CAN_FLEXCAN_FDCTRL_TDCEN	BIT(15)
/* FDCTRL transceiver delay compensation offset field */
#define CAN_FLEXCAN_FDCTRL_TDCOFF	GENMASK(12, 8)


/* CTRL2 register offset */
#define CAN_FLEXCAN_CTRL2		0x34
/* CTRL2 bit timing expansion enable */
#define CAN_FLEXCAN_CTRL2_BTE		BIT(13)


/* EPRS register offset */
#define CAN_FLEXCAN_EPRS		0xBF0
/* EPRS extended data phase prescaler division factor field */
#define CAN_FLEXCAN_EPRS_EDPRESDIV	GENMASK(25, 16)
/* EPRS extended nominal phase prescaler division factor field */
#define CAN_FLEXCAN_EPRS_ENPRESDIV	GENMASK(9, 0)


/* ENCBT register offset */
#define CAN_FLEXCAN_ENCBT		0xBF4
/* ENCBT nominal resync jump width field */
#define CAN_FLEXCAN_ENCBT_NRJW		GENMASK(28, 22)
/* ENCBT nominal time segment 2 field */
#define CAN_FLEXCAN_ENCBT_NTSEG2	GENMASK(18, 12)
/* ENCBT nominal time segment 1 field */
#define CAN_FLEXCAN_ENCBT_NTSEG1	GENMASK(7, 0)


/* EDCBT register offset */
#define CAN_FLEXCAN_EDCBT		0xBF8
/* EDCBT data phase resync jump width field */
#define CAN_FLEXCAN_EDCBT_DRJW		GENMASK(25, 22)
/* EDCBT data phase time segment 2 field */
#define CAN_FLEXCAN_EDCBT_DTSEG2	GENMASK(15, 12)
/* EDCBT data phase time segment 1 field */
#define CAN_FLEXCAN_EDCBT_DTSEG1	GENMASK(4, 0)


/* ETDC register offset */
#define CAN_FLEXCAN_ETDC		0xBFC
/* ETDC transceiver delay compensation enable */
#define CAN_FLEXCAN_ETDC_ETDCEN		BIT(31)
/* ETDC transceiver delay measurement disable */
#define CAN_FLEXCAN_ETDC_TDMDIS		BIT(30)
/* ETDC enhanced transceiver delay compensation offset field */
#define CAN_FLEXCAN_ETDC_ETDCOFF	GENMASK(22, 16)

/**
 * @endcond
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
 * @defgroup can_flexcan_dt_helpers FlexCAN Devicetree Helper Macros
 * @brief Devicetree macros to get timing limits based on timing-registers property
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

/* Helper: CTRL1 timing min initializer indexed by enum value 0 */
#define FLEXCAN_TIMING_MIN_0 CAN_FLEXCAN_CTRL1_TIMING_MIN_INITIALIZER
/* Helper: CBT timing min initializer indexed by enum value 1 */
#define FLEXCAN_TIMING_MIN_1 CAN_FLEXCAN_CBT_TIMING_MIN_INITIALIZER
/* Helper: ENCBT timing min initializer indexed by enum value 2 */
#define FLEXCAN_TIMING_MIN_2 CAN_FLEXCAN_ENCBT_TIMING_MIN_INITIALIZER

/* Helper: CTRL1 timing max initializer indexed by enum value 0 */
#define FLEXCAN_TIMING_MAX_0 CAN_FLEXCAN_CTRL1_TIMING_MAX_INITIALIZER
/* Helper: CBT timing max initializer indexed by enum value 1 */
#define FLEXCAN_TIMING_MAX_1 CAN_FLEXCAN_CBT_TIMING_MAX_INITIALIZER
/* Helper: ENCBT timing max initializer indexed by enum value 2 */
#define FLEXCAN_TIMING_MAX_2 CAN_FLEXCAN_ENCBT_TIMING_MAX_INITIALIZER

/* Helper: FDCBT data phase timing min initializer indexed by enum value 1 */
#define FLEXCAN_TIMING_DATA_MIN_1 CAN_FLEXCAN_FDCBT_TIMING_DATA_MIN_INITIALIZER
/* Helper: EDCBT data phase timing min initializer indexed by enum value 2 */
#define FLEXCAN_TIMING_DATA_MIN_2 CAN_FLEXCAN_EDCBT_TIMING_DATA_MIN_INITIALIZER

/* Helper: FDCBT data phase timing max initializer indexed by enum value 1 */
#define FLEXCAN_TIMING_DATA_MAX_1 CAN_FLEXCAN_FDCBT_TIMING_DATA_MAX_INITIALIZER
/* Helper: EDCBT data phase timing max initializer indexed by enum value 2 */
#define FLEXCAN_TIMING_DATA_MAX_2 CAN_FLEXCAN_EDCBT_TIMING_DATA_MAX_INITIALIZER

/**
 * @endcond
 */

/**
 * @brief Get the minimum timing limits for FlexCAN arbitration phase.
 *
 * This macro selects the timing limits initializer based on the devicetree
 * `timing-registers` enum value for the given instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @return A `struct can_timing` initializer for the minimum arbitration phase limits.
 */
#define CAN_FLEXCAN_DT_INST_TIMING_MIN(inst)		\
	UTIL_CAT(FLEXCAN_TIMING_MIN_, DT_INST_ENUM_IDX(inst, timing_registers))

/**
 * @brief Get the maximum timing limits for FlexCAN arbitration phase.
 *
 * This macro selects the timing limits initializer based on the devicetree
 * `timing-registers` enum value for the given instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @return A `struct can_timing` initializer for the maximum arbitration phase limits.
 */
#define CAN_FLEXCAN_DT_INST_TIMING_MAX(inst)		\
	UTIL_CAT(FLEXCAN_TIMING_MAX_, DT_INST_ENUM_IDX(inst, timing_registers))

/**
 * @brief Get the minimum timing limits for FlexCAN data phase (CAN FD).
 *
 * This macro selects the timing limits initializer based on the devicetree
 * `timing-registers` enum value for the given instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @return A `struct can_timing` initializer for the minimum data phase limits.
 */
#define CAN_FLEXCAN_DT_INST_TIMING_DATA_MIN(inst)	\
	UTIL_CAT(FLEXCAN_TIMING_DATA_MIN_, DT_INST_ENUM_IDX(inst, timing_registers))

/**
 * @brief Get the maximum timing limits for FlexCAN data phase (CAN FD).
 *
 * This macro selects the timing limits initializer based on the devicetree
 * `timing-registers` enum value for the given instance.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @return A `struct can_timing` initializer for the maximum data phase limits.
 */
#define CAN_FLEXCAN_DT_INST_TIMING_DATA_MAX(inst)	\
	UTIL_CAT(FLEXCAN_TIMING_DATA_MAX_, DT_INST_ENUM_IDX(inst, timing_registers))

/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAN_CAN_FLEXCAN_H_ */
