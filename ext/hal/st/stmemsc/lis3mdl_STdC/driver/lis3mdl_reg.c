/*
 ******************************************************************************
 * @file    lis3mdl_reg.c
 * @author  Sensors Software Solution Team
 * @brief   LIS3MDL driver file
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

#include "lis3mdl_reg.h"

/**
  * @defgroup    LIS3MDL
  * @brief       This file provides a set of functions needed to drive the
  *              lis3mdl enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup    LIS3MDL_Interfaces_Functions
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
int32_t lis3mdl_read_reg(lis3mdl_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lis3mdl_write_reg(lis3mdl_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LIS3MDL_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float lis3mdl_from_fs4_to_gauss(int16_t lsb)
{
  return ((float)lsb / 6842.0f);
}

float lis3mdl_from_fs8_to_gauss(int16_t lsb)
{
  return ((float)lsb / 3421.0f);
}

float lis3mdl_from_fs12_to_gauss(int16_t lsb)
{
  return ((float)lsb / 2281.0f);
}

float lis3mdl_from_fs16_to_gauss(int16_t lsb)
{
  return ((float)lsb / 1711.0f);
}

float lis3mdl_from_lsb_to_celsius(int16_t lsb)
{
  return ((float)lsb / 8.0f ) + ( 25.0f );
}

/**
  * @}
  *
  */

/**
  * @defgroup    LIS3MDL_Data_Generation
  * @brief       This section group all the functions concerning
  *              data generation
  * @{
  *
  */

/**
  * @brief  Output data rate selection.[set]
  *
  * @param  ctx         read / write interface definitions(ptr)
  * @param  val         change the values of om in reg CTRL_REG1
  * @retval             interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_data_rate_set(lis3mdl_ctx_t *ctx, lis3mdl_om_t val)
{
  lis3mdl_ctrl_reg1_t ctrl_reg1;
  lis3mdl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if (ret == 0)
  {
    ctrl_reg1.om = (uint8_t)val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }
  
  if (ret == 0)
  {
    /* set mode also for z axis, ctrl_reg4 -> omz */
    ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  }
  
  if (ret == 0)
  {
    ctrl_reg4.omz = (uint8_t)(((uint8_t) val >> 4) & 0x03U);
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG4,
                            (uint8_t*)&ctrl_reg4, 1);
  }

  return ret;
}

/**
  * @brief  Output data rate selection[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      Get the values of om in reg CTRL_REG1(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_data_rate_get(lis3mdl_ctx_t *ctx, lis3mdl_om_t *val)
{
  lis3mdl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  /* z axis, ctrl_reg4 -> omz is aligned with x/y axis ctrl_reg1 -> om*/
  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  switch (ctrl_reg1.om)
  {
    case LIS3MDL_LP_Hz625:
      *val = LIS3MDL_LP_Hz625;
      break;
    case LIS3MDL_LP_1kHz:
      *val = LIS3MDL_LP_1kHz;
      break;
    case LIS3MDL_MP_560Hz:
      *val = LIS3MDL_MP_560Hz;
      break;
    case LIS3MDL_HP_300Hz:
      *val = LIS3MDL_HP_300Hz;
      break;
    case LIS3MDL_UHP_155Hz:
      *val = LIS3MDL_UHP_155Hz;
      break;
    case LIS3MDL_LP_1Hz25:
      *val = LIS3MDL_LP_1Hz25;
      break;
    case LIS3MDL_LP_2Hz5:
      *val = LIS3MDL_LP_2Hz5;
      break;
    case LIS3MDL_LP_5Hz:
      *val = LIS3MDL_LP_5Hz;
      break;
    case LIS3MDL_LP_10Hz:
      *val = LIS3MDL_LP_10Hz;
      break;
    case LIS3MDL_LP_20Hz:
      *val = LIS3MDL_LP_20Hz;
      break;
    case LIS3MDL_LP_40Hz:
      *val = LIS3MDL_LP_40Hz;
      break;
    case LIS3MDL_LP_80Hz:
      *val = LIS3MDL_LP_80Hz;
      break;
    case LIS3MDL_MP_1Hz25:
      *val = LIS3MDL_MP_1Hz25;
      break;
    case LIS3MDL_MP_2Hz5:
      *val = LIS3MDL_MP_2Hz5;
      break;
    case LIS3MDL_MP_5Hz:
      *val = LIS3MDL_MP_5Hz;
      break;
    case LIS3MDL_MP_10Hz:
      *val = LIS3MDL_MP_10Hz;
      break;
    case LIS3MDL_MP_20Hz:
      *val = LIS3MDL_MP_20Hz;
      break;
    case LIS3MDL_MP_40Hz:
      *val = LIS3MDL_MP_40Hz;
      break;
    case LIS3MDL_MP_80Hz:
      *val = LIS3MDL_MP_80Hz;
      break;
    case LIS3MDL_HP_1Hz25:
      *val = LIS3MDL_HP_1Hz25;
      break;
    case LIS3MDL_HP_2Hz5:
      *val = LIS3MDL_HP_2Hz5;
      break;
    case LIS3MDL_HP_5Hz:
      *val = LIS3MDL_HP_5Hz;
      break;
    case LIS3MDL_HP_10Hz:
      *val = LIS3MDL_HP_10Hz;
      break;
    case LIS3MDL_HP_20Hz:
      *val = LIS3MDL_HP_20Hz;
      break;
    case LIS3MDL_HP_40Hz:
      *val = LIS3MDL_HP_40Hz;
      break;
    case LIS3MDL_HP_80Hz:
      *val = LIS3MDL_HP_80Hz;
      break;
    case LIS3MDL_UHP_1Hz25:
      *val = LIS3MDL_UHP_1Hz25;
      break;
    case LIS3MDL_UHP_2Hz5:
      *val = LIS3MDL_UHP_2Hz5;
      break;
    case LIS3MDL_UHP_5Hz:
      *val = LIS3MDL_UHP_5Hz;
      break;
    case LIS3MDL_UHP_10Hz:
      *val = LIS3MDL_UHP_10Hz;
      break;
    case LIS3MDL_UHP_20Hz:
      *val = LIS3MDL_UHP_20Hz;
      break;
    case LIS3MDL_UHP_40Hz:
      *val = LIS3MDL_UHP_40Hz;
      break;
    case LIS3MDL_UHP_80Hz:
      *val = LIS3MDL_UHP_80Hz;
      break;
    default:
      *val = LIS3MDL_UHP_80Hz;
      break;
  }

  return ret;
}

/**
  * @brief  Temperature sensor enable[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of temp_en in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_temperature_meas_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0)
  {
    ctrl_reg1.temp_en = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Temperature sensor enable[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of temp_en in reg CTRL_REG1(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_temperature_meas_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  *val = (uint8_t)ctrl_reg1.temp_en;

  return ret;
}

/**
  * @brief  Full-scale configuration[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of fs in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_full_scale_set(lis3mdl_ctx_t *ctx, lis3mdl_fs_t val)
{
  lis3mdl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0)
  {
    ctrl_reg2.fs = (uint8_t)val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Full-scale configuration[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of fs in reg CTRL_REG2(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_full_scale_get(lis3mdl_ctx_t *ctx, lis3mdl_fs_t *val)
{
  lis3mdl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  switch (ctrl_reg2.fs)
  {
    case LIS3MDL_4_GAUSS:
      *val = LIS3MDL_4_GAUSS;
      break;
    case LIS3MDL_8_GAUSS:
      *val = LIS3MDL_8_GAUSS;
      break;
    case LIS3MDL_12_GAUSS:
      *val = LIS3MDL_12_GAUSS;
      break;
    case LIS3MDL_16_GAUSS:
      *val = LIS3MDL_16_GAUSS;
      break;
    default:
      *val = LIS3MDL_4_GAUSS;
      break;
  }

  return ret;
}

/**
  * @brief  Operating mode selection[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of md in reg CTRL_REG3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_operating_mode_set(lis3mdl_ctx_t *ctx, lis3mdl_md_t val)
{
  lis3mdl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0)
  {
    ctrl_reg3.md = (uint8_t)val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }

  return ret;
}

/**
  * @brief  Operating mode selection[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      Get the values of md in reg CTRL_REG3(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_operating_mode_get(lis3mdl_ctx_t *ctx, lis3mdl_md_t *val)
{
  lis3mdl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  switch (ctrl_reg3.md)
  {
    case LIS3MDL_CONTINUOUS_MODE:
      *val = LIS3MDL_CONTINUOUS_MODE;
      break;
    case LIS3MDL_SINGLE_TRIGGER:
      *val = LIS3MDL_SINGLE_TRIGGER;
      break;
    case LIS3MDL_POWER_DOWN:
      *val = LIS3MDL_POWER_DOWN;
      break;
    default:
      *val = LIS3MDL_POWER_DOWN;
      break;
  }
  return ret;
}

/**
  * @brief  If this bit is high, device is set in low power to 0.625 Hz[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of lp in reg CTRL_REG3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_fast_low_power_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0)
  {
    ctrl_reg3.lp = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  If this bit is high, device is set in low power to 0.625 Hz[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of lp in reg CTRL_REG3(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_fast_low_power_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  *val = (uint8_t)ctrl_reg3.lp;

  return ret;
}

/**
  * @brief  Block data update[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of bdu in reg CTRL_REG5
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_block_data_update_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_ctrl_reg5_t ctrl_reg5;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  if(ret == 0)
  {
    ctrl_reg5.bdu = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  }
  return ret;
}

/**
  * @brief  Block data update[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of bdu in reg CTRL_REG5(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_block_data_update_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_ctrl_reg5_t ctrl_reg5;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  *val = (uint8_t)ctrl_reg5.bdu;

  return ret;
}

/**
  * @brief  fast_read allows reading the high part of DATA OUT only in order
  *         to increase reading efficiency[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of fast_read in reg CTRL_REG5
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_high_part_cycle_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_ctrl_reg5_t ctrl_reg5;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  if(ret == 0)
  {
    ctrl_reg5.fast_read = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  }

  return ret;
}

/**
  * @brief  fast_read allows reading the high part of DATA OUT only in order
  *         to increase reading efficiency[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of fast_read in reg CTRL_REG5(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_high_part_cycle_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_ctrl_reg5_t ctrl_reg5;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  *val = (uint8_t)ctrl_reg5.fast_read;

  return ret;
}

/**
  * @brief  Magnetic set of data available[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of zyxda in reg STATUS_REG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_mag_data_ready_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_status_reg_t status_reg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_STATUS_REG, (uint8_t*)&status_reg, 1);
  *val = (uint8_t)status_reg.zyxda;

  return ret;
}

/**
  * @brief  Magnetic set of data overrun[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of zyxor in reg STATUS_REG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_mag_data_ovr_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_status_reg_t status_reg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_STATUS_REG, (uint8_t*)&status_reg, 1);
  *val = (uint8_t)status_reg.zyxor;

  return ret;
}
/**
  * @brief  Magnetic output value[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      buffer that stores data read(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_magnetic_raw_get(lis3mdl_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis3mdl_read_reg(ctx, LIS3MDL_OUT_X_L, (uint8_t*) buff, 6);
  return ret;
}
/**
  * @brief  Temperature output value[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      buffer that stores data read(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_temperature_raw_get(lis3mdl_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;  
  ret = lis3mdl_read_reg(ctx, LIS3MDL_TEMP_OUT_L, (uint8_t*) buff, 2);
  return ret;
}
/**
  * @}
  *
  */

/**
  * @defgroup    LIS3MDL_Common
  * @brief       This section group common usefull functions
  * @{
  *
  */

/**
  * @brief  Device Who am I[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      buffer that stores data read(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_device_id_get(lis3mdl_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;  
  ret = lis3mdl_read_reg(ctx, LIS3MDL_WHO_AM_I, (uint8_t*) buff, 1);
  return ret;
}
/**
  * @brief  Self test[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of st in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_self_test_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0)
  {
    ctrl_reg1.st = (uint8_t)val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Self_test[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of st in reg CTRL_REG1(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_self_test_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  *val = (uint8_t)ctrl_reg1.st;

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of soft_rst in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_reset_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0)
  {
    ctrl_reg2.soft_rst = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of soft_rst in reg CTRL_REG2(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_reset_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  *val = (uint8_t)ctrl_reg2.soft_rst;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of reboot in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_boot_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0)
  {
    ctrl_reg2.reboot = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of reboot in reg CTRL_REG2(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_boot_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  *val = (uint8_t)ctrl_reg2.reboot;

  return ret;
}

/**
  * @brief  Big/Little Endian data selection[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of ble in reg CTRL_REG4
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_data_format_set(lis3mdl_ctx_t *ctx, lis3mdl_ble_t val)
{
  lis3mdl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0)
  {
    ctrl_reg4.ble = (uint8_t)val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  }
  return ret;
}

/**
  * @brief  Big/Little Endian data selection[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      Get the values of ble in reg CTRL_REG4(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_data_format_get(lis3mdl_ctx_t *ctx, lis3mdl_ble_t *val)
{
  lis3mdl_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  switch (ctrl_reg4.ble)
  {
    case LIS3MDL_LSB_AT_LOW_ADD:
      *val = LIS3MDL_LSB_AT_LOW_ADD;
      break;
    case LIS3MDL_MSB_AT_LOW_ADD:
      *val = LIS3MDL_MSB_AT_LOW_ADD;
      break;
    default:
      *val = LIS3MDL_LSB_AT_LOW_ADD;
      break;
  }
  return ret;
}

/**
  * @brief  Status register[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      Registers STATUS_REG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_status_get(lis3mdl_ctx_t *ctx, lis3mdl_status_reg_t *val)
{
  return lis3mdl_read_reg(ctx, LIS3MDL_STATUS_REG, (uint8_t*) val, 1);
}
/**
  * @}
  *
  */

/**
  * @defgroup    LIS3MDL_interrupts
  * @brief       This section group all the functions that manage interrupts
  * @{
  *
  */

/**
  * @brief  Interrupt configuration register[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      Registers INT_CFG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_config_set(lis3mdl_ctx_t *ctx, lis3mdl_int_cfg_t *val)
{
  return lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*) val, 1);
}

/**
  * @brief  Interrupt configuration register[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      Registers INT_CFG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_config_get(lis3mdl_ctx_t *ctx, lis3mdl_int_cfg_t *val)
{
  return lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*) val, 1);
}
/**
  * @brief  Interrupt enable on INT pin[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of ien in reg INT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_generation_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  if(ret == 0)
  {
    int_cfg.ien = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  }

  return ret;
}

/**
  * @brief  Interrupt enable on INT pin[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of ien in reg INT_CFG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_generation_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  *val = (uint8_t)int_cfg.ien;

  return ret;
}

/**
  * @brief   Interrupt request to the INT_SOURCE (25h) register
  *      mode (pulsed / latched)[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of lir in reg INT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_notification_mode_set(lis3mdl_ctx_t *ctx,
    lis3mdl_lir_t val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  if(ret == 0)
  {
    int_cfg.lir = (uint8_t)val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Interrupt request to the INT_SOURCE (25h) register
  *          mode (pulsed / latched)[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of lir in reg INT_CFG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_notification_mode_get(lis3mdl_ctx_t *ctx,
    lis3mdl_lir_t *val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  switch (int_cfg.lir)
  {
    case LIS3MDL_INT_PULSED:
      *val = LIS3MDL_INT_PULSED;
      break;
    case LIS3MDL_INT_LATCHED:
      *val = LIS3MDL_INT_LATCHED;
      break;
    default:
      *val = LIS3MDL_INT_PULSED;
      break;
  }

  return ret;
}

/**
  * @brief  Interrupt active-high/low[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of iea in reg INT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_polarity_set(lis3mdl_ctx_t *ctx, lis3mdl_iea_t val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  if(ret == 0)
  {
    int_cfg.iea = (uint8_t)val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  }

  return ret;
}

/**
  * @brief  Interrupt active-high/low[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of iea in reg INT_CFG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_polarity_get(lis3mdl_ctx_t *ctx, lis3mdl_iea_t *val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  switch (int_cfg.iea)
  {
    case LIS3MDL_ACTIVE_HIGH:
      *val = LIS3MDL_ACTIVE_HIGH;
      break;
    case LIS3MDL_ACTIVE_LOW:
      *val = LIS3MDL_ACTIVE_LOW;
      break;
    default:
      *val = LIS3MDL_ACTIVE_HIGH;
      break;
  }
  return ret;
}

/**
  * @brief  Enable interrupt generation on Z-axis[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of zien in reg INT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_on_z_ax_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  if(ret == 0)
  {
    int_cfg.zien = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Enable interrupt generation on Z-axis[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of zien in reg INT_CFG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_on_z_ax_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  *val = (uint8_t)int_cfg.zien;

  return ret;
}

/**
  * @brief  Enable interrupt generation on Y-axis[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of yien in reg INT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_on_y_ax_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  if(ret == 0)
  {
    int_cfg.yien = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Enable interrupt generation on Y-axis[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of yien in reg INT_CFG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_on_y_ax_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  *val = (uint8_t)int_cfg.yien;

  return ret;
}

/**
  * @brief  Enable interrupt generation on X-axis[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of xien in reg INT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_on_x_ax_set(lis3mdl_ctx_t *ctx, uint8_t val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  if(ret == 0)
  {
    int_cfg.xien = val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  }

  return ret;
}

/**
  * @brief  Enable interrupt generation on X-axis[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of xien in reg INT_CFG(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_on_x_ax_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_cfg_t int_cfg;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_CFG, (uint8_t*)&int_cfg, 1);
  *val = (uint8_t)int_cfg.xien;

  return ret;
}

/**
  * @brief Interrupt source register[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      register INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_source_get(lis3mdl_ctx_t *ctx, lis3mdl_int_src_t *val)
{
  return lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*) val, 1);
}

/**
  * @brief  Interrupt active flag[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of int in reg INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_interrupt_event_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_src_t int_src;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*)&int_src, 1);
  *val = (uint8_t)int_src.int_;

  return ret;
}

/**
  * @brief  Internal measurement range overflow on magnetic value[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of mroi in reg INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_mag_over_range_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_src_t int_src;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*)&int_src, 1);
  *val = (uint8_t)int_src.mroi;

  return ret;
}

/**
  * @brief  Value on Z-axis exceeds the threshold on the negative side.[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of nth_z in reg INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_neg_z_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_src_t int_src;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*)&int_src, 1);
  *val = (uint8_t)int_src.nth_z;

  return ret;
}

/**
  * @brief  Value on Y-axis exceeds the threshold on the negative side[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of nth_y in reg INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_neg_y_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_src_t int_src;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*)&int_src, 1);
  *val = (uint8_t)int_src.nth_y;

  return ret;
}
/**
  * @brief  Value on X-axis exceeds the threshold on the negative side[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of nth_x in reg INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_neg_x_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_src_t int_src;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*)&int_src, 1);
  *val = (uint8_t)int_src.nth_x;

  return ret;
}
/**
  * @brief  Value on Z-axis exceeds the threshold on the positive side[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pth_z in reg INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_pos_z_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_src_t int_src;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*)&int_src, 1);
  *val = (uint8_t)int_src.pth_z;

  return ret;
}
/**
  * @brief  Value on Y-axis exceeds the threshold on the positive side[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pth_y in reg INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_pos_y_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_src_t int_src;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*)&int_src, 1);
  *val = (uint8_t)int_src.pth_y;

  return ret;
}
/**
  * @brief  Value on X-axis exceeds the threshold on the positive side[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of pth_x in reg INT_SRC(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_pos_x_flag_get(lis3mdl_ctx_t *ctx, uint8_t *val)
{
  lis3mdl_int_src_t int_src;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_INT_SRC, (uint8_t*)&int_src, 1);
  *val = (uint8_t)int_src.pth_x;

  return ret;
}
/**
  * @brief  User-defined threshold value for pressure interrupt event[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param buff      buffer that contains data to write(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_threshold_set(lis3mdl_ctx_t *ctx, uint8_t *buff)
{
  return lis3mdl_read_reg(ctx, LIS3MDL_INT_THS_L, buff, 2);
}

/**
  * @brief  User-defined threshold value for pressure interrupt event[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param buff      buffer that stores data read(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_int_threshold_get(lis3mdl_ctx_t *ctx, uint8_t *buff)
{
  return lis3mdl_read_reg(ctx, LIS3MDL_INT_THS_L, buff, 2);
}
/**
  * @}
  *
  */

/**
  * @defgroup    LIS3MDL_Serial_Interface
  * @brief       This section group all the functions concerning
  *              serial interface management
  * @{
  *
  */

/**
  * @brief  SPI Serial Interface Mode selection[set]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      change the values of sim in reg CTRL_REG3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_spi_mode_set(lis3mdl_ctx_t *ctx, lis3mdl_sim_t val)
{
  lis3mdl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0)
  {
    ctrl_reg3.sim = (uint8_t)val;
    ret = lis3mdl_write_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection[get]
  *
  * @param  ctx      read / write interface definitions(ptr)
  * @param  val      get the values of sim in reg CTRL_REG3(ptr)
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lis3mdl_spi_mode_get(lis3mdl_ctx_t *ctx, lis3mdl_sim_t *val)
{
  lis3mdl_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lis3mdl_read_reg(ctx, LIS3MDL_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  switch (ctrl_reg3.sim)
  {
    case LIS3MDL_SPI_4_WIRE:
      *val = LIS3MDL_SPI_4_WIRE;
      break;
    case LIS3MDL_SPI_3_WIRE:
      *val = LIS3MDL_SPI_3_WIRE;
      break;
    default:
      *val = LIS3MDL_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/