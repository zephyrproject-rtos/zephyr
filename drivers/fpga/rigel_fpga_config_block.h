/****************************************************************************************
 * File Name: 		rigel_fpga_config_block.h
 * Author: 			  Muhammad Junaid Aslam
 * Organization:	Rapid Silicon
 * Repository: 		EmbeddedCommon-Dev
 *
 * @brief: This file contains defines and declarations for the registers
 * and related data declarations for eFPGA configuration block.
 *
 * @copyright:  	<Not Available>
 *
 * History:
 * 23-05-2023: junaidaslamRS Created this file for FPGA RIGEL_FCB Configuration
 *Block Programming.
 ****************************************************************************************/

#ifndef RS_RIGEL_FPGA_CONFIG_BLOCK_H_
#define RS_RIGEL_FPGA_CONFIG_BLOCK_H_

#include <stdbool.h>
#include <stdint.h>

#if !CONFIG_RS_RTOS_PORT
  #include "rs_config_controllers.h"
  #include "rs_image_mgmt.h"
#else
  #include "fpga_rs_xcb.h"
#endif

#define RIGEL_FCB_NR_MODE_REG 16

/*** RIGEL_FCB Checksum Status Bits ***/
enum RIGEL_FCB_CHECKSUM_STATUS {
  RIGEL_FCB_CHECKSUM_INCORRECT = 0,
  RIGEL_FCB_CHECKSUM_CORRECT = 1
};

/* *********************************************
 * RIGEL_FCB wl_mode_x has 6 write-only bits.
 * wl_mode[n][0] = ostrobe (odd strobe)
 * wl_mode[n][1] = estrobe (even strobe)
 * wl_mode[n][2] = sclk (slave clock)
 * wl_mode[n][4] = mclk (master clock)
 * wl_mode[n][5] = bottom (wordline walking bit)
 * wl_mode[n][6] = sclock (scan reg clock)
 * ********************************************/
enum RIGEL_FCB_WL_MODE_BIT_OFFSETS {
  RIGEL_FCB_WL_MODE_OSTROBE_OFFSET = 0,
  RIGEL_FCB_WL_MODE_ESTROBE_OFFSET = 1,
  RIGEL_FCB_WL_MODE_SCLK_OFFSET = 2,
  RIGEL_FCB_WL_MODE_MCLK_OFFSET = 4,
  RIGEL_FCB_WL_MODE_BOTTOM_OFFSET = 5,
  RIGEL_FCB_WL_MODE_SCLOCK_OFFSET = 6
};

#define RIGEL_FCB_WL_MODE_BIT_WIDTHS 1

/* *********************************************
 * RIGEL_FCB op_reg has 7 read-only bits.
 * op_reg[0] = ostrobe (odd strobe)
 * op_reg[1] = estrobe (even strobe)
 * op_reg[2] = sclk (slave clock)
 * op_reg[3] = dout (data out)
 * op_reg[4] = mclk (master clock)
 * op_reg[5] = bottom (wordline walking bit)
 * op_reg[6] = sclock (scan reg clock)
 * ********************************************/
enum RIGEL_FCB_OP_REG_BIT_OFFSETS {
  RIGEL_FCB_OP_REG_OSTROBE_OFFSET = 0,
  RIGEL_FCB_OP_REG_ESTROBE_OFFSET = 1,
  RIGEL_FCB_OP_REG_SCLK_OFFSET = 2,
  RIGEL_FCB_OP_REG_DOUT_OFFSET = 3,
  RIGEL_FCB_OP_REG_MCLK_OFFSET = 4,
  RIGEL_FCB_OP_REG_BOTTOM_OFFSET = 5,
  RIGEL_FCB_OP_REG_SCLOCK_OFFSET = 6,
  RIGEL_FCB_OP_REG_BL_GATING_OFFSET = 7
};

#define RIGEL_FCB_OP_REG_BIT_WIDTHS 1

/* *********************************************
 * RIGEL_FCB bl_status has 13 read-only bits.
 * bl_status[15:0] = BL_WR_CNT: BL write counter,
 * indicates number of writes bits in bl_data.
 * It is incremented by 32.
 * bl_status[16] = BL_CLR Clear: bl_data, BL_WR_CNT
 * and checksum status register
 * bl_status[17] = sel_bl_source: Load readback data
 * register, set 1 to load bl_data_in in readback
 * register, then set to 0.
 * ********************************************/
enum RIGEL_FCB_BL_STATUS_BIT_OFFSETS {
  RIGEL_FCB_BL_STATUS_BL_WR_CNT_OFFSET = 0,
  RIGEL_FCB_BL_STATUS_BL_CLR_OFFSET = 16,
  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_OFFSET = 17
};

#define RIGEL_FCB_BL_STATUS_BL_WR_CNT_WIDTH 16
#define RIGEL_FCB_BL_STATUS_BL_CLR_WIDTH 1
#define RIGEL_FCB_BL_STATUS_SEL_BL_SRC_WIDTH 1

/* *********************************************
 * RIGEL_FCB status has 1 read-only bit and 2
 * read/write bits. The 1 read-only bit indicates
 * checksum status. The 2 read/write bits are
 * used to enable checksum verification and
 * for indicating if config_done was done or not.
 * ********************************************/
enum RIGEL_FCB_STATUS_BIT_OFFSETS {
  RIGEL_FCB_STATUS_BL_PRECHECKSUM_EN_OFFSET = 0,
  RIGEL_FCB_STATUS_BL_CHECKSUM_STATUS_OFFSET = 8,
  RIGEL_FCB_STATUS_CFG_DONE_EN_OFFSET = 16
};

#define RIGEL_FCB_STATUS_BIT_WIDTHS 1

/* *******************************************
 * RIGEL_FCB_WL_MODES enum shows different modes
 * corresponding to wl_mode registers
 * from 0 to 15. OR means Operation Register
 * (OP_REG).
 * *******************************************/
enum RIGEL_FCB_WL_MODES {
  RIGEL_FCB_WL_MODE_CLR_OR = 0,
  RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_FLIP_OR = 1,
  RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE = 2,
  RIGEL_FCB_WL_MODE_INVERT_MPU_VAL_IN_OR = 3,
  RIGEL_FCB_WL_MODE_MPU_1_FLIP_OR_MPU_0_CLR_OR = 4,
  RIGEL_FCB_WL_MODE_FLIP_OR = 5,
  RIGEL_FCB_WL_MODE_MPU_1_FLIP_OR_MPU_0_NO_CHANGE = 6,
  RIGEL_FCB_WL_MODE_MPU_1_FLIP_OR_MPU_0_SET_OR = 7,
  RIGEL_FCB_WL_MODE_MPU_1_NO_CHANGE_MPU_0_CLR_OR = 8,
  RIGEL_FCB_WL_MODE_MPU_1_NO_CHANGE_MPU_0_FLIP_OR = 9,
  RIGEL_FCB_WL_MODE_MPU_1_NO_CHANGE_MPU_0_NO_CHANGE = 10,
  RIGEL_FCB_WL_MODE_MPU_1_NO_CHANGE_MPU_0_SET_OR = 11,
  RIGEL_FCB_WL_MODE_WRITE_OR = 12,
  RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_FLIP_OR = 13,
  RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE = 14,
  RIGEL_FCB_WL_MODE_SET_OR = 15
};

__attribute__((packed)) struct rigel_fcb_registers {
  /* ******************************************************************************************
   * RIGEL_FCB Registers wl_mode[n] are used to set the word line mode through
   * which operation register is modified for all of its bits at once. The
   * complete list of possible actions is described in the related enum
   * (RIGEL_FCB_WL_MODES) declared above.
   * *****************************************************************************************/
  volatile uint32_t wl_mode[RIGEL_FCB_NR_MODE_REG];
  /* *********************************************
   * RIGEL_FCB Register op_reg is used to read the
   * 6 bits controlling the data flow from
   * bitline register to the CLB rows.
   * ********************************************/
  volatile uint32_t op_reg;    // Read only
                               /* *********************************************
                                * RIGEL_FCB Register bl_wdata is to transfer
                                * the 32bit bitstream words at a time to bitline
                                * register.
                                * ***********************************************/
  volatile uint32_t bl_wdata;  // write-only
                               /* *********************************************
                                * RIGEL_FCB Register bl_status is to check the
                                * number of words written to the bitline register.
                                * ***********************************************/
  volatile uint32_t bl_status;
  /* *********************************************
   * RIGEL_FCB Register bl_rdata is to read back
   * the bitline register contents.
   * ***********************************************/
  volatile uint32_t bl_rdata;
  /* *********************************************
   * RIGEL_FCB Register chksum_word is used to set the
   * bistream checksum value. This value is used
   * to compare against the incoming bitstream
   * checksum which is calcualted on the fly.
   * ********************************************/
  volatile uint32_t checksum_word;
  /* *********************************************
   * RIGEL_FCB Register status is used to set the
   * CFG_DONE and Checksum Enable bits. While the
   * Checksum status (read-only) bit returns if the
   * checksum matched or not during transfer.
   * ***********************************************/
  volatile uint32_t status;
};

/******************************************************************
 * Bitstream Header information for programming the RIGEL_FCB
 * Any future updates to the header size must also be reflected in
 * the signing utility.
 ******************************************************************/
#define RIGEL_FCB_HDR_BITLINE_REG_WIDTH_MASK 0x0000FFFF
#define RIGEL_FCB_HDR_READBACK_MASK 0x00010000
#define RIGEL_FCB_HDR_BITLINE_REG_WIDTH_OFFSET 0
#define RIGEL_FCB_HDR_READBACK_OFFSET 16

__attribute__((packed)) struct rigel_fcb_bitstream_header {
  // First two words
  struct rs_action_header generic_hdr;
  // Third Header Word
  uint32_t bitstream_checksum;
  // Fourth Header Word
  uint32_t bitline_reg_width : 16;  // Bitline width in bytes
  uint32_t readback : 1;
  uint32_t reserved : 15;
};
/******************************************************************
 * End of Bitstream Header information for programming the RIGEL_FCB
 ******************************************************************/

/* ******************************************
 * @brief: Rigel_FCB_Init interface sets the
 * required parameters to work on specific
 * platform.
 *
 * @param [in]: uint32_t inBaseAddr.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Init(uint32_t inBaseAddr);

/* ******************************************
 * @brief: Rigel_FCB_Transfer_Param returns the
 * parameters of payload transfer for the use
 * of higher level applications.
 *
 * @param [in]: void *inHeader
 * @param [out]: bool *outTransferType
 * @param [out]: uint16_t *outBlockSize (in Words)
 * @param [out]: uint32_t *outBitStr_Size
 *
 * @return: xCB_ERROR_CODE
 *
 * @note if outBlockSize = 0, entire
 * bitstream can be transferred at once. But,
 * it will still be possible to transfer the
 * bitstream in blocks of user defined size.
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Get_Transfer_Param(
    void *inHeader, enum TRANSFER_TYPE *outTransferType, uint16_t *outBlockSize,
    uint32_t *outBitStr_Size);

/* ******************************************
 * @brief: Rigel_FCB_Config_Begin interface
 * is used to perform FCB registers settings
 * before kicking off the payload transfer.
 *
 * @param [in]: fcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Config_Begin(
    struct rigel_fcb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr);

/* ******************************************
 * @brief: Rigel_FCB_Config_End interface
 * is used to perform FCB registers settings
 * after the payload transfer.
 *
 * @param [in]: fcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Config_End(
    struct rigel_fcb_bitstream_header *inHeader);

/* ******************************************
 * @brief: Rigel_FCB_Payload_kickoff interface is
 * used to transfer the bitstreams to/from
 * Rigel_FCB configuration controller.
 *
 * @param [in]: Rigel_FCB_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Payload_kickoff(
    struct rigel_fcb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr);

/* ******************************************
 * @brief: Rigel_FCB_Bitstream_Header_Parser interface
 * will parse the header of Rigel_FCB config
 * controller type.
 *
 * @param [in]: inBitStream is a pointer to
 * bitstream.
 * @param [out]: outHeader contains the
 * parsed header information.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Bitstream_Header_Parser(void *inBitStream,
                                                      void *outHeader);

#endif /* RS_FPGA_CONFIG_BLOCK_H_ */
