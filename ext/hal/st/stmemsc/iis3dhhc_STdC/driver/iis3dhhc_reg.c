/*
 ******************************************************************************
 * @file    iis3dhhc_reg.c
 * @author  MEMS Software Solution Team
 * @date    20-December-2017
 * @brief   IIS3DHHC driver file
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

#include "iis3dhhc_reg.h"

/**
  * @addtogroup  iis3dhhc
  * @brief  This file provides a set of functions needed to drive the
  *         iis3dhhc enanced inertial module.
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
  * @param  iis3dhhc_ctx_t* ctx: read / write interface definitions
  * @param  uint8_t reg: register to read
  * @param  uint8_t* data: pointer to buffer that store the data read
  * @param  uint16_t len: number of consecutive register to read
  *
  */
int32_t iis3dhhc_read_reg(iis3dhhc_ctx_t* ctx, uint8_t reg, uint8_t* data,
                         uint16_t len)
{
  return ctx->read_reg(ctx->handle, reg, data, len);
}

/**
  * @brief  Write generic device register
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t reg: register to write
  * @param  uint8_t* data: pointer to data to write in register reg
  * @param  uint16_t len: number of consecutive register to write
  *
*/
int32_t iis3dhhc_write_reg(iis3dhhc_ctx_t* ctx, uint8_t reg, uint8_t* data,
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
  * @brief  block_data_update: [set] Blockdataupdate.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of bdu in reg CTRL_REG1
  *
  */
int32_t iis3dhhc_block_data_update_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.bdu = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  block_data_update: [get] Blockdataupdate.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of bdu in reg CTRL_REG1
  *
  */
int32_t iis3dhhc_block_data_update_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  *val = reg.ctrl_reg1.bdu;

  return mm_error;
}

/**
  * @brief  data_rate: [set]  Output data rate selection.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_norm_mod_en_t: change the values of norm_mod_en in
  *                                reg CTRL_REG1
  *
  */
int32_t iis3dhhc_data_rate_set(iis3dhhc_ctx_t *ctx, iis3dhhc_norm_mod_en_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.norm_mod_en = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  data_rate: [get]  Output data rate selection.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_norm_mod_en_t: Get the values of norm_mod_en in
  *                                reg CTRL_REG1
  *
  */
int32_t iis3dhhc_data_rate_get(iis3dhhc_ctx_t *ctx, iis3dhhc_norm_mod_en_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  *val = (iis3dhhc_norm_mod_en_t) reg.ctrl_reg1.norm_mod_en;

  return mm_error;
}

/**
  * @brief  offset_temp_comp: [set]  Offset temperature compensation enable.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of off_tcomp_en in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_offset_temp_comp_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.off_tcomp_en = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  offset_temp_comp: [get]  Offset temperature compensation enable.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of off_tcomp_en in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_offset_temp_comp_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  *val = reg.ctrl_reg4.off_tcomp_en;

  return mm_error;
}

/**
  * @brief  temperature_raw: [get]  Temperature output value.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t iis3dhhc_temperature_raw_get(iis3dhhc_ctx_t *ctx, uint8_t *buff)
{
  return iis3dhhc_read_reg(ctx, IIS3DHHC_OUT_TEMP_L, buff, 2);
}

/**
  * @brief  acceleration_raw: [get]  acceleration output value.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t iis3dhhc_acceleration_raw_get(iis3dhhc_ctx_t *ctx, uint8_t *buff)
{
  return iis3dhhc_read_reg(ctx, IIS3DHHC_OUT_X_L_XL, buff, 6);
}

/**
  * @brief  xl_data_ready: [get]  Acceleration set of data available.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of zyxda in reg STATUS
  *
  */
int32_t iis3dhhc_xl_data_ready_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_STATUS, &reg.byte, 1);
  *val = reg.status.zyxda;

  return mm_error;
}

/**
  * @brief  xl_data_ovr: [get]  Acceleration set of data overrun.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of zyxor in reg STATUS
  *
  */
int32_t iis3dhhc_xl_data_ovr_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_STATUS, &reg.byte, 1);
  *val = reg.status.zyxor;

  return mm_error;
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
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t * : buffer that stores data read
  *
  */
int32_t iis3dhhc_device_id_get(iis3dhhc_ctx_t *ctx, uint8_t *buff)
{
  return iis3dhhc_read_reg(ctx, IIS3DHHC_WHO_AM_I, buff, 1);
}

/**
  * @brief  reset: [set]  Software reset. Restore the default values
  *                       in user registers.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of sw_reset in reg CTRL_REG1
  *
  */
int32_t iis3dhhc_reset_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.sw_reset = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  reset: [get]  Software reset. Restore the default values
  *                       in user registers.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of sw_reset in reg CTRL_REG1
  *
  */
int32_t iis3dhhc_reset_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  *val = reg.ctrl_reg1.sw_reset;

  return mm_error;
}

/**
  * @brief  boot: [set]  Reboot memory content. Reload the
  *                      calibration parameters.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of boot in reg CTRL_REG1
  *
  */
int32_t iis3dhhc_boot_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.boot = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  boot: [get]  Reboot memory content. Reload
  *                      the calibration parameters.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of boot in reg CTRL_REG1
  *
  */
int32_t iis3dhhc_boot_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  *val = reg.ctrl_reg1.boot;

  return mm_error;
}

/**
  * @brief  self_test: [set] Selftest.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_st_t: change the values of st in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_self_test_set(iis3dhhc_ctx_t *ctx, iis3dhhc_st_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.st = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  self_test: [get] Selftest.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_st_t: Get the values of st in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_self_test_get(iis3dhhc_ctx_t *ctx, iis3dhhc_st_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  *val = (iis3dhhc_st_t) reg.ctrl_reg4.st;

  return mm_error;
}

/**
  * @brief  filter_config: [set]  Digital filtering Phase/bandwidth selection.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_dsp_t: change the values of dsp in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_filter_config_set(iis3dhhc_ctx_t *ctx, iis3dhhc_dsp_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.dsp = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  filter_config: [get]  Digital filtering Phase/bandwidth selection.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_dsp_t: Get the values of dsp in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_filter_config_get(iis3dhhc_ctx_t *ctx, iis3dhhc_dsp_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  *val = (iis3dhhc_dsp_t) reg.ctrl_reg4.dsp;

  return mm_error;
}

/**
  * @brief  status: [get] Statusregister.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_status_t: Registers STATUS
  *
  */
int32_t iis3dhhc_status_get(iis3dhhc_ctx_t *ctx, iis3dhhc_status_t *val)
{
  return iis3dhhc_read_reg(ctx, IIS3DHHC_STATUS, (uint8_t*) val, 1);
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
  * @brief   drdy_notification_mode: [set]  DRDY latched / pulsed, pulse
  *                                         duration is 1/4 ODR
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_drdy_pulse_t: change the values of drdy_pulse in
  *                               reg CTRL_REG1
  *
  */
int32_t iis3dhhc_drdy_notification_mode_set(iis3dhhc_ctx_t *ctx,
                                            iis3dhhc_drdy_pulse_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.drdy_pulse = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   drdy_notification_mode: [get]  DRDY latched / pulsed, pulse
  *                                         duration is 1/4 ODR
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_drdy_pulse_t: Get the values of drdy_pulse in
  *                               reg CTRL_REG1
  *
  */
int32_t iis3dhhc_drdy_notification_mode_get(iis3dhhc_ctx_t *ctx,
                                            iis3dhhc_drdy_pulse_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  *val = (iis3dhhc_drdy_pulse_t) reg.ctrl_reg1.drdy_pulse;

  return mm_error;
}

/**
  * @brief  int1_mode: [set]  It configures the INT1 pad as output for
  *                           FIFO flags or as external asynchronous
  *                           input trigger to FIFO.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_int1_ext_t: change the values of int1_ext in
  *                             reg INT1_CTRL
  *
  */
int32_t iis3dhhc_int1_mode_set(iis3dhhc_ctx_t *ctx, iis3dhhc_int1_ext_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  reg.int1_ctrl.int1_ext = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  int1_mode: [get]  It configures the INT1 pad as output
  *                           for FIFO flags or as external asynchronous
  *                           input trigger to FIFO.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_int1_ext_t: Get the values of int1_ext in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_int1_mode_get(iis3dhhc_ctx_t *ctx, iis3dhhc_int1_ext_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  *val = (iis3dhhc_int1_ext_t) reg.int1_ctrl.int1_ext;

  return mm_error;
}

/**
  * @brief   fifo_threshold_on_int1: [set]  FIFO watermark status
  *                                         on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int1_fth in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_fifo_threshold_on_int1_set(iis3dhhc_ctx_t *ctx,
                                            uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  reg.int1_ctrl.int1_fth = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   fifo_threshold_on_int1: [get]  FIFO watermark status
  *                                         on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int1_fth in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_fifo_threshold_on_int1_get(iis3dhhc_ctx_t *ctx,
                                            uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  *val = reg.int1_ctrl.int1_fth;

  return mm_error;
}

/**
  * @brief  fifo_full_on_int1: [set]  FIFO full flag on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int1_fss5 in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_fifo_full_on_int1_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  reg.int1_ctrl.int1_fss5 = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_full_on_int1: [get]  FIFO full flag on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int1_fss5 in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_fifo_full_on_int1_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  *val = reg.int1_ctrl.int1_fss5;

  return mm_error;
}

/**
  * @brief  fifo_ovr_on_int1: [set]  FIFO overrun interrupt on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int1_ovr in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_fifo_ovr_on_int1_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  reg.int1_ctrl.int1_ovr = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_ovr_on_int1: [get]  FIFO overrun interrupt on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int1_ovr in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_fifo_ovr_on_int1_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  *val = reg.int1_ctrl.int1_ovr;

  return mm_error;
}

/**
  * @brief  boot_on_int1: [set]  BOOT status on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int1_boot in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_boot_on_int1_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  reg.int1_ctrl.int1_boot = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  boot_on_int1: [get]  BOOT status on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int1_boot in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_boot_on_int1_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  *val = reg.int1_ctrl.int1_boot;

  return mm_error;
}

/**
  * @brief  drdy_on_int1: [set]  Data-ready signal on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int1_drdy in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_drdy_on_int1_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  reg.int1_ctrl.int1_drdy = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  drdy_on_int1: [get]  Data-ready signal on INT1 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int1_drdy in reg INT1_CTRL
  *
  */
int32_t iis3dhhc_drdy_on_int1_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT1_CTRL, &reg.byte, 1);
  *val = reg.int1_ctrl.int1_drdy;

  return mm_error;
}

/**
  * @brief   fifo_threshold_on_int2: [set]  FIFO watermark status
  *                                         on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int2_fth in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_fifo_threshold_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  reg.int2_ctrl.int2_fth = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief   fifo_threshold_on_int2: [get]  FIFO watermark status on
  *                                         INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int2_fth in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_fifo_threshold_on_int2_get(iis3dhhc_ctx_t *ctx,
                                            uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  *val = reg.int2_ctrl.int2_fth;

  return mm_error;
}

/**
  * @brief  fifo_full_on_int2: [set]  FIFO full flag on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int2_fss5 in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_fifo_full_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  reg.int2_ctrl.int2_fss5 = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_full_on_int2: [get]  FIFO full flag on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int2_fss5 in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_fifo_full_on_int2_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  *val = reg.int2_ctrl.int2_fss5;

  return mm_error;
}

/**
  * @brief  fifo_ovr_on_int2: [set]  FIFO overrun interrupt on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int2_ovr in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_fifo_ovr_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  reg.int2_ctrl.int2_ovr = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_ovr_on_int2: [get]  FIFO overrun interrupt on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int2_ovr in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_fifo_ovr_on_int2_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  *val = reg.int2_ctrl.int2_ovr;

  return mm_error;
}

/**
  * @brief  boot_on_int2: [set]  BOOT status on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int2_boot in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_boot_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  reg.int2_ctrl.int2_boot = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  boot_on_int2: [get]  BOOT status on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int2_boot in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_boot_on_int2_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  *val = reg.int2_ctrl.int2_boot;

  return mm_error;
}

/**
  * @brief  drdy_on_int2: [set]  Data-ready signal on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of int2_drdy in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_drdy_on_int2_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  reg.int2_ctrl.int2_drdy = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  drdy_on_int2: [get]  Data-ready signal on INT2 pin.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of int2_drdy in reg INT2_CTRL
  *
  */
int32_t iis3dhhc_drdy_on_int2_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_INT2_CTRL, &reg.byte, 1);
  *val = reg.int2_ctrl.int2_drdy;

  return mm_error;
}

/**
  * @brief  pin_mode: [set]  Push-pull/open drain selection on
  *                          interrupt pads.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_pp_od_t: change the values of pp_od in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_pin_mode_set(iis3dhhc_ctx_t *ctx, iis3dhhc_pp_od_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.pp_od = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  pin_mode: [get]  Push-pull/open drain selection on
  *                          interrupt pads.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_pp_od_t: Get the values of pp_od in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_pin_mode_get(iis3dhhc_ctx_t *ctx, iis3dhhc_pp_od_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  *val = (iis3dhhc_pp_od_t) reg.ctrl_reg4.pp_od;

  return mm_error;
}

/**
  * @}
  */

/**
  * @addtogroup  fifo
  * @brief   This section group all the functions concerning the
  *          fifo usage
  * @{
  */

/**
  * @brief  fifo: [set] FIFOenable.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of fifo_en in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_fifo_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  reg.ctrl_reg4.fifo_en = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo: [get] FIFOenable.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fifo_en in reg CTRL_REG4
  *
  */
int32_t iis3dhhc_fifo_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG4, &reg.byte, 1);
  *val = reg.ctrl_reg4.fifo_en;

  return mm_error;
}

/**
  * @brief  fifo_block_spi_hs: [set]  Enables the SPI high speed
                                      configuration for the FIFO block that
                                      is used to guarantee a minimum duration
                                      of the window in which writing operation
                                      of RAM output is blocked. This bit is
                                      recommended for SPI clock frequencies
                                      higher than 6 MHz.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of fifo_spi_hs_on in reg CTRL_REG5
  *
  */
int32_t iis3dhhc_fifo_block_spi_hs_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG5, &reg.byte, 1);
  reg.ctrl_reg5.fifo_spi_hs_on = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG5, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_block_spi_hs: [get]  Enables the SPI high speed configuration
                                      for the FIFO block that is used to
                                      guarantee a minimum duration of the
                                      window in which writing operation of
                                      RAM output is blocked.
                                      This bit is recommended for SPI
                                      clock frequencies higher than 6 MHz.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fifo_spi_hs_on in reg CTRL_REG5
  *
  */
int32_t iis3dhhc_fifo_block_spi_hs_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG5, &reg.byte, 1);
  *val = reg.ctrl_reg5.fifo_spi_hs_on;

  return mm_error;
}

/**
  * @brief  fifo_watermark: [set]  FIFO watermark level selection.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of fth in reg FIFO_CTRL
  *
  */
int32_t iis3dhhc_fifo_watermark_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_FIFO_CTRL, &reg.byte, 1);
  reg.fifo_ctrl.fth = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_FIFO_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_watermark: [get]  FIFO watermark level selection.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fth in reg FIFO_CTRL
  *
  */
int32_t iis3dhhc_fifo_watermark_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_FIFO_CTRL, &reg.byte, 1);
  *val = reg.fifo_ctrl.fth;

  return mm_error;
}

/**
  * @brief  fifo_mode: [set]  FIFO mode selection.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_fmode_t: change the values of fmode in reg FIFO_CTRL
  *
  */
int32_t iis3dhhc_fifo_mode_set(iis3dhhc_ctx_t *ctx, iis3dhhc_fmode_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_FIFO_CTRL, &reg.byte, 1);
  reg.fifo_ctrl.fmode = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_FIFO_CTRL, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  fifo_mode: [get]  FIFO mode selection.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_fmode_t: Get the values of fmode in reg FIFO_CTRL
  *
  */
int32_t iis3dhhc_fifo_mode_get(iis3dhhc_ctx_t *ctx, iis3dhhc_fmode_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_FIFO_CTRL, &reg.byte, 1);
  *val = (iis3dhhc_fmode_t) reg.fifo_ctrl.fmode;

  return mm_error;
}

/**
  * @brief  fifo_status: [get]  FIFO status register.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  iis3dhhc_fifo_src_t: Registers FIFO_SRC
  *
  */
int32_t iis3dhhc_fifo_status_get(iis3dhhc_ctx_t *ctx, iis3dhhc_fifo_src_t *val)
{
  return iis3dhhc_read_reg(ctx, IIS3DHHC_FIFO_SRC, (uint8_t*) val, 1);
}

/**
  * @brief  fifo_full_flag: [get]  FIFO stored data level.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fss in reg FIFO_SRC
  *
  */
int32_t iis3dhhc_fifo_full_flag_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_FIFO_SRC, &reg.byte, 1);
  *val = reg.fifo_src.fss;

  return mm_error;
}

/**
  * @brief  fifo_ovr_flag: [get]  FIFO overrun status flag.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of ovrn in reg FIFO_SRC
  *
  */
int32_t iis3dhhc_fifo_ovr_flag_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_FIFO_SRC, &reg.byte, 1);
  *val = reg.fifo_src.ovrn;

  return mm_error;
}

/**
  * @brief  fifo_fth_flag: [get]  FIFO watermark status.
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of fth in reg FIFO_SRC
  *
  */
int32_t iis3dhhc_fifo_fth_flag_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_FIFO_SRC, &reg.byte, 1);
  *val = reg.fifo_src.fth;

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
  * @brief  auto_add_inc: [set]  Register address automatically
  *                              incremented during a multiple byte access
  *                              with a serial interface (I2C or SPI).
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t val: change the values of if_add_inc in reg CTRL_REG1
  *
  */
int32_t iis3dhhc_auto_add_inc_set(iis3dhhc_ctx_t *ctx, uint8_t val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  reg.ctrl_reg1.if_add_inc = val;
  mm_error = iis3dhhc_write_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);

  return mm_error;
}

/**
  * @brief  auto_add_inc: [get]  Register address automatically incremented
  *                              during a multiple byte access with a serial
  *                              interface (I2C or SPI).
  *
  * @param  iis3dhhc_ctx_t *ctx: read / write interface definitions
  * @param  uint8_t: change the values of if_add_inc in reg CTRL_REG1
  *
  */
int32_t iis3dhhc_auto_add_inc_get(iis3dhhc_ctx_t *ctx, uint8_t *val)
{
  iis3dhhc_reg_t reg;
  int32_t mm_error;

  mm_error = iis3dhhc_read_reg(ctx, IIS3DHHC_CTRL_REG1, &reg.byte, 1);
  *val = reg.ctrl_reg1.if_add_inc;

  return mm_error;
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/