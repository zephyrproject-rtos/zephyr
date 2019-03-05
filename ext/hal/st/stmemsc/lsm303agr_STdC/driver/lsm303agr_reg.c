/*
  ******************************************************************************
  * @file    lsm303agr_reg.c
  * @author  Sensor Solutions Software Team
  * @brief   LSM303AGR driver file
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

#include "lsm303agr_reg.h"

/**
  * @defgroup  LSM303AGR
  * @brief     This file provides a set of functions needed to drive the
  *            lsm303agr enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup  LSM303AGR_Interfaces_Functions
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
int32_t lsm303agr_read_reg(lsm303agr_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lsm303agr_write_reg(lsm303agr_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LSM303AGR_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t lsm303agr_from_fs_2g_hr_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 16.0f ) * 0.98f;
}

float_t lsm303agr_from_fs_4g_hr_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 16.0f ) * 1.95f;
}

float_t lsm303agr_from_fs_8g_hr_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 16.0f ) * 3.9f;
}

float_t lsm303agr_from_fs_16g_hr_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 16.0f ) * 11.72f;
}

float_t lsm303agr_from_lsb_hr_to_celsius(int16_t lsb)
{
  return ( ( (float_t)lsb / 64.0f ) / 4.0f ) + 25.0f;
}

float_t lsm303agr_from_fs_2g_nm_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 64.0f ) * 3.9f;
}

float_t lsm303agr_from_fs_4g_nm_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 64.0f ) * 7.82f;
}

float_t lsm303agr_from_fs_8g_nm_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 64.0f ) * 15.63f;
}

float_t lsm303agr_from_fs_16g_nm_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 64.0f ) * 46.9f;
}

float_t lsm303agr_from_lsb_nm_to_celsius(int16_t lsb)
{
  return ( ( (float_t)lsb / 64.0f ) / 4.0f ) + 25.0f;
}

float_t lsm303agr_from_fs_2g_lp_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 256.0f ) * 15.63f;
}

float_t lsm303agr_from_fs_4g_lp_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 256.0f ) * 31.26f;
}

float_t lsm303agr_from_fs_8g_lp_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 256.0f ) * 62.52f;
}

float_t lsm303agr_from_fs_16g_lp_to_mg(int16_t lsb)
{
  return ((float_t)lsb / 256.0f ) * 187.58f;
}

float_t lsm303agr_from_lsb_lp_to_celsius(int16_t lsb)
{
  return ( ( (float_t)lsb / 256.0f ) * 1.0f ) + 25.0f;
}

float_t lsm303agr_from_lsb_to_mgauss(int16_t lsb)
{
  return (float_t)lsb * 1.5f;
}

/**
  * @}
  *
  */

/**
  * @defgroup   LSM303AGR_Data_generation
  * @brief      This section group all the functions concerning data generation
  * @{
  *
  */

/**
  * @brief  Temperature status register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_temp_status_reg_get(lsm303agr_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_AUX_A, buff, 1);
  return ret;
}

/**
  * @brief  Temperature data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of tda in reg STATUS_REG_AUX_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_temp_data_ready_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_status_reg_aux_a_t status_reg_aux_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_AUX_A,
                           (uint8_t*)&status_reg_aux_a, 1);
  *val = status_reg_aux_a.tda;

  return ret;
}

/**
  * @brief  Temperature data overrun.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of tor in reg STATUS_REG_AUX_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_temp_data_ovr_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_status_reg_aux_a_t status_reg_aux_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_AUX_A,
                           (uint8_t*)&status_reg_aux_a, 1);
  *val = status_reg_aux_a.tor;

  return ret;
}

/**
  * @brief  Temperature output value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_temperature_raw_get(lsm303agr_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_OUT_TEMP_L_A, buff, 2);
  return ret;
}

/**
  * @brief  Temperature sensor enable.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of temp_en in reg TEMP_CFG_REG_A.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_temperature_meas_set(lsm303agr_ctx_t *ctx,
                                       lsm303agr_temp_en_a_t val)
{
  lsm303agr_temp_cfg_reg_a_t temp_cfg_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_TEMP_CFG_REG_A,
                           (uint8_t*)&temp_cfg_reg_a, 1);
  if(ret == 0){
    temp_cfg_reg_a.temp_en = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_TEMP_CFG_REG_A,
                              (uint8_t*)&temp_cfg_reg_a, 1);
  }

  return ret;
}

/**
  * @brief  Temperature sensor enable.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of temp_en in reg TEMP_CFG_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_temperature_meas_get(lsm303agr_ctx_t *ctx,
                                      lsm303agr_temp_en_a_t *val)
{
  lsm303agr_temp_cfg_reg_a_t temp_cfg_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_TEMP_CFG_REG_A,
                           (uint8_t*)&temp_cfg_reg_a, 1);
  switch (temp_cfg_reg_a.temp_en){
    case LSM303AGR_TEMP_DISABLE:
      *val = LSM303AGR_TEMP_DISABLE;
      break;
    case LSM303AGR_TEMP_ENABLE:
      *val = LSM303AGR_TEMP_ENABLE;
      break;
    default:
      *val = LSM303AGR_TEMP_DISABLE;
      break;
  }

  return ret;
}

/**
  * @brief  Operating mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lpen in reg
  *                CTRL_REG1_A and HR in reg CTRL_REG4_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_operating_mode_set(lsm303agr_ctx_t *ctx,
                                        lsm303agr_op_md_a_t val)
{
  lsm303agr_ctrl_reg1_a_t ctrl_reg1_a;
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;
  uint8_t lpen, hr;

  if ( val == LSM303AGR_HR_12bit ){
    lpen = 0;
    hr   = 1;
  } else if (val == LSM303AGR_NM_10bit) {
    lpen = 0;
    hr   = 0;
  } else {
    lpen = 1;
    hr   = 0;
  }

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG1_A,
                           (uint8_t*)&ctrl_reg1_a, 1);
  ctrl_reg1_a.lpen = (uint8_t)lpen;
  if(ret == 0){
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG1_A,
                              (uint8_t*)&ctrl_reg1_a, 1);
  }
  if(ret == 0){
    ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                             (uint8_t*)&ctrl_reg4_a, 1);
  }
  if(ret == 0){
    ctrl_reg4_a.hr = hr;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG4_A,
                              (uint8_t*)&ctrl_reg4_a, 1);
  }

  return ret;
}

/**
  * @brief  Operating mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of lpen in reg CTRL_REG1_A and HR in
  *                reg CTRL_REG4_AG1_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_operating_mode_get(lsm303agr_ctx_t *ctx,
                                        lsm303agr_op_md_a_t *val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  lsm303agr_ctrl_reg1_a_t ctrl_reg1_a;
  int32_t ret;
  
  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG1_A,
                           (uint8_t*)&ctrl_reg1_a, 1);
  if(ret == 0){
    ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                             (uint8_t*)&ctrl_reg4_a, 1);
  }

  if ( ctrl_reg1_a.lpen != PROPERTY_DISABLE ){
    *val = LSM303AGR_LP_8bit;
  } else if (ctrl_reg4_a.hr  != PROPERTY_DISABLE ) {
    *val = LSM303AGR_HR_12bit;
  } else{
    *val = LSM303AGR_NM_10bit;
  }

  return ret;
}

/**
  * @brief  Output data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odr in reg CTRL_REG1_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_data_rate_set(lsm303agr_ctx_t *ctx,
                                   lsm303agr_odr_a_t val)
{
  lsm303agr_ctrl_reg1_a_t ctrl_reg1_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG1_A,
                           (uint8_t*)&ctrl_reg1_a, 1);
  if(ret == 0){
    ctrl_reg1_a.odr = (uint8_t)val;
   ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG1_A,
                             (uint8_t*)&ctrl_reg1_a, 1);
  }

  return ret;
}

/**
  * @brief  Output data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odr in reg CTRL_REG1_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_data_rate_get(lsm303agr_ctx_t *ctx,
                                   lsm303agr_odr_a_t *val)
{
  lsm303agr_ctrl_reg1_a_t ctrl_reg1_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG1_A,
                           (uint8_t*)&ctrl_reg1_a, 1);

  switch (ctrl_reg1_a.odr){
    case LSM303AGR_XL_POWER_DOWN:
      *val = LSM303AGR_XL_POWER_DOWN;
      break;
    case LSM303AGR_XL_ODR_1Hz:
      *val = LSM303AGR_XL_ODR_1Hz;
      break;
    case LSM303AGR_XL_ODR_10Hz:
      *val = LSM303AGR_XL_ODR_10Hz;
      break;
    case LSM303AGR_XL_ODR_25Hz:
      *val = LSM303AGR_XL_ODR_25Hz;
      break;
    case LSM303AGR_XL_ODR_50Hz:
      *val = LSM303AGR_XL_ODR_50Hz;
      break;
    case LSM303AGR_XL_ODR_100Hz:
      *val = LSM303AGR_XL_ODR_100Hz;
      break;
    case LSM303AGR_XL_ODR_200Hz:
      *val = LSM303AGR_XL_ODR_200Hz;
      break;
    case LSM303AGR_XL_ODR_400Hz:
      *val = LSM303AGR_XL_ODR_400Hz;
      break;
    case LSM303AGR_XL_ODR_1kHz620_LP:
      *val = LSM303AGR_XL_ODR_1kHz620_LP;
      break;
    case LSM303AGR_XL_ODR_1kHz344_NM_HP_5kHz376_LP:
      *val = LSM303AGR_XL_ODR_1kHz344_NM_HP_5kHz376_LP;
      break;
    default:
      *val = LSM303AGR_XL_POWER_DOWN;
      break;
  }

  return ret;
}

/**
  * @brief   High pass data from internal filter sent to output register and FIFO.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fds in reg CTRL_REG2_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_high_pass_on_outputs_set(lsm303agr_ctx_t *ctx,
                                              uint8_t val)
{
  lsm303agr_ctrl_reg2_a_t ctrl_reg2_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG2_A,
                           (uint8_t*)&ctrl_reg2_a, 1);
  if(ret == 0){
    ctrl_reg2_a.fds = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG2_A,
                              (uint8_t*)&ctrl_reg2_a, 1);
  }

  return ret;
}

/**
  * @brief  High pass data from internal filter sent to output
  *         register and FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fds in reg CTRL_REG2_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_high_pass_on_outputs_get(lsm303agr_ctx_t *ctx,
                                              uint8_t *val)
{
  lsm303agr_ctrl_reg2_a_t ctrl_reg2_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG2_A,
                           (uint8_t*)&ctrl_reg2_a, 1);
  *val = ctrl_reg2_a.fds;

  return ret;
}

/**
  * @brief  High-pass filter cutoff frequency selection.[set]
  *
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of hpcf in reg CTRL_REG2_A
  *
  * HPCF[2:1]\ft @1Hz    @10Hz  @25Hz  @50Hz @100Hz @200Hz @400Hz @1kHz6 ft@5kHz
  * AGGRESSIVE   0.02Hz  0.2Hz  0.5Hz  1Hz   2Hz    4Hz    8Hz    32Hz   100Hz
  * STRONG       0.008Hz 0.08Hz 0.2Hz  0.5Hz 1Hz    2Hz    4Hz    16Hz   50Hz
  * MEDIUM       0.004Hz 0.04Hz 0.1Hz  0.2Hz 0.5Hz  1Hz    2Hz    8Hz    25Hz
  * LIGHT        0.002Hz 0.02Hz 0.05Hz 0.1Hz 0.2Hz  0.5Hz  1Hz    4Hz    12Hz
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_high_pass_bandwidth_set(lsm303agr_ctx_t *ctx,
                                             lsm303agr_hpcf_a_t val)
{
  lsm303agr_ctrl_reg2_a_t ctrl_reg2_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG2_A,
                           (uint8_t*)&ctrl_reg2_a, 1);
  if(ret == 0){
    ctrl_reg2_a.hpcf = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG2_A,
                              (uint8_t*)&ctrl_reg2_a, 1);
  }

  return ret;
}

/**
  * @brief  High-pass filter cutoff frequency selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hpcf in reg CTRL_REG2_A.(ptr)
  *
  * HPCF[2:1]\ft @1Hz    @10Hz  @25Hz  @50Hz @100Hz @200Hz @400Hz @1kHz6 ft@5kHz
  * AGGRESSIVE   0.02Hz  0.2Hz  0.5Hz  1Hz   2Hz    4Hz    8Hz    32Hz   100Hz
  * STRONG       0.008Hz 0.08Hz 0.2Hz  0.5Hz 1Hz    2Hz    4Hz    16Hz   50Hz
  * MEDIUM       0.004Hz 0.04Hz 0.1Hz  0.2Hz 0.5Hz  1Hz    2Hz    8Hz    25Hz
  * LIGHT        0.002Hz 0.02Hz 0.05Hz 0.1Hz 0.2Hz  0.5Hz  1Hz    4Hz    12Hz
  *
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_high_pass_bandwidth_get(lsm303agr_ctx_t *ctx,
                                             lsm303agr_hpcf_a_t *val)
{
  lsm303agr_ctrl_reg2_a_t ctrl_reg2_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG2_A,
                           (uint8_t*)&ctrl_reg2_a, 1);

  switch (ctrl_reg2_a.hpcf){
    case LSM303AGR_AGGRESSIVE:
      *val = LSM303AGR_AGGRESSIVE;
      break;
    case LSM303AGR_STRONG:
      *val = LSM303AGR_STRONG;
      break;
    case LSM303AGR_MEDIUM:
      *val = LSM303AGR_MEDIUM;
      break;
    case LSM303AGR_LIGHT:
      *val = LSM303AGR_LIGHT;
      break;
    default:
      *val = LSM303AGR_AGGRESSIVE;
      break;
  }
  return ret;
}

/**
  * @brief  High-pass filter mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of hpm in reg CTRL_REG2_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_high_pass_mode_set(lsm303agr_ctx_t *ctx,
                                        lsm303agr_hpm_a_t val)
{
  lsm303agr_ctrl_reg2_a_t ctrl_reg2_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG2_A,
                           (uint8_t*)&ctrl_reg2_a, 1);
  if(ret == 0){
    ctrl_reg2_a.hpm = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG2_A,
                              (uint8_t*)&ctrl_reg2_a, 1);
  }

  return ret;
}

/**
  * @brief  High-pass filter mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hpm in reg CTRL_REG2_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_high_pass_mode_get(lsm303agr_ctx_t *ctx,
                                        lsm303agr_hpm_a_t *val)
{
  lsm303agr_ctrl_reg2_a_t ctrl_reg2_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG2_A,
                           (uint8_t*)&ctrl_reg2_a, 1);

  switch (ctrl_reg2_a.hpm){
    case LSM303AGR_NORMAL_WITH_RST:
      *val = LSM303AGR_NORMAL_WITH_RST;
      break;
    case LSM303AGR_REFERENCE_MODE:
      *val = LSM303AGR_REFERENCE_MODE;
      break;
    case LSM303AGR_NORMAL:
      *val = LSM303AGR_NORMAL;
      break;
    case LSM303AGR_AUTORST_ON_INT:
      *val = LSM303AGR_AUTORST_ON_INT;
      break;
    default:
      *val = LSM303AGR_NORMAL_WITH_RST;
      break;
  }
  return ret;
}

/**
  * @brief  Full-scale configuration.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fs in reg CTRL_REG4_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_full_scale_set(lsm303agr_ctx_t *ctx,
                                    lsm303agr_fs_a_t val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);
  if(ret == 0){
    ctrl_reg4_a.fs = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG4_A,
                              (uint8_t*)&ctrl_reg4_a, 1);
  }

  return ret;
}

/**
  * @brief  Full-scale configuration.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fs in reg CTRL_REG4_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_full_scale_get(lsm303agr_ctx_t *ctx,
                                    lsm303agr_fs_a_t *val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);

  switch (ctrl_reg4_a.fs){
    case LSM303AGR_2g:
      *val = LSM303AGR_2g;
      break;
    case LSM303AGR_4g:
      *val = LSM303AGR_4g;
      break;
    case LSM303AGR_8g:
      *val = LSM303AGR_8g;
      break;
    case LSM303AGR_16g:
      *val = LSM303AGR_16g;
      break;
    default:
      *val = LSM303AGR_2g;
      break;
  }
  return ret;
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of bdu in reg CTRL_REG4_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_block_data_update_set(lsm303agr_ctx_t *ctx,
                                           uint8_t val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);
  if(ret == 0){
    ctrl_reg4_a.bdu = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG4_A,
                              (uint8_t*)&ctrl_reg4_a, 1);
  }

  return ret;
}

/**
  * @brief  Block data update.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bdu in reg CTRL_REG4_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_block_data_update_get(lsm303agr_ctx_t *ctx,
                                           uint8_t *val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);
  *val = ctrl_reg4_a.bdu;

  return ret;
}

/**
  * @brief  Reference value for interrupt generation.[set]
  *         LSB = ~16@2g / ~31@4g / ~63@8g / ~127@16g
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that contains data to write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_filter_reference_set(lsm303agr_ctx_t *ctx,
                                          uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_REFERENCE_A, buff, 1);
  return ret;
}

/**
  * @brief  Reference value for interrupt generation.[get]
  *         LSB = ~16@2g / ~31@4g / ~63@8g / ~127@16g
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_filter_reference_get(lsm303agr_ctx_t *ctx,
                                          uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_REFERENCE_A, buff, 1);
  return ret;
}

/**
  * @brief  Acceleration set of data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of zyxda in reg STATUS_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_data_ready_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_status_reg_a_t status_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_A,
                           (uint8_t*)&status_reg_a, 1);
  *val = status_reg_a.zyxda;

  return ret;
}

/**
  * @brief  Acceleration set of data overrun.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of zyxor in reg STATUS_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_data_ovr_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_status_reg_a_t status_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_A,
                           (uint8_t*)&status_reg_a, 1);
  *val = status_reg_a.zyxor;

  return ret;
}

/**
  * @brief  Acceleration output value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_acceleration_raw_get(lsm303agr_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_OUT_X_L_A, buff, 6);
  return ret;
}

/**
  * @brief  These registers comprise a 3 group of
  *         16-bit number and represent hard-iron
  *         offset in order to compensate environmental
  *         effects. Data format is the same of
  *         output data raw: two’s complement with
  *         1LSb = 1.5mG. These values act on the
  *         magnetic output data value in order to
  *         delete the environmental offset.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that contains data to write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_user_offset_set(lsm303agr_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_OFFSET_X_REG_L_M, buff, 6);
  return ret;
}

/**
  * @brief  These registers comprise a 3 group of
  *         16-bit number and represent hard-iron
  *         offset in order to compensate environmental
  *         effects. Data format is the same of
  *         output data raw: two’s complement with
  *         1LSb = 1.5mG. These values act on the
  *         magnetic output data value in order to
  *         delete the environmental offset.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_user_offset_get(lsm303agr_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_OFFSET_X_REG_L_M, buff, 6);
  return ret;
}

/**
  * @brief  Operating mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of md in reg CFG_REG_A_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_operating_mode_set(lsm303agr_ctx_t *ctx,
                                         lsm303agr_md_m_t val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  if(ret == 0){
    cfg_reg_a_m.md = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_A_M,
                              (uint8_t*)&cfg_reg_a_m, 1);
  }

  return ret;
}

/**
  * @brief  Operating mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of md in reg CFG_REG_A_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_operating_mode_get(lsm303agr_ctx_t *ctx,
                                         lsm303agr_md_m_t *val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);

    switch (cfg_reg_a_m.md){
    case LSM303AGR_CONTINUOUS_MODE:
      *val = LSM303AGR_CONTINUOUS_MODE;
      break;
    case LSM303AGR_SINGLE_TRIGGER:
      *val = LSM303AGR_SINGLE_TRIGGER;
      break;
    case LSM303AGR_POWER_DOWN:
      *val = LSM303AGR_POWER_DOWN;
      break;
    default:
      *val = LSM303AGR_CONTINUOUS_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  Output data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odr in reg CFG_REG_A_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_data_rate_set(lsm303agr_ctx_t *ctx,
                                    lsm303agr_mg_odr_m_t val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  if(ret == 0){
    cfg_reg_a_m.odr = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_A_M,
                              (uint8_t*)&cfg_reg_a_m, 1);
  }

  return ret;
}

/**
  * @brief  Output data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odr in reg CFG_REG_A_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_data_rate_get(lsm303agr_ctx_t *ctx,
                                    lsm303agr_mg_odr_m_t *val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);

    switch (cfg_reg_a_m.odr){
    case LSM303AGR_MG_ODR_10Hz:
      *val = LSM303AGR_MG_ODR_10Hz;
      break;
    case LSM303AGR_MG_ODR_20Hz:
      *val = LSM303AGR_MG_ODR_20Hz;
      break;
    case LSM303AGR_MG_ODR_50Hz:
      *val = LSM303AGR_MG_ODR_50Hz;
      break;
    case LSM303AGR_MG_ODR_100Hz:
      *val = LSM303AGR_MG_ODR_100Hz;
      break;
    default:
      *val = LSM303AGR_MG_ODR_10Hz;
      break;
  }
  return ret;
}

/**
  * @brief  Enables high-resolution/low-power mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lp in reg CFG_REG_A_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_power_mode_set(lsm303agr_ctx_t *ctx,
                                     lsm303agr_lp_m_t val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  if(ret == 0){
    cfg_reg_a_m.lp = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_A_M,
                              (uint8_t*)&cfg_reg_a_m, 1);
  }

  return ret;
}

/**
  * @brief  Enables high-resolution/low-power mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of lp in reg CFG_REG_A_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_power_mode_get(lsm303agr_ctx_t *ctx,
                                     lsm303agr_lp_m_t *val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);

    switch (cfg_reg_a_m.lp){
    case LSM303AGR_HIGH_RESOLUTION:
      *val = LSM303AGR_HIGH_RESOLUTION;
      break;
    case LSM303AGR_LOW_POWER:
      *val = LSM303AGR_LOW_POWER;
      break;
    default:
      *val = LSM303AGR_HIGH_RESOLUTION;
      break;
  }
  return ret;
}

/**
  * @brief  Enables the magnetometer temperature compensation.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of comp_temp_en in reg CFG_REG_A_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_offset_temp_comp_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  if(ret == 0){
    cfg_reg_a_m.comp_temp_en = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_A_M,
                              (uint8_t*)&cfg_reg_a_m, 1);
  }

  return ret;
}

/**
  * @brief  Enables the magnetometer temperature compensation.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of comp_temp_en in reg CFG_REG_A_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_offset_temp_comp_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  *val = cfg_reg_a_m.comp_temp_en;

  return ret;
}

/**
  * @brief  Low-pass bandwidth selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lpf in reg CFG_REG_B_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_low_pass_bandwidth_set(lsm303agr_ctx_t *ctx,
                                             lsm303agr_lpf_m_t val)
{
  lsm303agr_cfg_reg_b_m_t cfg_reg_b_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_B_M,
                           (uint8_t*)&cfg_reg_b_m, 1);
  if(ret == 0){
    cfg_reg_b_m.lpf = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_B_M,
                              (uint8_t*)&cfg_reg_b_m, 1);
  }

  return ret;
}

/**
  * @brief  Low-pass bandwidth selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of lpf in reg CFG_REG_B_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_low_pass_bandwidth_get(lsm303agr_ctx_t *ctx,
                                             lsm303agr_lpf_m_t *val)
{
  lsm303agr_cfg_reg_b_m_t cfg_reg_b_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_B_M,
                           (uint8_t*)&cfg_reg_b_m, 1);

    switch (cfg_reg_b_m.lpf){
    case LSM303AGR_ODR_DIV_2:
      *val = LSM303AGR_ODR_DIV_2;
      break;
    case LSM303AGR_ODR_DIV_4:
      *val = LSM303AGR_ODR_DIV_4;
      break;
    default:
      *val = LSM303AGR_ODR_DIV_2;
      break;
  }
  return ret;
}

/**
  * @brief  Magnetometer sampling mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of set_rst in reg CFG_REG_B_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_set_rst_mode_set(lsm303agr_ctx_t *ctx,
                                       lsm303agr_set_rst_m_t val)
{
  lsm303agr_cfg_reg_b_m_t cfg_reg_b_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_B_M,
                           (uint8_t*)&cfg_reg_b_m, 1);
  if(ret == 0){
    cfg_reg_b_m.set_rst = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_B_M,
                              (uint8_t*)&cfg_reg_b_m, 1);
  }

  return ret;
}

/**
  * @brief  Magnetometer sampling mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of set_rst in reg CFG_REG_B_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_set_rst_mode_get(lsm303agr_ctx_t *ctx,
                                       lsm303agr_set_rst_m_t *val)
{
  lsm303agr_cfg_reg_b_m_t cfg_reg_b_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_B_M,
                           (uint8_t*)&cfg_reg_b_m, 1);

    switch (cfg_reg_b_m.set_rst){
    case LSM303AGR_SET_SENS_ODR_DIV_63:
      *val = LSM303AGR_SET_SENS_ODR_DIV_63;
      break;
    case LSM303AGR_SENS_OFF_CANC_EVERY_ODR:
      *val = LSM303AGR_SENS_OFF_CANC_EVERY_ODR;
      break;
    case LSM303AGR_SET_SENS_ONLY_AT_POWER_ON:
      *val = LSM303AGR_SET_SENS_ONLY_AT_POWER_ON;
      break;
    default:
      *val = LSM303AGR_SET_SENS_ODR_DIV_63;
      break;
  }
  return ret;
}

/**
  * @brief  Enables offset cancellation in single measurement mode.
  *         The OFF_CANC bit must be set
  *         to 1 when enabling offset
  *         cancellation in single measurement
  *         mode this means a call function:
  *         mag_set_rst_mode
  *         (SENS_OFF_CANC_EVERY_ODR) is need.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of off_canc_one_shot in reg CFG_REG_B_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_set_rst_sensor_single_set(lsm303agr_ctx_t *ctx,
                                                uint8_t val)
{
  lsm303agr_cfg_reg_b_m_t cfg_reg_b_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_B_M,
                           (uint8_t*)&cfg_reg_b_m, 1);
  if(ret == 0){
    cfg_reg_b_m.off_canc_one_shot = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_B_M,
                              (uint8_t*)&cfg_reg_b_m, 1);
  }

  return ret;
}

/**
  * @brief  Enables offset cancellation in single measurement mode.
  *         The OFF_CANC bit must be set to
  *         1 when enabling offset cancellation
  *         in single measurement mode this
  *         means a call function:
  *         mag_set_rst_mode
  *         (SENS_OFF_CANC_EVERY_ODR) is need.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of off_canc_one_shot in
  *                reg CFG_REG_B_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_set_rst_sensor_single_get(lsm303agr_ctx_t *ctx,
                                                uint8_t *val)
{
  lsm303agr_cfg_reg_b_m_t cfg_reg_b_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_B_M,
                           (uint8_t*)&cfg_reg_b_m, 1);
  *val = cfg_reg_b_m.off_canc_one_shot;

  return ret;
}

/**
  * @brief  Blockdataupdate.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of bdu in reg CFG_REG_C_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_block_data_update_set(lsm303agr_ctx_t *ctx,
                                            uint8_t val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  if(ret == 0){
    cfg_reg_c_m.bdu = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_C_M,
                              (uint8_t*)&cfg_reg_c_m, 1);
  }

  return ret;
}

/**
  * @brief  Blockdataupdate.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bdu in reg CFG_REG_C_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_block_data_update_get(lsm303agr_ctx_t *ctx,
                                            uint8_t *val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  *val = cfg_reg_c_m.bdu;

  return ret;
}

/**
  * @brief  Magnetic set of data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of zyxda in reg STATUS_REG_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_data_ready_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_status_reg_m_t status_reg_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_M,
                           (uint8_t*)&status_reg_m, 1);
  *val = status_reg_m.zyxda;

  return ret;
}

/**
  * @brief  Magnetic set of data overrun.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of zyxor in reg STATUS_REG_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_data_ovr_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_status_reg_m_t status_reg_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_M,
                           (uint8_t*)&status_reg_m, 1);
  *val = status_reg_m.zyxor;

  return ret;
}

/**
  * @brief  Magnetic output value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_magnetic_raw_get(lsm303agr_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_OUTX_L_REG_M, buff, 6);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup  common
  * @brief   This section group common usefull functions
  * @{
  *
  */

/**
  * @brief  DeviceWhoamI.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_device_id_get(lsm303agr_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_WHO_AM_I_A, buff, 1);
  return ret;
}

/**
  * @brief  Self-test.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of st in reg CTRL_REG4_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_self_test_set(lsm303agr_ctx_t *ctx,
                                   lsm303agr_st_a_t val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);
  if(ret == 0){
    ctrl_reg4_a.st = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG4_A,
                              (uint8_t*)&ctrl_reg4_a, 1);
  }

  return ret;
}

/**
  * @brief  Self-test.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st in reg CTRL_REG4_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_self_test_get(lsm303agr_ctx_t *ctx,
                                   lsm303agr_st_a_t *val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);

    switch (ctrl_reg4_a.st){
    case LSM303AGR_ST_DISABLE:
      *val = LSM303AGR_ST_DISABLE;
      break;
    case LSM303AGR_ST_POSITIVE:
      *val = LSM303AGR_ST_POSITIVE;
      break;
    case LSM303AGR_ST_NEGATIVE:
      *val = LSM303AGR_ST_NEGATIVE;
      break;
    default:
      *val = LSM303AGR_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ble in reg CTRL_REG4_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_data_format_set(lsm303agr_ctx_t *ctx,
                                     lsm303agr_ble_a_t val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);
  if(ret == 0){
    ctrl_reg4_a.ble = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG4_A,
                              (uint8_t*)&ctrl_reg4_a, 1);
  }

  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ble in reg CTRL_REG4_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_data_format_get(lsm303agr_ctx_t *ctx,
                                     lsm303agr_ble_a_t *val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);

    switch (ctrl_reg4_a.ble){
    case LSM303AGR_XL_LSB_AT_LOW_ADD:
      *val = LSM303AGR_XL_LSB_AT_LOW_ADD;
      break;
    case LSM303AGR_XL_MSB_AT_LOW_ADD:
      *val = LSM303AGR_XL_MSB_AT_LOW_ADD;
      break;
    default:
      *val = LSM303AGR_XL_LSB_AT_LOW_ADD;
      break;
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of boot in reg CTRL_REG5_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_boot_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  if(ret == 0){
    ctrl_reg5_a.boot = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG5_A,
                              (uint8_t*)&ctrl_reg5_a, 1);
  }

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of boot in reg CTRL_REG5_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_boot_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  *val = ctrl_reg5_a.boot;

  return ret;
}

/**
  * @brief  Info about device status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get register STATUS_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_status_get(lsm303agr_ctx_t *ctx,
                                lsm303agr_status_reg_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  DeviceWhoamI.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_device_id_get(lsm303agr_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_WHO_AM_I_M, buff, 1);
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of soft_rst in reg CFG_REG_A_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_reset_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  if(ret == 0){
    cfg_reg_a_m.soft_rst = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_A_M,
                              (uint8_t*)&cfg_reg_a_m, 1);
  }

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of soft_rst in reg CFG_REG_A_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_reset_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  *val = cfg_reg_a_m.soft_rst;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of reboot in reg CFG_REG_A_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_boot_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  if(ret == 0){
    cfg_reg_a_m.reboot = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_A_M,
                              (uint8_t*)&cfg_reg_a_m, 1);
  }

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of reboot in reg CFG_REG_A_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_boot_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_cfg_reg_a_m_t cfg_reg_a_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_A_M,
                           (uint8_t*)&cfg_reg_a_m, 1);
  *val = cfg_reg_a_m.reboot;

  return ret;
}

/**
  * @brief  Selftest.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of self_test in reg CFG_REG_C_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_self_test_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  if(ret == 0){
    cfg_reg_c_m.self_test = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_C_M,
                              (uint8_t*)&cfg_reg_c_m, 1);
  }

  return ret;
}

/**
  * @brief  Selftest.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of self_test in reg CFG_REG_C_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_self_test_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  *val = cfg_reg_c_m.self_test;

  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ble in reg CFG_REG_C_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_data_format_set(lsm303agr_ctx_t *ctx,
                                      lsm303agr_ble_m_t val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  if(ret == 0){
    cfg_reg_c_m.ble = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_C_M,
                              (uint8_t*)&cfg_reg_c_m, 1);
  }

  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ble in reg CFG_REG_C_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_data_format_get(lsm303agr_ctx_t *ctx,
                                      lsm303agr_ble_m_t *val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);

    switch (cfg_reg_c_m.ble){
    case LSM303AGR_MG_LSB_AT_LOW_ADD:
      *val = LSM303AGR_MG_LSB_AT_LOW_ADD;
      break;
    case LSM303AGR_MG_MSB_AT_LOW_ADD:
      *val = LSM303AGR_MG_MSB_AT_LOW_ADD;
      break;
    default:
      *val = LSM303AGR_MG_LSB_AT_LOW_ADD;
      break;
  }
  return ret;
}

/**
  * @brief  Info about device status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers STATUS_REG_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_status_get(lsm303agr_ctx_t *ctx,
                                 lsm303agr_status_reg_m_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_STATUS_REG_M, (uint8_t*) val, 1);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup   interrupts_generator_1_for_xl
  * @brief   This section group all the functions that manage the first
  *          interrupts generator of accelerometer
  * @{
  *
  */

/**
  * @brief  Interrupt generator 1 configuration register.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change register INT1_CFG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_gen_conf_set(lsm303agr_ctx_t *ctx,
                                       lsm303agr_int1_cfg_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_INT1_CFG_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt generator 1 configuration register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get register INT1_CFG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_gen_conf_get(lsm303agr_ctx_t *ctx,
                                       lsm303agr_int1_cfg_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT1_CFG_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt generator 1 source register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers INT1_SRC_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_gen_source_get(lsm303agr_ctx_t *ctx,
                                         lsm303agr_int1_src_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT1_SRC_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  User-defined threshold value for xl
  *         interrupt event on generator 1.[set]
  *         LSb = 16mg@2g / 32mg@4g / 62mg@8g / 186mg@16g
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ths in reg INT1_THS_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_gen_threshold_set(lsm303agr_ctx_t *ctx,
                                            uint8_t val)
{
  lsm303agr_int1_ths_a_t int1_ths_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT1_THS_A,
                           (uint8_t*)&int1_ths_a, 1);
  if(ret == 0){
    int1_ths_a.ths = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_INT1_THS_A,
                              (uint8_t*)&int1_ths_a, 1);
  }

  return ret;
}

/**
  * @brief  User-defined threshold value for xl
  *         interrupt event on generator 1.[get]
  *         LSb = 16mg@2g / 32mg@4g / 62mg@8g / 186mg@16g
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ths in reg INT1_THS_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_gen_threshold_get(lsm303agr_ctx_t *ctx,
                                            uint8_t *val)
{
  lsm303agr_int1_ths_a_t int1_ths_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT1_THS_A,
                           (uint8_t*)&int1_ths_a, 1);
  *val = int1_ths_a.ths;

  return ret;
}

/**
  * @brief  The minimum duration (LSb = 1/ODR) of the Interrupt 1 event to be
  *         recognized.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of d in reg INT1_DURATION_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_gen_duration_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_int1_duration_a_t int1_duration_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT1_DURATION_A,
                           (uint8_t*)&int1_duration_a, 1);
  if(ret == 0){
    int1_duration_a.d = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_INT1_DURATION_A,
                              (uint8_t*)&int1_duration_a, 1);
  }

  return ret;
}

/**
  * @brief  The minimum duration (LSb = 1/ODR) of the Interrupt 1 event to be
  *         recognized.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of d in reg INT1_DURATION_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_gen_duration_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_int1_duration_a_t int1_duration_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT1_DURATION_A,
                           (uint8_t*)&int1_duration_a, 1);
  *val = int1_duration_a.d;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup   interrupts_generator_2_for_xl
  * @brief   This section group all the functions that manage the second
  *          interrupts generator for accelerometer
  * @{
  *
  */

/**
  * @brief  Interrupt generator 2 configuration register.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change registers INT2_CFG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_gen_conf_set(lsm303agr_ctx_t *ctx,
                                       lsm303agr_int2_cfg_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_INT2_CFG_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt generator 2 configuration register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers INT2_CFG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_gen_conf_get(lsm303agr_ctx_t *ctx,
                                       lsm303agr_int2_cfg_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT2_CFG_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt generator 2 source register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers INT2_SRC_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_gen_source_get(lsm303agr_ctx_t *ctx,
                                         lsm303agr_int2_src_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT2_SRC_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  User-defined threshold value for xl
  *         interrupt event on generator 2.[set]
  *         LSb = 16mg@2g / 32mg@4g / 62mg@8g / 186mg@16g
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ths in reg INT2_THS_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_gen_threshold_set(lsm303agr_ctx_t *ctx,
                                            uint8_t val)
{
  lsm303agr_int2_ths_a_t int2_ths_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT2_THS_A,
                           (uint8_t*)&int2_ths_a, 1);
  if(ret == 0){
    int2_ths_a.ths = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_INT2_THS_A,
                              (uint8_t*)&int2_ths_a, 1);
  }

  return ret;
}

/**
  * @brief  User-defined threshold value for
  *         xl interrupt event on generator 2.[get]
  *         LSb = 16mg@2g / 32mg@4g / 62mg@8g / 186mg@16g
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ths in reg INT2_THS_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_gen_threshold_get(lsm303agr_ctx_t *ctx,
                                            uint8_t *val)
{
  lsm303agr_int2_ths_a_t int2_ths_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT2_THS_A,
                           (uint8_t*)&int2_ths_a, 1);
  *val = int2_ths_a.ths;

  return ret;
}

/**
  * @brief  The minimum duration (LSb = 1/ODR) of the Interrupt 1 event to be
  *         recognized.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of d in reg INT2_DURATION_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_gen_duration_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_int2_duration_a_t int2_duration_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT2_DURATION_A,
                           (uint8_t*)&int2_duration_a, 1);
  if(ret == 0){
    int2_duration_a.d = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_INT2_DURATION_A,
                              (uint8_t*)&int2_duration_a, 1);
  }

  return ret;
}

/**
  * @brief  The minimum duration (LSb = 1/ODR) of the Interrupt 1 event to be
  *         recognized.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of d in reg INT2_DURATION_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_gen_duration_get(lsm303agr_ctx_t *ctx,
                                           uint8_t *val)
{
  lsm303agr_int2_duration_a_t int2_duration_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT2_DURATION_A,
                           (uint8_t*)&int2_duration_a, 1);
  *val = int2_duration_a.d;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup  interrupt_pins_xl
  * @brief   This section group all the functions that manage interrupt
  *          pins of accelerometer
  * @{
  *
  */

/**
  * @brief  High-pass filter on interrupts/tap generator.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of hp in reg CTRL_REG2_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_high_pass_int_conf_set(lsm303agr_ctx_t *ctx,
                                            lsm303agr_hp_a_t val)
{
  lsm303agr_ctrl_reg2_a_t ctrl_reg2_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG2_A,
                           (uint8_t*)&ctrl_reg2_a, 1);
  if(ret == 0){
    ctrl_reg2_a.hp = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG2_A,
                              (uint8_t*)&ctrl_reg2_a, 1);
  }

  return ret;
}

/**
  * @brief  High-pass filter on interrupts/tap generator.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hp in reg CTRL_REG2_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_high_pass_int_conf_get(lsm303agr_ctx_t *ctx,
                                            lsm303agr_hp_a_t *val)
{
  lsm303agr_ctrl_reg2_a_t ctrl_reg2_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG2_A,
                           (uint8_t*)&ctrl_reg2_a, 1);

    switch (ctrl_reg2_a.hp){
    case LSM303AGR_DISC_FROM_INT_GENERATOR:
      *val = LSM303AGR_DISC_FROM_INT_GENERATOR;
      break;
    case LSM303AGR_ON_INT1_GEN:
      *val = LSM303AGR_ON_INT1_GEN;
      break;
    case LSM303AGR_ON_INT2_GEN:
      *val = LSM303AGR_ON_INT2_GEN;
      break;
    case LSM303AGR_ON_TAP_GEN:
      *val = LSM303AGR_ON_TAP_GEN;
      break;
    case LSM303AGR_ON_INT1_INT2_GEN:
      *val = LSM303AGR_ON_INT1_INT2_GEN;
      break;
    case LSM303AGR_ON_INT1_TAP_GEN:
      *val = LSM303AGR_ON_INT1_TAP_GEN;
      break;
    case LSM303AGR_ON_INT2_TAP_GEN:
      *val = LSM303AGR_ON_INT2_TAP_GEN;
      break;
    case LSM303AGR_ON_INT1_INT2_TAP_GEN:
      *val = LSM303AGR_ON_INT1_INT2_TAP_GEN;
      break;
    default:
      *val = LSM303AGR_DISC_FROM_INT_GENERATOR;
      break;
  }
  return ret;
}

/**
  * @brief  Int1 pin routing configuration register.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change registers CTRL_REG3_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_pin_int1_config_set(lsm303agr_ctx_t *ctx,
                                         lsm303agr_ctrl_reg3_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG3_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Int1 pin routing configuration register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers CTRL_REG3_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_pin_int1_config_get(lsm303agr_ctx_t *ctx,
                                         lsm303agr_ctrl_reg3_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG3_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  4D detection is enabled on INT2 pin when 6D bit on
  *         INT2_CFG_A (34h) is set to 1.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of d4d_int2 in reg CTRL_REG5_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_pin_detect_4d_set(lsm303agr_ctx_t *ctx,
                                            uint8_t val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  if(ret == 0){
    ctrl_reg5_a.d4d_int2 = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG5_A,
                              (uint8_t*)&ctrl_reg5_a, 1);
  }

  return ret;
}

/**
  * @brief  4D detection is enabled on INT2 pin when 6D bit on
  *         INT2_CFG_A (34h) is set to 1.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of d4d_int2 in reg CTRL_REG5_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2_pin_detect_4d_get(lsm303agr_ctx_t *ctx,
                                            uint8_t *val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  *val = ctrl_reg5_a.d4d_int2;

  return ret;
}

/**
  * @brief  Latch interrupt request on INT2_SRC_A (35h) register, with
  *         INT2_SRC_A (35h) register cleared by reading
  *         INT2_SRC_A (35h) itself.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lir_int2 in reg CTRL_REG5_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2pin_notification_mode_set(lsm303agr_ctx_t *ctx,
                                                   lsm303agr_lir_int2_a_t val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  if(ret == 0){
    ctrl_reg5_a.lir_int2 = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG5_A,
                              (uint8_t*)&ctrl_reg5_a, 1);
  }

  return ret;
}

/**
  * @brief  Latch interrupt request on INT2_SRC_A (35h) register, with
  *         INT2_SRC_A (35h) register cleared by reading
  *         INT2_SRC_A (35h) itself.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of lir_int2 in reg CTRL_REG5_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int2pin_notification_mode_get(lsm303agr_ctx_t *ctx,
                                                lsm303agr_lir_int2_a_t *val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);

    switch (ctrl_reg5_a.lir_int2){
    case LSM303AGR_INT2_PULSED:
      *val = LSM303AGR_INT2_PULSED;
      break;
    case LSM303AGR_INT2_LATCHED:
      *val = LSM303AGR_INT2_LATCHED;
      break;
    default:
      *val = LSM303AGR_INT2_PULSED;
      break;
  }
  return ret;
}

/**
  * @brief  4D detection is enabled on INT1 pin when 6D bit on
  *         INT1_CFG_A (30h) is set to 1.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of d4d_int1 in reg CTRL_REG5_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_pin_detect_4d_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  if(ret == 0){
    ctrl_reg5_a.d4d_int1 = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG5_A,
                              (uint8_t*)&ctrl_reg5_a, 1);
  }

  return ret;
}

/**
  * @brief  4D detection is enabled on INT1 pin when 6D bit on
  *         INT1_CFG_A (30h) is set to 1.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of d4d_int1 in reg CTRL_REG5_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1_pin_detect_4d_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  *val = ctrl_reg5_a.d4d_int1;

  return ret;
}

/**
  * @brief  Latch interrupt request on INT1_SRC_A (31h), with
  *         INT1_SRC_A(31h) register cleared by reading
  *         INT1_SRC_A (31h) itself.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lir_int1 in reg CTRL_REG5_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1pin_notification_mode_set(lsm303agr_ctx_t *ctx,
                                                   lsm303agr_lir_int1_a_t val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  if(ret == 0){
    ctrl_reg5_a.lir_int1 = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG5_A,
                              (uint8_t*)&ctrl_reg5_a, 1);
  }

  return ret;
}

/**
  * @brief  Latch interrupt request on INT1_SRC_A (31h), with
  *         INT1_SRC_A(31h) register cleared by reading
  *         INT1_SRC_A (31h) itself.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of lir_int1 in reg CTRL_REG5_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_int1pin_notification_mode_get(lsm303agr_ctx_t *ctx,
                                                   lsm303agr_lir_int1_a_t *val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);

    switch (ctrl_reg5_a.lir_int1){
    case LSM303AGR_INT1_PULSED:
      *val = LSM303AGR_INT1_PULSED;
      break;
    case LSM303AGR_INT1_LATCHED:
      *val = LSM303AGR_INT1_LATCHED;
      break;
    default:
      *val = LSM303AGR_INT1_PULSED;
      break;
  }
  return ret;
}

/**
  * @brief  Int2 pin routing configuration register.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change registers CTRL_REG6_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_pin_int2_config_set(lsm303agr_ctx_t *ctx,
                                         lsm303agr_ctrl_reg6_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG6_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Int2 pin routing configuration register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers CTRL_REG6_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_pin_int2_config_get(lsm303agr_ctx_t *ctx,
                                         lsm303agr_ctrl_reg6_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG6_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @}
  *
  */

  /**
  * @addtogroup  magnetometer interrupts
  * @brief       This section group all the functions that manage the
  *              magnetometer interrupts
  * @{
  *
  */

/**
  * @brief  The interrupt block recognition checks
  *         data after/before the hard-iron correction
  *         to discover the interrupt.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int_on_dataoff in reg CFG_REG_B_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_offset_int_conf_set(lsm303agr_ctx_t *ctx,
                                          lsm303agr_int_on_dataoff_m_t val)
{
  lsm303agr_cfg_reg_b_m_t cfg_reg_b_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_B_M,
                           (uint8_t*)&cfg_reg_b_m, 1);
  if(ret == 0){
    cfg_reg_b_m.int_on_dataoff = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_B_M,
                              (uint8_t*)&cfg_reg_b_m, 1);
  }

  return ret;
}

/**
  * @brief  The interrupt block recognition checks
  *         data after/before the hard-iron correction
  *         to discover the interrupt.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int_on_dataoff in reg CFG_REG_B_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_offset_int_conf_get(lsm303agr_ctx_t *ctx,
                                          lsm303agr_int_on_dataoff_m_t *val)
{
  lsm303agr_cfg_reg_b_m_t cfg_reg_b_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_B_M,
                           (uint8_t*)&cfg_reg_b_m, 1);

    switch (cfg_reg_b_m.int_on_dataoff){
    case LSM303AGR_CHECK_BEFORE:
      *val = LSM303AGR_CHECK_BEFORE;
      break;
    case LSM303AGR_CHECK_AFTER:
      *val = LSM303AGR_CHECK_AFTER;
      break;
    default:
      *val = LSM303AGR_CHECK_BEFORE;
      break;
  }
  return ret;
}

/**
  * @brief  Data-ready signal on INT_DRDY pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of drdy_on_pin in reg CFG_REG_C_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_drdy_on_pin_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  if(ret == 0){
    cfg_reg_c_m.int_mag = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_C_M,
                              (uint8_t*)&cfg_reg_c_m, 1);
  }

  return ret;
}

/**
  * @brief  Data-ready signal on INT_DRDY pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of drdy_on_pin in reg CFG_REG_C_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_drdy_on_pin_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  *val = cfg_reg_c_m.int_mag;

  return ret;
}

/**
  * @brief  Interrupt signal on INT_DRDY pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int_on_pin in reg CFG_REG_C_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_int_on_pin_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  if(ret == 0){
    cfg_reg_c_m.int_mag_pin = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_C_M,
                              (uint8_t*)&cfg_reg_c_m, 1);
  }

  return ret;
}

/**
  * @brief  Interrupt signal on INT_DRDY pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int_on_pin in reg CFG_REG_C_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_int_on_pin_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  *val = cfg_reg_c_m.int_mag_pin;

  return ret;
}

/**
  * @brief  Interrupt generator configuration register.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change registers INT_CRTL_REG_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_int_gen_conf_set(lsm303agr_ctx_t *ctx,
                                       lsm303agr_int_crtl_reg_m_t *val)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_INT_CRTL_REG_M, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt generator configuration register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers INT_CRTL_REG_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_int_gen_conf_get(lsm303agr_ctx_t *ctx,
                                       lsm303agr_int_crtl_reg_m_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT_CRTL_REG_M,
                           (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt generator source register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers INT_SOURCE_REG_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_int_gen_source_get(lsm303agr_ctx_t *ctx,
                                         lsm303agr_int_source_reg_m_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT_SOURCE_REG_M,
                           (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  User-defined threshold value for xl interrupt event on generator.
  *         Data format is the same of output
  *         data raw: two’s complement with
  *         1LSb = 1.5mG.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that contains data to write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_int_gen_treshold_set(lsm303agr_ctx_t *ctx,
                                           uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_INT_THS_L_REG_M, buff, 2);
  return ret;
}

/**
  * @brief  User-defined threshold value for xl interrupt event on generator.
  *         Data format is the same of output
  *         data raw: two’s complement with
  *         1LSb = 1.5mG.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_int_gen_treshold_get(lsm303agr_ctx_t *ctx,
                                           uint8_t *buff)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_INT_THS_L_REG_M, buff, 2);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup  accelerometer_fifo
  * @brief       This section group all the functions concerning the xl
  *              fifo usage
  * @{
  *
  */

/**
  * @brief  FIFOenable.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fifo_en in reg CTRL_REG5_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  if(ret == 0){
    ctrl_reg5_a.fifo_en = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG5_A,
                              (uint8_t*)&ctrl_reg5_a, 1);
  }

  return ret;
}

/**
  * @brief  FIFOenable.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fifo_en in reg CTRL_REG5_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_ctrl_reg5_a_t ctrl_reg5_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG5_A,
                           (uint8_t*)&ctrl_reg5_a, 1);
  *val = ctrl_reg5_a.fifo_en;

  return ret;
}

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fth in reg FIFO_CTRL_REG_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_watermark_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_fifo_ctrl_reg_a_t fifo_ctrl_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                           (uint8_t*)&fifo_ctrl_reg_a, 1);
  if(ret == 0){
    fifo_ctrl_reg_a.fth = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                              (uint8_t*)&fifo_ctrl_reg_a, 1);
  }

  return ret;
}

/**
  * @brief  FIFO watermark level selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fth in reg FIFO_CTRL_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_watermark_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_fifo_ctrl_reg_a_t fifo_ctrl_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                           (uint8_t*)&fifo_ctrl_reg_a, 1);
  *val = fifo_ctrl_reg_a.fth;

  return ret;
}

/**
  * @brief  Trigger FIFO selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of tr in reg FIFO_CTRL_REG_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_trigger_event_set(lsm303agr_ctx_t *ctx,
                                        lsm303agr_tr_a_t val)
{
  lsm303agr_fifo_ctrl_reg_a_t fifo_ctrl_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                           (uint8_t*)&fifo_ctrl_reg_a, 1);
  if(ret == 0){
    fifo_ctrl_reg_a.tr = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                              (uint8_t*)&fifo_ctrl_reg_a, 1);
  }

  return ret;
}

/**
  * @brief  Trigger FIFO selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of tr in reg FIFO_CTRL_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_trigger_event_get(lsm303agr_ctx_t *ctx,
                                            lsm303agr_tr_a_t *val)
{
  lsm303agr_fifo_ctrl_reg_a_t fifo_ctrl_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                           (uint8_t*)&fifo_ctrl_reg_a, 1);

    switch (fifo_ctrl_reg_a.tr){
    case LSM303AGR_INT1_GEN:
      *val = LSM303AGR_INT1_GEN;
      break;
    case LSM303AGR_INT2_GEN:
      *val = LSM303AGR_INT2_GEN;
      break;
    default:
      *val = LSM303AGR_INT1_GEN;
      break;
  }
  return ret;
}

/**
  * @brief  FIFO mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fm in reg FIFO_CTRL_REG_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_mode_set(lsm303agr_ctx_t *ctx,
                                   lsm303agr_fm_a_t val)
{
  lsm303agr_fifo_ctrl_reg_a_t fifo_ctrl_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                           (uint8_t*)&fifo_ctrl_reg_a, 1);
  if(ret == 0){
    fifo_ctrl_reg_a.fm = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                              (uint8_t*)&fifo_ctrl_reg_a, 1);
  }

  return ret;
}

/**
  * @brief  FIFO mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fm in reg FIFO_CTRL_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_mode_get(lsm303agr_ctx_t *ctx,
                                   lsm303agr_fm_a_t *val)
{
  lsm303agr_fifo_ctrl_reg_a_t fifo_ctrl_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_CTRL_REG_A,
                           (uint8_t*)&fifo_ctrl_reg_a, 1);

    switch (fifo_ctrl_reg_a.fm){
    case LSM303AGR_BYPASS_MODE:
      *val = LSM303AGR_BYPASS_MODE;
      break;
    case LSM303AGR_FIFO_MODE:
      *val = LSM303AGR_FIFO_MODE;
      break;
    case LSM303AGR_DYNAMIC_STREAM_MODE:
      *val = LSM303AGR_DYNAMIC_STREAM_MODE;
      break;
    case LSM303AGR_STREAM_TO_FIFO_MODE:
      *val = LSM303AGR_STREAM_TO_FIFO_MODE;
      break;
    default:
      *val = LSM303AGR_BYPASS_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  FIFO status register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers FIFO_SRC_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_status_get(lsm303agr_ctx_t *ctx,
                                     lsm303agr_fifo_src_reg_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_SRC_REG_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  FIFO stored data level.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fss in reg FIFO_SRC_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_data_level_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_fifo_src_reg_a_t fifo_src_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_SRC_REG_A,
                           (uint8_t*)&fifo_src_reg_a, 1);
  *val = fifo_src_reg_a.fss;

  return ret;
}

/**
  * @brief  Empty FIFO status flag.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of empty in reg FIFO_SRC_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_empty_flag_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_fifo_src_reg_a_t fifo_src_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_SRC_REG_A,
                           (uint8_t*)&fifo_src_reg_a, 1);
  *val = fifo_src_reg_a.empty;

  return ret;
}

/**
  * @brief  FIFO overrun status flag.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ovrn_fifo in reg FIFO_SRC_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_ovr_flag_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_fifo_src_reg_a_t fifo_src_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_SRC_REG_A,
                           (uint8_t*)&fifo_src_reg_a, 1);
  *val = fifo_src_reg_a.ovrn_fifo;

  return ret;
}

/**
  * @brief  FIFO watermark status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of wtm in reg FIFO_SRC_REG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_fifo_fth_flag_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_fifo_src_reg_a_t fifo_src_reg_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_FIFO_SRC_REG_A,
                           (uint8_t*)&fifo_src_reg_a, 1);
  *val = fifo_src_reg_a.wtm;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup  tap_generator
  * @brief       This section group all the functions that manage the tap and
  *              double tap event generation
  * @{
  *
  */

/**
  * @brief  Tap/Double Tap generator configuration register.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change registers CLICK_CFG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_tap_conf_set(lsm303agr_ctx_t *ctx,
                               lsm303agr_click_cfg_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_write_reg(ctx, LSM303AGR_CLICK_CFG_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Tap/Double Tap generator configuration register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers CLICK_CFG_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_tap_conf_get(lsm303agr_ctx_t *ctx,
                               lsm303agr_click_cfg_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_CLICK_CFG_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Tap/Double Tap generator source register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers CLICK_SRC_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_tap_source_get(lsm303agr_ctx_t *ctx,
                                 lsm303agr_click_src_a_t *val)
{
  int32_t ret;
  ret = lsm303agr_read_reg(ctx, LSM303AGR_CLICK_SRC_A, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  User-defined threshold value for Tap/Double Tap event.
  *         (1 LSB = full scale/128)[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ths in reg CLICK_THS_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_tap_threshold_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_click_ths_a_t click_ths_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CLICK_THS_A,
                           (uint8_t*)&click_ths_a, 1);
  if(ret == 0){
    click_ths_a.ths = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CLICK_THS_A,
                              (uint8_t*)&click_ths_a, 1);
  }

  return ret;
}

/**
  * @brief  User-defined threshold value for Tap/Double Tap event.
  *         (1 LSB = full scale/128)[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ths in reg CLICK_THS_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_tap_threshold_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_click_ths_a_t click_ths_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CLICK_THS_A,
                           (uint8_t*)&click_ths_a, 1);
  *val = click_ths_a.ths;

  return ret;
}

/**
  * @brief  The maximum time (1 LSB = 1/ODR) interval that can
  *         elapse between the start of the click-detection procedure
  *         and when the acceleration falls back below the threshold.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of tli in reg TIME_LIMIT_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_shock_dur_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_time_limit_a_t time_limit_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_TIME_LIMIT_A,
                           (uint8_t*)&time_limit_a, 1);
  if(ret == 0){
    time_limit_a.tli = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_TIME_LIMIT_A,
                              (uint8_t*)&time_limit_a, 1);
  }

  return ret;
}

/**
  * @brief  The maximum time (1 LSB = 1/ODR) interval that can
  *         elapse between the start of the click-detection procedure
  *         and when the acceleration falls back below the threshold.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of tli in reg TIME_LIMIT_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_shock_dur_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_time_limit_a_t time_limit_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_TIME_LIMIT_A,
                           (uint8_t*)&time_limit_a, 1);
  *val = time_limit_a.tli;

  return ret;
}

/**
  * @brief  The time (1 LSB = 1/ODR) interval that starts after the
  *         first click detection where the click-detection procedure
  *         is disabled, in cases where the device is configured for
  *         double-click detection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of tla in reg TIME_LATENCY_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_quiet_dur_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_time_latency_a_t time_latency_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_TIME_LATENCY_A,
                           (uint8_t*)&time_latency_a, 1);
  if(ret == 0){
    time_latency_a.tla = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_TIME_LATENCY_A,
                              (uint8_t*)&time_latency_a, 1);
  }

  return ret;
}

/**
  * @brief  The time (1 LSB = 1/ODR) interval that starts after the first click
  *         detection where the click-detection procedure is disabled, in cases
  *         where the device is configured for double-click detection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of tla in reg TIME_LATENCY_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_quiet_dur_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_time_latency_a_t time_latency_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_TIME_LATENCY_A,
                           (uint8_t*)&time_latency_a, 1);
  *val = time_latency_a.tla;

  return ret;
}

/**
  * @brief  The maximum interval of time (1 LSB = 1/ODR) that can elapse after
  *         the end of the latency interval in which the click-detection
  *         procedure can start, in cases where the device is configured for
  *         double-click detection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of tw in reg TIME_WINDOW_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_double_tap_timeout_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_time_window_a_t time_window_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_TIME_WINDOW_A,
                           (uint8_t*)&time_window_a, 1);
  if(ret == 0){
    time_window_a.tw = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_TIME_WINDOW_A,
                              (uint8_t*)&time_window_a, 1);
  }

  return ret;
}

/**
  * @brief  The maximum interval of time (1 LSB = 1/ODR) that can elapse after
  *         the end of the latency interval in which the click-detection
  *         procedure can start, in cases where the device is configured for
  *         double-click detection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of tw in reg TIME_WINDOW_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_double_tap_timeout_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_time_window_a_t time_window_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_TIME_WINDOW_A,
                           (uint8_t*)&time_window_a, 1);
  *val = time_window_a.tw;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup   activity_inactivity_xl
  * @brief        This section group all the functions concerning activity
  *               inactivity functionality foe accelerometer
  * @{
  *
  */

/**
  * @brief  Sleep-to-wake, return-to-sleep activation
  *         threshold in low-power mode.[set]
  *         1 LSb = 16mg@2g / 32mg@4g / 62mg@8g / 186mg@16g
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of acth in reg ACT_THS_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_act_threshold_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_act_ths_a_t act_ths_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_ACT_THS_A,
                           (uint8_t*)&act_ths_a, 1);
  if(ret == 0){
    act_ths_a.acth = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_ACT_THS_A,
                              (uint8_t*)&act_ths_a, 1);
  }

  return ret;
}

/**
  * @brief  Sleep-to-wake, return-to-sleep activation
  *          threshold in low-power mode.[get]
  *          1 LSb = 16mg@2g / 32mg@4g / 62mg@8g / 186mg@16g
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of acth in reg ACT_THS_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_act_threshold_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_act_ths_a_t act_ths_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_ACT_THS_A,
                           (uint8_t*)&act_ths_a, 1);
  *val = act_ths_a.acth;

  return ret;
}

/**
  * @brief  Sleep-to-wake, return-to-sleep duration = (8*1[LSb]+1)/ODR.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of actd in reg ACT_DUR_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_act_timeout_set(lsm303agr_ctx_t *ctx, uint8_t val)
{
  lsm303agr_act_dur_a_t act_dur_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_ACT_DUR_A,
                           (uint8_t*)&act_dur_a, 1);
  if(ret == 0){
    act_dur_a.actd = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_ACT_DUR_A,
                              (uint8_t*)&act_dur_a, 1);
  }

  return ret;
}

/**
  * @brief  Sleep-to-wake, return-to-sleep duration = (8*1[LSb]+1)/ODR.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of actd in reg ACT_DUR_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_act_timeout_get(lsm303agr_ctx_t *ctx, uint8_t *val)
{
  lsm303agr_act_dur_a_t act_dur_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_ACT_DUR_A,
                           (uint8_t*)&act_dur_a, 1);
  *val = act_dur_a.actd;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup  serial_interface
  * @brief       This section group all the functions concerning serial
  *              interface management
  * @{
  *
  */

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sim in reg CTRL_REG4_A
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_spi_mode_set(lsm303agr_ctx_t *ctx,
                                  lsm303agr_sim_a_t val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);
  if(ret == 0){
    ctrl_reg4_a.spi_enable = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CTRL_REG4_A,
                              (uint8_t*)&ctrl_reg4_a, 1);
  }

  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sim in reg CTRL_REG4_A.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_xl_spi_mode_get(lsm303agr_ctx_t *ctx,
                                  lsm303agr_sim_a_t *val)
{
  lsm303agr_ctrl_reg4_a_t ctrl_reg4_a;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CTRL_REG4_A,
                           (uint8_t*)&ctrl_reg4_a, 1);

    switch (ctrl_reg4_a.spi_enable){
    case LSM303AGR_SPI_4_WIRE:
      *val = LSM303AGR_SPI_4_WIRE;
      break;
    case LSM303AGR_SPI_3_WIRE:
      *val = LSM303AGR_SPI_3_WIRE;
      break;
    default:
      *val = LSM303AGR_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief  Enable/Disable I2C interface.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of i2c_dis in reg CFG_REG_C_M
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_i2c_interface_set(lsm303agr_ctx_t *ctx,
                                        lsm303agr_i2c_dis_m_t val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);
  if(ret == 0){
    cfg_reg_c_m.i2c_dis = (uint8_t)val;
    ret = lsm303agr_write_reg(ctx, LSM303AGR_CFG_REG_C_M,
                              (uint8_t*)&cfg_reg_c_m, 1);
  }

  return ret;
}

/**
  * @brief  Enable/Disable I2C interface.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of i2c_dis in reg CFG_REG_C_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm303agr_mag_i2c_interface_get(lsm303agr_ctx_t *ctx,
                                        lsm303agr_i2c_dis_m_t *val)
{
  lsm303agr_cfg_reg_c_m_t cfg_reg_c_m;
  int32_t ret;

  ret = lsm303agr_read_reg(ctx, LSM303AGR_CFG_REG_C_M,
                           (uint8_t*)&cfg_reg_c_m, 1);

    switch (cfg_reg_c_m.i2c_dis){
    case LSM303AGR_I2C_ENABLE:
      *val = LSM303AGR_I2C_ENABLE;
      break;
    case LSM303AGR_I2C_DISABLE:
      *val = LSM303AGR_I2C_DISABLE;
      break;
    default:
      *val = LSM303AGR_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/