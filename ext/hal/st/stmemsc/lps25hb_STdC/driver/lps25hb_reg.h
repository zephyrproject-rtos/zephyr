/*
 ******************************************************************************
 * @file    lps25hb_reg.h
 * @author  MEMS Software Solution Team
 * @date    20-September-2017
 * @brief   This file contains all the functions prototypes for the
 *          lps25hb_reg.c driver.
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
#ifndef __LPS25HB_DRIVER__H
#define __LPS25HB_DRIVER__H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup LPS25HB
  * @{
  *
  */

/** @defgroup LPS25HB_sensors_common_types
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

/** @defgroup lps25hb_interface
  * @{
  */

typedef int32_t (*lps25hb_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*lps25hb_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  lps25hb_write_ptr  write_reg;
  lps25hb_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} lps25hb_ctx_t;

/**
  * @}
  */


/** @defgroup lps25hb_Infos
  * @{
  */
  /** I2C Device Address 8 bit format if SA0=0 -> B9 if SA0=1 -> BB **/
#define LPS25HB_I2C_ADD_L    0xB9
#define LPS25HB_I2C_ADD_H    0xBB

/** Device Identification (Who am I) **/
#define LPS25HB_ID             0xBD

/**
  * @}
  */

/**
  * @defgroup lps25hb_Sensitivity
  * @{
  */

#define LPS25HB_FROM_LSB_TO_hPa(lsb)      (float)( lsb / 4096.0f )
#define LPS25HB_FROM_LSB_TO_degC(lsb)    ((float)( lsb / 480.0f ) + 42.5f )

/**
  * @}
  */

#define LPS25HB_REF_P_XL        0x08
#define LPS25HB_REF_P_L         0x09
#define LPS25HB_REF_P_H         0x0A
#define LPS25HB_WHO_AM_I        0x0F
#define LPS25HB_RES_CONF        0x10
typedef struct {
  uint8_t avgp             : 2;
  uint8_t avgt             : 2;
  uint8_t not_used_01      : 4;
} lps25hb_res_conf_t;

#define LPS25HB_CTRL_REG1       0x20
typedef struct {
  uint8_t sim              : 1;
  uint8_t reset_az         : 1;
  uint8_t bdu              : 1;
  uint8_t diff_en          : 1;
  uint8_t odr              : 4; /* pd + odr -> odr */
} lps25hb_ctrl_reg1_t;

#define LPS25HB_CTRL_REG2       0x21
typedef struct {
  uint8_t one_shot         : 1;
  uint8_t autozero         : 1;
  uint8_t swreset          : 1;
  uint8_t i2c_dis          : 1;
  uint8_t fifo_mean_dec    : 1;
  uint8_t stop_on_fth      : 1;
  uint8_t fifo_en          : 1;
  uint8_t boot             : 1;
} lps25hb_ctrl_reg2_t;

#define LPS25HB_CTRL_REG3       0x22
typedef struct {
  uint8_t int_s           : 2;
  uint8_t not_used_01     : 4;
  uint8_t pp_od           : 1;
  uint8_t int_h_l         : 1;
} lps25hb_ctrl_reg3_t;

#define LPS25HB_CTRL_REG4       0x23
typedef struct {
  uint8_t drdy            : 1;
  uint8_t f_ovr           : 1;
  uint8_t f_fth           : 1;
  uint8_t f_empty         : 1;
  uint8_t not_used_01     : 4;
} lps25hb_ctrl_reg4_t;

#define LPS25HB_INTERRUPT_CFG   0x24
typedef struct {
  uint8_t pe              : 2;  /* pl_e + ph_e -> pe */
  uint8_t lir             : 1;
  uint8_t not_used_01     : 5;
} lps25hb_interrupt_cfg_t;

#define LPS25HB_INT_SOURCE      0x25
typedef struct {
  uint8_t ph              : 1;
  uint8_t pl              : 1;
  uint8_t ia              : 1;
  uint8_t not_used_01     : 5;
} lps25hb_int_source_t;

#define LPS25HB_STATUS_REG      0x27
typedef struct {
  uint8_t t_da            : 1;
  uint8_t p_da            : 1;
  uint8_t not_used_01     : 2;
  uint8_t t_or            : 1;
  uint8_t p_or            : 1;
  uint8_t not_used_02     : 2;
} lps25hb_status_reg_t;

#define LPS25HB_PRESS_OUT_XL    0x28
#define LPS25HB_PRESS_OUT_L     0x29
#define LPS25HB_PRESS_OUT_H     0x2A
#define LPS25HB_TEMP_OUT_L      0x2B
#define LPS25HB_TEMP_OUT_H      0x2C
#define LPS25HB_FIFO_CTRL       0x2E
typedef struct {
  uint8_t wtm_point       : 5;
  uint8_t f_mode          : 3;
} lps25hb_fifo_ctrl_t;

#define LPS25HB_FIFO_STATUS     0x2F
typedef struct {
  uint8_t fss             : 5;
  uint8_t empty_fifo      : 1;
  uint8_t ovr             : 1;
  uint8_t fth_fifo        : 1;
} lps25hb_fifo_status_t;

#define LPS25HB_THS_P_L         0x30
#define LPS25HB_THS_P_H         0x31
#define LPS25HB_RPDS_L          0x39
#define LPS25HB_RPDS_H          0x3A

typedef union{
  lps25hb_res_conf_t            res_conf;
  lps25hb_ctrl_reg1_t           ctrl_reg1;
  lps25hb_ctrl_reg2_t           ctrl_reg2;
  lps25hb_ctrl_reg3_t           ctrl_reg3;
  lps25hb_ctrl_reg4_t           ctrl_reg4;
  lps25hb_interrupt_cfg_t       interrupt_cfg;
  lps25hb_int_source_t          int_source;
  lps25hb_status_reg_t          status_reg;
  lps25hb_fifo_ctrl_t           fifo_ctrl;
  lps25hb_fifo_status_t         fifo_status;
  bitwise_t                     bitwise;
  uint8_t                       byte;
} lps25hb_reg_t;

int32_t lps25hb_read_reg(lps25hb_ctx_t *ctx, uint8_t reg, uint8_t* data, uint16_t len);
int32_t lps25hb_write_reg(lps25hb_ctx_t *ctx, uint8_t reg, uint8_t* data, uint16_t len);

int32_t lps25hb_pressure_ref_set(lps25hb_ctx_t *ctx, uint8_t *buff);
int32_t lps25hb_pressure_ref_get(lps25hb_ctx_t *ctx, uint8_t *buff);

typedef enum {
  LPS25HB_P_AVG_8  = 0,
  LPS25HB_P_AVG_16 = 1,
  LPS25HB_P_AVG_32 = 2,
  LPS25HB_P_AVG_64 = 3,
} lps25hb_avgp_t;
int32_t lps25hb_pressure_avg_set(lps25hb_ctx_t *ctx, lps25hb_avgp_t val);
int32_t lps25hb_pressure_avg_get(lps25hb_ctx_t *ctx, lps25hb_avgp_t *val);

typedef enum {
  LPS25HB_T_AVG_8  = 0,
  LPS25HB_T_AVG_16 = 1,
  LPS25HB_T_AVG_32 = 2,
  LPS25HB_T_AVG_64 = 3,
} lps25hb_avgt_t;
int32_t lps25hb_temperature_avg_set(lps25hb_ctx_t *ctx, lps25hb_avgt_t val);
int32_t lps25hb_temperature_avg_get(lps25hb_ctx_t *ctx, lps25hb_avgt_t *val);

int32_t lps25hb_autozero_rst_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_autozero_rst_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_block_data_update_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_block_data_update_get(lps25hb_ctx_t *ctx, uint8_t *val);

typedef enum {
  LPS25HB_POWER_DOWN = 0,
  LPS25HB_ODR_1Hz    = 9,
  LPS25HB_ODR_7Hz    = 11,
  LPS25HB_ODR_12Hz5  = 12,
  LPS25HB_ODR_25Hz   = 13,
  LPS25HB_ONE_SHOT   = 8,
} lps25hb_odr_t;
int32_t lps25hb_data_rate_set(lps25hb_ctx_t *ctx, lps25hb_odr_t val);
int32_t lps25hb_data_rate_get(lps25hb_ctx_t *ctx, lps25hb_odr_t *val);

int32_t lps25hb_one_shoot_trigger_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_one_shoot_trigger_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_autozero_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_autozero_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_mean_decimator_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_fifo_mean_decimator_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_press_data_ready_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_temp_data_ready_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_temp_data_ovr_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_press_data_ovr_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_pressure_raw_get(lps25hb_ctx_t *ctx, uint8_t *buff);

int32_t lps25hb_temperature_raw_get(lps25hb_ctx_t *ctx, uint8_t *buff);

int32_t lps25hb_pressure_offset_set(lps25hb_ctx_t *ctx, uint8_t *buff);
int32_t lps25hb_pressure_offset_get(lps25hb_ctx_t *ctx, uint8_t *buff);

int32_t lps25hb_device_id_get(lps25hb_ctx_t *ctx, uint8_t *buff);

int32_t lps25hb_reset_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_reset_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_boot_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_boot_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_status_get(lps25hb_ctx_t *ctx, lps25hb_status_reg_t *val);

int32_t lps25hb_int_generation_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_int_generation_get(lps25hb_ctx_t *ctx, uint8_t *val);

typedef enum {
  LPS25HB_DRDY_OR_FIFO_FLAGS  = 0,
  LPS25HB_HIGH_PRES_INT       = 1,
  LPS25HB_LOW_PRES_INT        = 2,
  LPS25HB_EVERY_PRES_INT      = 3,
} lps25hb_int_s_t;
int32_t lps25hb_int_pin_mode_set(lps25hb_ctx_t *ctx, lps25hb_int_s_t val);
int32_t lps25hb_int_pin_mode_get(lps25hb_ctx_t *ctx, lps25hb_int_s_t *val);

typedef enum {
  LPS25HB_PUSH_PULL   = 0,
  LPS25HB_OPEN_DRAIN  = 1,
} lps25hb_pp_od_t;
int32_t lps25hb_pin_mode_set(lps25hb_ctx_t *ctx, lps25hb_pp_od_t val);
int32_t lps25hb_pin_mode_get(lps25hb_ctx_t *ctx, lps25hb_pp_od_t *val);

typedef enum {
  LPS25HB_ACTIVE_HIGH = 0,
  LPS25HB_ACTIVE_LOW  = 1,
} lps25hb_int_h_l_t;
int32_t lps25hb_int_polarity_set(lps25hb_ctx_t *ctx, lps25hb_int_h_l_t val);
int32_t lps25hb_int_polarity_get(lps25hb_ctx_t *ctx, lps25hb_int_h_l_t *val);

int32_t lps25hb_drdy_on_int_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_drdy_on_int_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_ovr_on_int_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_fifo_ovr_on_int_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_threshold_on_int_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_fifo_threshold_on_int_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_empty_on_int_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_fifo_empty_on_int_get(lps25hb_ctx_t *ctx, uint8_t *val);

typedef enum {
  LPS25HB_NO_THRESHOLD = 0,
  LPS25HB_POSITIVE     = 1,
  LPS25HB_NEGATIVE     = 2,
  LPS25HB_BOTH         = 3,
} lps25hb_pe_t;
int32_t lps25hb_sign_of_int_threshold_set(lps25hb_ctx_t *ctx, lps25hb_pe_t val);
int32_t lps25hb_sign_of_int_threshold_get(lps25hb_ctx_t *ctx, lps25hb_pe_t *val);

typedef enum {
  LPS25HB_INT_PULSED = 0,
  LPS25HB_INT_LATCHED = 1,
} lps25hb_lir_t;
int32_t lps25hb_int_notification_mode_set(lps25hb_ctx_t *ctx, lps25hb_lir_t val);
int32_t lps25hb_int_notification_mode_get(lps25hb_ctx_t *ctx, lps25hb_lir_t *val);

int32_t lps25hb_int_source_get(lps25hb_ctx_t *ctx, lps25hb_int_source_t *val);

int32_t lps25hb_int_on_press_high_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_int_on_press_low_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_interrupt_event_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_int_threshold_set(lps25hb_ctx_t *ctx, uint8_t *buff);
int32_t lps25hb_int_threshold_get(lps25hb_ctx_t *ctx, uint8_t *buff);

int32_t lps25hb_stop_on_fifo_threshold_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_stop_on_fifo_threshold_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_fifo_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_watermark_set(lps25hb_ctx_t *ctx, uint8_t val);
int32_t lps25hb_fifo_watermark_get(lps25hb_ctx_t *ctx, uint8_t *val);

typedef enum {
  LPS25HB_BYPASS_MODE               = 0,
  LPS25HB_FIFO_MODE                 = 1,
  LPS25HB_STREAM_MODE               = 2,
  LPS25HB_Stream_to_FIFO_mode      = 3,
  LPS25HB_BYPASS_TO_STREAM_MODE    = 4,
  LPS25HB_MEAN_MODE                 = 6,
  LPS25HB_BYPASS_TO_FIFO_MODE      = 7,
} lps25hb_f_mode_t;
int32_t lps25hb_fifo_mode_set(lps25hb_ctx_t *ctx, lps25hb_f_mode_t val);
int32_t lps25hb_fifo_mode_get(lps25hb_ctx_t *ctx, lps25hb_f_mode_t *val);

int32_t lps25hb_fifo_status_get(lps25hb_ctx_t *ctx, lps25hb_fifo_status_t *val);

int32_t lps25hb_fifo_data_level_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_empty_flag_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_ovr_flag_get(lps25hb_ctx_t *ctx, uint8_t *val);

int32_t lps25hb_fifo_fth_flag_get(lps25hb_ctx_t *ctx, uint8_t *val);

typedef enum {
  LPS25HB_SPI_4_WIRE = 0,
  LPS25HB_SPI_3_WIRE = 1,
} lps25hb_sim_t;
int32_t lps25hb_spi_mode_set(lps25hb_ctx_t *ctx, lps25hb_sim_t val);
int32_t lps25hb_spi_mode_get(lps25hb_ctx_t *ctx, lps25hb_sim_t *val);

typedef enum {
  LPS25HB_I2C_ENABLE  = 0,
  LPS25HB_I2C_DISABLE = 1,
} lps25hb_i2c_dis_t;
int32_t lps25hb_i2c_interface_set(lps25hb_ctx_t *ctx, lps25hb_i2c_dis_t val);
int32_t lps25hb_i2c_interface_get(lps25hb_ctx_t *ctx, lps25hb_i2c_dis_t *val);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /*__LPS25HB_DRIVER__H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
