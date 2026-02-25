/**
 * @file drivers/stepper/adi_tmc/tmcm3216/tmcm3216_reg.h
 *
 * @brief TMCM3216 Registers and Constants
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMCM3216_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMCM3216_REG_H_

/**
 * @brief TMCL Command Numbers (Section 4 of manual)
 */
#define TMCL_CMD_ROR  1  /* Rotate right */
#define TMCL_CMD_ROL  2  /* Rotate left */
#define TMCL_CMD_MST  3  /* Motor stop */
#define TMCL_CMD_MVP  4  /* Move to position */
#define TMCL_CMD_SAP  5  /* Set axis parameter */
#define TMCL_CMD_GAP  6  /* Get axis parameter */
#define TMCL_CMD_SGP  9  /* Set global parameter */
#define TMCL_CMD_GGP  10 /* Get global parameter */
#define TMCL_CMD_STGP 11 /* Store global parameter */
#define TMCL_CMD_RSGP 12 /* Restore global parameter */
#define TMCL_CMD_RFS  13 /* Reference search */
#define TMCL_CMD_SIO  14 /* Set input/output */
#define TMCL_CMD_GIO  15 /* Get input/output */

/**
 * @brief TMCL Status Codes
 */
#define TMCL_STATUS_OK             100
#define TMCL_STATUS_LOADED         101
#define TMCL_STATUS_WRONG_CHECKSUM 1
#define TMCL_STATUS_INVALID_CMD    2
#define TMCL_STATUS_WRONG_TYPE     3
#define TMCL_STATUS_INVALID_VALUE  4
#define TMCL_STATUS_EEPROM_LOCKED  5
#define TMCL_STATUS_CMD_NOT_AVAIL  6

/**
 * @brief MVP (Move to Position) Type Numbers
 */
#define TMCL_MVP_ABS   0 /* Absolute position */
#define TMCL_MVP_REL   1 /* Relative position */
#define TMCL_MVP_COORD 2 /* Coordinate position */

/**
 * @brief Axis Parameters (from manual Table 4.1)
 */
#define TMCL_AP_TARGET_POSITION       0
#define TMCL_AP_ACTUAL_POSITION       1
#define TMCL_AP_TARGET_VELOCITY       2
#define TMCL_AP_ACTUAL_VELOCITY       3
#define TMCL_AP_MAX_VELOCITY          4
#define TMCL_AP_MAX_ACCELERATION      5
#define TMCL_AP_MAX_CURRENT           6
#define TMCL_AP_STANDBY_CURRENT       7
#define TMCL_AP_POSITION_REACHED_FLAG 8  /* 0 = moving, 1 = position reached */
#define TMCL_AP_RIGHT_ENDSTOP         10 /* Right limit switch status */
#define TMCL_AP_LEFT_ENDSTOP          11 /* Left limit switch status */
#define TMCL_AP_MICROSTEP_RESOLUTION  140

/**
 * @brief Microstep Resolution Values
 */
#define TMCL_MRES_FULLSTEP 0
#define TMCL_MRES_HALFSTEP 1
#define TMCL_MRES_4        2
#define TMCL_MRES_8        3
#define TMCL_MRES_16       4
#define TMCL_MRES_32       5
#define TMCL_MRES_64       6
#define TMCL_MRES_128      7
#define TMCL_MRES_256      8

/**
 * @brief Reference Search Mode Values (for RFS command)
 */
#define TMCL_RFS_MODE_1  1  /* Left stop switch */
#define TMCL_RFS_MODE_2  2  /* Right then left stop switch */
#define TMCL_RFS_MODE_3  3  /* Right then left stop switch (both sides) */
#define TMCL_RFS_MODE_4  4  /* Left stop switch (both sides) */
#define TMCL_RFS_MODE_5  5  /* Home switch negative, reverse at left stop */
#define TMCL_RFS_MODE_6  6  /* Home switch positive, reverse at right stop */
#define TMCL_RFS_MODE_7  7  /* Home switch negative, ignore stops */
#define TMCL_RFS_MODE_8  8  /* Home switch positive, ignore stops */
#define TMCL_RFS_MODE_9  9  /* Encoder null channel positive */
#define TMCL_RFS_MODE_10 10 /* Encoder null channel negative */

/* TMCM-3216 data width (32-bit signed values) */
#define TMCM3216_DATA_BITS 31

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMCM3216_REG_H_ */
