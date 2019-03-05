/*
 ******************************************************************************
 * @file    lps25hb_reg.c
 * @author  MEMS Software Solution Team
 * @date    20-September-2017
 * @brief   LPS25HB driver file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "lps25hb_reg.h"

/**
  * @addtogroup  lps25hb
  * @brief  This file provides a set of functions needed to drive the
  *         lps25hb enanced inertial module.
  * @{
  */

/**
  * @addtogroup  interfaces_functions
  * @brief  This section provide a set of functions used to read and write
  *         a generic register of the device.
  * @{
  */

/**
  * @brief  Read generic device register
  *
  * @param  lps25hb_ctx_t* ctx: read / write interface definitions
  * @param  uint8_t reg: register to read
  * @param  uint8_t* data: pointer to buffer that store the data read
  * @param  uint16_t len: number of consecutive register to read
  *
  */
int32_t lps25hb_read_reg(lps25hb_ctx_t* ctx, uint8_t reg, uint8_t* data,
                         uint16_t len)
{
  return ctx->read_reg(ctx->handle, reg, data, len);
}

/**
  * @brief  Write generic device register
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t reg: register to write
  * @param  uint8_t* data: pointer to data to write in register reg
  * @param  uint16_t len: number of consecutive register to write
  *
*/
int32_t lps25hb_write_reg(lps25hb_ctx_t* ctx, uint8_t reg, uint8_t* data,
                          uint16_t len)
{
  return ctx->write_reg(ctx->handle, reg, data, len);
}

/**
  * @}
  */

/**
  * @addtogroup  data_generation_c
  * @brief   This section group all the functions concerning data generation
  * @{
  */

/**
  * @brief  pressure_ref: [set]  The Reference pressure value is a 24-bit
  *                              data expressed as 2’s complement. The value
  *                              is used when AUTOZERO or AUTORIFP function
  *                              is enabled.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that contains data to write
  *
  */
int32_t lps25hb_pressure_ref_set(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_REF_P_XL,  buff, 3);
}

/**
  * @brief  pressure_ref: [get]  The Reference pressure value is a 24-bit
  *                              data expressed as 2’s complement. The value
  *                              is used when AUTOZERO or AUTORIFP function
  *                              is enabled.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lps25hb_pressure_ref_get(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_REF_P_XL,  buff, 3);
}

/**
  * @brief  pressure_avg: [set]  Pressure internal average configuration.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_avgp_t: change the values of avgp in reg RES_CONF
  *
  */
int32_t lps25hb_pressure_avg_set(lps25hb_ctx_t *ctx, lps25hb_avgp_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_RES_CONF, &reg.byte, 1);
  reg.res_conf.avgp = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_RES_CONF, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pressure_avg: [get]  Pressure internal average configuration.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_avgp_t: Get the values of avgp in reg RES_CONF
  *
  */
int32_t lps25hb_pressure_avg_get(lps25hb_ctx_t *ctx, lps25hb_avgp_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_RES_CONF, &reg.byte, 1);
  *val = (lps25hb_avgp_t) reg.res_conf.avgp;

  return mm_error;
}

/**
  * @brief  temperature_avg: [set]  Temperature internal average configuration.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_avgt_t: change the values of avgt in reg RES_CONF
  *
  */
int32_t lps25hb_temperature_avg_set(lps25hb_ctx_t *ctx, lps25hb_avgt_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_RES_CONF, &reg.byte, 1);
  reg.res_conf.avgt = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_RES_CONF, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  temperature_avg: [get]  Temperature internal average configuration.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_avgt_t: Get the values of avgt in reg RES_CONF
  *
  */
int32_t lps25hb_temperature_avg_get(lps25hb_ctx_t *ctx, lps25hb_avgt_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_RES_CONF, &reg.byte, 1);
  *val = (lps25hb_avgt_t) reg.res_conf.avgt;

  return mm_error;
}

/**
  * @brief  autozero_rst: [set]  Reset Autozero function.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of reset_az in reg CTRL_REG1
  *
  */
int32_t lps25hb_autozero_rst_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.reset_az = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  autozero_rst: [get]  Reset Autozero function.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of reset_az in reg CTRL_REG1
  *
  */
int32_t lps25hb_autozero_rst_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  *val = reg.ctrl_reg1.reset_az;

  return mm_error;
}

/**
  * @brief  block_data_update: [set] Blockdataupdate.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of bdu in reg CTRL_REG1
  *
  */
int32_t lps25hb_block_data_update_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.bdu = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  block_data_update: [get] Blockdataupdate.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of bdu in reg CTRL_REG1
  *
  */
int32_t lps25hb_block_data_update_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  *val = reg.ctrl_reg1.bdu;

  return mm_error;
}

/**
  * @brief  data_rate: [set]  Output data rate selection.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_odr_t: change the values of odr in reg CTRL_REG1
  *
  */
int32_t lps25hb_data_rate_set(lps25hb_ctx_t *ctx, lps25hb_odr_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.odr = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  data_rate: [get]  Output data rate selection.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_odr_t: Get the values of odr in reg CTRL_REG1
  *
  */
int32_t lps25hb_data_rate_get(lps25hb_ctx_t *ctx, lps25hb_odr_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  *val = (lps25hb_odr_t) reg.ctrl_reg1.odr;

  return mm_error;
}

/**
  * @brief  one_shoot_trigger: [set]  One-shot mode. Device perform a
  *                                   single measure.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of one_shot in reg CTRL_REG2
  *
  */
int32_t lps25hb_one_shoot_trigger_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  reg.ctrl_reg2.one_shot = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  one_shoot_trigger: [get]  One-shot mode. Device perform a
  *                                   single measure.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of one_shot in reg CTRL_REG2
  *
  */
int32_t lps25hb_one_shoot_trigger_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  *val = reg.ctrl_reg2.one_shot;

  return mm_error;
}

/**
  * @brief  autozero: [set]  Enable Autozero function.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of autozero in reg CTRL_REG2
  *
  */
int32_t lps25hb_autozero_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  reg.ctrl_reg2.autozero = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  autozero: [get]  Enable Autozero function.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of autozero in reg CTRL_REG2
  *
  */
int32_t lps25hb_autozero_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  *val = reg.ctrl_reg2.autozero;

  return mm_error;
}

/**
  * @brief   fifo_mean_decimator: [set]  Enable to decimate the output
  *                                      pressure to 1Hz with FIFO Mean mode.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of fifo_mean_dec in reg CTRL_REG2
  *
  */
int32_t lps25hb_fifo_mean_decimator_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  reg.ctrl_reg2.fifo_mean_dec = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   fifo_mean_decimator: [get]  Enable to decimate the output
  *                                      pressure to 1Hz with FIFO Mean mode.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fifo_mean_dec in reg CTRL_REG2
  *
  */
int32_t lps25hb_fifo_mean_decimator_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  *val = reg.ctrl_reg2.fifo_mean_dec;

  return mm_error;
}

/**
  * @brief  press_data_ready: [get]  Pressure data available.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of p_da in reg STATUS_REG
  *
  */
int32_t lps25hb_press_data_ready_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_STATUS_REG, &reg.byte, 1);
  *val = reg.status_reg.p_da;

  return mm_error;
}

/**
  * @brief  temp_data_ready: [get]  Temperature data available.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of t_da in reg STATUS_REG
  *
  */
int32_t lps25hb_temp_data_ready_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_STATUS_REG, &reg.byte, 1);
  *val = reg.status_reg.t_da;

  return mm_error;
}

/**
  * @brief  temp_data_ovr: [get]   Temperature data overrun.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of t_or in reg STATUS_REG
  *
  */
int32_t lps25hb_temp_data_ovr_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_STATUS_REG, &reg.byte, 1);
  *val = reg.status_reg.t_or;

  return mm_error;
}

/**
  * @brief  press_data_ovr: [get]   Pressure data overrun.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of p_or in reg STATUS_REG
  *
  */
int32_t lps25hb_press_data_ovr_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_STATUS_REG, &reg.byte, 1);
  *val = reg.status_reg.p_or;

  return mm_error;
}

/**
  * @brief  pressure_raw: [get]  Pressure output value.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lps25hb_pressure_raw_get(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_PRESS_OUT_XL,  buff, 3);
}

/**
  * @brief  temperature_raw: [get]  Temperature output value.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lps25hb_temperature_raw_get(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_TEMP_OUT_L,  buff, 2);
}

/**
  * @brief  pressure_offset: [set]  The pressure offset value is 16-bit
  *                                 data that can be used to implement
  *                                 one-point calibration (OPC)
  *                                 after soldering.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that contains data to write
  *
  */
int32_t lps25hb_pressure_offset_set(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_RPDS_L,  buff, 2);
}

/**
  * @brief  pressure_offset: [get]  The pressure offset value is 16-bit
  *                                 data that can be used to implement
  *                                 one-point calibration (OPC) after
  *                                 soldering.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lps25hb_pressure_offset_get(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_RPDS_L,  buff, 2);
}

/**
  * @}
  */

/**
  * @addtogroup  common
  * @brief   This section group common usefull functions
  * @{
  */

/**
  * @brief  device_id: [get] DeviceWhoamI.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lps25hb_device_id_get(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_WHO_AM_I,  buff, 1);
}

/**
  * @brief  reset: [set]  Software reset. Restore the default values
  *                       in user registers
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of swreset in reg CTRL_REG2
  *
  */
int32_t lps25hb_reset_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  reg.ctrl_reg2.swreset = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  reset: [get]  Software reset. Restore the default values
  *                       in user registers
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of swreset in reg CTRL_REG2
  *
  */
int32_t lps25hb_reset_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  *val = reg.ctrl_reg2.swreset;

  return mm_error;
}

/**
  * @brief  boot: [set]  Reboot memory content. Reload the calibration
  *                      parameters
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of boot in reg CTRL_REG2
  *
  */
int32_t lps25hb_boot_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  reg.ctrl_reg2.boot = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  boot: [get]  Reboot memory content. Reload the calibration
  *                      parameters
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of boot in reg CTRL_REG2
  *
  */
int32_t lps25hb_boot_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  *val = reg.ctrl_reg2.boot;

  return mm_error;
}

/**
  * @brief  status: [get]
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_: registers STATUS_REG.
  *
  */
int32_t lps25hb_status_get(lps25hb_ctx_t *ctx, lps25hb_status_reg_t *val)
{
  return lps25hb_read_reg(ctx, LPS25HB_STATUS_REG, (uint8_t*) val, 1);
}

/**
  * @}
  */

/**
  * @addtogroup  interrupts
  * @brief   This section group all the functions that manage interrupts
  * @{
  */

/**
  * @brief  int_generation: [set]  Enable interrupt generation.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of diff_en in reg CTRL_REG1
  *
  */
int32_t lps25hb_int_generation_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.diff_en = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  int_generation: [get]  Enable interrupt generation.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of diff_en in reg CTRL_REG1
  *
  */
int32_t lps25hb_int_generation_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  *val = reg.ctrl_reg1.diff_en;

  return mm_error;
}

/**
  * @brief  int_pin_mode: [set]  Data signal on INT_DRDY pin control bits.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_int_s_t: change the values of int_s in reg CTRL_REG3
  *
  */
int32_t lps25hb_int_pin_mode_set(lps25hb_ctx_t *ctx, lps25hb_int_s_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);
  reg.ctrl_reg3.int_s = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  int_pin_mode: [get]  Data signal on INT_DRDY pin control bits.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_int_s_t: Get the values of int_s in reg CTRL_REG3
  *
  */
int32_t lps25hb_int_pin_mode_get(lps25hb_ctx_t *ctx, lps25hb_int_s_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);
  *val = (lps25hb_int_s_t) reg.ctrl_reg3.int_s;

  return mm_error;
}

/**
  * @brief  pin_mode: [set]  Push-pull/open drain selection on interrupt pads.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_pp_od_t: change the values of pp_od in reg CTRL_REG3
  *
  */
int32_t lps25hb_pin_mode_set(lps25hb_ctx_t *ctx, lps25hb_pp_od_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);
  reg.ctrl_reg3.pp_od = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_mode: [get]  Push-pull/open drain selection on interrupt pads.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_pp_od_t: Get the values of pp_od in reg CTRL_REG3
  *
  */
int32_t lps25hb_pin_mode_get(lps25hb_ctx_t *ctx, lps25hb_pp_od_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);
  *val = (lps25hb_pp_od_t) reg.ctrl_reg3.pp_od;

  return mm_error;
}

/**
  * @brief  int_polarity: [set]  Interrupt active-high/low.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_int_h_l_t: change the values of int_h_l in reg CTRL_REG3
  *
  */
int32_t lps25hb_int_polarity_set(lps25hb_ctx_t *ctx, lps25hb_int_h_l_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);
  reg.ctrl_reg3.int_h_l = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  int_polarity: [get]  Interrupt active-high/low.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_int_h_l_t: Get the values of int_h_l in reg CTRL_REG3
  *
  */
int32_t lps25hb_int_polarity_get(lps25hb_ctx_t *ctx, lps25hb_int_h_l_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG3, &reg.byte, 1);
  *val = (lps25hb_int_h_l_t) reg.ctrl_reg3.int_h_l;

  return mm_error;
}

/**
  * @brief  drdy_on_int: [set]  Data-ready signal on INT_DRDY pin.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of drdy in reg CTRL_REG4
  *
  */
int32_t lps25hb_drdy_on_int_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.drdy = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  drdy_on_int: [get]  Data-ready signal on INT_DRDY pin.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of drdy in reg CTRL_REG4
  *
  */
int32_t lps25hb_drdy_on_int_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);
  *val = reg.ctrl_reg4.drdy;

  return mm_error;
}

/**
  * @brief  fifo_ovr_on_int: [set]  FIFO overrun interrupt on INT_DRDY pin.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of f_ovr in reg CTRL_REG4
  *
  */
int32_t lps25hb_fifo_ovr_on_int_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.f_ovr = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_ovr_on_int: [get]  FIFO overrun interrupt on INT_DRDY pin.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of f_ovr in reg CTRL_REG4
  *
  */
int32_t lps25hb_fifo_ovr_on_int_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);
  *val = reg.ctrl_reg4.f_ovr;

  return mm_error;
}

/**
  * @brief   fifo_threshold_on_int: [set]  FIFO watermark status
  *                                        on INT_DRDY pin.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of f_fth in reg CTRL_REG4
  *
  */
int32_t lps25hb_fifo_threshold_on_int_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.f_fth = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   fifo_threshold_on_int: [get]  FIFO watermark status
  *                                        on INT_DRDY pin.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of f_fth in reg CTRL_REG4
  *
  */
int32_t lps25hb_fifo_threshold_on_int_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);
  *val = reg.ctrl_reg4.f_fth;

  return mm_error;
}

/**
  * @brief  fifo_empty_on_int: [set]  FIFO empty flag on INT_DRDY pin.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of f_empty in reg CTRL_REG4
  *
  */
int32_t lps25hb_fifo_empty_on_int_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.f_empty = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_empty_on_int: [get]  FIFO empty flag on INT_DRDY pin.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of f_empty in reg CTRL_REG4
  *
  */
int32_t lps25hb_fifo_empty_on_int_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG4, &reg.byte, 1);
  *val = reg.ctrl_reg4.f_empty;

  return mm_error;
}

/**
  * @brief   sign_of_int_threshold: [set]  Enable interrupt generation on
  *                                        pressure low/high event.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_pe_t: change the values of pe in reg INTERRUPT_CFG
  *
  */
int32_t lps25hb_sign_of_int_threshold_set(lps25hb_ctx_t *ctx,
                                          lps25hb_pe_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_INTERRUPT_CFG, &reg.byte, 1);
  reg.interrupt_cfg.pe = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_INTERRUPT_CFG, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   sign_of_int_threshold: [get]  Enable interrupt generation on
  *                                        pressure low/high event.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_pe_t: Get the values of pe in reg INTERRUPT_CFG
  *
  */
int32_t lps25hb_sign_of_int_threshold_get(lps25hb_ctx_t *ctx,
                                          lps25hb_pe_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_INTERRUPT_CFG, &reg.byte, 1);
  *val = (lps25hb_pe_t) reg.interrupt_cfg.pe;

  return mm_error;
}

/**
  * @brief   int_notification_mode: [set]  Interrupt request to the
  *                                        INT_SOURCE (25h) register
  *                                        mode (pulsed / latched)
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_lir_t: change the values of lir in reg INTERRUPT_CFG
  *
  */
int32_t lps25hb_int_notification_mode_set(lps25hb_ctx_t *ctx,
                                          lps25hb_lir_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_INTERRUPT_CFG, &reg.byte, 1);
  reg.interrupt_cfg.lir = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_INTERRUPT_CFG, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   int_notification_mode: [get]  Interrupt request to the
  *                                        INT_SOURCE (25h) register mode
  *                                        (pulsed / latched)
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_lir_t: Get the values of lir in reg INTERRUPT_CFG
  *
  */
int32_t lps25hb_int_notification_mode_get(lps25hb_ctx_t *ctx,
                                          lps25hb_lir_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_INTERRUPT_CFG, &reg.byte, 1);
  *val = (lps25hb_lir_t) reg.interrupt_cfg.lir;

  return mm_error;
}

/**
  * @brief  int_source: [get]  Interrupt source register
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_int_source_t: registers INT_SOURCE
  *
  */
int32_t lps25hb_int_source_get(lps25hb_ctx_t *ctx, lps25hb_int_source_t *val)
{
  return lps25hb_read_reg(ctx, LPS25HB_INT_SOURCE, (uint8_t*) val, 1);
}

/**
  * @brief  int_on_press_high: [get]  Differential pressure high
  *                                   interrupt flag.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of ph in reg INT_SOURCE
  *
  */
int32_t lps25hb_int_on_press_high_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_INT_SOURCE, &reg.byte, 1);
  *val = reg.int_source.ph;

  return mm_error;
}

/**
  * @brief  int_on_press_low: [get]  Differential pressure low
  *                                  interrupt flag.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of pl in reg INT_SOURCE
  *
  */
int32_t lps25hb_int_on_press_low_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_INT_SOURCE, &reg.byte, 1);
  *val = reg.int_source.pl;

  return mm_error;
}

/**
  * @brief  interrupt_event: [get]  Interrupt active flag.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of ia in reg INT_SOURCE
  *
  */
int32_t lps25hb_interrupt_event_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_INT_SOURCE, &reg.byte, 1);
  *val = reg.int_source.ia;

  return mm_error;
}

/**
  * @brief  int_threshold: [set]  User-defined threshold value for
  *                              pressure interrupt event
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that contains data to write
  *
  */
int32_t lps25hb_int_threshold_set(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_THS_P_L,  buff, 2);
}

/**
  * @brief  int_threshold: [get]  User-defined threshold value for
  *                              pressure interrupt event
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lps25hb_int_threshold_get(lps25hb_ctx_t *ctx, uint8_t *buff)
{
  return lps25hb_read_reg(ctx, LPS25HB_THS_P_L,  buff, 2);
}

/**
  * @}
  */

/**
  * @addtogroup  fifo
  * @brief   This section group all the functions concerning the fifo usage
  * @{
  */

/**
  * @brief   stop_on_fifo_threshold: [set]  Stop on FIFO watermark.
  *                                         Enable FIFO watermark level use.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of stop_on_fth in reg CTRL_REG2
  *
  */
int32_t lps25hb_stop_on_fifo_threshold_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  reg.ctrl_reg2.stop_on_fth = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   stop_on_fifo_threshold: [get]  Stop on FIFO watermark.
  *                                         Enable FIFO watermark
  *                                         level use.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of stop_on_fth in reg CTRL_REG2
  *
  */
int32_t lps25hb_stop_on_fifo_threshold_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  *val = reg.ctrl_reg2.stop_on_fth;

  return mm_error;
}

/**
  * @brief  fifo: [set] FIFOenable.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of fifo_en in reg CTRL_REG2
  *
  */
int32_t lps25hb_fifo_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  reg.ctrl_reg2.fifo_en = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo: [get] FIFOenable.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fifo_en in reg CTRL_REG2
  *
  */
int32_t lps25hb_fifo_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  *val = reg.ctrl_reg2.fifo_en;

  return mm_error;
}

/**
  * @brief  fifo_watermark: [set]  FIFO watermark level selection.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of wtm_point in reg FIFO_CTRL
  *
  */
int32_t lps25hb_fifo_watermark_set(lps25hb_ctx_t *ctx, uint8_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_FIFO_CTRL, &reg.byte, 1);
  reg.fifo_ctrl.wtm_point = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_FIFO_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_watermark: [get]  FIFO watermark level selection.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of wtm_point in reg FIFO_CTRL
  *
  */
int32_t lps25hb_fifo_watermark_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_FIFO_CTRL, &reg.byte, 1);
  *val = reg.fifo_ctrl.wtm_point;

  return mm_error;
}

/**
  * @brief  fifo_mode: [set]  FIFO mode selection.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_f_mode_t: change the values of f_mode in reg FIFO_CTRL
  *
  */
int32_t lps25hb_fifo_mode_set(lps25hb_ctx_t *ctx, lps25hb_f_mode_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_FIFO_CTRL, &reg.byte, 1);
  reg.fifo_ctrl.f_mode = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_FIFO_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_mode: [get]  FIFO mode selection.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_f_mode_t: Get the values of f_mode in reg FIFO_CTRL
  *
  */
int32_t lps25hb_fifo_mode_get(lps25hb_ctx_t *ctx, lps25hb_f_mode_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_FIFO_CTRL, &reg.byte, 1);
  *val = (lps25hb_f_mode_t) reg.fifo_ctrl.f_mode;

  return mm_error;
}

/**
  * @brief  fifo_status: [get]  FIFO status register.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_: registers FIFO_STATUS
  *
  */
int32_t lps25hb_fifo_status_get(lps25hb_ctx_t *ctx, lps25hb_fifo_status_t *val)
{
  return lps25hb_read_reg(ctx, LPS25HB_FIFO_STATUS, (uint8_t*) val, 1);
}

/**
  * @brief  fifo_data_level: [get]  FIFO stored data level.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fss in reg FIFO_STATUS
  *
  */
int32_t lps25hb_fifo_data_level_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_FIFO_STATUS, &reg.byte, 1);
  *val = reg.fifo_status.fss;

  return mm_error;
}

/**
  * @brief  fifo_empty_flag: [get]  Empty FIFO status flag.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of empty_fifo in reg FIFO_STATUS
  *
  */
int32_t lps25hb_fifo_empty_flag_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_FIFO_STATUS, &reg.byte, 1);
  *val = reg.fifo_status.empty_fifo;

  return mm_error;
}

/**
  * @brief  fifo_ovr_flag: [get]  FIFO overrun status flag.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of ovr in reg FIFO_STATUS
  *
  */
int32_t lps25hb_fifo_ovr_flag_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_FIFO_STATUS, &reg.byte, 1);
  *val = reg.fifo_status.ovr;

  return mm_error;
}

/**
  * @brief  fifo_fth_flag: [get]  FIFO watermark status.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fth_fifo in reg FIFO_STATUS
  *
  */
int32_t lps25hb_fifo_fth_flag_get(lps25hb_ctx_t *ctx, uint8_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_FIFO_STATUS, &reg.byte, 1);
  *val = reg.fifo_status.fth_fifo;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  serial_interface
  * @brief   This section group all the functions concerning serial
  *          interface management
  * @{
  */

/**
  * @brief  spi_mode: [set]  SPI Serial Interface Mode selection.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_sim_t: change the values of sim in reg CTRL_REG1
  *
  */
int32_t lps25hb_spi_mode_set(lps25hb_ctx_t *ctx, lps25hb_sim_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.sim = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  spi_mode: [get]  SPI Serial Interface Mode selection.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_sim_t: Get the values of sim in reg CTRL_REG1
  *
  */
int32_t lps25hb_spi_mode_get(lps25hb_ctx_t *ctx, lps25hb_sim_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG1, &reg.byte, 1);
  *val = (lps25hb_sim_t) reg.ctrl_reg1.sim;

  return mm_error;
}

/**
  * @brief  i2c_interface: [set]  Disable I2C interface.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_i2c_dis_t: change the values of i2c_dis in reg CTRL_REG2
  *
  */
int32_t lps25hb_i2c_interface_set(lps25hb_ctx_t *ctx, lps25hb_i2c_dis_t val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  reg.ctrl_reg2.i2c_dis = val;
  mm_error = lps25hb_write_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  i2c_interface: [get]  Disable I2C interface.
  *
  * @param  lps25hb_ctx_t *ctx: read / write interface definitions
  * @param  lps25hb_i2c_dis_t: Get the values of i2c_dis in reg CTRL_REG2
  *
  */
int32_t lps25hb_i2c_interface_get(lps25hb_ctx_t *ctx, lps25hb_i2c_dis_t *val)
{
  lps25hb_reg_t reg;
  int32_t mm_error;

  mm_error = lps25hb_read_reg(ctx, LPS25HB_CTRL_REG2, &reg.byte, 1);
  *val = (lps25hb_i2c_dis_t) reg.ctrl_reg2.i2c_dis;

  return mm_error;
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/