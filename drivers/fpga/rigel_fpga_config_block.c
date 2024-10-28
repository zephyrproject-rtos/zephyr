/****************************************************************************************
 * File Name: 		rigel_fpga_config_block.c
 * Author: 			  Muhammad Junaid Aslam
 * Organization:	Rapid Silicon
 * Repository: 		EmbeddedCommon-Dev
 *
 * @brief: This file contains driver implementation for eFPGA
 * configuration block programming.
 *
 * @copyright:  	<Not Available>
 *
 * History:
 * 23-05-2023: junaidaslamRS Created this file for FPGA Configuration Block
 *Programming
 ****************************************************************************************/

#include "rigel_fpga_config_block.h"

#if !CONFIG_RS_RTOS_PORT
  #include "mpaland_printf.h"
  #include "rs_config_controllers.h"
  #include "rs_util.h"
  #include "string.h"
#endif

static volatile struct rigel_fcb_registers *s_Rigel_FCB_Registers = NULL;
static uint32_t wordline_read_count = 0;

extern void delay_us(uint32_t us);
extern void add_nops(long unsigned int nops_counter);

/* ******************************************
 * @brief: rigel_fcb_early interface sets the
 * bottom signal value to 0 or 1.
 *
 * @param [in]: bool inBottom.
 *
 * @return: xCB_ERROR_CODE
 *
 * @note Set bottom_in to 0 or 1
 * ******************************************/
static enum xCB_ERROR_CODE rigel_fcb_early(bool inBottom) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  if (inBottom) {
    // bottom_in = 1
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
        (1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET);

    if (!read_reg_val(&s_Rigel_FCB_Registers->op_reg,
                      RIGEL_FCB_OP_REG_BOTTOM_OFFSET,
                      RIGEL_FCB_OP_REG_BIT_WIDTHS)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  } else {
    // bottom_in = 0
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
        (1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET);

    if (read_reg_val(&s_Rigel_FCB_Registers->op_reg,
                     RIGEL_FCB_OP_REG_BOTTOM_OFFSET,
                     RIGEL_FCB_OP_REG_BIT_WIDTHS)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: rigel_fcb_force interface resets
 * the word line shift register to 0 or 1.
 *
 * @param [in]: bool inBottom.
 *
 * @return: xCB_ERROR_CODE
 *
 * @note Force Sequence
 *
 * Start with mclk=0 sclk=0
 * On fcb_clock↑: set mclk=1, sclk=1, set bottom_in to 0 or 1
 * On fcb_clock↑: set mclk=0, sclk=0
 * ******************************************/
static enum xCB_ERROR_CODE rigel_fcb_force(bool inBottom) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  // mclk=0 sclk=0
  s_Rigel_FCB_Registers
      ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
      ((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) |
       (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET));
  if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
      read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
    lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
  } else {
    if (inBottom) {  // set mclk=1, sclk=1, set bottom_in to 1
      s_Rigel_FCB_Registers
          ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
          ((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) |
           (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET) |
           (1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET));
      if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                        RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
          !read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                        RIGEL_FCB_OP_REG_SCLK_OFFSET) ||
          !read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                        RIGEL_FCB_OP_REG_BOTTOM_OFFSET)) {
        lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
      }
    } else {  // set bottom_in to 0
      s_Rigel_FCB_Registers
          ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
          (1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET);
      if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                       RIGEL_FCB_OP_REG_BOTTOM_OFFSET)) {
        lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
      } else {  // set mclk=1, sclk=1
        s_Rigel_FCB_Registers
            ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
            ((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) |
             (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET));
        if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                          RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
            !read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                          RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
          lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
        }
      }
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {  // set mclk=0, sclk=0
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
        ((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) |
         (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET));
    if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                     RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
        read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                     RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: rigel_fcb_advance interface pushes
 * 0 or 1 in the wordline shift register.
 *
 * @param [in]: bool inBottom.
 *
 * @return: xCB_ERROR_CODE
 *
 * @note Advance Sequence
 *
 * Start with mclk=0 sclk=0
 * On fcb_clock↑: set mclk=1, set bottom_in to 0 or 1
 * On fcb_clock↑: set mclk=0
 * On fcb_clock↑: set sclk=1
 * On fcb_clock↑: set sclk=0
 * ******************************************/
static enum xCB_ERROR_CODE rigel_fcb_advance(bool inBottom) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  // set mclk=0, sclk=0
  s_Rigel_FCB_Registers
      ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
      ((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) |
       (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET));
  if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
      read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
    lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
  } else {
    if (inBottom) {
      // set mclk=1, set bottom_in to 1
      s_Rigel_FCB_Registers
          ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
          ((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) |
           (1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET));
      if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                        RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
          !read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                        RIGEL_FCB_OP_REG_BOTTOM_OFFSET)) {
        lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
      }
    } else {  // set mclk=1, set bottom_in to 0
      s_Rigel_FCB_Registers
          ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
          (1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET);
      if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                       RIGEL_FCB_OP_REG_BOTTOM_OFFSET)) {
        lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
      } else {
        s_Rigel_FCB_Registers
            ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
            (1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET);
        if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                          RIGEL_FCB_OP_REG_MCLK_OFFSET)) {
          lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
        }
      }
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // set mclk=0
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
        (1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET);
    if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                     RIGEL_FCB_WL_MODE_MCLK_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }

    // set sclk=1
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
        (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET);
    if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                      RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    } else {  // set sclk=0
      s_Rigel_FCB_Registers
          ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
          (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET);
      if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                       RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
        lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
      }
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: rigel_fcb_both interface asserts
 * strobes to even and odd latches.
 *
 * @return: xCB_ERROR_CODE
 *
 * @note BOTH Sequence
 *
 * Start with estrobe=ostrobe=0
 * On fcb_clock↑: set estrobe=ostrobe=1
 * On fcb_clock↑: set estrobe=ostrobe=0
 * ******************************************/
static enum xCB_ERROR_CODE rigel_fcb_both(void) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  // estrobe=ostrobe=0
  s_Rigel_FCB_Registers
      ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
      ((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) |
       (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
  if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
      read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
    lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
  } else {
    // set estrobe=ostrobe=1
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
        ((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) |
         (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
    if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                      RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
        !read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                      RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  }

  /* **************************************************************
   * As per the calculations shifting the bits
   * across the array and then strobing the wordline,
   * it was required that a 276ns delay be added.
   * If the system is running at 266MHz on PLL, then
   * the WL strobe (on-period), which is by default
   * 4 clock cycles of system clock, is of 15ns in length.
   * Now comparing it with 276ns, this time is very small
   * for bitlines to settle. We have now intentionally
   * stretched WL strobe (on-period) to 80 clock cycles
   * (through WL block’s configurations) using add_nops which
   * corresponds to 300ns. This time should be enough to settle
   * down the bitline without compromising on the system frequency.
   * **************************************************************/
  add_nops(10);

  // set estrobe=ostrobe=0
  if (lvErrorCode == xCB_SUCCESS) {
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
        ((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) |
         (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
    if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                     RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
        read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                     RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: rigel_fcb_even interface asserts
 * strobes to even latch.
 *
 * @return: xCB_ERROR_CODE
 *
 * @note EVEN Sequence
 *
 * Start with estrobe=ostrobe=0
 * On fcb_clock↑: set estrobe=1
 * On fcb_clock↑: set estrobe=0
 * ******************************************/
__attribute__((unused)) static enum xCB_ERROR_CODE rigel_fcb_even(void) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  // estrobe=ostrobe=0
  s_Rigel_FCB_Registers
      ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
      ((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) |
       (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
  if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
      read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
    lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
  } else {
    // set estrobe=1
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
        (1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET);
    if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                      RIGEL_FCB_OP_REG_ESTROBE_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  }

  // set estrobe=0
  if (lvErrorCode == xCB_SUCCESS) {
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
        (1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET);
    if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                     RIGEL_FCB_OP_REG_ESTROBE_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: rigel_fcb_odd interface asserts
 * strobes to odd latch.
 *
 * @return: xCB_ERROR_CODE
 *
 * @note ODD Sequence
 *
 * Start with estrobe=ostrobe=0
 * On fcb_clock↑: set ostrobe=1
 * On fcb_clock↑: set ostrobe=0
 * ******************************************/
__attribute__((unused)) static enum xCB_ERROR_CODE rigel_fcb_odd(void) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  // estrobe=ostrobe=0
  s_Rigel_FCB_Registers
      ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
      ((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) |
       (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
  if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
      read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                   RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
    lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
  } else {
    // set ostrobe=1
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
        (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET);
    if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                      RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  }

  // set ostrobe=0
  if (lvErrorCode == xCB_SUCCESS) {
    s_Rigel_FCB_Registers
        ->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
        (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET);
    if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
                     RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
      lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: Rigel_FCB_Init interface sets the
 * required parameters to work on specific
 * platform.
 *
 * @param [in]: uint32_t inBaseAddr.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Init(uint32_t inBaseAddr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  if (inBaseAddr == 0x00) {
    lvErrorCode = xCB_ERROR_INVALID_DATA;
  } else {
    s_Rigel_FCB_Registers = ((struct rigel_fcb_registers *)inBaseAddr);
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: Rigel_FCB_Config_Begin interface
 * is used to perform FCB registers settings
 * before kicking off the payload transfer.
 *
 * @param [in]: fcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 *
 * @note Sequence before starting transfer
 * of the payload:
 *
 * Set BL_Status->BL_CLR to Clear BL_WR_CNT
 * Clear Status
 * Store BL_Checksum
 * Enable BL_Checksum
 * Clear CFG_DONE
 * EARLY 1
 * FORCE 1
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Config_Begin(
    struct rigel_fcb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  (void)rs_sec_tfr_ptr;

  // Clear Status
  s_Rigel_FCB_Registers->status = 0x00;
  if (s_Rigel_FCB_Registers->status != 0x00) {
    lvErrorCode = xCB_ERROR_WRITE_ERROR;
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Set BL_Status->BL_CLR to Clear BL_WR_CNT and Checksum Status
    write_reg_val(&s_Rigel_FCB_Registers->bl_status,
                  RIGEL_FCB_BL_STATUS_BL_CLR_OFFSET,
                  RIGEL_FCB_BL_STATUS_BL_CLR_WIDTH, xCB_SET);
    if (read_reg_val(&s_Rigel_FCB_Registers->bl_status,
                     RIGEL_FCB_BL_STATUS_BL_WR_CNT_OFFSET,
                     RIGEL_FCB_BL_STATUS_BL_WR_CNT_WIDTH) != 0x00) {
      lvErrorCode = xCB_ERROR_UNEXPECTED_VALUE;
    }
  }

  // If it's not a readback and checksum bit is set in the header, store
  // BL_Checksum and enable checksum
  if ((lvErrorCode == xCB_SUCCESS) && (!inHeader->readback) &&
      (inHeader->generic_hdr.action_enum & RS_ACTION_CHECKSUM_PRESENT_MASK)) {
    s_Rigel_FCB_Registers->checksum_word = inHeader->bitstream_checksum;

    if (s_Rigel_FCB_Registers->checksum_word != inHeader->bitstream_checksum) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    }

    if (lvErrorCode == xCB_SUCCESS) {
      // Enable BL_Checksum
      write_reg_val(&s_Rigel_FCB_Registers->status,
                    RIGEL_FCB_STATUS_BL_PRECHECKSUM_EN_OFFSET,
                    RIGEL_FCB_STATUS_BIT_WIDTHS, xCB_SET);
      if (read_reg_val(&s_Rigel_FCB_Registers->status,
                       RIGEL_FCB_STATUS_BL_PRECHECKSUM_EN_OFFSET,
                       RIGEL_FCB_STATUS_BIT_WIDTHS) != xCB_SET) {
        lvErrorCode = xCB_ERROR_WRITE_ERROR;
      }
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Clear CFG_DONE
    write_reg_val(&s_Rigel_FCB_Registers->status,
                  RIGEL_FCB_STATUS_CFG_DONE_EN_OFFSET,
                  RIGEL_FCB_STATUS_BIT_WIDTHS, xCB_RESET);
    if (read_reg_val(&s_Rigel_FCB_Registers->status,
                     RIGEL_FCB_STATUS_CFG_DONE_EN_OFFSET,
                     RIGEL_FCB_STATUS_BIT_WIDTHS) != xCB_RESET) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {
    if (inHeader->readback) {
      // Setup readback
      // Set wordline read count to 0
      wordline_read_count = 0;

      // FORCE 0
      lvErrorCode = rigel_fcb_force(0);

      // EARLY 1
      if (lvErrorCode == xCB_SUCCESS) {
        lvErrorCode = rigel_fcb_early(1);
      }
    } else {
      // Setup write
      // EARLY 1
      lvErrorCode = rigel_fcb_early(1);

      // FORCE 1
      if (lvErrorCode == xCB_SUCCESS) {
        lvErrorCode = rigel_fcb_force(1);
      }
    }
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: Rigel_FCB_Config_End interface
 * is used to perform FCB registers settings
 * after the payload transfer.
 *
 * @param [in]: fcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 *
 * Check BL_CHECKSUM_STATUS
 * If ok Set CONFIG_DONE
 * Clear BL_PRECHECKSUM_EN
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Config_End(
    struct rigel_fcb_bitstream_header *inHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  (void)inHeader;

  // If it was not a readback and checksum enable was asserted
  if ((!inHeader->readback) &&
      (read_reg_val(&s_Rigel_FCB_Registers->status,
                    RIGEL_FCB_STATUS_BL_PRECHECKSUM_EN_OFFSET,
                    RIGEL_FCB_STATUS_BIT_WIDTHS) == xCB_SET)) {
    // Check BL_CHECKSUM_STATUS
    if (read_reg_val(&s_Rigel_FCB_Registers->status,
                     RIGEL_FCB_STATUS_BL_CHECKSUM_STATUS_OFFSET,
                     RIGEL_FCB_STATUS_BIT_WIDTHS) != xCB_SET) {
      lvErrorCode = xCB_ERROR_CHECKSUM_MATCH_FAILED;
    }
  }

  if (lvErrorCode == xCB_SUCCESS) {  // If ok Set CONFIG_DONE
    write_reg_val(&s_Rigel_FCB_Registers->status,
                  RIGEL_FCB_STATUS_CFG_DONE_EN_OFFSET,
                  RIGEL_FCB_STATUS_BIT_WIDTHS, xCB_SET);
    if (read_reg_val(&s_Rigel_FCB_Registers->status,
                     RIGEL_FCB_STATUS_CFG_DONE_EN_OFFSET,
                     RIGEL_FCB_STATUS_BIT_WIDTHS) != xCB_SET) {
      lvErrorCode = xCB_ERROR_WRITE_ERROR;
    }
  }

  // Clear BL_PRECHECKSUM_EN
  write_reg_val(&s_Rigel_FCB_Registers->status,
                RIGEL_FCB_STATUS_BL_PRECHECKSUM_EN_OFFSET,
                RIGEL_FCB_STATUS_BIT_WIDTHS, xCB_RESET);
  if (read_reg_val(&s_Rigel_FCB_Registers->status,
                   RIGEL_FCB_STATUS_BL_PRECHECKSUM_EN_OFFSET,
                   RIGEL_FCB_STATUS_BIT_WIDTHS) != xCB_RESET) {
    lvErrorCode = xCB_ERROR_WRITE_ERROR;
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s\r\n", Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: FCB_Payload_kickoff interface is
 * used to transfer the bitstreams to/from
 * FCB configuration controller.
 *
 * @param [in]: fcb_bitstream_header
 * @param [in]: rs_secure_transfer_info
 *
 * @return: xCB_ERROR_CODE
 *
 * Write Sequence:
 * FILL the Bitline Register
 * Execute BOTH
 * Execute ADVANCE 0
 *
 * Read Sequence:
 * READBACK
 * Execute ADVANCE 1
 * Execute EVEN or ODD depending wordline number
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Payload_kickoff(
    struct rigel_fcb_bitstream_header *inHeader,
    struct rs_secure_transfer_info *rs_sec_tfr_ptr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  if (inHeader->bitline_reg_width != (rs_sec_tfr_ptr->transfer_addr.len)) {
    lvErrorCode = xCB_ERROR_INVALID_DATA_LENGTH;
  } else if (inHeader->readback) {
    rs_sec_tfr_ptr->transfer_addr.tfr_type = RS_SECURE_RX;
    rs_sec_tfr_ptr->transfer_addr.read_addr =
        (uint32_t)&s_Rigel_FCB_Registers->bl_rdata;

    // Set BL_Status->sel_bl_source to load bl_data_in in readback register,
    // then clear it
    write_reg_val(&s_Rigel_FCB_Registers->bl_status,
                  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_OFFSET,
                  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_WIDTH, xCB_SET);
    write_reg_val(&s_Rigel_FCB_Registers->bl_status,
                  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_OFFSET,
                  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_WIDTH, xCB_RESET);
  } else {
    rs_sec_tfr_ptr->transfer_addr.write_addr =
        (uint32_t)&s_Rigel_FCB_Registers->bl_wdata;
    rs_sec_tfr_ptr->transfer_addr.tfr_type = RS_SECURE_TX;
  }

  if (inHeader->readback) {  // For Readback Make BL_Gating 1
    s_Rigel_FCB_Registers->op_reg |= (1 << RIGEL_FCB_OP_REG_BL_GATING_OFFSET);
  } else {  // For Configuration Make BL_Gating 0
    s_Rigel_FCB_Registers->op_reg &= ~(1 << RIGEL_FCB_OP_REG_BL_GATING_OFFSET);
  }

  // Perform the data transfer (FILL or READBACK)
  if ((lvErrorCode == xCB_SUCCESS) &&
      (rs_sec_tfr_ptr->rs_secure_transfer((void *)rs_sec_tfr_ptr) !=
       CRYPTO_SUCCESS)) {
    lvErrorCode = xCB_ERROR;
  }

  if (inHeader->readback) {  // For Readback Make BL_Gating 0
    s_Rigel_FCB_Registers->op_reg &= ~(1 << RIGEL_FCB_OP_REG_BL_GATING_OFFSET);
  } else {  // For Configuration Make BL_Gating 1
    s_Rigel_FCB_Registers->op_reg |= (1 << RIGEL_FCB_OP_REG_BL_GATING_OFFSET);
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Perform operations depending on whether it's a data configuration and
    // readback
    if (inHeader->readback) {
      // ADVANCE 1
      lvErrorCode = rigel_fcb_advance(1);

      if (lvErrorCode == xCB_SUCCESS) {
        // Both
        lvErrorCode = rigel_fcb_both();  // If the below line is to be used then
                                         // remove or comment this line
        // EVEN or ODD
        // lvErrorCode =
        //     (wordline_read_count % 2 == 0 ? rigel_fcb_even() :
        //     rigel_fcb_odd());
        wordline_read_count++;
      }
    } else {
      // Execute BOTH
      lvErrorCode = rigel_fcb_both();

      // Execute ADVANCE 0
      if (lvErrorCode == xCB_SUCCESS) {
        lvErrorCode = rigel_fcb_advance(0);
      }
    }
  }

  if ((lvErrorCode == xCB_SUCCESS) && (!inHeader->readback)) {
    uint32_t write_count = read_reg_val(&s_Rigel_FCB_Registers->bl_status,
                                        RIGEL_FCB_BL_STATUS_BL_WR_CNT_OFFSET,
                                        RIGEL_FCB_BL_STATUS_BL_WR_CNT_WIDTH);

    if (write_count !=
        (rs_sec_tfr_ptr->transfer_addr.len * xCB_BITS_IN_A_BYTE)) {
      lvErrorCode = xCB_ERROR_DATA_CORRUPTED;
    }

    // Note: Do not Set BL_Status->BL_CLR to 0x00 to clear BL_WR_CNT.
    // It will also clear Checksum status which is not desired.
    // BL_WR_CNT will automatically wrap to 32 on the last word transaction
    // of the ongoing block_size.
  }

  if (lvErrorCode != xCB_SUCCESS) {
    RS_LOG_ERROR("FCB", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(lvErrorCode));
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: Rigel_FCB_Bitstream_Header_Parser
 * interface will parse the header of FCB
 * config controller type.
 *
 * @param [in]: inBitStream is a pointer to
 * bitstream.
 * @param [out]: outHeader contains the
 * parsed header information.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE Rigel_FCB_Bitstream_Header_Parser(void *inBitStream,
                                                      void *outHeader) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rigel_fcb_bitstream_header *lvBitStream_Header =
      (struct rigel_fcb_bitstream_header *)outHeader;
  uint8_t *lvBitstream = (uint8_t *)inBitStream;

  if ((inBitStream == NULL) || (outHeader == NULL)) {
    lvErrorCode = xCB_ERROR_NULL_POINTER;
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Copy generic header as it is and move data pointer ahead
    memcpy((void *)lvBitStream_Header, (void *)lvBitstream,
           sizeof(struct rs_action_header));
    lvBitstream += sizeof(struct rs_action_header);

    // Read checksum if it's present
    if (lvBitStream_Header->generic_hdr.action_enum &
        RS_ACTION_CHECKSUM_PRESENT_MASK) {
      lvBitStream_Header->bitstream_checksum = *(uint32_t *)lvBitstream;
      lvBitstream += sizeof(lvBitStream_Header->bitstream_checksum);
    } else {
      lvBitStream_Header->bitstream_checksum = 0x0;
    }

    // Read bitline register width and readback bit
    lvBitStream_Header->bitline_reg_width =
        (*(uint32_t *)lvBitstream & RIGEL_FCB_HDR_BITLINE_REG_WIDTH_MASK) >>
        RIGEL_FCB_HDR_BITLINE_REG_WIDTH_OFFSET;
    lvBitStream_Header->readback =
        (*(uint32_t *)lvBitstream & RIGEL_FCB_HDR_READBACK_MASK) >>
        RIGEL_FCB_HDR_READBACK_OFFSET;
  }

  return lvErrorCode;
}

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
    uint32_t *outBitStr_Size) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
  struct rigel_fcb_bitstream_header *lvBitStream_Header =
      (struct rigel_fcb_bitstream_header *)inHeader;

  if ((inHeader == NULL) || (outTransferType == NULL) ||
      (outBitStr_Size == NULL)) {
    lvErrorCode = xCB_ERROR_NULL_POINTER;
  } else {
    *outTransferType =
        (lvBitStream_Header->readback ? TRANSFER_TYPE_RX : TRANSFER_TYPE_TX);
    *outBitStr_Size = lvBitStream_Header->generic_hdr.payload_size;
    *outBlockSize = lvBitStream_Header->bitline_reg_width;
  }

  return lvErrorCode;
}
