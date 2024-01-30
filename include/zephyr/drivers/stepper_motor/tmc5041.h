/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TMC5041_H
#define ZEPHYR_DRIVERS_TMC5041_H

#ifdef __cplusplus
extern "C" {
#endif

#define TMC5041_MOTOR_ADDR(m)     (0x20 << m)
#define TMC5041_MOTOR_ADDR_DRV(m) (m << 4)
#define TMC5041_MOTOR_ADDR_PWM(m) (m << 3)

/**
 * @name TMC5041 module registers
 * @anchor TMC5041_REGISTERS
 *
 * @{
 */

#define TMC5041_GCONF          0x00
#define TMC5041_GSTAT          0x01
#define TMC5041_INPUT          0x04
#define TMC5041_X_COMPARE      0x05

#define TMC5041_PWMCONF(motor) (0x10 | TMC5041_MOTOR_ADDR_PWM(motor))
#define TMC5041_PWM_STATUS(motor)   (0x11 | TMC5041_MOTOR_ADDR_PWM(motor))

#define TMC5041_RAMPMODE(motor)   (0x00 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_XACTUAL(motor)    (0x01 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VACTUAL(motor)    (0x02 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VSTART(motor)     (0x03 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_A1(motor)         (0x04 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_V1(motor)         (0x05 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_AMAX(motor)       (0x06 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VMAX(motor)       (0x07 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_DMAX(motor)       (0x08 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_D1(motor)         (0x0A | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VSTOP(motor)      (0x0B | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_TZEROWAIT(motor)  (0x0C | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_XTARGET(motor)    (0x0D | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_IHOLD_IRUN(motor) (0x10 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VCOOLTHRS(motor)  (0x11 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VHIGH(motor)      (0x12 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_SWMODE(motor)     (0x14 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_RAMPSTAT(motor)   (0x15 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_XLATCH(motor)     (0x16 | TMC5041_MOTOR_ADDR(motor))

#define TMC5041_MSLUT0(motor)   (0x60 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT1(motor)   (0x61 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT2(motor)   (0x62 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT3(motor)   (0x63 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT4(motor)   (0x64 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT5(motor)   (0x65 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT6(motor)   (0x66 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT7(motor)   (0x67 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUTSEL(motor) (0x68 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUTSTART(motor)   (0x69 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSCNT(motor)     (0x6A | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSCURACT(motor)  (0x6B | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_CHOPCONF(motor)  (0x6C | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_COOLCONF(motor)  (0x6D | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_DRVSTATUS(motor) (0x6F | TMC5041_MOTOR_ADDR_DRV(motor))

#define TMC5041_RAMPMODE_POSITIONING_MODE       0
#define TMC5041_RAMPMODE_POSITIVE_VELOCITY_MODE 1
#define TMC5041_RAMPMODE_NEGATIVE_VELOCITY_MODE 2
#define TMC5041_RAMPMODE_HOLD_MODE              3

#define TMC5041_SW_MODE_SG_STOP_ENABLE  BIT(10)
#define TMC5041_SW_MODE_SG_STOP_DISABLE BIT(0)

#define TMC5041_DRV_STATUS_SG_RESULT_MASK  GENMASK(9, 0)
#define TMC5041_DRV_STATUS_SG_STATUS_MASK  BIT(24)
#define TMC5041_DRV_STATUS_SG_STATUS_SHIFT 24

#define TMC5041_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT 16

#define TMC5041_IHOLD_MASK  0x1F
#define TMC5041_IHOLD_SHIFT 0
#define TMC5041_IHOLD(n)    ((n << TMC5041_IHOLD_SHIFT) & TMC5041_IHOLD_MASK)

#define TMC5041_IRUN_MASK  0x1F00
#define TMC5041_IRUN_SHIFT 8
#define TMC5041_IRUN(n)    ((n << TMC5041_IRUN_SHIFT) & TMC5041_IRUN_MASK)

#define TMC5041_IHOLDDELAY_MASK  0x0F0000
#define TMC5041_IHOLDDELAY_SHIFT 16
#define TMC5041_IHOLDDELAY(n)    ((n << TMC5041_IHOLDDELAY_SHIFT) & TMC5041_IHOLDDELAY_MASK)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_TMC5041_H */
