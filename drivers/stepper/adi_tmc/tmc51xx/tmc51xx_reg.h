/**
 * @file drivers/stepper/adi_tmc/tmc51xx/tmc51xx_reg.h
 *
 * @brief TMC51XX Registers
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC51XX_TMC51XX_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC51XX_TMC51XX_REG_H_

#include <adi_tmc_reg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name TMC51XX module registers
 * @anchor TMC51XX_REGISTERS
 *
 * @{
 */

#define TMC51XX_GCONF_EN_PWM_MODE_SHIFT        2
#define TMC51XX_GCONF_SHAFT_SHIFT              4
#define TMC51XX_GCONF_DIAG0_INT_PUSHPULL_SHIFT 12
#define TMC51XX_GCONF_TEST_MODE_SHIFT          17

#define TMC51XX_IHOLD_IRUN	0x10
#define TMC51XX_TPOWER_DOWN	0x11
#define TMC51XX_TSTEP		0x12
#define TMC51XX_TPWMTHRS	0x13
#define TMC51XX_TCOOLTHRS	0x14
#define TMC51XX_THIGH		0x15

#define TMC51XX_RAMPMODE	0x20
#define TMC51XX_XACTUAL		0x21
#define TMC51XX_VACTUAL		0x22
#define TMC51XX_VSTART		0x23
#define TMC51XX_A1		0x24
#define TMC51XX_V1		0x25
#define TMC51XX_AMAX		0x26
#define TMC51XX_VMAX		0x27
#define TMC51XX_DMAX		0x28
#define TMC51XX_D1		0x2A
#define TMC51XX_VSTOP		0x2B
#define TMC51XX_TZEROWAIT	0x2C
#define TMC51XX_XTARGET		0x2D
#define TMC51XX_SWMODE		0x34
#define TMC51XX_RAMPSTAT	0x35
#define TMC51XX_CHOPCONF	0x6C
#define TMC51XX_COOLCONF	0x6D
#define TMC51XX_DRVSTATUS	0x6F

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC51XX_TMC51XX_REG_H_ */
