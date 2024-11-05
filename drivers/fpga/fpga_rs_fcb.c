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
LOG_MODULE_REGISTER(rs_fpga_fcb);

#if DT_HAS_COMPAT_STATUS_OKAY(rapidsi_fcb)
#define DT_DRV_COMPAT rigel_fcb
static volatile struct rigel_fcb_registers *s_Rigel_FCB_Registers = NULL;
static uint32_t wordline_read_count = 0;
#else
#error Rapid Silicon FCB IP is not enabled in the Device Tree
#endif

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
static enum xCB_ERROR_CODE rigel_fcb_early(bool inBottom)
{
	enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

	if (inBottom) {
		// bottom_in = 1
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
			(1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET);

		if (!read_reg_val(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_BOTTOM_OFFSET,
				  RIGEL_FCB_OP_REG_BIT_WIDTHS)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}
	} else {
		// bottom_in = 0
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
			(1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET);

		if (read_reg_val(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_BOTTOM_OFFSET,
				 RIGEL_FCB_OP_REG_BIT_WIDTHS)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}
	}

	if (lvErrorCode != xCB_SUCCESS) {
		PRINT_ERROR(lvErrorCode);
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
static enum xCB_ERROR_CODE rigel_fcb_force(bool inBottom)
{
	enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
	// mclk=0 sclk=0
	s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
		((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) | (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET));
	if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
	    read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
		lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
	} else {
		if (inBottom) { // set mclk=1, sclk=1, set bottom_in to 1
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
		} else { // set bottom_in to 0
			s_Rigel_FCB_Registers
				->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
				(1 << RIGEL_FCB_WL_MODE_BOTTOM_OFFSET);
			if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
					 RIGEL_FCB_OP_REG_BOTTOM_OFFSET)) {
				lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
			} else { // set mclk=1, sclk=1
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

	if (lvErrorCode == xCB_SUCCESS) { // set mclk=0, sclk=0
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
			((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) |
			 (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET));
		if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
		    read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}
	}

	if (lvErrorCode != xCB_SUCCESS) {
		PRINT_ERROR(lvErrorCode);
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
static enum xCB_ERROR_CODE rigel_fcb_advance(bool inBottom)
{
	enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

	// set mclk=0, sclk=0
	s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
		((1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET) | (1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET));
	if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_MCLK_OFFSET) ||
	    read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
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
		} else { // set mclk=1, set bottom_in to 0
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
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
			(1 << RIGEL_FCB_WL_MODE_MCLK_OFFSET);
		if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_WL_MODE_MCLK_OFFSET)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}

		// set sclk=1
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
			(1 << RIGEL_FCB_WL_MODE_SCLK_OFFSET);
		if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_SCLK_OFFSET)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		} else { // set sclk=0
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
		PRINT_ERROR(lvErrorCode);
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
static enum xCB_ERROR_CODE rigel_fcb_both(void)
{
	enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

	// estrobe=ostrobe=0
	s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
		((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) | (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
	if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
	    read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
		lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
	} else {
		// set estrobe=ostrobe=1
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
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
	for(volatile int i = 10; i > 0;) {i--;} // Volatile to avoid optimization

	// set estrobe=ostrobe=0
	if (lvErrorCode == xCB_SUCCESS) {
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
			((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) |
			 (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
		if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
		    read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}
	}

	if (lvErrorCode != xCB_SUCCESS) {
		PRINT_ERROR(lvErrorCode);
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
__attribute__((unused)) static enum xCB_ERROR_CODE rigel_fcb_even(void)
{
	enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

	// estrobe=ostrobe=0
	s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
		((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) | (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
	if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
	    read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
		lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
	} else {
		// set estrobe=1
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
			(1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET);
		if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
				  RIGEL_FCB_OP_REG_ESTROBE_OFFSET)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}
	}

	// set estrobe=0
	if (lvErrorCode == xCB_SUCCESS) {
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
			(1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET);
		if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_ESTROBE_OFFSET)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}
	}

	if (lvErrorCode != xCB_SUCCESS) {
		PRINT_ERROR(lvErrorCode);
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
__attribute__((unused)) static enum xCB_ERROR_CODE rigel_fcb_odd(void)
{
	enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

	// estrobe=ostrobe=0
	s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
		((1 << RIGEL_FCB_WL_MODE_ESTROBE_OFFSET) | (1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET));
	if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_ESTROBE_OFFSET) ||
	    read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
		lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
	} else {
		// set ostrobe=1
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_SET_OR_MPU_0_NO_CHANGE] =
			(1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET);
		if (!read_reg_bit(&s_Rigel_FCB_Registers->op_reg,
				  RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}
	}

	// set ostrobe=0
	if (lvErrorCode == xCB_SUCCESS) {
		s_Rigel_FCB_Registers->wl_mode[RIGEL_FCB_WL_MODE_MPU_1_CLR_OR_MPU_0_NO_CHANGE] =
			(1 << RIGEL_FCB_WL_MODE_OSTROBE_OFFSET);
		if (read_reg_bit(&s_Rigel_FCB_Registers->op_reg, RIGEL_FCB_OP_REG_OSTROBE_OFFSET)) {
			lvErrorCode = xCB_ERROR_INVALID_BIT_VALUE;
		}
	}

	if (lvErrorCode != xCB_SUCCESS) {
		PRINT_ERROR(lvErrorCode);
	}

	return lvErrorCode;
}

/* ******************************************
 * @brief: Rigel_FCB_Bitstream_Header_Parser
 * interface will parse the header of FCB
 * config controller type.
 *
 * @param [in]: inUserData is a pointer to
 * User Data (which in this case is expected
 * to be FCB MetaData / Header).
 * @param [out]: outHeader contains the
 * parsed header information.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
static enum xCB_ERROR_CODE Rigel_FCB_Bitstream_Header_Parser(void *inUserData, void *outHeader)
{
	enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;
	struct rigel_fcb_bitstream_header *lvBitStream_Header =
		(struct rigel_fcb_bitstream_header *)outHeader;
	uint8_t *lvBitstream = (uint8_t *)inUserData;

	if ((inUserData == NULL) || (outHeader == NULL)) {
		lvErrorCode = xCB_ERROR_NULL_POINTER;
	}

	if (lvErrorCode == xCB_SUCCESS) {
		// Copy generic header as it is and move data pointer ahead
		memcpy((void *)lvBitStream_Header, (void *)lvBitstream,
		       sizeof(struct rs_action_header));
		lvBitstream += sizeof(struct rs_action_header);

		// Read checksum if it's present
		if (lvBitStream_Header->generic_hdr.action_enum & RS_ACTION_CHECKSUM_PRESENT_MASK) {
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

static /* ******************************************
	* @brief: Rigel_FCB_Config_Begin interface
	* is used to perform FCB registers settings
	* before kicking off the payload transfer.
	*
	* @param [in]: fcb_bitstream_header
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
	enum xCB_ERROR_CODE Rigel_FCB_Config_Begin(struct rigel_fcb_bitstream_header *inHeader)
{
	enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

	// Clear Status
	s_Rigel_FCB_Registers->status = 0x00;
	if (s_Rigel_FCB_Registers->status != 0x00) {
		lvErrorCode = xCB_ERROR_WRITE_ERROR;
	}

	if (lvErrorCode == xCB_SUCCESS) {
		// Set BL_Status->BL_CLR to Clear BL_WR_CNT and Checksum Status
		write_reg_val(&s_Rigel_FCB_Registers->bl_status, RIGEL_FCB_BL_STATUS_BL_CLR_OFFSET,
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
		write_reg_val(&s_Rigel_FCB_Registers->status, RIGEL_FCB_STATUS_CFG_DONE_EN_OFFSET,
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
		PRINT_ERROR(lvErrorCode);
	}

	return lvErrorCode;
}

/* ******************************************
 * @brief: Rigel_FCB_Config_End interface
 * is used to perform FCB registers settings
 * after the payload transfer.
 *
 * @param [in]: rigel_fcb_bitstream_header
 *
 * @return: xCB_ERROR_CODE
 *
 * Check BL_CHECKSUM_STATUS
 * If ok Set CONFIG_DONE
 * Clear BL_PRECHECKSUM_EN
 * ******************************************/
static enum xCB_ERROR_CODE Rigel_FCB_Config_End(struct rigel_fcb_bitstream_header *inHeader)
{
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

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
    LOG_ERR("FCB Configuration Failure Error Code:%d\r\n", lvErrorCode);
  }

  return lvErrorCode;
}

/* ******************************************
 * @brief: FCB_Payload_kickoff interface is
 * used to transfer the bitstreams to/from
 * FCB configuration controller.
 *
 * @param [in]: dev device structure
 * @param [in]: image_ptr to bitstream
 * @param [in]: img_size is size of the data
 * pointed to by image_ptr
 *
 * @return: error
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
int fcb_load(const struct device *dev, uint32_t *image_ptr, uint32_t img_size)
{
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  struct fcb_data *lvData = (struct fcb_data *)dev->data;

  if (lvData->fcb_header.bitline_reg_width != img_size) {
    lvErrorCode = xCB_ERROR_INVALID_DATA_LENGTH;
  } else if (lvData->fcb_header.readback) {
	lvData->ctx->src_addr = (uint8_t*)&s_Rigel_FCB_Registers->bl_rdata;
    // Set BL_Status->sel_bl_source to load bl_data_in in readback register,
    // then clear it
    write_reg_val(&s_Rigel_FCB_Registers->bl_status,
                  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_OFFSET,
                  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_WIDTH, xCB_SET);
    write_reg_val(&s_Rigel_FCB_Registers->bl_status,
                  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_OFFSET,
                  RIGEL_FCB_BL_STATUS_SEL_BL_SRC_WIDTH, xCB_RESET);
  } else {
	lvData->ctx->dest_addr = (uint8_t*)&s_Rigel_FCB_Registers->bl_wdata;
  }

  if(lvErrorCode == xCB_SUCCESS)
  {
	if (lvData->fcb_header.readback) {  // For Readback Make BL_Gating 1
		s_Rigel_FCB_Registers->op_reg |= (1 << RIGEL_FCB_OP_REG_BL_GATING_OFFSET);
	} else {  // For Configuration Make BL_Gating 0
		s_Rigel_FCB_Registers->op_reg &= ~(1 << RIGEL_FCB_OP_REG_BL_GATING_OFFSET);
	}
  }

  // Perform the data transfer (FILL or READBACK)
  if ((lvErrorCode == xCB_SUCCESS) &&
	  (lvData->ctx->bitstr_load_hndlr(lvData->ctx) != xCB_SUCCESS)) {
    lvErrorCode = xCB_ERROR;
  }

  if(lvErrorCode == xCB_SUCCESS)
  {
	if (lvData->fcb_header.readback) {  // For Readback Make BL_Gating 0
		s_Rigel_FCB_Registers->op_reg &= ~(1 << RIGEL_FCB_OP_REG_BL_GATING_OFFSET);
	} else {  // For Configuration Make BL_Gating 1
		s_Rigel_FCB_Registers->op_reg |= (1 << RIGEL_FCB_OP_REG_BL_GATING_OFFSET);
	}
  }

  if (lvErrorCode == xCB_SUCCESS) {
    // Perform operations depending on whether it's a data configuration and
    // readback
    if (lvData->fcb_header.readback) {
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

  if ((lvErrorCode == xCB_SUCCESS) && (!lvData->fcb_header.readback)) {
    uint32_t write_count = read_reg_val(&s_Rigel_FCB_Registers->bl_status,
                                        RIGEL_FCB_BL_STATUS_BL_WR_CNT_OFFSET,
                                        RIGEL_FCB_BL_STATUS_BL_WR_CNT_WIDTH);

    if (write_count !=
        (lvData->fcb_header.bitline_reg_width * xCB_BITS_IN_A_BYTE)) {
      lvErrorCode = xCB_ERROR_DATA_CORRUPTED;
    }

    // Note: Do not Set BL_Status->BL_CLR to 0x00 to clear BL_WR_CNT.
    // It will also clear Checksum status which is not desired.
    // BL_WR_CNT will automatically wrap to 32 on the last word transaction
    // of the ongoing block_size.
  }

  if (lvErrorCode != xCB_SUCCESS) {
    LOG_ERR("%s(%d) Error Code:%d\r\n", __func__, __LINE__, lvErrorCode);
  }

  return lvErrorCode;
}

enum FPGA_status fcb_get_status(const struct device *dev)
{
	const struct fcb_data *lvData = (struct fcb_data *)dev->data;
	return lvData->fpgaStatus;
}

int fcb_session_start(const struct device *dev, struct fpga_ctx *ctx)
{
	int error = xCB_SUCCESS;

	struct fcb_data *lvData = (struct fcb_data *)dev->data;

	lvData->ctx = ctx; // pointing to the fpga context for later operations.

	lvData->ctx->device = dev;

	if(s_Rigel_FCB_Registers == NULL) {
		error = xCB_ERROR_NULL_POINTER;
	}

	if(error == xCB_SUCCESS)
	{    
		if (lvData->ctx->meta_data == NULL) { // FCB Meta Data is Expected
			error = xCB_ERROR_NULL_POINTER;
		} else {
			error = Rigel_FCB_Bitstream_Header_Parser(lvData->ctx->meta_data,
						(void *)&lvData->fcb_header);
		}
		lvData->ctx->meta_data_per_block = false;
	}

	if (error == xCB_SUCCESS) {
		error = Rigel_FCB_Config_Begin(&lvData->fcb_header);  
	}

	if(error != xCB_SUCCESS) {
		PRINT_ERROR(error);
		error = -ECANCELED;
	}

	return error;
}

int fcb_session_free(const struct device *dev)
{
  int error = 0;

  struct fcb_data *lvData = (struct fcb_data *)dev->data;

  error = Rigel_FCB_Config_End(&lvData->fcb_header);

  s_Rigel_FCB_Registers = NULL;

  lvData->fpgaStatus = FPGA_STATUS_INACTIVE;

  lvData->ctx = NULL;  

  lvData->ctx->device = NULL;

  if(error != xCB_SUCCESS) {
    error = -ECANCELED;
  }

  return error;
}

int fcb_reset(const struct device *dev)
{
	int error = xCB_SUCCESS;

	#if CONFIG_RAPIDSI_OFE
		const struct device *ofe = DEVICE_DT_GET(DT_NODELABEL(ofe));
		if (ofe == NULL) {
			LOG_ERR("%s(%d) Error with OFE initialization\r\n", __func__, __LINE__);
			error = -ENOSYS;
		}
	#else
		#error "Enable OFE from the device tree to meet FCB dependency."
	#endif

	// Sending the reset pulse to global fpga reset.
	if (error == 0) {
		if ((ofe_reset(ofe, OFE_RESET_SUBSYS_FCB, 0x0) != 0) || /* RESET */
		    (ofe_reset(ofe, OFE_RESET_SUBSYS_FCB, 0x1) != 0)) { /* SET */
			LOG_ERR("%s(%d) global fpga reset error\r\n", __func__, __LINE__);
			error = -EIO;
		}
	}

	return error;
}

static int fcb_engine_on(bool value)
{
  int error = xCB_SUCCESS;

  #if CONFIG_RAPIDSI_OFE
    const struct device *ofe = DEVICE_DT_GET(DT_NODELABEL(ofe));
    if (ofe == NULL) {
      LOG_ERR("%s(%d) Error with OFE initialization\r\n", __func__, __LINE__);
      error = -ENOSYS;
    }
  #else
    #error "Enable OFE from the device tree to to meet FCB dependency."
  #endif

	if (error == 0) {
		// Setting the isolation bit to allow / prohibit writing the fabric.
    // 1 to alow writing and 0 to prohibit writing to fabric.
		scu_set_isolation_ctrl(ISOLATION_CTRL_FCB_OFFSET, value);
	}

	// Sending the reset pulse to global fpga reset.
	if (error == 0) {
    if(value) {
      if ((ofe_reset(ofe, OFE_RESET_SUBSYS_FCB, 0x0) != 0) || /* RESET */
          (ofe_reset(ofe, OFE_RESET_SUBSYS_FCB, 0x1) != 0)) { /* SET to Release Reset */
        LOG_ERR("%s(%d) global fpga reset release error\r\n", __func__, __LINE__);
        error = -EIO;
      }
    }
    else {
      if(ofe_reset(ofe, OFE_RESET_SUBSYS_FCB, 0x0) != 0) { /* Held in RESET */
        LOG_ERR("%s(%d) global fpga reset held error\r\n", __func__, __LINE__);
        error = -EIO;
      }
    }
	}

  return error;
}

int fcb_on(const struct device *dev)
{
  int error = fcb_engine_on(true);
  return error;
}

int fcb_off(const struct device *dev)
{
  int error = fcb_engine_on(false);
  return error;
}

const char *fcb_get_info(const struct device *dev)
{
	static struct fpga_transfer_param lvFCBTransferParam = {0};
	struct rigel_fcb_bitstream_header *lvFCBBitstrHeader =
		&((struct fcb_data *)(dev->data))->fcb_header;

	if (lvFCBBitstrHeader != NULL) {
		lvFCBTransferParam.FPGA_Transfer_Type =
			lvFCBBitstrHeader->readback ? FPGA_TRANSFER_TYPE_RX : FPGA_TRANSFER_TYPE_TX;
		lvFCBTransferParam.FCB_Transfer_Block_Size = lvFCBBitstrHeader->bitline_reg_width;
		lvFCBTransferParam.FCB_Bitstream_Size = lvFCBBitstrHeader->generic_hdr.payload_size;
	} else {
		return NULL;
	}

	return (char *)&lvFCBTransferParam;
}

static struct fpga_driver_api rigel_fcb_api = {.get_status = fcb_get_status,
					       .get_info = fcb_get_info,
					       .load = fcb_load,
					       .off = fcb_off,
					       .on = fcb_on,
					       .reset = fcb_reset,
					       .session_free = fcb_session_free,
					       .session_start = fcb_session_start};

static int fcb_init(const struct device *dev)
{
	int error = xCB_SUCCESS;

	s_Rigel_FCB_Registers = ((struct rigel_fcb_registers *)((struct fcb_config *)dev->config).base);

	struct fcb_data *lvData = (struct fcb_data *)dev->data;

  	error = fcb_on(dev); // By default turn on the fabric to be written with bitstreams.

	if (error == 0) {
		if (s_Rigel_FCB_Registers != NULL) {
			lvData->fpgaStatus = FPGA_STATUS_ACTIVE;
		} else {
			LOG_ERR("%s(%d) FCB Register Cluster Initialized to NULL\r\n", __func__,
				__LINE__);
			error = -ENOSYS;
		}
	}

	return error;
}

static struct fcb_data s_fcb_data = {.ctx = NULL, .fpgaStatus = FPGA_STATUS_INACTIVE};

static struct fcb_config s_fcb_config = {.base = DT_REG_ADDR(DT_NODELABEL(fcb))};

DEVICE_DT_DEFINE(DT_NODELABEL(fcb), fcb_init, /*Put Init func here*/
		 NULL,                        // Power Management
		 &s_fcb_data,                 // Data structure
		 &s_fcb_config,               // Configuration Structure
		 POST_KERNEL, CONFIG_RS_XCB_INIT_PRIORITY,
		 &rigel_fcb_api // Driver API
)
