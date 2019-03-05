/*
 ******************************************************************************
 * @file    stts751_reg.h
 * @author  Sensors Software Solution Team
 * @brief   This file contains all the functions prototypes for the
 *          stts751_reg.c driver.
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
#ifndef STTS751_REGS_H
#define STTS751_REGS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <math.h>

/** @addtogroup STTS751
  * @{
  *
  */

/** @defgroup STTS751_sensors_common_types
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

/** @addtogroup  STTS751_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
  * @{
  *
  */

typedef int32_t (*stts751_write_ptr)(void *, uint8_t, uint8_t*, uint16_t);
typedef int32_t (*stts751_read_ptr) (void *, uint8_t, uint8_t*, uint16_t);

typedef struct {
  /** Component mandatory fields **/
  stts751_write_ptr  write_reg;
  stts751_read_ptr   read_reg;
  /** Customizable optional pointer **/
  void *handle;
} stts751_ctx_t;

/**
  * @}
  *
  */

/** @defgroup STTS751_Infos
  * @{
  *
  */

/** I2C Device Address 8 bit format **/
#define STTS751_0xxxx_ADD_7K5  0x91U
#define STTS751_0xxxx_ADD_12K  0x93U
#define STTS751_0xxxx_ADD_20K  0x71U
#define STTS751_0xxxx_ADD_33K  0x73U

#define STTS751_1xxxx_ADD_7K5  0x95U
#define STTS751_1xxxx_ADD_12K  0x97U
#define STTS751_1xxxx_ADD_20K  0x75U
#define STTS751_1xxxx_ADD_33K  0x77U

/** Device Identification **/
/* Product ID */
#define STTS751_ID_0xxxx       0x00U
#define STTS751_ID_1xxxx       0x01U
/* Manufacturer ID */
#define STTS751_ID_MAN         0x53U
/* Revision number */
#define STTS751_REV            0x01U

/**
  * @}
  *
  */

#define STTS751_TEMPERATURE_HIGH            0x00U
#define STTS751_STATUS                      0x01U
typedef struct {
  uint8_t thrm                       : 1;
  uint8_t not_used_01                : 4;
  uint8_t t_low                      : 1;
  uint8_t t_high                     : 1;
  uint8_t busy                       : 1;
} stts751_status_t;

#define STTS751_TEMPERATURE_LOW             0x02U
#define STTS751_CONFIGURATION               0x03U
typedef struct {
  uint8_t not_used_01                : 2;
  uint8_t tres                       : 2;
  uint8_t not_used_02                : 2;
  uint8_t stop                       : 1;
  uint8_t mask1                      : 1;
} stts751_configuration_t;

#define STTS751_CONVERSION_RATE             0x04U
typedef struct {
  uint8_t conv                       : 4;
  uint8_t not_used_01                : 4;
} stts751_conversion_rate_t;

#define STTS751_TEMPERATURE_HIGH_LIMIT_HIGH 0x05U
#define STTS751_TEMPERATURE_HIGH_LIMIT_LOW  0x06U
#define STTS751_TEMPERATURE_LOW_LIMIT_HIGH  0x07U
#define STTS751_TEMPERATURE_LOW_LIMIT_LOW   0x08U
#define STTS751_ONE_SHOT                    0x0FU
#define STTS751_THERM_LIMIT                 0x20U
#define STTS751_THERM_HYSTERESIS            0x21U
#define STTS751_SMBUS_TIMEOUT               0x22U
typedef struct {
  uint8_t not_used_01                : 7;
  uint8_t timeout                    : 1;
} stts751_smbus_timeout_t;

#define STTS751_PRODUCT_ID                  0xFDU
#define STTS751_MANUFACTURER_ID             0xFEU
#define STTS751_REVISION_ID                 0xFFU

/**
  * @defgroup STTS751_Register_Union
  * @brief    This union group all the registers that has a bitfield
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
  stts751_status_t                       status;
  stts751_configuration_t                configuration;
  stts751_conversion_rate_t              conversion_rate;
  stts751_smbus_timeout_t                smbus_timeout;
  bitwise_t                              bitwise;
  uint8_t                                byte;
} stts751_reg_t;

/**
  * @}
  *
  */

int32_t stts751_read_reg(stts751_ctx_t *ctx, uint8_t reg, uint8_t* data,
                          uint16_t len);
int32_t stts751_write_reg(stts751_ctx_t *ctx, uint8_t reg, uint8_t* data,
                           uint16_t len);

extern float stts751_from_lsb_to_celsius(int16_t lsb);
extern int16_t stts751_from_celsius_to_lsb(float celsius);

typedef enum {
  STTS751_TEMP_ODR_OFF        = 0x80,
  STTS751_TEMP_ODR_ONE_SHOT   = 0x90,
  STTS751_TEMP_ODR_62mHz5     = 0x00,
  STTS751_TEMP_ODR_125mHz     = 0x01,
  STTS751_TEMP_ODR_250mHz     = 0x02,
  STTS751_TEMP_ODR_500mHz     = 0x03,
  STTS751_TEMP_ODR_1Hz        = 0x04,
  STTS751_TEMP_ODR_2Hz        = 0x05,
  STTS751_TEMP_ODR_4Hz        = 0x06,
  STTS751_TEMP_ODR_8Hz        = 0x07,
  STTS751_TEMP_ODR_16Hz       = 0x08, /* 9, 10, or 11-bit resolutions only */
  STTS751_TEMP_ODR_32Hz       = 0x09, /* 9 or 10-bit resolutions only */
} stts751_odr_t;
int32_t stts751_temp_data_rate_set(stts751_ctx_t *ctx, stts751_odr_t val);
int32_t stts751_temp_data_rate_get(stts751_ctx_t *ctx, stts751_odr_t *val);

typedef enum {
  STTS751_9bit      = 2,
  STTS751_10bit     = 0,
  STTS751_11bit     = 1,
  STTS751_12bit     = 3,
} stts751_tres_t;
int32_t stts751_resolution_set(stts751_ctx_t *ctx, stts751_tres_t val);
int32_t stts751_resolution_get(stts751_ctx_t *ctx, stts751_tres_t *val);

int32_t stts751_status_reg_get(stts751_ctx_t *ctx, stts751_status_t *val);

int32_t stts751_flag_busy_get(stts751_ctx_t *ctx, uint8_t *val);

int32_t stts751_temperature_raw_get(stts751_ctx_t *ctx, int16_t *buff);

int32_t stts751_pin_event_route_set(stts751_ctx_t *ctx, uint8_t val);
int32_t stts751_pin_event_route_get(stts751_ctx_t *ctx, uint8_t *val);


int32_t stts751_high_temperature_threshold_set(stts751_ctx_t *ctx,
                                               int16_t buff);
int32_t stts751_high_temperature_threshold_get(stts751_ctx_t *ctx,
                                               int16_t *buff);

int32_t stts751_low_temperature_threshold_set(stts751_ctx_t *ctx,
                                              int16_t buff);
int32_t stts751_low_temperature_threshold_get(stts751_ctx_t *ctx,
                                              int16_t *buff);

int32_t stts751_ota_thermal_limit_set(stts751_ctx_t *ctx, int8_t val);
int32_t stts751_ota_thermal_limit_get(stts751_ctx_t *ctx, int8_t *val);

int32_t stts751_ota_thermal_hyst_set(stts751_ctx_t *ctx, int8_t val);
int32_t stts751_ota_thermal_hyst_get(stts751_ctx_t *ctx, int8_t *val);

int32_t stts751_smbus_timeout_set(stts751_ctx_t *ctx, uint8_t val);
int32_t stts751_smbus_timeout_get(stts751_ctx_t *ctx, uint8_t *val);

typedef struct {
  uint8_t product_id;
  uint8_t manufacturer_id;
  uint8_t revision_id;
} stts751_id_t;
int32_t stts751_device_id_get(stts751_ctx_t *ctx, stts751_id_t *buff);

/**
  * @}
  *
  */

#ifdef __cplusplus
}
#endif

#endif /*STTS751_REGS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
