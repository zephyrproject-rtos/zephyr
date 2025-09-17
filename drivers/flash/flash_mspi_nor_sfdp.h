/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_FLASH_MSPI_NOR_USE_SFDP

#define BFP_DW16_SOFT_RESET_66_99 BIT(4)

#define BFP_DW16_4B_ADDR_ENTER_B7    BIT(0)
#define BFP_DW16_4B_ADDR_ENTER_06_B7 BIT(1)
#define BFP_DW16_4B_ADDR_PER_CMD     BIT(5)
#define BFP_DW16_4B_ADDR_ALWAYS      BIT(6)

#define BFP_DW18_CMD_EXT_SAME 0
#define BFP_DW18_CMD_EXT_INV  1

/* 32-bit words in SFDP arrays in devicetree are stored in little-endian byte
 * order. See jedec,jesd216.yaml
 */
#define SFDP_DW_BYTE_0_IDX(dw_no) \
	UTIL_DEC(UTIL_DEC(UTIL_DEC(UTIL_DEC(UTIL_X2(UTIL_X2(dw_no))))))
#define SFDP_DW_BYTE_1_IDX(dw_no) \
	UTIL_DEC(UTIL_DEC(UTIL_DEC(UTIL_X2(UTIL_X2(dw_no)))))
#define SFDP_DW_BYTE_2_IDX(dw_no) \
	UTIL_DEC(UTIL_DEC(UTIL_X2(UTIL_X2(dw_no))))
#define SFDP_DW_BYTE_3_IDX(dw_no) \
	UTIL_DEC(UTIL_X2(UTIL_X2(dw_no)))
#define SFDP_DW_BYTE(inst, prop, dw_no, byte_idx) \
	DT_INST_PROP_BY_IDX(inst, prop, SFDP_DW_BYTE_##byte_idx##_IDX(dw_no))
#define SFDP_DW_EXISTS(inst, prop, dw_no) \
	DT_INST_PROP_HAS_IDX(inst, prop, SFDP_DW_BYTE_3_IDX(dw_no))

#define SFDP_DW(inst, prop, dw_no) \
	COND_CODE_1(SFDP_DW_EXISTS(inst, prop, dw_no), \
		(((SFDP_DW_BYTE(inst, prop, dw_no, 3) << 24) | \
		  (SFDP_DW_BYTE(inst, prop, dw_no, 2) << 16) | \
		  (SFDP_DW_BYTE(inst, prop, dw_no, 1) << 8) | \
		  (SFDP_DW_BYTE(inst, prop, dw_no, 0) << 0))), \
		(0))

#define SFDP_FIELD(inst, prop, dw_no, mask) \
	FIELD_GET(mask, SFDP_DW(inst, prop, dw_no))

#define USES_8D_8D_8D(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_OCTAL && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_DUAL)
#define USES_8S_8S_8S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_OCTAL && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_1S_8D_8D(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_OCTAL_1_8_8 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_DUAL)
#define USES_1S_8S_8S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_OCTAL_1_8_8 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_1S_1S_8S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_OCTAL_1_1_8 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_4S_4D_4D(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_QUAD && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_DUAL)
#define USES_4S_4S_4S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_QUAD && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_1S_4D_4D(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_QUAD_1_4_4 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_DUAL)
#define USES_1S_4S_4S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_QUAD_1_4_4 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_1S_1S_4S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_QUAD_1_1_4 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_2S_2S_2S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_DUAL && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_1S_2D_2D(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_DUAL_1_2_2 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_DUAL)
#define USES_1S_2S_2S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_DUAL_1_2_2 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_1S_1S_2S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_DUAL_1_1_2 && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)
#define USES_1S_1D_1D(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_SINGLE && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_DUAL)
#define USES_1S_1S_1S(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_SINGLE && \
	 DT_INST_ENUM_IDX(inst, mspi_data_rate) == MSPI_DATA_RATE_SINGLE)

#define USES_OCTAL_IO(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_OCTAL)

#define BFP_DW1_ADDRESS_BYTES(inst) \
	SFDP_FIELD(inst, sfdp_bfp, 1, GENMASK(18, 17))

#define USES_4BYTE_ADDR(inst) \
	(USES_OCTAL_IO(inst) || \
	 DT_INST_PROP(inst, use_4byte_addressing) || \
	 BFP_DW1_ADDRESS_BYTES(inst) == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B)

#define BFP_ENTER_4BYTE_ADDR_METHODS(inst) \
	SFDP_FIELD(inst, sfdp_bfp, 16, GENMASK(31, 24))

#define HAS_4BYTE_ADDR_CMDS(inst) \
	(BFP_ENTER_4BYTE_ADDR_METHODS(inst) & BFP_DW16_4B_ADDR_PER_CMD)

#define BFP_DW18_CMD_EXT(inst) \
	SFDP_FIELD(inst, sfdp_bfp, 18, GENMASK(30, 29))

#define CMD_EXTENSION(inst) \
	(!USES_8D_8D_8D(inst) ?      CMD_EXTENSION_NONE : \
	 (BFP_DW18_CMD_EXT(inst) \
	  == BFP_DW18_CMD_EXT_INV) ? CMD_EXTENSION_INVERSE \
				   : CMD_EXTENSION_SAME)

/* 1st DWORD of 4-byte Address Instruction Table (ID FF84) indicates commands
 * that are supported by the chip.
 */
#define FF84_DW1_BIT(inst, bit) (SFDP_DW(inst, sfdp_ff84, 1) & BIT(bit))

#define SFDP_CMD_PP(inst) \
	USES_1S_4S_4S(inst) ? SPI_NOR_CMD_PP_1_4_4 : \
	USES_1S_1S_4S(inst) ? SPI_NOR_CMD_PP_1_1_4 : \
	SPI_NOR_CMD_PP
#define SFDP_CMD_PP_4B(inst) \
	USES_1S_8S_8S(inst) && FF84_DW1_BIT(inst, 24) ? 0x8E : \
	USES_1S_1S_8S(inst) && FF84_DW1_BIT(inst, 23) ? 0x84 : \
	USES_1S_4S_4S(inst) && FF84_DW1_BIT(inst, 8) ? SPI_NOR_CMD_PP_1_4_4_4B : \
	USES_1S_1S_4S(inst) && FF84_DW1_BIT(inst, 7) ? SPI_NOR_CMD_PP_1_1_4_4B : \
	FF84_DW1_BIT(inst, 6) ? SPI_NOR_CMD_PP_4B \
	: 0
#define SFDP_CMD_FAST_READ(inst) \
	USES_1S_8D_8D(inst) ? 0 : \
	USES_1S_8S_8S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 17, GENMASK(15, 8)) : \
	USES_1S_1S_8S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 17, GENMASK(31, 24)) : \
	USES_4S_4D_4D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 23, GENMASK(31, 24)) : \
	USES_4S_4S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 7, GENMASK(31, 24)) : \
	USES_1S_4D_4D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 23, GENMASK(15, 8)) : \
	USES_1S_4S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 3, GENMASK(15, 8)) : \
	USES_1S_1S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 3, GENMASK(31, 24)) : \
	USES_2S_2S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 6, GENMASK(31, 24)) : \
	USES_1S_2D_2D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 22, GENMASK(31, 24)) : \
	USES_1S_2S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 4, GENMASK(31, 24)) : \
	USES_1S_1S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 4, GENMASK(15, 8)) : \
	USES_1S_1D_1D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 22, GENMASK(15, 8)) : \
	SPI_NOR_CMD_READ_FAST
#define SFDP_CMD_FAST_READ_4B(inst) \
	USES_8D_8D_8D(inst) ? 0xEE : \
	USES_8S_8S_8S(inst) ? 0xEC : \
	USES_1S_8D_8D(inst) && FF84_DW1_BIT(inst, 22) ? 0xFD : \
	USES_1S_8S_8S(inst) && FF84_DW1_BIT(inst, 21) ? 0xCC : \
	USES_1S_1S_8S(inst) && FF84_DW1_BIT(inst, 20) ? 0x7C : \
	USES_4S_4D_4D(inst) ? 0 : \
	USES_4S_4S_4S(inst) ? 0 : \
	USES_1S_4D_4D(inst) && FF84_DW1_BIT(inst, 15) ? 0xEE : \
	USES_1S_4S_4S(inst) && FF84_DW1_BIT(inst, 5) ? 0xEC : \
	USES_1S_1S_4S(inst) && FF84_DW1_BIT(inst, 4) ? 0x6C : \
	USES_2S_2S_2S(inst) ? 0 : \
	USES_1S_2D_2D(inst) && FF84_DW1_BIT(inst, 14) ? 0xBE : \
	USES_1S_2S_2S(inst) && FF84_DW1_BIT(inst, 3) ? 0xBC : \
	USES_1S_1S_2S(inst) && FF84_DW1_BIT(inst, 2) ? 0x3C : \
	USES_1S_1D_1D(inst) && FF84_DW1_BIT(inst, 13) ? 0x0E : \
	FF84_DW1_BIT(inst, 1) ? SPI_NOR_CMD_READ_FAST_4B : \
	0

#define DEFAULT_CMD_INFO(inst) { \
	.pp_cmd = USES_4BYTE_ADDR(inst) && HAS_4BYTE_ADDR_CMDS(inst) \
		? SFDP_CMD_PP_4B(inst) \
		: SFDP_CMD_PP(inst), \
	.read_cmd = USES_4BYTE_ADDR(inst) && HAS_4BYTE_ADDR_CMDS(inst) \
		  ? SFDP_CMD_FAST_READ_4B(inst) \
		  : SFDP_CMD_FAST_READ(inst), \
	.read_mode_bit_cycles = \
		USES_1S_8S_8S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 17, GENMASK(7, 5)) : \
		USES_1S_1S_8S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 17, GENMASK(23, 21)) : \
		USES_4S_4D_4D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 23, GENMASK(23, 21)) : \
		USES_4S_4S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 7, GENMASK(23, 21)) : \
		USES_1S_4D_4D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 23, GENMASK(7, 5)) : \
		USES_1S_4S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 3, GENMASK(7, 5)) : \
		USES_1S_1S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 3, GENMASK(23, 21)) : \
		USES_2S_2S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 6, GENMASK(23, 21)) : \
		USES_1S_2D_2D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 22, GENMASK(23, 21)) : \
		USES_1S_2S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 4, GENMASK(23, 21)) : \
		USES_1S_1S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 4, GENMASK(7, 5)) : \
		USES_1S_1D_1D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 22, GENMASK(7, 5)) : \
		USES_1S_1S_1S(inst) ? 0 : \
		0, \
	.read_dummy_cycles = DT_INST_PROP_OR(inst, rx_dummy, \
		USES_8D_8D_8D(inst) ? SFDP_FIELD(inst, sfdp_ff05, 6, GENMASK(4, 0)) : \
		USES_8S_8S_8S(inst) ? SFDP_FIELD(inst, sfdp_ff05, 6, GENMASK(9, 5)) : \
		USES_1S_8S_8S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 17, GENMASK(4, 0)) : \
		USES_1S_1S_8S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 17, GENMASK(20, 16)) : \
		USES_4S_4D_4D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 23, GENMASK(20, 16)) : \
		USES_4S_4S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 7, GENMASK(20, 16)) : \
		USES_1S_4D_4D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 23, GENMASK(4, 0)) : \
		USES_1S_4S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 3, GENMASK(4, 0)) : \
		USES_1S_1S_4S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 3, GENMASK(20, 16)) : \
		USES_2S_2S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 6, GENMASK(20, 16)) : \
		USES_1S_2D_2D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 22, GENMASK(20, 16)) : \
		USES_1S_2S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 4, GENMASK(20, 16)) : \
		USES_1S_1S_2S(inst) ? SFDP_FIELD(inst, sfdp_bfp, 4, GENMASK(4, 0)) : \
		USES_1S_1D_1D(inst) ? SFDP_FIELD(inst, sfdp_bfp, 22, GENMASK(4, 0)) : \
		USES_1S_1S_1S(inst) ? 8 : \
		0), \
	.uses_4byte_addr = USES_4BYTE_ADDR(inst), \
	.cmd_extension = CMD_EXTENSION(inst), \
	.sfdp_addr_4 = USES_OCTAL_IO(inst) \
		     ? (SFDP_FIELD(inst, sfdp_ff05, 1, BIT(31)) == 0) \
		     : false, \
	.sfdp_dummy_20 = USES_OCTAL_IO(inst) \
		       ? (SFDP_FIELD(inst, sfdp_ff05, 1, BIT(30)) == 1) \
		       : false, \
	.rdsr_addr_4 = USES_OCTAL_IO(inst) \
		     ? (SFDP_FIELD(inst, sfdp_ff05, 1, BIT(29)) == 1) \
		     : false, \
	.rdsr_dummy = USES_OCTAL_IO(inst) \
		    ? (SFDP_FIELD(inst, sfdp_ff05, 1, BIT(28)) ? 8 : 4) \
		    : 0, \
	.rdid_addr_4 = USES_OCTAL_IO(inst) \
		     ? (SFDP_FIELD(inst, sfdp_ff05, 1, BIT(29)) == 1) \
		     : false, \
	.rdid_dummy = USES_OCTAL_IO(inst) \
		    ? (SFDP_FIELD(inst, sfdp_ff05, 1, BIT(28)) ? 8 : 4) \
		    : 0, }

/* Erase Types, 8th and 9th DWORD of BSP */
#define BFP_DW8_CMD_ET_1(inst) SFDP_FIELD(inst, sfdp_bfp, 8, GENMASK(15, 8))
#define BFP_DW8_EXP_ET_1(inst) SFDP_FIELD(inst, sfdp_bfp, 8, GENMASK(7, 0))
#define BFP_DW8_CMD_ET_2(inst) SFDP_FIELD(inst, sfdp_bfp, 8, GENMASK(31, 24))
#define BFP_DW8_EXP_ET_2(inst) SFDP_FIELD(inst, sfdp_bfp, 8, GENMASK(23, 16))
#define BFP_DW9_CMD_ET_3(inst) SFDP_FIELD(inst, sfdp_bfp, 9, GENMASK(15, 8))
#define BFP_DW9_EXP_ET_3(inst) SFDP_FIELD(inst, sfdp_bfp, 9, GENMASK(7, 0))
#define BFP_DW9_CMD_ET_4(inst) SFDP_FIELD(inst, sfdp_bfp, 9, GENMASK(31, 24))
#define BFP_DW9_EXP_ET_4(inst) SFDP_FIELD(inst, sfdp_bfp, 9, GENMASK(23, 16))

/* 4-byte Address instructions for Erase Types defined in 8th and 9th DWORD
 * of BFP; 2nd DWORD of 4-byte Address Instruction Table (ID FF84)
 */
#define FF84_DW2_CMD_ET_1(inst) SFDP_FIELD(inst, sfdp_ff84, 2, GENMASK(7, 0))
#define FF84_DW2_CMD_ET_2(inst) SFDP_FIELD(inst, sfdp_ff84, 2, GENMASK(15, 8))
#define FF84_DW2_CMD_ET_3(inst) SFDP_FIELD(inst, sfdp_ff84, 2, GENMASK(23, 16))
#define FF84_DW2_CMD_ET_4(inst) SFDP_FIELD(inst, sfdp_ff84, 2, GENMASK(31, 24))
/* Support for Erase Types in 4-byte Address mode; table FF84, 1st DWORD */
#define FF84_DW1_SUP_ET_1(inst) SFDP_FIELD(inst, sfdp_ff84, 1, BIT(9))
#define FF84_DW1_SUP_ET_2(inst) SFDP_FIELD(inst, sfdp_ff84, 1, BIT(10))
#define FF84_DW1_SUP_ET_3(inst) SFDP_FIELD(inst, sfdp_ff84, 1, BIT(11))
#define FF84_DW1_SUP_ET_4(inst) SFDP_FIELD(inst, sfdp_ff84, 1, BIT(12))

#define DEFAULT_ERASE_TYPES_DEFINE(inst) \
	static const struct jesd216_erase_type \
	dev##inst##_erase_types[JESD216_NUM_ERASE_TYPES] = \
		COND_CODE_1(SFDP_DW_EXISTS(inst, sfdp_bfp, 8), \
			({{ .cmd = BFP_DW8_CMD_ET_1(inst), \
			    .exp = BFP_DW8_EXP_ET_1(inst), }, \
			  { .cmd = BFP_DW8_CMD_ET_2(inst), \
			    .exp = BFP_DW8_EXP_ET_2(inst), }, \
			  { .cmd = BFP_DW9_CMD_ET_3(inst), \
			    .exp = BFP_DW9_EXP_ET_3(inst), }, \
			  { .cmd = BFP_DW9_CMD_ET_4(inst), \
			    .exp = BFP_DW9_EXP_ET_4(inst), }}), \
			({{ .cmd = SPI_NOR_CMD_SE, \
			    .exp = 0x0C }})); \
	static const struct jesd216_erase_type \
	dev##inst##_erase_types_4b[JESD216_NUM_ERASE_TYPES] = \
		COND_CODE_1(UTIL_AND(SFDP_DW_EXISTS(inst, sfdp_ff84, 2), \
				     SFDP_DW_EXISTS(inst, sfdp_bfp, 9)), \
			({{ .cmd = FF84_DW2_CMD_ET_1(inst), \
			    .exp = FF84_DW1_SUP_ET_1(inst) \
				 ? BFP_DW8_EXP_ET_1(inst) \
				 : 0, }, \
			  { .cmd = FF84_DW2_CMD_ET_2(inst), \
			    .exp = FF84_DW1_SUP_ET_2(inst) \
				 ? BFP_DW8_EXP_ET_2(inst) \
				 : 0, }, \
			  { .cmd = FF84_DW2_CMD_ET_3(inst), \
			    .exp = FF84_DW1_SUP_ET_3(inst) \
				 ? BFP_DW9_EXP_ET_3(inst) \
				 : 0, }, \
			  { .cmd = FF84_DW2_CMD_ET_4(inst), \
			    .exp = FF84_DW1_SUP_ET_4(inst) \
				 ? BFP_DW9_EXP_ET_4(inst) \
				 : 0, }}), \
			({{ .cmd = SPI_NOR_CMD_SE_4B, \
			    .exp = 0x0C }}))

#define DEFAULT_ERASE_TYPES(inst) \
	USES_4BYTE_ADDR(inst) && HAS_4BYTE_ADDR_CMDS(inst) \
	? dev##inst##_erase_types_4b \
	: dev##inst##_erase_types

#define BFP_DW15_QER(inst) \
	SFDP_FIELD(inst, sfdp_bfp, 15, GENMASK(22, 20))

#define BFP_DW19_OER(inst) \
	SFDP_FIELD(inst, sfdp_bfp, 19, GENMASK(22, 20))

#define ENTER_4BYTE_ADDR(inst) \
	(!USES_4BYTE_ADDR(inst) ?           ENTER_4BYTE_ADDR_NONE : \
	 (BFP_ENTER_4BYTE_ADDR_METHODS(inst) \
	  & (BFP_DW16_4B_ADDR_PER_CMD | \
	     BFP_DW16_4B_ADDR_ALWAYS)) ?    ENTER_4BYTE_ADDR_NONE : \
	 (BFP_ENTER_4BYTE_ADDR_METHODS(inst) \
	  & BFP_DW16_4B_ADDR_ENTER_B7) ?    ENTER_4BYTE_ADDR_B7 : \
	 (BFP_ENTER_4BYTE_ADDR_METHODS(inst) \
	  & BFP_DW16_4B_ADDR_ENTER_06_B7) ? ENTER_4BYTE_ADDR_06_B7 : \
					    ENTER_4BYTE_ADDR_NONE)

#define DEFAULT_SWITCH_INFO(inst) { \
	.quad_enable_req = BFP_DW15_QER(inst), \
	.octal_enable_req = BFP_DW19_OER(inst), \
	.enter_4byte_addr = ENTER_4BYTE_ADDR(inst) }

#define BFP_FLASH_SIZE(dw2) \
	((dw2 & BIT(31)) \
	 ? BIT(MIN(31, (dw2 & BIT_MASK(31)) - 3)) \
	 : dw2 / 8)

#define FLASH_SIZE(inst) \
	(DT_INST_NODE_HAS_PROP(inst, size) \
	 ? DT_INST_PROP(inst, size) / 8 \
	 : BFP_FLASH_SIZE(SFDP_DW(inst, sfdp_bfp, 2)))

#define BFP_FLASH_PAGE_EXP(inst) SFDP_FIELD(inst, sfdp_bfp, 11, GENMASK(7, 4))

#define FLASH_PAGE_SIZE(inst) \
	(BFP_FLASH_PAGE_EXP(inst) \
	 ? BIT(BFP_FLASH_PAGE_EXP(inst)) \
	 : SPI_NOR_PAGE_SIZE)

#define SFDP_BUILD_ASSERTS(inst) \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, sfdp_bfp), \
		"sfdp-bfp property needed in " \
			DT_NODE_FULL_NAME(DT_DRV_INST(inst))); \
	BUILD_ASSERT((DT_INST_ENUM_IDX(inst, mspi_io_mode) \
		      != MSPI_IO_MODE_OCTAL) || \
		     DT_INST_NODE_HAS_PROP(inst, sfdp_ff05), \
		"sfdp-ff05 property needed in " \
			DT_NODE_FULL_NAME(DT_DRV_INST(inst))); \
	BUILD_ASSERT(!USES_4BYTE_ADDR(inst) || \
		     DT_INST_NODE_HAS_PROP(inst, sfdp_ff84), \
		"sfdp-ff84 property needed in " \
			DT_NODE_FULL_NAME(DT_DRV_INST(inst))); \
	BUILD_ASSERT(!USES_8D_8D_8D(inst) || \
		     BFP_DW18_CMD_EXT(inst) <= BFP_DW18_CMD_EXT_INV, \
		"Unsupported Octal Command Extension mode in " \
			DT_NODE_FULL_NAME(DT_DRV_INST(inst))); \
	BUILD_ASSERT(!DT_INST_PROP(inst, use_4byte_addressing) || \
		     (BFP_DW1_ADDRESS_BYTES(inst) \
		      != JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B), \
		"Cannot use 4-byte addressing for " \
			DT_NODE_FULL_NAME(DT_DRV_INST(inst))); \
	BUILD_ASSERT(!DT_INST_PROP(inst, use_4byte_addressing) || \
		     (BFP_ENTER_4BYTE_ADDR_METHODS(inst) \
		      & (BFP_DW16_4B_ADDR_ENTER_B7 | \
			 BFP_DW16_4B_ADDR_ENTER_06_B7 | \
			 BFP_DW16_4B_ADDR_PER_CMD | \
			 BFP_DW16_4B_ADDR_ALWAYS)), \
		"No supported method of entering 4-byte addressing mode for " \
			DT_NODE_FULL_NAME(DT_DRV_INST(inst))); \
	BUILD_ASSERT(!DT_INST_PROP(inst, initial_soft_reset) || \
		     (SFDP_FIELD(inst, sfdp_bfp, 16, GENMASK(13, 8)) \
		      & BFP_DW16_SOFT_RESET_66_99), \
		"Cannot use 66h/99h soft reset sequence for " \
			DT_NODE_FULL_NAME(DT_DRV_INST(inst)))

#else

#define USES_4BYTE_ADDR(inst) \
	(DT_INST_ENUM_IDX(inst, mspi_io_mode) == MSPI_IO_MODE_OCTAL || \
	 DT_INST_PROP(inst, use_4byte_addressing))

#define DEFAULT_CMD_INFO(inst) { \
	.pp_cmd = USES_4BYTE_ADDR(inst) \
		? SPI_NOR_CMD_PP_4B \
		: SPI_NOR_CMD_PP, \
	.read_cmd = USES_4BYTE_ADDR(inst) \
		  ? SPI_NOR_CMD_READ_FAST_4B \
		  : SPI_NOR_CMD_READ_FAST, \
	.read_mode_bit_cycles = 0, \
	.read_dummy_cycles = 8, \
	.uses_4byte_addr = USES_4BYTE_ADDR(inst), \
	.cmd_extension = CMD_EXTENSION_NONE, \
	.sfdp_addr_4 = false, \
	.sfdp_dummy_20 = false, \
	.rdsr_addr_4 = false, \
	.rdsr_dummy = 0, \
	.rdid_addr_4 = false, \
	.rdid_dummy = 0, }

#define DEFAULT_ERASE_TYPES_DEFINE(inst) \
	static const struct jesd216_erase_type \
	dev##inst##_erase_types[JESD216_NUM_ERASE_TYPES] = \
		{{ .cmd = SPI_NOR_CMD_SE, \
		   .exp = 0x0C }}; \
	static const struct jesd216_erase_type \
	dev##inst##_erase_types_4b[JESD216_NUM_ERASE_TYPES] = \
		{{ .cmd = SPI_NOR_CMD_SE_4B, \
		   .exp = 0x0C }}

#define DEFAULT_ERASE_TYPES(inst) \
	USES_4BYTE_ADDR(inst) ? dev##inst##_erase_types_4b \
			      : dev##inst##_erase_types

#define DEFAULT_SWITCH_INFO(inst) { \
	.quad_enable_req = DT_INST_ENUM_IDX_OR(inst, quad_enable_requirements, \
					       JESD216_DW15_QER_VAL_NONE), \
	.octal_enable_req = OCTAL_ENABLE_REQ_NONE, \
	.enter_4byte_addr = ENTER_4BYTE_ADDR_NONE }

#define FLASH_SIZE(inst) (DT_INST_PROP(inst, size) / 8)

#define FLASH_PAGE_SIZE(inst) SPI_NOR_PAGE_SIZE

#define SFDP_BUILD_ASSERTS(inst)

#endif /* CONFIG_FLASH_MSPI_NOR_USE_SFDP */
