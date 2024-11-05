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
LOG_MODULE_REGISTER(rs_fpga_icb);

#if DT_HAS_COMPAT_STATUS_OKAY(rapidsi_icb)
#define DT_DRV_COMPAT rigel_icb
static volatile struct rs_icb_registers *s_ICB_Registers = NULL;
static volatile struct rs_icb_chain_lengths *s_ICB_Chain_Lengths = NULL;
#else
#error Rapid Silicon ICB IP is not enabled in the Device Tree
#endif

/* ******************************************
 * @brief: ICB_Bitstream_Header_Parser
 * interface will parse the header of ICB
 * config controller type.
 *
 * @param [in]: inBitStream is a pointer to
 * bitstream.
 * @param [out]: outHeader contains the
 * parsed header information.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE ICB_Bitstream_Header_Parser(void *inBitStream,
                                                   void *outHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_icb_bitstream_header *lvBitStream_Header =
      (struct rs_icb_bitstream_header *)outHeader;
  uint8_t *lvBitstream = (uint8_t *)inBitStream;

  if ((inBitStream == NULL) || (outHeader == NULL)) {
    lvErrorCode = xCB_ERROR_NULL_POINTER;
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Copy generic action header as it is and move data pointer ahead
    memcpy((void *)lvBitStream_Header, (void *)lvBitstream,
           sizeof(struct rs_action_header));

    lvBitstream += sizeof(struct rs_action_header);

    // Copy uncompressed original bitstream size
    lvBitStream_Header->bitstream_size = *(uint32_t *)lvBitstream;
    lvBitstream += sizeof(lvBitStream_Header->bitstream_size);

    // Read bitstream checksum if it exists
    if (lvBitStream_Header->generic_hdr.action_enum &
        RS_ACTION_CHECKSUM_PRESENT_MASK) {
      lvBitStream_Header->bitstream_checksum = *(uint32_t *)lvBitstream;
      lvBitstream += sizeof(lvBitStream_Header->bitstream_checksum);
      LOG_DBG("Extracted_Chksum:0x%08x lvBitstream:0x%08x\r\n",
                   lvBitStream_Header->bitstream_checksum, *lvBitstream);
    } else {
      lvBitStream_Header->bitstream_checksum = 0;
    }

    // Parsing the bitfields of the header
    lvBitStream_Header->cfg_cmd =
        (*(uint32_t *)lvBitstream & RS_ICB_HDR_CFG_CMD_MASK) >>
        RS_ICB_HDR_CFG_CMD_OFFSET;
    lvBitStream_Header->bit_twist =
        (*(uint32_t *)lvBitstream & RS_ICB_HDR_BIT_TWIST_MASK) >>
        RS_ICB_HDR_BIT_TWIST_OFFSET;
    lvBitStream_Header->byte_twist =
        (*(uint32_t *)lvBitstream & RS_ICB_HDR_BYTE_TWIST_MASK) >>
        RS_ICB_HDR_BYTE_TWIST_OFFSET;
    lvBitStream_Header->update =
        (*(uint32_t *)lvBitstream & RS_ICB_HDR_UPDATE_MASK) >>
        RS_ICB_HDR_UPDATE_OFFSET;
    lvBitStream_Header->capture =
        (*(uint32_t *)lvBitstream & RS_ICB_HDR_CAPTURE_MASK) >>
        RS_ICB_HDR_CAPTURE_OFFSET;
    // This bit suggests whether the packet is of command or data
    lvBitStream_Header->cmd_data =
        (*(uint32_t *)lvBitstream & RS_ICB_HDR_CMD_DATA_MASK) >>
        RS_ICB_HDR_CMD_DATA_OFFSET;
  }

  LOG_DBG(
            "\n------ ICB_HDR ------ \r\nChksum:0x%x \r\ncapture:%lu "
            "\r\ncfg_cmd:%lu \r\nchksum_prsnt:%lu "
            "\r\ncmd_data:%lu \r\npayload_len:%lu (words) \r\nupdate:%lu\r\n",
            lvBitStream_Header->bitstream_checksum, lvBitStream_Header->capture,
            lvBitStream_Header->cfg_cmd,
            (lvBitStream_Header->generic_hdr.action_enum &
            RS_ACTION_CHECKSUM_PRESENT_MASK) >>
                RS_ACTION_CHECKSUM_PRESENT_OFFSET,
            lvBitStream_Header->cmd_data,
            lvBitStream_Header->generic_hdr.payload_size / xCB_BYTES_IN_A_WORD,
            lvBitStream_Header->update
        );

  if (lvErrorCode != xCB_SUCCESS) {
    LOG_ERR("%s(%d) Error Code %d\r\n", __func__, __LINE__, lvErrorCode);
  }

  return lvErrorCode;
}

/********************************************************************************
 * @brief: This function performs soft_reset to ICB IOB and PLLs
 ********************************************************************************/
static enum xCB_ERROR_CODE rs_icb_soft_reset(void) {
  struct rs_icb_soft_reset lv_soft_reset = {0};
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  lv_soft_reset.value = xCB_RESET;  // Assert Reset
  REG_WRITE_32((void *)&s_ICB_Registers->Soft_Reset, (void *)&lv_soft_reset);
  lv_soft_reset.value = xCB_SET;  // De-Assert Reset
  REG_WRITE_32((void *)&s_ICB_Registers->Soft_Reset, (void *)&lv_soft_reset);

  LOG_DBG("**** ICB_Soft_Reset Asserted ****\r\n");

  lv_soft_reset.value = s_ICB_Registers->Soft_Reset.value;

  if (lv_soft_reset.value != xCB_SET) {
    lvErrorCode = xCB_ERROR_WRITE_ERROR;
    LOG_ERR("%s(%d) ErrorCode:%d\r\n", __func__, __LINE__, lvErrorCode);
  }

  return lvErrorCode;
}

/********************************************************************************
 * @brief: This function returns Shift Status of ICB Payload
 * return: ICB_SHIFT_STATUS
 ********************************************************************************/
static enum RS_ICB_SHIFT_STATUS RS_ICB_busy(void) {
  return s_ICB_Registers->Shift_Status.fsm_state;
}

/********************************************************************************
 * @brief: This function performs configuration for writing the
 * bitstream to ICB. The steps to do configuration are:
 * 1 - Setup the Configuration mode (CFG_CMD )
 * 2 - If the checksum mode is used, set the calculated checksum (CHKSUM_WORD).
 * 3 - Setup chains number;
 *		▪ Write the lengths of configuration chain in (CHAIN_LENGTH_0)
 * 4 - Set CFG_KICKOFF to one. The controller is ready to work
 *
 * @Param [in]: RS_ICB_Bitstream_Header.
 * @return: xCB_ERROR_CODE
 ********************************************************************************/
static enum xCB_ERROR_CODE RS_ICB_Config_Registers(
    struct rs_icb_bitstream_header *inHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_icb_cfg_cmd lv_Cfg_Cmd = {0};
  struct rs_icb_op_config lv_Op_Cfg = {0};

  if (inHeader != NULL) {
    lv_Cfg_Cmd.cfg_mode = inHeader->cfg_cmd;
    // Setting up the configuration mode
    REG_WRITE_32((void *)&s_ICB_Registers->Cfg_Cmd, (void *)&lv_Cfg_Cmd);
    if (s_ICB_Registers->Cfg_Cmd.cfg_mode != inHeader->cfg_cmd) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    } else {
      LOG_DBG("Written cfg_cmd:0x%x\r\n",
                   s_ICB_Registers->Cfg_Cmd.cfg_mode);
    }
  } else {
    lvErrorCode = xCB_ERROR_NULL_POINTER;
  }

  // Setting bit/byte_twist bits
  if (lvErrorCode == xCB_SUCCESS) {
    lv_Op_Cfg.bit_twist = inHeader->bit_twist;
    lv_Op_Cfg.byte_twist = inHeader->byte_twist;
    REG_WRITE_32((void *)&s_ICB_Registers->Op_Config, (void *)&lv_Op_Cfg);
    if ((s_ICB_Registers->Op_Config.bit_twist != inHeader->bit_twist) ||
        (s_ICB_Registers->Op_Config.byte_twist != inHeader->byte_twist)) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    } else {
      LOG_DBG("Written Op_Cfg Bit_Twist:0x%x Byte_Twist:0x%x\r\n",
                   s_ICB_Registers->Op_Config.bit_twist,
                   s_ICB_Registers->Op_Config.byte_twist);
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Setup Checksum register
    if (inHeader->generic_hdr.action_enum & RS_ACTION_CHECKSUM_PRESENT_MASK) {
      REG_WRITE_32((void *)&s_ICB_Registers->Chksum_Word,
                   (void *)&inHeader->bitstream_checksum);
      if (s_ICB_Registers->Chksum_Word.value != inHeader->bitstream_checksum) {
        lvErrorCode = xCB_ERROR_WRITE_ERROR;
      } else {
        LOG_DBG("Written Chksum_Word:0x%x\r\n",
                     (unsigned int)s_ICB_Registers->Chksum_Word.value);
      }
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    uint32_t lv_Payload_Length = inHeader->bitstream_size * xCB_BITS_IN_A_BYTE;
    // Setting up chain length register value
    // For now there is only a single chain length register in ICB
    REG_WRITE_32((void *)&s_ICB_Chain_Lengths->Chain_Length_Reg[0],
                 (void *)&lv_Payload_Length);
    if (s_ICB_Chain_Lengths->Chain_Length_Reg[0] != lv_Payload_Length) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    } else {
      LOG_DBG("Written payload_length:0x%x bits @ %p\r\n",
                   (unsigned int)s_ICB_Chain_Lengths->Chain_Length_Reg[0],
                   (void *)&s_ICB_Chain_Lengths->Chain_Length_Reg[0]);
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    lvErrorCode = xCB_ERROR_ICB_CONF_FAILED;
    LOG_ERR("%s(%d) ErrorCode:%d\r\n", __func__, __LINE__, lvErrorCode);
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: ICB_Config_Begin interface
 * is used to perform ICB registers settings
 * before kicking off the payload transfer.
 *
 * @param [in]: rs_icb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE ICB_Config_Begin(struct rs_icb_bitstream_header *inHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_icb_cfg_done lv_Cfg_Done = {0};
  struct rs_icb_cfg_kickoff lv_Cfg_Kickoff = {0};
  struct rs_icb_cmd_control lv_Cmd_Control = {0};
  static bool icb_soft_reset_done = false;

  if ((inHeader->cfg_cmd < RS_ICB_CNF_READ_POSTCHKSUM_MODE) &&
      (!icb_soft_reset_done)) {
    lvErrorCode = rs_icb_soft_reset();  // This needs to be done only once at
                                        // system boot up
    if (lvErrorCode == xCB_SUCCESS) {
      icb_soft_reset_done = true;
    }
  }

  lvErrorCode = RS_ICB_Config_Registers(inHeader);

  if (lvErrorCode == xCB_SUCCESS) {
    // Read command control register
    lv_Cmd_Control = s_ICB_Registers->Cmd_Control;
    // Modify command control register
    lv_Cmd_Control.cmd_data = inHeader->cmd_data;
    // Write command control register
    REG_WRITE_32((void *)&s_ICB_Registers->Cmd_Control,
                 (void *)&lv_Cmd_Control);
    if (s_ICB_Registers->Cmd_Control.cmd_data != inHeader->cmd_data) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    } else {
      LOG_DBG("Cmd_Ctl (cmd or data):0x%x\r\n",
                   s_ICB_Registers->Cmd_Control.cmd_data);
    }
  }

  // Clearing the config_done before setting config_kickoff
  if (lvErrorCode == xCB_SUCCESS) {
    lv_Cfg_Done.value = xCB_RESET;
    REG_WRITE_32((void *)&s_ICB_Registers->Cfg_Done, (void *)&lv_Cfg_Done);
    if (s_ICB_Registers->Cfg_Done.value != xCB_RESET) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    } else {
      LOG_DBG("Cleared cfg_done:0x%x\r\n",
                   (unsigned int)s_ICB_Registers->Cfg_Done.value);
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    lv_Cfg_Kickoff.value = xCB_ENABLE;
    REG_WRITE_32((void *)&s_ICB_Registers->Cfg_KickOff,
                 (void *)&lv_Cfg_Kickoff);

    if (s_ICB_Registers->Cfg_KickOff.value != xCB_ENABLE) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    } else {
      LOG_DBG("Enablded Kick-off:0x%x\r\n",
                   (unsigned int)s_ICB_Registers->Cfg_KickOff.value);
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    LOG_ERR("%s(%d) ErrorCode:%d\r\n", __func__, __LINE__, lvErrorCode);
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: RS_ICB_Config_End interface
 * is used to perform ICB registers settings
 * after the payload transfer.
 *
 * @param [in]: rs_icb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_ICB_Config_End(
    struct rs_icb_bitstream_header *inHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_icb_cmd_control lv_Cmd_Control = {0};

  REG_WRITE_32((void *)&lv_Cmd_Control, /* Initialize CMD_CTL local copy */
               (void *)&s_ICB_Registers->Cmd_Control);

  // Check if checksum matched in case checksum related transfer was used
  if ((inHeader->generic_hdr.action_enum & RS_ACTION_CHECKSUM_PRESENT_MASK) &&
      (s_ICB_Registers->Chksum_Status.value != xCB_SET)) {
    lvErrorCode = xCB_ERROR_CHECKSUM_MATCH_FAILED;
    LOG_DBG("Checksum: 0x%x Status:0x%x\r\n",
                 (unsigned int)s_ICB_Registers->Chksum_Word.value,
                 (unsigned int)s_ICB_Registers->Chksum_Status.value);
  } else {
    LOG_DBG("**** Checksum Matched ****\r\n");
  }

  if (lvErrorCode == xCB_SUCCESS) {
    if (s_ICB_Registers->Cfg_KickOff.value != xCB_DISABLE) {
      lvErrorCode = xCB_ERROR_ICB_TRANSFER_FAILED;
    }
  }

  // In case everything went ok and writes were made, check if config_done is
  // set.
  if ((lvErrorCode == xCB_SUCCESS) &&
      (inHeader->cfg_cmd < RS_ICB_CNF_READ_POSTCHKSUM_MODE)) {
    LOG_DBG("Check if config done is set\r\n");
    if (s_ICB_Registers->Cfg_Done.value != xCB_SET) {
      lvErrorCode = xCB_ERROR_UNEXPECTED_VALUE;
    } else {
      LOG_DBG("**** Config done is set ****\r\n");
    }
  }

  // In case capture bit is set in header, then the
  // appropriate bit is given high to low pulse in command_control register.
  if (lvErrorCode == xCB_SUCCESS) {
    if (inHeader->capture) {
      LOG_DBG("Triggering capture pulse\r\n");
      lv_Cmd_Control.capture_control = xCB_SET;
      REG_WRITE_32((void *)&s_ICB_Registers->Cmd_Control,
                   (void *)&lv_Cmd_Control);
      if (s_ICB_Registers->Cmd_Control.capture_control != xCB_SET) {
        lvErrorCode = xCB_ERROR_WRITE_ERROR;
      } else {
        delay_us(RS_ICB_CAP_UPD_PULSE_DELAY);
        lv_Cmd_Control.capture_control = xCB_RESET;
        lv_Cmd_Control.cmd_data = xCB_RESET;
        REG_WRITE_32((void *)&s_ICB_Registers->Cmd_Control,
                     (void *)&lv_Cmd_Control);
        if ((s_ICB_Registers->Cmd_Control.capture_control != xCB_RESET) ||
            (s_ICB_Registers->Cmd_Control.cmd_data != xCB_RESET)) {
          lvErrorCode = xCB_ERROR_WRITE_ERROR;
        } else {
          LOG_DBG("Sent capture pulse\r\n");
        }
      }
    }
  }

  // In case update bit is set in header, then the
  // appropriate bit is given high to low pulse in command_control register.
  if (lvErrorCode == xCB_SUCCESS) {
    if (inHeader->update) {
      LOG_DBG("triggering update pulse\r\n");

      lv_Cmd_Control.update_control = xCB_SET;
      REG_WRITE_32((void *)&s_ICB_Registers->Cmd_Control,
                   (void *)&lv_Cmd_Control);
      if (s_ICB_Registers->Cmd_Control.update_control != xCB_SET) {
        lvErrorCode = xCB_ERROR_WRITE_ERROR;
      } else {
        delay_us(RS_ICB_CAP_UPD_PULSE_DELAY);
        lv_Cmd_Control.update_control = xCB_RESET;
        lv_Cmd_Control.cmd_data = xCB_RESET;
        REG_WRITE_32((void *)&s_ICB_Registers->Cmd_Control,
                     (void *)&lv_Cmd_Control);
        if ((s_ICB_Registers->Cmd_Control.update_control != xCB_RESET) ||
            (s_ICB_Registers->Cmd_Control.cmd_data != xCB_RESET)) {
          lvErrorCode = xCB_ERROR_WRITE_ERROR;
        } else {
          LOG_DBG("Sent update pulse\r\n");
        }
      }
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    LOG_ERR("%s(%d) ErrorCode:%d\r\n", __func__, __LINE__, lvErrorCode);
  }

  return lvErrorCode;
}

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
int icb_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
  if (inHeader->cfg_cmd < RS_ICB_CNF_READ_POSTCHKSUM_MODE) {
    rs_sec_tfr_ptr->transfer_addr.write_addr =
        (uint32_t)&s_ICB_Registers->Bitstream_wData;
    rs_sec_tfr_ptr->transfer_addr.tfr_type = RS_SECURE_TX;
    LOG_DBG("Starting RS_SECURE_TX\r\n");
  } else {
    rs_sec_tfr_ptr->transfer_addr.read_addr =
        (uint32_t)&s_ICB_Registers->Bitstream_rData;
    rs_sec_tfr_ptr->transfer_addr.tfr_type = RS_SECURE_RX;
    LOG_DBG("Starting RS_SECURE_RX\r\n");
  }

  if ((rs_sec_tfr_ptr->rs_secure_transfer((void *)rs_sec_tfr_ptr)) !=
      CRYPTO_SUCCESS) {
    lvErrorCode = xCB_ERROR;
  }

  while (RS_ICB_busy())
    ;

  return lvErrorCode;
}

enum FPGA_status icb_get_status(const struct device *dev)
{
	const struct icb_data *lvData = (struct icb_data *)dev->data;
	return lvData->fpgaStatus;
}

int icb_session_start(const struct device *dev, struct fpga_ctx *ctx)
{
	int error = xCB_SUCCESS;

	struct icb_data *lvData = (struct icb_data *)dev->data;

	lvData->ctx = ctx; // pointing to the fpga context for later operations.

	lvData->ctx->device = dev;

	if((s_ICB_Registers == NULL) || (s_ICB_Chain_Lengths == NULL)) {
		error = xCB_ERROR_NULL_POINTER;
	}

	if(error == xCB_SUCCESS)
	{    
		if (lvData->ctx->meta_data == NULL) { // ICB Meta Data is Expected
			error = xCB_ERROR_NULL_POINTER;
		} else {
			error = ICB_Bitstream_Header_Parser(lvData->ctx->meta_data,
						(void *)&lvData->icb_header);
		}
		lvData->ctx->meta_data_per_block = false;
	}

	if (error == xCB_SUCCESS) {
		error = ICB_Config_Begin(&lvData->icb_header);  
	}

	if(error != xCB_SUCCESS) {
		PRINT_ERROR(error);
		error = -ECANCELED;
	}

	return error;
}

int icb_session_free(const struct device *dev)
{
  int error = 0;

  struct icb_data *lvData = (struct icb_data *)dev->data;

  error = ICB_Config_End(&lvData->icb_header);

  s_ICB_Registers = NULL;

  lvData->fpgaStatus = FPGA_STATUS_INACTIVE;

  lvData->ctx = NULL;  

  lvData->ctx->device = NULL;

  if(error != xCB_SUCCESS) {
    error = -ECANCELED;
  }

  return error;
}

int icb_reset(const struct device *dev)
{
  int error = xCB_SUCCESS;

  #if CONFIG_RAPIDSI_OFE
	const struct device *ofe = DEVICE_DT_GET(DT_NODELABEL(ofe));
	if (ofe == NULL) {
		LOG_ERR("%s(%d) Error with OFE initialization\r\n", __func__, __LINE__);
		error = -ENOSYS;
	}
  #else
    #error "Enable OFE from the device tree to to meet ICB dependency."
  #endif

	// Sending the reset pulse to global fpga reset.
	if (error == 0) {
		if ((ofe_reset(ofe, OFE_RESET_SUBSYS_ICB, 0x0) != 0) || /* RESET */
		    (ofe_reset(ofe, OFE_RESET_SUBSYS_ICB, 0x1) != 0)) { /* SET */
			LOG_ERR("%s(%d) global fpga reset error\r\n", __func__, __LINE__);
			error = -EIO;
		}
	}

	return error;
}

static int icb_engine_on(bool value)
{
    int error = xCB_SUCCESS;

    #if CONFIG_RAPIDSI_OFE
        const struct device *ofe = DEVICE_DT_GET(DT_NODELABEL(ofe));
        if (ofe == NULL) {
            LOG_ERR("%s(%d) Error with OFE initialization\r\n", __func__, __LINE__);
            error = -ENOSYS;
        }
    #else
        #error "Enable OFE from the device tree to meet ICB dependency."
    #endif

	if (error == 0) {
		// Setting the isolation bit to allow / prohibit writing the fabric.
        // 1 to alow writing and 0 to prohibit writing to fabric.
		scu_set_isolation_ctrl(ISOLATION_CTRL_ICB_OFFSET, value);
	}

	// Sending the reset pulse to global fpga reset.
	if (error == 0) {
        if(value) {
            if ((ofe_reset(ofe, OFE_RESET_SUBSYS_ICB, 0x0) != 0) || /* RESET */
                (ofe_reset(ofe, OFE_RESET_SUBSYS_ICB, 0x1) != 0)) { /* SET to Release Reset */
                LOG_ERR("%s(%d) global fpga reset release error\r\n", __func__, __LINE__);
                error = -EIO;
            }
        } else {
            if(ofe_reset(ofe, OFE_RESET_SUBSYS_ICB, 0x0) != 0) { /* Held in RESET */
                LOG_ERR("%s(%d) global fpga reset held error\r\n", __func__, __LINE__);
                error = -EIO;
            }
        }
	}

  return error;
}

int icb_on(const struct device *dev)
{
  int error = icb_engine_on(true);
  return error;
}

int icb_off(const struct device *dev)
{
  int error = icb_engine_on(false);
  return error;
}

const char *icb_get_info(const struct device *dev)
{
	static struct fpga_transfer_param lvICBTransferParam = {0};
	struct rs_icb_bitstream_header *lvICBBitstrHeader =
		&((struct icb_data *)(dev->data))->icb_header;

	if (lvICBBitstrHeader != NULL) {
		lvICBTransferParam.FPGA_Transfer_Type = \
            lvBitStream_Header->cfg_cmd < RS_ICB_CFG_MODE_READBACK_AND_POST_CHKSUM \
            ? FPGA_TRANSFER_TYPE_TX \
            : FPGA_TRANSFER_TYPE_RX;
		lvICBTransferParam.ICB_Transfer_Block_Size = lvBitStream_Header->bitstream_size;
		lvICBTransferParam.ICB_Bitstream_Size = lvBitStream_Header->bitstream_size;
	} else {
		return NULL;
	}

	return (char *)&lvICBTransferParam;
}

static struct fpga_driver_api rigel_icb_api = {.get_status = icb_get_status,
					       .get_info = icb_get_info,
					       .load = icb_load,
					       .off = icb_off,
					       .on = icb_on,
					       .reset = icb_reset,
					       .session_free = icb_session_free,
					       .session_start = icb_session_start};

static int icb_init(const struct device *dev)
{
	int error = xCB_SUCCESS;

	s_ICB_Registers = ((struct rs_icb_registers *)((struct icb_config *)dev->config)->base);
    s_ICB_Chain_Lengths = ((struct rs_icb_chain_lengths *)(((struct icb_config *)dev->config)->base + 0x30));

	struct icb_data *lvData = (struct icb_data *)dev->data;

    error = icb_on(dev); // By default turn on the fabric to be written with bitstreams.

	if (error == 0) {
		if ((s_ICB_Registers != NULL) && (s_ICB_Chain_Lengths != NULL)) {
			lvData->fpgaStatus = FPGA_STATUS_ACTIVE;
		} else {
			LOG_ERR("%s(%d) ICB Register Cluster Initialized to NULL\r\n", __func__,
				__LINE__);
			error = -ENOSYS;
		}
	}

	return error;
}

static struct icb_data s_icb_data = {.ctx = NULL, .fpgaStatus = FPGA_STATUS_INACTIVE};

static struct icb_config s_icb_config = {.base = DT_REG_ADDR(DT_NODELABEL(icb))};

DEVICE_DT_DEFINE(DT_NODELABEL(icb), icb_init, /*Put Init func here*/
		 NULL,                        // Power Management
		 &s_icb_data,                 // Data structure
		 &s_icb_config,               // Configuration Structure
		 POST_KERNEL, CONFIG_RS_XCB_INIT_PRIORITY,
		 &rigel_icb_api // Driver API
)
