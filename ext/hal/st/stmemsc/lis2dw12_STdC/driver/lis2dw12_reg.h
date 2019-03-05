/*
 ******************************************************************************
 * @file    lis2dw12_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          lis2dw12_reg.c driver.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2018 STMicroelectronics</center></h2>
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
#ifndef LIS2DW12_REGS_H
#define LIS2DW12_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LIS2DW12
  * @{
  *
  */

/** @defgroup LIS2DW12_sensors_common_types
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

/** @addtogroup  LIS2DW12_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*lis2dw12_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lis2dw12_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lis2dw12_write_ptr  write_reg;
  lis2dw12_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lis2dw12_ctx_t;

/**
  * @}
  *
  */

/** @defgroup LIS2DW12_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> 31 if SA0=1 -> 33 **/
#define LIS2DW12_I2C_ADD_L   0x31U
#define LIS2DW12_I2C_ADD_H   0x33U

/** Device Identification (Who am I) **/
#define LIS2DW12_ID            0x44U

/**
  * @}
  *
  */

#define LIS2DW12_OUT_T_L                     0x0DU
#define LIS2DW12_OUT_T_H                     0x0EU
#define LIS2DW12_WHO_AM_I                    0x0FU
#define LIS2DW12_CTRL1                       0x20U
typedef struct {
  uint8_t lp_mode                    : 2;
  uint8_t mode                       : 2;
  uint8_t odr                        : 4;
} lis2dw12_ctrl1_t;

#define LIS2DW12_CTRL2                       0x21U
typedef struct {
  uint8_t sim                        : 1;
  uint8_t i2c_disable                : 1;
  uint8_t if_add_inc                 : 1;
  uint8_t bdu                        : 1;
  uint8_t cs_pu_disc                 : 1;
  uint8_t not_used_01                : 1;
  uint8_t soft_reset                 : 1;
  uint8_t boot                       : 1;
} lis2dw12_ctrl2_t;

#define LIS2DW12_CTRL3                       0x22U
typedef struct {
  uint8_t slp_mode                   : 2;  /* slp_mode_sel + slp_mode_1 */
  uint8_t not_used_01                : 1;
  uint8_t h_lactive                  : 1;
  uint8_t lir                        : 1;
  uint8_t pp_od                      : 1;
  uint8_t st                         : 2;
} lis2dw12_ctrl3_t;

#define LIS2DW12_CTRL4_INT1_PAD_CTRL         0x23U
typedef struct {
  uint8_t int1_drdy                  : 1;
  uint8_t int1_fth                   : 1;
  uint8_t int1_diff5                 : 1;
  uint8_t int1_tap                   : 1;
  uint8_t int1_ff                    : 1;
  uint8_t int1_wu                    : 1;
  uint8_t int1_single_tap            : 1;
  uint8_t int1_6d                    : 1;
} lis2dw12_ctrl4_int1_pad_ctrl_t;

#define LIS2DW12_CTRL5_INT2_PAD_CTRL         0x24U
typedef struct {
  uint8_t int2_drdy                  : 1;
  uint8_t int2_fth                   : 1;
  uint8_t int2_diff5                 : 1;
  uint8_t int2_ovr                   : 1;
  uint8_t int2_drdy_t                : 1;
  uint8_t int2_boot                  : 1;
  uint8_t int2_sleep_chg             : 1;
  uint8_t int2_sleep_state           : 1;
} lis2dw12_ctrl5_int2_pad_ctrl_t;

#define LIS2DW12_CTRL6                       0x25U
typedef struct {
  uint8_t not_used_01                : 2;
  uint8_t low_noise                  : 1;
  uint8_t fds                        : 1;
  uint8_t fs                         : 2;
  uint8_t bw_filt                    : 2;
} lis2dw12_ctrl6_t;

#define LIS2DW12_OUT_T                       0x26U
#define LIS2DW12_STATUS                      0x27U
typedef struct {
  uint8_t drdy                       : 1;
  uint8_t ff_ia                      : 1;
  uint8_t _6d_ia                     : 1;
  uint8_t single_tap                 : 1;
  uint8_t double_tap                 : 1;
  uint8_t sleep_state                : 1;
  uint8_t wu_ia                      : 1;
  uint8_t fifo_ths                   : 1;
} lis2dw12_status_t;

#define LIS2DW12_OUT_X_L                     0x28U
#define LIS2DW12_OUT_X_H                     0x29U
#define LIS2DW12_OUT_Y_L                     0x2AU
#define LIS2DW12_OUT_Y_H                     0x2BU
#define LIS2DW12_OUT_Z_L                     0x2CU
#define LIS2DW12_OUT_Z_H                     0x2DU
#define LIS2DW12_FIFO_CTRL                   0x2EU
typedef struct {
  uint8_t fth                        : 5;
  uint8_t fmode                      : 3;
} lis2dw12_fifo_ctrl_t;

#define LIS2DW12_FIFO_SAMPLES                0x2FU
typedef struct {
  uint8_t diff                       : 6;
  uint8_t fifo_ovr                   : 1;
  uint8_t fifo_fth                   : 1;
} lis2dw12_fifo_samples_t;

#define LIS2DW12_TAP_THS_X                   0x30U
typedef struct {
  uint8_t tap_thsx                    : 5;
  uint8_t _6d_ths                     : 2;
  uint8_t _4d_en                      : 1;
} lis2dw12_tap_ths_x_t;

#define LIS2DW12_TAP_THS_Y                   0x31U
typedef struct {
  uint8_t tap_thsy                   : 5;
  uint8_t tap_prior                  : 3;
} lis2dw12_tap_ths_y_t;

#define LIS2DW12_TAP_THS_Z                   0x32U
typedef struct {
  uint8_t tap_thsz                   : 5;
  uint8_t tap_z_en                   : 1;
  uint8_t tap_y_en                   : 1;
  uint8_t tap_x_en                   : 1;
} lis2dw12_tap_ths_z_t;

#define LIS2DW12_INT_DUR                     0x33U
typedef struct {
  uint8_t shock                      : 2;
  uint8_t quiet                      : 2;
  uint8_t latency                    : 4;
} lis2dw12_int_dur_t;

#define LIS2DW12_WAKE_UP_THS                 0x34U
typedef struct {
  uint8_t wk_ths                     : 6;
  uint8_t sleep_on                   : 1;
  uint8_t single_double_tap          : 1;
} lis2dw12_wake_up_ths_t;

#define LIS2DW12_WAKE_UP_DUR                 0x35U
typedef struct {
  uint8_t sleep_dur                  : 4;
  uint8_t stationary                 : 1;
  uint8_t wake_dur                   : 2;
  uint8_t ff_dur                     : 1;
} lis2dw12_wake_up_dur_t;

#define LIS2DW12_FREE_FALL                   0x36U
typedef struct {
  uint8_t ff_ths                     : 3;
  uint8_t ff_dur                     : 5;
} lis2dw12_free_fall_t;

#define LIS2DW12_STATUS_DUP                  0x37U
typedef struct {
  uint8_t drdy                       : 1;
  uint8_t ff_ia                      : 1;
  uint8_t _6d_ia                     : 1;
  uint8_t single_tap                 : 1;
  uint8_t double_tap                 : 1;
  uint8_t sleep_state_ia             : 1;
  uint8_t drdy_t                     : 1;
  uint8_t ovr                        : 1;
} lis2dw12_status_dup_t;

#define LIS2DW12_WAKE_UP_SRC                 0x38U
typedef struct {
  uint8_t z_wu                       : 1;
  uint8_t y_wu                       : 1;
  uint8_t x_wu                       : 1;
  uint8_t wu_ia                      : 1;
  uint8_t sleep_state_ia             : 1;
  uint8_t ff_ia                      : 1;
  uint8_t not_used_01                : 2;
} lis2dw12_wake_up_src_t;

#define LIS2DW12_TAP_SRC                     0x39U
typedef struct {
  uint8_t z_tap                      : 1;
  uint8_t y_tap                      : 1;
  uint8_t x_tap                      : 1;
  uint8_t tap_sign                   : 1;
  uint8_t double_tap                 : 1;
  uint8_t single_tap                 : 1;
  uint8_t tap_ia                     : 1;
  uint8_t not_used_01                : 1;
} lis2dw12_tap_src_t;

#define LIS2DW12_SIXD_SRC                    0x3AU
typedef struct {
  uint8_t xl                         : 1;
  uint8_t xh                         : 1;
  uint8_t yl                         : 1;
  uint8_t yh                         : 1;
  uint8_t zl                         : 1;
  uint8_t zh                         : 1;
  uint8_t _6d_ia                     : 1;
  uint8_t not_used_01                : 1;
} lis2dw12_sixd_src_t;

#define LIS2DW12_ALL_INT_SRC                 0x3BU
typedef struct {
  uint8_t ff_ia                      : 1;
  uint8_t wu_ia                      : 1;
  uint8_t single_tap                 : 1;
  uint8_t double_tap                 : 1;
  uint8_t _6d_ia                     : 1;
  uint8_t sleep_change_ia            : 1;
  uint8_t not_used_01                : 2;
} lis2dw12_all_int_src_t;

#define LIS2DW12_X_OFS_USR                   0x3CU
#define LIS2DW12_Y_OFS_USR                   0x3DU
#define LIS2DW12_Z_OFS_USR                   0x3EU
#define LIS2DW12_CTRL_REG7                   0x3FU
typedef struct {
  uint8_t lpass_on6d                 : 1;
  uint8_t hp_ref_mode                : 1;
  uint8_t usr_off_w                  : 1;
  uint8_t usr_off_on_wu              : 1;
  uint8_t usr_off_on_out             : 1;
  uint8_t interrupts_enable          : 1;
  uint8_t int2_on_int1               : 1;
  uint8_t drdy_pulsed                : 1;
} lis2dw12_ctrl_reg7_t;

/**
  * @defgroup LIS2DW12_Register_Union
  * @brief    This union group all the registers that has a bitfield
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
  lis2dw12_ctrl1_t                   ctrl1;
  lis2dw12_ctrl2_t                   ctrl2;
  lis2dw12_ctrl3_t                   ctrl3;
  lis2dw12_ctrl4_int1_pad_ctrl_t     ctrl4_int1_pad_ctrl;
  lis2dw12_ctrl5_int2_pad_ctrl_t     ctrl5_int2_pad_ctrl;
  lis2dw12_ctrl6_t                   ctrl6;
  lis2dw12_status_t                  status;
  lis2dw12_fifo_ctrl_t               fifo_ctrl;
  lis2dw12_fifo_samples_t            fifo_samples;
  lis2dw12_tap_ths_x_t               tap_ths_x;
  lis2dw12_tap_ths_y_t               tap_ths_y;
  lis2dw12_tap_ths_z_t               tap_ths_z;
  lis2dw12_int_dur_t                 int_dur;
  lis2dw12_wake_up_ths_t             wake_up_ths;
  lis2dw12_wake_up_dur_t             wake_up_dur;
  lis2dw12_free_fall_t               free_fall;
  lis2dw12_status_dup_t              status_dup;
  lis2dw12_wake_up_src_t             wake_up_src;
  lis2dw12_tap_src_t                 tap_src;
  lis2dw12_sixd_src_t                sixd_src;
  lis2dw12_all_int_src_t             all_int_src;
  lis2dw12_ctrl_reg7_t               ctrl_reg7;
  bitwise_t                          bitwise;
  uint8_t                            byte;
} lis2dw12_reg_t;

/**
  * @}
  *
  */

int32_t lis2dw12_read_reg(lis2dw12_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);
int32_t lis2dw12_write_reg(lis2dw12_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);

extern float_t lis2dw12_from_fs2_to_mg(int16_t lsb);
extern float_t lis2dw12_from_fs4_to_mg(int16_t lsb);
extern float_t lis2dw12_from_fs8_to_mg(int16_t lsb);
extern float_t lis2dw12_from_fs16_to_mg(int16_t lsb);
extern float_t lis2dw12_from_fs2_lp1_to_mg(int16_t lsb);
extern float_t lis2dw12_from_fs4_lp1_to_mg(int16_t lsb);
extern float_t lis2dw12_from_fs8_lp1_to_mg(int16_t lsb);
extern float_t lis2dw12_from_fs16_lp1_to_mg(int16_t lsb);
extern float_t lis2dw12_from_lsb_to_celsius(int16_t lsb);

typedef enum {
  LIS2DW12_HIGH_PERFORMANCE                    = 0x04,
  LIS2DW12_CONT_LOW_PWR_4                      = 0x03,
  LIS2DW12_CONT_LOW_PWR_3                      = 0x02,
  LIS2DW12_CONT_LOW_PWR_2                      = 0x01,
  LIS2DW12_CONT_LOW_PWR_12bit                  = 0x00,
  LIS2DW12_SINGLE_LOW_PWR_4                    = 0x0B,
  LIS2DW12_SINGLE_LOW_PWR_3                    = 0x0A,
  LIS2DW12_SINGLE_LOW_PWR_2                    = 0x09,
  LIS2DW12_SINGLE_LOW_PWR_12bit                = 0x08,
  LIS2DW12_HIGH_PERFORMANCE_LOW_NOISE          = 0x14,
  LIS2DW12_CONT_LOW_PWR_LOW_NOISE_4            = 0x13,
  LIS2DW12_CONT_LOW_PWR_LOW_NOISE_3            = 0x12,
  LIS2DW12_CONT_LOW_PWR_LOW_NOISE_2            = 0x11,
  LIS2DW12_CONT_LOW_PWR_LOW_NOISE_12bit        = 0x10,
  LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_4          = 0x1B,
  LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_3          = 0x1A,
  LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_2          = 0x19,
  LIS2DW12_SINGLE_LOW_LOW_NOISE_PWR_12bit      = 0x18,
} lis2dw12_mode_t;
int32_t lis2dw12_power_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_mode_t val);
int32_t lis2dw12_power_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_mode_t *val);

typedef enum {
  LIS2DW12_XL_ODR_OFF            = 0x00,
  LIS2DW12_XL_ODR_1Hz6_LP_ONLY   = 0x01,
  LIS2DW12_XL_ODR_12Hz5          = 0x02,
  LIS2DW12_XL_ODR_25Hz           = 0x03,
  LIS2DW12_XL_ODR_50Hz           = 0x04,
  LIS2DW12_XL_ODR_100Hz          = 0x05,
  LIS2DW12_XL_ODR_200Hz          = 0x06,
  LIS2DW12_XL_ODR_400Hz          = 0x07,
  LIS2DW12_XL_ODR_800Hz          = 0x08,
  LIS2DW12_XL_ODR_1k6Hz          = 0x09,
  LIS2DW12_XL_SET_SW_TRIG        = 0x10,  /* Use this only in SINGLE mode */
  LIS2DW12_XL_SET_PIN_TRIG       = 0x20,  /* Use this only in SINGLE mode */
} lis2dw12_odr_t;
int32_t lis2dw12_data_rate_set(lis2dw12_ctx_t *ctx, lis2dw12_odr_t val);
int32_t lis2dw12_data_rate_get(lis2dw12_ctx_t *ctx, lis2dw12_odr_t *val);

int32_t lis2dw12_block_data_update_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_block_data_update_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2DW12_2g     = 0,
  LIS2DW12_4g     = 1,
  LIS2DW12_8g     = 2,
  LIS2DW12_16g    = 3,
} lis2dw12_fs_t;
int32_t lis2dw12_full_scale_set(lis2dw12_ctx_t *ctx, lis2dw12_fs_t val);
int32_t lis2dw12_full_scale_get(lis2dw12_ctx_t *ctx, lis2dw12_fs_t *val);

int32_t lis2dw12_status_reg_get(lis2dw12_ctx_t *ctx, lis2dw12_status_t *val);

int32_t lis2dw12_flag_data_ready_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef struct{
  lis2dw12_status_dup_t   status_dup;
  lis2dw12_wake_up_src_t  wake_up_src;
  lis2dw12_tap_src_t      tap_src;
  lis2dw12_sixd_src_t     sixd_src;
  lis2dw12_all_int_src_t  all_int_src;
} lis2dw12_all_sources_t;
int32_t lis2dw12_all_sources_get(lis2dw12_ctx_t *ctx,
                                 lis2dw12_all_sources_t *val);

int32_t lis2dw12_usr_offset_x_set(lis2dw12_ctx_t *ctx, uint8_t *buff);
int32_t lis2dw12_usr_offset_x_get(lis2dw12_ctx_t *ctx, uint8_t *buff);

int32_t lis2dw12_usr_offset_y_set(lis2dw12_ctx_t *ctx, uint8_t *buff);
int32_t lis2dw12_usr_offset_y_get(lis2dw12_ctx_t *ctx, uint8_t *buff);

int32_t lis2dw12_usr_offset_z_set(lis2dw12_ctx_t *ctx, uint8_t *buff);
int32_t lis2dw12_usr_offset_z_get(lis2dw12_ctx_t *ctx, uint8_t *buff);

typedef enum {
  LIS2DW12_LSb_977ug    = 0,
  LIS2DW12_LSb_15mg6    = 1,
} lis2dw12_usr_off_w_t;
int32_t lis2dw12_offset_weight_set(lis2dw12_ctx_t *ctx,
                                      lis2dw12_usr_off_w_t val);
int32_t lis2dw12_offset_weight_get(lis2dw12_ctx_t *ctx,
                                      lis2dw12_usr_off_w_t *val);

int32_t lis2dw12_temperature_raw_get(lis2dw12_ctx_t *ctx, uint8_t *buff);

int32_t lis2dw12_acceleration_raw_get(lis2dw12_ctx_t *ctx, uint8_t *buff);

int32_t lis2dw12_device_id_get(lis2dw12_ctx_t *ctx, uint8_t *buff);

int32_t lis2dw12_auto_increment_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_auto_increment_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_reset_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_reset_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_boot_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_boot_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2DW12_XL_ST_DISABLE      = 0,
  LIS2DW12_XL_ST_POSITIVE     = 1,
  LIS2DW12_XL_ST_NEGATIVE     = 2,
} lis2dw12_st_t;
int32_t lis2dw12_self_test_set(lis2dw12_ctx_t *ctx, lis2dw12_st_t val);
int32_t lis2dw12_self_test_get(lis2dw12_ctx_t *ctx, lis2dw12_st_t *val);

typedef enum {
  LIS2DW12_DRDY_LATCHED   = 0,
  LIS2DW12_DRDY_PULSED    = 1,
} lis2dw12_drdy_pulsed_t;
int32_t lis2dw12_data_ready_mode_set(lis2dw12_ctx_t *ctx,
                                     lis2dw12_drdy_pulsed_t val);
int32_t lis2dw12_data_ready_mode_get(lis2dw12_ctx_t *ctx,
                                     lis2dw12_drdy_pulsed_t *val);

typedef enum {
  LIS2DW12_LPF_ON_OUT         = 0x00,
  LIS2DW12_USER_OFFSET_ON_OUT  = 0x01,
  LIS2DW12_HIGH_PASS_ON_OUT    = 0x10,
} lis2dw12_fds_t;
int32_t lis2dw12_filter_path_set(lis2dw12_ctx_t *ctx, lis2dw12_fds_t val);
int32_t lis2dw12_filter_path_get(lis2dw12_ctx_t *ctx, lis2dw12_fds_t *val);

typedef enum {
  LIS2DW12_ODR_DIV_2     = 0,
  LIS2DW12_ODR_DIV_4     = 1,
  LIS2DW12_ODR_DIV_10    = 2,
  LIS2DW12_ODR_DIV_20    = 3,
} lis2dw12_bw_filt_t;
int32_t lis2dw12_filter_bandwidth_set(lis2dw12_ctx_t *ctx,
                                         lis2dw12_bw_filt_t val);
int32_t lis2dw12_filter_bandwidth_get(lis2dw12_ctx_t *ctx,
                                         lis2dw12_bw_filt_t *val);

int32_t lis2dw12_reference_mode_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_reference_mode_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2DW12_SPI_4_WIRE   = 0,
  LIS2DW12_SPI_3_WIRE   = 1,
} lis2dw12_sim_t;
int32_t lis2dw12_spi_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_sim_t val);
int32_t lis2dw12_spi_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_sim_t *val);

typedef enum {
  LIS2DW12_I2C_ENABLE    = 0,
  LIS2DW12_I2C_DISABLE   = 1,
} lis2dw12_i2c_disable_t;
int32_t lis2dw12_i2c_interface_set(lis2dw12_ctx_t *ctx,
                                   lis2dw12_i2c_disable_t val);
int32_t lis2dw12_i2c_interface_get(lis2dw12_ctx_t *ctx,
                                   lis2dw12_i2c_disable_t *val);

typedef enum {
  LIS2DW12_PULL_UP_CONNECT     = 0,
  LIS2DW12_PULL_UP_DISCONNECT  = 1,
} lis2dw12_cs_pu_disc_t;
int32_t lis2dw12_cs_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_cs_pu_disc_t val);
int32_t lis2dw12_cs_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_cs_pu_disc_t *val);

typedef enum {
  LIS2DW12_ACTIVE_HIGH  = 0,
  LIS2DW12_ACTIVE_LOW   = 1,
} lis2dw12_h_lactive_t;
int32_t lis2dw12_pin_polarity_set(lis2dw12_ctx_t *ctx,
                                  lis2dw12_h_lactive_t val);
int32_t lis2dw12_pin_polarity_get(lis2dw12_ctx_t *ctx,
                                  lis2dw12_h_lactive_t *val);

typedef enum {
  LIS2DW12_INT_PULSED   = 0,
  LIS2DW12_INT_LATCHED  = 1,
} lis2dw12_lir_t;
int32_t lis2dw12_int_notification_set(lis2dw12_ctx_t *ctx,
                                      lis2dw12_lir_t val);
int32_t lis2dw12_int_notification_get(lis2dw12_ctx_t *ctx,
                                      lis2dw12_lir_t *val);

typedef enum {
  LIS2DW12_PUSH_PULL   = 0,
  LIS2DW12_OPEN_DRAIN  = 1,
} lis2dw12_pp_od_t;
int32_t lis2dw12_pin_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_pp_od_t val);
int32_t lis2dw12_pin_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_pp_od_t *val);

int32_t lis2dw12_pin_int1_route_set(lis2dw12_ctx_t *ctx,
                                    lis2dw12_ctrl4_int1_pad_ctrl_t *val);
int32_t lis2dw12_pin_int1_route_get(lis2dw12_ctx_t *ctx,
                                    lis2dw12_ctrl4_int1_pad_ctrl_t *val);

int32_t lis2dw12_pin_int2_route_set(lis2dw12_ctx_t *ctx,
                                    lis2dw12_ctrl5_int2_pad_ctrl_t *val);
int32_t lis2dw12_pin_int2_route_get(lis2dw12_ctx_t *ctx,
                                    lis2dw12_ctrl5_int2_pad_ctrl_t *val);

int32_t lis2dw12_all_on_int1_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_all_on_int1_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_wkup_threshold_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_wkup_threshold_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_wkup_dur_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_wkup_dur_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2DW12_HP_FEED           = 0,
  LIS2DW12_USER_OFFSET_FEED  = 1,
} lis2dw12_usr_off_on_wu_t;
int32_t lis2dw12_wkup_feed_data_set(lis2dw12_ctx_t *ctx,
                                    lis2dw12_usr_off_on_wu_t val);
int32_t lis2dw12_wkup_feed_data_get(lis2dw12_ctx_t *ctx,
                                    lis2dw12_usr_off_on_wu_t *val);

typedef enum {
  LIS2DW12_NO_DETECTION        = 0,
  LIS2DW12_DETECT_ACT_INACT    = 1,
  LIS2DW12_DETECT_STAT_MOTION  = 3,
} lis2dw12_sleep_on_t;
int32_t lis2dw12_act_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_sleep_on_t val);
int32_t lis2dw12_act_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_sleep_on_t *val);

int32_t lis2dw12_act_sleep_dur_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_act_sleep_dur_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_tap_threshold_x_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_threshold_x_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_tap_threshold_y_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_threshold_y_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2DW12_XYZ    = 0,
  LIS2DW12_YXZ    = 1,
  LIS2DW12_XZY    = 2,
  LIS2DW12_ZYX    = 3,
  LIS2DW12_YZX    = 5,
  LIS2DW12_ZXY    = 6,
} lis2dw12_tap_prior_t;
int32_t lis2dw12_tap_axis_priority_set(lis2dw12_ctx_t *ctx,
                                       lis2dw12_tap_prior_t val);
int32_t lis2dw12_tap_axis_priority_get(lis2dw12_ctx_t *ctx,
                                       lis2dw12_tap_prior_t *val);

int32_t lis2dw12_tap_threshold_z_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_threshold_z_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_tap_detection_on_z_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_detection_on_z_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_tap_detection_on_y_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_detection_on_y_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_tap_detection_on_x_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_detection_on_x_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_tap_shock_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_shock_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_tap_quiet_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_quiet_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_tap_dur_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_tap_dur_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2DW12_ONLY_SINGLE          = 0,
  LIS2DW12_BOTH_SINGLE_DOUBLE   = 1,
} lis2dw12_single_double_tap_t;
int32_t lis2dw12_tap_mode_set(lis2dw12_ctx_t *ctx,
                              lis2dw12_single_double_tap_t val);
int32_t lis2dw12_tap_mode_get(lis2dw12_ctx_t *ctx,
                              lis2dw12_single_double_tap_t *val);

int32_t lis2dw12_tap_src_get(lis2dw12_ctx_t *ctx, lis2dw12_tap_src_t *val);

int32_t lis2dw12_6d_threshold_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_6d_threshold_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_4d_mode_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_4d_mode_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_6d_src_get(lis2dw12_ctx_t *ctx, lis2dw12_sixd_src_t *val);

typedef enum {
  LIS2DW12_ODR_DIV_2_FEED   = 0,
  LIS2DW12_LPF2_FEED        = 1,
} lis2dw12_lpass_on6d_t;
int32_t lis2dw12_6d_feed_data_set(lis2dw12_ctx_t *ctx,
                                  lis2dw12_lpass_on6d_t val);
int32_t lis2dw12_6d_feed_data_get(lis2dw12_ctx_t *ctx,
                                  lis2dw12_lpass_on6d_t *val);

int32_t lis2dw12_ff_dur_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_ff_dur_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2DW12_FF_TSH_5LSb_FS2g  = 0,
  LIS2DW12_FF_TSH_7LSb_FS2g  = 1,
  LIS2DW12_FF_TSH_8LSb_FS2g  = 2,
  LIS2DW12_FF_TSH_10LSb_FS2g = 3,
  LIS2DW12_FF_TSH_11LSb_FS2g = 4,
  LIS2DW12_FF_TSH_13LSb_FS2g = 5,
  LIS2DW12_FF_TSH_15LSb_FS2g = 6,
  LIS2DW12_FF_TSH_16LSb_FS2g = 7,
} lis2dw12_ff_ths_t;
int32_t lis2dw12_ff_threshold_set(lis2dw12_ctx_t *ctx,
                                  lis2dw12_ff_ths_t val);
int32_t lis2dw12_ff_threshold_get(lis2dw12_ctx_t *ctx,
                                  lis2dw12_ff_ths_t *val);

int32_t lis2dw12_fifo_watermark_set(lis2dw12_ctx_t *ctx, uint8_t val);
int32_t lis2dw12_fifo_watermark_get(lis2dw12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2DW12_BYPASS_MODE             = 0,
  LIS2DW12_FIFO_MODE               = 1,
  LIS2DW12_STREAM_TO_FIFO_MODE     = 3,
  LIS2DW12_BYPASS_TO_STREAM_MODE   = 4,
  LIS2DW12_STREAM_MODE             = 6,
} lis2dw12_fmode_t;
int32_t lis2dw12_fifo_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_fmode_t val);
int32_t lis2dw12_fifo_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_fmode_t *val);

int32_t lis2dw12_fifo_data_level_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_fifo_ovr_flag_get(lis2dw12_ctx_t *ctx, uint8_t *val);

int32_t lis2dw12_fifo_wtm_flag_get(lis2dw12_ctx_t *ctx, uint8_t *val);

/**
  * @}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /*LIS2DW12_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
