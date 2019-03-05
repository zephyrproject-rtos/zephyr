/*
 ******************************************************************************
 * @file    li3dhh_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          li3dhh_reg.c driver.
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
#ifndef LIS3DHH_REGS_H
#define LIS3DHH_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LIS3DHH
  * @{
  *
  */

/** @defgroup LIS3DHH_sensors_common_types
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

/** @addtogroup  LIS3DHH_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*lis3dhh_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lis3dhh_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lis3dhh_write_ptr  write_reg;
  lis3dhh_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lis3dhh_ctx_t;

/**
  * @}
  *
  */

/** @defgroup LIS3DHH_Infos
  * @{
  *
  */

/** Device Identification (Who am I) **/
#define LIS3DHH_ID            0x11U

/**
  * @}
  *
  */

#define LIS3DHH_WHO_AM_I      0x0FU
#define LIS3DHH_ID_REG        0x1BU
typedef struct {
  uint8_t not_used_01      : 7;
  uint8_t asic_id          : 1;
} lis3dhh_id_reg_t;

#define LIS3DHH_CTRL_REG1     0x20U
typedef struct {
  uint8_t bdu              : 1;
  uint8_t drdy_pulse       : 1;
  uint8_t sw_reset         : 1;
  uint8_t boot             : 1;
  uint8_t not_used_01      : 2;
  uint8_t if_add_inc       : 1;
  uint8_t norm_mod_en      : 1;
} lis3dhh_ctrl_reg1_t;

#define LIS3DHH_INT1_CTRL     0x21U
typedef struct {
  uint8_t not_used_01      : 2;
  uint8_t int1_ext         : 1;
  uint8_t int1_fth         : 1;
  uint8_t int1_fss5        : 1;
  uint8_t int1_ovr         : 1;
  uint8_t int1_boot        : 1;
  uint8_t int1_drdy        : 1;
} lis3dhh_int1_ctrl_t;

#define LIS3DHH_INT2_CTRL     0x22U
typedef struct {
  uint8_t not_used_01      : 3;
  uint8_t int2_fth         : 1;
  uint8_t int2_fss5        : 1;
  uint8_t int2_ovr         : 1;
  uint8_t int2_boot        : 1;
  uint8_t int2_drdy        : 1;
} lis3dhh_int2_ctrl_t;

#define LIS3DHH_CTRL_REG4     0x23U
typedef struct {
  uint8_t off_tcomp_en     : 1;
  uint8_t fifo_en          : 1;
  uint8_t pp_od            : 2;
  uint8_t st               : 2;
  uint8_t dsp              : 2;
} lis3dhh_ctrl_reg4_t;

#define LIS3DHH_CTRL_REG5     0x24U
typedef struct {
  uint8_t fifo_spi_hs_on   : 1;
  uint8_t not_used_01      : 7;
} lis3dhh_ctrl_reg5_t;

#define LIS3DHH_OUT_TEMP_L    0x25U
#define LIS3DHH_OUT_TEMP_H    0x26U
#define LIS3DHH_STATUS        0x27U
typedef struct {
  uint8_t xda              : 1;
  uint8_t yda              : 1;
  uint8_t zda              : 1;
  uint8_t zyxda            : 1;
  uint8_t _xor             : 1;
  uint8_t yor              : 1;
  uint8_t zor              : 1;
  uint8_t zyxor            : 1;
} lis3dhh_status_t;

#define LIS3DHH_OUT_X_L_XL    0x28U
#define LIS3DHH_OUT_X_H_XL    0x29U
#define LIS3DHH_OUT_Y_L_XL    0x2AU
#define LIS3DHH_OUT_Y_H_XL    0x2BU
#define LIS3DHH_OUT_Z_L_XL    0x2CU
#define LIS3DHH_OUT_Z_H_XL    0x2DU
#define LIS3DHH_FIFO_CTRL     0x2EU
typedef struct {
  uint8_t fth              : 5;
  uint8_t fmode            : 3;
} lis3dhh_fifo_ctrl_t;

#define LIS3DHH_FIFO_SRC      0x2FU
typedef struct {
  uint8_t fss      : 6;
  uint8_t ovrn      : 1;
  uint8_t fth      : 1;
} lis3dhh_fifo_src_t;

/**
  * @defgroup LIS3DHH_Register_Union
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
  lis3dhh_id_reg_t       id_reg;
  lis3dhh_ctrl_reg1_t    ctrl_reg1;
  lis3dhh_int1_ctrl_t    int1_ctrl;
  lis3dhh_int2_ctrl_t    int2_ctrl;
  lis3dhh_ctrl_reg4_t    ctrl_reg4;
  lis3dhh_ctrl_reg5_t    ctrl_reg5;
  lis3dhh_status_t       status;
  lis3dhh_fifo_ctrl_t    fifo_ctrl;
  lis3dhh_fifo_src_t     fifo_src;
  bitwise_t              bitwise;
  uint8_t                byte;
} lis3dhh_reg_t;

/**
  * @}
  *
  */

int32_t lis3dhh_read_reg(lis3dhh_ctx_t *ctx, uint8_t reg, uint8_t* data,
                         uint16_t len);
int32_t lis3dhh_write_reg(lis3dhh_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);

extern float_t lis3dhh_from_lsb_to_mg(int16_t lsb);
extern float_t lis3dhh_from_lsb_to_celsius(int16_t lsb);

int32_t lis3dhh_block_data_update_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_block_data_update_get(lis3dhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DHH_POWER_DOWN  = 0,
  LIS3DHH_1kHz1       = 1,
} lis3dhh_norm_mod_en_t;
int32_t lis3dhh_data_rate_set(lis3dhh_ctx_t *ctx, lis3dhh_norm_mod_en_t val);
int32_t lis3dhh_data_rate_get(lis3dhh_ctx_t *ctx, lis3dhh_norm_mod_en_t *val);

int32_t lis3dhh_offset_temp_comp_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_offset_temp_comp_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_temperature_raw_get(lis3dhh_ctx_t *ctx, uint8_t *buff);
int32_t lis3dhh_acceleration_raw_get(lis3dhh_ctx_t *ctx, uint8_t *buff);

int32_t lis3dhh_xl_data_ready_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_xl_data_ovr_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_device_id_get(lis3dhh_ctx_t *ctx, uint8_t *buff);
typedef enum {
  LIS3DHH_VER_A   = 0,
  LIS3DHH_VER_B   = 1,
} lis3dhh_asic_id_t;
int32_t lis3dhh_asic_id_get(lis3dhh_ctx_t *ctx, lis3dhh_asic_id_t *val);


int32_t lis3dhh_reset_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_reset_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_boot_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_boot_get(lis3dhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DHH_ST_DISABLE   = 0,
  LIS3DHH_ST_POSITIVE  = 1,
  LIS3DHH_ST_NEGATIVE  = 2,
} lis3dhh_st_t;
int32_t lis3dhh_self_test_set(lis3dhh_ctx_t *ctx, lis3dhh_st_t val);
int32_t lis3dhh_self_test_get(lis3dhh_ctx_t *ctx, lis3dhh_st_t *val);

typedef enum {
  LIS3DHH_LINEAR_PHASE_440Hz      = 0,
  LIS3DHH_LINEAR_PHASE_235Hz      = 1,
  LIS3DHH_NO_LINEAR_PHASE_440Hz   = 2,
  LIS3DHH_NO_LINEAR_PHASE_235Hz   = 3,
} lis3dhh_dsp_t;
int32_t lis3dhh_filter_config_set(lis3dhh_ctx_t *ctx, lis3dhh_dsp_t val);
int32_t lis3dhh_filter_config_get(lis3dhh_ctx_t *ctx, lis3dhh_dsp_t *val);

int32_t lis3dhh_status_get(lis3dhh_ctx_t *ctx, lis3dhh_status_t *val);

typedef enum {
  LIS3DHH_LATCHED  = 0,
  LIS3DHH_PULSED   = 1,
} lis3dhh_drdy_pulse_t;
int32_t lis3dhh_drdy_notification_mode_set(lis3dhh_ctx_t *ctx,
                                           lis3dhh_drdy_pulse_t val);
int32_t lis3dhh_drdy_notification_mode_get(lis3dhh_ctx_t *ctx,
                                           lis3dhh_drdy_pulse_t *val);


typedef enum {
  LIS3DHH_PIN_AS_INTERRUPT   = 0,
  LIS3DHH_PIN_AS_TRIGGER     = 1,
} lis3dhh_int1_ext_t;
int32_t lis3dhh_int1_mode_set(lis3dhh_ctx_t *ctx, lis3dhh_int1_ext_t val);
int32_t lis3dhh_int1_mode_get(lis3dhh_ctx_t *ctx, lis3dhh_int1_ext_t *val);


int32_t lis3dhh_fifo_threshold_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_threshold_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_full_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_full_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_ovr_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_ovr_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_boot_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_boot_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_drdy_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_drdy_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_threshold_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_threshold_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_full_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_full_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_ovr_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_ovr_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_boot_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_boot_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_drdy_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_drdy_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DHH_ALL_PUSH_PULL    = 0,
  LIS3DHH_INT1_OD_INT2_PP  = 1,
  LIS3DHH_INT1_PP_INT2_OD  = 2,
  LIS3DHH_ALL_OPEN_DRAIN   = 3,
} lis3dhh_pp_od_t;
int32_t lis3dhh_pin_mode_set(lis3dhh_ctx_t *ctx, lis3dhh_pp_od_t val);
int32_t lis3dhh_pin_mode_get(lis3dhh_ctx_t *ctx, lis3dhh_pp_od_t *val);

int32_t lis3dhh_fifo_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_block_spi_hs_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_block_spi_hs_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_watermark_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_fifo_watermark_get(lis3dhh_ctx_t *ctx, uint8_t *val);

typedef enum {
  LIS3DHH_BYPASS_MODE             = 0,
  LIS3DHH_FIFO_MODE               = 1,
  LIS3DHH_STREAM_TO_FIFO_MODE     = 3,
  LIS3DHH_BYPASS_TO_STREAM_MODE   = 4,
  LIS3DHH_DYNAMIC_STREAM_MODE     = 6,
} lis3dhh_fmode_t;
int32_t lis3dhh_fifo_mode_set(lis3dhh_ctx_t *ctx, lis3dhh_fmode_t val);
int32_t lis3dhh_fifo_mode_get(lis3dhh_ctx_t *ctx, lis3dhh_fmode_t *val);

int32_t lis3dhh_fifo_status_get(lis3dhh_ctx_t *ctx, lis3dhh_fifo_src_t *val);

int32_t lis3dhh_fifo_full_flag_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_ovr_flag_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_fifo_fth_flag_get(lis3dhh_ctx_t *ctx, uint8_t *val);

int32_t lis3dhh_auto_add_inc_set(lis3dhh_ctx_t *ctx, uint8_t val);
int32_t lis3dhh_auto_add_inc_get(lis3dhh_ctx_t *ctx, uint8_t *val);

/**
  *@}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /* LIS3DHH_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
