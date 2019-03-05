/*
 ******************************************************************************
 * @file    h3lis331dl_reg.c
 * @author  Sensors Software Solution Team
 * @brief   H3LIS331DL driver file
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

#include "h3lis331dl_reg.h"

/**
  * @defgroup    H3LIS331DL
  * @brief       This file provides a set of functions needed to drive the
  *              h3lis331dl enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup    H3LIS331DL_Interfaces_Functions
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
int32_t h3lis331dl_read_reg(h3lis331dl_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t h3lis331dl_write_reg(h3lis331dl_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    H3LIS331DL_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float h3lis331dl_from_fs100_to_mg(int16_t lsb)
{
  return ((float)lsb * 49.0f);
}

float h3lis331dl_from_fs200_to_mg(int16_t lsb)
{
  return ((float)lsb * 98.0f);
}

float h3lis331dl_from_fs400_to_mg(int16_t lsb)
{
  return ((float)lsb * 195.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup    H3LIS331DL_Data_Generation
  * @brief       This section group all the functions concerning
  *              data generation
  * @{
  *
  */

/**
  * @brief  X axis enable/disable.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of xen in reg CTRL_REG1
  *
  */
int32_t h3lis331dl_axis_x_data_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG1,
                            (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0) {
    ctrl_reg1.xen = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG1,
                              (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  X axis enable/disable.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of xen in reg CTRL_REG1
  *
  */
int32_t h3lis331dl_axis_x_data_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG1,
                            (uint8_t*)&ctrl_reg1, 1);
  *val = ctrl_reg1.xen;

  return ret;
}

/**
  * @brief  Y axis enable/disable.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of yen in reg CTRL_REG1
  *
  */
int32_t h3lis331dl_axis_y_data_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG1,
                            (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0) {
    ctrl_reg1.yen = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG1,
                              (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Y axis enable/disable.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of yen in reg CTRL_REG1
  *
  */
int32_t h3lis331dl_axis_y_data_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG1,
                            (uint8_t*)&ctrl_reg1, 1);
  *val = ctrl_reg1.yen;

  return ret;
}

/**
  * @brief  Z axis enable/disable.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of zen in reg CTRL_REG1
  *
  */
int32_t h3lis331dl_axis_z_data_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG1,
                            (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0) {
    ctrl_reg1.zen = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG1,
                              (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Z axis enable/disable.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of zen in reg CTRL_REG1
  *
  */
int32_t h3lis331dl_axis_z_data_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG1,
                            (uint8_t*)&ctrl_reg1, 1);
  *val = ctrl_reg1.zen;

  return ret;
}

/**
  * @brief  Accelerometer data rate selection.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of dr in reg CTRL_REG1
  *
  */
int32_t h3lis331dl_data_rate_set(h3lis331dl_ctx_t *ctx, h3lis331dl_dr_t val)
{
  h3lis331dl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG1,
                           (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0) {
    ctrl_reg1.pm = (uint8_t)val & 0x07U;
    ctrl_reg1.dr = ( (uint8_t)val & 0x30U ) >> 4;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG1,
                              (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer data rate selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of dr in reg CTRL_REG1
  *
  */
int32_t h3lis331dl_data_rate_get(h3lis331dl_ctx_t *ctx, h3lis331dl_dr_t *val)
{
  h3lis331dl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG1,
                           (uint8_t*)&ctrl_reg1, 1);

  switch ((ctrl_reg1.dr << 4) + ctrl_reg1.pm)
  {
    case H3LIS331DL_ODR_OFF:
      *val = H3LIS331DL_ODR_OFF;
      break;
    case H3LIS331DL_ODR_Hz5:
      *val = H3LIS331DL_ODR_Hz5;
      break;
    case H3LIS331DL_ODR_1Hz:
      *val = H3LIS331DL_ODR_1Hz;
      break;
    case H3LIS331DL_ODR_5Hz2:
      *val = H3LIS331DL_ODR_5Hz2;
      break;
    case H3LIS331DL_ODR_5Hz:
      *val = H3LIS331DL_ODR_5Hz;
      break;
    case H3LIS331DL_ODR_10Hz:
      *val = H3LIS331DL_ODR_10Hz;
      break;
    case H3LIS331DL_ODR_50Hz:
      *val = H3LIS331DL_ODR_50Hz;
      break;
    case H3LIS331DL_ODR_100Hz:
      *val = H3LIS331DL_ODR_100Hz;
      break;
    case H3LIS331DL_ODR_400Hz:
      *val = H3LIS331DL_ODR_400Hz;
      break;
    case H3LIS331DL_ODR_1kHz:
      *val = H3LIS331DL_ODR_1kHz;
      break;
    default:
      *val = H3LIS331DL_ODR_OFF;
      break;
  }

  return ret;
}

/**
  * @brief  High pass filter mode selection.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of hpm in reg CTRL_REG2
  *
  */
int32_t h3lis331dl_reference_mode_set(h3lis331dl_ctx_t *ctx,
                                     h3lis331dl_hpm_t val)
{
  h3lis331dl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG2,
                            (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0) {
    ctrl_reg2.hpm = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG2,
                              (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  High pass filter mode selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of hpm in reg CTRL_REG2
  *
  */
int32_t h3lis331dl_reference_mode_get(h3lis331dl_ctx_t *ctx,
                                     h3lis331dl_hpm_t *val)
{
  h3lis331dl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG2,
                           (uint8_t*)&ctrl_reg2, 1);

  switch (ctrl_reg2.hpm)
  {
    case H3LIS331DL_NORMAL_MODE:
      *val = H3LIS331DL_NORMAL_MODE;
      break;
    case H3LIS331DL_REF_MODE_ENABLE:
      *val = H3LIS331DL_REF_MODE_ENABLE;
      break;
    default:
      *val = H3LIS331DL_NORMAL_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  Accelerometer full-scale selection.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of fs in reg CTRL_REG4
  *
  */
int32_t h3lis331dl_full_scale_set(h3lis331dl_ctx_t *ctx, h3lis331dl_fs_t val)
{
  h3lis331dl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0) {
    ctrl_reg4.fs = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG4,
                              (uint8_t*)&ctrl_reg4, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer full-scale selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of fs in reg CTRL_REG4
  *
  */
int32_t h3lis331dl_full_scale_get(h3lis331dl_ctx_t *ctx, h3lis331dl_fs_t *val)
{
  h3lis331dl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);

  switch (ctrl_reg4.fs)
  {
    case H3LIS331DL_100g:
      *val = H3LIS331DL_100g;
      break;
    case H3LIS331DL_200g:
      *val = H3LIS331DL_200g;
      break;
    case H3LIS331DL_400g:
      *val = H3LIS331DL_400g;
      break;
    default:
      *val = H3LIS331DL_100g;
      break;
  }

  return ret;
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of bdu in reg CTRL_REG4
  *
  */
int32_t h3lis331dl_block_data_update_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0) {
    ctrl_reg4.bdu = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG4,
                              (uint8_t*)&ctrl_reg4, 1);
  }
  return ret;
}

/**
  * @brief  Block data update.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of bdu in reg CTRL_REG4
  *
  */
int32_t h3lis331dl_block_data_update_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);
  *val = ctrl_reg4.bdu;

  return ret;
}

/**
  * @brief  The STATUS_REG register is read by the interface.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         registers STATUS_REG
  *
  */
int32_t h3lis331dl_status_reg_get(h3lis331dl_ctx_t *ctx,
                                 h3lis331dl_status_reg_t *val)
{
  int32_t ret;
  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_STATUS_REG,
                            (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Accelerometer new data available.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of zyxda in reg STATUS_REG
  *
  */
int32_t h3lis331dl_flag_data_ready_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_status_reg_t status_reg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_STATUS_REG,
                           (uint8_t*)&status_reg, 1);
  *val = status_reg.zyxda;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    H3LIS331DL_Data_Output
  * @brief       This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief  Linear acceleration output register. The value is expressed
  *         as a 16-bit word in two’s complement.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t h3lis331dl_acceleration_raw_get(h3lis331dl_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_OUT_X_L, buff, 6);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    H3LIS331DL_Common
  * @brief       This section groups common useful functions.
  * @{
  *
  */

/**
  * @brief  Device Who am I.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t h3lis331dl_device_id_get(h3lis331dl_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of boot in reg CTRL_REG2
  *
  */
int32_t h3lis331dl_boot_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG2,
                            (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0) {
    ctrl_reg2.boot = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG2,
                              (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of boot in reg CTRL_REG2
  *
  */
int32_t h3lis331dl_boot_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG2,
                            (uint8_t*)&ctrl_reg2, 1);
  *val = ctrl_reg2.boot;

  return ret;
}

/**
  * @brief  Big/Little Endian Data selection.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of ble in reg CTRL_REG4
  *
  */
int32_t h3lis331dl_data_format_set(h3lis331dl_ctx_t *ctx,
                                   h3lis331dl_ble_t val)
{
  h3lis331dl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0) {
    ctrl_reg4.ble = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG4,
                              (uint8_t*)&ctrl_reg4, 1);
  }
  return ret;
}

/**
  * @brief  Big/Little Endian Data selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of ble in reg CTRL_REG4
  *
  */
int32_t h3lis331dl_data_format_get(h3lis331dl_ctx_t *ctx,
                                   h3lis331dl_ble_t *val)
{
  h3lis331dl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);

  switch (ctrl_reg4.ble)
  {
    case H3LIS331DL_LSB_AT_LOW_ADD:
      *val = H3LIS331DL_LSB_AT_LOW_ADD;
      break;
    case H3LIS331DL_MSB_AT_LOW_ADD:
      *val = H3LIS331DL_MSB_AT_LOW_ADD;
      break;
    default:
      *val = H3LIS331DL_LSB_AT_LOW_ADD;
      break;
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    H3LIS331DL_Filters
  * @brief       This section group all the functions concerning the
  *              filters configuration.
  * @{
  *
  */

/**
  * @brief  High pass filter cut-off frequency configuration.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of hpcf in reg CTRL_REG2
  *
  */
int32_t h3lis331dl_hp_bandwidth_set(h3lis331dl_ctx_t *ctx,
                                    h3lis331dl_hpcf_t val)
{
  h3lis331dl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG2,
                            (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0) {
    ctrl_reg2.hpcf = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG2,
                              (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  High pass filter cut-off frequency configuration.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of hpcf in reg CTRL_REG2
  *
  */
int32_t h3lis331dl_hp_bandwidth_get(h3lis331dl_ctx_t *ctx,
                                   h3lis331dl_hpcf_t *val)
{
  h3lis331dl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG2,
                            (uint8_t*)&ctrl_reg2, 1);

  switch (ctrl_reg2.hpcf)
  {
    case H3LIS331DL_CUT_OFF_8Hz:
      *val = H3LIS331DL_CUT_OFF_8Hz;
      break;
    case H3LIS331DL_CUT_OFF_16Hz:
      *val = H3LIS331DL_CUT_OFF_16Hz;
      break;
    case H3LIS331DL_CUT_OFF_32Hz:
      *val = H3LIS331DL_CUT_OFF_32Hz;
      break;
    case H3LIS331DL_CUT_OFF_64Hz:
      *val = H3LIS331DL_CUT_OFF_64Hz;
      break;
    default:
      *val = H3LIS331DL_CUT_OFF_8Hz;
      break;
  }

  return ret;
}

/**
  * @brief  Select High Pass filter path.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of hpen in reg CTRL_REG2
  *
  */
int32_t h3lis331dl_hp_path_set(h3lis331dl_ctx_t *ctx, h3lis331dl_hpen_t val)
{
  h3lis331dl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG2,
                            (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0) {
    ctrl_reg2.hpen = (uint8_t)val & 0x03U;
    ctrl_reg2.fds = ((uint8_t)val & 0x04U) >> 2;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG2,
                              (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Select High Pass filter path.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of hpen in reg CTRL_REG2
  *
  */
int32_t h3lis331dl_hp_path_get(h3lis331dl_ctx_t *ctx, h3lis331dl_hpen_t *val)
{
  h3lis331dl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG2,
                            (uint8_t*)&ctrl_reg2, 1);

  switch ( (ctrl_reg2.fds << 2) + ctrl_reg2.hpen )
  {
    case H3LIS331DL_HP_DISABLE:
      *val = H3LIS331DL_HP_DISABLE;
      break;
    case H3LIS331DL_HP_ON_OUT:
      *val = H3LIS331DL_HP_ON_OUT;
      break;
    case H3LIS331DL_HP_ON_INT1:
      *val = H3LIS331DL_HP_ON_INT1;
      break;
    case H3LIS331DL_HP_ON_INT2:
      *val = H3LIS331DL_HP_ON_INT2;
      break;
    case H3LIS331DL_HP_ON_INT1_INT2:
      *val = H3LIS331DL_HP_ON_INT1_INT2;
      break;
    case H3LIS331DL_HP_ON_INT1_INT2_OUT:
      *val = H3LIS331DL_HP_ON_INT1_INT2_OUT;
      break;
    case H3LIS331DL_HP_ON_INT2_OUT:
      *val = H3LIS331DL_HP_ON_INT2_OUT;
      break;
    case H3LIS331DL_HP_ON_INT1_OUT:
      *val = H3LIS331DL_HP_ON_INT1_OUT;
      break;
    default:
      *val = H3LIS331DL_HP_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Reading at this address zeroes instantaneously
  *         the content of the internal high pass-filter.
  *         If the high pass filter is enabled all three axes
  *         are instantaneously set to 0g. This allows to
  *         overcome the settling time of the high pass
  *         filter.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  *
  */
int32_t h3lis331dl_hp_reset_get(h3lis331dl_ctx_t *ctx)
{
  uint8_t dummy;
  int32_t ret;
  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_HP_FILTER_RESET,
                           (uint8_t*)&dummy, 1);
  return ret;
}

/**
  * @brief  Reference value for high-pass filter.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of ref in reg REFERENCE
  *
  */
int32_t h3lis331dl_hp_reference_value_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  int32_t ret;
  ret = h3lis331dl_write_reg(ctx, H3LIS331DL_REFERENCE, (uint8_t*)&val, 1);
  return ret;
}

/**
  * @brief  Reference value for high-pass filter.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of ref in reg REFERENCE
  *
  */
int32_t h3lis331dl_hp_reference_value_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  int32_t ret;
  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_REFERENCE, val, 1);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    H3LIS331DL_Serial_Interface
  * @brief       This section groups all the functions concerning serial
  *              interface management.
  * @{
  *
  */

/**
  * @brief  SPI 3- or 4-wire interface.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of sim in reg CTRL_REG4
  *
  */
int32_t h3lis331dl_spi_mode_set(h3lis331dl_ctx_t *ctx, h3lis331dl_sim_t val)
{
  h3lis331dl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0) {
    ctrl_reg4.sim = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG4,
                              (uint8_t*)&ctrl_reg4, 1);
  }
  return ret;
}

/**
  * @brief  SPI 3- or 4-wire interface.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of sim in reg CTRL_REG4
  *
  */
int32_t h3lis331dl_spi_mode_get(h3lis331dl_ctx_t *ctx, h3lis331dl_sim_t *val)
{
  h3lis331dl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);

  switch ( ctrl_reg4.sim )
  {
    case H3LIS331DL_SPI_4_WIRE:
      *val = H3LIS331DL_SPI_4_WIRE;
      break;
    case H3LIS331DL_SPI_3_WIRE:
      *val = H3LIS331DL_SPI_3_WIRE;
      break;
    default:
      *val = H3LIS331DL_SPI_4_WIRE;
      break;
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    H3LIS331DL_Interrupt_Pins
  * @brief       This section groups all the functions that manage
  *              interrupt pins.
  * @{
  *
  */

/**
  * @brief  Data signal on INT 1 pad control bits.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of i1_cfg in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_pin_int1_route_set(h3lis331dl_ctx_t *ctx,
                                     h3lis331dl_i1_cfg_t val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0) {
    ctrl_reg3.i1_cfg = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG3,
                              (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Data signal on INT 1 pad control bits.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of i1_cfg in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_pin_int1_route_get(h3lis331dl_ctx_t *ctx,
                                     h3lis331dl_i1_cfg_t *val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);

  switch ( ctrl_reg3.i1_cfg )
  {
    case H3LIS331DL_PAD1_INT1_SRC:
      *val = H3LIS331DL_PAD1_INT1_SRC;
      break;
    case H3LIS331DL_PAD1_INT1_OR_INT2_SRC:
      *val = H3LIS331DL_PAD1_INT1_OR_INT2_SRC;
      break;
    case H3LIS331DL_PAD1_DRDY:
      *val = H3LIS331DL_PAD1_DRDY;
      break;
    case H3LIS331DL_PAD1_BOOT:
      *val = H3LIS331DL_PAD1_BOOT;
      break;
    default:
      *val = H3LIS331DL_PAD1_INT1_SRC;
      break;
  }

  return ret;
}

/**
  * @brief  Latch interrupt request on INT1_SRC register, with INT1_SRC
  *         register cleared by reading INT1_SRC register.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of lir1 in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_int1_notification_set(h3lis331dl_ctx_t *ctx,
                                        h3lis331dl_lir1_t val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0) {
    ctrl_reg3.lir1 = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG3,
                              (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Latch interrupt request on INT1_SRC register, with INT1_SRC
  *         register cleared by reading INT1_SRC register.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of lir1 in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_int1_notification_get(h3lis331dl_ctx_t *ctx,
                                        h3lis331dl_lir1_t *val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);

  switch ( ctrl_reg3.lir1 )
  {
    case H3LIS331DL_INT1_PULSED:
      *val = H3LIS331DL_INT1_PULSED;
      break;
    case H3LIS331DL_INT1_LATCHED:
      *val = H3LIS331DL_INT1_LATCHED;
      break;
    default:
      *val = H3LIS331DL_INT1_PULSED;
      break;
  }

  return ret;
}

/**
  * @brief  Data signal on INT 2 pad control bits.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of i2_cfg in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_pin_int2_route_set(h3lis331dl_ctx_t *ctx,
                                     h3lis331dl_i2_cfg_t val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0) {
    ctrl_reg3.i2_cfg = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG3,
                              (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Data signal on INT 2 pad control bits.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of i2_cfg in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_pin_int2_route_get(h3lis331dl_ctx_t *ctx,
                                     h3lis331dl_i2_cfg_t *val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);

  switch ( ctrl_reg3.i2_cfg )
  {
    case H3LIS331DL_PAD2_INT2_SRC:
      *val = H3LIS331DL_PAD2_INT2_SRC;
      break;
    case H3LIS331DL_PAD2_INT1_OR_INT2_SRC:
      *val = H3LIS331DL_PAD2_INT1_OR_INT2_SRC;
      break;
    case H3LIS331DL_PAD2_DRDY:
      *val = H3LIS331DL_PAD2_DRDY;
      break;
    case H3LIS331DL_PAD2_BOOT:
      *val = H3LIS331DL_PAD2_BOOT;
      break;
    default:
      *val = H3LIS331DL_PAD2_INT2_SRC;
      break;
  }

  return ret;
}

/**
  * @brief  Latch interrupt request on INT2_SRC register, with INT2_SRC
  *         register cleared by reading INT2_SRC itself.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of lir2 in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_int2_notification_set(h3lis331dl_ctx_t *ctx,
                                        h3lis331dl_lir2_t val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0) {
    ctrl_reg3.lir2 = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG3,
                              (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Latch interrupt request on INT2_SRC register, with INT2_SRC
  *         register cleared by reading INT2_SRC itself.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of lir2 in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_int2_notification_get(h3lis331dl_ctx_t *ctx,
                                        h3lis331dl_lir2_t *val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);

  switch ( ctrl_reg3.lir2 )
  {
    case H3LIS331DL_INT2_PULSED:
      *val = H3LIS331DL_INT2_PULSED;
      break;
    case H3LIS331DL_INT2_LATCHED:
      *val = H3LIS331DL_INT2_LATCHED;
      break;
    default:
      *val = H3LIS331DL_INT2_PULSED;
      break;
  }

  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of pp_od in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_pin_mode_set(h3lis331dl_ctx_t *ctx, h3lis331dl_pp_od_t val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0) {
    ctrl_reg3.pp_od = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG3,
                              (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of pp_od in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_pin_mode_get(h3lis331dl_ctx_t *ctx, h3lis331dl_pp_od_t *val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);

  switch ( ctrl_reg3.pp_od )
  {
    case H3LIS331DL_PUSH_PULL:
      *val = H3LIS331DL_PUSH_PULL;
      break;
    case H3LIS331DL_OPEN_DRAIN:
      *val = H3LIS331DL_OPEN_DRAIN;
      break;
    default:
      *val = H3LIS331DL_PUSH_PULL;
      break;
  }

  return ret;
}

/**
  * @brief  Interrupt active-high/low.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of ihl in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_pin_polarity_set(h3lis331dl_ctx_t *ctx,
                                    h3lis331dl_ihl_t val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0) {
    ctrl_reg3.ihl = (uint8_t)val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG3,
                              (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Interrupt active-high/low.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of ihl in reg CTRL_REG3
  *
  */
int32_t h3lis331dl_pin_polarity_get(h3lis331dl_ctx_t *ctx,
                                    h3lis331dl_ihl_t *val)
{
  h3lis331dl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG3,
                            (uint8_t*)&ctrl_reg3, 1);

  switch ( ctrl_reg3.ihl )
  {
    case H3LIS331DL_ACTIVE_HIGH:
      *val = H3LIS331DL_ACTIVE_HIGH;
      break;
    case H3LIS331DL_ACTIVE_LOW:
      *val = H3LIS331DL_ACTIVE_LOW;
      break;
    default:
      *val = H3LIS331DL_ACTIVE_HIGH;
      break;
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    H3LIS331DL_interrupt_on_threshold
  * @brief       This section groups all the functions that manage
  *              the interrupt on threshold event generation.
  * @{
  *
  */

/**
  * @brief  Configure the interrupt 1 threshold sign.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         enable sign and axis for interrupt on threshold
  *
  */
int32_t h3lis331dl_int1_on_threshold_conf_set(h3lis331dl_ctx_t *ctx,
                                              int1_on_th_conf_t val)
{
  h3lis331dl_int1_cfg_t int1_cfg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_CFG, (uint8_t*)&int1_cfg, 1);
  if(ret == 0) {
    int1_cfg.xlie  = val.int1_xlie;
    int1_cfg.xhie  = val.int1_xhie;
    int1_cfg.ylie  = val.int1_ylie;
    int1_cfg.yhie  = val.int1_yhie;
    int1_cfg.zlie  = val.int1_zlie;
    int1_cfg.zhie  = val.int1_zhie;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_INT1_CFG,
                              (uint8_t*)&int1_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Configure the interrupt 1 threshold sign.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         enable sign and axis for interrupt on threshold
  *
  */
int32_t h3lis331dl_int1_on_threshold_conf_get(h3lis331dl_ctx_t *ctx,
                                             int1_on_th_conf_t *val)
{
  h3lis331dl_int1_cfg_t int1_cfg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_CFG, (uint8_t*)&int1_cfg, 1);
  val->int1_xlie = int1_cfg.xlie;
  val->int1_xhie = int1_cfg.xhie;
  val->int1_ylie = int1_cfg.ylie;
  val->int1_yhie = int1_cfg.yhie;
  val->int1_zlie = int1_cfg.zlie;
  val->int1_zhie = int1_cfg.zhie;

  return ret;
}

/**
  * @brief  AND/OR combination of Interrupt 1 events.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of aoi in reg INT1_CFG
  *
  */
int32_t h3lis331dl_int1_on_threshold_mode_set(h3lis331dl_ctx_t *ctx,
                                             h3lis331dl_int1_aoi_t val)
{
  h3lis331dl_int1_cfg_t int1_cfg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_CFG, (uint8_t*)&int1_cfg, 1);
  if(ret == 0) {
    int1_cfg.aoi = (uint8_t) val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_INT1_CFG,
                              (uint8_t*)&int1_cfg, 1);
  }
  return ret;
}

/**
  * @brief   AND/OR combination of Interrupt 1 events.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of aoi in reg INT1_CFG
  *
  */
int32_t h3lis331dl_int1_on_threshold_mode_get(h3lis331dl_ctx_t *ctx,
                                             h3lis331dl_int1_aoi_t *val)
{
  h3lis331dl_int1_cfg_t int1_cfg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_CFG, (uint8_t*)&int1_cfg, 1);

  switch ( int1_cfg.aoi )
  {
    case H3LIS331DL_INT1_ON_THRESHOLD_OR:
      *val = H3LIS331DL_INT1_ON_THRESHOLD_OR;
      break;
    case H3LIS331DL_INT1_ON_THRESHOLD_AND:
      *val = H3LIS331DL_INT1_ON_THRESHOLD_AND;
      break;
    default:
      *val = H3LIS331DL_INT1_ON_THRESHOLD_OR;
      break;
  }

  return ret;
}

/**
  * @brief  Interrupt generator 1 on threshold source register.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         registers INT1_SRC
  *
  */
int32_t h3lis331dl_int1_src_get(h3lis331dl_ctx_t *ctx,
                               h3lis331dl_int1_src_t *val)
{
  int32_t ret;
  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_SRC, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt 1 threshold.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of ths in reg INT1_THS
  *
  */
int32_t h3lis331dl_int1_treshold_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_int1_ths_t int1_ths;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_THS, (uint8_t*)&int1_ths, 1);
  if(ret == 0) {
    int1_ths.ths = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_INT1_THS,
                              (uint8_t*)&int1_ths, 1);
  }
  return ret;
}

/**
  * @brief  Interrupt 1 threshold.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of ths in reg INT1_THS
  *
  */
int32_t h3lis331dl_int1_treshold_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_int1_ths_t int1_ths;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_THS, (uint8_t*)&int1_ths, 1);
  *val = int1_ths.ths;

  return ret;
}

/**
  * @brief  Duration value for interrupt 1 generator.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of d in reg INT1_DURATION
  *
  */
int32_t h3lis331dl_int1_dur_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_int1_duration_t int1_duration;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_DURATION,
                           (uint8_t*)&int1_duration, 1);
  if(ret == 0) {
    int1_duration.d = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_INT1_DURATION,
                              (uint8_t*)&int1_duration, 1);
  }
  return ret;
}

/**
  * @brief  Duration value for interrupt 1 generator.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of d in reg INT1_DURATION
  *
  */
int32_t h3lis331dl_int1_dur_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_int1_duration_t int1_duration;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT1_DURATION,
                           (uint8_t*)&int1_duration, 1);
  *val = int1_duration.d;

  return ret;
}

/**
  * @brief  Configure the interrupt 2 threshold sign.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         enable sign and axis for interrupt on threshold
  *
  */
int32_t h3lis331dl_int2_on_threshold_conf_set(h3lis331dl_ctx_t *ctx,
                                              int2_on_th_conf_t val)
{
  h3lis331dl_int2_cfg_t int2_cfg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_CFG,
                           (uint8_t*)&int2_cfg, 1);
  if(ret == 0) {
    int2_cfg.xlie  = val.int2_xlie;
    int2_cfg.xhie  = val.int2_xhie;
    int2_cfg.ylie  = val.int2_ylie;
    int2_cfg.yhie  = val.int2_yhie;
    int2_cfg.zlie  = val.int2_zlie;
    int2_cfg.zhie  = val.int2_zhie;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_INT2_CFG,
                              (uint8_t*)&int2_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Configure the interrupt 2 threshold sign.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         enable sign and axis for interrupt on threshold
  *
  */
int32_t h3lis331dl_int2_on_threshold_conf_get(h3lis331dl_ctx_t *ctx,
                                              int2_on_th_conf_t *val)
{
  h3lis331dl_int2_cfg_t int2_cfg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_CFG, (uint8_t*)&int2_cfg, 1);
  val->int2_xlie = int2_cfg.xlie;
  val->int2_xhie = int2_cfg.xhie;
  val->int2_ylie = int2_cfg.ylie;
  val->int2_yhie = int2_cfg.yhie;
  val->int2_zlie = int2_cfg.zlie;
  val->int2_zhie = int2_cfg.zhie;

  return ret;
}

/**
  * @brief  AND/OR combination of Interrupt 2 events.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of aoi in reg INT2_CFG
  *
  */
int32_t h3lis331dl_int2_on_threshold_mode_set(h3lis331dl_ctx_t *ctx,
                                             h3lis331dl_int2_aoi_t val)
{
  h3lis331dl_int2_cfg_t int2_cfg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_CFG, (uint8_t*)&int2_cfg, 1);
  if(ret == 0) {
    int2_cfg.aoi = (uint8_t) val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_INT2_CFG,
                              (uint8_t*)&int2_cfg, 1);
  }
  return ret;
}

/**
  * @brief   AND/OR combination of Interrupt 2 events.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Get the values of aoi in reg INT2_CFG
  *
  */
int32_t h3lis331dl_int2_on_threshold_mode_get(h3lis331dl_ctx_t *ctx,
                                             h3lis331dl_int2_aoi_t *val)
{
  h3lis331dl_int2_cfg_t int2_cfg;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_CFG, (uint8_t*)&int2_cfg, 1);

  switch ( int2_cfg.aoi )
  {
    case H3LIS331DL_INT2_ON_THRESHOLD_OR:
      *val = H3LIS331DL_INT2_ON_THRESHOLD_OR;
      break;
    case H3LIS331DL_INT2_ON_THRESHOLD_AND:
      *val = H3LIS331DL_INT2_ON_THRESHOLD_AND;
      break;
    default:
      *val = H3LIS331DL_INT2_ON_THRESHOLD_OR;
      break;
  }

  return ret;
}

/**
  * @brief  Interrupt generator 2 on threshold source register.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         registers INT2_SRC
  *
  */
int32_t h3lis331dl_int2_src_get(h3lis331dl_ctx_t *ctx,
                               h3lis331dl_int2_src_t *val)
{
  int32_t ret;
  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_SRC, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Interrupt 2 threshold.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of ths in reg INT2_THS
  *
  */
int32_t h3lis331dl_int2_treshold_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_int2_ths_t int2_ths;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_THS, (uint8_t*)&int2_ths, 1);
  if(ret == 0) {
    int2_ths.ths = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_INT2_THS,
                              (uint8_t*)&int2_ths, 1);
  }
  return ret;
}

/**
  * @brief  Interrupt 2 threshold.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of ths in reg INT2_THS
  *
  */
int32_t h3lis331dl_int2_treshold_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_int2_ths_t int2_ths;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_THS, (uint8_t*)&int2_ths, 1);
  *val = int2_ths.ths;

  return ret;
}

/**
  * @brief  Duration value for interrupt 2 generator.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of d in reg INT2_DURATION
  *
  */
int32_t h3lis331dl_int2_dur_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_int2_duration_t int2_duration;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_DURATION,
                           (uint8_t*)&int2_duration, 1);
  if(ret == 0) {
    int2_duration.d = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_INT2_DURATION,
                              (uint8_t*)&int2_duration, 1);
  }
  return ret;
}

/**
  * @brief    Duration value for interrupt 2 generator.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of d in reg INT2_DURATION
  *
  */
int32_t h3lis331dl_int2_dur_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_int2_duration_t int2_duration;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_INT2_DURATION,
                           (uint8_t*)&int2_duration, 1);
  *val = int2_duration.d;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    H3LIS331DL_Wake_Up_Event
  * @brief       This section groups all the functions that manage the
  *              Wake Up event generation.
  * @{
  *
  */

/**
  * @brief  Turn-on mode selection for sleep to wake function.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of turnon in reg CTRL_REG5
  *
  */
int32_t h3lis331dl_wkup_to_sleep_set(h3lis331dl_ctx_t *ctx, uint8_t val)
{
  h3lis331dl_ctrl_reg5_t ctrl_reg5;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG5,
                            (uint8_t*)&ctrl_reg5, 1);
  if(ret == 0) {
    ctrl_reg5.turnon = val;
    ret = h3lis331dl_write_reg(ctx, H3LIS331DL_CTRL_REG5,
                              (uint8_t*)&ctrl_reg5, 1);
  }
  return ret;
}

/**
  * @brief  Turn-on mode selection for sleep to wake function.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of turnon in reg CTRL_REG5
  *
  */
int32_t h3lis331dl_wkup_to_sleep_get(h3lis331dl_ctx_t *ctx, uint8_t *val)
{
  h3lis331dl_ctrl_reg5_t ctrl_reg5;
  int32_t ret;

  ret = h3lis331dl_read_reg(ctx, H3LIS331DL_CTRL_REG5,
                            (uint8_t*)&ctrl_reg5, 1);
  *val = ctrl_reg5.turnon;

  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/