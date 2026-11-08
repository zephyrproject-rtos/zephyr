/**
 * @file drivers/stepper/adi_tmc/tmc5241/tmc5241_reg.h
 *
 * @brief TMC5241 / TMC2241 Registers
 *
 */

/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5241_TMC5241_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5241_TMC5241_REG_H_

#include <zephyr/sys/util.h>
#include <adi_tmc_reg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name TMC5241/TMC2241 module registers
 * @anchor TMC5241_REGISTERS
 *
 * @{
 */

/* ---- General registers (both chips) ----
 * GCONF/GSTAT addresses come from adi_tmc_reg.h: TMC5XXX_GCONF, TMC5XXX_GSTAT.
 */
#define TMC5241_IFCNT     0x02
#define TMC5241_NODECONF  0x03
#define TMC5241_IOIN      0x04
#define TMC5241_DIAG_CONF 0x07

/* ---- Velocity-dependent driver feature control (both chips) ---- */
#define TMC5241_IHOLD_IRUN 0x10
#define TMC5241_TPOWERDOWN 0x11
#define TMC5241_TSTEP      0x12
#define TMC5241_TPWMTHRS   0x13
#define TMC5241_TCOOLTHRS  0x14
#define TMC5241_THIGH      0x15

/* ---- Ramp generator registers: TMC5241 ONLY, absent on TMC2241 ---- */
#define TMC5241_RAMPMODE  0x20
#define TMC5241_XACTUAL   0x21
#define TMC5241_VACTUAL   0x22
#define TMC5241_VSTART    0x23
#define TMC5241_A1        0x24
#define TMC5241_V1        0x25
#define TMC5241_AMAX      0x26
#define TMC5241_VMAX      0x27
#define TMC5241_DMAX      0x28
#define TMC5241_TVMAX     0x29
#define TMC5241_D1        0x2A
#define TMC5241_VSTOP     0x2B
#define TMC5241_TZEROWAIT 0x2C
#define TMC5241_XTARGET   0x2D
#define TMC5241_V2        0x2E
#define TMC5241_A2        0x2F
#define TMC5241_D2        0x30
#define TMC5241_SWMODE    0x34
#define TMC5241_RAMPSTAT  0x35

/* RAMP_STAT latched event bits: event_stop_l/r, event_stop_sg, event_pos_reached */
#define TMC5241_RAMPSTAT_EVENT_MASK GENMASK(7, 4)
#define TMC5241_XLATCH    0x36

/* ---- Motor driver registers (both chips) ---- */
#define TMC5241_MSLUT_BASE 0x60 /* 0x60-0x67, default sine wave table */
#define TMC5241_MSLUTSEL   0x68
#define TMC5241_MSLUTSTART 0x69
#define TMC5241_MSCNT      0x6A
#define TMC5241_MSCURACT   0x6B
#define TMC5241_CHOPCONF   0x6C
#define TMC5241_COOLCONF   0x6D
#define TMC5241_DCCTRL     0x6E
#define TMC5241_DRVSTATUS  0x6F
#define TMC5241_PWMCONF    0x70
#define TMC5241_PWMSCALE   0x71
#define TMC5241_PWM_AUTO   0x72

/* ---- StallGuard4: both chips ---- */
#define TMC5241_SG4_THRS   0x74
#define TMC5241_SG4_RESULT 0x75
#define TMC5241_SG4_IND    0x76

/* ---- SG4_THRS bit masks ---- */
#define TMC5241_SG4_THRS_MASK                 GENMASK(7, 0)
#define TMC5241_SG4_THRS_SG_ANGLE_OFFSET_MASK BIT(9) /* reset value 1 */

/* ---- GCONF bit masks ---- */
#define TMC5241_GCONF_LENGTH_STEP_PULSE_MASK  GENMASK(20, 17) /* TMC5241 only */
#define TMC5241_GCONF_DIAG1_PUSHPULL_MASK     BIT(13)
#define TMC5241_GCONF_DIAG0_PUSHPULL_MASK     BIT(12)
#define TMC5241_GCONF_DIAG1_NPOSCOMP_DIR_MASK BIT(8) /* TMC5241 only, new field */
/* TMC2241 only: bits 7/6/5 have a different meaning on TMC5241 (diag0_nint_step,
 * reserved, reserved respectively). Routes DIAG0 in addition to DIAG_CONF below.
 */
#define TMC2241_GCONF_DIAG0_STALL_MASK        BIT(7)
#define TMC2241_GCONF_DIAG0_OTPW_MASK         BIT(6)
#define TMC2241_GCONF_DIAG0_ERROR_MASK        BIT(5)
#define TMC5241_GCONF_SHAFT_MASK              BIT(4)
#define TMC2241_GCONF_MULTISTEP_FILT_MASK     BIT(3) /* reserved on TMC5241 */
#define TMC5241_GCONF_EN_PWM_MODE_MASK        BIT(2)
#define TMC5241_GCONF_FAST_STANDSTILL_MASK    BIT(1)

/* ---- DIAG_CONF bit masks (DIAG0 routing only; DIAG1 unused by this driver) ----
 * diag0_ev_* bits are TMC5241 only (map RAMPSTAT events to DIAG0); TMC2241 has
 * no RAMPSTAT. diag0_stall/otpw/error exist at the same bit positions on both.
 */
#define TMC5241_DIAG_CONF_DIAG0_EV_POS_REACHED_MASK BIT(9) /* TMC5241 only */
#define TMC5241_DIAG_CONF_DIAG0_EV_STOP_SG_MASK     BIT(8) /* TMC5241 only */
#define TMC5241_DIAG_CONF_DIAG0_EV_STOP_REF_MASK    BIT(7) /* TMC5241 only */
#define TMC5241_DIAG_CONF_DIAG0_STALL_MASK          BIT(2)
#define TMC5241_DIAG_CONF_DIAG0_OTPW_MASK           BIT(1)
#define TMC5241_DIAG_CONF_DIAG0_ERROR_MASK          BIT(0)

/* ---- CHOPCONF bit masks ----
 * TOFF/MRES come from adi_tmc_reg.h: TMC5XXX_CHOPCONF_DRV_ENABLE_MASK,
 * TMC5XXX_CHOPCONF_MRES_MASK, TMC5XXX_CHOPCONF_MRES_SHIFT.
 */
#define TMC5241_CHOPCONF_INTPOL_MASK BIT(28)

/* ---- COOLCONF bit masks (StallGuard2) ---- */
#define TMC5241_COOLCONF_SFILT_MASK BIT(24)
#define TMC5241_COOLCONF_SGT_MASK   GENMASK(22, 16)

/* ---- DRVSTATUS bit masks ----
 * STST/SG_RESULT/SG_STATUS come from adi_tmc_reg.h: TMC5XXX_DRV_STATUS_STST_BIT,
 * TMC5XXX_DRV_STATUS_SG_RESULT_MASK, TMC5XXX_DRV_STATUS_SG_STATUS_MASK.
 */
#define TMC5241_DRVSTATUS_S2GB_MASK BIT(28)
#define TMC5241_DRVSTATUS_S2GA_MASK BIT(27)
#define TMC5241_DRVSTATUS_OTPW_MASK BIT(26)
#define TMC5241_DRVSTATUS_OT_MASK   BIT(25)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5241_TMC5241_REG_H_ */
