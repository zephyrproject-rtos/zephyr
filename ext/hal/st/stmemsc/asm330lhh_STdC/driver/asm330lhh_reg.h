/*
 ******************************************************************************
 * @file    asm330lhh_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          asm330lhh_reg.c driver.
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
#ifndef ASM330LHH_REGS_H
#define ASM330LHH_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup ASM330LHH
  * @{
  *
  */

/** @defgroup ASM330LHH_sensors_common_types
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

/** @addtogroup  ASM330LHH Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*asm330lhh_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*asm330lhh_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  asm330lhh_write_ptr  write_reg;
  asm330lhh_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} asm330lhh_ctx_t;

/**
  * @}
  *
  */

/** @defgroup ASM330LHH Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> D5 if SA0=1 -> D7 **/
#define ASM330LHH_I2C_ADD_L                    0xD5U
#define ASM330LHH_I2C_ADD_H                    0xD7U

/** Device Identification (Who am I) **/
#define ASM330LHH_ID                           0x6BU

/**
  * @}
  *
  */

#define ASM330LHH_PIN_CTRL                     0x02U
typedef struct {
  uint8_t not_used_01              : 6;
  uint8_t sdo_pu_en                : 1;
  uint8_t not_used_02              : 1;
} asm330lhh_pin_ctrl_t;

#define ASM330LHH_FIFO_CTRL1                   0x07U
typedef struct {
  uint8_t wtm                      : 8;
} asm330lhh_fifo_ctrl1_t;

#define ASM330LHH_FIFO_CTRL2                   0x08U
typedef struct {
  uint8_t wtm                      : 1;
  uint8_t not_used_01              : 3;
  uint8_t odrchg_en                : 1;
  uint8_t not_used_02              : 2;
  uint8_t stop_on_wtm              : 1;
} asm330lhh_fifo_ctrl2_t;

#define ASM330LHH_FIFO_CTRL3                   0x09U
typedef struct {
  uint8_t bdr_xl                   : 4;
  uint8_t bdr_gy                   : 4;
} asm330lhh_fifo_ctrl3_t;

#define ASM330LHH_FIFO_CTRL4                   0x0AU
typedef struct {
  uint8_t fifo_mode                : 3;
  uint8_t not_used_01              : 1;
  uint8_t odr_t_batch              : 2;
  uint8_t odr_ts_batch             : 2;
} asm330lhh_fifo_ctrl4_t;

#define ASM330LHH_COUNTER_BDR_REG1             0x0BU
typedef struct {
  uint8_t cnt_bdr_th               : 3;
  uint8_t not_used_01              : 2;
  uint8_t trig_counter_bdr         : 1;
  uint8_t rst_counter_bdr          : 1;
  uint8_t dataready_pulsed         : 1;
} asm330lhh_counter_bdr_reg1_t;

#define ASM330LHH_COUNTER_BDR_REG2             0x0CU
typedef struct {
  uint8_t cnt_bdr_th               : 8;
} asm330lhh_counter_bdr_reg2_t;

#define ASM330LHH_INT1_CTRL                    0x0DU
typedef struct {
  uint8_t int1_drdy_xl             : 1;
  uint8_t int1_drdy_g              : 1;
  uint8_t int1_boot                : 1;
  uint8_t int1_fifo_th             : 1;
  uint8_t int1_fifo_ovr            : 1;
  uint8_t int1_fifo_full           : 1;
  uint8_t int1_cnt_bdr             : 1;
  uint8_t den_drdy_flag            : 1;
} asm330lhh_int1_ctrl_t;

#define ASM330LHH_INT2_CTRL                    0x0EU
typedef struct {
  uint8_t int2_drdy_xl             : 1;
  uint8_t int2_drdy_g              : 1;
  uint8_t int2_drdy_temp           : 1;
  uint8_t int2_fifo_th             : 1;
  uint8_t int2_fifo_ovr            : 1;
  uint8_t int2_fifo_full           : 1;
  uint8_t int2_cnt_bdr             : 1;
  uint8_t not_used_01              : 1;
} asm330lhh_int2_ctrl_t;

#define ASM330LHH_WHO_AM_I                     0x0FU
#define ASM330LHH_CTRL1_XL                     0x10U
typedef struct {
  uint8_t not_used_01              : 1;
  uint8_t lpf2_xl_en               : 1;
  uint8_t fs_xl                    : 2;
  uint8_t odr_xl                   : 4;
} asm330lhh_ctrl1_xl_t;

#define ASM330LHH_CTRL2_G                      0x11U
typedef struct {
  uint8_t fs_g                     : 4; /* fs_4000 + fs_125 + fs_g */
  uint8_t odr_g                    : 4;
} asm330lhh_ctrl2_g_t;

#define ASM330LHH_CTRL3_C                      0x12U
typedef struct {
  uint8_t sw_reset                 : 1;
  uint8_t not_used_01              : 1;
  uint8_t if_inc                   : 1;
  uint8_t sim                      : 1;
  uint8_t pp_od                    : 1;
  uint8_t h_lactive                : 1;
  uint8_t bdu                      : 1;
  uint8_t boot                     : 1;
} asm330lhh_ctrl3_c_t;

#define ASM330LHH_CTRL4_C                      0x13U
typedef struct {
  uint8_t not_used_01              : 1;
  uint8_t lpf1_sel_g               : 1;
  uint8_t i2c_disable              : 1;
  uint8_t drdy_mask                : 1;
  uint8_t not_used_02              : 1;
  uint8_t int2_on_int1             : 1;
  uint8_t sleep_g                  : 1;
  uint8_t not_used_03              : 1;
} asm330lhh_ctrl4_c_t;

#define ASM330LHH_CTRL5_C                      0x14U
typedef struct {
  uint8_t st_xl                    : 2;
  uint8_t st_g                     : 2;
  uint8_t not_used_01              : 1;
  uint8_t rounding                 : 2;
  uint8_t not_used_02              : 1;
} asm330lhh_ctrl5_c_t;

#define ASM330LHH_CTRL6_G                      0x15U
typedef struct {
  uint8_t ftype                    : 3;
  uint8_t usr_off_w                : 1;
  uint8_t not_used_01              : 1;
  uint8_t den_mode                 : 3;   /* trig_en + lvl1_en + lvl2_en */
} asm330lhh_ctrl6_g_t;

#define ASM330LHH_CTRL7_G                      0x16U
typedef struct {
  uint8_t not_used_01              : 1;
  uint8_t usr_off_on_out           : 1;
  uint8_t not_used_02              : 2;
  uint8_t hpm_g                    : 2;
  uint8_t hp_en_g                  : 1;
  uint8_t not_used_03              : 1;
} asm330lhh_ctrl7_g_t;

#define ASM330LHH_CTRL8_XL                     0x17U
typedef struct {
  uint8_t low_pass_on_6d           : 1;
  uint8_t not_used_01              : 1;
  uint8_t hp_slope_xl_en           : 1;
  uint8_t fastsettl_mode_xl        : 1;
  uint8_t hp_ref_mode_xl           : 1;
  uint8_t hpcf_xl                  : 3;
} asm330lhh_ctrl8_xl_t;

#define ASM330LHH_CTRL9_XL                     0x18U
typedef struct {
  uint8_t not_used_01              : 2;
  uint8_t den_lh                   : 1;
  uint8_t den_xl_g                 : 2;   /* den_xl_en + den_xl_g */
  uint8_t den_z                    : 1;
  uint8_t den_y                    : 1;
  uint8_t den_x                    : 1;
} asm330lhh_ctrl9_xl_t;

#define ASM330LHH_CTRL10_C                     0x19U
typedef struct {
  uint8_t not_used_01              : 5;
  uint8_t timestamp_en             : 1;
  uint8_t not_used_02              : 2;
} asm330lhh_ctrl10_c_t;

#define ASM330LHH_ALL_INT_SRC                  0x1AU
typedef struct {
  uint8_t ff_ia                    : 1;
  uint8_t wu_ia                    : 1;
  uint8_t not_used_01              : 2;
  uint8_t d6d_ia                   : 1;
  uint8_t sleep_change             : 1;
  uint8_t not_used_02              : 1;
  uint8_t timestamp_endcount       : 1;
} asm330lhh_all_int_src_t;

#define ASM330LHH_WAKE_UP_SRC                  0x1BU
typedef struct {
  uint8_t z_wu                     : 1;
  uint8_t y_wu                     : 1;
  uint8_t x_wu                     : 1;
  uint8_t wu_ia                    : 1;
  uint8_t sleep_change             : 1;
  uint8_t ff_ia                    : 1;
  uint8_t not_used_01              : 2;
} asm330lhh_wake_up_src_t;

#define ASM330LHH_TAP_SRC                      0x1CU
typedef struct {
  uint8_t z_tap                    : 1;
  uint8_t y_tap                    : 1;
  uint8_t x_tap                    : 1;
  uint8_t tap_sign                 : 1;
  uint8_t double_tap               : 1;
  uint8_t single_tap               : 1;
  uint8_t tap_ia                   : 1;
  uint8_t not_used_01              : 1;
} asm330lhh_tap_src_t;

#define ASM330LHH_D6D_SRC                      0x1DU
typedef struct {
  uint8_t xl                       : 1;
  uint8_t xh                       : 1;
  uint8_t yl                       : 1;
  uint8_t yh                       : 1;
  uint8_t zl                       : 1;
  uint8_t zh                       : 1;
  uint8_t d6d_ia                   : 1;
  uint8_t den_drdy                 : 1;
} asm330lhh_d6d_src_t;

#define ASM330LHH_STATUS_REG                   0x1EU
typedef struct {
  uint8_t xlda                     : 1;
  uint8_t gda                      : 1;
  uint8_t tda                      : 1;
  uint8_t not_used_01              : 5;
} asm330lhh_status_reg_t;

#define ASM330LHH_OUT_TEMP_L                   0x20U
#define ASM330LHH_OUT_TEMP_H                   0x21U
#define ASM330LHH_OUTX_L_G                     0x22U
#define ASM330LHH_OUTX_H_G                     0x23U
#define ASM330LHH_OUTY_L_G                     0x24U
#define ASM330LHH_OUTY_H_G                     0x25U
#define ASM330LHH_OUTZ_L_G                     0x26U
#define ASM330LHH_OUTZ_H_G                     0x27U
#define ASM330LHH_OUTX_L_A                     0x28U
#define ASM330LHH_OUTX_H_A                     0x29U
#define ASM330LHH_OUTY_L_A                     0x2AU
#define ASM330LHH_OUTY_H_A                     0x2BU
#define ASM330LHH_OUTZ_L_A                     0x2CU
#define ASM330LHH_OUTZ_H_A                     0x2DU
#define ASM330LHH_FIFO_STATUS1                 0x3AU
typedef struct {
  uint8_t diff_fifo                : 8;
} asm330lhh_fifo_status1_t;

#define ASM330LHH_FIFO_STATUS2                 0x3BU
typedef struct {
  uint8_t diff_fifo                : 2;
  uint8_t not_used_01              : 1;
  uint8_t over_run_latched         : 1;
  uint8_t counter_bdr_ia           : 1;
  uint8_t fifo_full_ia             : 1;
  uint8_t fifo_ovr_ia              : 1;
  uint8_t fifo_wtm_ia              : 1;
} asm330lhh_fifo_status2_t;

#define ASM330LHH_TIMESTAMP0                   0x40U
#define ASM330LHH_TIMESTAMP1                   0x41U
#define ASM330LHH_TIMESTAMP2                   0x42U
#define ASM330LHH_TIMESTAMP3                   0x43U
#define ASM330LHH_INT_CFG0                     0x56U
typedef struct {
  uint8_t lir                      : 1;
  uint8_t not_used_01              : 3;
  uint8_t slope_fds                : 1;
  uint8_t sleep_status_on_int      : 1;
  uint8_t int_clr_on_read          : 1;
  uint8_t not_used_02              : 1;
} asm330lhh_int_cfg0_t;

#define ASM330LHH_INT_CFG1                     0x58U
typedef struct {
  uint8_t not_used_01              : 5;
  uint8_t inact_en                 : 2;
  uint8_t interrupts_enable        : 1;
} asm330lhh_int_cfg1_t;

#define ASM330LHH_THS_6D                       0x59U
typedef struct {
  uint8_t not_used_01              : 5;
  uint8_t sixd_ths                 : 2;
  uint8_t d4d_en                   : 1;
} asm330lhh_ths_6d_t;

#define ASM330LHH_INT_DUR2                     0x5AU
typedef struct {
  uint8_t shock                    : 2;
  uint8_t quiet                    : 2;
  uint8_t dur                      : 4;
} asm330lhh_int_dur2_t;

#define ASM330LHH_WAKE_UP_THS                  0x5BU
typedef struct {
  uint8_t wk_ths                   : 6;
  uint8_t usr_off_on_wu            : 1;
  uint8_t not_used_01              : 1;
} asm330lhh_wake_up_ths_t;

#define ASM330LHH_WAKE_UP_DUR                  0x5CU
typedef struct {
  uint8_t sleep_dur                : 4;
  uint8_t wake_ths_w               : 1;
  uint8_t wake_dur                 : 2;
  uint8_t ff_dur                   : 1;
} asm330lhh_wake_up_dur_t;

#define ASM330LHH_FREE_FALL                    0x5DU
typedef struct {
  uint8_t ff_ths                   : 3;
  uint8_t ff_dur                   : 5;
} asm330lhh_free_fall_t;

#define ASM330LHH_MD1_CFG                      0x5EU
typedef struct {
  uint8_t not_used_01              : 2;
  uint8_t int1_6d                  : 1;
  uint8_t not_used_02              : 1;
  uint8_t int1_ff                  : 1;
  uint8_t int1_wu                  : 1;
  uint8_t not_used_03              : 1;
  uint8_t int1_sleep_change        : 1;
} asm330lhh_md1_cfg_t;

#define ASM330LHH_MD2_CFG                      0x5FU
typedef struct {
  uint8_t int2_timestamp           : 1;
  uint8_t not_used_01              : 1;
  uint8_t int2_6d                  : 1;
  uint8_t not_used_02              : 1;
  uint8_t int2_ff                  : 1;
  uint8_t int2_wu                  : 1;
  uint8_t not_used_03              : 1;
  uint8_t int2_sleep_change        : 1;
} asm330lhh_md2_cfg_t;

#define ASM330LHH_INTERNAL_FREQ_FINE           0x63U
typedef struct {
  uint8_t freq_fine                : 8;
} asm330lhh_internal_freq_fine_t;

#define ASM330LHH_X_OFS_USR                    0x73U
#define ASM330LHH_Y_OFS_USR                    0x74U
#define ASM330LHH_Z_OFS_USR                    0x75U
#define ASM330LHH_FIFO_DATA_OUT_TAG            0x78U
typedef struct {
  uint8_t tag_parity               : 1;
  uint8_t tag_cnt                  : 2;
  uint8_t tag_sensor               : 5;
} asm330lhh_fifo_data_out_tag_t;

#define ASM330LHH_FIFO_DATA_OUT_X_L            0x79U
#define ASM330LHH_FIFO_DATA_OUT_X_H            0x7AU
#define ASM330LHH_FIFO_DATA_OUT_Y_L            0x7BU
#define ASM330LHH_FIFO_DATA_OUT_Y_H            0x7CU
#define ASM330LHH_FIFO_DATA_OUT_Z_L            0x7DU
#define ASM330LHH_FIFO_DATA_OUT_Z_H            0x7EU

/**
  * @defgroup ASM330LHH_Register_Union
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
  asm330lhh_pin_ctrl_t                      pin_ctrl;
  asm330lhh_fifo_ctrl1_t                    fifo_ctrl1;
  asm330lhh_fifo_ctrl2_t                    fifo_ctrl2;
  asm330lhh_fifo_ctrl3_t                    fifo_ctrl3;
  asm330lhh_fifo_ctrl4_t                    fifo_ctrl4;
  asm330lhh_counter_bdr_reg1_t              counter_bdr_reg1;
  asm330lhh_counter_bdr_reg2_t              counter_bdr_reg2;
  asm330lhh_int1_ctrl_t                     int1_ctrl;
  asm330lhh_int2_ctrl_t                     int2_ctrl;
  asm330lhh_ctrl1_xl_t                      ctrl1_xl;
  asm330lhh_ctrl2_g_t                       ctrl2_g;
  asm330lhh_ctrl3_c_t                       ctrl3_c;
  asm330lhh_ctrl4_c_t                       ctrl4_c;
  asm330lhh_ctrl5_c_t                       ctrl5_c;
  asm330lhh_ctrl6_g_t                       ctrl6_g;
  asm330lhh_ctrl7_g_t                       ctrl7_g;
  asm330lhh_ctrl8_xl_t                      ctrl8_xl;
  asm330lhh_ctrl9_xl_t                      ctrl9_xl;
  asm330lhh_ctrl10_c_t                      ctrl10_c;
  asm330lhh_all_int_src_t                   all_int_src;
  asm330lhh_wake_up_src_t                   wake_up_src;
  asm330lhh_tap_src_t                       tap_src;
  asm330lhh_d6d_src_t                       d6d_src;
  asm330lhh_status_reg_t                    status_reg;
  asm330lhh_fifo_status1_t                  fifo_status1;
  asm330lhh_fifo_status2_t                  fifo_status2;
  asm330lhh_int_cfg0_t                      tap_cfg0;
  asm330lhh_int_cfg1_t                      int_cfg1;
  asm330lhh_ths_6d_t                        ths_6d;
  asm330lhh_int_dur2_t                      int_dur2;
  asm330lhh_wake_up_ths_t                   wake_up_ths;
  asm330lhh_wake_up_dur_t                   wake_up_dur;
  asm330lhh_free_fall_t                     free_fall;
  asm330lhh_md1_cfg_t                       md1_cfg;
  asm330lhh_md2_cfg_t                       md2_cfg;
  asm330lhh_internal_freq_fine_t            internal_freq_fine;
  asm330lhh_fifo_data_out_tag_t             fifo_data_out_tag;
  bitwise_t                                 bitwise;
  uint8_t                                   byte;
} asm330lhh_reg_t;

/**
  * @}
  *
  */

int32_t asm330lhh_read_reg(asm330lhh_ctx_t *ctx, uint8_t reg, uint8_t* data,
                         uint16_t len);
int32_t asm330lhh_write_reg(asm330lhh_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);

extern float_t asm330lhh_from_fs2g_to_mg(int16_t lsb);
extern float_t asm330lhh_from_fs4g_to_mg(int16_t lsb);
extern float_t asm330lhh_from_fs8g_to_mg(int16_t lsb);
extern float_t asm330lhh_from_fs16g_to_mg(int16_t lsb);
extern float_t asm330lhh_from_fs125dps_to_mdps(int16_t lsb);
extern float_t asm330lhh_from_fs250dps_to_mdps(int16_t lsb);
extern float_t asm330lhh_from_fs500dps_to_mdps(int16_t lsb);
extern float_t asm330lhh_from_fs1000dps_to_mdps(int16_t lsb);
extern float_t asm330lhh_from_fs2000dps_to_mdps(int16_t lsb);
extern float_t asm330lhh_from_fs4000dps_to_mdps(int16_t lsb);
extern float_t asm330lhh_from_lsb_to_celsius(int16_t lsb);
extern float_t asm330lhh_from_lsb_to_nsec(int32_t lsb);

typedef enum {
  ASM330LHH_2g   = 0,
  ASM330LHH_16g  = 1, /* if XL_FS_MODE = ‘1’ -> ASM330LHH_2g */
  ASM330LHH_4g   = 2,
  ASM330LHH_8g   = 3,
} asm330lhh_fs_xl_t;
int32_t asm330lhh_xl_full_scale_set(asm330lhh_ctx_t *ctx, asm330lhh_fs_xl_t val);
int32_t asm330lhh_xl_full_scale_get(asm330lhh_ctx_t *ctx, asm330lhh_fs_xl_t *val);

typedef enum {
  ASM330LHH_XL_ODR_OFF    = 0,
  ASM330LHH_XL_ODR_12Hz5  = 1,
  ASM330LHH_XL_ODR_26Hz   = 2,
  ASM330LHH_XL_ODR_52Hz   = 3,
  ASM330LHH_XL_ODR_104Hz  = 4,
  ASM330LHH_XL_ODR_208Hz  = 5,
  ASM330LHH_XL_ODR_417Hz  = 6,
  ASM330LHH_XL_ODR_833Hz  = 7,
  ASM330LHH_XL_ODR_1667Hz = 8,
  ASM330LHH_XL_ODR_3333Hz = 9,
  ASM330LHH_XL_ODR_6667Hz = 10,
  ASM330LHH_XL_ODR_6Hz5   = 11, /* (low power only) */
} asm330lhh_odr_xl_t;
int32_t asm330lhh_xl_data_rate_set(asm330lhh_ctx_t *ctx, asm330lhh_odr_xl_t val);
int32_t asm330lhh_xl_data_rate_get(asm330lhh_ctx_t *ctx, asm330lhh_odr_xl_t *val);

typedef enum {
  ASM330LHH_125dps = 2,
  ASM330LHH_250dps = 0,
  ASM330LHH_500dps = 4,
  ASM330LHH_1000dps = 8,
  ASM330LHH_2000dps = 12,
  ASM330LHH_4000dps = 1,
} asm330lhh_fs_g_t;
int32_t asm330lhh_gy_full_scale_set(asm330lhh_ctx_t *ctx, asm330lhh_fs_g_t val);
int32_t asm330lhh_gy_full_scale_get(asm330lhh_ctx_t *ctx, asm330lhh_fs_g_t *val);

typedef enum {
  ASM330LHH_GY_ODR_OFF    = 0,
  ASM330LHH_GY_ODR_12Hz5  = 1,
  ASM330LHH_GY_ODR_26Hz   = 2,
  ASM330LHH_GY_ODR_52Hz   = 3,
  ASM330LHH_GY_ODR_104Hz  = 4,
  ASM330LHH_GY_ODR_208Hz  = 5,
  ASM330LHH_GY_ODR_417Hz  = 6,
  ASM330LHH_GY_ODR_833Hz  = 7,
  ASM330LHH_GY_ODR_1667Hz = 8,
  ASM330LHH_GY_ODR_3333Hz = 9,
  ASM330LHH_GY_ODR_6667Hz = 10,
} asm330lhh_odr_g_t;
int32_t asm330lhh_gy_data_rate_set(asm330lhh_ctx_t *ctx,
                                 asm330lhh_odr_g_t val);
int32_t asm330lhh_gy_data_rate_get(asm330lhh_ctx_t *ctx,
                                 asm330lhh_odr_g_t *val);

int32_t asm330lhh_block_data_update_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_block_data_update_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_LSb_1mg  = 0,
  ASM330LHH_LSb_16mg = 1,
} asm330lhh_usr_off_w_t;
int32_t asm330lhh_xl_offset_weight_set(asm330lhh_ctx_t *ctx,
                                     asm330lhh_usr_off_w_t val);
int32_t asm330lhh_xl_offset_weight_get(asm330lhh_ctx_t *ctx,
                                     asm330lhh_usr_off_w_t *val);

typedef struct {
  asm330lhh_all_int_src_t       all_int_src;
  asm330lhh_wake_up_src_t       wake_up_src;
  asm330lhh_tap_src_t           tap_src;
  asm330lhh_d6d_src_t           d6d_src;
  asm330lhh_status_reg_t        status_reg;
  } asm330lhh_all_sources_t;
int32_t asm330lhh_all_sources_get(asm330lhh_ctx_t *ctx,
                                asm330lhh_all_sources_t *val);

int32_t asm330lhh_status_reg_get(asm330lhh_ctx_t *ctx,
                                 asm330lhh_status_reg_t *val);

int32_t asm330lhh_xl_flag_data_ready_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_gy_flag_data_ready_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_temp_flag_data_ready_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_xl_usr_offset_x_set(asm330lhh_ctx_t *ctx, uint8_t *buff);
int32_t asm330lhh_xl_usr_offset_x_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

int32_t asm330lhh_xl_usr_offset_y_set(asm330lhh_ctx_t *ctx, uint8_t *buff);
int32_t asm330lhh_xl_usr_offset_y_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

int32_t asm330lhh_xl_usr_offset_z_set(asm330lhh_ctx_t *ctx, uint8_t *buff);
int32_t asm330lhh_xl_usr_offset_z_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

int32_t asm330lhh_xl_usr_offset_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_xl_usr_offset_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_timestamp_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_timestamp_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_timestamp_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

typedef enum {
  ASM330LHH_NO_ROUND      = 0,
  ASM330LHH_ROUND_XL      = 1,
  ASM330LHH_ROUND_GY      = 2,
  ASM330LHH_ROUND_GY_XL   = 3,
} asm330lhh_rounding_t;
int32_t asm330lhh_rounding_mode_set(asm330lhh_ctx_t *ctx,
                                  asm330lhh_rounding_t val);
int32_t asm330lhh_rounding_mode_get(asm330lhh_ctx_t *ctx,
                                  asm330lhh_rounding_t *val);

int32_t asm330lhh_temperature_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

int32_t asm330lhh_angular_rate_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

int32_t asm330lhh_acceleration_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

int32_t asm330lhh_fifo_out_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

int32_t asm330lhh_odr_cal_reg_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_odr_cal_reg_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_DRDY_LATCHED = 0,
  ASM330LHH_DRDY_PULSED  = 1,
} asm330lhh_dataready_pulsed_t;
int32_t asm330lhh_data_ready_mode_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_dataready_pulsed_t val);
int32_t asm330lhh_data_ready_mode_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_dataready_pulsed_t *val);

int32_t asm330lhh_device_id_get(asm330lhh_ctx_t *ctx, uint8_t *buff);

int32_t asm330lhh_reset_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_reset_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_auto_increment_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_auto_increment_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_boot_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_boot_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_XL_ST_DISABLE  = 0,
  ASM330LHH_XL_ST_POSITIVE = 1,
  ASM330LHH_XL_ST_NEGATIVE = 2,
} asm330lhh_st_xl_t;
int32_t asm330lhh_xl_self_test_set(asm330lhh_ctx_t *ctx, asm330lhh_st_xl_t val);
int32_t asm330lhh_xl_self_test_get(asm330lhh_ctx_t *ctx, asm330lhh_st_xl_t *val);

typedef enum {
  ASM330LHH_GY_ST_DISABLE  = 0,
  ASM330LHH_GY_ST_POSITIVE = 1,
  ASM330LHH_GY_ST_NEGATIVE = 3,
} asm330lhh_st_g_t;
int32_t asm330lhh_gy_self_test_set(asm330lhh_ctx_t *ctx, asm330lhh_st_g_t val);
int32_t asm330lhh_gy_self_test_get(asm330lhh_ctx_t *ctx, asm330lhh_st_g_t *val);

int32_t asm330lhh_xl_filter_lp2_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_xl_filter_lp2_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_gy_filter_lp1_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_gy_filter_lp1_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_filter_settling_mask_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_filter_settling_mask_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_ULTRA_LIGHT  = 0,
  ASM330LHH_VERY_LIGHT   = 1,
  ASM330LHH_LIGHT        = 2,
  ASM330LHH_MEDIUM       = 3,
  ASM330LHH_STRONG       = 4,
  ASM330LHH_VERY_STRONG  = 5,
  ASM330LHH_AGGRESSIVE   = 6,
  ASM330LHH_XTREME       = 7,
} asm330lhh_ftype_t;
int32_t asm330lhh_gy_lp1_bandwidth_set(asm330lhh_ctx_t *ctx, asm330lhh_ftype_t val);
int32_t asm330lhh_gy_lp1_bandwidth_get(asm330lhh_ctx_t *ctx, asm330lhh_ftype_t *val);

int32_t asm330lhh_xl_lp2_on_6d_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_xl_lp2_on_6d_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_HP_PATH_DISABLE_ON_OUT    = 0x00,
  ASM330LHH_SLOPE_ODR_DIV_4           = 0x10,
  ASM330LHH_HP_ODR_DIV_10             = 0x11,
  ASM330LHH_HP_ODR_DIV_20             = 0x12,
  ASM330LHH_HP_ODR_DIV_45             = 0x13,
  ASM330LHH_HP_ODR_DIV_100            = 0x14,
  ASM330LHH_HP_ODR_DIV_200            = 0x15,
  ASM330LHH_HP_ODR_DIV_400            = 0x16,
  ASM330LHH_HP_ODR_DIV_800            = 0x17,
  ASM330LHH_HP_REF_MD_ODR_DIV_10      = 0x31,
  ASM330LHH_HP_REF_MD_ODR_DIV_20      = 0x32,
  ASM330LHH_HP_REF_MD_ODR_DIV_45      = 0x33,
  ASM330LHH_HP_REF_MD_ODR_DIV_100     = 0x34,
  ASM330LHH_HP_REF_MD_ODR_DIV_200     = 0x35,
  ASM330LHH_HP_REF_MD_ODR_DIV_400     = 0x36,
  ASM330LHH_HP_REF_MD_ODR_DIV_800     = 0x37,
  ASM330LHH_LP_ODR_DIV_10             = 0x01,
  ASM330LHH_LP_ODR_DIV_20             = 0x02,
  ASM330LHH_LP_ODR_DIV_45             = 0x03,
  ASM330LHH_LP_ODR_DIV_100            = 0x04,
  ASM330LHH_LP_ODR_DIV_200            = 0x05,
  ASM330LHH_LP_ODR_DIV_400            = 0x06,
  ASM330LHH_LP_ODR_DIV_800            = 0x07,
} asm330lhh_hp_slope_xl_en_t;
int32_t asm330lhh_xl_hp_path_on_out_set(asm330lhh_ctx_t *ctx,
                                      asm330lhh_hp_slope_xl_en_t val);
int32_t asm330lhh_xl_hp_path_on_out_get(asm330lhh_ctx_t *ctx,
                                      asm330lhh_hp_slope_xl_en_t *val);

int32_t asm330lhh_xl_fast_settling_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_xl_fast_settling_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_USE_SLOPE = 0,
  ASM330LHH_USE_HPF   = 1,
} asm330lhh_slope_fds_t;
int32_t asm330lhh_xl_hp_path_internal_set(asm330lhh_ctx_t *ctx,
                                        asm330lhh_slope_fds_t val);
int32_t asm330lhh_xl_hp_path_internal_get(asm330lhh_ctx_t *ctx,
                                        asm330lhh_slope_fds_t *val);

typedef enum {
  ASM330LHH_HP_FILTER_NONE     = 0x00,
  ASM330LHH_HP_FILTER_16mHz    = 0x80,
  ASM330LHH_HP_FILTER_65mHz    = 0x81,
  ASM330LHH_HP_FILTER_260mHz   = 0x82,
  ASM330LHH_HP_FILTER_1Hz04    = 0x83,
} asm330lhh_hpm_g_t;
int32_t asm330lhh_gy_hp_path_internal_set(asm330lhh_ctx_t *ctx,
                                        asm330lhh_hpm_g_t val);
int32_t asm330lhh_gy_hp_path_internal_get(asm330lhh_ctx_t *ctx,
                                        asm330lhh_hpm_g_t *val);

typedef enum {
  ASM330LHH_PULL_UP_DISC       = 0,
  ASM330LHH_PULL_UP_CONNECT    = 1,
} asm330lhh_sdo_pu_en_t;
int32_t asm330lhh_sdo_sa0_mode_set(asm330lhh_ctx_t *ctx, asm330lhh_sdo_pu_en_t val);
int32_t asm330lhh_sdo_sa0_mode_get(asm330lhh_ctx_t *ctx, asm330lhh_sdo_pu_en_t *val);

typedef enum {
  ASM330LHH_SPI_4_WIRE = 0,
  ASM330LHH_SPI_3_WIRE = 1,
} asm330lhh_sim_t;
int32_t asm330lhh_spi_mode_set(asm330lhh_ctx_t *ctx, asm330lhh_sim_t val);
int32_t asm330lhh_spi_mode_get(asm330lhh_ctx_t *ctx, asm330lhh_sim_t *val);

typedef enum {
  ASM330LHH_I2C_ENABLE  = 0,
  ASM330LHH_I2C_DISABLE = 1,
} asm330lhh_i2c_disable_t;
int32_t asm330lhh_i2c_interface_set(asm330lhh_ctx_t *ctx,
                                  asm330lhh_i2c_disable_t val);
int32_t asm330lhh_i2c_interface_get(asm330lhh_ctx_t *ctx,
                                  asm330lhh_i2c_disable_t *val);

typedef struct {
    asm330lhh_int1_ctrl_t          int1_ctrl;
    asm330lhh_md1_cfg_t            md1_cfg;
} asm330lhh_pin_int1_route_t;
int32_t asm330lhh_pin_int1_route_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_pin_int1_route_t *val);
int32_t asm330lhh_pin_int1_route_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_pin_int1_route_t *val);

typedef struct {
  asm330lhh_int2_ctrl_t          int2_ctrl;
  asm330lhh_md2_cfg_t            md2_cfg;
} asm330lhh_pin_int2_route_t;
int32_t asm330lhh_pin_int2_route_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_pin_int2_route_t *val);
int32_t asm330lhh_pin_int2_route_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_pin_int2_route_t *val);

typedef enum {
  ASM330LHH_PUSH_PULL   = 0,
  ASM330LHH_OPEN_DRAIN  = 1,
} asm330lhh_pp_od_t;
int32_t asm330lhh_pin_mode_set(asm330lhh_ctx_t *ctx, asm330lhh_pp_od_t val);
int32_t asm330lhh_pin_mode_get(asm330lhh_ctx_t *ctx, asm330lhh_pp_od_t *val);

typedef enum {
  ASM330LHH_ACTIVE_HIGH = 0,
  ASM330LHH_ACTIVE_LOW  = 1,
} asm330lhh_h_lactive_t;
int32_t asm330lhh_pin_polarity_set(asm330lhh_ctx_t *ctx, asm330lhh_h_lactive_t val);
int32_t asm330lhh_pin_polarity_get(asm330lhh_ctx_t *ctx, asm330lhh_h_lactive_t *val);

int32_t asm330lhh_all_on_int1_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_all_on_int1_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_ALL_INT_PULSED            = 0,
  ASM330LHH_ALL_INT_LATCHED           = 3,
} asm330lhh_lir_t;
int32_t asm330lhh_int_notification_set(asm330lhh_ctx_t *ctx, asm330lhh_lir_t val);
int32_t asm330lhh_int_notification_get(asm330lhh_ctx_t *ctx, asm330lhh_lir_t *val);

typedef enum {
  ASM330LHH_LSb_FS_DIV_64       = 0,
  ASM330LHH_LSb_FS_DIV_256      = 1,
} asm330lhh_wake_ths_w_t;
int32_t asm330lhh_wkup_ths_weight_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_wake_ths_w_t val);
int32_t asm330lhh_wkup_ths_weight_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_wake_ths_w_t *val);

int32_t asm330lhh_wkup_threshold_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_wkup_threshold_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_xl_usr_offset_on_wkup_set(asm330lhh_ctx_t *ctx,
                                          uint8_t val);
int32_t asm330lhh_xl_usr_offset_on_wkup_get(asm330lhh_ctx_t *ctx,
                                          uint8_t *val);

int32_t asm330lhh_wkup_dur_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_wkup_dur_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_gy_sleep_mode_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_gy_sleep_mode_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_DRIVE_SLEEP_CHG_EVENT = 0,
  ASM330LHH_DRIVE_SLEEP_STATUS    = 1,
} asm330lhh_sleep_status_on_int_t;
int32_t asm330lhh_act_pin_notification_set(asm330lhh_ctx_t *ctx,
                                         asm330lhh_sleep_status_on_int_t val);
int32_t asm330lhh_act_pin_notification_get(asm330lhh_ctx_t *ctx,
                                         asm330lhh_sleep_status_on_int_t *val);

typedef enum {
  ASM330LHH_XL_AND_GY_NOT_AFFECTED      = 0,
  ASM330LHH_XL_12Hz5_GY_NOT_AFFECTED    = 1,
  ASM330LHH_XL_12Hz5_GY_SLEEP           = 2,
  ASM330LHH_XL_12Hz5_GY_PD              = 3,
} asm330lhh_inact_en_t;
int32_t asm330lhh_act_mode_set(asm330lhh_ctx_t *ctx,
                             asm330lhh_inact_en_t val);
int32_t asm330lhh_act_mode_get(asm330lhh_ctx_t *ctx,
                             asm330lhh_inact_en_t *val);

int32_t asm330lhh_act_sleep_dur_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_act_sleep_dur_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_DEG_80  = 0,
  ASM330LHH_DEG_70  = 1,
  ASM330LHH_DEG_60  = 2,
  ASM330LHH_DEG_50  = 3,
} asm330lhh_sixd_ths_t;
int32_t asm330lhh_6d_threshold_set(asm330lhh_ctx_t *ctx,
                                 asm330lhh_sixd_ths_t val);
int32_t asm330lhh_6d_threshold_get(asm330lhh_ctx_t *ctx,
                                 asm330lhh_sixd_ths_t *val);

int32_t asm330lhh_4d_mode_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_4d_mode_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_FF_TSH_156mg = 0,
  ASM330LHH_FF_TSH_219mg = 1,
  ASM330LHH_FF_TSH_250mg = 2,
  ASM330LHH_FF_TSH_312mg = 3,
  ASM330LHH_FF_TSH_344mg = 4,
  ASM330LHH_FF_TSH_406mg = 5,
  ASM330LHH_FF_TSH_469mg = 6,
  ASM330LHH_FF_TSH_500mg = 7,
} asm330lhh_ff_ths_t;
int32_t asm330lhh_ff_threshold_set(asm330lhh_ctx_t *ctx,
                                 asm330lhh_ff_ths_t val);
int32_t asm330lhh_ff_threshold_get(asm330lhh_ctx_t *ctx,
                                 asm330lhh_ff_ths_t *val);

int32_t asm330lhh_ff_dur_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_ff_dur_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_fifo_watermark_set(asm330lhh_ctx_t *ctx, uint16_t val);
int32_t asm330lhh_fifo_watermark_get(asm330lhh_ctx_t *ctx, uint16_t *val);

int32_t asm330lhh_fifo_virtual_sens_odr_chg_set(asm330lhh_ctx_t *ctx,
                                              uint8_t val);
int32_t asm330lhh_fifo_virtual_sens_odr_chg_get(asm330lhh_ctx_t *ctx,
                                              uint8_t *val);

int32_t asm330lhh_fifo_stop_on_wtm_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_fifo_stop_on_wtm_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_XL_NOT_BATCHED        =  0,
  ASM330LHH_XL_BATCHED_AT_12Hz5   =  1,
  ASM330LHH_XL_BATCHED_AT_26Hz    =  2,
  ASM330LHH_XL_BATCHED_AT_52Hz    =  3,
  ASM330LHH_XL_BATCHED_AT_104Hz   =  4,
  ASM330LHH_XL_BATCHED_AT_208Hz   =  5,
  ASM330LHH_XL_BATCHED_AT_417Hz   =  6,
  ASM330LHH_XL_BATCHED_AT_833Hz   =  7,
  ASM330LHH_XL_BATCHED_AT_1667Hz  =  8,
  ASM330LHH_XL_BATCHED_AT_3333Hz  =  9,
  ASM330LHH_XL_BATCHED_AT_6667Hz  = 10,
  ASM330LHH_XL_BATCHED_AT_6Hz5    = 11,
} asm330lhh_bdr_xl_t;
int32_t asm330lhh_fifo_xl_batch_set(asm330lhh_ctx_t *ctx, asm330lhh_bdr_xl_t val);
int32_t asm330lhh_fifo_xl_batch_get(asm330lhh_ctx_t *ctx, asm330lhh_bdr_xl_t *val);

typedef enum {
  ASM330LHH_GY_NOT_BATCHED         = 0,
  ASM330LHH_GY_BATCHED_AT_12Hz5    = 1,
  ASM330LHH_GY_BATCHED_AT_26Hz     = 2,
  ASM330LHH_GY_BATCHED_AT_52Hz     = 3,
  ASM330LHH_GY_BATCHED_AT_104Hz    = 4,
  ASM330LHH_GY_BATCHED_AT_208Hz    = 5,
  ASM330LHH_GY_BATCHED_AT_417Hz    = 6,
  ASM330LHH_GY_BATCHED_AT_833Hz    = 7,
  ASM330LHH_GY_BATCHED_AT_1667Hz   = 8,
  ASM330LHH_GY_BATCHED_AT_3333Hz   = 9,
  ASM330LHH_GY_BATCHED_AT_6667Hz   = 10,
  ASM330LHH_GY_BATCHED_6Hz5        = 11,
} asm330lhh_bdr_gy_t;
int32_t asm330lhh_fifo_gy_batch_set(asm330lhh_ctx_t *ctx, asm330lhh_bdr_gy_t val);
int32_t asm330lhh_fifo_gy_batch_get(asm330lhh_ctx_t *ctx, asm330lhh_bdr_gy_t *val);

typedef enum {
  ASM330LHH_BYPASS_MODE             = 0,
  ASM330LHH_FIFO_MODE               = 1,
  ASM330LHH_STREAM_TO_FIFO_MODE     = 3,
  ASM330LHH_BYPASS_TO_STREAM_MODE   = 4,
  ASM330LHH_STREAM_MODE             = 6,
  ASM330LHH_BYPASS_TO_FIFO_MODE     = 7,
} asm330lhh_fifo_mode_t;
int32_t asm330lhh_fifo_mode_set(asm330lhh_ctx_t *ctx, asm330lhh_fifo_mode_t val);
int32_t asm330lhh_fifo_mode_get(asm330lhh_ctx_t *ctx, asm330lhh_fifo_mode_t *val);

typedef enum {
  ASM330LHH_TEMP_NOT_BATCHED        = 0,
  ASM330LHH_TEMP_BATCHED_AT_52Hz    = 1,
  ASM330LHH_TEMP_BATCHED_AT_12Hz5   = 2,
  ASM330LHH_TEMP_BATCHED_AT_1Hz6    = 3,
} asm330lhh_odr_t_batch_t;
int32_t asm330lhh_fifo_temp_batch_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_odr_t_batch_t val);
int32_t asm330lhh_fifo_temp_batch_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_odr_t_batch_t *val);

typedef enum {
  ASM330LHH_NO_DECIMATION = 0,
  ASM330LHH_DEC_1         = 1,
  ASM330LHH_DEC_8         = 2,
  ASM330LHH_DEC_32        = 3,
} asm330lhh_odr_ts_batch_t;
int32_t asm330lhh_fifo_timestamp_decimation_set(asm330lhh_ctx_t *ctx,
                                              asm330lhh_odr_ts_batch_t val);
int32_t asm330lhh_fifo_timestamp_decimation_get(asm330lhh_ctx_t *ctx,
                                              asm330lhh_odr_ts_batch_t *val);

typedef enum {
  ASM330LHH_XL_BATCH_EVENT   = 0,
  ASM330LHH_GYRO_BATCH_EVENT = 1,
} asm330lhh_trig_counter_bdr_t;
int32_t asm330lhh_fifo_cnt_event_batch_set(asm330lhh_ctx_t *ctx,
                                         asm330lhh_trig_counter_bdr_t val);
int32_t asm330lhh_fifo_cnt_event_batch_get(asm330lhh_ctx_t *ctx,
                                         asm330lhh_trig_counter_bdr_t *val);

int32_t asm330lhh_rst_batch_counter_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_rst_batch_counter_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_batch_counter_threshold_set(asm330lhh_ctx_t *ctx,
                                            uint16_t val);
int32_t asm330lhh_batch_counter_threshold_get(asm330lhh_ctx_t *ctx,
                                            uint16_t *val);

int32_t asm330lhh_fifo_data_level_get(asm330lhh_ctx_t *ctx, uint16_t *val);

int32_t asm330lhh_fifo_status_get(asm330lhh_ctx_t *ctx,
                                asm330lhh_fifo_status2_t *val);

int32_t asm330lhh_fifo_full_flag_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_fifo_ovr_flag_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_fifo_wtm_flag_get(asm330lhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  ASM330LHH_GYRO_NC_TAG    = 1,
  ASM330LHH_XL_NC_TAG,
  ASM330LHH_TEMPERATURE_TAG,
  ASM330LHH_TIMESTAMP_TAG,
  ASM330LHH_CFG_CHANGE_TAG,
} asm330lhh_fifo_tag_t;
int32_t asm330lhh_fifo_sensor_tag_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_fifo_tag_t *val);

typedef enum {
  ASM330LHH_DEN_DISABLE    = 0,
  ASM330LHH_LEVEL_FIFO     = 6,
  ASM330LHH_LEVEL_LETCHED  = 3,
  ASM330LHH_LEVEL_TRIGGER  = 2,
  ASM330LHH_EDGE_TRIGGER   = 4,
} asm330lhh_den_mode_t;
int32_t asm330lhh_den_mode_set(asm330lhh_ctx_t *ctx,
                             asm330lhh_den_mode_t val);
int32_t asm330lhh_den_mode_get(asm330lhh_ctx_t *ctx,
                             asm330lhh_den_mode_t *val);

typedef enum {
  ASM330LHH_DEN_ACT_LOW  = 0,
  ASM330LHH_DEN_ACT_HIGH = 1,
} asm330lhh_den_lh_t;
int32_t asm330lhh_den_polarity_set(asm330lhh_ctx_t *ctx,
                                 asm330lhh_den_lh_t val);
int32_t asm330lhh_den_polarity_get(asm330lhh_ctx_t *ctx,
                                 asm330lhh_den_lh_t *val);

typedef enum {
  ASM330LHH_STAMP_IN_GY_DATA     = 0,
  ASM330LHH_STAMP_IN_XL_DATA     = 1,
  ASM330LHH_STAMP_IN_GY_XL_DATA  = 2,
} asm330lhh_den_xl_g_t;
int32_t asm330lhh_den_enable_set(asm330lhh_ctx_t *ctx,
                               asm330lhh_den_xl_g_t val);
int32_t asm330lhh_den_enable_get(asm330lhh_ctx_t *ctx,
                               asm330lhh_den_xl_g_t *val);

int32_t asm330lhh_den_mark_axis_x_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_den_mark_axis_x_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_den_mark_axis_y_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_den_mark_axis_y_get(asm330lhh_ctx_t *ctx, uint8_t *val);

int32_t asm330lhh_den_mark_axis_z_set(asm330lhh_ctx_t *ctx, uint8_t val);
int32_t asm330lhh_den_mark_axis_z_get(asm330lhh_ctx_t *ctx, uint8_t *val);

/**
  *@}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* ASM330LHH_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
