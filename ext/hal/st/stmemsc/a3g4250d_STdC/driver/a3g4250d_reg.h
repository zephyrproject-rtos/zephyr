/*
 ******************************************************************************
 * @file    a3g4250d_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          a3g4250d_reg.c driver.
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
#ifndef A3G4250D_REGS_H
#define A3G4250D_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup A3G4250D
  * @{
  *
  */

/** @defgroup A3G4250D_sensors_common_types
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

/**
  * @defgroup a3g4250d_interface
  * @{
  *
  */

typedef int32_t (*a3g4250d_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*a3g4250d_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  a3g4250d_write_ptr  write_reg;
  a3g4250d_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} a3g4250d_ctx_t;

/**
  * @}
  *
  */


/**
  * @defgroup a3g4250d_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> 0xD1 if SA0=1 -> 0xD3 **/
#define A3G4250D_I2C_ADD_L               0xD1U
#define A3G4250D_I2C_ADD_H               0xD3U

/** Device Identification (Who am I) **/
#define A3G4250D_ID                      0xD3U

/**
  * @}
  *
  */

#define A3G4250D_WHO_AM_I                0x0FU
#define A3G4250D_CTRL_REG1               0x20U
typedef struct {
  uint8_t pd                        : 4; /* xen yen zen pd */
  uint8_t bw                        : 2;
  uint8_t dr                        : 2;
} a3g4250d_ctrl_reg1_t;

#define A3G4250D_CTRL_REG2               0x21U
typedef struct {
  uint8_t hpcf                      : 4;
  uint8_t hpm                       : 2;
  uint8_t not_used_01               : 2;
} a3g4250d_ctrl_reg2_t;

#define A3G4250D_CTRL_REG3               0x22U
typedef struct {
  uint8_t i2_empty                  : 1;
  uint8_t i2_orun                   : 1;
  uint8_t i2_wtm                    : 1;
  uint8_t i2_drdy                   : 1;
  uint8_t pp_od                     : 1;
  uint8_t h_lactive                 : 1;
  uint8_t i1_boot                   : 1;
  uint8_t i1_int1                   : 1;
} a3g4250d_ctrl_reg3_t;

#define A3G4250D_CTRL_REG4               0x23U
typedef struct {
  uint8_t sim                       : 1;
  uint8_t st                        : 2;
  uint8_t not_used_01               : 3;
  uint8_t ble                       : 1;
  uint8_t not_used_02               : 1;
} a3g4250d_ctrl_reg4_t;

#define A3G4250D_CTRL_REG5               0x24U
typedef struct {
  uint8_t out_sel                   : 2;
  uint8_t int1_sel                  : 2;
  uint8_t hpen                      : 1;
  uint8_t not_used_01               : 1;
  uint8_t fifo_en                   : 1;
  uint8_t boot                      : 1;
} a3g4250d_ctrl_reg5_t;

#define A3G4250D_REFERENCE               0x25U
typedef struct {
  uint8_t ref                       : 8;
} a3g4250d_reference_t;

#define A3G4250D_OUT_TEMP                0x26U
#define A3G4250D_STATUS_REG              0x27U
typedef struct {
  uint8_t xda                       : 1;
  uint8_t yda                       : 1;
  uint8_t zda                       : 1;
  uint8_t zyxda                     : 1;
  uint8_t _xor                      : 1;
  uint8_t yor                       : 1;
  uint8_t zor                       : 1;
  uint8_t zyxor                     : 1;
} a3g4250d_status_reg_t;

#define A3G4250D_OUT_X_L                 0x28U
#define A3G4250D_OUT_X_H                 0x29U
#define A3G4250D_OUT_Y_L                 0x2AU
#define A3G4250D_OUT_Y_H                 0x2BU
#define A3G4250D_OUT_Z_L                 0x2CU
#define A3G4250D_OUT_Z_H                 0x2DU
#define A3G4250D_FIFO_CTRL_REG           0x2EU
typedef struct {
  uint8_t wtm                       : 5;
  uint8_t fm                        : 3;
} a3g4250d_fifo_ctrl_reg_t;

#define A3G4250D_FIFO_SRC_REG            0x2FU
typedef struct {
  uint8_t fss                       : 5;
  uint8_t empty                     : 1;
  uint8_t ovrn                      : 1;
  uint8_t wtm                       : 1;
} a3g4250d_fifo_src_reg_t;

#define A3G4250D_INT1_CFG                0x30U
typedef struct {
  uint8_t xlie                      : 1;
  uint8_t xhie                      : 1;
  uint8_t ylie                      : 1;
  uint8_t yhie                      : 1;
  uint8_t zlie                      : 1;
  uint8_t zhie                      : 1;
  uint8_t lir                       : 1;
  uint8_t and_or                    : 1;
} a3g4250d_int1_cfg_t;

#define A3G4250D_INT1_SRC                0x31U
typedef struct {
  uint8_t xl                        : 1;
  uint8_t xh                        : 1;
  uint8_t yl                        : 1;
  uint8_t yh                        : 1;
  uint8_t zl                        : 1;
  uint8_t zh                        : 1;
  uint8_t ia                        : 1;
  uint8_t not_used_01               : 1;
} a3g4250d_int1_src_t;

#define A3G4250D_INT1_TSH_XH             0x32U
typedef struct {
  uint8_t thsx                      : 7;
  uint8_t not_used_01               : 1;
} a3g4250d_int1_tsh_xh_t;

#define A3G4250D_INT1_TSH_XL             0x33U
typedef struct {
  uint8_t thsx                      : 8;
} a3g4250d_int1_tsh_xl_t;

#define A3G4250D_INT1_TSH_YH             0x34U
typedef struct {
  uint8_t thsy                      : 7;
  uint8_t not_used_01               : 1;
} a3g4250d_int1_tsh_yh_t;

#define A3G4250D_INT1_TSH_YL             0x35U
typedef struct {
  uint8_t thsy                      : 8;
} a3g4250d_int1_tsh_yl_t;

#define A3G4250D_INT1_TSH_ZH             0x36U
typedef struct {
  uint8_t thsz                      : 7;
  uint8_t not_used_01               : 1;
} a3g4250d_int1_tsh_zh_t;

#define A3G4250D_INT1_TSH_ZL             0x37U
typedef struct {
  uint8_t thsz                      : 8;
} a3g4250d_int1_tsh_zl_t;

#define A3G4250D_INT1_DURATION           0x38U
typedef struct {
  uint8_t d                         : 7;
  uint8_t wait                      : 1;
} a3g4250d_int1_duration_t;

/**
  * @defgroup LSM9DS1_Register_Union
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
  a3g4250d_ctrl_reg1_t        ctrl_reg1;
  a3g4250d_ctrl_reg2_t        ctrl_reg2;
  a3g4250d_ctrl_reg3_t        ctrl_reg3;
  a3g4250d_ctrl_reg4_t        ctrl_reg4;
  a3g4250d_ctrl_reg5_t        ctrl_reg5;
  a3g4250d_reference_t        reference;
  a3g4250d_status_reg_t       status_reg;
  a3g4250d_fifo_ctrl_reg_t    fifo_ctrl_reg;
  a3g4250d_fifo_src_reg_t     fifo_src_reg;
  a3g4250d_int1_cfg_t         int1_cfg;
  a3g4250d_int1_src_t         int1_src;
  a3g4250d_int1_tsh_xh_t      int1_tsh_xh;
  a3g4250d_int1_tsh_xl_t      int1_tsh_xl;
  a3g4250d_int1_tsh_yh_t      int1_tsh_yh;
  a3g4250d_int1_tsh_yl_t      int1_tsh_yl;
  a3g4250d_int1_tsh_zh_t      int1_tsh_zh;
  a3g4250d_int1_tsh_zl_t      int1_tsh_zl;
  a3g4250d_int1_duration_t    int1_duration;
  bitwise_t                   bitwise;
  uint8_t                     byte;
} a3g4250d_reg_t;

/**
  * @}
  *
  */

int32_t a3g4250d_read_reg(a3g4250d_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);
int32_t a3g4250d_write_reg(a3g4250d_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);

extern float_t a3g4250d_from_fs245dps_to_mdps(int16_t lsb);
extern float_t a3g4250d_from_lsb_to_celsius(int16_t lsb);

int32_t a3g4250d_axis_x_data_set(a3g4250d_ctx_t *ctx, uint8_t val);
int32_t a3g4250d_axis_x_data_get(a3g4250d_ctx_t *ctx, uint8_t *val);

int32_t a3g4250d_axis_y_data_set(a3g4250d_ctx_t *ctx, uint8_t val);
int32_t a3g4250d_axis_y_data_get(a3g4250d_ctx_t *ctx, uint8_t *val);

int32_t a3g4250d_axis_z_data_set(a3g4250d_ctx_t *ctx, uint8_t val);
int32_t a3g4250d_axis_z_data_get(a3g4250d_ctx_t *ctx, uint8_t *val);

typedef enum {
  A3G4250D_ODR_OFF     = 0x00,
  A3G4250D_ODR_SLEEP   = 0x08,
  A3G4250D_ODR_100Hz   = 0x0F,
  A3G4250D_ODR_200Hz   = 0x1F,
  A3G4250D_ODR_400Hz   = 0x2F,
  A3G4250D_ODR_800Hz   = 0x3F,
} a3g4250d_dr_t;
int32_t a3g4250d_data_rate_set(a3g4250d_ctx_t *ctx, a3g4250d_dr_t val);
int32_t a3g4250d_data_rate_get(a3g4250d_ctx_t *ctx, a3g4250d_dr_t *val);

int32_t a3g4250d_status_reg_get(a3g4250d_ctx_t *ctx,
                                a3g4250d_status_reg_t *val);

int32_t a3g4250d_flag_data_ready_get(a3g4250d_ctx_t *ctx, uint8_t *val);

int32_t a3g4250d_temperature_raw_get(a3g4250d_ctx_t *ctx, uint8_t *buff);

int32_t a3g4250d_angular_rate_raw_get(a3g4250d_ctx_t *ctx, uint8_t *buff);

int32_t a3g4250d_device_id_get(a3g4250d_ctx_t *ctx, uint8_t *buff);

typedef enum {
  A3G4250D_GY_ST_DISABLE    = 0,
  A3G4250D_GY_ST_POSITIVE   = 1,
  A3G4250D_GY_ST_NEGATIVE   = 3,
} a3g4250d_st_t;
int32_t a3g4250d_self_test_set(a3g4250d_ctx_t *ctx, a3g4250d_st_t val);
int32_t a3g4250d_self_test_get(a3g4250d_ctx_t *ctx, a3g4250d_st_t *val);

typedef enum {
  A3G4250D_AUX_LSB_AT_LOW_ADD   = 0,
  A3G4250D_AUX_MSB_AT_LOW_ADD   = 1,
} a3g4250d_ble_t;
int32_t a3g4250d_data_format_set(a3g4250d_ctx_t *ctx, a3g4250d_ble_t val);
int32_t a3g4250d_data_format_get(a3g4250d_ctx_t *ctx, a3g4250d_ble_t *val);

int32_t a3g4250d_boot_set(a3g4250d_ctx_t *ctx, uint8_t val);
int32_t a3g4250d_boot_get(a3g4250d_ctx_t *ctx, uint8_t *val);

typedef enum {
  A3G4250D_CUT_OFF_LOW        = 0,
  A3G4250D_CUT_OFF_MEDIUM     = 1,
  A3G4250D_CUT_OFF_HIGH       = 2,
  A3G4250D_CUT_OFF_VERY_HIGH  = 3,
} a3g4250d_bw_t;
int32_t a3g4250d_lp_bandwidth_set(a3g4250d_ctx_t *ctx, a3g4250d_bw_t val);
int32_t a3g4250d_lp_bandwidth_get(a3g4250d_ctx_t *ctx, a3g4250d_bw_t *val);

typedef enum {
  A3G4250D_HP_LEVEL_0   = 0,
  A3G4250D_HP_LEVEL_1   = 1,
  A3G4250D_HP_LEVEL_2   = 2,
  A3G4250D_HP_LEVEL_3   = 3,
  A3G4250D_HP_LEVEL_4   = 4,
  A3G4250D_HP_LEVEL_5   = 5,
  A3G4250D_HP_LEVEL_6   = 6,
  A3G4250D_HP_LEVEL_7   = 7,
  A3G4250D_HP_LEVEL_8   = 8,
  A3G4250D_HP_LEVEL_9   = 9,
} a3g4250d_hpcf_t;
int32_t a3g4250d_hp_bandwidth_set(a3g4250d_ctx_t *ctx,
                                  a3g4250d_hpcf_t val);
int32_t a3g4250d_hp_bandwidth_get(a3g4250d_ctx_t *ctx,
                                  a3g4250d_hpcf_t *val);

typedef enum {
  A3G4250D_HP_NORMAL_MODE_WITH_RST  = 0,
  A3G4250D_HP_REFERENCE_SIGNAL      = 1,
  A3G4250D_HP_NORMAL_MODE           = 2,
  A3G4250D_HP_AUTO_RESET_ON_INT     = 3,
} a3g4250d_hpm_t;
int32_t a3g4250d_hp_mode_set(a3g4250d_ctx_t *ctx, a3g4250d_hpm_t val);
int32_t a3g4250d_hp_mode_get(a3g4250d_ctx_t *ctx, a3g4250d_hpm_t *val);

typedef enum {
  A3G4250D_ONLY_LPF1_ON_OUT     = 0,
  A3G4250D_LPF1_HP_ON_OUT       = 1,
  A3G4250D_LPF1_LPF2_ON_OUT     = 2,
  A3G4250D_LPF1_HP_LPF2_ON_OUT  = 6,
} a3g4250d_out_sel_t;
int32_t a3g4250d_filter_path_set(a3g4250d_ctx_t *ctx,
                                 a3g4250d_out_sel_t val);
int32_t a3g4250d_filter_path_get(a3g4250d_ctx_t *ctx,
                                 a3g4250d_out_sel_t *val);

typedef enum {
  A3G4250D_ONLY_LPF1_ON_INT     = 0,
  A3G4250D_LPF1_HP_ON_INT       = 1,
  A3G4250D_LPF1_LPF2_ON_INT     = 2,
  A3G4250D_LPF1_HP_LPF2_ON_INT  = 6,
} a3g4250d_int1_sel_t;
int32_t a3g4250d_filter_path_internal_set(a3g4250d_ctx_t *ctx,
                                          a3g4250d_int1_sel_t val);
int32_t a3g4250d_filter_path_internal_get(a3g4250d_ctx_t *ctx,
                                          a3g4250d_int1_sel_t *val);

int32_t a3g4250d_hp_reference_value_set(a3g4250d_ctx_t *ctx, uint8_t val);
int32_t a3g4250d_hp_reference_value_get(a3g4250d_ctx_t *ctx, uint8_t *val);

typedef enum {
  A3G4250D_SPI_4_WIRE  = 0,
  A3G4250D_SPI_3_WIRE  = 1,
} a3g4250d_sim_t;
int32_t a3g4250d_spi_mode_set(a3g4250d_ctx_t *ctx, a3g4250d_sim_t val);
int32_t a3g4250d_spi_mode_get(a3g4250d_ctx_t *ctx, a3g4250d_sim_t *val);

typedef struct {
  uint8_t i1_int1             : 1;
  uint8_t i1_boot             : 1;
} a3g4250d_int1_route_t;
int32_t a3g4250d_pin_int1_route_set(a3g4250d_ctx_t *ctx,
                                    a3g4250d_int1_route_t val);
int32_t a3g4250d_pin_int1_route_get(a3g4250d_ctx_t *ctx,
                                    a3g4250d_int1_route_t *val);

typedef struct {
  uint8_t i2_empty             : 1;
  uint8_t i2_orun              : 1;
  uint8_t i2_wtm               : 1;
  uint8_t i2_drdy              : 1;
} a3g4250d_int2_route_t;
int32_t a3g4250d_pin_int2_route_set(a3g4250d_ctx_t *ctx,
                                    a3g4250d_int2_route_t val);
int32_t a3g4250d_pin_int2_route_get(a3g4250d_ctx_t *ctx,
                                    a3g4250d_int2_route_t *val);

typedef enum {
  A3G4250D_PUSH_PULL   = 0,
  A3G4250D_OPEN_DRAIN  = 1,
} a3g4250d_pp_od_t;
int32_t a3g4250d_pin_mode_set(a3g4250d_ctx_t *ctx, a3g4250d_pp_od_t val);
int32_t a3g4250d_pin_mode_get(a3g4250d_ctx_t *ctx, a3g4250d_pp_od_t *val);

typedef enum {
  A3G4250D_ACTIVE_HIGH  = 0,
  A3G4250D_ACTIVE_LOW   = 1,
} a3g4250d_h_lactive_t;
int32_t a3g4250d_pin_polarity_set(a3g4250d_ctx_t *ctx,
                                  a3g4250d_h_lactive_t val);
int32_t a3g4250d_pin_polarity_get(a3g4250d_ctx_t *ctx,
                                  a3g4250d_h_lactive_t *val);

typedef enum {
  A3G4250D_INT_PULSED   = 0,
  A3G4250D_INT_LATCHED  = 1,
} a3g4250d_lir_t;
int32_t a3g4250d_int_notification_set(a3g4250d_ctx_t *ctx,
                                      a3g4250d_lir_t val);
int32_t a3g4250d_int_notification_get(a3g4250d_ctx_t *ctx,
                                      a3g4250d_lir_t *val);

int32_t a3g4250d_int_on_threshold_conf_set(a3g4250d_ctx_t *ctx,
                                           a3g4250d_int1_cfg_t *val);
int32_t a3g4250d_int_on_threshold_conf_get(a3g4250d_ctx_t *ctx,
                                           a3g4250d_int1_cfg_t *val);

typedef enum {
  A3G4250D_INT1_ON_TH_AND  = 1,
  A3G4250D_INT1_ON_TH_OR   = 0,
} a3g4250d_and_or_t;
int32_t a3g4250d_int_on_threshold_mode_set(a3g4250d_ctx_t *ctx,
                                           a3g4250d_and_or_t val);
int32_t a3g4250d_int_on_threshold_mode_get(a3g4250d_ctx_t *ctx,
                                           a3g4250d_and_or_t *val);

int32_t a3g4250d_int_on_threshold_src_get(a3g4250d_ctx_t *ctx,
                                          a3g4250d_int1_src_t *val);

int32_t a3g4250d_int_x_treshold_set(a3g4250d_ctx_t *ctx, uint16_t val);
int32_t a3g4250d_int_x_treshold_get(a3g4250d_ctx_t *ctx, uint16_t *val);

int32_t a3g4250d_int_y_treshold_set(a3g4250d_ctx_t *ctx, uint16_t val);
int32_t a3g4250d_int_y_treshold_get(a3g4250d_ctx_t *ctx, uint16_t *val);

int32_t a3g4250d_int_z_treshold_set(a3g4250d_ctx_t *ctx, uint16_t val);
int32_t a3g4250d_int_z_treshold_get(a3g4250d_ctx_t *ctx, uint16_t *val);

int32_t a3g4250d_int_on_threshold_dur_set(a3g4250d_ctx_t *ctx, uint8_t val);
int32_t a3g4250d_int_on_threshold_dur_get(a3g4250d_ctx_t *ctx, uint8_t *val);

int32_t a3g4250d_fifo_enable_set(a3g4250d_ctx_t *ctx, uint8_t val);
int32_t a3g4250d_fifo_enable_get(a3g4250d_ctx_t *ctx, uint8_t *val);

int32_t a3g4250d_fifo_watermark_set(a3g4250d_ctx_t *ctx, uint8_t val);
int32_t a3g4250d_fifo_watermark_get(a3g4250d_ctx_t *ctx, uint8_t *val);

typedef enum {
  A3G4250D_FIFO_BYPASS_MODE     = 0x00,
  A3G4250D_FIFO_MODE            = 0x01,
  A3G4250D_FIFO_STREAM_MODE     = 0x02,
} a3g4250d_fifo_mode_t;
int32_t a3g4250d_fifo_mode_set(a3g4250d_ctx_t *ctx, a3g4250d_fifo_mode_t val);
int32_t a3g4250d_fifo_mode_get(a3g4250d_ctx_t *ctx, a3g4250d_fifo_mode_t *val);

int32_t a3g4250d_fifo_data_level_get(a3g4250d_ctx_t *ctx, uint8_t *val);

int32_t a3g4250d_fifo_empty_flag_get(a3g4250d_ctx_t *ctx, uint8_t *val);

int32_t a3g4250d_fifo_ovr_flag_get(a3g4250d_ctx_t *ctx, uint8_t *val);

int32_t a3g4250d_fifo_wtm_flag_get(a3g4250d_ctx_t *ctx, uint8_t *val);

/**
  * @}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* A3G4250D_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
