/**
 * @file drivers/stepper/adi_tmc/tmc524x/tmc524x_reg.h
 *
 * @brief TMC524X Registers
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC524X_TMC524X_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC524X_TMC524X_REG_H_

#include <adi_tmc_reg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name TMC524X module registers
 * @anchor TMC524X_REGISTERS
 *
 * @{
 */

#define TMC524X_GCONF_EN_PWM_MODE_SHIFT        2
#define TMC524X_GCONF_SHAFT_SHIFT              4
#define TMC524X_GCONF_DIAG0_INT_PUSHPULL_SHIFT 12
#define TMC524X_GCONF_TEST_MODE_SHIFT          17

#define TMC524X_IHOLD_IRUN	0x10
#define TMC524X_TPOWER_DOWN	0x11
#define TMC524X_TSTEP		0x12
#define TMC524X_TPWMTHRS	0x13
#define TMC524X_TCOOLTHRS	0x14
#define TMC524X_THIGH		0x15

#define TMC524X_RAMPMODE	0x20
#define TMC524X_XACTUAL		0x21
#define TMC524X_VACTUAL		0x22
#define TMC524X_VSTART		0x23
#define TMC524X_A1		0x24
#define TMC524X_V1		0x25
#define TMC524X_AMAX		0x26
#define TMC524X_VMAX		0x27
#define TMC524X_DMAX		0x28
#define TMC524X_D1		0x2A
#define TMC524X_VSTOP		0x2B
#define TMC524X_TZEROWAIT	0x2C
#define TMC524X_XTARGET		0x2D
#define TMC524X_SWMODE		0x34
#define TMC524X_RAMPSTAT	0x35
#define TMC524X_CHOPCONF	0x6C
#define TMC524X_COOLCONF	0x6D
#define TMC524X_DRVSTATUS	0x6F

/*
 * Field masks are defined only for the multi-bit fields programmed or read by
 * the driver.
 */

/* Position compare */
#define TMC524X_X_COMPARE        0x05
#define TMC524X_X_COMPARE_REPEAT 0x06

/* Integrated current sense driver configuration */
#define TMC524X_DRV_CONF      0x0A
#define TMC524X_GLOBAL_SCALER 0x0B

#define TMC524X_DRV_CONF_CURRENT_RANGE_MASK  GENMASK(1, 0)
#define TMC524X_DRV_CONF_CURRENT_RANGE_SHIFT 0
#define TMC524X_DRV_CONF_SLOPE_CONTROL_MASK  GENMASK(5, 4)
#define TMC524X_DRV_CONF_SLOPE_CONTROL_SHIFT 4

#define TMC524X_GLOBAL_SCALER_MASK  GENMASK(7, 0)
#define TMC524X_GLOBAL_SCALER_SHIFT 0

/* Eight point ramp generator extensions */
#define TMC524X_TVMAX   0x29
#define TMC524X_V2      0x2E
#define TMC524X_A2      0x2F
#define TMC524X_D2      0x30
#define TMC524X_AACTUAL 0x31

/* DcStep */
#define TMC524X_VDCMIN 0x33
#define TMC524X_DCCTRL 0x6E

#define TMC524X_DCCTRL_DC_TIME_MASK  GENMASK(9, 0)
#define TMC524X_DCCTRL_DC_TIME_SHIFT 0
#define TMC524X_DCCTRL_DC_SG_MASK    GENMASK(23, 16)
#define TMC524X_DCCTRL_DC_SG_SHIFT   16

/* Encoder interface */
#define TMC524X_ENCMODE       0x38
#define TMC524X_XENC          0x39
#define TMC524X_ENC_CONST     0x3A
#define TMC524X_ENC_STATUS    0x3B
#define TMC524X_ENC_LATCH     0x3C
#define TMC524X_ENC_DEVIATION 0x3D

/* Virtual reference switches */
#define TMC524X_VIRTUAL_STOP_L 0x3E
#define TMC524X_VIRTUAL_STOP_R 0x3F

/* On-chip ADC and protection thresholds */
#define TMC524X_ADC_VSUPPLY_AIN 0x50
#define TMC524X_ADC_TEMP        0x51
#define TMC524X_OTW_OV_VTH      0x52

#define TMC524X_ADC_VSUPPLY_MASK  GENMASK(12, 0)
#define TMC524X_ADC_VSUPPLY_SHIFT 0
#define TMC524X_ADC_AIN_MASK      GENMASK(28, 16)
#define TMC524X_ADC_AIN_SHIFT     16
#define TMC524X_ADC_TEMP_MASK     GENMASK(12, 0)
#define TMC524X_ADC_TEMP_SHIFT    0

#define TMC524X_OVERVOLTAGE_VTH_MASK         GENMASK(12, 0)
#define TMC524X_OVERVOLTAGE_VTH_SHIFT        0
#define TMC524X_OVERTEMPPREWARNING_VTH_MASK  GENMASK(28, 16)
#define TMC524X_OVERTEMPPREWARNING_VTH_SHIFT 16

/* Custom microstep table */
#define TMC524X_MSLUT0     0x60
#define TMC524X_MSLUT1     0x61
#define TMC524X_MSLUT2     0x62
#define TMC524X_MSLUT3     0x63
#define TMC524X_MSLUT4     0x64
#define TMC524X_MSLUT5     0x65
#define TMC524X_MSLUT6     0x66
#define TMC524X_MSLUT7     0x67
#define TMC524X_MSLUTSEL   0x68
#define TMC524X_MSLUTSTART 0x69
#define TMC524X_MSCNT      0x6A
#define TMC524X_MSCURACT   0x6B

/* StealthChop2 PWM tuning */
#define TMC524X_PWMCONF   0x70
#define TMC524X_PWM_SCALE 0x71
#define TMC524X_PWM_AUTO  0x72

#define TMC524X_PWMCONF_PWM_OFS_MASK           GENMASK(7, 0)
#define TMC524X_PWMCONF_PWM_OFS_SHIFT          0
#define TMC524X_PWMCONF_PWM_GRAD_MASK          GENMASK(15, 8)
#define TMC524X_PWMCONF_PWM_GRAD_SHIFT         8
#define TMC524X_PWMCONF_PWM_FREQ_MASK          GENMASK(17, 16)
#define TMC524X_PWMCONF_PWM_FREQ_SHIFT         16
#define TMC524X_PWMCONF_PWM_AUTOSCALE_SHIFT    18
#define TMC524X_PWMCONF_PWM_AUTOGRAD_SHIFT     19
#define TMC524X_PWMCONF_FREEWHEEL_MASK         GENMASK(21, 20)
#define TMC524X_PWMCONF_FREEWHEEL_SHIFT        20
#define TMC524X_PWMCONF_PWM_MEAS_SD_EN_SHIFT   22
#define TMC524X_PWMCONF_PWM_DIS_REG_STST_SHIFT 23
#define TMC524X_PWMCONF_PWM_REG_MASK           GENMASK(27, 24)
#define TMC524X_PWMCONF_PWM_REG_SHIFT          24
#define TMC524X_PWMCONF_PWM_LIM_MASK           GENMASK(31, 28)
#define TMC524X_PWMCONF_PWM_LIM_SHIFT          28

/* StallGuard4 */
#define TMC524X_SG4_THRS   0x74
#define TMC524X_SG4_RESULT 0x75
#define TMC524X_SG4_IND    0x76

#define TMC524X_SG4_THRS_SG4_THRS_MASK         GENMASK(7, 0)
#define TMC524X_SG4_THRS_SG4_THRS_SHIFT        0
#define TMC524X_SG4_THRS_SG4_FILT_EN_SHIFT     8
#define TMC524X_SG4_THRS_SG_ANGLE_OFFSET_SHIFT 9

#define TMC524X_SG4_RESULT_MASK  GENMASK(9, 0)
#define TMC524X_SG4_RESULT_SHIFT 0

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC524X_TMC524X_REG_H_ */
