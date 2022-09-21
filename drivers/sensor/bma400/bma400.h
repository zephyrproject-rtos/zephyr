/**
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_

#include <device.h>
#include <sys/util.h>
#include <zephyr/types.h>
#include <drivers/gpio.h>


/* ODR configurations  */
#define BMA400_ODR_12_5HZ                         (0x05)
#define BMA400_ODR_25HZ                           (0x06)
#define BMA400_ODR_50HZ                           (0x07)
#define BMA400_ODR_100HZ                          (0x08)
#define BMA400_ODR_200HZ                          (0x09)
#define BMA400_ODR_400HZ                          (0x0A)
#define BMA400_ODR_800HZ                          (0x0B)

#if defined(CONFIG_BMA400_ODR_12_5)
#define BMA400_ODR BMA400_ODR_12_5HZ
#elif defined(CONFIG_BMA400_ODR_25)
#define BMA400_ODR BMA400_ODR_25HZ
#elif defined(CONFIG_BMA400_ODR_50)
#define BMA400_ODR BMA400_ODR_50HZ
#elif defined(CONFIG_BMA400_ODR_100)
#define BMA400_ODR BMA400_ODR_100HZ
#elif defined(CONFIG_BMA400_ODR_200)
#define BMA400_ODR BMA400_ODR_200HZ
#elif defined(CONFIG_BMA400_ODR_400)
#define BMA400_ODR BMA400_ODR_400HZ
#elif defined(CONFIG_BMA400_ODR_800)
#define BMA400_ODR BMA400_ODR_800HZ
#endif

/* Accel Range configuration */
#define BMA400_RANGE_2G                           (0x00)
#define BMA400_RANGE_4G                           (0x01)
#define BMA400_RANGE_8G                           (0x02)
#define BMA400_RANGE_16G                          (0x03)

#if defined(CONFIG_BMA400_RANGE_2G)
#define BMA400_RANGE BMA400_RANGE_2G
#elif defined(CONFIG_BMA400_RANGE_4G)
#define BMA400_RANGE BMA400_RANGE_4G
#elif defined(CONFIG_BMA400_RANGE_8G)
#define BMA400_RANGE BMA400_RANGE_8G
#elif defined(CONFIG_BMA400_RANGE_16G)
#define BMA400_RANGE BMA400_RANGE_16G
#endif

/* CHIP ID VALUE */
#define BMA400_CHIP_ID                            (0x90)

/* BMA400 I2C address macros */
#define BMA400_I2C_ADDRESS_SDO_LOW                (0x14)
#define BMA400_I2C_ADDRESS_SDO_HIGH               (0x15)

/* Power mode configurations */
#define BMA400_MODE_NORMAL                        (0x02)
#define BMA400_MODE_SLEEP                         (0x00)
#define BMA400_MODE_LOW_POWER                     (0x01)

/* Enable / Disable macros */
#define BMA400_DISABLE                            (0)
#define BMA400_ENABLE                             (1)

/* Data/sensortime selection macros */
#define BMA400_DATA_ONLY                          (0x00)
#define BMA400_DATA_SENSOR_TIME                   (0x01)

/* ODR configurations  */
#define BMA400_ODR_12_5HZ                         (0x05)
#define BMA400_ODR_25HZ                           (0x06)
#define BMA400_ODR_50HZ                           (0x07)
#define BMA400_ODR_100HZ                          (0x08)
#define BMA400_ODR_200HZ                          (0x09)
#define BMA400_ODR_400HZ                          (0x0A)
#define BMA400_ODR_800HZ                          (0x0B)

/* Accel Range configuration */
#define BMA400_RANGE_2G                           (0x00)
#define BMA400_RANGE_4G                           (0x01)
#define BMA400_RANGE_8G                           (0x02)
#define BMA400_RANGE_16G                          (0x03)

/* Accel Axes selection settings for
 * DATA SAMPLING, WAKEUP, ORIENTATION CHANGE,
 * GEN1, GEN2 , ACTIVITY CHANGE
 */
#define BMA400_AXIS_X_EN                          (0x01)
#define BMA400_AXIS_Y_EN                          (0x02)
#define BMA400_AXIS_Z_EN                          (0x04)
#define BMA400_AXIS_XYZ_EN                        (0x07)

/* Accel filter(data_src_reg) selection settings */
#define BMA400_DATA_SRC_ACCEL_FILT_1              (0x00)
#define BMA400_DATA_SRC_ACCEL_FILT_2              (0x01)
#define BMA400_DATA_SRC_ACCEL_FILT_LP             (0x02)

/* Accel OSR (OSR,OSR_LP) settings */
#define BMA400_ACCEL_OSR_SETTING_0                (0x00)
#define BMA400_ACCEL_OSR_SETTING_1                (0x01)
#define BMA400_ACCEL_OSR_SETTING_2                (0x02)
#define BMA400_ACCEL_OSR_SETTING_3                (0x03)

/* Accel filt1_bw settings */
/* Accel filt1_bw = 0.48 * ODR */
#define BMA400_ACCEL_FILT1_BW_0                   (0x00)

/* Accel filt1_bw = 0.24 * ODR */
#define BMA400_ACCEL_FILT1_BW_1                   (0x01)

/* Auto wake-up timeout value of 10.24s */
#define BMA400_TIMEOUT_MAX_AUTO_WAKEUP            (0x0FFF)

/* Auto low power timeout value of 10.24s */
#define BMA400_TIMEOUT_MAX_AUTO_LP                (0x0FFF)

/* Reference Update macros */
#define BMA400_UPDATE_MANUAL                      (0x00)
#define BMA400_UPDATE_ONE_TIME                    (0x01)
#define BMA400_UPDATE_EVERY_TIME                  (0x02)
#define BMA400_UPDATE_LP_EVERY_TIME               (0x03)

/* Reference Update macros for orient interrupts */
#define BMA400_ORIENT_REFU_ACC_FILT_2             (0x01)
#define BMA400_ORIENT_REFU_ACC_FILT_LP            (0x02)

/* Number of samples needed for Auto-wakeup interrupt evaluation  */
#define BMA400_SAMPLE_COUNT_1                     (0x00)
#define BMA400_SAMPLE_COUNT_2                     (0x01)
#define BMA400_SAMPLE_COUNT_3                     (0x02)
#define BMA400_SAMPLE_COUNT_4                     (0x03)
#define BMA400_SAMPLE_COUNT_5                     (0x04)
#define BMA400_SAMPLE_COUNT_6                     (0x05)
#define BMA400_SAMPLE_COUNT_7                     (0x06)
#define BMA400_SAMPLE_COUNT_8                     (0x07)

/* Auto low power configurations */
/* Auto low power timeout disabled  */
#define BMA400_AUTO_LP_TIMEOUT_DISABLE            (0x00)

/* Auto low power entered on drdy interrupt */
#define BMA400_AUTO_LP_DRDY_TRIGGER               (0x01)

/* Auto low power entered on GEN1 interrupt */
#define BMA400_AUTO_LP_GEN1_TRIGGER               (0x02)

/* Auto low power entered on timeout of threshold value */
#define BMA400_AUTO_LP_TIMEOUT_EN                 (0x04)

/* Auto low power entered on timeout of threshold value
 * but reset on activity detection
 */
#define BMA400_AUTO_LP_TIME_RESET_EN              (0x08)

/*    TAP INTERRUPT CONFIG MACROS   */
/* Axes select for TAP interrupt */
#define BMA400_TAP_X_AXIS_EN                      (0x02)
#define BMA400_TAP_Y_AXIS_EN                      (0x01)
#define BMA400_TAP_Z_AXIS_EN                      (0x00)

/* TAP tics_th setting */

/* Maximum time between upper and lower peak of a tap, in data samples
 * this time depends on the mechanics of the device tapped onto
 * default = 12 samples
 */

/* Configures 6 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_6_DATA_SAMPLES             (0x00)

/* Configures 9 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_9_DATA_SAMPLES             (0x01)

/* Configures 12 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_12_DATA_SAMPLES            (0x02)

/* Configures 18 data samples for high-low tap signal change time */
#define BMA400_TICS_TH_18_DATA_SAMPLES            (0x03)

/* TAP Sensitivity setting */
/* It modifies the threshold for minimum TAP amplitude */
/* BMA400_TAP_SENSITIVITY_0 correspond to highest sensitivity */
#define BMA400_TAP_SENSITIVITY_0                  (0x00)
#define BMA400_TAP_SENSITIVITY_1                  (0x01)
#define BMA400_TAP_SENSITIVITY_2                  (0x02)
#define BMA400_TAP_SENSITIVITY_3                  (0x03)
#define BMA400_TAP_SENSITIVITY_4                  (0x04)
#define BMA400_TAP_SENSITIVITY_5                  (0x05)
#define BMA400_TAP_SENSITIVITY_6                  (0x06)

/* BMA400_TAP_SENSITIVITY_7 correspond to lowest sensitivity */
#define BMA400_TAP_SENSITIVITY_7                  (0x07)

/*  BMA400 TAP - quiet  settings */

/* Quiet refers to minimum quiet time before and after double tap,
 * in the data samples This time also defines the longest time interval
 * between two taps so that they are considered as double tap
 */

/* Configures 60 data samples quiet time between single or double taps */
#define BMA400_QUIET_60_DATA_SAMPLES              (0x00)

/* Configures 80 data samples quiet time between single or double taps */
#define BMA400_QUIET_80_DATA_SAMPLES              (0x01)

/* Configures 100 data samples quiet time between single or double taps */
#define BMA400_QUIET_100_DATA_SAMPLES             (0x02)

/* Configures 120 data samples quiet time between single or double taps */
#define BMA400_QUIET_120_DATA_SAMPLES             (0x03)

/*  BMA400 TAP - quiet_dt  settings */

/* quiet_dt refers to Minimum time between the two taps of a
 * double tap, in data samples
 */

/* Configures 4 data samples minimum time between double taps */
#define BMA400_QUIET_DT_4_DATA_SAMPLES            (0x00)

/* Configures 8 data samples minimum time between double taps */
#define BMA400_QUIET_DT_8_DATA_SAMPLES            (0x01)

/* Configures 12 data samples minimum time between double taps */
#define BMA400_QUIET_DT_12_DATA_SAMPLES           (0x02)

/* Configures 16 data samples minimum time between double taps */
#define BMA400_QUIET_DT_16_DATA_SAMPLES           (0x03)

/*    ACTIVITY CHANGE CONFIG MACROS   */
/* Data source for activity change detection */
#define BMA400_DATA_SRC_ACC_FILT1                 (0x00)
#define BMA400_DATA_SRC_ACC_FILT2                 (0x01)

/* Number of samples to evaluate for activity change detection */
#define BMA400_ACT_CH_SAMPLE_CNT_32               (0x00)
#define BMA400_ACT_CH_SAMPLE_CNT_64               (0x01)
#define BMA400_ACT_CH_SAMPLE_CNT_128              (0x02)
#define BMA400_ACT_CH_SAMPLE_CNT_256              (0x03)
#define BMA400_ACT_CH_SAMPLE_CNT_512              (0x04)

/* Interrupt pin configuration macros */
#define BMA400_INT_PUSH_PULL_ACTIVE_0             (0x00)
#define BMA400_INT_PUSH_PULL_ACTIVE_1             (0x01)
#define BMA400_INT_OPEN_DRIVE_ACTIVE_0            (0x02)
#define BMA400_INT_OPEN_DRIVE_ACTIVE_1            (0x03)

/* Interrupt Assertion status macros */
#define BMA400_ASSERTED_WAKEUP_INT                (0x0001)
#define BMA400_ASSERTED_ORIENT_CH                 (0x0002)
#define BMA400_ASSERTED_GEN1_INT                  (0x0004)
#define BMA400_ASSERTED_GEN2_INT                  (0x0008)
#define BMA400_ASSERTED_INT_OVERRUN               (0x0010)
#define BMA400_ASSERTED_FIFO_FULL_INT             (0x0020)
#define BMA400_ASSERTED_FIFO_WM_INT               (0x0040)
#define BMA400_ASSERTED_DRDY_INT                  (0x0080)
#define BMA400_ASSERTED_STEP_INT                  (0x0300)
#define BMA400_ASSERTED_S_TAP_INT                 (0x0400)
#define BMA400_ASSERTED_D_TAP_INT                 (0x0800)
#define BMA400_ASSERTED_ACT_CH_X                  (0x2000)
#define BMA400_ASSERTED_ACT_CH_Y                  (0x4000)
#define BMA400_ASSERTED_ACT_CH_Z                  (0x8000)

/* Generic interrupt criterion_sel configuration macros */
#define BMA400_ACTIVITY_INT                       (0x01)
#define BMA400_INACTIVITY_INT                     (0x00)

/* Generic interrupt axes evaluation logic configuration macros */
#define BMA400_ALL_AXES_INT                       (0x01)
#define BMA400_ANY_AXES_INT                       (0x00)

/* Generic interrupt hysteresis configuration macros */
#define BMA400_HYST_0_MG                          (0x00)
#define BMA400_HYST_24_MG                         (0x01)
#define BMA400_HYST_48_MG                         (0x02)
#define BMA400_HYST_96_MG                         (0x03)

/* BMA400 Register Address */
#define BMA400_REG_CHIP_ID                        (0x00)
#define BMA400_REG_STATUS                         (0x03)
#define BMA400_REG_ACCEL_DATA                     (0x04)
#define BMA400_REG_INT_STAT0                      (0x0E)
#define BMA400_REG_TEMP_DATA                      (0x11)
#define BMA400_REG_FIFO_LENGTH                    (0x12)
#define BMA400_REG_FIFO_DATA                      (0x14)
#define BMA400_REG_STEP_CNT_0                     (0x15)
#define BMA400_REG_ACCEL_CONFIG_0                 (0x19)
#define BMA400_REG_ACCEL_CONFIG_1                 (0x1A)
#define BMA400_REG_ACCEL_CONFIG_2                 (0x1B)
#define BMA400_REG_INT_CONF_0                     (0x1F)
#define BMA400_REG_INT_12_IO_CTRL                 (0x24)
#define BMA400_REG_INT_CONFIG1                    (0x20)
#define BMA400_REG_INT1_MAP                       (0x21)
#define BMA400_REG_FIFO_CONFIG_0                  (0x26)
#define BMA400_REG_FIFO_READ_EN                   (0x29)
#define BMA400_REG_AUTO_LOW_POW_0                 (0x2A)
#define BMA400_REG_AUTO_LOW_POW_1                 (0x2B)
#define BMA400_REG_AUTOWAKEUP_0                   (0x2C)
#define BMA400_REG_AUTOWAKEUP_1                   (0x2D)
#define BMA400_REG_WAKEUP_INT_CONF_0              (0x2F)
#define BMA400_REG_ORIENTCH_INT_CONFIG            (0x35)
#define BMA400_REG_GEN1INT_CONFIG0                (0x3F)
#define BMA400_REG_GEN1INT_CONFIG1                (0x40)
#define BMA400_REG_GEN1INT_CONFIG2                (0x41)
#define BMA400_REG_GEN2INT_CONFIG0                (0x4A)
#define BMA400_REG_ACT_CH_CONFIG_0                (0x55)
#define BMA400_REG_TAP_CONFIG                     (0x57)
#define BMA400_REG_SELF_TEST                      (0x7D)
#define BMA400_REG_COMMAND                        (0x7E)

/* BMA400 Command register */
#define BMA400_SOFT_RESET_CMD                     (0xB6)
#define BMA400_FIFO_FLUSH_CMD                     (0xB0)

/* BMA400 Delay definitions */
#define BMA400_DELAY_US_SOFT_RESET                (5000)
#define BMA400_DELAY_US_SELF_TEST                 (7000)
#define BMA400_DELAY_US_SELF_TEST_DATA_READ       (50000)

/* Interface selection macro */
#define BMA400_SPI_WR_MASK                        (0x7F)
#define BMA400_SPI_RD_MASK                        (0x80)

/* UTILITY MACROS */
#define BMA400_SET_LOW_BYTE                       (0x00FF)
#define BMA400_SET_HIGH_BYTE                      (0xFF00)

/* Interrupt mapping selection */
#define BMA400_DATA_READY_INT_MAP                 (0x01)
#define BMA400_FIFO_WM_INT_MAP                    (0x02)
#define BMA400_FIFO_FULL_INT_MAP                  (0x03)
#define BMA400_GEN2_INT_MAP                       (0x04)
#define BMA400_GEN1_INT_MAP                       (0x05)
#define BMA400_ORIENT_CH_INT_MAP                  (0x06)
#define BMA400_WAKEUP_INT_MAP                     (0x07)
#define BMA400_ACT_CH_INT_MAP                     (0x08)
#define BMA400_TAP_INT_MAP                        (0x09)
#define BMA400_STEP_INT_MAP                       (0x0A)
#define BMA400_INT_OVERRUN_MAP                    (0x0B)

/* BMA400 FIFO configurations */
#define BMA400_FIFO_AUTO_FLUSH                    (0x01)
#define BMA400_FIFO_STOP_ON_FULL                  (0x02)
#define BMA400_FIFO_TIME_EN                       (0x04)
#define BMA400_FIFO_DATA_SRC                      (0x08)
#define BMA400_FIFO_8_BIT_EN                      (0x10)
#define BMA400_FIFO_X_EN                          (0x20)
#define BMA400_FIFO_Y_EN                          (0x40)
#define BMA400_FIFO_Z_EN                          (0x80)

/* BMA400 FIFO data configurations */
#define BMA400_FIFO_EN_X                          (0x01)
#define BMA400_FIFO_EN_Y                          (0x02)
#define BMA400_FIFO_EN_Z                          (0x04)
#define BMA400_FIFO_EN_XY                         (0x03)
#define BMA400_FIFO_EN_YZ                         (0x06)
#define BMA400_FIFO_EN_XZ                         (0x05)
#define BMA400_FIFO_EN_XYZ                        (0x07)

/* BMA400 Self test configurations */
#define BMA400_SELF_TEST_DISABLE                  (0x00)
#define BMA400_SELF_TEST_ENABLE_POSITIVE          (0x07)
#define BMA400_SELF_TEST_ENABLE_NEGATIVE          (0x0F)

/* BMA400 FIFO data masks */
#define BMA400_FIFO_HEADER_MASK                   (0x3E)
#define BMA400_FIFO_BYTES_OVERREAD                (25)
#define BMA400_AWIDTH_MASK                        (0xEF)
#define BMA400_FIFO_DATA_EN_MASK                  (0x0E)

/* BMA400 Step status field - Activity status */
#define BMA400_STILL_ACT                          (0x00)
#define BMA400_WALK_ACT                           (0x01)
#define BMA400_RUN_ACT                            (0x02)

/* It is inserted when FIFO_CONFIG0.fifo_data_src
 * is changed during the FIFO read
 */
#define BMA400_FIFO_CONF0_CHANGE                  (0x01)

/* It is inserted when ACC_CONFIG0.filt1_bw
 * is changed during the FIFO read
 */
#define BMA400_ACCEL_CONF0_CHANGE                 (0x02)

/* It is inserted when ACC_CONFIG1.acc_range
 * acc_odr or osr is changed during the FIFO read
 */
#define BMA400_ACCEL_CONF1_CHANGE                 (0x04)

/* Accel width setting either 12/8 bit mode */
#define BMA400_12_BIT_FIFO_DATA                   (0x01)
#define BMA400_8_BIT_FIFO_DATA                    (0x00)

/* BMA400 FIFO header configurations */
#define BMA400_FIFO_SENSOR_TIME                   (0xA0)
#define BMA400_FIFO_EMPTY_FRAME                   (0x80)
#define BMA400_FIFO_CONTROL_FRAME                 (0x48)
#define BMA400_FIFO_XYZ_ENABLE                    (0x8E)
#define BMA400_FIFO_X_ENABLE                      (0x82)
#define BMA400_FIFO_Y_ENABLE                      (0x84)
#define BMA400_FIFO_Z_ENABLE                      (0x88)
#define BMA400_FIFO_XY_ENABLE                     (0x86)
#define BMA400_FIFO_YZ_ENABLE                     (0x8C)
#define BMA400_FIFO_XZ_ENABLE                     (0x8A)

/* BMA400 bit mask definitions */
#define BMA400_POWER_MODE_STATUS_MSK              (0x06)
#define BMA400_POWER_MODE_STATUS_POS              (1)

#define BMA400_POWER_MODE_MSK                     (0x03)

#define BMA400_ACCEL_ODR_MSK                      (0x0F)

#define BMA400_ACCEL_RANGE_MSK                    (0xC0)
#define BMA400_ACCEL_RANGE_POS                    (6)

#define BMA400_DATA_FILTER_MSK                    (0x0C)
#define BMA400_DATA_FILTER_POS                    (2)

#define BMA400_OSR_MSK                            (0x30)
#define BMA400_OSR_POS                            (4)

#define BMA400_OSR_LP_MSK                         (0x60)
#define BMA400_OSR_LP_POS                         (5)

#define BMA400_FILT_1_BW_MSK                      (0x80)
#define BMA400_FILT_1_BW_POS                      (7)

#define BMA400_WAKEUP_TIMEOUT_MSK                 (0x04)
#define BMA400_WAKEUP_TIMEOUT_POS                 (2)

#define BMA400_WAKEUP_THRES_LSB_MSK               (0x000F)

#define BMA400_WAKEUP_THRES_MSB_MSK               (0x0FF0)
#define BMA400_WAKEUP_THRES_MSB_POS               (4)

#define BMA400_WAKEUP_TIMEOUT_THRES_MSK           (0xF0)
#define BMA400_WAKEUP_TIMEOUT_THRES_POS           (4)

#define BMA400_WAKEUP_INTERRUPT_MSK               (0x02)
#define BMA400_WAKEUP_INTERRUPT_POS               (1)

#define BMA400_AUTO_LOW_POW_MSK                   (0x0F)

#define BMA400_AUTO_LP_THRES_MSK                  (0x0FF0)
#define BMA400_AUTO_LP_THRES_POS                  (4)

#define BMA400_AUTO_LP_THRES_LSB_MSK              (0x000F)

#define BMA400_WKUP_REF_UPDATE_MSK                (0x03)

#define BMA400_AUTO_LP_TIMEOUT_LSB_MSK            (0xF0)
#define BMA400_AUTO_LP_TIMEOUT_LSB_POS            (4)

#define BMA400_SAMPLE_COUNT_MSK                   (0x1C)
#define BMA400_SAMPLE_COUNT_POS                   (2)

#define BMA400_WAKEUP_EN_AXES_MSK                 (0xE0)
#define BMA400_WAKEUP_EN_AXES_POS                 (5)

#define BMA400_TAP_AXES_EN_MSK                    (0x18)
#define BMA400_TAP_AXES_EN_POS                    (3)

#define BMA400_TAP_QUIET_DT_MSK                   (0x30)
#define BMA400_TAP_QUIET_DT_POS                   (4)

#define BMA400_TAP_QUIET_MSK                      (0x0C)
#define BMA400_TAP_QUIET_POS                      (2)

#define BMA400_TAP_TICS_TH_MSK                    (0x03)

#define BMA400_TAP_SENSITIVITY_MSK                (0X07)

#define BMA400_ACT_CH_AXES_EN_MSK                 (0xE0)
#define BMA400_ACT_CH_AXES_EN_POS                 (5)

#define BMA400_ACT_CH_DATA_SRC_MSK                (0x10)
#define BMA400_ACT_CH_DATA_SRC_POS                (4)

#define BMA400_ACT_CH_NPTS_MSK                    (0x0F)

#define BMA400_INT_AXES_EN_MSK                    (0xE0)
#define BMA400_INT_AXES_EN_POS                    (5)

#define BMA400_INT_DATA_SRC_MSK                   (0x10)
#define BMA400_INT_DATA_SRC_POS                   (4)

#define BMA400_INT_REFU_MSK                       (0x0C)
#define BMA400_INT_REFU_POS                       (2)

#define BMA400_GEN1_ACT_REFU_MANUAL               (0x00) << 2
#define BMA400_GEN1_ACT_REFU_ONETIME              (0x01) << 2
#define BMA400_GEN1_ACT_REFU_EVERYTIME            (0x02) << 2
#define BMA400_GEN1_ACT_REFU_EVERYTIME_LP         (0x03) << 2

#define BMA400_GEN1_DATA_SRC_ACC_FILT1            (0x00) << 4
#define BMA400_GEN1_DATA_SRC_ACC_FILT2            (0x01) << 4

#define BMA400_GEN1_ACT_X_EN                      (0x01) << 5
#define BMA400_GEN1_ACT_Y_EN                      (0x01) << 6
#define BMA400_GEN1_ACT_Z_EN                      (0x01) << 7

#define BMA400_INT_HYST_MSK                       (0x03)

#define BMA400_GEN_INT_COMB_MSK                   (0x01)

#define BMA400_GEN_INT_CRITERION_MSK              (0x02)
#define BMA400_GEN_INT_CRITERION_POS              (0x01)

#define BMA400_INT_PIN1_CONF_MSK                  (0x06)
#define BMA400_INT_PIN1_CONF_POS                  (1)

#define BMA400_INT_PIN2_CONF_MSK                  (0x60)
#define BMA400_INT_PIN2_CONF_POS                  (5)

#define BMA400_INT_STATUS_MSK                     (0xE0)
#define BMA400_INT_STATUS_POS                     (5)

#define BMA400_EN_DRDY_MSK                        (0x80)
#define BMA400_EN_DRDY_POS                        (7)

#define BMA400_EN_FIFO_WM_MSK                     (0x40)
#define BMA400_EN_FIFO_WM_POS                     (6)

#define BMA400_EN_FIFO_FULL_MSK                   (0x20)
#define BMA400_EN_FIFO_FULL_POS                   (5)

#define BMA400_EN_INT_OVERRUN_MSK                 (0x10)
#define BMA400_EN_INT_OVERRUN_POS                 (4)

#define BMA400_EN_GEN2_MSK                        (0x08)
#define BMA400_EN_GEN2_POS                        (3)

#define BMA400_EN_GEN1_MSK                        (0x04)
#define BMA400_EN_GEN1_POS                        (2)

#define BMA400_EN_ORIENT_CH_MSK                   (0x02)
#define BMA400_EN_ORIENT_CH_POS                   (1)

#define BMA400_EN_LATCH_MSK                       (0x80)
#define BMA400_EN_LATCH_POS                       (7)

#define BMA400_EN_ACTCH_MSK                       (0x10)
#define BMA400_EN_ACTCH_POS                       (4)

#define BMA400_EN_D_TAP_MSK                       (0x08)
#define BMA400_EN_D_TAP_POS                       (3)

#define BMA400_EN_S_TAP_MSK                       (0x04)
#define BMA400_EN_S_TAP_POS                       (2)

#define BMA400_EN_STEP_INT_MSK                    (0x01)

#define BMA400_STEP_MAP_INT2_MSK                  (0x10)
#define BMA400_STEP_MAP_INT2_POS                  (4)

#define BMA400_EN_WAKEUP_INT_MSK                  (0x01)

#define BMA400_TAP_MAP_INT1_MSK                   (0x04)
#define BMA400_TAP_MAP_INT1_POS                   (2)

#define BMA400_TAP_MAP_INT2_MSK                   (0x40)
#define BMA400_TAP_MAP_INT2_POS                   (6)

#define BMA400_ACTCH_MAP_INT1_MSK                 (0x08)
#define BMA400_ACTCH_MAP_INT1_POS                 (3)

#define BMA400_ACTCH_MAP_INT2_MSK                 (0x80)
#define BMA400_ACTCH_MAP_INT2_POS                 (7)

#define BMA400_FIFO_BYTES_CNT_MSK                 (0x07)

#define BMA400_FIFO_TIME_EN_MSK                   (0x04)
#define BMA400_FIFO_TIME_EN_POS                   (2)

#define BMA400_FIFO_AXES_EN_MSK                   (0xE0)
#define BMA400_FIFO_AXES_EN_POS                   (5)

#define BMA400_FIFO_8_BIT_EN_MSK                  (0x10)
#define BMA400_FIFO_8_BIT_EN_POS                  (4)

/* Macro to SET and GET BITS of a register */
#define BMA400_SET_BITS(reg_data, bitname, data)	\
	((reg_data & ~(bitname##_MSK)) |		\
	 ((data << bitname##_POS) & bitname##_MSK))

#define BMA400_GET_BITS(reg_data, bitname)        ((reg_data & (bitname##_MSK)) >> \
                                                   (bitname##_POS))

#define BMA400_SET_BITS_POS_0(reg_data, bitname, data)	\
	((reg_data & ~(bitname##_MSK)) |		\
	 (data & bitname##_MSK))

#define BMA400_GET_BITS_POS_0(reg_data, bitname)  (reg_data & (bitname##_MSK))

#define BMA400_SET_BIT_VAL_0(reg_data, bitname)   (reg_data & ~(bitname##_MSK))

#define BMA400_GET_LSB(var)                       (uint8_t)(var & BMA400_SET_LOW_BYTE)
#define BMA400_GET_MSB(var)                       (uint8_t)((var & BMA400_SET_HIGH_BYTE) >> 8)

/* Macros used for Self test */

/*
 * Derivation of values obtained by :
 * Signal_Diff = ( (LSB/g value based on Accel Range) * (Minimum difference signal value) ) / 1000
 */

/* Self-test: Resulting minimum difference signal for BMA400 with Range 4G */

#define BMA400_ST_ACC_X_AXIS_SIGNAL_DIFF          (768)
#define BMA400_ST_ACC_Y_AXIS_SIGNAL_DIFF          (614)
#define BMA400_ST_ACC_Z_AXIS_SIGNAL_DIFF          (128)


struct bma400_data {
	const struct device *i2c;
	const uint8_t addr;
	int range;
	int resolution;

	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;
	int8_t temp_sample;

#ifdef CONFIG_BMA400_TRIGGER
	const struct device *dev;
	const struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

	struct sensor_trigger any_motion_trigger;
	sensor_trigger_handler_t any_motion_handler;

#if defined(CONFIG_BMA400_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_BMA400_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_BMA400_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_BMA400_TRIGGER */
};

#ifdef CONFIG_BMA400_TRIGGER
int bma400_attr_set(const struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);
int bma400_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
int bma400_init_interrupt(const struct device *dev);

#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA400_BMA400_H_ */
