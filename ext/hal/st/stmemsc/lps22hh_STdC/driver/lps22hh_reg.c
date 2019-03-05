/*
 ******************************************************************************
 * @file    lps22hh_reg.c
 * @author  Sensors Software Solution Team
 * @brief   LPS22HH driver file
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

#include "lps22hh_reg.h"

/**
  * @defgroup  LPS22HH
  * @brief     This file provides a set of functions needed to drive the
  *            lps22hh enhanced inertial module.
  * @{
  *
  */

/**
  * @defgroup  LPS22HH_Interfaces_Functions
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
  * @retval       interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_read_reg(lps22hh_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lps22hh_write_reg(lps22hh_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LPS22HH_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */
float_t lps22hh_from_lsb_to_hpa(int16_t lsb)
{
  return ( (float_t) lsb / 4096.0f );
}

float_t lps22hh_from_lsb_to_celsius(int16_t lsb)
{
  return ( (float_t) lsb / 100.0f );
}

/**
  * @}
  *
  */

/**
  * @defgroup  LPS22HH_Data_Generation
  * @brief     This section groups all the functions concerning
  *            data generation.
  * @{
  *
  */

/**
  * @brief  Reset Autozero function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reset_az in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_autozero_rst_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.reset_az = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Reset Autozero function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reset_az in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_autozero_rst_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  *val = reg.reset_az;

  return ret;
}

/**
  * @brief  Enable Autozero function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of autozero in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_autozero_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.autozero = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable Autozero function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of autozero in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_autozero_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  *val = reg.autozero;

  return ret;
}

/**
  * @brief  Reset AutoRifP function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reset_arp in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_snap_rst_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.reset_arp = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Reset AutoRifP function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of reset_arp in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_snap_rst_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  *val = reg.reset_arp;

  return ret;
}

/**
  * @brief  Enable AutoRefP function.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of autorefp in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_snap_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.autorefp = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable AutoRefP function.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of autorefp in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_snap_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  *val = reg.autorefp;

  return ret;
}

/**
  * @brief  Block Data Update.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdu in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_block_data_update_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_ctrl_reg1_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.bdu = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Block Data Update.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of bdu in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_block_data_update_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_ctrl_reg1_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  *val = reg.bdu;

  return ret;
}

/**
  * @brief  Output data rate selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of odr in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_data_rate_set(lps22hh_ctx_t *ctx, lps22hh_odr_t val)
{
  lps22hh_ctrl_reg1_t ctrl_reg1;
  lps22hh_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if (ret == 0) {
    ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  if (ret == 0) {
    ctrl_reg1.odr = (uint8_t)val & 0x07U;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }
  if (ret == 0) {
    ctrl_reg2.low_noise_en = ((uint8_t)val & 0x10U) >> 4;
    ctrl_reg2.one_shot = ((uint8_t)val & 0x08U) >> 3;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Output data rate selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of odr in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_data_rate_get(lps22hh_ctx_t *ctx, lps22hh_odr_t *val)
{
  lps22hh_ctrl_reg1_t ctrl_reg1;
  lps22hh_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if (ret == 0) {
    ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  if (ret == 0) {
    ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
    switch (((ctrl_reg2.low_noise_en << 4) + (ctrl_reg2.one_shot << 3) +
            ctrl_reg1.odr )) {
      case LPS22HH_POWER_DOWN:
        *val = LPS22HH_POWER_DOWN;
        break;
      case LPS22HH_ONE_SHOOT:
        *val = LPS22HH_ONE_SHOOT;
        break;
      case LPS22HH_1_Hz:
        *val = LPS22HH_1_Hz;
        break;
      case LPS22HH_10_Hz:
        *val = LPS22HH_10_Hz;
        break;
      case LPS22HH_25_Hz:
        *val = LPS22HH_25_Hz;
        break;
      case LPS22HH_50_Hz:
        *val = LPS22HH_50_Hz;
        break;
      case LPS22HH_75_Hz:
        *val = LPS22HH_75_Hz;
        break;
      case LPS22HH_1_Hz_LOW_NOISE:
        *val = LPS22HH_1_Hz_LOW_NOISE;
        break;
      case LPS22HH_10_Hz_LOW_NOISE:
        *val = LPS22HH_10_Hz_LOW_NOISE;
        break;
      case LPS22HH_25_Hz_LOW_NOISE:
        *val = LPS22HH_25_Hz_LOW_NOISE;
        break;
      case LPS22HH_50_Hz_LOW_NOISE:
        *val = LPS22HH_50_Hz_LOW_NOISE;
        break;
      case LPS22HH_75_Hz_LOW_NOISE:
        *val = LPS22HH_75_Hz_LOW_NOISE;
        break;
      case LPS22HH_100_Hz:
        *val = LPS22HH_100_Hz;
        break;
      case LPS22HH_200_Hz:
        *val = LPS22HH_200_Hz;
        break;
      default:
        *val = LPS22HH_POWER_DOWN;
        break;
    }
  }
  return ret;
}

/**
  * @brief  The Reference pressure value is a 16-bit data
  *         expressed as 2’s complement. The value is used
  *         when AUTOZERO or AUTORIFP function is enabled.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_ref_set(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret = lps22hh_write_reg(ctx, LPS22HH_REF_P_L, buff, 2);
  return ret;
}

/**
  * @brief  The Reference pressure value is a 16-bit
  *         data expressed as 2’s complement.
  *         The value is used when AUTOZERO or AUTORIFP
  *         function is enabled.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_ref_get(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_REF_P_L, buff, 2);
  return ret;
}

/**
  * @brief  The pressure offset value is 16-bit data
  *         that can be used to implement one-point
  *         calibration (OPC) after soldering.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_offset_set(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_write_reg(ctx, LPS22HH_RPDS_L, buff, 2);
  return ret;
}

/**
  * @brief  The pressure offset value is 16-bit
  *         data that can be used to implement
  *         one-point calibration (OPC) after
  *         soldering.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_offset_get(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_RPDS_L, buff, 2);
  return ret;
}

/**
  * @brief  Read all the interrupt/status flag of the device.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers STATUS,FIFO_STATUS2,INT_SOURCE
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_all_sources_get(lps22hh_ctx_t *ctx, lps22hh_all_sources_t *val)
{
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INT_SOURCE,
                         (uint8_t*) &(val->int_source), 1);
  if (ret == 0) {
    ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_STATUS2,
                           (uint8_t*) &(val->fifo_status2), 1);
  }
  if (ret == 0) {
    ret = lps22hh_read_reg(ctx, LPS22HH_STATUS,
                           (uint8_t*) &(val->status), 1);
  }
  return ret;
}

/**
  * @brief  The STATUS_REG register is read by the primary interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      structure of registers from STATUS to STATUS_REG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_status_reg_get(lps22hh_ctx_t *ctx, lps22hh_status_t *val)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_STATUS, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Pressure new data available.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of p_da in reg STATUS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_press_flag_data_ready_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_status_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_STATUS, (uint8_t*) &reg, 1);
  *val = reg.p_da;

  return ret;
}

/**
  * @brief  Temperature data available.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of t_da in reg STATUS
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_temp_flag_data_ready_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_status_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_STATUS, (uint8_t*) &reg, 1);
  *val = reg.t_da;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LPS22HH_Data_Output
  * @brief     This section groups all the data output functions.
  * @{
  *
  */

/**
  * @brief  Pressure output value.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pressure_raw_get(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_PRESS_OUT_XL, buff, 3);
  return ret;
}

/**
  * @brief  Temperature output value.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_temperature_raw_get(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_TEMP_OUT_L, buff, 2);
  return ret;
}

/**
  * @brief  Pressure output from FIFO value.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_pressure_raw_get(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_FIFO_DATA_OUT_PRESS_XL, buff, 3);
  return ret;
}

/**
  * @brief  Temperature output from FIFO value.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_temperature_raw_get(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_FIFO_DATA_OUT_TEMP_L, buff, 2);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LPS22HH_Common
  * @brief     This section groups common useful functions.
  * @{
  *
  */

/**
  * @brief  DeviceWhoamI[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_device_id_get(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_WHO_AM_I, buff, 1);
  return ret;
}

/**
  * @brief  Software reset. Restore the default values
  *         in user registers.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of swreset in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_reset_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.swreset = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief   Software reset. Restore the default values
  *          in user registers.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of swreset in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_reset_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  *val = reg.swreset;

  return ret;
}

/**
  * @brief  Register address automatically
  *         incremented during a multiple byte access
  *         with a serial interface.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of if_add_inc in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_auto_increment_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.if_add_inc = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Register address automatically
  *         incremented during a multiple byte
  *         access with a serial interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of if_add_inc in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_auto_increment_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  *val = reg.if_add_inc;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration
  *         parameters.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of boot in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_boot_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.boot = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration
  *         parameters.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of boot in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_boot_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  *val = reg.boot;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LPS22HH_Filters
  * @brief     This section group all the functions concerning the
  *            filters configuration.
  * @{
  *
  */

/**
  * @brief  Low-pass bandwidth selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lpfp_cfg in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_lp_bandwidth_set(lps22hh_ctx_t *ctx, lps22hh_lpfp_cfg_t val)
{
  lps22hh_ctrl_reg1_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.lpfp_cfg = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Low-pass bandwidth selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of lpfp_cfg in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_lp_bandwidth_get(lps22hh_ctx_t *ctx, lps22hh_lpfp_cfg_t *val)
{
  lps22hh_ctrl_reg1_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  switch (reg.lpfp_cfg) {
      case LPS22HH_LPF_ODR_DIV_2:
        *val = LPS22HH_LPF_ODR_DIV_2;
        break;
      case LPS22HH_LPF_ODR_DIV_9:
        *val = LPS22HH_LPF_ODR_DIV_9;
        break;
      case LPS22HH_LPF_ODR_DIV_20:
        *val = LPS22HH_LPF_ODR_DIV_20;
        break;
      default:
        *val = LPS22HH_LPF_ODR_DIV_2;
        break;
    }

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LPS22HH_Serial_Interface
  * @brief     This section groups all the functions concerning serial
  *            interface management
  * @{
  *
  */

/**
  * @brief  Enable/Disable I2C interface.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of i2c_disable in reg IF_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_i2c_interface_set(lps22hh_ctx_t *ctx,
                                  lps22hh_i2c_disable_t val)
{
  lps22hh_if_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.i2c_disable = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable/Disable I2C interface.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of i2c_disable in reg IF_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_i2c_interface_get(lps22hh_ctx_t *ctx,
                                  lps22hh_i2c_disable_t *val)
{
  lps22hh_if_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  switch (reg.i2c_disable) {
      case LPS22HH_I2C_ENABLE:
        *val = LPS22HH_I2C_ENABLE;
        break;
      case LPS22HH_I2C_DISABLE:
        *val = LPS22HH_I2C_DISABLE;
        break;
      default:
        *val = LPS22HH_I2C_ENABLE;
        break;
    }

  return ret;
}

/**
  * @brief  I3C Enable/Disable communication protocol.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int_en_i3c in reg IF_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_i3c_interface_set(lps22hh_ctx_t *ctx,
                                  lps22hh_i3c_disable_t val)
{
  lps22hh_if_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.i3c_disable = ((uint8_t)val & 0x01u);
    reg.int_en_i3c = ((uint8_t)val & 0x10U) >> 4;
    ret = lps22hh_write_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  I3C Enable/Disable communication protocol.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int_en_i3c in reg IF_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_i3c_interface_get(lps22hh_ctx_t *ctx,
                                  lps22hh_i3c_disable_t *val)
{
  lps22hh_if_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);

  switch ((reg.int_en_i3c << 4) + reg.int_en_i3c) {
    case LPS22HH_I3C_ENABLE:
      *val = LPS22HH_I3C_ENABLE;
      break;
    case LPS22HH_I3C_ENABLE_INT_PIN_ENABLE:
      *val = LPS22HH_I3C_ENABLE_INT_PIN_ENABLE;
      break;
    case LPS22HH_I3C_DISABLE:
      *val = LPS22HH_I3C_DISABLE;
      break;
    default:
      *val = LPS22HH_I3C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Enable/Disable pull-up on SDO pin.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sdo_pu_en in reg IF_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_sdo_sa0_mode_set(lps22hh_ctx_t *ctx, lps22hh_pu_en_t val)
{
  lps22hh_if_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.sdo_pu_en = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable/Disable pull-up on SDO pin.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sdo_pu_en in reg IF_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_sdo_sa0_mode_get(lps22hh_ctx_t *ctx, lps22hh_pu_en_t *val)
{
  lps22hh_if_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  switch (reg.sdo_pu_en) {
    case LPS22HH_PULL_UP_DISCONNECT:
      *val = LPS22HH_PULL_UP_DISCONNECT;
      break;
    case LPS22HH_PULL_UP_CONNECT:
      *val = LPS22HH_PULL_UP_CONNECT;
      break;
    default:
      *val = LPS22HH_PULL_UP_DISCONNECT;
      break;
  }

  return ret;
}

/**
  * @brief  Connect/Disconnect SDO/SA0 internal pull-up.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sda_pu_en in reg IF_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_sda_mode_set(lps22hh_ctx_t *ctx, lps22hh_pu_en_t val)
{
  lps22hh_if_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.sda_pu_en = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Connect/Disconnect SDO/SA0 internal pull-up.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sda_pu_en in reg IF_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_sda_mode_get(lps22hh_ctx_t *ctx, lps22hh_pu_en_t *val)
{
  lps22hh_if_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_IF_CTRL, (uint8_t*) &reg, 1);
  switch (reg.sda_pu_en) {
    case LPS22HH_PULL_UP_DISCONNECT:
      *val = LPS22HH_PULL_UP_DISCONNECT;
      break;
    case LPS22HH_PULL_UP_CONNECT:
      *val = LPS22HH_PULL_UP_CONNECT;
      break;
    default:
      *val = LPS22HH_PULL_UP_DISCONNECT;
      break;
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of sim in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_spi_mode_set(lps22hh_ctx_t *ctx, lps22hh_sim_t val)
{
  lps22hh_ctrl_reg1_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.sim = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of sim in reg CTRL_REG1
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_spi_mode_get(lps22hh_ctx_t *ctx, lps22hh_sim_t *val)
{
  lps22hh_ctrl_reg1_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG1, (uint8_t*) &reg, 1);
  switch (reg.sim) {
    case LPS22HH_SPI_4_WIRE:
      *val = LPS22HH_SPI_4_WIRE;
      break;
    case LPS22HH_SPI_3_WIRE:
      *val = LPS22HH_SPI_3_WIRE;
      break;
    default:
      *val = LPS22HH_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LPS22HH_Interrupt_Pins
  * @brief     This section groups all the functions that manage
  *            interrupt pins.
  * @{
  *
  */

/**
  * @brief  Latch interrupt request to the INT_SOURCE (24h) register.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of lir in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_int_notification_set(lps22hh_ctx_t *ctx, lps22hh_lir_t val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.lir = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Latch interrupt request to the INT_SOURCE (24h) register.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of lir in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_int_notification_get(lps22hh_ctx_t *ctx, lps22hh_lir_t *val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);

  switch (reg.lir) {
    case LPS22HH_INT_PULSED:
      *val = LPS22HH_INT_PULSED;
      break;
    case LPS22HH_INT_LATCHED:
      *val = LPS22HH_INT_LATCHED;
      break;
    default:
      *val = LPS22HH_INT_PULSED;
      break;
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of pp_od in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pin_mode_set(lps22hh_ctx_t *ctx, lps22hh_pp_od_t val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.pp_od = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of pp_od in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pin_mode_get(lps22hh_ctx_t *ctx, lps22hh_pp_od_t *val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);


  switch (reg.pp_od) {
    case LPS22HH_PUSH_PULL:
      *val = LPS22HH_PUSH_PULL;
      break;
    case LPS22HH_OPEN_DRAIN:
      *val = LPS22HH_OPEN_DRAIN;
      break;
    default:
      *val = LPS22HH_PUSH_PULL;
      break;
  }

  return ret;
}

/**
  * @brief  Interrupt active-high/low.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of int_h_l in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pin_polarity_set(lps22hh_ctx_t *ctx, lps22hh_int_h_l_t val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.int_h_l = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);
  }

  return ret;
}

/**
  * @brief  Interrupt active-high/low.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of int_h_l in reg CTRL_REG2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pin_polarity_get(lps22hh_ctx_t *ctx, lps22hh_int_h_l_t *val)
{
  lps22hh_ctrl_reg2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG2, (uint8_t*) &reg, 1);

  switch (reg.int_h_l) {
    case LPS22HH_ACTIVE_HIGH:
      *val = LPS22HH_ACTIVE_HIGH;
      break;
    case LPS22HH_ACTIVE_LOW:
      *val = LPS22HH_ACTIVE_LOW;
      break;
    default:
      *val = LPS22HH_ACTIVE_HIGH;
      break;
  }

  return ret;
}

/**
  * @brief  Select the signal that need to route on int pad.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers CTRL_REG3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pin_int_route_set(lps22hh_ctx_t *ctx,
                                  lps22hh_ctrl_reg3_t *val)
{
  int32_t ret;
  ret =  lps22hh_write_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Select the signal that need to route on int pad.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers CTRL_REG3
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_pin_int_route_get(lps22hh_ctx_t *ctx,
                                  lps22hh_ctrl_reg3_t *val)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*) val, 1);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup   LPS22HH_Interrupt_on_Threshold
  * @brief      This section groups all the functions that manage the
  *             interrupt on threshold event generation.
  * @{
  *
  */

/**
  * @brief   Enable interrupt generation on pressure low/high event.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of pe in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_int_on_threshold_set(lps22hh_ctx_t *ctx, lps22hh_pe_t val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.pe = (uint8_t)val;

    if (val == LPS22HH_NO_THRESHOLD){
      reg.diff_en = PROPERTY_DISABLE;
    }
    else{
      reg.diff_en = PROPERTY_ENABLE;
    }
    ret = lps22hh_write_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Enable interrupt generation on pressure low/high event.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of pe in reg INTERRUPT_CFG
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_int_on_threshold_get(lps22hh_ctx_t *ctx, lps22hh_pe_t *val)
{
  lps22hh_interrupt_cfg_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_INTERRUPT_CFG, (uint8_t*) &reg, 1);

  switch (reg.pe) {
    case LPS22HH_NO_THRESHOLD:
      *val = LPS22HH_NO_THRESHOLD;
      break;
    case LPS22HH_POSITIVE:
      *val = LPS22HH_POSITIVE;
      break;
    case LPS22HH_NEGATIVE:
      *val = LPS22HH_NEGATIVE;
      break;
    case LPS22HH_BOTH:
      *val = LPS22HH_BOTH;
      break;
    default:
      *val = LPS22HH_NO_THRESHOLD;
      break;
  }

  return ret;
}

/**
  * @brief  User-defined threshold value for pressure interrupt event.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that contains data to write
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_int_treshold_set(lps22hh_ctx_t *ctx, uint16_t buff)
{
  int32_t ret;
  lps22hh_ths_p_l_t ths_p_l;
  lps22hh_ths_p_h_t ths_p_h;
  
  ths_p_l.ths = (uint8_t)(buff & 0x00FFU);
  ths_p_h.ths = (uint8_t)((buff & 0x7F00U) >> 8);
  
  ret =  lps22hh_write_reg(ctx, LPS22HH_THS_P_L,
                           (uint8_t*)&ths_p_l, 1);
  if (ret == 0) {
      ret =  lps22hh_write_reg(ctx, LPS22HH_THS_P_H, 
                               (uint8_t*)&ths_p_h, 1);
  }
  return ret;
}

/**
  * @brief   User-defined threshold value for pressure interrupt event.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_int_treshold_get(lps22hh_ctx_t *ctx, uint16_t *buff)
{
  int32_t ret;
  lps22hh_ths_p_l_t ths_p_l;
  lps22hh_ths_p_h_t ths_p_h;
  
  ret =  lps22hh_read_reg(ctx, LPS22HH_THS_P_L,
                           (uint8_t*)&ths_p_l, 1);
  if (ret == 0) {
      ret =  lps22hh_read_reg(ctx, LPS22HH_THS_P_H, 
                               (uint8_t*)&ths_p_h, 1);
      *buff = (uint16_t)ths_p_h.ths << 8;
      *buff |= (uint16_t)ths_p_l.ths;
  }  
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup  LPS22HH_Fifo
  * @brief   This section group all the functions concerning the fifo usage.
  * @{
  *
  */

/**
  * @brief  Fifo Mode selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of f_mode in reg FIFO_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_mode_set(lps22hh_ctx_t *ctx, lps22hh_f_mode_t val)
{
  lps22hh_fifo_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_CTRL, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.f_mode = (uint8_t)val;
    ret = lps22hh_write_reg(ctx, LPS22HH_FIFO_CTRL, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  Fifo Mode selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      Get the values of f_mode in reg FIFO_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_mode_get(lps22hh_ctx_t *ctx, lps22hh_f_mode_t *val)
{
  lps22hh_fifo_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_CTRL, (uint8_t*) &reg, 1);

  switch (reg.f_mode) {
    case LPS22HH_BYPASS_MODE:
      *val = LPS22HH_BYPASS_MODE;
      break;
    case LPS22HH_FIFO_MODE:
      *val = LPS22HH_FIFO_MODE;
      break;
    case LPS22HH_STREAM_MODE:
      *val = LPS22HH_STREAM_MODE;
      break;
    case LPS22HH_DYNAMIC_STREAM_MODE:
      *val = LPS22HH_DYNAMIC_STREAM_MODE;
      break;
    case LPS22HH_BYPASS_TO_FIFO_MODE:
      *val = LPS22HH_BYPASS_TO_FIFO_MODE;
      break;
    case LPS22HH_BYPASS_TO_STREAM_MODE:
      *val = LPS22HH_BYPASS_TO_STREAM_MODE;
      break;
    case LPS22HH_STREAM_TO_FIFO_MODE:
      *val = LPS22HH_STREAM_TO_FIFO_MODE;
      break;
    default:
      *val = LPS22HH_BYPASS_MODE;
      break;
  }

  return ret;
}

/**
  * @brief  Sensing chain FIFO stop values memorization at
  *         threshold level.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of stop_on_wtm in reg FIFO_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_stop_on_wtm_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_fifo_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_CTRL, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.stop_on_wtm = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_FIFO_CTRL, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief   Sensing chain FIFO stop values memorization at threshold
  *          level.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of stop_on_wtm in reg FIFO_CTRL
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_stop_on_wtm_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_fifo_ctrl_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_CTRL, (uint8_t*) &reg, 1);
  *val = reg.stop_on_wtm;

  return ret;
}

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wtm in reg FIFO_WTM
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_watermark_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_fifo_wtm_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_WTM, (uint8_t*) &reg, 1);
  if (ret == 0) {
    reg.wtm = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_FIFO_WTM, (uint8_t*) &reg, 1);
  }
  return ret;
}

/**
  * @brief  FIFO watermark level selection.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of wtm in reg FIFO_WTM
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_watermark_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_fifo_wtm_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_WTM, (uint8_t*) &reg, 1);
  *val = reg.wtm;

  return ret;
}

/**
  * @brief  FIFO stored data level.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  buff     buffer that stores data read
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_data_level_get(lps22hh_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_FIFO_STATUS1, buff, 1);
  return ret;
}

/**
  * @brief  Read all the FIFO status flag of the device.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      registers FIFO_STATUS2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_src_get(lps22hh_ctx_t *ctx, lps22hh_fifo_status2_t *val)
{
  int32_t ret;
  ret =  lps22hh_read_reg(ctx, LPS22HH_FIFO_STATUS2, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Smart FIFO full status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_full_ia in reg FIFO_STATUS2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_full_flag_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_fifo_status2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_STATUS2, (uint8_t*) &reg, 1);
  *val = reg.fifo_full_ia;

  return ret;
}

/**
  * @brief  FIFO overrun status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_ovr_ia in reg FIFO_STATUS2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_ovr_flag_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_fifo_status2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_STATUS2, (uint8_t*) &reg, 1);
  *val = reg.fifo_ovr_ia;

  return ret;
}

/**
  * @brief  FIFO watermark status.[get]
  *
  * @param  ctx      read / write interface definitions
  * @param  val      change the values of fifo_wtm_ia in reg FIFO_STATUS2
  * @retval          interface status (MANDATORY: return 0 -> no Error)
  *
  */
int32_t lps22hh_fifo_wtm_flag_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_fifo_status2_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_FIFO_STATUS2, (uint8_t*)&reg, 1);
  *val = reg.fifo_wtm_ia;

  return ret;
}

/**
  * @brief  FIFO overrun interrupt on INT_DRDY pin.[set]
  *
  * @param  lps22hh_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of f_ovr in reg CTRL_REG3
  *
  */
int32_t lps22hh_fifo_ovr_on_int_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_ctrl_reg3_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.int_f_ovr = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  FIFO overrun interrupt on INT_DRDY pin.[get]
  *
  * @param  lps22hh_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of f_ovr in reg CTRL_REG3
  *
  */
int32_t lps22hh_fifo_ovr_on_int_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_ctrl_reg3_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  *val = reg.int_f_ovr;

  return ret;
}

/**
  * @brief  FIFO watermark status on INT_DRDY pin.[set]
  *
  * @param  lps22hh_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of f_fth in reg CTRL_REG3
  *
  */
int32_t lps22hh_fifo_threshold_on_int_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_ctrl_reg3_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.int_f_wtm = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  FIFO watermark status on INT_DRDY pin.[get]
  *
  * @param  lps22hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of f_fth in reg CTRL_REG3
  *
  */
int32_t lps22hh_fifo_threshold_on_int_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_ctrl_reg3_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  *val = reg.int_f_wtm;

  return ret;
}

/**
  * @brief  FIFO full flag on INT_DRDY pin.[set]
  *
  * @param  lps22hh_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of f_fss5 in reg CTRL_REG3
  *
  */
int32_t lps22hh_fifo_full_on_int_set(lps22hh_ctx_t *ctx, uint8_t val)
{
  lps22hh_ctrl_reg3_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  if (ret == 0) {
    reg.int_f_full = val;
    ret = lps22hh_write_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  }
  return ret;
}

/**
  * @brief  FIFO full flag on INT_DRDY pin.[get]
  *
  * @param  lps22hh_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of f_fss5 in reg CTRL_REG3
  *
  */
int32_t lps22hh_fifo_full_on_int_get(lps22hh_ctx_t *ctx, uint8_t *val)
{
  lps22hh_ctrl_reg3_t reg;
  int32_t ret;

  ret = lps22hh_read_reg(ctx, LPS22HH_CTRL_REG3, (uint8_t*)&reg, 1);
  *val = reg.int_f_full;

  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/