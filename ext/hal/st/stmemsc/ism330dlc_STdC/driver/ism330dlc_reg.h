/*
 ******************************************************************************
 * @file    ism330dlc_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          ism330dlc_reg.c driver.
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
#ifndef ISM330DLC_DRIVER_H
#define ISM330DLC_DRIVER_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup ISM330DLC
  * @{
  *
  */

/** @defgroup ISM330DLC_sensors_common_types
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

/** @addtogroup  LSM9DS1_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*ism330dlc_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*ism330dlc_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  ism330dlc_write_ptr  write_reg;
  ism330dlc_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} ism330dlc_ctx_t;

/**
  * @}
  *
  */

/** @defgroup ISM330DLC_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> D5 if SA0=1 -> D7 **/
#define ISM330DLC_I2C_ADD_L     0xD5U
#define ISM330DLC_I2C_ADD_H     0xD7U

/** Device Identification (Who am I) **/
#define ISM330DLC_ID            0x6AU

/**
  * @}
  *
  */

#define ISM330DLC_FUNC_CFG_ACCESS              0x01U
typedef struct {
  uint8_t not_used_01              : 7;
  uint8_t func_cfg_en              : 1;
} ism330dlc_func_cfg_access_t;

#define ISM330DLC_SENSOR_SYNC_TIME_FRAME       0x04U
typedef struct {
  uint8_t tph                      : 4;
  uint8_t not_used_01              : 4;
} ism330dlc_sensor_sync_time_frame_t;

#define ISM330DLC_SENSOR_SYNC_RES_RATIO        0x05U
typedef struct {
  uint8_t rr                       : 2;
  uint8_t not_used_01              : 6;
} ism330dlc_sensor_sync_res_ratio_t;

#define ISM330DLC_FIFO_CTRL1                   0x06U
typedef struct {
  uint8_t fth                      : 8;  /* + FIFO_CTRL2(fth) */
} ism330dlc_fifo_ctrl1_t;

#define ISM330DLC_FIFO_CTRL2                   0x07U
typedef struct {
  uint8_t fth                      : 3;  /* + FIFO_CTRL1(fth) */
  uint8_t fifo_temp_en             : 1;
  uint8_t not_used_01              : 4;
  uint8_t fifo_timer_en            : 1;
} ism330dlc_fifo_ctrl2_t;

#define ISM330DLC_FIFO_CTRL3                   0x08U
typedef struct {
  uint8_t dec_fifo_xl              : 3;
  uint8_t dec_fifo_gyro            : 3;
  uint8_t not_used_01              : 2;
} ism330dlc_fifo_ctrl3_t;

#define ISM330DLC_FIFO_CTRL4                   0x09U
typedef struct {
  uint8_t dec_ds3_fifo             : 3;
  uint8_t dec_ds4_fifo             : 3;
  uint8_t only_high_data           : 1;
  uint8_t stop_on_fth              : 1;
} ism330dlc_fifo_ctrl4_t;

#define ISM330DLC_FIFO_CTRL5                   0x0AU
typedef struct {
  uint8_t fifo_mode                : 3;
  uint8_t odr_fifo                 : 4;
  uint8_t not_used_01              : 1;
} ism330dlc_fifo_ctrl5_t;

#define ISM330DLC_DRDY_PULSE_CFG               0x0BU
typedef struct {
  uint8_t not_used_01              : 7;
  uint8_t drdy_pulsed              : 1;
} ism330dlc_drdy_pulse_cfg_t;

#define ISM330DLC_INT1_CTRL                    0x0DU
typedef struct {
  uint8_t int1_drdy_xl             : 1;
  uint8_t int1_drdy_g              : 1;
  uint8_t int1_boot                : 1;
  uint8_t int1_fth                 : 1;
  uint8_t int1_fifo_ovr            : 1;
  uint8_t int1_full_flag           : 1;
  uint8_t not_used_01              : 2;  
} ism330dlc_int1_ctrl_t;

#define ISM330DLC_INT2_CTRL                    0x0EU
typedef struct {
  uint8_t int2_drdy_xl             : 1;
  uint8_t int2_drdy_g              : 1;
  uint8_t int2_drdy_temp           : 1;
  uint8_t int2_fth                 : 1;
  uint8_t int2_fifo_ovr            : 1;
  uint8_t int2_full_flag           : 1;
  uint8_t not_used_01              : 2;
} ism330dlc_int2_ctrl_t;

#define ISM330DLC_WHO_AM_I                     0x0FU
#define ISM330DLC_CTRL1_XL                     0x10U
typedef struct {
  uint8_t bw0_xl                   : 1;
  uint8_t lpf1_bw_sel              : 1;
  uint8_t fs_xl                    : 2;
  uint8_t odr_xl                   : 4;
} ism330dlc_ctrl1_xl_t;

#define ISM330DLC_CTRL2_G                      0x11U
typedef struct {
  uint8_t not_used_01              : 1;
  uint8_t fs_g                     : 3;  /* fs_g + fs_125 */
  uint8_t odr_g                    : 4;
} ism330dlc_ctrl2_g_t;

#define ISM330DLC_CTRL3_C                      0x12U
typedef struct {
  uint8_t sw_reset                 : 1;
  uint8_t ble                      : 1;
  uint8_t if_inc                   : 1;
  uint8_t sim                      : 1;
  uint8_t pp_od                    : 1;
  uint8_t h_lactive                : 1;
  uint8_t bdu                      : 1;
  uint8_t boot                     : 1;
} ism330dlc_ctrl3_c_t;

#define ISM330DLC_CTRL4_C                      0x13U
typedef struct {
  uint8_t not_used_01              : 1;
  uint8_t lpf1_sel_g               : 1;
  uint8_t i2c_disable              : 1;
  uint8_t drdy_mask                : 1;
  uint8_t den_drdy_int1            : 1;
  uint8_t int2_on_int1             : 1;
  uint8_t sleep                    : 1;
  uint8_t den_xl_en                : 1;
} ism330dlc_ctrl4_c_t;

#define ISM330DLC_CTRL5_C                      0x14U
typedef struct {
  uint8_t st_xl                    : 2;
  uint8_t st_g                     : 2;
  uint8_t den_lh                   : 1;
  uint8_t rounding                 : 3;
} ism330dlc_ctrl5_c_t;

#define ISM330DLC_CTRL6_C                      0x15U
typedef struct {
  uint8_t ftype                    : 2;
  uint8_t not_used_01              : 1;
  uint8_t usr_off_w                : 1;
  uint8_t xl_hm_mode               : 1;
  uint8_t den_mode                 : 3;  /* trig_en + lvl_en + lvl2_en */
} ism330dlc_ctrl6_c_t;

#define ISM330DLC_CTRL7_G                      0x16U
typedef struct {
  uint8_t not_used_01              : 2;
  uint8_t rounding_status          : 1;
  uint8_t not_used_02              : 1;
  uint8_t hpm_g                    : 2;
  uint8_t hp_en_g                  : 1;
  uint8_t g_hm_mode                : 1;
} ism330dlc_ctrl7_g_t;

#define ISM330DLC_CTRL8_XL                     0x17U
typedef struct {
  uint8_t low_pass_on_6d           : 1;
  uint8_t not_used_01              : 1;
  uint8_t hp_slope_xl_en           : 1;
  uint8_t input_composite          : 1;
  uint8_t hp_ref_mode              : 1;
  uint8_t hpcf_xl                  : 2;
  uint8_t lpf2_xl_en               : 1;
} ism330dlc_ctrl8_xl_t;

#define ISM330DLC_CTRL9_XL                     0x18U
typedef struct {
  uint8_t not_used_01              : 2;
  uint8_t soft_en                  : 1;
  uint8_t not_used_02              : 1;
  uint8_t den_xl_g                 : 1;
  uint8_t den_z                    : 1;
  uint8_t den_y                    : 1;
  uint8_t den_x                    : 1;
} ism330dlc_ctrl9_xl_t;

#define ISM330DLC_CTRL10_C                     0x19U
typedef struct {
  uint8_t not_used_01              : 2;
  uint8_t func_en                  : 1;
  uint8_t tilt_en                  : 1;
  uint8_t not_used_02              : 1;
  uint8_t timer_en                 : 1;
  uint8_t not_used_03              : 2;
} ism330dlc_ctrl10_c_t;

#define ISM330DLC_MASTER_CONFIG                0x1AU
typedef struct {
  uint8_t master_on                : 1;
  uint8_t iron_en                  : 1;
  uint8_t pass_through_mode        : 1;
  uint8_t pull_up_en               : 1;
  uint8_t start_config             : 1;
  uint8_t not_used_01              : 1;
  uint8_t  data_valid_sel_fifo     : 1;
  uint8_t drdy_on_int1             : 1;
} ism330dlc_master_config_t;

#define ISM330DLC_WAKE_UP_SRC                  0x1BU
typedef struct {
  uint8_t z_wu                     : 1;
  uint8_t y_wu                     : 1;
  uint8_t x_wu                     : 1;
  uint8_t wu_ia                    : 1;
  uint8_t sleep_state_ia           : 1;
  uint8_t ff_ia                    : 1;
  uint8_t not_used_01              : 2;
} ism330dlc_wake_up_src_t;

#define ISM330DLC_TAP_SRC                      0x1CU
typedef struct {
  uint8_t z_tap                    : 1;
  uint8_t y_tap                    : 1;
  uint8_t x_tap                    : 1;
  uint8_t tap_sign                 : 1;
  uint8_t double_tap               : 1;
  uint8_t single_tap               : 1;
  uint8_t tap_ia                   : 1;
  uint8_t not_used_01              : 1;
} ism330dlc_tap_src_t;

#define ISM330DLC_D6D_SRC                      0x1DU
typedef struct {
  uint8_t xl                       : 1;
  uint8_t xh                       : 1;
  uint8_t yl                       : 1;
  uint8_t yh                       : 1;
  uint8_t zl                       : 1;
  uint8_t zh                       : 1;
  uint8_t d6d_ia                   : 1;
  uint8_t den_drdy                 : 1;
} ism330dlc_d6d_src_t;

#define ISM330DLC_STATUS_REG                   0x1EU
typedef struct {
  uint8_t xlda                     : 1;
  uint8_t gda                      : 1;
  uint8_t tda                      : 1;
  uint8_t not_used_01              : 5;
} ism330dlc_status_reg_t;

#define ISM330DLC_STATUS_SPIAUX                0x1EU
typedef struct {
  uint8_t xlda                     : 1;
  uint8_t gda                      : 1;
  uint8_t gyro_settling            : 1;
  uint8_t not_used_01              : 5;
} ism330dlc_status_spiaux_t;

#define ISM330DLC_OUT_TEMP_L                   0x20U
#define ISM330DLC_OUT_TEMP_H                   0x21U
#define ISM330DLC_OUTX_L_G                     0x22U
#define ISM330DLC_OUTX_H_G                     0x23U
#define ISM330DLC_OUTY_L_G                     0x24U
#define ISM330DLC_OUTY_H_G                     0x25U
#define ISM330DLC_OUTZ_L_G                     0x26U
#define ISM330DLC_OUTZ_H_G                     0x27U
#define ISM330DLC_OUTX_L_XL                    0x28U
#define ISM330DLC_OUTX_H_XL                    0x29U
#define ISM330DLC_OUTY_L_XL                    0x2AU
#define ISM330DLC_OUTY_H_XL                    0x2BU
#define ISM330DLC_OUTZ_L_XL                    0x2CU
#define ISM330DLC_OUTZ_H_XL                    0x2DU
#define ISM330DLC_SENSORHUB1_REG               0x2EU
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub1_reg_t;

#define ISM330DLC_SENSORHUB2_REG               0x2FU
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub2_reg_t;

#define ISM330DLC_SENSORHUB3_REG               0x30U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub3_reg_t;

#define ISM330DLC_SENSORHUB4_REG               0x31U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub4_reg_t;

#define ISM330DLC_SENSORHUB5_REG               0x32U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub5_reg_t;

#define ISM330DLC_SENSORHUB6_REG               0x33U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub6_reg_t;

#define ISM330DLC_SENSORHUB7_REG               0x34U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub7_reg_t;

#define ISM330DLC_SENSORHUB8_REG               0x35U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub8_reg_t;

#define ISM330DLC_SENSORHUB9_REG               0x36U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub9_reg_t;

#define ISM330DLC_SENSORHUB10_REG              0x37U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub10_reg_t;

#define ISM330DLC_SENSORHUB11_REG              0x38U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub11_reg_t;

#define ISM330DLC_SENSORHUB12_REG              0x39U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub12_reg_t;

#define ISM330DLC_FIFO_STATUS1                 0x3AU
typedef struct {
  uint8_t diff_fifo                : 8;  /* + FIFO_STATUS2(diff_fifo) */
} ism330dlc_fifo_status1_t;

#define ISM330DLC_FIFO_STATUS2                 0x3BU
typedef struct {
  uint8_t diff_fifo                : 3;  /* + FIFO_STATUS1(diff_fifo) */
  uint8_t not_used_01              : 1;
  uint8_t fifo_empty               : 1;
  uint8_t fifo_full_smart          : 1;
  uint8_t over_run                 : 1;
  uint8_t waterm                   : 1;
} ism330dlc_fifo_status2_t;

#define ISM330DLC_FIFO_STATUS3                 0x3CU
typedef struct {
  uint8_t fifo_pattern             : 8;  /* + FIFO_STATUS4(fifo_pattern) */
} ism330dlc_fifo_status3_t;

#define ISM330DLC_FIFO_STATUS4                 0x3DU
typedef struct {
  uint8_t fifo_pattern             : 2;  /* + FIFO_STATUS3(fifo_pattern) */
  uint8_t not_used_01              : 6;
} ism330dlc_fifo_status4_t;

#define ISM330DLC_FIFO_DATA_OUT_L              0x3E
#define ISM330DLC_FIFO_DATA_OUT_H              0x3F
#define ISM330DLC_TIMESTAMP0_REG               0x40
#define ISM330DLC_TIMESTAMP1_REG               0x41
#define ISM330DLC_TIMESTAMP2_REG               0x42

#define ISM330DLC_SENSORHUB13_REG              0x4DU
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub13_reg_t;

#define ISM330DLC_SENSORHUB14_REG              0x4EU
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub14_reg_t;

#define ISM330DLC_SENSORHUB15_REG              0x4FU
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub15_reg_t;

#define ISM330DLC_SENSORHUB16_REG              0x50U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub16_reg_t;

#define ISM330DLC_SENSORHUB17_REG              0x51U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub17_reg_t;

#define ISM330DLC_SENSORHUB18_REG              0x52U
typedef struct {
  uint8_t bit0                     : 1;
  uint8_t bit1                     : 1;
  uint8_t bit2                     : 1;
  uint8_t bit3                     : 1;
  uint8_t bit4                     : 1;
  uint8_t bit5                     : 1;
  uint8_t bit6                     : 1;
  uint8_t bit7                     : 1;
} ism330dlc_sensorhub18_reg_t;

#define ISM330DLC_FUNC_SRC1                    0x53U
typedef struct {
  uint8_t sensorhub_end_op         : 1;
  uint8_t si_end_op                : 1;
  uint8_t hi_fail                  : 1;
  uint8_t not_used_01              : 2;
  uint8_t tilt_ia                  : 1;
  uint8_t not_used_02              : 2;
} ism330dlc_func_src1_t;

#define ISM330DLC_FUNC_SRC2                    0x54U
typedef struct {
  uint8_t not_used_01              : 3;
  uint8_t slave0_nack              : 1;
  uint8_t slave1_nack              : 1;
  uint8_t slave2_nack              : 1;
  uint8_t slave3_nack              : 1;
  uint8_t not_used_02              : 1;
} ism330dlc_func_src2_t;

#define ISM330DLC_TAP_CFG                      0x58U
typedef struct {
  uint8_t lir                      : 1;
  uint8_t tap_z_en                 : 1;
  uint8_t tap_y_en                 : 1;
  uint8_t tap_x_en                 : 1;
  uint8_t slope_fds                : 1;
  uint8_t inact_en                 : 2;
  uint8_t interrupts_enable        : 1;
} ism330dlc_tap_cfg_t;

#define ISM330DLC_TAP_THS_6D                   0x59U
typedef struct {
  uint8_t tap_ths                  : 5;
  uint8_t sixd_ths                 : 2;
  uint8_t d4d_en                   : 1;
} ism330dlc_tap_ths_6d_t;

#define ISM330DLC_INT_DUR2                     0x5AU
typedef struct {
  uint8_t shock                    : 2;
  uint8_t quiet                    : 2;
  uint8_t dur                      : 4;
} ism330dlc_int_dur2_t;

#define ISM330DLC_WAKE_UP_THS                  0x5BU
typedef struct {
  uint8_t wk_ths                   : 6;
  uint8_t not_used_01              : 1;
  uint8_t single_double_tap        : 1;
} ism330dlc_wake_up_ths_t;

#define ISM330DLC_WAKE_UP_DUR                  0x5CU
typedef struct {
  uint8_t sleep_dur                : 4;
  uint8_t timer_hr                 : 1;
  uint8_t wake_dur                 : 2;
  uint8_t ff_dur                   : 1;
} ism330dlc_wake_up_dur_t;

#define ISM330DLC_FREE_FALL                    0x5DU
typedef struct {
  uint8_t ff_ths                   : 3;
  uint8_t ff_dur                   : 5;
} ism330dlc_free_fall_t;

#define ISM330DLC_MD1_CFG                      0x5EU
typedef struct {
  uint8_t int1_timer               : 1;
  uint8_t int1_tilt                : 1;
  uint8_t int1_6d                  : 1;
  uint8_t int1_double_tap          : 1;
  uint8_t int1_ff                  : 1;
  uint8_t int1_wu                  : 1;
  uint8_t int1_single_tap          : 1;
  uint8_t int1_inact_state         : 1;
} ism330dlc_md1_cfg_t;

#define ISM330DLC_MD2_CFG                      0x5FU
typedef struct {
  uint8_t int2_iron                : 1;
  uint8_t int2_tilt                : 1;
  uint8_t int2_6d                  : 1;
  uint8_t int2_double_tap          : 1;
  uint8_t int2_ff                  : 1;
  uint8_t int2_wu                  : 1;
  uint8_t int2_single_tap          : 1;
  uint8_t int2_inact_state         : 1;
} ism330dlc_md2_cfg_t;

#define ISM330DLC_MASTER_CMD_CODE              0x60U
typedef struct {
  uint8_t master_cmd_code          : 8;
} ism330dlc_master_cmd_code_t;

#define ISM330DLC_SENS_SYNC_SPI_ERROR_CODE     0x61U
typedef struct {
  uint8_t error_code               : 8;
} ism330dlc_sens_sync_spi_error_code_t;

#define ISM330DLC_OUT_MAG_RAW_X_L              0x66U
#define ISM330DLC_OUT_MAG_RAW_X_H              0x67U
#define ISM330DLC_OUT_MAG_RAW_Y_L              0x68U
#define ISM330DLC_OUT_MAG_RAW_Y_H              0x69U
#define ISM330DLC_OUT_MAG_RAW_Z_L              0x6AU
#define ISM330DLC_OUT_MAG_RAW_Z_H              0x6BU
#define ISM330DLC_INT_OIS                      0x6FU
typedef struct {
  uint8_t not_used_01              : 6;
  uint8_t lvl2_ois                 : 1;
  uint8_t int2_drdy_ois            : 1;
} ism330dlc_int_ois_t;

#define ISM330DLC_CTRL1_OIS                    0x70U
typedef struct {
  uint8_t ois_en_spi2              : 1;
  uint8_t fs_g_ois                 : 3;  /* fs_g_ois + fs_125_ois */
  uint8_t mode4_en                 : 1;
  uint8_t sim_ois                  : 1;
  uint8_t lvl1_ois                 : 1;
  uint8_t ble_ois                  : 1;
} ism330dlc_ctrl1_ois_t;

#define ISM330DLC_CTRL2_OIS                    0x71U
typedef struct {
  uint8_t hp_en_ois                : 1;
  uint8_t ftype_ois                : 2;
  uint8_t not_used_01              : 1;
  uint8_t hpm_ois                  : 2;
  uint8_t not_used_02              : 2;
} ism330dlc_ctrl2_ois_t;

#define ISM330DLC_CTRL3_OIS                    0x72U
typedef struct {
  uint8_t st_ois_clampdis          : 1;
  uint8_t st_ois                   : 2;
  uint8_t filter_xl_conf_ois       : 2;
  uint8_t fs_xl_ois                : 2;
  uint8_t den_lh_ois               : 1;
} ism330dlc_ctrl3_ois_t;

#define ISM330DLC_X_OFS_USR                    0x73U
#define ISM330DLC_Y_OFS_USR                    0x74U
#define ISM330DLC_Z_OFS_USR                    0x75U
#define ISM330DLC_SLV0_ADD                     0x02U
typedef struct {
  uint8_t rw_0                     : 1;
  uint8_t slave0_add               : 7;
} ism330dlc_slv0_add_t;

#define ISM330DLC_SLV0_SUBADD                  0x03U
typedef struct {
  uint8_t slave0_reg               : 8;
} ism330dlc_slv0_subadd_t;

#define ISM330DLC_SLAVE0_CONFIG                0x04U
typedef struct {
  uint8_t slave0_numop             : 3;
  uint8_t src_mode                 : 1;
  uint8_t aux_sens_on              : 2;
  uint8_t slave0_rate              : 2;
} ism330dlc_slave0_config_t;

#define ISM330DLC_SLV1_ADD                     0x05U
typedef struct {
  uint8_t r_1                      : 1;
  uint8_t slave1_add               : 7;
} ism330dlc_slv1_add_t;

#define ISM330DLC_SLV1_SUBADD                  0x06U
typedef struct {
  uint8_t slave1_reg               : 8;
} ism330dlc_slv1_subadd_t;

#define ISM330DLC_SLAVE1_CONFIG                0x07U
typedef struct {
  uint8_t slave1_numop             : 3;
  uint8_t not_used_01              : 2;
  uint8_t write_once               : 1;
  uint8_t slave1_rate              : 2;
} ism330dlc_slave1_config_t;

#define ISM330DLC_SLV2_ADD                     0x08U
typedef struct {
  uint8_t r_2                      : 1;
  uint8_t slave2_add               : 7;
} ism330dlc_slv2_add_t;

#define ISM330DLC_SLV2_SUBADD                  0x09U
typedef struct {
  uint8_t slave2_reg               : 8;
} ism330dlc_slv2_subadd_t;

#define ISM330DLC_SLAVE2_CONFIG                0x0AU
typedef struct {
  uint8_t slave2_numop             : 3;
  uint8_t not_used_01              : 3;
  uint8_t slave2_rate              : 2;
} ism330dlc_slave2_config_t;

#define ISM330DLC_SLV3_ADD                     0x0BU
typedef struct {
  uint8_t r_3                      : 1;
  uint8_t slave3_add               : 7;
} ism330dlc_slv3_add_t;

#define ISM330DLC_SLV3_SUBADD                  0x0CU
typedef struct {
  uint8_t slave3_reg               : 8;
} ism330dlc_slv3_subadd_t;

#define ISM330DLC_SLAVE3_CONFIG                0x0DU
typedef struct {
  uint8_t slave3_numop             : 3;
  uint8_t not_used_01              : 3;
  uint8_t slave3_rate              : 2;
} ism330dlc_slave3_config_t;

#define ISM330DLC_DATAWRITE_SRC_MODE_SUB_SLV0  0x0EU
typedef struct {
  uint8_t slave_dataw              : 8;
} ism330dlc_datawrite_src_mode_sub_slv0_t;

#define ISM330DLC_MAG_SI_XX                    0x24U
#define ISM330DLC_MAG_SI_XY                    0x25U
#define ISM330DLC_MAG_SI_XZ                    0x26U
#define ISM330DLC_MAG_SI_YX                    0x27U
#define ISM330DLC_MAG_SI_YY                    0x28U
#define ISM330DLC_MAG_SI_YZ                    0x29U
#define ISM330DLC_MAG_SI_ZX                    0x2AU
#define ISM330DLC_MAG_SI_ZY                    0x2BU
#define ISM330DLC_MAG_SI_ZZ                    0x2CU
#define ISM330DLC_MAG_OFFX_L                   0x2DU
#define ISM330DLC_MAG_OFFX_H                   0x2EU
#define ISM330DLC_MAG_OFFY_L                   0x2FU
#define ISM330DLC_MAG_OFFY_H                   0x30U
#define ISM330DLC_MAG_OFFZ_L                   0x31U
#define ISM330DLC_MAG_OFFZ_H                   0x32U

/**
  * @defgroup ISM330DLC_Register_Union
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
  ism330dlc_func_cfg_access_t                  func_cfg_access;
  ism330dlc_sensor_sync_time_frame_t           sensor_sync_time_frame;
  ism330dlc_sensor_sync_res_ratio_t            sensor_sync_res_ratio;
  ism330dlc_fifo_ctrl1_t                       fifo_ctrl1;
  ism330dlc_fifo_ctrl2_t                       fifo_ctrl2;
  ism330dlc_fifo_ctrl3_t                       fifo_ctrl3;
  ism330dlc_fifo_ctrl4_t                       fifo_ctrl4;
  ism330dlc_fifo_ctrl5_t                       fifo_ctrl5;
  ism330dlc_drdy_pulse_cfg_t                   drdy_pulse_cfg;
  ism330dlc_int1_ctrl_t                        int1_ctrl;
  ism330dlc_int2_ctrl_t                        int2_ctrl;
  ism330dlc_ctrl1_xl_t                         ctrl1_xl;
  ism330dlc_ctrl2_g_t                          ctrl2_g;
  ism330dlc_ctrl3_c_t                          ctrl3_c;
  ism330dlc_ctrl4_c_t                          ctrl4_c;
  ism330dlc_ctrl5_c_t                          ctrl5_c;
  ism330dlc_ctrl6_c_t                          ctrl6_c;
  ism330dlc_ctrl7_g_t                          ctrl7_g;
  ism330dlc_ctrl8_xl_t                         ctrl8_xl;
  ism330dlc_ctrl9_xl_t                         ctrl9_xl;
  ism330dlc_ctrl10_c_t                         ctrl10_c;
  ism330dlc_master_config_t                    master_config;
  ism330dlc_wake_up_src_t                      wake_up_src;
  ism330dlc_tap_src_t                          tap_src;
  ism330dlc_d6d_src_t                          d6d_src;
  ism330dlc_status_reg_t                       status_reg;
  ism330dlc_status_spiaux_t                    status_spiaux;
  ism330dlc_sensorhub1_reg_t                   sensorhub1_reg;
  ism330dlc_sensorhub2_reg_t                   sensorhub2_reg;
  ism330dlc_sensorhub3_reg_t                   sensorhub3_reg;
  ism330dlc_sensorhub4_reg_t                   sensorhub4_reg;
  ism330dlc_sensorhub5_reg_t                   sensorhub5_reg;
  ism330dlc_sensorhub6_reg_t                   sensorhub6_reg;
  ism330dlc_sensorhub7_reg_t                   sensorhub7_reg;
  ism330dlc_sensorhub8_reg_t                   sensorhub8_reg;
  ism330dlc_sensorhub9_reg_t                   sensorhub9_reg;
  ism330dlc_sensorhub10_reg_t                  sensorhub10_reg;
  ism330dlc_sensorhub11_reg_t                  sensorhub11_reg;
  ism330dlc_sensorhub12_reg_t                  sensorhub12_reg;
  ism330dlc_fifo_status1_t                     fifo_status1;
  ism330dlc_fifo_status2_t                     fifo_status2;
  ism330dlc_fifo_status3_t                     fifo_status3;
  ism330dlc_fifo_status4_t                     fifo_status4;
  ism330dlc_sensorhub13_reg_t                  sensorhub13_reg;
  ism330dlc_sensorhub14_reg_t                  sensorhub14_reg;
  ism330dlc_sensorhub15_reg_t                  sensorhub15_reg;
  ism330dlc_sensorhub16_reg_t                  sensorhub16_reg;
  ism330dlc_sensorhub17_reg_t                  sensorhub17_reg;
  ism330dlc_sensorhub18_reg_t                  sensorhub18_reg;
  ism330dlc_func_src1_t                        func_src1;
  ism330dlc_func_src2_t                        func_src2;
  ism330dlc_tap_cfg_t                          tap_cfg;
  ism330dlc_tap_ths_6d_t                       tap_ths_6d;
  ism330dlc_int_dur2_t                         int_dur2;
  ism330dlc_wake_up_ths_t                      wake_up_ths;
  ism330dlc_wake_up_dur_t                      wake_up_dur;
  ism330dlc_free_fall_t                        free_fall;
  ism330dlc_md1_cfg_t                          md1_cfg;
  ism330dlc_md2_cfg_t                          md2_cfg;
  ism330dlc_master_cmd_code_t                  master_cmd_code;
  ism330dlc_sens_sync_spi_error_code_t         sens_sync_spi_error_code;
  ism330dlc_int_ois_t                          int_ois;
  ism330dlc_ctrl1_ois_t                        ctrl1_ois;
  ism330dlc_ctrl2_ois_t                        ctrl2_ois;
  ism330dlc_ctrl3_ois_t                        ctrl3_ois;
  ism330dlc_slv0_add_t                         slv0_add;
  ism330dlc_slv0_subadd_t                      slv0_subadd;
  ism330dlc_slave0_config_t                    slave0_config;
  ism330dlc_slv1_add_t                         slv1_add;
  ism330dlc_slv1_subadd_t                      slv1_subadd;
  ism330dlc_slave1_config_t                    slave1_config;
  ism330dlc_slv2_add_t                         slv2_add;
  ism330dlc_slv2_subadd_t                      slv2_subadd;
  ism330dlc_slave2_config_t                    slave2_config;
  ism330dlc_slv3_add_t                         slv3_add;
  ism330dlc_slv3_subadd_t                      slv3_subadd;
  ism330dlc_slave3_config_t                    slave3_config;
  ism330dlc_datawrite_src_mode_sub_slv0_t      datawrite_src_mode_sub_slv0;
  bitwise_t                                    bitwise;
  uint8_t                                      byte;
} ism330dlc_reg_t;

/**
  * @}
  *
  */

int32_t ism330dlc_read_reg(ism330dlc_ctx_t *ctx, uint8_t reg, uint8_t* data,
                         uint16_t len);
int32_t ism330dlc_write_reg(ism330dlc_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);

extern float_t ism330dlc_from_fs2g_to_mg(int16_t lsb);
extern float_t ism330dlc_from_fs4g_to_mg(int16_t lsb);
extern float_t ism330dlc_from_fs8g_to_mg(int16_t lsb);
extern float_t ism330dlc_from_fs16g_to_mg(int16_t lsb);

extern float_t ism330dlc_from_fs125dps_to_mdps(int16_t lsb);
extern float_t ism330dlc_from_fs250dps_to_mdps(int16_t lsb);
extern float_t ism330dlc_from_fs500dps_to_mdps(int16_t lsb);
extern float_t ism330dlc_from_fs1000dps_to_mdps(int16_t lsb);
extern float_t ism330dlc_from_fs2000dps_to_mdps(int16_t lsb);

extern float_t ism330dlc_from_lsb_to_celsius(int16_t lsb);

typedef enum {
  ISM330DLC_2g       = 0,
  ISM330DLC_16g      = 1,
  ISM330DLC_4g       = 2,
  ISM330DLC_8g       = 3,
  ISM330DLC_XL_FS_ND = 4,  /* ERROR CODE */
} ism330dlc_fs_xl_t;
int32_t ism330dlc_xl_full_scale_set(ism330dlc_ctx_t *ctx, ism330dlc_fs_xl_t val);
int32_t ism330dlc_xl_full_scale_get(ism330dlc_ctx_t *ctx, ism330dlc_fs_xl_t *val);

typedef enum {
  ISM330DLC_XL_ODR_OFF      =  0,
  ISM330DLC_XL_ODR_12Hz5    =  1,
  ISM330DLC_XL_ODR_26Hz     =  2,
  ISM330DLC_XL_ODR_52Hz     =  3,
  ISM330DLC_XL_ODR_104Hz    =  4,
  ISM330DLC_XL_ODR_208Hz    =  5,
  ISM330DLC_XL_ODR_416Hz    =  6,
  ISM330DLC_XL_ODR_833Hz    =  7,
  ISM330DLC_XL_ODR_1k66Hz   =  8,
  ISM330DLC_XL_ODR_3k33Hz   =  9,
  ISM330DLC_XL_ODR_6k66Hz   = 10,
  ISM330DLC_XL_ODR_1Hz6     = 11,
} ism330dlc_odr_xl_t;
int32_t ism330dlc_xl_data_rate_set(ism330dlc_ctx_t *ctx, ism330dlc_odr_xl_t val);
int32_t ism330dlc_xl_data_rate_get(ism330dlc_ctx_t *ctx, ism330dlc_odr_xl_t *val);

typedef enum {
  ISM330DLC_250dps   = 0,
  ISM330DLC_125dps   = 1,
  ISM330DLC_500dps   = 2,
  ISM330DLC_1000dps  = 4,
  ISM330DLC_2000dps  = 6,
} ism330dlc_fs_g_t;
int32_t ism330dlc_gy_full_scale_set(ism330dlc_ctx_t *ctx, ism330dlc_fs_g_t val);
int32_t ism330dlc_gy_full_scale_get(ism330dlc_ctx_t *ctx, ism330dlc_fs_g_t *val);

typedef enum {
  ISM330DLC_GY_ODR_OFF    =  0,
  ISM330DLC_GY_ODR_12Hz5  =  1,
  ISM330DLC_GY_ODR_26Hz   =  2,
  ISM330DLC_GY_ODR_52Hz   =  3,
  ISM330DLC_GY_ODR_104Hz  =  4,
  ISM330DLC_GY_ODR_208Hz  =  5,
  ISM330DLC_GY_ODR_416Hz  =  6,
  ISM330DLC_GY_ODR_833Hz  =  7,
  ISM330DLC_GY_ODR_1k66Hz =  8,
  ISM330DLC_GY_ODR_3k33Hz =  9,
  ISM330DLC_GY_ODR_6k66Hz = 10,
} ism330dlc_odr_g_t;
int32_t ism330dlc_gy_data_rate_set(ism330dlc_ctx_t *ctx, ism330dlc_odr_g_t val);
int32_t ism330dlc_gy_data_rate_get(ism330dlc_ctx_t *ctx, ism330dlc_odr_g_t *val);

int32_t ism330dlc_block_data_update_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_block_data_update_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_LSb_1mg  = 0,
  ISM330DLC_LSb_16mg = 1,
} ism330dlc_usr_off_w_t;
int32_t ism330dlc_xl_offset_weight_set(ism330dlc_ctx_t *ctx,
                                     ism330dlc_usr_off_w_t val);
int32_t ism330dlc_xl_offset_weight_get(ism330dlc_ctx_t *ctx,
                                     ism330dlc_usr_off_w_t *val);

typedef enum {
  ISM330DLC_XL_HIGH_PERFORMANCE  = 0,
  ISM330DLC_XL_NORMAL            = 1,
} ism330dlc_xl_hm_mode_t;
int32_t ism330dlc_xl_power_mode_set(ism330dlc_ctx_t *ctx,
                                  ism330dlc_xl_hm_mode_t val);
int32_t ism330dlc_xl_power_mode_get(ism330dlc_ctx_t *ctx,
                                  ism330dlc_xl_hm_mode_t *val);

typedef enum {
  ISM330DLC_STAT_RND_DISABLE  = 0,
  ISM330DLC_STAT_RND_ENABLE   = 1,
} ism330dlc_rounding_status_t;
int32_t ism330dlc_rounding_on_status_set(ism330dlc_ctx_t *ctx,
                                       ism330dlc_rounding_status_t val);
int32_t ism330dlc_rounding_on_status_get(ism330dlc_ctx_t *ctx,
                                       ism330dlc_rounding_status_t *val);

typedef enum {
  ISM330DLC_GY_HIGH_PERFORMANCE  = 0,
  ISM330DLC_GY_NORMAL            = 1,
} ism330dlc_g_hm_mode_t;
int32_t ism330dlc_gy_power_mode_set(ism330dlc_ctx_t *ctx,
                                  ism330dlc_g_hm_mode_t val);
int32_t ism330dlc_gy_power_mode_get(ism330dlc_ctx_t *ctx,
                                  ism330dlc_g_hm_mode_t *val);

typedef struct {
  ism330dlc_wake_up_src_t        wake_up_src;
  ism330dlc_tap_src_t            tap_src;
  ism330dlc_d6d_src_t            d6d_src;
  ism330dlc_status_reg_t         status_reg;
  ism330dlc_func_src1_t          func_src1;
  ism330dlc_func_src2_t          func_src2;
} ism330dlc_all_sources_t;
int32_t ism330dlc_all_sources_get(ism330dlc_ctx_t *ctx,
                                ism330dlc_all_sources_t *val);

int32_t ism330dlc_status_reg_get(ism330dlc_ctx_t *ctx, ism330dlc_status_reg_t *val);

int32_t ism330dlc_xl_flag_data_ready_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_gy_flag_data_ready_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_temp_flag_data_ready_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_xl_usr_offset_set(ism330dlc_ctx_t *ctx, uint8_t *buff);
int32_t ism330dlc_xl_usr_offset_get(ism330dlc_ctx_t *ctx, uint8_t *buff);
int32_t ism330dlc_timestamp_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_timestamp_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_LSB_6ms4 = 0,
  ISM330DLC_LSB_25us = 1,
} ism330dlc_timer_hr_t;
int32_t ism330dlc_timestamp_res_set(ism330dlc_ctx_t *ctx, ism330dlc_timer_hr_t val);
int32_t ism330dlc_timestamp_res_get(ism330dlc_ctx_t *ctx, ism330dlc_timer_hr_t *val);

typedef enum {
  ISM330DLC_ROUND_DISABLE            = 0,
  ISM330DLC_ROUND_XL                 = 1,
  ISM330DLC_ROUND_GY                 = 2,
  ISM330DLC_ROUND_GY_XL              = 3,
  ISM330DLC_ROUND_SH1_TO_SH6         = 4,
  ISM330DLC_ROUND_XL_SH1_TO_SH6      = 5,
  ISM330DLC_ROUND_GY_XL_SH1_TO_SH12  = 6,
  ISM330DLC_ROUND_GY_XL_SH1_TO_SH6   = 7,
} ism330dlc_rounding_t;
int32_t ism330dlc_rounding_mode_set(ism330dlc_ctx_t *ctx, ism330dlc_rounding_t val);
int32_t ism330dlc_rounding_mode_get(ism330dlc_ctx_t *ctx, ism330dlc_rounding_t *val);

int32_t ism330dlc_temperature_raw_get(ism330dlc_ctx_t *ctx, uint8_t *buff);

int32_t ism330dlc_angular_rate_raw_get(ism330dlc_ctx_t *ctx, uint8_t *buff);

int32_t ism330dlc_acceleration_raw_get(ism330dlc_ctx_t *ctx, uint8_t *buff);

int32_t ism330dlc_mag_calibrated_raw_get(ism330dlc_ctx_t *ctx, uint8_t *buff);

int32_t ism330dlc_fifo_raw_data_get(ism330dlc_ctx_t *ctx, uint8_t *buffer,
                                  uint8_t len);

typedef enum {
  ISM330DLC_USER_BANK   = 0,
  ISM330DLC_BANK_A      = 1,
} ism330dlc_func_cfg_en_t;
int32_t ism330dlc_mem_bank_set(ism330dlc_ctx_t *ctx, ism330dlc_func_cfg_en_t val);
int32_t ism330dlc_mem_bank_get(ism330dlc_ctx_t *ctx, ism330dlc_func_cfg_en_t *val);

typedef enum {
  ISM330DLC_DRDY_LATCHED    = 0,
  ISM330DLC_DRDY_PULSED     = 1,
} ism330dlc_drdy_pulsed_t;
int32_t ism330dlc_data_ready_mode_set(ism330dlc_ctx_t *ctx,
                                    ism330dlc_drdy_pulsed_t val);
int32_t ism330dlc_data_ready_mode_get(ism330dlc_ctx_t *ctx,
                                    ism330dlc_drdy_pulsed_t *val);

int32_t ism330dlc_device_id_get(ism330dlc_ctx_t *ctx, uint8_t *buff);
int32_t ism330dlc_reset_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_reset_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_LSB_AT_LOW_ADD  = 0,
  ISM330DLC_MSB_AT_LOW_ADD  = 1,
} ism330dlc_ble_t;
int32_t ism330dlc_data_format_set(ism330dlc_ctx_t *ctx, ism330dlc_ble_t val);
int32_t ism330dlc_data_format_get(ism330dlc_ctx_t *ctx, ism330dlc_ble_t *val);

int32_t ism330dlc_auto_increment_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_auto_increment_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_boot_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_boot_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_XL_ST_DISABLE    = 0,
  ISM330DLC_XL_ST_POSITIVE   = 1,
  ISM330DLC_XL_ST_NEGATIVE   = 2,
} ism330dlc_st_xl_t;
int32_t ism330dlc_xl_self_test_set(ism330dlc_ctx_t *ctx, ism330dlc_st_xl_t val);
int32_t ism330dlc_xl_self_test_get(ism330dlc_ctx_t *ctx, ism330dlc_st_xl_t *val);

typedef enum {
  ISM330DLC_GY_ST_DISABLE    = 0,
  ISM330DLC_GY_ST_POSITIVE   = 1,
  ISM330DLC_GY_ST_NEGATIVE   = 3,
} ism330dlc_st_g_t;
int32_t ism330dlc_gy_self_test_set(ism330dlc_ctx_t *ctx, ism330dlc_st_g_t val);
int32_t ism330dlc_gy_self_test_get(ism330dlc_ctx_t *ctx, ism330dlc_st_g_t *val);

int32_t ism330dlc_filter_settling_mask_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_filter_settling_mask_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_USE_SLOPE = 0,
  ISM330DLC_USE_HPF   = 1,
} ism330dlc_slope_fds_t;
int32_t ism330dlc_xl_hp_path_internal_set(ism330dlc_ctx_t *ctx,
                                        ism330dlc_slope_fds_t val);
int32_t ism330dlc_xl_hp_path_internal_get(ism330dlc_ctx_t *ctx,
                                        ism330dlc_slope_fds_t *val);

typedef enum {
  ISM330DLC_XL_ANA_BW_1k5Hz = 0,
  ISM330DLC_XL_ANA_BW_400Hz = 1,
} ism330dlc_bw0_xl_t;
int32_t ism330dlc_xl_filter_analog_set(ism330dlc_ctx_t *ctx,
                                     ism330dlc_bw0_xl_t val);
int32_t ism330dlc_xl_filter_analog_get(ism330dlc_ctx_t *ctx,
                                     ism330dlc_bw0_xl_t *val);

typedef enum {
  ISM330DLC_XL_LP1_ODR_DIV_2 = 0,
  ISM330DLC_XL_LP1_ODR_DIV_4 = 1,
  ISM330DLC_XL_LP1_NA        = 2,
} ism330dlc_lpf1_bw_sel_t;
int32_t ism330dlc_xl_lp1_bandwidth_set(ism330dlc_ctx_t *ctx,
                                     ism330dlc_lpf1_bw_sel_t val);
int32_t ism330dlc_xl_lp1_bandwidth_get(ism330dlc_ctx_t *ctx,
                                     ism330dlc_lpf1_bw_sel_t *val);

typedef enum {
  ISM330DLC_XL_LOW_LAT_LP_ODR_DIV_50     = 0x00,
  ISM330DLC_XL_LOW_LAT_LP_ODR_DIV_100    = 0x01,
  ISM330DLC_XL_LOW_LAT_LP_ODR_DIV_9      = 0x02,
  ISM330DLC_XL_LOW_LAT_LP_ODR_DIV_400    = 0x03,
  ISM330DLC_XL_LOW_NOISE_LP_ODR_DIV_50   = 0x10,
  ISM330DLC_XL_LOW_NOISE_LP_ODR_DIV_100  = 0x11,
  ISM330DLC_XL_LOW_NOISE_LP_ODR_DIV_9    = 0x12,
  ISM330DLC_XL_LOW_NOISE_LP_ODR_DIV_400  = 0x13,
  ISM330DLC_XL_LP_NA                     = 0x14
} ism330dlc_input_composite_t;
int32_t ism330dlc_xl_lp2_bandwidth_set(ism330dlc_ctx_t *ctx,
                                     ism330dlc_input_composite_t val);
int32_t ism330dlc_xl_lp2_bandwidth_get(ism330dlc_ctx_t *ctx,
                                     ism330dlc_input_composite_t *val);

int32_t ism330dlc_xl_reference_mode_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_xl_reference_mode_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_XL_HP_ODR_DIV_4      = 0x00, /* Slope filter */
  ISM330DLC_XL_HP_ODR_DIV_100    = 0x01,
  ISM330DLC_XL_HP_ODR_DIV_9      = 0x02,
  ISM330DLC_XL_HP_ODR_DIV_400    = 0x03,
  ISM330DLC_XL_HP_NA             = 0x04,
} ism330dlc_hpcf_xl_t;
int32_t ism330dlc_xl_hp_bandwidth_set(ism330dlc_ctx_t *ctx,
                                    ism330dlc_hpcf_xl_t val);
int32_t ism330dlc_xl_hp_bandwidth_get(ism330dlc_ctx_t *ctx,
                                    ism330dlc_hpcf_xl_t *val);

typedef enum {
  ISM330DLC_XL_UI_LP1_ODR_DIV_2 = 0,
  ISM330DLC_XL_UI_LP1_ODR_DIV_4 = 1,
  ISM330DLC_XL_UI_LP1_NA        = 2,  /* ERROR CODE */
} ism330dlc_ui_lpf1_bw_sel_t;
int32_t ism330dlc_xl_ui_lp1_bandwidth_set(ism330dlc_ctx_t *ctx,
                                        ism330dlc_ui_lpf1_bw_sel_t val);
int32_t ism330dlc_xl_ui_lp1_bandwidth_get(ism330dlc_ctx_t *ctx,
                                        ism330dlc_ui_lpf1_bw_sel_t *val);

int32_t ism330dlc_xl_ui_slope_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_xl_ui_slope_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_AUX_LP_LIGHT          = 2,
  ISM330DLC_AUX_LP_NORMAL         = 3,
  ISM330DLC_AUX_LP_STRONG         = 0,
  ISM330DLC_AUX_LP_AGGRESSIVE     = 1,
} ism330dlc_filter_xl_conf_ois_t;
int32_t ism330dlc_xl_aux_lp_bandwidth_set(ism330dlc_ctx_t *ctx,
                                        ism330dlc_filter_xl_conf_ois_t val);
int32_t ism330dlc_xl_aux_lp_bandwidth_get(ism330dlc_ctx_t *ctx,
                                        ism330dlc_filter_xl_conf_ois_t *val);

typedef enum {
  ISM330DLC_LP2_ONLY                    = 0x00,

  ISM330DLC_HP_16mHz_LP2                = 0x80,
  ISM330DLC_HP_65mHz_LP2                = 0x90,
  ISM330DLC_HP_260mHz_LP2               = 0xA0,
  ISM330DLC_HP_1Hz04_LP2                = 0xB0,

  ISM330DLC_HP_DISABLE_LP1_LIGHT        = 0x0A,
  ISM330DLC_HP_DISABLE_LP1_NORMAL       = 0x09,
  ISM330DLC_HP_DISABLE_LP_STRONG        = 0x08,
  ISM330DLC_HP_DISABLE_LP1_AGGRESSIVE   = 0x0B,

  ISM330DLC_HP_16mHz_LP1_LIGHT          = 0x8A,
  ISM330DLC_HP_65mHz_LP1_NORMAL         = 0x99,
  ISM330DLC_HP_260mHz_LP1_STRONG        = 0xA8,
  ISM330DLC_HP_1Hz04_LP1_AGGRESSIVE     = 0xBB,
} ism330dlc_lpf1_sel_g_t;
int32_t ism330dlc_gy_band_pass_set(ism330dlc_ctx_t *ctx,
                                    ism330dlc_lpf1_sel_g_t val);
int32_t ism330dlc_gy_band_pass_get(ism330dlc_ctx_t *ctx,
                                    ism330dlc_lpf1_sel_g_t *val);

int32_t ism330dlc_gy_ui_high_pass_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_gy_ui_high_pass_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_HP_DISABLE_LP_173Hz        = 0x02,
  ISM330DLC_HP_DISABLE_LP_237Hz        = 0x01,
  ISM330DLC_HP_DISABLE_LP_351Hz        = 0x00,
  ISM330DLC_HP_DISABLE_LP_937Hz        = 0x03,

  ISM330DLC_HP_16mHz_LP_173Hz          = 0x82,
  ISM330DLC_HP_65mHz_LP_237Hz          = 0x91,
  ISM330DLC_HP_260mHz_LP_351Hz         = 0xA0,
  ISM330DLC_HP_1Hz04_LP_937Hz          = 0xB3,
} ism330dlc_hp_en_ois_t;
int32_t ism330dlc_gy_aux_bandwidth_set(ism330dlc_ctx_t *ctx,
                                     ism330dlc_hp_en_ois_t val);
int32_t ism330dlc_gy_aux_bandwidth_get(ism330dlc_ctx_t *ctx,
                                     ism330dlc_hp_en_ois_t *val);

int32_t ism330dlc_aux_status_reg_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_status_spiaux_t *val);

int32_t ism330dlc_aux_xl_flag_data_ready_get(ism330dlc_ctx_t *ctx,
                                             uint8_t *val);

int32_t ism330dlc_aux_gy_flag_data_ready_get(ism330dlc_ctx_t *ctx,
                                             uint8_t *val);

int32_t ism330dlc_aux_gy_flag_settling_get(ism330dlc_ctx_t *ctx,
                                           uint8_t *val);

typedef enum {
  ISM330DLC_AUX_DEN_DISABLE         = 0,
  ISM330DLC_AUX_DEN_LEVEL_LATCH     = 3,
  ISM330DLC_AUX_DEN_LEVEL_TRIG      = 2,
} ism330dlc_lvl_ois_t;
int32_t ism330dlc_aux_den_mode_set(ism330dlc_ctx_t *ctx,
                                 ism330dlc_lvl_ois_t val);
int32_t ism330dlc_aux_den_mode_get(ism330dlc_ctx_t *ctx,
                                 ism330dlc_lvl_ois_t *val);

int32_t ism330dlc_aux_drdy_on_int2_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_aux_drdy_on_int2_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_AUX_DISABLE   = 0,
  ISM330DLC_MODE_3_GY     = 1,
  ISM330DLC_MODE_4_GY_XL  = 3,
} ism330dlc_ois_en_spi2_t;
int32_t ism330dlc_aux_mode_set(ism330dlc_ctx_t *ctx,
                               ism330dlc_ois_en_spi2_t val);
int32_t ism330dlc_aux_mode_get(ism330dlc_ctx_t *ctx,
                               ism330dlc_ois_en_spi2_t *val);

typedef enum {
  ISM330DLC_250dps_AUX   = 0,
  ISM330DLC_125dps_AUX   = 1,
  ISM330DLC_500dps_AUX   = 2,
  ISM330DLC_1000dps_AUX  = 4,
  ISM330DLC_2000dps_AUX  = 6,
} ism330dlc_fs_g_ois_t;
int32_t ism330dlc_aux_gy_full_scale_set(ism330dlc_ctx_t *ctx,
                                      ism330dlc_fs_g_ois_t val);
int32_t ism330dlc_aux_gy_full_scale_get(ism330dlc_ctx_t *ctx,
                                      ism330dlc_fs_g_ois_t *val);

typedef enum {
  ISM330DLC_AUX_SPI_4_WIRE = 0,
  ISM330DLC_AUX_SPI_3_WIRE = 1,
} ism330dlc_sim_ois_t;
int32_t ism330dlc_aux_spi_mode_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_sim_ois_t val);
int32_t ism330dlc_aux_spi_mode_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_sim_ois_t *val);

typedef enum {
  ISM330DLC_AUX_LSB_AT_LOW_ADD = 0,
  ISM330DLC_AUX_MSB_AT_LOW_ADD = 1,
} ism330dlc_ble_ois_t;
int32_t ism330dlc_aux_data_format_set(ism330dlc_ctx_t *ctx,
                                    ism330dlc_ble_ois_t val);
int32_t ism330dlc_aux_data_format_get(ism330dlc_ctx_t *ctx,
                                    ism330dlc_ble_ois_t *val);

typedef enum {
  ISM330DLC_ENABLE_CLAMP    = 0,
  ISM330DLC_DISABLE_CLAMP   = 1,
} ism330dlc_st_ois_clampdis_t;
int32_t ism330dlc_aux_gy_clamp_set(ism330dlc_ctx_t *ctx,
                                 ism330dlc_st_ois_clampdis_t val);
int32_t ism330dlc_aux_gy_clamp_get(ism330dlc_ctx_t *ctx,
                                 ism330dlc_st_ois_clampdis_t *val);

typedef enum {
  ISM330DLC_AUX_GY_DISABLE  = 0,
  ISM330DLC_AUX_GY_POS      = 1,
  ISM330DLC_AUX_GY_NEG      = 3,
} ism330dlc_st_ois_t;
int32_t ism330dlc_aux_gy_self_test_set(ism330dlc_ctx_t *ctx,
                                     ism330dlc_st_ois_t val);
int32_t ism330dlc_aux_gy_self_test_get(ism330dlc_ctx_t *ctx,
                                     ism330dlc_st_ois_t *val);

typedef enum {
  ISM330DLC_AUX_2g   = 0,
  ISM330DLC_AUX_16g  = 1,
  ISM330DLC_AUX_4g   = 2,
  ISM330DLC_AUX_8g   = 3,
} ism330dlc_fs_xl_ois_t;
int32_t ism330dlc_aux_xl_full_scale_set(ism330dlc_ctx_t *ctx,
                                      ism330dlc_fs_xl_ois_t val);
int32_t ism330dlc_aux_xl_full_scale_get(ism330dlc_ctx_t *ctx,
                                      ism330dlc_fs_xl_ois_t *val);

typedef enum {
  ISM330DLC_AUX_DEN_ACTIVE_LOW   = 0,
  ISM330DLC_AUX_DEN_ACTIVE_HIGH  = 1,
} ism330dlc_den_lh_ois_t;
int32_t ism330dlc_aux_den_polarity_set(ism330dlc_ctx_t *ctx,
                                     ism330dlc_den_lh_ois_t val);
int32_t ism330dlc_aux_den_polarity_get(ism330dlc_ctx_t *ctx,
                                     ism330dlc_den_lh_ois_t *val);

typedef enum {
  ISM330DLC_SPI_4_WIRE  = 0,
  ISM330DLC_SPI_3_WIRE  = 1,
} ism330dlc_sim_t;
int32_t ism330dlc_spi_mode_set(ism330dlc_ctx_t *ctx, ism330dlc_sim_t val);
int32_t ism330dlc_spi_mode_get(ism330dlc_ctx_t *ctx, ism330dlc_sim_t *val);

typedef enum {
  ISM330DLC_I2C_ENABLE   = 0,
  ISM330DLC_I2C_DISABLE  = 1,
} ism330dlc_i2c_disable_t;
int32_t ism330dlc_i2c_interface_set(ism330dlc_ctx_t *ctx,
                                  ism330dlc_i2c_disable_t val);
int32_t ism330dlc_i2c_interface_get(ism330dlc_ctx_t *ctx,
                                  ism330dlc_i2c_disable_t *val);

typedef struct {
  uint8_t int1_drdy_xl             : 1;
  uint8_t int1_drdy_g              : 1;
  uint8_t int1_boot                : 1;
  uint8_t int1_fth                 : 1;
  uint8_t int1_fifo_ovr            : 1;
  uint8_t int1_full_flag           : 1;
  uint8_t int1_tilt                : 1;
  uint8_t int1_6d                  : 1;
  uint8_t int1_double_tap          : 1;
  uint8_t int1_ff                  : 1;
  uint8_t int1_wu                  : 1;
  uint8_t int1_single_tap          : 1;
  uint8_t int1_inact_state         : 1;
  uint8_t den_drdy_int1            : 1;
  uint8_t drdy_on_int1             : 1;
} ism330dlc_int1_route_t;
int32_t ism330dlc_pin_int1_route_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_int1_route_t val);
int32_t ism330dlc_pin_int1_route_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_int1_route_t *val);

typedef struct{
  uint8_t int2_drdy_xl             : 1;
  uint8_t int2_drdy_g              : 1;
  uint8_t int2_drdy_temp           : 1;
  uint8_t int2_fth                 : 1;
  uint8_t int2_fifo_ovr            : 1;
  uint8_t int2_full_flag           : 1;
  uint8_t int2_iron                : 1;
  uint8_t int2_tilt                : 1;
  uint8_t int2_6d                  : 1;
  uint8_t int2_double_tap          : 1;
  uint8_t int2_ff                  : 1;
  uint8_t int2_wu                  : 1;
  uint8_t int2_single_tap          : 1;
  uint8_t int2_inact_state         : 1;
} ism330dlc_int2_route_t;
int32_t ism330dlc_pin_int2_route_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_int2_route_t val);
int32_t ism330dlc_pin_int2_route_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_int2_route_t *val);

typedef enum {
  ISM330DLC_PUSH_PULL   = 0,
  ISM330DLC_OPEN_DRAIN  = 1,
} ism330dlc_pp_od_t;
int32_t ism330dlc_pin_mode_set(ism330dlc_ctx_t *ctx, ism330dlc_pp_od_t val);
int32_t ism330dlc_pin_mode_get(ism330dlc_ctx_t *ctx, ism330dlc_pp_od_t *val);

typedef enum {
  ISM330DLC_ACTIVE_HIGH   = 0,
  ISM330DLC_ACTIVE_LOW    = 1,
} ism330dlc_h_lactive_t;
int32_t ism330dlc_pin_polarity_set(ism330dlc_ctx_t *ctx, ism330dlc_h_lactive_t val);
int32_t ism330dlc_pin_polarity_get(ism330dlc_ctx_t *ctx, ism330dlc_h_lactive_t *val);

int32_t ism330dlc_all_on_int1_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_all_on_int1_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_INT_PULSED   = 0,
  ISM330DLC_INT_LATCHED  = 1,
} ism330dlc_lir_t;
int32_t ism330dlc_int_notification_set(ism330dlc_ctx_t *ctx, ism330dlc_lir_t val);
int32_t ism330dlc_int_notification_get(ism330dlc_ctx_t *ctx, ism330dlc_lir_t *val);

int32_t ism330dlc_wkup_threshold_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_wkup_threshold_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_wkup_dur_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_wkup_dur_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_gy_sleep_mode_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_gy_sleep_mode_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_PROPERTY_DISABLE          = 0,
  ISM330DLC_XL_12Hz5_GY_NOT_AFFECTED  = 1,
  ISM330DLC_XL_12Hz5_GY_SLEEP         = 2,
  ISM330DLC_XL_12Hz5_GY_PD            = 3,
} ism330dlc_inact_en_t;
int32_t ism330dlc_act_mode_set(ism330dlc_ctx_t *ctx, ism330dlc_inact_en_t val);
int32_t ism330dlc_act_mode_get(ism330dlc_ctx_t *ctx, ism330dlc_inact_en_t *val);

int32_t ism330dlc_act_sleep_dur_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_act_sleep_dur_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tap_src_get(ism330dlc_ctx_t *ctx, ism330dlc_tap_src_t *val);

int32_t ism330dlc_tap_detection_on_z_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_tap_detection_on_z_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tap_detection_on_y_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_tap_detection_on_y_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tap_detection_on_x_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_tap_detection_on_x_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tap_threshold_x_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_tap_threshold_x_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tap_shock_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_tap_shock_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tap_quiet_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_tap_quiet_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tap_dur_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_tap_dur_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_ONLY_SINGLE          = 0,
  ISM330DLC_BOTH_SINGLE_DOUBLE   = 1,
} ism330dlc_single_double_tap_t;
int32_t ism330dlc_tap_mode_set(ism330dlc_ctx_t *ctx,
                             ism330dlc_single_double_tap_t val);
int32_t ism330dlc_tap_mode_get(ism330dlc_ctx_t *ctx,
                             ism330dlc_single_double_tap_t *val);

typedef enum {
  ISM330DLC_ODR_DIV_2_FEED      = 0,
  ISM330DLC_LPF2_FEED           = 1,
} ism330dlc_low_pass_on_6d_t;
int32_t ism330dlc_6d_feed_data_set(ism330dlc_ctx_t *ctx,
                                 ism330dlc_low_pass_on_6d_t val);
int32_t ism330dlc_6d_feed_data_get(ism330dlc_ctx_t *ctx,
                                 ism330dlc_low_pass_on_6d_t *val);

typedef enum {
  ISM330DLC_DEG_80      = 0,
  ISM330DLC_DEG_70      = 1,
  ISM330DLC_DEG_60      = 2,
  ISM330DLC_DEG_50      = 3,
} ism330dlc_sixd_ths_t;
int32_t ism330dlc_6d_threshold_set(ism330dlc_ctx_t *ctx, ism330dlc_sixd_ths_t val);
int32_t ism330dlc_6d_threshold_get(ism330dlc_ctx_t *ctx, ism330dlc_sixd_ths_t *val);

int32_t ism330dlc_4d_mode_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_4d_mode_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_ff_dur_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_ff_dur_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_FF_TSH_156mg = 0,
  ISM330DLC_FF_TSH_219mg = 1,
  ISM330DLC_FF_TSH_250mg = 2,
  ISM330DLC_FF_TSH_312mg = 3,
  ISM330DLC_FF_TSH_344mg = 4,
  ISM330DLC_FF_TSH_406mg = 5,
  ISM330DLC_FF_TSH_469mg = 6,
  ISM330DLC_FF_TSH_500mg = 7,
} ism330dlc_ff_ths_t;
int32_t ism330dlc_ff_threshold_set(ism330dlc_ctx_t *ctx, ism330dlc_ff_ths_t val);
int32_t ism330dlc_ff_threshold_get(ism330dlc_ctx_t *ctx, ism330dlc_ff_ths_t *val);

int32_t ism330dlc_fifo_watermark_set(ism330dlc_ctx_t *ctx, uint16_t val);
int32_t ism330dlc_fifo_watermark_get(ism330dlc_ctx_t *ctx, uint16_t *val);

int32_t ism330dlc_fifo_data_level_get(ism330dlc_ctx_t *ctx, uint16_t *val);

int32_t ism330dlc_fifo_wtm_flag_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_fifo_pattern_get(ism330dlc_ctx_t *ctx, uint16_t *val);

int32_t ism330dlc_fifo_temp_batch_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_fifo_temp_batch_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_TRG_XL_GY_DRDY     = 0,
  ISM330DLC_TRG_SH_DRDY        = 1,
} ism330dlc_trigger_fifo_t;
int32_t ism330dlc_fifo_write_trigger_set(ism330dlc_ctx_t *ctx,
                                       ism330dlc_trigger_fifo_t val);
int32_t ism330dlc_fifo_write_trigger_get(ism330dlc_ctx_t *ctx,
                                       ism330dlc_trigger_fifo_t *val);

typedef enum {
  ISM330DLC_FIFO_XL_DISABLE  = 0,
  ISM330DLC_FIFO_XL_NO_DEC   = 1,
  ISM330DLC_FIFO_XL_DEC_2    = 2,
  ISM330DLC_FIFO_XL_DEC_3    = 3,
  ISM330DLC_FIFO_XL_DEC_4    = 4,
  ISM330DLC_FIFO_XL_DEC_8    = 5,
  ISM330DLC_FIFO_XL_DEC_16   = 6,
  ISM330DLC_FIFO_XL_DEC_32   = 7,
} ism330dlc_dec_fifo_xl_t;
int32_t ism330dlc_fifo_xl_batch_set(ism330dlc_ctx_t *ctx,
                                  ism330dlc_dec_fifo_xl_t val);
int32_t ism330dlc_fifo_xl_batch_get(ism330dlc_ctx_t *ctx,
                                  ism330dlc_dec_fifo_xl_t *val);

typedef enum {
  ISM330DLC_FIFO_GY_DISABLE = 0,
  ISM330DLC_FIFO_GY_NO_DEC  = 1,
  ISM330DLC_FIFO_GY_DEC_2   = 2,
  ISM330DLC_FIFO_GY_DEC_3   = 3,
  ISM330DLC_FIFO_GY_DEC_4   = 4,
  ISM330DLC_FIFO_GY_DEC_8   = 5,
  ISM330DLC_FIFO_GY_DEC_16  = 6,
  ISM330DLC_FIFO_GY_DEC_32  = 7,
} ism330dlc_dec_fifo_gyro_t;
int32_t ism330dlc_fifo_gy_batch_set(ism330dlc_ctx_t *ctx,
                                  ism330dlc_dec_fifo_gyro_t val);
int32_t ism330dlc_fifo_gy_batch_get(ism330dlc_ctx_t *ctx,
                                  ism330dlc_dec_fifo_gyro_t *val);

typedef enum {
  ISM330DLC_FIFO_DS3_DISABLE   = 0,
  ISM330DLC_FIFO_DS3_NO_DEC    = 1,
  ISM330DLC_FIFO_DS3_DEC_2     = 2,
  ISM330DLC_FIFO_DS3_DEC_3     = 3,
  ISM330DLC_FIFO_DS3_DEC_4     = 4,
  ISM330DLC_FIFO_DS3_DEC_8     = 5,
  ISM330DLC_FIFO_DS3_DEC_16    = 6,
  ISM330DLC_FIFO_DS3_DEC_32    = 7,
} ism330dlc_dec_ds3_fifo_t;
int32_t ism330dlc_fifo_dataset_3_batch_set(ism330dlc_ctx_t *ctx,
                                         ism330dlc_dec_ds3_fifo_t val);
int32_t ism330dlc_fifo_dataset_3_batch_get(ism330dlc_ctx_t *ctx,
                                         ism330dlc_dec_ds3_fifo_t *val);

typedef enum {
  ISM330DLC_FIFO_DS4_DISABLE  = 0,
  ISM330DLC_FIFO_DS4_NO_DEC   = 1,
  ISM330DLC_FIFO_DS4_DEC_2    = 2,
  ISM330DLC_FIFO_DS4_DEC_3    = 3,
  ISM330DLC_FIFO_DS4_DEC_4    = 4,
  ISM330DLC_FIFO_DS4_DEC_8    = 5,
  ISM330DLC_FIFO_DS4_DEC_16   = 6,
  ISM330DLC_FIFO_DS4_DEC_32   = 7,
} ism330dlc_dec_ds4_fifo_t;
int32_t ism330dlc_fifo_dataset_4_batch_set(ism330dlc_ctx_t *ctx,
                                         ism330dlc_dec_ds4_fifo_t val);
int32_t ism330dlc_fifo_dataset_4_batch_get(ism330dlc_ctx_t *ctx,
                                         ism330dlc_dec_ds4_fifo_t *val);

int32_t ism330dlc_fifo_xl_gy_8bit_format_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_fifo_xl_gy_8bit_format_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_fifo_stop_on_wtm_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_fifo_stop_on_wtm_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_BYPASS_MODE           = 0,
  ISM330DLC_FIFO_MODE             = 1,
  ISM330DLC_STREAM_TO_FIFO_MODE   = 3,
  ISM330DLC_BYPASS_TO_STREAM_MODE = 4,
  ISM330DLC_STREAM_MODE           = 6,
} ism330dlc_fifo_mode_t;
int32_t ism330dlc_fifo_mode_set(ism330dlc_ctx_t *ctx, ism330dlc_fifo_mode_t val);
int32_t ism330dlc_fifo_mode_get(ism330dlc_ctx_t *ctx, ism330dlc_fifo_mode_t *val);

typedef enum {
  ISM330DLC_FIFO_DISABLE   =  0,
  ISM330DLC_FIFO_12Hz5     =  1,
  ISM330DLC_FIFO_26Hz      =  2,
  ISM330DLC_FIFO_52Hz      =  3,
  ISM330DLC_FIFO_104Hz     =  4,
  ISM330DLC_FIFO_208Hz     =  5,
  ISM330DLC_FIFO_416Hz     =  6,
  ISM330DLC_FIFO_833Hz     =  7,
  ISM330DLC_FIFO_1k66Hz    =  8,
  ISM330DLC_FIFO_3k33Hz    =  9,
  ISM330DLC_FIFO_6k66Hz    = 10,
} ism330dlc_odr_fifo_t;
int32_t ism330dlc_fifo_data_rate_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_odr_fifo_t val);
int32_t ism330dlc_fifo_data_rate_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_odr_fifo_t *val);

typedef enum {
  ISM330DLC_DEN_ACT_LOW    = 0,
  ISM330DLC_DEN_ACT_HIGH   = 1,
} ism330dlc_den_lh_t;
int32_t ism330dlc_den_polarity_set(ism330dlc_ctx_t *ctx, ism330dlc_den_lh_t val);
int32_t ism330dlc_den_polarity_get(ism330dlc_ctx_t *ctx, ism330dlc_den_lh_t *val);

typedef enum {
  ISM330DLC_DEN_DISABLE    = 0,
  ISM330DLC_LEVEL_FIFO     = 6,
  ISM330DLC_LEVEL_LETCHED  = 3,
  ISM330DLC_LEVEL_TRIGGER  = 2,
  ISM330DLC_EDGE_TRIGGER   = 4,
} ism330dlc_den_mode_t;
int32_t ism330dlc_den_mode_set(ism330dlc_ctx_t *ctx, ism330dlc_den_mode_t val);
int32_t ism330dlc_den_mode_get(ism330dlc_ctx_t *ctx, ism330dlc_den_mode_t *val);

typedef enum {
  ISM330DLC_STAMP_IN_GY_DATA     = 0,
  ISM330DLC_STAMP_IN_XL_DATA     = 1,
  ISM330DLC_STAMP_IN_GY_XL_DATA  = 2,
} ism330dlc_den_xl_en_t;
int32_t ism330dlc_den_enable_set(ism330dlc_ctx_t *ctx, ism330dlc_den_xl_en_t val);
int32_t ism330dlc_den_enable_get(ism330dlc_ctx_t *ctx, ism330dlc_den_xl_en_t *val);

int32_t ism330dlc_den_mark_axis_z_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_den_mark_axis_z_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_den_mark_axis_y_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_den_mark_axis_y_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_den_mark_axis_x_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_den_mark_axis_x_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tilt_sens_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_tilt_sens_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_wrist_tilt_sens_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_wrist_tilt_sens_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_tilt_latency_set(ism330dlc_ctx_t *ctx, uint8_t *buff);
int32_t ism330dlc_tilt_latency_get(ism330dlc_ctx_t *ctx, uint8_t *buff);

int32_t ism330dlc_tilt_threshold_set(ism330dlc_ctx_t *ctx, uint8_t *buff);
int32_t ism330dlc_tilt_threshold_get(ism330dlc_ctx_t *ctx, uint8_t *buff);

int32_t ism330dlc_mag_soft_iron_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_mag_soft_iron_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_mag_hard_iron_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_mag_hard_iron_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_mag_soft_iron_mat_set(ism330dlc_ctx_t *ctx, uint8_t *buff);
int32_t ism330dlc_mag_soft_iron_mat_get(ism330dlc_ctx_t *ctx, uint8_t *buff);

int32_t ism330dlc_mag_offset_set(ism330dlc_ctx_t *ctx, uint8_t *buff);
int32_t ism330dlc_mag_offset_get(ism330dlc_ctx_t *ctx, uint8_t *buff);

int32_t ism330dlc_sh_sync_sens_frame_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_sh_sync_sens_frame_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_RES_RATIO_2_11  = 0,
  ISM330DLC_RES_RATIO_2_12  = 1,
  ISM330DLC_RES_RATIO_2_13  = 2,
  ISM330DLC_RES_RATIO_2_14  = 3,
} ism330dlc_rr_t;
int32_t ism330dlc_sh_sync_sens_ratio_set(ism330dlc_ctx_t *ctx, ism330dlc_rr_t val);
int32_t ism330dlc_sh_sync_sens_ratio_get(ism330dlc_ctx_t *ctx, ism330dlc_rr_t *val);

int32_t ism330dlc_sh_master_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_sh_master_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_sh_pass_through_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_sh_pass_through_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_EXT_PULL_UP       = 0,
  ISM330DLC_INTERNAL_PULL_UP  = 1,
} ism330dlc_pull_up_en_t;
int32_t ism330dlc_sh_pin_mode_set(ism330dlc_ctx_t *ctx, ism330dlc_pull_up_en_t val);
int32_t ism330dlc_sh_pin_mode_get(ism330dlc_ctx_t *ctx, ism330dlc_pull_up_en_t *val);

typedef enum {
  ISM330DLC_XL_GY_DRDY        = 0,
  ISM330DLC_EXT_ON_INT2_PIN   = 1,
} ism330dlc_start_config_t;
int32_t ism330dlc_sh_syncro_mode_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_start_config_t val);
int32_t ism330dlc_sh_syncro_mode_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_start_config_t *val);

int32_t ism330dlc_sh_drdy_on_int1_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_sh_drdy_on_int1_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef struct {
    ism330dlc_sensorhub1_reg_t   sh_byte_1;
    ism330dlc_sensorhub2_reg_t   sh_byte_2;
    ism330dlc_sensorhub3_reg_t   sh_byte_3;
    ism330dlc_sensorhub4_reg_t   sh_byte_4;
    ism330dlc_sensorhub5_reg_t   sh_byte_5;
    ism330dlc_sensorhub6_reg_t   sh_byte_6;
    ism330dlc_sensorhub7_reg_t   sh_byte_7;
    ism330dlc_sensorhub8_reg_t   sh_byte_8;
    ism330dlc_sensorhub9_reg_t   sh_byte_9;
    ism330dlc_sensorhub10_reg_t  sh_byte_10;
    ism330dlc_sensorhub11_reg_t  sh_byte_11;
    ism330dlc_sensorhub12_reg_t  sh_byte_12;
    ism330dlc_sensorhub13_reg_t  sh_byte_13;
    ism330dlc_sensorhub14_reg_t  sh_byte_14;
    ism330dlc_sensorhub15_reg_t  sh_byte_15;
    ism330dlc_sensorhub16_reg_t  sh_byte_16;
    ism330dlc_sensorhub17_reg_t  sh_byte_17;
    ism330dlc_sensorhub18_reg_t  sh_byte_18;
} ism330dlc_emb_sh_read_t;
int32_t ism330dlc_sh_read_data_raw_get(ism330dlc_ctx_t *ctx,
                                     ism330dlc_emb_sh_read_t *val);

int32_t ism330dlc_sh_cmd_sens_sync_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_sh_cmd_sens_sync_get(ism330dlc_ctx_t *ctx, uint8_t *val);

int32_t ism330dlc_sh_spi_sync_error_set(ism330dlc_ctx_t *ctx, uint8_t val);
int32_t ism330dlc_sh_spi_sync_error_get(ism330dlc_ctx_t *ctx, uint8_t *val);

typedef enum {
  ISM330DLC_NORMAL_MODE_READ  = 0,
  ISM330DLC_SRC_MODE_READ     = 1,
} ism330dlc_src_mode_t;
int32_t ism330dlc_sh_cfg_slave_0_rd_mode_set(ism330dlc_ctx_t *ctx,
                                           ism330dlc_src_mode_t val);
int32_t ism330dlc_sh_cfg_slave_0_rd_mode_get(ism330dlc_ctx_t *ctx,
                                           ism330dlc_src_mode_t *val);

typedef enum {
  ISM330DLC_SLV_0        = 0,
  ISM330DLC_SLV_0_1      = 1,
  ISM330DLC_SLV_0_1_2    = 2,
  ISM330DLC_SLV_0_1_2_3  = 3,
} ism330dlc_aux_sens_on_t;
int32_t ism330dlc_sh_num_of_dev_connected_set(ism330dlc_ctx_t *ctx,
                                            ism330dlc_aux_sens_on_t val);
int32_t ism330dlc_sh_num_of_dev_connected_get(ism330dlc_ctx_t *ctx,
                                            ism330dlc_aux_sens_on_t *val);

typedef struct{
  uint8_t   slv0_add;
  uint8_t   slv0_subadd;
  uint8_t   slv0_data;
} ism330dlc_sh_cfg_write_t;
int32_t ism330dlc_sh_cfg_write(ism330dlc_ctx_t *ctx, ism330dlc_sh_cfg_write_t *val);

typedef struct{
  uint8_t   slv_add;
  uint8_t   slv_subadd;
  uint8_t   slv_len;
} ism330dlc_sh_cfg_read_t;
int32_t ism330dlc_sh_slv0_cfg_read(ism330dlc_ctx_t *ctx,
                                 ism330dlc_sh_cfg_read_t *val);
int32_t ism330dlc_sh_slv1_cfg_read(ism330dlc_ctx_t *ctx,
                                 ism330dlc_sh_cfg_read_t *val);
int32_t ism330dlc_sh_slv2_cfg_read(ism330dlc_ctx_t *ctx,
                                 ism330dlc_sh_cfg_read_t *val);
int32_t ism330dlc_sh_slv3_cfg_read(ism330dlc_ctx_t *ctx,
                                 ism330dlc_sh_cfg_read_t *val);

typedef enum {
  ISM330DLC_SL0_NO_DEC   = 0,
  ISM330DLC_SL0_DEC_2    = 1,
  ISM330DLC_SL0_DEC_4    = 2,
  ISM330DLC_SL0_DEC_8    = 3,
} ism330dlc_slave0_rate_t;
int32_t ism330dlc_sh_slave_0_dec_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_slave0_rate_t val);
int32_t ism330dlc_sh_slave_0_dec_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_slave0_rate_t *val);

typedef enum {
  ISM330DLC_EACH_SH_CYCLE     = 0,
  ISM330DLC_ONLY_FIRST_CYCLE  = 1,
} ism330dlc_write_once_t;
int32_t ism330dlc_sh_write_mode_set(ism330dlc_ctx_t *ctx,
                                  ism330dlc_write_once_t val);
int32_t ism330dlc_sh_write_mode_get(ism330dlc_ctx_t *ctx,
                                  ism330dlc_write_once_t *val);

typedef enum {
  ISM330DLC_SL1_NO_DEC   = 0,
  ISM330DLC_SL1_DEC_2    = 1,
  ISM330DLC_SL1_DEC_4    = 2,
  ISM330DLC_SL1_DEC_8    = 3,
} ism330dlc_slave1_rate_t;
int32_t ism330dlc_sh_slave_1_dec_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_slave1_rate_t val);
int32_t ism330dlc_sh_slave_1_dec_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_slave1_rate_t *val);

typedef enum {
  ISM330DLC_SL2_NO_DEC  = 0,
  ISM330DLC_SL2_DEC_2   = 1,
  ISM330DLC_SL2_DEC_4   = 2,
  ISM330DLC_SL2_DEC_8   = 3,
} ism330dlc_slave2_rate_t;
int32_t ism330dlc_sh_slave_2_dec_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_slave2_rate_t val);
int32_t ism330dlc_sh_slave_2_dec_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_slave2_rate_t *val);

typedef enum {
  ISM330DLC_SL3_NO_DEC  = 0,
  ISM330DLC_SL3_DEC_2   = 1,
  ISM330DLC_SL3_DEC_4   = 2,
  ISM330DLC_SL3_DEC_8   = 3,
} ism330dlc_slave3_rate_t;
int32_t ism330dlc_sh_slave_3_dec_set(ism330dlc_ctx_t *ctx,
                                   ism330dlc_slave3_rate_t val);
int32_t ism330dlc_sh_slave_3_dec_get(ism330dlc_ctx_t *ctx,
                                   ism330dlc_slave3_rate_t *val);

/**
  * @}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* ISM330DLC_DRIVER_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
