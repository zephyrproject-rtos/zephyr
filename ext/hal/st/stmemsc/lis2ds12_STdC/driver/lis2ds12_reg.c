/*
 ******************************************************************************
 * @file    lis2ds12_reg.c
 * @author  MEMS Software Solution Team
 * @brief   LIS2DS12 driver file
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

#include "lis2ds12_reg.h"

/**
  * @addtogroup  lis2ds12
  * @brief  This file provides a set of functions needed to drive the
  *         lis2ds12 enanced inertial module.
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
  * @param  lis2ds12_ctx_t* ctx: read / write interface definitions
  * @param  uint8_t reg: register to read
  * @param  uint8_t* data: pointer to buffer that store the data read
  * @param  uint16_t len: number of consecutive register to read
  *
  */
int32_t lis2ds12_read_reg(lis2ds12_ctx_t* ctx, uint8_t reg, uint8_t* data,
                          uint16_t len)
{
  return ctx->read_reg(ctx->handle, reg, data, len);
}

/**
  * @brief  Write generic device register
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t reg: register to write
  * @param  uint8_t* data: pointer to data to write in register reg
  * @param  uint16_t len: number of consecutive register to write
  *
*/
int32_t lis2ds12_write_reg(lis2ds12_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_all_sources: FIFO_SRC, STATUS_DUP, WAKE_UP_SRC, TAP_SRC,
  *                               6D_SRC, FUNC_CK_GATE, FUNC_SRC.
  *
  */
int32_t lis2ds12_all_sources_get(lis2ds12_ctx_t *ctx,
                                 lis2ds12_all_sources_t *val)
{
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_SRC, &(val->byte[0]), 1);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STATUS_DUP, &(val->byte[1]), 4);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CK_GATE, &(val->byte[5]), 2);

  return mm_error;
}

/**
  * @brief  block_data_update: [set] Blockdataupdate.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of bdu in reg CTRL1
  *
  */
int32_t lis2ds12_block_data_update_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);
  reg.ctrl1.bdu = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  block_data_update: [get] Blockdataupdate.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of bdu in reg CTRL1
  *
  */
int32_t lis2ds12_block_data_update_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);
  *val = reg.ctrl1.bdu;

  return mm_error;
}

/**
  * @brief  xl_full_scale: [set]   Accelerometer full-scale selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_fs_t: change the values of fs in reg CTRL1
  *
  */
int32_t lis2ds12_xl_full_scale_set(lis2ds12_ctx_t *ctx, lis2ds12_fs_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);
  reg.ctrl1.fs = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  xl_full_scale: [get]   Accelerometer full-scale selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_fs_t: Get the values of fs in reg CTRL1
  *
  */
int32_t lis2ds12_xl_full_scale_get(lis2ds12_ctx_t *ctx, lis2ds12_fs_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);
  *val = (lis2ds12_fs_t) reg.ctrl1.fs;

  return mm_error;
}

/**
  * @brief  xl_data_rate: [set]  Accelerometer data rate selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_odr_t: change the values of odr in reg CTRL1
  *
  */
int32_t lis2ds12_xl_data_rate_set(lis2ds12_ctx_t *ctx, lis2ds12_odr_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);
  reg.ctrl1.odr = val & 0x0F;
  reg.ctrl1.hf_odr = (val & 0x10) >> 4;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  xl_data_rate: [get]  Accelerometer data rate selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_odr_t: Get the values of odr in reg CTRL1
  *
  */
int32_t lis2ds12_xl_data_rate_get(lis2ds12_ctx_t *ctx, lis2ds12_odr_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL1, &reg.byte, 1);
  *val = (lis2ds12_odr_t) ((reg.ctrl1.hf_odr << 4) + reg.ctrl1.odr);

  return mm_error;
}

/**
  * @brief  status_reg: [get]  The STATUS_REG register.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_status_reg_t: registers STATUS
  *
  */
int32_t lis2ds12_status_reg_get(lis2ds12_ctx_t *ctx, lis2ds12_status_t *val)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_STATUS, (uint8_t*) val, 1);
}

/**
  * @brief  xl_flag_data_ready: [get]  Accelerometer new data available.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of drdy in reg STATUS
  *
  */
int32_t lis2ds12_xl_flag_data_ready_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STATUS, &reg.byte, 1);
  *val = reg.status.drdy;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lis2ds12_acceleration_module_raw_get(lis2ds12_ctx_t *ctx,
                                             uint8_t *buff)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_MODULE_8BIT, buff, 1);
}

/**
  * @brief  temperature_raw: [get] Temperature data output register (r).
  *                                L and H registers together express a 16-bit
  *                                word in two’s complement.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lis2ds12_temperature_raw_get(lis2ds12_ctx_t *ctx, uint8_t *buff)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_OUT_T, buff, 1);
}

/**
  * @brief  acceleration_raw: [get] Linear acceleration output register.
  *                                 The value is expressed as a 16-bit word
  *                                 in two’s complement.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lis2ds12_acceleration_raw_get(lis2ds12_ctx_t *ctx, uint8_t *buff)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_OUT_X_L, buff, 6);
}

/**
  * @brief  number_of_steps: [get] Number of steps detected by step
  *                                counter routine.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lis2ds12_number_of_steps_get(lis2ds12_ctx_t *ctx, uint8_t *buff)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_STEP_COUNTER_L, buff, 2);
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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lis2ds12_device_id_get(lis2ds12_ctx_t *ctx, uint8_t *buff)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_WHO_AM_I, buff, 1);
}

/**
  * @brief  auto_increment: [set] Register address automatically
  *                               incremented during a multiple byte
  *                               access with a serial interface.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of if_add_inc in reg CTRL2
  *
  */
int32_t lis2ds12_auto_increment_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  reg.ctrl2.if_add_inc = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  auto_increment: [get] Register address automatically incremented
  *                               during a multiple byte access with a
  *                               serial interface.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of if_add_inc in reg CTRL2
  *
  */
int32_t lis2ds12_auto_increment_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  *val = reg.ctrl2.if_add_inc;

  return mm_error;
}

/**
  * @brief  mem_bank: [set] Enable access to the embedded functions/sensor
  *                         hub configuration registers.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_func_cfg_en_t: change the values of func_cfg_en in
  *                                 reg CTRL2
  *
  */
int32_t lis2ds12_mem_bank_set(lis2ds12_ctx_t *ctx, lis2ds12_func_cfg_en_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  if (val == LIS2DS12_ADV_BANK){
    mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
    reg.ctrl2.func_cfg_en = val;
    mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  }
  else {
    mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2_ADV, &reg.byte, 1);
    reg.ctrl2.func_cfg_en = val;
    mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL2_ADV, &reg.byte, 1);
  }
  return mm_error;
}

/**
  * @brief  reset: [set] Software reset. Restore the default values in
  *                      user registers.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of soft_reset in reg CTRL2
  *
  */
int32_t lis2ds12_reset_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  reg.ctrl2.soft_reset = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  reset: [get] Software reset. Restore the default values in
  *                      user registers.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of soft_reset in reg CTRL2
  *
  */
int32_t lis2ds12_reset_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  *val = reg.ctrl2.soft_reset;

  return mm_error;
}

/**
  * @brief  boot: [set] Reboot memory content. Reload the calibration
  *                     parameters.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of boot in reg CTRL2
  *
  */
int32_t lis2ds12_boot_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  reg.ctrl2.boot = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  boot: [get] Reboot memory content. Reload the calibration
  *                     parameters.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of boot in reg CTRL2
  *
  */
int32_t lis2ds12_boot_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  *val = reg.ctrl2.boot;

  return mm_error;
}

/**
  * @brief  xl_self_test: [set]
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_st_t: change the values of st in reg CTRL3
  *
  */
int32_t lis2ds12_xl_self_test_set(lis2ds12_ctx_t *ctx, lis2ds12_st_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  reg.ctrl3.st = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  xl_self_test: [get]
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_st_t: Get the values of st in reg CTRL3
  *
  */
int32_t lis2ds12_xl_self_test_get(lis2ds12_ctx_t *ctx, lis2ds12_st_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  *val = (lis2ds12_st_t) reg.ctrl3.st;

  return mm_error;
}

/**
  * @brief  data_ready_mode: [set]
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_drdy_pulsed_t: change the values of drdy_pulsed in
  *                                 reg CTRL5
  *
  */
int32_t lis2ds12_data_ready_mode_set(lis2ds12_ctx_t *ctx,
                                     lis2ds12_drdy_pulsed_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);
  reg.ctrl5.drdy_pulsed = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  data_ready_mode: [get]
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_drdy_pulsed_t: Get the values of drdy_pulsed in reg CTRL5
  *
  */
int32_t lis2ds12_data_ready_mode_get(lis2ds12_ctx_t *ctx,
                                     lis2ds12_drdy_pulsed_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);
  *val = (lis2ds12_drdy_pulsed_t) reg.ctrl5.drdy_pulsed;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_fds_slope_t: change the values of fds_slope in reg CTRL2
  *
  */
int32_t lis2ds12_xl_hp_path_set(lis2ds12_ctx_t *ctx, lis2ds12_fds_slope_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  reg.ctrl2.fds_slope = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  xl_hp_path: [get] High-pass filter data selection on output
  *                           register and FIFO.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_fds_slope_t: Get the values of fds_slope in reg CTRL2
  *
  */
int32_t lis2ds12_xl_hp_path_get(lis2ds12_ctx_t *ctx,
                                lis2ds12_fds_slope_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  *val = (lis2ds12_fds_slope_t) reg.ctrl2.fds_slope;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_sim_t: change the values of sim in reg CTRL2
  *
  */
int32_t lis2ds12_spi_mode_set(lis2ds12_ctx_t *ctx, lis2ds12_sim_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  reg.ctrl2.sim = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  spi_mode: [get]  SPI Serial Interface Mode selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_sim_t: Get the values of sim in reg CTRL2
  *
  */
int32_t lis2ds12_spi_mode_get(lis2ds12_ctx_t *ctx, lis2ds12_sim_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  *val = (lis2ds12_sim_t) reg.ctrl2.sim;

  return mm_error;
}

/**
  * @brief  i2c_interface: [set]  Disable / Enable I2C interface.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_i2c_disable_t: change the values of i2c_disable
  *                                 in reg CTRL2
  *
  */
int32_t lis2ds12_i2c_interface_set(lis2ds12_ctx_t *ctx,
                                   lis2ds12_i2c_disable_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  reg.ctrl2.i2c_disable = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  i2c_interface: [get]  Disable / Enable I2C interface.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_i2c_disable_t: Get the values of i2c_disable in
  *                                 reg CTRL2
  *
  */
int32_t lis2ds12_i2c_interface_get(lis2ds12_ctx_t *ctx,
                                   lis2ds12_i2c_disable_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL2, &reg.byte, 1);
  *val = (lis2ds12_i2c_disable_t) reg.ctrl2.i2c_disable;

  return mm_error;
}

/**
  * @brief  cs_mode: [set]  Connect/Disconnects pull-up in if_cs pad.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_if_cs_pu_dis_t: change the values of if_cs_pu_dis
  *                                  in reg FIFO_CTRL
  *
  */
int32_t lis2ds12_cs_mode_set(lis2ds12_ctx_t *ctx,
                             lis2ds12_if_cs_pu_dis_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);
  reg.fifo_ctrl.if_cs_pu_dis = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  cs_mode: [get]  Connect/Disconnects pull-up in if_cs pad.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_if_cs_pu_dis_t: Get the values of if_cs_pu_dis in
  *                                  reg FIFO_CTRL
  *
  */
int32_t lis2ds12_cs_mode_get(lis2ds12_ctx_t *ctx,
                             lis2ds12_if_cs_pu_dis_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);
  *val = (lis2ds12_if_cs_pu_dis_t) reg.fifo_ctrl.if_cs_pu_dis;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_pp_od_t: change the values of pp_od in reg CTRL3
  *
  */
int32_t lis2ds12_pin_mode_set(lis2ds12_ctx_t *ctx, lis2ds12_pp_od_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  reg.ctrl3.pp_od = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_mode: [get]  Push-pull/open-drain selection on interrupt pad.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_pp_od_t: Get the values of pp_od in reg CTRL3
  *
  */
int32_t lis2ds12_pin_mode_get(lis2ds12_ctx_t *ctx, lis2ds12_pp_od_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  *val = (lis2ds12_pp_od_t) reg.ctrl3.pp_od;

  return mm_error;
}

/**
  * @brief  pin_polarity: [set]  Interrupt active-high/low.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_h_lactive_t: change the values of h_lactive in reg CTRL3
  *
  */
int32_t lis2ds12_pin_polarity_set(lis2ds12_ctx_t *ctx,
                                  lis2ds12_h_lactive_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  reg.ctrl3.h_lactive = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_polarity: [get]  Interrupt active-high/low.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_h_lactive_t: Get the values of h_lactive in reg CTRL3
  *
  */
int32_t lis2ds12_pin_polarity_get(lis2ds12_ctx_t *ctx,
                                  lis2ds12_h_lactive_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  *val = (lis2ds12_h_lactive_t) reg.ctrl3.h_lactive;

  return mm_error;
}

/**
  * @brief  int_notification: [set]  Latched/pulsed interrupt.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_lir_t: change the values of lir in reg CTRL3
  *
  */
int32_t lis2ds12_int_notification_set(lis2ds12_ctx_t *ctx,
                                      lis2ds12_lir_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  reg.ctrl3.lir = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  int_notification: [get]  Latched/pulsed interrupt.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_lir_t: Get the values of lir in reg CTRL3
  *
  */
int32_t lis2ds12_int_notification_get(lis2ds12_ctx_t *ctx,
                                      lis2ds12_lir_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  *val = (lis2ds12_lir_t) reg.ctrl3.lir;

  return mm_error;
}

/**
  * @brief  pin_int1_route: [set] Select the signal that need to route
  *                               on int1 pad.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_pin_int1_route_t: union of registers from CTRL4 to
  *
  */
int32_t lis2ds12_pin_int1_route_set(lis2ds12_ctx_t *ctx,
                                    lis2ds12_pin_int1_route_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL4, &reg.byte, 1);
  reg.ctrl4.int1_drdy         = val.int1_drdy;
  reg.ctrl4.int1_fth          = val.int1_fth;
  reg.ctrl4.int1_6d           = val.int1_6d;
  reg.ctrl4.int1_tap          = val.int1_tap;
  reg.ctrl4.int1_ff           = val.int1_ff;
  reg.ctrl4.int1_wu           = val.int1_wu;
  reg.ctrl4.int1_s_tap        = val.int1_s_tap;
  reg.ctrl4.int1_master_drdy  = val.int1_master_drdy;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL4, &reg.byte, 1);

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);
  reg.wake_up_dur.int1_fss7   = val.int1_fss7;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_int1_route: [get] Select the signal that need to route on
  *                               int1 pad.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_pin_int1_route_t: union of registers from CTRL4 to
  *
  */
int32_t lis2ds12_pin_int1_route_get(lis2ds12_ctx_t *ctx,
                                    lis2ds12_pin_int1_route_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL4, &reg.byte, 1);
  val->int1_drdy          = reg.ctrl4.int1_drdy;
  val->int1_fth           = reg.ctrl4.int1_fth;
  val->int1_6d            = reg.ctrl4.int1_6d;
  val->int1_tap           = reg.ctrl4.int1_tap;
  val->int1_ff            = reg.ctrl4.int1_ff;
  val->int1_wu            = reg.ctrl4.int1_wu;
  val->int1_s_tap         = reg.ctrl4.int1_s_tap;
  val->int1_master_drdy   = reg.ctrl4.int1_master_drdy;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);
  val->int1_fss7 = reg.wake_up_dur.int1_fss7;

  return mm_error;
}

/**
  * @brief  pin_int2_route: [set] Select the signal that need to route on
  *                               int2 pad.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_pin_int2_route_t: union of registers from CTRL5 to
  *
  */
int32_t lis2ds12_pin_int2_route_set(lis2ds12_ctx_t *ctx,
                                    lis2ds12_pin_int2_route_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);
  reg.ctrl5.int2_boot       = val.int2_boot;
  reg.ctrl5.int2_tilt       = val.int2_tilt;
  reg.ctrl5.int2_sig_mot    = val.int2_sig_mot;
  reg.ctrl5.int2_step_det   = val.int2_step_det;
  reg.ctrl5.int2_fth        = val.int2_fth;
  reg.ctrl5.int2_drdy       = val.int2_drdy;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_int2_route: [get] Select the signal that need to route on
  *                               int2 pad.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_pin_int2_route_t: union of registers from CTRL5 to
  *
  */
int32_t lis2ds12_pin_int2_route_get(lis2ds12_ctx_t *ctx,
                                    lis2ds12_pin_int2_route_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);
  val->int2_boot     = reg.ctrl5.int2_boot;
  val->int2_tilt     = reg.ctrl5.int2_tilt;
  val->int2_sig_mot  = reg.ctrl5.int2_sig_mot;
  val->int2_step_det = reg.ctrl5.int2_step_det;
  val->int2_fth      = reg.ctrl5.int2_fth;
  val->int2_drdy     = reg.ctrl5.int2_drdy;

  return mm_error;
}

/**
  * @brief  all_on_int1: [set] All interrupt signals become available on
  *                            INT1 pin.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int2_on_int1 in reg CTRL5
  *
  */
int32_t lis2ds12_all_on_int1_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);
  reg.ctrl5.int2_on_int1 = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  all_on_int1: [get] All interrupt signals become available on
  *                            INT1 pin.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int2_on_int1 in reg CTRL5
  *
  */
int32_t lis2ds12_all_on_int1_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL5, &reg.byte, 1);
  *val = reg.ctrl5.int2_on_int1;

  return mm_error;
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
  * @brief  sh_pin_mode: [set] Connect / Disconnect pull-up on auxiliary
  *                            I2C line.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_tud_en_t: change the values of tud_en in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_sh_pin_mode_set(lis2ds12_ctx_t *ctx, lis2ds12_tud_en_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  reg.func_ctrl.tud_en = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  sh_pin_mode: [get] Connect / Disconnect pull-up on auxiliary
  *                            I2C line.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_tud_en_t: Get the values of tud_en in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_sh_pin_mode_get(lis2ds12_ctx_t *ctx, lis2ds12_tud_en_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  *val = (lis2ds12_tud_en_t) reg.func_ctrl.tud_en;

  return mm_error;
}

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
  * @brief  wkup_threshold: [set]  Threshold for wakeup [1 LSb = FS_XL / 64].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of wu_ths in reg WAKE_UP_THS
  *
  */
int32_t lis2ds12_wkup_threshold_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);
  reg.wake_up_ths.wu_ths = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  wkup_threshold: [get]  Threshold for wakeup [1 LSb = FS_XL / 64].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of wu_ths in reg WAKE_UP_THS
  *
  */
int32_t lis2ds12_wkup_threshold_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);
  *val = reg.wake_up_ths.wu_ths;

  return mm_error;
}

/**
  * @brief  wkup_dur: [set]  Wakeup duration [1 LSb = 1 / ODR].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of wu_dur in reg WAKE_UP_DUR
  *
  */
int32_t lis2ds12_wkup_dur_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);
  reg.wake_up_dur.wu_dur = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  wkup_dur: [get]  Wakeup duration [1 LSb = 1 / ODR].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of wu_dur in reg WAKE_UP_DUR
  *
  */
int32_t lis2ds12_wkup_dur_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);
  *val = reg.wake_up_dur.wu_dur;

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
  * @brief  sleep_mode: [set]  Enables Sleep mode.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sleep_on in reg WAKE_UP_THS
  *
  */
int32_t lis2ds12_sleep_mode_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);
  reg.wake_up_ths.sleep_on = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  sleep_mode: [get]  Enables Sleep mode.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sleep_on in reg WAKE_UP_THS
  *
  */
int32_t lis2ds12_sleep_mode_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);
  *val = reg.wake_up_ths.sleep_on;

  return mm_error;
}

/**
  * @brief  act_sleep_dur: [set] Duration to go in sleep mode
  *                              [1 LSb = 512 / ODR].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sleep_dur in reg WAKE_UP_DUR
  *
  */
int32_t lis2ds12_act_sleep_dur_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);
  reg.wake_up_dur.sleep_dur = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  act_sleep_dur: [get] Duration to go in sleep mode
  *                              [1 LSb = 512 / ODR].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sleep_dur in reg WAKE_UP_DUR
  *
  */
int32_t lis2ds12_act_sleep_dur_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg.byte, 1);
  *val = reg.wake_up_dur.sleep_dur;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tap_z_en in reg CTRL3
  *
  */
int32_t lis2ds12_tap_detection_on_z_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  reg.ctrl3.tap_z_en = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_detection_on_z: [get]  Enable Z direction in tap recognition.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tap_z_en in reg CTRL3
  *
  */
int32_t lis2ds12_tap_detection_on_z_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  *val = reg.ctrl3.tap_z_en;

  return mm_error;
}

/**
  * @brief  tap_detection_on_y: [set]  Enable Y direction in tap recognition.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tap_y_en in reg CTRL3
  *
  */
int32_t lis2ds12_tap_detection_on_y_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  reg.ctrl3.tap_y_en = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_detection_on_y: [get]  Enable Y direction in tap recognition.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tap_y_en in reg CTRL3
  *
  */
int32_t lis2ds12_tap_detection_on_y_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  *val = reg.ctrl3.tap_y_en;

  return mm_error;
}

/**
  * @brief  tap_detection_on_x: [set]  Enable X direction in tap recognition.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tap_x_en in reg CTRL3
  *
  */
int32_t lis2ds12_tap_detection_on_x_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  reg.ctrl3.tap_x_en = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_detection_on_x: [get]  Enable X direction in tap recognition.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tap_x_en in reg CTRL3
  *
  */
int32_t lis2ds12_tap_detection_on_x_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_CTRL3, &reg.byte, 1);
  *val = reg.ctrl3.tap_x_en;

  return mm_error;
}

/**
  * @brief  tap_threshold: [set] Threshold for tap recognition
  *                              [1 LSb = FS/32].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tap_ths in reg TAP_6D_THS
  *
  */
int32_t lis2ds12_tap_threshold_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);
  reg.tap_6d_ths.tap_ths = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_threshold: [get] Threshold for tap recognition
  *                              [1 LSb = FS/32].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tap_ths in reg TAP_6D_THS
  *
  */
int32_t lis2ds12_tap_threshold_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);
  *val = reg.tap_6d_ths.tap_ths;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of shock in reg INT_DUR
  *
  */
int32_t lis2ds12_tap_shock_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);
  reg.int_dur.shock = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of shock in reg INT_DUR
  *
  */
int32_t lis2ds12_tap_shock_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);
  *val = reg.int_dur.shock;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of quiet in reg INT_DUR
  *
  */
int32_t lis2ds12_tap_quiet_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);
  reg.int_dur.quiet = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of quiet in reg INT_DUR
  *
  */
int32_t lis2ds12_tap_quiet_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);
  *val = reg.int_dur.quiet;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of lat in reg INT_DUR
  *
  */
int32_t lis2ds12_tap_dur_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);
  reg.int_dur.lat = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of lat in reg INT_DUR
  *
  */
int32_t lis2ds12_tap_dur_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_INT_DUR, &reg.byte, 1);
  *val = reg.int_dur.lat;

  return mm_error;
}

/**
  * @brief  tap_mode: [set]  Single/double-tap event enable/disable.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_single_double_tap_t: change the values of
  *                                       single_double_tap in regWAKE_UP_THS
  *
  */
int32_t lis2ds12_tap_mode_set(lis2ds12_ctx_t *ctx,
                              lis2ds12_single_double_tap_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);
  reg.wake_up_ths.single_double_tap = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tap_mode: [get]  Single/double-tap event enable/disable.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_single_double_tap_t: Get the values of single_double_tap
  *                                       in reg WAKE_UP_THS
  *
  */
int32_t lis2ds12_tap_mode_get(lis2ds12_ctx_t *ctx,
                              lis2ds12_single_double_tap_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_THS, &reg.byte, 1);
  *val = (lis2ds12_single_double_tap_t) reg.wake_up_ths.single_double_tap;

  return mm_error;
}

/**
  * @brief  tap_src: [get]  TAP source register
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_tap_src_t: registers TAP_SRC
  *
  */
int32_t lis2ds12_tap_src_get(lis2ds12_ctx_t *ctx, lis2ds12_tap_src_t *val)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_TAP_SRC, (uint8_t*) val, 1);
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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_6d_ths_t: change the values of 6d_ths in reg TAP_6D_THS
  *
  */
int32_t lis2ds12_6d_threshold_set(lis2ds12_ctx_t *ctx, lis2ds12_6d_ths_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);
  reg.tap_6d_ths._6d_ths = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  6d_threshold: [get]  Threshold for 4D/6D function.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_6d_ths_t: Get the values of 6d_ths in reg TAP_6D_THS
  *
  */
int32_t lis2ds12_6d_threshold_get(lis2ds12_ctx_t *ctx, lis2ds12_6d_ths_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);
  *val = (lis2ds12_6d_ths_t) reg.tap_6d_ths._6d_ths;

  return mm_error;
}

/**
  * @brief  4d_mode: [set]  4D orientation detection enable.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of 4d_en in reg TAP_6D_THS
  *
  */
int32_t lis2ds12_4d_mode_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);
  reg.tap_6d_ths._4d_en = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  4d_mode: [get]  4D orientation detection enable.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of 4d_en in reg TAP_6D_THS
  *
  */
int32_t lis2ds12_4d_mode_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_TAP_6D_THS, &reg.byte, 1);
  *val = reg.tap_6d_ths._4d_en;

  return mm_error;
}

/**
  * @brief  6d_src: [get]  6D source register.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_6d_src_t: union of registers from 6D_SRC to
  *
  */
int32_t lis2ds12_6d_src_get(lis2ds12_ctx_t *ctx, lis2ds12_6d_src_t *val)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_6D_SRC, (uint8_t*) val, 1);
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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of ff_dur in reg
  *                      WAKE_UP_DUR/FREE_FALL
  *
  */
int32_t lis2ds12_ff_dur_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg[2];
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg[0].byte, 2);
  reg[1].free_fall.ff_dur = 0x1F & val;
  reg[0].wake_up_dur.ff_dur = (val & 0x20) >> 5;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg[0].byte, 2);

  return mm_error;
}

/**
  * @brief  ff_dur: [get]  Free-fall duration [1 LSb = 1 / ODR].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of ff_dur in reg WAKE_UP_DUR/FREE_FALL
  *
  */
int32_t lis2ds12_ff_dur_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg[2];
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_WAKE_UP_DUR, &reg[0].byte, 2);
  *val = (reg[0].wake_up_dur.ff_dur << 5) + reg[1].free_fall.ff_dur;

  return mm_error;
}

/**
  * @brief  ff_threshold: [set]  Free-fall threshold [1 LSB = 31.25 mg].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of ff_ths in reg FREE_FALL
  *
  */
int32_t lis2ds12_ff_threshold_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FREE_FALL, &reg.byte, 1);
  reg.free_fall.ff_ths = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FREE_FALL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  ff_threshold: [get]  Free-fall threshold [1 LSB = 31.25 mg].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of ff_ths in reg FREE_FALL
  *
  */
int32_t lis2ds12_ff_threshold_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FREE_FALL, &reg.byte, 1);
  *val = reg.free_fall.ff_ths;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of module_to_fifo in reg FIFO_CTRL
  *
  */
int32_t lis2ds12_fifo_xl_module_batch_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);
  reg.fifo_ctrl.module_to_fifo = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   fifo_xl_module_batch: [get]  Module routine result is send to
  *                                       FIFO instead of X,Y,Z acceleration
  *                                       data
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of module_to_fifo in reg FIFO_CTRL
  *
  */
int32_t lis2ds12_fifo_xl_module_batch_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);
  *val = reg.fifo_ctrl.module_to_fifo;

  return mm_error;
}

/**
  * @brief  fifo_mode: [set]  FIFO mode selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_fmode_t: change the values of fmode in reg FIFO_CTRL
  *
  */
int32_t lis2ds12_fifo_mode_set(lis2ds12_ctx_t *ctx, lis2ds12_fmode_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);
  reg.fifo_ctrl.fmode = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_mode: [get]  FIFO mode selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_fmode_t: Get the values of fmode in reg FIFO_CTRL
  *
  */
int32_t lis2ds12_fifo_mode_get(lis2ds12_ctx_t *ctx, lis2ds12_fmode_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_CTRL, &reg.byte, 1);
  *val = (lis2ds12_fmode_t) reg.fifo_ctrl.fmode;

  return mm_error;
}

/**
  * @brief  fifo_watermark: [set]  FIFO watermark level selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of fifo_watermark in reg FIFO_THS
  *
  */
int32_t lis2ds12_fifo_watermark_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  int32_t mm_error;

  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FIFO_THS, &val, 1);

  return mm_error;
}

/**
  * @brief  fifo_watermark: [get]  FIFO watermark level selection.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fifo_watermark in reg FIFO_THS
  *
  */
int32_t lis2ds12_fifo_watermark_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_THS, val, 1);

  return mm_error;
}

/**
  * @brief  fifo_full_flag: [get]  FIFO full, 256 unread samples.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of diff in reg FIFO_SRC
  *
  */
int32_t lis2ds12_fifo_full_flag_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_SRC, &reg.byte, 1);
  *val = reg.fifo_src.diff;

  return mm_error;
}

/**
  * @brief  fifo_ovr_flag: [get]  FIFO overrun status.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fifo_ovr in reg FIFO_SRC
  *
  */
int32_t lis2ds12_fifo_ovr_flag_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_SRC, &reg.byte, 1);
  *val = reg.fifo_src.fifo_ovr;

  return mm_error;
}

/**
  * @brief  fifo_wtm_flag: [get]  FIFO threshold status.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fth in reg FIFO_SRC
  *
  */
int32_t lis2ds12_fifo_wtm_flag_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_SRC, &reg.byte, 1);
  *val = reg.fifo_src.fth;

  return mm_error;
}

/**
  * @brief  fifo_data_level: [get] The number of unread samples
  *                                stored in FIFO.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint16_t: change the values of diff in reg FIFO_SAMPLES
  *
  */
int32_t lis2ds12_fifo_data_level_get(lis2ds12_ctx_t *ctx, uint16_t *val)
{
  lis2ds12_reg_t reg[2];
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FIFO_THS, &reg[0].byte, 2);
  *val = (reg[1].fifo_src.diff << 8) + reg[0].byte;

  return mm_error;
}

/**
  * @brief  fifo_src: [get] FIFO_SRCregister.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_fifo_src_t: registers FIFO_SRC
  *
  */
int32_t lis2ds12_fifo_src_get(lis2ds12_ctx_t *ctx, lis2ds12_fifo_src_t *val)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_FIFO_SRC, (uint8_t*) val, 1);
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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sc_mths in
  *                      reg STEP_COUNTER_MINTHS
  *
  */
int32_t lis2ds12_pedo_threshold_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                               &reg.byte, 1);
  reg. step_counter_minths.sc_mths = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                                &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pedo_threshold: [get] Minimum threshold value for step
  *                               counter routine.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sc_mths in reg  STEP_COUNTER_MINTHS
  *
  */
int32_t lis2ds12_pedo_threshold_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                               &reg.byte, 1);
  *val = reg. step_counter_minths.sc_mths;

  return mm_error;
}

/**
  * @brief  pedo_full_scale: [set]  Pedometer data range.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_pedo4g_t: change the values of pedo4g in
  *                            reg STEP_COUNTER_MINTHS
  *
  */
int32_t lis2ds12_pedo_full_scale_set(lis2ds12_ctx_t *ctx,
                                     lis2ds12_pedo4g_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                               &reg.byte, 1);
  reg. step_counter_minths.pedo4g = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                                &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pedo_full_scale: [get]  Pedometer data range.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_pedo4g_t: Get the values of pedo4g in
  *                            reg STEP_COUNTER_MINTHS
  *
  */
int32_t lis2ds12_pedo_full_scale_get(lis2ds12_ctx_t *ctx,
                                     lis2ds12_pedo4g_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                               &reg.byte, 1);
  *val = (lis2ds12_pedo4g_t) reg. step_counter_minths.pedo4g;

  return mm_error;
}

/**
  * @brief  pedo_step_reset: [set]  Reset pedometer step counter.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of rst_nstep in
  *                      reg STEP_COUNTER_MINTHS
  *
  */
int32_t lis2ds12_pedo_step_reset_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                               &reg.byte, 1);
  reg. step_counter_minths.rst_nstep = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                                &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pedo_step_reset: [get]  Reset pedometer step counter.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of rst_nstep in
  *                  reg STEP_COUNTER_MINTHS
  *
  */
int32_t lis2ds12_pedo_step_reset_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STEP_COUNTER_MINTHS,
                               &reg.byte, 1);
  *val = reg. step_counter_minths.rst_nstep;

  return mm_error;
}

/**
  * @brief   pedo_step_detect_flag: [get]  Step detection flag.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of step_detect in reg FUNC_CK_GATE
  *
  */
int32_t lis2ds12_pedo_step_detect_flag_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CK_GATE, &reg.byte, 1);
  *val = reg.func_ck_gate.step_detect;

  return mm_error;
}

/**
  * @brief  pedo_sens: [set]  Enable pedometer algorithm.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of step_cnt_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_pedo_sens_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  reg.func_ctrl.step_cnt_on = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pedo_sens: [get]  Enable pedometer algorithm.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of step_cnt_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_pedo_sens_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  *val = reg.func_ctrl.step_cnt_on;

  return mm_error;
}

/**
  * @brief   pedo_debounce_steps: [set] Minimum number of steps to start
  *                                     the increment step counter.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of deb_step in reg PEDO_DEB_REG
  *
  */
int32_t lis2ds12_pedo_debounce_steps_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_PEDO_DEB_REG, &reg.byte, 1);
  reg.pedo_deb_reg.deb_step = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_PEDO_DEB_REG, &reg.byte, 1);
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

  return mm_error;
}

/**
  * @brief   pedo_debounce_steps: [get] Minimum number of steps to start
  *                                     the increment step counter.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of deb_step in reg PEDO_DEB_REG
  *
  */
int32_t lis2ds12_pedo_debounce_steps_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_PEDO_DEB_REG, &reg.byte, 1);
  *val = reg.pedo_deb_reg.deb_step;
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

  return mm_error;
}

/**
  * @brief  pedo_timeout: [set] Debounce time. If the time between two
  *                             consecutive steps is greater than
  *                             DEB_TIME*80ms, the debouncer is reactivated.
  *                             Default value: 01101
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of deb_time in reg PEDO_DEB_REG
  *
  */
int32_t lis2ds12_pedo_timeout_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_PEDO_DEB_REG, &reg.byte, 1);
  reg.pedo_deb_reg.deb_time = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_PEDO_DEB_REG, &reg.byte, 1);
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

  return mm_error;
}

/**
  * @brief  pedo_timeout: [get] Debounce time. If the time between two
  *                             consecutive steps is greater than
  *                             DEB_TIME*80ms, the debouncer is reactivated.
  *                             Default value: 01101
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of deb_time in reg PEDO_DEB_REG
  *
  */
int32_t lis2ds12_pedo_timeout_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_PEDO_DEB_REG, &reg.byte, 1);
  *val = reg.pedo_deb_reg.deb_time;
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

  return mm_error;
}

/**
  * @brief  pedo_steps_period: [set] Period of time to detect at
  *                                  least one step to generate step
  *                                  recognition [1 LSb = 1.6384 s].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that contains data to write
  *
  */
int32_t lis2ds12_pedo_steps_period_set(lis2ds12_ctx_t *ctx, uint8_t *buff)
{
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_STEP_COUNT_DELTA, buff, 1);
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

  return mm_error;
}

/**
  * @brief  pedo_steps_period: [get] Period of time to detect at least
  *                                  one step to generate step recognition
  *                                  [1 LSb = 1.6384 s].
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t lis2ds12_pedo_steps_period_get(lis2ds12_ctx_t *ctx, uint8_t *buff)
{
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_STEP_COUNT_DELTA, buff, 1);
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sig_mot_detect in reg FUNC_CK_GATE
  *
  */
int32_t lis2ds12_motion_data_ready_flag_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CK_GATE, &reg.byte, 1);
  *val = reg.func_ck_gate.sig_mot_detect;

  return mm_error;
}

/**
  * @brief  motion_sens: [set]  Enable significant motion detection function.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sign_mot_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_motion_sens_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  reg.func_ctrl.sign_mot_on = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  motion_sens: [get]  Enable significant motion detection function.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sign_mot_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_motion_sens_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  *val = reg.func_ctrl.sign_mot_on;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sm_ths in reg SM_THS
  *
  */
int32_t lis2ds12_motion_threshold_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_SM_THS, &reg.byte, 1);
  reg.sm_ths.sm_ths = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_SM_THS, &reg.byte, 1);
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sm_ths in reg SM_THS
  *
  */
int32_t lis2ds12_motion_threshold_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_SM_THS, &reg.byte, 1);
  *val = reg.sm_ths.sm_ths;
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tilt_int in reg FUNC_CK_GATE
  *
  */
int32_t lis2ds12_tilt_data_ready_flag_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CK_GATE, &reg.byte, 1);
  *val = reg.func_ck_gate.tilt_int;

  return mm_error;
}

/**
  * @brief  tilt_sens: [set]  Enable tilt calculation.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of tilt_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_tilt_sens_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  reg.func_ctrl.tilt_on = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  tilt_sens: [get]  Enable tilt calculation.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of tilt_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_tilt_sens_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  *val = reg.func_ctrl.tilt_on;

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
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of module_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_module_sens_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  reg.func_ctrl.module_on = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  module_sens: [get]  Module processing enable.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of module_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_module_sens_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  *val = reg.func_ctrl.module_on;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  Sensor_hub
  * @brief   This section groups all the functions that manage the sensor
  *          hub functionality.
  * @{
  */

/**
  * @brief  sh_read_data_raw: [get]  Sensor hub output registers.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_sh_read_data_raw_t: registers from SENSORHUB1_REG
  *                                      to SENSORHUB6_REG
  *
  */
int32_t lis2ds12_sh_read_data_raw_get(lis2ds12_ctx_t *ctx,
                                      lis2ds12_sh_read_data_raw_t *val)
{
  return lis2ds12_read_reg(ctx, LIS2DS12_SENSORHUB1_REG, (uint8_t*) val, 6);
}

/**
  * @brief  sh_master: [set]  Sensor hub I2C master enable.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of master_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_sh_master_set(lis2ds12_ctx_t *ctx, uint8_t val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  reg.func_ctrl.master_on = val;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  sh_master: [get]  Sensor hub I2C master enable.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of master_on in reg FUNC_CTRL
  *
  */
int32_t lis2ds12_sh_master_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_CTRL, &reg.byte, 1);
  *val = reg.func_ctrl.master_on;

  return mm_error;
}

/**
  * @brief  sh_cfg_write: Configure slave to perform a write.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_sh_cfg_write_t: a structure that contain
  *                      - uint8_t slv_add;    8 bit i2c device address
  *                      - uint8_t slv_subadd; 8 bit register device address
  *                      - uint8_t slv_data;   8 bit data to write
  *
  */
int32_t lis2ds12_sh_cfg_write(lis2ds12_ctx_t *ctx,
                              lis2ds12_sh_cfg_write_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  reg.byte = val->slv_add;
  reg.slv0_add.rw_0 = 0;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_SLV0_ADD, &reg.byte, 1);
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_SLV0_SUBADD,
                               &(val->slv_subadd), 1);
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_DATAWRITE_SLV0,
                              &(val->slv_data), 1);
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

  return mm_error;
}

/**
  * @brief  sh_slv_cfg_read: [get] Configure slave 0 for perform a write/read.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  lis2ds12_sh_cfg_read_t: a structure that contain
  *                      - uint8_t slv_add;    8 bit i2c device address
  *                      - uint8_t slv_subadd; 8 bit register device address
  *                      - uint8_t slv_len;    num of bit to read
  *
  */
int32_t lis2ds12_sh_slv_cfg_read(lis2ds12_ctx_t *ctx,
                                 lis2ds12_sh_cfg_read_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_ADV_BANK);
  reg.byte = val->slv_add;
  reg.slv0_add.rw_0 = 1;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_SLV0_ADD, &reg.byte, 1);
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_SLV0_SUBADD,
                               &(val->slv_subadd), 1);
  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_SLV0_CONFIG, &reg.byte, 1);
  reg.slv0_config.slave0_numop = val->slv_len;
  mm_error = lis2ds12_write_reg(ctx, LIS2DS12_SLV0_CONFIG, &reg.byte, 1);
  mm_error = lis2ds12_mem_bank_set(ctx, LIS2DS12_USER_BANK);

  return mm_error;
}

/**
  * @brief  lis2ds12_sh_end_op_flag_get: [get] Sensor hub communication status.
  *
  * @param  lis2ds12_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sensorhub_end_op
  *
  */
int32_t lis2ds12_sh_end_op_flag_get(lis2ds12_ctx_t *ctx, uint8_t *val)
{
  lis2ds12_reg_t reg;
  int32_t mm_error;

  mm_error = lis2ds12_read_reg(ctx, LIS2DS12_FUNC_SRC, &reg.byte, 1);
  *val = reg.func_src.sensorhub_end_op;

  return mm_error;
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/