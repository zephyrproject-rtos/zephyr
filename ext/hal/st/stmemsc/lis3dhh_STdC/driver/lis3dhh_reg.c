/*
 ******************************************************************************
 * @file    lis3dhh_reg.c
 * @author  Sensor Solutions Software Team
 * @brief   LIS3DHH driver file
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

#include "lis3dhh_reg.h"

/**
  * @defgroup  LIS3DHH
  * @brief     This file provides a set of functions needed to drive the
  *            lis3dhh enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup  LIS3DHH_Interfaces_Functions
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
int32_t lis3dhh_read_reg(lis3dhh_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lis3dhh_write_reg(lis3dhh_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LIS3DHH_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t lis3dhh_from_lsb_to_mg(int16_t lsb)
{
  return ((float_t)lsb *0.076f);
}

float_t lis3dhh_from_lsb_to_celsius(int16_t lsb)
{
  return (((float_t)lsb / 16.0f) + 25.0f);
}

/**
  * @}
  *
  */

/**
  * @defgroup   LIS3DHH_Data_generation
  * @brief      This section groups all the functions concerning data
  *             generation
  * @{
  *
  */

/**
  * @brief  Blockdataupdate.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of bdu in reg CTRL_REG1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_block_data_update_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.bdu = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }

  return ret;
}

/**
  * @brief  Blockdataupdate.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of bdu in reg CTRL_REG1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_block_data_update_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  *val = ctrl_reg1.bdu;

  return ret;
}

/**
  * @brief  Output data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of norm_mod_en in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_data_rate_set(lis3dhh_ctx_t *ctx, lis3dhh_norm_mod_en_t val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.norm_mod_en = (uint8_t)val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }

  return ret;
}

/**
  * @brief  Output data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of norm_mod_en in reg CTRL_REG1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_data_rate_get(lis3dhh_ctx_t *ctx, lis3dhh_norm_mod_en_t *val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);

  switch (ctrl_reg1.norm_mod_en){
    case LIS3DHH_POWER_DOWN:
      *val = LIS3DHH_POWER_DOWN;
      break;
    case LIS3DHH_1kHz1:
      *val = LIS3DHH_1kHz1;
      break;
    default:
      *val = LIS3DHH_POWER_DOWN;
      break;
  }

  return ret;
}

/**
  * @brief  Offset temperature compensation enable.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of off_tcomp_en in reg CTRL_REG4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_offset_temp_comp_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0){
    ctrl_reg4.off_tcomp_en = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  }

  return ret;
}

/**
  * @brief  Offset temperature compensation enable.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of off_tcomp_en in reg CTRL_REG4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_offset_temp_comp_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  *val = ctrl_reg4.off_tcomp_en;

  return ret;
}

/**
  * @brief  Temperature output value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_temperature_raw_get(lis3dhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis3dhh_read_reg(ctx, LIS3DHH_OUT_TEMP_L, buff, 2);
  return ret;
}

/**
  * @brief  acceleration output value.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_acceleration_raw_get(lis3dhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis3dhh_read_reg(ctx, LIS3DHH_OUT_X_L_XL, buff, 6);
  return ret;
}

/**
  * @brief  Acceleration set of data available.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of zyxda in reg STATUS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_xl_data_ready_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_status_t status;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_STATUS, (uint8_t*)&status, 1);
  *val = status.zyxda;

  return ret;
}

/**
  * @brief  Acceleration set of data overrun.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of zyxor in reg STATUS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_xl_data_ovr_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_status_t status;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_STATUS, (uint8_t*)&status, 1);
  *val = status.zyxor;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS3DHH_common
  * @brief     This section group common useful functions
  * @{
  *
  */

/**
  * @brief  DeviceWhoamI.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_device_id_get(lis3dhh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lis3dhh_read_reg(ctx, LIS3DHH_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief  Asic identification.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of asic_id in reg ID_REG.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_asic_id_get(lis3dhh_ctx_t *ctx, lis3dhh_asic_id_t *val)
{
  lis3dhh_id_reg_t id_reg;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_ID_REG, (uint8_t*)&id_reg, 1);

  switch (id_reg.asic_id){
    case LIS3DHH_VER_A:
      *val = LIS3DHH_VER_A;
      break;
    case LIS3DHH_VER_B:
      *val = LIS3DHH_VER_B;
      break;
    default:
      *val = LIS3DHH_VER_A;
      break;
  }

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of sw_reset in reg CTRL_REG1.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_reset_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.sw_reset = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }

  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of sw_reset in reg CTRL_REG1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_reset_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  *val = ctrl_reg1.sw_reset;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of boot in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_boot_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.boot = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of boot in reg CTRL_REG1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_boot_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  *val = ctrl_reg1.boot;

  return ret;
}

/**
  * @brief  Selftest.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of st in reg CTRL_REG4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_self_test_set(lis3dhh_ctx_t *ctx, lis3dhh_st_t val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0){
    ctrl_reg4.st = (uint8_t)val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  }

  return ret;
}

/**
  * @brief  Selftest.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of st in reg CTRL_REG4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_self_test_get(lis3dhh_ctx_t *ctx, lis3dhh_st_t *val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);

  switch (ctrl_reg4.st){
    case LIS3DHH_ST_DISABLE:
      *val = LIS3DHH_ST_DISABLE;
      break;
    case LIS3DHH_ST_POSITIVE:
      *val = LIS3DHH_ST_POSITIVE;
      break;
    case LIS3DHH_ST_NEGATIVE:
      *val = LIS3DHH_ST_NEGATIVE;
      break;
    default:
      *val = LIS3DHH_ST_DISABLE;
      break;
  }

  return ret;
}

/**
  * @brief  Digital filtering Phase/bandwidth selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of dsp in reg CTRL_REG4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_filter_config_set(lis3dhh_ctx_t *ctx, lis3dhh_dsp_t val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0){
    ctrl_reg4.dsp = (uint8_t)val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  }

  return ret;
}

/**
  * @brief  Digital filtering Phase/bandwidth selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of dsp in reg CTRL_REG4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_filter_config_get(lis3dhh_ctx_t *ctx, lis3dhh_dsp_t *val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);

  switch (ctrl_reg4.dsp){
    case LIS3DHH_LINEAR_PHASE_440Hz:
      *val = LIS3DHH_LINEAR_PHASE_440Hz;
      break;
    case LIS3DHH_LINEAR_PHASE_235Hz:
      *val = LIS3DHH_LINEAR_PHASE_235Hz;
      break;
    case LIS3DHH_NO_LINEAR_PHASE_440Hz:
      *val = LIS3DHH_NO_LINEAR_PHASE_440Hz;
      break;
    case LIS3DHH_NO_LINEAR_PHASE_235Hz:
      *val = LIS3DHH_NO_LINEAR_PHASE_235Hz;
      break;
    default:
      *val = LIS3DHH_LINEAR_PHASE_440Hz;
      break;
  }

  return ret;
}

/**
  * @brief  Statusregister.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers STATUS.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_status_get(lis3dhh_ctx_t *ctx, lis3dhh_status_t *val)
{
  int32_t ret;
  ret = lis3dhh_read_reg(ctx, LIS3DHH_STATUS, (uint8_t*) val, 1);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS3DHH_interrupts
  * @brief     This section group all the functions that manage interrupts
  * @{
  *
  */

/**
  * @brief  DRDY latched / pulsed, pulse duration is 1/4 ODR.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of drdy_pulse in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_drdy_notification_mode_set(lis3dhh_ctx_t *ctx,
                                            lis3dhh_drdy_pulse_t val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.drdy_pulse = (uint8_t)val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }

  return ret;
}

/**
  * @brief   DRDY latched / pulsed, pulse duration is 1/4 ODR.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of drdy_pulse in reg CTRL_REG1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_drdy_notification_mode_get(lis3dhh_ctx_t *ctx,
                                            lis3dhh_drdy_pulse_t *val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);

  switch (ctrl_reg1.drdy_pulse){
    case LIS3DHH_LATCHED:
      *val = LIS3DHH_LATCHED;
      break;
    case LIS3DHH_PULSED:
      *val = LIS3DHH_PULSED;
      break;
    default:
      *val = LIS3DHH_LATCHED;
      break;
  }

  return ret;
}

/**
  * @brief  It configures the INT1 pad as output for FIFO flags or as
  *                external asynchronous input trigger to FIFO.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int1_ext in reg INT1_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_int1_mode_set(lis3dhh_ctx_t *ctx, lis3dhh_int1_ext_t val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  if(ret == 0){
    int1_ctrl.int1_ext = (uint8_t)val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  It configures the INT1 pad as output for FIFO flags or as
  *                external asynchronous input trigger to FIFO.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int1_ext in reg INT1_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_int1_mode_get(lis3dhh_ctx_t *ctx, lis3dhh_int1_ext_t *val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);

  switch (int1_ctrl.int1_ext){
    case LIS3DHH_PIN_AS_INTERRUPT:
      *val = LIS3DHH_PIN_AS_INTERRUPT;
      break;
    case LIS3DHH_PIN_AS_TRIGGER:
      *val = LIS3DHH_PIN_AS_TRIGGER;
      break;
    default:
      *val = LIS3DHH_PIN_AS_INTERRUPT;
      break;
  }

  return ret;
}

/**
  * @brief  FIFO watermark status on INT1 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int1_fth in reg INT1_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_threshold_on_int1_set(lis3dhh_ctx_t *ctx,
                                           uint8_t val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  if(ret == 0){
    int1_ctrl.int1_fth = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  FIFO watermark status on INT1 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int1_fth in reg INT1_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_threshold_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  *val = int1_ctrl.int1_fth;

  return ret;
}

/**
  * @brief  FIFO full flag on INT1 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int1_fss5 in reg INT1_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_full_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  if(ret == 0){
    int1_ctrl.int1_fss5 = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  FIFO full flag on INT1 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int1_fss5 in reg INT1_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_full_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  *val = int1_ctrl.int1_fss5;

  return ret;
}

/**
  * @brief  FIFO overrun interrupt on INT1 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int1_ovr in reg INT1_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_ovr_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  if(ret == 0){
    int1_ctrl.int1_ovr = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  FIFO overrun interrupt on INT1 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int1_ovr in reg INT1_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_ovr_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  *val = int1_ctrl.int1_ovr;

  return ret;
}

/**
  * @brief  BOOT status on INT1 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int1_boot in reg INT1_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_boot_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  if(ret == 0){
    int1_ctrl.int1_boot = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  BOOT status on INT1 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int1_boot in reg INT1_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_boot_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  *val = int1_ctrl.int1_boot;

  return ret;
}

/**
  * @brief  Data-ready signal on INT1 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int1_drdy in reg INT1_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_drdy_on_int1_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  if(ret == 0){
    int1_ctrl.int1_drdy = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  Data-ready signal on INT1 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int1_drdy in reg INT1_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_drdy_on_int1_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int1_ctrl_t int1_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT1_CTRL, (uint8_t*)&int1_ctrl, 1);
  *val = int1_ctrl.int1_drdy;

  return ret;
}

/**
  * @brief  FIFO watermark status on INT2 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int2_fth in reg INT2_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_threshold_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  if(ret == 0){
    int2_ctrl.int2_fth = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  FIFO watermark status on INT2 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int2_fth in reg INT2_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_threshold_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  *val = int2_ctrl.int2_fth;

  return ret;
}

/**
  * @brief  FIFO full flag on INT2 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int2_fss5 in reg INT2_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_full_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  if(ret == 0){
    int2_ctrl.int2_fss5 = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  FIFO full flag on INT2 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int2_fss5 in reg INT2_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_full_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  *val = int2_ctrl.int2_fss5;

  return ret;
}

/**
  * @brief  FIFO overrun interrupt on INT2 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int2_ovr in reg INT2_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_ovr_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  if(ret == 0){
    int2_ctrl.int2_ovr = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  FIFO overrun interrupt on INT2 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int2_ovr in reg INT2_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_ovr_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  *val = int2_ctrl.int2_ovr;

  return ret;
}

/**
  * @brief  BOOT status on INT2 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int2_boot in reg INT2_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_boot_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  if(ret == 0){
    int2_ctrl.int2_boot = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  BOOT status on INT2 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int2_boot in reg INT2_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_boot_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  *val = int2_ctrl.int2_boot;

  return ret;
}

/**
  * @brief  Data-ready signal on INT2 pin.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of int2_drdy in reg INT2_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_drdy_on_int2_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  if(ret == 0){
    int2_ctrl.int2_drdy = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  Data-ready signal on INT2 pin.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of int2_drdy in reg INT2_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_drdy_on_int2_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_int2_ctrl_t int2_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_INT2_CTRL, (uint8_t*)&int2_ctrl, 1);
  *val = int2_ctrl.int2_drdy;

  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of pp_od in reg CTRL_REG4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_pin_mode_set(lis3dhh_ctx_t *ctx, lis3dhh_pp_od_t val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0){
    ctrl_reg4.pp_od = (uint8_t)val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  }

  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of pp_od in reg CTRL_REG4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_pin_mode_get(lis3dhh_ctx_t *ctx, lis3dhh_pp_od_t *val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);

  switch (ctrl_reg4.pp_od){
    case LIS3DHH_ALL_PUSH_PULL:
      *val = LIS3DHH_ALL_PUSH_PULL;
      break;
    case LIS3DHH_INT1_OD_INT2_PP:
      *val = LIS3DHH_INT1_OD_INT2_PP;
      break;
    case LIS3DHH_INT1_PP_INT2_OD:
      *val = LIS3DHH_INT1_PP_INT2_OD;
      break;
    case LIS3DHH_ALL_OPEN_DRAIN:
      *val = LIS3DHH_ALL_OPEN_DRAIN;
      break;
    default:
      *val = LIS3DHH_ALL_PUSH_PULL;
      break;
  }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS3DHH_fifo
  * @brief     This section group all the functions concerning the
  *            fifo usage
  * @{
  *
  */

/**
  * @brief  FIFOenable.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fifo_en in reg CTRL_REG4
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  if(ret == 0){
    ctrl_reg4.fifo_en = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  }

  return ret;
}

/**
  * @brief  FIFOenable.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fifo_en in reg CTRL_REG4.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_ctrl_reg4_t ctrl_reg4;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG4, (uint8_t*)&ctrl_reg4, 1);
  *val = ctrl_reg4.fifo_en;

  return ret;
}

/**
  * @brief  Enables the SPI high speed configuration for the FIFO block that
            is used to guarantee a minimum duration of the window in which
            writing operation of RAM output is blocked. This bit is recommended
            for SPI clock frequencies higher than 6 MHz.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fifo_spi_hs_on in reg CTRL_REG5
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_block_spi_hs_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_ctrl_reg5_t ctrl_reg5;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  if(ret == 0){
    ctrl_reg5.fifo_spi_hs_on = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  }

  return ret;
}

/**
  * @brief  Enables the SPI high speed configuration for the FIFO block that
            is used to guarantee a minimum duration of the window in which
            writing operation of RAM output is blocked. This bit is recommended
            for SPI clock frequencies higher than 6 MHz.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fifo_spi_hs_on in reg CTRL_REG5.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_block_spi_hs_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_ctrl_reg5_t ctrl_reg5;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG5, (uint8_t*)&ctrl_reg5, 1);
  *val = ctrl_reg5.fifo_spi_hs_on;

  return ret;
}

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fth in reg FIFO_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_watermark_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  if(ret == 0){
    fifo_ctrl.fth = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
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
int32_t lis3dhh_fifo_watermark_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  *val = fifo_ctrl.fth;

  return ret;
}

/**
  * @brief  FIFO mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of fmode in reg FIFO_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_mode_set(lis3dhh_ctx_t *ctx, lis3dhh_fmode_t val)
{
  lis3dhh_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  if(ret == 0){
    fifo_ctrl.fmode = (uint8_t)val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }

  return ret;
}

/**
  * @brief  FIFO mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fmode in reg FIFO_CTRL.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_mode_get(lis3dhh_ctx_t *ctx, lis3dhh_fmode_t *val)
{
  lis3dhh_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);

  switch (fifo_ctrl.fmode){
    case LIS3DHH_BYPASS_MODE:
      *val = LIS3DHH_BYPASS_MODE;
      break;
    case LIS3DHH_FIFO_MODE:
      *val = LIS3DHH_FIFO_MODE;
      break;
    case LIS3DHH_STREAM_TO_FIFO_MODE:
      *val = LIS3DHH_STREAM_TO_FIFO_MODE;
      break;
    case LIS3DHH_BYPASS_TO_STREAM_MODE:
      *val = LIS3DHH_BYPASS_TO_STREAM_MODE;
      break;
    case LIS3DHH_DYNAMIC_STREAM_MODE:
      *val = LIS3DHH_DYNAMIC_STREAM_MODE;
      break;
    default:
      *val = LIS3DHH_BYPASS_MODE;
      break;
  }

  return ret;
}

/**
  * @brief  FIFO status register.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get registers FIFO_SRC.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_status_get(lis3dhh_ctx_t *ctx, lis3dhh_fifo_src_t *val)
{
  int32_t ret;
  ret = lis3dhh_read_reg(ctx, LIS3DHH_FIFO_SRC, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  FIFO stored data level.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fss in reg FIFO_SRC.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_full_flag_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_fifo_src_t fifo_src;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_FIFO_SRC, (uint8_t*)&fifo_src, 1);
  *val = fifo_src.fss;

  return ret;
}

/**
  * @brief  FIFO overrun status flag.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of ovrn in reg FIFO_SRC.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_ovr_flag_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_fifo_src_t fifo_src;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_FIFO_SRC, (uint8_t*)&fifo_src, 1);
  *val = fifo_src.ovrn;

  return ret;
}

/**
  * @brief  FIFO watermark status.[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of fth in reg FIFO_SRC.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_fifo_fth_flag_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_fifo_src_t fifo_src;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_FIFO_SRC, (uint8_t*)&fifo_src, 1);
  *val = fifo_src.fth;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LIS3DHH_serial_interface
  * @brief     This section group all the functions concerning serial
  *            interface management
  * @{
  *
  */

/**
  * @brief  Register address automatically incremented during a multiple byte
  *         access with a serial interface (I2C or SPI).[set]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Change the values of if_add_inc in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_auto_add_inc_set(lis3dhh_ctx_t *ctx, uint8_t val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.if_add_inc = val;
    ret = lis3dhh_write_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }

  return ret;
}

/**
  * @brief  Register address automatically incremented during a multiple byte
  *         access with a serial interface (I2C or SPI).[get]
  *
  * @param  ctx    Read / write interface definitions.(ptr)
  * @param  val    Get the values of if_add_inc in reg CTRL_REG1.(ptr)
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lis3dhh_auto_add_inc_get(lis3dhh_ctx_t *ctx, uint8_t *val)
{
  lis3dhh_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lis3dhh_read_reg(ctx, LIS3DHH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  *val = ctrl_reg1.if_add_inc;

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