/*
 ******************************************************************************
 * @file    lis2dw12_reg.c
 * @author  Sensors Software Solution Team
 * @brief   LIS2DW12 driver file
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

#include "lis2dw12_reg.h"

/**
  * @defgroup  LIS2DW12
  * @brief     This file provides a set of functions needed to drive the
  *            lis2dw12 enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup  LIS2DW12_Interfaces_Functions
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
int32_t lis2dw12_read_reg(lis2dw12_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lis2dw12_write_reg(lis2dw12_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LIS2DW12_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t lis2dw12_from_fs2_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.061f;
}

float_t lis2dw12_from_fs4_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.122f;
}

float_t lis2dw12_from_fs8_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.244f;
}

float_t lis2dw12_from_fs16_to_mg(int16_t lsb)
{
  return ((float_t)lsb) *0.488f;
}

float_t lis2dw12_from_fs2_lp1_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.061f;
}

float_t lis2dw12_from_fs4_lp1_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.122f;
}

float_t lis2dw12_from_fs8_lp1_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.244f;
}

float_t lis2dw12_from_fs16_lp1_to_mg(int16_t lsb)
{
  return ((float_t)lsb) * 0.488f;
}

float_t lis2dw12_from_lsb_to_celsius(int16_t lsb)
{
  return (((float_t)lsb / 16.0f) + 25.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Data_Generation
  * @brief     This section groups all the functions concerning
  *            data generation
  * @{
  *
  */

/**
  * @brief  Select accelerometer operating modes.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of mode / lp_mode in reg CTRL1
  *                  and low_noise in reg CTRL6
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_power_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_mode_t val)
{
  lis2dw12_ctrl1_t ctrl1;
  lis2dw12_ctrl6_t ctrl6;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL1,(uint8_t*) &ctrl1, 1);
  if (ret == 0) {
    ctrl1.mode = ( (uint8_t) val & 0x0CU ) >> 2;
    ctrl1.lp_mode = (uint8_t) val & 0x03U ;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL1,(uint8_t*) &ctrl1, 1);
  }
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &ctrl6, 1);
  }
  if (ret == 0) {
    ctrl6.low_noise = ( (uint8_t) val & 0x10U ) >> 4;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &ctrl6, 1);
  }
  return ret;
}

/**
  * @brief  Select accelerometer operating modes.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of mode / lp_mode in reg CTRL1
  *                  and low_noise in reg CTRL6
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_power_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_mode_t *val)
{
  lis2dw12_ctrl1_t ctrl1;
  lis2dw12_ctrl6_t ctrl6;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL1,(uint8_t*) &ctrl1, 1);
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &ctrl6, 1);

    switch (((ctrl6.low_noise << 4) + (ctrl1.mode << 2) +
            ctrl1.lp_mode)) {
      case LIS2DW12_HIGH_PERFORMANCE:
        *val = LIS2DW12_HIGH_PERFORMANCE;
        break;
      case LIS2DW12_CONT_LOW_PWR_4:
        *val = LIS2DW12_CONT_LOW_PWR_4;
        break;
      case LIS2DW12_CONT_LOW_PWR_3:
        *val = LIS2DW12_CONT_LOW_PWR_3;
        break;
      case LIS2DW12_CONT_LOW_PWR_2:
        *val = LIS2DW12_CONT_LOW_PWR_2;
        break;
      case LIS2DW12_CONT_LOW_PWR_12bit:
        *val = LIS2DW12_CONT_LOW_PWR_12bit;
        break;
      case LIS2DW12_SINGLE_LOW_PWR_4:
        *val = LIS2DW12_SINGLE_LOW_PWR_4;
        break;
      case LIS2DW12_SINGLE_LOW_PWR_3:
        *val = LIS2DW12_SINGLE_LOW_PWR_3;
        break;
      case LIS2DW12_SINGLE_LOW_PWR_2:
        *val = LIS2DW12_SINGLE_LOW_PWR_2;
        break;
      case LIS2DW12_SINGLE_LOW_PWR_12bit:
        *val = LIS2DW12_SINGLE_LOW_PWR_12bit;
        break;
      case LIS2DW12_HIGH_PERFORMANCE_LOW_NOISE:
        *val = LIS2DW12_HIGH_PERFORMANCE_LOW_NOISE;
        break;
      case LIS2DW12_CONT_LOW_PWR_LOW_NOISE_4:
        *val = LIS2DW12_CONT_LOW_PWR_LOW_NOISE_4;
        break;
      case LIS2DW12_CONT_LOW_PWR_LOW_NOISE_3:
        *val = LIS2DW12_CONT_LOW_PWR_LOW_NOISE_3;
        break;
      case LIS2DW12_CONT_LOW_PWR_LOW_NOISE_2:
        *val = LIS2DW12_CONT_LOW_PWR_LOW_NOISE_2;
        break;
      case LIS2DW12_CONT_LOW_PWR_LOW_NOISE_12bit:
        *val = LIS2DW12_CONT_LOW_PWR_LOW_NOISE_12bit;
        break;
      case LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_4:
        *val = LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_4;
        break;
      case LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_3:
        *val = LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_3;
        break;
      case LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_2:
        *val = LIS2DW12_SINGLE_LOW_PWR_LOW_NOISE_2;
        break;
      case LIS2DW12_SINGLE_LOW_LOW_NOISE_PWR_12bit:
        *val = LIS2DW12_SINGLE_LOW_LOW_NOISE_PWR_12bit;
        break;
      default:
        *val = LIS2DW12_HIGH_PERFORMANCE;
        break;
    }
  }
  return ret;
}

/**
  * @brief  Accelerometer data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odr in reg CTRL1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_data_rate_set(lis2dw12_ctx_t *ctx, lis2dw12_odr_t val)
{
  lis2dw12_ctrl1_t ctrl1;
  lis2dw12_ctrl3_t ctrl3;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL1,(uint8_t*) &ctrl1, 1);
  if (ret == 0) {
    ctrl1.odr = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL1,(uint8_t*) &ctrl1, 1);
  }
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &ctrl3, 1);
  }
  if (ret == 0) {
    ctrl3.slp_mode = ( (uint8_t) val & 0x30U ) >> 4;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &ctrl3, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer data rate selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of odr in reg CTRL1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_data_rate_get(lis2dw12_ctx_t *ctx, lis2dw12_odr_t *val)
{
  lis2dw12_ctrl1_t ctrl1;
  lis2dw12_ctrl3_t ctrl3;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL1,(uint8_t*) &ctrl1, 1);
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &ctrl3, 1);

    switch ((ctrl3.slp_mode << 4) + ctrl1.odr) {
      case LIS2DW12_XL_ODR_OFF:
        *val = LIS2DW12_XL_ODR_OFF;
        break;
      case LIS2DW12_XL_ODR_1Hz6_LP_ONLY:
        *val = LIS2DW12_XL_ODR_1Hz6_LP_ONLY;
        break;
      case LIS2DW12_XL_ODR_12Hz5:
        *val = LIS2DW12_XL_ODR_12Hz5;
        break;
      case LIS2DW12_XL_ODR_25Hz:
        *val = LIS2DW12_XL_ODR_25Hz;
        break;
       case LIS2DW12_XL_ODR_50Hz:
        *val = LIS2DW12_XL_ODR_50Hz;
        break;
      case LIS2DW12_XL_ODR_100Hz:
        *val = LIS2DW12_XL_ODR_100Hz;
        break;
      case LIS2DW12_XL_ODR_200Hz:
        *val = LIS2DW12_XL_ODR_200Hz;
        break;
      case LIS2DW12_XL_ODR_400Hz:
        *val = LIS2DW12_XL_ODR_400Hz;
        break;
       case LIS2DW12_XL_ODR_800Hz:
        *val = LIS2DW12_XL_ODR_800Hz;
        break;
      case LIS2DW12_XL_ODR_1k6Hz:
        *val = LIS2DW12_XL_ODR_1k6Hz;
        break;
      case LIS2DW12_XL_SET_SW_TRIG:
        *val = LIS2DW12_XL_SET_SW_TRIG;
        break;
      case LIS2DW12_XL_SET_PIN_TRIG:
        *val = LIS2DW12_XL_SET_PIN_TRIG;
        break;
      default:
        *val = LIS2DW12_XL_ODR_OFF;
        break;
    }
  }
  return ret;
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdu in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_block_data_update_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.bdu = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Block data update.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdu in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_block_data_update_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  *val = reg.bdu;

  return ret;
}

/**
  * @brief  Accelerometer full-scale selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fs in reg CTRL6
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_full_scale_set(lis2dw12_ctx_t *ctx, lis2dw12_fs_t val)
{
  lis2dw12_ctrl6_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.fs = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer full-scale selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fs in reg CTRL6
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_full_scale_get(lis2dw12_ctx_t *ctx, lis2dw12_fs_t *val)
{
  lis2dw12_ctrl6_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &reg, 1);

  switch (reg.fs) {
    case LIS2DW12_2g:
      *val = LIS2DW12_2g;
      break;
    case LIS2DW12_4g:
      *val = LIS2DW12_4g;
      break;
    case LIS2DW12_8g:
      *val = LIS2DW12_8g;
      break;
    case LIS2DW12_16g:
      *val = LIS2DW12_16g;
      break;
    default:
      *val = LIS2DW12_2g;
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
int32_t lis2dw12_status_reg_get(lis2dw12_ctx_t *ctx, lis2dw12_status_t *val)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_STATUS, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Accelerometer new data available.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of drdy in reg STATUS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_flag_data_ready_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_status_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_STATUS,(uint8_t*) &reg, 1);
  *val = reg.drdy;

  return ret;
}
/**
  * @brief   Read all the interrupt/status flag of the device.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers STATUS_DUP, WAKE_UP_SRC,
  *                  TAP_SRC, SIXD_SRC, ALL_INT_SRC
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_all_sources_get(lis2dw12_ctx_t *ctx,
                                 lis2dw12_all_sources_t *val)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_STATUS_DUP, (uint8_t*) val, 5);
  return ret;
}

/**
  * @brief  Accelerometer X-axis user offset correction expressed in two’s
  *         complement, weight depends on bit USR_OFF_W. The value must be
  *         in the range [-127 127].[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_usr_offset_x_set(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_write_reg(ctx, LIS2DW12_X_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer X-axis user offset correction expressed in two’s
  *         complement, weight depends on bit USR_OFF_W. The value must be
  *         in the range [-127 127].[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_usr_offset_x_get(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_X_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Y-axis user offset correction expressed in two’s
  *         complement, weight depends on bit USR_OFF_W. The value must be
  *         in the range [-127 127].[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_usr_offset_y_set(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_write_reg(ctx, LIS2DW12_Y_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Y-axis user offset correction expressed in two’s
  *         complement, weight depends on bit USR_OFF_W. The value must be
  *         in the range [-127 127].[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_usr_offset_y_get(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_Y_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Z-axis user offset correction expressed in two’s
  *         complement, weight depends on bit USR_OFF_W. The value must be
  *         in the range [-127 127].[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_usr_offset_z_set(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_write_reg(ctx, LIS2DW12_Z_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Z-axis user offset correction expressed in two’s
  *         complement, weight depends on bit USR_OFF_W. The value must be
  *         in the range [-127 127].[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_usr_offset_z_get(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_Z_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Weight of XL user offset bits of registers X_OFS_USR,
  *         Y_OFS_USR, Z_OFS_USR.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of usr_off_w in
  *                               reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_offset_weight_set(lis2dw12_ctx_t *ctx,
                                   lis2dw12_usr_off_w_t val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.usr_off_w = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Weight of XL user offset bits of registers X_OFS_USR,
  *         Y_OFS_USR, Z_OFS_USR.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of usr_off_w in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_offset_weight_get(lis2dw12_ctx_t *ctx,
                                   lis2dw12_usr_off_w_t *val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  switch (reg.usr_off_w) {
    case LIS2DW12_LSb_977ug:
      *val = LIS2DW12_LSb_977ug;
      break;
    case LIS2DW12_LSb_15mg6:
      *val = LIS2DW12_LSb_15mg6;
      break;
    default:
      *val = LIS2DW12_LSb_977ug;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Data_Output
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
int32_t lis2dw12_temperature_raw_get(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_OUT_T_L, buff, 2);
  return ret;
}

/**
  * @brief  Linear acceleration output register. The value is expressed as
  *         a 16-bit word in two’s complement.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_acceleration_raw_get(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_OUT_X_L, buff, 6);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Common
  * @brief     This section groups common useful functions.
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
int32_t lis2dw12_device_id_get(lis2dw12_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief  Register address automatically incremented during multiple byte
  *         access with a serial interface.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of if_add_inc in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_auto_increment_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.if_add_inc = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Register address automatically incremented during multiple
  *         byte access with a serial interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of if_add_inc in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_auto_increment_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  *val = reg.if_add_inc;

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of soft_reset in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_reset_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.soft_reset = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of soft_reset in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_reset_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  *val = reg.soft_reset;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of boot in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_boot_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.boot = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of boot in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_boot_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  *val = reg.boot;

  return ret;
}

/**
  * @brief  Sensor self-test enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of st in reg CTRL3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_self_test_set(lis2dw12_ctx_t *ctx, lis2dw12_st_t val)
{
  lis2dw12_ctrl3_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.st = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Sensor self-test enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of st in reg CTRL3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_self_test_get(lis2dw12_ctx_t *ctx, lis2dw12_st_t *val)
{
  lis2dw12_ctrl3_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);

  switch (reg.st) {
    case LIS2DW12_XL_ST_DISABLE:
      *val = LIS2DW12_XL_ST_DISABLE;
      break;
    case LIS2DW12_XL_ST_POSITIVE:
      *val = LIS2DW12_XL_ST_POSITIVE;
      break;
    case LIS2DW12_XL_ST_NEGATIVE:
      *val = LIS2DW12_XL_ST_NEGATIVE;
      break;
    default:
      *val = LIS2DW12_XL_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Data-ready pulsed / letched mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of drdy_pulsed in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_data_ready_mode_set(lis2dw12_ctx_t *ctx,
                                     lis2dw12_drdy_pulsed_t val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.drdy_pulsed = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Data-ready pulsed / letched mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of drdy_pulsed in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_data_ready_mode_get(lis2dw12_ctx_t *ctx,
                                     lis2dw12_drdy_pulsed_t *val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);

  switch (reg.drdy_pulsed) {
    case LIS2DW12_DRDY_LATCHED:
      *val = LIS2DW12_DRDY_LATCHED;
      break;
    case LIS2DW12_DRDY_PULSED:
      *val = LIS2DW12_DRDY_PULSED;
      break;
    default:
      *val = LIS2DW12_DRDY_LATCHED;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Filters
  * @brief     This section group all the functions concerning the filters
  *            configuration.
  * @{
  *
  */

/**
  * @brief  Accelerometer filtering path for outputs.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fds in reg CTRL6
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_filter_path_set(lis2dw12_ctx_t *ctx, lis2dw12_fds_t val)
{
  lis2dw12_ctrl6_t ctrl6;
  lis2dw12_ctrl_reg7_t ctrl_reg7;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &ctrl6, 1);
  if (ret == 0) {
    ctrl6.fds = ( (uint8_t) val & 0x10U ) >> 4;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &ctrl6, 1);
  }
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &ctrl_reg7, 1);
  }
  if (ret == 0) {
    ctrl_reg7.usr_off_on_out = (uint8_t) val & 0x01U;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &ctrl_reg7, 1);
  }

  return ret;
}

/**
  * @brief  Accelerometer filtering path for outputs.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fds in reg CTRL6
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_filter_path_get(lis2dw12_ctx_t *ctx, lis2dw12_fds_t *val)
{
  lis2dw12_ctrl6_t ctrl6;
  lis2dw12_ctrl_reg7_t ctrl_reg7;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &ctrl6, 1);
  if (ret == 0) {
   ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &ctrl_reg7, 1);

    switch ((ctrl6.fds << 4 ) + ctrl_reg7.usr_off_on_out) {
      case LIS2DW12_LPF_ON_OUT:
        *val = LIS2DW12_LPF_ON_OUT;
        break;
      case LIS2DW12_USER_OFFSET_ON_OUT:
        *val = LIS2DW12_USER_OFFSET_ON_OUT;
        break;
      case LIS2DW12_HIGH_PASS_ON_OUT:
        *val = LIS2DW12_HIGH_PASS_ON_OUT;
        break;
      default:
        *val = LIS2DW12_LPF_ON_OUT;
        break;
    }
  }
  return ret;
}

/**
  * @brief   Accelerometer cutoff filter frequency. Valid for low and high
  *          pass filter.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bw_filt in reg CTRL6
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_filter_bandwidth_set(lis2dw12_ctx_t *ctx,
                                         lis2dw12_bw_filt_t val)
{
  lis2dw12_ctrl6_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.bw_filt = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief   Accelerometer cutoff filter frequency. Valid for low and
  *          high pass filter.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of bw_filt in reg CTRL6
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_filter_bandwidth_get(lis2dw12_ctx_t *ctx,
                                         lis2dw12_bw_filt_t *val)
{
  lis2dw12_ctrl6_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6,(uint8_t*) &reg, 1);

  switch (reg.bw_filt) {
    case LIS2DW12_ODR_DIV_2:
      *val = LIS2DW12_ODR_DIV_2;
      break;
    case LIS2DW12_ODR_DIV_4:
      *val = LIS2DW12_ODR_DIV_4;
      break;
    case LIS2DW12_ODR_DIV_10:
      *val = LIS2DW12_ODR_DIV_10;
      break;
    case LIS2DW12_ODR_DIV_20:
      *val = LIS2DW12_ODR_DIV_20;
      break;
    default:
      *val = LIS2DW12_ODR_DIV_2;
      break;
  }
  return ret;
}

/**
  * @brief  Enable HP filter reference mode.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of hp_ref_mode in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_reference_mode_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.hp_ref_mode = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable HP filter reference mode.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of hp_ref_mode in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_reference_mode_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  *val = reg.hp_ref_mode;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   LIS2DW12_Serial_Interface
  * @brief      This section groups all the functions concerning main serial
  *             interface management (not auxiliary)
  * @{
  *
  */

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sim in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_spi_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_sim_t val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.sim = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sim in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_spi_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_sim_t *val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);

  switch (reg.sim) {
    case LIS2DW12_SPI_4_WIRE:
      *val = LIS2DW12_SPI_4_WIRE;
      break;
    case LIS2DW12_SPI_3_WIRE:
      *val = LIS2DW12_SPI_3_WIRE;
      break;
    default:
      *val = LIS2DW12_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief  Disable / Enable I2C interface.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of i2c_disable in
  *                                 reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_i2c_interface_set(lis2dw12_ctx_t *ctx,
                                   lis2dw12_i2c_disable_t val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.i2c_disable = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Disable / Enable I2C interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of i2c_disable in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_i2c_interface_get(lis2dw12_ctx_t *ctx,
                                   lis2dw12_i2c_disable_t *val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);

  switch (reg.i2c_disable) {
    case LIS2DW12_I2C_ENABLE:
      *val = LIS2DW12_I2C_ENABLE;
      break;
    case LIS2DW12_I2C_DISABLE:
      *val = LIS2DW12_I2C_DISABLE;
      break;
    default:
      *val = LIS2DW12_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Disconnect CS pull-up.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of cs_pu_disc in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_cs_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_cs_pu_disc_t val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.cs_pu_disc = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Disconnect CS pull-up.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of cs_pu_disc in reg CTRL2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_cs_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_cs_pu_disc_t *val)
{
  lis2dw12_ctrl2_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL2,(uint8_t*) &reg, 1);

  switch (reg.cs_pu_disc) {
    case LIS2DW12_PULL_UP_CONNECT:
      *val = LIS2DW12_PULL_UP_CONNECT;
      break;
    case LIS2DW12_PULL_UP_DISCONNECT:
      *val = LIS2DW12_PULL_UP_DISCONNECT;
      break;
    default:
      *val = LIS2DW12_PULL_UP_CONNECT;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Interrupt_Pins
  * @brief     This section groups all the functions that manage interrupt pins
  * @{
  *
  */

/**
  * @brief  Interrupt active-high/low.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of h_lactive in reg CTRL3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_pin_polarity_set(lis2dw12_ctx_t *ctx,
                                  lis2dw12_h_lactive_t val)
{
  lis2dw12_ctrl3_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.h_lactive = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Interrupt active-high/low.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of h_lactive in reg CTRL3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_pin_polarity_get(lis2dw12_ctx_t *ctx,
                                  lis2dw12_h_lactive_t *val)
{
  lis2dw12_ctrl3_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);

  switch (reg.h_lactive) {
    case LIS2DW12_ACTIVE_HIGH:
      *val = LIS2DW12_ACTIVE_HIGH;
      break;
    case LIS2DW12_ACTIVE_LOW:
      *val = LIS2DW12_ACTIVE_LOW;
      break;
    default:
      *val = LIS2DW12_ACTIVE_HIGH;
      break;
  }
  return ret;
}

/**
  * @brief  Latched/pulsed interrupt.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lir in reg CTRL3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_int_notification_set(lis2dw12_ctx_t *ctx,
                                      lis2dw12_lir_t val)
{
  lis2dw12_ctrl3_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.lir = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Latched/pulsed interrupt.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of lir in reg CTRL3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_int_notification_get(lis2dw12_ctx_t *ctx,
                                      lis2dw12_lir_t *val)
{
  lis2dw12_ctrl3_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);

  switch (reg.lir) {
    case LIS2DW12_INT_PULSED:
      *val = LIS2DW12_INT_PULSED;
      break;
    case LIS2DW12_INT_LATCHED:
      *val = LIS2DW12_INT_LATCHED;
      break;
    default:
      *val = LIS2DW12_INT_PULSED;
      break;
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of pp_od in reg CTRL3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_pin_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_pp_od_t val)
{
  lis2dw12_ctrl3_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.pp_od = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of pp_od in reg CTRL3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_pin_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_pp_od_t *val)
{
  lis2dw12_ctrl3_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL3,(uint8_t*) &reg, 1);

  switch (reg.pp_od) {
    case LIS2DW12_PUSH_PULL:
      *val = LIS2DW12_PUSH_PULL;
      break;
    case LIS2DW12_OPEN_DRAIN:
      *val = LIS2DW12_OPEN_DRAIN;
      break;
    default:
      *val = LIS2DW12_PUSH_PULL;
      break;
  }
  return ret;
}

/**
  * @brief  Select the signal that need to route on int1 pad.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      register CTRL4_INT1_PAD_CTRL.
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_pin_int1_route_set(lis2dw12_ctx_t *ctx,
                                    lis2dw12_ctrl4_int1_pad_ctrl_t *val)
{
  lis2dw12_ctrl5_int2_pad_ctrl_t ctrl5_int2_pad_ctrl;
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL5_INT2_PAD_CTRL,
                          (uint8_t*)&ctrl5_int2_pad_ctrl, 1);

  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }
  if (ret == 0) {
    if ((val->int1_tap |
         val->int1_ff |
         val->int1_wu |
         val->int1_single_tap |
         val->int1_6d |
         ctrl5_int2_pad_ctrl.int2_sleep_state |
         ctrl5_int2_pad_ctrl.int2_sleep_chg ) != PROPERTY_DISABLE){
      reg.interrupts_enable = PROPERTY_ENABLE;
    }
    else{
      reg.interrupts_enable = PROPERTY_DISABLE;
    }

    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL4_INT1_PAD_CTRL,
                                (uint8_t*) val, 1);
  } if (ret == 0) {
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Select the signal that need to route on int1 pad.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      register CTRL4_INT1_PAD_CTRL.
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_pin_int1_route_get(lis2dw12_ctx_t *ctx,
                                    lis2dw12_ctrl4_int1_pad_ctrl_t *val)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL4_INT1_PAD_CTRL,
                          (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief   Select the signal that need to route on int2 pad.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      register CTRL5_INT2_PAD_CTRL.
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_pin_int2_route_set(lis2dw12_ctx_t *ctx,
                                    lis2dw12_ctrl5_int2_pad_ctrl_t *val)
{
  lis2dw12_ctrl4_int1_pad_ctrl_t ctrl4_int1_pad_ctrl;
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL4_INT1_PAD_CTRL,
                          (uint8_t*) &ctrl4_int1_pad_ctrl, 1);

  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }
  if (ret == 0) {
    if ((ctrl4_int1_pad_ctrl.int1_tap |
         ctrl4_int1_pad_ctrl.int1_ff |
         ctrl4_int1_pad_ctrl.int1_wu |
         ctrl4_int1_pad_ctrl.int1_single_tap |
         ctrl4_int1_pad_ctrl.int1_6d |
         val->int2_sleep_state | val->int2_sleep_chg ) != PROPERTY_DISABLE) {
        reg.interrupts_enable = PROPERTY_ENABLE;
    }
    else{
      reg.interrupts_enable = PROPERTY_DISABLE;
    }

    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL5_INT2_PAD_CTRL,
                             (uint8_t*) val, 1);
  }
  if (ret == 0) {
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Select the signal that need to route on int2 pad.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      register CTRL5_INT2_PAD_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_pin_int2_route_get(lis2dw12_ctx_t *ctx,
                                    lis2dw12_ctrl5_int2_pad_ctrl_t *val)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL5_INT2_PAD_CTRL,
                          (uint8_t*) val, 1);
  return ret;
}
/**
  * @brief All interrupt signals become available on INT1 pin.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int2_on_int1 in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_all_on_int1_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.int2_on_int1 = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  All interrupt signals become available on INT1 pin.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int2_on_int1 in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_all_on_int1_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  *val = reg.int2_on_int1;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Wake_Up_Event
  * @brief     This section groups all the functions that manage the Wake
  *            Up event generation.
  * @{
  *
  */

/**
  * @brief  Threshold for wakeup.1 LSB = FS_XL / 64.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wk_ths in reg WAKE_UP_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_wkup_threshold_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_wake_up_ths_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.wk_ths = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Threshold for wakeup.1 LSB = FS_XL / 64.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wk_ths in reg WAKE_UP_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_wkup_threshold_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_wake_up_ths_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &reg, 1);
  *val = reg.wk_ths;

  return ret;
}

/**
  * @brief  Wake up duration event.1LSb = 1 / ODR.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wake_dur in reg WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_wkup_dur_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_wake_up_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.wake_dur = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Wake up duration event.1LSb = 1 / ODR.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wake_dur in reg WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_wkup_dur_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_wake_up_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &reg, 1);
  *val = reg.wake_dur;

  return ret;
}

/**
  * @brief  Data sent to wake-up interrupt function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of usr_off_on_wu in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_wkup_feed_data_set(lis2dw12_ctx_t *ctx,
                                    lis2dw12_usr_off_on_wu_t val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.usr_off_on_wu = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Data sent to wake-up interrupt function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of usr_off_on_wu in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_wkup_feed_data_get(lis2dw12_ctx_t *ctx,
                                    lis2dw12_usr_off_on_wu_t *val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);

  switch (reg.usr_off_on_wu) {
    case LIS2DW12_HP_FEED:
      *val = LIS2DW12_HP_FEED;
      break;
    case LIS2DW12_USER_OFFSET_FEED:
      *val = LIS2DW12_USER_OFFSET_FEED;
      break;
    default:
      *val = LIS2DW12_HP_FEED;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   LIS2DW12_Activity/Inactivity_Detection
  * @brief      This section groups all the functions concerning
  *             activity/inactivity detection.
  * @{
  *
  */

/**
  * @brief  Config activity / inactivity or
  *         stationary / motion detection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sleep_on / stationary in
  *                  reg WAKE_UP_THS / WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_act_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_sleep_on_t val)
{
  lis2dw12_wake_up_ths_t wake_up_ths;
  lis2dw12_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &wake_up_ths, 1);
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &wake_up_dur, 1);
  }
  if (ret == 0) {
    wake_up_ths.sleep_on = (uint8_t) val & 0x01U;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &wake_up_ths, 1);
  }
  if (ret == 0) {
    wake_up_dur.stationary = ((uint8_t)val & 0x02U) >> 1;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &wake_up_dur, 1);
  }

  return ret;
}

/**
  * @brief  Config activity / inactivity or
  *         stationary / motion detection. [get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sleep_on in reg WAKE_UP_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_act_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_sleep_on_t *val)
{
  lis2dw12_wake_up_ths_t wake_up_ths;
  lis2dw12_wake_up_dur_t wake_up_dur;;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &wake_up_ths, 1);
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &wake_up_dur, 1);

    switch ((wake_up_dur.stationary << 1) + wake_up_ths.sleep_on){
      case LIS2DW12_NO_DETECTION:
        *val = LIS2DW12_NO_DETECTION;
        break;
      case LIS2DW12_DETECT_ACT_INACT:
        *val = LIS2DW12_DETECT_ACT_INACT;
        break;
      case LIS2DW12_DETECT_STAT_MOTION:
        *val = LIS2DW12_DETECT_STAT_MOTION;
        break;
      default:
        *val = LIS2DW12_NO_DETECTION;
        break;
    }
  }
  return ret;
}

/**
  * @brief  Duration to go in sleep mode (1 LSb = 512 / ODR).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sleep_dur in reg WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_act_sleep_dur_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_wake_up_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.sleep_dur = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Duration to go in sleep mode (1 LSb = 512 / ODR).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sleep_dur in reg WAKE_UP_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_act_sleep_dur_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_wake_up_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &reg, 1);
  *val = reg.sleep_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Tap_Generator
  * @brief     This section groups all the functions that manage the tap
  *            and double tap event generation.
  * @{
  *
  */

/**
  * @brief  Threshold for tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_thsx in reg TAP_THS_X
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_threshold_x_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_tap_ths_x_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.tap_thsx = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Threshold for tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_thsx in reg TAP_THS_X
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_threshold_x_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_tap_ths_x_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  *val = reg.tap_thsx;

  return ret;
}

/**
  * @brief  Threshold for tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_thsy in reg TAP_THS_Y
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_threshold_y_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_tap_ths_y_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Y,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.tap_thsy = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_Y,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Threshold for tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_thsy in reg TAP_THS_Y
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_threshold_y_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_tap_ths_y_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Y,(uint8_t*) &reg, 1);
  *val = reg.tap_thsy;

  return ret;
}

/**
  * @brief  Selection of axis priority for TAP detection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_prior in reg TAP_THS_Y
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_axis_priority_set(lis2dw12_ctx_t *ctx,
                                       lis2dw12_tap_prior_t val)
{
  lis2dw12_tap_ths_y_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Y,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.tap_prior = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_Y,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Selection of axis priority for TAP detection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of tap_prior in reg TAP_THS_Y
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_axis_priority_get(lis2dw12_ctx_t *ctx,
                                       lis2dw12_tap_prior_t *val)
{
  lis2dw12_tap_ths_y_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Y,(uint8_t*) &reg, 1);

  switch (reg.tap_prior) {
    case LIS2DW12_XYZ:
      *val = LIS2DW12_XYZ;
      break;
    case LIS2DW12_YXZ:
      *val = LIS2DW12_YXZ;
      break;
    case LIS2DW12_XZY:
      *val = LIS2DW12_XZY;
      break;
    case LIS2DW12_ZYX:
      *val = LIS2DW12_ZYX;
      break;
    case LIS2DW12_YZX:
      *val = LIS2DW12_YZX;
      break;
    case LIS2DW12_ZXY:
      *val = LIS2DW12_ZXY;
      break;
    default:
      *val = LIS2DW12_XYZ;
      break;
  }
  return ret;
}

/**
  * @brief  Threshold for tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_thsz in reg TAP_THS_Z
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_threshold_z_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_tap_ths_z_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.tap_thsz = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Threshold for tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_thsz in reg TAP_THS_Z
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_threshold_z_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_tap_ths_z_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  *val = reg.tap_thsz;

  return ret;
}

/**
  * @brief  Enable Z direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_z_en in reg TAP_THS_Z
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_detection_on_z_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_tap_ths_z_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.tap_z_en = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable Z direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_z_en in reg TAP_THS_Z
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_detection_on_z_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_tap_ths_z_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  *val = reg.tap_z_en;

  return ret;
}

/**
  * @brief  Enable Y direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_y_en in reg TAP_THS_Z
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_detection_on_y_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_tap_ths_z_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.tap_y_en = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable Y direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_y_en in reg TAP_THS_Z
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_detection_on_y_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_tap_ths_z_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  *val = reg.tap_y_en;

  return ret;
}

/**
  * @brief  Enable X direction in tap recognition.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_x_en in reg TAP_THS_Z
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_detection_on_x_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_tap_ths_z_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.tap_x_en = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable X direction in tap recognition.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of tap_x_en in reg TAP_THS_Z
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_detection_on_x_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_tap_ths_z_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_Z,(uint8_t*) &reg, 1);
  *val = reg.tap_x_en;

  return ret;
}

/**
  * @brief  Maximum duration is the maximum time of an overthreshold signal
  *         detection to be recognized as a tap event. The default value
  *         of these bits is 00b which corresponds to 4*ODR_XL time.
  *         If the SHOCK[1:0] bits are set to a different value, 1LSB
  *         corresponds to 8*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of shock in reg INT_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_shock_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_int_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.shock = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Maximum duration is the maximum time of an overthreshold signal
  *         detection to be recognized as a tap event. The default value
  *         of these bits is 00b which corresponds to 4*ODR_XL time.
  *         If the SHOCK[1:0] bits are set to a different value, 1LSB
  *         corresponds to 8*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of shock in reg INT_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_shock_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_int_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  *val = reg.shock;

  return ret;
}

/**
  * @brief  Quiet time is the time after the first detected tap in which
  *         there must not be any overthreshold event.
  *         The default value of these bits is 00b which corresponds
  *         to 2*ODR_XL time. If the QUIET[1:0] bits are set to a different
  *         value, 1LSB corresponds to 4*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of quiet in reg INT_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_quiet_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_int_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.quiet = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Quiet time is the time after the first detected tap in which
  *         there must not be any overthreshold event.
  *         The default value of these bits is 00b which corresponds
  *         to 2*ODR_XL time. If the QUIET[1:0] bits are set to a different
  *         value, 1LSB corresponds to 4*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of quiet in reg INT_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_quiet_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_int_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  *val = reg.quiet;

  return ret;
}

/**
  * @brief  When double tap recognition is enabled, this register expresses
  *         the maximum time between two consecutive detected taps to
  *         determine a double tap event.
  *         The default value of these bits is 0000b which corresponds
  *         to 16*ODR_XL time. If the DUR[3:0] bits are set to a different
  *         value, 1LSB corresponds to 32*ODR_XL time.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of latency in reg INT_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_dur_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_int_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.latency = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  When double tap recognition is enabled, this register expresses
  *         the maximum time between two consecutive detected taps to
  *         determine a double tap event.
  *         The default value of these bits is 0000b which corresponds
  *         to 16*ODR_XL time. If the DUR[3:0] bits are set to a different
  *         value, 1LSB corresponds to 32*ODR_XL time.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of latency in reg INT_DUR
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_dur_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_int_dur_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_INT_DUR,(uint8_t*) &reg, 1);
  *val = reg.latency;

  return ret;
}

/**
  * @brief  Single/double-tap event enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of single_double_tap in reg WAKE_UP_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_mode_set(lis2dw12_ctx_t *ctx,
                              lis2dw12_single_double_tap_t val)
{
  lis2dw12_wake_up_ths_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.single_double_tap = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Single/double-tap event enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of single_double_tap in reg WAKE_UP_THS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_mode_get(lis2dw12_ctx_t *ctx,
                              lis2dw12_single_double_tap_t *val)
{
  lis2dw12_wake_up_ths_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_THS,(uint8_t*) &reg, 1);

  switch (reg.single_double_tap) {
    case LIS2DW12_ONLY_SINGLE:
      *val = LIS2DW12_ONLY_SINGLE;
      break;
    case LIS2DW12_BOTH_SINGLE_DOUBLE:
      *val = LIS2DW12_BOTH_SINGLE_DOUBLE;
      break;
    default:
      *val = LIS2DW12_ONLY_SINGLE;
      break;
  }

  return ret;
}

/**
  * @brief  Read the tap / double tap source register.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  lis2dw12_tap_src: union of registers from TAP_SRC to
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_tap_src_get(lis2dw12_ctx_t *ctx, lis2dw12_tap_src_t *val)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_SRC, (uint8_t*) val, 1);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   LIS2DW12_Six_Position_Detection(6D/4D)
  * @brief      This section groups all the functions concerning six
  *             position detection (6D).
  * @{
  *
  */

/**
  * @brief  Threshold for 4D/6D function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of 6d_ths in reg TAP_THS_X
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_6d_threshold_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_tap_ths_x_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg._6d_ths = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Threshold for 4D/6D function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of 6d_ths in reg TAP_THS_X
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_6d_threshold_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_tap_ths_x_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  *val = reg._6d_ths;

  return ret;
}

/**
  * @brief  4D orientation detection enable.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of 4d_en in reg TAP_THS_X
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_4d_mode_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_tap_ths_x_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg._4d_en = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  4D orientation detection enable.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of 4d_en in reg TAP_THS_X
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_4d_mode_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_tap_ths_x_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_TAP_THS_X,(uint8_t*) &reg, 1);
  *val = reg._4d_en;

  return ret;
}

/**
  * @brief  Read the 6D tap source register.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      union of registers from SIXD_SRC
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_6d_src_get(lis2dw12_ctx_t *ctx, lis2dw12_sixd_src_t *val)
{
  int32_t ret;
  ret = lis2dw12_read_reg(ctx, LIS2DW12_SIXD_SRC, (uint8_t*) val, 1);
  return ret;
}
/**
  * @brief  Data sent to 6D interrupt function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lpass_on6d in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_6d_feed_data_set(lis2dw12_ctx_t *ctx,
                                  lis2dw12_lpass_on6d_t val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.lpass_on6d = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Data sent to 6D interrupt function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of lpass_on6d in reg CTRL_REG7
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_6d_feed_data_get(lis2dw12_ctx_t *ctx,
                                  lis2dw12_lpass_on6d_t *val)
{
  lis2dw12_ctrl_reg7_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL_REG7,(uint8_t*) &reg, 1);

  switch (reg.lpass_on6d) {
    case LIS2DW12_ODR_DIV_2_FEED:
      *val = LIS2DW12_ODR_DIV_2_FEED;
      break;
    case LIS2DW12_LPF2_FEED:
      *val = LIS2DW12_LPF2_FEED;
      break;
    default:
      *val = LIS2DW12_ODR_DIV_2_FEED;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Free_Fall
  * @brief     This section group all the functions concerning
  *            the free fall detection.
  * @{
  *
  */

/**
  * @brief  Wake up duration event(1LSb = 1 / ODR).[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ff_dur in reg
  *                  WAKE_UP_DUR /F REE_FALL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_ff_dur_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_wake_up_dur_t wake_up_dur;
  lis2dw12_free_fall_t free_fall;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &wake_up_dur, 1);
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_FREE_FALL,(uint8_t*) &free_fall, 1);
  }
  if(ret == 0) {
    wake_up_dur.ff_dur = ( (uint8_t) val & 0x20U) >> 5;
    free_fall.ff_dur = (uint8_t) val & 0x1FU;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &wake_up_dur, 1);
  }
  if(ret == 0) {
    ret = lis2dw12_write_reg(ctx, LIS2DW12_FREE_FALL,(uint8_t*) &free_fall, 1);
  }

  return ret;
}

/**
  * @brief  Wake up duration event(1LSb = 1 / ODR).[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ff_dur in
  *                  reg WAKE_UP_DUR /F REE_FALL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_ff_dur_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_wake_up_dur_t wake_up_dur;
  lis2dw12_free_fall_t free_fall;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_WAKE_UP_DUR,(uint8_t*) &wake_up_dur, 1);
  if (ret == 0) {
    ret = lis2dw12_read_reg(ctx, LIS2DW12_FREE_FALL,(uint8_t*) &free_fall, 1);
    *val = (wake_up_dur.ff_dur << 5) + free_fall.ff_dur;
  }
  return ret;
}

/**
  * @brief  Free fall threshold setting.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of ff_ths in reg FREE_FALL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_ff_threshold_set(lis2dw12_ctx_t *ctx, lis2dw12_ff_ths_t val)
{
  lis2dw12_free_fall_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FREE_FALL,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.ff_ths = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_FREE_FALL,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Free fall threshold setting.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of ff_ths in reg FREE_FALL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_ff_threshold_get(lis2dw12_ctx_t *ctx,
                                  lis2dw12_ff_ths_t *val)
{
  lis2dw12_free_fall_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FREE_FALL,(uint8_t*) &reg, 1);

  switch (reg.ff_ths) {
    case LIS2DW12_FF_TSH_5LSb_FS2g:
      *val = LIS2DW12_FF_TSH_5LSb_FS2g;
      break;
    case LIS2DW12_FF_TSH_7LSb_FS2g:
      *val = LIS2DW12_FF_TSH_7LSb_FS2g;
      break;
    case LIS2DW12_FF_TSH_8LSb_FS2g:
      *val = LIS2DW12_FF_TSH_8LSb_FS2g;
      break;
    case LIS2DW12_FF_TSH_10LSb_FS2g:
      *val = LIS2DW12_FF_TSH_10LSb_FS2g;
      break;
    case LIS2DW12_FF_TSH_11LSb_FS2g:
      *val = LIS2DW12_FF_TSH_11LSb_FS2g;
      break;
    case LIS2DW12_FF_TSH_13LSb_FS2g:
      *val = LIS2DW12_FF_TSH_13LSb_FS2g;
      break;
    case LIS2DW12_FF_TSH_15LSb_FS2g:
      *val = LIS2DW12_FF_TSH_15LSb_FS2g;
      break;
    case LIS2DW12_FF_TSH_16LSb_FS2g:
      *val = LIS2DW12_FF_TSH_16LSb_FS2g;
      break;
    default:
      *val = LIS2DW12_FF_TSH_5LSb_FS2g;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS2DW12_Fifo
  * @brief     This section group all the functions concerning the fifo usage
  * @{
  *
  */

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fth in reg FIFO_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_fifo_watermark_set(lis2dw12_ctx_t *ctx, uint8_t val)
{
  lis2dw12_fifo_ctrl_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FIFO_CTRL,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.fth = val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_FIFO_CTRL,(uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  FIFO watermark level selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fth in reg FIFO_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_fifo_watermark_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_fifo_ctrl_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FIFO_CTRL,(uint8_t*) &reg, 1);
  *val = reg.fth;

  return ret;
}

/**
  * @brief  FIFO mode selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fmode in reg FIFO_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_fifo_mode_set(lis2dw12_ctx_t *ctx, lis2dw12_fmode_t val)
{
  lis2dw12_fifo_ctrl_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FIFO_CTRL,(uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.fmode = (uint8_t) val;
    ret = lis2dw12_write_reg(ctx, LIS2DW12_FIFO_CTRL,(uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  FIFO mode selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of fmode in reg FIFO_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_fifo_mode_get(lis2dw12_ctx_t *ctx, lis2dw12_fmode_t *val)
{
  lis2dw12_fifo_ctrl_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FIFO_CTRL,(uint8_t*) &reg, 1);

  switch (reg.fmode) {
    case LIS2DW12_BYPASS_MODE:
      *val = LIS2DW12_BYPASS_MODE;
      break;
    case LIS2DW12_FIFO_MODE:
      *val = LIS2DW12_FIFO_MODE;
      break;
    case LIS2DW12_STREAM_TO_FIFO_MODE:
      *val = LIS2DW12_STREAM_TO_FIFO_MODE;
      break;
    case LIS2DW12_BYPASS_TO_STREAM_MODE:
      *val = LIS2DW12_BYPASS_TO_STREAM_MODE;
      break;
    case LIS2DW12_STREAM_MODE:
      *val = LIS2DW12_STREAM_MODE;
      break;
    default:
      *val = LIS2DW12_BYPASS_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  Number of unread samples stored in FIFO.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of diff in reg FIFO_SAMPLES
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_fifo_data_level_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_fifo_samples_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FIFO_SAMPLES,(uint8_t*) &reg, 1);
  *val = reg.diff;

  return ret;
}
/**
  * @brief  FIFO overrun status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_ovr in reg FIFO_SAMPLES
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_fifo_ovr_flag_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_fifo_samples_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FIFO_SAMPLES,(uint8_t*) &reg, 1);
  *val = reg.fifo_ovr;

  return ret;
}
/**
  * @brief  FIFO threshold status flag.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_fth in reg FIFO_SAMPLES
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis2dw12_fifo_wtm_flag_get(lis2dw12_ctx_t *ctx, uint8_t *val)
{
  lis2dw12_fifo_samples_t reg;
  int32_t ret;

  ret = lis2dw12_read_reg(ctx, LIS2DW12_FIFO_SAMPLES,(uint8_t*) &reg, 1);
  *val = reg.fifo_fth;

  return ret;
}
/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/