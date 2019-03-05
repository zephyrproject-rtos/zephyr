/*
 ******************************************************************************
 * @file    stts751_reg.c
 * @author  Sensors Software Solution Team
 * @brief   STTS751 driver file
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

#include "stts751_reg.h"

/**
  * @defgroup  STTS751
  * @brief     This file provides a set of functions needed to drive the
  *            stts751 enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup  STTS751_Interfaces_Functions
  * @brief     This section provide a set of functions used to read and
  *            write a generic register of the device.
  *            MANDATORY: return 0 -> no Error.
  * @{
  *
  */

/**
  * @brief  Read generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to read
  * @param  data  pointer to buffer that store the data read(ptr)
  * @param  len   number of consecutive register to read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_read_reg(stts751_ctx_t* ctx, uint8_t reg, uint8_t* data,
                          uint16_t len)
{
  int32_t ret;
  ret = ctx->read_reg(ctx->handle, reg, data, len);
  return ret;
}

/**
  * @brief  Write generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to write
  * @param  data  pointer to data to write in register reg(ptr)
  * @param  len   number of consecutive register to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_write_reg(stts751_ctx_t* ctx, uint8_t reg, uint8_t* data,
                           uint16_t len)
{
  int32_t ret;
  ret = ctx->write_reg(ctx->handle, reg, data, len);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    STTS751_Sensitivity
  * @brief       These functions convert raw-data into engineering units and
  *              vice-versa .
  * @{
  *
  */

float stts751_from_lsb_to_celsius(int16_t lsb)
{
  return ((float)lsb) / 256.0f;
}

/**
  * @}
  *
  */

/**
  * @defgroup STTS751_Sensitivity_Reverse
  * @brief    This conversion is useful but not need by the driver.
  *
  *           REMOVING this union you are compliant with:
  *           MISRA-C 2012 [Rule 10.8] -> " Explicit cast of composite
  *                                         expression "
  *
  * @{
  *
  */

int16_t stts751_from_celsius_to_lsb(float celsius)
{
  return (int16_t)(celsius * 256.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup  STTS751_Data_Generation
  * @brief     This section groups all the functions concerning
  *            data generation
  * @{
  *
  */

/**
  * @brief  Temperature sensor data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the sensor data rate
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_temp_data_rate_set(stts751_ctx_t *ctx, stts751_odr_t val)
{
  stts751_configuration_t configuration;
  stts751_conversion_rate_t conversion_rate;
  uint8_t dummy_value = 0xAA;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_CONVERSION_RATE,
                         (uint8_t*)&conversion_rate, 1);
  if (ret == 0) {
    conversion_rate.conv = (uint8_t)val & 0x0FU;
    ret = stts751_write_reg(ctx, STTS751_CONVERSION_RATE,
                            (uint8_t*)&conversion_rate, 1);
  }
  if (ret == 0) {
    ret = stts751_read_reg(ctx, STTS751_CONFIGURATION,
                           (uint8_t*)&configuration, 1);
  }
  if (ret == 0) {
    configuration.stop = ((uint8_t)val & 0x80U) >> 7;
    ret = stts751_write_reg(ctx, STTS751_CONFIGURATION,
                            (uint8_t*)&configuration, 1);
  }
  if ((ret == 0) && (val == STTS751_TEMP_ODR_ONE_SHOT)) {
    ret = stts751_write_reg(ctx, STTS751_ONE_SHOT, &dummy_value, 1);
  }
  return ret;
}

/**
  * @brief  Temperature sensor data rate selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the sensor data rate
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_temp_data_rate_get(stts751_ctx_t *ctx, stts751_odr_t *val)
{
  stts751_conversion_rate_t conversion_rate;
  stts751_configuration_t configuration;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_CONVERSION_RATE,
                         (uint8_t*)&conversion_rate, 1);
  if (ret == 0) {
    ret = stts751_read_reg(ctx, STTS751_CONFIGURATION,
                           (uint8_t*)&configuration, 1);
  }
  switch ( (configuration.stop << 7) + conversion_rate.conv) {
    case STTS751_TEMP_ODR_OFF:
      *val = STTS751_TEMP_ODR_OFF;
      break;
    case STTS751_TEMP_ODR_ONE_SHOT:
      *val = STTS751_TEMP_ODR_ONE_SHOT;
      break;
    case STTS751_TEMP_ODR_62mHz5:
      *val = STTS751_TEMP_ODR_62mHz5;
      break;
    case STTS751_TEMP_ODR_125mHz:
      *val = STTS751_TEMP_ODR_125mHz;
      break;
     case STTS751_TEMP_ODR_250mHz:
      *val = STTS751_TEMP_ODR_250mHz;
      break;
    case STTS751_TEMP_ODR_500mHz:
      *val = STTS751_TEMP_ODR_500mHz;
      break;
    case STTS751_TEMP_ODR_1Hz:
      *val = STTS751_TEMP_ODR_1Hz;
      break;
    case STTS751_TEMP_ODR_2Hz:
      *val = STTS751_TEMP_ODR_2Hz;
      break;
     case STTS751_TEMP_ODR_4Hz:
      *val = STTS751_TEMP_ODR_4Hz;
      break;
    case STTS751_TEMP_ODR_8Hz:
      *val = STTS751_TEMP_ODR_8Hz;
      break;
    case STTS751_TEMP_ODR_16Hz:
      *val = STTS751_TEMP_ODR_16Hz;
      break;
    case STTS751_TEMP_ODR_32Hz:
      *val = STTS751_TEMP_ODR_32Hz;
      break;
    default:
      *val = STTS751_TEMP_ODR_OFF;
      break;
  }
  return ret;
}

/**
  * @brief  Temperature sensor resolution selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tres in reg CONFIGURATION
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_resolution_set(stts751_ctx_t *ctx, stts751_tres_t val)
{
  stts751_configuration_t reg;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_CONFIGURATION,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.tres = (uint8_t) val;
    ret = stts751_write_reg(ctx, STTS751_CONFIGURATION,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief Temperature sensor resolution selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of tres in reg CONFIGURATION
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_resolution_get(stts751_ctx_t *ctx, stts751_tres_t *val)
{
  stts751_configuration_t reg;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_CONFIGURATION,(uint8_t*) &reg, 1);

  switch (reg.tres) {
    case STTS751_9bit:
      *val = STTS751_9bit;
      break;
    case STTS751_10bit:
      *val = STTS751_10bit;
      break;
    case STTS751_11bit:
      *val = STTS751_11bit;
      break;
    case STTS751_12bit:
      *val = STTS751_12bit;
      break;
    default:
      *val = STTS751_9bit;
      break;
  }
  return ret;
}

/**
  * @brief  The STATUS_REG register of the device.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      union of registers from STATUS to
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_status_reg_get(stts751_ctx_t *ctx, stts751_status_t *val)
{
  int32_t ret;
  ret = stts751_read_reg(ctx, STTS751_STATUS, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Temperature sensor "conversion on-going" flag.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      get the values of busy in reg STATUS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_flag_busy_get(stts751_ctx_t *ctx, uint8_t *val)
{
  stts751_status_t reg;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_STATUS, (uint8_t*)&reg, 1);
  *val = reg.busy;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  STTS751_Data_Output
  * @brief     This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief  Temperature data output register (r). L and H registers
  *         together express a 16-bit word in two’s complement.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_temperature_raw_get(stts751_ctx_t *ctx, int16_t *buff)
{
  uint16_t temperature;
  uint8_t temperature_low;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_TEMPERATURE_HIGH,
                         (uint8_t*)&temperature, 1);
  if (ret == 0) {
    ret = stts751_read_reg(ctx, STTS751_TEMPERATURE_LOW,
                           &temperature_low, 1);

    temperature  = (temperature << 8) + temperature_low;
    *buff = (int16_t)temperature;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  STTS751_Interrupt_Pins
  * @brief     This section groups all the functions that manage event pin
  * @{
  *
  */

/**
  * @brief  Route interrupt signal threshold on event pad.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      set mask1 bit in register CONFIGURATION.
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_pin_event_route_set(stts751_ctx_t *ctx, uint8_t val)
{
  stts751_configuration_t reg;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_CONFIGURATION,(uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.mask1 = val;
    ret = stts751_write_reg(ctx, STTS751_CONFIGURATION, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Route interrupt signal threshold on event pad.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      get mask1 bit in register CONFIGURATION.
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_pin_event_route_get(stts751_ctx_t *ctx, uint8_t *val)
{
  stts751_configuration_t reg;
  int32_t ret;
  ret = stts751_read_reg(ctx, STTS751_CONFIGURATION, (uint8_t*)&reg, 1);
  *val = reg.mask1;
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  STTS751_Interrupt_on_threshold
  * @brief     This section groups all the functions that manage interrupt
  *            on threshold event
  * @{
  *
  */

/**
  * @brief  high temperature theshold.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_high_temperature_threshold_set(stts751_ctx_t *ctx,
                                               int16_t buff)
{
  uint8_t *temperature_ptr;
  int32_t ret;

  temperature_ptr = (uint8_t*)&buff;
  ret = stts751_write_reg(ctx, STTS751_TEMPERATURE_HIGH_LIMIT_LOW,
                          (uint8_t*)temperature_ptr, 1);

  if (ret == 0) {
    temperature_ptr++;
    ret = stts751_write_reg(ctx, STTS751_TEMPERATURE_HIGH_LIMIT_HIGH,
                            (uint8_t*)temperature_ptr, 1);
  }

  return ret;
}

/**
  * @brief  high temperature theshold.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_high_temperature_threshold_get(stts751_ctx_t *ctx,
                                               int16_t *buff)
{
  uint16_t temperature;
  uint8_t temperature_low;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_TEMPERATURE_HIGH_LIMIT_HIGH,
                         (uint8_t*)&temperature, 1);
  if (ret == 0) {
    ret = stts751_read_reg(ctx, STTS751_TEMPERATURE_HIGH_LIMIT_LOW,
                           &temperature_low, 1);

    temperature  = (temperature << 8) + temperature_low;
    *buff = (int16_t)temperature;
  }
  return ret;
}

/**
  * @brief  low temperature theshold.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_low_temperature_threshold_set(stts751_ctx_t *ctx,
                                              int16_t buff)
{

  uint8_t *temperature_ptr;
  int32_t ret;

  temperature_ptr = (uint8_t*)&buff;
  ret = stts751_write_reg(ctx, STTS751_TEMPERATURE_LOW_LIMIT_LOW,
                          (uint8_t*)temperature_ptr, 1);

  if (ret == 0) {
    temperature_ptr++;
    ret = stts751_write_reg(ctx, STTS751_TEMPERATURE_LOW_LIMIT_HIGH,
                            (uint8_t*)temperature_ptr, 1);
  }

  return ret;
}

/**
  * @brief  low temperature theshold.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_low_temperature_threshold_get(stts751_ctx_t *ctx,
                                              int16_t *buff)
{
  uint16_t temperature;
  uint8_t temperature_low;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_TEMPERATURE_LOW_LIMIT_HIGH,
                         (uint8_t*)&temperature, 1);
  if (ret == 0) {
    ret = stts751_read_reg(ctx, STTS751_TEMPERATURE_LOW_LIMIT_LOW,
                           &temperature_low, 1);

    temperature  = (temperature << 8) + temperature_low;
    *buff = (int16_t)temperature;
  }

  return ret;
}

/**
  * @}
  *
  */


  /**
  * @defgroup  STTS751 over temperature alarm
  * @brief     This section groups all the functions that manage
  *            over temperature alarm functionality.
  * @{
  *
  */

/**
  * @brief  Thermal Limit. 1 LSB = 1 degC (max 127 degC min -127 degC ).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reg THERM_LIMIT
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_ota_thermal_limit_set(stts751_ctx_t *ctx, int8_t val)
{
  int32_t ret;
  ret = stts751_write_reg(ctx, STTS751_THERM_LIMIT, (uint8_t*)&val, 1);
  return ret;
}

/**
  * @brief  Thermal Limit. 1 LSB = 1 degC (max 127 degC min -127 degC ).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      get the values of reg THERM_LIMIT
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_ota_thermal_limit_get(stts751_ctx_t *ctx, int8_t *val)
{
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_THERM_LIMIT, (uint8_t*)val, 1);
  return ret;
}

/**
  * @brief  Thermal hysteresis. 1 LSB = 1 degC.[set]
  *         max 127 degC min -127 degC.
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reg THERM_HYSTERESIS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_ota_thermal_hyst_set(stts751_ctx_t *ctx, int8_t val)
{
  int32_t ret;

  ret = stts751_write_reg(ctx, STTS751_THERM_HYSTERESIS, (uint8_t*)&val, 1);
  return ret;
}

/**
  * @brief  Thermal hysteresis. 1 LSB = 1 degC.[get]
  *         max 127 degC min -127 degC.
  *
  * @param  ctx      read / write interface definitions
  * @param  val      get the values of reg THERM_HYSTERESIS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_ota_thermal_hyst_get(stts751_ctx_t *ctx, int8_t *val)
{
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_THERM_HYSTERESIS, (uint8_t*)val, 1);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  STTS751_Common
  * @brief     This section groups common useful functions.
  * @{
  *
  */

/**
  * @brief  SMBus timeout.At power-up, the STTS751 is configured with an
  *         SMBus timeout of 25 to 35 milliseconds.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      set timeout bit in register SMBUS_TIMEOUT.
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_smbus_timeout_set(stts751_ctx_t *ctx, uint8_t val)
{
  stts751_smbus_timeout_t reg;
  int32_t ret;

  ret = stts751_read_reg(ctx, STTS751_SMBUS_TIMEOUT,(uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.timeout = val;
    ret = stts751_write_reg(ctx, STTS751_SMBUS_TIMEOUT, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  SMBus timeout.At power-up, the STTS751 is configured with an
  *         SMBus timeout of 25 to 35 milliseconds.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      get timeout bit in register SMBUS_TIMEOUT.
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_smbus_timeout_get(stts751_ctx_t *ctx, uint8_t *val)
{
  stts751_smbus_timeout_t reg;
  int32_t ret;
  ret = stts751_read_reg(ctx, STTS751_SMBUS_TIMEOUT, (uint8_t*)&reg, 1);
  *val = reg.timeout;
  return ret;
}

/**
  * @brief  Device Who am I.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t stts751_device_id_get(stts751_ctx_t *ctx, stts751_id_t *buff)
{
  int32_t ret;
  ret = stts751_read_reg(ctx, STTS751_PRODUCT_ID,
                         (uint8_t*)&buff->product_id, 1);
  if (ret == 0){
  ret = stts751_read_reg(ctx, STTS751_MANUFACTURER_ID,
                         (uint8_t*)&buff->manufacturer_id, 1);
  }
  if (ret == 0){
  ret = stts751_read_reg(ctx, STTS751_REVISION_ID,
                         (uint8_t*)&buff->revision_id, 1);
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/