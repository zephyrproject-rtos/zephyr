/*
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_JESD216_H_
#define ZEPHYR_DRIVERS_FLASH_JESD216_H_

#include <errno.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <zephyr/types.h>

/* Following are structures and constants supporting the JEDEC Serial
 * Flash Discoverable Parameters standard, JESD216 and its successors,
 * available at
 * https://www.jedec.org/standards-documents/docs/jesd216b
 */

#define JESD216_CMD_READ_SFDP   0x5A
#define JESD216_CMD_BURST_SFDP  0x5B

/* Layout of a JESD216 parameter header. */
struct jesd216_param_header {
	uint8_t id_lsb;		/* ID LSB */
	uint8_t rev_minor;	/* Minor revision number */
	uint8_t rev_major;	/* Major revision number */
	uint8_t len_dw;		/* Length of table in 32-bit DWORDs */
	uint8_t ptp[3];		/* Address of table in SFDP space (LSB@0) */
	uint8_t id_msb;		/* ID MSB */
} __packed;

/* Get the number of bytes required for the parameter table. */
static inline uint32_t jesd216_param_len(const struct jesd216_param_header *hp)
{
	return sizeof(uint32_t) * hp->len_dw;
}

/* Get the ID that identifies the content of the parameter table. */
static inline uint16_t jesd216_param_id(const struct jesd216_param_header *hp)
{
	return ((uint16_t)hp->id_msb << 8) | hp->id_lsb;
}

/* Get the address within the SFDP where the data for the table is
 * stored.
 */
static inline uint32_t jesd216_param_addr(const struct jesd216_param_header *hp)
{
	return ((hp->ptp[2] << 16)
		| (hp->ptp[1] << 8)
		| (hp->ptp[0] << 0));
}

/* Layout of the Serial Flash Discoverable Parameters header. */
struct jesd216_sfdp_header {
	uint32_t magic;		/* "SFDP" in little endian */
	uint8_t rev_minor;	/* Minor revision number */
	uint8_t rev_major;	/* Major revision number */
	uint8_t nph;		/* Number of parameter headers */
	uint8_t access;		/* Access protocol */
	struct jesd216_param_header phdr[]; /* Headers */
} __packed;

/* SFDP access protocol for backwards compatibility with JESD216B. */
#define JESD216_SFDP_AP_LEGACY 0xFF

/* The expected value from the jesd216_sfdp::magic field in host byte
 * order.
 */
#define JESD216_SFDP_MAGIC 0x50444653

/* All JESD216 data is read from the device in little-endian byte
 * order.  For JEDEC parameter tables defined through JESD216D-01 the
 * parameters are defined by 32-bit words that may need to be
 * byte-swapped to extract their information.
 *
 * A 16-bit ID from the parameter header is used to identify the
 * content of each table.  The first parameter table in the SFDP
 * hierarchy must be a Basic Flash Parameter table (ID 0xFF00).
 */

/* JESD216D-01 section 6.4: Basic Flash Parameter */
#define JESD216_SFDP_PARAM_ID_BFP 0xFF00
/* JESD216D-01 section 6.5: Sector Map Parameter */
#define JESD216_SFDP_PARAM_ID_SECTOR_MAP 0xFF81
/* JESD216D-01 section 6.6: 4-Byte Address Instruction Parameter */
#define JESD216_SFDP_PARAM_ID_4B_ADDR_INSTR 0xFF84
/* JESD216D-01 section 6.7: xSPI (Profile 1.0) Parameter */
#define JESD216_SFDP_PARAM_ID_XSPI_PROFILE_1V0 0xFF05
/* JESD216D-01 section 6.8: xSPI (Profile 2.0) Parameter */
#define JESD216_SFDP_PARAM_ID_XSPI_PROFILE_2V0 0xFF06

/* Macro to define the number of bytes required for the SFDP pheader
 * and @p nph parameter headers.
 *
 * @param nph the number of parameter headers to be read.  1 is
 * sufficient for basic functionality.
 *
 * @return required buffer size in bytes.
 */
#define JESD216_SFDP_SIZE(nph) (sizeof(struct jesd216_sfdp_header)	\
				+ ((nph) * sizeof(struct jesd216_param_header)))

/** Extract the magic number from the SFDP structure in host byte order.
 *
 * If this compares equal to JESD216_SFDP_MAGIC then the SFDP header
 * may have been read correctly.
 */
static inline uint32_t jesd216_sfdp_magic(const struct jesd216_sfdp_header *hp)
{
	return sys_le32_to_cpu(hp->magic);
}

/* Layout of the Basic Flash Parameters table.
 *
 * SFDP through JESD216B supported 9 DWORD values.  JESD216C extended
 * this to 17, and JESD216D to 20.
 *
 * All values are expected to be stored as little-endian and must be
 * converted to host byte order to extract the bit fields defined in
 * the standard.  Rather than pre-define layouts to access to all
 * potential fields this header provides functions for specific fields
 * known to be important, such as density and erase command support.
 */
struct jesd216_bfp {
	uint32_t dw1;
	uint32_t dw2;
	uint32_t dw3;
	uint32_t dw4;
	uint32_t dw5;
	uint32_t dw6;
	uint32_t dw7;
	uint32_t dw8;
	uint32_t dw9;
	uint32_t dw10[];
} __packed;

/* Provide a few word-specific flags and bitfield ranges for values
 * that an application or driver might expect to want to extract.
 *
 * See the JESD216 specification for the interpretation of these
 * bitfields.
 */
#define JESD216_SFDP_BFP_DW1_DTRCLK_FLG BIT(19)
#define JESD216_SFDP_BFP_DW1_ADDRBYTES_MASK (BIT(17) | BIT(18))
#define JESD216_SFDP_BFP_DW1_ADDRBYTES_SHFT 17
#define JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B 0
#define JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B 1
#define JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B 2
#define JESD216_SFDP_BFP_DW1_4KERASEINSTR_SHFT 8
#define JESD216_SFDP_BFP_DW1_4KERASEINSTR_MASK (0xFF << JESD216_SFDP_BFP_DW1_4KERASEINSTR_SHFT)
#define JESD216_SFDP_BFP_DW1_WEISWVSR_FLG BIT(4)
#define JESD216_SFDP_BFP_DW1_VSRBP_FLG BIT(3)
#define JESD216_SFDP_BFP_DW1_WRTGRAN_FLG BIT(2)
#define JESD216_SFDP_BFP_DW1_BSERSZ_SHFT 0
#define JESD216_SFDP_BFP_DW1_BSERSZ_MASK (0x03 << JESD216_SFDP_BFP_DW1_BSERSZ_SHFT)
#define JESD216_SFDP_BFP_DW1_BSERSZ_VAL_4KSUP 0x01
#define JESD216_SFDP_BFP_DW1_BSERSZ_VAL_4KNOTSUP 0x03

#define JESD216_SFDP_BFP_DW12_SUSPRESSUP_FLG BIT(31)

/* Data can be extracted from the BFP words using these APIs:
 *
 * * DW1 (capabilities) use DW1 bitfield macros above or
 *   jesd216_read_support().
 * * DW2 (density) use jesd216_bfp_density().
 * * DW3-DW7 (instr) use jesd216_bfp_read_support().
 * * DW8-DW9 (erase types) use jesd216_bfp_erase().
 *
 * JESD216A (16 DW)
 *
 * * DW10 (erase times) use jesd216_bfp_erase_type_times().
 * * DW11 (other times) use jesd216_bfp_decode_dw11().
 * * DW12-13 (suspend/resume) no API except
 *   JESD216_SFDP_BFP_DW12_SUSPRESSUP_FLG.
 * * DW14 (deep power down) use jesd216_bfp_decode_dw14().
 * * DW15-16 no API except jesd216_bfp_read_support().
 *
 * JESD216C (20 DW)
 * * DW17-20 (quad/oct support) no API except jesd216_bfp_read_support().
 */

/* Extract the density of the chip in bits from BFP DW2. */
static inline uint64_t jesd216_bfp_density(const struct jesd216_bfp *hp)
{
	uint32_t dw = sys_le32_to_cpu(hp->dw2);

	if (dw & BIT(31)) {
		return BIT64(dw & BIT_MASK(31));
	}
	return 1U + (uint64_t)dw;
}

/* Protocol mode enumeration types.
 *
 * Modes are identified by fields representing the number of I/O
 * signals and the data rate in the transfer.  The I/O width may be 1,
 * 2, 4, or 8 I/O signals.  The data rate may be single or double.
 * SDR is assumed; DDR is indicated by a D following the I/O width.
 *
 * A transfer has three phases, and width/rate is specified for each
 * in turn:
 * * Transfer of the command
 * * Transfer of the command modifier (e.g. address)
 * * Transfer of the data.
 *
 * Modes explicitly mentioned in JESD216 or JESD251 are given
 * enumeration values below, which can be used to extract information
 * about instruction support.
 */
enum jesd216_mode_type {
	JESD216_MODE_044,	/* implied instruction, execute in place */
	JESD216_MODE_088,
	JESD216_MODE_111,
	JESD216_MODE_112,
	JESD216_MODE_114,
	JESD216_MODE_118,
	JESD216_MODE_122,
	JESD216_MODE_144,
	JESD216_MODE_188,
	JESD216_MODE_222,
	JESD216_MODE_444,
	JESD216_MODE_44D4D,
	JESD216_MODE_888,
	JESD216_MODE_8D8D8D,
	JESD216_MODE_LIMIT,
};

/* Command to use for fast read operations in a specified protocol
 * mode.
 */
struct jesd216_instr {
	uint8_t instr;
	uint8_t mode_clocks;
	uint8_t wait_states;
};

/* Determine whether a particular operational mode is supported for
 * read, and possibly what command may be used.
 *
 * @note For @p mode JESD216_MODE_111 this function will return zero
 * to indicate that standard read (instruction 03h) is supported, but
 * without providing information on how.  SFDP does not provide an
 * indication of support for 1-1-1 Fast Read (0Bh).
 *
 * @param php pointer to the BFP header.
 *
 * @param bfp pointer to the BFP table.
 *
 * @param mode the desired protocol mode.
 *
 * @param res where to store instruction information.  Pass a null
 * pointer to test for support without retrieving instruction
 * information.
 *
 * @retval positive if instruction is supported and *res has been set.
 *
 * @retval 0 if instruction is supported but *res has not been set
 * (e.g. no instruction needed, or instruction cannot be read from
 * BFP).
 *
 * @retval -ENOTSUP if instruction is not supported.
 */
int jesd216_bfp_read_support(const struct jesd216_param_header *php,
			     const struct jesd216_bfp *bfp,
			     enum jesd216_mode_type mode,
			     struct jesd216_instr *res);

/* Description of a supported erase operation. */
struct jesd216_erase_type {
	/* The command opcode used for an erase operation. */
	uint8_t cmd;

	/* The value N when the erase operation erases a 2^N byte
	 * region.
	 */
	uint8_t exp;
};

/* The number of erase types defined in a JESD216 Basic Flash
 * Parameter table.
 */
#define JESD216_NUM_ERASE_TYPES 4

/* Extract a supported erase size and command from BFP DW8 or DW9.
 *
 * @param bfp pointer to the parameter table.
 *
 * @param idx the erase type index, from 1 through 4.  Only index 1 is
 * guaranteed to be present.
 *
 * @param etp where to store the command and size used for the erase.
 *
 * @retval 0 if the erase type index provided usable information.
 * @retval -EINVAL if the erase type index is undefined.
 */
int jesd216_bfp_erase(const struct jesd216_bfp *bfp,
		      uint8_t idx,
		      struct jesd216_erase_type *etp);

/* Extract typical and maximum erase times from DW10.
 *
 * @param php pointer to the BFP header.
 *
 * @param bfp pointer to the BFP table.
 *
 * @param idx the erase type index, from 1 through 4.  For meaningful
 * results the index should be one for which jesd216_bfp_erase()
 * returns success.
 *
 * @param typ_ms where to store the typical erase time (in
 * milliseconds) for the specified erase type.
 *
 * @retval -ENOTSUP if the erase type index is undefined.
 * @retval positive is a multiplier that converts typical erase times
 * to maximum erase times.
 */
int jesd216_bfp_erase_type_times(const struct jesd216_param_header *php,
				 const struct jesd216_bfp *bfp,
				 uint8_t idx,
				 uint32_t *typ_ms);

/* Get the page size from the Basic Flash Parameters.
 *
 * @param php pointer to the BFP header.
 *
 * @param bfp pointer to the BFP table.
 *
 * @return the page size in bytes from the parameters if supported,
 * otherwise 256.
 */
static inline uint32_t jesd216_bfp_page_size(const struct jesd216_param_header *php,
					     const struct jesd216_bfp *bfp)
{
	/* Page size introduced in JESD216A */
	if (php->len_dw < 11) {
		return 256;
	}

	uint32_t dw11 = sys_le32_to_cpu(bfp->dw10[1]);
	uint8_t exp = (dw11 >> 4) & 0x0F;

	return BIT(exp);
}

/* Decoded data from JESD216 DW11. */
struct jesd216_bfp_dw11 {
	/* Typical time for chip (die) erase, in milliseconds */
	uint16_t chip_erase_ms;

	/* Typical time for first byte program, in microseconds */
	uint16_t byte_prog_first_us;

	/* Typical time per byte for byte program after first, in
	 * microseconds
	 */
	uint16_t byte_prog_addl_us;

	/* Typical time for page program, in microseconds */
	uint16_t page_prog_us;

	/* Multiplier to get maximum time from typical times. */
	uint16_t typ_max_factor;

	/* Number of bytes in a page. */
	uint16_t page_size;
};

/* Get data from BFP DW11.
 *
 * @param php pointer to the BFP header.
 *
 * @param bfp pointer to the BFP table.
 *
 * @param res pointer to where to store the decoded data.
 *
 * @retval -ENOTSUP if this information is not available from this BFP table.
 * @retval 0 on successful storage into @c *res.
 */
int jesd216_bfp_decode_dw11(const struct jesd216_param_header *php,
			    const struct jesd216_bfp *bfp,
			    struct jesd216_bfp_dw11 *res);

/* Decoded data from JESD216 DW14 */
struct jesd216_bfp_dw14 {
	/* Instruction used to enter deep power-down */
	uint8_t enter_dpd_instr;

	/* Instruction used to exit deep power-down */
	uint8_t exit_dpd_instr;

	/* Bits defining ways busy status may be polled. */
	uint8_t poll_options;

	/* Time after issuing exit instruction until device is ready
	 * to accept a command, in nanoseconds.
	 */
	uint32_t exit_delay_ns;
};

/* Get data from BFP DW14.
 *
 * @param php pointer to the BFP header.
 *
 * @param bfp pointer to the BFP table.
 *
 * @param res pointer to where to store the decoded data.
 *
 * @retval -ENOTSUP if this information is not available from this BFP table.
 * @retval 0 on successful storage into @c *res.
 */
int jesd216_bfp_decode_dw14(const struct jesd216_param_header *php,
			    const struct jesd216_bfp *bfp,
			    struct jesd216_bfp_dw14 *res);

/* DW15 Quad Enable Requirements specifies status register QE bits.
 *
 * Two common configurations are summarized; see the specification for
 * full details of how to use these values.
 */
enum jesd216_dw15_qer_type {
	/* No QE status required for 1-1-4 or 1-4-4 mode */
	JESD216_DW15_QER_NONE = 0,
	JESD216_DW15_QER_S2B1v1 = 1,
	/* Bit 6 of SR byte must be set to enable 1-1-4 or 1-4-4 mode.
	 * SR is one byte.
	 */
	JESD216_DW15_QER_S1B6 = 2,
	JESD216_DW15_QER_S2B7 = 3,
	JESD216_DW15_QER_S2B1v4 = 4,
	JESD216_DW15_QER_S2B1v5 = 5,
	JESD216_DW15_QER_S2B1v6 = 6,
};

/* Decoded data from JESD216 DW15 */
struct jesd216_bfp_dw15 {
	/* If true clear NVECR bit 4 to disable HOLD/RESET */
	bool hold_reset_disable: 1;
	/* Encoded jesd216_qer_type */
	unsigned int qer: 3;
	/* 0-4-4 mode entry method */
	unsigned int entry_044: 4;
	/* 0-4-4 mode exit method */
	unsigned int exit_044: 6;
	/* True if 0-4-4 mode is supported */
	bool support_044: 1;
	/* 4-4-4 mode enable sequences */
	unsigned int enable_444: 5;
	/* 4-4-4 mode disable sequences */
	unsigned int disable_444: 4;
};

/* Get data from BFP DW15.
 *
 * @param php pointer to the BFP header.
 *
 * @param bfp pointer to the BFP table.
 *
 * @param res pointer to where to store the decoded data.
 *
 * @retval -ENOTSUP if this information is not available from this BFP table.
 * @retval 0 on successful storage into @c *res.
 */
int jesd216_bfp_decode_dw15(const struct jesd216_param_header *php,
			    const struct jesd216_bfp *bfp,
			    struct jesd216_bfp_dw15 *res);

/* Decoded data from JESD216_DW16 */
struct jesd216_bfp_dw16 {
	/* Bits specifying supported modes of entering 4-byte
	 * addressing.
	 */
	unsigned int enter_4ba: 8;

	/* Bits specifying supported modes of exiting 4-byte
	 * addressing.
	 */
	unsigned int exit_4ba: 10;

	/* Bits specifying the soft reset and rescue sequence to
	 * restore the device to its power-on state.
	 */
	unsigned int srrs_support: 6;

	/* Bits specifying how to modify status register 1, and which
	 * bits are non-volatile.
	 */
	unsigned int sr1_interface: 7;
};

/* Get data from BFP DW16.
 *
 * @param php pointer to the BFP header.
 *
 * @param bfp pointer to the BFP table.
 *
 * @param res pointer to where to store the decoded data.
 *
 * @retval -ENOTSUP if this information is not available from this BFP table.
 * @retval 0 on successful storage into @c *res.
 */
int jesd216_bfp_decode_dw16(const struct jesd216_param_header *php,
			    const struct jesd216_bfp *bfp,
			    struct jesd216_bfp_dw16 *res);

#endif /* ZEPHYR_DRIVERS_FLASH_JESD216_H_ */
