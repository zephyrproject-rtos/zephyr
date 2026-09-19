/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_TMC522X_H_
#define ZEPHYR_DRIVERS_STEPPER_TMC522X_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name TMC522X SPI communication constants */
/** @{ */
#define TMC522X_WRITE_BIT    0x80U
#define TMC522X_ADDRESS_MASK 0x7FU
/** @} */

/** @name TMC522X configuration registers */
/** @{ */
#define TMC522X_REG_GCONF        0x00
#define TMC522X_REG_GSTAT        0x01
#define TMC522X_REG_NODECONF     0x03
#define TMC522X_REG_IOIN         0x04
#define TMC522X_REG_DRVCONF      0x05
#define TMC522X_REG_GLOBALSCALER 0x06
#define TMC522X_REG_DIAG_CONF    0x07
/** @} */

/** @name TMC522X GCONF register fields */
/** @{ */
/** 0: SpreadCycle, 1: StealthChop2 */
#define TMC522X_GCONF_EN_PWM_MODE BIT(0)
#define TMC522X_GCONF_SD          BIT(7)
#define TMC522X_GCONF_DIRECT_MODE BIT(6)
#define TMC522X_GCONF_STOP_ENABLE BIT(4)
/** 1: Driver disable, 0: Driver enable */
#define TMC522X_GCONF_DRV_ENN     BIT(9)
/** @} */

/** @name TMC522X DRVCONF register fields */
/** @{ */
#define TMC522X_DRVCONF_FS_GAIN_MASK  GENMASK(3, 2)
#define TMC522X_DRVCONF_FS_SENSE_MASK GENMASK(1, 0)
/** @} */

/** @name TMC522X GLOBALSCALER register fields */
/** @{ */
#define TMC522X_GLOBALSCALER_A_MASK GENMASK(7, 0)
#define TMC522X_GLOBALSCALER_B_MASK GENMASK(15, 8)
/** @} */

/**
 * @name TMC522X DIAG_CONF register fields (0x07)
 *
 * Bits [31:28]: Output mode/polarity
 * Bits [27:14]: DIAG1 event routing
 * Bits [13:0]:  DIAG0 event routing
 */
/** @{ */
#define TMC522X_DIAG1_INVPP_MASK          BIT(31)
#define TMC522X_DIAG1_NOD_PP_MASK         BIT(30)
#define TMC522X_DIAG0_INVPP_MASK          BIT(29)
#define TMC522X_DIAG0_NOD_PP_MASK         BIT(28)
#define TMC522X_DIAG1_POS_REACHED_MASK    BIT(27)
#define TMC522X_DIAG1_EV_HOMING_MASK      BIT(26)
#define TMC522X_DIAG1_EV_DEVIATION_MASK   BIT(25)
#define TMC522X_DIAG1_EV_POS_REACHED_MASK BIT(24)
#define TMC522X_DIAG1_EV_STOP_SG_MASK     BIT(23)
#define TMC522X_DIAG1_EV_STOP_MASK        BIT(22)
#define TMC522X_DIAG1_XCOMP_MASK          BIT(21)
#define TMC522X_DIAG1_DIR_MASK            BIT(20)
#define TMC522X_DIAG1_STEP_MASK           BIT(19)
#define TMC522X_DIAG1_INDEX_MASK          BIT(18)
#define TMC522X_DIAG1_STALL_MASK          BIT(17)
#define TMC522X_DIAG1_STALL_PREWARN_MASK  BIT(16)
#define TMC522X_DIAG1_OTPW_MASK           BIT(15)
#define TMC522X_DIAG1_ERROR_MASK          BIT(14)
#define TMC522X_DIAG0_POS_REACHED_MASK    BIT(13)
#define TMC522X_DIAG0_EV_HOMING_MASK      BIT(12)
#define TMC522X_DIAG0_EV_DEVIATION_MASK   BIT(11)
#define TMC522X_DIAG0_EV_POS_REACHED_MASK BIT(10)
#define TMC522X_DIAG0_EV_STOP_SG_MASK     BIT(9)
#define TMC522X_DIAG0_EV_STOP_REF_MASK    BIT(8)
#define TMC522X_DIAG0_XCOMP_MASK          BIT(7)
#define TMC522X_DIAG0_DIR_MASK            BIT(6)
#define TMC522X_DIAG0_STEP_MASK           BIT(5)
#define TMC522X_DIAG0_INDEX_MASK          BIT(4)
#define TMC522X_DIAG0_STALL_MASK          BIT(3)
#define TMC522X_DIAG0_STALL_PREWARN_MASK  BIT(2)
#define TMC522X_DIAG0_OTPW_MASK           BIT(1)
#define TMC522X_DIAG0_ERROR_MASK          BIT(0)
/** @} */

/** @name TMC522X IOIN register fields */
/** @{ */
#define TMC522X_IOIN_EXT_CLK_MASK GENMASK(24, 23)
/** @} */

/** @name TMC522X clock frequency constants */
/** @{ */
#define TMC522X_INTERNAL_CLOCK_HZ  125000000
#define TMC522X_QUIESCENT_CLOCK_HZ 609000
/** @} */

/** @name TMC522X microstep look-up table registers */
/** @{ */
#define TMC522X_REG_MSLUT(x)    (0x08 + (x))
#define TMC522X_REG_MSLUT_START 0x10
#define TMC522X_REG_MSLUT_SEL   0x11
/** @} */

/** @name TMC522X position compare registers */
/** @{ */
#define TMC522X_REG_X_COMPARE       0x12
#define TMC522X_REG_X_COMPAR_REPEAT 0x13
/** @} */

/** @name TMC522X velocity-dependent configuration registers */
/** @{ */
#define TMC522X_REG_IHOLD_IRUN 0x14
#define TMC522X_REG_TPOWERDOWN 0x15
#define TMC522X_REG_TSTEP      0x16
#define TMC522X_REG_TPWMTHRS   0x17
#define TMC522X_REG_TCOOLTHRS  0x18
#define TMC522X_REG_THIGH      0x19
/** @} */

/** @name TMC522X velocity threshold helper values */
/** @{ */
#define TMC522X_STALLGUARD_ENABLED_ALL_SPEEDS 0xFFFFF
#define TMC522X_STALLGUARD_DISABLED           0
#define TMC522X_STEALTHCHOP_DISABLED          0xFFFFF
#define TMC522X_COOLSTEP_DISABLED             0xFFFFF
#define TMC522X_HIGHSPEED_DISABLED            0
#define TMC522X_VELOCITY_THRESH_MAX           0xFFFFF
/** @} */

/** @name TMC522X IHOLD_IRUN register fields */
/** @{ */
#define TMC522X_IHOLD_MASK      GENMASK(4, 0)
#define TMC522X_IRUN_MASK       GENMASK(12, 8)
#define TMC522X_IHOLDDELAY_MASK GENMASK(19, 16)
#define TMC522X_IRUNDELAY_MASK  GENMASK(27, 24)
/** @} */

/** @name TMC522X ramp generator registers */
/** @{ */
#define TMC522X_REG_RAMPMODE  0x1A
#define TMC522X_REG_XACTUAL   0x1B
#define TMC522X_REG_VACTUAL   0x1C
#define TMC522X_REG_AACTUAL   0x1D
#define TMC522X_REG_VSTART    0x1E
#define TMC522X_REG_A1        0x1F
#define TMC522X_REG_V1        0x20
#define TMC522X_REG_A2        0x21
#define TMC522X_REG_V2        0x22
#define TMC522X_REG_AMAX      0x23
#define TMC522X_REG_VMAX      0x24
#define TMC522X_REG_DMAX      0x25
#define TMC522X_REG_D2        0x26
#define TMC522X_REG_D1        0x27
#define TMC522X_REG_VSTOP     0x28
#define TMC522X_REG_TZEROWAIT 0x29
#define TMC522X_REG_TVMAX     0x2A
#define TMC522X_REG_XTARGET   0x2B
/** @} */

/** @name TMC522X ramp generator driver feature control registers */
/** @{ */
#define TMC522X_REG_VDCMIN    0x2C
#define TMC522X_REG_SW_MODE   0x2D
#define TMC522X_REG_RAMP_STAT 0x2E
#define TMC522X_REG_XLATCH    0x2F
/** @} */

/** @name TMC522X SW_MODE register fields (0x2D) */
/** @{ */
#define TMC522X_SW_MODE_STOP_L_ENABLE     BIT(0)
#define TMC522X_SW_MODE_STOP_R_ENABLE     BIT(1)
#define TMC522X_SW_MODE_POL_STOP_L        BIT(2)
#define TMC522X_SW_MODE_POL_STOP_R        BIT(3)
#define TMC522X_SW_MODE_SWAP_LR           BIT(4)
#define TMC522X_SW_MODE_LATCH_L_ACTIVE    BIT(5)
#define TMC522X_SW_MODE_LATCH_L_INACTIVE  BIT(6)
#define TMC522X_SW_MODE_LATCH_R_ACTIVE    BIT(7)
#define TMC522X_SW_MODE_LATCH_R_INACTIVE  BIT(8)
#define TMC522X_SW_MODE_SG_STOP_ENABLE    BIT(10)
#define TMC522X_SW_MODE_EN_SOFTSTOP       BIT(11)
#define TMC522X_SW_MODE_EN_VIRTUAL_STOP_L BIT(12)
#define TMC522X_SW_MODE_EN_VIRTUAL_STOP_R BIT(13)
#define TMC522X_SW_MODE_EN_REFL_HOMING    BIT(14)
#define TMC522X_SW_MODE_EN_REFR_HOMING    BIT(15)
#define TMC522X_SW_MODE_EN_AUTO_FEATURE   BIT(16)
/** @} */

/** @name TMC522X RAMP_STAT register fields */
/** @{ */
#define TMC522X_RAMP_STAT_STATUS_STOP_L    BIT(0)
#define TMC522X_RAMP_STAT_STATUS_STOP_R    BIT(1)
#define TMC522X_RAMP_STAT_STATUS_LATCH_L   BIT(2)
#define TMC522X_RAMP_STAT_STATUS_LATCH_R   BIT(3)
#define TMC522X_RAMP_STAT_EVENT_STOP_L     BIT(4)
#define TMC522X_RAMP_STAT_EVENT_STOP_R     BIT(5)
#define TMC522X_EVENT_STOP_SG_MASK         BIT(6)
#define TMC522X_EVENT_POS_REACH_MASK       BIT(7)
#define TMC522X_RAMP_STAT_VELOCITY_REACHED BIT(8)
#define TMC522X_RAMP_STAT_POSITION_REACHED BIT(9)
#define TMC522X_RAMP_STAT_VZERO            BIT(10)
#define TMC522X_RAMP_STAT_T_ZEROWAIT       BIT(11)
#define TMC522X_RAMP_STAT_SECOND_MOVE      BIT(12)
#define TMC522X_RAMP_STAT_STATUS_SG        BIT(13)
#define TMC522X_RAMP_STAT_EVENT_MASK       GENMASK(7, 4)
/** @} */

/** @name TMC522X BEMF registers */
/** @{ */
#define TMC522X_REG_X_BEMF         0x30
#define TMC522X_REG_BEMF_CONF      0x31
#define TMC522X_REG_BEMF_DEVIATION 0x32
#define TMC522X_REG_VIRTUAL_STOPL  0x33
#define TMC522X_REG_VIRTUAL_STOPR  0x34
#define TMC522X_REG_X_BEMF_LATCH   0x35
/** @} */

/** @name TMC522X motor driver registers */
/** @{ */
#define TMC522X_REG_MSCNT           0x36
#define TMC522X_REG_MSCURACT        0x37
#define TMC522X_REG_CHOPCONF        0x38
#define TMC522X_REG_COOLCONF        0x39
#define TMC522X_REG_DCCTRL          0x3A
#define TMC522X_REG_DRV_STATUS      0x3B
#define TMC522X_REG_PWMCONF         0x3C
#define TMC522X_REG_PWM_SCALE       0x3D
#define TMC522X_REG_PWM_AUTO        0x3E
#define TMC522X_REG_SG4_THRS        0x3F
#define TMC522X_REG_SG4_RESULT      0x40
#define TMC522X_REG_SG4_IND         0x41
#define TMC522X_REG_SG_PREWARN_CONF 0x42
#define TMC522X_REG_OTP_MODE        0x7D
/** @} */

/** @name TMC522X GSTAT register fields */
/** @{ */
#define TMC522X_GSTAT_RESET   BIT(0)
#define TMC522X_GSTAT_DRV_ERR BIT(1)
/** @} */

/** @name TMC522X CHOPCONF register fields */
/** @{ */
#define TMC522X_CHOPCONF_TOFF_MASK         GENMASK(3, 0)
#define TMC522X_CHOPCONF_HSTRT_TFD210_MASK GENMASK(6, 4)
#define TMC522X_CHOPCONF_HEND_OFFSET_MASK  GENMASK(10, 7)
#define TMC522X_CHOPCONF_CHM_MASK          BIT(14)
#define TMC522X_CHOPCONF_TBL_MASK          GENMASK(16, 15)
#define TMC522X_CHOPCONF_VHIGHFS_MASK      BIT(18)
#define TMC522X_CHOPCONF_VHIGHCHM_MASK     BIT(19)
#define TMC522X_CHOPCONF_MRES_MASK         GENMASK(27, 24)
#define TMC522X_CHOPCONF_MRES_SHIFT        24
#define TMC522X_CHOPCONF_DRV_EN_SW_MASK    BIT(9)
/** @} */

/** @name TMC522X COOLCONF register fields */
/** @{ */
#define TMC522X_COOLCONF_SEMIN_MASK   GENMASK(3, 0)
#define TMC522X_COOLCONF_SEMAX_MASK   GENMASK(7, 4)
#define TMC522X_COOLCONF_SEUP_MASK    GENMASK(9, 8)
#define TMC522X_COOLCONF_SG_STOP_MASK BIT(10)
#define TMC522X_COOLCONF_SEDN_MASK    GENMASK(14, 13)
#define TMC522X_COOLCONF_SEIMIN_MASK  BIT(15)
#define TMC522X_COOLCONF_SGT_MASK     GENMASK(22, 16)
#define TMC522X_COOLCONF_SGT_SHIFT    16
#define TMC522X_SGT_MIN               (-64)
#define TMC522X_SGT_MAX               63
#define TMC522X_SGT_FIELD_BITS        7
#define TMC522X_SGT_FIELD_MASK        0x7F
#define TMC522X_SGT_SIGN_BIT          0x40
#define TMC522X_SGT_SIGN_EXTEND       0x80
#define TMC522X_COOLCONF_SFILT_MASK   BIT(24)
/** @} */

/** @name TMC522X StallGuard4 register fields */
/** @{ */
#define TMC522X_SG4_THRS_MASK   GENMASK(8, 0)
#define TMC522X_SG4_THRS_MAX    510
#define TMC522X_SG4_RESULT_MASK GENMASK(8, 0)
#define TMC522X_SG4_RESULT_MAX  510
#define TMC522X_SG4_IND_0_MASK  GENMASK(7, 0)
#define TMC522X_SG4_IND_1_MASK  GENMASK(15, 8)
#define TMC522X_SG4_IND_2_MASK  GENMASK(23, 16)
#define TMC522X_SG4_IND_3_MASK  GENMASK(31, 24)
/** @} */

/** @name TMC522X PWMCONF register fields */
/** @{ */
#define TMC522X_PWMCONF_PWM_OFS_MASK            GENMASK(7, 0)
#define TMC522X_PWMCONF_PWM_GRAD_MASK           GENMASK(15, 8)
#define TMC522X_PWMCONF_FREQ_MASK               GENMASK(17, 16)
#define TMC522X_PWMCONF_AUTOSCALE_MASK          BIT(18)
#define TMC522X_PWMCONF_AUTOGRAD_MASK           BIT(19)
#define TMC522X_PWMCONF_FREEWHEEL_MASK          GENMASK(21, 20)
#define TMC522X_PWMCONF_PWM_MEAS_SD_ENABLE_MASK BIT(22)
#define TMC522X_PWMCONF_PWM_DIS_REG_STST_MASK   BIT(23)
#define TMC522X_PWMCONF_SG4_FILT_EN_MASK        BIT(24)
/** @} */

/** @name TMC522X PWM_SCALE register fields */
/** @{ */
#define TMC522X_PWM_SCALE_SUM_MASK GENMASK(9, 0)
/** @} */

/** @name TMC522X DRV_STATUS register fields */
/** @{ */
#define TMC522X_DRV_STATUS_SG_RESULT_MASK     GENMASK(9, 0)
#define TMC522X_SG_RESULT_MAX                 1023
#define TMC522X_DRV_STATUS_S2VSA              BIT(12)
#define TMC522X_DRV_STATUS_S2VSB              BIT(13)
#define TMC522X_DRV_STATUS_STEALTH            BIT(14)
#define TMC522X_DRV_STATUS_FSACTIVE           BIT(15)
#define TMC522X_DRV_STATUS_CS_ACTUAL_MASK     GENMASK(20, 16)
#define TMC522X_DRV_STATUS_RES_DET            BIT(22)
#define TMC522X_DRV_STATUS_STALLGUARD_PREWARN BIT(23)
#define TMC522X_DRV_STATUS_STALLGUARD         BIT(24)
#define TMC522X_DRV_STATUS_OT                 BIT(25)
#define TMC522X_DRV_STATUS_OTPW               BIT(26)
#define TMC522X_DRV_STATUS_S2GA               BIT(27)
#define TMC522X_DRV_STATUS_S2GB               BIT(28)
#define TMC522X_DRV_STATUS_OLA                BIT(29)
#define TMC522X_DRV_STATUS_OLB                BIT(30)
#define TMC522X_DRV_STATUS_STST               BIT(31)
#define TMC522X_DRV_STATUS_STEALTHCHOP        TMC522X_DRV_STATUS_STEALTH
/** @} */

/** @name TMC522X field extraction macros */
/** @{ */
#define TMC522X_GET_SG_RESULT(v)     ((v) & TMC522X_DRV_STATUS_SG_RESULT_MASK)
#define TMC522X_GET_CS_ACTUAL(v)     (((v) & TMC522X_DRV_STATUS_CS_ACTUAL_MASK) >> 16)
#define TMC522X_GET_PWM_SCALE_SUM(v) ((v) & TMC522X_PWM_SCALE_SUM_MASK)
#define TMC522X_GET_SG4_RESULT(v)    ((v) & TMC522X_SG4_RESULT_MASK)
#define TMC522X_GET_SG4_IND_0(v)     ((v) & TMC522X_SG4_IND_0_MASK)
#define TMC522X_GET_SG4_IND_1(v)     (((v) & TMC522X_SG4_IND_1_MASK) >> 8)
#define TMC522X_GET_SG4_IND_2(v)     (((v) & TMC522X_SG4_IND_2_MASK) >> 16)
#define TMC522X_GET_SG4_IND_3(v)     (((v) & TMC522X_SG4_IND_3_MASK) >> 24)
/** @} */

/** @name TMC522X microstep resolution values */
/** @{ */
#define TMC522X_MRES_256 0
#define TMC522X_MRES_128 1
#define TMC522X_MRES_64  2
#define TMC522X_MRES_32  3
#define TMC522X_MRES_16  4
#define TMC522X_MRES_8   5
#define TMC522X_MRES_4   6
#define TMC522X_MRES_2   7
#define TMC522X_MRES_1   8
/** @} */

/** @brief TMC522X ramp mode */
enum tmc522x_rampmode {
	/** Positioning mode (move to XTARGET) */
	TMC522X_RAMPMODE_POSITION = 0,
	/** Velocity mode, positive direction */
	TMC522X_RAMPMODE_POSITIVE_VELOCITY,
	/** Velocity mode, negative direction */
	TMC522X_RAMPMODE_NEGATIVE_VELOCITY,
	/** Hold mode (velocity = 0) */
	TMC522X_RAMPMODE_HOLD,
};

/** @brief TMC522X PWM freewheel mode */
enum tmc522x_pwm_freewheel {
	/** Normal operation */
	TMC522X_FREEWHEEL_NORMAL = 0,
	/** Freewheel */
	TMC522X_FREEWHEEL_FREEWHEEL,
	/** Coil short via low-side drivers */
	TMC522X_FREEWHEEL_COILSHORT_LS,
	/** Coil short via high-side drivers */
	TMC522X_FREEWHEEL_COILSHORT_HS,
};

/** @brief TMC522X PWM frequency */
enum tmc522x_pwm_freq {
	/** f_PWM = 2/1024 f_CLK */
	TMC522X_PWM_FREQ_2_1024 = 0,
	/** f_PWM = 2/683 f_CLK */
	TMC522X_PWM_FREQ_2_683,
	/** f_PWM = 2/512 f_CLK */
	TMC522X_PWM_FREQ_2_512,
	/** f_PWM = 2/410 f_CLK */
	TMC522X_PWM_FREQ_2_410,
};

/**
 * @brief Bus union to support both SPI and I2C.
 */
union tmc522x_bus {
#if CONFIG_TMC522X_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if CONFIG_TMC522X_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

/** @brief Callback to check bus readiness. */
typedef int (*tmc522x_bus_check_fn)(const union tmc522x_bus *bus);
/** @brief Callback to initialize the bus. */
typedef int (*tmc522x_bus_init_fn)(const union tmc522x_bus *bus);
/** @brief Callback to read a register over the bus. */
typedef int (*tmc522x_reg_read_fn)(const union tmc522x_bus *bus, uint8_t reg_addr,
				   uint32_t *reg_val);
/** @brief Callback to write a register over the bus. */
typedef int (*tmc522x_reg_write_fn)(const union tmc522x_bus *bus, uint8_t reg_addr,
				    uint32_t reg_val);

/**
 * @brief Bus I/O operations.
 */
struct tmc522x_bus_io {
	/** @brief Check bus readiness. */
	tmc522x_bus_check_fn check;
	/** @brief Initialize the bus. */
	tmc522x_bus_init_fn init;
	/** @brief Read a register. */
	tmc522x_reg_read_fn read;
	/** @brief Write a register. */
	tmc522x_reg_write_fn write;
};

/**
 * @brief CoolStep configuration for SpreadCycle mode (StallGuard2).
 */
struct tmc522x_coolstep_config {
	/** @brief Minimum CoolStep current threshold (COOLCONF bits 3:0, range 0-15). */
	uint8_t semin;
	/** @brief CoolStep hysteresis width (COOLCONF bits 7:4, range 0-15). */
	uint8_t semax;
	/** @brief Current increment step size (COOLCONF bits 9:8, range 0-3). */
	uint8_t seup;
	/** @brief Current decrement speed (COOLCONF bits 14:13, range 0-3). */
	uint8_t sedn;
	/** @brief Minimum current limit (COOLCONF bit 15). */
	bool seimin;
	/** @brief StallGuard2 threshold (COOLCONF bits 22:16, range -64 to 63). */
	int8_t sgt;
	/** @brief Enable StallGuard2 result filtering (COOLCONF bit 24). */
	bool sfilt;
	/** @brief CoolStep/StallGuard velocity threshold (TCOOLTHRS register, 20-bit). */
	uint32_t tcoolthrs;
};

/**
 * @brief CoolStep configuration for StealthChop2 mode (StallGuard4).
 */
struct tmc522x_coolstep_sg4_config {
	/** @brief Minimum CoolStep current threshold (COOLCONF bits 3:0, range 0-15). */
	uint8_t semin;
	/** @brief CoolStep hysteresis width (COOLCONF bits 7:4, range 0-15). */
	uint8_t semax;
	/** @brief Current increment step size (COOLCONF bits 9:8, range 0-3). */
	uint8_t seup;
	/** @brief Current decrement speed (COOLCONF bits 14:13, range 0-3). */
	uint8_t sedn;
	/** @brief Minimum current limit (COOLCONF bit 15). */
	bool seimin;
	/** @brief StallGuard4 threshold (SG4_THRS register, bits 8:0, range 0-511). */
	uint16_t sg4_thrs;
	/** @brief CoolStep/StallGuard velocity threshold (TCOOLTHRS register, 20-bit). */
	uint32_t tcoolthrs;
};

/**
 * @brief Write a value to a TMC522X register.
 *
 * @param dev Pointer to the TMC522X device.
 * @param reg_addr Register address to write to.
 * @param reg_val Value to write to the register.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_write(const struct device *dev, uint8_t reg_addr, uint32_t reg_val);

/**
 * @brief Read a value from a TMC522X register.
 *
 * @param dev Pointer to the TMC522X device.
 * @param reg_addr Register address to read from.
 * @param reg_val Pointer to store the read value.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_read(const struct device *dev, uint8_t reg_addr, uint32_t *reg_val);

/**
 * @brief Trigger the registered callback for stepper events.
 *
 * @param dev Pointer to the TMC522X stepper device.
 * @param event The stepper event that occurred.
 */
void tmc522x_stepper_trigger_cb(const struct device *dev, const enum stepper_ctrl_event event);

/**
 * @brief Get the stepper index for the given device.
 *
 * @param dev Pointer to the TMC522X stepper device.
 *
 * @return Index of the stepper.
 */
int tmc522x_stepper_index(const struct device *dev);

/**
 * @brief Reschedule the RAMP_STAT polling work.
 *
 * @param dev Pointer to the TMC522X device.
 */
void tmc522x_reschedule_rampstat_work(const struct device *dev);

/**
 * @brief Burst read multiple consecutive registers.
 *
 * @param dev Pointer to the TMC522X device.
 * @param start_addr Starting register address.
 * @param buf Buffer to store register values.
 * @param num_regs Number of registers to read.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf, size_t num_regs);

/**
 * @brief Set the chopper mode (SpreadCycle or StealthChop2).
 *
 * @param dev Pointer to the TMC522X device.
 * @param stealthchop2 true for StealthChop2, false for SpreadCycle.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_stepper_set_chopper_mode(const struct device *dev, bool stealthchop2);

/**
 * @brief Set the chopper hysteresis mode (CHM bit in CHOPCONF).
 *
 * @param dev Pointer to the TMC522X device.
 * @param classic_mode true for classic constant off-time, false for SpreadCycle.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_set_chm_mode(const struct device *dev, bool classic_mode);

/**
 * @brief Enable the driver outputs (clear DRV_ENN in GCONF).
 *
 * @param dev Pointer to the TMC522X device.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_drven_enable(const struct device *dev);

/**
 * @brief Disable the driver outputs (set DRV_ENN in GCONF).
 *
 * @param dev Pointer to the TMC522X device.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_drven_disable(const struct device *dev);

/**
 * @brief Get the actual velocity in microsteps per second.
 *
 * @param dev Pointer to the TMC522X device.
 * @param velocity Pointer to store the velocity value.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_get_actual_velocity(const struct device *dev, int32_t *velocity);

/**
 * @brief Get the actual acceleration.
 *
 * @param dev Pointer to the TMC522X device.
 * @param accel Pointer to store the acceleration value.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_get_actual_acceleration(const struct device *dev, int32_t *accel);

/**
 * @brief Set the StealthChop velocity threshold (TPWMTHRS register).
 *
 * @param dev Pointer to the TMC522X device.
 * @param velocity_pps Velocity threshold in pulses per second.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_set_stealthchop_threshold(const struct device *dev, uint32_t velocity_pps);

/**
 * @brief Set the CoolStep velocity threshold (TCOOLTHRS register).
 *
 * @param dev Pointer to the TMC522X device.
 * @param velocity_pps Velocity threshold in pulses per second.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_set_coolstep_threshold(const struct device *dev, uint32_t velocity_pps);

/**
 * @brief Set the full-step velocity threshold (THIGH register).
 *
 * @param dev Pointer to the TMC522X device.
 * @param velocity_pps Velocity threshold in pulses per second.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_set_fullstep_threshold(const struct device *dev, uint32_t velocity_pps);

/**
 * @brief Enable or disable the StallGuard stop event (SG_STOP in SW_MODE).
 *
 * @param dev Pointer to the TMC522X device.
 * @param enable true to enable, false to disable.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_enable_stallguard_event(const struct device *dev, bool enable);

/**
 * @brief Enable or disable StallGuard detection.
 *
 * @param dev Pointer to the TMC522X device.
 * @param enable true to enable, false to disable.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_enable_stallguard(const struct device *dev, bool enable);

/**
 * @brief Read and log the current StallGuard status registers.
 *
 * @param dev Pointer to the TMC522X device.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_check_stallguard_status(const struct device *dev);

/**
 * @brief Set the StallGuard2 threshold and filter (COOLCONF register).
 *
 * @param dev Pointer to the TMC522X device.
 * @param sgt Signed threshold value (-64 to 63).
 * @param sfilt true to enable result filtering.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_set_stallguard2_threshold(const struct device *dev, int8_t sgt, bool sfilt);

/**
 * @brief Set the StallGuard4 threshold and filter (SG4_THRS register).
 *
 * @param dev Pointer to the TMC522X device.
 * @param sg4_thrs Threshold value (0-511).
 * @param sg4_filt_en true to enable SG4 filtering.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_set_stallguard4_threshold(const struct device *dev, uint16_t sg4_thrs,
				      bool sg4_filt_en);

/**
 * @brief Enable CoolStep in SpreadCycle mode with StallGuard2.
 *
 * @param dev Pointer to the TMC522X device.
 * @param cfg Pointer to the CoolStep configuration.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_enable_coolstep_spreadcycle(const struct device *dev,
					const struct tmc522x_coolstep_config *cfg);

/**
 * @brief Enable CoolStep in StealthChop2 mode with StallGuard4.
 *
 * @param dev Pointer to the TMC522X device.
 * @param cfg Pointer to the CoolStep SG4 configuration.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_enable_coolstep_stealthchop2(const struct device *dev,
					 const struct tmc522x_coolstep_sg4_config *cfg);

/**
 * @brief Clear a stall condition and re-enable the motor.
 *
 * @param dev Pointer to the TMC522X device.
 *
 * @retval 0 on success.
 * @retval -EIO on failure.
 */
int tmc522x_clear_stall(const struct device *dev);

/** @brief SPI bus I/O operations. */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
extern const struct tmc522x_bus_io tmc522x_bus_io_spi;
#endif

/** @brief I2C bus I/O operations. */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
extern const struct tmc522x_bus_io tmc522x_bus_io_i2c;
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_TMC522X_H_ */
