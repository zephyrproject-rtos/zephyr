/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "fpga_rs_xcb.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#define LOG_LEVEL CONFIG_FPGA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rs_fpga_fcb);

#if DT_HAS_COMPAT_STATUS_OKAY(rapidsi_fcb)
  #define DT_DRV_COMPAT rigel_fcb
  static volatile struct rigel_fcb_registers *s_Rigel_FCB_Registers = NULL;
  static uint32_t wordline_read_count = 0;
#else
  #error Rapid Silicon FCB IP is not enabled in the Device Tree
#endif

enum FPGA_status fcb_get_status(const struct device *dev)
{
  const struct fcb_data *lvData = (struct fcb_data*)dev->data;
  return lvData->fpgaStatus;
}

int fcb_session_start(const struct device *dev, struct fpga_ctx *ctx)
{
  return xCB_SUCCESS;
}

int fcb_session_free(const struct device *dev)
{
  return xCB_SUCCESS;
}

int fcb_load(const struct device *dev, uint32_t *image_ptr,
			     uint32_t img_size)
{
  return xCB_SUCCESS;
}

int fcb_reset(const struct device *dev)
{
  return xCB_SUCCESS;
}

int fcb_on(const struct device *dev)
{
  return -ENOSYS;
}

int fcb_off(const struct device *dev)
{
  return -ENOSYS;
}

const char *fcb_get_info(const struct device *dev)
{
  static struct fpga_transfer_param lvFCBTransferParam = {0};
  struct rigel_fcb_bitstream_header *lvFCBBitstrHeader = \
    (struct rigel_fcb_bitstream_header *)((struct fcb_data*)(dev->data));

  if(lvFCBBitstrHeader != NULL) {
    lvFCBTransferParam.FPGA_Transfer_Type = \
      lvFCBBitstrHeader->readback ? FPGA_TRANSFER_TYPE_RX : FPGA_TRANSFER_TYPE_TX;
    lvFCBTransferParam.FCB_Transfer_Block_Size = lvFCBBitstrHeader->bitline_reg_width;
    lvFCBTransferParam.FCB_Bitstream_Size = lvFCBBitstrHeader->generic_hdr.payload_size;
  } else {
    return NULL;
  }

  return (char*)&lvFCBTransferParam;
}

static struct fpga_driver_api rigel_fcb_api = {
  .get_status = fcb_get_status,
  .get_info = fcb_get_info,
  .load = fcb_load,
  .off = fcb_off,
  .on = fcb_on,
  .reset = fcb_reset,
  .session_free = fcb_session_free,
  .session_start = fcb_session_start
};

static struct fcb_data s_fcb_data = {
  .ctx = NULL,
  .fpgaStatus = FPGA_STATUS_INACTIVE
};

static struct fcb_config s_fcb_config = {
  .base = DT_REG_ADDR(DT_NODELABEL(fcb))
};

static int fcb_init(const struct device *dev)
{
  s_Rigel_FCB_Registers = ((struct rigel_fcb_registers *)((struct fcb_config*)dev->config));
  struct fcb_data *lvData = (struct fcb_data*)dev->data;
  if(s_Rigel_FCB_Registers != NULL) {
    lvData->fpgaStatus = FPGA_STATUS_ACTIVE;
  }  
  return xCB_SUCCESS;
}


DEVICE_DT_DEFINE(
                    DT_NODELABEL(fcb), 
                    fcb_init, /*Put Init func here*/
                    NULL, // Power Management
                    &s_fcb_data, // Data structure
                    &s_fcb_config, // Configuration Structure
                    POST_KERNEL,
                    CONFIG_RS_XCB_INIT_PRIORITY, 
                    &rigel_fcb_api // Driver API
                )
