/*
  ******************************************************************************
  * @file    lsm6dsox_reg.c
  * @author  Sensor Solutions Software Team
  * @brief   LSM6DSOX driver file
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

#include "lsm6dsox_reg.h"

/**
  * @defgroup  LSM6DSOX
  * @brief     This file provides a set of functions needed to drive the
  *            lsm6dsox enhanced inertial module.
  * @{
  *
*/

/**
  * @defgroup  LSM6DSOX_Interfaces_Functions
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
int32_t lsm6dsox_read_reg(lsm6dsox_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lsm6dsox_write_reg(lsm6dsox_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup  LSM6DSOX_Sensitivity
  * @brief     These functions convert raw-data into engineering units.
  * @{
  *
  */
float_t lsm6dsox_from_fs2_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.061f;
}

float_t lsm6dsox_from_fs4_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.122f;
}

float_t lsm6dsox_from_fs8_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.244f;
}

float_t lsm6dsox_from_fs16_to_mg(int16_t lsb)
{
  return ((float_t)lsb) *0.488f;
}

float_t lsm6dsox_from_fs125_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) *4.375f;
}

float_t lsm6dsox_from_fs500_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) *17.50f;
}

float_t lsm6dsox_from_fs250_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) *8.750f;
}

float_t lsm6dsox_from_fs1000_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) *35.0f;
}

float_t lsm6dsox_from_fs2000_to_mdps(int16_t lsb)
{
  return ((float_t)lsb) *70.0f;
}

float_t lsm6dsox_from_lsb_to_celsius(int16_t lsb)
{
  return (((float_t)lsb / 256.0f) + 25.0f);
}

float_t lsm6dsox_from_lsb_to_nsec(int16_t lsb)
{
  return ((float_t)lsb * 25000.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_Data_Generation
  * @brief     This section groups all the functions concerning
  *            data generation.
  *
  */

/**
  * @brief  Accelerometer full-scale selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fs_xl in reg CTRL1_XL
  *
  */
int32_t lsm6dsox_xl_full_scale_set(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_fs_xl_t val)
{
  lsm6dsox_ctrl1_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.fs_xl = (uint8_t) val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer full-scale selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fs_xl in reg CTRL1_XL
  *
  */
int32_t lsm6dsox_xl_full_scale_get(lsm6dsox_ctx_t *ctx, lsm6dsox_fs_xl_t *val)
{
  lsm6dsox_ctrl1_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);
  switch (reg.fs_xl) {
    case LSM6DSOX_2g:
      *val = LSM6DSOX_2g;
      break;
    case LSM6DSOX_16g:
      *val = LSM6DSOX_16g;
      break;
    case LSM6DSOX_4g:
      *val = LSM6DSOX_4g;
      break;
    case LSM6DSOX_8g:
      *val = LSM6DSOX_8g;
      break;
    default:
      *val = LSM6DSOX_2g;
      break;
  }

  return ret;
}

/**
  * @brief  Accelerometer UI data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odr_xl in reg CTRL1_XL
  *
  */
int32_t lsm6dsox_xl_data_rate_set(lsm6dsox_ctx_t *ctx, lsm6dsox_odr_xl_t val)
{
  lsm6dsox_ctrl1_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.odr_xl = (uint8_t) val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer UI data rate selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of odr_xl in reg CTRL1_XL
  *
  */
int32_t lsm6dsox_xl_data_rate_get(lsm6dsox_ctx_t *ctx, lsm6dsox_odr_xl_t *val)
{
  lsm6dsox_ctrl1_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);

  switch (reg.odr_xl) {
    case LSM6DSOX_XL_ODR_OFF:
      *val = LSM6DSOX_XL_ODR_OFF;
      break;
    case LSM6DSOX_XL_ODR_12Hz5:
      *val = LSM6DSOX_XL_ODR_12Hz5;
      break;
    case LSM6DSOX_XL_ODR_26Hz:
      *val = LSM6DSOX_XL_ODR_26Hz;
      break;
    case LSM6DSOX_XL_ODR_52Hz:
      *val = LSM6DSOX_XL_ODR_52Hz;
      break;
    case LSM6DSOX_XL_ODR_104Hz:
      *val = LSM6DSOX_XL_ODR_104Hz;
      break;
    case LSM6DSOX_XL_ODR_208Hz:
      *val = LSM6DSOX_XL_ODR_208Hz;
      break;
    case LSM6DSOX_XL_ODR_417Hz:
      *val = LSM6DSOX_XL_ODR_417Hz;
      break;
    case LSM6DSOX_XL_ODR_833Hz:
      *val = LSM6DSOX_XL_ODR_833Hz;
      break;
    case LSM6DSOX_XL_ODR_1667Hz:
      *val = LSM6DSOX_XL_ODR_1667Hz;
      break;
    case LSM6DSOX_XL_ODR_3333Hz:
      *val = LSM6DSOX_XL_ODR_3333Hz;
      break;
    case LSM6DSOX_XL_ODR_6667Hz:
      *val = LSM6DSOX_XL_ODR_6667Hz;
      break;
    case LSM6DSOX_XL_ODR_6Hz5:
      *val = LSM6DSOX_XL_ODR_6Hz5;
      break;
    default:
      *val = LSM6DSOX_XL_ODR_OFF;
      break;
  }
  return ret;
}

/**
  * @brief  Gyroscope UI chain full-scale selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fs_g in reg CTRL2_G
  *
  */
int32_t lsm6dsox_gy_full_scale_set(lsm6dsox_ctx_t *ctx, lsm6dsox_fs_g_t val)
{
  lsm6dsox_ctrl2_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL2_G, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.fs_g = (uint8_t) val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL2_G, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  Gyroscope UI chain full-scale selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fs_g in reg CTRL2_G
  *
  */
int32_t lsm6dsox_gy_full_scale_get(lsm6dsox_ctx_t *ctx, lsm6dsox_fs_g_t *val)
{
  lsm6dsox_ctrl2_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL2_G, (uint8_t*)&reg, 1);
  switch (reg.fs_g) {
    case LSM6DSOX_250dps:
      *val = LSM6DSOX_250dps;
      break;
    case LSM6DSOX_125dps:
      *val = LSM6DSOX_125dps;
      break;
    case LSM6DSOX_500dps:
      *val = LSM6DSOX_500dps;
      break;
    case LSM6DSOX_1000dps:
      *val = LSM6DSOX_1000dps;
      break;
    case LSM6DSOX_2000dps:
      *val = LSM6DSOX_2000dps;
      break;
    default:
      *val = LSM6DSOX_250dps;
      break;
  }

  return ret;
}

/**
  * @brief  Gyroscope UI data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odr_g in reg CTRL2_G
  *
  */
int32_t lsm6dsox_gy_data_rate_set(lsm6dsox_ctx_t *ctx, lsm6dsox_odr_g_t val)
{
  lsm6dsox_ctrl2_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL2_G, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.odr_g = (uint8_t) val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL2_G, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  Gyroscope UI data rate selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of odr_g in reg CTRL2_G
  *
  */
int32_t lsm6dsox_gy_data_rate_get(lsm6dsox_ctx_t *ctx, lsm6dsox_odr_g_t *val)
{
  lsm6dsox_ctrl2_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL2_G, (uint8_t*)&reg, 1);
  switch (reg.odr_g) {
    case LSM6DSOX_GY_ODR_OFF:
      *val = LSM6DSOX_GY_ODR_OFF;
      break;
    case LSM6DSOX_GY_ODR_12Hz5:
      *val = LSM6DSOX_GY_ODR_12Hz5;
      break;
    case LSM6DSOX_GY_ODR_26Hz:
      *val = LSM6DSOX_GY_ODR_26Hz;
      break;
    case LSM6DSOX_GY_ODR_52Hz:
      *val = LSM6DSOX_GY_ODR_52Hz;
      break;
    case LSM6DSOX_GY_ODR_104Hz:
      *val = LSM6DSOX_GY_ODR_104Hz;
      break;
    case LSM6DSOX_GY_ODR_208Hz:
      *val = LSM6DSOX_GY_ODR_208Hz;
      break;
    case LSM6DSOX_GY_ODR_417Hz:
      *val = LSM6DSOX_GY_ODR_417Hz;
      break;
    case LSM6DSOX_GY_ODR_833Hz:
      *val = LSM6DSOX_GY_ODR_833Hz;
      break;
    case LSM6DSOX_GY_ODR_1667Hz:
      *val = LSM6DSOX_GY_ODR_1667Hz;
      break;
    case LSM6DSOX_GY_ODR_3333Hz:
      *val = LSM6DSOX_GY_ODR_3333Hz;
      break;
    case LSM6DSOX_GY_ODR_6667Hz:
      *val = LSM6DSOX_GY_ODR_6667Hz;
      break;
    default:
      *val = LSM6DSOX_GY_ODR_OFF;
      break;
  }
  return ret;
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdu in reg CTRL3_C
  *
  */
int32_t lsm6dsox_block_data_update_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.bdu = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Block data update.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdu in reg CTRL3_C
  *
  */
int32_t lsm6dsox_block_data_update_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  *val = reg.bdu;

  return ret;
}

/**
  * @brief  Weight of XL user offset bits of registers X_OFS_USR (73h),
  *         Y_OFS_USR (74h), Z_OFS_USR (75h).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of usr_off_w in reg CTRL6_C
  *
  */
int32_t lsm6dsox_xl_offset_weight_set(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_usr_off_w_t val)
{
  lsm6dsox_ctrl6_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.usr_off_w = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief    Weight of XL user offset bits of registers X_OFS_USR (73h),
  *           Y_OFS_USR (74h), Z_OFS_USR (75h).[get]
  *
  * @param    ctx      read / write interface definitions
  * @param    val      Get the values of usr_off_w in reg CTRL6_C
  *
  */
int32_t lsm6dsox_xl_offset_weight_get(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_usr_off_w_t *val)
{
  lsm6dsox_ctrl6_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);

  switch (reg.usr_off_w) {
    case LSM6DSOX_LSb_1mg:
      *val = LSM6DSOX_LSb_1mg;
      break;
    case LSM6DSOX_LSb_16mg:
      *val = LSM6DSOX_LSb_16mg;
      break;
    default:
      *val = LSM6DSOX_LSb_1mg;
      break;
  }
  return ret;
}

/**
  * @brief  Accelerometer power mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of xl_hm_mode in
  *                               reg CTRL6_C
  *
  */
int32_t lsm6dsox_xl_power_mode_set(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_xl_hm_mode_t val)
{
  lsm6dsox_ctrl5_c_t ctrl5_c;
  lsm6dsox_ctrl6_c_t ctrl6_c;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*) &ctrl5_c, 1);
  if (ret == 0) {
    ctrl5_c.xl_ulp_en = ((uint8_t)val & 0x02U) >> 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*) &ctrl5_c, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*) &ctrl6_c, 1);
  }
  if (ret == 0) {
    ctrl6_c.xl_hm_mode = (uint8_t)val & 0x01U;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*) &ctrl6_c, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer power mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of xl_hm_mode in reg CTRL6_C
  *
  */
int32_t lsm6dsox_xl_power_mode_get(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_xl_hm_mode_t *val)
{
  lsm6dsox_ctrl5_c_t ctrl5_c;
  lsm6dsox_ctrl6_c_t ctrl6_c;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*) &ctrl5_c, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*) &ctrl6_c, 1);
    switch ( (ctrl5_c.xl_ulp_en << 1) | ctrl6_c.xl_hm_mode) {
      case LSM6DSOX_HIGH_PERFORMANCE_MD:
        *val = LSM6DSOX_HIGH_PERFORMANCE_MD;
        break;
      case LSM6DSOX_LOW_NORMAL_POWER_MD:
        *val = LSM6DSOX_LOW_NORMAL_POWER_MD;
        break;
      case LSM6DSOX_ULTRA_LOW_POWER_MD:
        *val = LSM6DSOX_ULTRA_LOW_POWER_MD;
        break;
      default:
        *val = LSM6DSOX_HIGH_PERFORMANCE_MD;
        break;
    }
  }
  return ret;
}

/**
  * @brief  Operating mode for gyroscope.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of g_hm_mode in reg CTRL7_G
  *
  */
int32_t lsm6dsox_gy_power_mode_set(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_g_hm_mode_t val)
{
  lsm6dsox_ctrl7_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.g_hm_mode = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Operating mode for gyroscope.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of g_hm_mode in reg CTRL7_G
  *
  */
int32_t lsm6dsox_gy_power_mode_get(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_g_hm_mode_t *val)
{
  lsm6dsox_ctrl7_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  switch (reg.g_hm_mode) {
    case LSM6DSOX_GY_HIGH_PERFORMANCE:
      *val = LSM6DSOX_GY_HIGH_PERFORMANCE;
      break;
    case LSM6DSOX_GY_NORMAL:
      *val = LSM6DSOX_GY_NORMAL;
      break;
    default:
      *val = LSM6DSOX_GY_HIGH_PERFORMANCE;
      break;
  }
  return ret;
}

/**
  * @brief  Read all the interrupt flag of the device.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers ALL_INT_SRC; WAKE_UP_SRC;
  *                              TAP_SRC; D6D_SRC; STATUS_REG;
  *                              EMB_FUNC_STATUS; FSM_STATUS_A/B
  *
  */
int32_t lsm6dsox_all_sources_get(lsm6dsox_ctx_t *ctx,
                                lsm6dsox_all_sources_t *val)
{
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_ALL_INT_SRC,
                         (uint8_t*)&val->all_int_src, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_SRC,
                           (uint8_t*)&val->wake_up_src, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_SRC,
                           (uint8_t*)&val->tap_src, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_D6D_SRC,
                           (uint8_t*)&val->d6d_src, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_STATUS_REG,
                           (uint8_t*)&val->status_reg, 1);
  }
  if (ret == 0) {

    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_STATUS,
                           (uint8_t*)&val->emb_func_status, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_STATUS_A,
                           (uint8_t*)&val->fsm_status_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_STATUS_B,
                           (uint8_t*)&val->fsm_status_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  The STATUS_REG register is read by the primary interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      register STATUS_REG
  *
  */
int32_t lsm6dsox_status_reg_get(lsm6dsox_ctx_t *ctx, lsm6dsox_status_reg_t *val)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_STATUS_REG, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Accelerometer new data available.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of xlda in reg STATUS_REG
  *
  */
int32_t lsm6dsox_xl_flag_data_ready_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_status_reg_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_STATUS_REG, (uint8_t*)&reg, 1);
  *val = reg.xlda;

  return ret;
}

/**
  * @brief  Gyroscope new data available.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of gda in reg STATUS_REG
  *
  */
int32_t lsm6dsox_gy_flag_data_ready_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_status_reg_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_STATUS_REG, (uint8_t*)&reg, 1);
  *val = reg.gda;

  return ret;
}

/**
  * @brief  Temperature new data available.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tda in reg STATUS_REG
  *
  */
int32_t lsm6dsox_temp_flag_data_ready_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_status_reg_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_STATUS_REG, (uint8_t*)&reg, 1);
  *val = reg.tda;

  return ret;
}

/**
  * @brief  Accelerometer X-axis user offset correction expressed in
  *         two’s complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_xl_usr_offset_x_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_write_reg(ctx, LSM6DSOX_X_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer X-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_xl_usr_offset_x_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_X_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Y-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_xl_usr_offset_y_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_write_reg(ctx, LSM6DSOX_Y_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Y-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_xl_usr_offset_y_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_Y_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Z-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_xl_usr_offset_z_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_write_reg(ctx, LSM6DSOX_Z_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Z-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_xl_usr_offset_z_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_Z_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Enables user offset on out.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of usr_off_on_out in reg CTRL7_G
  *
  */
int32_t lsm6dsox_xl_usr_offset_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl7_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.usr_off_on_out = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  User offset on out flag.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      values of usr_off_on_out in reg CTRL7_G
  *
  */
int32_t lsm6dsox_xl_usr_offset_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl7_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  *val = reg.usr_off_on_out;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_Timestamp
  * @brief     This section groups all the functions that manage the
  *            timestamp generation.
  * @{
  *
  */

/**
  * @brief  Enables timestamp counter.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of timestamp_en in reg CTRL10_C
  *
  */
int32_t lsm6dsox_timestamp_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl10_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL10_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.timestamp_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL10_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables timestamp counter.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of timestamp_en in reg CTRL10_C
  *
  */
int32_t lsm6dsox_timestamp_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl10_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL10_C, (uint8_t*)&reg, 1);
  *val = reg.timestamp_en;

  return ret;
}

/**
  * @brief  Timestamp first data output register (r).
  *         The value is expressed as a 32-bit word and the bit
  *         resolution is 25 μs.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_timestamp_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TIMESTAMP0, buff, 4);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_Data output
  * @brief     This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief  Circular burst-mode (rounding) read of the output
  *         registers.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of rounding in reg CTRL5_C
  *
  */
int32_t lsm6dsox_rounding_mode_set(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_rounding_t val)
{
  lsm6dsox_ctrl5_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.rounding = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope UI chain full-scale selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of rounding in reg CTRL5_C
  *
  */
int32_t lsm6dsox_rounding_mode_get(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_rounding_t *val)
{
  lsm6dsox_ctrl5_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  switch (reg.rounding) {
    case LSM6DSOX_NO_ROUND:
      *val = LSM6DSOX_NO_ROUND;
      break;
    case LSM6DSOX_ROUND_XL:
      *val = LSM6DSOX_ROUND_XL;
      break;
    case LSM6DSOX_ROUND_GY:
      *val = LSM6DSOX_ROUND_GY;
      break;
    case LSM6DSOX_ROUND_GY_XL:
      *val = LSM6DSOX_ROUND_GY_XL;
      break;
    default:
      *val = LSM6DSOX_NO_ROUND;
      break;
  }
  return ret;
}

/**
  * @brief  rounding_on_status: [set] Source register rounding function in
  *                                   ALL_INT_SRC (1Ah), WAKE_UP_SRC(1Bh),
  *                                   TAP_SRC (1Ch), D6D_SRC (1Dh),
  *                                   STATUS_REG (1Eh) and
  *                                   EMB_FUNC_STATUS_MAINPAGE(35h),
  *                                   FSM_STATUS_A_MAINPAGE (36h),
  *                                   FSM_STATUS_B_MAINPAGE (37h),
  *                                   PROGSENS_STATUS_MAINPAGE (38h),
  *                                   STATUS_MASTER_MAINPAGE (39h),
  *                                   FIFO_STATUS1 (3Ah), FIFO_STATUS2(3Bh).
  *
  * @param  ctx      read / write interface definitions
  * @param  lsm6dsox_rounding_status_t: change the values of rounding_status
  *                                    in reg CTRL7_G
  *
  */
int32_t lsm6dsox_rounding_on_status_set(lsm6dsox_ctx_t *ctx,
                                       lsm6dsox_rounding_status_t val)
{
  lsm6dsox_ctrl5_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.rounding_status = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  rounding_on_status: [get] Source register rounding function in
  *                                   ALL_INT_SRC (1Ah), WAKE_UP_SRC(1Bh),
  *                                   TAP_SRC (1Ch), D6D_SRC (1Dh),
  *                                   STATUS_REG (1Eh) and
  *                                   EMB_FUNC_STATUS_MAINPAGE(35h),
  *                                   FSM_STATUS_A_MAINPAGE (36h),
  *                                   FSM_STATUS_B_MAINPAGE (37h),
  *                                   PROGSENS_STATUS_MAINPAGE (38h),
  *                                   STATUS_MASTER_MAINPAGE (39h),
  *                                   FIFO_STATUS1 (3Ah), FIFO_STATUS2(3Bh).
  *
  * @param  ctx      read / write interface definitions
  * @param  lsm6dsox_rounding_status_t: Get the values of rounding_status
  *                                    in reg CTRL7_G
  *
  */
int32_t lsm6dsox_rounding_on_status_get(lsm6dsox_ctx_t *ctx,
                                       lsm6dsox_rounding_status_t *val)
{
  lsm6dsox_ctrl5_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  switch (reg.rounding_status) {
    case LSM6DSOX_STAT_RND_DISABLE:
      *val = LSM6DSOX_STAT_RND_DISABLE;
      break;
    case LSM6DSOX_STAT_RND_ENABLE:
      *val = LSM6DSOX_STAT_RND_ENABLE;
      break;
    default:
      *val = LSM6DSOX_STAT_RND_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Temperature data output register (r).
  *         L and H registers together express a 16-bit word in two’s
  *         complement.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_temperature_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_OUT_TEMP_L, buff, 2);
  return ret;
}

/**
  * @brief  Angular rate sensor. The value is expressed as a 16-bit
  *         word in two’s complement.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_angular_rate_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_OUTX_L_G, buff, 6);
  return ret;
}

/**
  * @brief  Linear acceleration output register.
  *         The value is expressed as a 16-bit word in two’s complement.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_acceleration_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_OUTX_L_A, buff, 6);
  return ret;
}

/**
  * @brief  FIFO data output [get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_fifo_out_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_DATA_OUT_X_L, buff, 6);
  return ret;
}

/**
  * @brief  ois_angular_rate_raw: [get]  OIS angular rate sensor.
  *                                      The value is expressed as a
  *                                      16-bit word in two’s complement.
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm6dsox_ois_angular_rate_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  return lsm6dsox_read_reg(ctx, LSM6DSOX_UI_OUTX_L_G_OIS, buff, 6);
}

/**
  * @brief  ois_acceleration_raw: [get] OIS Linear acceleration output register.
  *                                     The value is expressed as a
  *                                     16-bit word in two’s complement.
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm6dsox_ois_acceleration_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  return lsm6dsox_read_reg(ctx, LSM6DSOX_UI_OUTX_L_A_OIS, buff, 6);
}

/**
  * @brief  aux_temperature_raw: [get]  Temperature from auxiliary
  *                                     interface.
  *                                     The value is expressed as a
  *                                     16-bit word in two’s complement.
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm6dsox_aux_temperature_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  return lsm6dsox_read_reg(ctx, LSM6DSOX_SPI2_OUT_TEMP_L, buff, 2);
}

/**
  * @brief  aux_ois_angular_rate_raw: [get] OIS angular rate sensor from
  *                                         auxiliary interface.
  *                                         The value is expressed as a
  *                                         16-bit word in two’s complement.
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm6dsox_aux_ois_angular_rate_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  return lsm6dsox_read_reg(ctx, LSM6DSOX_SPI2_OUTX_L_G_OIS, buff, 6);
}

/**
  * @brief  aux_ois_acceleration_raw: [get] OIS linear acceleration output
  *                                         register from auxiliary interface.
  *                                         The value is expressed as a
  *                                         16-bit word in two’s complement.
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm6dsox_aux_ois_acceleration_raw_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  return lsm6dsox_read_reg(ctx, LSM6DSOX_SPI2_OUTX_L_A_OIS, buff, 6);
}

/**
  * @brief  Step counter output register.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_number_of_steps_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_STEP_COUNTER_L, buff, 2);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Reset step counter register.[get]
  *
  * @param  ctx      read / write interface definitions
  *
  */
int32_t lsm6dsox_steps_reset(lsm6dsox_ctx_t *ctx)
{
  lsm6dsox_emb_func_src_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_SRC, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.pedo_rst_step = PROPERTY_ENABLE;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_SRC, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  prgsens_out: [get] Output value of all PROGSENSx decision trees.
  *
  * @param  ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm6dsox_mlc_out_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MLC0_SRC, buff, 8);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_common
  * @brief   This section groups common usefull functions.
  * @{
  *
  */

/**
  * @brief  Difference in percentage of the effective ODR(and timestamp rate)
  *         with respect to the typical.
  *         Step:  0.15%. 8-bit format, 2's complement.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of freq_fine in reg
  *                      INTERNAL_FREQ_FINE
  *
  */
int32_t lsm6dsox_odr_cal_reg_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_internal_freq_fine_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INTERNAL_FREQ_FINE, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.freq_fine = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_INTERNAL_FREQ_FINE,
                            (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Difference in percentage of the effective ODR(and timestamp rate)
  *         with respect to the typical.
  *         Step:  0.15%. 8-bit format, 2's complement.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of freq_fine in reg INTERNAL_FREQ_FINE
  *
  */
int32_t lsm6dsox_odr_cal_reg_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_internal_freq_fine_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INTERNAL_FREQ_FINE, (uint8_t*)&reg, 1);
  *val = reg.freq_fine;

  return ret;
}


/**
  * @brief  Enable access to the embedded functions/sensor
  *         hub configuration registers.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reg_access in
  *                               reg FUNC_CFG_ACCESS
  *
  */
int32_t lsm6dsox_mem_bank_set(lsm6dsox_ctx_t *ctx, lsm6dsox_reg_access_t val)
{
  lsm6dsox_func_cfg_access_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FUNC_CFG_ACCESS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.reg_access = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FUNC_CFG_ACCESS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable access to the embedded functions/sensor
  *         hub configuration registers.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of reg_access in
  *                               reg FUNC_CFG_ACCESS
  *
  */
int32_t lsm6dsox_mem_bank_get(lsm6dsox_ctx_t *ctx, lsm6dsox_reg_access_t *val)
{
  lsm6dsox_func_cfg_access_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FUNC_CFG_ACCESS, (uint8_t*)&reg, 1);
  switch (reg.reg_access) {
    case LSM6DSOX_USER_BANK:
      *val = LSM6DSOX_USER_BANK;
      break;
    case LSM6DSOX_SENSOR_HUB_BANK:
      *val = LSM6DSOX_SENSOR_HUB_BANK;
      break;
    case LSM6DSOX_EMBEDDED_FUNC_BANK:
      *val = LSM6DSOX_EMBEDDED_FUNC_BANK;
      break;
    default:
      *val = LSM6DSOX_USER_BANK;
      break;
  }
  return ret;
}

/**
  * @brief  Write a line(byte) in a page.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t address: page line address
  * @param  val      value to write
  *
  */
int32_t lsm6dsox_ln_pg_write_byte(lsm6dsox_ctx_t *ctx, uint16_t address,
                                 uint8_t *val)
{
  lsm6dsox_page_rw_t page_rw;
  lsm6dsox_page_sel_t page_sel;
  lsm6dsox_page_address_t page_address;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);

  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    page_rw.page_rw = 0x02; /* page_write enable */
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_SEL, (uint8_t*) &page_sel, 1);
  }

  if (ret == 0) {
    page_sel.page_sel = (((uint8_t)address >> 8) & 0x0FU);
    page_sel.not_used_01 = 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_SEL, (uint8_t*) &page_sel, 1);
  }
  if (ret == 0) {
    page_address.page_addr = (uint8_t)address & 0xFFU;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_ADDRESS,
                            (uint8_t*)&page_address, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_VALUE, val, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    page_rw.page_rw = 0x00; /* page_write disable */
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {

    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Write buffer in a page.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t address: page line address
  * @param  uint8_t *buf: buffer to write
  * @param  uint8_t len: buffer len
  *
  */
int32_t lsm6dsox_ln_pg_write(lsm6dsox_ctx_t *ctx, uint16_t address,
                            uint8_t *buf, uint8_t len)
{
  lsm6dsox_page_rw_t page_rw;
  lsm6dsox_page_sel_t page_sel;
  lsm6dsox_page_address_t  page_address;
  int32_t ret;
  uint8_t msb, lsb;
  uint8_t i ;

  msb = (((uint8_t)address >> 8) & 0x0fU);
  lsb = (uint8_t)address & 0xFFU;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    page_rw.page_rw = 0x02; /* page_write enable*/
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_SEL, (uint8_t*) &page_sel, 1);
  }
  if (ret == 0) {
    page_sel.page_sel = msb;
    page_sel.not_used_01 = 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_SEL, (uint8_t*) &page_sel, 1);
  }
  if (ret == 0) {
    page_address.page_addr = lsb;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_ADDRESS,
                            (uint8_t*)&page_address, 1);
  }

  if (ret == 0) {

    for (i = 0; ( (i < len) && (ret == 0) ); i++)
    {
      ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_VALUE, &buf[i], 1);

      /* Check if page wrap */
      if ( (lsb == 0x00U) && (ret == 0) ) {
        lsb++;
        msb++;
        ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_SEL, (uint8_t*)&page_sel, 1);
        if (ret == 0) {
          page_sel.page_sel = msb;
          page_sel.not_used_01 = 1;
          ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_SEL,
                                  (uint8_t*)&page_sel, 1);
        }
      }
    }
    page_sel.page_sel = 0;
    page_sel.not_used_01 = 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_SEL, (uint8_t*) &page_sel, 1);
  }
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    page_rw.page_rw = 0x00; /* page_write disable */
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }

  if (ret == 0) {

    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Read a line(byte) in a page.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t address: page line address
  * @param  val      read value
  *
  */
int32_t lsm6dsox_ln_pg_read_byte(lsm6dsox_ctx_t *ctx, uint16_t address,
                                uint8_t *val)
{
  lsm6dsox_page_rw_t page_rw;
  lsm6dsox_page_sel_t page_sel;
  lsm6dsox_page_address_t  page_address;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    page_rw.page_rw = 0x01; /* page_read enable*/
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_SEL, (uint8_t*) &page_sel, 1);
  }
  if (ret == 0) {
    page_sel.page_sel = (((uint8_t)address >> 8) & 0x0FU);
    page_sel.not_used_01 = 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_SEL, (uint8_t*) &page_sel, 1);
  }
  if (ret == 0) {
    page_address.page_addr = (uint8_t)address & 0x00FFU;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_ADDRESS,
                            (uint8_t*)&page_address, 1);
  }
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_VALUE, val, 2);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    page_rw.page_rw = 0x00; /* page_read disable */
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Data-ready pulsed / letched mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of
  *                                     dataready_pulsed in
  *                                     reg COUNTER_BDR_REG1
  *
  */
int32_t lsm6dsox_data_ready_mode_set(lsm6dsox_ctx_t *ctx,
                                    lsm6dsox_dataready_pulsed_t val)
{
  lsm6dsox_counter_bdr_reg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.dataready_pulsed = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Data-ready pulsed / letched mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of
  *                                     dataready_pulsed in
  *                                     reg COUNTER_BDR_REG1
  *
  */
int32_t lsm6dsox_data_ready_mode_get(lsm6dsox_ctx_t *ctx,
                                    lsm6dsox_dataready_pulsed_t *val)
{
  lsm6dsox_counter_bdr_reg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  switch (reg.dataready_pulsed) {
    case LSM6DSOX_DRDY_LATCHED:
      *val = LSM6DSOX_DRDY_LATCHED;
      break;
    case LSM6DSOX_DRDY_PULSED:
      *val = LSM6DSOX_DRDY_PULSED;
      break;
    default:
      *val = LSM6DSOX_DRDY_LATCHED;
      break;
  }
  return ret;
}

/**
  * @brief  Device "Who am I".[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_device_id_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief  Software reset. Restore the default values
  *         in user registers[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sw_reset in reg CTRL3_C
  *
  */
int32_t lsm6dsox_reset_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.sw_reset = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sw_reset in reg CTRL3_C
  *
  */
int32_t lsm6dsox_reset_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  *val = reg.sw_reset;

  return ret;
}

/**
  * @brief  Register address automatically incremented during a multiple byte
  *         access with a serial interface.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of if_inc in reg CTRL3_C
  *
  */
int32_t lsm6dsox_auto_increment_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.if_inc = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Register address automatically incremented during a multiple byte
  *         access with a serial interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of if_inc in reg CTRL3_C
  *
  */
int32_t lsm6dsox_auto_increment_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  *val = reg.if_inc;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of boot in reg CTRL3_C
  *
  */
int32_t lsm6dsox_boot_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.boot = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of boot in reg CTRL3_C
  *
  */
int32_t lsm6dsox_boot_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  *val = reg.boot;

  return ret;
}

/**
  * @brief  Linear acceleration sensor self-test enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of st_xl in reg CTRL5_C
  *
  */
int32_t lsm6dsox_xl_self_test_set(lsm6dsox_ctx_t *ctx, lsm6dsox_st_xl_t val)
{
  lsm6dsox_ctrl5_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.st_xl = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Linear acceleration sensor self-test enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of st_xl in reg CTRL5_C
  *
  */
int32_t lsm6dsox_xl_self_test_get(lsm6dsox_ctx_t *ctx, lsm6dsox_st_xl_t *val)
{
  lsm6dsox_ctrl5_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  switch (reg.st_xl) {
    case LSM6DSOX_XL_ST_DISABLE:
      *val = LSM6DSOX_XL_ST_DISABLE;
      break;
    case LSM6DSOX_XL_ST_POSITIVE:
      *val = LSM6DSOX_XL_ST_POSITIVE;
      break;
    case LSM6DSOX_XL_ST_NEGATIVE:
      *val = LSM6DSOX_XL_ST_NEGATIVE;
      break;
    default:
      *val = LSM6DSOX_XL_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Angular rate sensor self-test enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of st_g in reg CTRL5_C
  *
  */
int32_t lsm6dsox_gy_self_test_set(lsm6dsox_ctx_t *ctx, lsm6dsox_st_g_t val)
{
  lsm6dsox_ctrl5_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.st_g = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Angular rate sensor self-test enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of st_g in reg CTRL5_C
  *
  */
int32_t lsm6dsox_gy_self_test_get(lsm6dsox_ctx_t *ctx, lsm6dsox_st_g_t *val)
{
  lsm6dsox_ctrl5_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL5_C, (uint8_t*)&reg, 1);
  switch (reg.st_g) {
    case LSM6DSOX_GY_ST_DISABLE:
      *val = LSM6DSOX_GY_ST_DISABLE;
      break;
    case LSM6DSOX_GY_ST_POSITIVE:
      *val = LSM6DSOX_GY_ST_POSITIVE;
      break;
    case LSM6DSOX_GY_ST_NEGATIVE:
      *val = LSM6DSOX_GY_ST_NEGATIVE;
      break;
    default:
      *val = LSM6DSOX_GY_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_filters
  * @brief     This section group all the functions concerning the
  *            filters configuration
  * @{
  *
  */

/**
  * @brief  Accelerometer output from LPF2 filtering stage selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lpf2_xl_en in reg CTRL1_XL
  *
  */
int32_t lsm6dsox_xl_filter_lp2_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl1_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.lpf2_xl_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer output from LPF2 filtering stage selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lpf2_xl_en in reg CTRL1_XL
  *
  */
int32_t lsm6dsox_xl_filter_lp2_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl1_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL1_XL, (uint8_t*)&reg, 1);
  *val = reg.lpf2_xl_en;

  return ret;
}

/**
  * @brief  Enables gyroscope digital LPF1 if auxiliary SPI is disabled;
  *         the bandwidth can be selected through FTYPE [2:0]
  *         in CTRL6_C (15h).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lpf1_sel_g in reg CTRL4_C
  *
  */
int32_t lsm6dsox_gy_filter_lp1_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.lpf1_sel_g = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables gyroscope digital LPF1 if auxiliary SPI is disabled;
  *         the bandwidth can be selected through FTYPE [2:0]
  *         in CTRL6_C (15h).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lpf1_sel_g in reg CTRL4_C
  *
  */
int32_t lsm6dsox_gy_filter_lp1_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  *val = reg.lpf1_sel_g;

  return ret;
}

/**
  * @brief  Mask DRDY on pin (both XL & Gyro) until filter settling ends
  *         (XL and Gyro independently masked).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of drdy_mask in reg CTRL4_C
  *
  */
int32_t lsm6dsox_filter_settling_mask_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.drdy_mask = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Mask DRDY on pin (both XL & Gyro) until filter settling ends
  *         (XL and Gyro independently masked).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of drdy_mask in reg CTRL4_C
  *
  */
int32_t lsm6dsox_filter_settling_mask_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  *val = reg.drdy_mask;

  return ret;
}

/**
  * @brief  Gyroscope lp1 bandwidth.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ftype in reg CTRL6_C
  *
  */
int32_t lsm6dsox_gy_lp1_bandwidth_set(lsm6dsox_ctx_t *ctx, lsm6dsox_ftype_t val)
{
  lsm6dsox_ctrl6_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.ftype = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope lp1 bandwidth.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val       Get the values of ftype in reg CTRL6_C
  *
  */
int32_t lsm6dsox_gy_lp1_bandwidth_get(lsm6dsox_ctx_t *ctx, lsm6dsox_ftype_t *val)
{
  lsm6dsox_ctrl6_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);
  switch (reg.ftype) {
    case LSM6DSOX_ULTRA_LIGHT:
      *val = LSM6DSOX_ULTRA_LIGHT;
      break;
    case LSM6DSOX_VERY_LIGHT:
      *val = LSM6DSOX_VERY_LIGHT;
      break;
    case LSM6DSOX_LIGHT:
      *val = LSM6DSOX_LIGHT;
      break;
    case LSM6DSOX_MEDIUM:
      *val = LSM6DSOX_MEDIUM;
      break;
    case LSM6DSOX_STRONG:
      *val = LSM6DSOX_STRONG;
      break;
    case LSM6DSOX_VERY_STRONG:
      *val = LSM6DSOX_VERY_STRONG;
      break;
    case LSM6DSOX_AGGRESSIVE:
      *val = LSM6DSOX_AGGRESSIVE;
      break;
    case LSM6DSOX_XTREME:
      *val = LSM6DSOX_XTREME;
      break;
    default:
      *val = LSM6DSOX_ULTRA_LIGHT;
      break;
  }
  return ret;
}

/**
  * @brief  Low pass filter 2 on 6D function selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of low_pass_on_6d in reg CTRL8_XL
  *
  */
int32_t lsm6dsox_xl_lp2_on_6d_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl8_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.low_pass_on_6d = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Low pass filter 2 on 6D function selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of low_pass_on_6d in reg CTRL8_XL
  *
  */
int32_t lsm6dsox_xl_lp2_on_6d_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl8_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  *val = reg.low_pass_on_6d;

  return ret;
}

/**
  * @brief  Accelerometer slope filter / high-pass filter selection
  *         on output.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of hp_slope_xl_en
  *                                   in reg CTRL8_XL
  *
  */
int32_t lsm6dsox_xl_hp_path_on_out_set(lsm6dsox_ctx_t *ctx,
                                      lsm6dsox_hp_slope_xl_en_t val)
{
  lsm6dsox_ctrl8_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.hp_slope_xl_en = ((uint8_t)val & 0x10U) >> 4;
    reg.hp_ref_mode_xl = ((uint8_t)val & 0x20U) >> 5;
    reg.hpcf_xl = (uint8_t)val & 0x07U;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer slope filter / high-pass filter selection
  *         on output.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of hp_slope_xl_en
  *                                   in reg CTRL8_XL
  *
  */
int32_t lsm6dsox_xl_hp_path_on_out_get(lsm6dsox_ctx_t *ctx,
                                      lsm6dsox_hp_slope_xl_en_t *val)
{
  lsm6dsox_ctrl8_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  switch ((reg.hp_ref_mode_xl << 5) | (reg.hp_slope_xl_en << 4) |
          reg.hpcf_xl) {
    case LSM6DSOX_HP_PATH_DISABLE_ON_OUT:
      *val = LSM6DSOX_HP_PATH_DISABLE_ON_OUT;
      break;
    case LSM6DSOX_SLOPE_ODR_DIV_4:
      *val = LSM6DSOX_SLOPE_ODR_DIV_4;
      break;
    case LSM6DSOX_HP_ODR_DIV_10:
      *val = LSM6DSOX_HP_ODR_DIV_10;
      break;
    case LSM6DSOX_HP_ODR_DIV_20:
      *val = LSM6DSOX_HP_ODR_DIV_20;
      break;
    case LSM6DSOX_HP_ODR_DIV_45:
      *val = LSM6DSOX_HP_ODR_DIV_45;
      break;
    case LSM6DSOX_HP_ODR_DIV_100:
      *val = LSM6DSOX_HP_ODR_DIV_100;
      break;
    case LSM6DSOX_HP_ODR_DIV_200:
      *val = LSM6DSOX_HP_ODR_DIV_200;
      break;
    case LSM6DSOX_HP_ODR_DIV_400:
      *val = LSM6DSOX_HP_ODR_DIV_400;
      break;
    case LSM6DSOX_HP_ODR_DIV_800:
      *val = LSM6DSOX_HP_ODR_DIV_800;
      break;
    case LSM6DSOX_HP_REF_MD_ODR_DIV_10:
      *val = LSM6DSOX_HP_REF_MD_ODR_DIV_10;
      break;
    case LSM6DSOX_HP_REF_MD_ODR_DIV_20:
      *val = LSM6DSOX_HP_REF_MD_ODR_DIV_20;
      break;
    case LSM6DSOX_HP_REF_MD_ODR_DIV_45:
      *val = LSM6DSOX_HP_REF_MD_ODR_DIV_45;
      break;
    case LSM6DSOX_HP_REF_MD_ODR_DIV_100:
      *val = LSM6DSOX_HP_REF_MD_ODR_DIV_100;
      break;
    case LSM6DSOX_HP_REF_MD_ODR_DIV_200:
      *val = LSM6DSOX_HP_REF_MD_ODR_DIV_200;
      break;
    case LSM6DSOX_HP_REF_MD_ODR_DIV_400:
      *val = LSM6DSOX_HP_REF_MD_ODR_DIV_400;
      break;
    case LSM6DSOX_HP_REF_MD_ODR_DIV_800:
      *val = LSM6DSOX_HP_REF_MD_ODR_DIV_800;
      break;
    case LSM6DSOX_LP_ODR_DIV_10:
      *val = LSM6DSOX_LP_ODR_DIV_10;
      break;
    case LSM6DSOX_LP_ODR_DIV_20:
      *val = LSM6DSOX_LP_ODR_DIV_20;
      break;
    case LSM6DSOX_LP_ODR_DIV_45:
      *val = LSM6DSOX_LP_ODR_DIV_45;
      break;
    case LSM6DSOX_LP_ODR_DIV_100:
      *val = LSM6DSOX_LP_ODR_DIV_100;
      break;
    case LSM6DSOX_LP_ODR_DIV_200:
      *val = LSM6DSOX_LP_ODR_DIV_200;
      break;
    case LSM6DSOX_LP_ODR_DIV_400:
      *val = LSM6DSOX_LP_ODR_DIV_400;
      break;
    case LSM6DSOX_LP_ODR_DIV_800:
      *val = LSM6DSOX_LP_ODR_DIV_800;
      break;
    default:
      *val = LSM6DSOX_HP_PATH_DISABLE_ON_OUT;
      break;
  }

  return ret;
}

/**
  * @brief  Enables accelerometer LPF2 and HPF fast-settling mode.
  *         The filter sets the second samples after writing this bit.
  *         Active only during device exit from power-down mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fastsettl_mode_xl in
  *                  reg CTRL8_XL
  *
  */
int32_t lsm6dsox_xl_fast_settling_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl8_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.fastsettl_mode_xl = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables accelerometer LPF2 and HPF fast-settling mode.
  *         The filter sets the second samples after writing this bit.
  *         Active only during device exit from power-down mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fastsettl_mode_xl in reg CTRL8_XL
  *
  */
int32_t lsm6dsox_xl_fast_settling_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl8_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  *val = reg.fastsettl_mode_xl;

  return ret;
}

/**
  * @brief  HPF or SLOPE filter selection on wake-up and Activity/Inactivity
  *         functions.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of slope_fds in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_xl_hp_path_internal_set(lsm6dsox_ctx_t *ctx,
                                        lsm6dsox_slope_fds_t val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.slope_fds = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  HPF or SLOPE filter selection on wake-up and Activity/Inactivity
  *         functions.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Change the values of slope_fds in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_xl_hp_path_internal_get(lsm6dsox_ctx_t *ctx,
                                        lsm6dsox_slope_fds_t *val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  switch (reg.slope_fds) {
    case LSM6DSOX_USE_SLOPE:
      *val = LSM6DSOX_USE_SLOPE;
      break;
    case LSM6DSOX_USE_HPF:
      *val = LSM6DSOX_USE_HPF;
      break;
    default:
      *val = LSM6DSOX_USE_SLOPE;
      break;
  }
  return ret;
}

/**
  * @brief  Enables gyroscope digital high-pass filter. The filter is
  *         enabled only if the gyro is in HP mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of hp_en_g and hp_en_g
  *                            in reg CTRL7_G
  *
  */
int32_t lsm6dsox_gy_hp_path_internal_set(lsm6dsox_ctx_t *ctx,
                                        lsm6dsox_hpm_g_t val)
{
  lsm6dsox_ctrl7_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.hp_en_g = ((uint8_t)val & 0x80U) >> 7;
    reg.hpm_g = (uint8_t)val & 0x03U;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables gyroscope digital high-pass filter. The filter is
  *         enabled only if the gyro is in HP mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of hp_en_g and hp_en_g
  *                            in reg CTRL7_G
  *
  */
int32_t lsm6dsox_gy_hp_path_internal_get(lsm6dsox_ctx_t *ctx,
                                        lsm6dsox_hpm_g_t *val)
{
  lsm6dsox_ctrl7_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  switch ((reg.hp_en_g << 7) + reg.hpm_g) {
    case LSM6DSOX_HP_FILTER_NONE:
      *val = LSM6DSOX_HP_FILTER_NONE;
      break;
    case LSM6DSOX_HP_FILTER_16mHz:
      *val = LSM6DSOX_HP_FILTER_16mHz;
      break;
    case LSM6DSOX_HP_FILTER_65mHz:
      *val = LSM6DSOX_HP_FILTER_65mHz;
      break;
    case LSM6DSOX_HP_FILTER_260mHz:
      *val = LSM6DSOX_HP_FILTER_260mHz;
      break;
    case LSM6DSOX_HP_FILTER_1Hz04:
      *val = LSM6DSOX_HP_FILTER_1Hz04;
      break;
    default:
      *val = LSM6DSOX_HP_FILTER_NONE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_ Auxiliary_interface
  * @brief     This section groups all the functions concerning
  *            auxiliary interface.
  * @{
  *
  */

/**
  * @brief   OIS data reading from Auxiliary / Main SPI.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of spi2_read_en in reg UI_INT_OIS
  *
  */
int32_t lsm6dsox_ois_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_spi2_read_en_t val)
{
  lsm6dsox_func_cfg_access_t func_cfg_access;
  lsm6dsox_ui_int_ois_t ui_int_ois;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*)&ui_int_ois, 1);
  if (ret == 0) {
    ui_int_ois.spi2_read_en = ((uint8_t)val & 0x01U);
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_INT_OIS,
                             (uint8_t*)&ui_int_ois, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FUNC_CFG_ACCESS,
                            (uint8_t*)&func_cfg_access, 1);
  }
  if (ret == 0) {
    func_cfg_access.ois_ctrl_from_ui = ( ((uint8_t)val & 0x02U) >> 1 );
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FUNC_CFG_ACCESS,
                             (uint8_t*)&func_cfg_access, 1);
  }
  return ret;
}

/**
  * @brief  aux_ois_data: [get] OIS data reading from Auxiliary / Main SPI
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of spi2_read_en
  *                                 in reg UI_INT_OIS
  *
  */
int32_t lsm6dsox_ois_mode_get(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_spi2_read_en_t *val)
{
  lsm6dsox_func_cfg_access_t func_cfg_access;
  lsm6dsox_ui_int_ois_t ui_int_ois;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*)&ui_int_ois, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FUNC_CFG_ACCESS,
                            (uint8_t*)&func_cfg_access, 1);
  }
  switch ((func_cfg_access.ois_ctrl_from_ui << 1) + ui_int_ois.spi2_read_en) {
    case LSM6DSOX_OIS_CTRL_AUX_DATA_UI:
      *val = LSM6DSOX_OIS_CTRL_AUX_DATA_UI;
      break;
    case LSM6DSOX_OIS_CTRL_AUX_DATA_UI_AUX:
      *val = LSM6DSOX_OIS_CTRL_AUX_DATA_UI_AUX;
      break;
    case LSM6DSOX_OIS_CTRL_UI_AUX_DATA_UI:
      *val = LSM6DSOX_OIS_CTRL_UI_AUX_DATA_UI;
      break;
    case LSM6DSOX_OIS_CTRL_UI_AUX_DATA_UI_AUX:
      *val = LSM6DSOX_OIS_CTRL_UI_AUX_DATA_UI_AUX;
      break;
    default:
      *val = LSM6DSOX_OIS_CTRL_AUX_DATA_UI;
      break;
  }
  return ret;
}

/**
  * @brief  aOn auxiliary interface connect/disconnect SDO and OCS
  *         internal pull-up.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ois_pu_dis in
  *                               reg PIN_CTRL
  *
  */
int32_t lsm6dsox_aux_sdo_ocs_mode_set(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_ois_pu_dis_t val)
{
  lsm6dsox_pin_ctrl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PIN_CTRL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.ois_pu_dis = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PIN_CTRL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  On auxiliary interface connect/disconnect SDO and OCS
  *         internal pull-up.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of ois_pu_dis in reg PIN_CTRL
  *
  */
int32_t lsm6dsox_aux_sdo_ocs_mode_get(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_ois_pu_dis_t *val)
{
  lsm6dsox_pin_ctrl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PIN_CTRL, (uint8_t*)&reg, 1);
  switch (reg.ois_pu_dis) {
    case LSM6DSOX_AUX_PULL_UP_DISC:
      *val = LSM6DSOX_AUX_PULL_UP_DISC;
      break;
    case LSM6DSOX_AUX_PULL_UP_CONNECT:
      *val = LSM6DSOX_AUX_PULL_UP_CONNECT;
      break;
    default:
      *val = LSM6DSOX_AUX_PULL_UP_DISC;
      break;
  }
  return ret;
}

/**
  * @brief  OIS chain on aux interface power on mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ois_on in reg CTRL7_G
  *
  */
int32_t lsm6dsox_aux_pw_on_ctrl_set(lsm6dsox_ctx_t *ctx, lsm6dsox_ois_on_t val)
{
  lsm6dsox_ctrl7_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.ois_on_en = (uint8_t)val & 0x01U;
    reg.ois_on = (uint8_t)val & 0x01U;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  aux_pw_on_ctrl: [get]  OIS chain on aux interface power on mode
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of ois_on in reg CTRL7_G
  *
  */
int32_t lsm6dsox_aux_pw_on_ctrl_get(lsm6dsox_ctx_t *ctx, lsm6dsox_ois_on_t *val)
{
  lsm6dsox_ctrl7_g_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL7_G, (uint8_t*)&reg, 1);
  switch (reg.ois_on) {
    case LSM6DSOX_AUX_ON:
      *val = LSM6DSOX_AUX_ON;
      break;
    case LSM6DSOX_AUX_ON_BY_AUX_INTERFACE:
      *val = LSM6DSOX_AUX_ON_BY_AUX_INTERFACE;
      break;
    default:
      *val = LSM6DSOX_AUX_ON;
      break;
  }

  return ret;
}

/**
  * @brief  Accelerometer full-scale management between UI chain and
  *         OIS chain. When XL UI is on, the full scale is the same
  *         between UI/OIS and is chosen by the UI CTRL registers;
  *         when XL UI is in PD, the OIS can choose the FS.
  *         Full scales are independent between the UI/OIS chain
  *         but both bound to 8 g.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of xl_fs_mode in
  *                               reg CTRL8_XL
  *
  */
int32_t lsm6dsox_aux_xl_fs_mode_set(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_xl_fs_mode_t val)
{
  lsm6dsox_ctrl8_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.xl_fs_mode = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer full-scale management between UI chain and
  *         OIS chain. When XL UI is on, the full scale is the same
  *         between UI/OIS and is chosen by the UI CTRL registers;
  *         when XL UI is in PD, the OIS can choose the FS.
  *         Full scales are independent between the UI/OIS chain
  *         but both bound to 8 g.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of xl_fs_mode in reg CTRL8_XL
  *
  */
int32_t lsm6dsox_aux_xl_fs_mode_get(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_xl_fs_mode_t *val)
{
  lsm6dsox_ctrl8_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL8_XL, (uint8_t*)&reg, 1);
  switch (reg.xl_fs_mode) {
    case LSM6DSOX_USE_SAME_XL_FS:
      *val = LSM6DSOX_USE_SAME_XL_FS;
      break;
    case LSM6DSOX_USE_DIFFERENT_XL_FS:
      *val = LSM6DSOX_USE_DIFFERENT_XL_FS;
      break;
    default:
      *val = LSM6DSOX_USE_SAME_XL_FS;
      break;
  }

  return ret;
}

/**
  * @brief  The STATUS_SPIAux register is read by the auxiliary SPI.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get registers STATUS_SPIAUX
  *
  */
int32_t lsm6dsox_aux_status_reg_get(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_spi2_status_reg_ois_t *val)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SPI2_STATUS_REG_OIS, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  aux_xl_flag_data_ready: [get]  AUX accelerometer data available
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of xlda in reg STATUS_SPIAUX
  *
  */
int32_t lsm6dsox_aux_xl_flag_data_ready_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_spi2_status_reg_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SPI2_STATUS_REG_OIS, (uint8_t*)&reg, 1);
  *val = reg.xlda;

  return ret;
}

/**
  * @brief  aux_gy_flag_data_ready: [get]  AUX gyroscope data available.
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of gda in reg STATUS_SPIAUX
  *
  */
int32_t lsm6dsox_aux_gy_flag_data_ready_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_spi2_status_reg_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SPI2_STATUS_REG_OIS, (uint8_t*)&reg, 1);
  *val = reg.gda;

  return ret;
}

/**
  * @brief  High when the gyroscope output is in the settling phase.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of gyro_settling in reg STATUS_SPIAUX
  *
  */
int32_t lsm6dsox_aux_gy_flag_settling_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_spi2_status_reg_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SPI2_STATUS_REG_OIS, (uint8_t*)&reg, 1);
  *val = reg.gyro_settling;

  return ret;
}

/**
  * @brief  Indicates polarity of DEN signal on OIS chain.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_lh_ois in
  *                  reg INT_OIS
  *
  */
int32_t lsm6dsox_aux_den_polarity_set(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_den_lh_ois_t val)
{
  lsm6dsox_ui_int_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.den_lh_ois = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Indicates polarity of DEN signal on OIS chain.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of den_lh_ois in reg INT_OIS
  *
  */
int32_t lsm6dsox_aux_den_polarity_get(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_den_lh_ois_t *val)
{
  lsm6dsox_ui_int_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*)&reg, 1);
  switch (reg.den_lh_ois) {
    case LSM6DSOX_AUX_DEN_ACTIVE_LOW:
      *val = LSM6DSOX_AUX_DEN_ACTIVE_LOW;
      break;
    case LSM6DSOX_AUX_DEN_ACTIVE_HIGH:
      *val = LSM6DSOX_AUX_DEN_ACTIVE_HIGH;
      break;
    default:
      *val = LSM6DSOX_AUX_DEN_ACTIVE_LOW;
      break;
  }
  return ret;
}

/**
  * @brief  Configure DEN mode on the OIS chain.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lvl2_ois in reg INT_OIS
  *
  */
int32_t lsm6dsox_aux_den_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_lvl2_ois_t val)
{
  lsm6dsox_ui_ctrl1_ois_t ctrl1_ois;
  lsm6dsox_ui_int_ois_t int_ois;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*) &int_ois, 1);
  if (ret == 0) {
    int_ois.lvl2_ois = (uint8_t)val & 0x01U;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*) &int_ois, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*) &ctrl1_ois, 1);
  }
  if (ret == 0) {
    ctrl1_ois.lvl1_ois = ((uint8_t)val & 0x02U) >> 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*) &ctrl1_ois, 1);
  }
  return ret;
}

/**
  * @brief  Configure DEN mode on the OIS chain.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of lvl2_ois in reg INT_OIS
  *
  */
int32_t lsm6dsox_aux_den_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_lvl2_ois_t *val)
{
  lsm6dsox_ui_ctrl1_ois_t ctrl1_ois;
  lsm6dsox_ui_int_ois_t int_ois;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*) &int_ois, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*) &ctrl1_ois, 1);
    switch ((ctrl1_ois.lvl1_ois << 1) + int_ois.lvl2_ois) {
      case LSM6DSOX_AUX_DEN_DISABLE:
        *val = LSM6DSOX_AUX_DEN_DISABLE;
        break;
      case LSM6DSOX_AUX_DEN_LEVEL_LATCH:
        *val = LSM6DSOX_AUX_DEN_LEVEL_LATCH;
        break;
      case LSM6DSOX_AUX_DEN_LEVEL_TRIG:
        *val = LSM6DSOX_AUX_DEN_LEVEL_TRIG;
        break;
      default:
        *val = LSM6DSOX_AUX_DEN_DISABLE;
        break;
    }
  }
  return ret;
}

/**
  * @brief  Enables/Disable OIS chain DRDY on INT2 pin.
  *         This setting has priority over all other INT2 settings.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int2_drdy_ois in reg INT_OIS
  *
  */
int32_t lsm6dsox_aux_drdy_on_int2_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ui_int_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.int2_drdy_ois = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables/Disable OIS chain DRDY on INT2 pin.
  *         This setting has priority over all other INT2 settings.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int2_drdy_ois in reg INT_OIS
  *
  */
int32_t lsm6dsox_aux_drdy_on_int2_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ui_int_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_INT_OIS, (uint8_t*)&reg, 1);
  *val = reg.int2_drdy_ois;

  return ret;
}

/**
  * @brief  Enables OIS chain data processing for gyro in Mode 3 and Mode 4
  *         (mode4_en = 1) and accelerometer data in and Mode 4 (mode4_en = 1).
  *         When the OIS chain is enabled, the OIS outputs are available
  *         through the SPI2 in registers OUTX_L_G (22h) through
  *         OUTZ_H_G (27h) and STATUS_REG (1Eh) / STATUS_SPIAux, and
  *         LPF1 is dedicated to this chain.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ois_en_spi2 in
  *                                reg CTRL1_OIS
  *
  */
int32_t lsm6dsox_aux_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_ois_en_spi2_t val)
{
  lsm6dsox_ui_ctrl1_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.ois_en_spi2 = (uint8_t)val & 0x01U;
    reg.mode4_en = ((uint8_t)val & 0x02U) >> 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables OIS chain data processing for gyro in Mode 3 and Mode 4
  *         (mode4_en = 1) and accelerometer data in and Mode 4 (mode4_en = 1).
  *         When the OIS chain is enabled, the OIS outputs are available
  *         through the SPI2 in registers OUTX_L_G (22h) through
  *         OUTZ_H_G (27h) and STATUS_REG (1Eh) / STATUS_SPIAux, and
  *         LPF1 is dedicated to this chain.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of ois_en_spi2 in
  *                                reg CTRL1_OIS
  *
  */
int32_t lsm6dsox_aux_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_ois_en_spi2_t *val)
{
  lsm6dsox_ui_ctrl1_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  switch ((reg.mode4_en << 1) | reg.ois_en_spi2) {
    case LSM6DSOX_AUX_DISABLE:
      *val = LSM6DSOX_AUX_DISABLE;
      break;
    case LSM6DSOX_MODE_3_GY:
      *val = LSM6DSOX_MODE_3_GY;
      break;
    case LSM6DSOX_MODE_4_GY_XL:
      *val = LSM6DSOX_MODE_4_GY_XL;
      break;
    default:
      *val = LSM6DSOX_AUX_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Selects gyroscope OIS chain full-scale.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fs_g_ois in reg CTRL1_OIS
  *
  */
int32_t lsm6dsox_aux_gy_full_scale_set(lsm6dsox_ctx_t *ctx,
                                      lsm6dsox_fs_g_ois_t val)
{
  lsm6dsox_ui_ctrl1_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.fs_g_ois = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects gyroscope OIS chain full-scale.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fs_g_ois in reg CTRL1_OIS
  *
  */
int32_t lsm6dsox_aux_gy_full_scale_get(lsm6dsox_ctx_t *ctx,
                                      lsm6dsox_fs_g_ois_t *val)
{
  lsm6dsox_ui_ctrl1_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  switch (reg.fs_g_ois) {
    case LSM6DSOX_250dps_AUX:
      *val = LSM6DSOX_250dps_AUX;
      break;
    case LSM6DSOX_125dps_AUX:
      *val = LSM6DSOX_125dps_AUX;
      break;
    case LSM6DSOX_500dps_AUX:
      *val = LSM6DSOX_500dps_AUX;
      break;
    case LSM6DSOX_1000dps_AUX:
      *val = LSM6DSOX_1000dps_AUX;
      break;
    case LSM6DSOX_2000dps_AUX:
      *val = LSM6DSOX_2000dps_AUX;
      break;
    default:
      *val = LSM6DSOX_250dps_AUX;
      break;
  }
  return ret;
}

/**
  * @brief  SPI2 3- or 4-wire interface.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sim_ois in reg CTRL1_OIS
  *
  */
int32_t lsm6dsox_aux_spi_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_sim_ois_t val)
{
  lsm6dsox_ui_ctrl1_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.sim_ois = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  SPI2 3- or 4-wire interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sim_ois in reg CTRL1_OIS
  *
  */
int32_t lsm6dsox_aux_spi_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_sim_ois_t *val)
{
  lsm6dsox_ui_ctrl1_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL1_OIS, (uint8_t*)&reg, 1);
  switch (reg.sim_ois) {
    case LSM6DSOX_AUX_SPI_4_WIRE:
      *val = LSM6DSOX_AUX_SPI_4_WIRE;
      break;
    case LSM6DSOX_AUX_SPI_3_WIRE:
      *val = LSM6DSOX_AUX_SPI_3_WIRE;
      break;
    default:
      *val = LSM6DSOX_AUX_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief  Selects gyroscope digital LPF1 filter bandwidth.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ftype_ois in
  *                              reg CTRL2_OIS
  *
  */
int32_t lsm6dsox_aux_gy_lp1_bandwidth_set(lsm6dsox_ctx_t *ctx,
                                         lsm6dsox_ftype_ois_t val)
{
  lsm6dsox_ui_ctrl2_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL2_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.ftype_ois = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL2_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects gyroscope digital LPF1 filter bandwidth.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of ftype_ois in reg CTRL2_OIS
  *
  */
int32_t lsm6dsox_aux_gy_lp1_bandwidth_get(lsm6dsox_ctx_t *ctx,
                                         lsm6dsox_ftype_ois_t *val)
{
  lsm6dsox_ui_ctrl2_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL2_OIS, (uint8_t*)&reg, 1);
  switch (reg.ftype_ois) {
    case LSM6DSOX_351Hz39:
      *val = LSM6DSOX_351Hz39;
      break;
    case LSM6DSOX_236Hz63:
      *val = LSM6DSOX_236Hz63;
      break;
    case LSM6DSOX_172Hz70:
      *val = LSM6DSOX_172Hz70;
      break;
    case LSM6DSOX_937Hz91:
      *val = LSM6DSOX_937Hz91;
      break;
    default:
      *val = LSM6DSOX_351Hz39;
      break;
  }
  return ret;
}

/**
  * @brief  Selects gyroscope OIS chain digital high-pass filter cutoff.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of hpm_ois in reg CTRL2_OIS
  *
  */
int32_t lsm6dsox_aux_gy_hp_bandwidth_set(lsm6dsox_ctx_t *ctx,
                                        lsm6dsox_hpm_ois_t val)
{
  lsm6dsox_ui_ctrl2_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL2_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.hpm_ois = (uint8_t)val & 0x03U;
    reg.hp_en_ois = ((uint8_t)val & 0x10U) >> 4;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL2_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects gyroscope OIS chain digital high-pass filter cutoff.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of hpm_ois in reg CTRL2_OIS
  *
  */
int32_t lsm6dsox_aux_gy_hp_bandwidth_get(lsm6dsox_ctx_t *ctx,
                                        lsm6dsox_hpm_ois_t *val)
{
  lsm6dsox_ui_ctrl2_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL2_OIS, (uint8_t*)&reg, 1);
  switch ((reg.hp_en_ois << 4) | reg.hpm_ois) {
    case LSM6DSOX_AUX_HP_DISABLE:
      *val = LSM6DSOX_AUX_HP_DISABLE;
      break;
    case LSM6DSOX_AUX_HP_Hz016:
      *val = LSM6DSOX_AUX_HP_Hz016;
      break;
    case LSM6DSOX_AUX_HP_Hz065:
      *val = LSM6DSOX_AUX_HP_Hz065;
      break;
    case LSM6DSOX_AUX_HP_Hz260:
      *val = LSM6DSOX_AUX_HP_Hz260;
      break;
    case LSM6DSOX_AUX_HP_1Hz040:
      *val = LSM6DSOX_AUX_HP_1Hz040;
      break;
    default:
      *val = LSM6DSOX_AUX_HP_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Enable / Disables OIS chain clamp.
  *         Enable: All OIS chain outputs = 8000h
  *         during self-test; Disable: OIS chain self-test
  *         outputs dependent from the aux gyro full
  *         scale selected.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of st_ois_clampdis in
  *                                    reg CTRL3_OIS
  *
  */
int32_t lsm6dsox_aux_gy_clamp_set(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_st_ois_clampdis_t val)
{
  lsm6dsox_ui_ctrl3_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.st_ois_clampdis = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable / Disables OIS chain clamp.
  *         Enable: All OIS chain outputs = 8000h
  *         during self-test; Disable: OIS chain self-test
  *         outputs dependent from the aux gyro full
  *         scale selected.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of st_ois_clampdis in
  *                                    reg CTRL3_OIS
  *
  */
int32_t lsm6dsox_aux_gy_clamp_get(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_st_ois_clampdis_t *val)
{
  lsm6dsox_ui_ctrl3_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);
  switch (reg.st_ois_clampdis) {
    case LSM6DSOX_ENABLE_CLAMP:
      *val = LSM6DSOX_ENABLE_CLAMP;
      break;
    case LSM6DSOX_DISABLE_CLAMP:
      *val = LSM6DSOX_DISABLE_CLAMP;
      break;
    default:
      *val = LSM6DSOX_ENABLE_CLAMP;
      break;
  }
  return ret;
}

/**
  * @brief  Selects accelerometer OIS channel bandwidth.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of
  *                                       filter_xl_conf_ois in reg CTRL3_OIS
  *
  */
int32_t lsm6dsox_aux_xl_bandwidth_set(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_filter_xl_conf_ois_t val)
{
  lsm6dsox_ui_ctrl3_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.filter_xl_conf_ois = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects accelerometer OIS channel bandwidth.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of
  *                                       filter_xl_conf_ois in reg CTRL3_OIS
  *
  */
int32_t lsm6dsox_aux_xl_bandwidth_get(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_filter_xl_conf_ois_t *val)
{
  lsm6dsox_ui_ctrl3_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);

  switch (reg.filter_xl_conf_ois) {
    case LSM6DSOX_289Hz:
      *val = LSM6DSOX_289Hz;
      break;
    case LSM6DSOX_258Hz:
      *val = LSM6DSOX_258Hz;
      break;
    case LSM6DSOX_120Hz:
      *val = LSM6DSOX_120Hz;
      break;
    case LSM6DSOX_65Hz2:
      *val = LSM6DSOX_65Hz2;
      break;
    case LSM6DSOX_33Hz2:
      *val = LSM6DSOX_33Hz2;
      break;
    case LSM6DSOX_16Hz6:
      *val = LSM6DSOX_16Hz6;
      break;
    case LSM6DSOX_8Hz30:
      *val = LSM6DSOX_8Hz30;
      break;
    case LSM6DSOX_4Hz15:
      *val = LSM6DSOX_4Hz15;
      break;
    default:
      *val = LSM6DSOX_289Hz;
      break;
  }
  return ret;
}

/**
  * @brief  Selects accelerometer OIS channel full-scale.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fs_xl_ois in
  *                              reg CTRL3_OIS
  *
  */
int32_t lsm6dsox_aux_xl_full_scale_set(lsm6dsox_ctx_t *ctx,
                                      lsm6dsox_fs_xl_ois_t val)
{
  lsm6dsox_ui_ctrl3_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.fs_xl_ois = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects accelerometer OIS channel full-scale.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fs_xl_ois in reg CTRL3_OIS
  *
  */
int32_t lsm6dsox_aux_xl_full_scale_get(lsm6dsox_ctx_t *ctx,
                                      lsm6dsox_fs_xl_ois_t *val)
{
  lsm6dsox_ui_ctrl3_ois_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_UI_CTRL3_OIS, (uint8_t*)&reg, 1);
  switch (reg.fs_xl_ois) {
    case LSM6DSOX_AUX_2g:
      *val = LSM6DSOX_AUX_2g;
      break;
    case LSM6DSOX_AUX_16g:
      *val = LSM6DSOX_AUX_16g;
      break;
    case LSM6DSOX_AUX_4g:
      *val = LSM6DSOX_AUX_4g;
      break;
    case LSM6DSOX_AUX_8g:
      *val = LSM6DSOX_AUX_8g;
      break;
    default:
      *val = LSM6DSOX_AUX_2g;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_ main_serial_interface
  * @brief     This section groups all the functions concerning main
  *            serial interface management (not auxiliary)
  * @{
  *
  */

/**
  * @brief  Connect/Disconnect SDO/SA0 internal pull-up.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sdo_pu_en in
  *                              reg PIN_CTRL
  *
  */
int32_t lsm6dsox_sdo_sa0_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_sdo_pu_en_t val)
{
  lsm6dsox_pin_ctrl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PIN_CTRL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.sdo_pu_en = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PIN_CTRL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Connect/Disconnect SDO/SA0 internal pull-up.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sdo_pu_en in reg PIN_CTRL
  *
  */
int32_t lsm6dsox_sdo_sa0_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_sdo_pu_en_t *val)
{
  lsm6dsox_pin_ctrl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PIN_CTRL, (uint8_t*)&reg, 1);
  switch (reg.sdo_pu_en) {
    case LSM6DSOX_PULL_UP_DISC:
      *val = LSM6DSOX_PULL_UP_DISC;
      break;
    case LSM6DSOX_PULL_UP_CONNECT:
      *val = LSM6DSOX_PULL_UP_CONNECT;
      break;
    default:
      *val = LSM6DSOX_PULL_UP_DISC;
      break;
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sim in reg CTRL3_C
  *
  */
int32_t lsm6dsox_spi_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_sim_t val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.sim = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sim in reg CTRL3_C
  *
  */
int32_t lsm6dsox_spi_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_sim_t *val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  switch (reg.sim) {
    case LSM6DSOX_SPI_4_WIRE:
      *val = LSM6DSOX_SPI_4_WIRE;
      break;
    case LSM6DSOX_SPI_3_WIRE:
      *val = LSM6DSOX_SPI_3_WIRE;
      break;
    default:
      *val = LSM6DSOX_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief  Disable / Enable I2C interface.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of i2c_disable in
  *                                reg CTRL4_C
  *
  */
int32_t lsm6dsox_i2c_interface_set(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_i2c_disable_t val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.i2c_disable = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Disable / Enable I2C interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of i2c_disable in
  *                                reg CTRL4_C
  *
  */
int32_t lsm6dsox_i2c_interface_get(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_i2c_disable_t *val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  switch (reg.i2c_disable) {
    case LSM6DSOX_I2C_ENABLE:
      *val = LSM6DSOX_I2C_ENABLE;
      break;
    case LSM6DSOX_I2C_DISABLE:
      *val = LSM6DSOX_I2C_DISABLE;
      break;
    default:
      *val = LSM6DSOX_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @brief  I3C Enable/Disable communication protocol[.set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of i3c_disable
  *                                    in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_i3c_disable_set(lsm6dsox_ctx_t *ctx, lsm6dsox_i3c_disable_t val)
{
  lsm6dsox_i3c_bus_avb_t i3c_bus_avb;
  lsm6dsox_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if (ret == 0) {
    ctrl9_xl.i3c_disable = ((uint8_t)val & 0x80U) >> 7;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  }
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_I3C_BUS_AVB,
                           (uint8_t*)&i3c_bus_avb, 1);
  }
  if (ret == 0) {
    i3c_bus_avb.i3c_bus_avb_sel = (uint8_t)val & 0x03U;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_I3C_BUS_AVB,
                            (uint8_t*)&i3c_bus_avb, 1);
  }

  return ret;
}

/**
  * @brief  I3C Enable/Disable communication protocol.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of i3c_disable in
  *                                reg CTRL9_XL
  *
  */
int32_t lsm6dsox_i3c_disable_get(lsm6dsox_ctx_t *ctx, lsm6dsox_i3c_disable_t *val)
{
  lsm6dsox_ctrl9_xl_t ctrl9_xl;
  lsm6dsox_i3c_bus_avb_t i3c_bus_avb;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_I3C_BUS_AVB,
                           (uint8_t*)&i3c_bus_avb, 1);

    switch ((ctrl9_xl.i3c_disable << 7) | i3c_bus_avb.i3c_bus_avb_sel) {
      case LSM6DSOX_I3C_DISABLE:
        *val = LSM6DSOX_I3C_DISABLE;
        break;
      case LSM6DSOX_I3C_ENABLE_T_50us:
        *val = LSM6DSOX_I3C_ENABLE_T_50us;
        break;
      case LSM6DSOX_I3C_ENABLE_T_2us:
        *val = LSM6DSOX_I3C_ENABLE_T_2us;
        break;
      case LSM6DSOX_I3C_ENABLE_T_1ms:
        *val = LSM6DSOX_I3C_ENABLE_T_1ms;
        break;
      case LSM6DSOX_I3C_ENABLE_T_25ms:
        *val = LSM6DSOX_I3C_ENABLE_T_25ms;
        break;
      default:
        *val = LSM6DSOX_I3C_DISABLE;
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
  * @defgroup  LSM6DSOX_interrupt_pins
  * @brief     This section groups all the functions that manage interrup pins
  * @{
  *
  */

/**
  * @brief  Select the signal that need to route on int1 pad.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      struct of registers: INT1_CTRL,
  *                  MD1_CFG, EMB_FUNC_INT1, FSM_INT1_A,
  *                  FSM_INT1_B
  *
  */
int32_t lsm6dsox_pin_int1_route_set(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_pin_int1_route_t *val)
{
  lsm6dsox_pin_int2_route_t pin_int2_route;
  lsm6dsox_tap_cfg2_t tap_cfg2;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MLC_INT1,
                            (uint8_t*)&val->mlc_int1, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_INT1,
                            (uint8_t*)&val->emb_func_int1, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FSM_INT1_A,
                            (uint8_t*)&val->fsm_int1_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FSM_INT1_B,
                            (uint8_t*)&val->fsm_int1_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  if (ret == 0) {
    if ( ( val->emb_func_int1.int1_fsm_lc
         | val->emb_func_int1.int1_sig_mot
         | val->emb_func_int1.int1_step_detector
         | val->emb_func_int1.int1_tilt
         | val->fsm_int1_a.int1_fsm1
         | val->fsm_int1_a.int1_fsm2
         | val->fsm_int1_a.int1_fsm3
         | val->fsm_int1_a.int1_fsm4
         | val->fsm_int1_a.int1_fsm5
         | val->fsm_int1_a.int1_fsm6
         | val->fsm_int1_a.int1_fsm7
         | val->fsm_int1_a.int1_fsm8
         | val->fsm_int1_b.int1_fsm9
         | val->fsm_int1_b.int1_fsm10
         | val->fsm_int1_b.int1_fsm11
         | val->fsm_int1_b.int1_fsm12
         | val->fsm_int1_b.int1_fsm13
         | val->fsm_int1_b.int1_fsm14
         | val->fsm_int1_b.int1_fsm15
         | val->fsm_int1_b.int1_fsm16
         | val->mlc_int1.int1_mlc1
         | val->mlc_int1.int1_mlc2
         | val->mlc_int1.int1_mlc3
         | val->mlc_int1.int1_mlc4
         | val->mlc_int1.int1_mlc5
         | val->mlc_int1.int1_mlc6
         | val->mlc_int1.int1_mlc7
         | val->mlc_int1.int1_mlc8) != PROPERTY_DISABLE){
      val->md1_cfg.int1_emb_func = PROPERTY_ENABLE;
    }
    else{
      val->md1_cfg.int1_emb_func = PROPERTY_DISABLE;
    }
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_INT1_CTRL,
                            (uint8_t*)&val->int1_ctrl, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MD1_CFG, (uint8_t*)&val->md1_cfg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*) &tap_cfg2, 1);
  }

  if (ret == 0) {
    ret = lsm6dsox_pin_int2_route_get(ctx, &pin_int2_route);
  }
  if (ret == 0) {
    if ( ( pin_int2_route.int2_ctrl.int2_cnt_bdr
         | pin_int2_route.int2_ctrl.int2_drdy_g
         | pin_int2_route.int2_ctrl.int2_drdy_temp
         | pin_int2_route.int2_ctrl.int2_drdy_xl
         | pin_int2_route.int2_ctrl.int2_fifo_full
         | pin_int2_route.int2_ctrl.int2_fifo_ovr
         | pin_int2_route.int2_ctrl.int2_fifo_th
         | pin_int2_route.md2_cfg.int2_6d
         | pin_int2_route.md2_cfg.int2_double_tap
         | pin_int2_route.md2_cfg.int2_ff
         | pin_int2_route.md2_cfg.int2_wu
         | pin_int2_route.md2_cfg.int2_single_tap
         | pin_int2_route.md2_cfg.int2_sleep_change
         | val->int1_ctrl.den_drdy_flag
         | val->int1_ctrl.int1_boot
         | val->int1_ctrl.int1_cnt_bdr
         | val->int1_ctrl.int1_drdy_g
         | val->int1_ctrl.int1_drdy_xl
         | val->int1_ctrl.int1_fifo_full
         | val->int1_ctrl.int1_fifo_ovr
         | val->int1_ctrl.int1_fifo_th
         | val->md1_cfg.int1_shub
         | val->md1_cfg.int1_6d
         | val->md1_cfg.int1_double_tap
         | val->md1_cfg.int1_ff
         | val->md1_cfg.int1_wu
         | val->md1_cfg.int1_single_tap
         | val->md1_cfg.int1_sleep_change) != PROPERTY_DISABLE) {
      tap_cfg2.interrupts_enable = PROPERTY_ENABLE;
    }
    else{
      tap_cfg2.interrupts_enable = PROPERTY_DISABLE;
    }
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*) &tap_cfg2, 1);
  }
  return ret;
}

/**
  * @brief  Select the signal that need to route on int1 pad.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      struct of registers: INT1_CTRL, MD1_CFG,
  *                  EMB_FUNC_INT1, FSM_INT1_A, FSM_INT1_B
  *
  */
int32_t lsm6dsox_pin_int1_route_get(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_pin_int1_route_t *val)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MLC_INT1,
                            (uint8_t*)&val->mlc_int1, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_INT1,
                           (uint8_t*)&val->emb_func_int1, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_INT1_A,
                           (uint8_t*)&val->fsm_int1_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_INT1_B,
                           (uint8_t*)&val->fsm_int1_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INT1_CTRL,
                           (uint8_t*)&val->int1_ctrl, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MD1_CFG, (uint8_t*)&val->md1_cfg, 1);
  }

  return ret;
}

/**
  * @brief  Select the signal that need to route on int2 pad.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      union of registers INT2_CTRL,  MD2_CFG,
  *                  EMB_FUNC_INT2, FSM_INT2_A, FSM_INT2_B
  *
  */
int32_t lsm6dsox_pin_int2_route_set(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_pin_int2_route_t *val)
{
  lsm6dsox_pin_int1_route_t pin_int1_route;
  lsm6dsox_tap_cfg2_t tap_cfg2;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MLC_INT1,
                            (uint8_t*)&val->mlc_int2, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_INT2,
                            (uint8_t*)&val->emb_func_int2, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FSM_INT2_A,
                            (uint8_t*)&val->fsm_int2_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FSM_INT2_B,
                            (uint8_t*)&val->fsm_int2_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  if (ret == 0) {
    if (( val->emb_func_int2.int2_fsm_lc
        | val->emb_func_int2.int2_sig_mot
        | val->emb_func_int2.int2_step_detector
        | val->emb_func_int2.int2_tilt
        | val->fsm_int2_a.int2_fsm1
        | val->fsm_int2_a.int2_fsm2
        | val->fsm_int2_a.int2_fsm3
        | val->fsm_int2_a.int2_fsm4
        | val->fsm_int2_a.int2_fsm5
        | val->fsm_int2_a.int2_fsm6
        | val->fsm_int2_a.int2_fsm7
        | val->fsm_int2_a.int2_fsm8
        | val->fsm_int2_b.int2_fsm9
        | val->fsm_int2_b.int2_fsm10
        | val->fsm_int2_b.int2_fsm11
        | val->fsm_int2_b.int2_fsm12
        | val->fsm_int2_b.int2_fsm13
        | val->fsm_int2_b.int2_fsm14
        | val->fsm_int2_b.int2_fsm15
        | val->fsm_int2_b.int2_fsm16
        | val->mlc_int2.int2_mlc1
        | val->mlc_int2.int2_mlc2
        | val->mlc_int2.int2_mlc3
        | val->mlc_int2.int2_mlc4
        | val->mlc_int2.int2_mlc5
        | val->mlc_int2.int2_mlc6
        | val->mlc_int2.int2_mlc7
        | val->mlc_int2.int2_mlc8)!= PROPERTY_DISABLE ){
      val->md2_cfg.int2_emb_func = PROPERTY_ENABLE;
    }
    else{
      val->md2_cfg.int2_emb_func = PROPERTY_DISABLE;
    }
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_INT2_CTRL,
                            (uint8_t*)&val->int2_ctrl, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MD2_CFG, (uint8_t*)&val->md2_cfg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*) &tap_cfg2, 1);
  }

  if (ret == 0) {
    ret = lsm6dsox_pin_int1_route_get(ctx, &pin_int1_route);
  }

  if (ret == 0) {
    if ( ( val->int2_ctrl.int2_cnt_bdr
         | val->int2_ctrl.int2_drdy_g
         | val->int2_ctrl.int2_drdy_temp
         | val->int2_ctrl.int2_drdy_xl
         | val->int2_ctrl.int2_fifo_full
         | val->int2_ctrl.int2_fifo_ovr
         | val->int2_ctrl.int2_fifo_th
         | val->md2_cfg.int2_6d
         | val->md2_cfg.int2_double_tap
         | val->md2_cfg.int2_ff
         | val->md2_cfg.int2_wu
         | val->md2_cfg.int2_single_tap
         | val->md2_cfg.int2_sleep_change
         | pin_int1_route.int1_ctrl.den_drdy_flag
         | pin_int1_route.int1_ctrl.int1_boot
         | pin_int1_route.int1_ctrl.int1_cnt_bdr
         | pin_int1_route.int1_ctrl.int1_drdy_g
         | pin_int1_route.int1_ctrl.int1_drdy_xl
         | pin_int1_route.int1_ctrl.int1_fifo_full
         | pin_int1_route.int1_ctrl.int1_fifo_ovr
         | pin_int1_route.int1_ctrl.int1_fifo_th
         | pin_int1_route.md1_cfg.int1_6d
         | pin_int1_route.md1_cfg.int1_double_tap
         | pin_int1_route.md1_cfg.int1_ff
         | pin_int1_route.md1_cfg.int1_wu
         | pin_int1_route.md1_cfg.int1_single_tap
         | pin_int1_route.md1_cfg.int1_sleep_change ) != PROPERTY_DISABLE) {
      tap_cfg2.interrupts_enable = PROPERTY_ENABLE;
    }
    else{
      tap_cfg2.interrupts_enable = PROPERTY_DISABLE;
    }
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*) &tap_cfg2, 1);
  }
  return ret;
}

/**
  * @brief  Select the signal that need to route on int2 pad.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      union of registers INT2_CTRL,  MD2_CFG,
  *                  EMB_FUNC_INT2, FSM_INT2_A, FSM_INT2_B
  *
  */
int32_t lsm6dsox_pin_int2_route_get(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_pin_int2_route_t *val)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
      ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MLC_INT2,
                            (uint8_t*)&val->mlc_int2, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_INT2,
                           (uint8_t*)&val->emb_func_int2, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_INT2_A,
                           (uint8_t*)&val->fsm_int2_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_INT2_B,
                           (uint8_t*)&val->fsm_int2_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INT2_CTRL,
                           (uint8_t*)&val->int2_ctrl, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MD2_CFG, (uint8_t*)&val->md2_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of pp_od in reg CTRL3_C
  *
  */
int32_t lsm6dsox_pin_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_pp_od_t val)
{
  lsm6dsox_i3c_bus_avb_t i3c_bus_avb;
  lsm6dsox_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if (ret == 0) {
    ctrl3_c.pp_od = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_I3C_BUS_AVB,
                            (uint8_t*)&i3c_bus_avb, 1);
  }
  if (ret == 0) {
    i3c_bus_avb.pd_dis_int1 = ( (uint8_t) val & 0x02U ) >> 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_I3C_BUS_AVB,
                             (uint8_t*)&i3c_bus_avb, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of pp_od in reg CTRL3_C
  *
  */
int32_t lsm6dsox_pin_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_pp_od_t *val)
{
  lsm6dsox_i3c_bus_avb_t i3c_bus_avb;
  lsm6dsox_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_I3C_BUS_AVB,
                            (uint8_t*)&i3c_bus_avb, 1);
  }
  
  switch ( (i3c_bus_avb.pd_dis_int1 << 1) + ctrl3_c.pp_od) {
    case LSM6DSOX_PUSH_PULL:
      *val = LSM6DSOX_PUSH_PULL;
      break;
    case LSM6DSOX_OPEN_DRAIN:
      *val = LSM6DSOX_OPEN_DRAIN;
      break;
    case LSM6DSOX_INT1_NOPULL_DOWN_INT2_PUSH_PULL:
      *val = LSM6DSOX_INT1_NOPULL_DOWN_INT2_PUSH_PULL;
      break;
    case LSM6DSOX_INT1_NOPULL_DOWN_INT2_OPEN_DRAIN:
      *val = LSM6DSOX_INT1_NOPULL_DOWN_INT2_OPEN_DRAIN;
      break;
    default:
      *val = LSM6DSOX_PUSH_PULL;
      break;
  }
  return ret;
}

/**
  * @brief  Interrupt active-high/low.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of h_lactive in reg CTRL3_C
  *
  */
int32_t lsm6dsox_pin_polarity_set(lsm6dsox_ctx_t *ctx, lsm6dsox_h_lactive_t val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.h_lactive = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  Interrupt active-high/low.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of h_lactive in reg CTRL3_C
  *
  */
int32_t lsm6dsox_pin_polarity_get(lsm6dsox_ctx_t *ctx, lsm6dsox_h_lactive_t *val)
{
  lsm6dsox_ctrl3_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL3_C, (uint8_t*)&reg, 1);

  switch (reg.h_lactive) {
    case LSM6DSOX_ACTIVE_HIGH:
      *val = LSM6DSOX_ACTIVE_HIGH;
      break;
    case LSM6DSOX_ACTIVE_LOW:
      *val = LSM6DSOX_ACTIVE_LOW;
      break;
    default:
      *val = LSM6DSOX_ACTIVE_HIGH;
      break;
  }
  return ret;
}

/**
  * @brief  All interrupt signals become available on INT1 pin.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int2_on_int1 in reg CTRL4_C
  *
  */
int32_t lsm6dsox_all_on_int1_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.int2_on_int1 = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  All interrupt signals become available on INT1 pin.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int2_on_int1 in reg CTRL4_C
  *
  */
int32_t lsm6dsox_all_on_int1_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  *val = reg.int2_on_int1;

  return ret;
}

/**
  * @brief  Interrupt notification mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lir in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_int_notification_set(lsm6dsox_ctx_t *ctx, lsm6dsox_lir_t val)
{
  lsm6dsox_tap_cfg0_t tap_cfg0;
  lsm6dsox_page_rw_t page_rw;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*) &tap_cfg0, 1);
  if (ret == 0) {
    tap_cfg0.lir = (uint8_t)val & 0x01U;
    tap_cfg0.int_clr_on_read = (uint8_t)val & 0x01U;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*) &tap_cfg0, 1);
  }
  if (ret == 0) {

    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    page_rw.emb_func_lir = ((uint8_t)val & 0x02U) >> 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Interrupt notification mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of lir in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_int_notification_get(lsm6dsox_ctx_t *ctx, lsm6dsox_lir_t *val)
{
  lsm6dsox_tap_cfg0_t tap_cfg0;
  lsm6dsox_page_rw_t page_rw;
  int32_t ret;


  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*) &tap_cfg0, 1);
  if (ret == 0) {

      ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  if (ret == 0) {
    switch ((page_rw.emb_func_lir << 1) | tap_cfg0.lir) {
      case LSM6DSOX_ALL_INT_PULSED:
        *val = LSM6DSOX_ALL_INT_PULSED;
        break;
      case LSM6DSOX_BASE_LATCHED_EMB_PULSED:
        *val = LSM6DSOX_BASE_LATCHED_EMB_PULSED;
        break;
      case LSM6DSOX_BASE_PULSED_EMB_LATCHED:
        *val = LSM6DSOX_BASE_PULSED_EMB_LATCHED;
        break;
      case LSM6DSOX_ALL_INT_LATCHED:
        *val = LSM6DSOX_ALL_INT_LATCHED;
        break;
      default:
        *val = LSM6DSOX_ALL_INT_PULSED;
        break;
    }
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_PAGE_RW, (uint8_t*) &page_rw, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_Wake_Up_event
  * @brief     This section groups all the functions that manage the Wake Up
  *            event generation.
  * @{
  *
  */

/**
  * @brief  Weight of 1 LSB of wakeup threshold.[set]
  *         0: 1 LSB =FS_XL  /  64
  *         1: 1 LSB = FS_XL / 256
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wake_ths_w in
  *                                 reg WAKE_UP_DUR
  *
  */
int32_t lsm6dsox_wkup_ths_weight_set(lsm6dsox_ctx_t *ctx,
                                    lsm6dsox_wake_ths_w_t val)
{
  lsm6dsox_wake_up_dur_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.wake_ths_w = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Weight of 1 LSB of wakeup threshold.[get]
  *         0: 1 LSB =FS_XL  /  64
  *         1: 1 LSB = FS_XL / 256
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of wake_ths_w in
  *                                 reg WAKE_UP_DUR
  *
  */
int32_t lsm6dsox_wkup_ths_weight_get(lsm6dsox_ctx_t *ctx,
                                    lsm6dsox_wake_ths_w_t *val)
{
  lsm6dsox_wake_up_dur_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);

  switch (reg.wake_ths_w) {
    case LSM6DSOX_LSb_FS_DIV_64:
      *val = LSM6DSOX_LSb_FS_DIV_64;
      break;
    case LSM6DSOX_LSb_FS_DIV_256:
      *val = LSM6DSOX_LSb_FS_DIV_256;
      break;
    default:
      *val = LSM6DSOX_LSb_FS_DIV_64;
      break;
  }
  return ret;
}

/**
  * @brief  Threshold for wakeup: 1 LSB weight depends on WAKE_THS_W in
  *         WAKE_UP_DUR.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wk_ths in reg WAKE_UP_THS
  *
  */
int32_t lsm6dsox_wkup_threshold_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_wake_up_ths_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.wk_ths = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Threshold for wakeup: 1 LSB weight depends on WAKE_THS_W in
  *         WAKE_UP_DUR.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wk_ths in reg WAKE_UP_THS
  *
  */
int32_t lsm6dsox_wkup_threshold_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_wake_up_ths_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);
  *val = reg.wk_ths;

  return ret;
}

/**
  * @brief  Wake up duration event.[set]
  *         1LSb = 1 / ODR
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of usr_off_on_wu in reg WAKE_UP_THS
  *
  */
int32_t lsm6dsox_xl_usr_offset_on_wkup_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_wake_up_ths_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.usr_off_on_wu = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Wake up duration event.[get]
  *         1LSb = 1 / ODR
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of usr_off_on_wu in reg WAKE_UP_THS
  *
  */
int32_t lsm6dsox_xl_usr_offset_on_wkup_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_wake_up_ths_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);
  *val = reg.usr_off_on_wu;

  return ret;
}

/**
  * @brief  Wake up duration event.[set]
  *         1LSb = 1 / ODR
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wake_dur in reg WAKE_UP_DUR
  *
  */
int32_t lsm6dsox_wkup_dur_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_wake_up_dur_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.wake_dur = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Wake up duration event.[get]
  *         1LSb = 1 / ODR
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wake_dur in reg WAKE_UP_DUR
  *
  */
int32_t lsm6dsox_wkup_dur_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_wake_up_dur_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);
  *val = reg.wake_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_ Activity/Inactivity_detection
  * @brief     This section groups all the functions concerning
  *            activity/inactivity detection.
  * @{
  *
  */

/**
  * @brief  Enables gyroscope Sleep mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sleep_g in reg CTRL4_C
  *
  */
int32_t lsm6dsox_gy_sleep_mode_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.sleep_g = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables gyroscope Sleep mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sleep_g in reg CTRL4_C
  *
  */
int32_t lsm6dsox_gy_sleep_mode_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl4_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL4_C, (uint8_t*)&reg, 1);
  *val = reg.sleep_g;

  return ret;
}

/**
  * @brief  Drives the sleep status instead of
  *         sleep change on INT pins
  *         (only if INT1_SLEEP_CHANGE or
  *         INT2_SLEEP_CHANGE bits are enabled).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sleep_status_on_int in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_act_pin_notification_set(lsm6dsox_ctx_t *ctx,
                                         lsm6dsox_sleep_status_on_int_t val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.sleep_status_on_int = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Drives the sleep status instead of
  *         sleep change on INT pins (only if
  *         INT1_SLEEP_CHANGE or
  *         INT2_SLEEP_CHANGE bits are enabled).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sleep_status_on_int in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_act_pin_notification_get(lsm6dsox_ctx_t *ctx,
                                         lsm6dsox_sleep_status_on_int_t *val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  switch (reg.sleep_status_on_int) {
    case LSM6DSOX_DRIVE_SLEEP_CHG_EVENT:
      *val = LSM6DSOX_DRIVE_SLEEP_CHG_EVENT;
      break;
    case LSM6DSOX_DRIVE_SLEEP_STATUS:
      *val = LSM6DSOX_DRIVE_SLEEP_STATUS;
      break;
    default:
      *val = LSM6DSOX_DRIVE_SLEEP_CHG_EVENT;
      break;
  }
  return ret;
}

/**
  * @brief  Enable inactivity function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of inact_en in reg TAP_CFG2
  *
  */
int32_t lsm6dsox_act_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_inact_en_t val)
{
  lsm6dsox_tap_cfg2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.inact_en = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable inactivity function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of inact_en in reg TAP_CFG2
  *
  */
int32_t lsm6dsox_act_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_inact_en_t *val)
{
  lsm6dsox_tap_cfg2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*)&reg, 1);
  switch (reg.inact_en) {
    case LSM6DSOX_XL_AND_GY_NOT_AFFECTED:
      *val = LSM6DSOX_XL_AND_GY_NOT_AFFECTED;
      break;
    case LSM6DSOX_XL_12Hz5_GY_NOT_AFFECTED:
      *val = LSM6DSOX_XL_12Hz5_GY_NOT_AFFECTED;
      break;
    case LSM6DSOX_XL_12Hz5_GY_SLEEP:
      *val = LSM6DSOX_XL_12Hz5_GY_SLEEP;
      break;
    case LSM6DSOX_XL_12Hz5_GY_PD:
      *val = LSM6DSOX_XL_12Hz5_GY_PD;
      break;
    default:
      *val = LSM6DSOX_XL_AND_GY_NOT_AFFECTED;
      break;
  }
  return ret;
}

/**
  * @brief  Duration to go in sleep mode.[set]
  *         1 LSb = 512 / ODR
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sleep_dur in reg WAKE_UP_DUR
  *
  */
int32_t lsm6dsox_act_sleep_dur_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_wake_up_dur_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.sleep_dur = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Duration to go in sleep mode.[get]
  *         1 LSb = 512 / ODR
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sleep_dur in reg WAKE_UP_DUR
  *
  */
int32_t lsm6dsox_act_sleep_dur_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_wake_up_dur_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&reg, 1);
  *val = reg.sleep_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_tap_generator
  * @brief     This section groups all the functions that manage the
  *            tap and double tap event generation.
  * @{
  *
  */

/**
  * @brief  Enable Z direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_z_en in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_tap_detection_on_z_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.tap_z_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable Z direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_z_en in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_tap_detection_on_z_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  *val = reg.tap_z_en;

  return ret;
}

/**
  * @brief  Enable Y direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_y_en in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_tap_detection_on_y_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.tap_y_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable Y direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_y_en in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_tap_detection_on_y_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  *val = reg.tap_y_en;

  return ret;
}

/**
  * @brief  Enable X direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_x_en in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_tap_detection_on_x_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.tap_x_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable X direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_x_en in reg TAP_CFG0
  *
  */
int32_t lsm6dsox_tap_detection_on_x_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_tap_cfg0_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG0, (uint8_t*)&reg, 1);
  *val = reg.tap_x_en;

  return ret;
}

/**
  * @brief  X-axis tap recognition threshold.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_ths_x in reg TAP_CFG1
  *
  */
int32_t lsm6dsox_tap_threshold_x_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_tap_cfg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG1, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.tap_ths_x = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG1, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  X-axis tap recognition threshold.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_ths_x in reg TAP_CFG1
  *
  */
int32_t lsm6dsox_tap_threshold_x_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_tap_cfg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG1, (uint8_t*)&reg, 1);
  *val = reg.tap_ths_x;

  return ret;
}

/**
  * @brief  Selection of axis priority for TAP detection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_priority in
  *                                 reg TAP_CFG1
  *
  */
int32_t lsm6dsox_tap_axis_priority_set(lsm6dsox_ctx_t *ctx,
                                      lsm6dsox_tap_priority_t val)
{
  lsm6dsox_tap_cfg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG1, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.tap_priority = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG1, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selection of axis priority for TAP detection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of tap_priority in
  *                                 reg TAP_CFG1
  *
  */
int32_t lsm6dsox_tap_axis_priority_get(lsm6dsox_ctx_t *ctx,
                                      lsm6dsox_tap_priority_t *val)
{
  lsm6dsox_tap_cfg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG1, (uint8_t*)&reg, 1);
  switch (reg.tap_priority) {
    case LSM6DSOX_XYZ:
      *val = LSM6DSOX_XYZ;
      break;
    case LSM6DSOX_YXZ:
      *val = LSM6DSOX_YXZ;
      break;
    case LSM6DSOX_XZY:
      *val = LSM6DSOX_XZY;
      break;
    case LSM6DSOX_ZYX:
      *val = LSM6DSOX_ZYX;
      break;
    case LSM6DSOX_YZX:
      *val = LSM6DSOX_YZX;
      break;
    case LSM6DSOX_ZXY:
      *val = LSM6DSOX_ZXY;
      break;
    default:
      *val = LSM6DSOX_XYZ;
      break;
  }
  return ret;
}

/**
  * @brief  Y-axis tap recognition threshold.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_ths_y in reg TAP_CFG2
  *
  */
int32_t lsm6dsox_tap_threshold_y_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_tap_cfg2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.tap_ths_y = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Y-axis tap recognition threshold.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_ths_y in reg TAP_CFG2
  *
  */
int32_t lsm6dsox_tap_threshold_y_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_tap_cfg2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_CFG2, (uint8_t*)&reg, 1);
  *val = reg.tap_ths_y;

  return ret;
}

/**
  * @brief  Z-axis recognition threshold.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_ths_z in reg TAP_THS_6D
  *
  */
int32_t lsm6dsox_tap_threshold_z_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_tap_ths_6d_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.tap_ths_z = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Z-axis recognition threshold.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_ths_z in reg TAP_THS_6D
  *
  */
int32_t lsm6dsox_tap_threshold_z_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_tap_ths_6d_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  *val = reg.tap_ths_z;

  return ret;
}

/**
  * @brief  Maximum duration is the maximum time of an
  *         over threshold signal detection to be recognized
  *         as a tap event. The default value of these bits
  *         is 00b which corresponds to 4*ODR_XL time.
  *         If the SHOCK[1:0] bits are set to a different
  *         value, 1LSB corresponds to 8*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of shock in reg INT_DUR2
  *
  */
int32_t lsm6dsox_tap_shock_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_int_dur2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.shock = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Maximum duration is the maximum time of an
  *         over threshold signal detection to be recognized
  *         as a tap event. The default value of these bits
  *         is 00b which corresponds to 4*ODR_XL time.
  *         If the SHOCK[1:0] bits are set to a different
  *         value, 1LSB corresponds to 8*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of shock in reg INT_DUR2
  *
  */
int32_t lsm6dsox_tap_shock_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_int_dur2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  *val = reg.shock;

  return ret;
}

/**
  * @brief   Quiet time is the time after the first detected
  *          tap in which there must not be any over threshold
  *          event.
  *          The default value of these bits is 00b which
  *          corresponds to 2*ODR_XL time. If the QUIET[1:0]
  *          bits are set to a different value,
  *          1LSB corresponds to 4*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of quiet in reg INT_DUR2
  *
  */
int32_t lsm6dsox_tap_quiet_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_int_dur2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.quiet = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Quiet time is the time after the first detected
  *         tap in which there must not be any over threshold
  *         event.
  *         The default value of these bits is 00b which
  *         corresponds to 2*ODR_XL time.
  *         If the QUIET[1:0] bits are set to a different
  *         value, 1LSB corresponds to 4*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of quiet in reg INT_DUR2
  *
  */
int32_t lsm6dsox_tap_quiet_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_int_dur2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  *val = reg.quiet;

  return ret;
}

/**
  * @brief  When double tap recognition is enabled,
  *         this register expresses the maximum time
  *         between two consecutive detected taps to
  *         determine a double tap event.
  *         The default value of these bits is 0000b which
  *         corresponds to 16*ODR_XL time.
  *         If the DUR[3:0] bits are set to a different value,
  *         1LSB corresponds to 32*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of dur in reg INT_DUR2
  *
  */
int32_t lsm6dsox_tap_dur_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_int_dur2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.dur = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  When double tap recognition is enabled,
  *         this register expresses the maximum time
  *         between two consecutive detected taps to
  *         determine a double tap event.
  *         The default value of these bits is 0000b which
  *         corresponds to 16*ODR_XL time. If the DUR[3:0]
  *         bits are set to a different value,
  *         1LSB corresponds to 32*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of dur in reg INT_DUR2
  *
  */
int32_t lsm6dsox_tap_dur_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_int_dur2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_INT_DUR2, (uint8_t*)&reg, 1);
  *val = reg.dur;

  return ret;
}

/**
  * @brief  Single/double-tap event enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of single_double_tap in reg WAKE_UP_THS
  *
  */
int32_t lsm6dsox_tap_mode_set(lsm6dsox_ctx_t *ctx,
                             lsm6dsox_single_double_tap_t val)
{
  lsm6dsox_wake_up_ths_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.single_double_tap = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Single/double-tap event enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of single_double_tap in reg WAKE_UP_THS
  *
  */
int32_t lsm6dsox_tap_mode_get(lsm6dsox_ctx_t *ctx,
                             lsm6dsox_single_double_tap_t *val)
{
  lsm6dsox_wake_up_ths_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_THS, (uint8_t*)&reg, 1);

  switch (reg.single_double_tap) {
    case LSM6DSOX_ONLY_SINGLE:
      *val = LSM6DSOX_ONLY_SINGLE;
      break;
    case LSM6DSOX_BOTH_SINGLE_DOUBLE:
      *val = LSM6DSOX_BOTH_SINGLE_DOUBLE;
      break;
    default:
      *val = LSM6DSOX_ONLY_SINGLE;
      break;
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_ Six_position_detection(6D/4D)
  * @brief   This section groups all the functions concerning six position
  *          detection (6D).
  * @{
  *
  */

/**
  * @brief  Threshold for 4D/6D function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sixd_ths in reg TAP_THS_6D
  *
  */
int32_t lsm6dsox_6d_threshold_set(lsm6dsox_ctx_t *ctx, lsm6dsox_sixd_ths_t val)
{
  lsm6dsox_tap_ths_6d_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.sixd_ths = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Threshold for 4D/6D function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sixd_ths in reg TAP_THS_6D
  *
  */
int32_t lsm6dsox_6d_threshold_get(lsm6dsox_ctx_t *ctx, lsm6dsox_sixd_ths_t *val)
{
  lsm6dsox_tap_ths_6d_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  switch (reg.sixd_ths) {
    case LSM6DSOX_DEG_80:
      *val = LSM6DSOX_DEG_80;
      break;
    case LSM6DSOX_DEG_70:
      *val = LSM6DSOX_DEG_70;
      break;
    case LSM6DSOX_DEG_60:
      *val = LSM6DSOX_DEG_60;
      break;
    case LSM6DSOX_DEG_50:
      *val = LSM6DSOX_DEG_50;
      break;
    default:
      *val = LSM6DSOX_DEG_80;
      break;
  }
  return ret;
}

/**
  * @brief  4D orientation detection enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of d4d_en in reg TAP_THS_6D
  *
  */
int32_t lsm6dsox_4d_mode_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_tap_ths_6d_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.d4d_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  4D orientation detection enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of d4d_en in reg TAP_THS_6D
  *
  */
int32_t lsm6dsox_4d_mode_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_tap_ths_6d_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_TAP_THS_6D, (uint8_t*)&reg, 1);
  *val = reg.d4d_en;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_free_fall
  * @brief   This section group all the functions concerning the free
  *          fall detection.
  * @{
  *
*/
/**
  * @brief  Free fall threshold setting.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ff_ths in reg FREE_FALL
  *
  */
int32_t lsm6dsox_ff_threshold_set(lsm6dsox_ctx_t *ctx, lsm6dsox_ff_ths_t val)
{
  lsm6dsox_free_fall_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FREE_FALL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.ff_ths = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FREE_FALL, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Free fall threshold setting.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of ff_ths in reg FREE_FALL
  *
  */
int32_t lsm6dsox_ff_threshold_get(lsm6dsox_ctx_t *ctx, lsm6dsox_ff_ths_t *val)
{
  lsm6dsox_free_fall_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FREE_FALL, (uint8_t*)&reg, 1);
  switch (reg.ff_ths) {
    case LSM6DSOX_FF_TSH_156mg:
      *val = LSM6DSOX_FF_TSH_156mg;
      break;
    case LSM6DSOX_FF_TSH_219mg:
      *val = LSM6DSOX_FF_TSH_219mg;
      break;
    case LSM6DSOX_FF_TSH_250mg:
      *val = LSM6DSOX_FF_TSH_250mg;
      break;
    case LSM6DSOX_FF_TSH_312mg:
      *val = LSM6DSOX_FF_TSH_312mg;
      break;
    case LSM6DSOX_FF_TSH_344mg:
      *val = LSM6DSOX_FF_TSH_344mg;
      break;
    case LSM6DSOX_FF_TSH_406mg:
      *val = LSM6DSOX_FF_TSH_406mg;
      break;
    case LSM6DSOX_FF_TSH_469mg:
      *val = LSM6DSOX_FF_TSH_469mg;
      break;
    case LSM6DSOX_FF_TSH_500mg:
      *val = LSM6DSOX_FF_TSH_500mg;
      break;
    default:
      *val = LSM6DSOX_FF_TSH_156mg;
      break;
  }
  return ret;
}

/**
  * @brief  Free-fall duration event.[set]
  *         1LSb = 1 / ODR
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ff_dur in reg FREE_FALL
  *
  */
int32_t lsm6dsox_ff_dur_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_wake_up_dur_t wake_up_dur;
  lsm6dsox_free_fall_t free_fall;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&wake_up_dur, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FREE_FALL, (uint8_t*)&free_fall, 1);
  }
  if (ret == 0) {
    wake_up_dur.ff_dur = ((uint8_t)val & 0x20U) >> 5;
    free_fall.ff_dur = (uint8_t)val & 0x1FU;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_WAKE_UP_DUR,
                            (uint8_t*)&wake_up_dur, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FREE_FALL, (uint8_t*)&free_fall, 1);
  }
  return ret;
}

/**
  * @brief  Free-fall duration event.[get]
  *         1LSb = 1 / ODR
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ff_dur in reg FREE_FALL
  *
  */
int32_t lsm6dsox_ff_dur_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_wake_up_dur_t wake_up_dur;
  lsm6dsox_free_fall_t free_fall;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_WAKE_UP_DUR, (uint8_t*)&wake_up_dur, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FREE_FALL, (uint8_t*)&free_fall, 1);
    *val = (wake_up_dur.ff_dur << 5) + free_fall.ff_dur;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_fifo
  * @brief   This section group all the functions concerning the fifo usage
  * @{
  *
  */

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wtm in reg FIFO_CTRL1
  *
  */
int32_t lsm6dsox_fifo_watermark_set(lsm6dsox_ctx_t *ctx, uint16_t val)
{
  lsm6dsox_fifo_ctrl1_t fifo_ctrl1;
  lsm6dsox_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
  if (ret == 0) {
    fifo_ctrl1.wtm = 0x00FFU & (uint8_t)val;
    fifo_ctrl2.wtm = (uint8_t)(( 0x0100U & val ) >> 8);
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL1, (uint8_t*)&fifo_ctrl1, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
  }
  return ret;
}

/**
  * @brief  FIFO watermark level selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wtm in reg FIFO_CTRL1
  *
  */
int32_t lsm6dsox_fifo_watermark_get(lsm6dsox_ctx_t *ctx, uint16_t *val)
{
  lsm6dsox_fifo_ctrl1_t fifo_ctrl1;
  lsm6dsox_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL1, (uint8_t*)&fifo_ctrl1, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&fifo_ctrl2, 1);
    *val = ((uint16_t)fifo_ctrl2.wtm << 8) + (uint16_t)fifo_ctrl1.wtm;
  }
  return ret;
}

/**
  * @brief  FIFO compression feature initialization request [set].
  *
  * @param  ctx       read / write interface definitions
  * @param  val       change the values of FIFO_COMPR_INIT in
  *                   reg EMB_FUNC_INIT_B
  *
  */
int32_t lsm6dsox_compression_algo_init_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_emb_func_init_b_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.fifo_compr_init = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  FIFO compression feature initialization request [get].
  *
  * @param  ctx    read / write interface definitions
  * @param  val    change the values of FIFO_COMPR_INIT in
  *                reg EMB_FUNC_INIT_B
  *
  */
int32_t lsm6dsox_compression_algo_init_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_init_b_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.fifo_compr_init;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Enable and configure compression algo.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of uncoptr_rate in
  *                  reg FIFO_CTRL2
  *
  */
int32_t lsm6dsox_compression_algo_set(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_uncoptr_rate_t val)
{
  lsm6dsox_emb_func_en_b_t emb_func_en_b;
  lsm6dsox_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B,
                           (uint8_t*)&emb_func_en_b, 1);
  }
  if (ret == 0) {
    emb_func_en_b.fifo_compr_en = ((uint8_t)val & 0x04U) >> 2;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B,
                            (uint8_t*)&emb_func_en_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  if (ret == 0) {

    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2,
                           (uint8_t*)&fifo_ctrl2, 1);
  }
  if (ret == 0) {
    fifo_ctrl2.fifo_compr_rt_en = ((uint8_t)val & 0x04U) >> 2;
    fifo_ctrl2.uncoptr_rate = (uint8_t)val & 0x03U;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL2,
                            (uint8_t*)&fifo_ctrl2, 1);
  }
  return ret;
}

/**
  * @brief  Enable and configure compression algo.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of uncoptr_rate in
  *                  reg FIFO_CTRL2
  *
  */
int32_t lsm6dsox_compression_algo_get(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_uncoptr_rate_t *val)
{
  lsm6dsox_fifo_ctrl2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);

  switch ((reg.fifo_compr_rt_en<<2) | reg.uncoptr_rate) {
    case LSM6DSOX_CMP_DISABLE:
      *val = LSM6DSOX_CMP_DISABLE;
      break;
    case LSM6DSOX_CMP_ALWAYS:
      *val = LSM6DSOX_CMP_ALWAYS;
      break;
    case LSM6DSOX_CMP_8_TO_1:
      *val = LSM6DSOX_CMP_8_TO_1;
      break;
    case LSM6DSOX_CMP_16_TO_1:
      *val = LSM6DSOX_CMP_16_TO_1;
      break;
    case LSM6DSOX_CMP_32_TO_1:
      *val = LSM6DSOX_CMP_32_TO_1;
      break;
    default:
      *val = LSM6DSOX_CMP_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Enables ODR CHANGE virtual sensor to be batched in FIFO.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odrchg_en in reg FIFO_CTRL2
  *
  */
int32_t lsm6dsox_fifo_virtual_sens_odr_chg_set(lsm6dsox_ctx_t *ctx,
                                              uint8_t val)
{
  lsm6dsox_fifo_ctrl2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.odrchg_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Enables ODR CHANGE virtual sensor to be batched in FIFO.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odrchg_en in reg FIFO_CTRL2
  *
  */
int32_t lsm6dsox_fifo_virtual_sens_odr_chg_get(lsm6dsox_ctx_t *ctx,
                                              uint8_t *val)
{
  lsm6dsox_fifo_ctrl2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  *val = reg.odrchg_en;

  return ret;
}

/**
  * @brief  Enables/Disables compression algorithm runtime.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_compr_rt_en in
  *                  reg FIFO_CTRL2
  *
  */
int32_t lsm6dsox_compression_algo_real_time_set(lsm6dsox_ctx_t *ctx,
                                               uint8_t val)
{
  lsm6dsox_fifo_ctrl2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.fifo_compr_rt_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief   Enables/Disables compression algorithm runtime. [get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_compr_rt_en in reg FIFO_CTRL2
  *
  */
int32_t lsm6dsox_compression_algo_real_time_get(lsm6dsox_ctx_t *ctx,
                                               uint8_t *val)
{
  lsm6dsox_fifo_ctrl2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  *val = reg.fifo_compr_rt_en;

  return ret;
}

/**
  * @brief  Sensing chain FIFO stop values memorization at
  *         threshold level.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of stop_on_wtm in reg FIFO_CTRL2
  *
  */
int32_t lsm6dsox_fifo_stop_on_wtm_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_fifo_ctrl2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.stop_on_wtm = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Sensing chain FIFO stop values memorization at
  *         threshold level.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of stop_on_wtm in reg FIFO_CTRL2
  *
  */
int32_t lsm6dsox_fifo_stop_on_wtm_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_fifo_ctrl2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL2, (uint8_t*)&reg, 1);
  *val = reg.stop_on_wtm;

  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for accelerometer data.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdr_xl in reg FIFO_CTRL3
  *
  */
int32_t lsm6dsox_fifo_xl_batch_set(lsm6dsox_ctx_t *ctx, lsm6dsox_bdr_xl_t val)
{
  lsm6dsox_fifo_ctrl3_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL3, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.bdr_xl = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL3, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for accelerometer data.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of bdr_xl in reg FIFO_CTRL3
  *
  */
int32_t lsm6dsox_fifo_xl_batch_get(lsm6dsox_ctx_t *ctx, lsm6dsox_bdr_xl_t *val)
{
  lsm6dsox_fifo_ctrl3_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL3, (uint8_t*)&reg, 1);
  switch (reg.bdr_xl) {
    case LSM6DSOX_XL_NOT_BATCHED:
      *val = LSM6DSOX_XL_NOT_BATCHED;
      break;
    case LSM6DSOX_XL_BATCHED_AT_12Hz5:
      *val = LSM6DSOX_XL_BATCHED_AT_12Hz5;
      break;
    case LSM6DSOX_XL_BATCHED_AT_26Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_26Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_52Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_52Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_104Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_104Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_208Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_208Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_417Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_417Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_833Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_833Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_1667Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_1667Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_3333Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_3333Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_6667Hz:
      *val = LSM6DSOX_XL_BATCHED_AT_6667Hz;
      break;
    case LSM6DSOX_XL_BATCHED_AT_6Hz5:
      *val = LSM6DSOX_XL_BATCHED_AT_6Hz5;
      break;
    default:
      *val = LSM6DSOX_XL_NOT_BATCHED;
      break;
  }

  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for gyroscope data.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdr_gy in reg FIFO_CTRL3
  *
  */
int32_t lsm6dsox_fifo_gy_batch_set(lsm6dsox_ctx_t *ctx, lsm6dsox_bdr_gy_t val)
{
  lsm6dsox_fifo_ctrl3_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL3, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.bdr_gy = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL3, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for gyroscope data.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of bdr_gy in reg FIFO_CTRL3
  *
  */
int32_t lsm6dsox_fifo_gy_batch_get(lsm6dsox_ctx_t *ctx, lsm6dsox_bdr_gy_t *val)
{
  lsm6dsox_fifo_ctrl3_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL3, (uint8_t*)&reg, 1);
  switch (reg.bdr_gy) {
    case LSM6DSOX_GY_NOT_BATCHED:
      *val = LSM6DSOX_GY_NOT_BATCHED;
      break;
    case LSM6DSOX_GY_BATCHED_AT_12Hz5:
      *val = LSM6DSOX_GY_BATCHED_AT_12Hz5;
      break;
    case LSM6DSOX_GY_BATCHED_AT_26Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_26Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_52Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_52Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_104Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_104Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_208Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_208Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_417Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_417Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_833Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_833Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_1667Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_1667Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_3333Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_3333Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_6667Hz:
      *val = LSM6DSOX_GY_BATCHED_AT_6667Hz;
      break;
    case LSM6DSOX_GY_BATCHED_AT_6Hz5:
      *val = LSM6DSOX_GY_BATCHED_AT_6Hz5;
      break;
    default:
      *val = LSM6DSOX_GY_NOT_BATCHED;
      break;
  }
  return ret;
}

/**
  * @brief  FIFO mode selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_mode in reg FIFO_CTRL4
  *
  */
int32_t lsm6dsox_fifo_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_fifo_mode_t val)
{
  lsm6dsox_fifo_ctrl4_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.fifo_mode = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  FIFO mode selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fifo_mode in reg FIFO_CTRL4
  *
  */
int32_t lsm6dsox_fifo_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_fifo_mode_t *val)
{
  lsm6dsox_fifo_ctrl4_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);

  switch (reg.fifo_mode) {
    case LSM6DSOX_BYPASS_MODE:
      *val = LSM6DSOX_BYPASS_MODE;
      break;
    case LSM6DSOX_FIFO_MODE:
      *val = LSM6DSOX_FIFO_MODE;
      break;
    case LSM6DSOX_STREAM_TO_FIFO_MODE:
      *val = LSM6DSOX_STREAM_TO_FIFO_MODE;
      break;
    case LSM6DSOX_BYPASS_TO_STREAM_MODE:
      *val = LSM6DSOX_BYPASS_TO_STREAM_MODE;
      break;
    case LSM6DSOX_STREAM_MODE:
      *val = LSM6DSOX_STREAM_MODE;
      break;
    case LSM6DSOX_BYPASS_TO_FIFO_MODE:
      *val = LSM6DSOX_BYPASS_TO_FIFO_MODE;
      break;
    default:
      *val = LSM6DSOX_BYPASS_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for temperature data.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odr_t_batch in reg FIFO_CTRL4
  *
  */
int32_t lsm6dsox_fifo_temp_batch_set(lsm6dsox_ctx_t *ctx,
                                    lsm6dsox_odr_t_batch_t val)
{
  lsm6dsox_fifo_ctrl4_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.odr_t_batch = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for temperature data.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of odr_t_batch in reg FIFO_CTRL4
  *
  */
int32_t lsm6dsox_fifo_temp_batch_get(lsm6dsox_ctx_t *ctx,
                                    lsm6dsox_odr_t_batch_t *val)
{
  lsm6dsox_fifo_ctrl4_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);

  switch (reg.odr_t_batch) {
    case LSM6DSOX_TEMP_NOT_BATCHED:
      *val = LSM6DSOX_TEMP_NOT_BATCHED;
      break;
    case LSM6DSOX_TEMP_BATCHED_AT_1Hz6:
      *val = LSM6DSOX_TEMP_BATCHED_AT_1Hz6;
      break;
    case LSM6DSOX_TEMP_BATCHED_AT_12Hz5:
      *val = LSM6DSOX_TEMP_BATCHED_AT_12Hz5;
      break;
    case LSM6DSOX_TEMP_BATCHED_AT_52Hz:
      *val = LSM6DSOX_TEMP_BATCHED_AT_52Hz;
      break;
    default:
      *val = LSM6DSOX_TEMP_NOT_BATCHED;
      break;
  }
  return ret;
}

/**
  * @brief  Selects decimation for timestamp batching in FIFO.
  *         Writing rate will be the maximum rate between XL and
  *         GYRO BDR divided by decimation decoder.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odr_ts_batch in reg FIFO_CTRL4
  *
  */
int32_t lsm6dsox_fifo_timestamp_decimation_set(lsm6dsox_ctx_t *ctx,
                                              lsm6dsox_odr_ts_batch_t val)
{
  lsm6dsox_fifo_ctrl4_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.odr_ts_batch = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief   Selects decimation for timestamp batching in FIFO.
  *          Writing rate will be the maximum rate between XL and
  *          GYRO BDR divided by decimation decoder.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of odr_ts_batch in reg FIFO_CTRL4
  *
  */
int32_t lsm6dsox_fifo_timestamp_decimation_get(lsm6dsox_ctx_t *ctx,
                                              lsm6dsox_odr_ts_batch_t *val)
{
  lsm6dsox_fifo_ctrl4_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_CTRL4, (uint8_t*)&reg, 1);
  switch (reg.odr_ts_batch) {
    case LSM6DSOX_NO_DECIMATION:
      *val = LSM6DSOX_NO_DECIMATION;
      break;
    case LSM6DSOX_DEC_1:
      *val = LSM6DSOX_DEC_1;
      break;
    case LSM6DSOX_DEC_8:
      *val = LSM6DSOX_DEC_8;
      break;
    case LSM6DSOX_DEC_32:
      *val = LSM6DSOX_DEC_32;
      break;
    default:
      *val = LSM6DSOX_NO_DECIMATION;
      break;
  }
  return ret;
}

/**
  * @brief  Selects the trigger for the internal counter of batching events
  *         between XL and gyro.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of trig_counter_bdr
  *                  in reg COUNTER_BDR_REG1
  *
  */
int32_t lsm6dsox_fifo_cnt_event_batch_set(lsm6dsox_ctx_t *ctx,
                                         lsm6dsox_trig_counter_bdr_t val)
{
  lsm6dsox_counter_bdr_reg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.trig_counter_bdr = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Selects the trigger for the internal counter of batching events
  *         between XL and gyro.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of trig_counter_bdr
  *                                     in reg COUNTER_BDR_REG1
  *
  */
int32_t lsm6dsox_fifo_cnt_event_batch_get(lsm6dsox_ctx_t *ctx,
                                         lsm6dsox_trig_counter_bdr_t *val)
{
  lsm6dsox_counter_bdr_reg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  switch (reg.trig_counter_bdr) {
    case LSM6DSOX_XL_BATCH_EVENT:
      *val = LSM6DSOX_XL_BATCH_EVENT;
      break;
    case LSM6DSOX_GYRO_BATCH_EVENT:
      *val = LSM6DSOX_GYRO_BATCH_EVENT;
      break;
    default:
      *val = LSM6DSOX_XL_BATCH_EVENT;
      break;
  }
  return ret;
}

/**
  * @brief  Resets the internal counter of batching vents for a single sensor.
  *         This bit is automatically reset to zero if it was set to ‘1’.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of rst_counter_bdr in
  *                      reg COUNTER_BDR_REG1
  *
  */
int32_t lsm6dsox_rst_batch_counter_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_counter_bdr_reg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.rst_counter_bdr = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  Resets the internal counter of batching events for a single sensor.
  *         This bit is automatically reset to zero if it was set to ‘1’.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of rst_counter_bdr in
  *                  reg COUNTER_BDR_REG1
  *
  */
int32_t lsm6dsox_rst_batch_counter_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_counter_bdr_reg1_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1, (uint8_t*)&reg, 1);
  *val = reg.rst_counter_bdr;

  return ret;
}

/**
  * @brief  Batch data rate counter.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of cnt_bdr_th in
  *                  reg COUNTER_BDR_REG2 and COUNTER_BDR_REG1.
  *
  */
int32_t lsm6dsox_batch_counter_threshold_set(lsm6dsox_ctx_t *ctx, uint16_t val)
{
  lsm6dsox_counter_bdr_reg1_t counter_bdr_reg1;
  lsm6dsox_counter_bdr_reg2_t counter_bdr_reg2;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1,
                         (uint8_t*)&counter_bdr_reg1, 1);
  if (ret == 0) {
    counter_bdr_reg2.cnt_bdr_th =  0x00FFU & (uint8_t)val;
    counter_bdr_reg1.cnt_bdr_th = (uint8_t)(0x0700U & val) >> 8;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1,
                            (uint8_t*)&counter_bdr_reg1, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_COUNTER_BDR_REG2,
                            (uint8_t*)&counter_bdr_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Batch data rate counter.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of cnt_bdr_th in
  *                  reg COUNTER_BDR_REG2 and COUNTER_BDR_REG1.
  *
  */
int32_t lsm6dsox_batch_counter_threshold_get(lsm6dsox_ctx_t *ctx, uint16_t *val)
{
  lsm6dsox_counter_bdr_reg1_t counter_bdr_reg1;
  lsm6dsox_counter_bdr_reg2_t counter_bdr_reg2;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG1,
                         (uint8_t*)&counter_bdr_reg1, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_COUNTER_BDR_REG2,
                           (uint8_t*)&counter_bdr_reg2, 1);

    *val = ((uint16_t)counter_bdr_reg1.cnt_bdr_th << 8)
    + (uint16_t)counter_bdr_reg2.cnt_bdr_th;
  }

  return ret;
}

/**
  * @brief  Number of unread sensor data(TAG + 6 bytes) stored in FIFO.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of diff_fifo in reg FIFO_STATUS1
  *
  */
int32_t lsm6dsox_fifo_data_level_get(lsm6dsox_ctx_t *ctx, uint16_t *val)
{
  lsm6dsox_fifo_status1_t fifo_status1;
  lsm6dsox_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_STATUS1,
                         (uint8_t*)&fifo_status1, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_STATUS2,
                           (uint8_t*)&fifo_status2, 1);
    *val = ((uint16_t)fifo_status2.diff_fifo << 8) +
            (uint16_t)fifo_status1.diff_fifo;
  }
  return ret;
}

/**
  * @brief  FIFO status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers FIFO_STATUS2
  *
  */
int32_t lsm6dsox_fifo_status_get(lsm6dsox_ctx_t *ctx,
                                lsm6dsox_fifo_status2_t *val)
{
  int32_t ret;
  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_STATUS2, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Smart FIFO full status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_full_ia in reg FIFO_STATUS2
  *
  */
int32_t lsm6dsox_fifo_full_flag_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_fifo_status2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_STATUS2, (uint8_t*)&reg, 1);
  *val = reg.fifo_full_ia;

  return ret;
}

/**
  * @brief  FIFO overrun status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  fifo_over_run_latched in
  *                  reg FIFO_STATUS2
  *
  */
int32_t lsm6dsox_fifo_ovr_flag_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_fifo_status2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_STATUS2, (uint8_t*)&reg, 1);
  *val = reg.fifo_ovr_ia;

  return ret;
}

/**
  * @brief  FIFO watermark status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_wtm_ia in reg FIFO_STATUS2
  *
  */
int32_t lsm6dsox_fifo_wtm_flag_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_fifo_status2_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_STATUS2, (uint8_t*)&reg, 1);
  *val = reg.fifo_wtm_ia;

  return ret;
}

/**
  * @brief  Identifies the sensor in FIFO_DATA_OUT.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tag_sensor in reg FIFO_DATA_OUT_TAG
  *
  */
int32_t lsm6dsox_fifo_sensor_tag_get(lsm6dsox_ctx_t *ctx,
                                    lsm6dsox_fifo_tag_t *val)
{
  lsm6dsox_fifo_data_out_tag_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FIFO_DATA_OUT_TAG, (uint8_t*)&reg, 1);
  switch (reg.tag_sensor) {
    case LSM6DSOX_GYRO_NC_TAG:
      *val = LSM6DSOX_GYRO_NC_TAG;
      break;
    case LSM6DSOX_XL_NC_TAG:
      *val = LSM6DSOX_XL_NC_TAG;
      break;
    case LSM6DSOX_TEMPERATURE_TAG:
      *val = LSM6DSOX_TEMPERATURE_TAG;
      break;
    case LSM6DSOX_CFG_CHANGE_TAG:
      *val = LSM6DSOX_CFG_CHANGE_TAG;
      break;
    case LSM6DSOX_XL_NC_T_2_TAG:
      *val = LSM6DSOX_XL_NC_T_2_TAG;
      break;
    case LSM6DSOX_XL_NC_T_1_TAG:
      *val = LSM6DSOX_XL_NC_T_1_TAG;
      break;
    case LSM6DSOX_XL_2XC_TAG:
      *val = LSM6DSOX_XL_2XC_TAG;
      break;
    case LSM6DSOX_XL_3XC_TAG:
      *val = LSM6DSOX_XL_3XC_TAG;
      break;
    case LSM6DSOX_GYRO_NC_T_2_TAG:
      *val = LSM6DSOX_GYRO_NC_T_2_TAG;
      break;
    case LSM6DSOX_GYRO_NC_T_1_TAG:
      *val = LSM6DSOX_GYRO_NC_T_1_TAG;
      break;
    case LSM6DSOX_GYRO_2XC_TAG:
      *val = LSM6DSOX_GYRO_2XC_TAG;
      break;
    case LSM6DSOX_GYRO_3XC_TAG:
      *val = LSM6DSOX_GYRO_3XC_TAG;
      break;
    case LSM6DSOX_SENSORHUB_SLAVE0_TAG:
      *val = LSM6DSOX_SENSORHUB_SLAVE0_TAG;
      break;
    case LSM6DSOX_SENSORHUB_SLAVE1_TAG:
      *val = LSM6DSOX_SENSORHUB_SLAVE1_TAG;
      break;
    case LSM6DSOX_SENSORHUB_SLAVE2_TAG:
      *val = LSM6DSOX_SENSORHUB_SLAVE2_TAG;
      break;
    case LSM6DSOX_SENSORHUB_SLAVE3_TAG:
      *val = LSM6DSOX_SENSORHUB_SLAVE3_TAG;
      break;
    case LSM6DSOX_STEP_CPUNTER_TAG:
      *val = LSM6DSOX_STEP_CPUNTER_TAG;
      break;
    case LSM6DSOX_GAME_ROTATION_TAG:
      *val = LSM6DSOX_GAME_ROTATION_TAG;
      break;
    case LSM6DSOX_GEOMAG_ROTATION_TAG:
      *val = LSM6DSOX_GEOMAG_ROTATION_TAG;
      break;
    case LSM6DSOX_ROTATION_TAG:
      *val = LSM6DSOX_ROTATION_TAG;
      break;
    case LSM6DSOX_SENSORHUB_NACK_TAG:
      *val = LSM6DSOX_SENSORHUB_NACK_TAG;
      break;
    default:
      *val = LSM6DSOX_GYRO_NC_TAG;
      break;
  }
  return ret;
}

/**
  * @brief  :  Enable FIFO batching of pedometer embedded
  *            function values.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of gbias_fifo_en in
  *                  reg LSM6DSOX_EMB_FUNC_FIFO_CFG
  *
  */
int32_t lsm6dsox_fifo_pedo_batch_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_emb_func_fifo_cfg_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_FIFO_CFG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.pedo_fifo_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_FIFO_CFG,
                            (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Enable FIFO batching of pedometer embedded function values.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of pedo_fifo_en in
  *                  reg LSM6DSOX_EMB_FUNC_FIFO_CFG
  *
  */
int32_t lsm6dsox_fifo_pedo_batch_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_fifo_cfg_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_FIFO_CFG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.pedo_fifo_en;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Enable FIFO batching data of first slave.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  batch_ext_sens_0_en in
  *                  reg SLV0_CONFIG
  *
  */
int32_t lsm6dsox_sh_batch_slave_0_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_slv0_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV0_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.batch_ext_sens_0_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV0_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Enable FIFO batching data of first slave.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  batch_ext_sens_0_en in
  *                  reg SLV0_CONFIG
  *
  */
int32_t lsm6dsox_sh_batch_slave_0_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_slv0_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV0_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.batch_ext_sens_0_en;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Enable FIFO batching data of second slave.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  batch_ext_sens_1_en in
  *                  reg SLV1_CONFIG
  *
  */
int32_t lsm6dsox_sh_batch_slave_1_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_slv1_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV1_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.batch_ext_sens_1_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV1_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief   Enable FIFO batching data of second slave.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  batch_ext_sens_1_en in
  *                  reg SLV1_CONFIG
  *
  */
int32_t lsm6dsox_sh_batch_slave_1_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_slv1_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV1_CONFIG, (uint8_t*)&reg, 1);
    *val = reg.batch_ext_sens_1_en;
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Enable FIFO batching data of third slave.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  batch_ext_sens_2_en in
  *                  reg SLV2_CONFIG
  *
  */
int32_t lsm6dsox_sh_batch_slave_2_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_slv2_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);

  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV2_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.batch_ext_sens_2_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV2_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Enable FIFO batching data of third slave.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  batch_ext_sens_2_en in
  *                  reg SLV2_CONFIG
  *
  */
int32_t lsm6dsox_sh_batch_slave_2_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_slv2_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV2_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.batch_ext_sens_2_en;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief   Enable FIFO batching data of fourth slave.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  batch_ext_sens_3_en
  *                  in reg SLV3_CONFIG
  *
  */
int32_t lsm6dsox_sh_batch_slave_3_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_slv3_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV3_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.batch_ext_sens_3_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV3_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Enable FIFO batching data of fourth slave.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of  batch_ext_sens_3_en in
  *                  reg SLV3_CONFIG
  *
  */
int32_t lsm6dsox_sh_batch_slave_3_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_slv3_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV3_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.batch_ext_sens_3_en;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_DEN_functionality
  * @brief     This section groups all the functions concerning
  *            DEN functionality.
  * @{
  *
*/

/**
  * @brief  DEN functionality marking mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_mode in reg CTRL6_C
  *
  */
int32_t lsm6dsox_den_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_den_mode_t val)
{
  lsm6dsox_ctrl6_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.den_mode = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  DEN functionality marking mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of den_mode in reg CTRL6_C
  *
  */
int32_t lsm6dsox_den_mode_get(lsm6dsox_ctx_t *ctx, lsm6dsox_den_mode_t *val)
{
  lsm6dsox_ctrl6_c_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL6_C, (uint8_t*)&reg, 1);

  switch (reg.den_mode) {
    case LSM6DSOX_DEN_DISABLE:
      *val = LSM6DSOX_DEN_DISABLE;
      break;
    case LSM6DSOX_LEVEL_FIFO:
      *val = LSM6DSOX_LEVEL_FIFO;
      break;
    case LSM6DSOX_LEVEL_LETCHED:
      *val = LSM6DSOX_LEVEL_LETCHED;
      break;
    case LSM6DSOX_LEVEL_TRIGGER:
      *val = LSM6DSOX_LEVEL_TRIGGER;
      break;
    case LSM6DSOX_EDGE_TRIGGER:
      *val = LSM6DSOX_EDGE_TRIGGER;
      break;
    default:
      *val = LSM6DSOX_DEN_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  DEN active level configuration.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_lh in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_polarity_set(lsm6dsox_ctx_t *ctx, lsm6dsox_den_lh_t val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.den_lh = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  DEN active level configuration.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of den_lh in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_polarity_get(lsm6dsox_ctx_t *ctx, lsm6dsox_den_lh_t *val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);

  switch (reg.den_lh) {
    case LSM6DSOX_DEN_ACT_LOW:
      *val = LSM6DSOX_DEN_ACT_LOW;
      break;
    case LSM6DSOX_DEN_ACT_HIGH:
      *val = LSM6DSOX_DEN_ACT_HIGH;
      break;
    default:
      *val = LSM6DSOX_DEN_ACT_LOW;
      break;
  }
  return ret;
}

/**
  * @brief  DEN enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_xl_g in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_enable_set(lsm6dsox_ctx_t *ctx, lsm6dsox_den_xl_g_t val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.den_xl_g = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  DEN enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of den_xl_g in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_enable_get(lsm6dsox_ctx_t *ctx, lsm6dsox_den_xl_g_t *val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);

  switch (reg.den_xl_g) {
    case LSM6DSOX_STAMP_IN_GY_DATA:
      *val = LSM6DSOX_STAMP_IN_GY_DATA;
      break;
    case LSM6DSOX_STAMP_IN_XL_DATA:
      *val = LSM6DSOX_STAMP_IN_XL_DATA;
      break;
    case LSM6DSOX_STAMP_IN_GY_XL_DATA:
      *val = LSM6DSOX_STAMP_IN_GY_XL_DATA;
      break;
    default:
      *val = LSM6DSOX_STAMP_IN_GY_DATA;
      break;
  }
  return ret;
}

/**
  * @brief  DEN value stored in LSB of X-axis.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_z in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_mark_axis_x_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.den_z = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  DEN value stored in LSB of X-axis.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_z in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_mark_axis_x_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  *val = reg.den_z;

  return ret;
}

/**
  * @brief  DEN value stored in LSB of Y-axis.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_y in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_mark_axis_y_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.den_y = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  DEN value stored in LSB of Y-axis.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_y in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_mark_axis_y_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  *val = reg.den_y;

  return ret;
}

/**
  * @brief  DEN value stored in LSB of Z-axis.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_x in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_mark_axis_z_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.den_x = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  }

  return ret;
}

/**
  * @brief  DEN value stored in LSB of Z-axis.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of den_x in reg CTRL9_XL
  *
  */
int32_t lsm6dsox_den_mark_axis_z_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_ctrl9_xl_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_CTRL9_XL, (uint8_t*)&reg, 1);
  *val = reg.den_x;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_Pedometer
  * @brief     This section groups all the functions that manage pedometer.
  * @{
  *
*/

/**
  * @brief  Enable pedometer algorithm.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      turn on and configure pedometer
  *
  */
int32_t lsm6dsox_pedo_sens_set(lsm6dsox_ctx_t *ctx, lsm6dsox_pedo_md_t val)
{
  lsm6dsox_emb_func_en_a_t emb_func_en_a;
  lsm6dsox_emb_func_en_b_t emb_func_en_b;
  lsm6dsox_pedo_cmd_reg_t pedo_cmd_reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_PEDO_CMD_REG,
                                (uint8_t*)&pedo_cmd_reg);
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A,
                           (uint8_t*)&emb_func_en_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B,
                           (uint8_t*)&emb_func_en_b, 1);

    emb_func_en_a.pedo_en = (uint8_t)val & 0x01U;
    emb_func_en_b.mlc_en = ((uint8_t)val & 0x02U)>>1;
    pedo_cmd_reg.fp_rejection_en = ((uint8_t)val & 0x10U)>>4;
    pedo_cmd_reg.ad_det_en = ((uint8_t)val & 0x20U)>>5;
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A,
                            (uint8_t*)&emb_func_en_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B,
                            (uint8_t*)&emb_func_en_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  if (ret == 0) {
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_PEDO_CMD_REG,
                                   (uint8_t*)&pedo_cmd_reg);
  }
  return ret;
}

/**
  * @brief  Enable pedometer algorithm.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      turn on and configure pedometer
  *
  */
int32_t lsm6dsox_pedo_sens_get(lsm6dsox_ctx_t *ctx, lsm6dsox_pedo_md_t *val)
{
  lsm6dsox_emb_func_en_a_t emb_func_en_a;
  lsm6dsox_emb_func_en_b_t emb_func_en_b;
  lsm6dsox_pedo_cmd_reg_t pedo_cmd_reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_PEDO_CMD_REG,
                                (uint8_t*)&pedo_cmd_reg);
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A,
                           (uint8_t*)&emb_func_en_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B,
                           (uint8_t*)&emb_func_en_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  switch ( (pedo_cmd_reg.ad_det_en <<5) | (pedo_cmd_reg.fp_rejection_en << 4) |
           (emb_func_en_b.mlc_en << 1) | emb_func_en_a.pedo_en) {
    case LSM6DSOX_PEDO_DISABLE:
      *val = LSM6DSOX_PEDO_DISABLE;
      break;
    case LSM6DSOX_PEDO_BASE_MODE:
      *val = LSM6DSOX_PEDO_BASE_MODE;
      break;
    case LSM6DSOX_PEDO_ADV_MODE:
      *val = LSM6DSOX_PEDO_ADV_MODE;
      break;
    case LSM6DSOX_FALSE_STEP_REJ:
      *val = LSM6DSOX_FALSE_STEP_REJ;
      break;
    case LSM6DSOX_FALSE_STEP_REJ_ADV_MODE:
      *val = LSM6DSOX_FALSE_STEP_REJ_ADV_MODE;
      break;
    default:
      *val = LSM6DSOX_PEDO_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Interrupt status bit for step detection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of is_step_det in reg EMB_FUNC_STATUS
  *
  */
int32_t lsm6dsox_pedo_step_detect_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_status_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_STATUS, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.is_step_det;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Pedometer debounce configuration register (r/w).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_pedo_debounce_steps_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_PEDO_DEB_STEPS_CONF, buff);
  return ret;
}

/**
  * @brief  Pedometer debounce configuration register (r/w).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_pedo_debounce_steps_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_PEDO_DEB_STEPS_CONF, buff);
  return ret;
}

/**
  * @brief  Time period register for step detection on delta time (r/w).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_pedo_steps_period_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_PEDO_SC_DELTAT_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_PEDO_SC_DELTAT_H,
                                   &buff[index]);
  }
  return ret;
}

/**
  * @brief   Time period register for step detection on delta time (r/w).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_pedo_steps_period_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_PEDO_SC_DELTAT_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_PEDO_SC_DELTAT_H,
                                  &buff[index]);
  }
  return ret;
}

/**
  * @brief  Set when user wants to generate interrupt on count overflow
  *         event/every step.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of carry_count_en in reg PEDO_CMD_REG
  *
  */
int32_t lsm6dsox_pedo_int_mode_set(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_carry_count_en_t val)
{
  lsm6dsox_pedo_cmd_reg_t reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_PEDO_CMD_REG, (uint8_t*)&reg);
  if (ret == 0) {
    reg.carry_count_en = (uint8_t)val;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_PEDO_CMD_REG,
                                   (uint8_t*)&reg);
  }
  return ret;
}

/**
  * @brief  Set when user wants to generate interrupt on count overflow
  *         event/every step.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of carry_count_en in reg PEDO_CMD_REG
  *
  */
int32_t lsm6dsox_pedo_int_mode_get(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_carry_count_en_t *val)
{
  lsm6dsox_pedo_cmd_reg_t reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_PEDO_CMD_REG, (uint8_t*)&reg);
  switch (reg.carry_count_en) {
    case LSM6DSOX_EVERY_STEP:
      *val = LSM6DSOX_EVERY_STEP;
      break;
    case LSM6DSOX_COUNT_OVERFLOW:
      *val = LSM6DSOX_COUNT_OVERFLOW;
      break;
    default:
      *val = LSM6DSOX_EVERY_STEP;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_significant_motion
  * @brief   This section groups all the functions that manage the
  *          significant motion detection.
  * @{
  *
  */

/**
  * @brief  Enable significant motion detection function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sign_motion_en in reg EMB_FUNC_EN_A
  *
  */
int32_t lsm6dsox_motion_sens_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_emb_func_en_a_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.sign_motion_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Enable significant motion detection function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sign_motion_en in reg EMB_FUNC_EN_A
  *
  */
int32_t lsm6dsox_motion_sens_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_en_a_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.sign_motion_en;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief   Interrupt status bit for significant motion detection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of is_sigmot in reg EMB_FUNC_STATUS
  *
  */
int32_t lsm6dsox_motion_flag_data_ready_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_status_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_STATUS, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.is_sigmot;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_tilt_detection
  * @brief     This section groups all the functions that manage the tilt
  *            event detection.
  * @{
  *
  */

/**
  * @brief  Enable tilt calculation.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tilt_en in reg EMB_FUNC_EN_A
  *
  */
int32_t lsm6dsox_tilt_sens_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_emb_func_en_a_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.tilt_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Enable tilt calculation.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tilt_en in reg EMB_FUNC_EN_A
  *
  */
int32_t lsm6dsox_tilt_sens_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_en_a_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_A, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.tilt_en;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Interrupt status bit for tilt detection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of is_tilt in reg EMB_FUNC_STATUS
  *
  */
int32_t lsm6dsox_tilt_flag_data_ready_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_status_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_STATUS, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.is_tilt;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_ magnetometer_sensor
  * @brief     This section groups all the functions that manage additional
  *            magnetometer sensor.
  * @{
  *
  */

/**
  * @brief  External magnetometer sensitivity value register for
  *         Sensor hub.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_sh_mag_sensitivity_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SENSITIVITY_L,
                                 &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SENSITIVITY_H,
                                   &buff[index]);
  }

  return ret;
}

/**
  * @brief  External magnetometer sensitivity value register for
  *         Sensor hub.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_sh_mag_sensitivity_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SENSITIVITY_L,
                                &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SENSITIVITY_H,
                                  &buff[index]);
  }

  return ret;
}

/**
  * @brief  External magnetometer sensitivity value register for
  *         Machine Learning Core.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_mlc_mag_sensitivity_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MLC_MAG_SENSITIVITY_L,
                                  &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MLC_MAG_SENSITIVITY_H,
                                    &buff[index]);
  }
  return ret;
}

/**
  * @brief  External magnetometer sensitivity value register for
  *         Machine Learning Core.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_mlc_mag_sensitivity_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MLC_MAG_SENSITIVITY_L,
                                 &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MLC_MAG_SENSITIVITY_H,
                                   &buff[index]);
  }
  return ret;
}


/**
  * @brief  Offset for hard-iron compensation register (r/w).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_mag_offset_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_OFFX_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_OFFX_H, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_OFFY_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_OFFY_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_OFFZ_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_OFFZ_H, &buff[index]);
  }

  return ret;
}

/**
  * @brief  Offset for hard-iron compensation register (r/w).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_mag_offset_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_OFFX_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_OFFX_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_OFFY_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_OFFY_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_OFFZ_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_OFFZ_H, &buff[index]);
  }
  return ret;
}

/**
  * @brief  Soft-iron (3x3 symmetric) matrix correction
  *         register (r/w). The value is expressed as
  *         half-precision floating-point format:
  *         SEEEEEFFFFFFFFFF
  *         S: 1 sign bit;
  *         E: 5 exponent bits;
  *         F: 10 fraction bits).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_mag_soft_iron_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_XX_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_XX_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_XY_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_XY_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_XZ_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_XZ_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_YY_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_YY_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_YZ_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_YZ_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_ZZ_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_SI_ZZ_H, &buff[index]);
  }

  return ret;
}

/**
  * @brief  Soft-iron (3x3 symmetric) matrix
  *         correction register (r/w).
  *         The value is expressed as half-precision
  *         floating-point format:
  *         SEEEEEFFFFFFFFFF
  *         S: 1 sign bit;
  *         E: 5 exponent bits;
  *         F: 10 fraction bits.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_mag_soft_iron_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_XX_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_XX_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_XY_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_XY_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_XZ_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_XZ_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_YY_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_YY_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_YZ_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_YZ_H, &buff[index]);
  }
  if (ret == 0) {
    index++;

    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_ZZ_L, &buff[index]);
  }
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_SI_ZZ_H, &buff[index]);
  }

  return ret;
}

/**
  * @brief  Magnetometer Z-axis coordinates
  *         rotation (to be aligned to
  *         accelerometer/gyroscope axes
  *         orientation).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of mag_z_axis in reg MAG_CFG_A
  *
  */
int32_t lsm6dsox_mag_z_orient_set(lsm6dsox_ctx_t *ctx, lsm6dsox_mag_z_axis_t val)
{
  lsm6dsox_mag_cfg_a_t reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_CFG_A, (uint8_t*)&reg);
  if (ret == 0) {
    reg.mag_z_axis = (uint8_t) val;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_CFG_A, (uint8_t*)&reg);
  }

  return ret;
}

/**
  * @brief  Magnetometer Z-axis coordinates
  *         rotation (to be aligned to
  *         accelerometer/gyroscope axes
  *         orientation).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of mag_z_axis in reg MAG_CFG_A
  *
  */
int32_t lsm6dsox_mag_z_orient_get(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_mag_z_axis_t *val)
{
  lsm6dsox_mag_cfg_a_t reg;
  int32_t ret;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_CFG_A, (uint8_t*)&reg);
  switch (reg.mag_z_axis) {
    case LSM6DSOX_Z_EQ_Y:
      *val = LSM6DSOX_Z_EQ_Y;
      break;
    case LSM6DSOX_Z_EQ_MIN_Y:
      *val = LSM6DSOX_Z_EQ_MIN_Y;
      break;
    case LSM6DSOX_Z_EQ_X:
      *val = LSM6DSOX_Z_EQ_X;
      break;
    case LSM6DSOX_Z_EQ_MIN_X:
      *val = LSM6DSOX_Z_EQ_MIN_X;
      break;
    case LSM6DSOX_Z_EQ_MIN_Z:
      *val = LSM6DSOX_Z_EQ_MIN_Z;
      break;
    case LSM6DSOX_Z_EQ_Z:
      *val = LSM6DSOX_Z_EQ_Z;
      break;
    default:
      *val = LSM6DSOX_Z_EQ_Y;
      break;
  }
  return ret;
}

/**
  * @brief   Magnetometer Y-axis coordinates
  *          rotation (to be aligned to
  *          accelerometer/gyroscope axes
  *          orientation).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of mag_y_axis in reg MAG_CFG_A
  *
  */
int32_t lsm6dsox_mag_y_orient_set(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_mag_y_axis_t val)
{
  lsm6dsox_mag_cfg_a_t reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_CFG_A, (uint8_t*)&reg);
  if (ret == 0) {
    reg.mag_y_axis = (uint8_t)val;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_CFG_A,(uint8_t*) &reg);
  }
  return ret;
}

/**
  * @brief  Magnetometer Y-axis coordinates
  *         rotation (to be aligned to
  *         accelerometer/gyroscope axes
  *         orientation).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of mag_y_axis in reg MAG_CFG_A
  *
  */
int32_t lsm6dsox_mag_y_orient_get(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_mag_y_axis_t *val)
{
  lsm6dsox_mag_cfg_a_t reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_CFG_A, (uint8_t*)&reg);
  switch (reg.mag_y_axis) {
    case LSM6DSOX_Y_EQ_Y:
      *val = LSM6DSOX_Y_EQ_Y;
      break;
    case LSM6DSOX_Y_EQ_MIN_Y:
      *val = LSM6DSOX_Y_EQ_MIN_Y;
      break;
    case LSM6DSOX_Y_EQ_X:
      *val = LSM6DSOX_Y_EQ_X;
      break;
    case LSM6DSOX_Y_EQ_MIN_X:
      *val = LSM6DSOX_Y_EQ_MIN_X;
      break;
    case LSM6DSOX_Y_EQ_MIN_Z:
      *val = LSM6DSOX_Y_EQ_MIN_Z;
      break;
    case LSM6DSOX_Y_EQ_Z:
      *val = LSM6DSOX_Y_EQ_Z;
      break;
    default:
      *val = LSM6DSOX_Y_EQ_Y;
      break;
  }
  return ret;
}

/**
  * @brief  Magnetometer X-axis coordinates
  *         rotation (to be aligned to
  *         accelerometer/gyroscope axes
  *         orientation).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of mag_x_axis in reg MAG_CFG_B
  *
  */
int32_t lsm6dsox_mag_x_orient_set(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_mag_x_axis_t val)
{
  lsm6dsox_mag_cfg_b_t reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_CFG_B, (uint8_t*)&reg);
  if (ret == 0) {
    reg.mag_x_axis = (uint8_t)val;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_MAG_CFG_B, (uint8_t*)&reg);
  }
  return ret;
}

/**
  * @brief   Magnetometer X-axis coordinates
  *          rotation (to be aligned to
  *          accelerometer/gyroscope axes
  *          orientation).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of mag_x_axis in reg MAG_CFG_B
  *
  */
int32_t lsm6dsox_mag_x_orient_get(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_mag_x_axis_t *val)
{
  lsm6dsox_mag_cfg_b_t reg;
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_MAG_CFG_B, (uint8_t*)&reg);
  switch (reg.mag_x_axis) {
    case LSM6DSOX_X_EQ_Y:
      *val = LSM6DSOX_X_EQ_Y;
      break;
    case LSM6DSOX_X_EQ_MIN_Y:
      *val = LSM6DSOX_X_EQ_MIN_Y;
      break;
    case LSM6DSOX_X_EQ_X:
      *val = LSM6DSOX_X_EQ_X;
      break;
    case LSM6DSOX_X_EQ_MIN_X:
      *val = LSM6DSOX_X_EQ_MIN_X;
      break;
    case LSM6DSOX_X_EQ_MIN_Z:
      *val = LSM6DSOX_X_EQ_MIN_Z;
      break;
    case LSM6DSOX_X_EQ_Z:
      *val = LSM6DSOX_X_EQ_Z;
      break;
    default:
      *val = LSM6DSOX_X_EQ_Y;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_significant_motion
  * @brief     This section groups all the functions that manage the
  *            state_machine.
  * @{
  *
  */

/**
  * @brief   Interrupt status bit for FSM long counter
  *          timeout interrupt event.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of is_fsm_lc in reg EMB_FUNC_STATUS
  *
  */
int32_t lsm6dsox_long_cnt_flag_data_ready_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_status_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_STATUS, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.is_fsm_lc;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Final State Machine global enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fsm_en in reg EMB_FUNC_EN_B
  *
  */
int32_t lsm6dsox_emb_fsm_en_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  int32_t ret;
  lsm6dsox_emb_func_en_b_t reg;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.fsm_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Final State Machine global enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  uint8_t *: return the values of fsm_en in reg EMB_FUNC_EN_B
  *
  */
int32_t lsm6dsox_emb_fsm_en_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  int32_t ret;
  lsm6dsox_emb_func_en_b_t reg;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.fsm_en;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Finite State Machine enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      union of registers from FSM_ENABLE_A to FSM_ENABLE_B
  *
  */
int32_t lsm6dsox_fsm_enable_set(lsm6dsox_ctx_t *ctx,
                                lsm6dsox_emb_fsm_enable_t *val)
{
  int32_t ret;
  lsm6dsox_emb_func_en_b_t emb_func_en_b;
  lsm6dsox_emb_func_init_b_t emb_func_init_b;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FSM_ENABLE_A,
                            (uint8_t*)&val->fsm_enable_a, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FSM_ENABLE_B,
                            (uint8_t*)&val->fsm_enable_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B, 
                            (uint8_t*)&emb_func_init_b, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B,
                            (uint8_t*)&emb_func_en_b, 1);
  }
  if (ret == 0) {
    if ( (val->fsm_enable_a.fsm1_en   |
          val->fsm_enable_a.fsm2_en   |
          val->fsm_enable_a.fsm3_en   |
          val->fsm_enable_a.fsm4_en   |
          val->fsm_enable_a.fsm5_en   |
          val->fsm_enable_a.fsm6_en   |
          val->fsm_enable_a.fsm7_en   |
          val->fsm_enable_a.fsm8_en   |
          val->fsm_enable_b.fsm9_en   |
          val->fsm_enable_b.fsm10_en  |
          val->fsm_enable_b.fsm11_en  |
          val->fsm_enable_b.fsm12_en  |
          val->fsm_enable_b.fsm13_en  |
          val->fsm_enable_b.fsm14_en  |
          val->fsm_enable_b.fsm15_en  |
          val->fsm_enable_b.fsm16_en  )
          != PROPERTY_DISABLE)
    {
      emb_func_en_b.fsm_en = PROPERTY_ENABLE;
      emb_func_init_b.fsm_init = PROPERTY_ENABLE;
    }
    else
    {
      emb_func_en_b.fsm_en = PROPERTY_DISABLE;
      emb_func_init_b.fsm_init = PROPERTY_DISABLE;
    }
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B,
                             (uint8_t*)&emb_func_en_b, 1);
    if (ret == 0) {
      ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B,
                               (uint8_t*)&emb_func_init_b, 1);
    }
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Final State Machine enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      union of registers from FSM_ENABLE_A to FSM_ENABLE_B
  *
  */
int32_t lsm6dsox_fsm_enable_get(lsm6dsox_ctx_t *ctx,
                               lsm6dsox_emb_fsm_enable_t *val)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_ENABLE_A, (uint8_t*) val, 2);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  FSM long counter status register. Long counter value is an
  *         unsigned integer value (16-bit format).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_long_cnt_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FSM_LONG_COUNTER_L, buff, 2);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  FSM long counter status register. Long counter value is an
  *         unsigned integer value (16-bit format).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_long_cnt_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_LONG_COUNTER_L, buff, 2);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Clear FSM long counter value.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fsm_lc_clr in
  *                  reg FSM_LONG_COUNTER_CLEAR
  *
  */
int32_t lsm6dsox_long_clr_set(lsm6dsox_ctx_t *ctx, lsm6dsox_fsm_lc_clr_t val)
{
  lsm6dsox_fsm_long_counter_clear_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_LONG_COUNTER_CLEAR,
    (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg. fsm_lc_clr = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_FSM_LONG_COUNTER_CLEAR,
    (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Clear FSM long counter value.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fsm_lc_clr in
  *                  reg FSM_LONG_COUNTER_CLEAR
  *
  */
int32_t lsm6dsox_long_clr_get(lsm6dsox_ctx_t *ctx, lsm6dsox_fsm_lc_clr_t *val)
{
  lsm6dsox_fsm_long_counter_clear_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_LONG_COUNTER_CLEAR,
    (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    switch (reg.fsm_lc_clr) {
      case LSM6DSOX_LC_NORMAL:
        *val = LSM6DSOX_LC_NORMAL;
        break;
      case LSM6DSOX_LC_CLEAR:
        *val = LSM6DSOX_LC_CLEAR;
        break;
      case LSM6DSOX_LC_CLEAR_DONE:
        *val = LSM6DSOX_LC_CLEAR_DONE;
        break;
      default:
        *val = LSM6DSOX_LC_NORMAL;
        break;
    }
  }

  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  FSM output registers[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      struct of registers from FSM_OUTS1 to FSM_OUTS16
  *
  */
int32_t lsm6dsox_fsm_out_get(lsm6dsox_ctx_t *ctx, lsm6dsox_fsm_out_t *val)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_FSM_OUTS1, (uint8_t*) &val, 16);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Finite State Machine ODR configuration.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fsm_odr in reg EMB_FUNC_ODR_CFG_B
  *
  */
int32_t lsm6dsox_fsm_data_rate_set(lsm6dsox_ctx_t *ctx, lsm6dsox_fsm_odr_t val)
{
  lsm6dsox_emb_func_odr_cfg_b_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_ODR_CFG_B,
                           (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.not_used_01 = 3; /* set default values */
    reg.not_used_02 = 1; /* set default values */
    reg.fsm_odr = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_ODR_CFG_B,
                            (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Finite State Machine ODR configuration.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fsm_odr in reg EMB_FUNC_ODR_CFG_B
  *
  */
int32_t lsm6dsox_fsm_data_rate_get(lsm6dsox_ctx_t *ctx, lsm6dsox_fsm_odr_t *val)
{
  lsm6dsox_emb_func_odr_cfg_b_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_ODR_CFG_B,
                           (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    switch (reg.fsm_odr) {
      case LSM6DSOX_ODR_FSM_12Hz5:
        *val = LSM6DSOX_ODR_FSM_12Hz5;
        break;
      case LSM6DSOX_ODR_FSM_26Hz:
        *val = LSM6DSOX_ODR_FSM_26Hz;
        break;
      case LSM6DSOX_ODR_FSM_52Hz:
        *val = LSM6DSOX_ODR_FSM_52Hz;
        break;
      case LSM6DSOX_ODR_FSM_104Hz:
        *val = LSM6DSOX_ODR_FSM_104Hz;
        break;
      default:
        *val = LSM6DSOX_ODR_FSM_12Hz5;
        break;
    }
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  FSM initialization request.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fsm_init in reg FSM_INIT
  *
  */
int32_t lsm6dsox_fsm_init_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_emb_func_init_b_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.fsm_init = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  FSM initialization request.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fsm_init in reg FSM_INIT
  *
  */
int32_t lsm6dsox_fsm_init_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_init_b_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.fsm_init;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  FSM long counter timeout register (r/w). The long counter
  *         timeout value is an unsigned integer value (16-bit format).
  *         When the long counter value reached this value,
  *         the FSM generates an interrupt.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_long_cnt_int_value_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_FSM_LC_TIMEOUT_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_FSM_LC_TIMEOUT_H,
                                   &buff[index]);
  }

  return ret;
}

/**
  * @brief  FSM long counter timeout register (r/w). The long counter
  *         timeout value is an unsigned integer value (16-bit format).
  *         When the long counter value reached this value,
  *         the FSM generates an interrupt.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_long_cnt_int_value_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_FSM_LC_TIMEOUT_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_FSM_LC_TIMEOUT_H,
                                  &buff[index]);
  }

  return ret;
}

/**
  * @brief  FSM number of programs register.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_fsm_number_of_programs_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;

  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_FSM_PROGRAMS, buff);

  return ret;
}

/**
  * @brief  FSM number of programs register.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_fsm_number_of_programs_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;

  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_FSM_PROGRAMS, buff);

  return ret;
}

/**
  * @brief  FSM start address register (r/w).
  *         First available address is 0x033C.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  *
  */
int32_t lsm6dsox_fsm_start_address_set(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_FSM_START_ADD_L, &buff[index]);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_write_byte(ctx, LSM6DSOX_FSM_START_ADD_H,
                                   &buff[index]);
  }
  return ret;
}

/**
  * @brief  FSM start address register (r/w).
  *         First available address is 0x033C.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  *
  */
int32_t lsm6dsox_fsm_start_address_get(lsm6dsox_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  uint8_t index;

  index = 0x00U;
  ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_FSM_START_ADD_L, buff);
  if (ret == 0) {
    index++;
    ret = lsm6dsox_ln_pg_read_byte(ctx, LSM6DSOX_FSM_START_ADD_H, buff);
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @addtogroup  Machine Learning Core
  * @brief   This section group all the functions concerning the
  *          usage of Machine Learning Core
  * @{
  *
  */

/**
  * @brief  Enable Machine Learning Core.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of mlc_en in
  *                  reg EMB_FUNC_EN_B and progsens_init
  *                  in EMB_FUNC_INIT_B
  *
  */
int32_t lsm6dsox_mlc_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_emb_func_en_b_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.mlc_en = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B, (uint8_t*)&reg, 1);
  }
  if ((val != PROPERTY_DISABLE) && (ret == 0)){
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B,
                                 (uint8_t*)&reg, 1);
    if (ret == 0) {
      reg.mlc_en = val;
      ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_INIT_B,
                                  (uint8_t*)&reg, 1);
    }
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Enable Machine Learning Core.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of mlc_en in
  *                  reg EMB_FUNC_EN_B
  *
  */
int32_t lsm6dsox_mlc_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_emb_func_en_b_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_EN_B, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
    *val  = reg.mlc_en;
  }
  return ret;
}

/**
  * @brief  Machine Learning Core status register[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      register PROGSENS_STATUS_MAINPAGE
  *
  */
int32_t lsm6dsox_mlc_status_get(lsm6dsox_ctx_t *ctx,
                                    lsm6dsox_progsens_status_mainpage_t *val)
{
  return lsm6dsox_read_reg(ctx, LSM6DSOX_PROGSENS_STATUS_MAINPAGE,
                           (uint8_t*) val, 1);
}

/**
  * @brief  Machine Learning Core data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      get the values of progsens_odr in
  *                  reg EMB_FUNC_ODR_CFG_C
  *
  */
int32_t lsm6dsox_mlc_data_rate_set(lsm6dsox_ctx_t *ctx,
                                       lsm6dsox_mlc_odr_t val)
{
  lsm6dsox_emb_func_odr_cfg_c_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_ODR_CFG_C,
                            (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.mlc_odr = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_EMB_FUNC_ODR_CFG_C, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Machine Learning Core data rate selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of progsens_odr in
  *                  reg EMB_FUNC_ODR_CFG_C
  *
  */
int32_t lsm6dsox_mlc_data_rate_get(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_mlc_odr_t *val)
{
  lsm6dsox_emb_func_odr_cfg_c_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_EMBEDDED_FUNC_BANK);
  if (ret == 0) {  
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_EMB_FUNC_ODR_CFG_C, 
                            (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    switch (reg.mlc_odr) {
      case LSM6DSOX_ODR_PRGS_12Hz5:
        *val = LSM6DSOX_ODR_PRGS_12Hz5;
        break;
      case LSM6DSOX_ODR_PRGS_26Hz:
        *val = LSM6DSOX_ODR_PRGS_26Hz;
        break;
      case LSM6DSOX_ODR_PRGS_52Hz:
        *val = LSM6DSOX_ODR_PRGS_52Hz;
        break;
      case LSM6DSOX_ODR_PRGS_104Hz:
        *val = LSM6DSOX_ODR_PRGS_104Hz;
        break;
      default:
        *val = LSM6DSOX_ODR_PRGS_12Hz5;
        break;
    }
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LSM6DSOX_Sensor_hub
  * @brief     This section groups all the functions that manage the
  *            sensor hub.
  * @{
  *
  */

/**
* @brief  Sensor hub output registers.[get]
*
* @param  ctx      read / write interface definitions
* @param  val      union of registers from SENSOR_HUB_1 to SENSOR_HUB_18
*
  */
int32_t lsm6dsox_sh_read_data_raw_get(lsm6dsox_ctx_t *ctx,
                                     lsm6dsox_emb_sh_read_t *val)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SENSOR_HUB_1, (uint8_t*) val, 18U);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Number of external sensors to be read by the sensor hub.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of aux_sens_on in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_slave_connected_set(lsm6dsox_ctx_t *ctx,
                                       lsm6dsox_aux_sens_on_t val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.aux_sens_on = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Number of external sensors to be read by the sensor hub.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of aux_sens_on in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_slave_connected_get(lsm6dsox_ctx_t *ctx,
                                       lsm6dsox_aux_sens_on_t *val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    switch (reg.aux_sens_on) {
      case LSM6DSOX_SLV_0:
        *val = LSM6DSOX_SLV_0;
        break;
      case LSM6DSOX_SLV_0_1:
        *val = LSM6DSOX_SLV_0_1;
        break;
      case LSM6DSOX_SLV_0_1_2:
        *val = LSM6DSOX_SLV_0_1_2;
        break;
      case LSM6DSOX_SLV_0_1_2_3:
        *val = LSM6DSOX_SLV_0_1_2_3;
        break;
      default:
        *val = LSM6DSOX_SLV_0;
        break;
    }
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Sensor hub I2C master enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of master_on in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_master_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.master_on = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Sensor hub I2C master enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of master_on in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_master_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.master_on;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Master I2C pull-up enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of shub_pu_en in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_pin_mode_set(lsm6dsox_ctx_t *ctx, lsm6dsox_shub_pu_en_t val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.shub_pu_en = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Master I2C pull-up enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of shub_pu_en in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_pin_mode_get(lsm6dsox_ctx_t *ctx,
                                lsm6dsox_shub_pu_en_t *val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    switch (reg.shub_pu_en) {
      case LSM6DSOX_EXT_PULL_UP:
        *val = LSM6DSOX_EXT_PULL_UP;
        break;
      case LSM6DSOX_INTERNAL_PULL_UP:
        *val = LSM6DSOX_INTERNAL_PULL_UP;
        break;
      default:
        *val = LSM6DSOX_EXT_PULL_UP;
        break;
    }
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  I2C interface pass-through.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of pass_through_mode in
  *                  reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_pass_through_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.pass_through_mode = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  I2C interface pass-through.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of pass_through_mode in
  *                  reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_pass_through_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.pass_through_mode;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Sensor hub trigger signal selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of start_config in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_syncro_mode_set(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_start_config_t val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.start_config = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Sensor hub trigger signal selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of start_config in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_syncro_mode_get(lsm6dsox_ctx_t *ctx,
                                   lsm6dsox_start_config_t *val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    switch (reg.start_config) {
      case LSM6DSOX_EXT_ON_INT2_PIN:
        *val = LSM6DSOX_EXT_ON_INT2_PIN;
        break;
      case LSM6DSOX_XL_GY_DRDY:
        *val = LSM6DSOX_XL_GY_DRDY;
        break;
      default:
        *val = LSM6DSOX_EXT_ON_INT2_PIN;
        break;
    }
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Slave 0 write operation is performed only at the first
  *         sensor hub cycle.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of write_once in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_write_mode_set(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_write_once_t val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.write_once = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Slave 0 write operation is performed only at the first sensor
  *         hub cycle.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of write_once in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_write_mode_get(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_write_once_t *val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    switch (reg.write_once) {
      case LSM6DSOX_EACH_SH_CYCLE:
        *val = LSM6DSOX_EACH_SH_CYCLE;
        break;
      case LSM6DSOX_ONLY_FIRST_CYCLE:
        *val = LSM6DSOX_ONLY_FIRST_CYCLE;
        break;
      default:
        *val = LSM6DSOX_EACH_SH_CYCLE;
        break;
    }
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Reset Master logic and output registers.[set]
  *
  * @param  ctx      read / write interface definitions
  *
  */
int32_t lsm6dsox_sh_reset_set(lsm6dsox_ctx_t *ctx)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.rst_master_regs = PROPERTY_ENABLE;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.rst_master_regs = PROPERTY_DISABLE;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Reset Master logic and output registers.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of rst_master_regs in reg MASTER_CONFIG
  *
  */
int32_t lsm6dsox_sh_reset_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_master_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_MASTER_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    *val = reg.rst_master_regs;
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Rate at which the master communicates.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of shub_odr in reg slv1_CONFIG
  *
  */
int32_t lsm6dsox_sh_data_rate_set(lsm6dsox_ctx_t *ctx, lsm6dsox_shub_odr_t val)
{
  lsm6dsox_slv0_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV1_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    reg.shub_odr = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV1_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Rate at which the master communicates.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of shub_odr in reg slv1_CONFIG
  *
  */
int32_t lsm6dsox_sh_data_rate_get(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_shub_odr_t *val)
{
  lsm6dsox_slv0_config_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV1_CONFIG, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    switch (reg.shub_odr) {
      case LSM6DSOX_SH_ODR_104Hz:
        *val = LSM6DSOX_SH_ODR_104Hz;
        break;
      case LSM6DSOX_SH_ODR_52Hz:
        *val = LSM6DSOX_SH_ODR_52Hz;
        break;
      case LSM6DSOX_SH_ODR_26Hz:
        *val = LSM6DSOX_SH_ODR_26Hz;
        break;
      case LSM6DSOX_SH_ODR_13Hz:
        *val = LSM6DSOX_SH_ODR_13Hz;
        break;
      default:
        *val = LSM6DSOX_SH_ODR_104Hz;
        break;
    }
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Configure slave 0 for perform a write.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      a structure that contain
  *                      - uint8_t slv1_add;    8 bit i2c device address
  *                      - uint8_t slv1_subadd; 8 bit register device address
  *                      - uint8_t slv1_data;   8 bit data to write
  *
  */
int32_t lsm6dsox_sh_cfg_write(lsm6dsox_ctx_t *ctx, lsm6dsox_sh_cfg_write_t *val)
{
  lsm6dsox_slv0_add_t reg;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    reg.slave0 = val->slv0_add;
    reg.rw_0 = 0;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV0_ADD, (uint8_t*)&reg, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV0_SUBADD,
    &(val->slv0_subadd), 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_DATAWRITE_SLV0,
    &(val->slv0_data), 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Configure slave 0 for perform a read.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Structure that contain
  *                      - uint8_t slv1_add;    8 bit i2c device address
  *                      - uint8_t slv1_subadd; 8 bit register device address
  *                      - uint8_t slv1_len;    num of bit to read
  *
  */
int32_t lsm6dsox_sh_slv0_cfg_read(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_sh_cfg_read_t *val)
{
  lsm6dsox_slv0_add_t slv0_add;
  lsm6dsox_slv0_config_t slv0_config;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    slv0_add.slave0 = val->slv_add;
    slv0_add.rw_0 = 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV0_ADD, (uint8_t*)&slv0_add, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV0_SUBADD,
    &(val->slv_subadd), 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV0_CONFIG,
                           (uint8_t*)&slv0_config, 1);
  }
  if (ret == 0) {
    slv0_config.slave0_numop = val->slv_len;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV0_CONFIG,
                            (uint8_t*)&slv0_config, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Configure slave 0 for perform a write/read.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Structure that contain
  *                      - uint8_t slv1_add;    8 bit i2c device address
  *                      - uint8_t slv1_subadd; 8 bit register device address
  *                      - uint8_t slv1_len;    num of bit to read
  *
  */
int32_t lsm6dsox_sh_slv1_cfg_read(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_sh_cfg_read_t *val)
{
  lsm6dsox_slv1_add_t slv1_add;
  lsm6dsox_slv1_config_t slv1_config;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    slv1_add.slave1_add = val->slv_add;
    slv1_add.r_1 = 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV1_ADD, (uint8_t*)&slv1_add, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV1_SUBADD,
    &(val->slv_subadd), 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV1_CONFIG,
                           (uint8_t*)&slv1_config, 1);
  }
  if (ret == 0) {
    slv1_config.slave1_numop = val->slv_len;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV1_CONFIG,
                            (uint8_t*)&slv1_config, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @brief  Configure slave 0 for perform a write/read.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Structure that contain
  *                      - uint8_t slv2_add;    8 bit i2c device address
  *                      - uint8_t slv2_subadd; 8 bit register device address
  *                      - uint8_t slv2_len;    num of bit to read
  *
  */
int32_t lsm6dsox_sh_slv2_cfg_read(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_sh_cfg_read_t *val)
{
  lsm6dsox_slv2_add_t slv2_add;
  lsm6dsox_slv2_config_t slv2_config;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    slv2_add.slave2_add = val->slv_add;
    slv2_add.r_2 = 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV2_ADD, (uint8_t*)&slv2_add, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV2_SUBADD,
    &(val->slv_subadd), 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV2_CONFIG,
                           (uint8_t*)&slv2_config, 1);
  }
  if (ret == 0) {
    slv2_config.slave2_numop = val->slv_len;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV2_CONFIG,
                            (uint8_t*)&slv2_config, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief Configure slave 0 for perform a write/read.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Structure that contain
  *                      - uint8_t slv3_add;    8 bit i2c device address
  *                      - uint8_t slv3_subadd; 8 bit register device address
  *                      - uint8_t slv3_len;    num of bit to read
  *
  */
int32_t lsm6dsox_sh_slv3_cfg_read(lsm6dsox_ctx_t *ctx,
                                 lsm6dsox_sh_cfg_read_t *val)
{
  lsm6dsox_slv3_add_t slv3_add;
  lsm6dsox_slv3_config_t slv3_config;
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    slv3_add.slave3_add = val->slv_add;
    slv3_add.r_3 = 1;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV3_ADD, (uint8_t*)&slv3_add, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV3_SUBADD,
    &(val->slv_subadd), 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_SLV3_CONFIG,
                           (uint8_t*)&slv3_config, 1);
  }
  if (ret == 0) {
    slv3_config.slave3_numop = val->slv_len;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_SLV3_CONFIG,
                            (uint8_t*)&slv3_config, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }
  return ret;
}

/**
  * @brief  Sensor hub source register.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      union of registers from STATUS_MASTER to
  *
  */
int32_t lsm6dsox_sh_status_get(lsm6dsox_ctx_t *ctx,
                              lsm6dsox_status_master_t *val)
{
  int32_t ret;

  ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_SENSOR_HUB_BANK);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_STATUS_MASTER, (uint8_t*) val, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_mem_bank_set(ctx, LSM6DSOX_USER_BANK);
  }

  return ret;
}

/**
  * @}
  *
  */

  /**
  * @addtogroup  Sensors for Smart Mobile Devices
  * @brief   This section groups all the functions that manage the
  *          Sensors for Smart Mobile Devices.
  * @{
  *
  */

/**
  * @brief  s4s_tph_res: [set] Sensor synchronization time frame resolution
  *
  * @param  *ctx   read / write interface definitions
  * @param  val    change the values of tph_h_sel in LSM6DSOX_S4S_TPH_L
  *
  */
int32_t lsm6dsox_s4s_tph_res_set(lsm6dsox_ctx_t *ctx,
                                lsm6dsox_s4s_tph_res_t val)
{
  lsm6dsox_s4s_tph_l_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_TPH_L, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.tph_h_sel = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_S4S_TPH_L, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  s4s_tph_res: [get] Sensor synchronization time frame resolution
  *
  * @param  *ctx   read / write interface definitions
  * @param  val    get the values of tph_h_sel in LSM6DSOX_S4S_TPH_L
  *
  */
int32_t lsm6dsox_s4s_tph_res_get(lsm6dsox_ctx_t *ctx,
                                lsm6dsox_s4s_tph_res_t *val)
{
  lsm6dsox_s4s_tph_l_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_TPH_L, (uint8_t*)&reg, 1);
  switch (reg.tph_h_sel) {
      case LSM6DSOX_S4S_TPH_7bit:
        *val = LSM6DSOX_S4S_TPH_7bit;
        break;
      case LSM6DSOX_S4S_TPH_15bit:
        *val = LSM6DSOX_S4S_TPH_15bit;
        break;
      default:
        *val = LSM6DSOX_S4S_TPH_7bit;
        break;
    }

  return ret;
}

/**
  * @brief  s4s_tph_val: [set] Sensor synchronization time frame
  *
  * @param  *ctx   read / write interface definitions
  * @param  val    change the values of tph_l in S4S_TPH_L and
  *                tph_h in S4S_TPH_H
  *
  */
int32_t lsm6dsox_s4s_tph_val_set(lsm6dsox_ctx_t *ctx, uint16_t val)
{
  lsm6dsox_s4s_tph_l_t s4s_tph_l;
  lsm6dsox_s4s_tph_h_t s4s_tph_h;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_TPH_L, (uint8_t*)&s4s_tph_l, 1);
  if (ret == 0) {
    s4s_tph_l.tph_l = (uint8_t)(val & 0x007FU);
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_S4S_TPH_L, (uint8_t*)&s4s_tph_l, 1);
  }
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_TPH_H, (uint8_t*)&s4s_tph_h, 1);
    s4s_tph_h.tph_h = (uint8_t)(val & 0x7F80U) >> 7;
  }
  if (ret == 0) {
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_S4S_TPH_H, (uint8_t*)&s4s_tph_h, 1);
  }
  return ret;
}

/**
  * @brief  s4s_tph_val: [get] Sensor synchronization time frame.
  *
  * @param  *ctx   read / write interface definitions
  * @param  val    get the values of tph_l in S4S_TPH_L and
  *                tph_h in S4S_TPH_H
  *
  */
int32_t lsm6dsox_s4s_tph_val_get(lsm6dsox_ctx_t *ctx, uint16_t *val)
{
  lsm6dsox_s4s_tph_l_t s4s_tph_l;
  lsm6dsox_s4s_tph_h_t s4s_tph_h;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_TPH_L, (uint8_t*)&s4s_tph_l, 1);
  if (ret == 0) {
    ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_TPH_H, (uint8_t*)&s4s_tph_h, 1);
    *val =  s4s_tph_h.tph_h;
    *val =  *val << 7;
    *val += s4s_tph_l.tph_l;
  }
  return ret;
}

/**
  * @brief  s4s_res_ratio: [set]Sensor synchronization resolution
  *                        ratio register.
  *
  * @param  *ctx   read / write interface definitions.
  * @param  val    change the values of rr in S4S_RR.
  *
  */
int32_t lsm6dsox_s4s_res_ratio_set(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_s4s_res_ratio_t val)
{
  lsm6dsox_s4s_rr_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_RR, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.rr = (uint8_t)val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_S4S_RR, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  s4s_res_ratio: [get]Sensor synchronization resolution
  *                        ratio register.
  *
  * @param  *ctx   read / write interface definitions
  * @param  val    get the values of rr in S4S_RR
  *
  */
int32_t lsm6dsox_s4s_res_ratio_get(lsm6dsox_ctx_t *ctx,
                                  lsm6dsox_s4s_res_ratio_t *val)
{
  lsm6dsox_s4s_rr_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_RR, (uint8_t*)&reg, 1);
  switch (reg.rr) {
    case LSM6DSOX_S4S_DT_RES_11:
      *val = LSM6DSOX_S4S_DT_RES_11;
      break;
    case LSM6DSOX_S4S_DT_RES_12:
      *val = LSM6DSOX_S4S_DT_RES_12;
      break;
    case LSM6DSOX_S4S_DT_RES_13:
      *val = LSM6DSOX_S4S_DT_RES_13;
      break;
    case LSM6DSOX_S4S_DT_RES_14:
      *val = LSM6DSOX_S4S_DT_RES_14;
      break;
    default:
      *val = LSM6DSOX_S4S_DT_RES_11;
      break;
  }
  return ret;
}

/**
  * @brief  s4s_command: [set] s4s master command.
  *
  * @param  *ctx   read / write interface definitions.
  * @param  val    change the values of S4S_ST_CMD_CODE.
  *
  */
int32_t lsm6dsox_s4s_command_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_s4s_st_cmd_code_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_ST_CMD_CODE, (uint8_t*)&reg, 1);
 
  if (ret == 0) {
    reg.s4s_st_cmd_code = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_S4S_ST_CMD_CODE, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  s4s_command: [get] s4s master command.
  *
  * @param  *ctx   read / write interface definitions.
  * @param  val    get the values of S4S_ST_CMD_CODE.
  *
  */
int32_t lsm6dsox_s4s_command_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_s4s_st_cmd_code_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_ST_CMD_CODE, (uint8_t*)&reg, 1);
  *val = reg.s4s_st_cmd_code;

  return ret;
}

/**
  * @brief  s4s_dt: [set] S4S DT register.
  *
  * @param  *ctx   read / write interface definitions.
  * @param  val    change the values of S4S_DT_REG.
  *
  */
int32_t lsm6dsox_s4s_dt_set(lsm6dsox_ctx_t *ctx, uint8_t val)
{
  lsm6dsox_s4s_dt_reg_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_DT_REG, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.dt = val;
    ret = lsm6dsox_write_reg(ctx, LSM6DSOX_S4S_DT_REG, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  s4s_dt: [get] S4S DT register.
  *
  * @param  *ctx   read / write interface definitions.
  * @param  val    get the values of S4S_DT_REG.
  *
  */
int32_t lsm6dsox_s4s_dt_get(lsm6dsox_ctx_t *ctx, uint8_t *val)
{
  lsm6dsox_s4s_dt_reg_t reg;
  int32_t ret;

  ret = lsm6dsox_read_reg(ctx, LSM6DSOX_S4S_DT_REG, (uint8_t*)&reg, 1);
  *val = reg.dt;

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
