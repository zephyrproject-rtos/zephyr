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

#define CONFIGURE_PCB_DATA 0x004
#define CONFIGURE_PCB_PARITY_DATA 0x005

/* ******************************************
 * @brief: PCB_Bitstream_Header_Parser
 * interface will parse the header of PCB
 * config controller type.
 *
 * @param [in]: inBitStream is a pointer to
 * bitstream.
 * @param [out]: outHeader contains the
 * parsed header information.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE PCB_Bitstream_Header_Parser(void *inBitStream,
                                                void *outHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  struct rs_pcb_bitstream_header *lvBitStream_Header =
      (struct rs_pcb_bitstream_header *)outHeader;
  uint32_t *lvBitstream = (uint32_t *)inBitStream;

  if ((inBitStream == NULL) || (outHeader == NULL)) {
    lvErrorCode = xCB_ERROR_NULL_POINTER;
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Copy generic header as it is
    memcpy((void *)lvBitStream_Header, (void *)lvBitstream,
           sizeof(struct rs_action_header));

    // Parsing the bitfields of the third word of the bitstream header
    lvBitStream_Header->Total_Nr_of_RAM_Blocks =
        (lvBitstream[RS_PCB_HDR_THIRD_WORD_OFFSET] &
         RS_PCB_HDR_NR_RAM_BLOCKS_MASK) >>
        (RS_PCB_HDR_NR_RAM_BLOCKS_OFFSET);
    lvBitStream_Header->Skew =
        (lvBitstream[RS_PCB_HDR_THIRD_WORD_OFFSET] & RS_PCB_HDR_SKEW_MASK) >>
        (RS_PCB_HDR_SKEW_OFFSET);
    lvBitStream_Header->parity =
        (lvBitstream[RS_PCB_HDR_THIRD_WORD_OFFSET] & RS_PCB_HDR_PARITY_MASK) >>
        (RS_PCB_HDR_PARITY_OFFSET);
    lvBitStream_Header->even =
        (lvBitstream[RS_PCB_HDR_THIRD_WORD_OFFSET] & RS_PCB_HDR_EVEN_MASK) >>
        (RS_PCB_HDR_EVEN_OFFSET);
    lvBitStream_Header->split =
        (lvBitstream[RS_PCB_HDR_THIRD_WORD_OFFSET] & RS_PCB_HDR_SPLIT_MASK) >>
        (RS_PCB_HDR_SPLIT_OFFSET);

    // Parsing the PL_SELECT Register
    lvBitStream_Header->PL_SELECT =
        *(struct rs_pcb_pl_select
              *)&lvBitstream[RS_PCB_HDR_PL_SELECT_WORD_OFFSET];

    // Parsing the PL_ROW Register
    lvBitStream_Header->PL_ROW =
        *(struct rs_pcb_pl_row *)&lvBitstream[RS_PCB_HDR_PL_ROW_WORD_OFFSET];

    // Parsing the PL_COL Register
    lvBitStream_Header->PL_COL =
        *(struct rs_pcb_pl_col *)&lvBitstream[RS_PCB_HDR_PL_COL_WORD_OFFSET];

    // Parsing the Bitfields of the Seventh word of bitstream header
    lvBitStream_Header->parity_nibble_rw =
        (lvBitstream[RS_PCB_HDR_SEVENTH_WORD_OFFSET] &
         RS_PCB_HDR_PARITY_NIBBLE_RW_MASK) >>
        (RS_PCB_HDR_PARITY_NIBBLE_RW_OFFSET);
  }

  LOG_DBG(
			"\n ...... PCB_HDR ...... \r\nBitStr_Size:%d \r\nNr_Blocks:%d "
			"\r\nSkew:%d \r\nParity:%d \r\nEven:%d \r\nsplit:%d "
			"\r\nPL_SELECT:0x%x \r\nPL_ROW:0x%x \r\nPL_COL:0x%x "
			"\r\nParity_nibble_rw:0x%x\r\n",
			lvBitStream_Header->generic_hdr.payload_size,
			lvBitStream_Header->Total_Nr_of_RAM_Blocks,
			lvBitStream_Header->Skew, lvBitStream_Header->parity,
			lvBitStream_Header->even, lvBitStream_Header->split,
			*(unsigned int *)&lvBitStream_Header->PL_SELECT,
			*(unsigned int *)&lvBitStream_Header->PL_ROW,
			*(unsigned int *)&lvBitStream_Header->PL_COL,
			(unsigned int)lvBitStream_Header->parity_nibble_rw
		);

  if (lvErrorCode != xCB_SUCCESS) {
    PRINT_ERROR(lvErrorCode);
  }

  return lvErrorCode;
}

/* ****************************************************
 * @brief: The following function will perform a timing
 * calibration on the PL data bus to determine the
 * proper number of wait states for the current
 * APB clock speed.
 *
 * @return: xCB_ERROR_CODE
 * ****************************************************/
static enum xCB_ERROR_CODE RS_PCB_Start_Wait_Cycles_Calibration(void) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_pcb_pl_stat lv_pcb_pl_stat = {0};

  lv_pcb_pl_stat.calib_done = 0;
  lv_pcb_pl_stat.calib = xCB_ENABLE;  // Enable the calibration process
  lv_pcb_pl_stat.cal_wait = 0;

  // Write to Register
  REG_WRITE_32((void *)&s_PCB_Registers->Pl_Stat, (void *)&lv_pcb_pl_stat);

  k_usleep(RS_PCB_WAIT_STATE_CALIB_TIME);

  // Check if calibration is done
  if (s_PCB_Registers->Pl_Stat.calib_done != xCB_SET) {
    lvErrorCode = xCB_ERROR_TIMEOUT;
  }

  if (lvErrorCode != xCB_SUCCESS) {
    PRINT_ERROR(lvErrorCode);
  }

  return lvErrorCode;
}

/* ****************************************************
 * @brief: FCB_Config_Registers performs configuration
 * of PCB registers PL_CTL, PL_SELECT, PL_ROW, PL_COL
 * and PL_EXTRA.
 *
 * @param [in]: FCB_Bitstream_Header extracted from
 * the bitstream BOP.
 * ****************************************************/
static enum xCB_ERROR_CODE RS_PCB_Config_Registers(
    struct rs_pcb_bitstream_header *inHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_pcb_pl_ctl lv_pcb_pl_ctl = {0};

  // Perform Wait Cycles Calibration
  lvErrorCode = RS_PCB_Start_Wait_Cycles_Calibration();

  if (lvErrorCode == xCB_SUCCESS) {
    lv_pcb_pl_ctl = s_PCB_Registers->Pl_Ctl;
    if (lv_pcb_pl_ctl.clock_wait_cycles !=
        s_PCB_Registers->Pl_Ctl.clock_wait_cycles) {
      lvErrorCode = xCB_ERROR_READ_ERROR;
    } else {
      // Writing wait cycles of APB data bus
      lv_pcb_pl_ctl.clock_wait_cycles = s_PCB_Registers->Pl_Stat.cal_wait;

      // Set the Enable bit in PL_CTL register
      lv_pcb_pl_ctl.enable = xCB_SET;

      // Enable the A_INC (auto increment bit)
      lv_pcb_pl_ctl.auto_increment = xCB_ENABLE;

      // If more than one Ram blocks to be used, Set S_INC to xCB_ENABLE
      if (inHeader->Total_Nr_of_RAM_Blocks > 1) {
        lv_pcb_pl_ctl.select_increment = xCB_ENABLE;
      }

      lv_pcb_pl_ctl.even = inHeader->even;
      lv_pcb_pl_ctl.parity = inHeader->parity;
      lv_pcb_pl_ctl.skew_control = inHeader->Skew;
      lv_pcb_pl_ctl.split_bits = inHeader->split;

      // Write rest of the bit fields of the pl_ctl register
      REG_WRITE_32((void *)&s_PCB_Registers->Pl_Ctl, (void *)&lv_pcb_pl_ctl);

      if ((s_PCB_Registers->Pl_Ctl.even != inHeader->even) ||
          (s_PCB_Registers->Pl_Ctl.parity != inHeader->parity) ||
          (s_PCB_Registers->Pl_Ctl.skew_control != inHeader->Skew) ||
          (s_PCB_Registers->Pl_Ctl.split_bits != inHeader->split) ||
          (s_PCB_Registers->Pl_Ctl.clock_wait_cycles !=
           s_PCB_Registers->Pl_Stat.cal_wait) ||
          (s_PCB_Registers->Pl_Ctl.enable != xCB_SET) ||
          (s_PCB_Registers->Pl_Ctl.auto_increment != xCB_ENABLE)) {
        lvErrorCode = xCB_ERROR_WRITE_ERROR;
      }
    }

    if ((s_PCB_Registers->Pl_Ctl.clock_wait_cycles !=
         lv_pcb_pl_ctl.clock_wait_cycles) ||
        (s_PCB_Registers->Pl_Ctl.enable != lv_pcb_pl_ctl.enable) ||
        (s_PCB_Registers->Pl_Ctl.auto_increment !=
         lv_pcb_pl_ctl.auto_increment) ||
        (s_PCB_Registers->Pl_Ctl.select_increment !=
         lv_pcb_pl_ctl.select_increment)) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    REG_WRITE_32((void *)&s_PCB_Registers->Pl_Select,
                 (void *)&inHeader->PL_SELECT);
    if ((s_PCB_Registers->Pl_Select.col != inHeader->PL_SELECT.col) ||
        (s_PCB_Registers->Pl_Select.row != inHeader->PL_SELECT.row) ||
        (s_PCB_Registers->Pl_Select.offset != inHeader->PL_SELECT.offset)) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    REG_WRITE_32((void *)&s_PCB_Registers->Pl_Row, (void *)&inHeader->PL_ROW);
    if ((s_PCB_Registers->Pl_Row.offset != inHeader->PL_ROW.offset) ||
        (s_PCB_Registers->Pl_Row.stride != inHeader->PL_ROW.stride)) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    REG_WRITE_32((void *)&s_PCB_Registers->Pl_Col, (void *)&inHeader->PL_COL);
    if ((inHeader->PL_COL.offset != s_PCB_Registers->Pl_Col.offset) ||
        (inHeader->PL_COL.stride != s_PCB_Registers->Pl_Col.stride)) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Writing Parity bits to PL_EXTRA register
    struct rs_pcb_pl_extra lv_pcb_pl_extra;
    lv_pcb_pl_extra.parity_nibble_rw = inHeader->parity_nibble_rw;
    REG_WRITE_32((void *)&s_PCB_Registers->Pl_Extra, (void *)&lv_pcb_pl_extra);
    if (s_PCB_Registers->Pl_Extra.parity_nibble_rw !=
        inHeader->parity_nibble_rw) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
	PRINT_ERROR(lvErrorCode);
    lvErrorCode = xCB_ERROR_PCB_CONF_FAILED;
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: PCB_Config_Begin interface
 * is used to perform PCB registers settings
 * before kicking off the payload transfer.
 *
 * @param [in]: rs_pcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE PCB_Config_Begin(
    struct rs_pcb_bitstream_header *inHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  lvErrorCode = RS_PCB_Config_Registers(inHeader);

  return lvErrorCode;
}

/* ******************************************
 * @brief: PCB_Config_End interface
 * is used to perform PCB registers settings
 * after the payload transfer.
 *
 * @param [in]: rs_pcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE PCB_Config_End(
    struct rs_pcb_bitstream_header *inHeader) {
  (void)inHeader;
  return xCB_SUCCESS;
}

/* ********************************************
 * @brief: This function Kicks off the payload 
 * transfer
 *
 * @param [in]: dev device structure
 * @param [in]: image_ptr to bitstream
 * @param [in]: img_size is size of the data
 * pointed to by image_ptr in bytes
 *
 * @return: error
 * ********************************************/
int pcb_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
  int error = 0;

  struct pcb_data *lvData = (struct pcb_data *)dev->data;

  if ((lvData->pcb_header.generic_hdr.action_enum & RS_ACTION_CMD_MASK) ==
    CONFIGURE_PCB_DATA) {

    lvData->ctx->dest_addr = (uint8_t*)&s_PCB_Registers->Pl_Targ.value;

    // Perform the data transfer (FILL or READBACK)
    error = lvData->ctx->bitstr_load_hndlr(lvData->ctx);

  } else if ((lvData->pcb_header.generic_hdr.action_enum & RS_ACTION_CMD_MASK) ==
             CONFIGURE_PCB_PARITY_DATA) {
    if (lvData->pcb_header.parity) {
      error = xCB_ERROR_INVALID_BIT_VALUE;
    } else {
      const uint32_t data_count_in_words = (img_size / 4);  // Number of Words
      const uint32_t *data = (uint32_t *)lvData->ctx->src_addr;
      /*
        Since we Expect Parity Word for Each
        Data Word, The length of data should
        always be a multiple of two in this
        function.
      */
      if (data_count_in_words & 0x00000001) {
        error = xCB_ERROR_INVALID_DATA_LENGTH;
      }

      if(error == 0)
      {
        for (uint32_t i = 0; i < data_count_in_words; i += 2) {
          uint32_t lvParity = ((*data++) & RS_PCB_PL_EXTRA_PARITY_MASK);
          uint32_t lvDataWord = (*data++);
          s_PCB_Registers->Pl_Extra.parity_nibble_rw =
              lvParity;  // First we send parity
          s_PCB_Registers->Pl_Targ.value =
              lvDataWord;  // Then we send corresponding data word
        }
      }
    }
  } else {
    error = xCB_ERROR_UNEXPECTED_VALUE;
  }

  if (error != xCB_SUCCESS) {
    PRINT_ERROR(error);
    error = -ECANCELED;
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

	if(s_PCB_Registers == NULL) {
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
		lvPCBTransferParam.Transfer_Block_Size = lvPCBBitstrHeader->generic_hdr.payload_size;
		lvPCBTransferParam.Bitstream_Size = lvPCBBitstrHeader->generic_hdr.payload_size;
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

static const struct pcb_config s_pcb_config = {.base = DT_REG_ADDR(DT_NODELABEL(pcb))};

DEVICE_DT_DEFINE(DT_NODELABEL(pcb), pcb_init, /*Put Init func here*/
		 NULL,                        // Power Management
		 &s_pcb_data,                 // Data structure
		 &s_pcb_config,               // Configuration Structure
		 POST_KERNEL, CONFIG_RS_XCB_INIT_PRIORITY,
		 &s_pcb_api // Driver API
)
