/*
 ******************************************************************************
 * @file    lps22hb_reg.c
 * @author  Sensors Software Solution Team
 * @brief   LPS22HB driver file
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

#include "lps22hb_reg.h"

/**
  * @defgroup    LPS22HB
  * @brief       This file provides a set of functions needed to drive the
  *              ultra-compact piezoresistive absolute pressure sensor.
  * @{
  *
  */

/** 
  * @defgroup    LPS22HB_Interfaces_functions
  * @brief       This section provide a set of functions used to read and
  *              write a generic register of the device.
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
int32_t lps22hb_read_reg(lps22hb_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
int32_t lps22hb_write_reg(lps22hb_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @defgroup    LPS22HB_Sensitivity
  * @brief       These functions convert raw-data into engineering units.
  * @{
  *
  */

float_t lps22hb_from_lsb_to_hpa(uint32_t lsb)
{
  return ( (float_t)lsb / 4096.0f );
}

float_t lps22hb_from_lsb_to_degc(int16_t lsb)
{
  return ( (float_t)lsb / 100.0f );
}

/**
  * @}
  *
  */

/**
  * @defgroup    LPS22HB_data_generation_c
  * @brief       This section group all the functions concerning data
  *              generation
  * @{
  *
  */


/**
  * @brief  Reset Autozero function.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of reset_az in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */

int32_t lps22hb_autozero_rst_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  if(ret == 0){
    interrupt_cfg.reset_az = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_INTERRUPT_CFG,
                            (uint8_t*)&interrupt_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Reset Autozero function.[get] 
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of reset_az in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_autozero_rst_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  *val = interrupt_cfg.reset_az;

  return ret;
}

/**
  * @brief  Enable Autozero function.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of autozero in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_autozero_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG, 
                         (uint8_t*)&interrupt_cfg, 1);
  if(ret == 0){
    interrupt_cfg.autozero = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_INTERRUPT_CFG,
                            (uint8_t*)&interrupt_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Enable Autozero function.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of autozero in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_autozero_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  *val = interrupt_cfg.autozero;

  return ret;
}

/**
  * @brief  Reset AutoRifP function.[set] 
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of reset_arp in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_snap_rst_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  if(ret == 0){
    interrupt_cfg.reset_arp = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_INTERRUPT_CFG,
                            (uint8_t*)&interrupt_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Reset AutoRifP function.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of reset_arp in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_snap_rst_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  *val = interrupt_cfg.reset_arp;

  return ret;
}

/**
  * @brief  Enable AutoRifP function.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of autorifp in reg INTERRUPT_CFG.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_snap_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  if(ret == 0){
    interrupt_cfg.autorifp = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_INTERRUPT_CFG,
                            (uint8_t*)&interrupt_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Enable AutoRifP function.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of autorifp in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_snap_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  *val = interrupt_cfg.autorifp;

  return ret;
}

/**
  * @brief  Block data update.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of bdu in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_block_data_update_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.bdu = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Block data update.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of bdu in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_block_data_update_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  *val = ctrl_reg1.bdu;

  return ret;
}

/**
  * @brief  Low-pass bandwidth selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of lpfp in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_low_pass_filter_mode_set(lps22hb_ctx_t *ctx,
                                          lps22hb_lpfp_t val)
{
  lps22hb_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.lpfp = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief   Low-pass bandwidth selection.[get] 
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of lpfp in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_low_pass_filter_mode_get(lps22hb_ctx_t *ctx,
                                         lps22hb_lpfp_t *val)
{
  lps22hb_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  switch (ctrl_reg1.lpfp){
    case LPS22HB_LPF_ODR_DIV_2:
      *val = LPS22HB_LPF_ODR_DIV_2;
      break;
    case LPS22HB_LPF_ODR_DIV_9:
      *val = LPS22HB_LPF_ODR_DIV_9;
      break;
    case LPS22HB_LPF_ODR_DIV_20:
      *val = LPS22HB_LPF_ODR_DIV_20;
      break;
    default:
      *val = LPS22HB_LPF_ODR_DIV_2;
      break;
  }
  return ret;
}

/**
  * @brief  Output data rate selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of odr in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_data_rate_set(lps22hb_ctx_t *ctx, lps22hb_odr_t val)
{
  lps22hb_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.odr = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  Output data rate selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of odr in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_data_rate_get(lps22hb_ctx_t *ctx, lps22hb_odr_t *val)
{
  lps22hb_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  switch (ctrl_reg1.odr){
    case LPS22HB_POWER_DOWN:
      *val = LPS22HB_POWER_DOWN;
      break;
    case LPS22HB_ODR_1_Hz:
      *val = LPS22HB_ODR_1_Hz;
      break;
    case LPS22HB_ODR_10_Hz:
      *val = LPS22HB_ODR_10_Hz;
      break;
    case LPS22HB_ODR_25_Hz:
      *val = LPS22HB_ODR_25_Hz;
      break;
    case LPS22HB_ODR_50_Hz:
      *val = LPS22HB_ODR_50_Hz;
      break;
    case LPS22HB_ODR_75_Hz:
      *val = LPS22HB_ODR_75_Hz;
      break;
    default:
      *val = LPS22HB_ODR_1_Hz;
      break;
  }

  return ret;
}

/**
  * @brief  One-shot mode. Device perform a single measure.[set] 
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of one_shot in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_one_shoot_trigger_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0){
    ctrl_reg2.one_shot = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  One-shot mode. Device perform a single measure.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of one_shot in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_one_shoot_trigger_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  *val = ctrl_reg2.one_shot;

  return ret;
}

/**
  * @brief  pressure_ref:   The Reference pressure value is a 24-bit data
  *         expressed as 2’s complement. The value is used when AUTOZERO
  *         or AUTORIFP function is enabled.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_ref_set(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_write_reg(ctx, LPS22HB_REF_P_XL, buff, 3);
  return ret;
}

/**
  * @brief  pressure_ref:   The Reference pressure value is a 24-bit data
  *         expressed as 2’s complement. The value is used when AUTOZERO
  *         or AUTORIFP function is enabled.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_ref_get(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_REF_P_XL, buff, 3);
  return ret;
}

/**
  * @brief  The pressure offset value is 16-bit data that can be used to
  *         implement one-point calibration (OPC) after soldering.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_offset_set(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_write_reg(ctx, LPS22HB_RPDS_L, buff, 2);
  return ret;
}

/**
  * @brief  The pressure offset value is 16-bit data that can be used to
  *         implement one-point calibration (OPC) after soldering.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_offset_get(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_RPDS_L, buff, 2);
  return ret;
}

/**
  * @brief  Pressure data available.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of p_da in reg STATUS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_press_data_ready_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_status_t status;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_STATUS, (uint8_t*)&status, 1);
  *val = status.p_da;

  return ret;
}

/**
  * @brief  Temperature data available.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of t_da in reg STATUS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_temp_data_ready_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_status_t status;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_STATUS, (uint8_t*)&status, 1);
  *val = status.t_da;

  return ret;
}

/**
  * @brief  Pressure data overrun.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of p_or in reg STATUS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_press_data_ovr_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_status_t status;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_STATUS, (uint8_t*)&status, 1);
  *val = status.p_or;

  return ret;
}

/**
  * @brief  Temperature data overrun.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of t_or in reg STATUS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_temp_data_ovr_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_status_t status;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_STATUS, (uint8_t*)&status, 1);
  *val = status.t_or;

  return ret;
}

/**
  * @brief  Pressure output value[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pressure_raw_get(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_PRESS_OUT_XL, buff, 3);
  return ret;
}

/**
  * @brief  temperature_raw:   Temperature output value[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read.
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_temperature_raw_get(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_TEMP_OUT_L, (uint8_t*) buff, 2);
  return ret;
}

/**
  * @brief  Low-pass filter reset register. If the LPFP is active, in
  *         order to avoid the transitory phase, the filter can be
  *         reset by reading this register before generating pressure
  *         measurements.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_low_pass_rst_get(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_LPFP_RES, (uint8_t*) buff, 1);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LPS22HB_common
  * @brief       This section group common usefull functions
  * @{
  *
  */

/**
  * @brief  Device Who am I[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_device_id_get(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_WHO_AM_I, (uint8_t*) buff, 1);
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of swreset in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_reset_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0){
    ctrl_reg2.swreset = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Software reset. Restore the default values in user registers[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of swreset in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_reset_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  *val = ctrl_reg2.swreset;

  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of boot in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_boot_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0){
    ctrl_reg2.boot = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Reboot memory content. Reload the calibration parameters.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of boot in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_boot_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  *val = ctrl_reg2.boot;

  return ret;
}

/**
  * @brief  Low current mode.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of lc_en in reg RES_CONF
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_low_power_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_res_conf_t res_conf;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_RES_CONF, (uint8_t*)&res_conf, 1);
  if(ret == 0){
    res_conf.lc_en = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_RES_CONF, (uint8_t*)&res_conf, 1);
  }
  return ret;
}

/**
  * @brief  Low current mode.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of lc_en in reg RES_CONF
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_low_power_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_res_conf_t res_conf;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_RES_CONF, (uint8_t*)&res_conf, 1);
  *val = res_conf.lc_en;

  return ret;
}

/**
  * @brief  If ‘1’ indicates that the Boot (Reboot) phase is running.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of boot_status in reg INT_SOURCE
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_boot_status_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_int_source_t int_source;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INT_SOURCE, (uint8_t*)&int_source, 1);
  *val = int_source.boot_status;

  return ret;
}

/**
  * @brief  All the status bit, FIFO and data generation[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Structure of registers from FIFO_STATUS to STATUS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_dev_status_get(lps22hb_ctx_t *ctx, lps22hb_dev_stat_t *val)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_FIFO_STATUS, (uint8_t*) val, 2);
  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LPS22HB_interrupts
  * @brief       This section group all the functions that manage interrupts
  * @{
  *
  */

/**
  * @brief  Enable interrupt generation on pressure low/high event.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pe in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_sign_of_int_threshold_set(lps22hb_ctx_t *ctx,
                                           lps22hb_pe_t val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  if(ret == 0){
    interrupt_cfg.pe = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_INTERRUPT_CFG,
                            (uint8_t*)&interrupt_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Enable interrupt generation on pressure low/high event.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of pe in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_sign_of_int_threshold_get(lps22hb_ctx_t *ctx,
                                           lps22hb_pe_t *val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  switch (interrupt_cfg.pe){
    case LPS22HB_NO_THRESHOLD:
      *val = LPS22HB_NO_THRESHOLD;
      break;
    case LPS22HB_POSITIVE:
      *val = LPS22HB_POSITIVE;
      break;
    case LPS22HB_NEGATIVE:
      *val = LPS22HB_NEGATIVE;
      break;
    case LPS22HB_BOTH:
      *val = LPS22HB_BOTH;
      break;
    default:
      *val = LPS22HB_NO_THRESHOLD;
      break;
  }
  return ret;
}

/**
  * @brief  Interrupt request to the INT_SOURCE (25h) register
  *         mode (pulsed / latched) [set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of lir in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_notification_mode_set(lps22hb_ctx_t *ctx,
                                           lps22hb_lir_t val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  if(ret == 0){
    interrupt_cfg.lir = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_INTERRUPT_CFG,
                            (uint8_t*)&interrupt_cfg, 1);
  }
  return ret;
}

/**
  * @brief   Interrupt request to the INT_SOURCE (25h) register
  *          mode (pulsed / latched) [get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of lir in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_notification_mode_get(lps22hb_ctx_t *ctx,
                                          lps22hb_lir_t *val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  switch (interrupt_cfg.lir){
    case LPS22HB_INT_PULSED:
      *val = LPS22HB_INT_PULSED;
      break;
    case LPS22HB_INT_LATCHED:
      *val = LPS22HB_INT_LATCHED;
      break;
    default:
      *val = LPS22HB_INT_PULSED;
      break;
  }
  return ret;
}

/**
  * @brief  Enable interrupt generation.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of diff_en in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_generation_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  if(ret == 0){
    interrupt_cfg.diff_en = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_INTERRUPT_CFG,
                            (uint8_t*)&interrupt_cfg, 1);
  }
  return ret;
}

/**
  * @brief  Enable interrupt generation.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of diff_en in reg INTERRUPT_CFG
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_generation_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_interrupt_cfg_t interrupt_cfg;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INTERRUPT_CFG,
                         (uint8_t*)&interrupt_cfg, 1);
  *val = interrupt_cfg.diff_en;

  return ret;
}

/**
  * @brief  User-defined threshold value for pressure interrupt event[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that contains data to write
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_threshold_set(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_write_reg(ctx, LPS22HB_THS_P_L, (uint8_t*) buff, 2);
  return ret;
}

/**
  * @brief  User-defined threshold value for pressure interrupt event[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  buff   Buffer that stores data read
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_threshold_get(lps22hb_ctx_t *ctx, uint8_t *buff)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_THS_P_L, (uint8_t*) buff, 2);
  return ret;
}

/**
  * @brief  Data signal on INT_DRDY pin control bits.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of int_s in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_pin_mode_set(lps22hb_ctx_t *ctx, lps22hb_int_s_t val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0){
    ctrl_reg3.int_s = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Data signal on INT_DRDY pin control bits.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of int_s in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_pin_mode_get(lps22hb_ctx_t *ctx, lps22hb_int_s_t *val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  switch (ctrl_reg3.int_s){
    case LPS22HB_DRDY_OR_FIFO_FLAGS:
      *val = LPS22HB_DRDY_OR_FIFO_FLAGS;
      break;
    case LPS22HB_HIGH_PRES_INT:
      *val = LPS22HB_HIGH_PRES_INT;
      break;
    case LPS22HB_LOW_PRES_INT:
      *val = LPS22HB_LOW_PRES_INT;
      break;
    case LPS22HB_EVERY_PRES_INT:
      *val = LPS22HB_EVERY_PRES_INT;
      break;
    default:
      *val = LPS22HB_DRDY_OR_FIFO_FLAGS;
      break;
  }
  return ret;
}

/**
  * @brief  Data-ready signal on INT_DRDY pin.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of drdy in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_drdy_on_int_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0){
    ctrl_reg3.drdy = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Data-ready signal on INT_DRDY pin.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of drdy in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_drdy_on_int_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  *val = ctrl_reg3.drdy;

  return ret;
}

/**
  * @brief  FIFO overrun interrupt on INT_DRDY pin.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of f_ovr in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_ovr_on_int_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0){
    ctrl_reg3.f_ovr = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  FIFO overrun interrupt on INT_DRDY pin.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of f_ovr in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_ovr_on_int_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  *val = ctrl_reg3.f_ovr;

  return ret;
}

/**
  * @brief  FIFO watermark status on INT_DRDY pin.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of f_fth in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_threshold_on_int_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0){
    ctrl_reg3.f_fth = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief   FIFO watermark status on INT_DRDY pin.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of f_fth in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_threshold_on_int_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  *val = ctrl_reg3.f_fth;

  return ret;
}

/**
  * @brief  FIFO full flag on INT_DRDY pin.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of f_fss5 in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_full_on_int_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0){
    ctrl_reg3.f_fss5 = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  FIFO full flag on INT_DRDY pin.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of f_fss5 in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_full_on_int_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  *val = ctrl_reg3.f_fss5;

  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pp_od in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pin_mode_set(lps22hb_ctx_t *ctx, lps22hb_pp_od_t val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0){
    ctrl_reg3.pp_od = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Push-pull/open drain selection on interrupt pads.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of pp_od in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_pin_mode_get(lps22hb_ctx_t *ctx, lps22hb_pp_od_t *val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  switch (ctrl_reg3.pp_od){
    case LPS22HB_PUSH_PULL:
      *val = LPS22HB_PUSH_PULL;
      break;
    case LPS22HB_OPEN_DRAIN:
      *val = LPS22HB_OPEN_DRAIN;
      break;
    default:
      *val = LPS22HB_PUSH_PULL;
      break;
  }
  return ret;
}

/**
  * @brief  Interrupt active-high/low.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of int_h_l in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_polarity_set(lps22hb_ctx_t *ctx, lps22hb_int_h_l_t val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  if(ret == 0){
    ctrl_reg3.int_h_l = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  }
  return ret;
}

/**
  * @brief  Interrupt active-high/low.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of int_h_l in reg CTRL_REG3
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_polarity_get(lps22hb_ctx_t *ctx, lps22hb_int_h_l_t *val)
{
  lps22hb_ctrl_reg3_t ctrl_reg3;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG3, (uint8_t*)&ctrl_reg3, 1);
  switch (ctrl_reg3.int_h_l){
    case LPS22HB_ACTIVE_HIGH:
      *val = LPS22HB_ACTIVE_HIGH;
      break;
    case LPS22HB_ACTIVE_LOW:
      *val = LPS22HB_ACTIVE_LOW;
      break;
    default:
      *val = LPS22HB_ACTIVE_HIGH;
      break;
  }
  return ret;
}

/**
  * @brief  Interrupt source register[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Register INT_SOURCE
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_source_get(lps22hb_ctx_t *ctx, lps22hb_int_source_t *val)
{
  int32_t ret;
  ret =  lps22hb_read_reg(ctx, LPS22HB_INT_SOURCE, (uint8_t*) val, 1);
  return ret;
}

/**
  * @brief  Differential pressure high interrupt flag.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ph in reg INT_SOURCE
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_on_press_high_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_int_source_t int_source;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INT_SOURCE, (uint8_t*)&int_source, 1);
  *val = int_source.ph;

  return ret;
}

/**
  * @brief  Differential pressure low interrupt flag.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of pl in reg INT_SOURCE
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_int_on_press_low_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_int_source_t int_source;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INT_SOURCE, (uint8_t*)&int_source, 1);
  *val = int_source.pl;

  return ret;
}

/**
  * @brief  Interrupt active flag.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ia in reg INT_SOURCE
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_interrupt_event_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_int_source_t int_source;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_INT_SOURCE, (uint8_t*)&int_source, 1);
  *val = int_source.ia;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LPS22HB_fifo
  * @brief       This section group all the functions concerning the
  *              fifo usage
  * @{
  *
  */

/**
  * @brief   Stop on FIFO watermark. Enable FIFO watermark level use.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of stop_on_fth in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_stop_on_fifo_threshold_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0){
    ctrl_reg2.stop_on_fth = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief   Stop on FIFO watermark. Enable FIFO watermark level use.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of stop_on_fth in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_stop_on_fifo_threshold_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  *val = ctrl_reg2.stop_on_fth;

  return ret;
}

/**
  * @brief  FIFO enable.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fifo_en in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0){
    ctrl_reg2.fifo_en = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  FIFO enable.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fifo_en in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  *val = ctrl_reg2.fifo_en;

  return ret;
}

/**
  * @brief  FIFO watermark level selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of wtm in reg FIFO_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_watermark_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  if(ret == 0){
    fifo_ctrl.wtm = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }
  return ret;
}

/**
  * @brief  FIFO watermark level selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of wtm in reg FIFO_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_watermark_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  *val = fifo_ctrl.wtm;

  return ret;
}

/**
  * @brief  FIFO mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of f_mode in reg FIFO_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_mode_set(lps22hb_ctx_t *ctx, lps22hb_f_mode_t val)
{
  lps22hb_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  if(ret == 0){
    fifo_ctrl.f_mode = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  }
  return ret;
}

/**
  * @brief  FIFO mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of f_mode in reg FIFO_CTRL
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_mode_get(lps22hb_ctx_t *ctx, lps22hb_f_mode_t *val)
{
  lps22hb_fifo_ctrl_t fifo_ctrl;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_FIFO_CTRL, (uint8_t*)&fifo_ctrl, 1);
  switch (fifo_ctrl.f_mode){
    case LPS22HB_BYPASS_MODE:
      *val = LPS22HB_BYPASS_MODE;
      break;
    case LPS22HB_FIFO_MODE:
      *val = LPS22HB_FIFO_MODE;
      break;
    case LPS22HB_STREAM_MODE:
      *val = LPS22HB_STREAM_MODE;
      break;
    case LPS22HB_STREAM_TO_FIFO_MODE:
      *val = LPS22HB_STREAM_TO_FIFO_MODE;
      break;
    case LPS22HB_BYPASS_TO_STREAM_MODE:
      *val = LPS22HB_BYPASS_TO_STREAM_MODE;
      break;
    case LPS22HB_DYNAMIC_STREAM_MODE:
      *val = LPS22HB_DYNAMIC_STREAM_MODE;
      break;
    case LPS22HB_BYPASS_TO_FIFO_MODE:
      *val = LPS22HB_BYPASS_TO_FIFO_MODE;
      break;
    default:
      *val = LPS22HB_BYPASS_MODE;
      break;
  }
  return ret;
}

/**
  * @brief  FIFO stored data level.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fss in reg FIFO_STATUS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_data_level_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_fifo_status_t fifo_status;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_FIFO_STATUS, (uint8_t*)&fifo_status, 1);
  *val = fifo_status.fss;

  return ret;
}

/**
  * @brief  FIFO overrun status.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of ovr in reg FIFO_STATUS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_ovr_flag_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_fifo_status_t fifo_status;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_FIFO_STATUS, (uint8_t*)&fifo_status, 1);
  *val = fifo_status.ovr;

  return ret;
}

/**
  * @brief  FIFO watermark status.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of fth_fifo in reg FIFO_STATUS
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_fifo_fth_flag_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_fifo_status_t fifo_status;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_FIFO_STATUS, (uint8_t*)&fifo_status, 1);
  *val = fifo_status.fth_fifo;

  return ret;
}

/**
  * @}
  *
  */

/**
  * @defgroup    LPS22HB_serial_interface
  * @brief       This section group all the functions concerning serial
  *              interface management
  * @{
  *
  */

/**
  * @brief  SPI Serial Interface Mode selection.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of sim in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_spi_mode_set(lps22hb_ctx_t *ctx, lps22hb_sim_t val)
{
  lps22hb_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  if(ret == 0){
    ctrl_reg1.sim = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  }
  return ret;
}

/**
  * @brief  SPI Serial Interface Mode selection.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of sim in reg CTRL_REG1
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_spi_mode_get(lps22hb_ctx_t *ctx, lps22hb_sim_t *val)
{
  lps22hb_ctrl_reg1_t ctrl_reg1;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG1, (uint8_t*)&ctrl_reg1, 1);
  switch (ctrl_reg1.sim){
    case LPS22HB_SPI_4_WIRE:
      *val = LPS22HB_SPI_4_WIRE;
      break;
    case LPS22HB_SPI_3_WIRE:
      *val = LPS22HB_SPI_3_WIRE;
      break;
    default:
      *val = LPS22HB_SPI_4_WIRE;
      break;
  }
  return ret;
}

/**
  * @brief  Disable I2C interface.[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of i2c_dis in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_i2c_interface_set(lps22hb_ctx_t *ctx, lps22hb_i2c_dis_t val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0){
    ctrl_reg2.i2c_dis = (uint8_t)val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Disable I2C interface.[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Get the values of i2c_dis in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_i2c_interface_get(lps22hb_ctx_t *ctx, lps22hb_i2c_dis_t *val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  switch (ctrl_reg2.i2c_dis){
    case LPS22HB_I2C_ENABLE:
      *val = LPS22HB_I2C_ENABLE;
      break;
    case LPS22HB_I2C_DISABLE:
      *val = LPS22HB_I2C_DISABLE;
      break;
    default:
      *val = LPS22HB_I2C_ENABLE;
      break;
  }
  return ret;
}

/**
  * @brief  Register address automatically incremented during a
  *         multiple byte access with a serial interface (I2C or SPI).[set]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of if_add_inc in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_auto_add_inc_set(lps22hb_ctx_t *ctx, uint8_t val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  if(ret == 0){
    ctrl_reg2.if_add_inc = val;
    ret = lps22hb_write_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  }
  return ret;
}

/**
  * @brief  Register address automatically incremented during a
  *         multiple byte access with a serial interface (I2C or SPI).[get]
  *
  * @param  ctx    Read / write interface definitions
  * @param  val    Change the values of if_add_inc in reg CTRL_REG2
  * @retval        Interface status (MANDATORY: return 0 -> no Error).
  *
  */
int32_t lps22hb_auto_add_inc_get(lps22hb_ctx_t *ctx, uint8_t *val)
{
  lps22hb_ctrl_reg2_t ctrl_reg2;
  int32_t ret;

  ret = lps22hb_read_reg(ctx, LPS22HB_CTRL_REG2, (uint8_t*)&ctrl_reg2, 1);
  *val = ctrl_reg2.if_add_inc;

  return ret;
}

/**
  * @}
  *
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/