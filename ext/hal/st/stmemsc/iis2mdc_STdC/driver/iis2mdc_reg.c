/*
 ******************************************************************************
 * @file    iis2mdc_reg.c
 * @author  Sensors Software Solution Team
 * @brief   IIS2MDC driver file
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

#include "iis2mdc_reg.h"
/**
  * @defgroup  IIS2MDC
  * @brief     This file provides a set of functions needed to drive the
  *            iis2mdc enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup  IIS2MDC_Interfaces_Functions
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
int32_t iis2mdc_read_reg(iis2mdc_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t iis2mdc_write_reg(iis2mdc_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    IIS2MDC_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float iis2mdc_from_lsb_to_mgauss(int16_t lsb)
{
  return ((float)lsb) * 1.5f;
}

float iis2mdc_from_lsb_to_celsius(int16_t lsb)
{
  return (((float)lsb / 8.0f) + 25.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup   IIS2MDC_data_generation.
  * @brief      This section group all the functions concerning
  *             data generation.
  * @{
  *
  */

/**
  * @brief  These registers comprise a 3 group of 16-bit number and represent
  *         hard-iron offset in order to compensate environmental effects.
  *         Data format is the same of output data raw: two’s complement with
  *         1LSb = 1.5mG.
  *         These values act on the magnetic output data value in order to
  *         delete the environmental offset.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_mag_user_offset_set(iis2mdc_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  iis2mdc_write_reg(ctx, IIS2MDC_OFFSET_X_REG_L, buff, 6);
  return ret;
}

/**
  * @brief  These registers comprise a 3 group of 16-bit number and represent
  *         hard-iron offset in order to compensate environmental effects.
  *         Data format is the same of output data raw: two’s complement with
  *         1LSb = 1.5mG.
  *         These values act on the magnetic output data value in order to
  *         delete the environmental offset.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_mag_user_offset_get(iis2mdc_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  iis2mdc_read_reg(ctx, IIS2MDC_OFFSET_X_REG_L, buff, 6);
  return ret;
}

/**
  * @brief  Operating mode selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of md in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_operating_mode_set(iis2mdc_ctx_t *ctx, iis2mdc_md_t val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.md = (uint8_t)val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Operating mode selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of md in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_operating_mode_get(iis2mdc_ctx_t *ctx, iis2mdc_md_t *val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  switch (reg.md) {
    case IIS2MDC_CONTINUOUS_MODE:
      *val = IIS2MDC_CONTINUOUS_MODE;
      break;
    case IIS2MDC_SINGLE_TRIGGER:
      *val = IIS2MDC_SINGLE_TRIGGER;
      break;
    case IIS2MDC_POWER_DOWN:
      *val = IIS2MDC_POWER_DOWN;
      break;
    default:
      *val = IIS2MDC_CONTINUOUS_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  Output data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odr in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_data_rate_set(iis2mdc_ctx_t *ctx, iis2mdc_odr_t val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.odr = (uint8_t)val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Output data rate selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of odr in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_data_rate_get(iis2mdc_ctx_t *ctx, iis2mdc_odr_t *val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  switch (reg.odr) {
    case IIS2MDC_ODR_10Hz:
      *val = IIS2MDC_ODR_10Hz;
      break;
    case IIS2MDC_ODR_20Hz:
      *val = IIS2MDC_ODR_20Hz;
      break;
    case IIS2MDC_ODR_50Hz:
      *val = IIS2MDC_ODR_50Hz;
      break;
    case IIS2MDC_ODR_100Hz:
      *val = IIS2MDC_ODR_100Hz;
      break;
    default:
      *val = IIS2MDC_ODR_10Hz;
      break;
  }
  return ret;
}

/**
  * @brief  Enables high-resolution/low-power mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lp in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_power_mode_set(iis2mdc_ctx_t *ctx, iis2mdc_lp_t val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.lp = (uint8_t)val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables high-resolution/low-power mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of lp in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_power_mode_get(iis2mdc_ctx_t *ctx, iis2mdc_lp_t *val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  switch (reg.lp) {
    case IIS2MDC_HIGH_RESOLUTION:
      *val = IIS2MDC_HIGH_RESOLUTION;
      break;
    case IIS2MDC_LOW_POWER:
      *val = IIS2MDC_LOW_POWER;
      break;
    default:
      *val = IIS2MDC_HIGH_RESOLUTION;
      break;
  }
  return ret;
}

/**
  * @brief  Enables the magnetometer temperature compensation.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of comp_temp_en in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_offset_temp_comp_set(iis2mdc_ctx_t *ctx, uint8_t val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.comp_temp_en = val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables the magnetometer temperature compensation.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of comp_temp_en in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_offset_temp_comp_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  *val = reg.comp_temp_en;

  return ret;
}

/**
  * @brief  Low-pass bandwidth selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lpf in reg CFG_REG_B
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_low_pass_bandwidth_set(iis2mdc_ctx_t *ctx,
                                       iis2mdc_lpf_t val)
{
  iis2mdc_cfg_reg_b_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.lpf = (uint8_t)val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Low-pass bandwidth selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of lpf in reg CFG_REG_B
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_low_pass_bandwidth_get(iis2mdc_ctx_t *ctx,
                                       iis2mdc_lpf_t *val)
{
  iis2mdc_cfg_reg_b_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  switch (reg.lpf) {
    case IIS2MDC_ODR_DIV_2:
      *val = IIS2MDC_ODR_DIV_2;
      break;
    case IIS2MDC_ODR_DIV_4:
      *val = IIS2MDC_ODR_DIV_4;
      break;
    default:
      *val = IIS2MDC_ODR_DIV_2;
      break;
  }
  return ret;
}

/**
  * @brief  Reset puse mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of set_rst in
  *                            reg CFG_REG_B
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_set_rst_mode_set(iis2mdc_ctx_t *ctx, iis2mdc_set_rst_t val)
{
  iis2mdc_cfg_reg_b_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.set_rst = (uint8_t)val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Reset puse mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of set_rst in reg CFG_REG_B
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_set_rst_mode_get(iis2mdc_ctx_t *ctx, iis2mdc_set_rst_t *val)
{
  iis2mdc_cfg_reg_b_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  switch (reg.set_rst) {
    case IIS2MDC_SET_SENS_ODR_DIV_63:
      *val = IIS2MDC_SET_SENS_ODR_DIV_63;
      break;
    case IIS2MDC_SENS_OFF_CANC_EVERY_ODR:
      *val = IIS2MDC_SENS_OFF_CANC_EVERY_ODR;
      break;
    case IIS2MDC_SET_SENS_ONLY_AT_POWER_ON:
      *val = IIS2MDC_SET_SENS_ONLY_AT_POWER_ON;
      break;
    default:
      *val = IIS2MDC_SET_SENS_ODR_DIV_63;
      break;
  }
  return ret;
}

/**
  * @brief  Enables offset cancellation in single measurement mode.
  *         The OFF_CANC bit must be set to 1 when enabling offset
  *         cancellation in single measurement mode this means a
  *         call function "set_rst_mode(SENS_OFF_CANC_EVERY_ODR)"
  *         is need.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of off_canc_one_shot in
  *                  reg CFG_REG_B
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_set_rst_sensor_single_set(iis2mdc_ctx_t *ctx, uint8_t val)
{
  iis2mdc_cfg_reg_b_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.off_canc_one_shot = val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables offset cancellation in single measurement mode.
  *         The OFF_CANC bit must be set to 1 when enabling offset
  *         cancellation in single measurement mode this means a
  *         call function "set_rst_mode(SENS_OFF_CANC_EVERY_ODR)"
  *         is need.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of off_canc_one_shot in reg CFG_REG_B
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_set_rst_sensor_single_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_cfg_reg_b_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  *val = reg.off_canc_one_shot;

  return ret;
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdu in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_block_data_update_set(iis2mdc_ctx_t *ctx, uint8_t val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.bdu = val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Block data update.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdu in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_block_data_update_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  *val = reg.bdu;

  return ret;
}

/**
  * @brief  Magnetic set of data available.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of zyxda in reg STATUS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_mag_data_ready_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_status_reg_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_STATUS_REG, (uint8_t*) &reg, 1);
  *val = reg.zyxda;

  return ret;
}

/**
  * @brief  Magnetic set of data overrun.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of zyxor in reg STATUS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_mag_data_ovr_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_status_reg_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_STATUS_REG, (uint8_t*) &reg, 1);
  *val = reg.zyxor;

  return ret;
}

/**
  * @brief  Magnetic output value.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_magnetic_raw_get(iis2mdc_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  iis2mdc_read_reg(ctx, IIS2MDC_OUTX_L_REG, buff, 6);
  return ret;
}

/**
  * @brief  Temperature output value.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_temperature_raw_get(iis2mdc_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  iis2mdc_read_reg(ctx, IIS2MDC_TEMP_OUT_L_REG, buff, 2);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   IIS2MDC_common
  * @brief      This section group common usefull functions
  * @{
  *
  */

/**
  * @brief  Device Who am I.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_device_id_get(iis2mdc_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  iis2mdc_read_reg(ctx, IIS2MDC_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user
  *         registers.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of soft_rst in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_reset_set(iis2mdc_ctx_t *ctx, uint8_t val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.soft_rst = val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of soft_rst in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_reset_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  *val = reg.soft_rst;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reboot in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_boot_set(iis2mdc_ctx_t *ctx, uint8_t val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.reboot = val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reboot in reg CFG_REG_A
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_boot_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_cfg_reg_a_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_A, (uint8_t*) &reg, 1);
  *val = reg.reboot;

  return ret;
}

/**
  * @brief  Selftest.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of self_test in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_self_test_set(iis2mdc_ctx_t *ctx, uint8_t val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.self_test = val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Selftest.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of self_test in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_self_test_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  *val = reg.self_test;

  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ble in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_data_format_set(iis2mdc_ctx_t *ctx, iis2mdc_ble_t val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.ble = (uint8_t)val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of ble in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_data_format_get(iis2mdc_ctx_t *ctx, iis2mdc_ble_t *val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  switch (reg.ble) {
    case IIS2MDC_LSB_AT_LOW_ADD:
      *val = IIS2MDC_LSB_AT_LOW_ADD;
      break;
    case IIS2MDC_MSB_AT_LOW_ADD:
      *val = IIS2MDC_MSB_AT_LOW_ADD;
      break;
    default:
      *val = IIS2MDC_LSB_AT_LOW_ADD;
      break;
  }
  return ret;
}

/**
  * @brief  Info about device status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers STATUS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_status_get(iis2mdc_ctx_t *ctx, iis2mdc_status_reg_t *val)
{
  int32_t ret;
  ret =  iis2mdc_read_reg(ctx, IIS2MDC_STATUS_REG, (uint8_t*) val, 1);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   IIS2MDC_interrupts
  * @brief      This section group all the functions that manage
  *             interrupts.
  * @{
  *
  */

/**
  * @brief  The interrupt block recognition checks data after/before the
  *         hard-iron correction to discover the interrupt.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int_on_dataoff in
  *                                   reg CFG_REG_B
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_offset_int_conf_set(iis2mdc_ctx_t *ctx,
                                    iis2mdc_int_on_dataoff_t val)
{
  iis2mdc_cfg_reg_b_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.int_on_dataoff = (uint8_t)val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  The interrupt block recognition checks data after/before the
  *         hard-iron correction to discover the interrupt.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of int_on_dataoff in
  *                                   reg CFG_REG_B
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_offset_int_conf_get(iis2mdc_ctx_t *ctx,
                                    iis2mdc_int_on_dataoff_t *val)
{
  iis2mdc_cfg_reg_b_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_B, (uint8_t*) &reg, 1);
  switch (reg.int_on_dataoff) {
    case IIS2MDC_CHECK_BEFORE:
      *val = IIS2MDC_CHECK_BEFORE;
      break;
    case IIS2MDC_CHECK_AFTER:
      *val = IIS2MDC_CHECK_AFTER;
      break;
    default:
      *val = IIS2MDC_CHECK_BEFORE;
      break;
  }
  return ret;
}

/**
  * @brief  Data-ready signal on INT_DRDY pin.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of drdy_on_pin in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_drdy_on_pin_set(iis2mdc_ctx_t *ctx, uint8_t val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.drdy_on_pin = val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Data-ready signal on INT_DRDY pin.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of drdy_on_pin in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_drdy_on_pin_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  *val = reg.drdy_on_pin;

  return ret;
}

/**
  * @brief  Interrupt signal on INT_DRDY pin.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int_on_pin in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_int_on_pin_set(iis2mdc_ctx_t *ctx, uint8_t val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.int_on_pin = val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Interrupt signal on INT_DRDY pin.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int_on_pin in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_int_on_pin_get(iis2mdc_ctx_t *ctx, uint8_t *val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  *val = reg.int_on_pin;

  return ret;
}

/**
  * @brief  Interrupt generator configuration register.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers INT_CRTL_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_int_gen_conf_set(iis2mdc_ctx_t *ctx,
                                 iis2mdc_int_crtl_reg_t *val)
{
  int32_t ret;
  ret =  iis2mdc_write_reg(ctx, IIS2MDC_INT_CRTL_REG, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt generator configuration register.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers INT_CRTL_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_int_gen_conf_get(iis2mdc_ctx_t *ctx,
                                 iis2mdc_int_crtl_reg_t *val)
{
  int32_t ret;
  ret =  iis2mdc_read_reg(ctx, IIS2MDC_INT_CRTL_REG, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt generator source register.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers INT_SOURCE_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_int_gen_source_get(iis2mdc_ctx_t *ctx,
                                   iis2mdc_int_source_reg_t *val)
{
  int32_t ret;
  ret =  iis2mdc_read_reg(ctx, IIS2MDC_INT_SOURCE_REG, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  User-defined threshold value for xl interrupt event on generator.
  *         Data format is the same of output data raw:
  *         two’s complement with 1LSb = 1.5mG.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_int_gen_treshold_set(iis2mdc_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  iis2mdc_write_reg(ctx, IIS2MDC_INT_THS_L_REG, buff, 2);
  return ret;
}

/**
  * @brief  User-defined threshold value for xl interrupt event on generator.
  *         Data format is the same of output data raw:
  *         two’s complement with 1LSb = 1.5mG.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_int_gen_treshold_get(iis2mdc_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  iis2mdc_read_reg(ctx, IIS2MDC_INT_THS_L_REG, buff, 2);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   IIS2MDC_serial_interface
  * @brief      This section group all the functions concerning serial
  *             interface management
  * @{
  *
  */

/**
  * @brief  Enable/Disable I2C interface.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of i2c_dis in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_i2c_interface_set(iis2mdc_ctx_t *ctx, iis2mdc_i2c_dis_t val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.i2c_dis = (uint8_t)val;
    ret = iis2mdc_write_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable/Disable I2C interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of i2c_dis in reg CFG_REG_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t iis2mdc_i2c_interface_get(iis2mdc_ctx_t *ctx, iis2mdc_i2c_dis_t *val)
{
  iis2mdc_cfg_reg_c_t reg;
  int32_t ret;

  ret = iis2mdc_read_reg(ctx, IIS2MDC_CFG_REG_C, (uint8_t*) &reg, 1);
  switch (reg.i2c_dis) {
    case IIS2MDC_I2C_ENABLE:
      *val = IIS2MDC_I2C_ENABLE;
      break;
    case IIS2MDC_I2C_DISABLE:
      *val = IIS2MDC_I2C_DISABLE;
      break;
    default:
      *val = IIS2MDC_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/