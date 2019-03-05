/*
 ******************************************************************************
 * @file    lsm6ds3_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          lsm6ds3_reg.c driver.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2019 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LSM6DS3_REGS_H
#define LSM6DS3_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LSM6DS3
  * @{
  *
  */

/** @defgroup LSM6DS3_sensors_common_types
  * @{
  *
  */

#ifndef MEMS_SHARED_TYPES
#define MEMS_SHARED_TYPES

/**
  * @defgroup axisXbitXX_t
  * @brief    These unions are useful to represent different sensors data type.
  *           These unions are not need by the driver.
  *
  *           REMOVING the unions you are compliant with:
  *           MISRA-C 2012 [Rule 19.2] -> " Union are not allowed "
  *
  * @{
  *
  */

typedef union{
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

typedef union{
  int16_t i16bit;
  uint8_t u8bit[2];
} axis1bit16_t;

typedef union{
  int32_t i32bit[3];
  uint8_t u8bit[12];
} axis3bit32_t;

typedef union{
  int32_t i32bit;
  uint8_t u8bit[4];
} axis1bit32_t;

/**
  * @}
  *
  */

typedef struct{
  uint8_t bit0       : 1;
  uint8_t bit1       : 1;
  uint8_t bit2       : 1;
  uint8_t bit3       : 1;
  uint8_t bit4       : 1;
  uint8_t bit5       : 1;
  uint8_t bit6       : 1;
  uint8_t bit7       : 1;
} bitwise_t;

#define PROPERTY_DISABLE                (0U)
#define PROPERTY_ENABLE                 (1U)

#endif /* MEMS_SHARED_TYPES */

/**
  * @}
  *
  */

/** @addtogroup  LSM6DS3_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*lsm6ds3_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lsm6ds3_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lsm6ds3_write_ptr  write_reg;
  lsm6ds3_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lsm6ds3_ctx_t;

/**
  * @}
  *
  */

/** @defgroup LSM6DS3_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> 0xD5 if SA0=1 -> 0xD7 **/
#define LSM6DS3_I2C_ADD_L                          0xD5U
#define LSM6DS3_I2C_ADD_H                          0xD7U
/** Device Identification (Who am I) **/
#define LSM6DS3_ID                                 0x69U

/**
  * @}
  *
  */

#define LSM6DS3_FUNC_CFG_ACCESS                    0x01U
typedef struct {
  uint8_t not_used_01                 : 7;
  uint8_t func_cfg_en                 : 1;
} lsm6ds3_func_cfg_access_t;

#define LSM6DS3_SENSOR_SYNC_TIME_FRAME             0x04U
typedef struct {
  uint8_t tph                         : 8;
} lsm6ds3_sensor_sync_time_frame_t;

#define LSM6DS3_FIFO_CTRL1                         0x06U
typedef struct {
  uint8_t fth                         : 8;
} lsm6ds3_fifo_ctrl1_t;

#define LSM6DS3_FIFO_CTRL2                         0x07U
typedef struct {
  uint8_t fth                         : 4;
  uint8_t not_used_01                 : 2;
  uint8_t  timer_pedo_fifo_drdy       : 1;
  uint8_t timer_pedo_fifo_en          : 1;
} lsm6ds3_fifo_ctrl2_t;

#define LSM6DS3_FIFO_CTRL3                         0x08U
typedef struct {
  uint8_t dec_fifo_xl                 : 3;
  uint8_t dec_fifo_gyro               : 3;
  uint8_t not_used_01                 : 2;
} lsm6ds3_fifo_ctrl3_t;

#define LSM6DS3_FIFO_CTRL4                         0x09U
typedef struct {
  uint8_t dec_ds3_fifo                : 3;
  uint8_t dec_ds4_fifo                : 3;
  uint8_t only_high_data              : 1;
  uint8_t not_used_01                 : 1;
} lsm6ds3_fifo_ctrl4_t;

#define LSM6DS3_FIFO_CTRL5                         0x0AU
typedef struct {
  uint8_t fifo_mode                   : 3;
  uint8_t odr_fifo                    : 4;
  uint8_t not_used_01                 : 1;
} lsm6ds3_fifo_ctrl5_t;

#define LSM6DS3_ORIENT_CFG_G                       0x0BU
typedef struct {
  uint8_t orient                      : 3;
  uint8_t sign_g                      : 3;  /* SignX_G) + SignY_G + SignZ_G */
  uint8_t not_used_01                 : 2;
} lsm6ds3_orient_cfg_g_t;

#define LSM6DS3_INT1_CTRL                          0x0DU
typedef struct {
  uint8_t int1_drdy_xl                : 1;
  uint8_t int1_drdy_g                 : 1;
  uint8_t int1_boot                   : 1;
  uint8_t int1_fth                    : 1;
  uint8_t int1_fifo_ovr               : 1;
  uint8_t int1_full_flag              : 1;
  uint8_t int1_sign_mot               : 1;
  uint8_t int1_step_detector          : 1;
} lsm6ds3_int1_ctrl_t;

#define LSM6DS3_INT2_CTRL                          0x0EU
typedef struct {
  uint8_t int2_drdy_xl                : 1;
  uint8_t int2_drdy_g                 : 1;
  uint8_t int2_drdy_temp              : 1;
  uint8_t int2_fth                    : 1;
  uint8_t int2_fifo_ovr               : 1;
  uint8_t int2_full_flag              : 1;
  uint8_t int2_step_count_ov          : 1;
  uint8_t int2_step_delta             : 1;
} lsm6ds3_int2_ctrl_t;

#define LSM6DS3_WHO_AM_I                           0x0FU
#define LSM6DS3_CTRL1_XL                           0x10U
typedef struct {
  uint8_t bw_xl                       : 2;
  uint8_t fs_xl                       : 2;
  uint8_t odr_xl                      : 4;
} lsm6ds3_ctrl1_xl_t;

#define LSM6DS3_CTRL2_G                            0x11U
typedef struct {
  uint8_t not_used_01                 : 1;
  uint8_t fs_g                        : 3;    /* FS_G + FS_125 */
  uint8_t odr_g                       : 4;
} lsm6ds3_ctrl2_g_t;

#define LSM6DS3_CTRL3_C                            0x12U
typedef struct {
  uint8_t sw_reset                    : 1;
  uint8_t ble                         : 1;
  uint8_t if_inc                      : 1;
  uint8_t sim                         : 1;
  uint8_t pp_od                       : 1;
  uint8_t h_lactive                   : 1;
  uint8_t bdu                         : 1;
  uint8_t boot                        : 1;
} lsm6ds3_ctrl3_c_t;

#define LSM6DS3_CTRL4_C                            0x13U
typedef struct {
  uint8_t stop_on_fth                 : 1;
  uint8_t not_used_01                 : 1;
  uint8_t i2c_disable                 : 1;
  uint8_t drdy_mask                   : 1;
  uint8_t fifo_temp_en                : 1;
  uint8_t int2_on_int1                : 1;
  uint8_t sleep_g                     : 1;
  uint8_t xl_bw_scal_odr              : 1;
} lsm6ds3_ctrl4_c_t;

#define LSM6DS3_CTRL5_C                            0x14U
typedef struct {
  uint8_t st_xl                       : 2;
  uint8_t st_g                        : 2;
  uint8_t not_used_01                 : 1;
  uint8_t rounding                    : 3;
} lsm6ds3_ctrl5_c_t;

#define LSM6DS3_CTRL6_C                            0x15U
typedef struct {
  uint8_t not_used_01                 : 4;
  uint8_t xl_hm_mode                  : 1;
  uint8_t den_mode                    : 3;  /* trig_en + lvl1_en + lvl2_en */
} lsm6ds3_ctrl6_c_t;

#define LSM6DS3_CTRL7_G                            0x16U
typedef struct {
  uint8_t not_used_01                 : 2;
  uint8_t rounding_status             : 1;
  uint8_t hpcf_g                      : 2;
  uint8_t hp_g_rst                    : 1;
  uint8_t hp_g_en                     : 1;
  uint8_t g_hm_mode                   : 1;
} lsm6ds3_ctrl7_g_t;

#define LSM6DS3_CTRL8_XL                           0x17U
typedef struct {
  uint8_t low_pass_on_6d              : 1;
  uint8_t not_used_01                 : 1;
  uint8_t hp_slope_xl_en              : 1;
  uint8_t not_used_02                 : 2;
  uint8_t hpcf_xl                     : 2;
  uint8_t lpf2_xl_en                  : 1;
} lsm6ds3_ctrl8_xl_t;

#define LSM6DS3_CTRL9_XL                           0x18U
typedef struct {
  uint8_t not_used_01                 : 2;
  uint8_t soft_en                     : 1;
  uint8_t xen_xl                      : 1;
  uint8_t yen_xl                      : 1;
  uint8_t zen_xl                      : 1;
  uint8_t not_used_02                 : 2;
} lsm6ds3_ctrl9_xl_t;

#define LSM6DS3_CTRL10_C                           0x19U
typedef struct {
  uint8_t sign_motion_en              : 1;
  uint8_t pedo_rst_step               : 1;
  uint8_t func_en                     : 1;
  uint8_t xen_g                       : 1;
  uint8_t yen_g                       : 1;
  uint8_t zen_g                       : 1;
  uint8_t not_used_01                 : 2;
} lsm6ds3_ctrl10_c_t;

#define LSM6DS3_MASTER_CONFIG                      0x1AU
typedef struct {
  uint8_t master_on                   : 1;
  uint8_t iron_en                     : 1;
  uint8_t pass_through_mode           : 1;
  uint8_t pull_up_en                  : 1;
  uint8_t start_config                : 1;
  uint8_t not_used_01                 : 1;
  uint8_t data_valid_sel_fifo         : 1;
  uint8_t drdy_on_int1                : 1;
} lsm6ds3_master_config_t;

#define LSM6DS3_WAKE_UP_SRC                        0x1BU
typedef struct {
  uint8_t z_wu                        : 1;
  uint8_t y_wu                        : 1;
  uint8_t x_wu                        : 1;
  uint8_t wu_ia                       : 1;
  uint8_t sleep_state_ia              : 1;
  uint8_t ff_ia                       : 1;
  uint8_t not_used_01                 : 2;
} lsm6ds3_wake_up_src_t;

#define LSM6DS3_TAP_SRC                            0x1CU
typedef struct {
  uint8_t z_tap                       : 1;
  uint8_t y_tap                       : 1;
  uint8_t x_tap                       : 1;
  uint8_t tap_sign                    : 1;
  uint8_t double_tap                  : 1;
  uint8_t single_tap                  : 1;
  uint8_t tap_ia                      : 1;
  uint8_t not_used_01                 : 1;
} lsm6ds3_tap_src_t;

#define LSM6DS3_D6D_SRC                            0x1DU
typedef struct {
  uint8_t xl                          : 1;
  uint8_t xh                          : 1;
  uint8_t yl                          : 1;
  uint8_t yh                          : 1;
  uint8_t zl                          : 1;
  uint8_t zh                          : 1;
  uint8_t d6d_ia                      : 1;
  uint8_t not_used_01                 : 1;
} lsm6ds3_d6d_src_t;

#define LSM6DS3_STATUS_REG                         0x1EU
typedef struct {
  uint8_t xlda                        : 1;
  uint8_t gda                         : 1;
  uint8_t tda                         : 1;
  uint8_t not_used_01                 : 5;
} lsm6ds3_status_reg_t;

#define LSM6DS3_OUT_TEMP_L                         0x20U
#define LSM6DS3_OUT_TEMP_H                         0x21U
#define LSM6DS3_OUTX_L_G                           0x22U
#define LSM6DS3_OUTX_H_G                           0x23U
#define LSM6DS3_OUTY_L_G                           0x24U
#define LSM6DS3_OUTY_H_G                           0x25U
#define LSM6DS3_OUTZ_L_G                           0x26U
#define LSM6DS3_OUTZ_H_G                           0x27U
#define LSM6DS3_OUTX_L_XL                          0x28U
#define LSM6DS3_OUTX_H_XL                          0x29U
#define LSM6DS3_OUTY_L_XL                          0x2AU
#define LSM6DS3_OUTY_H_XL                          0x2BU
#define LSM6DS3_OUTZ_L_XL                          0x2CU
#define LSM6DS3_OUTZ_H_XL                          0x2DU
#define LSM6DS3_SENSORHUB1_REG                     0x2EU
typedef struct {
  uint8_t shub1_0                     : 1;
  uint8_t shub1_1                     : 1;
  uint8_t shub1_2                     : 1;
  uint8_t shub1_3                     : 1;
  uint8_t shub1_4                     : 1;
  uint8_t shub1_5                     : 1;
  uint8_t shub1_6                     : 1;
  uint8_t shub1_7                     : 1;
} lsm6ds3_sensorhub1_reg_t;

#define LSM6DS3_SENSORHUB2_REG                     0x2FU
typedef struct {
  uint8_t shub2_0                     : 1;
  uint8_t shub2_1                     : 1;
  uint8_t shub2_2                     : 1;
  uint8_t shub2_3                     : 1;
  uint8_t shub2_4                     : 1;
  uint8_t shub2_5                     : 1;
  uint8_t shub2_6                     : 1;
  uint8_t shub2_7                     : 1;
} lsm6ds3_sensorhub2_reg_t;

#define LSM6DS3_SENSORHUB3_REG                     0x30U
typedef struct {
  uint8_t shub3_0                     : 1;
  uint8_t shub3_1                     : 1;
  uint8_t shub3_2                     : 1;
  uint8_t shub3_3                     : 1;
  uint8_t shub3_4                     : 1;
  uint8_t shub3_5                     : 1;
  uint8_t shub3_6                     : 1;
  uint8_t shub3_7                     : 1;
} lsm6ds3_sensorhub3_reg_t;

#define LSM6DS3_SENSORHUB4_REG                     0x31U
typedef struct {
  uint8_t shub4_0                     : 1;
  uint8_t shub4_1                     : 1;
  uint8_t shub4_2                     : 1;
  uint8_t shub4_3                     : 1;
  uint8_t shub4_4                     : 1;
  uint8_t shub4_5                     : 1;
  uint8_t shub4_6                     : 1;
  uint8_t shub4_7                     : 1;
} lsm6ds3_sensorhub4_reg_t;

#define LSM6DS3_SENSORHUB5_REG                     0x32U
typedef struct {
  uint8_t shub5_0                     : 1;
  uint8_t shub5_1                     : 1;
  uint8_t shub5_2                     : 1;
  uint8_t shub5_3                     : 1;
  uint8_t shub5_4                     : 1;
  uint8_t shub5_5                     : 1;
  uint8_t shub5_6                     : 1;
  uint8_t shub5_7                     : 1;
} lsm6ds3_sensorhub5_reg_t;

#define LSM6DS3_SENSORHUB6_REG                     0x33U
typedef struct {
  uint8_t shub6_0                     : 1;
  uint8_t shub6_1                     : 1;
  uint8_t shub6_2                     : 1;
  uint8_t shub6_3                     : 1;
  uint8_t shub6_4                     : 1;
  uint8_t shub6_5                     : 1;
  uint8_t shub6_6                     : 1;
  uint8_t shub6_7                     : 1;
} lsm6ds3_sensorhub6_reg_t;

#define LSM6DS3_SENSORHUB7_REG                     0x34U
typedef struct {
  uint8_t shub7_0                     : 1;
  uint8_t shub7_1                     : 1;
  uint8_t shub7_2                     : 1;
  uint8_t shub7_3                     : 1;
  uint8_t shub7_4                     : 1;
  uint8_t shub7_5                     : 1;
  uint8_t shub7_6                     : 1;
  uint8_t shub7_7                     : 1;
} lsm6ds3_sensorhub7_reg_t;

#define LSM6DS3_SENSORHUB8_REG                     0x35U
typedef struct {
  uint8_t shub8_0                     : 1;
  uint8_t shub8_1                     : 1;
  uint8_t shub8_2                     : 1;
  uint8_t shub8_3                     : 1;
  uint8_t shub8_4                     : 1;
  uint8_t shub8_5                     : 1;
  uint8_t shub8_6                     : 1;
  uint8_t shub8_7                     : 1;
} lsm6ds3_sensorhub8_reg_t;

#define LSM6DS3_SENSORHUB9_REG                     0x36U
typedef struct {
  uint8_t shub9_0                     : 1;
  uint8_t shub9_1                     : 1;
  uint8_t shub9_2                     : 1;
  uint8_t shub9_3                     : 1;
  uint8_t shub9_4                     : 1;
  uint8_t shub9_5                     : 1;
  uint8_t shub9_6                     : 1;
  uint8_t shub9_7                     : 1;
} lsm6ds3_sensorhub9_reg_t;

#define LSM6DS3_SENSORHUB10_REG                    0x37U
typedef struct {
  uint8_t shub10_0                    : 1;
  uint8_t shub10_1                    : 1;
  uint8_t shub10_2                    : 1;
  uint8_t shub10_3                    : 1;
  uint8_t shub10_4                    : 1;
  uint8_t shub10_5                    : 1;
  uint8_t shub10_6                    : 1;
  uint8_t shub10_7                    : 1;
} lsm6ds3_sensorhub10_reg_t;

#define LSM6DS3_SENSORHUB11_REG                    0x38U
typedef struct {
  uint8_t shub11_0                    : 1;
  uint8_t shub11_1                    : 1;
  uint8_t shub11_2                    : 1;
  uint8_t shub11_3                    : 1;
  uint8_t shub11_4                    : 1;
  uint8_t shub11_5                    : 1;
  uint8_t shub11_6                    : 1;
  uint8_t shub11_7                    : 1;
} lsm6ds3_sensorhub11_reg_t;

#define LSM6DS3_SENSORHUB12_REG                    0x39U
typedef struct {
  uint8_t shub12_0                    : 1;
  uint8_t shub12_1                    : 1;
  uint8_t shub12_2                    : 1;
  uint8_t shub12_3                    : 1;
  uint8_t shub12_4                    : 1;
  uint8_t shub12_5                    : 1;
  uint8_t shub12_6                    : 1;
  uint8_t shub12_7                    : 1;
} lsm6ds3_sensorhub12_reg_t;

#define LSM6DS3_FIFO_STATUS1                       0x3AU
typedef struct {
  uint8_t diff_fifo                   : 8;
} lsm6ds3_fifo_status1_t;

#define LSM6DS3_FIFO_STATUS2                       0x3BU
typedef struct {
  uint8_t diff_fifo                   : 4;
  uint8_t fifo_empty                  : 1;
  uint8_t fifo_full                   : 1;
  uint8_t fifo_over_run               : 1;
  uint8_t fth                         : 1;
} lsm6ds3_fifo_status2_t;

#define LSM6DS3_FIFO_STATUS3                        0x3CU
typedef struct {
  uint8_t fifo_pattern                : 8;
} lsm6ds3_fifo_status3_t;

#define LSM6DS3_FIFO_STATUS4                       0x3DU
typedef struct {
  uint8_t fifo_pattern                : 2;
  uint8_t not_used_01                 : 6;
} lsm6ds3_fifo_status4_t;

#define LSM6DS3_FIFO_DATA_OUT_L                    0x3EU
#define LSM6DS3_FIFO_DATA_OUT_H                    0x3FU
#define LSM6DS3_TIMESTAMP0_REG                     0x40U
#define LSM6DS3_TIMESTAMP1_REG                     0x41U
#define LSM6DS3_TIMESTAMP2_REG                     0x42U
#define LSM6DS3_STEP_TIMESTAMP_L                   0x49U
#define LSM6DS3_STEP_TIMESTAMP_H                   0x4AU
#define LSM6DS3_STEP_COUNTER_L                     0x4BU
#define LSM6DS3_STEP_COUNTER_H                     0x4CU
#define LSM6DS3_SENSORHUB13_REG                    0x4DU
typedef struct {
  uint8_t shub13_0                    : 1;
  uint8_t shub13_1                    : 1;
  uint8_t shub13_2                    : 1;
  uint8_t shub13_3                    : 1;
  uint8_t shub13_4                    : 1;
  uint8_t shub13_5                    : 1;
  uint8_t shub13_6                    : 1;
  uint8_t shub13_7                    : 1;
} lsm6ds3_sensorhub13_reg_t;

#define LSM6DS3_SENSORHUB14_REG                    0x4EU
typedef struct {
  uint8_t shub14_0                    : 1;
  uint8_t shub14_1                    : 1;
  uint8_t shub14_2                    : 1;
  uint8_t shub14_3                    : 1;
  uint8_t shub14_4                    : 1;
  uint8_t shub14_5                    : 1;
  uint8_t shub14_6                    : 1;
  uint8_t shub14_7                    : 1;
} lsm6ds3_sensorhub14_reg_t;

#define LSM6DS3_SENSORHUB15_REG                    0x4FU
typedef struct {
  uint8_t shub15_0                    : 1;
  uint8_t shub15_1                    : 1;
  uint8_t shub15_2                    : 1;
  uint8_t shub15_3                    : 1;
  uint8_t shub15_4                    : 1;
  uint8_t shub15_5                    : 1;
  uint8_t shub15_6                    : 1;
  uint8_t shub15_7                    : 1;
} lsm6ds3_sensorhub15_reg_t;

#define LSM6DS3_SENSORHUB16_REG                    0x50U
typedef struct {
  uint8_t shub16_0                    : 1;
  uint8_t shub16_1                    : 1;
  uint8_t shub16_2                    : 1;
  uint8_t shub16_3                    : 1;
  uint8_t shub16_4                    : 1;
  uint8_t shub16_5                    : 1;
  uint8_t shub16_6                    : 1;
  uint8_t shub16_7                    : 1;
} lsm6ds3_sensorhub16_reg_t;

#define LSM6DS3_SENSORHUB17_REG                    0x51U
typedef struct {
  uint8_t shub17_0                    : 1;
  uint8_t shub17_1                    : 1;
  uint8_t shub17_2                    : 1;
  uint8_t shub17_3                    : 1;
  uint8_t shub17_4                    : 1;
  uint8_t shub17_5                    : 1;
  uint8_t shub17_6                    : 1;
  uint8_t shub17_7                    : 1;
} lsm6ds3_sensorhub17_reg_t;

#define LSM6DS3_SENSORHUB18_REG                    0x52U
typedef struct {
  uint8_t shub18_0                    : 1;
  uint8_t shub18_1                    : 1;
  uint8_t shub18_2                    : 1;
  uint8_t shub18_3                    : 1;
  uint8_t shub18_4                    : 1;
  uint8_t shub18_5                    : 1;
  uint8_t shub18_6                    : 1;
  uint8_t shub18_7                    : 1;
} lsm6ds3_sensorhub18_reg_t;

#define LSM6DS3_FUNC_SRC                           0x53U
typedef struct {
  uint8_t sensor_hub_end_op           : 1;
  uint8_t si_end_op                   : 1;
  uint8_t not_used_01                 : 1;
  uint8_t step_overflow               : 1;
  uint8_t step_detected               : 1;
  uint8_t tilt_ia                     : 1;
  uint8_t sign_motion_ia              : 1;
  uint8_t step_count_delta_ia         : 1;
} lsm6ds3_func_src_t;

#define LSM6DS3_TAP_CFG                            0x58U
typedef struct {
  uint8_t lir                         : 1;
  uint8_t tap_z_en                    : 1;
  uint8_t tap_y_en                    : 1;
  uint8_t tap_x_en                    : 1;
  uint8_t slope_fds                   : 1;
  uint8_t tilt_en                     : 1;
  uint8_t pedo_en                     : 1;
  uint8_t timer_en                    : 1;
} lsm6ds3_tap_cfg_t;

#define LSM6DS3_TAP_THS_6D                         0x59U
typedef struct {
  uint8_t tap_ths                     : 5;
  uint8_t sixd_ths                    : 2;
  uint8_t d4d_en                      : 1;
} lsm6ds3_tap_ths_6d_t;

#define LSM6DS3_INT_DUR2                           0x5AU
typedef struct {
  uint8_t shock                       : 2;
  uint8_t quiet                       : 2;
  uint8_t dur                         : 4;
} lsm6ds3_int_dur2_t;

#define LSM6DS3_WAKE_UP_THS                        0x5BU
typedef struct {
  uint8_t wk_ths                      : 6;
  uint8_t inactivity                  : 1;
  uint8_t single_double_tap           : 1;
} lsm6ds3_wake_up_ths_t;

#define LSM6DS3_WAKE_UP_DUR                        0x5CU
typedef struct {
  uint8_t sleep_dur                   : 4;
  uint8_t timer_hr                    : 1;
  uint8_t wake_dur                    : 2;
  uint8_t ff_dur                      : 1;
} lsm6ds3_wake_up_dur_t;

#define LSM6DS3_FREE_FALL                          0x5DU
typedef struct {
  uint8_t ff_ths                      : 3;
  uint8_t ff_dur                      : 5;
} lsm6ds3_free_fall_t;

#define LSM6DS3_MD1_CFG                            0x5EU
typedef struct {
  uint8_t int1_timer                  : 1;
  uint8_t int1_tilt                   : 1;
  uint8_t int1_6d                     : 1;
  uint8_t int1_double_tap             : 1;
  uint8_t int1_ff                     : 1;
  uint8_t int1_wu                     : 1;
  uint8_t int1_single_tap             : 1;
  uint8_t int1_inact_state            : 1;
} lsm6ds3_md1_cfg_t;

#define LSM6DS3_MD2_CFG                            0x5FU
typedef struct {
  uint8_t int2_iron                   : 1;
  uint8_t int2_tilt                   : 1;
  uint8_t int2_6d                     : 1;
  uint8_t int2_double_tap             : 1;
  uint8_t int2_ff                     : 1;
  uint8_t int2_wu                     : 1;
  uint8_t int2_single_tap             : 1;
  uint8_t int2_inact_state            : 1;
} lsm6ds3_md2_cfg_t;

#define LSM6DS3_OUT_MAG_RAW_X_L                    0x66U
#define LSM6DS3_OUT_MAG_RAW_X_H                    0x67U
#define LSM6DS3_OUT_MAG_RAW_Y_L                    0x68U
#define LSM6DS3_OUT_MAG_RAW_Y_H                    0x69U
#define LSM6DS3_OUT_MAG_RAW_Z_L                    0x6AU
#define LSM6DS3_OUT_MAG_RAW_Z_H                    0x6BU
#define LSM6DS3_SLV0_ADD                           0x02U
typedef struct {
  uint8_t rw_0                        : 1;
  uint8_t slave0_add                  : 7;
} lsm6ds3_slv0_add_t;

#define LSM6DS3_SLV0_SUBADD                        0x03U
typedef struct {
  uint8_t slave0_reg                  : 8;
} lsm6ds3_slv0_subadd_t;

#define LSM6DS3_SLAVE0_CONFIG                      0x04U
typedef struct {
  uint8_t slave0_numop                : 3;
  uint8_t src_mode                    : 1;
  uint8_t aux_sens_on                 : 2;
  uint8_t slave0_rate                 : 2;
} lsm6ds3_slave0_config_t;

#define LSM6DS3_SLV1_ADD                           0x05U
typedef struct {
  uint8_t r_1                         : 1;
  uint8_t slave1_add                  : 7;
} lsm6ds3_slv1_add_t;

#define LSM6DS3_SLV1_SUBADD                        0x06U
typedef struct {
  uint8_t slave1_reg                  : 8;
} lsm6ds3_slv1_subadd_t;

#define LSM6DS3_SLAVE1_CONFIG                      0x07U
typedef struct {
  uint8_t slave1_numop                : 3;
  uint8_t not_used_01                 : 3;
  uint8_t slave1_rate                 : 2;
} lsm6ds3_slave1_config_t;

#define LSM6DS3_SLV2_ADD                           0x08U
typedef struct {
  uint8_t r_2                         : 1;
  uint8_t slave2_add                  : 7;
} lsm6ds3_slv2_add_t;

#define LSM6DS3_SLV2_SUBADD                        0x09U
typedef struct {
  uint8_t slave2_reg                  : 8;
} lsm6ds3_slv2_subadd_t;

#define LSM6DS3_SLAVE2_CONFIG                      0x0AU
typedef struct {
  uint8_t slave2_numop                : 3;
  uint8_t not_used_01                 : 3;
  uint8_t slave2_rate                 : 2;
} lsm6ds3_slave2_config_t;

#define LSM6DS3_SLV3_ADD                           0x0BU
typedef struct {
  uint8_t r_3                         : 1;
  uint8_t slave3_add                  : 7;
} lsm6ds3_slv3_add_t;

#define LSM6DS3_SLV3_SUBADD                        0x0CU
typedef struct {
  uint8_t slave3_reg                  : 8;
} lsm6ds3_slv3_subadd_t;

#define LSM6DS3_SLAVE3_CONFIG                      0x0DU
typedef struct {
  uint8_t slave3_numop                : 3;
  uint8_t not_used_01                 : 3;
  uint8_t slave3_rate                 : 2;
} lsm6ds3_slave3_config_t;

#define LSM6DS3_DATAWRITE_SRC_MODE_SUB_SLV0        0x0EU
typedef struct {
  uint8_t slave_dataw                 : 8;
} lsm6ds3_datawrite_src_mode_sub_slv0_t;

#define LSM6DS3_PEDO_THS_REG                       0x0FU
typedef struct {
  uint8_t ths_min                     : 5;
  uint8_t not_used_01                 : 2;
  uint8_t pedo_4g                     : 1;
} lsm6ds3_pedo_ths_reg_t;

#define LSM6DS3_SM_THS                             0x13U
typedef struct {
  uint8_t sm_ths                      : 8;
} lsm6ds3_sm_ths_t;

#define LSM6DS3_PEDO_DEB_REG                       0x14U
typedef struct {
  uint8_t deb_step                    : 3;
  uint8_t deb_time                    : 5;
} lsm6ds3_pedo_deb_reg_t;

#define LSM6DS3_STEP_COUNT_DELTA                   0x15U
typedef struct {
  uint8_t sc_delta                    : 8;
} lsm6ds3_step_count_delta_t;

#define LSM6DS3_MAG_SI_XX                          0x24U
#define LSM6DS3_MAG_SI_XY                          0x25U
#define LSM6DS3_MAG_SI_XZ                          0x26U
#define LSM6DS3_MAG_SI_YX                          0x27U
#define LSM6DS3_MAG_SI_YY                          0x28U
#define LSM6DS3_MAG_SI_YZ                          0x29U
#define LSM6DS3_MAG_SI_ZX                          0x2AU
#define LSM6DS3_MAG_SI_ZY                          0x2BU
#define LSM6DS3_MAG_SI_ZZ                          0x2CU
#define LSM6DS3_MAG_OFFX_L                         0x2DU
#define LSM6DS3_MAG_OFFX_H                         0x2EU
#define LSM6DS3_MAG_OFFY_L                         0x2FU
#define LSM6DS3_MAG_OFFY_H                         0x30U
#define LSM6DS3_MAG_OFFZ_L                         0x31U
#define LSM6DS3_MAG_OFFZ_H                         0x32U

/**
  * @defgroup LSM6DS3_Register_Union
  * @brief    This union group all the registers that has a bit-field
  *           description.
  *           This union is useful but not need by the driver.
  *
  *           REMOVING this union you are compliant with:
  *           MISRA-C 2012 [Rule 19.2] -> " Union are not allowed "
  *
  * @{
  *
  */

typedef union{
  lsm6ds3_func_cfg_access_t                  func_cfg_access;
  lsm6ds3_sensor_sync_time_frame_t           sensor_sync_time_frame;
  lsm6ds3_fifo_ctrl1_t                       fifo_ctrl1;
  lsm6ds3_fifo_ctrl2_t                       fifo_ctrl2;
  lsm6ds3_fifo_ctrl3_t                       fifo_ctrl3;
  lsm6ds3_fifo_ctrl4_t                       fifo_ctrl4;
  lsm6ds3_fifo_ctrl5_t                       fifo_ctrl5;
  lsm6ds3_orient_cfg_g_t                     orient_cfg_g;
  lsm6ds3_int1_ctrl_t                        int1_ctrl;
  lsm6ds3_int2_ctrl_t                        int2_ctrl;
  lsm6ds3_ctrl1_xl_t                         ctrl1_xl;
  lsm6ds3_ctrl2_g_t                          ctrl2_g;
  lsm6ds3_ctrl3_c_t                          ctrl3_c;
  lsm6ds3_ctrl4_c_t                          ctrl4_c;
  lsm6ds3_ctrl5_c_t                          ctrl5_c;
  lsm6ds3_ctrl6_c_t                          ctrl6_c;
  lsm6ds3_ctrl7_g_t                          ctrl7_g;
  lsm6ds3_ctrl8_xl_t                         ctrl8_xl;
  lsm6ds3_ctrl9_xl_t                         ctrl9_xl;
  lsm6ds3_ctrl10_c_t                         ctrl10_c;
  lsm6ds3_master_config_t                    master_config;
  lsm6ds3_wake_up_src_t                      wake_up_src;
  lsm6ds3_tap_src_t                          tap_src;
  lsm6ds3_d6d_src_t                          d6d_src;
  lsm6ds3_status_reg_t                       status_reg;
  lsm6ds3_sensorhub1_reg_t                   sensorhub1_reg;
  lsm6ds3_sensorhub2_reg_t                   sensorhub2_reg;
  lsm6ds3_sensorhub3_reg_t                   sensorhub3_reg;
  lsm6ds3_sensorhub4_reg_t                   sensorhub4_reg;
  lsm6ds3_sensorhub5_reg_t                   sensorhub5_reg;
  lsm6ds3_sensorhub6_reg_t                   sensorhub6_reg;
  lsm6ds3_sensorhub7_reg_t                   sensorhub7_reg;
  lsm6ds3_sensorhub8_reg_t                   sensorhub8_reg;
  lsm6ds3_sensorhub9_reg_t                   sensorhub9_reg;
  lsm6ds3_sensorhub10_reg_t                  sensorhub10_reg;
  lsm6ds3_sensorhub11_reg_t                  sensorhub11_reg;
  lsm6ds3_sensorhub12_reg_t                  sensorhub12_reg;
  lsm6ds3_fifo_status1_t                     fifo_status1;
  lsm6ds3_fifo_status2_t                     fifo_status2;
  lsm6ds3_fifo_status3_t                     fifo_status3;
  lsm6ds3_fifo_status4_t                     fifo_status4;
  lsm6ds3_sensorhub13_reg_t                  sensorhub13_reg;
  lsm6ds3_sensorhub14_reg_t                  sensorhub14_reg;
  lsm6ds3_sensorhub15_reg_t                  sensorhub15_reg;
  lsm6ds3_sensorhub16_reg_t                  sensorhub16_reg;
  lsm6ds3_sensorhub17_reg_t                  sensorhub17_reg;
  lsm6ds3_sensorhub18_reg_t                  sensorhub18_reg;
  lsm6ds3_func_src_t                         func_src;
  lsm6ds3_tap_cfg_t                          tap_cfg;
  lsm6ds3_tap_ths_6d_t                       tap_ths_6d;
  lsm6ds3_int_dur2_t                         int_dur2;
  lsm6ds3_wake_up_ths_t                      wake_up_ths;
  lsm6ds3_wake_up_dur_t                      wake_up_dur;
  lsm6ds3_free_fall_t                        free_fall;
  lsm6ds3_md1_cfg_t                          md1_cfg;
  lsm6ds3_md2_cfg_t                          md2_cfg;
  lsm6ds3_slv0_add_t                         slv0_add;
  lsm6ds3_slv0_subadd_t                      slv0_subadd;
  lsm6ds3_slave0_config_t                    slave0_config;
  lsm6ds3_slv1_add_t                         slv1_add;
  lsm6ds3_slv1_subadd_t                      slv1_subadd;
  lsm6ds3_slave1_config_t                    slave1_config;
  lsm6ds3_slv2_add_t                         slv2_add;
  lsm6ds3_slv2_subadd_t                      slv2_subadd;
  lsm6ds3_slave2_config_t                    slave2_config;
  lsm6ds3_slv3_add_t                         slv3_add;
  lsm6ds3_slv3_subadd_t                      slv3_subadd;
  lsm6ds3_slave3_config_t                    slave3_config;
  lsm6ds3_datawrite_src_mode_sub_slv0_t      datawrite_src_mode_sub_slv0;
  lsm6ds3_pedo_ths_reg_t                     pedo_ths_reg;
  lsm6ds3_sm_ths_t                           sm_ths;
  lsm6ds3_pedo_deb_reg_t                     pedo_deb_reg;
  lsm6ds3_step_count_delta_t                 step_count_delta;
  bitwise_t                                  bitwise;
  uint8_t                                    byte;
} lsm6ds3_reg_t;

/**
  * @}
  *
  */

int32_t lsm6ds3_read_reg(lsm6ds3_ctx_t *ctx, uint8_t reg, uint8_t* data,
                         uint16_t len);
int32_t lsm6ds3_write_reg(lsm6ds3_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);

extern float_t lsm6ds3_from_fs2g_to_mg(int16_t lsb);
extern float_t lsm6ds3_from_fs4g_to_mg(int16_t lsb);
extern float_t lsm6ds3_from_fs8g_to_mg(int16_t lsb);
extern float_t lsm6ds3_from_fs16g_to_mg(int16_t lsb);

extern float_t lsm6ds3_from_fs125dps_to_mdps(int16_t lsb);
extern float_t lsm6ds3_from_fs250dps_to_mdps(int16_t lsb);
extern float_t lsm6ds3_from_fs500dps_to_mdps(int16_t lsb);
extern float_t lsm6ds3_from_fs1000dps_to_mdps(int16_t lsb);
extern float_t lsm6ds3_from_fs2000dps_to_mdps(int16_t lsb);

extern float_t lsm6ds3_from_lsb_to_celsius(int16_t lsb);

typedef enum {
  LSM6DS3_GY_ORIENT_XYZ = 0,
  LSM6DS3_GY_ORIENT_XZY = 1,
  LSM6DS3_GY_ORIENT_YXZ = 2,
  LSM6DS3_GY_ORIENT_YZX = 3,
  LSM6DS3_GY_ORIENT_ZXY = 4,
  LSM6DS3_GY_ORIENT_ZYX = 5,
} lsm6ds3_gy_orient_t;
int32_t lsm6ds3_gy_data_orient_set(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_gy_orient_t val);
int32_t lsm6ds3_gy_data_orient_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_gy_orient_t *val);

typedef enum {
  LSM6DS3_GY_SIGN_PPP = 0,
  LSM6DS3_GY_SIGN_PPN = 1,
  LSM6DS3_GY_SIGN_PNP = 2,
  LSM6DS3_GY_SIGN_NPP = 4,
  LSM6DS3_GY_SIGN_NNP = 6,
  LSM6DS3_GY_SIGN_NPN = 5,
  LSM6DS3_GY_SIGN_PNN = 3,
  LSM6DS3_GY_SIGN_NNN = 7,
} lsm6ds3_gy_sgn_t;
int32_t lsm6ds3_gy_data_sign_set(lsm6ds3_ctx_t *ctx, lsm6ds3_gy_sgn_t val);
int32_t lsm6ds3_gy_data_sign_get(lsm6ds3_ctx_t *ctx, lsm6ds3_gy_sgn_t *val);

typedef enum {
  LSM6DS3_2g  = 0,
  LSM6DS3_16g = 1,
  LSM6DS3_4g  = 2,
  LSM6DS3_8g  = 3,
} lsm6ds3_xl_fs_t;
int32_t lsm6ds3_xl_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_xl_fs_t val);
int32_t lsm6ds3_xl_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_xl_fs_t *val);

typedef enum {
  LSM6DS3_XL_ODR_OFF    = 0,
  LSM6DS3_XL_ODR_12Hz5  = 1,
  LSM6DS3_XL_ODR_26Hz   = 2,
  LSM6DS3_XL_ODR_52Hz   = 3,
  LSM6DS3_XL_ODR_104Hz  = 4,
  LSM6DS3_XL_ODR_208Hz  = 5,
  LSM6DS3_XL_ODR_416Hz  = 6,
  LSM6DS3_XL_ODR_833Hz  = 7,
  LSM6DS3_XL_ODR_1k66Hz = 8,
  LSM6DS3_XL_ODR_3k33Hz = 9,
  LSM6DS3_XL_ODR_6k66Hz = 10,
} lsm6ds3_odr_xl_t;
int32_t lsm6ds3_xl_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_xl_t val);
int32_t lsm6ds3_xl_data_rate_get(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_xl_t *val);

typedef enum {
  LSM6DS3_250dps   = 0,
  LSM6DS3_125dps   = 1,
  LSM6DS3_500dps   = 2,
  LSM6DS3_1000dps  = 4,
  LSM6DS3_2000dps  = 6,
} lsm6ds3_fs_g_t;
int32_t lsm6ds3_gy_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_g_t val);
int32_t lsm6ds3_gy_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_g_t *val);

typedef enum {
  LSM6DS3_GY_ODR_OFF    = 0,
  LSM6DS3_GY_ODR_12Hz5  = 1,
  LSM6DS3_GY_ODR_26Hz   = 2,
  LSM6DS3_GY_ODR_52Hz   = 3,
  LSM6DS3_GY_ODR_104Hz  = 4,
  LSM6DS3_GY_ODR_208Hz  = 5,
  LSM6DS3_GY_ODR_416Hz  = 6,
  LSM6DS3_GY_ODR_833Hz  = 7,
  LSM6DS3_GY_ODR_1k66Hz = 8,
} lsm6ds3_odr_g_t;
int32_t lsm6ds3_gy_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_g_t val);
int32_t lsm6ds3_gy_data_rate_get(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_g_t *val);

int32_t lsm6ds3_block_data_update_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_block_data_update_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_XL_HIGH_PERFORMANCE = 0,
  LSM6DS3_XL_NORMAL           = 1,
} lsm6ds3_xl_hm_mode_t;
int32_t lsm6ds3_xl_power_mode_set(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_xl_hm_mode_t val);
int32_t lsm6ds3_xl_power_mode_get(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_xl_hm_mode_t *val);

typedef enum {
  LSM6DS3_STAT_RND_DISABLE = 0,
  LSM6DS3_STAT_RND_ENABLE  = 1,
} lsm6ds3_rnd_stat_t;
int32_t lsm6ds3_rounding_on_status_set(lsm6ds3_ctx_t *ctx,
                                       lsm6ds3_rnd_stat_t val);
int32_t lsm6ds3_rounding_on_status_get(lsm6ds3_ctx_t *ctx,
                                       lsm6ds3_rnd_stat_t *val);

typedef enum {
  LSM6DS3_GY_HIGH_PERFORMANCE = 0,
  LSM6DS3_GY_NORMAL           = 1,
} lsm6ds3_g_hm_mode_t;
int32_t lsm6ds3_gy_power_mode_set(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_g_hm_mode_t val);
int32_t lsm6ds3_gy_power_mode_get(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_g_hm_mode_t *val);

int32_t lsm6ds3_xl_axis_x_data_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_xl_axis_x_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_xl_axis_y_data_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_xl_axis_y_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_xl_axis_z_data_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_xl_axis_z_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_gy_axis_x_data_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_gy_axis_x_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_gy_axis_y_data_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_gy_axis_y_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_gy_axis_z_data_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_gy_axis_z_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef struct {
  lsm6ds3_wake_up_src_t  wake_up_src;
  lsm6ds3_tap_src_t      tap_src;
  lsm6ds3_d6d_src_t      d6d_src;
  lsm6ds3_func_src_t     func_src;
} lsm6ds3_all_src_t;
int32_t lsm6ds3_all_sources_get(lsm6ds3_ctx_t *ctx, lsm6ds3_all_src_t *val);

int32_t lsm6ds3_status_reg_get(lsm6ds3_ctx_t *ctx, lsm6ds3_status_reg_t *val);

int32_t lsm6ds3_xl_flag_data_ready_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_gy_flag_data_ready_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_temp_flag_data_ready_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_timestamp_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_timestamp_rst_set(lsm6ds3_ctx_t *ctx);

int32_t lsm6ds3_timestamp_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_timestamp_get(lsm6ds3_ctx_t *ctx, uint8_t *val);
typedef enum {
  LSM6DS3_LSB_6ms4 = 0,
  LSM6DS3_LSB_25us = 1,
} lsm6ds3_ts_res_t;
int32_t lsm6ds3_timestamp_res_set(lsm6ds3_ctx_t *ctx, lsm6ds3_ts_res_t val);
int32_t lsm6ds3_timestamp_res_get(lsm6ds3_ctx_t *ctx, lsm6ds3_ts_res_t *val);

typedef enum {
  LSM6DS3_ROUND_DISABLE             = 0,
  LSM6DS3_ROUND_XL                  = 1,
  LSM6DS3_ROUND_GY                  = 2,
  LSM6DS3_ROUND_GY_XL               = 3,
  LSM6DS3_ROUND_SH1_TO_SH6          = 4,
  LSM6DS3_ROUND_XL_SH1_TO_SH6       = 5,
  LSM6DS3_ROUND_GY_XL_SH1_TO_SH12   = 6,
  LSM6DS3_ROUND_GY_XL_SH1_TO_SH6    = 7,
} lsm6ds3_rounding_t;
int32_t lsm6ds3_rounding_mode_set(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_rounding_t val);
int32_t lsm6ds3_rounding_mode_get(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_rounding_t *val);

int32_t lsm6ds3_temperature_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_angular_rate_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_acceleration_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_fifo_raw_data_get(lsm6ds3_ctx_t *ctx, uint8_t *buffer, uint8_t len);

int32_t lsm6ds3_number_of_steps_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_mag_calibrated_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

typedef enum {
  LSM6DS3_USER_BANK                                = 0,
  LSM6DS3_EMBEDDED_FUNC_BANK                       = 1,
} lsm6ds3_func_cfg_en_t;
int32_t lsm6ds3_mem_bank_set(lsm6ds3_ctx_t *ctx, lsm6ds3_func_cfg_en_t val);
int32_t lsm6ds3_mem_bank_get(lsm6ds3_ctx_t *ctx, lsm6ds3_func_cfg_en_t *val);

int32_t lsm6ds3_device_id_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_reset_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_reset_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_LSB_AT_LOW_ADD = 0,
  LSM6DS3_MSB_AT_LOW_ADD = 1,
} lsm6ds3_ble_t;
int32_t lsm6ds3_data_format_set(lsm6ds3_ctx_t *ctx, lsm6ds3_ble_t val);
int32_t lsm6ds3_data_format_get(lsm6ds3_ctx_t *ctx, lsm6ds3_ble_t *val);

int32_t lsm6ds3_auto_increment_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_auto_increment_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_boot_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_boot_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_XL_ST_DISABLE  = 0,
  LSM6DS3_XL_ST_POSITIVE = 1,
  LSM6DS3_XL_ST_NEGATIVE = 2,
} lsm6ds3_st_xl_t;
int32_t lsm6ds3_xl_self_test_set(lsm6ds3_ctx_t *ctx, lsm6ds3_st_xl_t val);
int32_t lsm6ds3_xl_self_test_get(lsm6ds3_ctx_t *ctx, lsm6ds3_st_xl_t *val);

typedef enum {
  LSM6DS3_GY_ST_DISABLE  = 0,
  LSM6DS3_GY_ST_POSITIVE = 1,
  LSM6DS3_GY_ST_NEGATIVE = 3,
} lsm6ds3_st_g_t;
int32_t lsm6ds3_gy_self_test_set(lsm6ds3_ctx_t *ctx, lsm6ds3_st_g_t val);
int32_t lsm6ds3_gy_self_test_get(lsm6ds3_ctx_t *ctx, lsm6ds3_st_g_t *val);

int32_t lsm6ds3_filter_settling_mask_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_filter_settling_mask_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_HP_CUT_OFF_8mHz1   = 0,
  LSM6DS3_HP_CUT_OFF_32mHz4  = 1,
  LSM6DS3_HP_CUT_OFF_2Hz07   = 2,
  LSM6DS3_HP_CUT_OFF_16Hz32  = 3,
} lsm6ds3_hpcf_g_t;
int32_t lsm6ds3_gy_hp_bandwidth_set(lsm6ds3_ctx_t *ctx, lsm6ds3_hpcf_g_t val);
int32_t lsm6ds3_gy_hp_bandwidth_get(lsm6ds3_ctx_t *ctx, lsm6ds3_hpcf_g_t *val);

int32_t lsm6ds3_gy_hp_reset_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_gy_hp_reset_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_XL_HP_ODR_DIV_4   = 0,
  LSM6DS3_XL_HP_ODR_DIV_100 = 1,
  LSM6DS3_XL_HP_ODR_DIV_9   = 2,
  LSM6DS3_XL_HP_ODR_DIV_400 = 3,
} lsm6ds3_hp_bw_t;
int32_t lsm6ds3_xl_hp_bandwidth_set(lsm6ds3_ctx_t *ctx, lsm6ds3_hp_bw_t val);
int32_t lsm6ds3_xl_hp_bandwidth_get(lsm6ds3_ctx_t *ctx, lsm6ds3_hp_bw_t *val);

typedef enum {
  LSM6DS3_XL_LP_ODR_DIV_50  = 0,
  LSM6DS3_XL_LP_ODR_DIV_100 = 1,
  LSM6DS3_XL_LP_ODR_DIV_9   = 2,
  LSM6DS3_XL_LP_ODR_DIV_400 = 3,
} lsm6ds3_lp_bw_t;
int32_t lsm6ds3_xl_lp2_bandwidth_set(lsm6ds3_ctx_t *ctx, lsm6ds3_lp_bw_t val);
int32_t lsm6ds3_xl_lp2_bandwidth_get(lsm6ds3_ctx_t *ctx, lsm6ds3_lp_bw_t *val);

typedef enum {
  LSM6DS3_ANTI_ALIASING_400Hz = 0,
  LSM6DS3_ANTI_ALIASING_200Hz = 1,
  LSM6DS3_ANTI_ALIASING_100Hz = 2,
  LSM6DS3_ANTI_ALIASING_50Hz  = 3,
} lsm6ds3_bw_xl_t;
int32_t lsm6ds3_xl_filter_analog_set(lsm6ds3_ctx_t *ctx, lsm6ds3_bw_xl_t val);
int32_t lsm6ds3_xl_filter_analog_get(lsm6ds3_ctx_t *ctx, lsm6ds3_bw_xl_t *val);

typedef enum {
  LSM6DS3_SPI_4_WIRE = 0,
  LSM6DS3_SPI_3_WIRE = 1,
} lsm6ds3_sim_t;
int32_t lsm6ds3_spi_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_sim_t val);
int32_t lsm6ds3_spi_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_sim_t *val);

typedef enum {
  LSM6DS3_I2C_ENABLE  = 0,
  LSM6DS3_I2C_DISABLE = 1,
} lsm6ds3_i2c_dis_t;
int32_t lsm6ds3_i2c_interface_set(lsm6ds3_ctx_t *ctx, lsm6ds3_i2c_dis_t val);
int32_t lsm6ds3_i2c_interface_get(lsm6ds3_ctx_t *ctx, lsm6ds3_i2c_dis_t *val);

typedef struct {
  uint8_t  int1_drdy_xl         : 1;
  uint8_t  int1_drdy_g          : 1;
  uint8_t  int1_boot            : 1;
  uint8_t  int1_fth             : 1;
  uint8_t  int1_fifo_ovr        : 1;
  uint8_t  int1_full_flag       : 1;
  uint8_t  int1_sign_mot        : 1;
  uint8_t  int1_step_detector   : 1;
  uint8_t  int1_timer           : 1;
  uint8_t  int1_tilt            : 1;
  uint8_t  int1_6d              : 1;
  uint8_t  int1_double_tap      : 1;
  uint8_t  int1_ff              : 1;
  uint8_t  int1_wu              : 1;
  uint8_t  int1_single_tap      : 1;
  uint8_t  int1_inact_state     : 1;
  uint8_t  drdy_on_int1         : 1;
} lsm6ds3_int1_route_t;
int32_t lsm6ds3_pin_int1_route_set(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_int1_route_t *val);
int32_t lsm6ds3_pin_int1_route_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_int1_route_t *val);

typedef struct {
  uint8_t int2_drdy_xl          : 1;
  uint8_t int2_drdy_g           : 1;
  uint8_t int2_drdy_temp        : 1;
  uint8_t int2_fth              : 1;
  uint8_t int2_fifo_ovr         : 1;
  uint8_t int2_full_flag        : 1;
  uint8_t int2_step_count_ov    : 1;
  uint8_t int2_step_delta       : 1;
  uint8_t int2_iron             : 1;
  uint8_t int2_tilt             : 1;
  uint8_t int2_6d               : 1;
  uint8_t int2_double_tap       : 1;
  uint8_t int2_ff               : 1;
  uint8_t int2_wu               : 1;
  uint8_t int2_single_tap       : 1;
  uint8_t int2_inact_state      : 1;
  uint8_t start_config          : 1;
} lsm6ds3_int2_route_t;
int32_t lsm6ds3_pin_int2_route_set(lsm6ds3_ctx_t *ctx, lsm6ds3_int2_route_t *val);
int32_t lsm6ds3_pin_int2_route_get(lsm6ds3_ctx_t *ctx, lsm6ds3_int2_route_t *val);

typedef enum {
  LSM6DS3_PUSH_PULL  = 0,
  LSM6DS3_OPEN_DRAIN = 1,
} lsm6ds3_pp_od_t;
int32_t lsm6ds3_pin_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_pp_od_t val);
int32_t lsm6ds3_pin_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_pp_od_t *val);

typedef enum {
  LSM6DS3_ACTIVE_HIGH = 0,
  LSM6DS3_ACTIVE_LOW  = 1,
} lsm6ds3_pin_pol_t;
int32_t lsm6ds3_pin_polarity_set(lsm6ds3_ctx_t *ctx, lsm6ds3_pin_pol_t val);
int32_t lsm6ds3_pin_polarity_get(lsm6ds3_ctx_t *ctx,  lsm6ds3_pin_pol_t *val);

int32_t lsm6ds3_all_on_int1_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_all_on_int1_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_INT_PULSED  = 0,
  LSM6DS3_INT_LATCHED = 1,
} lsm6ds3_lir_t;
int32_t lsm6ds3_int_notification_set(lsm6ds3_ctx_t *ctx, lsm6ds3_lir_t val);
int32_t lsm6ds3_int_notification_get(lsm6ds3_ctx_t *ctx, lsm6ds3_lir_t *val);

int32_t lsm6ds3_wkup_src_get(lsm6ds3_ctx_t *ctx, lsm6ds3_wake_up_src_t *val);

int32_t lsm6ds3_wkup_threshold_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_wkup_threshold_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_wkup_dur_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_wkup_dur_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_gy_sleep_mode_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_gy_sleep_mode_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_act_mode_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_act_mode_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_act_sleep_dur_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_act_sleep_dur_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tap_src_get(lsm6ds3_ctx_t *ctx, lsm6ds3_tap_src_t *val);

int32_t lsm6ds3_tap_detection_on_z_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_tap_detection_on_z_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tap_detection_on_y_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_tap_detection_on_y_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tap_detection_on_x_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_tap_detection_on_x_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tap_threshold_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_tap_threshold_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tap_shock_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_tap_shock_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tap_quiet_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_tap_quiet_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tap_dur_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_tap_dur_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_ONLY_DOUBLE = 1,
  LSM6DS3_SINGLE_DOUBLE = 0,
} lsm6ds3_tap_md_t;
int32_t lsm6ds3_tap_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_tap_md_t val);
int32_t lsm6ds3_tap_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_tap_md_t *val);

typedef enum {
  LSM6DS3_ODR_DIV_2_FEED = 0,
  LSM6DS3_LPF2_FEED = 1,
} lsm6ds3_low_pass_on_6d_t;
int32_t lsm6ds3_6d_feed_data_set(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_low_pass_on_6d_t val);
int32_t lsm6ds3_6d_feed_data_get(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_low_pass_on_6d_t *val);

int32_t lsm6ds3_6d_src_get(lsm6ds3_ctx_t *ctx, lsm6ds3_d6d_src_t *val);

typedef enum {
  LSM6DS3_DEG_80 = 0,
  LSM6DS3_DEG_70 = 1,
  LSM6DS3_DEG_60 = 2,
  LSM6DS3_DEG_50 = 3,
} lsm6ds3_sixd_ths_t;
int32_t lsm6ds3_6d_threshold_set(lsm6ds3_ctx_t *ctx, lsm6ds3_sixd_ths_t val);
int32_t lsm6ds3_6d_threshold_get(lsm6ds3_ctx_t *ctx, lsm6ds3_sixd_ths_t *val);

int32_t lsm6ds3_4d_mode_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_4d_mode_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_156_mg = 0,
  LSM6DS3_219_mg = 1,
  LSM6DS3_250_mg = 2,
  LSM6DS3_312_mg = 3,
  LSM6DS3_344_mg = 4,
  LSM6DS3_406_mg = 5,
  LSM6DS3_469_mg = 6,
  LSM6DS3_500_mg = 7,
} lsm6ds3_ff_ths_t;
int32_t lsm6ds3_ff_threshold_set(lsm6ds3_ctx_t *ctx, lsm6ds3_ff_ths_t val);
int32_t lsm6ds3_ff_threshold_get(lsm6ds3_ctx_t *ctx, lsm6ds3_ff_ths_t *val);

int32_t lsm6ds3_ff_dur_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_ff_dur_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_fifo_watermark_set(lsm6ds3_ctx_t *ctx, uint16_t val);
int32_t lsm6ds3_fifo_watermark_get(lsm6ds3_ctx_t *ctx, uint16_t *val);

typedef enum {
  LSM6DS3_TRG_XL_GY_DRDY   = 0,
  LSM6DS3_TRG_STEP_DETECT  = 1,
} lsm6ds3_tmr_ped_fifo_drdy_t;
int32_t lsm6ds3_fifo_write_trigger_set(lsm6ds3_ctx_t *ctx,
                                       lsm6ds3_tmr_ped_fifo_drdy_t val);
int32_t lsm6ds3_fifo_write_trigger_get(lsm6ds3_ctx_t *ctx,
                                       lsm6ds3_tmr_ped_fifo_drdy_t *val);

int32_t lsm6ds3_fifo_pedo_batch_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_fifo_pedo_batch_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_FIFO_XL_DISABLE = 0,
  LSM6DS3_FIFO_XL_NO_DEC  = 1,
  LSM6DS3_FIFO_XL_DEC_2   = 2,
  LSM6DS3_FIFO_XL_DEC_3   = 3,
  LSM6DS3_FIFO_XL_DEC_4   = 4,
  LSM6DS3_FIFO_XL_DEC_8   = 5,
  LSM6DS3_FIFO_XL_DEC_16  = 6,
  LSM6DS3_FIFO_XL_DEC_32  = 7,
} lsm6ds3_dec_fifo_xl_t;
int32_t lsm6ds3_fifo_xl_batch_set(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_dec_fifo_xl_t val);
int32_t lsm6ds3_fifo_xl_batch_get(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_dec_fifo_xl_t *val);

typedef enum {
  LSM6DS3_FIFO_GY_DISABLE = 0,
  LSM6DS3_FIFO_GY_NO_DEC  = 1,
  LSM6DS3_FIFO_GY_DEC_2   = 2,
  LSM6DS3_FIFO_GY_DEC_3   = 3,
  LSM6DS3_FIFO_GY_DEC_4   = 4,
  LSM6DS3_FIFO_GY_DEC_8   = 5,
  LSM6DS3_FIFO_GY_DEC_16  = 6,
  LSM6DS3_FIFO_GY_DEC_32  = 7,
} lsm6ds3_dec_fifo_gyro_t;
int32_t lsm6ds3_fifo_gy_batch_set(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_dec_fifo_gyro_t val);
int32_t lsm6ds3_fifo_gy_batch_get(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_dec_fifo_gyro_t *val);

typedef enum {
  LSM6DS3_FIFO_DS3_DISABLE = 0,
  LSM6DS3_FIFO_DS3_NO_DEC  = 1,
  LSM6DS3_FIFO_DS3_DEC_2   = 2,
  LSM6DS3_FIFO_DS3_DEC_3   = 3,
  LSM6DS3_FIFO_DS3_DEC_4   = 4,
  LSM6DS3_FIFO_DS3_DEC_8   = 5,
  LSM6DS3_FIFO_DS3_DEC_16  = 6,
  LSM6DS3_FIFO_DS3_DEC_32  = 7,
} lsm6ds3_dec_ds3_fifo_t;
int32_t lsm6ds3_fifo_dataset_3_batch_set(lsm6ds3_ctx_t *ctx,
                                         lsm6ds3_dec_ds3_fifo_t val);
int32_t lsm6ds3_fifo_dataset_3_batch_get(lsm6ds3_ctx_t *ctx,
                                         lsm6ds3_dec_ds3_fifo_t *val);

typedef enum {
  LSM6DS3_FIFO_DS4_DISABLE = 0,
  LSM6DS3_FIFO_DS4_NO_DEC  = 1,
  LSM6DS3_FIFO_DS4_DEC_2   = 2,
  LSM6DS3_FIFO_DS4_DEC_3   = 3,
  LSM6DS3_FIFO_DS4_DEC_4   = 4,
  LSM6DS3_FIFO_DS4_DEC_8   = 5,
  LSM6DS3_FIFO_DS4_DEC_16  = 6,
  LSM6DS3_FIFO_DS4_DEC_32  = 7,
} lsm6ds3_dec_ds4_fifo_t;
int32_t lsm6ds3_fifo_dataset_4_batch_set(lsm6ds3_ctx_t *ctx,
                                         lsm6ds3_dec_ds4_fifo_t val);
int32_t lsm6ds3_fifo_dataset_4_batch_get(lsm6ds3_ctx_t *ctx,
                                         lsm6ds3_dec_ds4_fifo_t *val);

int32_t lsm6ds3_fifo_xl_gy_8bit_format_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_fifo_xl_gy_8bit_format_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_BYPASS_MODE            = 0,
  LSM6DS3_FIFO_MODE              = 1,
  LSM6DS3_STREAM_TO_FIFO_MODE    = 3,
  LSM6DS3_BYPASS_TO_STREAM_MODE  = 4,
  LSM6DS3_STREAM_MODE            = 6,
} lsm6ds3_fifo_md_t;
int32_t lsm6ds3_fifo_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_fifo_md_t val);
int32_t lsm6ds3_fifo_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_fifo_md_t *val);

typedef enum {
  LSM6DS3_FIFO_DISABLE = 0,
  LSM6DS3_FIFO_12Hz5   = 1,
  LSM6DS3_FIFO_26Hz    = 2,
  LSM6DS3_FIFO_52Hz    = 3,
  LSM6DS3_FIFO_104Hz   = 4,
  LSM6DS3_FIFO_208Hz   = 5,
  LSM6DS3_FIFO_416Hz   = 6,
  LSM6DS3_FIFO_833Hz   = 7,
  LSM6DS3_FIFO_1k66Hz  = 8,
  LSM6DS3_FIFO_3k33Hz  = 9,
  LSM6DS3_FIFO_6k66Hz  = 10,
} lsm6ds3_odr_fifo_t;
int32_t lsm6ds3_fifo_data_rate_set(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_odr_fifo_t val);
int32_t lsm6ds3_fifo_data_rate_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_odr_fifo_t *val);

int32_t lsm6ds3_fifo_stop_on_wtm_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_fifo_stop_on_wtm_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_fifo_temp_batch_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_fifo_temp_batch_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_fifo_data_level_get(lsm6ds3_ctx_t *ctx, uint16_t *val);

int32_t lsm6ds3_fifo_full_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_fifo_ovr_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_fifo_wtm_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_fifo_pattern_get(lsm6ds3_ctx_t *ctx, uint16_t *val);

typedef enum {
  LSM6DS3_DEN_DISABLE    = 0,
  LSM6DS3_LEVEL_FIFO     = 6,
  LSM6DS3_LEVEL_LETCHED  = 3,
  LSM6DS3_LEVEL_TRIGGER  = 2,
  LSM6DS3_EDGE_TRIGGER   = 4,
} lsm6ds3_den_mode_t;
int32_t lsm6ds3_den_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_den_mode_t val);
int32_t lsm6ds3_den_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_den_mode_t *val);

int32_t lsm6ds3_pedo_step_reset_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_pedo_step_reset_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_pedo_timestamp_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_pedo_step_detect_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_pedo_sens_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_pedo_sens_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_pedo_threshold_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_pedo_threshold_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_PEDO_AT_2g = 0,
  LSM6DS3_PEDO_AT_4g = 1,
} lsm6ds3_pedo_fs_t;
int32_t lsm6ds3_pedo_full_scale_set(lsm6ds3_ctx_t *ctx,
                                    lsm6ds3_pedo_fs_t val);
int32_t lsm6ds3_pedo_full_scale_get(lsm6ds3_ctx_t *ctx,
                                    lsm6ds3_pedo_fs_t *val);

int32_t lsm6ds3_pedo_debounce_steps_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_pedo_debounce_steps_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_pedo_timeout_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_pedo_timeout_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_motion_sens_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_motion_sens_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_motion_event_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_motion_threshold_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_motion_threshold_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_sc_delta_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_sc_delta_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tilt_event_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_tilt_sens_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_tilt_sens_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_mag_soft_iron_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_mag_soft_iron_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_mag_hard_iron_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_mag_hard_iron_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_mag_soft_iron_end_op_flag_get(lsm6ds3_ctx_t *ctx,
                                              uint8_t *val);

int32_t lsm6ds3_mag_soft_iron_coeff_set(lsm6ds3_ctx_t *ctx, uint8_t *buff);
int32_t lsm6ds3_mag_soft_iron_coeff_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_mag_offset_set(lsm6ds3_ctx_t *ctx, uint8_t *buff);
int32_t lsm6ds3_mag_offset_get(lsm6ds3_ctx_t *ctx, uint8_t *buff);

int32_t lsm6ds3_sh_sync_sens_frame_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_sh_sync_sens_frame_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_sh_master_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_sh_master_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

int32_t lsm6ds3_sh_pass_through_set(lsm6ds3_ctx_t *ctx, uint8_t val);
int32_t lsm6ds3_sh_pass_through_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_EXT_PULL_UP      = 0,
  LSM6DS3_INTERNAL_PULL_UP = 1,
} lsm6ds3_sh_pin_md_t;
int32_t lsm6ds3_sh_pin_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_sh_pin_md_t val);
int32_t lsm6ds3_sh_pin_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_sh_pin_md_t *val);

typedef enum {
  LSM6DS3_XL_GY_DRDY      = 0,
  LSM6DS3_EXT_ON_INT2_PIN = 1,
} lsm6ds3_start_cfg_t;
int32_t lsm6ds3_sh_syncro_mode_set(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_start_cfg_t val);
int32_t lsm6ds3_sh_syncro_mode_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_start_cfg_t *val);

typedef struct {
    lsm6ds3_sensorhub1_reg_t   sh_byte_1;
    lsm6ds3_sensorhub2_reg_t   sh_byte_2;
    lsm6ds3_sensorhub3_reg_t   sh_byte_3;
    lsm6ds3_sensorhub4_reg_t   sh_byte_4;
    lsm6ds3_sensorhub5_reg_t   sh_byte_5;
    lsm6ds3_sensorhub6_reg_t   sh_byte_6;
    lsm6ds3_sensorhub7_reg_t   sh_byte_7;
    lsm6ds3_sensorhub8_reg_t   sh_byte_8;
    lsm6ds3_sensorhub9_reg_t   sh_byte_9;
    lsm6ds3_sensorhub10_reg_t  sh_byte_10;
    lsm6ds3_sensorhub11_reg_t  sh_byte_11;
    lsm6ds3_sensorhub12_reg_t  sh_byte_12;
    lsm6ds3_sensorhub13_reg_t  sh_byte_13;
    lsm6ds3_sensorhub14_reg_t  sh_byte_14;
    lsm6ds3_sensorhub15_reg_t  sh_byte_15;
    lsm6ds3_sensorhub16_reg_t  sh_byte_16;
    lsm6ds3_sensorhub17_reg_t  sh_byte_17;
    lsm6ds3_sensorhub18_reg_t  sh_byte_18;
} lsm6ds3_sh_read_t;
int32_t lsm6ds3_sh_read_data_raw_get(lsm6ds3_ctx_t *ctx,
                                     lsm6ds3_sh_read_t *buff);

typedef enum {
  LSM6DS3_SLV_0        = 0,
  LSM6DS3_SLV_0_1      = 1,
  LSM6DS3_SLV_0_1_2    = 2,
  LSM6DS3_SLV_0_1_2_3  = 3,
} lsm6ds3_aux_sens_on_t;
int32_t lsm6ds3_sh_num_of_dev_connected_set(lsm6ds3_ctx_t *ctx,
                                            lsm6ds3_aux_sens_on_t val);
int32_t lsm6ds3_sh_num_of_dev_connected_get(lsm6ds3_ctx_t *ctx,
                                            lsm6ds3_aux_sens_on_t *val);

typedef struct{
  uint8_t   slv0_add;
  uint8_t   slv0_subadd;
  uint8_t   slv0_data;
} lsm6ds3_sh_cfg_write_t;
int32_t lsm6ds3_sh_cfg_write(lsm6ds3_ctx_t *ctx, lsm6ds3_sh_cfg_write_t *val);

typedef struct{
  uint8_t   slv_add;
  uint8_t   slv_subadd;
  uint8_t   slv_len;
} lsm6ds3_sh_cfg_read_t;
int32_t lsm6ds3_sh_slv0_cfg_read(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_sh_cfg_read_t *val);
int32_t lsm6ds3_sh_slv1_cfg_read(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_sh_cfg_read_t *val);
int32_t lsm6ds3_sh_slv2_cfg_read(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_sh_cfg_read_t *val);
int32_t lsm6ds3_sh_slv3_cfg_read(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_sh_cfg_read_t *val);

int32_t lsm6ds3_sh_end_op_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM6DS3_USE_SLOPE = 0,
  LSM6DS3_USE_HPF   = 1,
} lsm6ds3_slope_fds_t;
int32_t lsm6ds3_xl_hp_path_internal_set(lsm6ds3_ctx_t *ctx,
                                        lsm6ds3_slope_fds_t val);
int32_t lsm6ds3_xl_hp_path_internal_get(lsm6ds3_ctx_t *ctx,
                                        lsm6ds3_slope_fds_t *val);

/**
  *@}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* LSM6DS3_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
