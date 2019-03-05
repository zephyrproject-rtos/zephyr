/*
 ******************************************************************************
 * @file    lis3dh_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          lis3dh_reg.c driver.
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
#ifndef LIS3DH_REGS_H
#define LIS3DH_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LIS3DH
  * @{
  *
  */

/** @defgroup LIS3DH_sensors_common_types
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

/** @addtogroup LIS3MDL_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*lis3dh_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lis3dh_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lis3dh_write_ptr  write_reg;
  lis3dh_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lis3dh_ctx_t;

/**
  * @}
  *
  */

/** @defgroup LIS3DH_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format if SA0=0 -> 31 if SA0=1 -> 33 **/
#define LIS3DH_I2C_ADD_L   0x31U
#define LIS3DH_I2C_ADD_H   0x33U

/** Device Identification (Who am I) **/
#define LIS3DH_ID          0x33U

/**
  * @}
  *
  */

#define LIS3DH_STATUS_REG_AUX        0x07U
typedef struct {
  uint8_t _1da              : 1;
  uint8_t _2da              : 1;
  uint8_t _3da              : 1;
  uint8_t _321da            : 1;
  uint8_t _1or              : 1;
  uint8_t _2or              : 1;
  uint8_t _3or              : 1;
  uint8_t _321or            : 1;
} lis3dh_status_reg_aux_t;

#define LIS3DH_OUT_ADC1_L            0x08U
#define LIS3DH_OUT_ADC1_H            0x09U
#define LIS3DH_OUT_ADC2_L            0x0AU
#define LIS3DH_OUT_ADC2_H            0x0BU
#define LIS3DH_OUT_ADC3_L            0x0CU
#define LIS3DH_OUT_ADC3_H            0x0DU
#define LIS3DH_WHO_AM_I              0x0FU

#define LIS3DH_CTRL_REG0             0x1EU
typedef struct {
  uint8_t not_used_01       : 7;
  uint8_t sdo_pu_disc       : 1;
} lis3dh_ctrl_reg0_t;

#define LIS3DH_TEMP_CFG_REG          0x1FU
typedef struct {
  uint8_t not_used_01       : 6;
  uint8_t adc_pd            : 1;
  uint8_t temp_en           : 1;
} lis3dh_temp_cfg_reg_t;

#define LIS3DH_CTRL_REG1             0x20U
typedef struct {
  uint8_t xen               : 1;
  uint8_t yen               : 1;
  uint8_t zen               : 1;
  uint8_t lpen              : 1;
  uint8_t odr               : 4;
} lis3dh_ctrl_reg1_t;

#define LIS3DH_CTRL_REG2             0x21U
typedef struct {
  uint8_t hp                : 3; /* HPCLICK + HP_IA2 + HP_IA1 -> HP */
  uint8_t fds               : 1;
  uint8_t hpcf              : 2;
  uint8_t hpm               : 2;
} lis3dh_ctrl_reg2_t;

#define LIS3DH_CTRL_REG3             0x22U
typedef struct {
  uint8_t not_used_01       : 1;
  uint8_t i1_overrun        : 1;
  uint8_t i1_wtm            : 1;
  uint8_t not_used_02       : 1;
  uint8_t i1_zyxda          : 1;
  uint8_t i1_ia2            : 1;
  uint8_t i1_ia1            : 1;
  uint8_t i1_click          : 1;
} lis3dh_ctrl_reg3_t;

#define LIS3DH_CTRL_REG4             0x23U
typedef struct {
  uint8_t sim               : 1;
  uint8_t st                : 2;
  uint8_t hr                : 1;
  uint8_t fs                : 2;
  uint8_t ble               : 1;
  uint8_t bdu               : 1;
} lis3dh_ctrl_reg4_t;

#define LIS3DH_CTRL_REG5             0x24U
typedef struct {
  uint8_t d4d_int2          : 1;
  uint8_t lir_int2          : 1;
  uint8_t d4d_int1          : 1;
  uint8_t lir_int1          : 1;
  uint8_t not_used_01       : 2;
  uint8_t fifo_en           : 1;
  uint8_t boot              : 1;
} lis3dh_ctrl_reg5_t;

#define LIS3DH_CTRL_REG6            0x25U
typedef struct {
  uint8_t not_used_01       : 1;
  uint8_t int_polarity      : 1;
  uint8_t not_used_02       : 1;
  uint8_t i2_act            : 1;
  uint8_t i2_boot           : 1;
  uint8_t i2_ia2            : 1;
  uint8_t i2_ia1            : 1;
  uint8_t i2_click          : 1;
} lis3dh_ctrl_reg6_t;

#define LIS3DH_REFERENCE            0x26U
#define LIS3DH_STATUS_REG           0x27U
typedef struct {
  uint8_t xda               : 1;
  uint8_t yda               : 1;
  uint8_t zda               : 1;
  uint8_t zyxda             : 1;
  uint8_t _xor              : 1;
  uint8_t yor               : 1;
  uint8_t zor               : 1;
  uint8_t zyxor             : 1;
} lis3dh_status_reg_t;

#define LIS3DH_OUT_X_L              0x28U
#define LIS3DH_OUT_X_H              0x29U
#define LIS3DH_OUT_Y_L              0x2AU
#define LIS3DH_OUT_Y_H              0x2BU
#define LIS3DH_OUT_Z_L              0x2CU
#define LIS3DH_OUT_Z_H              0x2DU
#define LIS3DH_FIFO_CTRL_REG        0x2EU
typedef struct {
  uint8_t fth               : 5;
  uint8_t tr                : 1;
  uint8_t fm                : 2;
} lis3dh_fifo_ctrl_reg_t;

#define LIS3DH_FIFO_SRC_REG         0x2FU
typedef struct {
  uint8_t fss               : 5;
  uint8_t empty             : 1;
  uint8_t ovrn_fifo         : 1;
  uint8_t wtm               : 1;
} lis3dh_fifo_src_reg_t;

#define LIS3DH_INT1_CFG             0x30U
typedef struct {
  uint8_t xlie              : 1;
  uint8_t xhie              : 1;
  uint8_t ylie              : 1;
  uint8_t yhie              : 1;
  uint8_t zlie              : 1;
  uint8_t zhie              : 1;
  uint8_t _6d               : 1;
  uint8_t aoi               : 1;
} lis3dh_int1_cfg_t;

#define LIS3DH_INT1_SRC             0x31U
typedef struct {
  uint8_t xl                : 1;
  uint8_t xh                : 1;
  uint8_t yl                : 1;
  uint8_t yh                : 1;
  uint8_t zl                : 1;
  uint8_t zh                : 1;
  uint8_t ia                : 1;
  uint8_t not_used_01       : 1;
} lis3dh_int1_src_t;

#define LIS3DH_INT1_THS             0x32U
typedef struct {
  uint8_t ths               : 7;
  uint8_t not_used_01       : 1;
} lis3dh_int1_ths_t;

#define LIS3DH_INT1_DURATION        0x33U
typedef struct {
  uint8_t d                 : 7;
  uint8_t not_used_01       : 1;
} lis3dh_int1_duration_t;

#define LIS3DH_INT2_CFG             0x34U
typedef struct {
  uint8_t xlie              : 1;
  uint8_t xhie              : 1;
  uint8_t ylie              : 1;
  uint8_t yhie              : 1;
  uint8_t zlie              : 1;
  uint8_t zhie              : 1;
  uint8_t _6d               : 1;
  uint8_t aoi               : 1;
} lis3dh_int2_cfg_t;

#define LIS3DH_INT2_SRC             0x35U
typedef struct {
  uint8_t xl                : 1;
  uint8_t xh                : 1;
  uint8_t yl                : 1;
  uint8_t yh                : 1;
  uint8_t zl                : 1;
  uint8_t zh                : 1;
  uint8_t ia                : 1;
  uint8_t not_used_01       : 1;
} lis3dh_int2_src_t;

#define LIS3DH_INT2_THS             0x36U
typedef struct {
  uint8_t ths               : 7;
  uint8_t not_used_01       : 1;
} lis3dh_int2_ths_t;

#define LIS3DH_INT2_DURATION        0x37U
typedef struct {
  uint8_t d                 : 7;
  uint8_t not_used_01       : 1;
} lis3dh_int2_duration_t;

#define LIS3DH_CLICK_CFG            0x38U
typedef struct {
  uint8_t xs                : 1;
  uint8_t xd                : 1;
  uint8_t ys                : 1;
  uint8_t yd                : 1;
  uint8_t zs                : 1;
  uint8_t zd                : 1;
  uint8_t not_used_01       : 2;
} lis3dh_click_cfg_t;

#define LIS3DH_CLICK_SRC            0x39U
typedef struct {
  uint8_t x                 : 1;
  uint8_t y                 : 1;
  uint8_t z                 : 1;
  uint8_t sign              : 1;
  uint8_t sclick            : 1;
  uint8_t dclick            : 1;
  uint8_t ia                : 1;
  uint8_t not_used_01       : 1;
} lis3dh_click_src_t;

#define LIS3DH_CLICK_THS            0x3AU
typedef struct {
  uint8_t ths               : 7;
  uint8_t lir_click         : 1;
} lis3dh_click_ths_t;

#define LIS3DH_TIME_LIMIT           0x3BU
typedef struct {
  uint8_t tli               : 7;
  uint8_t not_used_01       : 1;
} lis3dh_time_limit_t;

#define LIS3DH_TIME_LATENCY         0x3CU
typedef struct {
  uint8_t tla               : 8;
} lis3dh_time_latency_t;

#define LIS3DH_TIME_WINDOW          0x3DU
typedef struct {
  uint8_t tw                : 8;
} lis3dh_time_window_t;

#define LIS3DH_ACT_THS              0x3EU
typedef struct {
  uint8_t acth              : 7;
  uint8_t not_used_01       : 1;
} lis3dh_act_ths_t;

#define LIS3DH_ACT_DUR              0x3FU
typedef struct {
  uint8_t actd              : 8;
} lis3dh_act_dur_t;

/**
  * @defgroup LIS3DH_Register_Union
  * @brief    This union group all the registers that has a bitfield
  *           description.
  *           This union is usefull but not need by the driver.
  *
  *           REMOVING this union you are complient with:
  *           MISRA-C 2012 [Rule 19.2] -> " Union are not allowed "
  *
  * @{
  *
  */
typedef union{
  lis3dh_status_reg_aux_t status_reg_aux;
  lis3dh_ctrl_reg0_t      ctrl_reg0;
  lis3dh_temp_cfg_reg_t   temp_cfg_reg;
  lis3dh_ctrl_reg1_t      ctrl_reg1;
  lis3dh_ctrl_reg2_t      ctrl_reg2;
  lis3dh_ctrl_reg3_t      ctrl_reg3;
  lis3dh_ctrl_reg4_t      ctrl_reg4;
  lis3dh_ctrl_reg5_t      ctrl_reg5;
  lis3dh_ctrl_reg6_t      ctrl_reg6;
  lis3dh_status_reg_t     status_reg;
  lis3dh_fifo_ctrl_reg_t  fifo_ctrl_reg;
  lis3dh_fifo_src_reg_t   fifo_src_reg;
  lis3dh_int1_cfg_t       int1_cfg;
  lis3dh_int1_src_t       int1_src;
  lis3dh_int1_ths_t       int1_ths;
  lis3dh_int1_duration_t  int1_duration;
  lis3dh_int2_cfg_t       int2_cfg;
  lis3dh_int2_src_t       int2_src;
  lis3dh_int2_ths_t       int2_ths;
  lis3dh_int2_duration_t  int2_duration;
  lis3dh_click_cfg_t      click_cfg;
  lis3dh_click_src_t      click_src;
  lis3dh_click_ths_t      click_ths;
  lis3dh_time_limit_t     time_limit;
  lis3dh_time_latency_t   time_latency;
  lis3dh_time_window_t    time_window;
  lis3dh_act_ths_t        act_ths;
  lis3dh_act_dur_t        act_dur;
  bitwise_t                 bitwise;
  uint8_t                   byte;
} lis3dh_reg_t;

/**
  * @}
  *
  */

int32_t lis3dh_read_reg(lis3dh_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);
int32_t lis3dh_write_reg(lis3dh_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);

extern float lis3dh_from_fs2_hr_to_mg(int16_t lsb);
extern float lis3dh_from_fs4_hr_to_mg(int16_t lsb);
extern float lis3dh_from_fs8_hr_to_mg(int16_t lsb);
extern float lis3dh_from_fs16_hr_to_mg(int16_t lsb);
extern float lis3dh_from_lsb_hr_to_celsius(int16_t lsb);

extern float lis3dh_from_fs2_nm_to_mg(int16_t lsb);
extern float lis3dh_from_fs4_nm_to_mg(int16_t lsb);
extern float lis3dh_from_fs8_nm_to_mg(int16_t lsb);
extern float lis3dh_from_fs16_nm_to_mg(int16_t lsb);
extern float lis3dh_from_lsb_nm_to_celsius(int16_t lsb);

extern float lis3dh_from_fs2_lp_to_mg(int16_t lsb);
extern float lis3dh_from_fs4_lp_to_mg(int16_t lsb);
extern float lis3dh_from_fs8_lp_to_mg(int16_t lsb);
extern float lis3dh_from_fs16_lp_to_mg(int16_t lsb);
extern float lis3dh_from_lsb_lp_to_celsius(int16_t lsb);

int32_t lis3dh_temp_status_reg_get(lis3dh_ctx_t *ctx, uint8_t *buff);
int32_t lis3dh_temp_data_ready_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_temp_data_ovr_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_temperature_raw_get(lis3dh_ctx_t *ctx, uint8_t *buff);

int32_t lis3dh_adc_raw_get(lis3dh_ctx_t *ctx, uint8_t *buff);

typedef enum {
  LIS3DH_AUX_DISABLE          = 0,
  LIS3DH_AUX_ON_TEMPERATURE   = 3,
  LIS3DH_AUX_ON_PADS          = 1,
} lis3dh_temp_en_t;
int32_t lis3dh_aux_adc_set(lis3dh_ctx_t *ctx, lis3dh_temp_en_t val);
int32_t lis3dh_aux_adc_get(lis3dh_ctx_t *ctx, lis3dh_temp_en_t *val);

typedef enum {
  LIS3DH_HR_12bit   = 0,
  LIS3DH_NM_10bit   = 1,
  LIS3DH_LP_8bit    = 2,
} lis3dh_op_md_t;
int32_t lis3dh_operating_mode_set(lis3dh_ctx_t *ctx,
                                    lis3dh_op_md_t val);
int32_t lis3dh_operating_mode_get(lis3dh_ctx_t *ctx,
                                    lis3dh_op_md_t *val);

typedef enum {
  LIS3DH_POWER_DOWN                      = 0x00,
  LIS3DH_ODR_1Hz                         = 0x01,
  LIS3DH_ODR_10Hz                        = 0x02,
  LIS3DH_ODR_25Hz                        = 0x03,
  LIS3DH_ODR_50Hz                        = 0x04,
  LIS3DH_ODR_100Hz                       = 0x05,
  LIS3DH_ODR_200Hz                       = 0x06,
  LIS3DH_ODR_400Hz                       = 0x07,
  LIS3DH_ODR_1kHz620_LP                  = 0x08,
  LIS3DH_ODR_5kHz376_LP_1kHz344_NM_HP    = 0x09,
} lis3dh_odr_t;
int32_t lis3dh_data_rate_set(lis3dh_ctx_t *ctx, lis3dh_odr_t val);
int32_t lis3dh_data_rate_get(lis3dh_ctx_t *ctx, lis3dh_odr_t *val);

int32_t lis3dh_high_pass_on_outputs_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_high_pass_on_outputs_get(lis3dh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DH_AGGRESSIVE  = 0,
  LIS3DH_STRONG      = 1,
  LIS3DH_MEDIUM      = 2,
  LIS3DH_LIGHT       = 3,
} lis3dh_hpcf_t;
int32_t lis3dh_high_pass_bandwidth_set(lis3dh_ctx_t *ctx,
                                         lis3dh_hpcf_t val);
int32_t lis3dh_high_pass_bandwidth_get(lis3dh_ctx_t *ctx,
                                         lis3dh_hpcf_t *val);

typedef enum {
  LIS3DH_NORMAL_WITH_RST  = 0,
  LIS3DH_REFERENCE_MODE   = 1,
  LIS3DH_NORMAL           = 2,
  LIS3DH_AUTORST_ON_INT   = 3,
} lis3dh_hpm_t;
int32_t lis3dh_high_pass_mode_set(lis3dh_ctx_t *ctx, lis3dh_hpm_t val);
int32_t lis3dh_high_pass_mode_get(lis3dh_ctx_t *ctx, lis3dh_hpm_t *val);

typedef enum {
  LIS3DH_2g   = 0,
  LIS3DH_4g   = 1,
  LIS3DH_8g   = 2,
  LIS3DH_16g  = 3,
} lis3dh_fs_t;
int32_t lis3dh_full_scale_set(lis3dh_ctx_t *ctx, lis3dh_fs_t val);
int32_t lis3dh_full_scale_get(lis3dh_ctx_t *ctx, lis3dh_fs_t *val);

int32_t lis3dh_block_data_update_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_block_data_update_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_filter_reference_set(lis3dh_ctx_t *ctx, uint8_t *buff);
int32_t lis3dh_filter_reference_get(lis3dh_ctx_t *ctx, uint8_t *buff);

int32_t lis3dh_xl_data_ready_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_xl_data_ovr_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_acceleration_raw_get(lis3dh_ctx_t *ctx, uint8_t *buff);

int32_t lis3dh_device_id_get(lis3dh_ctx_t *ctx, uint8_t *buff);

typedef enum {
  LIS3DH_ST_DISABLE   = 0,
  LIS3DH_ST_POSITIVE  = 1,
  LIS3DH_ST_NEGATIVE  = 2,
} lis3dh_st_t;
int32_t lis3dh_self_test_set(lis3dh_ctx_t *ctx, lis3dh_st_t val);
int32_t lis3dh_self_test_get(lis3dh_ctx_t *ctx, lis3dh_st_t *val);

typedef enum {
  LIS3DH_LSB_AT_LOW_ADD = 0,
  LIS3DH_MSB_AT_LOW_ADD = 1,
} lis3dh_ble_t;
int32_t lis3dh_data_format_set(lis3dh_ctx_t *ctx, lis3dh_ble_t val);
int32_t lis3dh_data_format_get(lis3dh_ctx_t *ctx, lis3dh_ble_t *val);

int32_t lis3dh_boot_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_boot_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_status_get(lis3dh_ctx_t *ctx, lis3dh_status_reg_t *val);

int32_t lis3dh_int1_gen_conf_set(lis3dh_ctx_t *ctx,
                                   lis3dh_int1_cfg_t *val);
int32_t lis3dh_int1_gen_conf_get(lis3dh_ctx_t *ctx,
                                   lis3dh_int1_cfg_t *val);

int32_t lis3dh_int1_gen_source_get(lis3dh_ctx_t *ctx,
                                     lis3dh_int1_src_t *val);

int32_t lis3dh_int1_gen_threshold_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_int1_gen_threshold_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_int1_gen_duration_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_int1_gen_duration_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_int2_gen_conf_set(lis3dh_ctx_t *ctx,
                                   lis3dh_int2_cfg_t *val);
int32_t lis3dh_int2_gen_conf_get(lis3dh_ctx_t *ctx,
                                   lis3dh_int2_cfg_t *val);

int32_t lis3dh_int2_gen_source_get(lis3dh_ctx_t *ctx,
                                     lis3dh_int2_src_t *val);

int32_t lis3dh_int2_gen_threshold_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_int2_gen_threshold_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_int2_gen_duration_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_int2_gen_duration_get(lis3dh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DH_DISC_FROM_INT_GENERATOR  = 0,
  LIS3DH_ON_INT1_GEN              = 1,
  LIS3DH_ON_INT2_GEN              = 2,
  LIS3DH_ON_TAP_GEN               = 4,
  LIS3DH_ON_INT1_INT2_GEN         = 3,
  LIS3DH_ON_INT1_TAP_GEN          = 5,
  LIS3DH_ON_INT2_TAP_GEN          = 6,
  LIS3DH_ON_INT1_INT2_TAP_GEN     = 7,
} lis3dh_hp_t;
int32_t lis3dh_high_pass_int_conf_set(lis3dh_ctx_t *ctx,
                                        lis3dh_hp_t val);
int32_t lis3dh_high_pass_int_conf_get(lis3dh_ctx_t *ctx,
                                        lis3dh_hp_t *val);

int32_t lis3dh_pin_int1_config_set(lis3dh_ctx_t *ctx,
                                     lis3dh_ctrl_reg3_t *val);
int32_t lis3dh_pin_int1_config_get(lis3dh_ctx_t *ctx,
                                     lis3dh_ctrl_reg3_t *val);

int32_t lis3dh_int2_pin_detect_4d_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_int2_pin_detect_4d_get(lis3dh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DH_INT2_PULSED   = 0,
  LIS3DH_INT2_LATCHED  = 1,
} lis3dh_lir_int2_t;
int32_t lis3dh_int2_pin_notification_mode_set(lis3dh_ctx_t *ctx,
                                                lis3dh_lir_int2_t val);
int32_t lis3dh_int2_pin_notification_mode_get(lis3dh_ctx_t *ctx,
                                                lis3dh_lir_int2_t *val);

int32_t lis3dh_int1_pin_detect_4d_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_int1_pin_detect_4d_get(lis3dh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DH_INT1_PULSED   = 0,
  LIS3DH_INT1_LATCHED  = 1,
} lis3dh_lir_int1_t;
int32_t lis3dh_int1_pin_notification_mode_set(lis3dh_ctx_t *ctx,
                                                lis3dh_lir_int1_t val);
int32_t lis3dh_int1_pin_notification_mode_get(lis3dh_ctx_t *ctx,
                                                lis3dh_lir_int1_t *val);

int32_t lis3dh_pin_int2_config_set(lis3dh_ctx_t *ctx,
                                     lis3dh_ctrl_reg6_t *val);
int32_t lis3dh_pin_int2_config_get(lis3dh_ctx_t *ctx,
                                     lis3dh_ctrl_reg6_t *val);

int32_t lis3dh_fifo_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_fifo_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_fifo_watermark_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_fifo_watermark_get(lis3dh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DH_INT1_GEN = 0,
  LIS3DH_INT2_GEN = 1,
} lis3dh_tr_t;
int32_t lis3dh_fifo_trigger_event_set(lis3dh_ctx_t *ctx,
                                        lis3dh_tr_t val);
int32_t lis3dh_fifo_trigger_event_get(lis3dh_ctx_t *ctx,
                                        lis3dh_tr_t *val);

typedef enum {
  LIS3DH_BYPASS_MODE           = 0,
  LIS3DH_FIFO_MODE             = 1,
  LIS3DH_DYNAMIC_STREAM_MODE   = 2,
  LIS3DH_STREAM_TO_FIFO_MODE   = 3,
} lis3dh_fm_t;
int32_t lis3dh_fifo_mode_set(lis3dh_ctx_t *ctx, lis3dh_fm_t val);
int32_t lis3dh_fifo_mode_get(lis3dh_ctx_t *ctx, lis3dh_fm_t *val);

int32_t lis3dh_fifo_status_get(lis3dh_ctx_t *ctx,
                                 lis3dh_fifo_src_reg_t *val);

int32_t lis3dh_fifo_data_level_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_fifo_empty_flag_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_fifo_ovr_flag_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_fifo_fth_flag_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_tap_conf_set(lis3dh_ctx_t *ctx, lis3dh_click_cfg_t *val);
int32_t lis3dh_tap_conf_get(lis3dh_ctx_t *ctx, lis3dh_click_cfg_t *val);

int32_t lis3dh_tap_source_get(lis3dh_ctx_t *ctx,
                                lis3dh_click_src_t *val);

int32_t lis3dh_tap_threshold_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_tap_threshold_get(lis3dh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DH_TAP_PULSED   = 0,
  LIS3DH_TAP_LATCHED  = 1,
} lis3dh_lir_click_t;
int32_t lis3dh_tap_notification_mode_set(lis3dh_ctx_t *ctx,
                                           lis3dh_lir_click_t val);
int32_t lis3dh_tap_notification_mode_get(lis3dh_ctx_t *ctx,
                                           lis3dh_lir_click_t *val);

int32_t lis3dh_shock_dur_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_shock_dur_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_quiet_dur_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_quiet_dur_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_double_tap_timeout_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_double_tap_timeout_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_act_threshold_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_act_threshold_get(lis3dh_ctx_t *ctx, uint8_t *val);

int32_t lis3dh_act_timeout_set(lis3dh_ctx_t *ctx, uint8_t val);
int32_t lis3dh_act_timeout_get(lis3dh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DH_PULL_UP_DISCONNECT  = 0,
  LIS3DH_PULL_UP_CONNECT     = 1,
} lis3dh_sdo_pu_disc_t;
int32_t lis3dh_pin_sdo_sa0_mode_set(lis3dh_ctx_t *ctx,
                                      lis3dh_sdo_pu_disc_t val);
int32_t lis3dh_pin_sdo_sa0_mode_get(lis3dh_ctx_t *ctx,
                                      lis3dh_sdo_pu_disc_t *val);

typedef enum {
  LIS3DH_SPI_4_WIRE = 0,
  LIS3DH_SPI_3_WIRE = 1,
} lis3dh_sim_t;
int32_t lis3dh_spi_mode_set(lis3dh_ctx_t *ctx, lis3dh_sim_t val);
int32_t lis3dh_spi_mode_get(lis3dh_ctx_t *ctx, lis3dh_sim_t *val);

/**
  * @}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* LIS3DH_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
