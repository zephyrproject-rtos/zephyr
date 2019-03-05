/*
 ******************************************************************************
 * @file    l20g20is_reg.c
 * @author  Sensors Software Solution Team
 * @brief   L20G20IS driver file
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
#include "l20g20is_reg.h"

/**
  * @defgroup    L20G20IS
  * @brief       This file provides a set of functions needed to drive the
  *              l20g20is enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup    L20G20IS_Interfaces_Functions
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
int32_t l20g20is_read_reg(l20g20is_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t l20g20is_write_reg(l20g20is_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    L20G20IS_Sensitivity
  * @brief      These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t l20g20is_from_fs100dps_to_mdps(int16_t lsb)
{
  return (((float_t)lsb *1000.0f)/262.0f);
}

float_t l20g20is_from_fs200dps_to_mdps(int16_t lsb)
{
  return (((float_t)lsb *1000.0f)/131.0f);
}

float_t l20g20is_from_lsb_to_celsius(int16_t lsb)
{
  return (((float_t)lsb *0.0625f)+25.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup    L20G20IS_Data_generation
  * @brief       This section groups all the functions concerning data
  *              generation.
  * @{
  *
  */

/**
  * @brief  Gyroscope new data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "xyda_ois" in reg DATA_STATUS_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_flag_data_ready_get(l20g20is_ctx_t *ctx, uint8_t *val)
{
  l20g20is_data_status_ois_t data_status_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_DATA_STATUS_OIS,
                          (uint8_t*)&data_status_ois, 1);
  *val = data_status_ois.xyda_ois;

  return ret;
}

/**
  * @brief  Gyroscope data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "pw" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_data_rate_set(l20g20is_ctx_t *ctx,
                                  l20g20is_gy_data_rate_t val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  if(ret == 0){
    ctrl1_ois.pw = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL1_OIS,
                             (uint8_t*)&ctrl1_ois, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of pw in reg CTRL1_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_data_rate_get(l20g20is_ctx_t *ctx,
                                  l20g20is_gy_data_rate_t *val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  switch (ctrl1_ois.pw){
    case L20G20IS_GY_OFF:
      *val = L20G20IS_GY_OFF;
      break;
    case L20G20IS_GY_SLEEP:
      *val = L20G20IS_GY_SLEEP;
      break;
    case L20G20IS_GY_9k33Hz:
      *val = L20G20IS_GY_9k33Hz;
      break;
    default:
      *val = L20G20IS_GY_OFF;
      break;
  }
  return ret;
}

/**
  * @brief  Directional user orientation selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    "orient" Swap X axis with Y axis.
  *                "signy"  Y-axis angular rate sign selection.
  *                "signx"  X-axis angular rate sign selection.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_orient_set(l20g20is_ctx_t *ctx,
                               l20g20is_gy_orient_t val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  l20g20is_ctrl2_ois_t ctrl2_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  if(ret == 0) {
    ctrl1_ois.orient  = val.orient;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  }
  if(ret == 0) {
    ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  }
  if(ret == 0) {
    ctrl2_ois.signx  = val.signx;
    ctrl2_ois.signy  = val.signy;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  }

  return ret;
}

/**
  * @brief  Directional user orientation selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    "orient" Swap X axis with Y axis.
  *                "signy"  Y-axis angular rate sign selection.
  *                "signx"  X-axis angular rate sign selection.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_orient_get(l20g20is_ctx_t *ctx,
                                         l20g20is_gy_orient_t *val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  l20g20is_ctrl2_ois_t ctrl2_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  if(ret == 0) {
    ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
    val->orient = ctrl1_ois.orient;
    val->signy = ctrl2_ois.signy;
    val->signy = ctrl2_ois.signy;
  }
  return ret;
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odu in reg CTRL1_OIS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_block_data_update_set(l20g20is_ctx_t *ctx, uint8_t val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  if(ret == 0){
    ctrl1_ois.odu = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL1_OIS,
                             (uint8_t*)&ctrl1_ois, 1);
  }
  return ret;
}

/**
  * @brief  Blockdataupdate.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odu in reg CTRL1_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_block_data_update_get(l20g20is_ctx_t *ctx, uint8_t *val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  *val = (uint8_t)ctrl1_ois.odu;

  return ret;
}

/**
  * @brief  User offset correction.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    "offx" offset on X axis.
                   "offy" offset on Y axis.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_angular_rate_offset_set(l20g20is_ctx_t *ctx,
                                         l20g20is_off_t val)
{
  l20g20is_off_x_t off_x;
  l20g20is_off_y_t off_y;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_OFF_X, (uint8_t*)&off_x, 1);
  if(ret == 0) {
    off_x.offx  = val.offx;
    ret = l20g20is_write_reg(ctx, L20G20IS_OFF_X, (uint8_t*)&off_x, 1);
  }
  if(ret == 0) {
    ret = l20g20is_read_reg(ctx, L20G20IS_OFF_Y, (uint8_t*)&off_y, 1);
  }
  if(ret == 0) {
    off_y.offy  = val.offy;
    ret = l20g20is_write_reg(ctx, L20G20IS_OFF_Y, (uint8_t*)&off_y, 1);
  }

  return ret;
}

/**
  * @brief  User offset correction.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    "offx" offset on X axis.
                   "offy" offset on Y axis.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_angular_rate_offset_get(l20g20is_ctx_t *ctx,
                                          l20g20is_off_t *val)
{
  l20g20is_off_x_t off_x;
  l20g20is_off_y_t off_y;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_OFF_X, (uint8_t*)&off_x, 1);
  if(ret == 0) {
    ret = l20g20is_read_reg(ctx, L20G20IS_OFF_Y, (uint8_t*)&off_y, 1);
    val->offx = off_x.offx;
    val->offy = off_y.offy;
  }


  return ret;
}

/**
  * @brief  Gyroscope full-scale selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fs_sel" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_full_scale_set(l20g20is_ctx_t *ctx,
                                   l20g20is_gy_fs_t val)
{
  l20g20is_ois_cfg_reg_t ois_cfg_reg;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_OIS_CFG_REG,
                          (uint8_t*)&ois_cfg_reg, 1);
  if(ret == 0){
    ois_cfg_reg.fs_sel = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_OIS_CFG_REG,
                             (uint8_t*)&ois_cfg_reg, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fs_sel in reg OIS_CFG_REG.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_full_scale_get(l20g20is_ctx_t *ctx,
                                   l20g20is_gy_fs_t *val)
{
  l20g20is_ois_cfg_reg_t ois_cfg_reg;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_OIS_CFG_REG,
                          (uint8_t*)&ois_cfg_reg, 1);
  switch (ois_cfg_reg.fs_sel){
    case L20G20IS_100dps:
      *val = L20G20IS_100dps;
      break;
    case L20G20IS_200dps:
      *val = L20G20IS_200dps;
      break;
    default:
      *val = L20G20IS_100dps;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    L20G20IS_Dataoutput
  * @brief       This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief  Temperature data output register (r). L and H registers together
  *         express a 16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_temperature_raw_get(l20g20is_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = l20g20is_read_reg(ctx, L20G20IS_TEMP_OUT_L, buff, 2);
  return ret;
}

/**
  * @brief  Angular rate sensor. The value is expressed as a 16-bit word in
  *         two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_angular_rate_raw_get(l20g20is_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = l20g20is_read_reg(ctx, L20G20IS_OUT_X_L, buff, 4);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    L20G20IS_Common
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
int32_t l20g20is_dev_id_get(l20g20is_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = l20g20is_read_reg(ctx, L20G20IS_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief  Device status register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Data available on all axis.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_dev_status_get(l20g20is_ctx_t *ctx,
                                l20g20is_dev_status_t *val)
{
  l20g20is_data_status_ois_t data_status_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_DATA_STATUS_OIS,
                          (uint8_t*)&data_status_ois, 1);
  val->xyda_ois = data_status_ois.xyda_ois;

  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "ble" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_dev_data_format_set(l20g20is_ctx_t *ctx,
                                     l20g20is_ble_t val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  if(ret == 0){
    ctrl1_ois.ble = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL1_OIS,
                             (uint8_t*)&ctrl1_ois, 1);
  }
  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ble in reg CTRL1_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_dev_data_format_get(l20g20is_ctx_t *ctx,
                                     l20g20is_ble_t *val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  switch (ctrl1_ois.ble){
    case L20G20IS_LSB_LOW_ADDRESS:
      *val = L20G20IS_LSB_LOW_ADDRESS;
      break;
    case L20G20IS_MSB_LOW_ADDRESS:
      *val = L20G20IS_MSB_LOW_ADDRESS;
      break;
    default:
      *val = L20G20IS_LSB_LOW_ADDRESS;
      break;
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of boot in reg CTRL1_OIS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_dev_boot_set(l20g20is_ctx_t *ctx, uint8_t val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  if(ret == 0){
    ctrl1_ois.boot = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of boot in reg CTRL1_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_dev_boot_get(l20g20is_ctx_t *ctx, uint8_t *val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  *val = (uint8_t)ctrl1_ois.boot;

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sw_rst in reg CTRL2_OIS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_dev_reset_set(l20g20is_ctx_t *ctx, uint8_t val)
{
  l20g20is_ctrl2_ois_t ctrl2_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  if(ret == 0){
    ctrl2_ois.sw_rst = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  }
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user
  *          registers.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sw_rst in reg CTRL2_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_dev_reset_get(l20g20is_ctx_t *ctx, uint8_t *val)
{
  l20g20is_ctrl2_ois_t ctrl2_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  *val = (uint8_t)ctrl2_ois.sw_rst;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    L20G20IS_Filters
  * @brief      This section group all the functions concerning the
  *              filters configuration.
  * @{
  *
  */

/**
  * @brief  Gyroscope high-pass filter bandwidth selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "hpf" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_filter_hp_bandwidth_set(l20g20is_ctx_t *ctx,
                                            l20g20is_gy_hp_bw_t val)
{
  l20g20is_ctrl2_ois_t ctrl2_ois;
  l20g20is_ois_cfg_reg_t ois_cfg_reg;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  if(ret == 0){
    ctrl2_ois.hpf = ((uint8_t)val & 0x80U) >> 4;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL2_OIS,
                             (uint8_t*)&ctrl2_ois, 1);
  }
  if(ret == 0){
    ret = l20g20is_read_reg(ctx, L20G20IS_OIS_CFG_REG,
                            (uint8_t*)&ois_cfg_reg, 1);
  }
  if(ret == 0){
    ois_cfg_reg.hpf_bw = (uint8_t)val & 0x03U;
    ret = l20g20is_write_reg(ctx, L20G20IS_OIS_CFG_REG,
                             (uint8_t*)&ois_cfg_reg, 1);
  }

  return ret;
}

/**
  * @brief  Gyroscope high-pass filter bandwidth selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hpf in reg CTRL2_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_filter_hp_bandwidth_get(l20g20is_ctx_t *ctx,
                                            l20g20is_gy_hp_bw_t *val)
{
  l20g20is_ctrl2_ois_t ctrl2_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  switch (ctrl2_ois.hpf){
    case L20G20IS_HPF_BYPASS:
      *val = L20G20IS_HPF_BYPASS;
      break;
    case L20G20IS_HPF_BW_23mHz:
      *val = L20G20IS_HPF_BW_23mHz;
      break;
    case L20G20IS_HPF_BW_91mHz:
      *val = L20G20IS_HPF_BW_91mHz;
      break;
    case L20G20IS_HPF_BW_324mHz:
      *val = L20G20IS_HPF_BW_324mHz;
      break;
    case L20G20IS_HPF_BW_1Hz457:
      *val = L20G20IS_HPF_BW_1Hz457;
      break;
    default:
      *val = L20G20IS_HPF_BYPASS;
      break;
  }
  return ret;
}

/**
  * @brief  Gyroscope high-pass filter reset.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of hp_rst in reg CTRL2_OIS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_filter_hp_reset_set(l20g20is_ctx_t *ctx, uint8_t val)
{
  l20g20is_ctrl2_ois_t ctrl2_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  if(ret == 0){
    ctrl2_ois.hp_rst = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope high-pass filter reset.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hp_rst in reg CTRL2_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_filter_hp_reset_get(l20g20is_ctx_t *ctx, uint8_t *val)
{
  l20g20is_ctrl2_ois_t ctrl2_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  *val = (uint8_t)ctrl2_ois.hp_rst;

  return ret;
}

/**
  * @brief   Gyroscope filter low pass cutoff frequency selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "lpf_bw" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_filter_lp_bandwidth_set(l20g20is_ctx_t *ctx,
                                            l20g20is_gy_lp_bw_t val)
{
  l20g20is_ctrl2_ois_t ctrl2_ois;
  l20g20is_ctrl3_ois_t ctrl3_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  if(ret == 0){
    ctrl2_ois.lpf_bw = (uint8_t)val & 0x03U;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
  }
  if(ret == 0){
    ret = l20g20is_read_reg(ctx, L20G20IS_CTRL3_OIS,
                              (uint8_t*)&ctrl3_ois, 1);
  }
  if(ret == 0){
    ctrl3_ois.lpf_bw = ((uint8_t)val & 0x04U) >> 2;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL3_OIS,
                               (uint8_t*)&ctrl3_ois, 1);
  }
  return ret;
}

/**
  * @brief   Gyroscope filter low pass cutoff frequency selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of lpf_bw in reg CTRL2_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_filter_lp_bandwidth_get(l20g20is_ctx_t *ctx,
                                            l20g20is_gy_lp_bw_t *val)
{
  l20g20is_ctrl2_ois_t ctrl2_ois;
  l20g20is_ctrl3_ois_t ctrl3_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL3_OIS,(uint8_t*)&ctrl3_ois, 1);
  if(ret == 0){
    ret = l20g20is_read_reg(ctx, L20G20IS_CTRL2_OIS, (uint8_t*)&ctrl2_ois, 1);
    switch ( (ctrl3_ois.lpf_bw << 2) + ctrl2_ois.lpf_bw){
      case L20G20IS_LPF_BW_1150Hz:
        *val = L20G20IS_LPF_BW_1150Hz;
        break;
      case L20G20IS_LPF_BW_290Hz:
        *val = L20G20IS_LPF_BW_290Hz;
        break;
      case L20G20IS_LPF_BW_210Hz:
        *val = L20G20IS_LPF_BW_210Hz;
        break;
      case L20G20IS_LPF_BW_160Hz:
        *val = L20G20IS_LPF_BW_160Hz;
        break;
      case L20G20IS_LPF_BW_450Hz:
        *val = L20G20IS_LPF_BW_450Hz;
        break;
      default:
        *val = L20G20IS_LPF_BW_290Hz;
        break;
    }
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    L20G20IS_Serial_interface
  * @brief      This section groups all the functions concerning main
  *              serial interface management.
  * @{
  *
  */

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "sim" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_spi_mode_set(l20g20is_ctx_t *ctx, l20g20is_sim_t val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  if(ret == 0){
    ctrl1_ois.sim = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sim in reg CTRL1_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_spi_mode_get(l20g20is_ctx_t *ctx, l20g20is_sim_t *val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  switch (ctrl1_ois.sim){
    case L20G20IS_SPI_4_WIRE:
      *val = L20G20IS_SPI_4_WIRE;
      break;
    case L20G20IS_SPI_3_WIRE:
      *val = L20G20IS_SPI_3_WIRE;
      break;
    default:
      *val = L20G20IS_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    L20G20IS_Interrupt_pins
  * @brief       This section groups all the functions that manage
  *              interrupt pins.
  * @{
  *
  */

/**
  * @brief  Latched/pulsed interrupt.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "dr_pulsed" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_pin_notification_set(l20g20is_ctx_t *ctx,
                                      l20g20is_lir_t val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  if(ret == 0){
    ctrl1_ois.dr_pulsed = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL1_OIS,
                             (uint8_t*)&ctrl1_ois, 1);
  }
  return ret;
}

/**
  * @brief  Latched/pulsed interrupt.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dr_pulsed in reg CTRL1_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_pin_notification_get(l20g20is_ctx_t *ctx,
                                      l20g20is_lir_t *val)
{
  l20g20is_ctrl1_ois_t ctrl1_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL1_OIS, (uint8_t*)&ctrl1_ois, 1);
  switch (ctrl1_ois.dr_pulsed){
    case L20G20IS_INT_LATCHED:
      *val = L20G20IS_INT_LATCHED;
      break;
    case L20G20IS_INT_PULSED:
      *val = L20G20IS_INT_PULSED;
      break;
    default:
      *val = L20G20IS_INT_LATCHED;
      break;
  }
  return ret;
}

/**
  * @brief  Interrupt pin active-high/low.Interrupt active-high/low.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "h_l_active" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_pin_polarity_set(l20g20is_ctx_t *ctx,
                                  l20g20is_pin_pol_t val)
{
  l20g20is_ctrl3_ois_t ctrl3_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL3_OIS, (uint8_t*)&ctrl3_ois, 1);
  if(ret == 0){
    ctrl3_ois.h_l_active = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL3_OIS,
                             (uint8_t*)&ctrl3_ois, 1);
  }
  return ret;
}

/**
  * @brief  Interrupt pin active-high/low.Interrupt active-high/low.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of h_l_active in reg CTRL3_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_pin_polarity_get(l20g20is_ctx_t *ctx,
                                  l20g20is_pin_pol_t *val)
{
  l20g20is_ctrl3_ois_t ctrl3_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL3_OIS, (uint8_t*)&ctrl3_ois, 1);
  switch (ctrl3_ois.h_l_active){
    case L20G20IS_ACTIVE_HIGH:
      *val = L20G20IS_ACTIVE_HIGH;
      break;
    case L20G20IS_ACTIVE_LOW:
      *val = L20G20IS_ACTIVE_LOW;
      break;
    default:
      *val = L20G20IS_ACTIVE_HIGH;
      break;
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "drdy_od" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_pin_mode_set(l20g20is_ctx_t *ctx, l20g20is_pp_od_t val)
{
  l20g20is_ctrl4_ois_t ctrl4_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL4_OIS, (uint8_t*)&ctrl4_ois, 1);
  if(ret == 0){
    ctrl4_ois.drdy_od = (uint8_t)val;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL4_OIS, (uint8_t*)&ctrl4_ois, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of drdy_od in reg CTRL4_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_pin_mode_get(l20g20is_ctx_t *ctx, l20g20is_pp_od_t *val)
{
  l20g20is_ctrl4_ois_t ctrl4_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL4_OIS, (uint8_t*)&ctrl4_ois, 1);
  switch (ctrl4_ois.drdy_od){
    case L20G20IS_PUSH_PULL:
      *val = L20G20IS_PUSH_PULL;
      break;
    case L20G20IS_OPEN_DRAIN:
      *val = L20G20IS_OPEN_DRAIN;
      break;
    default:
      *val = L20G20IS_PUSH_PULL;
      break;
  }
  return ret;
}

/**
  * @brief  Route a signal on DRDY pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    "temp_data_on_drdy" Temperature data-ready signal.
  *                "drdy_en" Angular rate data-ready signal.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_pin_drdy_route_set(l20g20is_ctx_t *ctx,
                                    l20g20is_pin_drdy_route_t val)
{
  l20g20is_ctrl4_ois_t ctrl4_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL4_OIS, (uint8_t*)&ctrl4_ois, 1);
  if(ret == 0) {
    ctrl4_ois.drdy_en  = val.drdy_en;
    ctrl4_ois.temp_data_on_drdy  = val.temp_data_on_drdy;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL4_OIS, (uint8_t*)&ctrl4_ois, 1);
  }
  return ret;
}

/**
  * @brief  Route a signal on DRDY pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    "temp_data_on_drdy" Temperature data-ready signal.
  *                "drdy_en" Angular rate data-ready signal.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_pin_drdy_route_get(l20g20is_ctx_t *ctx,
                                    l20g20is_pin_drdy_route_t *val)
{
  l20g20is_ctrl4_ois_t ctrl4_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL4_OIS, (uint8_t*)&ctrl4_ois, 1);
  val->temp_data_on_drdy = ctrl4_ois.temp_data_on_drdy;
  val->drdy_en = ctrl4_ois.drdy_en;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    L20G20IS_Self_test
  * @brief       This section groups all the functions that manage self
  *              test configuration.
  * @{
  *
  */

/**
  * @brief  Enable/disable self-test mode for gyroscope.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "st_en" in reg L20G20IS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_self_test_set(l20g20is_ctx_t *ctx,
                                  l20g20is_gy_self_test_t val)
{
  l20g20is_ctrl3_ois_t ctrl3_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL3_OIS, (uint8_t*)&ctrl3_ois, 1);
  if(ret == 0){
    ctrl3_ois.st_en = ((uint8_t)val & 0x02U) >> 1;
    ctrl3_ois.st_sign = (uint8_t)val & 0x01U;
    ret = l20g20is_write_reg(ctx, L20G20IS_CTRL3_OIS,
                             (uint8_t*)&ctrl3_ois, 1);
  }
  return ret;
}

/**
  * @brief  Enable/disable self-test mode for gyroscope.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st_en in reg CTRL3_OIS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t l20g20is_gy_self_test_get(l20g20is_ctx_t *ctx,
                                  l20g20is_gy_self_test_t *val)
{
  l20g20is_ctrl3_ois_t ctrl3_ois;
  int32_t ret;

  ret = l20g20is_read_reg(ctx, L20G20IS_CTRL3_OIS, (uint8_t*)&ctrl3_ois, 1);
  switch ((ctrl3_ois.st_en << 1) + ctrl3_ois.st_sign){
    case L20G20IS_ST_DISABLE:
      *val = L20G20IS_ST_DISABLE;
      break;
    case L20G20IS_ST_POSITIVE:
      *val = L20G20IS_ST_POSITIVE;
      break;
    case L20G20IS_ST_NEGATIVE:
      *val = L20G20IS_ST_NEGATIVE;
      break;
    default:
      *val = L20G20IS_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/