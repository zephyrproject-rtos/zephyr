/*
 ******************************************************************************
 * @file    iis328dq_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          iis328dq_reg.c driver.
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
#ifndef IIS328DQ_REGS_H
#define IIS328DQ_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup IIS328DQ
  * @{
  *
  */

/** @defgroup IIS328DQ_sensors_common_types
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

/** @addtogroup  IIS328DQ_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*iis328dq_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*iis328dq_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  iis328dq_write_ptr  write_reg;
  iis328dq_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} iis328dq_ctx_t;

/**
  * @}
  *
  */


/** @defgroup IIS328DQ_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> 0x31 if SA0=1 -> 0x33 **/
#define IIS328DQ_I2C_ADD_L     0x31
#define IIS328DQ_I2C_ADD_H     0x33

/** Device Identification (Who am I) **/
#define IIS328DQ_ID            0x32

/**
  * @}
  *
  */

/**
  * @addtogroup  IIS328DQ_Sensitivity
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

#define IIS328DQ_FROM_FS_2g_TO_mg(lsb)    (float)( (lsb >> 4 ) * 0.98f )
#define IIS328DQ_FROM_FS_4g_TO_mg(lsb)    (float)( (lsb >> 4 ) * 1.95f )
#define IIS328DQ_FROM_FS_8g_TO_mg(lsb)    (float)( (lsb >> 4 ) * 3.91f )

/**
  * @}
  *
  */

#define IIS328DQ_WHO_AM_I                  0x0FU
#define IIS328DQ_CTRL_REG1                 0x20U
typedef struct {
  uint8_t xen                      : 1;
  uint8_t yen                      : 1;
  uint8_t zen                      : 1;
  uint8_t dr                       : 2;
  uint8_t pm                       : 3;
} iis328dq_ctrl_reg1_t;

#define IIS328DQ_CTRL_REG2                 0x21U
typedef struct {
  uint8_t hpcf                     : 2;
  uint8_t hpen                     : 2;
  uint8_t fds                      : 1;
  uint8_t hpm                      : 2;
  uint8_t boot                     : 1;
} iis328dq_ctrl_reg2_t;

#define IIS328DQ_CTRL_REG3                 0x22U
typedef struct {
  uint8_t i1_cfg                   : 2;
  uint8_t lir1                     : 1;
  uint8_t i2_cfg                   : 2;
  uint8_t lir2                     : 1;
  uint8_t pp_od                    : 1;
  uint8_t ihl                      : 1;
} iis328dq_ctrl_reg3_t;

#define IIS328DQ_CTRL_REG4                 0x23U
typedef struct {
  uint8_t sim                      : 1;
  uint8_t st                       : 3; /* STsign + ST */
  uint8_t fs                       : 2;
  uint8_t ble                      : 1;
  uint8_t bdu                      : 1;
} iis328dq_ctrl_reg4_t;

#define IIS328DQ_CTRL_REG5                 0x24U
typedef struct {
  uint8_t turnon                   : 2;
  uint8_t not_used_01              : 6;
} iis328dq_ctrl_reg5_t;

#define IIS328DQ_HP_FILTER_RESET           0x25U
#define IIS328DQ_REFERENCE                 0x26U
#define IIS328DQ_STATUS_REG                0x27U
typedef struct {
  uint8_t xda                      : 1;
  uint8_t yda                      : 1;
  uint8_t zda                      : 1;
  uint8_t zyxda                    : 1;
  uint8_t _xor                     : 1;
  uint8_t yor                      : 1;
  uint8_t zor                      : 1;
  uint8_t zyxor                    : 1;
} iis328dq_status_reg_t;

#define IIS328DQ_OUT_X_L                   0x28U
#define IIS328DQ_OUT_X_H                   0x29U
#define IIS328DQ_OUT_Y_L                   0x2AU
#define IIS328DQ_OUT_Y_H                   0x2BU
#define IIS328DQ_OUT_Z_L                   0x2CU
#define IIS328DQ_OUT_Z_H                   0x2DU
#define IIS328DQ_INT1_CFG                  0x30U
typedef struct {
  uint8_t xlie                     : 1;
  uint8_t xhie                     : 1;
  uint8_t ylie                     : 1;
  uint8_t yhie                     : 1;
  uint8_t zlie                     : 1;
  uint8_t zhie                     : 1;
  uint8_t _6d                      : 1;
  uint8_t aoi                      : 1;
} iis328dq_int1_cfg_t;

#define IIS328DQ_INT1_SRC                  0x31U
typedef struct {
  uint8_t xl                       : 1;
  uint8_t xh                       : 1;
  uint8_t yl                       : 1;
  uint8_t yh                       : 1;
  uint8_t zl                       : 1;
  uint8_t zh                       : 1;
  uint8_t ia                       : 1;
  uint8_t not_used_01              : 1;
} iis328dq_int1_src_t;

#define IIS328DQ_INT1_THS                  0x32U
typedef struct {
  uint8_t ths                      : 7;
  uint8_t not_used_01              : 1;
} iis328dq_int1_ths_t;

#define IIS328DQ_INT1_DURATION             0x33U
typedef struct {
  uint8_t d                        : 7;
  uint8_t not_used_01              : 1;
} iis328dq_int1_duration_t;

#define IIS328DQ_INT2_CFG                  0x34U
typedef struct {
  uint8_t xlie                     : 1;
  uint8_t xhie                     : 1;
  uint8_t ylie                     : 1;
  uint8_t yhie                     : 1;
  uint8_t zlie                     : 1;
  uint8_t zhie                     : 1;
  uint8_t _6d                      : 1;
  uint8_t aoi                      : 1;
} iis328dq_int2_cfg_t;

#define IIS328DQ_INT2_SRC                  0x35U
typedef struct {
  uint8_t xl                       : 1;
  uint8_t xh                       : 1;
  uint8_t yl                       : 1;
  uint8_t yh                       : 1;
  uint8_t zl                       : 1;
  uint8_t zh                       : 1;
  uint8_t ia                       : 1;
  uint8_t not_used_01              : 1;
} iis328dq_int2_src_t;

#define IIS328DQ_INT2_THS                  0x36U
typedef struct {
  uint8_t ths                      : 7;
  uint8_t not_used_01              : 1;
} iis328dq_int2_ths_t;

#define IIS328DQ_INT2_DURATION             0x37U
typedef struct {
  uint8_t d                        : 7;
  uint8_t not_used_01              : 1;
} iis328dq_int2_duration_t;

/**
  * @defgroup IIS328DQ_Register_Union
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
  iis328dq_ctrl_reg1_t                     ctrl_reg1;
  iis328dq_ctrl_reg2_t                     ctrl_reg2;
  iis328dq_ctrl_reg3_t                     ctrl_reg3;
  iis328dq_ctrl_reg4_t                     ctrl_reg4;
  iis328dq_ctrl_reg5_t                     ctrl_reg5;
  iis328dq_status_reg_t                    status_reg;
  iis328dq_int1_cfg_t                      int1_cfg;
  iis328dq_int1_src_t                      int1_src;
  iis328dq_int1_ths_t                      int1_ths;
  iis328dq_int1_duration_t                 int1_duration;
  iis328dq_int2_cfg_t                      int2_cfg;
  iis328dq_int2_src_t                      int2_src;
  iis328dq_int2_ths_t                      int2_ths;
  iis328dq_int2_duration_t                 int2_duration;
  bitwise_t                                 bitwise;
  uint8_t                                   byte;
} iis328dq_reg_t;

/**
  * @}
  *
  */

int32_t iis328dq_read_reg(iis328dq_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);
int32_t iis328dq_write_reg(iis328dq_ctx_t *ctx, uint8_t reg, uint8_t* data,
                            uint16_t len);

extern float iis328dq_from_fs2_to_mg(int16_t lsb);
extern float iis328dq_from_fs4_to_mg(int16_t lsb);
extern float iis328dq_from_fs8_to_mg(int16_t lsb);

int32_t iis328dq_axis_x_data_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_axis_x_data_get(iis328dq_ctx_t *ctx, uint8_t *val);

int32_t iis328dq_axis_y_data_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_axis_y_data_get(iis328dq_ctx_t *ctx, uint8_t *val);

int32_t iis328dq_axis_z_data_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_axis_z_data_get(iis328dq_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS328DQ_ODR_OFF   = 0x00,
  IIS328DQ_ODR_Hz5   = 0x02,
  IIS328DQ_ODR_1Hz   = 0x03,
  IIS328DQ_ODR_5Hz2  = 0x04,
  IIS328DQ_ODR_5Hz   = 0x05,
  IIS328DQ_ODR_10Hz  = 0x06,
  IIS328DQ_ODR_50Hz  = 0x01,
  IIS328DQ_ODR_100Hz = 0x11,
  IIS328DQ_ODR_400Hz = 0x21,
  IIS328DQ_ODR_1kHz  = 0x31,
} iis328dq_dr_t;
int32_t iis328dq_data_rate_set(iis328dq_ctx_t *ctx, iis328dq_dr_t val);
int32_t iis328dq_data_rate_get(iis328dq_ctx_t *ctx, iis328dq_dr_t *val);

typedef enum {
  IIS328DQ_NORMAL_MODE      = 0,
  IIS328DQ_REF_MODE_ENABLE  = 1,
} iis328dq_hpm_t;
int32_t iis328dq_reference_mode_set(iis328dq_ctx_t *ctx,
                                     iis328dq_hpm_t val);
int32_t iis328dq_reference_mode_get(iis328dq_ctx_t *ctx,
                                     iis328dq_hpm_t *val);

typedef enum {
  IIS328DQ_2g  = 0,
  IIS328DQ_4g  = 1,
  IIS328DQ_8g  = 3,
} iis328dq_fs_t;
int32_t iis328dq_full_scale_set(iis328dq_ctx_t *ctx, iis328dq_fs_t val);
int32_t iis328dq_full_scale_get(iis328dq_ctx_t *ctx, iis328dq_fs_t *val);

int32_t iis328dq_block_data_update_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_block_data_update_get(iis328dq_ctx_t *ctx, uint8_t *val);

int32_t iis328dq_status_reg_get(iis328dq_ctx_t *ctx,
                                 iis328dq_status_reg_t *val);

int32_t iis328dq_flag_data_ready_get(iis328dq_ctx_t *ctx,
                                      uint8_t *val);

int32_t iis328dq_acceleration_raw_get(iis328dq_ctx_t *ctx, uint8_t *buff);

int32_t iis328dq_device_id_get(iis328dq_ctx_t *ctx, uint8_t *buff);

int32_t iis328dq_boot_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_boot_get(iis328dq_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS328DQ_ST_DISABLE   = 0,
  IIS328DQ_ST_POSITIVE  = 1,
  IIS328DQ_ST_NEGATIVE  = 5,
} iis328dq_st_t;
int32_t iis328dq_self_test_set(iis328dq_ctx_t *ctx, iis328dq_st_t val);
int32_t iis328dq_self_test_get(iis328dq_ctx_t *ctx, iis328dq_st_t *val);

typedef enum {
  IIS328DQ_LSB_AT_LOW_ADD  = 0,
  IIS328DQ_MSB_AT_LOW_ADD  = 1,
} iis328dq_ble_t;
int32_t iis328dq_data_format_set(iis328dq_ctx_t *ctx, iis328dq_ble_t val);
int32_t iis328dq_data_format_get(iis328dq_ctx_t *ctx, iis328dq_ble_t *val);

typedef enum {
  IIS328DQ_CUT_OFF_8Hz   = 0,
  IIS328DQ_CUT_OFF_16Hz  = 1,
  IIS328DQ_CUT_OFF_32Hz  = 2,
  IIS328DQ_CUT_OFF_64Hz  = 3,
} iis328dq_hpcf_t;
int32_t iis328dq_hp_bandwidth_set(iis328dq_ctx_t *ctx,
                                   iis328dq_hpcf_t val);
int32_t iis328dq_hp_bandwidth_get(iis328dq_ctx_t *ctx,
                                   iis328dq_hpcf_t *val);

typedef enum {
  IIS328DQ_HP_DISABLE            = 0,
  IIS328DQ_HP_ON_OUT             = 4,
  IIS328DQ_HP_ON_INT1            = 1,
  IIS328DQ_HP_ON_INT2            = 2,
  IIS328DQ_HP_ON_INT1_INT2       = 3,
  IIS328DQ_HP_ON_INT1_INT2_OUT   = 7,
  IIS328DQ_HP_ON_INT2_OUT        = 6,
  IIS328DQ_HP_ON_INT1_OUT        = 5,
} iis328dq_hpen_t;
int32_t iis328dq_hp_path_set(iis328dq_ctx_t *ctx, iis328dq_hpen_t val);
int32_t iis328dq_hp_path_get(iis328dq_ctx_t *ctx, iis328dq_hpen_t *val);

int32_t iis328dq_hp_reset_get(iis328dq_ctx_t *ctx);

int32_t iis328dq_hp_reference_value_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_hp_reference_value_get(iis328dq_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS328DQ_SPI_4_WIRE  = 0,
  IIS328DQ_SPI_3_WIRE  = 1,
} iis328dq_sim_t;
int32_t iis328dq_spi_mode_set(iis328dq_ctx_t *ctx, iis328dq_sim_t val);
int32_t iis328dq_spi_mode_get(iis328dq_ctx_t *ctx, iis328dq_sim_t *val);

typedef enum {
  IIS328DQ_PAD1_INT1_SRC           = 0,
  IIS328DQ_PAD1_INT1_OR_INT2_SRC   = 1,
  IIS328DQ_PAD1_DRDY               = 2,
  IIS328DQ_PAD1_BOOT               = 3,
} iis328dq_i1_cfg_t;
int32_t iis328dq_pin_int1_route_set(iis328dq_ctx_t *ctx,
                                     iis328dq_i1_cfg_t val);
int32_t iis328dq_pin_int1_route_get(iis328dq_ctx_t *ctx,
                                     iis328dq_i1_cfg_t *val);

typedef enum {
  IIS328DQ_INT1_PULSED   = 0,
  IIS328DQ_INT1_LATCHED  = 1,
} iis328dq_lir1_t;
int32_t iis328dq_int1_notification_set(iis328dq_ctx_t *ctx,
                                        iis328dq_lir1_t val);
int32_t iis328dq_int1_notification_get(iis328dq_ctx_t *ctx,
                                        iis328dq_lir1_t *val);

typedef enum {
  IIS328DQ_PAD2_INT2_SRC           = 0,
  IIS328DQ_PAD2_INT1_OR_INT2_SRC   = 1,
  IIS328DQ_PAD2_DRDY               = 2,
  IIS328DQ_PAD2_BOOT               = 3,
} iis328dq_i2_cfg_t;
int32_t iis328dq_pin_int2_route_set(iis328dq_ctx_t *ctx,
                                     iis328dq_i2_cfg_t val);
int32_t iis328dq_pin_int2_route_get(iis328dq_ctx_t *ctx,
                                     iis328dq_i2_cfg_t *val);

typedef enum {
  IIS328DQ_INT2_PULSED   = 0,
  IIS328DQ_INT2_LATCHED  = 1,
} iis328dq_lir2_t;
int32_t iis328dq_int2_notification_set(iis328dq_ctx_t *ctx,
                                        iis328dq_lir2_t val);
int32_t iis328dq_int2_notification_get(iis328dq_ctx_t *ctx,
                                        iis328dq_lir2_t *val);

typedef enum {
  IIS328DQ_PUSH_PULL   = 0,
  IIS328DQ_OPEN_DRAIN  = 1,
} iis328dq_pp_od_t;
int32_t iis328dq_pin_mode_set(iis328dq_ctx_t *ctx, iis328dq_pp_od_t val);
int32_t iis328dq_pin_mode_get(iis328dq_ctx_t *ctx, iis328dq_pp_od_t *val);

typedef enum {
  IIS328DQ_ACTIVE_HIGH  = 0,
  IIS328DQ_ACTIVE_LOW   = 1,
} iis328dq_ihl_t;
int32_t iis328dq_pin_polarity_set(iis328dq_ctx_t *ctx,
                                   iis328dq_ihl_t val);
int32_t iis328dq_pin_polarity_get(iis328dq_ctx_t *ctx,
                                   iis328dq_ihl_t *val);

typedef struct {
  uint8_t int1_xlie             : 1;
  uint8_t int1_xhie             : 1;
  uint8_t int1_ylie             : 1;
  uint8_t int1_yhie             : 1;
  uint8_t int1_zlie             : 1;
  uint8_t int1_zhie             : 1;
} int1_on_th_conf_t;
int32_t iis328dq_int1_on_threshold_conf_set(iis328dq_ctx_t *ctx,
                                             int1_on_th_conf_t val);
int32_t iis328dq_int1_on_threshold_conf_get(iis328dq_ctx_t *ctx,
                                             int1_on_th_conf_t *val);

typedef enum {
  IIS328DQ_INT1_ON_THRESHOLD_OR   = 0,
  IIS328DQ_INT1_ON_THRESHOLD_AND  = 1,
} iis328dq_int1_aoi_t;
int32_t iis328dq_int1_on_threshold_mode_set(iis328dq_ctx_t *ctx,
                                             iis328dq_int1_aoi_t val);
int32_t iis328dq_int1_on_threshold_mode_get(iis328dq_ctx_t *ctx,
                                             iis328dq_int1_aoi_t *val);

int32_t iis328dq_int1_src_get(iis328dq_ctx_t *ctx,
                               iis328dq_int1_src_t *val);

int32_t iis328dq_int1_treshold_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_int1_treshold_get(iis328dq_ctx_t *ctx, uint8_t *val);

int32_t iis328dq_int1_dur_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_int1_dur_get(iis328dq_ctx_t *ctx, uint8_t *val);

typedef struct {
  uint8_t int2_xlie             : 1;
  uint8_t int2_xhie             : 1;
  uint8_t int2_ylie             : 1;
  uint8_t int2_yhie             : 1;
  uint8_t int2_zlie             : 1;
  uint8_t int2_zhie             : 1;
} int2_on_th_conf_t;
int32_t iis328dq_int2_on_threshold_conf_set(iis328dq_ctx_t *ctx,
                                             int2_on_th_conf_t val);
int32_t iis328dq_int2_on_threshold_conf_get(iis328dq_ctx_t *ctx,
                                             int2_on_th_conf_t *val);

typedef enum {
  IIS328DQ_INT2_ON_THRESHOLD_OR   = 0,
  IIS328DQ_INT2_ON_THRESHOLD_AND  = 1,
} iis328dq_int2_aoi_t;
int32_t iis328dq_int2_on_threshold_mode_set(iis328dq_ctx_t *ctx,
                                             iis328dq_int2_aoi_t val);
int32_t iis328dq_int2_on_threshold_mode_get(iis328dq_ctx_t *ctx,
                                             iis328dq_int2_aoi_t *val);

int32_t iis328dq_int2_src_get(iis328dq_ctx_t *ctx,
                               iis328dq_int2_src_t *val);

int32_t iis328dq_int2_treshold_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_int2_treshold_get(iis328dq_ctx_t *ctx, uint8_t *val);

int32_t iis328dq_int2_dur_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_int2_dur_get(iis328dq_ctx_t *ctx, uint8_t *val);

int32_t iis328dq_wkup_to_sleep_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_wkup_to_sleep_get(iis328dq_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS328DQ_6D_INT1_DISABLE   = 0,
  IIS328DQ_6D_INT1_MOVEMENT  = 1,
  IIS328DQ_6D_INT1_POSITION  = 3,
} iis328dq_int1_6d_t;
int32_t iis328dq_int1_6d_mode_set(iis328dq_ctx_t *ctx,
                                   iis328dq_int1_6d_t val);
int32_t iis328dq_int1_6d_mode_get(iis328dq_ctx_t *ctx,
                                   iis328dq_int1_6d_t *val);

int32_t iis328dq_int1_6d_src_get(iis328dq_ctx_t *ctx,
                                  iis328dq_int1_src_t *val);

int32_t iis328dq_int1_6d_treshold_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_int1_6d_treshold_get(iis328dq_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS328DQ_6D_INT2_DISABLE   = 0,
  IIS328DQ_6D_INT2_MOVEMENT  = 1,
  IIS328DQ_6D_INT2_POSITION  = 3,
} iis328dq_int2_6d_t;
int32_t iis328dq_int2_6d_mode_set(iis328dq_ctx_t *ctx,
                                   iis328dq_int2_6d_t val);
int32_t iis328dq_int2_6d_mode_get(iis328dq_ctx_t *ctx,
                                   iis328dq_int2_6d_t *val);

int32_t iis328dq_int2_6d_src_get(iis328dq_ctx_t *ctx,
                                  iis328dq_int2_src_t *val);

int32_t iis328dq_int2_6d_treshold_set(iis328dq_ctx_t *ctx, uint8_t val);
int32_t iis328dq_int2_6d_treshold_get(iis328dq_ctx_t *ctx, uint8_t *val);

/**
  *@}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* IIS328DQ_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
