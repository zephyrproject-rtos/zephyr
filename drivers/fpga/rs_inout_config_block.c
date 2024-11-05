/****************************************************************************************
 * File Name: 		inout_config_block.c
 * Author: 			  Muhammad Junaid Aslam
 * Organization:	Rapid Silicon
 * Repository: 		EmbeddedCommon-Dev
 * Branch:			  config_controllers
 *
 * @brief: This file contains defines and declarations for the registers
 * and related data declarations for input output configuration block.
 *
 * @copyright:  	<Not Available>
 *
 * History:
 * 11-10-2022: junaidaslamRS Created this file for FPGA PCB Configuration Block
 *Programming
 ****************************************************************************************/

#include "rs_inout_config_block.h"

#include "mpaland_printf.h"
#include "rs_util.h"
#include "string.h"

static volatile struct rs_icb_registers *s_ICB_Registers = NULL;
static volatile struct rs_icb_chain_lengths *s_ICB_Chain_Lengths = NULL;

extern void delay_us(uint32_t us);

/* ******************************************
 * @brief: RS_ICB_Init interface sets the
 * required parameters to work on specific
 * platform.
 *
 * @param [in]: uint32_t inBaseAddr.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_ICB_Init(uint32_t inBaseAddr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  if (inBaseAddr == 0x00) {
    lvErrorCode = xCB_ERROR_INVALID_DATA;
  } else {
    s_ICB_Registers = ((struct rs_icb_registers *)inBaseAddr);
    s_ICB_Chain_Lengths = ((struct rs_icb_chain_lengths *)(inBaseAddr + 0x30));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: RS_ICB_Get_Transfer_Param returns the
 * parameters of payload transfer for the use
 * of higher level applications.
 *
 * @param [in]: void *inHeader
 * @param [out]: bool *outTransferType
 * @param [out]: uint32_t *payload_length (uncompressed original)
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_ICB_Get_Transfer_Param(
    void *inHeader, enum TRANSFER_TYPE *outTransferType,
    uint32_t *payload_length) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_icb_bitstream_header *lvBitStream_Header =
      (struct rs_icb_bitstream_header *)inHeader;

  if ((inHeader == NULL) || (outTransferType == NULL) ||
      (payload_length == NULL)) {
    lvErrorCode = xCB_ERROR_NULL_POINTER;
  } else {
    *outTransferType =
        lvBitStream_Header->cfg_cmd < RS_ICB_CFG_MODE_READBACK_AND_POST_CHKSUM
            ? TRANSFER_TYPE_TX
            : TRANSFER_TYPE_RX;
    *payload_length = lvBitStream_Header->bitstream_size;
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

  RS_LOG_DEBUG("ICB", "**** ICB_Soft_Reset Asserted ****\r\n");

  lv_soft_reset.value = s_ICB_Registers->Soft_Reset.value;

  if (lv_soft_reset.value != xCB_SET) {
    lvErrorCode = xCB_ERROR_WRITE_ERROR;
    RS_LOG_ERROR("ICB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
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
 * bitstream to FCB. The steps to do configuration are:
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
      RS_LOG_DEBUG("ICB", "Written cfg_cmd:0x%x\r\n",
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
      RS_LOG_DEBUG("ICB", "Written Op_Cfg Bit_Twist:0x%x Byte_Twist:0x%x\r\n",
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
        RS_LOG_DEBUG("ICB", "Written Chksum_Word:0x%x\r\n",
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
      RS_LOG_DEBUG("ICB", "Written payload_length:0x%x bits @ %p\r\n",
                   (unsigned int)s_ICB_Chain_Lengths->Chain_Length_Reg[0],
                   (void *)&s_ICB_Chain_Lengths->Chain_Length_Reg[0]);
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    lvErrorCode = xCB_ERROR_ICB_CONF_FAILED;
    RS_LOG_ERROR("ICB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: RS_ICB_Config_Begin interface
 * is used to perform ICB registers settings
 * before kicking off the payload transfer.
 *
 * @param [in]: rs_icb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_ICB_Config_Begin(
    struct rs_icb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_icb_cfg_done lv_Cfg_Done = {0};
  struct rs_icb_cfg_kickoff lv_Cfg_Kickoff = {0};
  struct rs_icb_cmd_control lv_Cmd_Control = {0};
  static bool icb_soft_reset_done = false;

  (void)rs_sec_tfr_ptr;

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
      RS_LOG_DEBUG("ICB", "Cmd_Ctl (cmd or data):0x%x\r\n",
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
      RS_LOG_DEBUG("ICB", "Cleared cfg_done:0x%x\r\n",
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
      RS_LOG_DEBUG("ICB", "Enablded Kick-off:0x%x\r\n",
                   (unsigned int)s_ICB_Registers->Cfg_KickOff.value);
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("ICB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
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
    RS_LOG_DEBUG("ICB", "Checksum: 0x%x Status:0x%x\r\n",
                 (unsigned int)s_ICB_Registers->Chksum_Word.value,
                 (unsigned int)s_ICB_Registers->Chksum_Status.value);
  } else {
    RS_LOG_DEBUG("ICB", "**** Checksum Matched ****\r\n");
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
    RS_LOG_DEBUG("ICB", "Check if config done is set\r\n");
    if (s_ICB_Registers->Cfg_Done.value != xCB_SET) {
      lvErrorCode = xCB_ERROR_UNEXPECTED_VALUE;
    } else {
      RS_LOG_DEBUG("ICB", "**** Config done is set ****\r\n");
    }
  }

  // In case capture bit is set in header, then the
  // appropriate bit is given high to low pulse in command_control register.
  if (lvErrorCode == xCB_SUCCESS) {
    if (inHeader->capture) {
      RS_LOG_DEBUG("ICB", "Triggering capture pulse\r\n");
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
          RS_LOG_DEBUG("ICB", "Sent capture pulse\r\n");
        }
      }
    }
  }

  // In case update bit is set in header, then the
  // appropriate bit is given high to low pulse in command_control register.
  if (lvErrorCode == xCB_SUCCESS) {
    if (inHeader->update) {
      RS_LOG_DEBUG("ICB", "triggering update pulse\r\n");

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
          RS_LOG_DEBUG("ICB", "Sent update pulse\r\n");
        }
      }
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("ICB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/********************************************************************************
 * @brief: This function Kicks off the payload transfer
 *
 * @Param [in]: rs_icb_bitstream_header.
 * @Param [in]: rs_secure_transfer_info.
 *
 * @return: xCB_ERROR_CODE
 ********************************************************************************/
enum xCB_ERROR_CODE RS_ICB_Payload_kickoff(
    struct rs_icb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  if (inHeader->cfg_cmd < RS_ICB_CNF_READ_POSTCHKSUM_MODE) {
    rs_sec_tfr_ptr->transfer_addr.write_addr =
        (uint32_t)&s_ICB_Registers->Bitstream_wData;
    rs_sec_tfr_ptr->transfer_addr.tfr_type = RS_SECURE_TX;
    RS_LOG_DEBUG("ICB", "Starting RS_SECURE_TX\r\n");
  } else {
    rs_sec_tfr_ptr->transfer_addr.read_addr =
        (uint32_t)&s_ICB_Registers->Bitstream_rData;
    rs_sec_tfr_ptr->transfer_addr.tfr_type = RS_SECURE_RX;
    RS_LOG_DEBUG("ICB", "Starting RS_SECURE_RX\r\n");
  }

  if ((rs_sec_tfr_ptr->rs_secure_transfer((void *)rs_sec_tfr_ptr)) !=
      CRYPTO_SUCCESS) {
    lvErrorCode = xCB_ERROR;
  }

  while (RS_ICB_busy());

  return lvErrorCode;
}

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
enum xCB_ERROR_CODE RS_ICB_Bitstream_Header_Parser(void *inBitStream,
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
      RS_LOG_DEBUG("ICB", "Extracted_Chksum:0x%08x lvBitstream:0x%08x\r\n",
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

  RS_LOG_DEBUG(
      "ICB",
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
      lvBitStream_Header->update);

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("ICB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}
