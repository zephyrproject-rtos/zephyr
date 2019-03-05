/*
 ******************************************************************************
 * @file    lsm303ah_reg.c
 * @author  MEMS Software Solution Team
 * @date    19-December-2017
 * @brief   LSM303AH driver file
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

#include "lsm303ah_reg.h"

/**
  * @addtogroup  lsm303ah
  * @brief  This file provides a set of functions needed to drive the
  *         lsm303ah enanced inertial module.
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
  * @param  lsm303ah_ctx_t* ctx: read / write interface definitions
  * @param  uint8_t reg: register to read
  * @param  uint8_t* data: pointer to buffer that store the data read
  * @param  uint16_t len: number of consecutive register to read
  *
  */
int32_t lsm303ah_read_reg(lsm303ah_ctx_t* ctx, uint8_t reg, uint8_t* data,
                          uint16_t len)
{
  return ctx->read_reg(ctx->handle, reg, data, len);
}

/**
  * @brief  Write generic device register
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t reg: register to write
  * @param  uint8_t* data: pointer to data to write in register reg
  * @param  uint16_t len: number of consecutive register to write
  *
*/
int32_t lsm303ah_write_reg(lsm303ah_ctx_t* ctx, uint8_t reg, uint8_t* data,
                           uint16_t len)
{
  return ctx->write_reg(ctx->handle, reg, data, len);
}

/**
  * @}
  */

/**
  * @addtogroup  data_generation_c
  * @brief   This section groups all the functions concerning data generation
  * @{
  */

/**
  * @brief  all_sources: [get] Read all the interrupt/status flag of
  *                            the device.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_all_sources: FIFO_SRC, STATUS_DUP, WAKE_UP_SRC,
  *                               TAP_SRC, 6D_SRC, FUNC_CK_GATE, FUNC_SRC.
  *
  */
int32_t lsm303ah_xl_all_sources_get(lsm303ah_ctx_t *ctx,
                                 lsm303ah_xl_all_sources_t *val)
{
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_SRC_A,
                               &(val->byte[0]), 1);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STATUS_DUP_A,
                               &(val->byte[1]), 4);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CK_GATE_A,
                               &(val->byte[5]), 2);

  return mm_error;
}

/**
  * @brief  block_data_update: [set] Blockdataupdate.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of bdu in reg CTRL1
  *
  */
int32_t lsm303ah_xl_block_data_update_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);
  reg.ctrl1_a.bdu = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  block_data_update: [get] Blockdataupdate.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of bdu in reg CTRL1
  *
  */
int32_t lsm303ah_xl_block_data_update_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);
  *val = reg.ctrl1_a.bdu;

  return mm_error;
}

/**
  * @brief  block_data_update: [set] Blockdataupdate.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of bdu in reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_block_data_update_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  reg.cfg_reg_c_m.bdu = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  block_data_update: [get] Blockdataupdate.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of bdu in reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_block_data_update_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  *val = reg.cfg_reg_c_m.bdu;

  return mm_error;
}

/**
  * @brief  data_format: [set]  Big/Little Endian data selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_ble_t: change the values of ble in reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_data_format_set(lsm303ah_ctx_t *ctx, lsm303ah_mg_ble_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  reg.cfg_reg_c_m.ble = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  data_format: [get]  Big/Little Endian data selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_ble_t: Get the values of ble in reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_data_format_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_mg_ble_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  *val = (lsm303ah_mg_ble_t) reg.cfg_reg_c_m.ble;

  return mm_error;
}

/**
  * @brief  xl_full_scale: [set]   Accelerometer full-scale selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_fs_t: change the values of fs in reg CTRL1
  *
  */
int32_t lsm303ah_xl_full_scale_set(lsm303ah_ctx_t *ctx, lsm303ah_xl_fs_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);
  reg.ctrl1_a.fs = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  xl_full_scale: [get]   Accelerometer full-scale selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_fs_t: Get the values of fs in reg CTRL1
  *
  */
int32_t lsm303ah_xl_full_scale_get(lsm303ah_ctx_t *ctx, lsm303ah_xl_fs_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);
  *val = (lsm303ah_xl_fs_t) reg.ctrl1_a.fs;

  return mm_error;
}

/**
  * @brief  xl_data_rate: [set]  Accelerometer data rate selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_odr_t: change the values of odr in reg CTRL1
  *
  */
int32_t lsm303ah_xl_data_rate_set(lsm303ah_ctx_t *ctx, lsm303ah_xl_odr_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);
  reg.ctrl1_a.odr = val & 0x0F;
  reg.ctrl1_a.hf_odr = (val & 0x10) >> 4;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  xl_data_rate: [get]  Accelerometer data rate selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_odr_t: Get the values of odr in reg CTRL1
  *
  */
int32_t lsm303ah_xl_data_rate_get(lsm303ah_ctx_t *ctx, lsm303ah_xl_odr_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL1_A, &reg.byte, 1);
  *val = (lsm303ah_xl_odr_t) ((reg.ctrl1_a.hf_odr << 4) + reg.ctrl1_a.odr);

  return mm_error;
}

/**
  * @brief  status_reg: [get]  The STATUS_REG register.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_status_reg_t: registers STATUS
  *
  */
int32_t lsm303ah_xl_status_reg_get(lsm303ah_ctx_t *ctx,
                                   lsm303ah_status_a_t *val)
{
  return lsm303ah_read_reg(ctx, LSM303AH_STATUS_A, (uint8_t*) val, 1);
}

/**
  * @brief  status: [get]  Info about device status.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_status_reg_t: registers STATUS_REG
  *
  */
int32_t lsm303ah_mg_status_get(lsm303ah_ctx_t *ctx,
                               lsm303ah_status_reg_m_t *val)
{
  return lsm303ah_read_reg(ctx, LSM303AH_STATUS_REG_M, (uint8_t*) val, 1);
}

/**
  * @brief  xl_flag_data_ready: [get]  Accelerometer new data available.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of drdy in reg STATUS
  *
  */
int32_t lsm303ah_xl_flag_data_ready_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STATUS_A, &reg.byte, 1);
  *val = reg.status_a.drdy;

  return mm_error;
}

/**
  * @brief  mag_data_ready: [get]  Magnetic set of data available.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of zyxda in reg STATUS_REG
  *
  */
int32_t lsm303ah_mg_data_ready_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STATUS_REG_M, &reg.byte, 1);
  *val = reg.status_reg_m.zyxda;

  return mm_error;
}

/**
  * @brief  mag_data_ovr: [get]  Magnetic set of data overrun.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of zyxor in reg STATUS_REG
  *
  */
int32_t lsm303ah_mg_data_ovr_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STATUS_REG_M, &reg.byte, 1);
  *val = reg.status_reg_m.zyxor;

  return mm_error;
}

/**
  * @brief  mag_user_offset: [set]  These registers comprise a 3 group of
  *                                 16-bit number and represent hard-iron
  *                                 offset in order to compensate environmental
  *                                 effects. Data format is the same of
  *                                 output data raw: two’s complement with
  *                                 1LSb = 1.5mG. These values act on the
  *                                 magnetic output data value in order to
  *                                 delete the environmental offset.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that contains data to write
  *
  */
int32_t lsm303ah_mg_user_offset_set(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_write_reg(ctx, LSM303AH_OFFSET_X_REG_L_M, buff, 6);
}

/**
  * @brief  mag_user_offset: [get]  These registers comprise a 3 group of
  *                                 16-bit number and represent hard-iron
  *                                 offset in order to compensate environmental
  *                                 effects. Data format is the same of
  *                                 output data raw: two’s complement with
  *                                 1LSb = 1.5mG. These values act on the
  *                                 magnetic output data value in order to
  *                                 delete the environmental offset.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_mg_user_offset_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_OFFSET_X_REG_L_M, buff, 6);
}

/**
  * @brief  operating_mode: [set]  Operating mode selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_md_t: change the values of md in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_operating_mode_set(lsm303ah_ctx_t *ctx,
                                       lsm303ah_mg_md_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  reg.cfg_reg_a_m.md = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  operating_mode: [get]  Operating mode selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_md_t: Get the values of md in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_operating_mode_get(lsm303ah_ctx_t *ctx,
                                       lsm303ah_mg_md_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  *val = (lsm303ah_mg_md_t) reg.cfg_reg_a_m.md;

  return mm_error;
}

/**
  * @brief  data_rate: [set]  Output data rate selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_odr_t: change the values of odr in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_data_rate_set(lsm303ah_ctx_t *ctx, lsm303ah_mg_odr_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  reg.cfg_reg_a_m.odr = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  data_rate: [get]  Output data rate selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_odr_t: Get the values of odr in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_data_rate_get(lsm303ah_ctx_t *ctx, lsm303ah_mg_odr_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  *val = (lsm303ah_mg_odr_t) reg.cfg_reg_a_m.odr;

  return mm_error;
}

/**
  * @brief  power_mode: [set]  Enables high-resolution/low-power mode.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_lp_t: change the values of lp in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_power_mode_set(lsm303ah_ctx_t *ctx, lsm303ah_mg_lp_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  reg.cfg_reg_a_m.lp = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  power_mode: [get]  Enables high-resolution/low-power mode.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_lp_t: Get the values of lp in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_power_mode_get(lsm303ah_ctx_t *ctx, lsm303ah_mg_lp_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  *val = (lsm303ah_mg_lp_t) reg.cfg_reg_a_m.lp;

  return mm_error;
}

/**
  * @brief  offset_temp_comp: [set]  Enables the magnetometer temperature
  *                                  compensation.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of comp_temp_en in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_offset_temp_comp_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  reg.cfg_reg_a_m.comp_temp_en = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  offset_temp_comp: [get]  Enables the magnetometer temperature
  *                                  compensation.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of comp_temp_en in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_offset_temp_comp_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  *val = reg.cfg_reg_a_m.comp_temp_en;

  return mm_error;
}

/**
  * @brief  set_rst_mode: [set]
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_set_rst_t: change the values of set_rst in
  *                            reg CFG_REG_B
  *
  */
int32_t lsm303ah_mg_set_rst_mode_set(lsm303ah_ctx_t *ctx,
                                     lsm303ah_mg_set_rst_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);
  reg.cfg_reg_b_m.set_rst = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  set_rst_mode: [get]
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_set_rst_t: Get the values of set_rst in reg CFG_REG_B
  *
  */
int32_t lsm303ah_mg_set_rst_mode_get(lsm303ah_ctx_t *ctx,
                                     lsm303ah_mg_set_rst_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);
  *val = (lsm303ah_mg_set_rst_t) reg.cfg_reg_b_m.set_rst;

  return mm_error;
}

/**
  * @brief   set_rst_sensor_single: [set] Enables offset cancellation
  *                                       in single measurement mode.
  *                                       The OFF_CANC bit must be set
  *                                       to 1 when enabling offset
  *                                       cancellation in single measurement
  *                                       mode this means a call function:
  *                                       set_rst_mode(SENS_OFF_CANC_EVERY_ODR)
  *                                       is need.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of off_canc_one_shot in
  *                      reg CFG_REG_B
  *
  */
int32_t lsm303ah_mg_set_rst_sensor_single_set(lsm303ah_ctx_t *ctx,
                                              uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);
  reg.cfg_reg_b_m.off_canc_one_shot = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   set_rst_sensor_single: [get] Enables offset cancellation
  *                                       in single measurement mode.
  *                                       The OFF_CANC bit must be set to
  *                                       1 when enabling offset cancellation
  *                                       in single measurement mode this
  *                                       means a call function:
  *                                       set_rst_mode(SENS_OFF_CANC_EVERY_ODR)
  *                                       is need.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of off_canc_one_shot in reg CFG_REG_B
  *
  */
int32_t lsm303ah_mg_set_rst_sensor_single_get(lsm303ah_ctx_t *ctx,
                                              uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);
  *val = reg.cfg_reg_b_m.off_canc_one_shot;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  Dataoutput
  * @brief   This section groups all the data output functions.
  * @{
  */

/**
  * @brief   acceleration_module_raw: [get]   Module output value (8-bit).
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_acceleration_module_raw_get(lsm303ah_ctx_t *ctx,
                                             uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_MODULE_8BIT_A, buff, 1);
}

/**
  * @brief  temperature_raw: [get] Temperature data output register (r).
  *                                L and H registers together express a 16-bit
  *                                word in two’s complement.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_xl_temperature_raw_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_OUT_T_A, buff, 1);
}

/**
  * @brief  acceleration_raw: [get] Linear acceleration output register.
  *                                 The value is expressed as a 16-bit word
  *                                 in two’s complement.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_acceleration_raw_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_OUT_X_L_A, buff, 6);
}

/**
  * @brief  magnetic_raw: [get]  Magnetic output value.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_magnetic_raw_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_OUTX_L_REG_M, buff, 6);
}

/**
  * @brief  number_of_steps: [get] Number of steps detected by step
  *                                counter routine.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_number_of_steps_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_STEP_COUNTER_L_A, buff, 2);
}

/**
  * @}
  */

/**
  * @addtogroup  common
  * @brief   This section groups common usefull functions.
  * @{
  */

/**
  * @brief  device_id: [get] DeviceWhoamI.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_xl_device_id_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_WHO_AM_I_A, buff, 1);
}

/**
  * @brief  device_id: [get] DeviceWhoamI.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_mg_device_id_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_WHO_AM_I_M, buff, 1);
}

/**
  * @brief  auto_increment: [set] Register address automatically
  *                               incremented during a multiple byte
  *                               access with a serial interface.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of if_add_inc in reg CTRL2
  *
  */
int32_t lsm303ah_xl_auto_increment_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  reg.ctrl2_a.if_add_inc = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  auto_increment: [get] Register address automatically incremented
  *                               during a multiple byte access with a
  *                               serial interface.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of if_add_inc in reg CTRL2
  *
  */
int32_t lsm303ah_xl_auto_increment_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  *val = reg.ctrl2_a.if_add_inc;

  return mm_error;
}

/**
  * @brief  mem_bank: [set] Enable access to the embedded functions/sensor
  *                         hub configuration registers.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_func_cfg_en_t: change the values of func_cfg_en in
  *                                 reg CTRL2
  *
  */
int32_t lsm303ah_xl_mem_bank_set(lsm303ah_ctx_t *ctx,
                                 lsm303ah_xl_func_cfg_en_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  if (val == LSM303AH_XL_ADV_BANK){
    mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
    reg.ctrl2_a.func_cfg_en = val;
    mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  }
  else {
    mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_ADV_A, &reg.byte, 1);
    reg.ctrl2_a.func_cfg_en = val;
    mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL2_ADV_A, &reg.byte, 1);
  }
  return mm_error;
}

/**
  * @brief  reset: [set] Software reset. Restore the default values in
  *                      user registers.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of soft_reset in reg CTRL2
  *
  */
int32_t lsm303ah_xl_reset_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  reg.ctrl2_a.soft_reset = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  reset: [get] Software reset. Restore the default values in
  *                      user registers.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of soft_reset in reg CTRL2
  *
  */
int32_t lsm303ah_xl_reset_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  *val = reg.ctrl2_a.soft_reset;

  return mm_error;
}

/**
  * @brief  reset: [set]  Software reset. Restore the default values in
  *                       user registers.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of soft_rst in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_reset_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  reg.cfg_reg_a_m.soft_rst = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  reset: [get]  Software reset. Restore the default values
  *                       in user registers.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of soft_rst in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_reset_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  *val = reg.cfg_reg_a_m.soft_rst;

  return mm_error;
}

/**
  * @brief  boot: [set] Reboot memory content. Reload the calibration
  *                     parameters.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of boot in reg CTRL2
  *
  */
int32_t lsm303ah_xl_boot_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  reg.ctrl2_a.boot = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  boot: [get] Reboot memory content. Reload the calibration
  *                     parameters.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of boot in reg CTRL2
  *
  */
int32_t lsm303ah_xl_boot_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  *val = reg.ctrl2_a.boot;

  return mm_error;
}

/**
  * @brief  boot: [set]  Reboot memory content. Reload the calibration
  *                      parameters.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of reboot in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_boot_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  reg.cfg_reg_a_m.reboot = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  boot: [get]  Reboot memory content. Reload the
  *                      calibration parameters.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of reboot in reg CFG_REG_A
  *
  */
int32_t lsm303ah_mg_boot_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
  *val = reg.cfg_reg_a_m.reboot;

  return mm_error;
}

/**
  * @brief  xl_self_test: [set]
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_st_t: change the values of st in reg CTRL3
  *
  */
int32_t lsm303ah_xl_self_test_set(lsm303ah_ctx_t *ctx, lsm303ah_xl_st_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  reg.ctrl3_a.st = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  xl_self_test: [get]
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_st_t: Get the values of st in reg CTRL3
  *
  */
int32_t lsm303ah_xl_self_test_get(lsm303ah_ctx_t *ctx, lsm303ah_xl_st_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  *val = (lsm303ah_xl_st_t) reg.ctrl3_a.st;

  return mm_error;
}

/**
  * @brief  self_test: [set] Selftest.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of self_test in reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_self_test_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  reg.cfg_reg_c_m.self_test = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  self_test: [get] Selftest.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of self_test in reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_self_test_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  *val = reg.cfg_reg_c_m.self_test;

  return mm_error;
}

/**
  * @brief  data_ready_mode: [set]
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_drdy_pulsed_t: change the values of drdy_pulsed in
  *                                 reg CTRL5
  *
  */
int32_t lsm303ah_xl_data_ready_mode_set(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_drdy_pulsed_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);
  reg.ctrl5_a.drdy_pulsed = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  data_ready_mode: [get]
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_drdy_pulsed_t: Get the values of drdy_pulsed in
  *                                    reg CTRL5
  *
  */
int32_t lsm303ah_xl_data_ready_mode_get(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_drdy_pulsed_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);
  *val = (lsm303ah_xl_drdy_pulsed_t) reg.ctrl5_a.drdy_pulsed;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  Filters
  * @brief   This section group all the functions concerning the filters
  *          configuration.
  * @{
  */

/**
  * @brief  xl_hp_path: [set] High-pass filter data selection on output
  *                           register and FIFO.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_fds_slope_t: change the values of fds_slope in
  *                                  reg CTRL2
  *
  */
int32_t lsm303ah_xl_hp_path_set(lsm303ah_ctx_t *ctx,
                                lsm303ah_xl_fds_slope_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  reg.ctrl2_a.fds_slope = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  xl_hp_path: [get] High-pass filter data selection on output
  *                           register and FIFO.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_fds_slope_t: Get the values of fds_slope in reg CTRL2
  *
  */
int32_t lsm303ah_xl_hp_path_get(lsm303ah_ctx_t *ctx,
                                lsm303ah_xl_fds_slope_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  *val = (lsm303ah_xl_fds_slope_t) reg.ctrl2_a.fds_slope;

  return mm_error;
}

/**
  * @brief  low_pass_bandwidth: [set]  Low-pass bandwidth selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_lpf_t: change the values of lpf in reg CFG_REG_B
  *
  */
int32_t lsm303ah_mg_low_pass_bandwidth_set(lsm303ah_ctx_t *ctx,
                                       lsm303ah_mg_lpf_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);
  reg.cfg_reg_b_m.lpf = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  low_pass_bandwidth: [get]  Low-pass bandwidth selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_lpf_t: Get the values of lpf in reg CFG_REG_B
  *
  */
int32_t lsm303ah_mg_low_pass_bandwidth_get(lsm303ah_ctx_t *ctx,
                                       lsm303ah_mg_lpf_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);
  *val = (lsm303ah_mg_lpf_t) reg.cfg_reg_b_m.lpf;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup   Auxiliary_interface
  * @brief   This section groups all the functions concerning auxiliary
  *          interface.
  * @{
  */

/**
  * @brief  spi_mode: [set]  SPI Serial Interface Mode selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_sim_t: change the values of sim in reg CTRL2
  *
  */
int32_t lsm303ah_xl_spi_mode_set(lsm303ah_ctx_t *ctx, lsm303ah_xl_sim_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  reg.ctrl2_a.sim = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  spi_mode: [get]  SPI Serial Interface Mode selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_sim_t: Get the values of sim in reg CTRL2
  *
  */
int32_t lsm303ah_xl_spi_mode_get(lsm303ah_ctx_t *ctx, lsm303ah_xl_sim_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  *val = (lsm303ah_xl_sim_t) reg.ctrl2_a.sim;

  return mm_error;
}

/**
  * @brief  i2c_interface: [set]  Disable / Enable I2C interface.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_i2c_disable_t: change the values of i2c_disable
  *                                 in reg CTRL2
  *
  */
int32_t lsm303ah_xl_i2c_interface_set(lsm303ah_ctx_t *ctx,
                                   lsm303ah_xl_i2c_disable_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  reg.ctrl2_a.i2c_disable = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  i2c_interface: [get]  Disable / Enable I2C interface.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_i2c_disable_t: Get the values of i2c_disable in
  *                                 reg CTRL2
  *
  */
int32_t lsm303ah_xl_i2c_interface_get(lsm303ah_ctx_t *ctx,
                                   lsm303ah_xl_i2c_disable_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL2_A, &reg.byte, 1);
  *val = (lsm303ah_xl_i2c_disable_t) reg.ctrl2_a.i2c_disable;

  return mm_error;
}

/**
  * @brief  i2c_interface: [set]  Enable/Disable I2C interface.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_i2c_dis_t: change the values of i2c_dis in
  *                                reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_i2c_interface_set(lsm303ah_ctx_t *ctx,
                                      lsm303ah_mg_i2c_dis_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  reg.cfg_reg_c_m.i2c_dis = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  i2c_interface: [get]  Enable/Disable I2C interface.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_i2c_dis_t: Get the values of i2c_dis in reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_i2c_interface_get(lsm303ah_ctx_t *ctx,
                                      lsm303ah_mg_i2c_dis_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  *val = (lsm303ah_mg_i2c_dis_t) reg.cfg_reg_c_m.i2c_dis;

  return mm_error;
}

/**
  * @brief  cs_mode: [set]  Connect/Disconnects pull-up in if_cs pad.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_if_cs_pu_dis_t: change the values of if_cs_pu_dis
  *                                  in reg FIFO_CTRL
  *
  */
int32_t lsm303ah_xl_cs_mode_set(lsm303ah_ctx_t *ctx,
                             lsm303ah_xl_if_cs_pu_dis_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);
  reg.fifo_ctrl_a.if_cs_pu_dis = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  cs_mode: [get]  Connect/Disconnects pull-up in if_cs pad.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_if_cs_pu_dis_t: Get the values of if_cs_pu_dis in
  *                                  reg FIFO_CTRL
  *
  */
int32_t lsm303ah_xl_cs_mode_get(lsm303ah_ctx_t *ctx,
                             lsm303ah_xl_if_cs_pu_dis_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);
  *val = (lsm303ah_xl_if_cs_pu_dis_t) reg.fifo_ctrl_a.if_cs_pu_dis;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup   main_serial_interface
  * @brief   This section groups all the functions concerning main serial
  *          interface management (not auxiliary)
  * @{
  */

/**
  * @brief  pin_mode: [set]  Push-pull/open-drain selection on interrupt pad.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_pp_od_t: change the values of pp_od in reg CTRL3
  *
  */
int32_t lsm303ah_xl_pin_mode_set(lsm303ah_ctx_t *ctx, lsm303ah_xl_pp_od_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  reg.ctrl3_a.pp_od = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_mode: [get]  Push-pull/open-drain selection on interrupt pad.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_pp_od_t: Get the values of pp_od in reg CTRL3
  *
  */
int32_t lsm303ah_xl_pin_mode_get(lsm303ah_ctx_t *ctx,
                                 lsm303ah_xl_pp_od_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  *val = (lsm303ah_xl_pp_od_t) reg.ctrl3_a.pp_od;

  return mm_error;
}

/**
  * @brief  pin_polarity: [set]  Interrupt active-high/low.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_h_lactive_t: change the values of h_lactive in
  *                                  reg CTRL3
  *
  */
int32_t lsm303ah_xl_pin_polarity_set(lsm303ah_ctx_t *ctx,
                                  lsm303ah_xl_h_lactive_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  reg.ctrl3_a.h_lactive = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_polarity: [get]  Interrupt active-high/low.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_h_lactive_t: Get the values of h_lactive in reg CTRL3
  *
  */
int32_t lsm303ah_xl_pin_polarity_get(lsm303ah_ctx_t *ctx,
                                  lsm303ah_xl_h_lactive_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  *val = (lsm303ah_xl_h_lactive_t) reg.ctrl3_a.h_lactive;

  return mm_error;
}

/**
  * @brief  int_notification: [set]  Latched/pulsed interrupt.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_lir_t: change the values of lir in reg CTRL3
  *
  */
int32_t lsm303ah_xl_int_notification_set(lsm303ah_ctx_t *ctx,
                                      lsm303ah_xl_lir_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  reg.ctrl3_a.lir = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  int_notification: [get]  Latched/pulsed interrupt.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_lir_t: Get the values of lir in reg CTRL3
  *
  */
int32_t lsm303ah_xl_int_notification_get(lsm303ah_ctx_t *ctx,
                                      lsm303ah_xl_lir_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  *val = (lsm303ah_xl_lir_t) reg.ctrl3_a.lir;

  return mm_error;
}

/**
  * @brief  pin_int1_route: [set] Select the signal that need to route
  *                               on int1 pad.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_pin_int1_route_t: union of registers from CTRL4 to
  *
  */
int32_t lsm303ah_xl_pin_int1_route_set(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_pin_int1_route_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL4_A, &reg.byte, 1);
  reg.ctrl4_a.int1_drdy         = val.int1_drdy;
  reg.ctrl4_a.int1_fth          = val.int1_fth;
  reg.ctrl4_a.int1_6d           = val.int1_6d;
  reg.ctrl4_a.int1_tap          = val.int1_tap;
  reg.ctrl4_a.int1_ff           = val.int1_ff;
  reg.ctrl4_a.int1_wu           = val.int1_wu;
  reg.ctrl4_a.int1_s_tap        = val.int1_s_tap;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL4_A, &reg.byte, 1);

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  reg.wake_up_dur_a.int1_fss7   = val.int1_fss7;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_int1_route: [get] Select the signal that need to route on
  *                               int1 pad.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_pin_int1_route_t: union of registers from CTRL4 to
  *
  */
int32_t lsm303ah_xl_pin_int1_route_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_pin_int1_route_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL4_A, &reg.byte, 1);
  val->int1_drdy          = reg.ctrl4_a.int1_drdy;
  val->int1_fth           = reg.ctrl4_a.int1_fth;
  val->int1_6d            = reg.ctrl4_a.int1_6d;
  val->int1_tap           = reg.ctrl4_a.int1_tap;
  val->int1_ff            = reg.ctrl4_a.int1_ff;
  val->int1_wu            = reg.ctrl4_a.int1_wu;
  val->int1_s_tap         = reg.ctrl4_a.int1_s_tap;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  val->int1_fss7 = reg.wake_up_dur_a.int1_fss7;

  return mm_error;
}

/**
  * @brief  pin_int2_route: [set] Select the signal that need to route on
  *                               int2 pad.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_pin_int2_route_t: union of registers from CTRL5 to
  *
  */
int32_t lsm303ah_xl_pin_int2_route_set(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_pin_int2_route_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);
  reg.ctrl5_a.int2_boot       = val.int2_boot;
  reg.ctrl5_a.int2_tilt       = val.int2_tilt;
  reg.ctrl5_a.int2_sig_mot    = val.int2_sig_mot;
  reg.ctrl5_a.int2_step       = val.int2_step;
  reg.ctrl5_a.int2_fth        = val.int2_fth;
  reg.ctrl5_a.int2_drdy       = val.int2_drdy;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_int2_route: [get] Select the signal that need to route on
  *                               int2 pad.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_pin_int2_route_t: union of registers from CTRL5 to
  *
  */
int32_t lsm303ah_xl_pin_int2_route_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_xl_pin_int2_route_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);
  val->int2_boot     = reg.ctrl5_a.int2_boot;
  val->int2_tilt     = reg.ctrl5_a.int2_tilt;
  val->int2_sig_mot  = reg.ctrl5_a.int2_sig_mot;
  val->int2_step     = reg.ctrl5_a.int2_step;
  val->int2_fth      = reg.ctrl5_a.int2_fth;
  val->int2_drdy     = reg.ctrl5_a.int2_drdy;

  return mm_error;
}

/**
  * @brief  all_on_int1: [set] All interrupt signals become available on
  *                            INT1 pin.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int2_on_int1 in reg CTRL5
  *
  */
int32_t lsm303ah_xl_all_on_int1_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);
  reg.ctrl5_a.int2_on_int1 = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  all_on_int1: [get] All interrupt signals become available on
  *                            INT1 pin.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int2_on_int1 in reg CTRL5
  *
  */
int32_t lsm303ah_xl_all_on_int1_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL5_A, &reg.byte, 1);
  *val = reg.ctrl5_a.int2_on_int1;

  return mm_error;
}

/**
  * @brief  drdy_on_pin: [set]  Data-ready signal on INT_DRDY pin.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of drdy_on_pin in reg CFG_REG_C
  *
  */
int32_t lsm303ah_mg_drdy_on_pin_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  reg.cfg_reg_c_m.int_mag = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  drdy_on_pin: [get]  Data-ready signal on INT_DRDY pin.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of drdy_on_pin in reg CFG_REG_C_M
  *
  */
int32_t lsm303ah_mg_drdy_on_pin_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  *val = reg.cfg_reg_c_m.int_mag;

  return mm_error;
}

/**
  * @brief  int_on_pin: [set]  Interrupt signal on INT_DRDY pin.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int_on_pin in reg CFG_REG_C_M
  *
  */
int32_t lsm303ah_mg_int_on_pin_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  reg.cfg_reg_c_m.int_mag_pin = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  int_on_pin: [get]  Interrupt signal on INT_DRDY pin.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int_on_pin in reg CFG_REG_C_M
  *
  */
int32_t lsm303ah_mg_int_on_pin_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_C_M, &reg.byte, 1);
  *val = reg.cfg_reg_c_m.int_mag_pin;

  return mm_error;
}

/**
  * @brief  int_gen_conf: [set]  Interrupt generator configuration register
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_int_crtl_reg_m_t: registers INT_CRTL_REG
  *
  */
int32_t lsm303ah_mg_int_gen_conf_set(lsm303ah_ctx_t *ctx,
                                 lsm303ah_int_crtl_reg_m_t *val)
{
  return lsm303ah_write_reg(ctx, LSM303AH_INT_CRTL_REG_M, (uint8_t*) val, 1);
}

/**
  * @brief  int_gen_conf: [get]  Interrupt generator configuration register
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_int_crtl_reg_m_t: registers INT_CRTL_REG
  *
  */
int32_t lsm303ah_mg_int_gen_conf_get(lsm303ah_ctx_t *ctx,
                                 lsm303ah_int_crtl_reg_m_t *val)
{
  return lsm303ah_read_reg(ctx, LSM303AH_INT_CRTL_REG_M, (uint8_t*) val, 1);
}

/**
  * @brief  int_gen_source: [get]  Interrupt generator source register
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_int_source_reg_m_t: registers INT_SOURCE_REG
  *
  */
int32_t lsm303ah_mg_int_gen_source_get(lsm303ah_ctx_t *ctx,
                                   lsm303ah_int_source_reg_m_t *val)
{
  return lsm303ah_read_reg(ctx, LSM303AH_INT_SOURCE_REG_M, (uint8_t*) val, 1);
}

/**
  * @brief  int_gen_treshold: [set]  User-defined threshold value for xl
  *                                  interrupt event on generator.
  *                                  Data format is the same of output
  *                                  data raw: two’s complement with
  *                                  1LSb = 1.5mG.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that contains data to write
  *
  */
int32_t lsm303ah_mg_int_gen_treshold_set(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_write_reg(ctx, LSM303AH_INT_THS_L_REG_M, buff, 2);
}

/**
  * @brief  int_gen_treshold: [get]  User-defined threshold value for
  *                                  xl interrupt event on generator.
  *                                  Data format is the same of output
  *                                  data raw: two’s complement with
  *                                  1LSb = 1.5mG.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_mg_int_gen_treshold_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  return lsm303ah_read_reg(ctx, LSM303AH_INT_THS_L_REG_M, buff, 2);
}

/**
  * @}
  */

/**
  * @addtogroup  interrupt_pins
  * @brief   This section groups all the functions that manage interrup pins
  * @{
  */



/**
  * @}
  */

/**
  * @addtogroup  Wake_Up_event
  * @brief   This section groups all the functions that manage the Wake Up
  *          event generation.
  * @{
  */

/**
  * @brief  offset_int_conf: [set]  The interrupt block recognition checks
  *                                 data after/before the hard-iron correction
  *                                 to discover the interrupt.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_int_on_dataoff_t: change the values of int_on_dataoff
  *                                       in reg CFG_REG_B
  *
  */
int32_t lsm303ah_mg_offset_int_conf_set(lsm303ah_ctx_t *ctx,
                                    lsm303ah_mg_int_on_dataoff_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);
  reg.cfg_reg_b_m.int_on_dataoff = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  offset_int_conf: [get]  The interrupt block recognition checks
  *                                 data after/before the hard-iron correction
  *                                 to discover the interrupt.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_mg_int_on_dataoff_t: Get the values of int_on_dataoff in
  *                                   reg CFG_REG_B
  *
  */
int32_t lsm303ah_mg_offset_int_conf_get(lsm303ah_ctx_t *ctx,
                                    lsm303ah_mg_int_on_dataoff_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CFG_REG_B_M, &reg.byte, 1);
  *val = (lsm303ah_mg_int_on_dataoff_t) reg.cfg_reg_b_m.int_on_dataoff;

  return mm_error;
}

  /**
  * @brief  wkup_threshold: [set]  Threshold for wakeup [1 LSb = FS_XL / 64].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of wu_ths in reg WAKE_UP_THS
  *
  */
int32_t lsm303ah_xl_wkup_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  reg.wake_up_ths_a.wu_ths = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  wkup_threshold: [get]  Threshold for wakeup [1 LSb = FS_XL / 64].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of wu_ths in reg WAKE_UP_THS
  *
  */
int32_t lsm303ah_xl_wkup_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  *val = reg.wake_up_ths_a.wu_ths;

  return mm_error;
}

/**
  * @brief  wkup_dur: [set]  Wakeup duration [1 LSb = 1 / ODR].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of wu_dur in reg WAKE_UP_DUR
  *
  */
int32_t lsm303ah_xl_wkup_dur_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  reg.wake_up_dur_a.wu_dur = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  wkup_dur: [get]  Wakeup duration [1 LSb = 1 / ODR].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of wu_dur in reg WAKE_UP_DUR
  *
  */
int32_t lsm303ah_xl_wkup_dur_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  *val = reg.wake_up_dur_a.wu_dur;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup   Activity/Inactivity_detection
  * @brief   This section groups all the functions concerning
  *          activity/inactivity detection.
  * @{
  */
/**
  * @brief  sleep_mode: [set]  Enables gyroscope Sleep mode.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sleep_on in reg WAKE_UP_THS
  *
  */
int32_t lsm303ah_xl_sleep_mode_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  reg.wake_up_ths_a.sleep_on = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  sleep_mode: [get]  Enables gyroscope Sleep mode.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sleep_on in reg WAKE_UP_THS
  *
  */
int32_t lsm303ah_xl_sleep_mode_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  *val = reg.wake_up_ths_a.sleep_on;

  return mm_error;
}

/**
  * @brief  act_sleep_dur: [set] Duration to go in sleep mode
  *                              [1 LSb = 512 / ODR].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sleep_dur in reg WAKE_UP_DUR
  *
  */
int32_t lsm303ah_xl_act_sleep_dur_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  reg.wake_up_dur_a.sleep_dur = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  act_sleep_dur: [get] Duration to go in sleep mode
  *                              [1 LSb = 512 / ODR].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sleep_dur in reg WAKE_UP_DUR
  *
  */
int32_t lsm303ah_xl_act_sleep_dur_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  *val = reg.wake_up_dur_a.sleep_dur;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  tap_generator
  * @brief   This section groups all the functions that manage the tap and
  *          double tap event generation.
  * @{
  */

/**
  * @brief  tap_detection_on_z: [set]  Enable Z direction in tap recognition.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tap_z_en in reg CTRL3
  *
  */
int32_t lsm303ah_xl_tap_detection_on_z_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  reg.ctrl3_a.tap_z_en = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_detection_on_z: [get]  Enable Z direction in tap recognition.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tap_z_en in reg CTRL3
  *
  */
int32_t lsm303ah_xl_tap_detection_on_z_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  *val = reg.ctrl3_a.tap_z_en;

  return mm_error;
}

/**
  * @brief  tap_detection_on_y: [set]  Enable Y direction in tap recognition.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tap_y_en in reg CTRL3
  *
  */
int32_t lsm303ah_xl_tap_detection_on_y_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  reg.ctrl3_a.tap_y_en = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_detection_on_y: [get]  Enable Y direction in tap recognition.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tap_y_en in reg CTRL3
  *
  */
int32_t lsm303ah_xl_tap_detection_on_y_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  *val = reg.ctrl3_a.tap_y_en;

  return mm_error;
}

/**
  * @brief  tap_detection_on_x: [set]  Enable X direction in tap recognition.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tap_x_en in reg CTRL3
  *
  */
int32_t lsm303ah_xl_tap_detection_on_x_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  reg.ctrl3_a.tap_x_en = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_detection_on_x: [get]  Enable X direction in tap recognition.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tap_x_en in reg CTRL3
  *
  */
int32_t lsm303ah_xl_tap_detection_on_x_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_CTRL3_A, &reg.byte, 1);
  *val = reg.ctrl3_a.tap_x_en;

  return mm_error;
}

/**
  * @brief  tap_threshold: [set] Threshold for tap recognition
  *                              [1 LSb = FS/32].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tap_ths in reg TAP_6D_THS
  *
  */
int32_t lsm303ah_xl_tap_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);
  reg.tap_6d_ths_a.tap_ths = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_threshold: [get] Threshold for tap recognition
  *                              [1 LSb = FS/32].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tap_ths in reg TAP_6D_THS
  *
  */
int32_t lsm303ah_xl_tap_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);
  *val = reg.tap_6d_ths_a.tap_ths;

  return mm_error;
}

/**
  * @brief  tap_shock: [set] Maximum duration is the maximum time of
  *                          an overthreshold signal detection to be
  *                          recognized as a tap event. The default value
  *                          of these bits is 00b which corresponds to
  *                          4*ODR_XL time. If the SHOCK[1:0] bits are set
  *                          to a different value, 1LSB corresponds to
  *                          8*ODR_XL time.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of shock in reg INT_DUR
  *
  */
int32_t lsm303ah_xl_tap_shock_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);
  reg.int_dur_a.shock = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_shock: [get] Maximum duration is the maximum time of an
  *                          overthreshold signal detection to be recognized
  *                          as a tap event. The default value of these bits
  *                          is 00b which corresponds to 4*ODR_XL time.
  *                          If the SHOCK[1:0] bits are set to a different
                             value, 1LSB corresponds to 8*ODR_XL time.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of shock in reg INT_DUR
  *
  */
int32_t lsm303ah_xl_tap_shock_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);
  *val = reg.int_dur_a.shock;

  return mm_error;
}

/**
  * @brief  tap_quiet: [set] Quiet time is the time after the first
  *                          detected tap in which there must not be any
  *                          overthreshold event. The default value of these
  *                          bits is 00b which corresponds to 2*ODR_XL time.
  *                          If the QUIET[1:0] bits are set to a different
  *                          value, 1LSB corresponds to 4*ODR_XL time.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of quiet in reg INT_DUR
  *
  */
int32_t lsm303ah_xl_tap_quiet_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);
  reg.int_dur_a.quiet = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_quiet: [get] Quiet time is the time after the first detected
  *                          tap in which there must not be any overthreshold
  *                          event. The default value of these bits is 00b
  *                          which corresponds to 2*ODR_XL time.
  *                          If the QUIET[1:0] bits are set to a different
  *                          value, 1LSB corresponds to 4*ODR_XL time.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of quiet in reg INT_DUR
  *
  */
int32_t lsm303ah_xl_tap_quiet_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);
  *val = reg.int_dur_a.quiet;

  return mm_error;
}

/**
  * @brief  tap_dur: [set] When double tap recognition is enabled, this
  *                        register expresses the maximum time between two
  *                        consecutive detected taps to determine a double
  *                        tap event. The default value of these bits is
  *                        0000b which corresponds to 16*ODR_XL time.
  *                        If the DUR[3:0] bits are set to a different value,
  *                        1LSB corresponds to 32*ODR_XL time.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of lat in reg INT_DUR
  *
  */
int32_t lsm303ah_xl_tap_dur_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);
  reg.int_dur_a.lat = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_dur: [get] When double tap recognition is enabled,
  *                        this register expresses the maximum time
  *                        between two consecutive detected taps to
  *                        determine a double tap event. The default
  *                        value of these bits is 0000b which corresponds
  *                        to 16*ODR_XL time. If the DUR[3:0] bits are set
  *                        to a different value, 1LSB corresponds to
  *                        32*ODR_XL time.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of lat in reg INT_DUR
  *
  */
int32_t lsm303ah_xl_tap_dur_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_INT_DUR_A, &reg.byte, 1);
  *val = reg.int_dur_a.lat;

  return mm_error;
}

/**
  * @brief  tap_mode: [set]  Single/double-tap event enable/disable.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_single_double_tap_t: change the values of
  *                                       single_double_tap in regWAKE_UP_THS
  *
  */
int32_t lsm303ah_xl_tap_mode_set(lsm303ah_ctx_t *ctx,
                              lsm303ah_xl_single_double_tap_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  reg.wake_up_ths_a.single_double_tap = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_mode: [get]  Single/double-tap event enable/disable.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_single_double_tap_t: Get the values of
  *                                        single_double_tap in
  *                                        reg WAKE_UP_THS
  *
  */
int32_t lsm303ah_xl_tap_mode_get(lsm303ah_ctx_t *ctx,
                              lsm303ah_xl_single_double_tap_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg.byte, 1);
  *val = (lsm303ah_xl_single_double_tap_t)reg.wake_up_ths_a.single_double_tap;

  return mm_error;
}

/**
  * @brief  tap_src: [get]  TAP source register
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_tap_src_t: registers TAP_SRC
  *
  */
int32_t lsm303ah_xl_tap_src_get(lsm303ah_ctx_t *ctx,
                                lsm303ah_tap_src_a_t *val)
{
  return lsm303ah_read_reg(ctx, LSM303AH_TAP_SRC_A, (uint8_t*) val, 1);
}

/**
  * @}
  */

/**
  * @addtogroup   Six_position_detection(6D/4D)
  * @brief   This section groups all the functions concerning six
  *          position detection (6D).
  * @{
  */

/**
  * @brief  6d_threshold: [set]  Threshold for 4D/6D function.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_6d_ths_t: change the values of 6d_ths in reg TAP_6D_THS
  *
  */
int32_t lsm303ah_xl_6d_threshold_set(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_6d_ths_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);
  reg.tap_6d_ths_a._6d_ths = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  6d_threshold: [get]  Threshold for 4D/6D function.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_6d_ths_t: Get the values of 6d_ths in reg TAP_6D_THS
  *
  */
int32_t lsm303ah_xl_6d_threshold_get(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_6d_ths_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);
  *val = (lsm303ah_xl_6d_ths_t) reg.tap_6d_ths_a._6d_ths;

  return mm_error;
}

/**
  * @brief  4d_mode: [set]  4D orientation detection enable.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of 4d_en in reg TAP_6D_THS
  *
  */
int32_t lsm303ah_xl_4d_mode_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);
  reg.tap_6d_ths_a._4d_en = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  4d_mode: [get]  4D orientation detection enable.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of 4d_en in reg TAP_6D_THS
  *
  */
int32_t lsm303ah_xl_4d_mode_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_TAP_6D_THS_A, &reg.byte, 1);
  *val = reg.tap_6d_ths_a._4d_en;

  return mm_error;
}

/**
  * @brief  6d_src: [get]  6D source register.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_6d_src_t: union of registers from 6D_SRC to
  *
  */
int32_t lsm303ah_xl_6d_src_get(lsm303ah_ctx_t *ctx, lsm303ah_6d_src_a_t *val)
{
  return lsm303ah_read_reg(ctx, LSM303AH_6D_SRC_A, (uint8_t*) val, 1);
}

/**
  * @}
  */

/**
  * @addtogroup  free_fall
  * @brief   This section group all the functions concerning the
  *          free fall detection.
  * @{
  */

/**
  * @brief  ff_dur: [set]  Free-fall duration [1 LSb = 1 / ODR].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of ff_dur in reg
  *                      WAKE_UP_DUR/FREE_FALL
  *
  */
int32_t lsm303ah_xl_ff_dur_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg[2];
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg[0].byte, 2);
  reg[1].free_fall_a.ff_dur = 0x1F & val;
  reg[0].wake_up_dur_a.ff_dur = (val & 0x20) >> 5;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg[0].byte, 2);

  return mm_error;
}

/**
  * @brief  ff_dur: [get]  Free-fall duration [1 LSb = 1 / ODR].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of ff_dur in reg WAKE_UP_DUR/FREE_FALL
  *
  */
int32_t lsm303ah_xl_ff_dur_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg[2];
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_WAKE_UP_THS_A, &reg[0].byte, 2);
  *val = (reg[0].wake_up_dur_a.ff_dur << 5) + reg[1].free_fall_a.ff_dur;

  return mm_error;
}

/**
  * @brief  ff_threshold: [set]  Free-fall threshold [1 LSB = 31.25 mg].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of ff_ths in reg FREE_FALL
  *
  */
int32_t lsm303ah_xl_ff_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FREE_FALL_A, &reg.byte, 1);
  reg.free_fall_a.ff_ths = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FREE_FALL_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  ff_threshold: [get]  Free-fall threshold [1 LSB = 31.25 mg].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of ff_ths in reg FREE_FALL
  *
  */
int32_t lsm303ah_xl_ff_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FREE_FALL_A, &reg.byte, 1);
  *val = reg.free_fall_a.ff_ths;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  Fifo
  * @brief   This section group all the functions concerning the fifo usage
  * @{
  */

/**
  * @brief   fifo_xl_module_batch: [set]  Module routine result is send to
  *          FIFO instead of X,Y,Z acceleration data
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of module_to_fifo in reg FIFO_CTRL
  *
  */
int32_t lsm303ah_xl_fifo_xl_module_batch_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);
  reg.fifo_ctrl_a.module_to_fifo = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   fifo_xl_module_batch: [get]  Module routine result is send to
  *                                       FIFO instead of X,Y,Z acceleration
  *                                       data
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of module_to_fifo in reg FIFO_CTRL
  *
  */
int32_t lsm303ah_xl_fifo_xl_module_batch_get(lsm303ah_ctx_t *ctx,
                                             uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);
  *val = reg.fifo_ctrl_a.module_to_fifo;

  return mm_error;
}

/**
  * @brief  fifo_mode: [set]  FIFO mode selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_fmode_t: change the values of fmode in reg FIFO_CTRL
  *
  */
int32_t lsm303ah_xl_fifo_mode_set(lsm303ah_ctx_t *ctx,
                                  lsm303ah_xl_fmode_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);
  reg.fifo_ctrl_a.fmode = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_mode: [get]  FIFO mode selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_fmode_t: Get the values of fmode in reg FIFO_CTRL
  *
  */
int32_t lsm303ah_xl_fifo_mode_get(lsm303ah_ctx_t *ctx,
                                  lsm303ah_xl_fmode_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_CTRL_A, &reg.byte, 1);
  *val = (lsm303ah_xl_fmode_t) reg.fifo_ctrl_a.fmode;

  return mm_error;
}

/**
  * @brief  fifo_watermark: [set]  FIFO watermark level selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of fifo_watermark in reg FIFO_THS
  *
  */
int32_t lsm303ah_xl_fifo_watermark_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  int32_t mm_error;

  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FIFO_THS_A, &val, 1);

  return mm_error;
}

/**
  * @brief  fifo_watermark: [get]  FIFO watermark level selection.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fifo_watermark in reg FIFO_THS
  *
  */
int32_t lsm303ah_xl_fifo_watermark_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_THS_A, val, 1);

  return mm_error;
}

/**
  * @brief  fifo_full_flag: [get]  FIFO full, 256 unread samples.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of diff in reg FIFO_SRC
  *
  */
int32_t lsm303ah_xl_fifo_full_flag_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_SRC_A, &reg.byte, 1);
  *val = reg.fifo_src_a.diff;

  return mm_error;
}

/**
  * @brief  fifo_ovr_flag: [get]  FIFO overrun status.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fifo_ovr in reg FIFO_SRC
  *
  */
int32_t lsm303ah_xl_fifo_ovr_flag_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_SRC_A, &reg.byte, 1);
  *val = reg.fifo_src_a.fifo_ovr;

  return mm_error;
}

/**
  * @brief  fifo_wtm_flag: [get]  FIFO threshold status.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fth in reg FIFO_SRC
  *
  */
int32_t lsm303ah_xl_fifo_wtm_flag_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_SRC_A, &reg.byte, 1);
  *val = reg.fifo_src_a.fth;

  return mm_error;
}

/**
  * @brief  fifo_data_level: [get] The number of unread samples
  *                                stored in FIFO.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint16_t: change the values of diff in reg FIFO_SAMPLES
  *
  */
int32_t lsm303ah_xl_fifo_data_level_get(lsm303ah_ctx_t *ctx, uint16_t *val)
{
  lsm303ah_reg_t reg[2];
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FIFO_SRC_A, &reg[0].byte, 2);
  *val = (reg[1].fifo_src_a.diff << 7) + reg[0].byte;

  return mm_error;
}

/**
  * @brief  fifo_src: [get] FIFO_SRCregister.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_fifo_src_t: registers FIFO_SRC
  *
  */
int32_t lsm303ah_xl_fifo_src_get(lsm303ah_ctx_t *ctx,
                                 lsm303ah_fifo_src_a_t *val)
{
  return lsm303ah_read_reg(ctx, LSM303AH_FIFO_SRC_A, (uint8_t*) val, 1);
}

/**
  * @}
  */

/**
  * @addtogroup  Pedometer
  * @brief   This section groups all the functions that manage pedometer.
  * @{
  */

/**
  * @brief  pedo_threshold: [set] Minimum threshold value for step
  *                               counter routine.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sc_mths in
  *                      reg STEP_COUNTER_MINTHS
  *
  */
int32_t lsm303ah_xl_pedo_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                               &reg.byte, 1);
  reg. step_counter_minths_a.sc_mths = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                                &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pedo_threshold: [get] Minimum threshold value for step
  *                               counter routine.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sc_mths in reg  STEP_COUNTER_MINTHS
  *
  */
int32_t lsm303ah_xl_pedo_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                               &reg.byte, 1);
  *val = reg. step_counter_minths_a.sc_mths;

  return mm_error;
}

/**
  * @brief  pedo_full_scale: [set]  Pedometer data range.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_pedo4g_t: change the values of pedo4g in
  *                            reg STEP_COUNTER_MINTHS
  *
  */
int32_t lsm303ah_xl_pedo_full_scale_set(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_pedo4g_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                               &reg.byte, 1);
  reg. step_counter_minths_a.pedo4g = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                                &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pedo_full_scale: [get]  Pedometer data range.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  lsm303ah_xl_pedo4g_t: Get the values of pedo4g in
  *                            reg STEP_COUNTER_MINTHS
  *
  */
int32_t lsm303ah_xl_pedo_full_scale_get(lsm303ah_ctx_t *ctx,
                                     lsm303ah_xl_pedo4g_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                               &reg.byte, 1);
  *val = (lsm303ah_xl_pedo4g_t) reg. step_counter_minths_a.pedo4g;

  return mm_error;
}

/**
  * @brief  pedo_step_reset: [set]  Reset pedometer step counter.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of rst_nstep in
  *                      reg STEP_COUNTER_MINTHS
  *
  */
int32_t lsm303ah_xl_pedo_step_reset_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                               &reg.byte, 1);
  reg. step_counter_minths_a.rst_nstep = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                                &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pedo_step_reset: [get]  Reset pedometer step counter.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of rst_nstep in
  *                  reg STEP_COUNTER_MINTHS
  *
  */
int32_t lsm303ah_xl_pedo_step_reset_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STEP_COUNTER_MINTHS_A,
                               &reg.byte, 1);
  *val = reg. step_counter_minths_a.rst_nstep;

  return mm_error;
}

/**
  * @brief   pedo_step_detect_flag: [get]  Step detection flag.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of step_detect in reg FUNC_CK_GATE
  *
  */
int32_t lsm303ah_xl_pedo_step_detect_flag_get(lsm303ah_ctx_t *ctx,
                                              uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CK_GATE_A, &reg.byte, 1);
  *val = reg.func_ck_gate_a.step_detect;

  return mm_error;
}

/**
  * @brief  pedo_sens: [set]  Enable pedometer algorithm.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of step_cnt_on in reg FUNC_CTRL
  *
  */
int32_t lsm303ah_xl_pedo_sens_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);
  reg.func_ctrl_a.step_cnt_on = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pedo_sens: [get]  Enable pedometer algorithm.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of step_cnt_on in reg FUNC_CTRL
  *
  */
int32_t lsm303ah_xl_pedo_sens_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);
  *val = reg.func_ctrl_a.step_cnt_on;

  return mm_error;
}

/**
  * @brief   pedo_debounce_steps: [set] Minimum number of steps to start
  *                                     the increment step counter.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of deb_step in reg PEDO_DEB_REG
  *
  */
int32_t lsm303ah_xl_pedo_debounce_steps_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_ADV_BANK);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_PEDO_DEB_REG_A, &reg.byte, 1);
  reg.pedo_deb_reg_a.deb_step = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_PEDO_DEB_REG_A, &reg.byte, 1);
  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_USER_BANK);

  return mm_error;
}

/**
  * @brief   pedo_debounce_steps: [get] Minimum number of steps to start
  *                                     the increment step counter.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of deb_step in reg PEDO_DEB_REG
  *
  */
int32_t lsm303ah_xl_pedo_debounce_steps_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_ADV_BANK);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_PEDO_DEB_REG_A, &reg.byte, 1);
  *val = reg.pedo_deb_reg_a.deb_step;
  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_USER_BANK);

  return mm_error;
}

/**
  * @brief  pedo_timeout: [set] Debounce time. If the time between two
  *                             consecutive steps is greater than
  *                             DEB_TIME*80ms, the debouncer is reactivated.
  *                             Default value: 01101
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of deb_time in reg PEDO_DEB_REG
  *
  */
int32_t lsm303ah_xl_pedo_timeout_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_ADV_BANK);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_PEDO_DEB_REG_A, &reg.byte, 1);
  reg.pedo_deb_reg_a.deb_time = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_PEDO_DEB_REG_A, &reg.byte, 1);
  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_USER_BANK);

  return mm_error;
}

/**
  * @brief  pedo_timeout: [get] Debounce time. If the time between two
  *                             consecutive steps is greater than
  *                             DEB_TIME*80ms, the debouncer is reactivated.
  *                             Default value: 01101
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of deb_time in reg PEDO_DEB_REG
  *
  */
int32_t lsm303ah_xl_pedo_timeout_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_ADV_BANK);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_PEDO_DEB_REG_A, &reg.byte, 1);
  *val = reg.pedo_deb_reg_a.deb_time;
  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_USER_BANK);

  return mm_error;
}

/**
  * @brief  pedo_steps_period: [set] Period of time to detect at
  *                                  least one step to generate step
  *                                  recognition [1 LSb = 1.6384 s].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that contains data to write
  *
  */
int32_t lsm303ah_xl_pedo_steps_period_set(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  int32_t mm_error;

  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_ADV_BANK);
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_STEP_COUNT_DELTA_A, buff, 1);
  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_USER_BANK);

  return mm_error;
}

/**
  * @brief  pedo_steps_period: [get] Period of time to detect at least
  *                                  one step to generate step recognition
  *                                  [1 LSb = 1.6384 s].
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lsm303ah_xl_pedo_steps_period_get(lsm303ah_ctx_t *ctx, uint8_t *buff)
{
  int32_t mm_error;

  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_ADV_BANK);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_STEP_COUNT_DELTA_A, buff, 1);
  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_USER_BANK);

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  significant_motion
  * @brief   This section groups all the functions that manage the
  *          significant motion detection.
  * @{
  */

/**
  * @brief   motion_data_ready_flag: [get] Significant motion event
  *                                        detection status.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sig_mot_detect in reg FUNC_CK_GATE
  *
  */
int32_t lsm303ah_xl_motion_data_ready_flag_get(lsm303ah_ctx_t *ctx,
                                               uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CK_GATE_A, &reg.byte, 1);
  *val = reg.func_ck_gate_a.sig_mot_detect;

  return mm_error;
}

/**
  * @brief  motion_sens: [set]  Enable significant motion detection function.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sign_mot_on in reg FUNC_CTRL
  *
  */
int32_t lsm303ah_xl_motion_sens_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);
  reg.func_ctrl_a.sign_mot_on = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  motion_sens: [get]  Enable significant motion detection function.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sign_mot_on in reg FUNC_CTRL
  *
  */
int32_t lsm303ah_xl_motion_sens_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);
  *val = reg.func_ctrl_a.sign_mot_on;

  return mm_error;
}

/**
  * @brief  motion_threshold: [set] These bits define the threshold value
  *                                 which corresponds to the number of steps
  *                                 to be performed by the user upon a change
  *                                 of location before the significant motion
  *                                 interrupt is generated. It is expressed
  *                                 as an 8-bit unsigned value.
  *                                 The default value of this field is equal
  *                                 to 6 (= 00000110b).
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sm_ths in reg SM_THS
  *
  */
int32_t lsm303ah_xl_motion_threshold_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_ADV_BANK);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_SM_THS_A, &reg.byte, 1);
  reg.sm_ths_a.sm_ths = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_SM_THS_A, &reg.byte, 1);
  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_USER_BANK);

  return mm_error;
}

/**
  * @brief  motion_threshold: [get] These bits define the threshold value
  *                                 which corresponds to the number of steps
  *                                 to be performed by the user upon a change
  *                                 of location before the significant motion
  *                                 interrupt is generated. It is expressed as
  *                                 an 8-bit unsigned value. The default value
  *                                 of this field is equal to 6 (= 00000110b).
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sm_ths in reg SM_THS
  *
  */
int32_t lsm303ah_xl_motion_threshold_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_ADV_BANK);
  mm_error = lsm303ah_read_reg(ctx, LSM303AH_SM_THS_A, &reg.byte, 1);
  *val = reg.sm_ths_a.sm_ths;
  mm_error = lsm303ah_xl_mem_bank_set(ctx, LSM303AH_XL_USER_BANK);

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  tilt_detection
  * @brief   This section groups all the functions that manage the tilt
  *          event detection.
  * @{
  */

/**
  * @brief   tilt_data_ready_flag: [get]  Tilt event detection status.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tilt_int in reg FUNC_CK_GATE
  *
  */
int32_t lsm303ah_xl_tilt_data_ready_flag_get(lsm303ah_ctx_t *ctx,
                                             uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CK_GATE_A, &reg.byte, 1);
  *val = reg.func_ck_gate_a.tilt_int;

  return mm_error;
}

/**
  * @brief  tilt_sens: [set]  Enable tilt calculation.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tilt_on in reg FUNC_CTRL
  *
  */
int32_t lsm303ah_xl_tilt_sens_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);
  reg.func_ctrl_a.tilt_on = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tilt_sens: [get]  Enable tilt calculation.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tilt_on in reg FUNC_CTRL
  *
  */
int32_t lsm303ah_xl_tilt_sens_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);
  *val = reg.func_ctrl_a.tilt_on;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  module
  * @brief   This section groups all the functions that manage
  *          module calculation
  * @{
  */

/**
  * @brief  module_sens: [set]  Module processing enable.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of module_on in reg FUNC_CTRL
  *
  */
int32_t lsm303ah_xl_module_sens_set(lsm303ah_ctx_t *ctx, uint8_t val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);
  reg.func_ctrl_a.module_on = val;
  mm_error = lsm303ah_write_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  module_sens: [get]  Module processing enable.
  *
  * @param  lsm303ah_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of module_on in reg FUNC_CTRL
  *
  */
int32_t lsm303ah_xl_module_sens_get(lsm303ah_ctx_t *ctx, uint8_t *val)
{
  lsm303ah_reg_t reg;
  int32_t mm_error;

  mm_error = lsm303ah_read_reg(ctx, LSM303AH_FUNC_CTRL_A, &reg.byte, 1);
  *val = reg.func_ctrl_a.module_on;

  return mm_error;
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/