/**
 * @file drivers/stepper/adi_tmc/tmc50xx/tmc50xx_reg.h
 *
 * @brief TMC50XX Registers
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC50XX_TMC50XX_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC50XX_TMC50XX_REG_H_

#include <adi_tmc_reg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name TMC50XX module registers
 * @anchor TMC50XX_REGISTERS
 *
 * @{
 */

#define TMC50XX_MOTOR_ADDR(m)     (0x20 << (m))
#define TMC50XX_MOTOR_ADDR_DRV(m) ((m) << 4)

#define TMC50XX_RAMPMODE(motor)   (0x00 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_XACTUAL(motor)    (0x01 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_VACTUAL(motor)    (0x02 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_VSTART(motor)     (0x03 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_A1(motor)         (0x04 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_V1(motor)         (0x05 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_AMAX(motor)       (0x06 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_VMAX(motor)       (0x07 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_DMAX(motor)       (0x08 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_D1(motor)         (0x0A | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_VSTOP(motor)      (0x0B | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_TZEROWAIT(motor)  (0x0C | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_XTARGET(motor)    (0x0D | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_SWMODE(motor)     (0x14 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_RAMPSTAT(motor)   (0x15 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_CHOPCONF(motor)   (0x6C | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_COOLCONF(motor)   (0x6D | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_DRVSTATUS(motor)  (0x6F | TMC50XX_MOTOR_ADDR_DRV(motor))

#define TMC50XX_MOTOR_ADDR_PWM(m) ((m) << 3)

#define TMC50XX_GCONF_POSCMP_ENABLE_SHIFT 3
#define TMC50XX_GCONF_TEST_MODE_SHIFT     7
#define TMC50XX_GCONF_SHAFT_SHIFT(n)      ((n) ? 8 : 9)
#define TMC50XX_LOCK_GCONF_SHIFT          10

#define TMC50XX_PWMCONF(motor)    (0x10 | TMC50XX_MOTOR_ADDR_PWM(motor))
#define TMC50XX_PWM_STATUS(motor) (0x11 | TMC50XX_MOTOR_ADDR_PWM(motor))

#define TMC50XX_IHOLD_IRUN(motor) (0x10 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_VCOOLTHRS(motor)  (0x11 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_VHIGH(motor)      (0x12 | TMC50XX_MOTOR_ADDR(motor))
#define TMC50XX_XLATCH(motor)     (0x16 | TMC50XX_MOTOR_ADDR(motor))

#define TMC50XX_MSLUT0(motor)     (0x60 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUT1(motor)     (0x61 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUT2(motor)     (0x62 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUT3(motor)     (0x63 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUT4(motor)     (0x64 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUT5(motor)     (0x65 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUT6(motor)     (0x66 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUT7(motor)     (0x67 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUTSEL(motor)   (0x68 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSLUTSTART(motor) (0x69 | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSCNT(motor)      (0x6A | TMC50XX_MOTOR_ADDR_DRV(motor))
#define TMC50XX_MSCURACT(motor)   (0x6B | TMC50XX_MOTOR_ADDR_DRV(motor))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC50XX_TMC50XX_REG_H_ */
