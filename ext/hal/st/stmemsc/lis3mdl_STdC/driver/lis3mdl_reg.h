/*
 ******************************************************************************
 * @file    lis3mdl_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          lis3mdl_reg.c driver.
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
#ifndef LIS3MDL_REGS_H
#define LIS3MDL_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LIS3MDL
  * @{
  *
  */

/** @defgroup LIS3MDL_sensors_common_types
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

/** @addtogroup  LIS3MDL_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*lis3mdl_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lis3mdl_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lis3mdl_write_ptr  write_reg;
  lis3mdl_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lis3mdl_ctx_t;

/**
  * @}
  *
  */

/** @defgroup LIS3MDL_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format  if SA0=0 -> 0x39 if SA0=1 -> 0x3D **/
#define LIS3MDL_I2C_ADD_L   0x39U
#define LIS3MDL_I2C_ADD_H   0x3DU

/** Device Identification (Who am I) **/
#define LIS3MDL_ID          0x3DU

/**
  * @}
  *
  */

/**
  * @addtogroup  LIS3MDL_Sensitivity
  * @brief       These macro are maintained for back compatibility.
  *              in order to convert data into engineering units please
  *              use functions:
  *                -> _from_fs4_to_gauss(int16_t lsb);
  *                -> _from_fs8_to_gauss(int16_t lsb);
  *                -> _from_fs12_to_gauss(int16_t lsb);
  *                -> _from_fs16_to_gauss(int16_t lsb);
  *                -> _from_lsb_to_celsius(int16_t lsb);
  *
  *              REMOVING the MACRO you are compliant with:
  *              MISRA-C 2012 [Dir 4.9] -> " avoid function-like macros "
  * @{
  *
  */

#define LIS3MDL_FROM_FS_4G_TO_G(lsb)     (float)( lsb / 6842.0f )
#define LIS3MDL_FROM_FS_8G_TO_G(lsb)     (float)( lsb / 3421.0f )
#define LIS3MDL_FROM_FS_12G_TO_G(lsb)    (float)( lsb / 2281.0f )
#define LIS3MDL_FROM_FS_16G_TO_G(lsb)    (float)( lsb / 1711.0f )

#define LIS3MDL_FROM_LSB_TO_degC(lsb)    (float)(lsb / 8.0f ) + ( 25.0f )

/**
  * @}
  *
  */

#define LIS3MDL_WHO_AM_I       0x0FU
#define LIS3MDL_CTRL_REG1      0x20U
typedef struct{
  uint8_t st              : 1;
  uint8_t om              : 6; /* om + do + fast_odr -> om */
  uint8_t temp_en         : 1;
} lis3mdl_ctrl_reg1_t;

#define LIS3MDL_CTRL_REG2      0x21U
typedef struct
{
  uint8_t not_used_01     : 2;
  uint8_t soft_rst        : 1;
  uint8_t reboot          : 1;
  uint8_t not_used_02     : 1;
  uint8_t fs              : 2;
  uint8_t not_used_03     : 1;
} lis3mdl_ctrl_reg2_t;

#define LIS3MDL_CTRL_REG3      0x22U
typedef struct{
  uint8_t md              : 2;
  uint8_t sim             : 1;
  uint8_t not_used_01     : 2;
  uint8_t lp              : 1;
  uint8_t not_used_02     : 2;
} lis3mdl_ctrl_reg3_t;

#define LIS3MDL_CTRL_REG4      0x23U
typedef struct{
  uint8_t not_used_01     : 1;
  uint8_t ble             : 1;
  uint8_t omz             : 2;
  uint8_t not_used_02     : 4;
} lis3mdl_ctrl_reg4_t;

#define LIS3MDL_CTRL_REG5      0x24U
typedef struct{
  uint8_t not_used_01     : 6;
  uint8_t bdu             : 1;
  uint8_t fast_read       : 1;
} lis3mdl_ctrl_reg5_t;

#define LIS3MDL_STATUS_REG     0x27U
typedef struct{
  uint8_t xda             : 1;
  uint8_t yda             : 1;
  uint8_t zda             : 1;
  uint8_t zyxda           : 1;
  uint8_t _xor            : 1;
  uint8_t yor             : 1;
  uint8_t zor             : 1;
  uint8_t zyxor           : 1;
} lis3mdl_status_reg_t;

#define LIS3MDL_OUT_X_L        0x28U
#define LIS3MDL_OUT_X_H        0x29U
#define LIS3MDL_OUT_Y_L        0x2AU
#define LIS3MDL_OUT_Y_H        0x2BU
#define LIS3MDL_OUT_Z_L        0x2CU
#define LIS3MDL_OUT_Z_H        0x2DU
#define LIS3MDL_TEMP_OUT_L     0x2EU
#define LIS3MDL_TEMP_OUT_H     0x2FU
#define LIS3MDL_INT_CFG        0x30U
typedef struct{
  uint8_t ien             : 1;
  uint8_t lir             : 1;
  uint8_t iea             : 1;
  uint8_t not_used_01     : 2;
  uint8_t zien            : 1;
  uint8_t yien            : 1;
  uint8_t xien            : 1;
} lis3mdl_int_cfg_t;

#define LIS3MDL_INT_SRC        0x31U
typedef struct{
  uint8_t int_            : 1;
  uint8_t mroi            : 1;
  uint8_t nth_z           : 1;
  uint8_t nth_y           : 1;
  uint8_t nth_x           : 1;
  uint8_t pth_z           : 1;
  uint8_t pth_y           : 1;
  uint8_t pth_x           : 1;
} lis3mdl_int_src_t;

#define LIS3MDL_INT_THS_L      0x32U
#define LIS3MDL_INT_THS_H      0x33U

/**
  * @defgroup LIS3MDL_Register_Union
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
  lis3mdl_ctrl_reg1_t      ctrl_reg1;
  lis3mdl_ctrl_reg2_t      ctrl_reg2;
  lis3mdl_ctrl_reg3_t      ctrl_reg3;
  lis3mdl_ctrl_reg4_t      ctrl_reg4;
  lis3mdl_ctrl_reg5_t      ctrl_reg5;
  lis3mdl_status_reg_t     status_reg;
  lis3mdl_int_cfg_t        int_cfg;
  lis3mdl_int_src_t        int_src;
  bitwise_t                bitwise;
  uint8_t                  byte;
} lis3mdl_reg_t;

/**
  * @}
  *
  */

int32_t lis3mdl_read_reg(lis3mdl_ctx_t *ctx, uint8_t reg, uint8_t* data,
                         uint16_t len);
int32_t lis3mdl_write_reg(lis3mdl_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);

extern float lis3mdl_from_fs4_to_gauss(int16_t lsb);
extern float lis3mdl_from_fs8_to_gauss(int16_t lsb);
extern float lis3mdl_from_fs12_to_gauss(int16_t lsb);
extern float lis3mdl_from_fs16_to_gauss(int16_t lsb);
extern float lis3mdl_from_lsb_to_celsius(int16_t lsb);

typedef enum{
  LIS3MDL_LP_Hz625      = 0x00,
  LIS3MDL_LP_1kHz       = 0x01,
  LIS3MDL_MP_560Hz      = 0x11,
  LIS3MDL_HP_300Hz      = 0x21,
  LIS3MDL_UHP_155Hz     = 0x31,

  LIS3MDL_LP_1Hz25      = 0x02,
  LIS3MDL_LP_2Hz5       = 0x04,
  LIS3MDL_LP_5Hz        = 0x06,
  LIS3MDL_LP_10Hz       = 0x08,
  LIS3MDL_LP_20Hz       = 0x0A,
  LIS3MDL_LP_40Hz       = 0x0C,
  LIS3MDL_LP_80Hz       = 0x0E,

  LIS3MDL_MP_1Hz25      = 0x12,
  LIS3MDL_MP_2Hz5       = 0x14,
  LIS3MDL_MP_5Hz        = 0x16,
  LIS3MDL_MP_10Hz       = 0x18,
  LIS3MDL_MP_20Hz       = 0x1A,
  LIS3MDL_MP_40Hz       = 0x1C,
  LIS3MDL_MP_80Hz       = 0x1E,

  LIS3MDL_HP_1Hz25      = 0x22,
  LIS3MDL_HP_2Hz5       = 0x24,
  LIS3MDL_HP_5Hz        = 0x26,
  LIS3MDL_HP_10Hz       = 0x28,
  LIS3MDL_HP_20Hz       = 0x2A,
  LIS3MDL_HP_40Hz       = 0x2C,
  LIS3MDL_HP_80Hz       = 0x2E,

  LIS3MDL_UHP_1Hz25     = 0x32,
  LIS3MDL_UHP_2Hz5      = 0x34,
  LIS3MDL_UHP_5Hz       = 0x36,
  LIS3MDL_UHP_10Hz      = 0x38,
  LIS3MDL_UHP_20Hz      = 0x3A,
  LIS3MDL_UHP_40Hz      = 0x3C,
  LIS3MDL_UHP_80Hz      = 0x3E,

} lis3mdl_om_t;
int32_t lis3mdl_data_rate_set(lis3mdl_ctx_t *ctx, lis3mdl_om_t val);
int32_t lis3mdl_data_rate_get(lis3mdl_ctx_t *ctx, lis3mdl_om_t *val);

int32_t lis3mdl_temperature_meas_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_temperature_meas_get(lis3mdl_ctx_t *ctx, uint8_t *val);

typedef enum{
  LIS3MDL_4_GAUSS   = 0,
  LIS3MDL_8_GAUSS   = 1,
  LIS3MDL_12_GAUSS  = 2,
  LIS3MDL_16_GAUSS  = 3,
} lis3mdl_fs_t;
int32_t lis3mdl_full_scale_set(lis3mdl_ctx_t *ctx, lis3mdl_fs_t val);
int32_t lis3mdl_full_scale_get(lis3mdl_ctx_t *ctx, lis3mdl_fs_t *val);

typedef enum{
  LIS3MDL_CONTINUOUS_MODE  = 0,
  LIS3MDL_SINGLE_TRIGGER   = 1,
  LIS3MDL_POWER_DOWN       = 2,
} lis3mdl_md_t;
int32_t lis3mdl_operating_mode_set(lis3mdl_ctx_t *ctx, lis3mdl_md_t val);
int32_t lis3mdl_operating_mode_get(lis3mdl_ctx_t *ctx, lis3mdl_md_t *val);

int32_t lis3mdl_fast_low_power_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_fast_low_power_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_block_data_update_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_block_data_update_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_high_part_cycle_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_high_part_cycle_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_mag_data_ready_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_mag_data_ovr_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_magnetic_raw_get(lis3mdl_ctx_t *ctx, uint8_t *buff);

int32_t lis3mdl_temperature_raw_get(lis3mdl_ctx_t *ctx, uint8_t *buff);

int32_t lis3mdl_device_id_get(lis3mdl_ctx_t *ctx, uint8_t *buff);

int32_t lis3mdl_self_test_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_self_test_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_reset_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_reset_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_boot_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_boot_get(lis3mdl_ctx_t *ctx, uint8_t *val);

typedef enum{
  LIS3MDL_LSB_AT_LOW_ADD  = 0,
  LIS3MDL_MSB_AT_LOW_ADD  = 1,
} lis3mdl_ble_t;
int32_t lis3mdl_data_format_set(lis3mdl_ctx_t *ctx, lis3mdl_ble_t val);
int32_t lis3mdl_data_format_get(lis3mdl_ctx_t *ctx, lis3mdl_ble_t *val);

int32_t lis3mdl_status_get(lis3mdl_ctx_t *ctx, lis3mdl_status_reg_t *val);

int32_t lis3mdl_int_config_set(lis3mdl_ctx_t *ctx, lis3mdl_int_cfg_t *val);
int32_t lis3mdl_int_config_get(lis3mdl_ctx_t *ctx, lis3mdl_int_cfg_t *val);

int32_t lis3mdl_int_generation_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_int_generation_get(lis3mdl_ctx_t *ctx, uint8_t *val);

typedef enum{
  LIS3MDL_INT_PULSED   = 0,
  LIS3MDL_INT_LATCHED  = 1,
} lis3mdl_lir_t;
int32_t lis3mdl_int_notification_mode_set(lis3mdl_ctx_t *ctx,
                                          lis3mdl_lir_t val);
int32_t lis3mdl_int_notification_mode_get(lis3mdl_ctx_t *ctx,
                                          lis3mdl_lir_t *val);

typedef enum{
  LIS3MDL_ACTIVE_HIGH  = 0,
  LIS3MDL_ACTIVE_LOW   = 1,
} lis3mdl_iea_t;
int32_t lis3mdl_int_polarity_set(lis3mdl_ctx_t *ctx, lis3mdl_iea_t val);
int32_t lis3mdl_int_polarity_get(lis3mdl_ctx_t *ctx, lis3mdl_iea_t *val);

int32_t lis3mdl_int_on_z_ax_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_int_on_z_ax_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_on_y_ax_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_int_on_y_ax_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_on_x_ax_set(lis3mdl_ctx_t *ctx, uint8_t val);
int32_t lis3mdl_int_on_x_ax_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_source_get(lis3mdl_ctx_t *ctx, lis3mdl_int_src_t *val);

int32_t lis3mdl_interrupt_event_flag_get(lis3mdl_ctx_t *ctx,
    uint8_t *val);

int32_t lis3mdl_int_mag_over_range_flag_get(lis3mdl_ctx_t *ctx,
    uint8_t *val);

int32_t lis3mdl_int_neg_z_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_neg_y_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_neg_x_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_pos_z_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_pos_y_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_pos_x_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val);

int32_t lis3mdl_int_threshold_set(lis3mdl_ctx_t *ctx, uint8_t *buff);
int32_t lis3mdl_int_threshold_get(lis3mdl_ctx_t *ctx, uint8_t *buff);

typedef enum{
  LIS3MDL_SPI_4_WIRE   = 0,
  LIS3MDL_SPI_3_WIRE   = 1,
} lis3mdl_sim_t;
int32_t lis3mdl_spi_mode_set(lis3mdl_ctx_t *ctx, lis3mdl_sim_t val);
int32_t lis3mdl_spi_mode_get(lis3mdl_ctx_t *ctx, lis3mdl_sim_t *val);

/**
  *@}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* LIS3MDL_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
