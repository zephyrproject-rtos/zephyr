/*
 ******************************************************************************
 * @file    lsm9ds1_reg.c
 * @author  Sensors Software Solution Team
 * @brief   LSM9DS1 driver file
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
#include "lsm9ds1_reg.h"

/**
  * @defgroup    LSM9DS1
  * @brief       This file provides a set of functions needed to drive the
  *              lsm9ds1 enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup    LSM9DS1_Interfaces_Functions
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
int32_t lsm9ds1_read_reg(lsm9ds1_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lsm9ds1_write_reg(lsm9ds1_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LSM9DS1_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t lsm9ds1_from_fs2g_to_mg(int16_t lsb)
{
  return ((float_t)lsb *0.061f);
}

float_t lsm9ds1_from_fs4g_to_mg(int16_t lsb)
{
  return ((float_t)lsb *0.122f);
}

float_t lsm9ds1_from_fs8g_to_mg(int16_t lsb)
{
  return ((float_t)lsb *0.244f);
}

float_t lsm9ds1_from_fs16g_to_mg(int16_t lsb)
{
  return ((float_t)lsb *0.732f);
}

float_t lsm9ds1_from_fs245dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb *8.75f);
}

float_t lsm9ds1_from_fs500dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb *17.50f);
}

float_t lsm9ds1_from_fs2000dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb *70.0f);
}

float_t lsm9ds1_from_fs4gauss_to_mG(int16_t lsb)
{
  return ((float_t)lsb *0.14f);
}

float_t lsm9ds1_from_fs8gauss_to_mG(int16_t lsb)
{
  return ((float_t)lsb *0.29f);
}

float_t lsm9ds1_from_fs12gauss_to_mG(int16_t lsb)
{
  return ((float_t)lsb *0.43f);
}

float_t lsm9ds1_from_fs16gauss_to_mG(int16_t lsb)
{
  return ((float_t)lsb *0.58f);
}

float_t lsm9ds1_from_lsb_to_celsius(int16_t lsb)
{
  return (((float_t)lsb / 16.0f) + 25.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup   LSM9DS1_Data_generation
  * @brief      This section groups all the functions concerning data
  *             generation
  * @{
  *
  */

/**
  * @brief  Gyroscope full-scale selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fs_g" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_full_scale_set(lsm9ds1_ctx_t *ctx, lsm9ds1_gy_fs_t val)
{
  lsm9ds1_ctrl_reg1_g_t ctrl_reg1_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_G, (uint8_t*)&ctrl_reg1_g, 1);
  if(ret == 0){
    ctrl_reg1_g.fs_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG1_G,
                            (uint8_t*)&ctrl_reg1_g, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fs_g in reg CTRL_REG1_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_full_scale_get(lsm9ds1_ctx_t *ctx, lsm9ds1_gy_fs_t *val)
{
  lsm9ds1_ctrl_reg1_g_t ctrl_reg1_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_G, (uint8_t*)&ctrl_reg1_g, 1);
  switch (ctrl_reg1_g.fs_g){
    case LSM9DS1_245dps:
      *val = LSM9DS1_245dps;
      break;
    case LSM9DS1_500dps:
      *val = LSM9DS1_500dps;
      break;
    case LSM9DS1_2000dps:
      *val = LSM9DS1_2000dps;
      break;
    default:
      *val = LSM9DS1_245dps;
      break;
  }
  return ret;
}

/**
  * @brief  Data rate selection when both the accelerometer and gyroscope
  *         are activated.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "odr_g" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_imu_data_rate_set(lsm9ds1_ctx_t *ctx, lsm9ds1_imu_odr_t val)
{
  lsm9ds1_ctrl_reg1_g_t ctrl_reg1_g;
  lsm9ds1_ctrl_reg6_xl_t ctrl_reg6_xl;
  lsm9ds1_ctrl_reg3_g_t ctrl_reg3_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_G, (uint8_t*)&ctrl_reg1_g, 1);
  if(ret == 0){
    ctrl_reg1_g.odr_g = (uint8_t)val & 0x07U;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG1_G,
                            (uint8_t*)&ctrl_reg1_g, 1);
  }
  if(ret == 0){
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                         (uint8_t*)&ctrl_reg6_xl, 1);
  }
  if(ret == 0){
    ctrl_reg6_xl.odr_xl = (((uint8_t)val & 0x70U) >> 4);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                            (uint8_t*)&ctrl_reg6_xl, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_G,
                           (uint8_t*)&ctrl_reg3_g, 1);
  }
  if(ret == 0){
    ctrl_reg3_g.lp_mode = (((uint8_t)val & 0x80U) >> 7);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG3_G,
                            (uint8_t*)&ctrl_reg3_g, 1);
  }

  return ret;
}

/**
  * @brief  Data rate selection when both the accelerometer and gyroscope
  *         are activated.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odr_g in reg CTRL_REG1_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_imu_data_rate_get(lsm9ds1_ctx_t *ctx, lsm9ds1_imu_odr_t *val)
{
  lsm9ds1_ctrl_reg1_g_t ctrl_reg1_g;
  lsm9ds1_ctrl_reg6_xl_t ctrl_reg6_xl;
  lsm9ds1_ctrl_reg3_g_t ctrl_reg3_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_G, (uint8_t*)&ctrl_reg1_g, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                           (uint8_t*)&ctrl_reg6_xl, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_G,
                           (uint8_t*)&ctrl_reg3_g, 1);
  }
  switch ((ctrl_reg3_g.lp_mode << 7) | (ctrl_reg6_xl.odr_xl << 4) |
          ctrl_reg1_g.odr_g){
    case LSM9DS1_IMU_OFF:
      *val = LSM9DS1_IMU_OFF;
      break;
    case LSM9DS1_GY_OFF_XL_10Hz:
      *val = LSM9DS1_GY_OFF_XL_10Hz;
      break;
    case LSM9DS1_GY_OFF_XL_50Hz:
      *val = LSM9DS1_GY_OFF_XL_50Hz;
      break;
    case LSM9DS1_GY_OFF_XL_119Hz:
      *val = LSM9DS1_GY_OFF_XL_119Hz;
      break;
    case LSM9DS1_GY_OFF_XL_238Hz:
      *val = LSM9DS1_GY_OFF_XL_238Hz;
      break;
    case LSM9DS1_GY_OFF_XL_476Hz:
      *val = LSM9DS1_GY_OFF_XL_476Hz;
      break;
    case LSM9DS1_GY_OFF_XL_952Hz:
      *val = LSM9DS1_GY_OFF_XL_952Hz;
      break;
    case LSM9DS1_XL_OFF_GY_14Hz9:
      *val = LSM9DS1_XL_OFF_GY_14Hz9;
      break;
    case LSM9DS1_XL_OFF_GY_59Hz5:
      *val = LSM9DS1_XL_OFF_GY_59Hz5;
      break;
    case LSM9DS1_XL_OFF_GY_119Hz:
      *val = LSM9DS1_XL_OFF_GY_119Hz;
      break;
    case LSM9DS1_XL_OFF_GY_238Hz:
      *val = LSM9DS1_XL_OFF_GY_238Hz;
      break;
    case LSM9DS1_XL_OFF_GY_476Hz:
      *val = LSM9DS1_XL_OFF_GY_476Hz;
      break;
    case LSM9DS1_XL_OFF_GY_952Hz:
      *val = LSM9DS1_XL_OFF_GY_952Hz;
      break;
    case LSM9DS1_IMU_14Hz9:
      *val = LSM9DS1_IMU_14Hz9;
      break;
    case LSM9DS1_IMU_59Hz5:
      *val = LSM9DS1_IMU_59Hz5;
      break;
    case LSM9DS1_IMU_119Hz:
      *val = LSM9DS1_IMU_119Hz;
      break;
    case LSM9DS1_IMU_238Hz:
      *val = LSM9DS1_IMU_238Hz;
      break;
    case LSM9DS1_IMU_476Hz:
      *val = LSM9DS1_IMU_476Hz;
      break;
    case LSM9DS1_IMU_952Hz:
      *val = LSM9DS1_IMU_952Hz;
      break;
    case LSM9DS1_XL_OFF_GY_14Hz9_LP:
      *val = LSM9DS1_XL_OFF_GY_14Hz9_LP;
      break;
    case LSM9DS1_XL_OFF_GY_59Hz5_LP:
      *val = LSM9DS1_XL_OFF_GY_59Hz5_LP;
      break;
    case LSM9DS1_XL_OFF_GY_119Hz_LP:
      *val = LSM9DS1_XL_OFF_GY_119Hz_LP;
      break;
    case LSM9DS1_IMU_14Hz9_LP:
      *val = LSM9DS1_IMU_14Hz9_LP;
      break;
    case LSM9DS1_IMU_59Hz5_LP:
      *val = LSM9DS1_IMU_59Hz5_LP;
      break;
    case LSM9DS1_IMU_119Hz_LP:
      *val = LSM9DS1_IMU_119Hz_LP;
      break;
    default:
      *val = LSM9DS1_IMU_OFF;
    break;
  }

  return ret;
}

/**
  * @brief   Configure gyro orientation.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Directional user orientation selection.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_orient_set(lsm9ds1_ctx_t *ctx, lsm9ds1_gy_orient_t val)
{
  lsm9ds1_orient_cfg_g_t orient_cfg_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_ORIENT_CFG_G,
                         (uint8_t*)&orient_cfg_g, 1);
  if(ret == 0) {
    orient_cfg_g.orient   = val.orient;
    orient_cfg_g.signx_g  = val.signx_g;
    orient_cfg_g.signy_g  = val.signy_g;
    orient_cfg_g.signz_g  = val.signz_g;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_ORIENT_CFG_G, (uint8_t*)&orient_cfg_g, 1);
  }
  return ret;
}

/**
  * @brief   Configure gyro orientation.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Directional user orientation selection.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_orient_get(lsm9ds1_ctx_t *ctx, lsm9ds1_gy_orient_t *val)
{
  lsm9ds1_orient_cfg_g_t orient_cfg_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_ORIENT_CFG_G,
                         (uint8_t*)&orient_cfg_g, 1);
  val->orient = orient_cfg_g.orient;
  val->signz_g = orient_cfg_g.signz_g;
  val->signy_g = orient_cfg_g.signy_g;
  val->signx_g = orient_cfg_g.signx_g;

  return ret;
}

/**
  * @brief  Accelerometer new data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "xlda" in reg STATUS_REG.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_flag_data_ready_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_status_reg_t status_reg;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_STATUS_REG, (uint8_t*)&status_reg, 1);
  *val = status_reg.xlda;

  return ret;
}

/**
  * @brief  Gyroscope new data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "gda" in reg STATUS_REG.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_flag_data_ready_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_status_reg_t status_reg;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_STATUS_REG, (uint8_t*)&status_reg, 1);
  *val = status_reg.gda;

  return ret;
}

/**
  * @brief  Temperature new data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "tda" in reg STATUS_REG.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_temp_flag_data_ready_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_status_reg_t status_reg;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_STATUS_REG, (uint8_t*)&status_reg, 1);
  *val = status_reg.tda;

  return ret;
}

/**
  * @brief  Enable gyroscope axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Gyroscope’s pitch axis (X) output enable.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_axis_set(lsm9ds1_ctx_t *ctx, lsm9ds1_gy_axis_t val)
{
  lsm9ds1_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0) {
    ctrl_reg4.xen_g = val.xen_g;
    ctrl_reg4.yen_g = val.yen_g;
    ctrl_reg4.zen_g = val.zen_g;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  }
  return ret;
}

/**
  * @brief  Enable gyroscope axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Gyroscope’s pitch axis (X) output enable.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_axis_get(lsm9ds1_ctx_t *ctx, lsm9ds1_gy_axis_t *val)
{
  lsm9ds1_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  val->xen_g = ctrl_reg4.xen_g;
  val->yen_g = ctrl_reg4.yen_g;
  val->zen_g = ctrl_reg4.zen_g;

  return ret;
}

/**
  * @brief  Enable accelerometer axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer’s X-axis output enable.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_axis_set(lsm9ds1_ctx_t *ctx, lsm9ds1_xl_axis_t val)
{
  lsm9ds1_ctrl_reg5_xl_t ctrl_reg5_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG5_XL,
                         (uint8_t*)&ctrl_reg5_xl, 1);
  if(ret == 0) {
    ctrl_reg5_xl.xen_xl = val.xen_xl;
    ctrl_reg5_xl.yen_xl = val.yen_xl;
    ctrl_reg5_xl.zen_xl = val.zen_xl;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG5_XL, (uint8_t*)&ctrl_reg5_xl, 1);
  }
  return ret;
}

/**
  * @brief  Enable accelerometer axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer’s X-axis output enable.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_axis_get(lsm9ds1_ctx_t *ctx, lsm9ds1_xl_axis_t *val)
{
  lsm9ds1_ctrl_reg5_xl_t ctrl_reg5_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG5_XL,
                         (uint8_t*)&ctrl_reg5_xl, 1);
  val->xen_xl = ctrl_reg5_xl.xen_xl;
  val->yen_xl = ctrl_reg5_xl.yen_xl;
  val->zen_xl = ctrl_reg5_xl.zen_xl;

  return ret;
}

/**
  * @brief  Decimation of acceleration data on OUT REG and FIFO.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "dec" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_decimation_set(lsm9ds1_ctx_t *ctx, lsm9ds1_dec_t val)
{
  lsm9ds1_ctrl_reg5_xl_t ctrl_reg5_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG5_XL,
                         (uint8_t*)&ctrl_reg5_xl, 1);
  if(ret == 0){
    ctrl_reg5_xl.dec = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG5_XL,
                            (uint8_t*)&ctrl_reg5_xl, 1);
  }
  return ret;
}

/**
  * @brief  Decimation of acceleration data on OUT REG and FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dec in reg CTRL_REG5_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_decimation_get(lsm9ds1_ctx_t *ctx, lsm9ds1_dec_t *val)
{
  lsm9ds1_ctrl_reg5_xl_t ctrl_reg5_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG5_XL,
                         (uint8_t*)&ctrl_reg5_xl, 1);
  switch (ctrl_reg5_xl.dec){
    case LSM9DS1_NO_DECIMATION:
      *val = LSM9DS1_NO_DECIMATION;
      break;
    case LSM9DS1_EVERY_2_SAMPLES:
      *val = LSM9DS1_EVERY_2_SAMPLES;
      break;
    case LSM9DS1_EVERY_4_SAMPLES:
      *val = LSM9DS1_EVERY_4_SAMPLES;
      break;
    case LSM9DS1_EVERY_8_SAMPLES:
      *val = LSM9DS1_EVERY_8_SAMPLES;
      break;
    default:
      *val = LSM9DS1_NO_DECIMATION;
      break;
  }
  return ret;
}

/**
  * @brief  Accelerometer full-scale selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fs_xl" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_full_scale_set(lsm9ds1_ctx_t *ctx, lsm9ds1_xl_fs_t val)
{
  lsm9ds1_ctrl_reg6_xl_t ctrl_reg6_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                         (uint8_t*)&ctrl_reg6_xl, 1);
  if(ret == 0){
    ctrl_reg6_xl.fs_xl = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                            (uint8_t*)&ctrl_reg6_xl, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fs_xl in reg CTRL_REG6_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_full_scale_get(lsm9ds1_ctx_t *ctx, lsm9ds1_xl_fs_t *val)
{
  lsm9ds1_ctrl_reg6_xl_t ctrl_reg6_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                         (uint8_t*)&ctrl_reg6_xl, 1);
  switch (ctrl_reg6_xl.fs_xl){
    case LSM9DS1_2g:
      *val = LSM9DS1_2g;
      break;
    case LSM9DS1_16g:
      *val = LSM9DS1_16g;
      break;
    case LSM9DS1_4g:
      *val = LSM9DS1_4g;
      break;
    case LSM9DS1_8g:
      *val = LSM9DS1_8g;
      break;
    default:
      *val = LSM9DS1_2g;
      break;
  }
  return ret;
}

/**
  * @brief  Blockdataupdate.[set]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Change the values of bdu in reg CTRL_REG8.
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_block_data_update_set(lsm9ds1_ctx_t *ctx_mag,
                                      lsm9ds1_ctx_t *ctx_imu, uint8_t val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  lsm9ds1_ctrl_reg5_m_t ctrl_reg5_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ctrl_reg8.bdu = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG5_M,
                           (uint8_t*)&ctrl_reg5_m, 1);
  }
  if(ret == 0){
    ctrl_reg5_m.fast_read = (uint8_t)(~val);
    ctrl_reg5_m.bdu = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_mag, LSM9DS1_CTRL_REG5_M,
                            (uint8_t*)&ctrl_reg5_m, 1);
  }

  return ret;
}

/**
  * @brief  Blockdataupdate.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Get the values of bdu in reg CTRL_REG8.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_block_data_update_get(lsm9ds1_ctx_t *ctx_mag,
                                      lsm9ds1_ctx_t *ctx_imu, uint8_t *val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  lsm9ds1_ctrl_reg5_m_t ctrl_reg5_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG5_M, (uint8_t*)&ctrl_reg5_m, 1);
    *val = (uint8_t)(ctrl_reg5_m.bdu & ctrl_reg8.bdu);
  }
  return ret;
}

/**
  * @brief  This register is a 16-bit register and represents the X offset
  *         used to compensate environmental effects (data is expressed as
  *         two’s complement).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data to be write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_offset_set(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_write_reg(ctx, LSM9DS1_OFFSET_X_REG_L_M, buff, 6);
  return ret;
}

/**
  * @brief  This register is a 16-bit register and represents the X offset
  *         used to compensate environmental effects (data is expressed as
  *         two’s complement).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_offset_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_OFFSET_X_REG_L_M, buff, 6);
  return ret;
}

/**
  * @brief  Magnetometer data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fast_odr" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_data_rate_set(lsm9ds1_ctx_t *ctx,
                                  lsm9ds1_mag_data_rate_t val)
{
  lsm9ds1_ctrl_reg1_m_t ctrl_reg1_m;
  lsm9ds1_ctrl_reg3_m_t ctrl_reg3_m;
  lsm9ds1_ctrl_reg4_m_t ctrl_reg4_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_M, (uint8_t*)&ctrl_reg1_m, 1);
  if(ret == 0){
    ctrl_reg1_m.fast_odr = (((uint8_t)val & 0x08U) >> 3);
    ctrl_reg1_m._do = ((uint8_t)val & 0x07U);
    ctrl_reg1_m.om = (((uint8_t)val & 0x30U) >> 4);
    ctrl_reg1_m.temp_comp = PROPERTY_ENABLE;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG1_M,
                            (uint8_t*)&ctrl_reg1_m, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_M,
                           (uint8_t*)&ctrl_reg3_m, 1);
  }
  if(ret == 0){
    ctrl_reg3_m.md = (((uint8_t)val & 0xC0U) >> 6);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG3_M,
                            (uint8_t*)&ctrl_reg3_m, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG4_M, (uint8_t*)&ctrl_reg4_m, 1);
  }
  if(ret == 0){
    ctrl_reg4_m.omz = (((uint8_t)val & 0x30U) >> 4);;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG4_M,
                            (uint8_t*)&ctrl_reg4_m, 1);
  }
  return ret;
}

/**
  * @brief  Magnetometer data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fast_odr in reg CTRL_REG1_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_data_rate_get(lsm9ds1_ctx_t *ctx,
                                  lsm9ds1_mag_data_rate_t *val)
{
  lsm9ds1_ctrl_reg1_m_t ctrl_reg1_m;
  lsm9ds1_ctrl_reg3_m_t ctrl_reg3_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_M, (uint8_t*)&ctrl_reg1_m, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_M, (uint8_t*)&ctrl_reg3_m, 1);
  }
  switch ((ctrl_reg3_m.md << 6) | (ctrl_reg1_m.om << 4) |
          (ctrl_reg1_m.fast_odr << 3) | ctrl_reg1_m._do){
    case LSM9DS1_MAG_POWER_DOWN:
      *val = LSM9DS1_MAG_POWER_DOWN;
      break;
    case LSM9DS1_MAG_LP_0Hz625:
      *val = LSM9DS1_MAG_LP_0Hz625;
      break;
    case LSM9DS1_MAG_LP_1Hz25:
      *val = LSM9DS1_MAG_LP_1Hz25;
      break;
    case LSM9DS1_MAG_LP_2Hz5:
      *val = LSM9DS1_MAG_LP_2Hz5;
      break;
    case LSM9DS1_MAG_LP_5Hz:
      *val = LSM9DS1_MAG_LP_5Hz;
      break;
    case LSM9DS1_MAG_LP_10Hz:
      *val = LSM9DS1_MAG_LP_10Hz;
      break;
    case LSM9DS1_MAG_LP_20Hz:
      *val = LSM9DS1_MAG_LP_20Hz;
      break;
    case LSM9DS1_MAG_LP_40Hz:
      *val = LSM9DS1_MAG_LP_40Hz;
      break;
    case LSM9DS1_MAG_LP_80Hz:
      *val = LSM9DS1_MAG_LP_80Hz;
      break;
    case LSM9DS1_MAG_MP_0Hz625:
      *val = LSM9DS1_MAG_MP_0Hz625;
      break;
    case LSM9DS1_MAG_MP_1Hz25:
      *val = LSM9DS1_MAG_MP_1Hz25;
      break;
    case LSM9DS1_MAG_MP_2Hz5:
      *val = LSM9DS1_MAG_MP_2Hz5;
      break;
    case LSM9DS1_MAG_MP_5Hz:
      *val = LSM9DS1_MAG_MP_5Hz;
      break;
    case LSM9DS1_MAG_MP_10Hz:
      *val = LSM9DS1_MAG_MP_10Hz;
      break;
    case LSM9DS1_MAG_MP_20Hz:
      *val = LSM9DS1_MAG_MP_20Hz;
      break;
    case LSM9DS1_MAG_MP_40Hz:
      *val = LSM9DS1_MAG_MP_40Hz;
      break;
    case LSM9DS1_MAG_MP_80Hz:
      *val = LSM9DS1_MAG_MP_80Hz;
      break;
    case LSM9DS1_MAG_HP_0Hz625:
      *val = LSM9DS1_MAG_HP_0Hz625;
      break;
    case LSM9DS1_MAG_HP_1Hz25:
      *val = LSM9DS1_MAG_HP_1Hz25;
      break;
    case LSM9DS1_MAG_HP_2Hz5:
      *val = LSM9DS1_MAG_HP_2Hz5;
      break;
    case LSM9DS1_MAG_HP_5Hz:
      *val = LSM9DS1_MAG_HP_5Hz;
      break;
    case LSM9DS1_MAG_HP_10Hz:
      *val = LSM9DS1_MAG_HP_10Hz;
      break;
    case LSM9DS1_MAG_HP_20Hz:
      *val = LSM9DS1_MAG_HP_20Hz;
      break;
    case LSM9DS1_MAG_HP_40Hz:
      *val = LSM9DS1_MAG_HP_40Hz;
      break;
    case LSM9DS1_MAG_HP_80Hz:
      *val = LSM9DS1_MAG_HP_80Hz;
      break;
    case LSM9DS1_MAG_UHP_0Hz625:
      *val = LSM9DS1_MAG_UHP_0Hz625;
      break;
    case LSM9DS1_MAG_UHP_1Hz25:
      *val = LSM9DS1_MAG_UHP_1Hz25;
      break;
    case LSM9DS1_MAG_UHP_2Hz5:
      *val = LSM9DS1_MAG_UHP_2Hz5;
      break;
    case LSM9DS1_MAG_UHP_5Hz:
      *val = LSM9DS1_MAG_UHP_5Hz;
      break;
    case LSM9DS1_MAG_UHP_10Hz:
      *val = LSM9DS1_MAG_UHP_10Hz;
      break;
    case LSM9DS1_MAG_UHP_20Hz:
      *val = LSM9DS1_MAG_UHP_20Hz;
      break;
    case LSM9DS1_MAG_UHP_40Hz:
      *val = LSM9DS1_MAG_UHP_40Hz;
      break;
    case LSM9DS1_MAG_UHP_80Hz:
      *val = LSM9DS1_MAG_UHP_80Hz;
      break;
    case LSM9DS1_MAG_UHP_155Hz:
      *val = LSM9DS1_MAG_UHP_155Hz;
      break;
    case LSM9DS1_MAG_HP_300Hz:
      *val = LSM9DS1_MAG_HP_300Hz;
      break;
    case LSM9DS1_MAG_MP_560Hz:
      *val = LSM9DS1_MAG_MP_560Hz;
      break;
    case LSM9DS1_MAG_LP_1000Hz:
      *val = LSM9DS1_MAG_LP_1000Hz;
      break;
    case LSM9DS1_MAG_ONE_SHOT:
      *val = LSM9DS1_MAG_ONE_SHOT;
      break;
    default:
      *val = LSM9DS1_MAG_POWER_DOWN;
      break;
  }
  return ret;
}

/**
  * @brief  Magnetometer full Scale Selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fs" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_full_scale_set(lsm9ds1_ctx_t *ctx, lsm9ds1_mag_fs_t val)
{
  lsm9ds1_ctrl_reg2_m_t ctrl_reg2_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG2_M, (uint8_t*)&ctrl_reg2_m, 1);
  if(ret == 0){
    ctrl_reg2_m.fs = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG2_M,
                            (uint8_t*)&ctrl_reg2_m, 1);
  }
  return ret;
}

/**
  * @brief  Magnetometer full scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fs in reg CTRL_REG2_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_full_scale_get(lsm9ds1_ctx_t *ctx, lsm9ds1_mag_fs_t *val)
{
  lsm9ds1_ctrl_reg2_m_t ctrl_reg2_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG2_M, (uint8_t*)&ctrl_reg2_m, 1);
  switch (ctrl_reg2_m.fs){
    case LSM9DS1_4Ga:
      *val = LSM9DS1_4Ga;
      break;
    case LSM9DS1_8Ga:
      *val = LSM9DS1_8Ga;
      break;
    case LSM9DS1_12Ga:
      *val = LSM9DS1_12Ga;
      break;
    case LSM9DS1_16Ga:
      *val = LSM9DS1_16Ga;
      break;
    default:
      *val = LSM9DS1_4Ga;
      break;
  }
  return ret;
}

/**
  * @brief  New data available from magnetometer.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "zyxda" in reg STATUS_REG_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_flag_data_ready_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_status_reg_m_t status_reg_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_STATUS_REG_M,
                         (uint8_t*)&status_reg_m, 1);
  *val = status_reg_m.zyxda;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup     LSM9DS1_Dataoutput
  * @brief        This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief  Temperature data output register (r). L and H registers
  *         together express a 16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_temperature_raw_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_OUT_TEMP_L, buff, 2);
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
int32_t lsm9ds1_angular_rate_raw_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_OUT_X_L_G, buff, 6);
  return ret;
}

/**
  * @brief  Linear acceleration output register. The value is expressed as
  *         a 16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_acceleration_raw_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_OUT_X_L_XL, buff, 6);
  return ret;
}

/**
  * @brief  Magnetic sensor. The value is expressed as a 16-bit word in
  *         two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores the data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_magnetic_raw_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_OUT_X_L_M, buff, 6);
  return ret;
}

/**
  * @brief  Internal measurement range overflow on magnetic value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "mroi" in reg INT_SRC_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_magnetic_overflow_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_int_src_m_t int_src_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_SRC_M, (uint8_t*)&int_src_m, 1);
  *val = int_src_m.mroi;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM9DS1_Common
  * @brief       This section groups common usefull functions.
  * @{
  *
  */

/**
  * @brief  DeviceWhoamI.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  buff      Buffer that stores the data read.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_dev_id_get(lsm9ds1_ctx_t *ctx_mag, lsm9ds1_ctx_t *ctx_imu,
                           lsm9ds1_id_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_WHO_AM_I,
                         (uint8_t*)&(buff->imu), 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_WHO_AM_I_M,
                           (uint8_t*)&(buff->mag), 1);
  }
  return ret;
}

/**
  * @brief  Device status register.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Device status registers.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_dev_status_get(lsm9ds1_ctx_t *ctx_mag, lsm9ds1_ctx_t *ctx_imu,
                               lsm9ds1_status_t *val)
{
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_STATUS_REG,
                         (uint8_t*)&(val->status_imu), 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_STATUS_REG_M,
                           (uint8_t*)&(val->status_mag), 1);
  }

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[set]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val    Change the values of sw_reset in reg CTRL_REG8.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_dev_reset_set(lsm9ds1_ctx_t *ctx_mag, lsm9ds1_ctx_t *ctx_imu,
                              uint8_t val)
{
  lsm9ds1_ctrl_reg2_m_t ctrl_reg2_m;
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ctrl_reg8.sw_reset = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_CTRL_REG8,
                            (uint8_t*)&ctrl_reg8, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG2_M,
                           (uint8_t*)&ctrl_reg2_m, 1);
  }
  if(ret == 0){
    ctrl_reg2_m.soft_rst = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_mag, LSM9DS1_CTRL_REG2_M,
                            (uint8_t*)&ctrl_reg2_m, 1);
  }

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Get the values of sw_reset in reg CTRL_REG8.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_dev_reset_get(lsm9ds1_ctx_t *ctx_mag, lsm9ds1_ctx_t *ctx_imu,
                              uint8_t *val)
{
  lsm9ds1_ctrl_reg2_m_t ctrl_reg2_m;
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG2_M,
                           (uint8_t*)&ctrl_reg2_m, 1);
    *val = (uint8_t)(ctrl_reg2_m.soft_rst & ctrl_reg8.sw_reset);
  }
  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[set]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Change the values of "ble" in reg LSM9DS1.
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_dev_data_format_set(lsm9ds1_ctx_t *ctx_mag,
                                    lsm9ds1_ctx_t *ctx_imu,
                                    lsm9ds1_ble_t val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  lsm9ds1_ctrl_reg4_m_t ctrl_reg4_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8,
                         (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ctrl_reg8.ble = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_CTRL_REG8,
                            (uint8_t*)&ctrl_reg8, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG4_M,
                           (uint8_t*)&ctrl_reg4_m, 1);
  }
  if(ret == 0){
    ctrl_reg4_m.ble = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_mag, LSM9DS1_CTRL_REG4_M,
                            (uint8_t*)&ctrl_reg4_m, 1);
  }
  return ret;
}

/**
  * @brief  Big/Little Endian data selection.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Get the values of ble in reg CTRL_REG8.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_dev_data_format_get(lsm9ds1_ctx_t *ctx_mag,
                                    lsm9ds1_ctx_t *ctx_imu,
                                    lsm9ds1_ble_t *val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  lsm9ds1_ctrl_reg4_m_t ctrl_reg4_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG4_M,
                           (uint8_t*)&ctrl_reg4_m, 1);
  }
  switch (ctrl_reg8.ble & ctrl_reg4_m.ble){
    case LSM9DS1_LSB_LOW_ADDRESS:
      *val = LSM9DS1_LSB_LOW_ADDRESS;
      break;
    case LSM9DS1_MSB_LOW_ADDRESS:
      *val = LSM9DS1_MSB_LOW_ADDRESS;
      break;
    default:
      *val = LSM9DS1_LSB_LOW_ADDRESS;
      break;
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Change the values of boot in reg CTRL_REG8.
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_dev_boot_set(lsm9ds1_ctx_t *ctx_mag, lsm9ds1_ctx_t *ctx_imu,
                             uint8_t val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  lsm9ds1_ctrl_reg2_m_t ctrl_reg2_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8,
                         (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ctrl_reg8.boot = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_CTRL_REG8,
                            (uint8_t*)&ctrl_reg8, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG2_M,
                           (uint8_t*)&ctrl_reg2_m, 1);
  }
  if(ret == 0){
    ctrl_reg2_m.reboot = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_mag, LSM9DS1_CTRL_REG2_M,
                            (uint8_t*)&ctrl_reg2_m, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Get the values of boot in reg CTRL_REG8.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_dev_boot_get(lsm9ds1_ctx_t *ctx_mag, lsm9ds1_ctx_t *ctx_imu,
                             uint8_t *val)
{
  lsm9ds1_ctrl_reg2_m_t ctrl_reg2_m;
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
  ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG2_M,
                         (uint8_t*)&ctrl_reg2_m, 1);
    *val = (uint8_t)ctrl_reg2_m.reboot & ctrl_reg8.boot;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM9DS1_Filters
  * @brief       This section group all the functions concerning the
                 filters configuration
  * @{
  *
  */

/**
  * @brief  Reference value for gyroscope’s digital high-pass filter.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data to be write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_reference_set(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_write_reg(ctx, LSM9DS1_REFERENCE_G, buff, 1);
  return ret;
}

/**
  * @brief  Reference value for gyroscope’s digital high-pass filter.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_reference_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_REFERENCE_G, buff, 1);
  return ret;
}

/**
  * @brief  Gyroscope lowpass filter bandwidth selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "bw_g" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_lp_bandwidth_set(lsm9ds1_ctx_t *ctx,
                                           lsm9ds1_gy_lp_bw_t val)
{
  lsm9ds1_ctrl_reg1_g_t ctrl_reg1_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_G, (uint8_t*)&ctrl_reg1_g, 1);
  if(ret == 0){
    ctrl_reg1_g.bw_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG1_G,
                            (uint8_t*)&ctrl_reg1_g, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope lowpass filter bandwidth selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bw_g in reg CTRL_REG1_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_lp_bandwidth_get(lsm9ds1_ctx_t *ctx,
                                           lsm9ds1_gy_lp_bw_t *val)
{
  lsm9ds1_ctrl_reg1_g_t ctrl_reg1_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_G, (uint8_t*)&ctrl_reg1_g, 1);
  switch (ctrl_reg1_g.bw_g){
    case LSM9DS1_LP_STRONG:
      *val = LSM9DS1_LP_STRONG;
      break;
    case LSM9DS1_LP_MEDIUM:
      *val = LSM9DS1_LP_MEDIUM;
      break;
    case LSM9DS1_LP_LIGHT:
      *val = LSM9DS1_LP_LIGHT;
      break;
    case LSM9DS1_LP_ULTRA_LIGHT:
      *val = LSM9DS1_LP_ULTRA_LIGHT;
      break;
    default:
      *val = LSM9DS1_LP_STRONG;
      break;
  }
  return ret;
}

/**
  * @brief  Gyro output filter path configuration.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "out_sel" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_out_path_set(lsm9ds1_ctx_t *ctx,
                                       lsm9ds1_gy_out_path_t val)
{
  lsm9ds1_ctrl_reg2_g_t ctrl_reg2_g;
  lsm9ds1_ctrl_reg3_g_t ctrl_reg3_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG2_G,
                         (uint8_t*)&ctrl_reg2_g, 1);
  if(ret == 0){
    ctrl_reg2_g.out_sel = ((uint8_t)val & 0x03U);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG2_G,
                            (uint8_t*)&ctrl_reg2_g, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_G,
                           (uint8_t*)&ctrl_reg3_g, 1);
  }
  if(ret == 0){
    ctrl_reg3_g.hp_en = (((uint8_t)val & 0x10U) >> 4 );
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG3_G,
                            (uint8_t*)&ctrl_reg3_g, 1);
  }

  return ret;
}

/**
  * @brief  Gyro output filter path configuration.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of out_sel in reg CTRL_REG2_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_out_path_get(lsm9ds1_ctx_t *ctx,
                                       lsm9ds1_gy_out_path_t *val)
{
  lsm9ds1_ctrl_reg2_g_t ctrl_reg2_g;
  lsm9ds1_ctrl_reg3_g_t ctrl_reg3_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG2_G,
                         (uint8_t*)&ctrl_reg2_g, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_G,
                           (uint8_t*)&ctrl_reg3_g, 1);
  }
  switch ( (ctrl_reg3_g.hp_en << 4) | ctrl_reg2_g.out_sel){
    case LSM9DS1_LPF1_OUT:
      *val = LSM9DS1_LPF1_OUT;
      break;
    case LSM9DS1_LPF1_HPF_OUT:
      *val = LSM9DS1_LPF1_HPF_OUT;
      break;
    case LSM9DS1_LPF1_LPF2_OUT:
      *val = LSM9DS1_LPF1_LPF2_OUT;
      break;
    case LSM9DS1_LPF1_HPF_LPF2_OUT:
      *val = LSM9DS1_LPF1_HPF_LPF2_OUT;
      break;
    default:
      *val = LSM9DS1_LPF1_OUT;
      break;
  }

  return ret;
}

/**
  * @brief  Gyro interrupt filter path configuration.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "int_sel" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_int_path_set(lsm9ds1_ctx_t *ctx,
                                       lsm9ds1_gy_int_path_t val)
{
  lsm9ds1_ctrl_reg2_g_t ctrl_reg2_g;
  lsm9ds1_ctrl_reg3_g_t ctrl_reg3_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG2_G,
                         (uint8_t*)&ctrl_reg2_g, 1);
  if(ret == 0){
    ctrl_reg2_g.int_sel = ((uint8_t)val & 0x03U);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG2_G,
                            (uint8_t*)&ctrl_reg2_g, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_G,
                           (uint8_t*)&ctrl_reg3_g, 1);
  }
  if(ret == 0){
    ctrl_reg3_g.hp_en = (((uint8_t)val & 0x10U) >> 4 );
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG3_G,
                            (uint8_t*)&ctrl_reg3_g, 1);
  }
  return ret;
}

/**
  * @brief  Gyro interrupt filter path configuration.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int_sel in reg CTRL_REG2_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_int_path_get(lsm9ds1_ctx_t *ctx,
                                       lsm9ds1_gy_int_path_t *val)
{
  lsm9ds1_ctrl_reg2_g_t ctrl_reg2_g;
  lsm9ds1_ctrl_reg3_g_t ctrl_reg3_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG2_G,
                         (uint8_t*)&ctrl_reg2_g, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_G,
                           (uint8_t*)&ctrl_reg3_g, 1);
  }
  switch ( (ctrl_reg3_g.hp_en << 4) | ctrl_reg2_g.int_sel){
    case LSM9DS1_LPF1_INT:
      *val = LSM9DS1_LPF1_INT;
      break;
    case LSM9DS1_LPF1_HPF_INT:
      *val = LSM9DS1_LPF1_HPF_INT;
      break;
    case LSM9DS1_LPF1_LPF2_INT:
      *val = LSM9DS1_LPF1_LPF2_INT;
      break;
    case LSM9DS1_LPF1_HPF_LPF2_INT:
      *val = LSM9DS1_LPF1_HPF_LPF2_INT;
      break;
    default:
      *val = LSM9DS1_LPF1_INT;
      break;
  }
  return ret;
}

/**
  * @brief  Gyroscope high-pass filter bandwidth selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "hpcf_g" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_hp_bandwidth_set(lsm9ds1_ctx_t *ctx,
                                           lsm9ds1_gy_hp_bw_t val)
{
  lsm9ds1_ctrl_reg3_g_t ctrl_reg3_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_G, (uint8_t*)&ctrl_reg3_g, 1);
  if(ret == 0){
    ctrl_reg3_g.hpcf_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG3_G,
                            (uint8_t*)&ctrl_reg3_g, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope high-pass filter bandwidth selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hpcf_g in reg CTRL_REG3_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_filter_hp_bandwidth_get(lsm9ds1_ctx_t *ctx,
                                           lsm9ds1_gy_hp_bw_t *val)
{
  lsm9ds1_ctrl_reg3_g_t ctrl_reg3_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_G,
                         (uint8_t*)&ctrl_reg3_g, 1);
  switch (ctrl_reg3_g.hpcf_g){
    case LSM9DS1_HP_EXTREME:
      *val = LSM9DS1_HP_EXTREME;
      break;
    case LSM9DS1_HP_ULTRA_STRONG:
      *val = LSM9DS1_HP_ULTRA_STRONG;
      break;
    case LSM9DS1_HP_STRONG:
      *val = LSM9DS1_HP_STRONG;
      break;
    case LSM9DS1_HP_ULTRA_HIGH:
      *val = LSM9DS1_HP_ULTRA_HIGH;
      break;
    case LSM9DS1_HP_HIGH:
      *val = LSM9DS1_HP_HIGH;
      break;
    case LSM9DS1_HP_MEDIUM:
      *val = LSM9DS1_HP_MEDIUM;
      break;
    case LSM9DS1_HP_LOW:
      *val = LSM9DS1_HP_LOW;
      break;
    case LSM9DS1_HP_ULTRA_LOW:
      *val = LSM9DS1_HP_ULTRA_LOW;
      break;
    case LSM9DS1_HP_LIGHT:
      *val = LSM9DS1_HP_LIGHT;
      break;
    case LSM9DS1_HP_ULTRA_LIGHT:
      *val = LSM9DS1_HP_ULTRA_LIGHT;
      break;
    default:
      *val = LSM9DS1_HP_EXTREME;
      break;
  }
  return ret;
}

/**
  * @brief  Configure accelerometer anti aliasing filter.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "bw_xl" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_aalias_bandwidth_set(lsm9ds1_ctx_t *ctx,
                                               lsm9ds1_xl_aa_bw_t val)
{
  lsm9ds1_ctrl_reg6_xl_t ctrl_reg6_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                         (uint8_t*)&ctrl_reg6_xl, 1);
  if(ret == 0){
    ctrl_reg6_xl.bw_xl = ((uint8_t)val & 0x03U);
    ctrl_reg6_xl.bw_scal_odr = (((uint8_t)val & 0x10U) >> 4 );
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                            (uint8_t*)&ctrl_reg6_xl, 1);
  }
  return ret;
}

/**
  * @brief  Configure accelerometer anti aliasing filter.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bw_xl in reg CTRL_REG6_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_aalias_bandwidth_get(lsm9ds1_ctx_t *ctx,
                                               lsm9ds1_xl_aa_bw_t *val)
{
  lsm9ds1_ctrl_reg6_xl_t ctrl_reg6_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG6_XL,
                         (uint8_t*)&ctrl_reg6_xl, 1);
  switch ((ctrl_reg6_xl.bw_scal_odr << 4) | ctrl_reg6_xl.bw_xl){
    case LSM9DS1_AUTO:
      *val = LSM9DS1_AUTO;
      break;
    case LSM9DS1_408Hz:
      *val = LSM9DS1_408Hz;
      break;
    case LSM9DS1_211Hz:
      *val = LSM9DS1_211Hz;
      break;
    case LSM9DS1_105Hz:
      *val = LSM9DS1_105Hz;
      break;
    case LSM9DS1_50Hz:
      *val = LSM9DS1_50Hz;
      break;
    default:
      *val = LSM9DS1_AUTO;
      break;
  }
  return ret;
}

/**
  * @brief  Configure HP accelerometer filter.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "hpis1" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_int_path_set(lsm9ds1_ctx_t *ctx,
                                       lsm9ds1_xl_hp_path_t val)
{
  lsm9ds1_ctrl_reg7_xl_t ctrl_reg7_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                         (uint8_t*)&ctrl_reg7_xl, 1);
  if(ret == 0){
    ctrl_reg7_xl.hpis1 = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                            (uint8_t*)&ctrl_reg7_xl, 1);
  }
  return ret;
}

/**
  * @brief  .[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hpis1 in reg CTRL_REG7_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_int_path_get(lsm9ds1_ctx_t *ctx,
                                       lsm9ds1_xl_hp_path_t *val)
{
  lsm9ds1_ctrl_reg7_xl_t ctrl_reg7_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                         (uint8_t*)&ctrl_reg7_xl, 1);
  switch (ctrl_reg7_xl.hpis1){
    case LSM9DS1_HP_DIS:
      *val = LSM9DS1_HP_DIS;
      break;
    case LSM9DS1_HP_EN:
      *val = LSM9DS1_HP_EN;
      break;
    default:
      *val = LSM9DS1_HP_DIS;
      break;
  }
  return ret;
}

/**
  * @brief  Accelerometer output filter path configuration.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fds" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_out_path_set(lsm9ds1_ctx_t *ctx,
                                       lsm9ds1_xl_out_path_t val)
{
  lsm9ds1_ctrl_reg7_xl_t ctrl_reg7_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                         (uint8_t*)&ctrl_reg7_xl, 1);
  if(ret == 0){
    ctrl_reg7_xl.fds = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                            (uint8_t*)&ctrl_reg7_xl, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer output filter path configuration.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fds in reg CTRL_REG7_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_out_path_get(lsm9ds1_ctx_t *ctx,
                                       lsm9ds1_xl_out_path_t *val)
{
  lsm9ds1_ctrl_reg7_xl_t ctrl_reg7_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                         (uint8_t*)&ctrl_reg7_xl, 1);
  switch (ctrl_reg7_xl.fds){
    case LSM9DS1_LP_OUT:
      *val = LSM9DS1_LP_OUT;
      break;
    case LSM9DS1_HP_OUT:
      *val = LSM9DS1_HP_OUT;
      break;
    default:
      *val = LSM9DS1_LP_OUT;
      break;
  }
  return ret;
}

/**
  * @brief  Accelerometer digital filter low pass cutoff frequency
  *         selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "dcf" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_lp_bandwidth_set(lsm9ds1_ctx_t *ctx,
                                           lsm9ds1_xl_lp_bw_t val)
{
  lsm9ds1_ctrl_reg7_xl_t ctrl_reg7_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                         (uint8_t*)&ctrl_reg7_xl, 1);
  if(ret == 0){
    ctrl_reg7_xl.dcf = ((uint8_t)val & 0x10U) >> 4;
    ctrl_reg7_xl.hr = ((uint8_t)val & 0x03U);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                            (uint8_t*)&ctrl_reg7_xl, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer digital filter low pass cutoff frequency
  *         selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dcf in reg CTRL_REG7_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_lp_bandwidth_get(lsm9ds1_ctx_t *ctx,
                                           lsm9ds1_xl_lp_bw_t *val)
{
  lsm9ds1_ctrl_reg7_xl_t ctrl_reg7_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                         (uint8_t*)&ctrl_reg7_xl, 1);
  switch ((ctrl_reg7_xl.dcf << 4) + ctrl_reg7_xl.hr){
    case LSM9DS1_LP_DISABLE:
      *val = LSM9DS1_LP_DISABLE;
      break;
    case LSM9DS1_LP_ODR_DIV_50:
      *val = LSM9DS1_LP_ODR_DIV_50;
      break;
    case LSM9DS1_LP_ODR_DIV_100:
      *val = LSM9DS1_LP_ODR_DIV_100;
      break;
    case LSM9DS1_LP_ODR_DIV_9:
      *val = LSM9DS1_LP_ODR_DIV_9;
      break;
    case LSM9DS1_LP_ODR_DIV_400:
      *val = LSM9DS1_LP_ODR_DIV_400;
      break;
    default:
      *val = LSM9DS1_LP_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Accelerometer digital filter high pass cutoff frequency
  *         selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "dcf" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_hp_bandwidth_set(lsm9ds1_ctx_t *ctx,
                                           lsm9ds1_xl_hp_bw_t val)
{
  lsm9ds1_ctrl_reg7_xl_t ctrl_reg7_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                         (uint8_t*)&ctrl_reg7_xl, 1);
  if(ret == 0){
    ctrl_reg7_xl.dcf = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                            (uint8_t*)&ctrl_reg7_xl, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer digital filter high pass cutoff frequency
  *         selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dcf in reg CTRL_REG7_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_filter_hp_bandwidth_get(lsm9ds1_ctx_t *ctx,
                                           lsm9ds1_xl_hp_bw_t *val)
{
  lsm9ds1_ctrl_reg7_xl_t ctrl_reg7_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG7_XL,
                         (uint8_t*)&ctrl_reg7_xl, 1);

  switch (ctrl_reg7_xl.dcf){
    case LSM9DS1_HP_ODR_DIV_50:
      *val = LSM9DS1_HP_ODR_DIV_50;
      break;
    case LSM9DS1_HP_ODR_DIV_100:
      *val = LSM9DS1_HP_ODR_DIV_100;
      break;
    case LSM9DS1_HP_ODR_DIV_9:
      *val = LSM9DS1_HP_ODR_DIV_9;
      break;
    case LSM9DS1_HP_ODR_DIV_400:
      *val = LSM9DS1_HP_ODR_DIV_400;
      break;
    default:
      *val = LSM9DS1_HP_ODR_DIV_50;
      break;
  }
  return ret;
}

/**
  * @brief  Mask DRDY on pin (both XL & Gyro) until filter settling ends.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of drdy_mask_bit in reg CTRL_REG9.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_filter_settling_mask_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  if(ret == 0){
    ctrl_reg9.drdy_mask_bit = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  }
  return ret;
}

/**
  * @brief  Mask DRDY on pin (both XL & Gyro) until filter settling ends.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of drdy_mask_bit in reg CTRL_REG9.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_filter_settling_mask_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  *val = (uint8_t)ctrl_reg9.drdy_mask_bit;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup     LSM9DS1_Serial_interface
  * @brief        This section groups all the functions concerning main
  *               serial interface management (not auxiliary)
  * @{
  *
  */

/**
  * @brief  Register address automatically incremented during a multiple
  *         byte access with a serial interface.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "if_add_inc" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_auto_increment_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ctrl_reg8.if_add_inc = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  }
  return ret;
}

/**
  * @brief  Register address automatically incremented during a multiple
  *         byte access with a serial interface.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of if_add_inc in reg CTRL_REG8.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_auto_increment_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  *val = (uint8_t)ctrl_reg8.if_add_inc;
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Change the values of "sim" in reg LSM9DS1.
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_spi_mode_set(lsm9ds1_ctx_t *ctx_mag, lsm9ds1_ctx_t *ctx_imu,
                             lsm9ds1_sim_t val)
{
  lsm9ds1_ctrl_reg3_m_t ctrl_reg3_m;
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ctrl_reg8.sim = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  }
  if(ret == 0){
      ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG3_M,
                             (uint8_t*)&ctrl_reg3_m, 1);
  }
  if(ret == 0){
    ctrl_reg3_m.sim = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_mag, LSM9DS1_CTRL_REG3_M,
                            (uint8_t*)&ctrl_reg3_m, 1);
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Get the values of sim in reg CTRL_REG8.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_spi_mode_get(lsm9ds1_ctx_t *ctx_mag, lsm9ds1_ctx_t *ctx_imu,
                             lsm9ds1_sim_t *val)
{
  lsm9ds1_ctrl_reg3_m_t ctrl_reg3_m;
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
      ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG3_M,
                             (uint8_t*)&ctrl_reg3_m, 1);
  }
  switch (ctrl_reg8.sim & ctrl_reg3_m.sim){
    case LSM9DS1_SPI_4_WIRE:
      *val = LSM9DS1_SPI_4_WIRE;
      break;
    case LSM9DS1_SPI_3_WIRE:
      *val = LSM9DS1_SPI_3_WIRE;
      break;
    default:
      *val = LSM9DS1_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief  Enable / Disable I2C interface.[set]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Change the values of "i2c_disable" in reg LSM9DS1.
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_i2c_interface_set(lsm9ds1_ctx_t *ctx_mag,
                                  lsm9ds1_ctx_t *ctx_imu,
                                  lsm9ds1_i2c_dis_t val)
{
  lsm9ds1_ctrl_reg3_m_t ctrl_reg3_m;
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  if(ret == 0){
    ctrl_reg9.i2c_disable = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_CTRL_REG9,
                            (uint8_t*)&ctrl_reg9, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG3_M,
                           (uint8_t*)&ctrl_reg3_m, 1);
  }
  if(ret == 0){
    ctrl_reg3_m.i2c_disable = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_mag, LSM9DS1_CTRL_REG3_M,
                            (uint8_t*)&ctrl_reg3_m, 1);
  }
  return ret;
}

/**
  * @brief  Enable / Disable I2C interface.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Get the values of i2c_disable in reg CTRL_REG9.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_i2c_interface_get(lsm9ds1_ctx_t *ctx_mag,
                                  lsm9ds1_ctx_t *ctx_imu,
                                  lsm9ds1_i2c_dis_t *val)
{
  lsm9ds1_ctrl_reg3_m_t ctrl_reg3_m;
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_CTRL_REG3_M,
                           (uint8_t*)&ctrl_reg3_m, 1);
  }
  switch (ctrl_reg9.i2c_disable & ctrl_reg3_m.i2c_disable){
    case LSM9DS1_I2C_ENABLE:
      *val = LSM9DS1_I2C_ENABLE;
      break;
    case LSM9DS1_I2C_DISABLE:
      *val = LSM9DS1_I2C_DISABLE;
      break;
    default:
      *val = LSM9DS1_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup     LSM9DS1_Interrupt_pins
  * @brief        This section groups all the functions that manage
  *               interrupt pins
  * @{
  *
  */

/**
  * @brief  AND/OR combination of accelerometer’s interrupt events.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "aoi_xl" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_logic_set(lsm9ds1_ctx_t *ctx, lsm9ds1_pin_logic_t val)
{
  lsm9ds1_int_gen_cfg_xl_t int_gen_cfg_xl;
  lsm9ds1_int_gen_cfg_g_t int_gen_cfg_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_XL,
                         (uint8_t*)&int_gen_cfg_xl, 1);
  if(ret == 0){
    int_gen_cfg_xl.aoi_xl = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_CFG_XL,
                            (uint8_t*)&int_gen_cfg_xl, 1);
  }
  if(ret == 0){
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_G,
                         (uint8_t*)&int_gen_cfg_g, 1);
  }
  if(ret == 0){
    int_gen_cfg_g.aoi_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_CFG_G,
                            (uint8_t*)&int_gen_cfg_g, 1);
  }

  return ret;
}

/**
  * @brief  AND/OR combination of accelerometer’s interrupt events.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of aoi_xl in reg INT_GEN_CFG_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_logic_get(lsm9ds1_ctx_t *ctx, lsm9ds1_pin_logic_t *val)
{
  lsm9ds1_int_gen_cfg_xl_t int_gen_cfg_xl;
  lsm9ds1_int_gen_cfg_g_t int_gen_cfg_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_XL,
                         (uint8_t*)&int_gen_cfg_xl, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_G,
                           (uint8_t*)&int_gen_cfg_g, 1);
  }
  switch (int_gen_cfg_xl.aoi_xl & int_gen_cfg_g.aoi_g){
    case LSM9DS1_LOGIC_OR:
      *val = LSM9DS1_LOGIC_OR;
      break;
    case LSM9DS1_LOGIC_AND:
      *val = LSM9DS1_LOGIC_AND;
      break;
    default:
      *val = LSM9DS1_LOGIC_OR;
      break;
  }
  return ret;
}

/**
  * @brief  Route a signal on INT 1_A/G pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer data ready on INT 1_A/G pin.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_int1_route_set(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_pin_int1_route_t val)
{
  lsm9ds1_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  if(ret == 0) {
    int1_ctrl.int1_drdy_xl  = val.int1_drdy_xl;
    int1_ctrl.int1_drdy_g  = val.int1_drdy_g;
    int1_ctrl.int1_boot  = val.int1_boot;
    int1_ctrl.int1_fth  = val.int1_fth;
    int1_ctrl.int1_ovr  = val.int1_ovr;
    int1_ctrl.int1_fss5  = val.int1_fss5;
    int1_ctrl.int1_ig_xl  = val.int1_ig_xl;
    int1_ctrl.int1_ig_g  = val.int1_ig_g;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  }
  return ret;
}

/**
  * @brief  Route a signal on INT 1_A/G pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer data ready on INT 1_A/G pin.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_int1_route_get(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_pin_int1_route_t *val)
{
  lsm9ds1_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);

  val->int1_drdy_xl = int1_ctrl.int1_drdy_xl;
  val->int1_drdy_g = int1_ctrl.int1_drdy_g;
  val->int1_boot = int1_ctrl.int1_boot;
  val->int1_fth = int1_ctrl.int1_fth;
  val->int1_ovr = int1_ctrl.int1_ovr;
  val->int1_fss5 = int1_ctrl.int1_fss5;
  val->int1_ig_xl = int1_ctrl.int1_ig_xl;
  val->int1_ig_g = int1_ctrl.int1_ig_g;

  return ret;
}

/**
  * @brief  Route a signal on INT 2_A/G pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer data ready on INT2_A/G pin.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_int2_route_set(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_pin_int2_route_t val)
{
  lsm9ds1_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  if(ret == 0) {
    int2_ctrl.int2_drdy_xl  = val.int2_drdy_xl;
    int2_ctrl.int2_inact  = val.int2_inact;
    int2_ctrl.int2_drdy_g  = val.int2_drdy_g;
    int2_ctrl.int2_drdy_temp  = val.int2_drdy_temp;
    int2_ctrl.int2_fth  = val.int2_fth;
    int2_ctrl.int2_ovr  = val.int2_ovr;
    int2_ctrl.int2_fss5  = val.int2_fss5;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  }
  return ret;
}

/**
  * @brief  Route a signal on INT 2_A/G pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer data ready on INT2_A/G pin.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_int2_route_get(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_pin_int2_route_t *val)
{
  lsm9ds1_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  val->int2_drdy_xl = int2_ctrl.int2_drdy_xl;
  val->int2_inact = int2_ctrl.int2_inact;
  val->int2_fss5 = int2_ctrl.int2_fss5;
  val->int2_ovr = int2_ctrl.int2_ovr;
  val->int2_fth = int2_ctrl.int2_fth;
  val->int2_drdy_temp = int2_ctrl.int2_drdy_temp;
  val->int2_drdy_g = int2_ctrl.int2_drdy_g;

  return ret;
}

/**
  * @brief  Latched/pulsed interrupt.[set]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Change the values of "lir_xl1" in reg LSM9DS1.
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_notification_set(lsm9ds1_ctx_t *ctx_mag,
                                     lsm9ds1_ctx_t *ctx_imu,
                                     lsm9ds1_lir_t val)
{
  lsm9ds1_int_gen_cfg_g_t int_gen_cfg_g;
  lsm9ds1_int_cfg_m_t int_cfg_m;
  lsm9ds1_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG4,
                         (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0){
    ctrl_reg4.lir_xl1 = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_INT_GEN_CFG_G,
                           (uint8_t*)&int_gen_cfg_g, 1);
  }
  if(ret == 0){
    int_gen_cfg_g.lir_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_INT_GEN_CFG_G,
                            (uint8_t*)&int_gen_cfg_g, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_INT_CFG_M,
                           (uint8_t*)&int_cfg_m, 1);
  }
  if(ret == 0){
    int_cfg_m.iel = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_mag, LSM9DS1_INT_CFG_M,
                            (uint8_t*)&int_cfg_m, 1);
  }

  return ret;
}

/**
  * @brief  Configure the interrupt notification mode.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Get the values of iel in reg INT_CFG_M.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_notification_get(lsm9ds1_ctx_t *ctx_mag,
                                     lsm9ds1_ctx_t *ctx_imu,
                                     lsm9ds1_lir_t *val)
{
  lsm9ds1_int_cfg_m_t int_cfg_m;
  lsm9ds1_int_gen_cfg_g_t int_gen_cfg_g;
  lsm9ds1_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG4,
                         (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_INT_GEN_CFG_G,
                           (uint8_t*)&int_gen_cfg_g, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_INT_CFG_M,
                           (uint8_t*)&int_cfg_m, 1);
  }
  switch (int_cfg_m.iel & int_gen_cfg_g.lir_g & ctrl_reg4.lir_xl1){
    case LSM9DS1_INT_LATCHED:
      *val = LSM9DS1_INT_LATCHED;
      break;
    case LSM9DS1_INT_PULSED:
      *val = LSM9DS1_INT_PULSED;
      break;
    default:
      *val = LSM9DS1_INT_LATCHED;
      break;
  }

  return ret;
}
/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "pp_od" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_mode_set(lsm9ds1_ctx_t *ctx, lsm9ds1_pp_od_t val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  if(ret == 0){
    ctrl_reg8.pp_od = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of pp_od in reg CTRL_REG8.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_mode_get(lsm9ds1_ctx_t *ctx, lsm9ds1_pp_od_t *val)
{
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG8, (uint8_t*)&ctrl_reg8, 1);
  switch (ctrl_reg8.pp_od){
    case LSM9DS1_PUSH_PULL:
      *val = LSM9DS1_PUSH_PULL;
      break;
    case LSM9DS1_OPEN_DRAIN:
      *val = LSM9DS1_OPEN_DRAIN;
      break;
    default:
      *val = LSM9DS1_PUSH_PULL;
      break;
  }
  return ret;
}

/**
  * @brief  Route a signal on INT_M pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val      Interrupt enable on the INT_M pin.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_int_m_route_set(lsm9ds1_ctx_t *ctx,
                                    lsm9ds1_pin_m_route_t val)
{
  lsm9ds1_int_cfg_m_t int_cfg_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_CFG_M, (uint8_t*)&int_cfg_m, 1);
  if(ret == 0) {
    int_cfg_m.ien  = val.ien;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_CFG_M, (uint8_t*)&int_cfg_m, 1);
  }
  return ret;
}

/**
  * @brief  Route a signal on INT_M pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val      Interrupt enable on the INT_M pin.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_int_m_route_get(lsm9ds1_ctx_t *ctx,
                                    lsm9ds1_pin_m_route_t *val)
{
  lsm9ds1_int_cfg_m_t int_cfg_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_CFG_M, (uint8_t*)&int_cfg_m, 1);
  val->ien = int_cfg_m.ien;

  return ret;
}

/**
  * @brief  Configure the interrupt pin polarity.[set]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Change the values of "iea" in reg LSM9DS1.
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_polarity_set(lsm9ds1_ctx_t *ctx_mag,
                                 lsm9ds1_ctx_t *ctx_imu,
                                 lsm9ds1_polarity_t val)
{
  lsm9ds1_int_cfg_m_t int_cfg_m;
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_INT_CFG_M,
                         (uint8_t*)&int_cfg_m, 1);
  if(ret == 0){
    int_cfg_m.iea = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx_mag, LSM9DS1_INT_CFG_M,
                            (uint8_t*)&int_cfg_m, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8,
                           (uint8_t*)&ctrl_reg8, 1);
  }
  if(ret == 0){
    ctrl_reg8.h_lactive = (uint8_t)(~val);
    ret = lsm9ds1_write_reg(ctx_imu, LSM9DS1_CTRL_REG8,
                            (uint8_t*)&ctrl_reg8, 1);
  }

  return ret;
}

/**
  * @brief  Configure the interrupt pin polarity.[get]
  *
  * @param  ctx_mag   Read / write magnetometer interface definitions.(ptr)
  * @param  ctx_imu   Read / write imu interface definitions.(ptr)
  * @param  val       Get the values of iea in reg INT_CFG_M.(ptr)
  * @retval           Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_pin_polarity_get(lsm9ds1_ctx_t *ctx_mag,
                                 lsm9ds1_ctx_t *ctx_imu,
                                 lsm9ds1_polarity_t *val)
{
  lsm9ds1_int_cfg_m_t int_cfg_m;
  lsm9ds1_ctrl_reg8_t ctrl_reg8;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx_mag, LSM9DS1_INT_CFG_M, (uint8_t*)&int_cfg_m, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx_imu, LSM9DS1_CTRL_REG8,
                           (uint8_t*)&ctrl_reg8, 1);
  }
  switch (int_cfg_m.iea & (~ctrl_reg8.h_lactive)){
    case LSM9DS1_ACTIVE_LOW:
      *val = LSM9DS1_ACTIVE_LOW;
      break;
    case LSM9DS1_ACTIVE_HIGH:
      *val = LSM9DS1_ACTIVE_HIGH;
      break;
    default:
      *val = LSM9DS1_ACTIVE_LOW;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup     LSM9DS1_Interrupt_on_threshold
  * @brief        This section group all the functions concerning the
  *               interrupt on threshold configuration
  * @{
  *
  */

/**
  * @brief  Enable interrupt generation on threshold event.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Enable interrupt generation on accelerometer’s X-axis
  *                low event.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_trshld_axis_set(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_xl_trshld_en_t val)
{
  lsm9ds1_int_gen_cfg_xl_t int_gen_cfg_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_XL,
                         (uint8_t*)&int_gen_cfg_xl, 1);
  if(ret == 0) {
    int_gen_cfg_xl.xlie_xl  = val.xlie_xl;
    int_gen_cfg_xl.xhie_xl  = val.xhie_xl;
    int_gen_cfg_xl.ylie_xl  = val.ylie_xl;
    int_gen_cfg_xl.zhie_xl  = val.zhie_xl;
    int_gen_cfg_xl.yhie_xl  = val.yhie_xl;
    int_gen_cfg_xl.zlie_xl  = val.zlie_xl;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_CFG_XL, (uint8_t*)&int_gen_cfg_xl, 1);
  }

  return ret;
}

/**
  * @brief  Enable interrupt generation on threshold event.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Enable interrupt generation on accelerometer’s X-axis
  *                low event.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_trshld_axis_get(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_xl_trshld_en_t *val)
{
  lsm9ds1_int_gen_cfg_xl_t int_gen_cfg_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_XL,
                         (uint8_t*)&int_gen_cfg_xl, 1);
  val->xlie_xl = int_gen_cfg_xl.xlie_xl;
  val->xhie_xl = int_gen_cfg_xl.xhie_xl;
  val->ylie_xl = int_gen_cfg_xl.ylie_xl;
  val->yhie_xl = int_gen_cfg_xl.yhie_xl;
  val->zlie_xl = int_gen_cfg_xl.zlie_xl;
  val->zhie_xl = int_gen_cfg_xl.zhie_xl;

  return ret;
}

/**
  * @brief  Axis interrupt threshold.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data to be write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_trshld_set(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_X_XL, buff, 3);
  return ret;
}

/**
  * @brief  Axis interrupt threshold.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_trshld_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_X_XL, buff, 3);
  return ret;
}

/**
  * @brief  Enter/exit interrupt duration value.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of dur_xl in reg INT_GEN_DUR_XL.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_trshld_min_sample_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_int_gen_dur_xl_t int_gen_dur_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_DUR_XL,
                         (uint8_t*)&int_gen_dur_xl, 1);
  if(ret == 0){
    int_gen_dur_xl.dur_xl = (uint8_t)val;
    if (val != 0x00U){
      int_gen_dur_xl.wait_xl = PROPERTY_ENABLE;
    } else {
      int_gen_dur_xl.wait_xl = PROPERTY_DISABLE;
    }
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_DUR_XL,
                            (uint8_t*)&int_gen_dur_xl, 1);
  }

  return ret;
}

/**
  * @brief  Enter/exit interrupt duration value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dur_xl in reg INT_GEN_DUR_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_trshld_min_sample_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_int_gen_dur_xl_t int_gen_dur_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_DUR_XL,
                         (uint8_t*)&int_gen_dur_xl, 1);
  *val = (uint8_t)int_gen_dur_xl.dur_xl;

  return ret;
}

/**
  * @brief  Angular rate sensor interrupt on threshold source.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Pitch(X)low.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_src_get(lsm9ds1_ctx_t *ctx,
                                  lsm9ds1_gy_trshld_src_t *val)
{
  lsm9ds1_int_gen_src_g_t int_gen_src_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_SRC_G,
                         (uint8_t*)&int_gen_src_g, 1);
  val->xl_g = int_gen_src_g.xl_g;
  val->xh_g = int_gen_src_g.xh_g;
  val->yl_g = int_gen_src_g.yl_g;
  val->yh_g = int_gen_src_g.yh_g;
  val->zl_g = int_gen_src_g.zl_g;
  val->zh_g = int_gen_src_g.zh_g;
  val->ia_g = int_gen_src_g.ia_g;

  return ret;
}

/**
  * @brief  Accelerometer interrupt on threshold source.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Accelerometer’s X low. event.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_trshld_src_get(lsm9ds1_ctx_t *ctx,
                                  lsm9ds1_xl_trshld_src_t *val)
{
  lsm9ds1_int_gen_src_xl_t int_gen_src_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_SRC_XL,
                         (uint8_t*)&int_gen_src_xl, 1);
  val->xl_xl = int_gen_src_xl.xl_xl;
  val->xh_xl = int_gen_src_xl.xh_xl;
  val->yl_xl = int_gen_src_xl.yl_xl;
  val->yh_xl = int_gen_src_xl.yh_xl;
  val->zl_xl = int_gen_src_xl.zl_xl;
  val->zh_xl = int_gen_src_xl.zh_xl;
  val->ia_xl = int_gen_src_xl.ia_xl;

  return ret;
}

/**
  * @brief  Enable interrupt generation on threshold event.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Enable interrupt generation on gyroscope’s pitch
  *                (X) axis low event.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_axis_set(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_gy_trshld_en_t val)
{
  lsm9ds1_int_gen_cfg_g_t int_gen_cfg_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_G,
                         (uint8_t*)&int_gen_cfg_g, 1);
  if(ret == 0) {
    int_gen_cfg_g.xlie_g  = val.xlie_g;
    int_gen_cfg_g.xhie_g  = val.xhie_g;
    int_gen_cfg_g.ylie_g  = val.ylie_g;
    int_gen_cfg_g.yhie_g  = val.yhie_g;
    int_gen_cfg_g.zlie_g  = val.zlie_g;
    int_gen_cfg_g.zhie_g  = val.zhie_g;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_CFG_G,
                            (uint8_t*)&int_gen_cfg_g, 1);
  }
  return ret;
}

/**
  * @brief  Enable interrupt generation on threshold event.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Enable interrupt generation on gyroscope’s pitch
  *                 (X) axis low event.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_axis_get(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_gy_trshld_en_t *val)
{
  lsm9ds1_int_gen_cfg_g_t int_gen_cfg_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_G,
                         (uint8_t*)&int_gen_cfg_g, 1);
  val->xlie_g = int_gen_cfg_g.xlie_g;
  val->xhie_g = int_gen_cfg_g.xhie_g;
  val->ylie_g = int_gen_cfg_g.ylie_g;
  val->yhie_g = int_gen_cfg_g.yhie_g;
  val->zlie_g = int_gen_cfg_g.zlie_g;
  val->zhie_g = int_gen_cfg_g.zhie_g;
  return ret;
}

/**
  * @brief  Decrement or reset counter mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "dcrm_g" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_mode_set(lsm9ds1_ctx_t *ctx, lsm9ds1_dcrm_g_t val)
{
  lsm9ds1_int_gen_ths_xh_g_t int_gen_ths_xh_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_XH_G,
                         (uint8_t*)&int_gen_ths_xh_g, 1);
  if(ret == 0){
    int_gen_ths_xh_g.dcrm_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_XH_G,
                            (uint8_t*)&int_gen_ths_xh_g, 1);
  }
  return ret;
}

/**
  * @brief  Decrement or reset counter mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dcrm_g in reg INT_GEN_THS_XH_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_mode_get(lsm9ds1_ctx_t *ctx, lsm9ds1_dcrm_g_t *val)
{
  lsm9ds1_int_gen_ths_xh_g_t int_gen_ths_xh_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_XH_G,
                         (uint8_t*)&int_gen_ths_xh_g, 1);
  switch (int_gen_ths_xh_g.dcrm_g){
    case LSM9DS1_RESET_MODE:
      *val = LSM9DS1_RESET_MODE;
      break;
    case LSM9DS1_DECREMENT_MODE:
      *val = LSM9DS1_DECREMENT_MODE;
      break;
    default:
      *val = LSM9DS1_RESET_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  Angular rate sensor interrupt threshold on pitch (X) axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ths_g_x in reg INT_GEN_THS_XH_G.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_x_set(lsm9ds1_ctx_t *ctx, uint16_t val)
{
  lsm9ds1_int_gen_ths_xh_g_t int_gen_ths_xh_g;
  lsm9ds1_int_gen_ths_xl_g_t int_gen_ths_xl_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_XH_G,
                         (uint8_t*)&int_gen_ths_xh_g, 1);
  if(ret == 0){
    int_gen_ths_xh_g.ths_g_x = (uint8_t)((val & 0x7F00U) >> 8);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_XH_G,
                            (uint8_t*)&int_gen_ths_xh_g, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_XL_G,
                           (uint8_t*)&int_gen_ths_xl_g, 1);
  }
  if(ret == 0){
    int_gen_ths_xl_g.ths_g_x = (uint8_t)(val & 0x00FFU);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_XL_G,
                            (uint8_t*)&int_gen_ths_xl_g, 1);
  }

  return ret;
}

/**
  * @brief  Angular rate sensor interrupt threshold on pitch (X) axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ths_g_x in reg INT_GEN_THS_XH_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_x_get(lsm9ds1_ctx_t *ctx, uint16_t *val)
{
  lsm9ds1_int_gen_ths_xh_g_t int_gen_ths_xh_g;
  lsm9ds1_int_gen_ths_xl_g_t int_gen_ths_xl_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_XH_G,
                         (uint8_t*)&int_gen_ths_xh_g, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_XL_G,
                           (uint8_t*)&int_gen_ths_xl_g, 1);
  }
  *val = int_gen_ths_xh_g.ths_g_x;
  *val = *val << 8;
  *val += int_gen_ths_xl_g.ths_g_x;
  return ret;
}


/**
  * @brief  Angular rate sensor interrupt threshold on roll (Y) axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ths_g_y in reg INT_GEN_THS_YH_G.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_y_set(lsm9ds1_ctx_t *ctx, uint16_t val)
{
  lsm9ds1_int_gen_ths_yh_g_t int_gen_ths_yh_g;
  lsm9ds1_int_gen_ths_yl_g_t int_gen_ths_yl_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_YH_G,
                         (uint8_t*)&int_gen_ths_yh_g, 1);
  if(ret == 0){
    int_gen_ths_yh_g.ths_g_y = (uint8_t)((val & 0x7F00U) >> 8);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_YH_G,
                            (uint8_t*)&int_gen_ths_yh_g, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_YL_G,
                           (uint8_t*)&int_gen_ths_yl_g, 1);
  }
  if(ret == 0){
    int_gen_ths_yl_g.ths_g_y = (uint8_t)(val & 0x00FFU);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_YL_G,
                            (uint8_t*)&int_gen_ths_yl_g, 1);
  }

  return ret;
}

/**
  * @brief  Angular rate sensor interrupt threshold on roll (Y) axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ths_g_y in reg INT_GEN_THS_YH_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_y_get(lsm9ds1_ctx_t *ctx, uint16_t *val)
{
  lsm9ds1_int_gen_ths_yh_g_t int_gen_ths_yh_g;
  lsm9ds1_int_gen_ths_yl_g_t int_gen_ths_yl_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_YH_G,
                         (uint8_t*)&int_gen_ths_yh_g, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_YL_G,
                           (uint8_t*)&int_gen_ths_yl_g, 1);
  }
  *val = (uint8_t)int_gen_ths_yh_g.ths_g_y;
  *val = *val << 8;
  *val += int_gen_ths_yl_g.ths_g_y;
  return ret;
}

/**
  * @brief  Angular rate sensor interrupt thresholds on yaw (Z) axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ths_g_z in reg INT_GEN_THS_ZH_G.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_z_set(lsm9ds1_ctx_t *ctx, uint16_t val)
{
  lsm9ds1_int_gen_ths_zh_g_t int_gen_ths_zh_g;
  lsm9ds1_int_gen_ths_zl_g_t int_gen_ths_zl_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_ZH_G,
                         (uint8_t*)&int_gen_ths_zh_g, 1);
  if(ret == 0){
    int_gen_ths_zh_g.ths_g_z = (uint8_t)((val & 0x7F00U) >> 8);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_ZH_G,
                            (uint8_t*)&int_gen_ths_zh_g, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_ZL_G,
                           (uint8_t*)&int_gen_ths_zl_g, 1);
  }
  if(ret == 0){
    int_gen_ths_zl_g.ths_g_z = (uint8_t)(val & 0x00FFU);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_ZL_G,
                            (uint8_t*)&int_gen_ths_zl_g, 1);
  }

  return ret;
}

/**
  * @brief  Angular rate sensor interrupt thresholds on yaw (Z) axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ths_g_z in reg INT_GEN_THS_ZH_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_z_get(lsm9ds1_ctx_t *ctx, uint16_t *val)
{
  lsm9ds1_int_gen_ths_zh_g_t int_gen_ths_zh_g;
  lsm9ds1_int_gen_ths_zl_g_t int_gen_ths_zl_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_ZH_G,
                         (uint8_t*)&int_gen_ths_zh_g, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_ZL_G,
                           (uint8_t*)&int_gen_ths_zl_g, 1);
  }
  *val = int_gen_ths_zh_g.ths_g_z;
  *val = *val << 8;
  *val += int_gen_ths_zl_g.ths_g_z;

  return ret;
}

/**
  * @brief  Enter/exit interrupt duration value.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of dur_g in reg INT_GEN_DUR_G.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_min_sample_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_int_gen_dur_g_t int_gen_dur_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_DUR_G,
                         (uint8_t*)&int_gen_dur_g, 1);
  if(ret == 0){
    if (val != 0x00U){
      int_gen_dur_g.wait_g = PROPERTY_ENABLE;
    } else {
      int_gen_dur_g.wait_g = PROPERTY_DISABLE;
    }
    int_gen_dur_g.dur_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_DUR_G,
                            (uint8_t*)&int_gen_dur_g, 1);
  }

  return ret;
}

/**
  * @brief  Enter/exit interrupt duration value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dur_g in reg INT_GEN_DUR_G.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_trshld_min_sample_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_int_gen_dur_g_t int_gen_dur_g;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_DUR_G,
                         (uint8_t*)&int_gen_dur_g, 1);
  *val = (uint8_t)int_gen_dur_g.dur_g;

  return ret;
}

/**
  * @brief  Enable interrupt generation on threshold event.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Enable interrupt generation on Z-axis. .
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_trshld_axis_set(lsm9ds1_ctx_t *ctx,
                                    lsm9ds1_mag_trshld_axis_t val)
{
  lsm9ds1_int_cfg_m_t int_cfg_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_CFG_M, (uint8_t*)&int_cfg_m, 1);
  if(ret == 0) {
    int_cfg_m.zien  = val.zien;
    int_cfg_m.xien  = val.xien;
    int_cfg_m.yien  = val.yien;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_CFG_M, (uint8_t*)&int_cfg_m, 1);
  }

  return ret;
}

/**
  * @brief  Enable interrupt generation on threshold event.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     Enable interrupt generation on Z-axis. .(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_trshld_axis_get(lsm9ds1_ctx_t *ctx,
                                    lsm9ds1_mag_trshld_axis_t *val)
{
  lsm9ds1_int_cfg_m_t int_cfg_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_CFG_M, (uint8_t*)&int_cfg_m, 1);
  val->zien = int_cfg_m.zien;
  val->yien = int_cfg_m.yien;
  val->xien = int_cfg_m.xien;

  return ret;
}

/**
  * @brief  Magnetometer interrupt on threshold source.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val     This bit signals when the interrupt event occurs.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_trshld_src_get(lsm9ds1_ctx_t *ctx,
                                   lsm9ds1_mag_trshld_src_t *val)
{
  lsm9ds1_int_src_m_t int_src_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_SRC_M, (uint8_t*)&int_src_m, 1);
  val->_int = int_src_m._int;
  val->nth_z = int_src_m.nth_z;
  val->nth_y = int_src_m.nth_y;
  val->nth_x = int_src_m.nth_x;
  val->pth_z = int_src_m.pth_z;
  val->pth_y = int_src_m.pth_y;
  val->pth_x = int_src_m.pth_x;

  return ret;
}

/**
  * @brief  The value is expressed in 15-bit unsigned.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "ths" in reg INT_THS_L_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_trshld_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_int_ths_l_m_t int_ths_l_m;
  lsm9ds1_int_ths_h_m_t int_ths_h_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_THS_L_M,
                         (uint8_t*)&int_ths_l_m, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_THS_H_M,
                           (uint8_t*)&int_ths_h_m, 1);
  }

  *val = int_ths_h_m.ths;
  *val = *val << 8;
  *val += int_ths_l_m.ths;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup     LSM9DS1_ Activity/Inactivity_detection
  * @brief        This section groups all the functions concerning
  *               activity/inactivity detection.
  * @{
  *
  */

/**
  * @brief  Inactivity threshold.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of act_ths in reg ACT_THS.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_act_threshold_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_act_ths_t act_ths;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_ACT_THS, (uint8_t*)&act_ths, 1);
  if(ret == 0){
    act_ths.act_ths = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_ACT_THS, (uint8_t*)&act_ths, 1);
  }
  return ret;
}

/**
  * @brief  Inactivity threshold.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of act_ths in reg ACT_THS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_act_threshold_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_act_ths_t act_ths;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_ACT_THS, (uint8_t*)&act_ths, 1);
  *val = (uint8_t)act_ths.act_ths;

  return ret;
}

/**
  * @brief  Gyroscope operating mode during inactivity.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "sleep_on_inact_en" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_act_mode_set(lsm9ds1_ctx_t *ctx, lsm9ds1_act_mode_t val)
{
  lsm9ds1_act_ths_t act_ths;
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_ACT_THS, (uint8_t*)&act_ths, 1);
  if(ret == 0){
    act_ths.sleep_on_inact_en = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_ACT_THS, (uint8_t*)&act_ths, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  }
  if(ret == 0){
    ctrl_reg9.sleep_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  }

  return ret;
}

/**
  * @brief  Gyroscope operating mode during inactivity.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sleep_on_inact_en in reg ACT_THS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_act_mode_get(lsm9ds1_ctx_t *ctx, lsm9ds1_act_mode_t *val)
{
  lsm9ds1_act_ths_t act_ths;
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_ACT_THS, (uint8_t*)&act_ths, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  }
  switch (act_ths.sleep_on_inact_en & ctrl_reg9.sleep_g){
    case LSM9DS1_GYRO_POWER_DOWN:
      *val = LSM9DS1_GYRO_POWER_DOWN;
      break;
    case LSM9DS1_GYRO_SLEEP:
      *val = LSM9DS1_GYRO_SLEEP;
      break;
    default:
      *val = LSM9DS1_GYRO_POWER_DOWN;
      break;
  }

  return ret;
}

/**
  * @brief  Inactivity duration in number of sample.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data to be write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_act_duration_set(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_write_reg(ctx, LSM9DS1_ACT_DUR, buff, 1);
  return ret;
}

/**
  * @brief  Inactivity duration in number of sample.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_act_duration_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_ACT_DUR, buff, 1);
  return ret;
}

/**
  * @brief  Inactivity interrupt output signal.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Return an option of "lsm9ds1_inact_t".(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_act_src_get(lsm9ds1_ctx_t *ctx, lsm9ds1_inact_t *val)
{
  lsm9ds1_status_reg_t status_reg;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_STATUS_REG, (uint8_t*)&status_reg, 1);
  switch (status_reg.inact){
    case LSM9DS1_ACTIVITY:
      *val = LSM9DS1_ACTIVITY;
      break;
    case LSM9DS1_INACTIVITY:
      *val = LSM9DS1_INACTIVITY;
      break;
    default:
      *val = LSM9DS1_ACTIVITY;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup     LSM9DS1_ Six_position_detection(6D/4D).
  * @brief        This section groups all the functions concerning six
  *               position detection (6D).
  * @{
  *
  */

/**
  * @brief  6D feature working mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "6d" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_6d_mode_set(lsm9ds1_ctx_t *ctx, lsm9ds1_6d_mode_t val)
{
  lsm9ds1_int_gen_cfg_xl_t int_gen_cfg_xl;
  lsm9ds1_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_XL,
                         (uint8_t*)&int_gen_cfg_xl, 1);
  if(ret == 0){
    int_gen_cfg_xl._6d = ((uint8_t)val & 0x01U);
    int_gen_cfg_xl.aoi_xl = ( ( (uint8_t)val & 0x02U ) >> 1 );
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_CFG_XL,
                            (uint8_t*)&int_gen_cfg_xl, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG4,
                           (uint8_t*)&ctrl_reg4, 1);
  }
  if(ret == 0){
    ctrl_reg4._4d_xl1 = ( ( (uint8_t)val & 0x04U ) >> 2 );
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);
  }
  return ret;
}

/**
  * @brief  6D feature working mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of 6d in reg INT_GEN_CFG_XL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_6d_mode_get(lsm9ds1_ctx_t *ctx, lsm9ds1_6d_mode_t *val)
{
  lsm9ds1_int_gen_cfg_xl_t int_gen_cfg_xl;
  lsm9ds1_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_CFG_XL,
                         (uint8_t*)&int_gen_cfg_xl, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG4,
                           (uint8_t*)&ctrl_reg4, 1);
  }
  switch ( (ctrl_reg4._4d_xl1 << 2) | (int_gen_cfg_xl.aoi_xl << 1)
          | int_gen_cfg_xl.aoi_xl ){
    case LSM9DS1_POS_MOVE_RECO_DISABLE:
      *val = LSM9DS1_POS_MOVE_RECO_DISABLE;
      break;
    case LSM9DS1_6D_MOVE_RECO:
      *val = LSM9DS1_6D_MOVE_RECO;
      break;
    case LSM9DS1_4D_MOVE_RECO:
      *val = LSM9DS1_4D_MOVE_RECO;
      break;
    case LSM9DS1_6D_POS_RECO:
      *val = LSM9DS1_6D_POS_RECO;
      break;
    case LSM9DS1_4D_POS_RECO:
      *val = LSM9DS1_4D_POS_RECO;
      break;
    default:
      *val = LSM9DS1_POS_MOVE_RECO_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  6D functionality axis interrupt threshold.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data to be write.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_6d_threshold_set(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_write_reg(ctx, LSM9DS1_INT_GEN_THS_X_XL, buff, 3);
  return ret;
}

/**
  * @brief  6D functionality axis interrupt threshold.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_6d_threshold_get(lsm9ds1_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_THS_X_XL, buff, 3);
  return ret;
}

/**
  * @brief  .[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    .(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_6d_src_get(lsm9ds1_ctx_t *ctx, lsm9ds1_6d_src_t *val)
{
  lsm9ds1_int_gen_src_xl_t int_gen_src_xl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_INT_GEN_SRC_XL,
                         (uint8_t*)&int_gen_src_xl, 1);
  val->xl_xl = int_gen_src_xl.xl_xl;
  val->xh_xl = int_gen_src_xl.xh_xl;
  val->yl_xl = int_gen_src_xl.yl_xl;
  val->yh_xl = int_gen_src_xl.yh_xl;
  val->zl_xl = int_gen_src_xl.zl_xl;
  val->zh_xl = int_gen_src_xl.zh_xl;
  val->ia_xl = int_gen_src_xl.ia_xl;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup     LSM9DS1_Fifo
  * @brief        This section group all the functions concerning the
  *               fifo usage
  * @{
  *
  */

/**
  * @brief  Sensing chain FIFO stop values memorization at threshold
  *         level.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of stop_on_fth in reg CTRL_REG9.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_stop_on_wtm_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  if(ret == 0){
    ctrl_reg9.stop_on_fth = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  }
  return ret;
}

/**
  * @brief  Sensing chain FIFO stop values memorization at
  *         threshold level.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of stop_on_fth in reg CTRL_REG9.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_stop_on_wtm_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  *val = (uint8_t)ctrl_reg9.stop_on_fth;

  return ret;
}

/**
  * @brief  FIFO mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of "fifo_en" in reg LSM9DS1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_mode_set(lsm9ds1_ctx_t *ctx, lsm9ds1_fifo_md_t val)
{
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  lsm9ds1_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  if(ret == 0){
    ctrl_reg9.fifo_en = ( ( (uint8_t)val & 0x10U ) >> 4);
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  }
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }
  if(ret == 0){
    fifo_ctrl.fmode = ( (uint8_t)val & 0x0FU );
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }
  return ret;
}

/**
  * @brief  FIFO mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fifo_en in reg CTRL_REG9.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_mode_get(lsm9ds1_ctx_t *ctx, lsm9ds1_fifo_md_t *val)
{
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  lsm9ds1_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  if(ret == 0){
    ret = lsm9ds1_read_reg(ctx, LSM9DS1_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }
  switch ((ctrl_reg9.fifo_en << 4) | ctrl_reg9.fifo_en){
    case LSM9DS1_FIFO_OFF:
      *val = LSM9DS1_FIFO_OFF;
      break;
    case LSM9DS1_BYPASS_MODE:
      *val = LSM9DS1_BYPASS_MODE;
      break;
    case LSM9DS1_FIFO_MODE:
      *val = LSM9DS1_FIFO_MODE;
      break;
    case LSM9DS1_STREAM_TO_FIFO_MODE:
      *val = LSM9DS1_STREAM_TO_FIFO_MODE;
      break;
    case LSM9DS1_BYPASS_TO_STREAM_MODE:
      *val = LSM9DS1_BYPASS_TO_STREAM_MODE;
      break;
    case LSM9DS1_STREAM_MODE:
      *val = LSM9DS1_STREAM_MODE;
      break;
    default:
      *val = LSM9DS1_FIFO_OFF;
      break;
  }

  return ret;
}

/**
  * @brief  Batching of temperature data.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fifo_temp_en in reg CTRL_REG9.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_temp_batch_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  if(ret == 0){
    ctrl_reg9.fifo_temp_en = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  }
  return ret;
}

/**
  * @brief  Batching of temperature data.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fifo_temp_en in reg CTRL_REG9.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_temp_batch_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_ctrl_reg9_t ctrl_reg9;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG9, (uint8_t*)&ctrl_reg9, 1);
  *val = (uint8_t)ctrl_reg9.fifo_temp_en;

  return ret;
}

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fth in reg FIFO_CTRL.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_watermark_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  if(ret == 0){
    fifo_ctrl.fth = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }
  return ret;
}

/**
  * @brief  FIFO watermark level selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fth in reg FIFO_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_watermark_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  *val = (uint8_t)fifo_ctrl.fth;

  return ret;
}

/**
  * @brief  FIFOfullstatus.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "fss" in reg FIFO_SRC.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_full_flag_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_fifo_src_t fifo_src;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_FIFO_SRC, (uint8_t*)&fifo_src, 1);
  *val = fifo_src.fss;

  return ret;
}

/**
  * @brief  Number of unread words (16-bit axes) stored in FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "fss" in reg FIFO_SRC.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_data_level_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_fifo_src_t fifo_src;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_FIFO_SRC, (uint8_t*)&fifo_src, 1);
  *val = fifo_src.fss;

  return ret;
}

/**
  * @brief  FIFO overrun status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "ovrn" in reg FIFO_SRC.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_ovr_flag_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_fifo_src_t fifo_src;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_FIFO_SRC, (uint8_t*)&fifo_src, 1);
  *val = fifo_src.ovrn;

  return ret;
}

/**
  * @brief  FIFO watermark status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Iet the values of "fth" in reg FIFO_SRC.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_fifo_wtm_flag_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_fifo_src_t fifo_src;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_FIFO_SRC, (uint8_t*)&fifo_src, 1);
  *val = fifo_src.fth;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup     LSM9DS1_Self_test
  * @brief        This section groups all the functions that manage self
  *               test configuration
  * @{
  *
  */

/**
  * @brief  Enable/disable self-test mode for accelerometer.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of st_xl in reg CTRL_REG10.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_self_test_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_ctrl_reg10_t ctrl_reg10;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG10, (uint8_t*)&ctrl_reg10, 1);
  if(ret == 0){
    ctrl_reg10.st_xl = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG10, (uint8_t*)&ctrl_reg10, 1);
  }
  return ret;
}

/**
  * @brief  Enable/disable self-test mode for accelerometer.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st_xl in reg CTRL_REG10.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_xl_self_test_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_ctrl_reg10_t ctrl_reg10;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG10, (uint8_t*)&ctrl_reg10, 1);
  *val = (uint8_t)ctrl_reg10.st_xl;

  return ret;
}

/**
  * @brief  Enable/disable self-test mode for gyroscope.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of st_g in reg CTRL_REG10.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_self_test_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_ctrl_reg10_t ctrl_reg10;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG10, (uint8_t*)&ctrl_reg10, 1);
  if(ret == 0){
    ctrl_reg10.st_g = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG10, (uint8_t*)&ctrl_reg10, 1);
  }
  return ret;
}

/**
  * @brief  Enable/disable self-test mode for gyroscope.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st_g in reg CTRL_REG10.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_gy_self_test_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_ctrl_reg10_t ctrl_reg10;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG10, (uint8_t*)&ctrl_reg10, 1);
  *val = (uint8_t)ctrl_reg10.st_g;

  return ret;
}

/**
  * @brief  Enable/disable self-test mode for magnatic sensor.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of st in reg CTRL_REG1_M.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_self_test_set(lsm9ds1_ctx_t *ctx, uint8_t val)
{
  lsm9ds1_ctrl_reg1_m_t ctrl_reg1_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_M, (uint8_t*)&ctrl_reg1_m, 1);
  if(ret == 0){
    ctrl_reg1_m.st = (uint8_t)val;
    ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG1_M,
                            (uint8_t*)&ctrl_reg1_m, 1);
  }
  return ret;
}

/**
  * @brief  Enable/disable self-test mode for magnatic sensor.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st in reg CTRL_REG1_M.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lsm9ds1_mag_self_test_get(lsm9ds1_ctx_t *ctx, uint8_t *val)
{
  lsm9ds1_ctrl_reg1_m_t ctrl_reg1_m;
  int32_t ret;

  ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_M, (uint8_t*)&ctrl_reg1_m, 1);
  *val = (uint8_t)ctrl_reg1_m.st;

  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
