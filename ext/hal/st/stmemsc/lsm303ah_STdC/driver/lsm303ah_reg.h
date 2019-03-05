/*
 ******************************************************************************
 * @file    lsm303ah_reg.h
 * @author  MEMS Software Solution Team
 * @date    19-December-2017
 * @brief   This file contains all the functions prototypes for the
 *          lsm303ah_reg.c driver.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LSM303AH_DRIVER__H
#define __LSM303AH_DRIVER__H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LSM303AH
  * @{
  *
  */

/** @defgroup LSM303AH_sensors_common_types
  * @{
  *
  */

#ifndef MEMS_SHARED_TYPES
#define MEMS_SHARED_TYPES

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

/** @defgroup lsm303ah_interface
  * @{
  */

typedef int32_t (*lsm303ah_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lsm303ah_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lsm303ah_write_ptr  write_reg;
  lsm303ah_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lsm303ah_ctx_t;

/**
  * @}
  */

/** @defgroup lsm303ah_Infos
  * @{
  */
  /** I2C Device Address 8 bit format **/
#define LSM303AH_I2C_ADD_XL     0x3B
#define LSM303AH_I2C_ADD_MG       0x3D

/** Device Identification (Who am I) **/
#define LSM303AH_ID_XL            0x43
#define LSM303AH_ID_MG            0x40

/**
  * @}
  */

/**
  * @defgroup lsm303ah_Sensitivity
  * @{
  */

#define LSM303AH_FROM_FS_2g_TO_mg(lsb)    (float)(lsb *  61.0f) / 1000.0f
#define LSM303AH_FROM_FS_4g_TO_mg(lsb)    (float)(lsb * 122.0f) / 1000.0f
#define LSM303AH_FROM_FS_8g_TO_mg(lsb)    (float)(lsb * 244.0f) / 1000.0f
#define LSM303AH_FROM_FS_16g_TO_mg(lsb)   (float)(lsb * 488.0f) / 1000.0f

#define LSM303AH_FROM_LSB_TO_mG(lsb)      (float)(lsb * 1.5f)

#define LSM303AH_FROM_LSB_TO_degC(lsb)  ((float)((int16_t)lsb>>8)*1.0f + 25.0f)

/**
  * @}
  */

#define LSM303AH_MODULE_8BIT_A           0x0C
#define LSM303AH_WHO_AM_I_A              0x0F
#define LSM303AH_CTRL1_A                 0x20
typedef struct {
  uint8_t bdu                 : 1;
  uint8_t hf_odr              : 1;
  uint8_t fs                  : 2;
  uint8_t odr                 : 4;
} lsm303ah_ctrl1_a_t;

#define LSM303AH_CTRL2_A                 0x21
typedef struct {
  uint8_t sim                 : 1;
  uint8_t i2c_disable         : 1;
  uint8_t if_add_inc          : 1;
  uint8_t fds_slope           : 1;
  uint8_t func_cfg_en         : 1;
  uint8_t not_used_01         : 1;
  uint8_t soft_reset          : 1;
  uint8_t boot                : 1;
} lsm303ah_ctrl2_a_t;

#define LSM303AH_CTRL3_A                 0x22
typedef struct {
  uint8_t pp_od               : 1;
  uint8_t h_lactive           : 1;
  uint8_t lir                 : 1;
  uint8_t tap_z_en            : 1;
  uint8_t tap_y_en            : 1;
  uint8_t tap_x_en            : 1;
  uint8_t st                  : 2;
} lsm303ah_ctrl3_a_t;

#define LSM303AH_CTRL4_A                 0x23
typedef struct {
  uint8_t int1_drdy           : 1;
  uint8_t int1_fth            : 1;
  uint8_t int1_6d             : 1;
  uint8_t int1_tap            : 1;
  uint8_t int1_ff             : 1;
  uint8_t int1_wu             : 1;
  uint8_t int1_s_tap          : 1;
  uint8_t not_used_01    : 1;
} lsm303ah_ctrl4_a_t;

#define LSM303AH_CTRL5_A                 0x24
typedef struct {
  uint8_t int2_drdy           : 1;
  uint8_t int2_fth            : 1;
  uint8_t int2_step           : 1;
  uint8_t int2_sig_mot        : 1;
  uint8_t int2_tilt           : 1;
  uint8_t int2_on_int1        : 1;
  uint8_t int2_boot           : 1;
  uint8_t drdy_pulsed         : 1;
} lsm303ah_ctrl5_a_t;

#define LSM303AH_FIFO_CTRL_A             0x25
typedef struct {
  uint8_t if_cs_pu_dis        : 1;
  uint8_t not_used_01         : 2;
  uint8_t module_to_fifo      : 1;
  uint8_t int2_step_count_ov  : 1;
  uint8_t fmode               : 3;
} lsm303ah_fifo_ctrl_a_t;

#define LSM303AH_OUT_T_A                 0x26
#define LSM303AH_STATUS_A                0x27
typedef struct {
  uint8_t drdy                : 1;
  uint8_t ff_ia               : 1;
  uint8_t _6d_ia              : 1;
  uint8_t single_tap          : 1;
  uint8_t double_tap          : 1;
  uint8_t sleep_state         : 1;
  uint8_t wu_ia               : 1;
  uint8_t fifo_ths            : 1;
} lsm303ah_status_a_t;

#define LSM303AH_OUT_X_L_A               0x28
#define LSM303AH_OUT_X_H_A               0x29
#define LSM303AH_OUT_Y_L_A               0x2A
#define LSM303AH_OUT_Y_H_A               0x2B
#define LSM303AH_OUT_Z_L_A               0x2C
#define LSM303AH_OUT_Z_H_A               0x2D
#define LSM303AH_FIFO_THS_A              0x2E
#define LSM303AH_FIFO_SRC_A              0x2F
typedef struct {
  uint8_t not_used_01         : 5;
  uint8_t diff                : 1;
  uint8_t fifo_ovr            : 1;
  uint8_t fth                 : 1;
} lsm303ah_fifo_src_a_t;

#define LSM303AH_FIFO_SAMPLES_A          0x30
#define LSM303AH_TAP_6D_THS_A            0x31
typedef struct {
  uint8_t tap_ths             : 5;
  uint8_t _6d_ths             : 2;
  uint8_t _4d_en              : 1;
} lsm303ah_tap_6d_ths_a_t;

#define LSM303AH_INT_DUR_A               0x32
typedef struct {
  uint8_t shock               : 2;
  uint8_t quiet               : 2;
  uint8_t lat                 : 4;
} lsm303ah_int_dur_a_t;

#define LSM303AH_WAKE_UP_THS_A           0x33
typedef struct {
  uint8_t wu_ths              : 6;
  uint8_t sleep_on            : 1;
  uint8_t single_double_tap   : 1;
} lsm303ah_wake_up_ths_a_t;

#define LSM303AH_WAKE_UP_DUR_A           0x34
typedef struct {
  uint8_t sleep_dur           : 4;
  uint8_t int1_fss7           : 1;
  uint8_t wu_dur              : 2;
  uint8_t ff_dur              : 1;
} lsm303ah_wake_up_dur_a_t;

#define LSM303AH_FREE_FALL_A             0x35
typedef struct {
  uint8_t ff_ths              : 3;
  uint8_t ff_dur              : 5;
} lsm303ah_free_fall_a_t;

#define LSM303AH_STATUS_DUP_A            0x36
typedef struct {
  uint8_t drdy                : 1;
  uint8_t ff_ia               : 1;
  uint8_t _6d_ia              : 1;
  uint8_t single_tap          : 1;
  uint8_t double_tap          : 1;
  uint8_t sleep_state         : 1;
  uint8_t wu_ia               : 1;
  uint8_t ovr                 : 1;
} lsm303ah_status_dup_a_t;

#define LSM303AH_WAKE_UP_SRC_A           0x37
typedef struct {
  uint8_t z_wu                : 1;
  uint8_t y_wu                : 1;
  uint8_t x_wu                : 1;
  uint8_t wu_ia               : 1;
  uint8_t sleep_state_ia      : 1;
  uint8_t ff_ia               : 1;
  uint8_t not_used_01         : 2;
} lsm303ah_wake_up_src_a_t;

#define LSM303AH_TAP_SRC_A               0x38
typedef struct {
  uint8_t z_tap               : 1;
  uint8_t y_tap               : 1;
  uint8_t x_tap               : 1;
  uint8_t tap_sign            : 1;
  uint8_t double_tap          : 1;
  uint8_t single_tap          : 1;
  uint8_t tap_ia              : 1;
  uint8_t not_used_01         : 1;
} lsm303ah_tap_src_a_t;

#define LSM303AH_6D_SRC_A                0x39
typedef struct {
  uint8_t xl                  : 1;
  uint8_t xh                  : 1;
  uint8_t yl                  : 1;
  uint8_t yh                  : 1;
  uint8_t zl                  : 1;
  uint8_t zh                  : 1;
  uint8_t _6d_ia              : 1;
  uint8_t not_used_01         : 1;
} lsm303ah_6d_src_a_t;

#define LSM303AH_STEP_COUNTER_MINTHS_A   0x3A
typedef struct {
  uint8_t sc_mths             : 6;
  uint8_t pedo4g              : 1;
  uint8_t rst_nstep           : 1;
} lsm303ah_step_counter_minths_a_t;

#define LSM303AH_STEP_COUNTER_L_A        0x3B
#define LSM303AH_STEP_COUNTER_H_A        0x3C
#define LSM303AH_FUNC_CK_GATE_A          0x3D
typedef struct {
  uint8_t ck_gate_func        : 1;
  uint8_t step_detect         : 1;
  uint8_t rst_pedo            : 1;
  uint8_t rst_sign_mot        : 1;
  uint8_t sig_mot_detect      : 1;
  uint8_t fs_src              : 2;
  uint8_t tilt_int            : 1;
} lsm303ah_func_ck_gate_a_t;

#define LSM303AH_FUNC_SRC_A              0x3E
typedef struct {
  uint8_t not_used_01         : 1;
  uint8_t module_ready        : 1;
  uint8_t rst_tilt            : 1;
  uint8_t not_used_02         : 5;
} lsm303ah_func_src_a_t;

#define LSM303AH_FUNC_CTRL_A             0x3F
typedef struct {
  uint8_t step_cnt_on         : 1;
  uint8_t sign_mot_on         : 1;
  uint8_t not_used_01         : 2;
  uint8_t tilt_on             : 1;
  uint8_t module_on           : 1;
  uint8_t not_used_02         : 2;
} lsm303ah_func_ctrl_a_t;

#define LSM303AH_PEDO_DEB_REG_A          0x2B
typedef struct {
  uint8_t deb_step            : 3;
  uint8_t deb_time            : 5;
} lsm303ah_pedo_deb_reg_a_t;

#define LSM303AH_SM_THS_A                0x34
typedef struct {
  uint8_t sm_ths              : 8;
} lsm303ah_sm_ths_a_t;

#define LSM303AH_STEP_COUNT_DELTA_A      0x3A
typedef struct {
  uint8_t step_count_d        : 8;
} lsm303ah_step_count_delta_a_t;

#define LSM303AH_CTRL2_ADV_A             0x3F
typedef struct {
  uint8_t sim                 : 1;
  uint8_t i2c_disable         : 1;
  uint8_t if_add_inc          : 1;
  uint8_t fds_slope           : 1;
  uint8_t func_cfg_en         : 1;
  uint8_t not_used_01         : 1;
  uint8_t soft_reset          : 1;
  uint8_t boot                : 1;
} lsm303ah_ctrl2_adv_a_t;

#define LSM303AH_OFFSET_X_REG_L_M          0x45
#define LSM303AH_OFFSET_X_REG_H_M          0x46
#define LSM303AH_OFFSET_Y_REG_L_M          0x47
#define LSM303AH_OFFSET_Y_REG_H_M          0x48
#define LSM303AH_OFFSET_Z_REG_L_M          0x49
#define LSM303AH_OFFSET_Z_REG_H_M          0x4A
#define LSM303AH_WHO_AM_I_M                0x4F
#define LSM303AH_CFG_REG_A_M               0x60
typedef struct {
  uint8_t md                     : 2;
  uint8_t odr                    : 2;
  uint8_t lp                     : 1;
  uint8_t soft_rst               : 1;
  uint8_t reboot                 : 1;
  uint8_t comp_temp_en           : 1;
} lsm303ah_cfg_reg_a_m_t;

#define LSM303AH_CFG_REG_B_M               0x61
typedef struct {
  uint8_t lpf                    : 1;
  uint8_t set_rst                : 2; /* off_canc + set_freq */
  uint8_t int_on_dataoff         : 1;
  uint8_t off_canc_one_shot      : 1;
  uint8_t not_used_01            : 3;
} lsm303ah_cfg_reg_b_m_t;

#define LSM303AH_CFG_REG_C_M               0x62
typedef struct {
  uint8_t int_mag                : 1;
  uint8_t self_test              : 1;
  uint8_t not_used_01            : 1;
  uint8_t ble                    : 1;
  uint8_t bdu                    : 1;
  uint8_t i2c_dis                : 1;
  uint8_t int_mag_pin            : 1;
  uint8_t not_used_02            : 1;
} lsm303ah_cfg_reg_c_m_t;

#define LSM303AH_INT_CRTL_REG_M            0x63
typedef struct {
  uint8_t ien                    : 1;
  uint8_t iel                    : 1;
  uint8_t iea                    : 1;
  uint8_t not_used_01            : 2;
  uint8_t zien                   : 1;
  uint8_t yien                   : 1;
  uint8_t xien                   : 1;
} lsm303ah_int_crtl_reg_m_t;

#define LSM303AH_INT_SOURCE_REG_M          0x64
typedef struct {
  uint8_t _int                    : 1;
  uint8_t mroi                   : 1;
  uint8_t n_th_s_z               : 1;
  uint8_t n_th_s_y               : 1;
  uint8_t n_th_s_x               : 1;
  uint8_t p_th_s_z               : 1;
  uint8_t p_th_s_y               : 1;
  uint8_t p_th_s_x               : 1;
} lsm303ah_int_source_reg_m_t;

#define LSM303AH_INT_THS_L_REG_M           0x65
#define LSM303AH_INT_THS_H_REG_M           0x66
#define LSM303AH_STATUS_REG_M              0x67
typedef struct {
  uint8_t xda                    : 1;
  uint8_t yda                    : 1;
  uint8_t zda                    : 1;
  uint8_t zyxda                  : 1;
  uint8_t _xor                   : 1;
  uint8_t yor                    : 1;
  uint8_t zor                    : 1;
  uint8_t zyxor                  : 1;
} lsm303ah_status_reg_m_t;

#define LSM303AH_OUTX_L_REG_M              0x68
#define LSM303AH_OUTX_H_REG_M              0x69
#define LSM303AH_OUTY_L_REG_M              0x6A
#define LSM303AH_OUTY_H_REG_M              0x6B
#define LSM303AH_OUTZ_L_REG_M              0x6C
#define LSM303AH_OUTZ_H_REG_M              0x6D

typedef union{
  lsm303ah_ctrl1_a_t                      ctrl1_a;
  lsm303ah_ctrl2_a_t                      ctrl2_a;
  lsm303ah_ctrl3_a_t                      ctrl3_a;
  lsm303ah_ctrl4_a_t                      ctrl4_a;
  lsm303ah_ctrl5_a_t                      ctrl5_a;
  lsm303ah_fifo_ctrl_a_t                  fifo_ctrl_a;
  lsm303ah_status_a_t                     status_a;
  lsm303ah_fifo_src_a_t                   fifo_src_a;
  lsm303ah_tap_6d_ths_a_t                 tap_6d_ths_a;
  lsm303ah_int_dur_a_t                    int_dur_a;
  lsm303ah_wake_up_ths_a_t                wake_up_ths_a;
  lsm303ah_wake_up_dur_a_t                wake_up_dur_a;
  lsm303ah_free_fall_a_t                  free_fall_a;
  lsm303ah_status_dup_a_t                 status_dup_a;
  lsm303ah_wake_up_src_a_t                wake_up_src_a;
  lsm303ah_tap_src_a_t                    tap_src_a;
  lsm303ah_6d_src_a_t                     _6d_src_a;
  lsm303ah_step_counter_minths_a_t        step_counter_minths_a;
  lsm303ah_func_ck_gate_a_t               func_ck_gate_a;
  lsm303ah_func_src_a_t                   func_src_a;
  lsm303ah_func_ctrl_a_t                  func_ctrl_a;
  lsm303ah_pedo_deb_reg_a_t               pedo_deb_reg_a;
  lsm303ah_sm_ths_a_t                     sm_ths_a;
  lsm303ah_step_count_delta_a_t           step_count_delta_a;
  lsm303ah_ctrl2_adv_a_t                  ctrl2_adv_a;
  lsm303ah_cfg_reg_a_m_t                  cfg_reg_a_m;
  lsm303ah_cfg_reg_b_m_t                  cfg_reg_b_m;
  lsm303ah_cfg_reg_c_m_t                  cfg_reg_c_m;
  lsm303ah_int_crtl_reg_m_t               int_crtl_reg_m;
  lsm303ah_int_source_reg_m_t             int_source_reg_m;
  lsm303ah_status_reg_m_t                 status_reg_m;
  bitwise_t                               bitwise;
  uint8_t                                 byte;
} lsm303ah_reg_t;
int32_t lsm303ah_read_reg(lsm303ah_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);
int32_t lsm303ah_write_reg(lsm303ah_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);

typedef union {
  struct {
  lsm303ah_fifo_src_a_t       fifo_src_a;
  lsm303ah_status_dup_a_t     status_dup_a;
  lsm303ah_wake_up_src_a_t    wake_up_src_a;
  lsm303ah_tap_src_a_t        tap_src_a;
  lsm303ah_6d_src_a_t         _6d_src_a;
  lsm303ah_func_ck_gate_a_t   func_ck_gate_a;
  lsm303ah_func_src_a_t       func_src_a;
  } reg;
  uint8_t byte[7];
} lsm303ah_xl_all_sources_t;
int32_t lsm303ah_xl_all_sources_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_all_sources_t *val);


int32_t lsm303ah_xl_block_data_update_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_block_data_update_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_mg_block_data_update_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_mg_block_data_update_get(lsm303ah_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM303AH_MG_LSB_AT_LOW_ADD  = 0,
  LSM303AH_MG_MSB_AT_LOW_ADD  = 1,
} lsm303ah_mg_ble_t;
int32_t lsm303ah_mg_data_format_set(lsm303ah_ctx_t *ctx,
                                    lsm303ah_mg_ble_t val);
int32_t lsm303ah_mg_data_format_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_mg_ble_t *val);

typedef enum {
  LSM303AH_XL_2g = 0,
  LSM303AH_XL_16g = 1,
  LSM303AH_XL_4g = 2,
  LSM303AH_XL_8g = 3,
} lsm303ah_xl_fs_t;
int32_t lsm303ah_xl_full_scale_set(lsm303ah_ctx_t *ctx,
                                   lsm303ah_xl_fs_t val);
int32_t lsm303ah_xl_full_scale_get(lsm303ah_ctx_t *ctx,
                                   lsm303ah_xl_fs_t *val);

typedef enum {
  LSM303AH_XL_ODR_OFF         = 0x00,
  LSM303AH_XL_ODR_1Hz_LP      = 0x08,
  LSM303AH_XL_ODR_12Hz5_LP    = 0x09,
  LSM303AH_XL_ODR_25Hz_LP     = 0x0A,
  LSM303AH_XL_ODR_50Hz_LP     = 0x0B,
  LSM303AH_XL_ODR_100Hz_LP    = 0x0C,
  LSM303AH_XL_ODR_200Hz_LP    = 0x0D,
  LSM303AH_XL_ODR_400Hz_LP    = 0x0E,
  LSM303AH_XL_ODR_800Hz_LP    = 0x0F,
  LSM303AH_XL_ODR_12Hz5_HR    = 0x01,
  LSM303AH_XL_ODR_25Hz_HR     = 0x02,
  LSM303AH_XL_ODR_50Hz_HR     = 0x03,
  LSM303AH_XL_ODR_100Hz_HR    = 0x04,
  LSM303AH_XL_ODR_200Hz_HR    = 0x05,
  LSM303AH_XL_ODR_400Hz_HR    = 0x06,
  LSM303AH_XL_ODR_800Hz_HR    = 0x07,
  LSM303AH_XL_ODR_1k6Hz_HF    = 0x15,
  LSM303AH_XL_ODR_3k2Hz_HF    = 0x16,
  LSM303AH_XL_ODR_6k4Hz_HF    = 0x17,
} lsm303ah_xl_odr_t;
int32_t lsm303ah_xl_data_rate_set(lsm303ah_ctx_t *ctx, lsm303ah_xl_odr_t val);
int32_t lsm303ah_xl_data_rate_get(lsm303ah_ctx_t *ctx, lsm303ah_xl_odr_t *val);

int32_t lsm303ah_xl_status_reg_get(lsm303ah_ctx_t *ctx,
                                   lsm303ah_status_a_t *val);

int32_t lsm303ah_mg_status_get(lsm303ah_ctx_t *ctx,
                               lsm303ah_status_reg_m_t *val);

int32_t lsm303ah_xl_flag_data_ready_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_mg_data_ready_get(lsm303ah_ctx_t *ctx, uint8_t *val);
int32_t lsm303ah_mg_data_ovr_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_mg_user_offset_set(lsm303ah_ctx_t *ctx, uint8_t *buff);
int32_t lsm303ah_mg_user_offset_get(lsm303ah_ctx_t *ctx, uint8_t *buff);

typedef enum {
  LSM303AH_MG_CONTINUOUS_MODE  = 0,
  LSM303AH_MG_SINGLE_TRIGGER   = 1,
  LSM303AH_MG_POWER_DOWN       = 2,
} lsm303ah_mg_md_t;
int32_t lsm303ah_mg_operating_mode_set(lsm303ah_ctx_t *ctx,
                                       lsm303ah_mg_md_t val);
int32_t lsm303ah_mg_operating_mode_get(lsm303ah_ctx_t *ctx,
                                       lsm303ah_mg_md_t *val);

typedef enum {
  LSM303AH_MG_ODR_10Hz   = 0,
  LSM303AH_MG_ODR_20Hz   = 1,
  LSM303AH_MG_ODR_50Hz   = 2,
  LSM303AH_MG_ODR_100Hz  = 3,
} lsm303ah_mg_odr_t;
int32_t lsm303ah_mg_data_rate_set(lsm303ah_ctx_t *ctx,
                                  lsm303ah_mg_odr_t val);
int32_t lsm303ah_mg_data_rate_get(lsm303ah_ctx_t *ctx,
                                  lsm303ah_mg_odr_t *val);

typedef enum {
  LSM303AH_MG_HIGH_RESOLUTION  = 0,
  LSM303AH_MG_LOW_POWER        = 1,
} lsm303ah_mg_lp_t;
int32_t lsm303ah_mg_power_mode_set(lsm303ah_ctx_t *ctx,
                                   lsm303ah_mg_lp_t val);
int32_t lsm303ah_mg_power_mode_get(lsm303ah_ctx_t *ctx,
                                   lsm303ah_mg_lp_t *val);

int32_t lsm303ah_mg_offset_temp_comp_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_mg_offset_temp_comp_get(lsm303ah_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM303AH_MG_SET_SENS_ODR_DIV_63        = 0,
  LSM303AH_MG_SENS_OFF_CANC_EVERY_ODR    = 1,
  LSM303AH_MG_SET_SENS_ONLY_AT_POWER_ON  = 2,
} lsm303ah_mg_set_rst_t;
int32_t lsm303ah_mg_set_rst_mode_set(lsm303ah_ctx_t *ctx,
                                 lsm303ah_mg_set_rst_t val);
int32_t lsm303ah_mg_set_rst_mode_get(lsm303ah_ctx_t *ctx,
                                 lsm303ah_mg_set_rst_t *val);

int32_t lsm303ah_mg_set_rst_sensor_single_set(lsm303ah_ctx_t *ctx,
                                          uint8_t val);
int32_t lsm303ah_mg_set_rst_sensor_single_get(lsm303ah_ctx_t *ctx,
                                          uint8_t *val);

int32_t lsm303ah_acceleration_module_raw_get(lsm303ah_ctx_t *ctx,
                                             uint8_t *buff);

int32_t lsm303ah_magnetic_raw_get(lsm303ah_ctx_t *ctx, uint8_t *buff);

int32_t lsm303ah_xl_temperature_raw_get(lsm303ah_ctx_t *ctx, uint8_t *buff);

int32_t lsm303ah_acceleration_raw_get(lsm303ah_ctx_t *ctx, uint8_t *buff);

int32_t lsm303ah_number_of_steps_get(lsm303ah_ctx_t *ctx, uint8_t *buff);

int32_t lsm303ah_xl_device_id_get(lsm303ah_ctx_t *ctx, uint8_t *buff);

int32_t lsm303ah_mg_device_id_get(lsm303ah_ctx_t *ctx, uint8_t *buff);

int32_t lsm303ah_xl_auto_increment_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_auto_increment_get(lsm303ah_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM303AH_XL_USER_BANK   = 0,
  LSM303AH_XL_ADV_BANK    = 1,
} lsm303ah_xl_func_cfg_en_t;
int32_t lsm303ah_xl_mem_bank_set(lsm303ah_ctx_t *ctx,
                              lsm303ah_xl_func_cfg_en_t val);

int32_t lsm303ah_xl_reset_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_reset_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_mg_reset_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_mg_reset_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_boot_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_boot_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_mg_boot_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_mg_boot_get(lsm303ah_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM303AH_XL_ST_DISABLE     = 0,
  LSM303AH_XL_ST_POSITIVE    = 1,
  LSM303AH_XL_ST_NEGATIVE    = 2,
} lsm303ah_xl_st_t;
int32_t lsm303ah_xl_self_test_set(lsm303ah_ctx_t *ctx, lsm303ah_xl_st_t val);
int32_t lsm303ah_xl_self_test_get(lsm303ah_ctx_t *ctx, lsm303ah_xl_st_t *val);

int32_t lsm303ah_mg_self_test_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_mg_self_test_get(lsm303ah_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM303AH_XL_DRDY_LATCHED   = 0,
  LSM303AH_XL_DRDY_PULSED    = 1,
} lsm303ah_xl_drdy_pulsed_t;
int32_t lsm303ah_xl_data_ready_mode_set(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_drdy_pulsed_t val);
int32_t lsm303ah_xl_data_ready_mode_get(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_drdy_pulsed_t *val);

typedef enum {
  LSM303AH_XL_HP_INTERNAL_ONLY  = 0,
  LSM303AH_XL_HP_ON_OUTPUTS     = 1,
} lsm303ah_xl_fds_slope_t;
int32_t lsm303ah_xl_hp_path_set(lsm303ah_ctx_t *ctx,
                                lsm303ah_xl_fds_slope_t val);
int32_t lsm303ah_xl_hp_path_get(lsm303ah_ctx_t *ctx,
                                lsm303ah_xl_fds_slope_t *val);

typedef enum {
  LSM303AH_MG_ODR_DIV_2  = 0,
  LSM303AH_MG_ODR_DIV_4  = 1,
} lsm303ah_mg_lpf_t;
int32_t lsm303ah_mg_low_pass_bandwidth_set(lsm303ah_ctx_t *ctx,
                                       lsm303ah_mg_lpf_t val);
int32_t lsm303ah_mg_low_pass_bandwidth_get(lsm303ah_ctx_t *ctx,
                                       lsm303ah_mg_lpf_t *val);

typedef enum {
  LSM303AH_XL_SPI_4_WIRE   = 0,
  LSM303AH_XL_SPI_3_WIRE   = 1,
} lsm303ah_xl_sim_t;
int32_t lsm303ah_xl_spi_mode_set(lsm303ah_ctx_t *ctx, lsm303ah_xl_sim_t val);
int32_t lsm303ah_xl_spi_mode_get(lsm303ah_ctx_t *ctx, lsm303ah_xl_sim_t *val);

typedef enum {
  LSM303AH_XL_I2C_ENABLE   = 0,
  LSM303AH_XL_I2C_DISABLE  = 1,
} lsm303ah_xl_i2c_disable_t;
int32_t lsm303ah_xl_i2c_interface_set(lsm303ah_ctx_t *ctx,
                                   lsm303ah_xl_i2c_disable_t val);
int32_t lsm303ah_xl_i2c_interface_get(lsm303ah_ctx_t *ctx,
                                   lsm303ah_xl_i2c_disable_t *val);

typedef enum {
  LSM303AH_MG_I2C_ENABLE   = 0,
  LSM303AH_MG_I2C_DISABLE  = 1,
} lsm303ah_mg_i2c_dis_t;
int32_t lsm303ah_mg_i2c_interface_set(lsm303ah_ctx_t *ctx,
                                  lsm303ah_mg_i2c_dis_t val);
int32_t lsm303ah_mg_i2c_interface_get(lsm303ah_ctx_t *ctx,
                                  lsm303ah_mg_i2c_dis_t *val);

typedef enum {
  LSM303AH_XL_PULL_UP_CONNECTED     = 0,
  LSM303AH_XL_PULL_UP_DISCONNECTED  = 1,
} lsm303ah_xl_if_cs_pu_dis_t;
int32_t lsm303ah_xl_cs_mode_set(lsm303ah_ctx_t *ctx,
                             lsm303ah_xl_if_cs_pu_dis_t val);
int32_t lsm303ah_xl_cs_mode_get(lsm303ah_ctx_t *ctx,
                             lsm303ah_xl_if_cs_pu_dis_t *val);

typedef enum {
  LSM303AH_XL_PUSH_PULL   = 0,
  LSM303AH_XL_OPEN_DRAIN  = 1,
} lsm303ah_xl_pp_od_t;
int32_t lsm303ah_xl_pin_mode_set(lsm303ah_ctx_t *ctx,
                                 lsm303ah_xl_pp_od_t val);
int32_t lsm303ah_xl_pin_mode_get(lsm303ah_ctx_t *ctx,
                                 lsm303ah_xl_pp_od_t *val);

typedef enum {
  LSM303AH_XL_ACTIVE_HIGH  = 0,
  LSM303AH_XL_ACTIVE_LOW   = 1,
} lsm303ah_xl_h_lactive_t;
int32_t lsm303ah_xl_pin_polarity_set(lsm303ah_ctx_t *ctx,
                                  lsm303ah_xl_h_lactive_t val);
int32_t lsm303ah_xl_pin_polarity_get(lsm303ah_ctx_t *ctx,
                                  lsm303ah_xl_h_lactive_t *val);

typedef enum {
  LSM303AH_XL_INT_PULSED   = 0,
  LSM303AH_XL_INT_LATCHED  = 1,
} lsm303ah_xl_lir_t;
int32_t lsm303ah_xl_int_notification_set(lsm303ah_ctx_t *ctx,
                                      lsm303ah_xl_lir_t val);
int32_t lsm303ah_xl_int_notification_get(lsm303ah_ctx_t *ctx,
                                      lsm303ah_xl_lir_t *val);

typedef struct{
  uint8_t int1_drdy               : 1;
  uint8_t int1_fth                : 1;
  uint8_t int1_6d                 : 1;
  uint8_t int1_tap                : 1;
  uint8_t int1_ff                 : 1;
  uint8_t int1_wu                 : 1;
  uint8_t int1_s_tap              : 1;
  uint8_t int1_fss7               : 1;
} lsm303ah_xl_pin_int1_route_t;
int32_t lsm303ah_xl_pin_int1_route_set(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_pin_int1_route_t val);
int32_t lsm303ah_xl_pin_int1_route_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_pin_int1_route_t *val);

typedef struct{
  uint8_t int2_boot               : 1;
  uint8_t int2_tilt               : 1;
  uint8_t int2_sig_mot            : 1;
  uint8_t int2_step               : 1;
  uint8_t int2_fth                : 1;
  uint8_t int2_drdy               : 1;
} lsm303ah_xl_pin_int2_route_t;
int32_t lsm303ah_xl_pin_int2_route_set(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_pin_int2_route_t val);
int32_t lsm303ah_xl_pin_int2_route_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_pin_int2_route_t *val);

int32_t lsm303ah_xl_all_on_int1_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_all_on_int1_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_mg_drdy_on_pin_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_mg_drdy_on_pin_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_mg_int_on_pin_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_mg_int_on_pin_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_mg_int_gen_conf_set(lsm303ah_ctx_t *ctx,
                                 lsm303ah_int_crtl_reg_m_t *val);
int32_t lsm303ah_mg_int_gen_conf_get(lsm303ah_ctx_t *ctx,
                                 lsm303ah_int_crtl_reg_m_t *val);

int32_t lsm303ah_mg_int_gen_source_get(lsm303ah_ctx_t *ctx,
                                   lsm303ah_int_source_reg_m_t *val);

int32_t lsm303ah_mg_int_gen_treshold_set(lsm303ah_ctx_t *ctx, uint8_t *buff);
int32_t lsm303ah_mg_int_gen_treshold_get(lsm303ah_ctx_t *ctx, uint8_t *buff);

typedef enum {
  LSM303AH_MG_CHECK_BEFORE  = 0,
  LSM303AH_MG_CHECK_AFTER   = 1,
} lsm303ah_mg_int_on_dataoff_t;
int32_t lsm303ah_mg_offset_int_conf_set(lsm303ah_ctx_t *ctx,
                                    lsm303ah_mg_int_on_dataoff_t val);
int32_t lsm303ah_mg_offset_int_conf_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_mg_int_on_dataoff_t *val);

int32_t lsm303ah_xl_wkup_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_wkup_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_wkup_dur_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_wkup_dur_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_sleep_mode_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_sleep_mode_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_act_sleep_dur_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_act_sleep_dur_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_tap_detection_on_z_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_tap_detection_on_z_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_tap_detection_on_y_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_tap_detection_on_y_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_tap_detection_on_x_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_tap_detection_on_x_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_tap_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_tap_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_tap_shock_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_tap_shock_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_tap_quiet_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_tap_quiet_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_tap_dur_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_tap_dur_get(lsm303ah_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM303AH_XL_ONLY_SINGLE  = 0,
  LSM303AH_XL_ONLY_DOUBLE  = 1,
} lsm303ah_xl_single_double_tap_t;
int32_t lsm303ah_xl_tap_mode_set(lsm303ah_ctx_t *ctx,
                              lsm303ah_xl_single_double_tap_t val);
int32_t lsm303ah_xl_tap_mode_get(lsm303ah_ctx_t *ctx,
                              lsm303ah_xl_single_double_tap_t *val);

int32_t lsm303ah_xl_tap_src_get(lsm303ah_ctx_t *ctx,
                                lsm303ah_tap_src_a_t *val);

typedef enum {
  LSM303AH_XL_DEG_80   = 0,
  LSM303AH_XL_DEG_70   = 1,
  LSM303AH_XL_DEG_60   = 2,
  LSM303AH_XL_DEG_50   = 3,
} lsm303ah_xl_6d_ths_t;
int32_t lsm303ah_xl_6d_threshold_set(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_6d_ths_t val);
int32_t lsm303ah_xl_6d_threshold_get(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_6d_ths_t *val);

int32_t lsm303ah_xl_4d_mode_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_4d_mode_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_6d_src_get(lsm303ah_ctx_t *ctx, lsm303ah_6d_src_a_t *val);

int32_t lsm303ah_xl_ff_dur_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_ff_dur_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_ff_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_ff_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_fifo_xl_module_batch_set(lsm303ah_ctx_t *ctx,
                                             uint8_t val);
int32_t lsm303ah_xl_fifo_xl_module_batch_get(lsm303ah_ctx_t *ctx,
                                             uint8_t *val);

typedef enum {
  LSM303AH_XL_BYPASS_MODE            = 0,
  LSM303AH_XL_FIFO_MODE              = 1,
  LSM303AH_XL_STREAM_TO_FIFO_MODE    = 3,
  LSM303AH_XL_BYPASS_TO_STREAM_MODE  = 4,
  LSM303AH_XL_STREAM_MODE            = 6,
} lsm303ah_xl_fmode_t;
int32_t lsm303ah_xl_fifo_mode_set(lsm303ah_ctx_t *ctx,
                                  lsm303ah_xl_fmode_t val);
int32_t lsm303ah_xl_fifo_mode_get(lsm303ah_ctx_t *ctx,
                                  lsm303ah_xl_fmode_t *val);

int32_t lsm303ah_xl_fifo_watermark_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_fifo_watermark_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_fifo_full_flag_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_fifo_ovr_flag_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_fifo_wtm_flag_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_fifo_data_level_get(lsm303ah_ctx_t *ctx, uint16_t *val);

int32_t lsm303ah_xl_fifo_src_get(lsm303ah_ctx_t *ctx,
                                 lsm303ah_fifo_src_a_t *val);

int32_t lsm303ah_xl_pedo_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_pedo_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val);

typedef enum {
  LSM303AH_XL_PEDO_AT_2g  = 0,
  LSM303AH_XL_PEDO_AT_4g  = 1,
} lsm303ah_xl_pedo4g_t;
int32_t lsm303ah_xl_pedo_full_scale_set(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_pedo4g_t val);
int32_t lsm303ah_xl_pedo_full_scale_get(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_pedo4g_t *val);

int32_t lsm303ah_xl_pedo_step_reset_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_pedo_step_reset_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_pedo_step_detect_flag_get(lsm303ah_ctx_t *ctx,
                                              uint8_t *val);

int32_t lsm303ah_xl_pedo_sens_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_pedo_sens_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_pedo_debounce_steps_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_pedo_debounce_steps_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_pedo_timeout_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_pedo_timeout_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_pedo_steps_period_set(lsm303ah_ctx_t *ctx, uint8_t *buff);
int32_t lsm303ah_xl_pedo_steps_period_get(lsm303ah_ctx_t *ctx, uint8_t *buff);
int32_t lsm303ah_xl_motion_data_ready_flag_get(lsm303ah_ctx_t *ctx,
                                               uint8_t *val);

int32_t lsm303ah_xl_motion_sens_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_motion_sens_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_motion_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_motion_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_tilt_data_ready_flag_get(lsm303ah_ctx_t *ctx,
                                             uint8_t *val);

int32_t lsm303ah_xl_tilt_sens_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_tilt_sens_get(lsm303ah_ctx_t *ctx, uint8_t *val);

int32_t lsm303ah_xl_module_sens_set(lsm303ah_ctx_t *ctx, uint8_t val);
int32_t lsm303ah_xl_module_sens_get(lsm303ah_ctx_t *ctx, uint8_t *val);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /*__LSM303AH_DRIVER__H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
