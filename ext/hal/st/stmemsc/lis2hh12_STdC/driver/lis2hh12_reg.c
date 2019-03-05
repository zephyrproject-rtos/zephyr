/*
 ******************************************************************************
 * @file    lis2hh12_reg.c
 * @author  Sensors Software Solution Team
 * @brief   LIS2HH12 driver file
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
#include "lis2hh12_reg.h"

/**
  * @defgroup    LIS2HH12
  * @brief       This file provides a set of functions needed to drive the
  *              lis2hh12 enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup    LIS2HH12_Interfaces_Functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
  *              MANDATORY: return 0 -> no Error.
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
  * @retval       interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2hh12_read_reg(lis2hh12_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @retval       interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2hh12_write_reg(lis2hh12_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LIS2HH12_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t lis2hh12_from_fs2g_to_mg(int16_t lsb)
{
  return ((float_t)lsb *0.061f);
}

float_t lis2hh12_from_fs4g_to_mg(int16_t lsb)
{
  return ((float_t)lsb *0.122f);
}

float_t lis2hh12_from_fs8g_to_mg(int16_t lsb)
{
  return ((float_t)lsb *0.244f);
}

float_t lis2hh12_from_lsb_to_celsius(int16_t lsb)
{
  return (((float_t)lsb / 8.0f) + 25.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Data_generation
  * @brief       This section groups all the functions concerning
  *              data generation
  * @{
  *
  */

/**
  * @brief   Enable accelerometer axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer’s X-axis output enable.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_axis_set(lis2hh12_ctx_t *ctx, lis2hh12_xl_axis_t val)
{
  lis2hh12_ctrl1_t ctrl1;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  if(ret == 0) {
    ctrl1.xen  = val.xen;
    ctrl1.yen  = val.yen;
    ctrl1.zen  = val.zen;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  }
  return ret;
}

/**
  * @brief   Enable accelerometer axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer’s X-axis output enable.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_axis_get(lis2hh12_ctx_t *ctx, lis2hh12_xl_axis_t *val)
{
  lis2hh12_ctrl1_t ctrl1;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  val->xen = ctrl1.xen;
  val->yen = ctrl1.yen;
  val->zen = ctrl1.zen;

  return ret;
}

/**
  * @brief  Blockdataupdate.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of bdu in reg CTRL1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_block_data_update_set(lis2hh12_ctx_t *ctx, uint8_t val)
{
  lis2hh12_ctrl1_t ctrl1;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  if(ret == 0){
    ctrl1.bdu = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  }
  return ret;
}

/**
  * @brief  Blockdataupdate.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bdu in reg CTRL1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_block_data_update_get(lis2hh12_ctx_t *ctx, uint8_t *val)
{
  lis2hh12_ctrl1_t ctrl1;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  *val = (uint8_t)ctrl1.bdu;

  return ret;
}

/**
  * @brief   Accelerometer data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "odr" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_data_rate_set(lis2hh12_ctx_t *ctx,
                                  lis2hh12_xl_data_rate_t val)
{
  lis2hh12_ctrl1_t ctrl1;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  if(ret == 0){
    ctrl1.odr = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odr in reg CTRL1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_data_rate_get(lis2hh12_ctx_t *ctx,
                                  lis2hh12_xl_data_rate_t *val)
{
  lis2hh12_ctrl1_t ctrl1;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  switch (ctrl1.odr){
    case LIS2HH12_XL_ODR_OFF:
      *val = LIS2HH12_XL_ODR_OFF;
      break;
    case LIS2HH12_XL_ODR_10Hz:
      *val = LIS2HH12_XL_ODR_10Hz;
      break;
    case LIS2HH12_XL_ODR_50Hz:
      *val = LIS2HH12_XL_ODR_50Hz;
      break;
    case LIS2HH12_XL_ODR_100Hz:
      *val = LIS2HH12_XL_ODR_100Hz;
      break;
    case LIS2HH12_XL_ODR_200Hz:
      *val = LIS2HH12_XL_ODR_200Hz;
      break;
    case LIS2HH12_XL_ODR_400Hz:
      *val = LIS2HH12_XL_ODR_400Hz;
      break;
    case LIS2HH12_XL_ODR_800Hz:
      *val = LIS2HH12_XL_ODR_800Hz;
      break;
    default:
      *val = LIS2HH12_XL_ODR_OFF;
      break;
  }
  return ret;
}

/**
  * @brief   Accelerometer full-scale selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fs" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_full_scale_set(lis2hh12_ctx_t *ctx,
                                   lis2hh12_xl_fs_t val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  if(ret == 0){
    ctrl4.fs = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fs in reg CTRL4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_full_scale_get(lis2hh12_ctx_t *ctx,
                                   lis2hh12_xl_fs_t *val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  switch (ctrl4.fs){
    case LIS2HH12_2g:
      *val = LIS2HH12_2g;
      break;
    case LIS2HH12_4g:
      *val = LIS2HH12_4g;
      break;
    case LIS2HH12_8g:
      *val = LIS2HH12_8g;
      break;
    default:
      *val = LIS2HH12_2g;
      break;
  }
  return ret;
}

/**
  * @brief   Decimation of acceleration data on OUT REG and FIFO.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "dec" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_decimation_set(lis2hh12_ctx_t *ctx, lis2hh12_dec_t val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  if(ret == 0){
    ctrl5.dec = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  }
  return ret;
}

/**
  * @brief   Decimation of acceleration data on OUT REG and FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dec in reg CTRL5.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_decimation_get(lis2hh12_ctx_t *ctx, lis2hh12_dec_t *val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  switch (ctrl5.dec){
    case LIS2HH12_NO_DECIMATION:
      *val = LIS2HH12_NO_DECIMATION;
      break;
    case LIS2HH12_EVERY_2_SAMPLES:
      *val = LIS2HH12_EVERY_2_SAMPLES;
      break;
    case LIS2HH12_EVERY_4_SAMPLES:
      *val = LIS2HH12_EVERY_4_SAMPLES;
      break;
    case LIS2HH12_EVERY_8_SAMPLES:
      *val = LIS2HH12_EVERY_8_SAMPLES;
      break;
    default:
      *val = LIS2HH12_NO_DECIMATION;
      break;
  }
  return ret;
}

/**
  * @brief   New data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "zyxda" in reg STATUS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_flag_data_ready_get(lis2hh12_ctx_t *ctx, uint8_t *val)
{
  lis2hh12_status_t status;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_STATUS, (uint8_t*)&status, 1);
  *val = status.zyxda;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Dataoutput
  * @brief       This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief   Temperature data output register (r). L and H registers together
  *          express a 16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_temperature_raw_get(lis2hh12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2hh12_read_reg(ctx, LIS2HH12_TEMP_L, buff, 2);
  return ret;
}

/**
  * @brief   Linear acceleration output register. The value is expressed
  *          as a 16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_acceleration_raw_get(lis2hh12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2hh12_read_reg(ctx, LIS2HH12_OUT_X_L, buff,
                            6);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Common
  * @brief       This section groups common usefull functions.
  * @{
  *
  */

/**
  * @brief  DeviceWhoamI.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_dev_id_get(lis2hh12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2hh12_read_reg(ctx, LIS2HH12_WHO_AM_I, buff,
                            1);
  return ret;
}

/**
  * @brief   Software reset. Restore the default values
  *          in user registers.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of soft_reset in reg CTRL5.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_dev_reset_set(lis2hh12_ctx_t *ctx, uint8_t val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  if(ret == 0){
    ctrl5.soft_reset = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  }
  return ret;
}

/**
  * @brief   Software reset. Restore the default values in
  *          user registers.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of soft_reset in reg CTRL5.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_dev_reset_get(lis2hh12_ctx_t *ctx, uint8_t *val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  *val = (uint8_t)ctrl5.soft_reset;

  return ret;
}

/**
  * @brief   Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of boot in reg CTRL6.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_dev_boot_set(lis2hh12_ctx_t *ctx, uint8_t val)
{
  lis2hh12_ctrl6_t ctrl6;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL6, (uint8_t*)&ctrl6, 1);
  if(ret == 0){
    ctrl6.boot = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL6, (uint8_t*)&ctrl6, 1);
  }
  return ret;
}

/**
  * @brief   Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of boot in reg CTRL6.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_dev_boot_get(lis2hh12_ctx_t *ctx, uint8_t *val)
{
  lis2hh12_ctrl6_t ctrl6;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL6, (uint8_t*)&ctrl6, 1);
  *val = (uint8_t)ctrl6.boot;

  return ret;
}

/**
  * @brief   Device status register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     X-axis new data available.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_dev_status_get(lis2hh12_ctx_t *ctx,
                                lis2hh12_status_reg_t *val)
{
  lis2hh12_status_t status;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_STATUS, (uint8_t*)&status, 1);
  val->xda = status.xda;
  val->yda = status.yda;
  val->zda = status.zda;
  val->zyxda = status.zyxda;
  val->_xor = status._xor;
  val->yor = status.yor;
  val->zor = status.zor;
  val->zyxor = status.zyxor;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Filters
  * @brief       This section group all the functions concerning the
  *              filters configuration
  * @{
  *
  */

/**
  * @brief   Accelerometer filter routing on interrupt generators.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "hpis" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_int_path_set(lis2hh12_ctx_t *ctx,
                                        lis2hh12_xl_hp_path_t val)
{
  lis2hh12_ctrl2_t ctrl2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  if(ret == 0){
    ctrl2.hpis = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer filter routing on interrupt generators.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hpis in reg CTRL2.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_int_path_get(lis2hh12_ctx_t *ctx,
                                        lis2hh12_xl_hp_path_t *val)
{
  lis2hh12_ctrl2_t ctrl2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  switch (ctrl2.hpis){
    case LIS2HH12_HP_DISABLE:
      *val = LIS2HH12_HP_DISABLE;
      break;
    case LIS2HH12_HP_ON_INT_GEN_1:
      *val = LIS2HH12_HP_ON_INT_GEN_1;
      break;
    case LIS2HH12_HP_ON_INT_GEN_2:
      *val = LIS2HH12_HP_ON_INT_GEN_2;
      break;
    default:
      *val = LIS2HH12_HP_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   Accelerometer output filter path configuration.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fds" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_out_path_set(lis2hh12_ctx_t *ctx,
                                        lis2hh12_xl_out_path_t val)
{
  lis2hh12_ctrl1_t ctrl1;
  lis2hh12_ctrl2_t ctrl2;

  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);

  if(ret == 0){
    ctrl1.hr = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  }
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  }
  if(ret == 0){
    ctrl2.fds = ( (uint8_t) val & 0x02U ) >> 1;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer output filter path configuration.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fds in reg CTRL2.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_out_path_get(lis2hh12_ctx_t *ctx,
                                        lis2hh12_xl_out_path_t *val)
{
  lis2hh12_ctrl1_t ctrl1;
  lis2hh12_ctrl2_t ctrl2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL1, (uint8_t*)&ctrl1, 1);
  if(ret == 0){

    ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);

    switch ((ctrl2.fds << 1) | ctrl1.hr){
      case LIS2HH12_BYPASSED:
        *val = LIS2HH12_BYPASSED;
        break;
      case LIS2HH12_FILT_HP:
        *val = LIS2HH12_FILT_HP;
        break;
      case LIS2HH12_FILT_LP:
        *val = LIS2HH12_FILT_LP;
        break;
      default:
        *val = LIS2HH12_BYPASSED;
        break;
    }
  }
  return ret;
}

/**
  * @brief   Accelerometer digital filter high pass cutoff
  *          frequency selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "hpm" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_hp_bandwidth_set(lis2hh12_ctx_t *ctx,
                                            lis2hh12_xl_hp_bw_t val)
{
  lis2hh12_ctrl2_t ctrl2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  if(ret == 0){
    ctrl2.hpm = (uint8_t) val & 0x01U;
    ctrl2.dfc = (((uint8_t) val  & 0x30U )>> 4);
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer digital filter high pass cutoff frequency
  *          selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hpm in reg CTRL2.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_hp_bandwidth_get(lis2hh12_ctx_t *ctx,
                                            lis2hh12_xl_hp_bw_t *val)
{
  lis2hh12_ctrl2_t ctrl2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  switch ((ctrl2.dfc << 4) | ctrl2.hpm){
    case LIS2HH12_HP_ODR_DIV_50:
      *val = LIS2HH12_HP_ODR_DIV_50;
      break;
    case LIS2HH12_HP_ODR_DIV_100:
      *val = LIS2HH12_HP_ODR_DIV_100;
      break;
    case LIS2HH12_HP_ODR_DIV_9:
      *val = LIS2HH12_HP_ODR_DIV_9;
      break;
    case LIS2HH12_HP_ODR_DIV_400:
      *val = LIS2HH12_HP_ODR_DIV_400;
      break;
    case LIS2HH12_HP_ODR_DIV_50_REF_MD:
      *val = LIS2HH12_HP_ODR_DIV_50_REF_MD;
      break;
    case LIS2HH12_HP_ODR_DIV_100_REF_MD:
      *val = LIS2HH12_HP_ODR_DIV_100_REF_MD;
      break;
    case LIS2HH12_HP_ODR_DIV_9_REF_MD:
      *val = LIS2HH12_HP_ODR_DIV_9_REF_MD;
      break;
    case LIS2HH12_HP_ODR_DIV_400_REF_MD:
      *val = LIS2HH12_HP_ODR_DIV_400_REF_MD;
      break;
    default:
      *val = LIS2HH12_HP_ODR_DIV_50;
      break;
  }
  return ret;
}

/**
  * @brief   Accelerometer digital filter low pass cutoff frequency
  *          selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "dfc" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_low_bandwidth_set(lis2hh12_ctx_t *ctx,
                                             lis2hh12_xl_lp_bw_t val)
{
  lis2hh12_ctrl2_t ctrl2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  if(ret == 0){
    ctrl2.dfc = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer digital filter low pass cutoff frequency
  *          selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dfc in reg CTRL2.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_low_bandwidth_get(lis2hh12_ctx_t *ctx,
                                             lis2hh12_xl_lp_bw_t *val)
{
  lis2hh12_ctrl2_t ctrl2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL2, (uint8_t*)&ctrl2, 1);
  switch (ctrl2.dfc){
    case LIS2HH12_LP_ODR_DIV_50:
      *val = LIS2HH12_LP_ODR_DIV_50;
      break;
    case LIS2HH12_LP_ODR_DIV_100:
      *val = LIS2HH12_LP_ODR_DIV_100;
      break;
    case LIS2HH12_LP_ODR_DIV_9:
      *val = LIS2HH12_LP_ODR_DIV_9;
      break;
    case LIS2HH12_LP_ODR_DIV_400:
      *val = LIS2HH12_LP_ODR_DIV_400;
      break;
    default:
      *val = LIS2HH12_LP_ODR_DIV_50;
      break;
  }
  return ret;
}

/**
  * @brief   Set anti-aliasing filter bandwidth.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "bw_scale_odr" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_aalias_bandwidth_set(lis2hh12_ctx_t *ctx,
                                                lis2hh12_xl_filt_aa_bw_t val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  if(ret == 0){
    ctrl4.bw_scale_odr = ((( uint8_t) val & 0x10U ) >> 4);
    ctrl4.bw = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  }
  return ret;
}

/**
  * @brief   Set anti-aliasing filter bandwidth.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bw_scale_odr in reg CTRL4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_aalias_bandwidth_get(lis2hh12_ctx_t *ctx,
                                                lis2hh12_xl_filt_aa_bw_t *val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  switch ( (ctrl4.bw_scale_odr << 4) | ctrl4.bw){
    case LIS2HH12_AUTO:
      *val = LIS2HH12_AUTO;
      break;
    case LIS2HH12_408Hz:
      *val = LIS2HH12_408Hz;
      break;
    case LIS2HH12_211Hz:
      *val = LIS2HH12_211Hz;
      break;
    case LIS2HH12_105Hz:
      *val = LIS2HH12_105Hz;
      break;
    case LIS2HH12_50Hz:
      *val = LIS2HH12_50Hz;
      break;     
    default:
      *val = LIS2HH12_AUTO;
      break;
  }
  return ret;
}

/**
  * @brief   Reference value for acelerometer digital high-pass filter.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data to be write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_reference_set(lis2hh12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2hh12_write_reg(ctx, LIS2HH12_XL_REFERENCE, buff, 6);
  return ret;
}

/**
  * @brief   Reference value for acelerometer digital high-pass filter.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_filter_reference_get(lis2hh12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2hh12_read_reg(ctx, LIS2HH12_XL_REFERENCE, buff, 6);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Serial_interface
  * @brief       This section groups all the functions concerning main
  *              serial interface management (not auxiliary)
  * @{
  *
  */

/**
  * @brief   SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "sim" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_spi_mode_set(lis2hh12_ctx_t *ctx, lis2hh12_sim_t val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  if(ret == 0){
    ctrl4.sim = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  }
  return ret;
}

/**
  * @brief   SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sim in reg CTRL4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_spi_mode_get(lis2hh12_ctx_t *ctx, lis2hh12_sim_t *val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  switch (ctrl4.sim){
    case LIS2HH12_SPI_4_WIRE:
      *val = LIS2HH12_SPI_4_WIRE;
      break;
    case LIS2HH12_SPI_3_WIRE:
      *val = LIS2HH12_SPI_3_WIRE;
      break;
    default:
      *val = LIS2HH12_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief   Disable I2C interface.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "i2c_disable" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_i2c_interface_set(lis2hh12_ctx_t *ctx,
                                   lis2hh12_i2c_dis_t val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  if(ret == 0){
    ctrl4.i2c_disable = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  }
  return ret;
}

/**
  * @brief   Disable I2C interface.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of i2c_disable in reg CTRL4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_i2c_interface_get(lis2hh12_ctx_t *ctx,
                                   lis2hh12_i2c_dis_t *val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  switch (ctrl4.i2c_disable){
    case LIS2HH12_I2C_ENABLE:
      *val = LIS2HH12_I2C_ENABLE;
      break;
    case LIS2HH12_I2C_DISABLE:
      *val = LIS2HH12_I2C_DISABLE;
      break;
    default:
      *val = LIS2HH12_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @brief   Register address automatically incremented during a
  *          multiple byte access with a serial interface.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "if_add_inc" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_auto_increment_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_auto_inc_t val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  if(ret == 0){
    ctrl4.if_add_inc = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  }
  return ret;
}

/**
  * @brief   Register address automatically incremented during a multiple
  *          byte access with a serial interface.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of if_add_inc in reg CTRL4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_auto_increment_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_auto_inc_t *val)
{
  lis2hh12_ctrl4_t ctrl4;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL4, (uint8_t*)&ctrl4, 1);
  switch (ctrl4.if_add_inc){
    case LIS2HH12_DISABLE:
      *val = LIS2HH12_DISABLE;
      break;
    case LIS2HH12_ENABLE:
      *val = LIS2HH12_ENABLE;
      break;
    default:
      *val = LIS2HH12_DISABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Interrupt_pins
  * @brief       This section groups all the functions that manage
  *              interrupt pins
  * @{
  *
  */

/**
  * @brief   Route a signal on INT 1 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer data ready on INT 1 pin.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_int1_route_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_pin_int1_route_t val)
{
  lis2hh12_ctrl3_t ctrl3;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL3, (uint8_t*)&ctrl3, 1);
  if(ret == 0) {
    ctrl3.int1_drdy  = val.int1_drdy;
    ctrl3.int1_fth  = val.int1_fth;
    ctrl3.int1_ovr  = val.int1_ovr;
    ctrl3.int1_ig1  = val.int1_ig1;
    ctrl3.int1_ig2  = val.int1_ig2;
    ctrl3.int1_inact  = val.int1_inact;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL3, (uint8_t*)&ctrl3, 1);
  }
  return ret;
}

/**
  * @brief   Route a signal on INT 1 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer data ready on INT 1 pin.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_int1_route_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_pin_int1_route_t *val)
{
  lis2hh12_ctrl3_t ctrl3;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL3, (uint8_t*)&ctrl3, 1);
  val->int1_drdy = ctrl3.int1_drdy;
  val->int1_fth = ctrl3.int1_fth;
  val->int1_ovr = ctrl3.int1_ovr;
  val->int1_ig1 = ctrl3.int1_ig1;
  val->int1_ig2 = ctrl3.int1_ig2;
  val->int1_inact = ctrl3.int1_inact;

  return ret;
}

/**
  * @brief   Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "pp_od" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_mode_set(lis2hh12_ctx_t *ctx, lis2hh12_pp_od_t val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  if(ret == 0){
    ctrl5.pp_od = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  }
  return ret;
}

/**
  * @brief   Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of pp_od in reg CTRL5.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_mode_get(lis2hh12_ctx_t *ctx, lis2hh12_pp_od_t *val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  switch (ctrl5.pp_od){
    case LIS2HH12_PUSH_PULL:
      *val = LIS2HH12_PUSH_PULL;
      break;
    case LIS2HH12_OPEN_DRAIN:
      *val = LIS2HH12_OPEN_DRAIN;
      break;
    default:
      *val = LIS2HH12_PUSH_PULL;
      break;
  }
  return ret;
}

/**
  * @brief   Interrupt active-high/low.Interrupt active-high/low.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "h_lactive" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_polarity_set(lis2hh12_ctx_t *ctx,
                                  lis2hh12_pin_pol_t val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  if(ret == 0){
    ctrl5.h_lactive = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  }
  return ret;
}

/**
  * @brief   Interrupt active-high/low.Interrupt active-high/low.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of h_lactive in reg CTRL5.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_polarity_get(lis2hh12_ctx_t *ctx,
                                  lis2hh12_pin_pol_t *val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  switch (ctrl5.h_lactive){
    case LIS2HH12_ACTIVE_HIGH:
      *val = LIS2HH12_ACTIVE_HIGH;
      break;
    case LIS2HH12_ACTIVE_LOW:
      *val = LIS2HH12_ACTIVE_LOW;
      break;
    default:
      *val = LIS2HH12_ACTIVE_HIGH;
      break;
  }
  return ret;
}

/**
  * @brief   Route a signal on INT 2 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer data ready on INT2 pin.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_int2_route_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_pin_int2_route_t val)
{
  lis2hh12_ctrl6_t ctrl6;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL6, (uint8_t*)&ctrl6, 1);
  if(ret == 0) {
    ctrl6.int2_drdy   = val.int2_drdy;
    ctrl6.int2_fth    = val.int2_fth;
    ctrl6.int2_empty  = val.int2_empty;
    ctrl6.int2_ig1    = val.int2_ig1;
    ctrl6.int2_ig2    = val.int2_ig2;
    ctrl6.int2_boot   = val.int2_boot;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL6, (uint8_t*)&ctrl6, 1);
  }
  return ret;
}

/**
  * @brief   Route a signal on INT 2 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer data ready on INT2 pin.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_int2_route_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_pin_int2_route_t *val)
{
  lis2hh12_ctrl6_t ctrl6;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL6, (uint8_t*)&ctrl6, 1);
  val->int2_drdy  = ctrl6.int2_drdy;
  val->int2_fth   = ctrl6.int2_fth;
  val->int2_empty = ctrl6.int2_empty;
  val->int2_ig1   = ctrl6.int2_ig1;
  val->int2_ig2   = ctrl6.int2_ig2;
  val->int2_boot  = ctrl6.int2_boot;
  return ret;
}

/**
  * @brief    Latched/pulsed interrupt.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "lir" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_notification_set(lis2hh12_ctx_t *ctx,
                                      lis2hh12_lir_t val)
{
  lis2hh12_ctrl7_t ctrl7;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  if(ret == 0){
    ctrl7.lir = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  }
  return ret;
}

/**
  * @brief    Latched/pulsed interrupt.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of lir in reg CTRL7.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_notification_get(lis2hh12_ctx_t *ctx,
                                      lis2hh12_lir_t *val)
{
  lis2hh12_ctrl7_t ctrl7;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  switch (ctrl7.lir){
    case LIS2HH12_INT_PULSED:
      *val = LIS2HH12_INT_PULSED;
      break;
    case LIS2HH12_INT_LATCHED:
      *val = LIS2HH12_INT_LATCHED;
      break;
    default:
      *val = LIS2HH12_INT_PULSED;
      break;
  }
  return ret;
}

/**
  * @brief   AND/OR combination of accelerometer’s interrupt events.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "aoi" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_logic_set(lis2hh12_ctx_t *ctx,
                               lis2hh12_pin_logic_t val)
{
  lis2hh12_ig_cfg1_t ig_cfg1;
  lis2hh12_ig_cfg2_t ig_cfg2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG1, (uint8_t*)&ig_cfg1, 1);
  if(ret == 0){
    ig_cfg1.aoi = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_CFG1, (uint8_t*)&ig_cfg1, 1);
  }
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }
  if(ret == 0){
    ig_cfg2.aoi = (((uint8_t) val & 0x02U) >> 1);
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }

  return ret;
}

/**
  * @brief   AND/OR combination of accelerometer’s interrupt events.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of aoi in reg IG_CFG1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_pin_logic_get(lis2hh12_ctx_t *ctx,
                               lis2hh12_pin_logic_t *val)
{
  lis2hh12_ig_cfg1_t ig_cfg1;
  lis2hh12_ig_cfg2_t ig_cfg2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG1, (uint8_t*)&ig_cfg1, 1);
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }

  switch ( (ig_cfg2.aoi << 1) |  ig_cfg1.aoi){
    case LIS2HH12_IG1_OR_IG2_OR:
      *val = LIS2HH12_IG1_OR_IG2_OR;
      break;
    case LIS2HH12_IG1_AND_IG2_OR:
      *val = LIS2HH12_IG1_AND_IG2_OR;
      break;
    case LIS2HH12_IG1_OR_IG2_AND:
      *val = LIS2HH12_IG1_OR_IG2_AND;
      break;
    case LIS2HH12_IG1_AND_IG2_AND:
      *val = LIS2HH12_IG1_AND_IG2_AND;
      break;
    default:
      *val = LIS2HH12_IG1_OR_IG2_OR;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Interrupt_on_threshold
  * @brief       This section group all the functions concerning the
  *              interrupt on threshold configuration
  * @{
  *
  */

/**
  * @brief   Decrement or reset counter mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "dcrm" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_mode_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_dcrm_t val)
{
  lis2hh12_ctrl7_t ctrl7;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  if(ret == 0){
    ctrl7.dcrm = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  }
  return ret;
}

/**
  * @brief   Decrement or reset counter mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dcrm in reg CTRL7.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_mode_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_dcrm_t *val)
{
  lis2hh12_ctrl7_t ctrl7;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  switch (ctrl7.dcrm){
    case LIS2HH12_RESET_MODE:
      *val = LIS2HH12_RESET_MODE;
      break;
    case LIS2HH12_DECREMENT_MODE:
      *val = LIS2HH12_DECREMENT_MODE;
      break;
    default:
      *val = LIS2HH12_RESET_MODE;
      break;
  }
  return ret;
}

/**
  * @brief   Enable interrupt generation on threshold event.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Enable interrupt generation on accelerometer’s
  *                X-axis low event.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_axis_set(lis2hh12_ctx_t *ctx,
                                    lis2hh12_xl_trshld_en_t val)
{
  lis2hh12_ig_cfg1_t ig_cfg1;
  lis2hh12_ig_cfg2_t ig_cfg2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG1, (uint8_t*)&ig_cfg1, 1);
  if(ret == 0) {
    ig_cfg1.xlie  = (uint8_t)val.ig1_xlie;
    ig_cfg1.xhie  = (uint8_t)val.ig1_xhie;
    ig_cfg1.ylie  = (uint8_t)val.ig1_ylie;
    ig_cfg1.yhie  = (uint8_t)val.ig1_yhie;
    ig_cfg1.zlie  = (uint8_t)val.ig1_zlie;
    ig_cfg1.zhie  = (uint8_t)val.ig1_zhie;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_CFG1, (uint8_t*)&ig_cfg1, 1);
  }
  if(ret == 0) {
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }
  if(ret == 0) {
    ig_cfg2.xlie  = (uint8_t)val.ig2_xlie;
    ig_cfg2.xhie  = (uint8_t)val.ig2_xhie;
    ig_cfg2.ylie  = (uint8_t)val.ig2_ylie;
    ig_cfg2.yhie  = (uint8_t)val.ig2_yhie;
    ig_cfg2.zlie  = (uint8_t)val.ig2_zlie;
    ig_cfg2.zhie  = (uint8_t)val.ig2_zhie;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }
  return ret;
}

/**
  * @brief   Enable interrupt generation on threshold event.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Enable interrupt generation on accelerometer’s
  *                X-axis low event.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_axis_get(lis2hh12_ctx_t *ctx,
                                    lis2hh12_xl_trshld_en_t *val)
{
  lis2hh12_ig_cfg1_t ig_cfg1;
  lis2hh12_ig_cfg2_t ig_cfg2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG1,(uint8_t*)&ig_cfg1, 1);

  if(ret == 0) {
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }
  val->ig1_xlie = ig_cfg1.xlie;
  val->ig1_xhie = ig_cfg1.xhie;
  val->ig1_ylie = ig_cfg1.ylie;
  val->ig1_yhie = ig_cfg1.yhie;
  val->ig1_zlie = ig_cfg1.zlie;
  val->ig1_zhie = ig_cfg1.zhie;

  val->ig2_xlie = ig_cfg2.xlie;
  val->ig2_xhie = ig_cfg2.xhie;
  val->ig2_ylie = ig_cfg2.ylie;
  val->ig2_yhie = ig_cfg2.yhie;
  val->ig2_zlie = ig_cfg2.zlie;
  val->ig2_zhie = ig_cfg2.zhie;
  return ret;
}

/**
  * @brief   Accelerometer interrupt on threshold source.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer’s X low. event.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_src_get(lis2hh12_ctx_t *ctx,
                                   lis2hh12_xl_trshld_src_t *val)
{
  lis2hh12_ig_src1_t ig_src1;
  lis2hh12_ig_src2_t ig_src2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_SRC1, (uint8_t*)&ig_src1, 1);
  if(ret == 0) {
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_SRC2, (uint8_t*)&ig_src2, 1);
  }
  val->ig1_xl = ig_src1.xl;
  val->ig1_xh = ig_src1.xh;
  val->ig1_yl = ig_src1.yl;
  val->ig1_yh = ig_src1.yh;
  val->ig1_zl = ig_src1.zl;
  val->ig1_zh = ig_src1.zh;
  val->ig1_ia = ig_src1.ia;
  val->ig2_xl = ig_src2.xl;
  val->ig2_xh = ig_src2.xh;
  val->ig2_yl = ig_src2.yl;
  val->ig2_yh = ig_src2.yh;
  val->ig2_zl = ig_src2.zl;
  val->ig2_zh = ig_src2.zh;
  val->ig2_ia = ig_src2.ia;

  return ret;
}

/**
  * @brief   Axis interrupt threshold.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data to be write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_set(lis2hh12_ctx_t *ctx, uint8_t ig1_x,
                               uint8_t ig1_y, uint8_t ig1_z,
                               uint8_t ig2_xyz)
{
  int32_t ret;

  ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_THS_X1, &ig1_x, 1);
  if(ret == 0) {
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_THS_Y1, &ig1_y, 1);
  }
  if(ret == 0) {
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_THS_Z1, &ig1_z, 1);
  }
  if(ret == 0) {
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_THS2, &ig2_xyz, 1);
  }

  return ret;
}

/**
  * @brief   Axis interrupt threshold.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_get(lis2hh12_ctx_t *ctx, uint8_t *ig1_x,
                               uint8_t *ig1_y, uint8_t *ig1_z,
                               uint8_t *ig2_xyz)
{
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_THS_X1, ig1_x, 1);
  if(ret == 0) {
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_THS_Y1, ig1_y, 1);
  }
  if(ret == 0) {
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_THS_Z1, ig1_z, 1);
  }
  if(ret == 0) {
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_THS2, ig2_xyz, 1);
  }

  return ret;
}

/**
  * @brief   Enter/exit interrupt duration value.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of dur1 in reg IG_DUR1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_min_sample_set(lis2hh12_ctx_t *ctx,
                                          uint8_t ig1_sam, uint8_t ig2_sam)
{
  lis2hh12_ig_dur1_t ig_dur1;
  lis2hh12_ig_dur2_t ig_dur2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_DUR1, (uint8_t*)&ig_dur1, 1);
  if(ret == 0){
    if (ig1_sam == 0x00U){
      ig_dur1.wait1 = PROPERTY_DISABLE;
    }
    else{
      ig_dur1.wait1 = PROPERTY_ENABLE;
    }
    ig_dur1.dur1 = ig1_sam;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_DUR1, (uint8_t*)&ig_dur1, 1);
  }
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_DUR2, (uint8_t*)&ig_dur2, 1);
  }
  if(ret == 0){
    if (ig2_sam == 0x00U){
      ig_dur2.wait2 = PROPERTY_DISABLE;
    }
    else{
      ig_dur2.wait2 = PROPERTY_ENABLE;
    }
    ig_dur2.dur2 = ig2_sam;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_DUR2, (uint8_t*)&ig_dur2, 1);
  }

  return ret;
}

/**
  * @brief   Enter/exit interrupt duration value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dur1 in reg IG_DUR1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_trshld_min_sample_get(lis2hh12_ctx_t *ctx,
                                          uint8_t *ig1_sam, uint8_t *ig2_sam)
{
  lis2hh12_ig_dur1_t ig_dur1;
  lis2hh12_ig_dur2_t ig_dur2;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_DUR1, (uint8_t*)&ig_dur1, 1);
  *ig1_sam = (uint8_t)ig_dur1.dur1;

  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_DUR2, (uint8_t*)&ig_dur2, 1);
    *ig2_sam = (uint8_t)ig_dur2.dur2;
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Activity/Inactivity_detection
  * @brief       This section groups all the functions concerning
  *              activity/inactivity detection.
  * @{
  *
  */

/**
  * @brief   Inactivity threshold.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ths in reg ACT_THS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_act_threshold_set(lis2hh12_ctx_t *ctx, uint8_t val)
{
  lis2hh12_act_ths_t act_ths;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_ACT_THS, (uint8_t*)&act_ths, 1);
  if(ret == 0){
    act_ths.ths = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_ACT_THS, (uint8_t*)&act_ths, 1);
  }
  return ret;
}

/**
  * @brief   Inactivity threshold.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ths in reg ACT_THS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_act_threshold_get(lis2hh12_ctx_t *ctx, uint8_t *val)
{
  lis2hh12_act_ths_t act_ths;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_ACT_THS, (uint8_t*)&act_ths, 1);
  *val = (uint8_t)act_ths.ths;

  return ret;
}

/**
  * @brief   Inactivity duration in number of sample.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of dur in reg ACT_DUR.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_act_duration_set(lis2hh12_ctx_t *ctx, uint8_t val)
{
  lis2hh12_act_dur_t act_dur;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_ACT_DUR, (uint8_t*)&act_dur, 1);
  if(ret == 0){
    act_dur.dur = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_ACT_DUR, (uint8_t*)&act_dur, 1);
  }
  return ret;
}

/**
  * @brief   Inactivity duration in number of sample.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dur in reg ACT_DUR.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_act_duration_get(lis2hh12_ctx_t *ctx, uint8_t *val)
{
  lis2hh12_act_dur_t act_dur;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_ACT_DUR, (uint8_t*)&act_dur, 1);
  *val = (uint8_t)act_dur.dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Six_position_detection(6D/4D).
  * @brief       This section groups all the functions concerning six
  *              position detection (6D).
  * @{
  *
  */

/**
  * @brief   6D feature working mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Configure 6D feature working mode.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_6d_mode_set(lis2hh12_ctx_t *ctx, lis2hh12_6d_mode_t val)
{
  lis2hh12_ig_cfg1_t ig_cfg1;
  lis2hh12_ig_cfg2_t ig_cfg2;
  lis2hh12_ctrl7_t ctrl7;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  if(ret == 0){
    ctrl7._4d_ig = ((uint8_t)val & 0x10U) >> 4;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  }
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }
  if(ret == 0){
    ig_cfg2._6d = ((uint8_t)val & 0x02U) >> 1;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG1, (uint8_t*)&ig_cfg1, 1);
  }
  if(ret == 0){
    ig_cfg1._6d = (uint8_t)val & 0x01U;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_IG_CFG1, (uint8_t*)&ig_cfg1, 1);
  }

  return ret;
}

/**
  * @brief   6D feature working mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the configuration of 6D feature.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_6d_mode_get(lis2hh12_ctx_t *ctx, lis2hh12_6d_mode_t *val)
{
  lis2hh12_ig_cfg1_t ig_cfg1;
  lis2hh12_ig_cfg2_t ig_cfg2;
  lis2hh12_ctrl7_t ctrl7;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL7, (uint8_t*)&ctrl7, 1);
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG2, (uint8_t*)&ig_cfg2, 1);
  }
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_IG_CFG1, (uint8_t*)&ig_cfg1, 1);
  }

  switch ( (ctrl7._4d_ig << 4) | (ig_cfg2._6d << 1) | ig_cfg1._6d){
    case LIS2HH12_6D_4D_DISABLE:
      *val = LIS2HH12_6D_4D_DISABLE;
      break;
    case LIS2HH12_ENABLE_ON_IG1_6D:
      *val = LIS2HH12_ENABLE_ON_IG1_6D;
      break;
    case LIS2HH12_ENABLE_ON_IG2_6D:
      *val = LIS2HH12_ENABLE_ON_IG2_6D;
      break;
    case LIS2HH12_ENABLE_ON_IG1_4D:
      *val = LIS2HH12_ENABLE_ON_IG1_4D;
      break;
    case LIS2HH12_ENABLE_ON_IG2_4D:
      *val = LIS2HH12_ENABLE_ON_IG2_4D;
      break;
    default:
      *val = LIS2HH12_6D_4D_DISABLE;
      break;
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Fifo
  * @brief       This section group all the functions concerning
  *              the fifo usage
  * @{
  *
  */

/**
  * @brief   FIFO watermark level selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of stop_fth in reg CTRL3.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_fifo_watermark_set(lis2hh12_ctx_t *ctx, uint8_t val)
{
  lis2hh12_fifo_ctrl_t fifo_ctrl;
  lis2hh12_ctrl3_t ctrl3;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL3, (uint8_t*)&ctrl3, 1);
  if(ret == 0){
  ret = lis2hh12_read_reg(ctx, LIS2HH12_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }

  if(ret == 0){
    if (val == 0x00U){
      ctrl3.stop_fth = PROPERTY_DISABLE;
    }
    else{
      ctrl3.stop_fth = PROPERTY_ENABLE;

    }
    fifo_ctrl.fth = val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL3, (uint8_t*)&ctrl3, 1);
  }

  return ret;
}

/**
  * @brief   FIFO watermark level selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of stop_fth in reg CTRL3.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_fifo_watermark_get(lis2hh12_ctx_t *ctx, uint8_t *val)
{

  lis2hh12_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  *val = (uint8_t)fifo_ctrl.fth;

  return ret;
}

/**
  * @brief   FIFO mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fifo_en" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_fifo_mode_set(lis2hh12_ctx_t *ctx, lis2hh12_fifo_md_t val)
{
  lis2hh12_fifo_ctrl_t fifo_ctrl;
  lis2hh12_ctrl3_t ctrl3;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL3, (uint8_t*)&ctrl3, 1);
  if(ret == 0){
    ctrl3.fifo_en = (( (uint8_t) val & 0x10U) >> 4);
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL3, (uint8_t*)&ctrl3, 1);
  }
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }
  if(ret == 0){
    fifo_ctrl.fmode = ((uint8_t)val & 0x0FU);
    ret = lis2hh12_write_reg(ctx, LIS2HH12_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }
  return ret;
}

/**
  * @brief   FIFO mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fifo_en in reg CTRL3.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_fifo_mode_get(lis2hh12_ctx_t *ctx, lis2hh12_fifo_md_t *val)
{
  lis2hh12_fifo_ctrl_t fifo_ctrl;
  lis2hh12_ctrl3_t ctrl3;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL3, (uint8_t*)&ctrl3, 1);
  if(ret == 0){
    ret = lis2hh12_read_reg(ctx, LIS2HH12_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }

  switch ( (ctrl3.fifo_en << 4) | fifo_ctrl.fmode ){
    case LIS2HH12_FIFO_OFF:
      *val = LIS2HH12_FIFO_OFF;
      break;
    case LIS2HH12_BYPASS_MODE:
      *val = LIS2HH12_BYPASS_MODE;
      break;
    case LIS2HH12_FIFO_MODE:
      *val = LIS2HH12_FIFO_MODE;
      break;
    case LIS2HH12_STREAM_MODE:
      *val = LIS2HH12_STREAM_MODE;
      break;
    case LIS2HH12_STREAM_TO_FIFO_MODE:
      *val = LIS2HH12_STREAM_TO_FIFO_MODE;
      break;
    case LIS2HH12_BYPASS_TO_STREAM_MODE:
      *val = LIS2HH12_BYPASS_TO_STREAM_MODE;
      break;
    case LIS2HH12_BYPASS_TO_FIFO_MODE:
      *val = LIS2HH12_BYPASS_TO_FIFO_MODE;
      break;
    default:
      *val = LIS2HH12_FIFO_OFF;
      break;
  }
  return ret;
}

/**
  * @brief  FIFOstatus.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    FIFOfullflag.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_fifo_status_get(lis2hh12_ctx_t *ctx,
                                 lis2hh12_fifo_stat_t *val)
{
  lis2hh12_fifo_src_t fifo_src;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_FIFO_SRC, (uint8_t*)&fifo_src, 1);
  val->fss = fifo_src.fss;
  val->empty = fifo_src.empty;
  val->ovr = fifo_src.ovr;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS2HH12_Self_test
  * @brief       This section groups all the functions that manage
  *              self test configuration
  * @{
  *
  */

/**
  * @brief   Enable/disable self-test mode for accelerometer.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "st" in reg LIS2HH12.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_self_test_set(lis2hh12_ctx_t *ctx,
                                  lis2hh12_xl_st_t val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  if(ret == 0){
    ctrl5.st = (uint8_t)val;
    ret = lis2hh12_write_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  }
  return ret;
}

/**
  * @brief   Enable/disable self-test mode for accelerometer.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st in reg CTRL5.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis2hh12_xl_self_test_get(lis2hh12_ctx_t *ctx,
                                  lis2hh12_xl_st_t *val)
{
  lis2hh12_ctrl5_t ctrl5;
  int32_t ret;

  ret = lis2hh12_read_reg(ctx, LIS2HH12_CTRL5, (uint8_t*)&ctrl5, 1);
  switch (ctrl5.st){
    case LIS2HH12_ST_DISABLE:
      *val = LIS2HH12_ST_DISABLE;
      break;
    case LIS2HH12_ST_POSITIVE:
      *val = LIS2HH12_ST_POSITIVE;
      break;
    case LIS2HH12_ST_NEGATIVE:
      *val = LIS2HH12_ST_NEGATIVE;
      break;
    default:
      *val = LIS2HH12_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/