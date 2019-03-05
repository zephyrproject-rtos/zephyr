/*
 ******************************************************************************
 * @file    lsm6ds3_reg.c
 * @author  Sensors Software Solution Team
 * @brief   LSM6DS3 driver file
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
#include "lsm6ds3_reg.h"

/**
  * @defgroup    LSM6DS3
  * @brief       This file provides a set of functions needed to drive the
  *              lsm6ds3 enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup    LSM6DS3_Interfaces_Functions
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
int32_t lsm6ds3_read_reg(lsm6ds3_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lsm6ds3_write_reg(lsm6ds3_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LSM6DS3_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t lsm6ds3_from_fs2g_to_mg(int16_t lsb)
{
  return ((float_t)lsb * 61.0f / 1000.0f);
}

float_t lsm6ds3_from_fs4g_to_mg(int16_t lsb)
{
  return ((float_t)lsb * 122.0f / 1000.0f);
}

float_t lsm6ds3_from_fs8g_to_mg(int16_t lsb)
{
  return ((float_t)lsb * 244.0f / 1000.0f);
}

float_t lsm6ds3_from_fs16g_to_mg(int16_t lsb)
{
  return ((float_t)lsb * 488.0f / 1000.0f);
}

float_t lsm6ds3_from_fs125dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb   * 4375.0f / 1000.0f);
}

float_t lsm6ds3_from_fs250dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb  * 8750.0f / 1000.0f);
}

float_t lsm6ds3_from_fs500dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 1750.0f / 100.0f);
}

float_t lsm6ds3_from_fs1000dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 35.0f);
}

float_t lsm6ds3_from_fs2000dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 70.0f);
}

float_t lsm6ds3_from_lsb_to_celsius(int16_t lsb)
{
  return ((float_t)lsb / 16.0f + 25.0f );
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Data_generation
  * @brief       This section groups all the functions concerning
  *              data generation
  * @{
  *
  */

/**
  * @brief   Gyroscope directional user-orientation selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of orient in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_data_orient_set(lsm6ds3_ctx_t *ctx, lsm6ds3_gy_orient_t val)
{
  lsm6ds3_orient_cfg_g_t orient_cfg_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_ORIENT_CFG_G,
                         (uint8_t*)&orient_cfg_g, 1);
  if(ret == 0){
    orient_cfg_g.orient = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_ORIENT_CFG_G,
                            (uint8_t*)&orient_cfg_g, 1);
  }
  return ret;
}

/**
  * @brief   Gyroscope directional user-orientation selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of orient in reg ORIENT_CFG_G
  *
  */
int32_t lsm6ds3_gy_data_orient_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_gy_orient_t *val)
{
  lsm6ds3_orient_cfg_g_t orient_cfg_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_ORIENT_CFG_G,
                         (uint8_t*)&orient_cfg_g, 1);

  switch (orient_cfg_g.orient)
  {
    case LSM6DS3_GY_ORIENT_XYZ:
      *val = LSM6DS3_GY_ORIENT_XYZ;
      break;
    case LSM6DS3_GY_ORIENT_XZY:
      *val = LSM6DS3_GY_ORIENT_XZY;
      break;
    case LSM6DS3_GY_ORIENT_YXZ:
      *val = LSM6DS3_GY_ORIENT_YXZ;
      break;
    case LSM6DS3_GY_ORIENT_YZX:
      *val = LSM6DS3_GY_ORIENT_YZX;
      break;
    case LSM6DS3_GY_ORIENT_ZXY:
      *val = LSM6DS3_GY_ORIENT_ZXY;
      break;
    case LSM6DS3_GY_ORIENT_ZYX:
      *val = LSM6DS3_GY_ORIENT_ZYX;
      break;
    default:
      *val = LSM6DS3_GY_ORIENT_XYZ;
      break;
  }
  return ret;
}

/**
  * @brief  angular rate sign.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sign_g in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_data_sign_set(lsm6ds3_ctx_t *ctx, lsm6ds3_gy_sgn_t val)
{
  lsm6ds3_orient_cfg_g_t orient_cfg_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_ORIENT_CFG_G,
                         (uint8_t*)&orient_cfg_g, 1);
  if(ret == 0){
    orient_cfg_g.sign_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_ORIENT_CFG_G,
                            (uint8_t*)&orient_cfg_g, 1);
  }
  return ret;
}

/**
  * @brief  angularratesign.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of sign_g in reg ORIENT_CFG_G
  *
  */
int32_t lsm6ds3_gy_data_sign_get(lsm6ds3_ctx_t *ctx, lsm6ds3_gy_sgn_t *val)
{
  lsm6ds3_orient_cfg_g_t orient_cfg_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_ORIENT_CFG_G,
                         (uint8_t*)&orient_cfg_g, 1);

  switch (orient_cfg_g.sign_g)
  {
    case LSM6DS3_GY_SIGN_PPP:
      *val =LSM6DS3_GY_SIGN_PPP;
      break;
    case LSM6DS3_GY_SIGN_PPN:
      *val = LSM6DS3_GY_SIGN_PPN;
      break;
    case LSM6DS3_GY_SIGN_PNP:
      *val = LSM6DS3_GY_SIGN_PNP;
      break;
    case LSM6DS3_GY_SIGN_NPP:
      *val = LSM6DS3_GY_SIGN_NPP;
      break;
    case LSM6DS3_GY_SIGN_NNP:
      *val = LSM6DS3_GY_SIGN_NNP;
      break;
    case LSM6DS3_GY_SIGN_NPN:
      *val = LSM6DS3_GY_SIGN_NPN;
      break;
    case LSM6DS3_GY_SIGN_PNN:
      *val = LSM6DS3_GY_SIGN_PNN;
      break;
    case LSM6DS3_GY_SIGN_NNN:
      *val = LSM6DS3_GY_SIGN_NNN;
      break;
    default:
      *val = LSM6DS3_GY_SIGN_PPP;
      break;
  }
  return ret;
}

/**
  * @brief   Accelerometer full-scale selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of fs_xl in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_xl_fs_t val)
{
  lsm6ds3_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  if(ret == 0){
    ctrl1_xl.fs_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer full-scale selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of fs_xl in reg CTRL1_XL
  *
  */
int32_t lsm6ds3_xl_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_xl_fs_t *val)
{
  lsm6ds3_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);

  switch (ctrl1_xl.fs_xl)
  {
    case LSM6DS3_2g:
      *val = LSM6DS3_2g;
      break;
    case LSM6DS3_16g:
      *val = LSM6DS3_16g;
      break;
    case LSM6DS3_4g:
      *val = LSM6DS3_4g;
      break;
    case LSM6DS3_8g:
      *val = LSM6DS3_8g;
      break;
    default:
      *val = LSM6DS3_2g;
      break;
  }
  return ret;
}

/**
  * @brief   Accelerometer data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of odr_xl in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_xl_t val)
{
  lsm6ds3_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  if(ret == 0){
    ctrl1_xl.odr_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer data rate selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of odr_xl in reg CTRL1_XL
  *
  */
int32_t lsm6ds3_xl_data_rate_get(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_xl_t *val)
{
  lsm6ds3_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);

  switch (ctrl1_xl.odr_xl)
  {
    case LSM6DS3_XL_ODR_OFF:
      *val = LSM6DS3_XL_ODR_OFF;
      break;
    case LSM6DS3_XL_ODR_12Hz5:
      *val = LSM6DS3_XL_ODR_12Hz5;
      break;
    case LSM6DS3_XL_ODR_26Hz:
      *val = LSM6DS3_XL_ODR_26Hz;
      break;
    case LSM6DS3_XL_ODR_52Hz:
      *val = LSM6DS3_XL_ODR_52Hz;
      break;
    case LSM6DS3_XL_ODR_104Hz:
      *val = LSM6DS3_XL_ODR_104Hz;
      break;
    case LSM6DS3_XL_ODR_208Hz:
      *val = LSM6DS3_XL_ODR_208Hz;
      break;
    case LSM6DS3_XL_ODR_416Hz:
      *val = LSM6DS3_XL_ODR_416Hz;
      break;
    case LSM6DS3_XL_ODR_833Hz:
      *val = LSM6DS3_XL_ODR_833Hz;
      break;
    case LSM6DS3_XL_ODR_1k66Hz:
      *val = LSM6DS3_XL_ODR_1k66Hz;
      break;
    case LSM6DS3_XL_ODR_3k33Hz:
      *val = LSM6DS3_XL_ODR_3k33Hz;
      break;
    case LSM6DS3_XL_ODR_6k66Hz:
      *val = LSM6DS3_XL_ODR_6k66Hz;
      break;
    default:
      *val = LSM6DS3_XL_ODR_OFF;
      break;
  }
  return ret;
}

/**
  * @brief   Gyroscope UI chain full-scale selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of fs_g in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_g_t val)
{
  lsm6ds3_ctrl2_g_t ctrl2_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  if(ret == 0){
    ctrl2_g.fs_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  }
  return ret;
}

/**
  * @brief   Gyroscope UI chain full-scale selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of fs_g in reg CTRL2_G
  *
  */
int32_t lsm6ds3_gy_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_fs_g_t *val)
{
  lsm6ds3_ctrl2_g_t ctrl2_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL2_G, (uint8_t*)&ctrl2_g, 1);

  switch (ctrl2_g.fs_g)
  {
    case LSM6DS3_250dps:
      *val = LSM6DS3_250dps;
      break;
    case LSM6DS3_125dps:
      *val = LSM6DS3_125dps;
      break;
    case LSM6DS3_500dps:
      *val = LSM6DS3_500dps;
      break;
    case LSM6DS3_1000dps:
      *val = LSM6DS3_1000dps;
      break;
    case LSM6DS3_2000dps:
      *val = LSM6DS3_2000dps;
      break;
    default:
      *val = LSM6DS3_250dps;
      break;
  }
  return ret;
}

/**
  * @brief   Gyroscope UI data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of odr_g in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_g_t val)
{
  lsm6ds3_ctrl2_g_t ctrl2_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  if(ret == 0){
    ctrl2_g.odr_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  }
  return ret;
}

/**
  * @brief   Gyroscope UI data rate selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of odr_g in reg CTRL2_G
  *
  */
int32_t lsm6ds3_gy_data_rate_get(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_g_t *val)
{
  lsm6ds3_ctrl2_g_t ctrl2_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL2_G, (uint8_t*)&ctrl2_g, 1);

  switch (ctrl2_g.odr_g)
  {
    case LSM6DS3_GY_ODR_OFF:
      *val = LSM6DS3_GY_ODR_OFF;
      break;
    case LSM6DS3_GY_ODR_12Hz5:
      *val = LSM6DS3_GY_ODR_12Hz5;
      break;
    case LSM6DS3_GY_ODR_26Hz:
      *val = LSM6DS3_GY_ODR_26Hz;
      break;
    case LSM6DS3_GY_ODR_52Hz:
      *val = LSM6DS3_GY_ODR_52Hz;
      break;
    case LSM6DS3_GY_ODR_104Hz:
      *val = LSM6DS3_GY_ODR_104Hz;
      break;
    case LSM6DS3_GY_ODR_208Hz:
      *val = LSM6DS3_GY_ODR_208Hz;
      break;
    case LSM6DS3_GY_ODR_416Hz:
      *val = LSM6DS3_GY_ODR_416Hz;
      break;
    case LSM6DS3_GY_ODR_833Hz:
      *val = LSM6DS3_GY_ODR_833Hz;
      break;
    case LSM6DS3_GY_ODR_1k66Hz:
      *val = LSM6DS3_GY_ODR_1k66Hz;
      break;
    default:
      *val = LSM6DS3_GY_ODR_OFF;
      break;
  }
  return ret;
}

/**
  * @brief  Blockdataupdate.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of bdu in reg CTRL3_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_block_data_update_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.bdu = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief  Blockdataupdate.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of bdu in reg CTRL3_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_block_data_update_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  *val = (uint8_t)ctrl3_c.bdu;

  return ret;
}

/**
  * @brief   High-performance operating mode for accelerometer.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of xl_hm_mode in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_power_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_xl_hm_mode_t val)
{
  lsm6ds3_ctrl6_c_t ctrl6_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL6_C, (uint8_t*)&ctrl6_c, 1);
  if(ret == 0){
    ctrl6_c.xl_hm_mode = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL6_C, (uint8_t*)&ctrl6_c, 1);
  }
  return ret;
}

/**
  * @brief   High-performance operating mode for accelerometer.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of xl_hm_mode in reg CTRL6_C
  *
  */
int32_t lsm6ds3_xl_power_mode_get(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_xl_hm_mode_t *val)
{
  lsm6ds3_ctrl6_c_t ctrl6_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL6_C, (uint8_t*)&ctrl6_c, 1);

  switch (ctrl6_c.xl_hm_mode)
  {
    case  LSM6DS3_XL_HIGH_PERFORMANCE:
      *val =  LSM6DS3_XL_HIGH_PERFORMANCE;
      break;
    case LSM6DS3_XL_NORMAL:
      *val = LSM6DS3_XL_NORMAL;
      break;
    default:
      *val =  LSM6DS3_XL_HIGH_PERFORMANCE;
      break;
  }
  return ret;
}

/**
  * @brief   Source register rounding function on ADD HERE ROUNDING REGISTERS.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of rounding_status in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_rounding_on_status_set(lsm6ds3_ctx_t *ctx, lsm6ds3_rnd_stat_t val)
{
  lsm6ds3_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  if(ret == 0){
    ctrl7_g.rounding_status = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  }
  return ret;
}

/**
  * @brief   Source register rounding function on ADD HERE ROUNDING REGISTERS.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of rounding_status in reg CTRL7_G
  *
  */
int32_t lsm6ds3_rounding_on_status_get(lsm6ds3_ctx_t *ctx,
                                       lsm6ds3_rnd_stat_t *val)
{
  lsm6ds3_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);

  switch (ctrl7_g.rounding_status)
  {
    case LSM6DS3_STAT_RND_DISABLE:
      *val = LSM6DS3_STAT_RND_DISABLE;
      break;
    case LSM6DS3_STAT_RND_ENABLE:
      *val = LSM6DS3_STAT_RND_ENABLE;
      break;
    default:
      *val = LSM6DS3_STAT_RND_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   High-performance operating mode disable for gyroscope.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of g_hm_mode in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_power_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_g_hm_mode_t val)
{
  lsm6ds3_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  if(ret == 0){
    ctrl7_g.g_hm_mode = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  }
  return ret;
}

/**
  * @brief   High-performance operating mode disable for gyroscope.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of g_hm_mode in reg CTRL7_G
  *
  */
int32_t lsm6ds3_gy_power_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_g_hm_mode_t *val)
{
  lsm6ds3_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);

  switch (ctrl7_g.g_hm_mode)
  {
    case  LSM6DS3_GY_HIGH_PERFORMANCE:
      *val =  LSM6DS3_GY_HIGH_PERFORMANCE;
      break;
    case LSM6DS3_GY_NORMAL:
      *val = LSM6DS3_GY_NORMAL;
      break;
    default:
      *val =  LSM6DS3_GY_HIGH_PERFORMANCE;
      break;
  }
  return ret;
}

/**
  * @brief   Accelerometer X-axis output enable/disable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of xen_xl in reg CTRL9_XL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_axis_x_data_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if(ret == 0){
    ctrl9_xl.xen_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer X-axis output enable/disable.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of xen_xl in reg CTRL9_XL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_axis_x_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  *val = (uint8_t)ctrl9_xl.xen_xl;

  return ret;
}

/**
  * @brief   Accelerometer Y-axis output enable/disable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of yen_xl in reg CTRL9_XL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_axis_y_data_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if(ret == 0){
    ctrl9_xl.yen_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer Y-axis output enable/disable.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of yen_xl in reg CTRL9_XL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_axis_y_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  *val = (uint8_t)ctrl9_xl.yen_xl;

  return ret;
}

/**
  * @brief   Accelerometer Z-axis output enable/disable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of zen_xl in reg CTRL9_XL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_axis_z_data_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if(ret == 0){
    ctrl9_xl.zen_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer Z-axis output enable/disable.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of zen_xl in reg CTRL9_XL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_axis_z_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  *val = (uint8_t)ctrl9_xl.zen_xl;

  return ret;
}

/**
  * @brief   Gyroscope pitch axis (X) output enable/disable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of xen_g in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_axis_x_data_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  if(ret == 0){
    ctrl10_c.xen_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  }
  return ret;
}

/**
  * @brief   Gyroscope pitch axis (X) output enable/disable.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of xen_g in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_axis_x_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  *val = (uint8_t)ctrl10_c.xen_g;

  return ret;
}

/**
  * @brief   Gyroscope pitch axis (Y) output enable/disable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of yen_g in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_axis_y_data_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  if(ret == 0){
    ctrl10_c.yen_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  }
  return ret;
}

/**
  * @brief   Gyroscope pitch axis (Y) output enable/disable.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of yen_g in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_axis_y_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  *val = (uint8_t)ctrl10_c.yen_g;

  return ret;
}

/**
  * @brief   Gyroscope pitch axis (Z) output enable/disable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of zen_g in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_axis_z_data_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  if(ret == 0){
    ctrl10_c.zen_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  }
  return ret;
}

/**
  * @brief   Gyroscope pitch axis (Z) output enable/disable.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of zen_g in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_axis_z_data_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  *val = (uint8_t)ctrl10_c.zen_g;

  return ret;
}

/**
  * @brief   Read all the interrupt/status flag of the device. ELENCA I REGISTRI[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Read all the interrupt flag of the device:
  *                     WAKE_UP_SRC, TAP_SRC, D6D_SRC, FUNC_SRC.
  *
  */
int32_t lsm6ds3_all_sources_get(lsm6ds3_ctx_t *ctx, lsm6ds3_all_src_t *val)
{
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_SRC,
                         (uint8_t*)&(val->wake_up_src), 1);
  if(ret == 0) {
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_SRC,
                           (uint8_t*)&(val->tap_src), 1);
  }
  if(ret == 0) {
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_D6D_SRC,
                           (uint8_t*)&(val->d6d_src), 1);
  }
  if(ret == 0) {
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_FUNC_SRC,
                           (uint8_t*)&(val->func_src), 1);
  }
  return ret;
}

/**
  * @brief   The STATUS_REG register of the device.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val          The STATUS_REG register of the device.
  *
  */
int32_t lsm6ds3_status_reg_get(lsm6ds3_ctx_t *ctx, lsm6ds3_status_reg_t *val)
{
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_STATUS_REG, (uint8_t*)&val, 1);

  return ret;
}

/**
  * @brief   Accelerometer new data available.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of xlda in reg STATUS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_flag_data_ready_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_status_reg_t status_reg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_STATUS_REG, (uint8_t*)&status_reg, 1);
  *val = (uint8_t)status_reg.xlda;

  return ret;
}

/**
  * @brief   Gyroscope new data available.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of gda in reg STATUS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_flag_data_ready_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_status_reg_t status_reg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_STATUS_REG, (uint8_t*)&status_reg, 1);
  *val = (uint8_t)status_reg.gda;

  return ret;
}

/**
  * @brief   Temperature new data available.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of tda in reg STATUS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_temp_flag_data_ready_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_status_reg_t status_reg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_STATUS_REG, (uint8_t*)&status_reg, 1);
  *val = (uint8_t)status_reg.tda;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Timestamp
  * @brief       This section groups all the functions that manage the
  *              timestamp generation.
  * @{
  *
  */

/**
  * @brief   Timestamp first byte data output register (r). The value is
  *          expressed as a 24-bit word and the bit resolution is defined
  *          by setting the value in WAKE_UP_DUR (5Ch).[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_timestamp_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TIMESTAMP0_REG, buff,
                            3);
  return ret;
}

/**
  * @brief  Reset the timer.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data to be write
  *
  */
int32_t lsm6ds3_timestamp_rst_set(lsm6ds3_ctx_t *ctx)
{
  int32_t ret;
  uint8_t rst_val = 0xAA;

  ret = lsm6ds3_write_reg(ctx, LSM6DS3_TIMESTAMP2_REG, &rst_val, 1);
  return ret;
}

/**
  * @brief   Timestamp count enable, output data are collected in
  *          TIMESTAMP0_REG (40h), TIMESTAMP1_REG (41h),
  *          TIMESTAMP2_REG (42h) register.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of timer_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_timestamp_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  if(ret == 0){
    tap_cfg.timer_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Timestamp count enable, output data are collected in
  *          TIMESTAMP0_REG (40h), TIMESTAMP1_REG (41h),
  *          TIMESTAMP2_REG (42h) register.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of timer_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_timestamp_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  *val = (uint8_t)tap_cfg.timer_en;

  return ret;
}

/**
  * @brief   Timestamp register resolution setting.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of timer_hr in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_timestamp_res_set(lsm6ds3_ctx_t *ctx, lsm6ds3_ts_res_t val)
{
  lsm6ds3_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_DUR, (uint8_t*)&wake_up_dur, 1);
  if(ret == 0){
    wake_up_dur.timer_hr = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_WAKE_UP_DUR,
                            (uint8_t*)&wake_up_dur, 1);
  }
  return ret;
}

/**
  * @brief   Timestamp register resolution setting.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of timer_hr in reg WAKE_UP_DUR
  *
  */
int32_t lsm6ds3_timestamp_res_get(lsm6ds3_ctx_t *ctx, lsm6ds3_ts_res_t *val)
{
  lsm6ds3_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_DUR, (uint8_t*)&wake_up_dur, 1);

  switch (wake_up_dur.timer_hr)
  {
    case LSM6DS3_LSB_6ms4:
      *val = LSM6DS3_LSB_6ms4;
      break;
    case LSM6DS3_LSB_25us:
      *val = LSM6DS3_LSB_25us;
      break;
    default:
      *val = LSM6DS3_LSB_6ms4;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Dataoutput
  * @brief       This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief   Circular burst-mode (rounding) read from output registers
  *          through the primary interface.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of rounding in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_rounding_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_rounding_t val)
{
  lsm6ds3_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  if(ret == 0){
    ctrl5_c.rounding = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  }
  return ret;
}

/**
  * @brief   Circular burst-mode (rounding) read from output registers
  *          through the primary interface.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of rounding in reg CTRL5_C
  *
  */
int32_t lsm6ds3_rounding_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_rounding_t *val)
{
  lsm6ds3_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);

  switch (ctrl5_c.rounding)
  {
    case LSM6DS3_ROUND_DISABLE:
      *val = LSM6DS3_ROUND_DISABLE;
      break;
    case LSM6DS3_ROUND_XL:
      *val = LSM6DS3_ROUND_XL;
      break;
    case LSM6DS3_ROUND_GY:
      *val = LSM6DS3_ROUND_GY;
      break;
    case LSM6DS3_ROUND_GY_XL:
      *val = LSM6DS3_ROUND_GY_XL;
      break;
    case LSM6DS3_ROUND_SH1_TO_SH6:
      *val = LSM6DS3_ROUND_SH1_TO_SH6;
      break;
    case  LSM6DS3_ROUND_XL_SH1_TO_SH6:
      *val =  LSM6DS3_ROUND_XL_SH1_TO_SH6;
      break;
    case  LSM6DS3_ROUND_GY_XL_SH1_TO_SH12:
      *val =  LSM6DS3_ROUND_GY_XL_SH1_TO_SH12;
      break;
    case  LSM6DS3_ROUND_GY_XL_SH1_TO_SH6:
      *val =  LSM6DS3_ROUND_GY_XL_SH1_TO_SH6;
      break;
    default:
      *val = LSM6DS3_ROUND_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   Temperature data output register (r). L and H registers together
  *          express a 16-bit word in two’s complement.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_temperature_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_OUT_TEMP_L, buff, 2);
  return ret;
}

/**
  * @brief   Angular rate sensor. The value is expressed as a 16-bit word
  *          in two’s complement..[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_angular_rate_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_OUTX_L_G, buff, 6);
  return ret;
}

/**
  * @brief   Linear acceleration output register. The value is expressed
  *          as a 16-bit word in two’s complement.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_acceleration_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_OUTX_L_XL, buff, 6);
  return ret;
}

/**
  * @brief   fifo_raw_data: [get]  read data in FIFO.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t *: data buffer to store FIFO data.
  * @param  uint8_t : number of data to read from FIFO.
  *
  */
int32_t lsm6ds3_fifo_raw_data_get(lsm6ds3_ctx_t *ctx, uint8_t *buffer, uint8_t len)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_DATA_OUT_L, buffer, len);
  return ret;
}

/**
  * @brief   Step counter output register..[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_number_of_steps_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_STEP_COUNTER_L, buff, 2);
  return ret;
}

/**
  * @brief   magnetometer raw data.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_mag_calibrated_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_OUT_MAG_RAW_X_L, buff, 6);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Common
  * @brief       This section groups common usefull functions.
  * @{
  *
  */

/**
  * @brief   Enable access to the embedded functions.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of func_cfg_en in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_mem_bank_set(lsm6ds3_ctx_t *ctx, lsm6ds3_func_cfg_en_t val)
{
  lsm6ds3_func_cfg_access_t func_cfg_access;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FUNC_CFG_ACCESS,
                         (uint8_t*)&func_cfg_access, 1);
  if(ret == 0){
    func_cfg_access.func_cfg_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FUNC_CFG_ACCESS,
                            (uint8_t*)&func_cfg_access, 1);
  }
  return ret;
}

/**
  * @brief   Enable access to the embedded functions.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of func_cfg_en in reg FUNC_CFG_ACCESS
  *
  */
int32_t lsm6ds3_mem_bank_get(lsm6ds3_ctx_t *ctx, lsm6ds3_func_cfg_en_t *val)
{
  lsm6ds3_func_cfg_access_t func_cfg_access;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FUNC_CFG_ACCESS,
                            (uint8_t*)&func_cfg_access, 1);

  switch (func_cfg_access.func_cfg_en)
  {
    case LSM6DS3_USER_BANK:
      *val = LSM6DS3_USER_BANK;
      break;
    case LSM6DS3_EMBEDDED_FUNC_BANK:
      *val = LSM6DS3_EMBEDDED_FUNC_BANK;
      break;
    default:
      *val = LSM6DS3_USER_BANK;
      break;
  }
  return ret;
}

/**
  * @brief  DeviceWhoamI..[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_device_id_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief   Software reset. Restore the default values in user registers.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sw_reset in reg CTRL3_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_reset_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.sw_reset = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief   Software reset. Restore the default values in user registers.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sw_reset in reg CTRL3_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_reset_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  *val = (uint8_t)ctrl3_c.sw_reset;

  return ret;
}

/**
  * @brief   Big/Little Endian Data selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of ble in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_data_format_set(lsm6ds3_ctx_t *ctx, lsm6ds3_ble_t val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.ble = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief   Big/Little Endian Data selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of ble in reg CTRL3_C
  *
  */
int32_t lsm6ds3_data_format_get(lsm6ds3_ctx_t *ctx, lsm6ds3_ble_t *val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);

  switch (ctrl3_c.ble)
  {
    case LSM6DS3_LSB_AT_LOW_ADD:
      *val = LSM6DS3_LSB_AT_LOW_ADD;
      break;
    case LSM6DS3_MSB_AT_LOW_ADD:
      *val = LSM6DS3_MSB_AT_LOW_ADD;
      break;
    default:
      *val = LSM6DS3_LSB_AT_LOW_ADD;
      break;
  }
  return ret;
}

/**
  * @brief   Register address automatically incremented during a multiple
  *          byte access with a serial interface.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of if_inc in reg CTRL3_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_auto_increment_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.if_inc = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief   Register address automatically incremented during a multiple
  *          byte access with a serial interface.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of if_inc in reg CTRL3_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_auto_increment_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  *val = (uint8_t)ctrl3_c.if_inc;

  return ret;
}

/**
  * @brief   Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of boot in reg CTRL3_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_boot_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.boot = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief   Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of boot in reg CTRL3_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_boot_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  *val = (uint8_t)ctrl3_c.boot;

  return ret;
}

/**
  * @brief   Linear acceleration sensor self-test enable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of st_xl in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_self_test_set(lsm6ds3_ctx_t *ctx, lsm6ds3_st_xl_t val)
{
  lsm6ds3_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  if(ret == 0){
    ctrl5_c.st_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  }
  return ret;
}

/**
  * @brief   Linear acceleration sensor self-test enable.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of st_xl in reg CTRL5_C
  *
  */
int32_t lsm6ds3_xl_self_test_get(lsm6ds3_ctx_t *ctx, lsm6ds3_st_xl_t *val)
{
  lsm6ds3_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);

  switch (ctrl5_c.st_xl)
  {
    case LSM6DS3_XL_ST_DISABLE:
      *val = LSM6DS3_XL_ST_DISABLE;
      break;
    case LSM6DS3_XL_ST_POSITIVE:
      *val = LSM6DS3_XL_ST_POSITIVE;
      break;
    case LSM6DS3_XL_ST_NEGATIVE:
      *val = LSM6DS3_XL_ST_NEGATIVE;
      break;
    default:
      *val = LSM6DS3_XL_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   Angular rate sensor self-test enable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of st_g in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_self_test_set(lsm6ds3_ctx_t *ctx, lsm6ds3_st_g_t val)
{
  lsm6ds3_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  if(ret == 0){
    ctrl5_c.st_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  }
  return ret;
}

/**
  * @brief   Angular rate sensor self-test enable.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of st_g in reg CTRL5_C
  *
  */
int32_t lsm6ds3_gy_self_test_get(lsm6ds3_ctx_t *ctx, lsm6ds3_st_g_t *val)
{
  lsm6ds3_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL5_C, (uint8_t*)&ctrl5_c, 1);

  switch (ctrl5_c.st_g)
  {
    case LSM6DS3_GY_ST_DISABLE:
      *val = LSM6DS3_GY_ST_DISABLE;
      break;
    case LSM6DS3_GY_ST_POSITIVE:
      *val = LSM6DS3_GY_ST_POSITIVE;
      break;
    case LSM6DS3_GY_ST_NEGATIVE:
      *val = LSM6DS3_GY_ST_NEGATIVE;
      break;
    default:
      *val = LSM6DS3_GY_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Filters
  * @brief       This section group all the functions concerning the
  *              filters configuration
  * @{
  *
  */

/**
  * @brief   Mask DRDY on pin (both XL & Gyro) until filter settling ends
  *          (XL and Gyro independently masked).[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of drdy_mask in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_filter_settling_mask_set(lsm6ds3_ctx_t *ctx,
                                         uint8_t val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.drdy_mask = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief   Mask DRDY on pin (both XL & Gyro) until filter settling ends
  *          (XL and Gyro independently masked).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of drdy_mask in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_filter_settling_mask_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = (uint8_t)ctrl4_c.drdy_mask;

  return ret;
}

/**
  * @brief   Gyroscope high-pass filter cutoff frequency selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of hpcf_g in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_hp_bandwidth_set(lsm6ds3_ctx_t *ctx, lsm6ds3_hpcf_g_t val)
{
  lsm6ds3_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  if(ret == 0){
    ctrl7_g.hpcf_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  }
  return ret;
}

/**
  * @brief   Gyroscope high-pass filter cutoff frequency selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of hpcf_g in reg CTRL7_G
  *
  */
int32_t lsm6ds3_gy_hp_bandwidth_get(lsm6ds3_ctx_t *ctx, lsm6ds3_hpcf_g_t *val)
{
  lsm6ds3_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);

  switch (ctrl7_g.hpcf_g)
  {
    case LSM6DS3_HP_CUT_OFF_8mHz1:
      *val = LSM6DS3_HP_CUT_OFF_8mHz1;
      break;
    case LSM6DS3_HP_CUT_OFF_32mHz4:
      *val = LSM6DS3_HP_CUT_OFF_32mHz4;
      break;
    case LSM6DS3_HP_CUT_OFF_2Hz07:
      *val = LSM6DS3_HP_CUT_OFF_2Hz07;
      break;
    case LSM6DS3_HP_CUT_OFF_16Hz32:
      *val = LSM6DS3_HP_CUT_OFF_16Hz32;
      break;
    default:
      *val = LSM6DS3_HP_CUT_OFF_8mHz1;
      break;
  }
  return ret;
}

/**
  * @brief   Gyro digital HP filter reset.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of hp_g_rst in reg CTRL7_G
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_hp_reset_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  if(ret == 0){
    ctrl7_g.hp_g_rst = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  }
  return ret;
}

/**
  * @brief   Gyro digital HP filter reset.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of hp_g_rst in reg CTRL7_G
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_hp_reset_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  *val = (uint8_t)ctrl7_g.hp_g_rst;

  return ret;
}

/**
  * @brief   Accelerometer slope filter and high-pass filter configuration
  *          and cut-off setting.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of hp_slope_xl_en in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_hp_bandwidth_set(lsm6ds3_ctx_t *ctx, lsm6ds3_hp_bw_t val)
{
  lsm6ds3_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  if(ret == 0){
	ctrl8_xl.hp_slope_xl_en = PROPERTY_ENABLE;
    ctrl8_xl.hpcf_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  }
  return ret;
}

/**
  * @brief   Accelerometer slope filter and high-pass filter configuration
  *          and cut-off setting.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of hp_slope_xl_en in reg CTRL8_XL
  *
  */
int32_t lsm6ds3_xl_hp_bandwidth_get(lsm6ds3_ctx_t *ctx, lsm6ds3_hp_bw_t *val)
{
  lsm6ds3_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);

  switch (ctrl8_xl.hpcf_xl)
  {
    case LSM6DS3_XL_HP_ODR_DIV_4:
      *val = LSM6DS3_XL_HP_ODR_DIV_4;
      break;
    case LSM6DS3_XL_HP_ODR_DIV_100:
      *val = LSM6DS3_XL_HP_ODR_DIV_100;
      break;
    case LSM6DS3_XL_HP_ODR_DIV_9:
      *val = LSM6DS3_XL_HP_ODR_DIV_9;
      break;
    case LSM6DS3_XL_HP_ODR_DIV_400:
      *val = LSM6DS3_XL_HP_ODR_DIV_400;
      break;
    default:
      *val = LSM6DS3_XL_HP_ODR_DIV_4;
      break;
  }
  return ret;
}

/**
  * @brief    Accelerometer low-pass filter configuration and
  *           cut-off setting.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of lpf2_xl_en in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_lp2_bandwidth_set(lsm6ds3_ctx_t *ctx, lsm6ds3_lp_bw_t val)
{
  lsm6ds3_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  if(ret == 0){
    ctrl8_xl.lpf2_xl_en = PROPERTY_ENABLE;
    ctrl8_xl.hpcf_xl= (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  }
  return ret;
}

/**
  * @brief    Accelerometer low-pass filter configuration and cut-off
  *           setting.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of lpf2_xl_en in reg CTRL8_XL
  *
  */
int32_t lsm6ds3_xl_lp2_bandwidth_get(lsm6ds3_ctx_t *ctx, lsm6ds3_lp_bw_t *val)
{
  lsm6ds3_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);

  switch (ctrl8_xl.hpcf_xl)
  {
    case LSM6DS3_XL_LP_ODR_DIV_50:
      *val = LSM6DS3_XL_LP_ODR_DIV_50;
      break;
    case LSM6DS3_XL_LP_ODR_DIV_100:
      *val = LSM6DS3_XL_LP_ODR_DIV_100;
      break;
    case LSM6DS3_XL_LP_ODR_DIV_9:
      *val = LSM6DS3_XL_LP_ODR_DIV_9;
      break;
    case LSM6DS3_XL_LP_ODR_DIV_400:
      *val = LSM6DS3_XL_LP_ODR_DIV_400;
      break;
    default:
      *val = LSM6DS3_XL_LP_ODR_DIV_50;
      break;
  }
  return ret;
}

/**
  * @brief   Anti-aliasing filter bandwidth selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of bw_xl in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_xl_filter_analog_set(lsm6ds3_ctx_t *ctx, lsm6ds3_bw_xl_t val)
{
  lsm6ds3_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  if(ret == 0){
    ctrl1_xl.bw_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  }
  return ret;
}

/**
  * @brief   Anti-aliasing filter bandwidth selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of bw_xl in reg CTRL1_XL
  *
  */
int32_t lsm6ds3_xl_filter_analog_get(lsm6ds3_ctx_t *ctx, lsm6ds3_bw_xl_t *val)
{
  lsm6ds3_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);

  switch (ctrl1_xl.bw_xl)
  {
    case  LSM6DS3_ANTI_ALIASING_400Hz:
      *val =  LSM6DS3_ANTI_ALIASING_400Hz;
      break;
    case  LSM6DS3_ANTI_ALIASING_200Hz:
      *val =  LSM6DS3_ANTI_ALIASING_200Hz;
      break;
    case  LSM6DS3_ANTI_ALIASING_100Hz:
      *val =  LSM6DS3_ANTI_ALIASING_100Hz;
      break;
    case LSM6DS3_ANTI_ALIASING_50Hz:
      *val = LSM6DS3_ANTI_ALIASING_50Hz;
      break;
    default:
      *val =  LSM6DS3_ANTI_ALIASING_400Hz;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Serial_interface
  * @brief       This section groups all the functions concerning main
  *              serial interface management (not auxiliary)
  * @{
  *
  */

/**
  * @brief   SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sim in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_spi_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_sim_t val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.sim = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief   SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of sim in reg CTRL3_C
  *
  */
int32_t lsm6ds3_spi_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_sim_t *val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);

  switch (ctrl3_c.sim)
  {
    case LSM6DS3_SPI_4_WIRE:
      *val = LSM6DS3_SPI_4_WIRE;
      break;
    case LSM6DS3_SPI_3_WIRE:
      *val = LSM6DS3_SPI_3_WIRE;
      break;
    default:
      *val = LSM6DS3_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief   Disable / Enable I2C interface.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of i2c_disable in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_i2c_interface_set(lsm6ds3_ctx_t *ctx, lsm6ds3_i2c_dis_t val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.i2c_disable = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief   Disable / Enable I2C interface.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of i2c_disable in reg CTRL4_C
  *
  */
int32_t lsm6ds3_i2c_interface_get(lsm6ds3_ctx_t *ctx, lsm6ds3_i2c_dis_t *val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);

  switch (ctrl4_c.i2c_disable)
  {
    case LSM6DS3_I2C_ENABLE:
      *val = LSM6DS3_I2C_ENABLE;
      break;
    case LSM6DS3_I2C_DISABLE:
      *val = LSM6DS3_I2C_DISABLE;
      break;
    default:
      *val = LSM6DS3_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Interrupt_pins
  * @brief       This section groups all the functions that manage
  *              interrupt pins
  * @{
  *
  */

/**
  * @brief   Select the signal that need to route on int1 pad.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val          Select the signal that need to route on int1 pad.
  *
  */
int32_t lsm6ds3_pin_int1_route_set(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_int1_route_t *val)
{
  lsm6ds3_int1_ctrl_t int1_ctrl;
  lsm6ds3_md1_cfg_t md1_cfg;
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);

  if(ret == 0) {
    int1_ctrl.int1_drdy_xl        = val->int1_drdy_xl;
    int1_ctrl.int1_drdy_g         = val->int1_drdy_g;
    int1_ctrl.int1_boot           = val->int1_boot;
    int1_ctrl.int1_fth            = val->int1_fth;
    int1_ctrl.int1_fifo_ovr       = val->int1_fifo_ovr;
    int1_ctrl.int1_full_flag      = val->int1_full_flag;
    int1_ctrl.int1_sign_mot       = val->int1_sign_mot;
    int1_ctrl.int1_step_detector  = val->int1_step_detector;
    md1_cfg.int1_timer            = val->int1_timer;
    md1_cfg.int1_tilt             = val->int1_tilt;
    md1_cfg.int1_6d               = val->int1_6d;
    md1_cfg.int1_double_tap       = val->int1_double_tap;
    md1_cfg.int1_ff               = val->int1_ff;
    md1_cfg.int1_wu               = val->int1_wu;
    md1_cfg.int1_single_tap       = val->int1_single_tap;
    md1_cfg.int1_inact_state      = val->int1_inact_state;
    master_config.drdy_on_int1    = val->drdy_on_int1;

    ret = lsm6ds3_write_reg(ctx, LSM6DS3_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
    if(ret == 0) {
      ret = lsm6ds3_write_reg(ctx, LSM6DS3_MD1_CFG, (uint8_t*)&md1_cfg, 1);
    }
    if(ret == 0) {
      ret = lsm6ds3_write_reg(ctx, LSM6DS3_MASTER_CONFIG,
                              (uint8_t*)&master_config, 1);
    }
  }
  return ret;
}

/**
  * @brief   Select the signal that need to route on int1 pad.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val          Select the signal that need to route on int1 pad.
  *
  */
int32_t lsm6ds3_pin_int1_route_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_int1_route_t *val)
{
  lsm6ds3_int1_ctrl_t int1_ctrl;
  lsm6ds3_md1_cfg_t md1_cfg;
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  if(ret == 0) {
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_MD1_CFG, (uint8_t*)&md1_cfg, 1);
  }
  if(ret == 0) {
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);
  }
  if(ret == 0) {
    val->int1_drdy_xl        = int1_ctrl.int1_drdy_xl;
    val->int1_drdy_g         = int1_ctrl.int1_drdy_g;
    val->int1_boot           = int1_ctrl.int1_boot;
    val->int1_fth            = int1_ctrl.int1_fth;
    val->int1_fifo_ovr       = int1_ctrl.int1_fifo_ovr;
    val->int1_full_flag      = int1_ctrl.int1_full_flag;
    val->int1_sign_mot       = int1_ctrl.int1_sign_mot;
    val->int1_step_detector  = int1_ctrl.int1_step_detector;
    val->int1_timer          = md1_cfg.int1_timer;
    val->int1_tilt           = md1_cfg.int1_tilt;
    val->int1_6d             = md1_cfg.int1_6d;
    val->int1_double_tap     = md1_cfg.int1_double_tap;
    val->int1_ff             = md1_cfg.int1_ff;
    val->int1_wu             = md1_cfg.int1_wu;
    val->int1_single_tap     = md1_cfg.int1_single_tap;
    val->int1_inact_state    = md1_cfg.int1_inact_state;
    val->drdy_on_int1        = master_config.drdy_on_int1;
  }
  return ret;
}

/**
  * @brief   Select the signal that need to route on int1 pad.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Select the signal that need to route on int1 pad.
  *
  */
int32_t lsm6ds3_pin_int2_route_set(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_int2_route_t *val)
{
  lsm6ds3_int2_ctrl_t int2_ctrl;
  lsm6ds3_md2_cfg_t md2_cfg;
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);

  if(ret == 0) {
    int2_ctrl.int2_drdy_xl       = val->int2_drdy_xl;
    int2_ctrl.int2_drdy_g        = val->int2_drdy_g;
    int2_ctrl.int2_drdy_temp     = val->int2_drdy_temp;
    int2_ctrl.int2_fth           = val->int2_fth;
    int2_ctrl.int2_fifo_ovr      = val->int2_fifo_ovr;
    int2_ctrl.int2_full_flag     = val->int2_full_flag;
    int2_ctrl.int2_step_count_ov = val->int2_step_count_ov;
    int2_ctrl.int2_step_delta    = val->int2_step_delta;
    md2_cfg.int2_iron            = val->int2_iron;
    md2_cfg.int2_tilt            = val->int2_tilt;
    md2_cfg.int2_6d              = val->int2_6d;
    md2_cfg.int2_double_tap      = val->int2_double_tap;
    md2_cfg.int2_ff              = val->int2_ff;
    md2_cfg.int2_wu              = val->int2_wu;
    md2_cfg.int2_single_tap      = val->int2_single_tap;
    md2_cfg.int2_inact_state     = val->int2_inact_state;
    master_config.start_config   = val->start_config;

    ret = lsm6ds3_write_reg(ctx, LSM6DS3_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  }
  if(ret == 0) {
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_MD2_CFG, (uint8_t*)&md2_cfg, 1);
  }
  if(ret == 0) {
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_MASTER_CONFIG,
                              (uint8_t*)&master_config, 1);
  }
  return ret;
}

/**
  * @brief   Select the signal that need to route on int1 pad.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         Select the signal that need to route on int1 pad.
  *
  */
int32_t lsm6ds3_pin_int2_route_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_int2_route_t *val)
{
  lsm6ds3_int2_ctrl_t int2_ctrl;
  lsm6ds3_md2_cfg_t md2_cfg;
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  if(ret == 0) {
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_MD2_CFG, (uint8_t*)&md2_cfg, 1);
  }
  if(ret == 0) {
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                            (uint8_t*)&master_config, 1);

    val->int2_drdy_xl       = int2_ctrl.int2_drdy_xl;
    val->int2_drdy_g        = int2_ctrl.int2_drdy_g;
    val->int2_drdy_temp     = int2_ctrl.int2_drdy_temp;
    val->int2_fth           = int2_ctrl.int2_fth;
    val->int2_fifo_ovr      = int2_ctrl.int2_fifo_ovr;
    val->int2_full_flag     = int2_ctrl.int2_full_flag;
    val->int2_step_count_ov = int2_ctrl.int2_step_count_ov;
    val->int2_step_delta    = int2_ctrl.int2_step_delta;
    val->int2_iron          = md2_cfg.int2_iron;
    val->int2_tilt          = md2_cfg.int2_tilt;
    val->int2_6d            = md2_cfg.int2_6d;
    val->int2_double_tap    = md2_cfg.int2_double_tap;
    val->int2_ff            = md2_cfg.int2_ff;
    val->int2_wu            = md2_cfg.int2_wu;
    val->int2_single_tap    = md2_cfg.int2_single_tap;
    val->int2_inact_state   = md2_cfg.int2_inact_state;
    val->start_config       = master_config.start_config;
  }

  return ret;
}

/**
  * @brief   Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pp_od in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pin_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_pp_od_t val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.pp_od = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief   Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of pp_od in reg CTRL3_C
  *
  */
int32_t lsm6ds3_pin_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_pp_od_t *val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);

  switch (ctrl3_c.pp_od)
  {
    case LSM6DS3_PUSH_PULL:
      *val = LSM6DS3_PUSH_PULL;
      break;
    case LSM6DS3_OPEN_DRAIN:
      *val = LSM6DS3_OPEN_DRAIN;
      break;
    default:
      *val = LSM6DS3_PUSH_PULL;
      break;
  }
  return ret;
}

/**
  * @brief   Interrupt active-high/low.Interrupt active-high/low.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of h_lactive in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pin_polarity_set(lsm6ds3_ctx_t *ctx, lsm6ds3_pin_pol_t val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.h_lactive = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief   Interrupt active-high/low.Interrupt active-high/low.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of h_lactive in reg CTRL3_C
  *
  */
int32_t lsm6ds3_pin_polarity_get(lsm6ds3_ctx_t *ctx, lsm6ds3_pin_pol_t *val)
{
  lsm6ds3_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL3_C, (uint8_t*)&ctrl3_c, 1);

  switch (ctrl3_c.h_lactive)
  {
    case LSM6DS3_ACTIVE_HIGH:
      *val = LSM6DS3_ACTIVE_HIGH;
      break;
    case LSM6DS3_ACTIVE_LOW:
      *val = LSM6DS3_ACTIVE_LOW;
      break;
    default:
      *val = LSM6DS3_ACTIVE_HIGH;
      break;
  }
  return ret;
}

/**
  * @brief   All interrupt signals become available on INT1 pin.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of int2_on_int1 in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_all_on_int1_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.int2_on_int1 = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief   All interrupt signals become available on INT1 pin.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of int2_on_int1 in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_all_on_int1_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = (uint8_t)ctrl4_c.int2_on_int1;

  return ret;
}

/**
  * @brief   Latched/pulsed interrupt.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of lir in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_int_notification_set(lsm6ds3_ctx_t *ctx, lsm6ds3_lir_t val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  if(ret == 0){
    tap_cfg.lir = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Latched/pulsed interrupt.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of lir in reg TAP_CFG
  *
  */
int32_t lsm6ds3_int_notification_get(lsm6ds3_ctx_t *ctx, lsm6ds3_lir_t *val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);

  switch (tap_cfg.lir)
  {
    case LSM6DS3_INT_PULSED:
      *val = LSM6DS3_INT_PULSED;
      break;
    case LSM6DS3_INT_LATCHED:
      *val = LSM6DS3_INT_LATCHED;
      break;
    default:
      *val = LSM6DS3_INT_PULSED;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Wake_Up_event
  * @brief       This section groups all the functions that manage the
  *              Wake Up event generation.
  * @{
  *
  */

/**
  * @brief   Read the wake_up_src status flag of the device.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val          Read the wake_up_src status flag of the device.
  *
  */
int32_t lsm6ds3_wkup_src_get(lsm6ds3_ctx_t *ctx, lsm6ds3_wake_up_src_t *val)
{
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_SRC, (uint8_t*)val, 1);

  return ret;
}

/**
  * @brief   Threshold for wakeup (1 LSB = FS_XL / 64).[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of wk_ths in reg WAKE_UP_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_wkup_threshold_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_THS, (uint8_t*)&wake_up_ths, 1);
  if(ret == 0){
    wake_up_ths.wk_ths = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_WAKE_UP_THS,
                            (uint8_t*)&wake_up_ths, 1);
  }
  return ret;
}

/**
  * @brief   Threshold for wakeup (1 LSB = FS_XL / 64).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of wk_ths in reg WAKE_UP_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_wkup_threshold_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_THS, (uint8_t*)&wake_up_ths, 1);
  *val = (uint8_t)wake_up_ths.wk_ths;

  return ret;
}

/**
  * @brief   Wake up duration event.1LSb = 1 / ODR[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of wake_dur in reg WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_wkup_dur_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_DUR, (uint8_t*)&wake_up_dur, 1);
  if(ret == 0){
    wake_up_dur.wake_dur = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_WAKE_UP_DUR,
                            (uint8_t*)&wake_up_dur, 1);
  }
  return ret;
}

/**
  * @brief   Wake up duration event.1LSb = 1 / ODR[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of wake_dur in reg WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_wkup_dur_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_DUR, (uint8_t*)&wake_up_dur, 1);
  *val = (uint8_t)wake_up_dur.wake_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Activity/Inactivity_detection
  * @brief       This section groups all the functions concerning
  *              activity/inactivity detection.
  * @{
  *
  */

/**
  * @brief   Enables gyroscope Sleep mode.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sleep_g in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_sleep_mode_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.sleep_g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief   Enables gyroscope Sleep mode.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sleep_g in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_gy_sleep_mode_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = (uint8_t)ctrl4_c.sleep_g;

  return ret;
}

/**
  * @brief   Enable/Disable inactivity function.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of inactivity in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_act_mode_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_THS, (uint8_t*)&wake_up_ths, 1);
  if(ret == 0){
    wake_up_ths.inactivity = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_WAKE_UP_THS,
                            (uint8_t*)&wake_up_ths, 1);
  }
  return ret;
}

/**
  * @brief   Enable/Disable inactivity function.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of inactivity in reg WAKE_UP_THS
  *
  */
int32_t lsm6ds3_act_mode_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_THS, (uint8_t*)&wake_up_ths, 1);
  *val = wake_up_ths.inactivity;

  return ret;
}

/**
  * @brief   Duration to go in sleep mode. 1 LSb = 512 / ODR[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sleep_dur in reg WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_act_sleep_dur_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_DUR, (uint8_t*)&wake_up_dur, 1);
  if(ret == 0){
    wake_up_dur.sleep_dur = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_WAKE_UP_DUR,
                            (uint8_t*)&wake_up_dur, 1);
  }
  return ret;
}

/**
  * @brief   Duration to go in sleep mode. 1 LSb = 512 / ODR[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sleep_dur in reg WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_act_sleep_dur_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_DUR, (uint8_t*)&wake_up_dur, 1);
  *val = (uint8_t)wake_up_dur.sleep_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Tap_generator
  * @brief       This section groups all the functions that manage the
  *              tap and double tap event generation.
  * @{
  *
  */

/**
  * @brief   Read the tap_src status flag of the device.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val          Read the tap_src status flag of the device.
  *
  */
int32_t lsm6ds3_tap_src_get(lsm6ds3_ctx_t *ctx, lsm6ds3_tap_src_t *val)
{
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_SRC, (uint8_t*)val, 1);

  return ret;
}

/**
  * @brief   Enable Z direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of tap_z_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_detection_on_z_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  if(ret == 0){
    tap_cfg.tap_z_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Enable Z direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of tap_z_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_detection_on_z_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  *val = (uint8_t)tap_cfg.tap_z_en;

  return ret;
}

/**
  * @brief   Enable Y direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of tap_y_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_detection_on_y_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  if(ret == 0){
    tap_cfg.tap_y_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Enable Y direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of tap_y_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_detection_on_y_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  *val = (uint8_t)tap_cfg.tap_y_en;

  return ret;
}

/**
  * @brief   Enable X direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of tap_x_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_detection_on_x_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  if(ret == 0){
    tap_cfg.tap_x_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Enable X direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of tap_x_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_detection_on_x_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  *val = (uint8_t)tap_cfg.tap_x_en;

  return ret;
}

/**
  * @brief   Threshold for tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of tap_ths in reg TAP_THS_6D
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_threshold_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_tap_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_THS_6D, (uint8_t*)&tap_ths_6d, 1);
  if(ret == 0){
    tap_ths_6d.tap_ths = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_THS_6D, (uint8_t*)&tap_ths_6d, 1);
  }
  return ret;
}

/**
  * @brief   Threshold for tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of tap_ths in reg TAP_THS_6D
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_threshold_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_tap_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_THS_6D, (uint8_t*)&tap_ths_6d, 1);
  *val = (uint8_t)tap_ths_6d.tap_ths;

  return ret;
}

/**
  * @brief   Maximum duration is the maximum time of an overthreshold signal
  *          detection to be recognized as a tap event. The default value
  *          of these bits is 00b which corresponds to 4*ODR_XL time.
  *          If the SHOCK[1:0] bits are set to a different value,
  *          1LSB corresponds to 8*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of shock in reg INT_DUR2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_shock_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_int_dur2_t int_dur2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  if(ret == 0){
    int_dur2.shock = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  }
  return ret;
}

/**
  * @brief   Maximum duration is the maximum time of an overthreshold signal
  *          detection to be recognized as a tap event.
  *          The default value of these bits is 00b which corresponds
  *          to 4*ODR_XL time. If the SHOCK[1:0] bits are set to a different
  *          value, 1LSB corresponds to 8*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of shock in reg INT_DUR2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_shock_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_int_dur2_t int_dur2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  *val = (uint8_t)int_dur2.shock;

  return ret;
}

/**
  * @brief   Quiet time is the time after the first detected tap in
  *          which there must not be any over-threshold event.
  *          The default value of these bits is 00b which corresponds
  *          to 2*ODR_XL time. If the QUIET[1:0] bits are set to a
  *          different value, 1LSB corresponds to 4*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of quiet in reg INT_DUR2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_quiet_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_int_dur2_t int_dur2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  if(ret == 0){
    int_dur2.quiet = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  }
  return ret;
}

/**
  * @brief   Quiet time is the time after the first detected tap in which
  *          there must not be any over-threshold event. The default value
  *          of these bits is 00b which corresponds to 2*ODR_XL time.
  *          If the QUIET[1:0] bits are set to a different value,
  *          1LSB corresponds to 4*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of quiet in reg INT_DUR2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_quiet_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_int_dur2_t int_dur2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  *val = (uint8_t)int_dur2.quiet;

  return ret;
}

/**
  * @brief   When double tap recognition is enabled, this register
  *          expresses the maximum time between two consecutive detected
  *          taps to determine a double tap event. The default value of
  *          these bits is 0000b which corresponds to 16*ODR_XL time.
  *          If the DUR[3:0] bits are set to a different value,
  *          1LSB corresponds to 32*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of dur in reg INT_DUR2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_dur_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_int_dur2_t int_dur2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  if(ret == 0){
    int_dur2.dur = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  }
  return ret;
}

/**
  * @brief   When double tap recognition is enabled, this register
  *          expresses the maximum time between two consecutive detected
  *          taps to determine a double tap event.
  *          The default value of these bits is 0000b which corresponds
  *          to 16*ODR_XL time. If the DUR[3:0] bits are set to a
  *          different value, 1LSB corresponds to 32*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of dur in reg INT_DUR2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_dur_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_int_dur2_t int_dur2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_INT_DUR2, (uint8_t*)&int_dur2, 1);
  *val = (uint8_t)int_dur2.dur;

  return ret;
}

/**
  * @brief   Single/double-tap event enable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of single_double_tap in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tap_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_tap_md_t val)
{
  lsm6ds3_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_THS, (uint8_t*)&wake_up_ths, 1);
  if(ret == 0){
    wake_up_ths.single_double_tap = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_WAKE_UP_THS,
                            (uint8_t*)&wake_up_ths, 1);
  }
  return ret;
}

/**
  * @brief   Single/double-tap event enable.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of single_double_tap in reg WAKE_UP_THS
  *
  */
int32_t lsm6ds3_tap_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_tap_md_t *val)
{
  lsm6ds3_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_WAKE_UP_THS, (uint8_t*)&wake_up_ths, 1);

  switch (wake_up_ths.single_double_tap)
  {
    case LSM6DS3_ONLY_DOUBLE:
      *val = LSM6DS3_ONLY_DOUBLE;
      break;
    case LSM6DS3_SINGLE_DOUBLE:
      *val = LSM6DS3_SINGLE_DOUBLE;
      break;
    default:
      *val = LSM6DS3_ONLY_DOUBLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Six_position_detection(6D/4D)
  * @brief       This section groups all the functions concerning
  *              six position detection (6D).
  * @{
  *
  */

/**
  * @brief   LPF2 feed 6D function selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of low_pass_on_6d in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_6d_feed_data_set(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_low_pass_on_6d_t val)
{
  lsm6ds3_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  if(ret == 0){
    ctrl8_xl.low_pass_on_6d = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  }
  return ret;
}

/**
  * @brief   LPF2 feed 6D function selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of low_pass_on_6d in reg CTRL8_XL
  *
  */
int32_t lsm6ds3_6d_feed_data_get(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_low_pass_on_6d_t *val)
{
  lsm6ds3_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);

  switch (ctrl8_xl.low_pass_on_6d)
  {
    case LSM6DS3_ODR_DIV_2_FEED:
      *val = LSM6DS3_ODR_DIV_2_FEED;
      break;
    case LSM6DS3_LPF2_FEED:
      *val = LSM6DS3_LPF2_FEED;
      break;
    default:
      *val = LSM6DS3_ODR_DIV_2_FEED;
      break;
  }
  return ret;
}

/**
  * @brief   Read the d6d_src status flag of the device.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val          Read the d6d_src status flag of the device.
  *
  */
int32_t lsm6ds3_6d_src_get(lsm6ds3_ctx_t *ctx, lsm6ds3_d6d_src_t *val)
{
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_D6D_SRC, (uint8_t*)val, 1);

  return ret;
}

/**
  * @brief   Threshold for 4D/6D function.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sixd_ths in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_6d_threshold_set(lsm6ds3_ctx_t *ctx, lsm6ds3_sixd_ths_t val)
{
  lsm6ds3_tap_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_THS_6D, (uint8_t*)&tap_ths_6d, 1);
  if(ret == 0){
    tap_ths_6d.sixd_ths = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_THS_6D,
                            (uint8_t*)&tap_ths_6d, 1);
  }
  return ret;
}

/**
  * @brief   Threshold for 4D/6D function.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of sixd_ths in reg TAP_THS_6D
  *
  */
int32_t lsm6ds3_6d_threshold_get(lsm6ds3_ctx_t *ctx, lsm6ds3_sixd_ths_t *val)
{
  lsm6ds3_tap_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_THS_6D, (uint8_t*)&tap_ths_6d, 1);

  switch (tap_ths_6d.sixd_ths)
  {
    case LSM6DS3_DEG_80:
      *val = LSM6DS3_DEG_80;
      break;
    case LSM6DS3_DEG_70:
      *val = LSM6DS3_DEG_70;
      break;
    case LSM6DS3_DEG_60:
      *val = LSM6DS3_DEG_60;
      break;
    case LSM6DS3_DEG_50:
      *val = LSM6DS3_DEG_50;
      break;
    default:
      *val = LSM6DS3_DEG_80;
      break;
  }
  return ret;
}

/**
  * @brief   4D orientation detection enable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of d4d_en in reg TAP_THS_6D
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_4d_mode_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_tap_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_THS_6D, (uint8_t*)&tap_ths_6d, 1);
  if(ret == 0){
    tap_ths_6d.d4d_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_THS_6D,
                            (uint8_t*)&tap_ths_6d, 1);
  }
  return ret;
}

/**
  * @brief   4D orientation detection enable.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of d4d_en in reg TAP_THS_6D
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_4d_mode_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_tap_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_THS_6D, (uint8_t*)&tap_ths_6d, 1);
  *val = (uint8_t)tap_ths_6d.d4d_en;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Free_fall
  * @brief       This section group all the functions concerning the
  *              free fall detection.
  * @{
  *
  */

/**
  * @brief   Free fall threshold setting.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of ff_ths in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_ff_threshold_set(lsm6ds3_ctx_t *ctx, lsm6ds3_ff_ths_t val)
{
  lsm6ds3_free_fall_t free_fall;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FREE_FALL, (uint8_t*)&free_fall, 1);
  if(ret == 0){
    free_fall.ff_ths = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FREE_FALL, (uint8_t*)&free_fall, 1);
  }
  return ret;
}

/**
  * @brief   Free fall threshold setting.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of ff_ths in reg FREE_FALL
  *
  */
int32_t lsm6ds3_ff_threshold_get(lsm6ds3_ctx_t *ctx, lsm6ds3_ff_ths_t *val)
{
  lsm6ds3_free_fall_t free_fall;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FREE_FALL, (uint8_t*)&free_fall, 1);

  switch (free_fall.ff_ths)
  {
    case LSM6DS3_156_mg:
      *val = LSM6DS3_156_mg;
      break;
    case LSM6DS3_219_mg:
      *val = LSM6DS3_219_mg;
      break;
    case LSM6DS3_250_mg:
      *val = LSM6DS3_250_mg;
      break;
    case LSM6DS3_312_mg:
      *val = LSM6DS3_312_mg;
      break;
    case LSM6DS3_344_mg:
      *val = LSM6DS3_344_mg;
      break;
    case LSM6DS3_406_mg:
      *val = LSM6DS3_406_mg;
      break;
    case LSM6DS3_469_mg:
      *val = LSM6DS3_469_mg;
      break;
    case LSM6DS3_500_mg:
      *val = LSM6DS3_500_mg;
      break;
    default:
      *val = LSM6DS3_156_mg;
      break;
  }
  return ret;
}

/**
  * @brief   Free-fall duration event. 1LSb = 1 / ODR[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of ff_dur in reg FREE_FALL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_ff_dur_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_free_fall_t free_fall;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FREE_FALL, (uint8_t*)&free_fall, 1);
  if(ret == 0){
    free_fall.ff_dur = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FREE_FALL, (uint8_t*)&free_fall, 1);
  }
  return ret;
}

/**
  * @brief   Free-fall duration event. 1LSb = 1 / ODR[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of ff_dur in reg FREE_FALL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_ff_dur_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_free_fall_t free_fall;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FREE_FALL, (uint8_t*)&free_fall, 1);
  *val = (uint8_t)free_fall.ff_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Fifo
  * @brief       This section group all the functions concerning the
  *              fifo usage
  * @{
  *
  */

/**
  * @brief   FIFO watermark level selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of fth in reg FIFO_CTRL1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_watermark_set(lsm6ds3_ctx_t *ctx, uint16_t val)
{
  lsm6ds3_fifo_ctrl1_t fifo_ctrl1;
  lsm6ds3_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL1, (uint8_t*)&fifo_ctrl1, 1);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
  }
  if(ret == 0){
    fifo_ctrl2.fth = (uint8_t)((val & 0x0F00U) >> 8);
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
  }
  if(ret == 0){
    fifo_ctrl1.fth = (uint8_t)(val & 0x00FF0U);
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL1, (uint8_t*)&fifo_ctrl1, 1);
  }
  return ret;
}

/**
  * @brief   FIFO watermark level selection.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of fth in reg FIFO_CTRL1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_watermark_get(lsm6ds3_ctx_t *ctx, uint16_t *val)
{
  lsm6ds3_fifo_ctrl1_t fifo_ctrl1;
  lsm6ds3_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL1, (uint8_t*)&fifo_ctrl1, 1);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
    *val = (uint16_t)fifo_ctrl2.fth << 8;
    *val |= fifo_ctrl1.fth;
  }
  return ret;
}

/**
  * @brief   trigger signal for FIFO write operation.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of  timer_pedo_fifo_drdy in reg LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_write_trigger_set(lsm6ds3_ctx_t *ctx,
                               lsm6ds3_tmr_ped_fifo_drdy_t val)
{
  lsm6ds3_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
  if(ret == 0){
    fifo_ctrl2. timer_pedo_fifo_drdy = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
  }
  return ret;
}

/**
  * @brief   trigger signal for FIFO write operation.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of  timer_pedo_fifo_drdy in
  *                     reg FIFO_CTRL2
  *
  */
int32_t lsm6ds3_fifo_write_trigger_get(lsm6ds3_ctx_t *ctx,
                                       lsm6ds3_tmr_ped_fifo_drdy_t *val)
{
  lsm6ds3_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);

  switch (fifo_ctrl2. timer_pedo_fifo_drdy)
  {
    case LSM6DS3_TRG_XL_GY_DRDY:
      *val = LSM6DS3_TRG_XL_GY_DRDY;
      break;
    case LSM6DS3_TRG_STEP_DETECT:
      *val = LSM6DS3_TRG_STEP_DETECT;
      break;
    default:
      *val = LSM6DS3_TRG_XL_GY_DRDY;
      break;
  }
  return ret;
}

/**
  * @brief   Pedometer step counter and timestamp as 4th FIFO data set.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of timer_pedo_fifo_en in reg FIFO_CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_pedo_batch_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
  if(ret == 0){
    fifo_ctrl2.timer_pedo_fifo_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL2,
                            (uint8_t*)&fifo_ctrl2, 1);
  }
  return ret;
}

/**
  * @brief   Pedometer step counter and timestamp as 4th FIFO data set.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of timer_pedo_fifo_en in reg FIFO_CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_pedo_batch_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
  *val = (uint8_t)fifo_ctrl2.timer_pedo_fifo_en;

  return ret;
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO) for
  *          accelerometer data.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of dec_fifo_xl in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_xl_batch_set(lsm6ds3_ctx_t *ctx, lsm6ds3_dec_fifo_xl_t val)
{
  lsm6ds3_fifo_ctrl3_t fifo_ctrl3;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL3, (uint8_t*)&fifo_ctrl3, 1);
  if(ret == 0){
    fifo_ctrl3.dec_fifo_xl = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL3,
                            (uint8_t*)&fifo_ctrl3, 1);
  }
  return ret;
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO) for
  *          accelerometer data.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of dec_fifo_xl in reg FIFO_CTRL3
  *
  */
int32_t lsm6ds3_fifo_xl_batch_get(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_dec_fifo_xl_t *val)
{
  lsm6ds3_fifo_ctrl3_t fifo_ctrl3;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL3, (uint8_t*)&fifo_ctrl3, 1);

  switch (fifo_ctrl3.dec_fifo_xl)
  {
    case LSM6DS3_FIFO_XL_DISABLE:
      *val = LSM6DS3_FIFO_XL_DISABLE;
      break;
    case LSM6DS3_FIFO_XL_NO_DEC:
      *val = LSM6DS3_FIFO_XL_NO_DEC;
      break;
    case LSM6DS3_FIFO_XL_DEC_2:
      *val = LSM6DS3_FIFO_XL_DEC_2;
      break;
    case LSM6DS3_FIFO_XL_DEC_3:
      *val = LSM6DS3_FIFO_XL_DEC_3;
      break;
    case LSM6DS3_FIFO_XL_DEC_4:
      *val = LSM6DS3_FIFO_XL_DEC_4;
      break;
    case LSM6DS3_FIFO_XL_DEC_8:
      *val = LSM6DS3_FIFO_XL_DEC_8;
      break;
    case LSM6DS3_FIFO_XL_DEC_16:
      *val = LSM6DS3_FIFO_XL_DEC_16;
      break;
    case LSM6DS3_FIFO_XL_DEC_32:
      *val = LSM6DS3_FIFO_XL_DEC_32;
      break;
    default:
      *val = LSM6DS3_FIFO_XL_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for gyroscope data.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of dec_fifo_gyro in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_gy_batch_set(lsm6ds3_ctx_t *ctx, lsm6ds3_dec_fifo_gyro_t val)
{
  lsm6ds3_fifo_ctrl3_t fifo_ctrl3;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL3, (uint8_t*)&fifo_ctrl3, 1);
  if(ret == 0){
    fifo_ctrl3.dec_fifo_gyro = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL3,
                            (uint8_t*)&fifo_ctrl3, 1);
  }
  return ret;
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for gyroscope data.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of dec_fifo_gyro in reg FIFO_CTRL3
  *
  */
int32_t lsm6ds3_fifo_gy_batch_get(lsm6ds3_ctx_t *ctx,
                                  lsm6ds3_dec_fifo_gyro_t *val)
{
  lsm6ds3_fifo_ctrl3_t fifo_ctrl3;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL3, (uint8_t*)&fifo_ctrl3, 1);

  switch (fifo_ctrl3.dec_fifo_gyro)
  {
    case LSM6DS3_FIFO_GY_DISABLE:
      *val = LSM6DS3_FIFO_GY_DISABLE;
      break;
    case LSM6DS3_FIFO_GY_NO_DEC:
      *val = LSM6DS3_FIFO_GY_NO_DEC;
      break;
    case LSM6DS3_FIFO_GY_DEC_2:
      *val = LSM6DS3_FIFO_GY_DEC_2;
      break;
    case LSM6DS3_FIFO_GY_DEC_3:
      *val = LSM6DS3_FIFO_GY_DEC_3;
      break;
    case LSM6DS3_FIFO_GY_DEC_4:
      *val = LSM6DS3_FIFO_GY_DEC_4;
      break;
    case LSM6DS3_FIFO_GY_DEC_8:
      *val = LSM6DS3_FIFO_GY_DEC_8;
      break;
    case LSM6DS3_FIFO_GY_DEC_16:
      *val = LSM6DS3_FIFO_GY_DEC_16;
      break;
    case LSM6DS3_FIFO_GY_DEC_32:
      *val = LSM6DS3_FIFO_GY_DEC_32;
      break;
    default:
      *val = LSM6DS3_FIFO_GY_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for third data set.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of dec_ds3_fifo in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_dataset_3_batch_set(lsm6ds3_ctx_t *ctx,
                                         lsm6ds3_dec_ds3_fifo_t val)
{
  lsm6ds3_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL4, (uint8_t*)&fifo_ctrl4, 1);
  if(ret == 0){
    fifo_ctrl4.dec_ds3_fifo = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL4,
                            (uint8_t*)&fifo_ctrl4, 1);
  }
  return ret;
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for third data set.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of dec_ds3_fifo in reg FIFO_CTRL4
  *
  */
int32_t lsm6ds3_fifo_dataset_3_batch_get(lsm6ds3_ctx_t *ctx,
                                          lsm6ds3_dec_ds3_fifo_t *val)
{
  lsm6ds3_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL4, (uint8_t*)&fifo_ctrl4, 1);

  switch (fifo_ctrl4.dec_ds3_fifo)
  {
    case LSM6DS3_FIFO_DS3_DISABLE:
      *val = LSM6DS3_FIFO_DS3_DISABLE;
      break;
    case LSM6DS3_FIFO_DS3_NO_DEC:
      *val = LSM6DS3_FIFO_DS3_NO_DEC;
      break;
    case LSM6DS3_FIFO_DS3_DEC_2:
      *val = LSM6DS3_FIFO_DS3_DEC_2;
      break;
    case LSM6DS3_FIFO_DS3_DEC_3:
      *val = LSM6DS3_FIFO_DS3_DEC_3;
      break;
    case LSM6DS3_FIFO_DS3_DEC_4:
      *val = LSM6DS3_FIFO_DS3_DEC_4;
      break;
    case LSM6DS3_FIFO_DS3_DEC_8:
      *val = LSM6DS3_FIFO_DS3_DEC_8;
      break;
    case LSM6DS3_FIFO_DS3_DEC_16:
      *val = LSM6DS3_FIFO_DS3_DEC_16;
      break;
    case LSM6DS3_FIFO_DS3_DEC_32:
      *val = LSM6DS3_FIFO_DS3_DEC_32;
      break;
    default:
      *val = LSM6DS3_FIFO_DS3_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for fourth data set.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of dec_ds4_fifo in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_dataset_4_batch_set(lsm6ds3_ctx_t *ctx,
                                         lsm6ds3_dec_ds4_fifo_t val)
{
  lsm6ds3_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL4, (uint8_t*)&fifo_ctrl4, 1);
  if(ret == 0){
    fifo_ctrl4.dec_ds4_fifo = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL4,
                            (uint8_t*)&fifo_ctrl4, 1);
  }
  return ret;
}

/**
  * @brief   Selects Batching Data Rate (writing frequency in FIFO)
  *          for fourth data set.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of dec_ds4_fifo in reg FIFO_CTRL4
  *
  */
int32_t lsm6ds3_fifo_dataset_4_batch_get(lsm6ds3_ctx_t *ctx,
                                         lsm6ds3_dec_ds4_fifo_t *val)
{
  lsm6ds3_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL4, (uint8_t*)&fifo_ctrl4, 1);

  switch (fifo_ctrl4.dec_ds4_fifo)
  {
    case LSM6DS3_FIFO_DS4_DISABLE:
      *val = LSM6DS3_FIFO_DS4_DISABLE;
      break;
    case LSM6DS3_FIFO_DS4_NO_DEC:
      *val = LSM6DS3_FIFO_DS4_NO_DEC;
      break;
    case LSM6DS3_FIFO_DS4_DEC_2:
      *val = LSM6DS3_FIFO_DS4_DEC_2;
      break;
    case LSM6DS3_FIFO_DS4_DEC_3:
      *val = LSM6DS3_FIFO_DS4_DEC_3;
      break;
    case LSM6DS3_FIFO_DS4_DEC_4:
      *val = LSM6DS3_FIFO_DS4_DEC_4;
      break;
    case LSM6DS3_FIFO_DS4_DEC_8:
      *val = LSM6DS3_FIFO_DS4_DEC_8;
      break;
    case LSM6DS3_FIFO_DS4_DEC_16:
      *val = LSM6DS3_FIFO_DS4_DEC_16;
      break;
    case LSM6DS3_FIFO_DS4_DEC_32:
      *val = LSM6DS3_FIFO_DS4_DEC_32;
      break;
    default:
      *val = LSM6DS3_FIFO_DS4_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   8-bit data storage in FIFO.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of only_high_data in reg FIFO_CTRL4
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_xl_gy_8bit_format_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL4, (uint8_t*)&fifo_ctrl4, 1);
  if(ret == 0){
    fifo_ctrl4.only_high_data = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL4,
                            (uint8_t*)&fifo_ctrl4, 1);
  }
  return ret;
}

/**
  * @brief   8-bit data storage in FIFO.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of only_high_data in reg FIFO_CTRL4
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_xl_gy_8bit_format_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL4, (uint8_t*)&fifo_ctrl4, 1);
  *val = (uint8_t)fifo_ctrl4.only_high_data;

  return ret;
}

/**
  * @brief   FIFO mode selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of fifo_mode in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_fifo_md_t val)
{
  lsm6ds3_fifo_ctrl5_t fifo_ctrl5;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL5, (uint8_t*)&fifo_ctrl5, 1);
  if(ret == 0){
    fifo_ctrl5.fifo_mode = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL5,
                            (uint8_t*)&fifo_ctrl5, 1);
  }
  return ret;
}

/**
  * @brief   FIFO mode selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of fifo_mode in reg FIFO_CTRL5
  *
  */
int32_t lsm6ds3_fifo_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_fifo_md_t *val)
{
  lsm6ds3_fifo_ctrl5_t fifo_ctrl5;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL5, (uint8_t*)&fifo_ctrl5, 1);

  switch (fifo_ctrl5.fifo_mode)
  {
    case LSM6DS3_BYPASS_MODE:
      *val = LSM6DS3_BYPASS_MODE;
      break;
    case LSM6DS3_FIFO_MODE:
      *val = LSM6DS3_FIFO_MODE;
      break;
    case  LSM6DS3_STREAM_TO_FIFO_MODE:
      *val =  LSM6DS3_STREAM_TO_FIFO_MODE;
      break;
    case  LSM6DS3_BYPASS_TO_STREAM_MODE:
      *val =  LSM6DS3_BYPASS_TO_STREAM_MODE;
      break;
    default:
      *val = LSM6DS3_BYPASS_MODE;
      break;
  }
  return ret;
}

/**
  * @brief   FIFO ODR selection, setting FIFO_MODE also.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of odr_fifo in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_data_rate_set(lsm6ds3_ctx_t *ctx, lsm6ds3_odr_fifo_t val)
{
  lsm6ds3_fifo_ctrl5_t fifo_ctrl5;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL5, (uint8_t*)&fifo_ctrl5, 1);
  if(ret == 0){
    fifo_ctrl5.odr_fifo = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_FIFO_CTRL5,
                            (uint8_t*)&fifo_ctrl5, 1);
  }
  return ret;
}

/**
  * @brief   FIFO ODR selection, setting FIFO_MODE also.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of odr_fifo in reg FIFO_CTRL5
  *
  */
int32_t lsm6ds3_fifo_data_rate_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_odr_fifo_t *val)
{
  lsm6ds3_fifo_ctrl5_t fifo_ctrl5;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_CTRL5, (uint8_t*)&fifo_ctrl5, 1);

  switch (fifo_ctrl5.odr_fifo)
  {
    case LSM6DS3_FIFO_DISABLE:
      *val = LSM6DS3_FIFO_DISABLE;
      break;
    case LSM6DS3_FIFO_12Hz5:
      *val = LSM6DS3_FIFO_12Hz5;
      break;
    case LSM6DS3_FIFO_26Hz:
      *val = LSM6DS3_FIFO_26Hz;
      break;
    case LSM6DS3_FIFO_52Hz:
      *val = LSM6DS3_FIFO_52Hz;
      break;
    case LSM6DS3_FIFO_104Hz:
      *val = LSM6DS3_FIFO_104Hz;
      break;
    case LSM6DS3_FIFO_208Hz:
      *val = LSM6DS3_FIFO_208Hz;
      break;
    case LSM6DS3_FIFO_416Hz:
      *val = LSM6DS3_FIFO_416Hz;
      break;
    case LSM6DS3_FIFO_833Hz:
      *val = LSM6DS3_FIFO_833Hz;
      break;
    case LSM6DS3_FIFO_1k66Hz:
      *val = LSM6DS3_FIFO_1k66Hz;
      break;
    case LSM6DS3_FIFO_3k33Hz:
      *val = LSM6DS3_FIFO_3k33Hz;
      break;
    case LSM6DS3_FIFO_6k66Hz:
      *val = LSM6DS3_FIFO_6k66Hz;
      break;
    default:
      *val = LSM6DS3_FIFO_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief   Sensing chain FIFO stop values memorization at
  *          threshold level.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of stop_on_fth in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_stop_on_wtm_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.stop_on_fth = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief   Sensing chain FIFO stop values memorization at
  *          threshold level.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of stop_on_fth in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_stop_on_wtm_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = (uint8_t)ctrl4_c.stop_on_fth;

  return ret;
}

/**
  * @brief   batching of temperature data.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of fifo_temp_en in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_temp_batch_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.fifo_temp_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief   batching of temperature data.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of fifo_temp_en in reg CTRL4_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_temp_batch_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = (uint8_t)ctrl4_c.fifo_temp_en;

  return ret;
}

/**
  * @brief   Number of unread words (16-bit axes) stored in FIFO.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of diff_fifo in reg FIFO_STATUS1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_data_level_get(lsm6ds3_ctx_t *ctx, uint16_t *val)
{
  lsm6ds3_fifo_status1_t fifo_status1;
  lsm6ds3_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_STATUS1,
                         (uint8_t*)&fifo_status1, 1);

  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_STATUS2,
                           (uint8_t*)&fifo_status2, 1);

    *val = (uint16_t)fifo_status2.diff_fifo << 8;
    *val |= fifo_status1.diff_fifo;
  }
  return ret;
}

/**
  * @brief   Smart FIFO full status.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of fifo_empty in reg FIFO_STATUS2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_full_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_STATUS2,
                         (uint8_t*)&fifo_status2, 1);
  *val = (uint8_t)fifo_status2.fifo_empty;

  return ret;
}

/**
  * @brief   FIFO overrun status.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of fifo_full in reg FIFO_STATUS2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_ovr_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_STATUS2,
                         (uint8_t*)&fifo_status2, 1);
  *val = (uint8_t)fifo_status2.fifo_full;

  return ret;
}

/**
  * @brief   FIFO watermark status.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of fth in reg FIFO_STATUS2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_wtm_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_STATUS2,
                         (uint8_t*)&fifo_status2, 1);
  *val = (uint8_t)fifo_status2.fth;

  return ret;
}

/**
  * @brief   Word of recursive pattern read at the next reading.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of fifo_pattern in reg FIFO_STATUS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_fifo_pattern_get(lsm6ds3_ctx_t *ctx, uint16_t *val)
{
  lsm6ds3_fifo_status3_t fifo_status3;
  lsm6ds3_fifo_status4_t fifo_status4;

  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_STATUS3,
                         (uint8_t*)&fifo_status3, 1);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_FIFO_STATUS4,
                           (uint8_t*)&fifo_status4, 1);

    *val = (uint16_t)fifo_status4.fifo_pattern << 8;
    *val |= fifo_status3.fifo_pattern;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_DEN_functionality
  * @brief       This section groups all the functions concerning DEN
  *              functionality.
  * @{
  *
  */

/**
  * @brief   DEN functionality marking mode.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of den_mode in reg CTRL6_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_den_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_den_mode_t val)
{
  lsm6ds3_ctrl6_c_t ctrl6_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL6_C, (uint8_t*)&ctrl6_c, 1);

  if(ret == 0){
    ctrl6_c.den_mode = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL6_C, (uint8_t*)&ctrl6_c, 1);
  }
  return ret;
}

/**
  * @brief   DEN functionality marking mode.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of den_mode in reg CTRL6_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_den_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_den_mode_t *val)
{
  lsm6ds3_ctrl6_c_t ctrl6_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL6_C, (uint8_t*)&ctrl6_c, 1);

  switch (ctrl6_c.den_mode)
  {
    case LSM6DS3_DEN_DISABLE:
      *val = LSM6DS3_DEN_DISABLE;
      break;
    case LSM6DS3_LEVEL_FIFO:
      *val = LSM6DS3_LEVEL_FIFO;
      break;
    case LSM6DS3_LEVEL_LETCHED:
      *val = LSM6DS3_LEVEL_LETCHED;
      break;
    case LSM6DS3_LEVEL_TRIGGER:
      *val = LSM6DS3_LEVEL_TRIGGER;
      break;
    case LSM6DS3_EDGE_TRIGGER:
      *val = LSM6DS3_EDGE_TRIGGER;
      break;
    default:
      *val = LSM6DS3_DEN_DISABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Pedometer
  * @brief       This section groups all the functions that manage pedometer.
  * @{
  *
  */

/**
  * @brief   Reset pedometer step counter.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pedo_rst_step in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_step_reset_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  if(ret == 0){
    ctrl10_c.pedo_rst_step = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  }
  return ret;
}

/**
  * @brief   Reset pedometer step counter.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of pedo_rst_step in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_step_reset_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  *val = (uint8_t)ctrl10_c.pedo_rst_step;

  return ret;
}

/**
  * @brief   Step counter timestamp information register (r). When a step is
  *          detected, the value of TIMESTAMP_REG register is copied in
  *          STEP_TIMESTAMP_L..[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_pedo_timestamp_raw_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_STEP_TIMESTAMP_L, buff, 2);
  return ret;
}

/**
  * @brief   Step detector event detection status
  *          (0:not detected / 1:detected).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of step_detected in reg FUNC_SRC
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_step_detect_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_func_src_t func_src;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FUNC_SRC, (uint8_t*)&func_src, 1);
  *val = (uint8_t)func_src.step_detected;

  return ret;
}

/**
  * @brief   Enable pedometer algorithm.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pedo_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_sens_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = 0;
  if (val == PROPERTY_ENABLE){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    if(ret == 0){
      ctrl10_c.func_en = PROPERTY_ENABLE;
      ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    }
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  if(ret == 0){
    tap_cfg.pedo_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Enable pedometer algorithm.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of pedo_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_sens_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  *val = (uint8_t)tap_cfg.pedo_en;

  return ret;
}

/**
  * @brief   Configurable minimum threshold (PEDO_4G 1LSB = 16 mg ,
  *          PEDO_2G 1LSB = 32 mg).[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of ths_min in reg PEDO_THS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_threshold_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_pedo_ths_reg_t pedo_ths_reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_PEDO_THS_REG,
                           (uint8_t*)&pedo_ths_reg, 1);
  }
  if(ret == 0){
    pedo_ths_reg.ths_min = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_PEDO_THS_REG,
                            (uint8_t*)&pedo_ths_reg, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Configurable minimum threshold (PEDO_4G 1LSB = 16 mg,
  *          PEDO_2G 1LSB = 32 mg).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of ths_min in reg PEDO_THS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_threshold_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_pedo_ths_reg_t pedo_ths_reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_PEDO_THS_REG,
                           (uint8_t*)&pedo_ths_reg, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
    *val = (uint8_t)pedo_ths_reg.ths_min;
  }
  return ret;
}

/**
  * @brief   This bit sets the internal full scale used in
  *          pedometer functions.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pedo_4g in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_full_scale_set(lsm6ds3_ctx_t *ctx, lsm6ds3_pedo_fs_t val)
{
  lsm6ds3_pedo_ths_reg_t pedo_ths_reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_PEDO_THS_REG,
                           (uint8_t*)&pedo_ths_reg, 1);
  }
  if(ret == 0){
    pedo_ths_reg.pedo_4g = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_PEDO_THS_REG,
                            (uint8_t*)&pedo_ths_reg, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   This bit sets the internal full scale used in pedometer
  *          functions.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of pedo_4g in reg PEDO_THS_REG
  *
  */
int32_t lsm6ds3_pedo_full_scale_get(lsm6ds3_ctx_t *ctx, lsm6ds3_pedo_fs_t *val)
{
  lsm6ds3_pedo_ths_reg_t pedo_ths_reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_PEDO_THS_REG,
                           (uint8_t*)&pedo_ths_reg, 1);
    switch (pedo_ths_reg.pedo_4g)
    {
      case LSM6DS3_PEDO_AT_2g:
        *val = LSM6DS3_PEDO_AT_2g;
        break;
      case LSM6DS3_PEDO_AT_4g:
        *val = LSM6DS3_PEDO_AT_4g;
        break;
      default:
        *val = LSM6DS3_PEDO_AT_2g;
        break;
    }
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Pedometer debounce configuration register (r/w).[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of deb_step in reg PEDO_DEB_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_debounce_steps_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_pedo_deb_reg_t pedo_deb_reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_PEDO_DEB_REG,
                           (uint8_t*)&pedo_deb_reg, 1);
  }
  if(ret == 0){
    pedo_deb_reg.deb_step = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_PEDO_DEB_REG,
                            (uint8_t*)&pedo_deb_reg, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Pedometer debounce configuration register (r/w).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of deb_step in reg PEDO_DEB_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_debounce_steps_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_pedo_deb_reg_t pedo_deb_reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_PEDO_DEB_REG,
                           (uint8_t*)&pedo_deb_reg, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
    *val = (uint8_t)pedo_deb_reg.deb_step;
  }
  return ret;
}

/**
  * @brief   Debounce time. If the time between two consecutive steps is
  *          greater than DEB_TIME*80ms, the debounce is reactivated.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of deb_time in reg PEDO_DEB_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_timeout_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_pedo_deb_reg_t pedo_deb_reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_PEDO_DEB_REG,
                           (uint8_t*)&pedo_deb_reg, 1);
  }
  if(ret == 0){
    pedo_deb_reg.deb_time = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_PEDO_DEB_REG,
                            (uint8_t*)&pedo_deb_reg, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Debounce time. If the time between two consecutive steps is
  *          greater than DEB_TIME*80ms, the debounce is reactivated.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of deb_time in reg PEDO_DEB_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_pedo_timeout_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_pedo_deb_reg_t pedo_deb_reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_PEDO_DEB_REG,
                           (uint8_t*)&pedo_deb_reg, 1);
  }
  if(ret == 0){
    *val = (uint8_t)pedo_deb_reg.deb_time;
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Significant_motion
  * @brief       This section groups all the functions that manage the
  *              significant motion detection.
  * @{
  *
  */

/**
  * @brief   Enable significant motion detection function.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sign_motion_en in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_motion_sens_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  if(ret == 0){
    ctrl10_c.sign_motion_en = (uint8_t)val;
    if (val == PROPERTY_ENABLE){
      ctrl10_c.func_en = PROPERTY_ENABLE;
    }
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  }
  return ret;
}

/**
  * @brief   Enable significant motion detection function.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sign_motion_en in reg CTRL10_C
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_motion_sens_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  *val = (uint8_t)ctrl10_c.sign_motion_en;

  return ret;
}

/**
  * @brief   Significant motion event detection status
  *          (0:not detected / 1:detected).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sign_motion_ia in reg FUNC_SRC
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_motion_event_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_func_src_t func_src;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FUNC_SRC, (uint8_t*)&func_src, 1);
  *val = (uint8_t)func_src.sign_motion_ia;

  return ret;
}

/**
  * @brief   Significant motion threshold.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sm_ths in reg SM_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_motion_threshold_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_sm_ths_t sm_ths;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SM_THS, (uint8_t*)&sm_ths, 1);
  }
  if(ret == 0){
    sm_ths.sm_ths = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SM_THS, (uint8_t*)&sm_ths, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Significant motion threshold.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sm_ths in reg SM_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_motion_threshold_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_sm_ths_t sm_ths;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SM_THS, (uint8_t*)&sm_ths, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
    *val = (uint8_t)sm_ths.sm_ths;
  }
  return ret;
}

/**
  * @brief   Time period register for step detection on delta time
  *          (1LSB = 1.6384 s).[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sc_delta in reg STEP_COUNT_DELTA
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sc_delta_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_step_count_delta_t  step_count_delta;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_STEP_COUNT_DELTA,
                           (uint8_t*)& step_count_delta, 1);
  }
  if(ret == 0){
    step_count_delta.sc_delta = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_STEP_COUNT_DELTA,
                            (uint8_t*)& step_count_delta, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Time period register for step detection on delta time
  *          (1LSB = 1.6384 s).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sc_delta in reg  STEP_COUNT_DELTA
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sc_delta_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_step_count_delta_t  step_count_delta;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_STEP_COUNT_DELTA,
                           (uint8_t*)& step_count_delta, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
    *val = (uint8_t) step_count_delta.sc_delta;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Tilt_detection
  * @brief       This section groups all the functions that manage
  *              the tilt event detection.
  * @{
  *
  */

/**
  * @brief   Tilt event detection status(0:not detected / 1:detected).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of tilt_ia in reg FUNC_SRC
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tilt_event_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_func_src_t func_src;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FUNC_SRC, (uint8_t*)&func_src, 1);
  *val = (uint8_t)func_src.tilt_ia;

  return ret;
}

/**
  * @brief   Enable tilt calculation.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of tilt_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tilt_sens_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = 0;
  if (val == PROPERTY_ENABLE){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    if(ret == 0){
      ctrl10_c.func_en = PROPERTY_ENABLE;
      ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    }
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  if(ret == 0){
    tap_cfg.tilt_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Enable tilt calculation.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of tilt_en in reg TAP_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_tilt_sens_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  *val = (uint8_t)tap_cfg.tilt_en;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Magnetometer_sensor
  * @brief       This section groups all the functions that manage
  *              additional magnetometer sensor.
  * @{
  *
  */

/**
  * @brief   Enable soft-iron correction algorithm for magnetometer.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of soft_en in reg CTRL9_XL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_mag_soft_iron_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  lsm6ds3_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = 0;
  if (val == PROPERTY_ENABLE){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    if(ret == 0){
      ctrl10_c.func_en = PROPERTY_ENABLE;
      ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    }
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  }
  if(ret == 0){
    ctrl9_xl.soft_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief   Enable soft-iron correction algorithm for magnetometer.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of soft_en in reg CTRL9_XL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_mag_soft_iron_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  *val = (uint8_t)ctrl9_xl.soft_en;

  return ret;
}

/**
  * @brief   Enable hard-iron correction algorithm for magnetometer.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of iron_en in reg MASTER_CONFIG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_mag_hard_iron_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = 0;
  if (val == PROPERTY_ENABLE){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    ctrl10_c.func_en = PROPERTY_ENABLE;
    if(ret == 0){
      ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    }
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                           (uint8_t*)&master_config, 1);
  }
  if(ret == 0){
    master_config.iron_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_MASTER_CONFIG,
                            (uint8_t*)&master_config, 1);
  }
  return ret;
}

/**
  * @brief   Enable hard-iron correction algorithm for magnetometer.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of iron_en in reg MASTER_CONFIG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_mag_hard_iron_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);
  *val = (uint8_t)master_config.iron_en;

  return ret;
}

/**
  * @brief   Hard/soft-iron calculation status (0: on-going / 1: idle).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of si_end_op in reg FUNC_SRC
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_mag_soft_iron_end_op_flag_get(lsm6ds3_ctx_t *ctx,
                                               uint8_t *val)
{
  lsm6ds3_func_src_t func_src;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FUNC_SRC, (uint8_t*)&func_src, 1);
  *val = (uint8_t)func_src.si_end_op;

  return ret;
}

/**
  * @brief   Soft-iron matrix correction registers[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data to be write
  *
  */
int32_t lsm6ds3_mag_soft_iron_coeff_set(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_MAG_SI_XX, buff, 9);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Soft-iron matrix correction registers[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_mag_soft_iron_coeff_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_MAG_SI_XX, buff, 9);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Offset for hard-iron compensation register (r/w).
  *          The value is expressed as a 16-bit word in two’s complement.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data to be write
  *
  */
int32_t lsm6ds3_mag_offset_set(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
   ret = lsm6ds3_write_reg(ctx, LSM6DS3_MAG_OFFX_L, buff, 6);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Offset for hard-iron compensation register(r/w).
  *          The value is expressed as a 16-bit word in two’s complement.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_mag_offset_get(lsm6ds3_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_MAG_OFFX_L, buff, 6);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LSM6DS3_Sensor_hub
  * @brief       This section groups all the functions that manage the
  *              sensor hub functionality.
  * @{
  *
  */

/**
  * @brief   Sensor synchronization time frame with the step of 500 ms
  *          and full range of 5 s. Unsigned 8-bit.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of tph in reg  SENSOR_SYNC_TIME_FRAME
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_sync_sens_frame_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_sensor_sync_time_frame_t  sensor_sync_time_frame;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_SENSOR_SYNC_TIME_FRAME,
                            (uint8_t*)& sensor_sync_time_frame, 1);
  if(ret == 0){
     sensor_sync_time_frame.tph = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SENSOR_SYNC_TIME_FRAME,
                               (uint8_t*)& sensor_sync_time_frame, 1);
  }
  return ret;
}

/**
  * @brief   Sensor synchronization time frame with the step of 500 ms and
  *          full range of 5 s. Unsigned 8-bit.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of tph in reg  SENSOR_SYNC_TIME_FRAME
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_sync_sens_frame_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_sensor_sync_time_frame_t  sensor_sync_time_frame;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_SENSOR_SYNC_TIME_FRAME,
                         (uint8_t*)& sensor_sync_time_frame, 1);
  *val = (uint8_t) sensor_sync_time_frame.tph;

  return ret;
}
/**
  * @brief   Sensor hub I2C master enable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of master_on in reg MASTER_CONFIG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_master_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_ctrl10_c_t ctrl10_c;
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = 0;
  if (val == PROPERTY_ENABLE){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    if(ret == 0){
      ctrl10_c.func_en = PROPERTY_ENABLE;
      ret = lsm6ds3_write_reg(ctx, LSM6DS3_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
    }
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                           (uint8_t*)&master_config, 1);
  }
  if(ret == 0){
    master_config.master_on = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_MASTER_CONFIG,
                            (uint8_t*)&master_config, 1);
  }
  return ret;
}

/**
  * @brief   Sensor hub I2C master enable.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of master_on in reg MASTER_CONFIG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_master_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);
  *val = (uint8_t)master_config.master_on;

  return ret;
}
/**
  * @brief   I2C interface pass-through.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pass_through_mode in
  *                  reg MASTER_CONFIG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_pass_through_set(lsm6ds3_ctx_t *ctx, uint8_t val)
{
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);
  if(ret == 0){
    master_config.pass_through_mode = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_MASTER_CONFIG,
                            (uint8_t*)&master_config, 1);
  }
  return ret;
}

/**
  * @brief   I2C interface pass-through.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of pass_through_mode in reg MASTER_CONFIG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_pass_through_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);
  *val = (uint8_t)master_config.pass_through_mode;

  return ret;
}
/**
  * @brief   Master I2C pull-up enable/disable.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pull_up_en in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_pin_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_sh_pin_md_t val)
{
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);
  if(ret == 0){
    master_config.pull_up_en = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_MASTER_CONFIG,
                            (uint8_t*)&master_config, 1);
  }
  return ret;
}

/**
  * @brief   Master I2C pull-up enable/disable.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of pull_up_en in reg MASTER_CONFIG
  *
  */
int32_t lsm6ds3_sh_pin_mode_get(lsm6ds3_ctx_t *ctx, lsm6ds3_sh_pin_md_t *val)
{
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);

  switch (master_config.pull_up_en)
  {
    case LSM6DS3_EXT_PULL_UP:
      *val = LSM6DS3_EXT_PULL_UP;
      break;
    case LSM6DS3_INTERNAL_PULL_UP:
      *val = LSM6DS3_INTERNAL_PULL_UP;
      break;
    default:
      *val = LSM6DS3_EXT_PULL_UP;
      break;
  }
  return ret;
}

/**
  * @brief   Sensor hub trigger signal selection.[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of start_config in reg  LSM6DS3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_syncro_mode_set(lsm6ds3_ctx_t *ctx, lsm6ds3_start_cfg_t val)
{
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);
  if(ret == 0){
    master_config.start_config = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_MASTER_CONFIG,
                            (uint8_t*)&master_config, 1);
  }
  return ret;
}

/**
  * @brief   Sensor hub trigger signal selection.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         get the values of start_config in reg MASTER_CONFIG
  *
  */
int32_t lsm6ds3_sh_syncro_mode_get(lsm6ds3_ctx_t *ctx,
                                   lsm6ds3_start_cfg_t *val)
{
  lsm6ds3_master_config_t master_config;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_MASTER_CONFIG,
                         (uint8_t*)&master_config, 1);

  switch (master_config.start_config)
  {
    case LSM6DS3_XL_GY_DRDY:
      *val = LSM6DS3_XL_GY_DRDY;
      break;
    case LSM6DS3_EXT_ON_INT2_PIN:
      *val = LSM6DS3_EXT_ON_INT2_PIN;
      break;
    default:
      *val = LSM6DS3_XL_GY_DRDY;
      break;
  }
  return ret;
}

/**
  * @brief   Sensor hub output registers.[get]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  buff        buffer that stores data read
  *
  */
int32_t lsm6ds3_sh_read_data_raw_get(lsm6ds3_ctx_t *ctx,
                                     lsm6ds3_sh_read_t *buff)
{
  int32_t ret;
  ret = lsm6ds3_read_reg(ctx, LSM6DS3_SENSORHUB1_REG,
                         (uint8_t*)&(buff->sh_byte_1), 12);

  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SENSORHUB13_REG,
                           (uint8_t*)&(buff->sh_byte_13), 6);
  }
  return ret;
}

/**
  * @brief  sh_cfg_write: Configure slave 0 for perform a write.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_sh_cfg_write_t: a structure that contain
  *                      - uint8_t slv1_add;    8 bit i2c device address
  *                      - uint8_t slv1_subadd; 8 bit register device address
  *                      - uint8_t slv1_data;   8 bit data to write
  *
  */
int32_t lsm6ds3_sh_cfg_write(lsm6ds3_ctx_t *ctx, lsm6ds3_sh_cfg_write_t *val)
{
  lsm6ds3_slv0_add_t slv0_add;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    slv0_add.slave0_add = val->slv0_add >> 1;
    slv0_add.rw_0 = 0;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV0_ADD, (uint8_t*)&slv0_add, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV0_SUBADD,
                            &(val->slv0_subadd), 1);
  }
  if(ret == 0){
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_DATAWRITE_SRC_MODE_SUB_SLV0,
                            &(val->slv0_data), 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }

  return ret;
}

/**
  * @brief  sh_slv0_cfg_read: [get] Configure slave 0 for perform a write/read.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_sh_cfg_read_t: a structure that contain
  *                      - uint8_t slv1_add;    8 bit i2c device address
  *                      - uint8_t slv1_subadd; 8 bit register device address
  *                      - uint8_t slv1_len;    num of bit to read
  *
  */
int32_t lsm6ds3_sh_slv0_cfg_read(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_sh_cfg_read_t *val)
{
  lsm6ds3_slv0_add_t slv0_add;
  lsm6ds3_slave0_config_t slave0_config;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    slv0_add.slave0_add = val->slv_add >> 1;
    slv0_add.rw_0 = 1;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV0_ADD, (uint8_t*)&slv0_add, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV0_SUBADD,
                            &(val->slv_subadd), 1);
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SLAVE0_CONFIG,
                           (uint8_t*)&slave0_config, 1);
  }
  if(ret == 0){
    slave0_config.slave0_numop = val->slv_len;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLAVE0_CONFIG,
                            (uint8_t*)&slave0_config, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief  sh_slv1_cfg_read: [get] Configure slave 0 for perform a write/read.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_sh_cfg_read_t: a structure that contain
  *                      - uint8_t slv1_add;    8 bit i2c device address
  *                      - uint8_t slv1_subadd; 8 bit register device address
  *                      - uint8_t slv1_len;    num of bit to read
  *
  */
int32_t lsm6ds3_sh_slv1_cfg_read(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_sh_cfg_read_t *val)
{
  lsm6ds3_slv1_add_t slv1_add;
  lsm6ds3_slave1_config_t slave1_config;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    slv1_add.slave1_add = val->slv_add >> 1;;
    slv1_add.r_1 = 1;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV1_ADD, (uint8_t*)&slv1_add, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV1_SUBADD,
                            &(val->slv_subadd), 1);
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SLAVE1_CONFIG, (uint8_t*)&slave1_config, 1);
  }
  if(ret == 0){
    slave1_config.slave1_numop = val->slv_len;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLAVE1_CONFIG, (uint8_t*)&slave1_config, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief  sh_slv2_cfg_read: [get] Configure slave 0 for perform a write/read.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_sh_cfg_read_t: a structure that contain
  *                      - uint8_t slv2_add;    8 bit i2c device address
  *                      - uint8_t slv2_subadd; 8 bit register device address
  *                      - uint8_t slv2_len;    num of bit to read
  *
  */
int32_t lsm6ds3_sh_slv2_cfg_read(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_sh_cfg_read_t *val)
{
  lsm6ds3_slv2_add_t slv2_add;
  lsm6ds3_slave2_config_t slave2_config;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    slv2_add.slave2_add = val->slv_add >> 1;
    slv2_add.r_2 = 1;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV2_ADD,
                            (uint8_t*)&slv2_add, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV2_SUBADD,
                            &(val->slv_subadd), 1);
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SLAVE2_CONFIG,
                           (uint8_t*)&slave2_config, 1);
  }
  if(ret == 0){
    slave2_config.slave2_numop = val->slv_len;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLAVE2_CONFIG,
                            (uint8_t*)&slave2_config, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief  sh_slv3_cfg_read: [get] Configure slave 0 for perform a write/read.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_sh_cfg_read_t: a structure that contain
  *                      - uint8_t slv3_add;    8 bit i2c device address
  *                      - uint8_t slv3_subadd; 8 bit register device address
  *                      - uint8_t slv3_len;    num of bit to read
  *
  */
int32_t lsm6ds3_sh_slv3_cfg_read(lsm6ds3_ctx_t *ctx,
                                 lsm6ds3_sh_cfg_read_t *val)
{
  lsm6ds3_slv3_add_t slv3_add;
  lsm6ds3_slave3_config_t slave3_config;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    slv3_add.slave3_add = val->slv_add >> 1;
    slv3_add.r_3 = 1;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV3_ADD, (uint8_t*)&slv3_add, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLV3_SUBADD,
                            &(val->slv_subadd), 1);
  }
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SLAVE3_CONFIG,
                           (uint8_t*)&slave3_config, 1);
  }
  if(ret == 0){
    slave3_config.slave3_numop = val->slv_len;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLAVE3_CONFIG,
                            (uint8_t*)&slave3_config, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Sensor hub communication status (0: on-going / 1: idle).[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sensor_hub_end_op in reg FUNC_SRC
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lsm6ds3_sh_end_op_flag_get(lsm6ds3_ctx_t *ctx, uint8_t *val)
{
  lsm6ds3_func_src_t func_src;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_FUNC_SRC, (uint8_t*)&func_src, 1);
  *val = (uint8_t)func_src.sensor_hub_end_op;

  return ret;
}

/**
  * @brief   xl_hp_path_internal: [set] HPF or SLOPE filter selection on
  *                                     wake-up and Activity/Inactivity
  *                                     functions.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_slope_fds_t: change the values of slope_fds in reg TAP_CFG
  *
  */
int32_t lsm6ds3_xl_hp_path_internal_set(lsm6ds3_ctx_t *ctx,
                                        lsm6ds3_slope_fds_t val)
{
  lsm6ds3_tap_cfg_t tap_cfg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  if(ret == 0){
    tap_cfg.slope_fds = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&tap_cfg, 1);
  }
  return ret;
}

/**
  * @brief   xl_hp_path_internal: [get] HPF or SLOPE filter selection on
  *                                     wake-up and Activity/Inactivity
  *                                     functions.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_slope_fds_t: Get the values of slope_fds in reg TAP_CFG
  *
  */
int32_t lsm6ds3_xl_hp_path_internal_get(lsm6ds3_ctx_t *ctx,
                                        lsm6ds3_slope_fds_t *val)
{
  lsm6ds3_tap_cfg_t reg;
  int32_t ret;

  ret = lsm6ds3_read_reg(ctx, LSM6DS3_TAP_CFG, (uint8_t*)&reg, 1);
  switch (reg.slope_fds)
  {
    case LSM6DS3_USE_SLOPE:
      *val = LSM6DS3_USE_SLOPE;
      break;
    case LSM6DS3_USE_HPF:
      *val = LSM6DS3_USE_HPF;
      break;
    default:
      *val = LSM6DS3_USE_SLOPE;
      break;
  }
  return ret;
}

/**
  * @brief   sh_num_of_dev_connected: [set] Number of external sensors to
  *                                         be read by the sensor hub.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_aux_sens_on_t: change the values of aux_sens_on in
  *                                reg SLAVE0_CONFIG
  *
  */
int32_t lsm6ds3_sh_num_of_dev_connected_set(lsm6ds3_ctx_t *ctx,
                                            lsm6ds3_aux_sens_on_t val)
{
  lsm6ds3_slave0_config_t reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SLAVE0_CONFIG, (uint8_t*)&reg, 1);
  }
  if(ret == 0){
    reg.aux_sens_on = (uint8_t)val;
    ret = lsm6ds3_write_reg(ctx, LSM6DS3_SLAVE0_CONFIG, (uint8_t*)&reg, 1);
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @brief   sh_num_of_dev_connected: [get] Number of external sensors to
  *                                         be read by the sensor hub.
  *
  * @param  lsm6ds3_ctx_t *ctx: read / write interface definitions
  * @param  lsm6ds3_aux_sens_on_t: Get the values of aux_sens_on in
  *                                reg SLAVE0_CONFIG
  *
  */
int32_t lsm6ds3_sh_num_of_dev_connected_get(lsm6ds3_ctx_t *ctx,
                                            lsm6ds3_aux_sens_on_t *val)
{
  lsm6ds3_slave0_config_t reg;
  int32_t ret;

  ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_EMBEDDED_FUNC_BANK);
  if(ret == 0){
    ret = lsm6ds3_read_reg(ctx, LSM6DS3_SLAVE0_CONFIG, (uint8_t*)&reg, 1);
    switch (reg.aux_sens_on)
    {
      case LSM6DS3_SLV_0:
        *val = LSM6DS3_SLV_0;
        break;
      case LSM6DS3_SLV_0_1:
        *val = LSM6DS3_SLV_0_1;
        break;
      case LSM6DS3_SLV_0_1_2:
        *val = LSM6DS3_SLV_0_1_2;
        break;
        case LSM6DS3_SLV_0_1_2_3:
        *val = LSM6DS3_SLV_0_1_2_3;
        break;
      default:
        *val = LSM6DS3_SLV_0;
        break;
    }
  }
  if(ret == 0){
    ret = lsm6ds3_mem_bank_set(ctx, LSM6DS3_USER_BANK);
  }
  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
