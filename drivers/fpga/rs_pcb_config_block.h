/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef RS_RS_PRELOAD_CONFIG_BLOCK_H_
#define RS_RS_PRELOAD_CONFIG_BLOCK_H_

#include <stdbool.h>
#include <stdint.h>

#if !CONFIG_RS_RTOS_PORT
  #include "rs_config_controllers.h"
  #include "rs_image_mgmt.h"
#endif

#define RS_PCB_WAIT_STATE_CALIB_TIME 1  // in microseconds
#define RS_PCB_PL_EXTRA_PARITY_MASK 0x0000000F

/* *********************************************
 * PCB Register pl_ctl is used to configure the
 * BRAM attributes as well as memory block test.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_ctl {
  volatile uint32_t enable : 1;
  volatile uint32_t reserved_1 : 1;
  volatile uint32_t skew_control : 2;
  volatile uint32_t auto_increment : 1;
  volatile uint32_t select_increment : 1;
  volatile uint32_t parity : 1;
  volatile uint32_t even : 1;
  volatile uint32_t clock_wait_cycles : 6;
  volatile uint32_t split_bits : 2;
  volatile uint32_t reserved_2 : 12;
  volatile uint32_t bist_fail : 1;
  volatile uint32_t bist_pass : 1;
  volatile uint32_t bist_start : 1;
  volatile uint32_t pl_init_control : 1;
};

/* *********************************************
 * PCB Register pl_stat is used to start the
 * calibration of the clock_wait_cycles.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_stat {
  volatile uint32_t calib : 1;
  volatile uint32_t calib_done : 1;
  volatile uint32_t reserved_1 : 6;
  volatile uint32_t cal_wait : 6;
  volatile uint32_t reserved_2 : 18;
};

/* *********************************************
 * PCB Register pl_cfg is read-only register
 * providing number of Ram rows and columns
 * in selected RAM/FIFO of the eFPGA.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_cfg {
  volatile uint32_t ram_size : 16;
  volatile uint32_t row : 8;
  volatile uint32_t col : 8;
};

/* *********************************************
 * PCB Register pl_select is used to specify
 * the first values of Ram row/col number out of
 * total number of rows and cols of ram blocks.
 * offset is the 36bit location to start read or
 * write.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_select {
  volatile uint32_t offset : 12;
  volatile uint32_t row : 10;
  volatile uint32_t col : 10;
};

/* *********************************************
 * PCB Register pl_extra is used to specify
 * parity bits for each byte of a word. the
 * parity_nibble_rw from bit 0 to 3 specify the
 * parity bit for byte 0 to 3 respectively.
 * Similarly, the parity_nibble_ro is read only
 * for data returned on respective parity bit.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_extra {
  volatile uint32_t parity_nibble_rw : 4;
  volatile uint32_t reserved_1 : 4;
  volatile uint32_t parity_nibble_r0 : 4;
  volatile uint32_t reserved_2 : 20;
};

/* *********************************************
 * PCB Register pl_row is used to specify the
 * stride for taking the jump to the next RAM
 * block in the row of ram blocks. The offset
 * specifies the starting row in the selected
 * ram block.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_row {
  volatile uint32_t offset : 10;
  volatile uint32_t reserved_1 : 6;
  volatile uint32_t stride : 10;
  volatile uint32_t reserved_2 : 6;
};

/* *********************************************
 * PCB Register pl_col is used to specify the
 * stride for taking the jump to the next RAM
 * block in the col of ram blocks. The offset
 * specifies the starting col in the selected
 * ram block.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_col {
  volatile uint32_t offset : 10;
  volatile uint32_t reserved_1 : 6;
  volatile uint32_t stride : 10;
  volatile uint32_t reserved_2 : 6;
};

/* *********************************************
 * PCB Register pl_targ register specifies the
 * address where the bitstream should be writen.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_targ { volatile uint32_t value; };

/* *********************************************
 * PCB Register pl_targ register specifies the
 * address from where the bitstream can be read.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_data { volatile uint32_t value; };

/* *********************************************
 * PCB Register pl_reserved specifies the
 * address which is not used by PCB.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_pl_reserved { volatile uint32_t value; };

/* *********************************************
 * PCB_Registers encapsulates all of the
 * configuration registers of preload config
 * block.
 * ********************************************/
__attribute__((packed)) struct rs_pcb_registers {  // Offset
  volatile struct rs_pcb_pl_ctl Pl_Ctl;            // 0x40
  volatile struct rs_pcb_pl_stat Pl_Stat;          // 0x44
  volatile struct rs_pcb_pl_cfg Pl_Cfg;            // 0x48 READ-ONLY
  volatile struct rs_pcb_pl_select Pl_Select;      // 0x4C
  volatile struct rs_pcb_pl_reserved
      Pl_Reserved;  // ... 0x50 was skipped by QuickLogic
  volatile struct rs_pcb_pl_extra Pl_Extra;  // 0x54
  volatile struct rs_pcb_pl_row Pl_Row;      // 0x58
  volatile struct rs_pcb_pl_col Pl_Col;      // 0x5C
  volatile struct rs_pcb_pl_targ Pl_Targ;    // 0x60
  volatile struct rs_pcb_pl_data Pl_Data;    // 0x64
};

/******************************************************************
 * Bitstream Header information for programming the PCB
 * Any future updates to the header size must also be reflected in
 * the signing utility.
 ******************************************************************/

#define RS_PCB_HDR_THIRD_WORD_OFFSET 2
#define RS_PCB_HDR_PL_SELECT_WORD_OFFSET 3
#define RS_PCB_HDR_PL_ROW_WORD_OFFSET 4
#define RS_PCB_HDR_PL_COL_WORD_OFFSET 5
#define RS_PCB_HDR_SEVENTH_WORD_OFFSET 6

/* Masks and offsets are defined for bitfields
 * or less than word sized data within the
 * header structure. For example, the bitfields
 * in the PCB_Bitstream_Header struct are packed
 * in a single 32 bit header entry in the bitstream.
 * Hence, it is needed to properly parse them to
 * be useful.
 */

/**** Third Header Word Bitfields Masks and Offsets ****/
#define RS_PCB_HDR_NR_RAM_BLOCKS_MASK 0x0000FFFF
#define RS_PCB_HDR_SKEW_MASK 0x00030000
#define RS_PCB_HDR_PARITY_MASK 0x00040000
#define RS_PCB_HDR_EVEN_MASK 0x00080000
#define RS_PCB_HDR_SPLIT_MASK 0x00300000
#define RS_PCB_HDR_RESERVED_1_MASK 0xFFC00000

#define RS_PCB_HDR_NR_RAM_BLOCKS_OFFSET 0
#define RS_PCB_HDR_SKEW_OFFSET 16
#define RS_PCB_HDR_PARITY_OFFSET 18
#define RS_PCB_HDR_EVEN_OFFSET 19
#define RS_PCB_HDR_SPLIT_OFFSET 20
#define RS_PCB_HDR_RESERVED_1_OFFSET 22

/**** Seventh Header Word Bitfields Masks and Offsets ****/
#define RS_PCB_HDR_PARITY_NIBBLE_RW_MASK 0x0000000F
#define RS_PCB_HDR_RESERVED_2_MASK 0xFFFFFFF0

#define RS_PCB_HDR_PARITY_NIBBLE_RW_OFFSET 0
#define RS_PCB_HDR_RESERVED_2_OFFSET 4

__attribute__((packed)) struct rs_pcb_bitstream_header {
  // First and Second Header Words
  struct rs_action_header generic_hdr;
  // Third Header Word
  uint32_t Total_Nr_of_RAM_Blocks : 16;
  uint32_t Skew : 2;
  uint32_t parity : 1;
  uint32_t even : 1;
  uint32_t split : 2;
  uint32_t reserved_1 : 10;
  // Fourth Header Word
  struct rs_pcb_pl_select PL_SELECT;
  // Fifth Header Word
  struct rs_pcb_pl_row PL_ROW;
  // Sixth Header Word
  struct rs_pcb_pl_col PL_COL;
  // Seventh Header Word
  uint32_t parity_nibble_rw : 4;
  uint32_t reserved_2 : 28;
};
/******************************************************************
 * End of Bitstream Header information for programming the PCB
 ******************************************************************/

/* ******************************************
 * @brief: RS_PCB_Init interface sets the
 * required parameters to work on specific
 * platform.
 *
 * @param [in]: uint32_t inBaseAddr.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_Init(uint32_t inBaseAddr);

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
                                              uint32_t *outBitStr_Size);

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
    struct rs_secure_transfer_info *rs_sec_tfr_ptr);

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
enum xCB_ERROR_CODE RS_PCB_Config_End(struct rs_pcb_bitstream_header *inHeader);

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
    struct rs_secure_transfer_info *rs_sec_tfr_ptr);

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
    struct rs_secure_transfer_info *rs_sec_tfr_ptr);

/* ******************************************
 * @brief: PCB_bitstream_Rx_kickoff interface
 * is used to read the bitstreams from PCB
 * configuration controller.
 *
 * @param [in]: rs_pcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE RS_PCB_bitstream_Rx_kickoff(
    struct rs_pcb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr);

/* ******************************************
 * @brief: RS_PCB_Bitstream_Header_Parser
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
                                                   void *outHeader);

#endif /* RS_PRELOAD_CONFIG_BLOCK_H_ */
