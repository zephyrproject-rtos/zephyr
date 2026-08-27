/**
 * @file drivers/stepper/adi_tmc/adi_tmc_reg.h
 *
 * @brief Common TMC5xxx register definitions and field masks
 *
 * @details This header holds only the register fields and constants that are
 * identical across the ADI Trinamic 5-series parts (TMC50xx, TMC51xx, etc.).
 * The part-specific register address maps live in each driver's own
 * `tmcXXxx_reg.h` header (e.g. `tmc50xx/tmc50xx_reg.h`).
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_COMMON_ADI_TMC_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_COMMON_ADI_TMC_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define TMC5XXX_WRITE_BIT        0x80U
#define TMC5XXX_ADDRESS_MASK     0x7FU

#define TMC5XXX_CLOCK_FREQ_SHIFT 24
#define TMC5XXX_ACCEL_CALC_SHIFT 17

#define TMC5XXX_GCONF 0x00
#define TMC5XXX_GSTAT 0x01

#define TMC5XXX_RAMPGEN_VMAX_MAX_VALUE (GENMASK(22, 9))

#define TMC5XXX_RAMPMODE_POSITIONING_MODE       0
#define TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE 1
#define TMC5XXX_RAMPMODE_NEGATIVE_VELOCITY_MODE 2
#define TMC5XXX_RAMPMODE_HOLD_MODE              3

#define TMC5XXX_SG_MIN_VALUE -64
#define TMC5XXX_SG_MAX_VALUE 63
#define TMC5XXX_SW_MODE_SG_STOP_ENABLE BIT(10)

#define TMC5XXX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT 16

#define TMC5XXX_IHOLD_MASK  GENMASK(4, 0)
#define TMC5XXX_IHOLD_SHIFT 0
#define TMC5XXX_IHOLD(n)    (((n) << TMC5XXX_IHOLD_SHIFT) & TMC5XXX_IHOLD_MASK)

#define TMC5XXX_IRUN_MASK  GENMASK(12, 8)
#define TMC5XXX_IRUN_SHIFT 8
#define TMC5XXX_IRUN(n)    (((n) << TMC5XXX_IRUN_SHIFT) & TMC5XXX_IRUN_MASK)

#define TMC5XXX_IHOLDDELAY_MASK  GENMASK(19, 16)
#define TMC5XXX_IHOLDDELAY_SHIFT 16
#define TMC5XXX_IHOLDDELAY(n)    (((n) << TMC5XXX_IHOLDDELAY_SHIFT) & TMC5XXX_IHOLDDELAY_MASK)

#define TMC5XXX_CHOPCONF_DRV_ENABLE_MASK GENMASK(3, 0)
#define TMC5XXX_CHOPCONF_MRES_MASK       GENMASK(27, 24)
#define TMC5XXX_CHOPCONF_MRES_SHIFT      24

#define TMC5XXX_RAMPSTAT_INT_MASK  GENMASK(9, 4)
#define TMC5XXX_RAMPSTAT_INT_SHIFT 4

#define TMC5XXX_RAMPSTAT_POS_REACHED_MASK BIT(9)
#define TMC5XXX_POS_REACHED									   \
	(TMC5XXX_RAMPSTAT_POS_REACHED_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_RAMPSTAT_POS_REACHED_EVENT_MASK BIT(7)
#define TMC5XXX_POS_REACHED_EVENT                                                                  \
	(TMC5XXX_RAMPSTAT_POS_REACHED_EVENT_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_POS_REACHED_AND_EVENT (TMC5XXX_POS_REACHED | TMC5XXX_POS_REACHED_EVENT)

#define TMC5XXX_RAMPSTAT_STOP_SG_EVENT_MASK BIT(6)
#define TMC5XXX_STOP_SG_EVENT                                                                      \
	(TMC5XXX_RAMPSTAT_STOP_SG_EVENT_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_RAMPSTAT_STOP_RIGHT_EVENT_MASK BIT(5)
#define TMC5XXX_STOP_RIGHT_EVENT                                                                   \
	(TMC5XXX_RAMPSTAT_STOP_RIGHT_EVENT_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_RAMPSTAT_STOP_LEFT_EVENT_MASK BIT(4)
#define TMC5XXX_STOP_LEFT_EVENT                                                                    \
	(TMC5XXX_RAMPSTAT_STOP_LEFT_EVENT_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_DRV_STATUS_STST_BIT        BIT(31)
#define TMC5XXX_DRV_STATUS_SG_RESULT_MASK  GENMASK(9, 0)
#define TMC5XXX_DRV_STATUS_SG_STATUS_MASK  BIT(24)
#define TMC5XXX_DRV_STATUS_SG_STATUS_SHIFT 24

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_COMMON_ADI_TMC_REG_H_ */
