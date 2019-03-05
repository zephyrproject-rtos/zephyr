/*
 ******************************************************************************
 * @file    lis2hh12_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          lis2hh12_reg.c driver.
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
#ifndef LIS2HH12_REGS_H
#define LIS2HH12_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LIS2HH12
  * @{
  *
  */

/** @defgroup LIS2HH12_sensors_common_types
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

/** @addtogroup  LIS2HH12_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*lis2hh12_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lis2hh12_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lis2hh12_write_ptr  write_reg;
  lis2hh12_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lis2hh12_ctx_t;

/**
  * @}
  *
  */

/** @defgroup LIS2HH12_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> 0x3D if SA0=1 -> 0x3B **/
#define LIS2HH12_I2C_ADD_L                   0x3DU
#define LIS2HH12_I2C_ADD_H                   0x3BU
/** Device Identification (Who am I) **/
#define LIS2HH12_ID                          0x41U

/**
  * @}
  *
  */

#define LIS2HH12_TEMP_L                      0x0BU
#define LIS2HH12_TEMP_H                      0x0CU
#define LIS2HH12_WHO_AM_I                    0x0FU
#define LIS2HH12_ACT_THS                     0x1EU
typedef struct {
  uint8_t ths              : 7;
  uint8_t not_used_01      : 1;
} lis2hh12_act_ths_t;

#define LIS2HH12_ACT_DUR                     0x1FU
typedef struct {
  uint8_t dur      : 8;
} lis2hh12_act_dur_t;

#define LIS2HH12_CTRL1                       0x20U
typedef struct {
  uint8_t xen              : 1;
  uint8_t yen              : 1;
  uint8_t zen              : 1;
  uint8_t bdu              : 1;
  uint8_t odr              : 3;
  uint8_t hr               : 1;
} lis2hh12_ctrl1_t;

#define LIS2HH12_CTRL2                       0x21U
typedef struct {
  uint8_t hpis             : 2;
  uint8_t fds              : 1;
  uint8_t hpm              : 2;
  uint8_t dfc              : 2;
  uint8_t not_used_01      : 1;
} lis2hh12_ctrl2_t;

#define LIS2HH12_CTRL3                       0x22U
typedef struct {
  uint8_t int1_drdy        : 1;
  uint8_t int1_fth         : 1;
  uint8_t int1_ovr         : 1;
  uint8_t int1_ig1         : 1;
  uint8_t int1_ig2         : 1;
  uint8_t int1_inact       : 1;
  uint8_t stop_fth         : 1;
  uint8_t fifo_en          : 1;
} lis2hh12_ctrl3_t;

#define LIS2HH12_CTRL4                       0x23U
typedef struct {
  uint8_t sim              : 1;
  uint8_t i2c_disable      : 1;
  uint8_t if_add_inc       : 1;
  uint8_t bw_scale_odr     : 1;
  uint8_t fs               : 2;
  uint8_t bw               : 2;
} lis2hh12_ctrl4_t;

#define LIS2HH12_CTRL5                       0x24U
typedef struct {
  uint8_t pp_od            : 1;
  uint8_t h_lactive        : 1;
  uint8_t st               : 2;
  uint8_t dec              : 2;
  uint8_t soft_reset       : 1;
  uint8_t debug            : 1;
} lis2hh12_ctrl5_t;

#define LIS2HH12_CTRL6                       0x25U
typedef struct {
  uint8_t int2_drdy        : 1;
  uint8_t int2_fth         : 1;
  uint8_t int2_empty       : 1;
  uint8_t int2_ig1         : 1;
  uint8_t int2_ig2         : 1;
  uint8_t int2_boot        : 1;
  uint8_t not_used_01      : 1;
  uint8_t boot             : 1;
} lis2hh12_ctrl6_t;

#define LIS2HH12_CTRL7                       0x26U
typedef struct {
  uint8_t _4d_ig           : 2;
  uint8_t lir              : 2;
  uint8_t dcrm             : 2;
  uint8_t not_used_01      : 2;
} lis2hh12_ctrl7_t;

#define LIS2HH12_STATUS                      0x27U
typedef struct {
  uint8_t xda              : 1;
  uint8_t yda              : 1;
  uint8_t zda              : 1;
  uint8_t zyxda            : 1;
  uint8_t _xor             : 1;
  uint8_t yor              : 1;
  uint8_t zor              : 1;
  uint8_t zyxor            : 1;
} lis2hh12_status_t;

#define LIS2HH12_OUT_X_L                     0x28U
#define LIS2HH12_OUT_X_H                     0x29U
#define LIS2HH12_OUT_Y_L                     0x2AU
#define LIS2HH12_OUT_Y_H                     0x2BU
#define LIS2HH12_OUT_Z_L                     0x2CU
#define LIS2HH12_OUT_Z_H                     0x2DU
#define LIS2HH12_FIFO_CTRL                   0x2EU
typedef struct {
  uint8_t fth              : 5;
  uint8_t fmode            : 3;
} lis2hh12_fifo_ctrl_t;

#define LIS2HH12_FIFO_SRC                    0x2FU
typedef struct {
  uint8_t fss              : 5;
  uint8_t empty            : 1;
  uint8_t ovr              : 1;
  uint8_t fth              : 1;
} lis2hh12_fifo_src_t;

#define LIS2HH12_IG_CFG1                     0x30U
typedef struct {
  uint8_t xlie             : 1;
  uint8_t xhie             : 1;
  uint8_t ylie             : 1;
  uint8_t yhie             : 1;
  uint8_t zlie             : 1;
  uint8_t zhie             : 1;
  uint8_t _6d              : 1;
  uint8_t aoi              : 1;
} lis2hh12_ig_cfg1_t;

#define LIS2HH12_IG_SRC1                     0x31U
typedef struct {
  uint8_t xl               : 1;
  uint8_t xh               : 1;
  uint8_t yl               : 1;
  uint8_t yh               : 1;
  uint8_t zl               : 1;
  uint8_t zh               : 1;
  uint8_t ia               : 1;
  uint8_t not_used_01      : 1;
} lis2hh12_ig_src1_t;

#define LIS2HH12_IG_THS_X1                   0x32U
#define LIS2HH12_IG_THS_Y1                   0x33U
#define LIS2HH12_IG_THS_Z1                   0x34U
#define LIS2HH12_IG_DUR1                     0x35U
typedef struct {
  uint8_t dur1             : 7;
  uint8_t wait1            : 1;
} lis2hh12_ig_dur1_t;

#define LIS2HH12_IG_CFG2                     0x36U
typedef struct {
  uint8_t xlie             : 1;
  uint8_t xhie             : 1;
  uint8_t ylie             : 1;
  uint8_t yhie             : 1;
  uint8_t zlie             : 1;
  uint8_t zhie             : 1;
  uint8_t _6d              : 1;
  uint8_t aoi              : 1;
} lis2hh12_ig_cfg2_t;

#define LIS2HH12_IG_SRC2                     0x37U
typedef struct {
  uint8_t xl               : 1;
  uint8_t xh               : 1;
  uint8_t yl               : 1;
  uint8_t yh               : 1;
  uint8_t zl               : 1;
  uint8_t zh               : 1;
  uint8_t ia               : 1;
  uint8_t not_used_01      : 1;
} lis2hh12_ig_src2_t;

#define LIS2HH12_IG_THS2                     0x38U
#define LIS2HH12_IG_DUR2                     0x39U
typedef struct {
  uint8_t dur2             : 7;
  uint8_t wait2            : 1;
} lis2hh12_ig_dur2_t;

#define LIS2HH12_XL_REFERENCE                0x3AU
#define LIS2HH12_XH_REFERENCE                0x3BU
#define LIS2HH12_YL_REFERENCE                0x3CU
#define LIS2HH12_YH_REFERENCE                0x3DU
#define LIS2HH12_ZL_REFERENCE                0x3EU
#define LIS2HH12_ZH_REFERENCE                0x3FU

/**
  * @defgroup LIS2HH12_Register_Union
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
  lis2hh12_act_ths_t       act_ths;
  lis2hh12_act_dur_t       act_dur;
  lis2hh12_ctrl1_t         ctrl1;
  lis2hh12_ctrl2_t         ctrl2;
  lis2hh12_ctrl3_t         ctrl3;
  lis2hh12_ctrl4_t         ctrl4;
  lis2hh12_ctrl5_t         ctrl5;
  lis2hh12_ctrl6_t         ctrl6;
  lis2hh12_ctrl7_t         ctrl7;
  lis2hh12_status_t        status;
  lis2hh12_fifo_ctrl_t     fifo_ctrl;
  lis2hh12_fifo_src_t      fifo_src;
  lis2hh12_ig_cfg1_t       ig_cfg1;
  lis2hh12_ig_src1_t       ig_src1;
  lis2hh12_ig_dur1_t       ig_dur1;
  lis2hh12_ig_cfg2_t       ig_cfg2;
  lis2hh12_ig_src2_t       ig_src2;
  lis2hh12_ig_dur2_t       ig_dur2;
  bitwise_t                bitwise;
  uint8_t                  byte;
} lis2hh12_reg_t;

/**
  * @}
  *
  */

int32_t lis2hh12_read_reg(lis2hh12_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);
int32_t lis2hh12_write_reg(lis2hh12_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);

extern float_t lis2hh12_from_fs2g_to_mg(int16_t lsb);
extern float_t lis2hh12_from_fs4g_to_mg(int16_t lsb);
extern float_t lis2hh12_from_fs8g_to_mg(int16_t lsb);
extern float_t lis2hh12_from_lsb_to_celsius(int16_t lsb);

typedef struct {
  uint8_t xen              : 1;
  uint8_t yen              : 1;
  uint8_t zen              : 1;
} lis2hh12_xl_axis_t;
int32_t lis2hh12_xl_axis_set(lis2hh12_ctx_t *ctx, lis2hh12_xl_axis_t val);
int32_t lis2hh12_xl_axis_get(lis2hh12_ctx_t *ctx, lis2hh12_xl_axis_t *val);

int32_t lis2hh12_block_data_update_set(lis2hh12_ctx_t *ctx, uint8_t val);
int32_t lis2hh12_block_data_update_get(lis2hh12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2HH12_XL_ODR_OFF      = 0x00,
  LIS2HH12_XL_ODR_10Hz     = 0x01,
  LIS2HH12_XL_ODR_50Hz     = 0x02,
  LIS2HH12_XL_ODR_100Hz    = 0x03,
  LIS2HH12_XL_ODR_200Hz    = 0x04,
  LIS2HH12_XL_ODR_400Hz    = 0x05,
  LIS2HH12_XL_ODR_800Hz    = 0x06,
} lis2hh12_xl_data_rate_t;
int32_t lis2hh12_xl_data_rate_set(lis2hh12_ctx_t *ctx,
                                  lis2hh12_xl_data_rate_t val);
int32_t lis2hh12_xl_data_rate_get(lis2hh12_ctx_t *ctx,
                                  lis2hh12_xl_data_rate_t *val);

typedef enum {
  LIS2HH12_2g              = 0x00,
  LIS2HH12_16g             = 0x01,
  LIS2HH12_4g              = 0x02,
  LIS2HH12_8g              = 0x03,
} lis2hh12_xl_fs_t;
int32_t lis2hh12_xl_full_scale_set(lis2hh12_ctx_t *ctx,
                                   lis2hh12_xl_fs_t val);
int32_t lis2hh12_xl_full_scale_get(lis2hh12_ctx_t *ctx,
                                   lis2hh12_xl_fs_t *val);

typedef enum {
  LIS2HH12_NO_DECIMATION   = 0x00,
  LIS2HH12_EVERY_2_SAMPLES = 0x01,
  LIS2HH12_EVERY_4_SAMPLES = 0x02,
  LIS2HH12_EVERY_8_SAMPLES = 0x03,
} lis2hh12_dec_t;
int32_t lis2hh12_xl_decimation_set(lis2hh12_ctx_t *ctx, lis2hh12_dec_t val);
int32_t lis2hh12_xl_decimation_get(lis2hh12_ctx_t *ctx, lis2hh12_dec_t *val);

int32_t lis2hh12_xl_flag_data_ready_get(lis2hh12_ctx_t *ctx, uint8_t *val);

int32_t lis2hh12_temperature_raw_get(lis2hh12_ctx_t *ctx, uint8_t *buff);

int32_t lis2hh12_acceleration_raw_get(lis2hh12_ctx_t *ctx, uint8_t *buff);

int32_t lis2hh12_dev_id_get(lis2hh12_ctx_t *ctx, uint8_t *buff);

int32_t lis2hh12_dev_reset_set(lis2hh12_ctx_t *ctx, uint8_t val);
int32_t lis2hh12_dev_reset_get(lis2hh12_ctx_t *ctx, uint8_t *val);

int32_t lis2hh12_dev_boot_set(lis2hh12_ctx_t *ctx, uint8_t val);
int32_t lis2hh12_dev_boot_get(lis2hh12_ctx_t *ctx, uint8_t *val);

typedef struct {
  uint8_t xda              : 1;
  uint8_t yda              : 1;
  uint8_t zda              : 1;
  uint8_t zyxda            : 1;
  uint8_t _xor             : 1;
  uint8_t yor              : 1;
  uint8_t zor              : 1;
  uint8_t zyxor            : 1;
} lis2hh12_status_reg_t;
int32_t lis2hh12_dev_status_get(lis2hh12_ctx_t *ctx,
                                lis2hh12_status_reg_t *val);

typedef enum {
  LIS2HH12_HP_DISABLE      = 0x00,
  LIS2HH12_HP_ON_INT_GEN_1 = 0x02,
  LIS2HH12_HP_ON_INT_GEN_2 = 0x01,
  LIS2HH12_HP_ON_BOTH_GEN  = 0x03,
} lis2hh12_xl_hp_path_t;
int32_t lis2hh12_xl_filter_int_path_set(lis2hh12_ctx_t *ctx,
                                        lis2hh12_xl_hp_path_t val);
int32_t lis2hh12_xl_filter_int_path_get(lis2hh12_ctx_t *ctx,
                                        lis2hh12_xl_hp_path_t *val);

typedef enum {
  LIS2HH12_BYPASSED        = 0x00,
  LIS2HH12_FILT_HP         = 0x02,
  LIS2HH12_FILT_LP         = 0x01,
} lis2hh12_xl_out_path_t;
int32_t lis2hh12_xl_filter_out_path_set(lis2hh12_ctx_t *ctx,
                                        lis2hh12_xl_out_path_t val);
int32_t lis2hh12_xl_filter_out_path_get(lis2hh12_ctx_t *ctx,
                                        lis2hh12_xl_out_path_t *val);

typedef enum {
  LIS2HH12_HP_ODR_DIV_50          = 0x00,
  LIS2HH12_HP_ODR_DIV_100         = 0x10,
  LIS2HH12_HP_ODR_DIV_9           = 0x20,
  LIS2HH12_HP_ODR_DIV_400         = 0x30,
  LIS2HH12_HP_ODR_DIV_50_REF_MD   = 0x01,
  LIS2HH12_HP_ODR_DIV_100_REF_MD  = 0x11,
  LIS2HH12_HP_ODR_DIV_9_REF_MD    = 0x21,
  LIS2HH12_HP_ODR_DIV_400_REF_MD  = 0x31,
} lis2hh12_xl_hp_bw_t;
int32_t lis2hh12_xl_filter_hp_bandwidth_set(lis2hh12_ctx_t *ctx,
                                            lis2hh12_xl_hp_bw_t val);
int32_t lis2hh12_xl_filter_hp_bandwidth_get(lis2hh12_ctx_t *ctx,
                                            lis2hh12_xl_hp_bw_t *val);

typedef enum {
  LIS2HH12_LP_ODR_DIV_50   = 0,
  LIS2HH12_LP_ODR_DIV_100  = 1,
  LIS2HH12_LP_ODR_DIV_9    = 2,
  LIS2HH12_LP_ODR_DIV_400  = 3,
} lis2hh12_xl_lp_bw_t;
int32_t lis2hh12_xl_filter_low_bandwidth_set(lis2hh12_ctx_t *ctx,
                                             lis2hh12_xl_lp_bw_t val);
int32_t lis2hh12_xl_filter_low_bandwidth_get(lis2hh12_ctx_t *ctx,
                                             lis2hh12_xl_lp_bw_t *val);

typedef enum {
  LIS2HH12_AUTO      = 0x00,
  LIS2HH12_408Hz     = 0x10,
  LIS2HH12_211Hz     = 0x11,
  LIS2HH12_105Hz     = 0x12,
  LIS2HH12_50Hz      = 0x13,
} lis2hh12_xl_filt_aa_bw_t;
int32_t lis2hh12_xl_filter_aalias_bandwidth_set(lis2hh12_ctx_t *ctx,
                                                lis2hh12_xl_filt_aa_bw_t val);
int32_t lis2hh12_xl_filter_aalias_bandwidth_get(lis2hh12_ctx_t *ctx,
                                                lis2hh12_xl_filt_aa_bw_t *val);

int32_t lis2hh12_xl_filter_reference_set(lis2hh12_ctx_t *ctx, uint8_t *buff);
int32_t lis2hh12_xl_filter_reference_get(lis2hh12_ctx_t *ctx, uint8_t *buff);

typedef enum {
  LIS2HH12_SPI_4_WIRE = 0x00,
  LIS2HH12_SPI_3_WIRE = 0x01,
} lis2hh12_sim_t;
int32_t lis2hh12_spi_mode_set(lis2hh12_ctx_t *ctx, lis2hh12_sim_t val);
int32_t lis2hh12_spi_mode_get(lis2hh12_ctx_t *ctx, lis2hh12_sim_t *val);

typedef enum {
  LIS2HH12_I2C_ENABLE  = 0x00,
  LIS2HH12_I2C_DISABLE = 0x01,
} lis2hh12_i2c_dis_t;
int32_t lis2hh12_i2c_interface_set(lis2hh12_ctx_t *ctx,
                                   lis2hh12_i2c_dis_t val);
int32_t lis2hh12_i2c_interface_get(lis2hh12_ctx_t *ctx,
                                   lis2hh12_i2c_dis_t *val);

typedef enum {
  LIS2HH12_DISABLE = 0x00,
  LIS2HH12_ENABLE  = 0x01,
} lis2hh12_auto_inc_t;
int32_t lis2hh12_auto_increment_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_auto_inc_t val);
int32_t lis2hh12_auto_increment_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_auto_inc_t *val);

typedef struct {
  uint8_t int1_drdy            : 1;
  uint8_t int1_fth             : 1;
  uint8_t int1_ovr             : 1;
  uint8_t int1_ig1             : 1;
  uint8_t int1_ig2             : 1;
  uint8_t int1_inact           : 1;
} lis2hh12_pin_int1_route_t;
int32_t lis2hh12_pin_int1_route_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_pin_int1_route_t val);
int32_t lis2hh12_pin_int1_route_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_pin_int1_route_t *val);

typedef enum {
  LIS2HH12_PUSH_PULL       = 0x00,
  LIS2HH12_OPEN_DRAIN      = 0x01,
} lis2hh12_pp_od_t;
int32_t lis2hh12_pin_mode_set(lis2hh12_ctx_t *ctx, lis2hh12_pp_od_t val);
int32_t lis2hh12_pin_mode_get(lis2hh12_ctx_t *ctx, lis2hh12_pp_od_t *val);

typedef enum {
  LIS2HH12_ACTIVE_HIGH     = 0x00,
  LIS2HH12_ACTIVE_LOW      = 0x01,
} lis2hh12_pin_pol_t;
int32_t lis2hh12_pin_polarity_set(lis2hh12_ctx_t *ctx,
                                  lis2hh12_pin_pol_t val);
int32_t lis2hh12_pin_polarity_get(lis2hh12_ctx_t *ctx,
                                  lis2hh12_pin_pol_t *val);

typedef struct {
  uint8_t int2_drdy            : 1;
  uint8_t int2_fth             : 1;
  uint8_t int2_empty           : 1;
  uint8_t int2_ig1             : 1;
  uint8_t int2_ig2             : 1;
  uint8_t int2_boot            : 1;
} lis2hh12_pin_int2_route_t;
int32_t lis2hh12_pin_int2_route_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_pin_int2_route_t val);
int32_t lis2hh12_pin_int2_route_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_pin_int2_route_t *val);

typedef enum {
  LIS2HH12_INT_PULSED      = 0x00,
  LIS2HH12_INT_LATCHED     = 0x01,
} lis2hh12_lir_t;
int32_t lis2hh12_pin_notification_set(lis2hh12_ctx_t *ctx,
                                      lis2hh12_lir_t val);
int32_t lis2hh12_pin_notification_get(lis2hh12_ctx_t *ctx,
                                      lis2hh12_lir_t *val);
typedef enum {
  LIS2HH12_IG1_OR_IG2_OR   = 0x00,
  LIS2HH12_IG1_AND_IG2_OR  = 0x01,
  LIS2HH12_IG1_OR_IG2_AND  = 0x10,
  LIS2HH12_IG1_AND_IG2_AND = 0x11,
} lis2hh12_pin_logic_t;
int32_t lis2hh12_pin_logic_set(lis2hh12_ctx_t *ctx,
                               lis2hh12_pin_logic_t val);
int32_t lis2hh12_pin_logic_get(lis2hh12_ctx_t *ctx,
                               lis2hh12_pin_logic_t *val);

typedef enum {
  LIS2HH12_RESET_MODE      = 0x00,
  LIS2HH12_DECREMENT_MODE  = 0x01,
} lis2hh12_dcrm_t;
int32_t lis2hh12_xl_trshld_mode_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_dcrm_t val);
int32_t lis2hh12_xl_trshld_mode_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_dcrm_t *val);

typedef struct {
  uint16_t ig1_xlie             : 1;
  uint16_t ig1_xhie             : 1;
  uint16_t ig1_ylie             : 1;
  uint16_t ig1_yhie             : 1;
  uint16_t ig1_zlie             : 1;
  uint16_t ig1_zhie             : 1;
  uint16_t ig2_xlie             : 1;
  uint16_t ig2_xhie             : 1;
  uint16_t ig2_ylie             : 1;
  uint16_t ig2_yhie             : 1;
  uint16_t ig2_zlie             : 1;
  uint16_t ig2_zhie             : 1;
} lis2hh12_xl_trshld_en_t;
int32_t lis2hh12_xl_trshld_axis_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_xl_trshld_en_t val);
int32_t lis2hh12_xl_trshld_axis_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_xl_trshld_en_t *val);

typedef struct {
  uint16_t ig1_xl             : 1;
  uint16_t ig1_xh             : 1;
  uint16_t ig1_yl             : 1;
  uint16_t ig1_yh             : 1;
  uint16_t ig1_zl             : 1;
  uint16_t ig1_zh             : 1;
  uint16_t ig1_ia             : 1;
  uint16_t ig2_xl             : 1;
  uint16_t ig2_xh             : 1;
  uint16_t ig2_yl             : 1;
  uint16_t ig2_yh             : 1;
  uint16_t ig2_zl             : 1;
  uint16_t ig2_zh             : 1;
  uint16_t ig2_ia             : 1;
} lis2hh12_xl_trshld_src_t;
int32_t lis2hh12_xl_trshld_src_get(lis2hh12_ctx_t *ctx,
                                   lis2hh12_xl_trshld_src_t *val);

int32_t lis2hh12_xl_trshld_set(lis2hh12_ctx_t *ctx, uint8_t ig1_x,
                               uint8_t ig1_y, uint8_t ig1_z,
                               uint8_t ig2_xyz);
int32_t lis2hh12_xl_trshld_get(lis2hh12_ctx_t *ctx, uint8_t *ig1_x,
                               uint8_t *ig1_y, uint8_t *ig1_z,
                               uint8_t *ig2_xyz);

int32_t lis2hh12_xl_trshld_min_sample_set(lis2hh12_ctx_t *ctx,
                                          uint8_t ig1_sam, uint8_t ig2_sam);
int32_t lis2hh12_xl_trshld_min_sample_get(lis2hh12_ctx_t *ctx,
                                          uint8_t *ig1_sam, uint8_t *ig2_sam);

int32_t lis2hh12_act_threshold_set(lis2hh12_ctx_t *ctx, uint8_t val);
int32_t lis2hh12_act_threshold_get(lis2hh12_ctx_t *ctx, uint8_t *val);

int32_t lis2hh12_act_duration_set(lis2hh12_ctx_t *ctx, uint8_t val);
int32_t lis2hh12_act_duration_get(lis2hh12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2HH12_6D_4D_DISABLE       = 0x00,
  LIS2HH12_ENABLE_ON_IG1_6D    = 0x01,
  LIS2HH12_ENABLE_ON_IG2_6D    = 0x02,
  LIS2HH12_ENABLE_ON_IG1_4D    = 0x11,
  LIS2HH12_ENABLE_ON_IG2_4D    = 0x12,
} lis2hh12_6d_mode_t;
int32_t lis2hh12_6d_mode_set(lis2hh12_ctx_t *ctx, lis2hh12_6d_mode_t val);
int32_t lis2hh12_6d_mode_get(lis2hh12_ctx_t *ctx, lis2hh12_6d_mode_t *val);

int32_t lis2hh12_fifo_watermark_set(lis2hh12_ctx_t *ctx, uint8_t val);
int32_t lis2hh12_fifo_watermark_get(lis2hh12_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS2HH12_FIFO_OFF               = 0x00,
  LIS2HH12_BYPASS_MODE            = 0x10,
  LIS2HH12_FIFO_MODE              = 0x11,
  LIS2HH12_STREAM_MODE            = 0x12,
  LIS2HH12_STREAM_TO_FIFO_MODE   = 0x13,
  LIS2HH12_BYPASS_TO_STREAM_MODE = 0x14,
  LIS2HH12_BYPASS_TO_FIFO_MODE   = 0x17,
} lis2hh12_fifo_md_t;
int32_t lis2hh12_fifo_mode_set(lis2hh12_ctx_t *ctx, lis2hh12_fifo_md_t val);
int32_t lis2hh12_fifo_mode_get(lis2hh12_ctx_t *ctx, lis2hh12_fifo_md_t *val);

typedef struct {
  uint8_t fss             : 1;
  uint8_t empty           : 1;
  uint8_t ovr             : 1;
} lis2hh12_fifo_stat_t;
int32_t lis2hh12_fifo_status_get(lis2hh12_ctx_t *ctx,
                                 lis2hh12_fifo_stat_t *val);

typedef enum {
  LIS2HH12_ST_DISABLE    = 0x00,
  LIS2HH12_ST_POSITIVE   = 0x01,
  LIS2HH12_ST_NEGATIVE   = 0x02,
} lis2hh12_xl_st_t;
int32_t lis2hh12_xl_self_test_set(lis2hh12_ctx_t *ctx,
                                  lis2hh12_xl_st_t val);
int32_t lis2hh12_xl_self_test_get(lis2hh12_ctx_t *ctx,
                                  lis2hh12_xl_st_t *val);

/**
  *@}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* LIS2HH12_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/