/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "fpga_rs_xcb.h"
#include <rapidsi_scu.h>

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/drivers/misc/rapidsi/rapidsi_ofe.h>

#define LOG_LEVEL CONFIG_FPGA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rs_fpga_pcb);

#if DT_HAS_COMPAT_STATUS_OKAY(rapidsi_pcb)
  #define DT_DRV_COMPAT rigel_pcb
  static volatile struct rs_pcb_registers *s_PCB_Registers = NULL;
#else
  #error Rapid Silicon PCB IP is not enabled in the Device Tree
#endif

/* ********************************************
 * @brief: This function Kicks off the payload 
 * transfer
 *
 * @param [in]: dev device structure
 * @param [in]: image_ptr to bitstream
 * @param [in]: img_size is size of the data
 * pointed to by image_ptr
 *
 * @return: error
 * ********************************************/
int pcb_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
  int error = 0;

  struct pcb_data *lvData = (struct pcb_data *)dev->data;

  // Perform the data transfer (FILL or READBACK)
  error = lvData->ctx->bitstr_load_hndlr(lvData->ctx);

  if (error != xCB_SUCCESS) {
    PRINT_ERROR(error);
    error = -ECANCELED;
  } else {
    while (PCB_busy());
  }

  return error;
}

enum FPGA_status pcb_get_status(const struct device *dev)
{
	const struct pcb_data *lvData = (struct pcb_data *)dev->data;
	return lvData->fpgaStatus;
}

int pcb_session_start(const struct device *dev, struct fpga_ctx *ctx)
{
	int error = xCB_SUCCESS;

	struct pcb_data *lvData = (struct pcb_data *)dev->data;

	lvData->ctx = ctx; // pointing to the fpga context for later operations.

	lvData->ctx->device = dev;

	if((s_PCB_Registers == NULL) || (s_PCB_Chain_Lengths == NULL)) {
		error = xCB_ERROR_NULL_POINTER;
	}

	if(error == xCB_SUCCESS)
	{    
		if (lvData->ctx->meta_data == NULL) { // PCB Meta Data is Expected
			error = xCB_ERROR_NULL_POINTER;
		} else {
			error = PCB_Bitstream_Header_Parser(lvData->ctx->meta_data,
						(void *)&lvData->pcb_header);
		}
		lvData->ctx->meta_data_per_block = false;
	}

	if (error == xCB_SUCCESS) {
		error = PCB_Config_Begin(&lvData->pcb_header);  
	}

	if(error != xCB_SUCCESS) {
		PRINT_ERROR(error);
		error = -ECANCELED;
	}

	return error;
}

int pcb_session_free(const struct device *dev)
{
  int error = 0;

  struct pcb_data *lvData = (struct pcb_data *)dev->data;

  error = PCB_Config_End(&lvData->pcb_header);

  s_PCB_Registers = NULL;

  lvData->fpgaStatus = FPGA_STATUS_INACTIVE;

  lvData->ctx = NULL;  

  lvData->ctx->device = NULL;

  if(error != xCB_SUCCESS) {
    PRINT_ERROR(error);
    error = -ECANCELED;
  }

  return error;
}

int pcb_reset(const struct device *dev)
{
	return -ENOSYS;
}

int pcb_on(const struct device *dev)
{
  return -ENOSYS;
}

int pcb_off(const struct device *dev)
{
  return -ENOSYS;
}

const char *pcb_get_info(const struct device *dev)
{
	static struct fpga_transfer_param lvPCBTransferParam = {0};
	struct rs_pcb_bitstream_header *lvPCBBitstrHeader =
		&((struct pcb_data *)(dev->data))->pcb_header;

	if (lvPCBBitstrHeader != NULL) {
		lvPCBTransferParam.FPGA_Transfer_Type = \
            lvPCBBitstrHeader->cfg_cmd < RS_PCB_CFG_MODE_READBACK_AND_POST_CHKSUM \
            ? FPGA_TRANSFER_TYPE_TX \
            : FPGA_TRANSFER_TYPE_RX;
		lvPCBTransferParam.Transfer_Block_Size = lvPCBBitstrHeader->bitstream_size;
		lvPCBTransferParam.Bitstream_Size = lvPCBBitstrHeader->bitstream_size;
	} else {
		return NULL;
	}

	return (char *)&lvPCBTransferParam;
}

static struct fpga_driver_api s_pcb_api = {.get_status = pcb_get_status,
					       .get_info = pcb_get_info,
					       .load = pcb_load,
					       .off = pcb_off,
					       .on = pcb_on,
					       .reset = pcb_reset,
					       .session_free = pcb_session_free,
					       .session_start = pcb_session_start};

static int pcb_init(const struct device *dev)
{
	int error = xCB_SUCCESS;

	s_PCB_Registers = ((struct rs_pcb_registers *)((struct pcb_config *)dev->config)->base);

	struct pcb_data *lvData = (struct pcb_data *)dev->data;

	if (error == 0) {
		if (s_PCB_Registers != NULL) {
			lvData->fpgaStatus = FPGA_STATUS_ACTIVE;
		} else {
			LOG_ERR("%s(%d) PCB Register Cluster Initialized to NULL\r\n", __func__,
				__LINE__);
			error = -ENOSYS;
		}
	}

	return error;
}

static struct pcb_data s_pcb_data = {.ctx = NULL, .fpgaStatus = FPGA_STATUS_INACTIVE};

static struct pcb_config s_pcb_config = {.base = DT_REG_ADDR(DT_NODELABEL(pcb))};

DEVICE_DT_DEFINE(DT_NODELABEL(pcb), pcb_init, /*Put Init func here*/
		 NULL,                        // Power Management
		 &s_pcb_data,                 // Data structure
		 &s_pcb_config,               // Configuration Structure
		 POST_KERNEL, CONFIG_RS_XCB_INIT_PRIORITY,
		 &s_pcb_api // Driver API
)
