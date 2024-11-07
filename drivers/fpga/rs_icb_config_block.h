/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef RS_RS_INOUT_CONFIG_BLOCK_H_
#define RS_RS_INOUT_CONFIG_BLOCK_H_

#include <stdbool.h>
#include <stdint.h>

#define RS_ICB_NR_OF_CHAIN_LEN_REGISTERS 1
#define RS_ICB_CAP_UPD_PULSE_DELAY 10  // microseconds
#define RS_ICB_CNF_WRITE_MODE 0
#define RS_ICB_CNF_WRITE_PRECHKSUM_MODE 1
#define RS_ICB_CNF_READ_POSTCHKSUM_MODE 2
#define RS_ICB_CNF_READ_MODE 3

/*** ICB Configuration Mode Enumerations ***/
enum RS_ICB_CFG_MODE {
  RS_ICB_CFG_MODE_CONFIG_ONLY = 0,
  RS_ICB_CFG_MODE_CONFIG_AND_PRE_CHKSUM = 1,
  RS_ICB_CFG_MODE_READBACK_AND_POST_CHKSUM = 2,
  RS_ICB_CFG_MODE_READBACK = 3
};

/*** ICB Shift Status Enumerations ***/
enum RS_ICB_SHIFT_STATUS {
  RS_ICB_SHIFT_STATUS_NOT_WORKING = 0,
  RS_ICB_SHIFT_STATUS_CTRL_WAIT_WRITE_READ_DATA = 1,
  RS_ICB_SHIFT_STATUS_SHIFTING_DATA = 2
};

/*** ICB CMD_CONTROL Options ***/
enum RS_ICB_CMD_DATA_CYCLE { RS_ICB_DATA_CYCLE = 0, RS_ICB_CMD_CYCLE = 1 };

/* *********************************************
 * ICB Register cfg_cmd to program the
 * configuration mode according to the
 * ICB_CFG_CMD enum.
 * ********************************************/
__attribute__((packed)) struct rs_icb_cfg_cmd {
  volatile uint32_t cfg_mode : 2;  // Select Mode from ICB_CFG_MODE
  volatile uint32_t reserved : 30;
};

/* *********************************************
 * ICB Register cfg_kickoff is used to start the
 * eFPGA configuration according to the
 * configuration mode set in register CFG_CMD
 * ********************************************/
__attribute__((packed)) struct rs_icb_cfg_kickoff {
  volatile uint32_t value : 1;
  volatile uint32_t reserved : 31;
};

/* *********************************************
 * ICB Register cfg_done is used to set the
 * configuration status to done or not done.
 * ********************************************/
__attribute__((packed)) struct rs_icb_cfg_done {
  volatile uint32_t value : 1;
  volatile uint32_t reserved : 31;
};

/* *********************************************
 * ICB Register chksum_word is used to set the
 * bistream checksum value. This value is used
 * to compare against the incoming bitstream
 * checksum which is calcualted on the fly.
 * ********************************************/
__attribute__((packed)) struct rs_icb_chksum_word { volatile uint32_t value; };

/* *********************************************
 * ICB Register chksum_status is set to 1 if the
 * checksum calculated in the above step matches
 * the checksum in chksum_word register otherwise
 * set to 0. The value field is read only.
 * ********************************************/
__attribute__((packed)) struct rs_icb_chksum_status {
  volatile uint32_t value : 1;
  volatile uint32_t reserved : 31;
};

/* *********************************************
 * ICB Register soft_reset bit is set to 1 if the
 * eFPGA needs to be reset.
 * ********************************************/
__attribute__((packed)) struct rs_icb_soft_reset {
  volatile uint32_t value : 1;
  volatile uint32_t reserved : 31;
};

/* *********************************************
 * ICB Register cmd_control bit not used for ICB
 * The value field is write only.
 * ********************************************/
__attribute__((packed)) struct rs_icb_cmd_control {
  volatile uint32_t cmd_data : 1;
  volatile uint32_t capture_control : 1;
  volatile uint32_t update_control : 1;
  volatile uint32_t reserved : 29;
};

/* *********************************************
 * ICB Register op_config contains following:
 * 	- start_chain_num and end_chain_num for
 * 	programming the required number of chains.
 * 	if they are not equal then auto load mode
 * 	must be used along with word_align bit set
 * 	to 1.
 * 	bit_twist/byte_twist are used to set the
 * 	endianness of the bitstream being programmed.
 * 	For reference, look at the document named,
 * 	configuration_controller.docx
 * ********************************************/
__attribute__((packed)) struct rs_icb_op_config {
  volatile uint32_t bit_twist : 1;
  volatile uint32_t reserved_1 : 3;
  volatile uint32_t byte_twist : 1;
  volatile uint32_t reserved_2 : 3;
  volatile uint32_t word_align : 1;
  volatile uint32_t reserved_3 : 7;
  volatile uint32_t start_chain_num : 8;
  volatile uint32_t end_chain_num : 8;
};

/* *********************************************
 * ICB Register shift_status specifies state
 * machine as well as bit counter for total
 * number of configuration bits.
 * FSM states:
 * 	- Not working
 * 	- controller waiting write/read data
 * 	- controller shifting data
 * ********************************************/
__attribute__((packed)) struct rs_icb_shift_status {
  volatile uint32_t shift_count : 28;
  volatile uint32_t fsm_state : 3;
};

/* *********************************************
 * ICB Register bitstream_wData is used to write
 * configuration data.
 * ********************************************/
__attribute__((packed)) struct rs_icb_bitstream_wData {
  volatile uint32_t wData;
};

/* *********************************************
 * ICB Register bitstream_rData is used to read
 * configuration data.
 * ********************************************/
__attribute__((packed)) struct rs_icb_bitstream_rData {
  volatile uint32_t rData;
};

/* Do not change the order of elements of following structure */
__attribute__((packed)) struct rs_icb_registers {          // Offsets
  volatile struct rs_icb_cfg_cmd Cfg_Cmd;                  // 0x00
  volatile struct rs_icb_cfg_kickoff Cfg_KickOff;          // 0x04
  volatile struct rs_icb_cfg_done Cfg_Done;                // 0x08
  volatile struct rs_icb_chksum_word Chksum_Word;          // 0x0C
  volatile struct rs_icb_chksum_status Chksum_Status;      // 0x10
  volatile struct rs_icb_soft_reset Soft_Reset;            // 0x14
  volatile struct rs_icb_cmd_control Cmd_Control;          // 0x18
  volatile struct rs_icb_op_config Op_Config;              // 0x1C
  volatile struct rs_icb_shift_status Shift_Status;        // 0x20
  volatile struct rs_icb_bitstream_wData Bitstream_wData;  // 0x24
  volatile struct rs_icb_bitstream_rData Bitstream_rData;  // 0x28
};

/*** ICB Registers ***/

/*** ICB Chain Length Specifier Registers ***/
__attribute__((packed)) struct rs_icb_chain_lengths {  // Offsets
  volatile uint32_t Chain_Length_Reg[RS_ICB_NR_OF_CHAIN_LEN_REGISTERS];  // 0x30
};

/******************************************************************
 * Bitstream Header information for programming the ICB
 * Any future updates to the header size must also be reflected in
 * the signing utility.
 ******************************************************************/
/* Masks and offsets are defined for bitfields
 * or less than word sized data within the
 * header structure. For example, the bitfields
 * in the ICB_Bitstream_Header struct are packed
 * in a single 32 bit header entry in the bitsream.
 * Hence, it is needed to properly parse them to
 * be useful.
 */

#define RS_ICB_HDR_CFG_CMD_MASK 0x00000003
#define RS_ICB_HDR_BIT_TWIST_MASK 0x00000004
#define RS_ICB_HDR_BYTE_TWIST_MASK 0x00000008
#define RS_ICB_HDR_CMD_DATA_MASK 0x00000010
#define RS_ICB_HDR_UPDATE_MASK 0x00000020
#define RS_ICB_HDR_CAPTURE_MASK 0x00000040
#define RS_ICB_HDR_RESERVED_MASK 0xFFFFFF80

#define RS_ICB_HDR_CFG_CMD_OFFSET 0
#define RS_ICB_HDR_BIT_TWIST_OFFSET 2
#define RS_ICB_HDR_BYTE_TWIST_OFFSET 3
#define RS_ICB_HDR_CMD_DATA_OFFSET 4
#define RS_ICB_HDR_UPDATE_OFFSET 5
#define RS_ICB_HDR_CAPTURE_OFFSET 6
#define RS_ICB_HDR_RESERVED_OFFSET 7

__attribute__((packed)) struct rs_icb_bitstream_header {
  struct rs_action_header generic_hdr;
  uint32_t bitstream_size;  // uncompressed original
  uint32_t bitstream_checksum;
  uint32_t cfg_cmd : 2;
  uint32_t bit_twist : 1;
  uint32_t byte_twist : 1;
  uint32_t cmd_data : 1;
  uint32_t update : 1;
  uint32_t capture : 1;
  uint32_t reserved : 25;
};
/******************************************************************
 * End of Bitstream Header information for programming the ICB
 ******************************************************************/

#endif /* RS_INOUT_CONFIG_BLOCK_H_ */
