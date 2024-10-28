/****************************************************************************************
 * File Name: 		rs_preload_config_block.c
 * Author: 			  Muhammad Junaid Aslam
 * Organization:	Rapid Silicon
 * Repository: 		EmbeddedCommon-Dev
 *
 * @brief: This file contains defines and declarations for the registers
 * and related data declarations for preload configuration block for BRAM.
 *
 * @copyright:  	<Not Available>
 *
 * History:
 * 11-10-2022: junaidaslamRS Created this file for FPGA PCB Configuration Block
 *Programming
 ****************************************************************************************/

#include "rs_preload_config_block.h"

#include "mpaland_printf.h"
#include "rs_image_mgmt.h"
#include "rs_util.h"
#include "string.h"

static volatile struct rs_pcb_registers *s_PCB_Registers = NULL;

extern void delay_us(uint32_t us);

/* ******************************************
 * @brief: FCB_Driver_Init interface sets the
 * required parameters to work on specific
 * platform.
 *
 * @param [in]: uint32_t inBaseAddr.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_Init(uint32_t inBaseAddr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  if (inBaseAddr == 0x00) {
    lvErrorCode = xCB_ERROR_INVALID_DATA;
  } else {
    s_PCB_Registers = (struct rs_pcb_registers *)(inBaseAddr + 0x40);
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

  delay_us(RS_PCB_WAIT_STATE_CALIB_TIME);

  // Check if calibration is done
  if (s_PCB_Registers->Pl_Stat.calib_done != xCB_SET) {
    lvErrorCode = xCB_ERROR_TIMEOUT;
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("PCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
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
    lvErrorCode = xCB_ERROR_PCB_CONF_FAILED;
    if (lvErrorCode != xCB_SUCCESS) {
      RS_LOG_ERROR("PCB", "%s(%d):%s\r\n", __func__, __LINE__,
                   Err_to_Str(lvErrorCode));
    }
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: RS_PCB_Config_Begin interface
 * is used to perform PCB registers settings
 * before kicking off the payload transfer.
 *
 * @param [in]: rs_pcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_Config_Begin(
    struct rs_pcb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  (void)rs_sec_tfr_ptr;

  lvErrorCode = RS_PCB_Config_Registers(inHeader);

  return lvErrorCode;
}

/* ******************************************
 * @brief: RS_PCB_Config_End interface
 * is used to perform PCB registers settings
 * after the payload transfer.
 *
 * @param [in]: rs_pcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_Config_End(
    struct rs_pcb_bitstream_header *inHeader) {
  (void)inHeader;
  return xCB_SUCCESS;
}

/* ******************************************
 * @brief: RS_PCB_bitstream_Tx_kickoff_Data
 * interface is used to write the bitstreams
 * to PCB configuration controller.
 *
 * @param [in]: rs_pcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_bitstream_Tx_kickoff_Data(
    struct rs_secure_transfer_info *rs_sec_tfr_ptr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  rs_sec_tfr_ptr->transfer_addr.write_addr =
      (uint32_t)&s_PCB_Registers->Pl_Targ.value;
  rs_sec_tfr_ptr->transfer_addr.tfr_type = RS_SECURE_TX;

  if ((rs_sec_tfr_ptr->rs_secure_transfer((void *)rs_sec_tfr_ptr)) !=
      CRYPTO_SUCCESS) {
    lvErrorCode = xCB_ERROR;
  }

  if (lvErrorCode != xCB_SUCCESS) {
    lvErrorCode = xCB_ERROR_PCB_BITSTREAM_TX_FAILED;
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("PCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: RS_PCB_bitstream_Tx_kickoff_ParityData
 * interface is used to write the bitstreams
 * to PCB configuration controller.
 *
 * @param [in]: rs_pcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_bitstream_Tx_kickoff_ParityData(
    struct rs_secure_transfer_info *rs_sec_tfr_ptr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  const uint32_t data_count_in_words =
      rs_sec_tfr_ptr->transfer_addr.len / 4;  // Number of Words
  const uint32_t *data = (uint32_t *)rs_sec_tfr_ptr->transfer_addr.read_addr;

  /*
    Since we Expect Parity Word for Each
    Data Word, The length of data should
    always be a multiple of two in this
    function.
  */
  if (data_count_in_words & 0x00000001) {
    lvErrorCode = xCB_ERROR_INVALID_DATA_LENGTH;
  }

  for (uint32_t i = 0; i < data_count_in_words; i += 2) {
    uint32_t lvParity = ((*data++) & RS_PCB_PL_EXTRA_PARITY_MASK);
    uint32_t lvData = (*data++);
    s_PCB_Registers->Pl_Extra.parity_nibble_rw =
        lvParity;  // First we send parity
    s_PCB_Registers->Pl_Targ.value =
        lvData;  // Then we send corresponding data word
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("PCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  if (lvErrorCode != xCB_SUCCESS) {
    lvErrorCode = xCB_ERROR_PCB_BITSTREAM_TX_FAILED;
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: PCB_bitstream_Rx_kickoff interface
 * is used to read the bitstreams from PCB
 * configuration controller.
 *
 * @param [in]: rs_pcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 *
 * @note: This function does not support
 * Reading for Chunking to perform content
 * Verification. It only supports non-chunking
 * method for direct data comparison.
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_bitstream_Rx_kickoff(
    struct rs_pcb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  REG_WRITE_32((void *)&s_PCB_Registers->Pl_Select,
               (void *)&inHeader->PL_SELECT);

  RS_LOG_DEBUG(
      "PCB",
      "\n%s Setting PL_SELECT: %p = 0x%x --> Offset:0x%x Row:0x%x Col:0x%x\r\n",
      __func__, (void *)&s_PCB_Registers->Pl_Select,
      *(unsigned int *)&s_PCB_Registers->Pl_Select,
      s_PCB_Registers->Pl_Select.offset, s_PCB_Registers->Pl_Select.row,
      s_PCB_Registers->Pl_Select.col);

  if ((s_PCB_Registers->Pl_Select.col != inHeader->PL_SELECT.col) ||
      (s_PCB_Registers->Pl_Select.row != inHeader->PL_SELECT.row) ||
      (s_PCB_Registers->Pl_Select.offset != inHeader->PL_SELECT.offset)) {
    lvErrorCode = xCB_ERROR_WRITE_ERROR;
  }

  if (lvErrorCode == xCB_SUCCESS) {
    rs_sec_tfr_ptr->transfer_addr.read_addr =
        (uint32_t)&s_PCB_Registers->Pl_Targ.value;
    rs_sec_tfr_ptr->transfer_addr.len = inHeader->generic_hdr.payload_size;
    rs_sec_tfr_ptr->transfer_addr.tfr_type = RS_SECURE_RX;
    if ((rs_sec_tfr_ptr->rs_secure_transfer((void *)rs_sec_tfr_ptr)) !=
        CRYPTO_SUCCESS) {
      lvErrorCode = xCB_ERROR;
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    lvErrorCode = xCB_ERROR_PCB_BITSTREAM_RX_FAILED;
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("PCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: RS_PCB_Get_Transfer_Param returns the
 * parameters of payload transfer for the use
 * of higher level applications.
 *
 * @param [in]: void *inHeader
 * @param [out]: uint32_t *outBitStr_Size
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_Get_Transfer_Param(void *inHeader,
                                              uint32_t *outBitStr_Size) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rs_pcb_bitstream_header *lvBitStream_Header =
      (struct rs_pcb_bitstream_header *)inHeader;

  if ((inHeader == NULL) || (outBitStr_Size == NULL)) {
    lvErrorCode = xCB_ERROR_NULL_POINTER;
  } else {
    *outBitStr_Size = lvBitStream_Header->generic_hdr.payload_size;
  }

  return lvErrorCode;
}
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
enum xCB_ERROR_CODE RS_PCB_Bitstream_Header_Parser(void *inBitStream,
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

  RS_LOG_DEBUG("PCB",
               "\n ...... PCB_HDR ...... \r\nBitStr_Size:%lu \r\nNr_Blocks:%lu "
               "\r\nSkew:%lu \r\nParity:%lu \r\nEven:%lu \r\nsplit:%lu "
               "\r\nPL_SELECT:0x%x \r\nPL_ROW:0x%x \r\nPL_COL:0x%x "
               "\r\nParity_nibble_rw:0x%x\r\n",
               lvBitStream_Header->generic_hdr.payload_size,
               lvBitStream_Header->Total_Nr_of_RAM_Blocks,
               lvBitStream_Header->Skew, lvBitStream_Header->parity,
               lvBitStream_Header->even, lvBitStream_Header->split,
               *(unsigned int *)&lvBitStream_Header->PL_SELECT,
               *(unsigned int *)&lvBitStream_Header->PL_ROW,
               *(unsigned int *)&lvBitStream_Header->PL_COL,
               (unsigned int)lvBitStream_Header->parity_nibble_rw);

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("PCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}
