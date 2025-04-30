/**
 * @file drivers/stepper/adi_tmc/tmc2130_reg.h
 *
 * @brief TMC2130 Registers
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Navimatix GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_2130_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_2130_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name TMC2130 module registers
 * @anchor TMC2130_REGISTERS
 *
 * @{
 */

#define TMC2130_WRITE_BIT                  0x80
#define TMC2130_ADDRESS_MASK               0x7FU
#define TMC2130_GCONF_STEALTH_CHOP_SHIFT   2
#define TMC2130_IHOLDDELAY_SHIFT           16
#define TMC2130_IRUN_SHIFT                 8
#define TMC2130_IRUN_MASK                  GENMASK(12, 8)
#define TMC2130_IHOLD_MASK                 GENMASK(4, 0)
#define TMC2130_CHOPCONF_MRES_MASK         GENMASK(27, 24)
#define TMC2130_CHOPCONF_MRES_SHIFT        24
#define TMC2130_CHOPCONF_DOUBLE_EDGE_SHIFT 29

#define TMC2130_TPWMTHRS_MAX_VALUE   ((1 << 20) - 1)
#define TMC2130_TPOWERDOWN_MAX_VALUE ((1 << 8) - 1)
#define TMC2130_IRUN_MAX_VALUE       ((1 << 5) - 1)
#define TMC2130_IHOLD_MAX_VALUE      ((1 << 5) - 1)
#define TMC2130_IHOLDDELAY_MAX_VALUE ((1 << 4) - 1)

#define TMC2130_GCONF      0x00
#define TMC2130_GSTAT      0x01
#define TMC2130_IHOLD_IRUN 0x10
#define TMC2130_TPOWERDOWN 0x11
#define TMC2130_TSTEP      0x12
#define TMC2130_TPWMTHRS   0x13
#define TMC2130_CHOPCONF   0x6c
#define TMC2130_PWMCONF    0x70

/* Initial register values */
#define TMC2130_GCONF_INIT(stealth_chop)                                                           \
	(0x00000000 + (stealth_chop << TMC2130_GCONF_STEALTH_CHOP_SHIFT))
#define TMC2130_IHOLD_IRUN_INIT(iholddelay, irun, ihold)                                           \
	(0x00000000 + (iholddelay << TMC2130_IHOLDDELAY_SHIFT) +                                   \
	 ((irun << TMC2130_IRUN_SHIFT) & TMC2130_IRUN_MASK) + (ihold & TMC2130_IHOLD_MASK))
#define TMC2130_TPOWERDOWN_INIT(tpowerdown) (0x00000000 + tpowerdown)
#define TMC2130_TPWMTHRS_INIT(tpwmthrs)     (0x00000000 + tpwmthrs)
#define TMC2130_CHOPCONF_INIT(ustep_res, double_edge)                                              \
	(0x00000002 + (double_edge << TMC2130_CHOPCONF_DOUBLE_EDGE_SHIFT) +                        \
	 ((ustep_res << TMC2130_CHOPCONF_MRES_SHIFT) & TMC2130_CHOPCONF_MRES_MASK)) /* TOFF=3 */
#define TMC2130_PWMCONF_INIT 0x00040000 /* pwm_autoscale=1 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_2130_REG_H_ */
