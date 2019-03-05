/*
 ******************************************************************************
 * @file    iis3dhhc_reg.h
 * @author  MEMS Software Solution Team
 * @date    20-December-2017
 * @brief   This file contains all the functions prototypes for the
 *          iis3dhhc_reg.c driver.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __IIS3DHHC_DRIVER__H
#define __IIS3DHHC_DRIVER__H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup IIS3DHHC
  * @{
  *
  */

/** @defgroup IIS3DHHC_sensors_common_types
  * @{
  *
  */

#ifndef MEMS_SHARED_TYPES
#define MEMS_SHARED_TYPES

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

/** @defgroup iis3dhhc_interface
  * @{
  */

typedef int32_t (*iis3dhhc_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*iis3dhhc_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  iis3dhhc_write_ptr  write_reg;
  iis3dhhc_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} iis3dhhc_ctx_t;

/**
  * @}
  */


/** @defgroup iis3dhhc_Infos
  * @{
  */

/** Device Identification (Who am I) **/
#define IIS3DHHC_ID            0x11

/**
  * @}
  */

/**
  * @defgroup iis3dhhc_Sensitivity
  * @{
  */

#define IIS3DHHC_FROM_LSB_TO_mg(lsb)   (float)((lsb * 76.0f) / 1000.0f)
#define IIS3DHHC_FROM_LSB_TO_degC(lsb) (float)(((int16_t)lsb>>4) / 16.0f) + 25.0f

/**
  * @}
  */

#define IIS3DHHC_WHO_AM_I      0x0F

#define IIS3DHHC_CTRL_REG1     0x20
typedef struct {
  uint8_t bdu              : 1;
  uint8_t drdy_pulse       : 1;
  uint8_t sw_reset         : 1;
  uint8_t boot             : 1;
  uint8_t not_used_01      : 2;
  uint8_t if_add_inc       : 1;
  uint8_t norm_mod_en      : 1;
} iis3dhhc_ctrl_reg1_t;

#define IIS3DHHC_INT1_CTRL     0x21
typedef struct {
  uint8_t not_used_01      : 2;
  uint8_t int1_ext         : 1;
  uint8_t int1_fth         : 1;
  uint8_t int1_fss5        : 1;
  uint8_t int1_ovr         : 1;
  uint8_t int1_boot        : 1;
  uint8_t int1_drdy        : 1;
} iis3dhhc_int1_ctrl_t;

#define IIS3DHHC_INT2_CTRL     0x22
typedef struct {
  uint8_t not_used_01      : 3;
  uint8_t int2_fth         : 1;
  uint8_t int2_fss5        : 1;
  uint8_t int2_ovr         : 1;
  uint8_t int2_boot        : 1;
  uint8_t int2_drdy        : 1;
} iis3dhhc_int2_ctrl_t;

#define IIS3DHHC_CTRL_REG4     0x23
typedef struct {
  uint8_t off_tcomp_en     : 1;
  uint8_t fifo_en          : 1;
  uint8_t pp_od            : 2;
  uint8_t st               : 2;
  uint8_t dsp              : 2;
} iis3dhhc_ctrl_reg4_t;

#define IIS3DHHC_CTRL_REG5     0x24
typedef struct {
  uint8_t fifo_spi_hs_on   : 1;
  uint8_t not_used_01      : 7;
} iis3dhhc_ctrl_reg5_t;

#define IIS3DHHC_OUT_TEMP_L    0x25
#define IIS3DHHC_OUT_TEMP_H    0x26
#define IIS3DHHC_STATUS        0x27
typedef struct {
  uint8_t xda              : 1;
  uint8_t yda              : 1;
  uint8_t zda              : 1;
  uint8_t zyxda            : 1;
  uint8_t _xor             : 1;
  uint8_t yor              : 1;
  uint8_t zor              : 1;
  uint8_t zyxor            : 1;
} iis3dhhc_status_t;

#define IIS3DHHC_OUT_X_L_XL    0x28
#define IIS3DHHC_OUT_X_H_XL    0x29
#define IIS3DHHC_OUT_Y_L_XL    0x2A
#define IIS3DHHC_OUT_Y_H_XL    0x2B
#define IIS3DHHC_OUT_Z_L_XL    0x2C
#define IIS3DHHC_OUT_Z_H_XL    0x2D
#define IIS3DHHC_FIFO_CTRL     0x2E
typedef struct {
  uint8_t fth              : 5;
  uint8_t fmode            : 3;
} iis3dhhc_fifo_ctrl_t;

#define IIS3DHHC_FIFO_SRC      0x2F
typedef struct {
  uint8_t fss      : 6;
  uint8_t ovrn      : 1;
  uint8_t fth      : 1;
} iis3dhhc_fifo_src_t;

typedef union{
  iis3dhhc_ctrl_reg1_t    ctrl_reg1;
  iis3dhhc_int1_ctrl_t    int1_ctrl;
  iis3dhhc_int2_ctrl_t    int2_ctrl;
  iis3dhhc_ctrl_reg4_t    ctrl_reg4;
  iis3dhhc_ctrl_reg5_t    ctrl_reg5;
  iis3dhhc_status_t       status;
  iis3dhhc_fifo_ctrl_t    fifo_ctrl;
  iis3dhhc_fifo_src_t     fifo_src;
  bitwise_t              bitwise;
  uint8_t                byte;
} iis3dhhc_reg_t;
int32_t iis3dhhc_read_reg(iis3dhhc_ctx_t *ctx, uint8_t reg, uint8_t* data,
                         uint16_t len);
int32_t iis3dhhc_write_reg(iis3dhhc_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);

int32_t iis3dhhc_block_data_update_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_block_data_update_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS3DHHC_POWER_DOWN  = 0,
  IIS3DHHC_1kHz1       = 1,
} iis3dhhc_norm_mod_en_t;
int32_t iis3dhhc_data_rate_set(iis3dhhc_ctx_t *ctx, iis3dhhc_norm_mod_en_t val);
int32_t iis3dhhc_data_rate_get(iis3dhhc_ctx_t *ctx, iis3dhhc_norm_mod_en_t *val);

int32_t iis3dhhc_offset_temp_comp_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_offset_temp_comp_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_temperature_raw_get(iis3dhhc_ctx_t *ctx, uint8_t *buff);
int32_t iis3dhhc_acceleration_raw_get(iis3dhhc_ctx_t *ctx, uint8_t *buff);

int32_t iis3dhhc_xl_data_ready_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_xl_data_ovr_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_device_id_get(iis3dhhc_ctx_t *ctx, uint8_t *buff);

int32_t iis3dhhc_reset_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_reset_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_boot_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_boot_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS3DHHC_ST_DISABLE   = 0,
  IIS3DHHC_ST_POSITIVE  = 1,
  IIS3DHHC_ST_NEGATIVE  = 2,
} iis3dhhc_st_t;
int32_t iis3dhhc_self_test_set(iis3dhhc_ctx_t *ctx, iis3dhhc_st_t val);
int32_t iis3dhhc_self_test_get(iis3dhhc_ctx_t *ctx, iis3dhhc_st_t *val);

typedef enum {
  IIS3DHHC_LINEAR_PHASE_440Hz      = 0,
  IIS3DHHC_LINEAR_PHASE_235Hz      = 1,
  IIS3DHHC_NO_LINEAR_PHASE_440Hz  = 2,
  IIS3DHHC_NO_LINEAR_PHASE_235Hz  = 3,
} iis3dhhc_dsp_t;
int32_t iis3dhhc_filter_config_set(iis3dhhc_ctx_t *ctx, iis3dhhc_dsp_t val);
int32_t iis3dhhc_filter_config_get(iis3dhhc_ctx_t *ctx, iis3dhhc_dsp_t *val);

int32_t iis3dhhc_status_get(iis3dhhc_ctx_t *ctx, iis3dhhc_status_t *val);

typedef enum {
  IIS3DHHC_LATCHED  = 0,
  IIS3DHHC_PULSED   = 1,
} iis3dhhc_drdy_pulse_t;
int32_t iis3dhhc_drdy_notification_mode_set(iis3dhhc_ctx_t *ctx,
                                            iis3dhhc_drdy_pulse_t val);
int32_t iis3dhhc_drdy_notification_mode_get(iis3dhhc_ctx_t *ctx,
                                            iis3dhhc_drdy_pulse_t *val);


typedef enum {
  IIS3DHHC_PIN_AS_INTERRUPT  = 0,
  IIS3DHHC_PIN_AS_TRIGGER     = 1,
} iis3dhhc_int1_ext_t;
int32_t iis3dhhc_int1_mode_set(iis3dhhc_ctx_t *ctx, iis3dhhc_int1_ext_t val);
int32_t iis3dhhc_int1_mode_get(iis3dhhc_ctx_t *ctx, iis3dhhc_int1_ext_t *val);


int32_t iis3dhhc_fifo_threshold_on_int1_set(iis3dhhc_ctx_t *ctx,
                                            uint8_t val);
int32_t iis3dhhc_fifo_threshold_on_int1_get(iis3dhhc_ctx_t *ctx,
                                            uint8_t *val);

int32_t iis3dhhc_fifo_full_on_int1_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_fifo_full_on_int1_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_fifo_ovr_on_int1_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_fifo_ovr_on_int1_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_boot_on_int1_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_boot_on_int1_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_drdy_on_int1_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_drdy_on_int1_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_fifo_threshold_on_int2_set(iis3dhhc_ctx_t *ctx,
                                            uint8_t val);
int32_t iis3dhhc_fifo_threshold_on_int2_get(iis3dhhc_ctx_t *ctx,
                                            uint8_t *val);

int32_t iis3dhhc_fifo_full_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_fifo_full_on_int2_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_fifo_ovr_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_fifo_ovr_on_int2_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_boot_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_boot_on_int2_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_drdy_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_drdy_on_int2_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS3DHHC_ALL_PUSH_PULL    = 0,
  IIS3DHHC_INT1_OD_INT2_PP  = 1,
  IIS3DHHC_INT1_PP_INT2_OD  = 2,
  IIS3DHHC_ALL_OPEN_DRAIN   = 3,
} iis3dhhc_pp_od_t;
int32_t iis3dhhc_pin_mode_set(iis3dhhc_ctx_t *ctx, iis3dhhc_pp_od_t val);
int32_t iis3dhhc_pin_mode_get(iis3dhhc_ctx_t *ctx, iis3dhhc_pp_od_t *val);

int32_t iis3dhhc_fifo_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_fifo_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_fifo_block_spi_hs_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_fifo_block_spi_hs_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_fifo_watermark_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_fifo_watermark_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

typedef enum {
  IIS3DHHC_BYPASS_MODE             = 0,
  IIS3DHHC_FIFO_MODE               = 1,
  IIS3DHHC_STREAM_TO_FIFO_MODE    = 3,
  IIS3DHHC_BYPASS_TO_STREAM_MODE  = 4,
  IIS3DHHC_DYNAMIC_STREAM_MODE    = 6,
} iis3dhhc_fmode_t;
int32_t iis3dhhc_fifo_mode_set(iis3dhhc_ctx_t *ctx, iis3dhhc_fmode_t val);
int32_t iis3dhhc_fifo_mode_get(iis3dhhc_ctx_t *ctx, iis3dhhc_fmode_t *val);

int32_t iis3dhhc_fifo_status_get(iis3dhhc_ctx_t *ctx,
                                iis3dhhc_fifo_src_t *val);

int32_t iis3dhhc_fifo_full_flag_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_fifo_ovr_flag_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_fifo_fth_flag_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

int32_t iis3dhhc_auto_add_inc_set(iis3dhhc_ctx_t *ctx, uint8_t val);
int32_t iis3dhhc_auto_add_inc_get(iis3dhhc_ctx_t *ctx, uint8_t *val);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /*__IIS3DHHC_DRIVER__H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
