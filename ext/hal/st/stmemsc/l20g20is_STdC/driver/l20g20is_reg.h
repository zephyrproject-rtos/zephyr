/*
 ******************************************************************************
 * @file    l20g20is_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          l20g20is_reg.c driver.
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
#ifndef L20G20IS_REGS_H
#define L20G20IS_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup L20G20IS
  * @{
  *
  */

/** @defgroup L20G20IS_sensors_common_types
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

/** @addtogroup  L20G20IS_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*l20g20is_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*l20g20is_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  l20g20is_write_ptr  write_reg;
  l20g20is_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} l20g20is_ctx_t;

/**
  * @}
  *
  */

/** @defgroup L20G20IS_Infos
  * @{
  *
  */

/** Device Identification (Who am I) **/
#define L20G20IS_ID                     0xDAU

/**
  * @}
  *
  */

#define L20G20IS_WHO_AM_I               0x00U
#define L20G20IS_TEMP_OUT_L             0x01U
#define L20G20IS_TEMP_OUT_H             0x02U
#define L20G20IS_OUT_X_L                0x03U
#define L20G20IS_OUT_X_H                0x04U
#define L20G20IS_OUT_Y_L                0x05U
#define L20G20IS_OUT_Y_H                0x06U
#define L20G20IS_DATA_STATUS_OIS        0x0AU
typedef struct {
  uint8_t not_used_01         : 1;
  uint8_t yda_ois             : 1;
  uint8_t xda_ois             : 1;
  uint8_t xyda_ois            : 1;
  uint8_t not_used_02         : 1;
  uint8_t yor_ois             : 1;
  uint8_t xor_ois             : 1;
  uint8_t xyor_ois            : 1;
} l20g20is_data_status_ois_t;

#define L20G20IS_CTRL1_OIS              0x0BU
typedef struct {
  uint8_t pw                  : 2;
  uint8_t orient              : 1;
  uint8_t odu                 : 1;
  uint8_t sim                 : 1;
  uint8_t ble                 : 1;
  uint8_t dr_pulsed           : 1;
  uint8_t boot                : 1;
} l20g20is_ctrl1_ois_t;

#define L20G20IS_CTRL2_OIS              0x0CU
typedef struct {
  uint8_t hpf                 : 1;
  uint8_t sw_rst              : 1;
  uint8_t hp_rst              : 1;
  uint8_t not_used_01         : 1;
  uint8_t lpf_bw              : 2;
  uint8_t signy               : 1;
  uint8_t signx               : 1;
} l20g20is_ctrl2_ois_t;

#define L20G20IS_CTRL3_OIS              0x0DU
typedef struct {
  uint8_t lpf_bw              : 1;
  uint8_t h_l_active          : 1;
  uint8_t not_used_01         : 1;
  uint8_t st_en               : 1;
  uint8_t st_sign             : 1;
  uint8_t not_used_02         : 3;
} l20g20is_ctrl3_ois_t;

#define L20G20IS_CTRL4_OIS              0x0EU
typedef struct {
  uint8_t not_used_01         : 1;
  uint8_t drdy_od             : 1;
  uint8_t temp_data_on_drdy   : 1;
  uint8_t not_used_02         : 1;
  uint8_t drdy_en             : 1;
  uint8_t not_used_03         : 3;
} l20g20is_ctrl4_ois_t;

#define L20G20IS_OFF_X                  0x0FU
typedef struct {
  uint8_t offx                : 7;
  uint8_t not_used_01         : 1;
} l20g20is_off_x_t;

#define L20G20IS_OFF_Y                  0x10U
typedef struct {
  uint8_t offy                : 7;
  uint8_t not_used_01         : 1;
} l20g20is_off_y_t;

#define L20G20IS_OIS_CFG_REG            0x1FU
typedef struct {
  uint8_t hpf_bw              : 2;
  uint8_t not_used_01         : 1;
  uint8_t fs_sel              : 1;
  uint8_t not_used_02         : 4;
} l20g20is_ois_cfg_reg_t;

/**
  * @defgroup L20G20IS_Register_Union
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
  l20g20is_data_status_ois_t            data_status_ois;
  l20g20is_ctrl1_ois_t                  ctrl1_ois;
  l20g20is_ctrl2_ois_t                  ctrl2_ois;
  l20g20is_ctrl3_ois_t                  ctrl3_ois;
  l20g20is_ctrl4_ois_t                  ctrl4_ois;
  l20g20is_off_x_t                      off_x;
  l20g20is_off_y_t                      off_y;
  l20g20is_ois_cfg_reg_t                ois_cfg_reg;
  bitwise_t                             bitwise;
  uint8_t                               byte;
} l20g20is_reg_t;

/**
  * @}
  *
  */

int32_t l20g20is_read_reg(l20g20is_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);
int32_t l20g20is_write_reg(l20g20is_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);

extern float_t l20g20is_from_fs100dps_to_mdps(int16_t lsb);
extern float_t l20g20is_from_fs200dps_to_mdps(int16_t lsb);

extern float_t l20g20is_from_lsb_to_celsius(int16_t lsb);

int32_t l20g20is_gy_flag_data_ready_get(l20g20is_ctx_t *ctx, uint8_t *val);

typedef enum {
  L20G20IS_GY_OFF    = 0,
  L20G20IS_GY_SLEEP  = 2,
  L20G20IS_GY_9k33Hz = 3,
} l20g20is_gy_data_rate_t;
int32_t l20g20is_gy_data_rate_set(l20g20is_ctx_t *ctx,
                                  l20g20is_gy_data_rate_t val);
int32_t l20g20is_gy_data_rate_get(l20g20is_ctx_t *ctx,
                                  l20g20is_gy_data_rate_t *val);

typedef struct {
  uint8_t orient    : 1;
  uint8_t signy     : 1;
  uint8_t signx     : 1;
} l20g20is_gy_orient_t;
int32_t l20g20is_gy_orient_set(l20g20is_ctx_t *ctx,
                               l20g20is_gy_orient_t val);
int32_t l20g20is_gy_orient_get(l20g20is_ctx_t *ctx,
                               l20g20is_gy_orient_t *val);

int32_t l20g20is_block_data_update_set(l20g20is_ctx_t *ctx, uint8_t val);
int32_t l20g20is_block_data_update_get(l20g20is_ctx_t *ctx, uint8_t *val);

typedef struct {
  uint8_t offx             : 7;
  uint8_t offy             : 7;
} l20g20is_off_t;
int32_t l20g20is_angular_rate_offset_set(l20g20is_ctx_t *ctx,
                                         l20g20is_off_t val);
int32_t l20g20is_angular_rate_offset_get(l20g20is_ctx_t *ctx,
                                         l20g20is_off_t *val);

typedef enum {
  L20G20IS_100dps   = 0,
  L20G20IS_200dps   = 1,
} l20g20is_gy_fs_t;
int32_t l20g20is_gy_full_scale_set(l20g20is_ctx_t *ctx,
                                   l20g20is_gy_fs_t val);
int32_t l20g20is_gy_full_scale_get(l20g20is_ctx_t *ctx,
                                   l20g20is_gy_fs_t *val);

int32_t l20g20is_temperature_raw_get(l20g20is_ctx_t *ctx, uint8_t *buff);

int32_t l20g20is_angular_rate_raw_get(l20g20is_ctx_t *ctx, uint8_t *buff);

int32_t l20g20is_dev_id_get(l20g20is_ctx_t *ctx, uint8_t *buff);

typedef struct {
  uint8_t xyda_ois             : 1;
} l20g20is_dev_status_t;
int32_t l20g20is_dev_status_get(l20g20is_ctx_t *ctx,
                                l20g20is_dev_status_t *val);

typedef enum {
  L20G20IS_LSB_LOW_ADDRESS = 0,
  L20G20IS_MSB_LOW_ADDRESS = 1,
} l20g20is_ble_t;
int32_t l20g20is_dev_data_format_set(l20g20is_ctx_t *ctx,
                                     l20g20is_ble_t val);
int32_t l20g20is_dev_data_format_get(l20g20is_ctx_t *ctx,
                                     l20g20is_ble_t *val);

int32_t l20g20is_dev_boot_set(l20g20is_ctx_t *ctx, uint8_t val);
int32_t l20g20is_dev_boot_get(l20g20is_ctx_t *ctx, uint8_t *val);

int32_t l20g20is_dev_reset_set(l20g20is_ctx_t *ctx, uint8_t val);
int32_t l20g20is_dev_reset_get(l20g20is_ctx_t *ctx, uint8_t *val);

typedef enum {
  L20G20IS_HPF_BYPASS    = 0x00,
  L20G20IS_HPF_BW_23mHz  = 0x80,
  L20G20IS_HPF_BW_91mHz  = 0x81,
  L20G20IS_HPF_BW_324mHz = 0x82,
  L20G20IS_HPF_BW_1Hz457 = 0x83,
} l20g20is_gy_hp_bw_t;
int32_t l20g20is_gy_filter_hp_bandwidth_set(l20g20is_ctx_t *ctx,
                                            l20g20is_gy_hp_bw_t val);
int32_t l20g20is_gy_filter_hp_bandwidth_get(l20g20is_ctx_t *ctx,
                                            l20g20is_gy_hp_bw_t *val);

int32_t l20g20is_gy_filter_hp_reset_set(l20g20is_ctx_t *ctx, uint8_t val);
int32_t l20g20is_gy_filter_hp_reset_get(l20g20is_ctx_t *ctx, uint8_t *val);

typedef enum {
  L20G20IS_LPF_BW_290Hz  = 0x00,
  L20G20IS_LPF_BW_210Hz  = 0x01,
  L20G20IS_LPF_BW_160Hz  = 0x02,
  L20G20IS_LPF_BW_450Hz  = 0x03,
  L20G20IS_LPF_BW_1150Hz = 0x04,
} l20g20is_gy_lp_bw_t;
int32_t l20g20is_gy_filter_lp_bandwidth_set(l20g20is_ctx_t *ctx,
                                            l20g20is_gy_lp_bw_t val);
int32_t l20g20is_gy_filter_lp_bandwidth_get(l20g20is_ctx_t *ctx,
                                            l20g20is_gy_lp_bw_t *val);

typedef enum {
  L20G20IS_SPI_4_WIRE = 0,
  L20G20IS_SPI_3_WIRE = 1,
} l20g20is_sim_t;
int32_t l20g20is_spi_mode_set(l20g20is_ctx_t *ctx, l20g20is_sim_t val);
int32_t l20g20is_spi_mode_get(l20g20is_ctx_t *ctx, l20g20is_sim_t *val);

typedef enum {
  L20G20IS_INT_PULSED = 1,
  L20G20IS_INT_LATCHED = 0,
} l20g20is_lir_t;
int32_t l20g20is_pin_notification_set(l20g20is_ctx_t *ctx,
                                      l20g20is_lir_t val);
int32_t l20g20is_pin_notification_get(l20g20is_ctx_t *ctx,
                                      l20g20is_lir_t *val);

typedef enum {
  L20G20IS_ACTIVE_HIGH = 0,
  L20G20IS_ACTIVE_LOW = 1,
} l20g20is_pin_pol_t;
int32_t l20g20is_pin_polarity_set(l20g20is_ctx_t *ctx,
                                  l20g20is_pin_pol_t val);
int32_t l20g20is_pin_polarity_get(l20g20is_ctx_t *ctx,
                                  l20g20is_pin_pol_t *val);

typedef enum {
  L20G20IS_PUSH_PULL = 0,
  L20G20IS_OPEN_DRAIN = 1,
} l20g20is_pp_od_t;
int32_t l20g20is_pin_mode_set(l20g20is_ctx_t *ctx, l20g20is_pp_od_t val);
int32_t l20g20is_pin_mode_get(l20g20is_ctx_t *ctx, l20g20is_pp_od_t *val);

typedef struct {
  uint8_t temp_data_on_drdy             : 1;
  uint8_t drdy_en                       : 1;
} l20g20is_pin_drdy_route_t;
int32_t l20g20is_pin_drdy_route_set(l20g20is_ctx_t *ctx,
                                    l20g20is_pin_drdy_route_t val);
int32_t l20g20is_pin_drdy_route_get(l20g20is_ctx_t *ctx,
                                    l20g20is_pin_drdy_route_t *val);

typedef enum {
  L20G20IS_ST_DISABLE  = 0x00,
  L20G20IS_ST_POSITIVE = 0x02,
  L20G20IS_ST_NEGATIVE = 0x03,
} l20g20is_gy_self_test_t;
int32_t l20g20is_gy_self_test_set(l20g20is_ctx_t *ctx,
                                  l20g20is_gy_self_test_t val);
int32_t l20g20is_gy_self_test_get(l20g20is_ctx_t *ctx,
                                  l20g20is_gy_self_test_t *val);

/**
  *@}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* L20G20IS_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
