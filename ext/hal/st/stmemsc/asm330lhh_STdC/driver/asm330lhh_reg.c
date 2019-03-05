/*
 ******************************************************************************
 * @file    asm330lhh_reg.c
 * @author  Sensors Software Solution Team
 * @brief   ASM330LHH driver file
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

#include "asm330lhh_reg.h"

/**
  * @defgroup    ASM330LHH
  * @brief       This file provides a set of functions needed to drive the
  *              asm330lhh enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup    ASM330LHH_Interfaces_Functions
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
int32_t asm330lhh_read_reg(asm330lhh_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t asm330lhh_write_reg(asm330lhh_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    ASM330LHH_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t asm330lhh_from_fs2g_to_mg(int16_t lsb)
{
  return ((float_t)lsb * 0.061f);
}

float_t asm330lhh_from_fs4g_to_mg(int16_t lsb)
{
  return ((float_t)lsb * 0.122f);
}

float_t asm330lhh_from_fs8g_to_mg(int16_t lsb)
{
  return ((float_t)lsb * 0.244f);
}

float_t asm330lhh_from_fs16g_to_mg(int16_t lsb)
{
  return ((float_t)lsb * 0.488f);
}

float_t asm330lhh_from_fs125dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 4.375f);
}

float_t asm330lhh_from_fs250dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 8.75f);
}

float_t asm330lhh_from_fs500dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 17.50f);
}

float_t asm330lhh_from_fs1000dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 35.0f);
}

float_t asm330lhh_from_fs2000dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 70.0f);
}

float_t asm330lhh_from_fs4000dps_to_mdps(int16_t lsb)
{
  return ((float_t)lsb * 140.0f);
}

float_t asm330lhh_from_lsb_to_celsius(int16_t lsb)
{
  return (((float_t)lsb / 256.0f) + 25.0f);
}

float_t asm330lhh_from_lsb_to_nsec(int32_t lsb)
{
  return ((float_t)lsb * 25000.0f);
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
  * @brief  Accelerometer full-scale selection[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fs_xl in reg CTRL1_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_full_scale_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_fs_xl_t val)
{
  asm330lhh_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  if(ret == 0){
    ctrl1_xl.fs_xl = (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL1_XL,
                              (uint8_t*)&ctrl1_xl, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fs_xl in reg CTRL1_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_full_scale_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_fs_xl_t *val)
{
  asm330lhh_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  switch (ctrl1_xl.fs_xl){
    case ASM330LHH_2g:
      *val = ASM330LHH_2g;
      break;
    case ASM330LHH_16g:
      *val = ASM330LHH_16g;
      break;
    case ASM330LHH_4g:
      *val = ASM330LHH_4g;
      break;
    case ASM330LHH_8g:
      *val = ASM330LHH_8g;
      break;
    default:
      *val = ASM330LHH_2g;
      break;
  }
  return ret;
}

/**
  * @brief  Accelerometer UI data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odr_xl in reg CTRL1_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_data_rate_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_odr_xl_t val)
{
  asm330lhh_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  if(ret == 0){
    ctrl1_xl.odr_xl= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL1_XL,
                              (uint8_t*)&ctrl1_xl, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer UI data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odr_xl in reg CTRL1_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_data_rate_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_odr_xl_t *val)
{
  asm330lhh_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  switch (ctrl1_xl.odr_xl){
    case ASM330LHH_XL_ODR_OFF:
      *val = ASM330LHH_XL_ODR_OFF;
      break;
    case ASM330LHH_XL_ODR_12Hz5:
      *val = ASM330LHH_XL_ODR_12Hz5;
      break;
    case ASM330LHH_XL_ODR_26Hz:
      *val = ASM330LHH_XL_ODR_26Hz;
      break;
    case ASM330LHH_XL_ODR_52Hz:
      *val = ASM330LHH_XL_ODR_52Hz;
      break;
    case ASM330LHH_XL_ODR_104Hz:
      *val = ASM330LHH_XL_ODR_104Hz;
      break;
    case ASM330LHH_XL_ODR_208Hz:
      *val = ASM330LHH_XL_ODR_208Hz;
      break;
    case ASM330LHH_XL_ODR_417Hz:
      *val = ASM330LHH_XL_ODR_417Hz;
      break;
    case ASM330LHH_XL_ODR_833Hz:
      *val = ASM330LHH_XL_ODR_833Hz;
      break;
    case ASM330LHH_XL_ODR_1667Hz:
      *val = ASM330LHH_XL_ODR_1667Hz;
      break;
    case ASM330LHH_XL_ODR_3333Hz:
      *val = ASM330LHH_XL_ODR_3333Hz;
      break;
    case ASM330LHH_XL_ODR_6667Hz:
      *val = ASM330LHH_XL_ODR_6667Hz;
      break;
    case ASM330LHH_XL_ODR_6Hz5:
      *val = ASM330LHH_XL_ODR_6Hz5;
      break;
    default:
      *val = ASM330LHH_XL_ODR_OFF;
      break;
  }
  return ret;
}

/**
  * @brief  Gyroscope UI chain full-scale selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fs_g in reg CTRL2_G
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_full_scale_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_fs_g_t val)
{
  asm330lhh_ctrl2_g_t ctrl2_g;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  if(ret == 0){
    ctrl2_g.fs_g= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope UI chain full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fs_g in reg CTRL2_G
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_full_scale_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_fs_g_t *val)
{
  asm330lhh_ctrl2_g_t ctrl2_g;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  switch (ctrl2_g.fs_g){
    case ASM330LHH_125dps:
      *val = ASM330LHH_125dps;
      break;
    case ASM330LHH_250dps:
      *val = ASM330LHH_250dps;
      break;
    case ASM330LHH_500dps:
      *val = ASM330LHH_500dps;
      break;
    case ASM330LHH_1000dps:
      *val = ASM330LHH_1000dps;
      break;
    case ASM330LHH_2000dps:
      *val = ASM330LHH_2000dps;
      break;
    case ASM330LHH_4000dps:
      *val = ASM330LHH_4000dps;
      break;
    default:
      *val = ASM330LHH_125dps;
      break;
  }
  return ret;
}

/**
  * @brief  Gyroscope data rate.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odr_g in reg CTRL2_G
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_data_rate_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_odr_g_t val)
{
  asm330lhh_ctrl2_g_t ctrl2_g;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  if(ret == 0){
   ctrl2_g.odr_g= (uint8_t)val;
   ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope data rate.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odr_g in reg CTRL2_G
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_data_rate_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_odr_g_t *val)
{
  asm330lhh_ctrl2_g_t ctrl2_g;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL2_G, (uint8_t*)&ctrl2_g, 1);
  switch (ctrl2_g.odr_g){
    case ASM330LHH_GY_ODR_OFF:
      *val = ASM330LHH_GY_ODR_OFF;
      break;
    case ASM330LHH_GY_ODR_12Hz5:
      *val = ASM330LHH_GY_ODR_12Hz5;
      break;
    case ASM330LHH_GY_ODR_26Hz:
      *val = ASM330LHH_GY_ODR_26Hz;
      break;
    case ASM330LHH_GY_ODR_52Hz:
      *val = ASM330LHH_GY_ODR_52Hz;
      break;
    case ASM330LHH_GY_ODR_104Hz:
      *val = ASM330LHH_GY_ODR_104Hz;
      break;
    case ASM330LHH_GY_ODR_208Hz:
      *val = ASM330LHH_GY_ODR_208Hz;
      break;
    case ASM330LHH_GY_ODR_417Hz:
      *val = ASM330LHH_GY_ODR_417Hz;
      break;
    case ASM330LHH_GY_ODR_833Hz:
      *val = ASM330LHH_GY_ODR_833Hz;
      break;
    case ASM330LHH_GY_ODR_1667Hz:
      *val = ASM330LHH_GY_ODR_1667Hz;
      break;
    case ASM330LHH_GY_ODR_3333Hz:
      *val = ASM330LHH_GY_ODR_3333Hz;
      break;
    case ASM330LHH_GY_ODR_6667Hz:
      *val = ASM330LHH_GY_ODR_6667Hz;
      break;
    default:
      *val = ASM330LHH_GY_ODR_OFF;
      break;
  }
  return ret;
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of bdu in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_block_data_update_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.bdu= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief  Block data update.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of bdu in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_block_data_update_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  *val = ctrl3_c.bdu;

  return ret;
}

/**
  * @brief  Weight of XL user offset bits of registers X_OFS_USR (73h),
  *         Y_OFS_USR (74h), Z_OFS_USR (75h).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of usr_off_w in reg CTRL6_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_offset_weight_set(asm330lhh_ctx_t *ctx,
                                     asm330lhh_usr_off_w_t val)
{
  asm330lhh_ctrl6_g_t ctrl6_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);
  if(ret == 0){
    ctrl6_c.usr_off_w= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);
  }
  return ret;
}

/**
  * @brief  Weight of XL user offset bits of registers X_OFS_USR (73h),
  *         Y_OFS_USR (74h), Z_OFS_USR (75h).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of usr_off_w in reg CTRL6_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_offset_weight_get(asm330lhh_ctx_t *ctx,
                                       asm330lhh_usr_off_w_t *val)
{
  asm330lhh_ctrl6_g_t ctrl6_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);

  switch (ctrl6_c.usr_off_w){
    case ASM330LHH_LSb_1mg:
      *val = ASM330LHH_LSb_1mg;
      break;
    case ASM330LHH_LSb_16mg:
      *val = ASM330LHH_LSb_16mg;
      break;
    default:
      *val = ASM330LHH_LSb_1mg;
      break;
  }
  return ret;
}

/**
  * @brief  Read all the interrupt flag of the device.
  *[get]
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers ALL_INT_SRC; WAKE_UP_SRC;
  *                              TAP_SRC; D6D_SRC; STATUS_REG;
  *                              EMB_FUNC_STATUS; FSM_STATUS_A/B
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_all_sources_get(asm330lhh_ctx_t *ctx,
                                  asm330lhh_all_sources_t *val)
{
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_ALL_INT_SRC,
                           (uint8_t*)&val->all_int_src, 1);
  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_SRC,
                             (uint8_t*)&val->wake_up_src, 1);
  }
  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_TAP_SRC,
                             (uint8_t*)&val->tap_src, 1);
  }
  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_D6D_SRC,
                             (uint8_t*)&val->d6d_src, 1);
  }
  if(ret == 0){
  ret = asm330lhh_read_reg(ctx, ASM330LHH_STATUS_REG,
                           (uint8_t*)&val->status_reg, 1);
  }

  return ret;
}

/**
  * @brief  The STATUS_REG register is read by the primary interface.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get register STATUS_REG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_status_reg_get(asm330lhh_ctx_t *ctx,
                                 asm330lhh_status_reg_t *val)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_STATUS_REG, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Accelerometer new data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of xlda in reg STATUS_REG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_flag_data_ready_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_status_reg_t status_reg;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_STATUS_REG,
                           (uint8_t*)&status_reg, 1);
  *val = status_reg.xlda;

  return ret;
}

/**
  * @brief  Gyroscope new data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of gda in reg STATUS_REG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_flag_data_ready_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_status_reg_t status_reg;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_STATUS_REG,
                           (uint8_t*)&status_reg, 1);
  *val = status_reg.gda;

  return ret;
}

/**
  * @brief  Temperature new data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of tda in reg STATUS_REG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_temp_flag_data_ready_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_status_reg_t status_reg;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_STATUS_REG,
                           (uint8_t*)&status_reg, 1);
  *val = status_reg.tda;

  return ret;
}

/**
  * @brief  Accelerometer X-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that contains data to write
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_x_set(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_write_reg(ctx, ASM330LHH_X_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer X-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_x_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_X_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Y-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that contains data to write
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_y_set(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_write_reg(ctx, ASM330LHH_Y_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Y-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_y_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_Y_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer Z-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that contains data to write
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_z_set(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_write_reg(ctx, ASM330LHH_Z_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Accelerometer X-axis user offset correction expressed in two’s
  *         complement, weight depends on USR_OFF_W in CTRL6_C (15h).
  *         The value must be in the range [-127 127].[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_z_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_Z_OFS_USR, buff, 1);
  return ret;
}

/**
  * @brief  Enables user offset on out.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of usr_off_on_out in reg CTRL7_G
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  if(ret == 0){
    ctrl7_g.usr_off_on_out= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  }
  return ret;
}

/**
  * @brief  Get user offset on out flag.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get values of usr_off_on_out in reg CTRL7_G
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  *val = ctrl7_g.usr_off_on_out;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_Timestamp
  * @brief      This section groups all the functions that manage the
  *             timestamp generation.
  * @{
  *
  */

/**
  * @brief  Enables timestamp counter.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of timestamp_en in reg CTRL10_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_timestamp_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  if(ret == 0){
    ctrl10_c.timestamp_en= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL10_C,
                              (uint8_t*)&ctrl10_c, 1);
  }
  return ret;
}

/**
  * @brief  Enables timestamp counter.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of timestamp_en in reg CTRL10_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_timestamp_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl10_c_t ctrl10_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL10_C, (uint8_t*)&ctrl10_c, 1);
  *val = ctrl10_c.timestamp_en;

  return ret;
}

/**
  * @brief  Timestamp first data output register (r).
  *         The value is expressed as a 32-bit word and the bit resolution
  *         is 25 μs.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_timestamp_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_TIMESTAMP0, buff, 4);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_Data output
  * @brief      This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief  Circular burst-mode (rounding) read of the output registers.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of rounding in reg CTRL5_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_rounding_mode_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_rounding_t val)
{
  asm330lhh_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  if(ret == 0){
    ctrl5_c.rounding= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope UI chain full-scale selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of rounding in reg CTRL5_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_rounding_mode_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_rounding_t *val)
{
  asm330lhh_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  switch (ctrl5_c.rounding){
    case ASM330LHH_NO_ROUND:
      *val = ASM330LHH_NO_ROUND;
      break;
    case ASM330LHH_ROUND_XL:
      *val = ASM330LHH_ROUND_XL;
      break;
    case ASM330LHH_ROUND_GY:
      *val = ASM330LHH_ROUND_GY;
      break;
    case ASM330LHH_ROUND_GY_XL:
      *val = ASM330LHH_ROUND_GY_XL;
      break;
    default:
      *val = ASM330LHH_NO_ROUND;
      break;
  }
  return ret;
}

/**
  * @brief  Temperature data output register (r).
  *         L and H registers together express a 16-bit word in two’s
  *         complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_temperature_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_OUT_TEMP_L, buff, 2);
  return ret;
}

/**
  * @brief  Angular rate sensor. The value is expressed as a 16-bit
  *         word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_angular_rate_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_OUTX_L_G, buff, 6);
  return ret;
}

/**
  * @brief  Linear acceleration output register. The value is expressed as a
  *         16-bit word in two’s complement.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_acceleration_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_OUTX_L_A, buff, 6);
  return ret;
}

/**
  * @brief  FIFO data output.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_out_raw_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_DATA_OUT_X_L, buff, 6);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_common
  * @brief      This section groups common usefull functions.
  * @{
  *
  */

/**
  * @brief  Difference in percentage of the effective ODR (and timestamp rate)
  *         with respect to the typical.[set]
  *         Step:  0.15%. 8-bit format, 2's complement.
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of freq_fine in reg INTERNAL_FREQ_FINE
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_odr_cal_reg_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_internal_freq_fine_t internal_freq_fine;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INTERNAL_FREQ_FINE,
                           (uint8_t*)&internal_freq_fine, 1);
  if(ret == 0){
    internal_freq_fine.freq_fine= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_INTERNAL_FREQ_FINE,
                              (uint8_t*)&internal_freq_fine, 1);
  }
  return ret;
}

/**
  * @brief  Difference in percentage of the effective ODR (and timestamp rate)
  *         with respect to the typical.[get]
  *         Step:  0.15%. 8-bit format, 2's complement.
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of freq_fine in reg INTERNAL_FREQ_FINE
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_odr_cal_reg_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_internal_freq_fine_t internal_freq_fine;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INTERNAL_FREQ_FINE,
                           (uint8_t*)&internal_freq_fine, 1);
  *val = internal_freq_fine.freq_fine;

  return ret;
}

/**
  * @brief  Data-ready pulsed / letched mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of dataready_pulsed in
  *                reg COUNTER_BDR_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_data_ready_mode_set(asm330lhh_ctx_t *ctx,
                                      asm330lhh_dataready_pulsed_t val)
{
  asm330lhh_counter_bdr_reg1_t counter_bdr_reg1;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                           (uint8_t*)&counter_bdr_reg1, 1);
  if(ret == 0){
    counter_bdr_reg1.dataready_pulsed= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                              (uint8_t*)&counter_bdr_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Data-ready pulsed / letched mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dataready_pulsed in
  *                reg COUNTER_BDR_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_data_ready_mode_get(asm330lhh_ctx_t *ctx,
                                      asm330lhh_dataready_pulsed_t *val)
{
  asm330lhh_counter_bdr_reg1_t counter_bdr_reg1;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                           (uint8_t*)&counter_bdr_reg1, 1);
  switch (counter_bdr_reg1.dataready_pulsed){
    case ASM330LHH_DRDY_LATCHED:
      *val = ASM330LHH_DRDY_LATCHED;
      break;
    case ASM330LHH_DRDY_PULSED:
      *val = ASM330LHH_DRDY_PULSED;
      break;
    default:
      *val = ASM330LHH_DRDY_LATCHED;
      break;
  }
  return ret;
}

/**
  * @brief  Device Who am I.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_device_id_get(asm330lhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sw_reset in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_reset_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.sw_reset= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sw_reset in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_reset_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  *val = ctrl3_c.sw_reset;

  return ret;
}

/**
  * @brief  Register address automatically incremented during a multiple byte
  *         access with a serial interface.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of if_inc in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_auto_increment_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.if_inc= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief  Register address automatically incremented during a multiple byte
  *         access with a serial interface.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of if_inc in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_auto_increment_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  *val = ctrl3_c.if_inc;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of boot in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_boot_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.boot= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of boot in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_boot_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  *val = ctrl3_c.boot;

  return ret;
}



/**
  * @brief  Linear acceleration sensor self-test enable.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of st_xl in reg CTRL5_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_self_test_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_st_xl_t val)
{
  asm330lhh_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  if(ret == 0){
    ctrl5_c.st_xl= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  }
  return ret;
}

/**
  * @brief  Linear acceleration sensor self-test enable.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st_xl in reg CTRL5_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_self_test_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_st_xl_t *val)
{
  asm330lhh_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);

  switch (ctrl5_c.st_xl){
    case ASM330LHH_XL_ST_DISABLE:
      *val = ASM330LHH_XL_ST_DISABLE;
      break;
    case ASM330LHH_XL_ST_POSITIVE:
      *val = ASM330LHH_XL_ST_POSITIVE;
      break;
    case ASM330LHH_XL_ST_NEGATIVE:
      *val = ASM330LHH_XL_ST_NEGATIVE;
      break;
    default:
      *val = ASM330LHH_XL_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Angular rate sensor self-test enable.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of st_g in reg CTRL5_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_self_test_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_st_g_t val)
{
  asm330lhh_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  if(ret == 0){
    ctrl5_c.st_g= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);
  }
  return ret;
}

/**
  * @brief  Angular rate sensor self-test enable.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st_g in reg CTRL5_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_self_test_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_st_g_t *val)
{
  asm330lhh_ctrl5_c_t ctrl5_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL5_C, (uint8_t*)&ctrl5_c, 1);

  switch (ctrl5_c.st_g){
    case ASM330LHH_GY_ST_DISABLE:
      *val = ASM330LHH_GY_ST_DISABLE;
      break;
    case ASM330LHH_GY_ST_POSITIVE:
      *val = ASM330LHH_GY_ST_POSITIVE;
      break;
    case ASM330LHH_GY_ST_NEGATIVE:
      *val = ASM330LHH_GY_ST_NEGATIVE;
      break;
    default:
      *val = ASM330LHH_GY_ST_DISABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_filters
  * @brief      This section group all the functions concerning the
  *             filters configuration
  * @{
  *
  */

/**
  * @brief  Accelerometer output from LPF2 filtering stage selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lpf2_xl_en in reg CTRL1_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_filter_lp2_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  if(ret == 0){
    ctrl1_xl.lpf2_xl_en= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL1_XL,
                              (uint8_t*)&ctrl1_xl, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer output from LPF2 filtering stage selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lpf2_xl_en in reg CTRL1_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_filter_lp2_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl1_xl_t ctrl1_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL1_XL, (uint8_t*)&ctrl1_xl, 1);
  *val = ctrl1_xl.lpf2_xl_en;

  return ret;
}

/**
  * @brief  Enables gyroscope digital LPF1 if auxiliary SPI is disabled;
  *         the bandwidth can be selected through FTYPE [2:0] in CTRL6_C.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lpf1_sel_g in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_filter_lp1_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.lpf1_sel_g= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief  Enables gyroscope digital LPF1 if auxiliary SPI is disabled;
  *         the bandwidth can be selected through FTYPE [2:0] in CTRL6_C.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lpf1_sel_g in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_filter_lp1_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = ctrl4_c.lpf1_sel_g;

  return ret;
}

/**
  * @brief  Mask DRDY on pin (both XL & Gyro) until filter settling ends
  *         (XL and Gyro independently masked).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of drdy_mask in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_filter_settling_mask_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.drdy_mask= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief  Mask DRDY on pin (both XL & Gyro) until filter settling ends
  *         (XL and Gyro independently masked).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of drdy_mask in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_filter_settling_mask_get(asm330lhh_ctx_t *ctx,
                                           uint8_t *val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = ctrl4_c.drdy_mask;

  return ret;
}

/**
  * @brief  Gyroscope low pass filter 1 bandwidth.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ftype in reg CTRL6_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_lp1_bandwidth_set(asm330lhh_ctx_t *ctx,
                                       asm330lhh_ftype_t val)
{
  asm330lhh_ctrl6_g_t ctrl6_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);
  if(ret == 0){
    ctrl6_c.ftype= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);
  }
  return ret;
}

/**
  * @brief  Gyroscope low pass filter 1 bandwidth.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ftype in reg CTRL6_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_lp1_bandwidth_get(asm330lhh_ctx_t *ctx,
                                       asm330lhh_ftype_t *val)
{
  asm330lhh_ctrl6_g_t ctrl6_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);

  switch (ctrl6_c.ftype){
    case ASM330LHH_ULTRA_LIGHT:
      *val = ASM330LHH_ULTRA_LIGHT;
      break;
    case ASM330LHH_VERY_LIGHT:
      *val = ASM330LHH_VERY_LIGHT;
      break;
    case ASM330LHH_LIGHT:
      *val = ASM330LHH_LIGHT;
      break;
    case ASM330LHH_MEDIUM:
      *val = ASM330LHH_MEDIUM;
      break;
    case ASM330LHH_STRONG:
      *val = ASM330LHH_STRONG;
      break;
    case ASM330LHH_VERY_STRONG:
      *val = ASM330LHH_VERY_STRONG;
      break;
    case ASM330LHH_AGGRESSIVE:
      *val = ASM330LHH_AGGRESSIVE;
      break;
    case ASM330LHH_XTREME:
      *val = ASM330LHH_XTREME;
      break;
    default:
      *val = ASM330LHH_ULTRA_LIGHT;
      break;
  }
  return ret;
}

/**
  * @brief  Low pass filter 2 on 6D function selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of low_pass_on_6d in reg CTRL8_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_lp2_on_6d_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  if(ret == 0){
    ctrl8_xl.low_pass_on_6d= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL8_XL,
                              (uint8_t*)&ctrl8_xl, 1);
  }
  return ret;
}

/**
  * @brief  Low pass filter 2 on 6D function selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of low_pass_on_6d in reg CTRL8_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_lp2_on_6d_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  *val = ctrl8_xl.low_pass_on_6d;

  return ret;
}

/**
  * @brief  Accelerometer slope filter / high-pass filter selection
  *         on output.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of hp_slope_xl_en in reg CTRL8_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_hp_path_on_out_set(asm330lhh_ctx_t *ctx,
                                        asm330lhh_hp_slope_xl_en_t val)
{
  asm330lhh_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  if(ret == 0){
    ctrl8_xl.hp_slope_xl_en = (((uint8_t)val & 0x10U) >> 4);
    ctrl8_xl.hp_ref_mode_xl = (((uint8_t)val & 0x20U) >> 5);
    ctrl8_xl.hpcf_xl = (uint8_t)val & 0x07U;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL8_XL,
                              (uint8_t*)&ctrl8_xl, 1);
  }
  return ret;
}

/**
  * @brief  Accelerometer slope filter / high-pass filter selection on
  *         output.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hp_slope_xl_en in reg CTRL8_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_hp_path_on_out_get(asm330lhh_ctx_t *ctx,
                                        asm330lhh_hp_slope_xl_en_t *val)
{
  asm330lhh_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  switch (( (ctrl8_xl.hp_ref_mode_xl << 5) +(ctrl8_xl.hp_slope_xl_en << 4) +
             ctrl8_xl.hpcf_xl )){
    case ASM330LHH_HP_PATH_DISABLE_ON_OUT:
      *val = ASM330LHH_HP_PATH_DISABLE_ON_OUT;
      break;
    case ASM330LHH_SLOPE_ODR_DIV_4:
      *val = ASM330LHH_SLOPE_ODR_DIV_4;
      break;
    case ASM330LHH_HP_ODR_DIV_10:
      *val = ASM330LHH_HP_ODR_DIV_10;
      break;
    case ASM330LHH_HP_ODR_DIV_20:
      *val = ASM330LHH_HP_ODR_DIV_20;
      break;
    case ASM330LHH_HP_ODR_DIV_45:
      *val = ASM330LHH_HP_ODR_DIV_45;
      break;
    case ASM330LHH_HP_ODR_DIV_100:
      *val = ASM330LHH_HP_ODR_DIV_100;
      break;
    case ASM330LHH_HP_ODR_DIV_200:
      *val = ASM330LHH_HP_ODR_DIV_200;
      break;
    case ASM330LHH_HP_ODR_DIV_400:
      *val = ASM330LHH_HP_ODR_DIV_400;
      break;
    case ASM330LHH_HP_ODR_DIV_800:
      *val = ASM330LHH_HP_ODR_DIV_800;
      break;
    case ASM330LHH_HP_REF_MD_ODR_DIV_10:
      *val = ASM330LHH_HP_REF_MD_ODR_DIV_10;
      break;
    case ASM330LHH_HP_REF_MD_ODR_DIV_20:
      *val = ASM330LHH_HP_REF_MD_ODR_DIV_20;
      break;
    case ASM330LHH_HP_REF_MD_ODR_DIV_45:
      *val = ASM330LHH_HP_REF_MD_ODR_DIV_45;
      break;
    case ASM330LHH_HP_REF_MD_ODR_DIV_100:
      *val = ASM330LHH_HP_REF_MD_ODR_DIV_100;
      break;
    case ASM330LHH_HP_REF_MD_ODR_DIV_200:
      *val = ASM330LHH_HP_REF_MD_ODR_DIV_200;
      break;
    case ASM330LHH_HP_REF_MD_ODR_DIV_400:
      *val = ASM330LHH_HP_REF_MD_ODR_DIV_400;
      break;
    case ASM330LHH_HP_REF_MD_ODR_DIV_800:
      *val = ASM330LHH_HP_REF_MD_ODR_DIV_800;
      break;
    case ASM330LHH_LP_ODR_DIV_10:
      *val = ASM330LHH_LP_ODR_DIV_10;
      break;
    case ASM330LHH_LP_ODR_DIV_20:
      *val = ASM330LHH_LP_ODR_DIV_20;
      break;
    case ASM330LHH_LP_ODR_DIV_45:
      *val = ASM330LHH_LP_ODR_DIV_45;
      break;
    case ASM330LHH_LP_ODR_DIV_100:
      *val = ASM330LHH_LP_ODR_DIV_100;
      break;
    case ASM330LHH_LP_ODR_DIV_200:
      *val = ASM330LHH_LP_ODR_DIV_200;
      break;
    case ASM330LHH_LP_ODR_DIV_400:
      *val = ASM330LHH_LP_ODR_DIV_400;
      break;
    case ASM330LHH_LP_ODR_DIV_800:
      *val = ASM330LHH_LP_ODR_DIV_800;
      break;
    default:
      *val = ASM330LHH_HP_PATH_DISABLE_ON_OUT;
      break;
  }
  return ret;
}

/**
  * @brief  Enables accelerometer LPF2 and HPF fast-settling mode.
  *         The filter sets the second samples after writing this bit.
  *         Active only during device exit from powerdown mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fastsettl_mode_xl in reg CTRL8_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_fast_settling_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  if(ret == 0){
    ctrl8_xl.fastsettl_mode_xl= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL8_XL,
                              (uint8_t*)&ctrl8_xl, 1);
  }
  return ret;
}

/**
  * @brief  Enables accelerometer LPF2 and HPF fast-settling mode.
  *         The filter sets the second samples after writing
  *         this bit. Active only during device exit from powerdown mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fastsettl_mode_xl in reg CTRL8_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_fast_settling_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl8_xl_t ctrl8_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL8_XL, (uint8_t*)&ctrl8_xl, 1);
  *val = ctrl8_xl.fastsettl_mode_xl;

  return ret;
}

/**
  * @brief  HPF or SLOPE filter selection on wake-up and Activity/Inactivity
  *         functions.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of slope_fds in reg TAP_CFG0
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_hp_path_internal_set(asm330lhh_ctx_t *ctx,
                                          asm330lhh_slope_fds_t val)
{
  asm330lhh_int_cfg0_t int_cfg0;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG0, (uint8_t*)&int_cfg0, 1);
  if(ret == 0){
    int_cfg0.slope_fds= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_INT_CFG0,
                              (uint8_t*)&int_cfg0, 1);
  }
  return ret;
}

/**
  * @brief  HPF or SLOPE filter selection on wake-up and Activity/Inactivity
  *         functions.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of slope_fds in reg TAP_CFG0
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_hp_path_internal_get(asm330lhh_ctx_t *ctx,
                                          asm330lhh_slope_fds_t *val)
{
  asm330lhh_int_cfg0_t int_cfg0;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG0, (uint8_t*)&int_cfg0, 1);
  switch (int_cfg0.slope_fds){
    case ASM330LHH_USE_SLOPE:
      *val = ASM330LHH_USE_SLOPE;
      break;
    case ASM330LHH_USE_HPF:
      *val = ASM330LHH_USE_HPF;
      break;
    default:
      *val = ASM330LHH_USE_SLOPE;
      break;
  }
  return ret;
}

/**
  * @brief  Enables gyroscope digital high-pass filter. The filter is enabled
  *         only if the gyro is in HP mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hp_en_g and hp_en_g in reg CTRL7_G
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_hp_path_internal_set(asm330lhh_ctx_t *ctx,
                                          asm330lhh_hpm_g_t val)
{
  asm330lhh_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  if(ret == 0){
    ctrl7_g.hp_en_g = (((uint8_t)val & 0x80U) >> 7);
    ctrl7_g.hpm_g = (uint8_t)val & 0x03U;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL7_G, (uint8_t*)&ctrl7_g, 1);
  }
  return ret;
}

/**
  * @brief    Enables gyroscope digital high-pass filter. The filter is
  *           enabled only if the gyro is in HP mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of hp_en_g and hp_en_g in reg CTRL7_G
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_hp_path_internal_get(asm330lhh_ctx_t *ctx,
                                          asm330lhh_hpm_g_t *val)
{
  asm330lhh_ctrl7_g_t ctrl7_g;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL7_G, (uint8_t*)&ctrl7_g, 1);

  switch ((ctrl7_g.hp_en_g << 7) + ctrl7_g.hpm_g){
    case ASM330LHH_HP_FILTER_NONE:
      *val = ASM330LHH_HP_FILTER_NONE;
      break;
    case ASM330LHH_HP_FILTER_16mHz:
      *val = ASM330LHH_HP_FILTER_16mHz;
      break;
    case ASM330LHH_HP_FILTER_65mHz:
      *val = ASM330LHH_HP_FILTER_65mHz;
      break;
    case ASM330LHH_HP_FILTER_260mHz:
      *val = ASM330LHH_HP_FILTER_260mHz;
      break;
    case ASM330LHH_HP_FILTER_1Hz04:
      *val = ASM330LHH_HP_FILTER_1Hz04;
      break;
    default:
      *val = ASM330LHH_HP_FILTER_NONE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_ main_serial_interface
  * @brief      This section groups all the functions concerning main
  *             serial interface management (not auxiliary)
  * @{
  *
  */

/**
  * @brief  Connect/Disconnect SDO/SA0 internal pull-up.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sdo_pu_en in reg PIN_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_sdo_sa0_mode_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_sdo_pu_en_t val)
{
  asm330lhh_pin_ctrl_t pin_ctrl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_PIN_CTRL, (uint8_t*)&pin_ctrl, 1);
  if(ret == 0){
    pin_ctrl.sdo_pu_en= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_PIN_CTRL, (uint8_t*)&pin_ctrl, 1);
  }
  return ret;
}

/**
  * @brief  Connect/Disconnect SDO/SA0 internal pull-up.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sdo_pu_en in reg PIN_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_sdo_sa0_mode_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_sdo_pu_en_t *val)
{
  asm330lhh_pin_ctrl_t pin_ctrl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_PIN_CTRL, (uint8_t*)&pin_ctrl, 1);

  switch (pin_ctrl.sdo_pu_en){
    case ASM330LHH_PULL_UP_DISC:
      *val = ASM330LHH_PULL_UP_DISC;
      break;
    case ASM330LHH_PULL_UP_CONNECT:
      *val = ASM330LHH_PULL_UP_CONNECT;
      break;
    default:
      *val = ASM330LHH_PULL_UP_DISC;
      break;
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sim in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_spi_mode_set(asm330lhh_ctx_t *ctx, asm330lhh_sim_t val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.sim= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sim in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_spi_mode_get(asm330lhh_ctx_t *ctx, asm330lhh_sim_t *val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);

  switch (ctrl3_c.sim){
    case ASM330LHH_SPI_4_WIRE:
      *val = ASM330LHH_SPI_4_WIRE;
      break;
    case ASM330LHH_SPI_3_WIRE:
      *val = ASM330LHH_SPI_3_WIRE;
      break;
    default:
      *val = ASM330LHH_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief  Disable / Enable I2C interface.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of i2c_disable in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_i2c_interface_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_i2c_disable_t val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.i2c_disable= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief  Disable / Enable I2C interface.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of i2c reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_i2c_interface_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_i2c_disable_t *val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);

  switch (ctrl4_c.i2c_disable){
    case ASM330LHH_I2C_ENABLE:
      *val = ASM330LHH_I2C_ENABLE;
      break;
    case ASM330LHH_I2C_DISABLE:
      *val = ASM330LHH_I2C_DISABLE;
      break;
    default:
      *val = ASM330LHH_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_interrupt_pins
  * @brief      This section groups all the functions that manage
  *             interrup pins
  * @{
  *
  */

/**
  * @brief   Select the signal that need to route on int1 pad[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Structure of registers: INT1_CTRL,MD1_CFG,
  *                EMB_FUNC_INT1, FSM_INT1_A, FSM_INT1_B
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_pin_int1_route_set(asm330lhh_ctx_t *ctx,
                                     asm330lhh_pin_int1_route_t *val)
{
  asm330lhh_int_cfg1_t tap_cfg2;
  int32_t ret;

  ret = asm330lhh_write_reg(ctx, ASM330LHH_INT1_CTRL,
                            (uint8_t*)&val->int1_ctrl, 1);

  if(ret == 0){
    ret = asm330lhh_write_reg(ctx, ASM330LHH_MD1_CFG,
                              (uint8_t*)&val->md1_cfg, 1);
  }
  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG1, (uint8_t*)&tap_cfg2, 1);
    if ((val->int1_ctrl.den_drdy_flag |
         val->int1_ctrl.int1_boot |
         val->int1_ctrl.int1_cnt_bdr |
         val->int1_ctrl.int1_drdy_g |
         val->int1_ctrl.int1_drdy_xl |
         val->int1_ctrl.int1_fifo_full |
         val->int1_ctrl.int1_fifo_ovr |
         val->int1_ctrl.int1_fifo_th |
         val->md1_cfg.int1_6d |
         val->md1_cfg.int1_ff |
         val->md1_cfg.int1_wu |
         val->md1_cfg.int1_sleep_change)!= PROPERTY_DISABLE){
      tap_cfg2.interrupts_enable = PROPERTY_ENABLE;
    }
    else{
      tap_cfg2.interrupts_enable = PROPERTY_DISABLE;
    }
  }
  if(ret == 0){
    ret = asm330lhh_write_reg(ctx, ASM330LHH_INT_CFG1,
                              (uint8_t*)&tap_cfg2, 1);
  }
  return ret;
}

/**
  * @brief  Select the signal that need to route on int1 pad.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Structure of registers: INT1_CTRL, MD1_CFG,
  *                EMB_FUNC_INT1, FSM_INT1_A, FSM_INT1_B.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_pin_int1_route_get(asm330lhh_ctx_t *ctx,
                                     asm330lhh_pin_int1_route_t *val)
{
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT1_CTRL,
                           (uint8_t*)&val->int1_ctrl, 1);

  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_MD1_CFG,
                             (uint8_t*)&val->md1_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Select the signal that need to route on int2 pad[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Structure of registers INT2_CTRL,  MD2_CFG,
  *                EMB_FUNC_INT2, FSM_INT2_A, FSM_INT2_B
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_pin_int2_route_set(asm330lhh_ctx_t *ctx,
                                     asm330lhh_pin_int2_route_t *val)
{
  asm330lhh_int_cfg1_t tap_cfg2;
  int32_t ret;


  ret = asm330lhh_write_reg(ctx, ASM330LHH_INT2_CTRL,
                            (uint8_t*)&val->int2_ctrl, 1);

  if(ret == 0){
    ret = asm330lhh_write_reg(ctx, ASM330LHH_MD2_CFG,
                              (uint8_t*)&val->md2_cfg, 1);
  }
  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG1,
                             (uint8_t*)&tap_cfg2, 1);
  }
  if(ret == 0){
    if ((val->int2_ctrl.int2_drdy_xl |
         val->int2_ctrl.int2_drdy_g |
         val->int2_ctrl.int2_drdy_temp |
         val->int2_ctrl.int2_fifo_th |
         val->int2_ctrl.int2_fifo_ovr |
         val->int2_ctrl.int2_fifo_full |
         val->int2_ctrl.int2_cnt_bdr |
         val->md2_cfg.int2_6d |
         val->md2_cfg.int2_ff |
         val->md2_cfg.int2_wu |
         val->md2_cfg.int2_sleep_change) != PROPERTY_DISABLE){
      tap_cfg2.interrupts_enable = PROPERTY_ENABLE;
    }
    else{
      tap_cfg2.interrupts_enable = PROPERTY_DISABLE;
    }
    ret = asm330lhh_write_reg(ctx, ASM330LHH_INT_CFG1,
                              (uint8_t*)&tap_cfg2, 1);
  }
  return ret;
}

/**
  * @brief  Select the signal that need to route on int2 pad.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Structure of registers INT2_CTRL,  MD2_CFG,
  *                EMB_FUNC_INT2, FSM_INT2_A, FSM_INT2_B.[get]
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_pin_int2_route_get(asm330lhh_ctx_t *ctx,
                                     asm330lhh_pin_int2_route_t *val)
{
  int32_t ret;

    ret = asm330lhh_read_reg(ctx, ASM330LHH_INT2_CTRL,
                             (uint8_t*)&val->int2_ctrl, 1);
  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_MD2_CFG,
                             (uint8_t*)&val->md2_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of pp_od in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_pin_mode_set(asm330lhh_ctx_t *ctx, asm330lhh_pp_od_t val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.pp_od= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of pp_od in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_pin_mode_get(asm330lhh_ctx_t *ctx, asm330lhh_pp_od_t *val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);

  switch (ctrl3_c.pp_od){
    case ASM330LHH_PUSH_PULL:
      *val = ASM330LHH_PUSH_PULL;
      break;
    case ASM330LHH_OPEN_DRAIN:
      *val = ASM330LHH_OPEN_DRAIN;
      break;
    default:
      *val = ASM330LHH_PUSH_PULL;
      break;
  }
  return ret;
}

/**
  * @brief  Interrupt active-high/low.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of h_lactive in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_pin_polarity_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_h_lactive_t val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  if(ret == 0){
    ctrl3_c.h_lactive= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);
  }
  return ret;
}

/**
  * @brief  Interrupt active-high/low.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of h_lactive in reg CTRL3_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_pin_polarity_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_h_lactive_t *val)
{
  asm330lhh_ctrl3_c_t ctrl3_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL3_C, (uint8_t*)&ctrl3_c, 1);

  switch (ctrl3_c.h_lactive){
    case ASM330LHH_ACTIVE_HIGH:
      *val = ASM330LHH_ACTIVE_HIGH;
      break;
    case ASM330LHH_ACTIVE_LOW:
      *val = ASM330LHH_ACTIVE_LOW;
      break;
    default:
      *val = ASM330LHH_ACTIVE_HIGH;
      break;
  }
  return ret;
}

/**
  * @brief  All interrupt signals become available on INT1 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int2_on_int1 in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_all_on_int1_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.int2_on_int1= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief  All interrupt signals become available on INT1 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int2_on_int1 in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_all_on_int1_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = ctrl4_c.int2_on_int1;

  return ret;
}

/**
  * @brief  All interrupt signals notification mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of lir in reg TAP_CFG0
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_int_notification_set(asm330lhh_ctx_t *ctx,
                                       asm330lhh_lir_t val)
{
  asm330lhh_int_cfg0_t int_cfg0;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG0, (uint8_t*)&int_cfg0, 1);
  if(ret == 0){
    int_cfg0.lir = (uint8_t)val & 0x01U;
    int_cfg0.int_clr_on_read = (uint8_t)val & 0x01U;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_INT_CFG0,
                              (uint8_t*)&int_cfg0, 1);
  }
  return ret;
}

/**
  * @brief  All interrupt signals notification mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of lir in reg TAP_CFG0
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_int_notification_get(asm330lhh_ctx_t *ctx,
                                       asm330lhh_lir_t *val)
{
  asm330lhh_int_cfg0_t int_cfg0;
  int32_t ret;

  *val = ASM330LHH_ALL_INT_PULSED;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG0, (uint8_t*)&int_cfg0, 1);

  switch ((int_cfg0.lir << 1) + int_cfg0.int_clr_on_read){
    case ASM330LHH_ALL_INT_PULSED:
      *val = ASM330LHH_ALL_INT_PULSED;
      break;
    case ASM330LHH_ALL_INT_LATCHED:
      *val = ASM330LHH_ALL_INT_LATCHED;
      break;
    default:
      *val = ASM330LHH_ALL_INT_PULSED;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_Wake_Up_event
  * @brief      This section groups all the functions that manage the
  *             Wake Up event generation.
  * @{
  *
  */

/**
  * @brief  Weight of 1 LSB of wakeup threshold.[set]
  *         0: 1 LSB =FS_XL  /  64
  *         1: 1 LSB = FS_XL / 256
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of wake_ths_w in reg WAKE_UP_DUR
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_wkup_ths_weight_set(asm330lhh_ctx_t *ctx,
                                      asm330lhh_wake_ths_w_t val)
{
  asm330lhh_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                           (uint8_t*)&wake_up_dur, 1);
  if(ret == 0){
    wake_up_dur.wake_ths_w= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                              (uint8_t*)&wake_up_dur, 1);
  }
  return ret;
}

/**
  * @brief  Weight of 1 LSB of wakeup threshold.[get]
  *         0: 1 LSB =FS_XL  /  64
  *         1: 1 LSB = FS_XL / 256
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of wake_ths_w in reg WAKE_UP_DUR
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_wkup_ths_weight_get(asm330lhh_ctx_t *ctx,
                                      asm330lhh_wake_ths_w_t *val)
{
  asm330lhh_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                           (uint8_t*)&wake_up_dur, 1);

  switch (wake_up_dur.wake_ths_w){
    case ASM330LHH_LSb_FS_DIV_64:
      *val = ASM330LHH_LSb_FS_DIV_64;
      break;
    case ASM330LHH_LSb_FS_DIV_256:
      *val = ASM330LHH_LSb_FS_DIV_256;
      break;
    default:
      *val = ASM330LHH_LSb_FS_DIV_64;
      break;
  }
  return ret;
}

/**
  * @brief  Threshold for wakeup: 1 LSB weight depends on WAKE_THS_W in
  *         WAKE_UP_DUR.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of wk_ths in reg WAKE_UP_THS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_wkup_threshold_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_THS,
                           (uint8_t*)&wake_up_ths, 1);
  if(ret == 0){
    wake_up_ths.wk_ths= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_WAKE_UP_THS,
                              (uint8_t*)&wake_up_ths, 1);
  }
  return ret;
}

/**
  * @brief  Threshold for wakeup: 1 LSB weight depends on WAKE_THS_W in
  *         WAKE_UP_DUR.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of wk_ths in reg WAKE_UP_THS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_wkup_threshold_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_THS,
                           (uint8_t*)&wake_up_ths, 1);
  *val = wake_up_ths.wk_ths;

  return ret;
}

/**
  * @brief  Wake up duration event( 1LSb = 1 / ODR ).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of usr_off_on_wu in reg WAKE_UP_THS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_on_wkup_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_THS,
                           (uint8_t*)&wake_up_ths, 1);
  if(ret == 0){
    wake_up_ths.usr_off_on_wu= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_WAKE_UP_THS,
                              (uint8_t*)&wake_up_ths, 1);
  }
  return ret;
}

/**
  * @brief  Wake up duration event( 1LSb = 1 / ODR ).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of usr_off_on_wu in reg WAKE_UP_THS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_xl_usr_offset_on_wkup_get(asm330lhh_ctx_t *ctx,
                                            uint8_t *val)
{
  asm330lhh_wake_up_ths_t wake_up_ths;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_THS,
                           (uint8_t*)&wake_up_ths, 1);
  *val = wake_up_ths.usr_off_on_wu;

  return ret;
}

/**
  * @brief  Wake up duration event(1LSb = 1 / ODR).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of wake_dur in reg WAKE_UP_DUR
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_wkup_dur_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                           (uint8_t*)&wake_up_dur, 1);
  if(ret == 0){
    wake_up_dur.wake_dur= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                              (uint8_t*)&wake_up_dur, 1);
  }
  return ret;
}

/**
  * @brief  Wake up duration event(1LSb = 1 / ODR).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of wake_dur in reg WAKE_UP_DUR
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_wkup_dur_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                           (uint8_t*)&wake_up_dur, 1);
  *val = wake_up_dur.wake_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_ Activity/Inactivity_detection
  * @brief      This section groups all the functions concerning
  *             activity/inactivity detection.
  * @{
  *
  */

/**
  * @brief  Enables gyroscope Sleep mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sleep_g in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_sleep_mode_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  if(ret == 0){
    ctrl4_c.sleep_g= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  }
  return ret;
}

/**
  * @brief  Enables gyroscope Sleep mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sleep_g in reg CTRL4_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_gy_sleep_mode_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl4_c_t ctrl4_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL4_C, (uint8_t*)&ctrl4_c, 1);
  *val = ctrl4_c.sleep_g;

  return ret;
}

/**
  * @brief  Drives the sleep status instead of sleep change on INT pins
  *         (only if INT1_SLEEP_CHANGE or INT2_SLEEP_CHANGE bits
  *         are enabled).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sleep_status_on_int in reg TAP_CFG0
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_act_pin_notification_set(asm330lhh_ctx_t *ctx,
                                           asm330lhh_sleep_status_on_int_t val)
{
  asm330lhh_int_cfg0_t int_cfg0;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG0, (uint8_t*)&int_cfg0, 1);
  if(ret == 0){
    int_cfg0. sleep_status_on_int= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_INT_CFG0,
                              (uint8_t*)&int_cfg0, 1);
  }
  return ret;
}

/**
  * @brief  Drives the sleep status instead of sleep change on INT pins
  *         (only if INT1_SLEEP_CHANGE or INT2_SLEEP_CHANGE bits
  *         are enabled).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sleep_status_on_int in reg TAP_CFG0
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_act_pin_notification_get(asm330lhh_ctx_t *ctx,
                                       asm330lhh_sleep_status_on_int_t *val)
{
  asm330lhh_int_cfg0_t int_cfg0;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG0, (uint8_t*)&int_cfg0, 1);
  switch (int_cfg0. sleep_status_on_int){
    case ASM330LHH_DRIVE_SLEEP_CHG_EVENT:
      *val = ASM330LHH_DRIVE_SLEEP_CHG_EVENT;
      break;
    case ASM330LHH_DRIVE_SLEEP_STATUS:
      *val = ASM330LHH_DRIVE_SLEEP_STATUS;
      break;
    default:
      *val = ASM330LHH_DRIVE_SLEEP_CHG_EVENT;
      break;
  }
  return ret;
}

/**
  * @brief  Enable inactivity function.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of inact_en in reg TAP_CFG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_act_mode_set(asm330lhh_ctx_t *ctx, asm330lhh_inact_en_t val)
{
  asm330lhh_int_cfg1_t tap_cfg2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG1, (uint8_t*)&tap_cfg2, 1);
  if(ret == 0){
    tap_cfg2.inact_en= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_INT_CFG1, (uint8_t*)&tap_cfg2, 1);
  }
  return ret;
}

/**
  * @brief  Enable inactivity function.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of inact_en in reg TAP_CFG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_act_mode_get(asm330lhh_ctx_t *ctx,
                               asm330lhh_inact_en_t *val)
{
  asm330lhh_int_cfg1_t tap_cfg2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_INT_CFG1, (uint8_t*)&tap_cfg2, 1);

  switch (tap_cfg2.inact_en){
    case ASM330LHH_XL_AND_GY_NOT_AFFECTED:
      *val = ASM330LHH_XL_AND_GY_NOT_AFFECTED;
      break;
    case ASM330LHH_XL_12Hz5_GY_NOT_AFFECTED:
      *val = ASM330LHH_XL_12Hz5_GY_NOT_AFFECTED;
      break;
    case ASM330LHH_XL_12Hz5_GY_SLEEP:
      *val = ASM330LHH_XL_12Hz5_GY_SLEEP;
      break;
    case ASM330LHH_XL_12Hz5_GY_PD:
      *val = ASM330LHH_XL_12Hz5_GY_PD;
      break;
    default:
      *val = ASM330LHH_XL_AND_GY_NOT_AFFECTED;
      break;
  }
  return ret;
}

/**
  * @brief  Duration to go in sleep mode (1 LSb = 512 / ODR).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sleep_dur in reg WAKE_UP_DUR
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_act_sleep_dur_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                           (uint8_t*)&wake_up_dur, 1);
  if(ret == 0){
    wake_up_dur.sleep_dur= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                              (uint8_t*)&wake_up_dur, 1);
  }
  return ret;
}

/**
  * @brief  Duration to go in sleep mode.(1 LSb = 512 / ODR).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sleep_dur in reg WAKE_UP_DUR
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_act_sleep_dur_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_wake_up_dur_t wake_up_dur;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                           (uint8_t*)&wake_up_dur, 1);
  *val = wake_up_dur.sleep_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_ Six_position_detection(6D/4D)
  * @brief      This section groups all the functions concerning six
  *             position detection (6D).
  * @{
  *
  */

/**
  * @brief  Threshold for 4D/6D function.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sixd_ths in reg TAP_THS_6D
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_6d_threshold_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_sixd_ths_t val)
{
  asm330lhh_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_THS_6D,
                           (uint8_t*)&tap_ths_6d, 1);
  if(ret == 0){
    tap_ths_6d.sixd_ths= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_THS_6D,
                              (uint8_t*)&tap_ths_6d, 1);
  }
  return ret;
}

/**
  * @brief  Threshold for 4D/6D function.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sixd_ths in reg TAP_THS_6D
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_6d_threshold_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_sixd_ths_t *val)
{
  asm330lhh_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_THS_6D,
                           (uint8_t*)&tap_ths_6d, 1);

  switch (tap_ths_6d.sixd_ths){
    case ASM330LHH_DEG_80:
      *val = ASM330LHH_DEG_80;
      break;
    case ASM330LHH_DEG_70:
      *val = ASM330LHH_DEG_70;
      break;
    case ASM330LHH_DEG_60:
      *val = ASM330LHH_DEG_60;
      break;
    case ASM330LHH_DEG_50:
      *val = ASM330LHH_DEG_50;
      break;
    default:
      *val = ASM330LHH_DEG_80;
      break;
  }
  return ret;
}

/**
  * @brief  4D orientation detection enable.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of d4d_en in reg TAP_THS_6D
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_4d_mode_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_THS_6D,
                           (uint8_t*)&tap_ths_6d, 1);
  if(ret == 0){
    tap_ths_6d.d4d_en= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_THS_6D,
                              (uint8_t*)&tap_ths_6d, 1);
  }
  return ret;
}

/**
  * @brief  4D orientation detection enable.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of d4d_en in reg TAP_THS_6D
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_4d_mode_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ths_6d_t tap_ths_6d;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_THS_6D,
                           (uint8_t*)&tap_ths_6d, 1);
  *val = tap_ths_6d.d4d_en;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_free_fall
  * @brief      This section group all the functions concerning the free
  *             fall detection.
  * @{
  *
  */

/**
  * @brief  Free fall threshold setting.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ff_ths in reg FREE_FALL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_ff_threshold_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_ff_ths_t val)
{
  asm330lhh_free_fall_t free_fall;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FREE_FALL, (uint8_t*)&free_fall, 1);
  if(ret == 0){
    free_fall.ff_ths= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FREE_FALL,
                              (uint8_t*)&free_fall, 1);
  }
  return ret;
}

/**
  * @brief  Free fall threshold setting.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ff_ths in reg FREE_FALL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_ff_threshold_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_ff_ths_t *val)
{
  asm330lhh_free_fall_t free_fall;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FREE_FALL, (uint8_t*)&free_fall, 1);

  switch (free_fall.ff_ths){
    case ASM330LHH_FF_TSH_156mg:
      *val = ASM330LHH_FF_TSH_156mg;
      break;
    case ASM330LHH_FF_TSH_219mg:
      *val = ASM330LHH_FF_TSH_219mg;
      break;
    case ASM330LHH_FF_TSH_250mg:
      *val = ASM330LHH_FF_TSH_250mg;
      break;
    case ASM330LHH_FF_TSH_312mg:
      *val = ASM330LHH_FF_TSH_312mg;
      break;
    case ASM330LHH_FF_TSH_344mg:
      *val = ASM330LHH_FF_TSH_344mg;
      break;
    case ASM330LHH_FF_TSH_406mg:
      *val = ASM330LHH_FF_TSH_406mg;
      break;
    case ASM330LHH_FF_TSH_469mg:
      *val = ASM330LHH_FF_TSH_469mg;
      break;
    case ASM330LHH_FF_TSH_500mg:
      *val = ASM330LHH_FF_TSH_500mg;
      break;
    default:
      *val = ASM330LHH_FF_TSH_156mg;
      break;
  }
  return ret;
}

/**
  * @brief  Free-fall duration event(1LSb = 1 / ODR).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ff_dur in reg FREE_FALL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_ff_dur_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_wake_up_dur_t wake_up_dur;
  asm330lhh_free_fall_t free_fall;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                           (uint8_t*)&wake_up_dur, 1);
  if(ret == 0){
    wake_up_dur.ff_dur = (val & 0x20U) >> 5;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                              (uint8_t*)&wake_up_dur, 1);
  }
  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_FREE_FALL,
                             (uint8_t*)&free_fall, 1);
  }
  if(ret == 0){
    free_fall.ff_dur = val & 0x1FU;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FREE_FALL,
                              (uint8_t*)&free_fall, 1);
  }
  return ret;
}

/**
  * @brief  Free-fall duration event(1LSb = 1 / ODR).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of ff_dur in reg FREE_FALL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_ff_dur_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_wake_up_dur_t wake_up_dur;
  asm330lhh_free_fall_t free_fall;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_WAKE_UP_DUR,
                           (uint8_t*)&wake_up_dur, 1);

  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_FREE_FALL,
                             (uint8_t*)&free_fall, 1);
  }
  *val = (wake_up_dur.ff_dur << 5) + free_fall.ff_dur;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_fifo
  * @brief      This section group all the functions concerning
  *             the fifo usage
  * @{
  *
  */

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of wtm in reg FIFO_CTRL1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_watermark_set(asm330lhh_ctx_t *ctx, uint16_t val)
{
  asm330lhh_fifo_ctrl1_t fifo_ctrl1;
  asm330lhh_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL2,
                           (uint8_t*)&fifo_ctrl2, 1);
  if(ret == 0){
    fifo_ctrl1.wtm = (uint8_t)(0x00FFU & val);
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL1,
                              (uint8_t*)&fifo_ctrl1, 1);
  }
  if(ret == 0){
    fifo_ctrl2.wtm = (uint8_t)(( 0x0100U & val ) >> 8);
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL2,
                              (uint8_t*)&fifo_ctrl2, 1);
  }
  return ret;
}

/**
  * @brief  FIFO watermark level selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of wtm in reg FIFO_CTRL1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_watermark_get(asm330lhh_ctx_t *ctx, uint16_t *val)
{
  asm330lhh_fifo_ctrl1_t fifo_ctrl1;
  asm330lhh_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL2,
                           (uint8_t*)&fifo_ctrl2, 1);
  if(ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL1,
                             (uint8_t*)&fifo_ctrl1, 1);
  }
  *val = fifo_ctrl2.wtm;
  *val = *val << 8;
  *val += fifo_ctrl1.wtm;
  return ret;
}

/**
  * @brief  Enables ODR CHANGE virtual sensor to be batched in FIFO.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odrchg_en in reg FIFO_CTRL2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_virtual_sens_odr_chg_set(asm330lhh_ctx_t *ctx,
                                                uint8_t val)
{
  asm330lhh_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL2,
                           (uint8_t*)&fifo_ctrl2, 1);
  if(ret == 0){
    fifo_ctrl2.odrchg_en= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL2,
                              (uint8_t*)&fifo_ctrl2, 1);
  }

  return ret;
}

/**
  * @brief  Enables ODR CHANGE virtual sensor to be batched in FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odrchg_en in reg FIFO_CTRL2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_virtual_sens_odr_chg_get(asm330lhh_ctx_t *ctx,
                                                uint8_t *val)
{
  asm330lhh_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL2,
                           (uint8_t*)&fifo_ctrl2, 1);
  *val = fifo_ctrl2.odrchg_en;

  return ret;
}

/**
  * @brief  Sensing chain FIFO stop values memorization at threshold
  *         level.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of stop_on_wtm in reg FIFO_CTRL2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_stop_on_wtm_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL2,
                           (uint8_t*)&fifo_ctrl2, 1);
  if(ret == 0){
    fifo_ctrl2.stop_on_wtm= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL2,
                              (uint8_t*)&fifo_ctrl2, 1);
  }
  return ret;
}

/**
  * @brief  Sensing chain FIFO stop values memorization at threshold
  *         level.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of stop_on_wtm in reg FIFO_CTRL2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_stop_on_wtm_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_fifo_ctrl2_t fifo_ctrl2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL2,
                           (uint8_t*)&fifo_ctrl2, 1);
  *val = fifo_ctrl2.stop_on_wtm;

  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for accelerometer data.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of bdr_xl in reg FIFO_CTRL3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_xl_batch_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_bdr_xl_t val)
{
  asm330lhh_fifo_ctrl3_t fifo_ctrl3;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL3,
                           (uint8_t*)&fifo_ctrl3, 1);
  if(ret == 0){
    fifo_ctrl3.bdr_xl= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL3,
                              (uint8_t*)&fifo_ctrl3, 1);
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for accelerometer data.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bdr_xl in reg FIFO_CTRL3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_xl_batch_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_bdr_xl_t *val)
{
  asm330lhh_fifo_ctrl3_t fifo_ctrl3;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL3,
                           (uint8_t*)&fifo_ctrl3, 1);

  switch (fifo_ctrl3.bdr_xl){
    case ASM330LHH_XL_NOT_BATCHED:
      *val = ASM330LHH_XL_NOT_BATCHED;
      break;
    case ASM330LHH_XL_BATCHED_AT_12Hz5:
      *val = ASM330LHH_XL_BATCHED_AT_12Hz5;
      break;
    case ASM330LHH_XL_BATCHED_AT_26Hz:
      *val = ASM330LHH_XL_BATCHED_AT_26Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_52Hz:
      *val = ASM330LHH_XL_BATCHED_AT_52Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_104Hz:
      *val = ASM330LHH_XL_BATCHED_AT_104Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_208Hz:
      *val = ASM330LHH_XL_BATCHED_AT_208Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_417Hz:
      *val = ASM330LHH_XL_BATCHED_AT_417Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_833Hz:
      *val = ASM330LHH_XL_BATCHED_AT_833Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_1667Hz:
      *val = ASM330LHH_XL_BATCHED_AT_1667Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_3333Hz:
      *val = ASM330LHH_XL_BATCHED_AT_3333Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_6667Hz:
      *val = ASM330LHH_XL_BATCHED_AT_6667Hz;
      break;
    case ASM330LHH_XL_BATCHED_AT_6Hz5:
      *val = ASM330LHH_XL_BATCHED_AT_6Hz5;
      break;
    default:
      *val = ASM330LHH_XL_NOT_BATCHED;
      break;
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for gyroscope data.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of bdr_gy in reg FIFO_CTRL3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_gy_batch_set(asm330lhh_ctx_t *ctx,
                                    asm330lhh_bdr_gy_t val)
{
  asm330lhh_fifo_ctrl3_t fifo_ctrl3;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL3,
                           (uint8_t*)&fifo_ctrl3, 1);
  if(ret == 0){
    fifo_ctrl3.bdr_gy= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL3,
                              (uint8_t*)&fifo_ctrl3, 1);
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for gyroscope data.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bdr_gy in reg FIFO_CTRL3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_gy_batch_get(asm330lhh_ctx_t *ctx,
                                    asm330lhh_bdr_gy_t *val)
{
  asm330lhh_fifo_ctrl3_t fifo_ctrl3;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL3,
                           (uint8_t*)&fifo_ctrl3, 1);

  switch (fifo_ctrl3.bdr_gy){
    case ASM330LHH_GY_NOT_BATCHED:
      *val = ASM330LHH_GY_NOT_BATCHED;
      break;
    case ASM330LHH_GY_BATCHED_AT_12Hz5:
      *val = ASM330LHH_GY_BATCHED_AT_12Hz5;
      break;
    case ASM330LHH_GY_BATCHED_AT_26Hz:
      *val = ASM330LHH_GY_BATCHED_AT_26Hz;
      break;
    case ASM330LHH_GY_BATCHED_AT_52Hz:
      *val = ASM330LHH_GY_BATCHED_AT_52Hz;
      break;
    case ASM330LHH_GY_BATCHED_AT_104Hz:
      *val = ASM330LHH_GY_BATCHED_AT_104Hz;
      break;
    case ASM330LHH_GY_BATCHED_AT_208Hz:
      *val = ASM330LHH_GY_BATCHED_AT_208Hz;
      break;
    case ASM330LHH_GY_BATCHED_AT_417Hz:
      *val = ASM330LHH_GY_BATCHED_AT_417Hz;
      break;
    case ASM330LHH_GY_BATCHED_AT_833Hz:
      *val = ASM330LHH_GY_BATCHED_AT_833Hz;
      break;
    case ASM330LHH_GY_BATCHED_AT_1667Hz:
      *val = ASM330LHH_GY_BATCHED_AT_1667Hz;
      break;
    case ASM330LHH_GY_BATCHED_AT_3333Hz:
      *val = ASM330LHH_GY_BATCHED_AT_3333Hz;
      break;
    case ASM330LHH_GY_BATCHED_AT_6667Hz:
      *val = ASM330LHH_GY_BATCHED_AT_6667Hz;
      break;
    case ASM330LHH_GY_BATCHED_6Hz5:
      *val = ASM330LHH_GY_BATCHED_6Hz5;
      break;
    default:
      *val = ASM330LHH_GY_NOT_BATCHED;
      break;
  }
  return ret;
}

/**
  * @brief  FIFO mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fifo_mode in reg FIFO_CTRL4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_mode_set(asm330lhh_ctx_t *ctx,
                                asm330lhh_fifo_mode_t val)
{
  asm330lhh_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL4,
                           (uint8_t*)&fifo_ctrl4, 1);
  if(ret == 0){
    fifo_ctrl4.fifo_mode= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL4,
                              (uint8_t*)&fifo_ctrl4, 1);
  }
  return ret;
}

/**
  * @brief  FIFO mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fifo_mode in reg FIFO_CTRL4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_mode_get(asm330lhh_ctx_t *ctx,
                                asm330lhh_fifo_mode_t *val)
{
  asm330lhh_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL4,
                           (uint8_t*)&fifo_ctrl4, 1);

  switch (fifo_ctrl4.fifo_mode){
    case ASM330LHH_BYPASS_MODE:
      *val = ASM330LHH_BYPASS_MODE;
      break;
    case ASM330LHH_FIFO_MODE:
      *val = ASM330LHH_FIFO_MODE;
      break;
    case ASM330LHH_STREAM_TO_FIFO_MODE:
      *val = ASM330LHH_STREAM_TO_FIFO_MODE;
      break;
    case ASM330LHH_BYPASS_TO_STREAM_MODE:
      *val = ASM330LHH_BYPASS_TO_STREAM_MODE;
      break;
    case ASM330LHH_STREAM_MODE:
      *val = ASM330LHH_STREAM_MODE;
      break;
    case ASM330LHH_BYPASS_TO_FIFO_MODE:
      *val = ASM330LHH_BYPASS_TO_FIFO_MODE;
      break;
    default:
      *val = ASM330LHH_BYPASS_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for temperature data.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odr_t_batch in reg FIFO_CTRL4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_temp_batch_set(asm330lhh_ctx_t *ctx,
                                      asm330lhh_odr_t_batch_t val)
{
  asm330lhh_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL4,
                           (uint8_t*)&fifo_ctrl4, 1);
  if(ret == 0){
    fifo_ctrl4.odr_t_batch= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL4,
                              (uint8_t*)&fifo_ctrl4, 1);
  }
  return ret;
}

/**
  * @brief  Selects Batching Data Rate (writing frequency in FIFO)
  *         for temperature data.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odr_t_batch in reg FIFO_CTRL4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_temp_batch_get(asm330lhh_ctx_t *ctx,
                                      asm330lhh_odr_t_batch_t *val)
{
  asm330lhh_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL4,
                           (uint8_t*)&fifo_ctrl4, 1);

  switch (fifo_ctrl4.odr_t_batch){
    case ASM330LHH_TEMP_NOT_BATCHED:
      *val = ASM330LHH_TEMP_NOT_BATCHED;
      break;
    case ASM330LHH_TEMP_BATCHED_AT_52Hz:
      *val = ASM330LHH_TEMP_BATCHED_AT_52Hz;
      break;
    case ASM330LHH_TEMP_BATCHED_AT_12Hz5:
      *val = ASM330LHH_TEMP_BATCHED_AT_12Hz5;
      break;
    case ASM330LHH_TEMP_BATCHED_AT_1Hz6:
      *val = ASM330LHH_TEMP_BATCHED_AT_1Hz6;
      break;
    default:
      *val = ASM330LHH_TEMP_NOT_BATCHED;
      break;
  }
  return ret;
}

/**
  * @brief  Selects decimation for timestamp batching in FIFO.
  *         Writing rate will be the maximum rate between XL and
  *         GYRO BDR divided by decimation decoder.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of odr_ts_batch in reg FIFO_CTRL4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_timestamp_decimation_set(asm330lhh_ctx_t *ctx,
                                                asm330lhh_odr_ts_batch_t val)
{
  asm330lhh_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL4,
                           (uint8_t*)&fifo_ctrl4, 1);
  if(ret == 0){
    fifo_ctrl4.odr_ts_batch= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_FIFO_CTRL4,
                              (uint8_t*)&fifo_ctrl4, 1);
  }
  return ret;
}

/**
  * @brief  Selects decimation for timestamp batching in FIFO.
  *         Writing rate will be the maximum rate between XL and
  *         GYRO BDR divided by decimation decoder.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of odr_ts_batch in reg
  *                                 FIFO_CTRL4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_timestamp_decimation_get(asm330lhh_ctx_t *ctx,
                                                asm330lhh_odr_ts_batch_t *val)
{
  asm330lhh_fifo_ctrl4_t fifo_ctrl4;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_CTRL4,
                           (uint8_t*)&fifo_ctrl4, 1);

  switch (fifo_ctrl4.odr_ts_batch){
    case ASM330LHH_NO_DECIMATION:
      *val = ASM330LHH_NO_DECIMATION;
      break;
    case ASM330LHH_DEC_1:
      *val = ASM330LHH_DEC_1;
      break;
    case ASM330LHH_DEC_8:
      *val = ASM330LHH_DEC_8;
      break;
    case ASM330LHH_DEC_32:
      *val = ASM330LHH_DEC_32;
      break;
    default:
      *val = ASM330LHH_NO_DECIMATION;
      break;
  }
  return ret;
}

/**
  * @brief  Selects the trigger for the internal counter of batching events
  *         between XL and gyro.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of trig_counter_bdr in
  *                reg COUNTER_BDR_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_cnt_event_batch_set(asm330lhh_ctx_t *ctx,
                                           asm330lhh_trig_counter_bdr_t val)
{
  asm330lhh_counter_bdr_reg1_t counter_bdr_reg1;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                           (uint8_t*)&counter_bdr_reg1, 1);
  if(ret == 0){
    counter_bdr_reg1.trig_counter_bdr= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                              (uint8_t*)&counter_bdr_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Selects the trigger for the internal counter of batching events
  *          between XL and gyro.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of trig_counter_bdr
  *                in reg COUNTER_BDR_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_cnt_event_batch_get(asm330lhh_ctx_t *ctx,
                                           asm330lhh_trig_counter_bdr_t *val)
{
  asm330lhh_counter_bdr_reg1_t counter_bdr_reg1;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                           (uint8_t*)&counter_bdr_reg1, 1);

  switch (counter_bdr_reg1.trig_counter_bdr){
    case ASM330LHH_XL_BATCH_EVENT:
      *val = ASM330LHH_XL_BATCH_EVENT;
      break;
    case ASM330LHH_GYRO_BATCH_EVENT:
      *val = ASM330LHH_GYRO_BATCH_EVENT;
      break;
    default:
      *val = ASM330LHH_XL_BATCH_EVENT;
      break;
  }
  return ret;
}

/**
  * @brief  Resets the internal counter of batching events for a single sensor.
  *         This bit is automatically reset to zero if it was set to ‘1’.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of rst_counter_bdr in reg COUNTER_BDR_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_rst_batch_counter_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_counter_bdr_reg1_t counter_bdr_reg1;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                           (uint8_t*)&counter_bdr_reg1, 1);
  if(ret == 0){
    counter_bdr_reg1.rst_counter_bdr= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                              (uint8_t*)&counter_bdr_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Resets the internal counter of batching events for a single sensor.
  *         This bit is automatically reset to zero if it was set to ‘1’.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of rst_counter_bdr in reg COUNTER_BDR_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_rst_batch_counter_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_counter_bdr_reg1_t counter_bdr_reg1;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                           (uint8_t*)&counter_bdr_reg1, 1);
  *val = counter_bdr_reg1.rst_counter_bdr;

  return ret;
}

/**
  * @brief  Batch data rate counter.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of cnt_bdr_th in reg COUNTER_BDR_REG2
  *                and COUNTER_BDR_REG1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_batch_counter_threshold_set(asm330lhh_ctx_t *ctx,
                                              uint16_t val)
{
  asm330lhh_counter_bdr_reg2_t counter_bdr_reg1;
  asm330lhh_counter_bdr_reg2_t counter_bdr_reg2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                           (uint8_t*)&counter_bdr_reg1, 1);
  if (ret == 0){
    counter_bdr_reg1.cnt_bdr_th = (uint8_t)((0x0700U & val) >> 8);
    ret = asm330lhh_write_reg(ctx, ASM330LHH_COUNTER_BDR_REG1, (uint8_t*)&counter_bdr_reg1, 1);
  }
  if (ret == 0){
    counter_bdr_reg2.cnt_bdr_th = (uint8_t)(0x00FFU & val);
    ret = asm330lhh_write_reg(ctx, ASM330LHH_COUNTER_BDR_REG2,
                              (uint8_t*)&counter_bdr_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Batch data rate counter.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of cnt_bdr_th in reg COUNTER_BDR_REG2
  *                and COUNTER_BDR_REG1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_batch_counter_threshold_get(asm330lhh_ctx_t *ctx,
                                              uint16_t *val)
{
  asm330lhh_counter_bdr_reg1_t counter_bdr_reg1;
  asm330lhh_counter_bdr_reg2_t counter_bdr_reg2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG1,
                           (uint8_t*)&counter_bdr_reg1, 1);
  if (ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_COUNTER_BDR_REG2,
                             (uint8_t*)&counter_bdr_reg2, 1);
  }

  *val = counter_bdr_reg1.cnt_bdr_th;
  *val = *val << 8;
  *val += counter_bdr_reg2.cnt_bdr_th;
  return ret;
}

/**
  * @brief  Number of unread sensor data (TAG + 6 bytes) stored in FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of diff_fifo in reg FIFO_STATUS1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_data_level_get(asm330lhh_ctx_t *ctx, uint16_t *val)
{
  asm330lhh_fifo_status1_t fifo_status1;
  asm330lhh_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_STATUS1,
                           (uint8_t*)&fifo_status1, 1);
  if (ret == 0){
    ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_STATUS2,
                             (uint8_t*)&fifo_status2, 1);
    *val = fifo_status2.diff_fifo;
    *val = *val << 8;
    *val += fifo_status1.diff_fifo;
  }
  return ret;
}

/**
  * @brief  Smart FIFO status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Registers FIFO_STATUS2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_status_get(asm330lhh_ctx_t *ctx,
                                  asm330lhh_fifo_status2_t *val)
{
  int32_t ret;
  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_STATUS2, (uint8_t*)val, 1);
  return ret;
}

/**
  * @brief  Smart FIFO full status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fifo_full_ia in reg FIFO_STATUS2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_full_flag_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_STATUS2,
                           (uint8_t*)&fifo_status2, 1);
  *val = fifo_status2.fifo_full_ia;

  return ret;
}

/**
  * @brief  FIFO overrun status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of  fifo_over_run_latched in
  *                reg FIFO_STATUS2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_ovr_flag_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_STATUS2,
                           (uint8_t*)&fifo_status2, 1);
  *val = fifo_status2. fifo_ovr_ia;

  return ret;
}

/**
  * @brief  FIFO watermark status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fifo_wtm_ia in reg FIFO_STATUS2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_wtm_flag_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_fifo_status2_t fifo_status2;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_STATUS2,
                           (uint8_t*)&fifo_status2, 1);
  *val = fifo_status2.fifo_wtm_ia;

  return ret;
}

/**
  * @brief  Identifies the sensor in FIFO_DATA_OUT.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of tag_sensor in reg FIFO_DATA_OUT_TAG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_fifo_sensor_tag_get(asm330lhh_ctx_t *ctx,
                                      asm330lhh_fifo_tag_t *val)
{
  asm330lhh_fifo_data_out_tag_t fifo_data_out_tag;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_FIFO_DATA_OUT_TAG,
                           (uint8_t*)&fifo_data_out_tag, 1);

  switch (fifo_data_out_tag.tag_sensor){
    case ASM330LHH_GYRO_NC_TAG:
      *val = ASM330LHH_GYRO_NC_TAG;
      break;
    case ASM330LHH_XL_NC_TAG:
      *val = ASM330LHH_XL_NC_TAG;
      break;
    case ASM330LHH_TEMPERATURE_TAG:
      *val = ASM330LHH_TEMPERATURE_TAG;
      break;
    case ASM330LHH_TIMESTAMP_TAG:
      *val = ASM330LHH_TIMESTAMP_TAG;
      break;
    case ASM330LHH_CFG_CHANGE_TAG:
      *val = ASM330LHH_CFG_CHANGE_TAG;
      break;
    default:
      *val = ASM330LHH_CFG_CHANGE_TAG;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   ASM330LHH_DEN_functionality
  * @brief      This section groups all the functions concerning
  *             DEN functionality.
  * @{
  *
  */

/**
  * @brief  DEN functionality marking mode.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_mode in reg CTRL6_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_mode_set(asm330lhh_ctx_t *ctx, asm330lhh_den_mode_t val)
{
  asm330lhh_ctrl6_g_t ctrl6_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);
  if(ret == 0){
    ctrl6_c.den_mode= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);
  }
  return ret;
}

/**
  * @brief  DEN functionality marking mode.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of den_mode in reg CTRL6_C
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_mode_get(asm330lhh_ctx_t *ctx,
                               asm330lhh_den_mode_t *val)
{
  asm330lhh_ctrl6_g_t ctrl6_c;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL6_G, (uint8_t*)&ctrl6_c, 1);

  switch (ctrl6_c.den_mode){
    case ASM330LHH_DEN_DISABLE:
      *val = ASM330LHH_DEN_DISABLE;
      break;
    case ASM330LHH_LEVEL_FIFO:
      *val = ASM330LHH_LEVEL_FIFO;
      break;
    case ASM330LHH_LEVEL_LETCHED:
      *val = ASM330LHH_LEVEL_LETCHED;
      break;
    case ASM330LHH_LEVEL_TRIGGER:
      *val = ASM330LHH_LEVEL_TRIGGER;
      break;
    case ASM330LHH_EDGE_TRIGGER:
      *val = ASM330LHH_EDGE_TRIGGER;
      break;
    default:
      *val = ASM330LHH_DEN_DISABLE;
      break;
  }
  return ret;
}

/**
  * @brief  DEN active level configuration.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_lh in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_polarity_set(asm330lhh_ctx_t *ctx,
                                   asm330lhh_den_lh_t val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if(ret == 0){
    ctrl9_xl.den_lh= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL9_XL,
                              (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief  DEN active level configuration.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of den_lh in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_polarity_get(asm330lhh_ctx_t *ctx,
                                   asm330lhh_den_lh_t *val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);

  switch (ctrl9_xl.den_lh){
    case ASM330LHH_DEN_ACT_LOW:
      *val = ASM330LHH_DEN_ACT_LOW;
      break;
    case ASM330LHH_DEN_ACT_HIGH:
      *val = ASM330LHH_DEN_ACT_HIGH;
      break;
    default:
      *val = ASM330LHH_DEN_ACT_LOW;
      break;
  }
  return ret;
}

/**
  * @brief  DEN configuration.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_xl_g in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_enable_set(asm330lhh_ctx_t *ctx,
                                 asm330lhh_den_xl_g_t val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if(ret == 0){
    ctrl9_xl.den_xl_g= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL9_XL,
                              (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief  DEN configuration.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of den_xl_g in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_enable_get(asm330lhh_ctx_t *ctx,
                                 asm330lhh_den_xl_g_t *val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);

  switch (ctrl9_xl.den_xl_g){
    case ASM330LHH_STAMP_IN_GY_DATA:
      *val = ASM330LHH_STAMP_IN_GY_DATA;
      break;
    case ASM330LHH_STAMP_IN_XL_DATA:
      *val = ASM330LHH_STAMP_IN_XL_DATA;
      break;
    case ASM330LHH_STAMP_IN_GY_XL_DATA:
      *val = ASM330LHH_STAMP_IN_GY_XL_DATA;
      break;
    default:
      *val = ASM330LHH_STAMP_IN_GY_DATA;
      break;
  }
  return ret;
}

/**
  * @brief  DEN value stored in LSB of X-axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_z in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_mark_axis_x_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if(ret == 0){
    ctrl9_xl.den_z= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL9_XL,
                              (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief  DEN value stored in LSB of X-axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_z in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_mark_axis_x_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  *val = ctrl9_xl.den_z;

  return ret;
}

/**
  * @brief  DEN value stored in LSB of Y-axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_y in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_mark_axis_y_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if(ret == 0){
    ctrl9_xl.den_y= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL9_XL,
                              (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief  DEN value stored in LSB of Y-axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_y in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_mark_axis_y_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  *val = ctrl9_xl.den_y;

  return ret;
}

/**
  * @brief  DEN value stored in LSB of Z-axis.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_x in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_mark_axis_z_set(asm330lhh_ctx_t *ctx, uint8_t val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  if(ret == 0){
    ctrl9_xl.den_x= (uint8_t)val;
    ret = asm330lhh_write_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  }
  return ret;
}

/**
  * @brief  DEN value stored in LSB of Z-axis.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of den_x in reg CTRL9_XL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t asm330lhh_den_mark_axis_z_get(asm330lhh_ctx_t *ctx, uint8_t *val)
{
  asm330lhh_ctrl9_xl_t ctrl9_xl;
  int32_t ret;

  ret = asm330lhh_read_reg(ctx, ASM330LHH_CTRL9_XL, (uint8_t*)&ctrl9_xl, 1);
  *val = ctrl9_xl.den_x;

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
