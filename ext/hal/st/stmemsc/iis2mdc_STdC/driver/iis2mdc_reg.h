/*
 ******************************************************************************
 * @file    iis2mdc_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          iis2mdc_reg.c driver.
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
#ifndef IIS2MDC_REGS_H
#define IIS2MDC_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup IIS2MDC
  * @{
  *
  */

/** @defgroup IIS2MDC_sensors_common_types
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
  * @}
  *
  */

/** @addtogroup  IIS2MDC_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*iis2mdc_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*iis2mdc_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  iis2mdc_write_ptr  write_reg;
  iis2mdc_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} iis2mdc_ctx_t;

/**
  * @}
  *
  */

/** @defgroup iis2mdc_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format **/
#define IIS2MDC_I2C_ADD       0x3DU

/** Device Identification (Who am I) **/
#define IIS2MDC_ID            0x40U

/**
  * @}
  *
  */

/**
  * @addtogroup  IIS2MDC_Sensitivity
  * @brief       These macro are maintained for back compatibility.
  *              in order to convert data into engineering units please
  *              use functions:
  *                -> _from_lsb_to_mgauss(int16_t lsb);
  *                -> _from_lsb_to_celsius(int16_t lsb);
  *
  *              REMOVING the MACRO you are compliant with:
  *              MISRA-C 2012 [Dir 4.9] -> " avoid function-like macros "
  * @{
  *
  */

#define IIS2MDC_FROM_LSB_TO_mG(lsb)     (float)(lsb * 1.5f)
#define IIS2MDC_FROM_LSB_TO_degC(lsb)   (float)(lsb / 8.0f) + (25.0f)

/**
  * @}
  *
  */

#define IIS2MDC_OFFSET_X_REG_L          0x45U
#define IIS2MDC_OFFSET_X_REG_H          0x46U
#define IIS2MDC_OFFSET_Y_REG_L          0x47U
#define IIS2MDC_OFFSET_Y_REG_H          0x48U
#define IIS2MDC_OFFSET_Z_REG_L          0x49U
#define IIS2MDC_OFFSET_Z_REG_H          0x4AU
#define IIS2MDC_WHO_AM_I                0x4FU
#define IIS2MDC_CFG_REG_A               0x60U
typedef struct {
  uint8_t md                     : 2;
  uint8_t odr                    : 2;
  uint8_t lp                     : 1;
  uint8_t soft_rst               : 1;
  uint8_t reboot                 : 1;
  uint8_t comp_temp_en           : 1;
} iis2mdc_cfg_reg_a_t;

#define IIS2MDC_CFG_REG_B               0x61U
typedef struct {
  uint8_t lpf                    : 1;
  uint8_t set_rst                : 2; /* OFF_CANC + Set_FREQ */
  uint8_t int_on_dataoff         : 1;
  uint8_t off_canc_one_shot      : 1;
  uint8_t not_used_01            : 3;
} iis2mdc_cfg_reg_b_t;

#define IIS2MDC_CFG_REG_C               0x62U
typedef struct {
  uint8_t drdy_on_pin            : 1;
  uint8_t self_test              : 1;
  uint8_t not_used_01            : 1;
  uint8_t ble                    : 1;
  uint8_t bdu                    : 1;
  uint8_t i2c_dis                : 1;
  uint8_t int_on_pin             : 1;
  uint8_t not_used_02            : 1;
} iis2mdc_cfg_reg_c_t;

#define IIS2MDC_INT_CRTL_REG            0x63U
typedef struct {
  uint8_t ien                    : 1;
  uint8_t iel                    : 1;
  uint8_t iea                    : 1;
  uint8_t not_used_01            : 2;
  uint8_t zien                   : 1;
  uint8_t yien                   : 1;
  uint8_t xien                   : 1;
} iis2mdc_int_crtl_reg_t;

#define IIS2MDC_INT_SOURCE_REG          0x64U
typedef struct {
  uint8_t _int                    : 1;
  uint8_t mroi                   : 1;
  uint8_t n_th_s_z               : 1;
  uint8_t n_th_s_y               : 1;
  uint8_t n_th_s_x               : 1;
  uint8_t p_th_s_z               : 1;
  uint8_t p_th_s_y               : 1;
  uint8_t p_th_s_x               : 1;
} iis2mdc_int_source_reg_t;

#define IIS2MDC_INT_THS_L_REG           0x65U
#define IIS2MDC_INT_THS_H_REG           0x66U
#define IIS2MDC_STATUS_REG              0x67U
typedef struct {
  uint8_t xda                    : 1;
  uint8_t yda                    : 1;
  uint8_t zda                    : 1;
  uint8_t zyxda                  : 1;
  uint8_t _xor                   : 1;
  uint8_t yor                    : 1;
  uint8_t zor                    : 1;
  uint8_t zyxor                  : 1;
} iis2mdc_status_reg_t;

#define IIS2MDC_OUTX_L_REG              0x68U
#define IIS2MDC_OUTX_H_REG              0x69U
#define IIS2MDC_OUTY_L_REG              0x6AU
#define IIS2MDC_OUTY_H_REG              0x6BU
#define IIS2MDC_OUTZ_L_REG              0x6CU
#define IIS2MDC_OUTZ_H_REG              0x6DU
#define IIS2MDC_TEMP_OUT_L_REG          0x6EU
#define IIS2MDC_TEMP_OUT_H_REG          0x6FU

typedef union{
  iis2mdc_cfg_reg_a_t            cfg_reg_a;
  iis2mdc_cfg_reg_b_t            cfg_reg_b;
  iis2mdc_cfg_reg_c_t            cfg_reg_c;
  iis2mdc_int_crtl_reg_t         int_crtl_reg;
  iis2mdc_int_source_reg_t       int_source_reg;
  iis2mdc_status_reg_t           status_reg;
  bitwise_t                      bitwise;
  uint8_t                        byte;
} iis2mdc_reg_t;

int32_t iis2mdc_read_reg(iis2mdc_ctx_t *ctx, uint8_t reg, uint8_t* data,
                         uint16_t len);
int32_t iis2mdc_write_reg(iis2mdc_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);

float iis2mdc_from_lsb_to_mgauss(int16_t lsb);
float iis2mdc_from_lsb_to_celsius(int16_t lsb);

int32_t iis2mdc_mag_user_offset_set(iis2mdc_ctx_t *ctx, uint8_t *buff);
int32_t iis2mdc_mag_user_offset_get(iis2mdc_ctx_t *ctx, uint8_t *buff);
typedef enum {
  IIS2MDC_CONTINUOUS_MODE  = 0,
  IIS2MDC_SINGLE_TRIGGER   = 1,
  IIS2MDC_POWER_DOWN       = 2,
} iis2mdc_md_t;
int32_t iis2mdc_operating_mode_set(iis2mdc_ctx_t *ctx, iis2mdc_md_t val);
int32_t iis2mdc_operating_mode_get(iis2mdc_ctx_t *ctx, iis2mdc_md_t *val);

typedef enum {
  IIS2MDC_ODR_10Hz   = 0,
  IIS2MDC_ODR_20Hz   = 1,
  IIS2MDC_ODR_50Hz   = 2,
  IIS2MDC_ODR_100Hz  = 3,
} iis2mdc_odr_t;
int32_t iis2mdc_data_rate_set(iis2mdc_ctx_t *ctx, iis2mdc_odr_t val);
int32_t iis2mdc_data_rate_get(iis2mdc_ctx_t *ctx, iis2mdc_odr_t *val);

typedef enum {
  IIS2MDC_HIGH_RESOLUTION  = 0,
  IIS2MDC_LOW_POWER        = 1,
} iis2mdc_lp_t;
int32_t iis2mdc_power_mode_set(iis2mdc_ctx_t *ctx, iis2mdc_lp_t val);
int32_t iis2mdc_power_mode_get(iis2mdc_ctx_t *ctx, iis2mdc_lp_t *val);

int32_t iis2mdc_offset_temp_comp_set(iis2mdc_ctx_t *ctx, uint8_t val);
int32_t iis2mdc_offset_temp_comp_get(iis2mdc_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS2MDC_ODR_DIV_2  = 0,
  IIS2MDC_ODR_DIV_4  = 1,
} iis2mdc_lpf_t;
int32_t iis2mdc_low_pass_bandwidth_set(iis2mdc_ctx_t *ctx,
                                       iis2mdc_lpf_t val);
int32_t iis2mdc_low_pass_bandwidth_get(iis2mdc_ctx_t *ctx,
                                       iis2mdc_lpf_t *val);

typedef enum {
  IIS2MDC_SET_SENS_ODR_DIV_63        = 0,
  IIS2MDC_SENS_OFF_CANC_EVERY_ODR    = 1,
  IIS2MDC_SET_SENS_ONLY_AT_POWER_ON  = 2,
} iis2mdc_set_rst_t;
int32_t iis2mdc_set_rst_mode_set(iis2mdc_ctx_t *ctx,
                                 iis2mdc_set_rst_t val);
int32_t iis2mdc_set_rst_mode_get(iis2mdc_ctx_t *ctx,
                                 iis2mdc_set_rst_t *val);

int32_t iis2mdc_set_rst_sensor_single_set(iis2mdc_ctx_t *ctx,
                                          uint8_t val);
int32_t iis2mdc_set_rst_sensor_single_get(iis2mdc_ctx_t *ctx,
                                          uint8_t *val);

int32_t iis2mdc_block_data_update_set(iis2mdc_ctx_t *ctx, uint8_t val);
int32_t iis2mdc_block_data_update_get(iis2mdc_ctx_t *ctx, uint8_t *val);

int32_t iis2mdc_mag_data_ready_get(iis2mdc_ctx_t *ctx, uint8_t *val);

int32_t iis2mdc_mag_data_ovr_get(iis2mdc_ctx_t *ctx, uint8_t *val);

int32_t iis2mdc_magnetic_raw_get(iis2mdc_ctx_t *ctx, uint8_t *buff);

int32_t iis2mdc_temperature_raw_get(iis2mdc_ctx_t *ctx, uint8_t *buff);

int32_t iis2mdc_device_id_get(iis2mdc_ctx_t *ctx, uint8_t *buff);

int32_t iis2mdc_reset_set(iis2mdc_ctx_t *ctx, uint8_t val);
int32_t iis2mdc_reset_get(iis2mdc_ctx_t *ctx, uint8_t *val);

int32_t iis2mdc_boot_set(iis2mdc_ctx_t *ctx, uint8_t val);
int32_t iis2mdc_boot_get(iis2mdc_ctx_t *ctx, uint8_t *val);

int32_t iis2mdc_self_test_set(iis2mdc_ctx_t *ctx, uint8_t val);
int32_t iis2mdc_self_test_get(iis2mdc_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS2MDC_LSB_AT_LOW_ADD  = 0,
  IIS2MDC_MSB_AT_LOW_ADD  = 1,
} iis2mdc_ble_t;
int32_t iis2mdc_data_format_set(iis2mdc_ctx_t *ctx, iis2mdc_ble_t val);
int32_t iis2mdc_data_format_get(iis2mdc_ctx_t *ctx, iis2mdc_ble_t *val);

int32_t iis2mdc_status_get(iis2mdc_ctx_t *ctx, iis2mdc_status_reg_t *val);

typedef enum {
  IIS2MDC_CHECK_BEFORE  = 0,
  IIS2MDC_CHECK_AFTER   = 1,
} iis2mdc_int_on_dataoff_t;
int32_t iis2mdc_offset_int_conf_set(iis2mdc_ctx_t *ctx,
                                    iis2mdc_int_on_dataoff_t val);
int32_t iis2mdc_offset_int_conf_get(iis2mdc_ctx_t *ctx,
                                    iis2mdc_int_on_dataoff_t *val);

int32_t iis2mdc_drdy_on_pin_set(iis2mdc_ctx_t *ctx, uint8_t val);
int32_t iis2mdc_drdy_on_pin_get(iis2mdc_ctx_t *ctx, uint8_t *val);

int32_t iis2mdc_int_on_pin_set(iis2mdc_ctx_t *ctx, uint8_t val);
int32_t iis2mdc_int_on_pin_get(iis2mdc_ctx_t *ctx, uint8_t *val);

int32_t iis2mdc_int_gen_conf_set(iis2mdc_ctx_t *ctx,
                                 iis2mdc_int_crtl_reg_t *val);
int32_t iis2mdc_int_gen_conf_get(iis2mdc_ctx_t *ctx,
                                 iis2mdc_int_crtl_reg_t *val);

int32_t iis2mdc_int_gen_source_get(iis2mdc_ctx_t *ctx,
                                   iis2mdc_int_source_reg_t *val);

int32_t iis2mdc_int_gen_treshold_set(iis2mdc_ctx_t *ctx, uint8_t *buff);
int32_t iis2mdc_int_gen_treshold_get(iis2mdc_ctx_t *ctx, uint8_t *buff);

typedef enum {
  IIS2MDC_I2C_ENABLE   = 0,
  IIS2MDC_I2C_DISABLE  = 1,
} iis2mdc_i2c_dis_t;
int32_t iis2mdc_i2c_interface_set(iis2mdc_ctx_t *ctx,
                                  iis2mdc_i2c_dis_t val);
int32_t iis2mdc_i2c_interface_get(iis2mdc_ctx_t *ctx,
                                  iis2mdc_i2c_dis_t *val);

/**
  * @}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* IIS2MDC_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
