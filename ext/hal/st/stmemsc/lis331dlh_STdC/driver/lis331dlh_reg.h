/*
 ******************************************************************************
 * @file    lis331dlh_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          lis331dlh_reg.c driver.
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
#ifndef LIS331DLH_REGS_H
#define LIS331DLH_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LIS331DLH
  * @{
  *
  */

/** @defgroup LIS331DLH_sensors_common_types
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

/** @addtogroup  LIS331DLH_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*lis331dlh_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lis331dlh_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lis331dlh_write_ptr  write_reg;
  lis331dlh_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lis331dlh_ctx_t;

/**
  * @}
  *
  */


/** @defgroup LIS331DLH_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> 0x31 if SA0=1 -> 0x33 **/
#define LIS331DLH_I2C_ADD_L     0x31U
#define LIS331DLH_I2C_ADD_H     0x33U

/** Device Identification (Who am I) **/
#define LIS331DLH_ID            0x32U

/**
  * @}
  *
  */

/**
  * @addtogroup  LIS331DLH_Sensitivity
  * @brief       These macro are maintained for back compatibility.
  *              in order to convert data into engineering units please
  *              use functions:
  *                -> _from_fs2_to_mg(int16_t lsb);
  *                -> _from_fs4_to_mg(int16_t lsb);
  *                -> _from_fs8_to_mg(int16_t lsb);
  *
  *              REMOVING the MACRO you are compliant with:
  *              MISRA-C 2012 [Dir 4.9] -> " avoid function-like macros "
  * @{
  *
  */

#define LIS331DLH_FROM_FS_2g_TO_mg(lsb)    (float)( (lsb >> 4 ) * 1.0f )
#define LIS331DLH_FROM_FS_4g_TO_mg(lsb)    (float)( (lsb >> 4 ) * 2.0f )
#define LIS331DLH_FROM_FS_8g_TO_mg(lsb)    (float)( (lsb >> 4 ) * 3.9f )

/**
  * @}
  *
  */

#define LIS331DLH_WHO_AM_I                  0x0FU
#define LIS331DLH_CTRL_REG1                 0x20U
typedef struct {
  uint8_t xen                      : 1;
  uint8_t yen                      : 1;
  uint8_t zen                      : 1;
  uint8_t dr                       : 2;
  uint8_t pm                       : 3;
} lis331dlh_ctrl_reg1_t;

#define LIS331DLH_CTRL_REG2                 0x21U
typedef struct {
  uint8_t hpcf                     : 2;
  uint8_t hpen                     : 2;
  uint8_t fds                      : 1;
  uint8_t hpm                      : 2;
  uint8_t boot                     : 1;
} lis331dlh_ctrl_reg2_t;

#define LIS331DLH_CTRL_REG3                 0x22U
typedef struct {
  uint8_t i1_cfg                   : 2;
  uint8_t lir1                     : 1;
  uint8_t i2_cfg                   : 2;
  uint8_t lir2                     : 1;
  uint8_t pp_od                    : 1;
  uint8_t ihl                      : 1;
} lis331dlh_ctrl_reg3_t;

#define LIS331DLH_CTRL_REG4                 0x23U
typedef struct {
  uint8_t sim                      : 1;
  uint8_t st                       : 3; /* STsign + ST */
  uint8_t fs                       : 2;
  uint8_t ble                      : 1;
  uint8_t bdu                      : 1;
} lis331dlh_ctrl_reg4_t;

#define LIS331DLH_CTRL_REG5                 0x24U
typedef struct {
  uint8_t turnon                   : 2;
  uint8_t not_used_01              : 6;
} lis331dlh_ctrl_reg5_t;

#define LIS331DLH_HP_FILTER_RESET           0x25U
#define LIS331DLH_REFERENCE                 0x26U
#define LIS331DLH_STATUS_REG                0x27U
typedef struct {
  uint8_t xda                      : 1;
  uint8_t yda                      : 1;
  uint8_t zda                      : 1;
  uint8_t zyxda                    : 1;
  uint8_t _xor                     : 1;
  uint8_t yor                      : 1;
  uint8_t zor                      : 1;
  uint8_t zyxor                    : 1;
} lis331dlh_status_reg_t;

#define LIS331DLH_OUT_X_L                   0x28U
#define LIS331DLH_OUT_X_H                   0x29U
#define LIS331DLH_OUT_Y_L                   0x2AU
#define LIS331DLH_OUT_Y_H                   0x2BU
#define LIS331DLH_OUT_Z_L                   0x2CU
#define LIS331DLH_OUT_Z_H                   0x2DU
#define LIS331DLH_INT1_CFG                  0x30U
typedef struct {
  uint8_t xlie                     : 1;
  uint8_t xhie                     : 1;
  uint8_t ylie                     : 1;
  uint8_t yhie                     : 1;
  uint8_t zlie                     : 1;
  uint8_t zhie                     : 1;
  uint8_t _6d                      : 1;
  uint8_t aoi                      : 1;
} lis331dlh_int1_cfg_t;

#define LIS331DLH_INT1_SRC                  0x31U
typedef struct {
  uint8_t xl                       : 1;
  uint8_t xh                       : 1;
  uint8_t yl                       : 1;
  uint8_t yh                       : 1;
  uint8_t zl                       : 1;
  uint8_t zh                       : 1;
  uint8_t ia                       : 1;
  uint8_t not_used_01              : 1;
} lis331dlh_int1_src_t;

#define LIS331DLH_INT1_THS                  0x32U
typedef struct {
  uint8_t ths                      : 7;
  uint8_t not_used_01              : 1;
} lis331dlh_int1_ths_t;

#define LIS331DLH_INT1_DURATION             0x33U
typedef struct {
  uint8_t d                        : 7;
  uint8_t not_used_01              : 1;
} lis331dlh_int1_duration_t;

#define LIS331DLH_INT2_CFG                  0x34U
typedef struct {
  uint8_t xlie                     : 1;
  uint8_t xhie                     : 1;
  uint8_t ylie                     : 1;
  uint8_t yhie                     : 1;
  uint8_t zlie                     : 1;
  uint8_t zhie                     : 1;
  uint8_t _6d                      : 1;
  uint8_t aoi                      : 1;
} lis331dlh_int2_cfg_t;

#define LIS331DLH_INT2_SRC                  0x35U
typedef struct {
  uint8_t xl                       : 1;
  uint8_t xh                       : 1;
  uint8_t yl                       : 1;
  uint8_t yh                       : 1;
  uint8_t zl                       : 1;
  uint8_t zh                       : 1;
  uint8_t ia                       : 1;
  uint8_t not_used_01              : 1;
} lis331dlh_int2_src_t;

#define LIS331DLH_INT2_THS                  0x36U
typedef struct {
  uint8_t ths                      : 7;
  uint8_t not_used_01              : 1;
} lis331dlh_int2_ths_t;

#define LIS331DLH_INT2_DURATION             0x37U
typedef struct {
  uint8_t d                        : 7;
  uint8_t not_used_01              : 1;
} lis331dlh_int2_duration_t;

/**
  * @defgroup LIS331DLH_Register_Union
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
  lis331dlh_ctrl_reg1_t                     ctrl_reg1;
  lis331dlh_ctrl_reg2_t                     ctrl_reg2;
  lis331dlh_ctrl_reg3_t                     ctrl_reg3;
  lis331dlh_ctrl_reg4_t                     ctrl_reg4;
  lis331dlh_ctrl_reg5_t                     ctrl_reg5;
  lis331dlh_status_reg_t                    status_reg;
  lis331dlh_int1_cfg_t                      int1_cfg;
  lis331dlh_int1_src_t                      int1_src;
  lis331dlh_int1_ths_t                      int1_ths;
  lis331dlh_int1_duration_t                 int1_duration;
  lis331dlh_int2_cfg_t                      int2_cfg;
  lis331dlh_int2_src_t                      int2_src;
  lis331dlh_int2_ths_t                      int2_ths;
  lis331dlh_int2_duration_t                 int2_duration;
  bitwise_t                                 bitwise;
  uint8_t                                   byte;
} lis331dlh_reg_t;

/**
  * @}
  *
  */

int32_t lis331dlh_read_reg(lis331dlh_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);
int32_t lis331dlh_write_reg(lis331dlh_ctx_t *ctx, uint8_t reg, uint8_t* data,
                            uint16_t len);

extern float lis331dlh_from_fs2_to_mg(int16_t lsb);
extern float lis331dlh_from_fs4_to_mg(int16_t lsb);
extern float lis331dlh_from_fs8_to_mg(int16_t lsb);

int32_t lis331dlh_axis_x_data_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_axis_x_data_get(lis331dlh_ctx_t *ctx, uint8_t *val);

int32_t lis331dlh_axis_y_data_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_axis_y_data_get(lis331dlh_ctx_t *ctx, uint8_t *val);

int32_t lis331dlh_axis_z_data_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_axis_z_data_get(lis331dlh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS331DLH_ODR_OFF   = 0x00,
  LIS331DLH_ODR_Hz5   = 0x02,
  LIS331DLH_ODR_1Hz   = 0x03,
  LIS331DLH_ODR_2Hz   = 0x04,
  LIS331DLH_ODR_5Hz   = 0x05,
  LIS331DLH_ODR_10Hz  = 0x06,
  LIS331DLH_ODR_50Hz  = 0x01,
  LIS331DLH_ODR_100Hz = 0x11,
  LIS331DLH_ODR_400Hz = 0x21,
  LIS331DLH_ODR_1kHz  = 0x31,
} lis331dlh_dr_t;
int32_t lis331dlh_data_rate_set(lis331dlh_ctx_t *ctx, lis331dlh_dr_t val);
int32_t lis331dlh_data_rate_get(lis331dlh_ctx_t *ctx, lis331dlh_dr_t *val);

typedef enum {
  LIS331DLH_NORMAL_MODE      = 0,
  LIS331DLH_REF_MODE_ENABLE  = 1,
} lis331dlh_hpm_t;
int32_t lis331dlh_reference_mode_set(lis331dlh_ctx_t *ctx,
                                     lis331dlh_hpm_t val);
int32_t lis331dlh_reference_mode_get(lis331dlh_ctx_t *ctx,
                                     lis331dlh_hpm_t *val);

typedef enum {
  LIS331DLH_2g  = 0,
  LIS331DLH_4g  = 1,
  LIS331DLH_8g  = 3,
} lis331dlh_fs_t;
int32_t lis331dlh_full_scale_set(lis331dlh_ctx_t *ctx, lis331dlh_fs_t val);
int32_t lis331dlh_full_scale_get(lis331dlh_ctx_t *ctx, lis331dlh_fs_t *val);

int32_t lis331dlh_block_data_update_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_block_data_update_get(lis331dlh_ctx_t *ctx, uint8_t *val);

int32_t lis331dlh_status_reg_get(lis331dlh_ctx_t *ctx,
                                 lis331dlh_status_reg_t *val);

int32_t lis331dlh_flag_data_ready_get(lis331dlh_ctx_t *ctx,
                                      uint8_t *val);

int32_t lis331dlh_acceleration_raw_get(lis331dlh_ctx_t *ctx, uint8_t *buff);

int32_t lis331dlh_device_id_get(lis331dlh_ctx_t *ctx, uint8_t *buff);

int32_t lis331dlh_boot_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_boot_get(lis331dlh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS331DLH_ST_DISABLE   = 0,
  LIS331DLH_ST_POSITIVE  = 1,
  LIS331DLH_ST_NEGATIVE  = 5,
} lis331dlh_st_t;
int32_t lis331dlh_self_test_set(lis331dlh_ctx_t *ctx, lis331dlh_st_t val);
int32_t lis331dlh_self_test_get(lis331dlh_ctx_t *ctx, lis331dlh_st_t *val);

typedef enum {
  LIS331DLH_LSB_AT_LOW_ADD  = 0,
  LIS331DLH_MSB_AT_LOW_ADD  = 1,
} lis331dlh_ble_t;
int32_t lis331dlh_data_format_set(lis331dlh_ctx_t *ctx, lis331dlh_ble_t val);
int32_t lis331dlh_data_format_get(lis331dlh_ctx_t *ctx, lis331dlh_ble_t *val);

typedef enum {
  LIS331DLH_CUT_OFF_8Hz   = 0,
  LIS331DLH_CUT_OFF_16Hz  = 1,
  LIS331DLH_CUT_OFF_32Hz  = 2,
  LIS331DLH_CUT_OFF_64Hz  = 3,
} lis331dlh_hpcf_t;
int32_t lis331dlh_hp_bandwidth_set(lis331dlh_ctx_t *ctx,
                                   lis331dlh_hpcf_t val);
int32_t lis331dlh_hp_bandwidth_get(lis331dlh_ctx_t *ctx,
                                   lis331dlh_hpcf_t *val);

typedef enum {
  LIS331DLH_HP_DISABLE            = 0,
  LIS331DLH_HP_ON_OUT             = 4,
  LIS331DLH_HP_ON_INT1            = 1,
  LIS331DLH_HP_ON_INT2            = 2,
  LIS331DLH_HP_ON_INT1_INT2       = 3,
  LIS331DLH_HP_ON_INT1_INT2_OUT   = 7,
  LIS331DLH_HP_ON_INT2_OUT        = 6,
  LIS331DLH_HP_ON_INT1_OUT        = 5,
} lis331dlh_hpen_t;
int32_t lis331dlh_hp_path_set(lis331dlh_ctx_t *ctx, lis331dlh_hpen_t val);
int32_t lis331dlh_hp_path_get(lis331dlh_ctx_t *ctx, lis331dlh_hpen_t *val);

int32_t lis331dlh_hp_reset_get(lis331dlh_ctx_t *ctx);

int32_t lis331dlh_hp_reference_value_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_hp_reference_value_get(lis331dlh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS331DLH_SPI_4_WIRE  = 0,
  LIS331DLH_SPI_3_WIRE  = 1,
} lis331dlh_sim_t;
int32_t lis331dlh_spi_mode_set(lis331dlh_ctx_t *ctx, lis331dlh_sim_t val);
int32_t lis331dlh_spi_mode_get(lis331dlh_ctx_t *ctx, lis331dlh_sim_t *val);

typedef enum {
  LIS331DLH_PAD1_INT1_SRC           = 0,
  LIS331DLH_PAD1_INT1_OR_INT2_SRC   = 1,
  LIS331DLH_PAD1_DRDY               = 2,
  LIS331DLH_PAD1_BOOT               = 3,
} lis331dlh_i1_cfg_t;
int32_t lis331dlh_pin_int1_route_set(lis331dlh_ctx_t *ctx,
                                     lis331dlh_i1_cfg_t val);
int32_t lis331dlh_pin_int1_route_get(lis331dlh_ctx_t *ctx,
                                     lis331dlh_i1_cfg_t *val);

typedef enum {
  LIS331DLH_INT1_PULSED   = 0,
  LIS331DLH_INT1_LATCHED  = 1,
} lis331dlh_lir1_t;
int32_t lis331dlh_int1_notification_set(lis331dlh_ctx_t *ctx,
                                        lis331dlh_lir1_t val);
int32_t lis331dlh_int1_notification_get(lis331dlh_ctx_t *ctx,
                                        lis331dlh_lir1_t *val);

typedef enum {
  LIS331DLH_PAD2_INT2_SRC           = 0,
  LIS331DLH_PAD2_INT1_OR_INT2_SRC   = 1,
  LIS331DLH_PAD2_DRDY               = 2,
  LIS331DLH_PAD2_BOOT               = 3,
} lis331dlh_i2_cfg_t;
int32_t lis331dlh_pin_int2_route_set(lis331dlh_ctx_t *ctx,
                                     lis331dlh_i2_cfg_t val);
int32_t lis331dlh_pin_int2_route_get(lis331dlh_ctx_t *ctx,
                                     lis331dlh_i2_cfg_t *val);

typedef enum {
  LIS331DLH_INT2_PULSED   = 0,
  LIS331DLH_INT2_LATCHED  = 1,
} lis331dlh_lir2_t;
int32_t lis331dlh_int2_notification_set(lis331dlh_ctx_t *ctx,
                                        lis331dlh_lir2_t val);
int32_t lis331dlh_int2_notification_get(lis331dlh_ctx_t *ctx,
                                        lis331dlh_lir2_t *val);

typedef enum {
  LIS331DLH_PUSH_PULL   = 0,
  LIS331DLH_OPEN_DRAIN  = 1,
} lis331dlh_pp_od_t;
int32_t lis331dlh_pin_mode_set(lis331dlh_ctx_t *ctx, lis331dlh_pp_od_t val);
int32_t lis331dlh_pin_mode_get(lis331dlh_ctx_t *ctx, lis331dlh_pp_od_t *val);

typedef enum {
  LIS331DLH_ACTIVE_HIGH  = 0,
  LIS331DLH_ACTIVE_LOW   = 1,
} lis331dlh_ihl_t;
int32_t lis331dlh_pin_polarity_set(lis331dlh_ctx_t *ctx,
                                   lis331dlh_ihl_t val);
int32_t lis331dlh_pin_polarity_get(lis331dlh_ctx_t *ctx,
                                   lis331dlh_ihl_t *val);

typedef struct {
  uint8_t int1_xlie             : 1;
  uint8_t int1_xhie             : 1;
  uint8_t int1_ylie             : 1;
  uint8_t int1_yhie             : 1;
  uint8_t int1_zlie             : 1;
  uint8_t int1_zhie             : 1;
} int1_on_th_conf_t;
int32_t lis331dlh_int1_on_threshold_conf_set(lis331dlh_ctx_t *ctx,
                                             int1_on_th_conf_t val);
int32_t lis331dlh_int1_on_threshold_conf_get(lis331dlh_ctx_t *ctx,
                                             int1_on_th_conf_t *val);

typedef enum {
  LIS331DLH_INT1_ON_THRESHOLD_OR   = 0,
  LIS331DLH_INT1_ON_THRESHOLD_AND  = 1,
} lis331dlh_int1_aoi_t;
int32_t lis331dlh_int1_on_threshold_mode_set(lis331dlh_ctx_t *ctx,
                                             lis331dlh_int1_aoi_t val);
int32_t lis331dlh_int1_on_threshold_mode_get(lis331dlh_ctx_t *ctx,
                                             lis331dlh_int1_aoi_t *val);

int32_t lis331dlh_int1_src_get(lis331dlh_ctx_t *ctx,
                               lis331dlh_int1_src_t *val);

int32_t lis331dlh_int1_treshold_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_int1_treshold_get(lis331dlh_ctx_t *ctx, uint8_t *val);

int32_t lis331dlh_int1_dur_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_int1_dur_get(lis331dlh_ctx_t *ctx, uint8_t *val);

typedef struct {
  uint8_t int2_xlie             : 1;
  uint8_t int2_xhie             : 1;
  uint8_t int2_ylie             : 1;
  uint8_t int2_yhie             : 1;
  uint8_t int2_zlie             : 1;
  uint8_t int2_zhie             : 1;
} int2_on_th_conf_t;
int32_t lis331dlh_int2_on_threshold_conf_set(lis331dlh_ctx_t *ctx,
                                             int2_on_th_conf_t val);
int32_t lis331dlh_int2_on_threshold_conf_get(lis331dlh_ctx_t *ctx,
                                             int2_on_th_conf_t *val);

typedef enum {
  LIS331DLH_INT2_ON_THRESHOLD_OR   = 0,
  LIS331DLH_INT2_ON_THRESHOLD_AND  = 1,
} lis331dlh_int2_aoi_t;
int32_t lis331dlh_int2_on_threshold_mode_set(lis331dlh_ctx_t *ctx,
                                             lis331dlh_int2_aoi_t val);
int32_t lis331dlh_int2_on_threshold_mode_get(lis331dlh_ctx_t *ctx,
                                             lis331dlh_int2_aoi_t *val);

int32_t lis331dlh_int2_src_get(lis331dlh_ctx_t *ctx,
                               lis331dlh_int2_src_t *val);

int32_t lis331dlh_int2_treshold_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_int2_treshold_get(lis331dlh_ctx_t *ctx, uint8_t *val);

int32_t lis331dlh_int2_dur_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_int2_dur_get(lis331dlh_ctx_t *ctx, uint8_t *val);

int32_t lis331dlh_wkup_to_sleep_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_wkup_to_sleep_get(lis331dlh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS331DLH_6D_INT1_DISABLE   = 0,
  LIS331DLH_6D_INT1_MOVEMENT  = 1,
  LIS331DLH_6D_INT1_POSITION  = 3,
} lis331dlh_int1_6d_t;
int32_t lis331dlh_int1_6d_mode_set(lis331dlh_ctx_t *ctx,
                                   lis331dlh_int1_6d_t val);
int32_t lis331dlh_int1_6d_mode_get(lis331dlh_ctx_t *ctx,
                                   lis331dlh_int1_6d_t *val);

int32_t lis331dlh_int1_6d_src_get(lis331dlh_ctx_t *ctx,
                                  lis331dlh_int1_src_t *val);

int32_t lis331dlh_int1_6d_treshold_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_int1_6d_treshold_get(lis331dlh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS331DLH_6D_INT2_DISABLE   = 0,
  LIS331DLH_6D_INT2_MOVEMENT  = 1,
  LIS331DLH_6D_INT2_POSITION  = 3,
} lis331dlh_int2_6d_t;
int32_t lis331dlh_int2_6d_mode_set(lis331dlh_ctx_t *ctx,
                                   lis331dlh_int2_6d_t val);
int32_t lis331dlh_int2_6d_mode_get(lis331dlh_ctx_t *ctx,
                                   lis331dlh_int2_6d_t *val);

int32_t lis331dlh_int2_6d_src_get(lis331dlh_ctx_t *ctx,
                                  lis331dlh_int2_src_t *val);

int32_t lis331dlh_int2_6d_treshold_set(lis331dlh_ctx_t *ctx, uint8_t val);
int32_t lis331dlh_int2_6d_treshold_get(lis331dlh_ctx_t *ctx, uint8_t *val);

/**
  *@}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* LIS331DLH_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
