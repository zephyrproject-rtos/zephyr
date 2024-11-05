/**
 * @file drivers/stepper/adi/tmc_reg.h
 *
 * @brief TMC Registers
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_STEPPER_ADI_TMC5041

#define TMC5041_MOTOR_ADDR(m)     (0x20 << (m))
#define TMC5041_MOTOR_ADDR_DRV(m) ((m) << 4)
#define TMC5041_MOTOR_ADDR_PWM(m) ((m) << 3)

/**
 * @name TMC5041 module registers
 * @anchor TMC5041_REGISTERS
 *
 * @{
 */

#define TMC5041_WRITE_BIT    0x80U
#define TMC5041_ADDRESS_MASK 0x7FU

#define TMC5041_GCONF_POSCMP_ENABLE_SHIFT 3
#define TMC5041_GCONF_TEST_MODE_SHIFT     7
#define TMC5041_GCONF_SHAFT_SHIFT(n)      ((n) ? 8 : 9)
#define TMC5041_LOCK_GCONF_SHIFT          10

#define TMC5041_GCONF     0x00
#define TMC5041_GSTAT     0x01
#define TMC5041_INPUT     0x04
#define TMC5041_X_COMPARE 0x05

#define TMC5041_PWMCONF(motor)    (0x10 | TMC5041_MOTOR_ADDR_PWM(motor))
#define TMC5041_PWM_STATUS(motor) (0x11 | TMC5041_MOTOR_ADDR_PWM(motor))

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

#define TMC5041_MSLUT0(motor)     (0x60 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT1(motor)     (0x61 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT2(motor)     (0x62 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT3(motor)     (0x63 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT4(motor)     (0x64 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT5(motor)     (0x65 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT6(motor)     (0x66 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT7(motor)     (0x67 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUTSEL(motor)   (0x68 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUTSTART(motor) (0x69 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSCNT(motor)      (0x6A | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSCURACT(motor)   (0x6B | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_CHOPCONF(motor)   (0x6C | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_COOLCONF(motor)   (0x6D | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_DRVSTATUS(motor)  (0x6F | TMC5041_MOTOR_ADDR_DRV(motor))

#define TMC5041_RAMPMODE_POSITIONING_MODE       0
#define TMC5041_RAMPMODE_POSITIVE_VELOCITY_MODE 1
#define TMC5041_RAMPMODE_NEGATIVE_VELOCITY_MODE 2
#define TMC5041_RAMPMODE_HOLD_MODE              3

#define TMC5041_SW_MODE_SG_STOP_ENABLE BIT(10)

#define TMC5041_RAMPSTAT_INT_MASK  GENMASK(7, 4)
#define TMC5041_RAMPSTAT_INT_SHIFT 4

#define TMC5041_RAMPSTAT_POS_REACHED_EVENT_MASK BIT(7)
#define TMC5041_POS_REACHED_EVENT                                                                  \
	(TMC5041_RAMPSTAT_POS_REACHED_EVENT_MASK >> TMC5041_RAMPSTAT_INT_SHIFT)

#define TMC5041_RAMPSTAT_STOP_SG_EVENT_MASK BIT(6)
#define TMC5041_STOP_SG_EVENT	(TMC5041_RAMPSTAT_STOP_SG_EVENT_MASK >> TMC5041_RAMPSTAT_INT_SHIFT)

#define TMC5041_RAMPSTAT_STOP_RIGHT_EVENT_MASK BIT(5)
#define TMC5041_STOP_RIGHT_EVENT                                                                   \
	(TMC5041_RAMPSTAT_STOP_RIGHT_EVENT_MASK >> TMC5041_RAMPSTAT_INT_SHIFT)

#define TMC5041_RAMPSTAT_STOP_LEFT_EVENT_MASK BIT(4)
#define TMC5041_STOP_LEFT_EVENT                                                                    \
	(TMC5041_RAMPSTAT_STOP_LEFT_EVENT_MASK >> TMC5041_RAMPSTAT_INT_SHIFT)

#define TMC5041_DRV_STATUS_STST_BIT        BIT(31)
#define TMC5041_DRV_STATUS_SG_RESULT_MASK  GENMASK(9, 0)
#define TMC5041_DRV_STATUS_SG_STATUS_MASK  BIT(24)
#define TMC5041_DRV_STATUS_SG_STATUS_SHIFT 24

#define TMC5041_SG_MIN_VALUE -64
#define TMC5041_SG_MAX_VALUE 63

#define TMC5041_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT 16

#define TMC5041_IHOLD_MASK  GENMASK(4, 0)
#define TMC5041_IHOLD_SHIFT 0
#define TMC5041_IHOLD(n)    (((n) << TMC5041_IHOLD_SHIFT) & TMC5041_IHOLD_MASK)

#define TMC5041_IRUN_MASK  GENMASK(12, 8)
#define TMC5041_IRUN_SHIFT 8
#define TMC5041_IRUN(n)    (((n) << TMC5041_IRUN_SHIFT) & TMC5041_IRUN_MASK)

#define TMC5041_IHOLDDELAY_MASK  GENMASK(19, 16)
#define TMC5041_IHOLDDELAY_SHIFT 16
#define TMC5041_IHOLDDELAY(n)    (((n) << TMC5041_IHOLDDELAY_SHIFT) & TMC5041_IHOLDDELAY_MASK)

#define TMC5041_CHOPCONF_DRV_ENABLE_MASK GENMASK(3, 0)
#define TMC5041_CHOPCONF_MRES_MASK       GENMASK(27, 24)
#define TMC5041_CHOPCONF_MRES_SHIFT      24

#define TMC5041_CLOCK_FREQ_SHIFT 24

#endif

#ifdef CONFIG_STEPPER_ADI_TMC51XX

/**
 * @name TMC51XX module registers
 * @anchor TMC51XX_REGISTERS
 *
 * @{
 */

#define TMC51XX_GCONF_TEST_MODE_SHIFT 17
#define TMC51XX_GCONF_SHAFT_SHIFT 4

#define TMC51XX_DRV_STATUS_STST_BIT        BIT(31)

#define TMC51XX_WRITE_BIT       0x80U
#define TMC51XX_ADDRESS_MASK    0x7FU

#define TMC51XX_GCONF			0x00
#define TMC51XX_GSTAT			0x01
#define TMC51XX_IFCNT			0x02
#define TMC51XX_SLAVECONF      	0x03
#define TMC51XX_INP_OUT        	0x04
#define TMC51XX_X_COMPARE      	0x05
#define TMC51XX_OTP_PROG       	0x06
#define TMC51XX_OTP_READ       	0x07
#define TMC51XX_FACTORY_CONF   	0x08
#define TMC51XX_SHORT_CONF     	0x09
#define TMC51XX_DRV_CONF       	0x0A
#define TMC51XX_GLOBAL_SCALER  	0x0B
#define TMC51XX_OFFSET_READ    	0x0C
#define TMC51XX_IHOLD_IRUN     	0x10
#define TMC51XX_TPOWERDOWN     	0x11
#define TMC51XX_TSTEP          	0x12
#define TMC51XX_TPWMTHRS       	0x13
#define TMC51XX_TCOOLTHRS      	0x14
#define TMC51XX_THIGH          	0x15

#define TMC51XX_RAMPMODE       	0x20
#define TMC51XX_XACTUAL        	0x21
#define TMC51XX_VACTUAL        	0x22
#define TMC51XX_VSTART         	0x23
#define TMC51XX_A1             	0x24
#define TMC51XX_V1             	0x25
#define TMC51XX_AMAX           	0x26
#define TMC51XX_VMAX           	0x27
#define TMC51XX_DMAX           	0x28
#define TMC51XX_D1             	0x2A
#define TMC51XX_VSTOP          	0x2B
#define TMC51XX_TZEROWAIT      	0x2C
#define TMC51XX_XTARGET        	0x2D

#define TMC51XX_VDCMIN         	0x33
#define TMC51XX_SWMODE         	0x34
#define TMC51XX_RAMPSTAT       	0x35
#define TMC51XX_XLATCH         	0x36
#define TMC51XX_ENCMODE        	0x38
#define TMC51XX_XENC           	0x39
#define TMC51XX_ENC_CONST      	0x3A
#define TMC51XX_ENC_STATUS     	0x3B
#define TMC51XX_ENC_LATCH      	0x3C
#define TMC51XX_ENC_DEVIATION  	0x3D

#define TMC51XX_MSLUT0         	0x60
#define TMC51XX_MSLUT1         	0x61
#define TMC51XX_MSLUT2         	0x62
#define TMC51XX_MSLUT3         	0x63
#define TMC51XX_MSLUT4         	0x64
#define TMC51XX_MSLUT5         	0x65
#define TMC51XX_MSLUT6         	0x66
#define TMC51XX_MSLUT7         	0x67
#define TMC51XX_MSLUTSEL       	0x68
#define TMC51XX_MSLUTSTART     	0x69
#define TMC51XX_MSCNT          	0x6A
#define TMC51XX_MSCURACT       	0x6B
#define TMC51XX_CHOPCONF       	0x6C
#define TMC51XX_COOLCONF       	0x6D
#define TMC51XX_DCCTRL         	0x6E
#define TMC51XX_DRVSTATUS     	0x6F
#define TMC51XX_PWMCONF        	0x70
#define TMC51XX_PWMSCALE       	0x71
#define TMC51XX_PWM_AUTO       	0x72
#define TMC51XX_LOST_STEPS     	0x73


#define TMC51XX_RAMPMODE_POSITIONING_MODE       0
#define TMC51XX_RAMPMODE_POSITIVE_VELOCITY_MODE 1
#define TMC51XX_RAMPMODE_NEGATIVE_VELOCITY_MODE 2
#define TMC51XX_RAMPMODE_HOLD_MODE              3

#define TMC51XX_SW_MODE_SG_STOP_ENABLE BIT(10)

#define TMC51XX_RAMPSTAT_INT_MASK  GENMASK(7, 4)
#define TMC51XX_RAMPSTAT_INT_SHIFT 4

#define TMC51XX_RAMPSTAT_POS_REACHED_EVENT_MASK BIT(7)
#define TMC51XX_POS_REACHED_EVENT                                                                  \
	(TMC51XX_RAMPSTAT_POS_REACHED_EVENT_MASK >> TMC51XX_RAMPSTAT_INT_SHIFT)

#define TMC51XX_RAMPSTAT_STOP_SG_EVENT_MASK BIT(6)
#define TMC51XX_STOP_SG_EVENT	(TMC51XX_RAMPSTAT_STOP_SG_EVENT_MASK >> TMC51XX_RAMPSTAT_INT_SHIFT)

#define TMC51XX_RAMPSTAT_STOP_RIGHT_EVENT_MASK BIT(5)
#define TMC51XX_STOP_RIGHT_EVENT                                                                   \
	(TMC51XX_RAMPSTAT_STOP_RIGHT_EVENT_MASK >> TMC51XX_RAMPSTAT_INT_SHIFT)

#define TMC51XX_RAMPSTAT_STOP_LEFT_EVENT_MASK BIT(4)
#define TMC51XX_STOP_LEFT_EVENT                                                                    \
	(TMC51XX_RAMPSTAT_STOP_LEFT_EVENT_MASK >> TMC51XX_RAMPSTAT_INT_SHIFT)

#define TMC51XX_DRV_STATUS_STST_BIT        BIT(31)
#define TMC51XX_DRV_STATUS_SG_RESULT_MASK  GENMASK(9, 0)
#define TMC51XX_DRV_STATUS_SG_STATUS_MASK  BIT(24)
#define TMC51XX_DRV_STATUS_SG_STATUS_SHIFT 24

#define TMC51XX_SG_MIN_VALUE -64
#define TMC51XX_SG_MAX_VALUE 63

#define TMC51XX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT 16

#define TMC51XX_IHOLD_MASK  GENMASK(4, 0)
#define TMC51XX_IHOLD_SHIFT 0
#define TMC51XX_IHOLD(n)    (((n) << TMC51XX_IHOLD_SHIFT) & TMC51XX_IHOLD_MASK)

#define TMC51XX_IRUN_MASK  GENMASK(12, 8)
#define TMC51XX_IRUN_SHIFT 8
#define TMC51XX_IRUN(n)    (((n) << TMC51XX_IRUN_SHIFT) & TMC51XX_IRUN_MASK)

#define TMC51XX_IHOLDDELAY_MASK  GENMASK(19, 16)
#define TMC51XX_IHOLDDELAY_SHIFT 16
#define TMC51XX_IHOLDDELAY(n)    (((n) << TMC51XX_IHOLDDELAY_SHIFT) & TMC51XX_IHOLDDELAY_MASK)

#define TMC51XX_CHOPCONF_DRV_ENABLE_MASK GENMASK(3, 0)
#define TMC51XX_CHOPCONF_MRES_MASK       GENMASK(27, 24)
#define TMC51XX_CHOPCONF_MRES_SHIFT      24

#define TMC51XX_CLOCK_FREQ_SHIFT 24

#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_REG_H_ */
