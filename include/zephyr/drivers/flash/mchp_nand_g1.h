/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_NAND_G1_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_NAND_G1_H__

#define CMD_READ0	0
#define CMD_READ1	0x1
#define CMD_RNDOUT	0x5
#define CMD_PAGEPROG	0x10
#define CMD_READ2	0x30
#define CMD_READOOB	0x50
#define CMD_ERASE1	0x60
#define CMD_STATUS	0x70
#define CMD_SEQIN	0x80
#define CMD_RNDIN	0x85
#define CMD_READID	0x90
#define CMD_ERASE2	0xd0
#define CMD_PARAM	0xec
#define CMD_GET_FEAT	0xee
#define CMD_SET_FEAT	0xef
#define CMD_RESET	0xff

#define STATUS_FAIL	BIT(0)
#define STATUS_FAIL_N1	BIT(1)
#define STATUS_ARDY	BIT(5)
#define STATUS_RDY	BIT(6)
#define STATUS_WP	BIT(7)

#define ONFI_PARAM_PAGES	3
#define ONFI_PARAM_SIZE		256
#define ONFI_CRC_BASE		0x4F4E
#define ONFI_CRC_SIZE		254
#define ONFI_DEF_ECC_SIZE	512

#define PARAMS_OFFSET_REVISION	4
#define   PARAMS_REVISION_1_0	BIT(1)
#define   PARAMS_REVISION_2_0	BIT(2)
#define   PARAMS_REVISION_2_1	BIT(3)
#define PARAMS_OFFSET_FEATURES	6
#define   PARAMS_FEATURE_BUSWIDTH	BIT(0)
#define   PARAMS_FEATURE_EXTENDED_PARAM	BIT(7)
#define PARAMS_OFFSET_OPT_CMD	8
#define   PARAMS_OPT_CMD_SET_GET_FEATURES	BIT(2)
#define PARAMS_OFFSET_MFR_NAME	32
#define PARAMS_OFFSET_MOD_NAME	44
#define PARAMS_OFFSET_MFR_ID	64
#define PARAMS_OFFSET_PAGESIZE	80
#define PARAMS_OFFSET_SPARESIZE	84
#define PARAMS_OFFSET_BLOCKSIZE	92
#define PARAMS_OFFSET_UNITSIZE	96
#define PARAMS_OFFSET_UNIT_NUM	100
#define PARAMS_OFFSET_ADDRCYCLE	101
#define PARAMS_OFFSET_CELLBITS	102
#define PARAMS_OFFSET_ECC_BITS	112
#define PARAMS_OFFSET_TIMING	129
#define   PARAMS_TIMING_MODE_0	BIT(0)
#define   PARAMS_TIMING_MODE_1	BIT(1)
#define   PARAMS_TIMING_MODE_2	BIT(2)
#define   PARAMS_TIMING_MODE_3	BIT(3)
#define   PARAMS_TIMING_MODE_4	BIT(4)
#define   PARAMS_TIMING_MODE_5	BIT(5)
#define PARAMS_OFFSET_CRC	254

#define MAX_ID_LEN 5

enum nand_op_id {
	OP_RESET,
	OP_READID,
	OP_ONFI,
	OP_PARAM,
	OP_GET_FEAT,
	OP_SET_FEAT,
	OP_STATUS,
	OP_READ,
	OP_WRITE,
	OP_PROGRAM,
	OP_ERASE,
	OP_MAX
};

enum nand_op_flag {
	F_ADDR = BIT(0), /* Use custom address */
	F_CMD2 = BIT(1), /* Operation has cmd2 */
	F_BUSY = BIT(2), /* Wait for ready */
	F_PAGE = BIT(3), /* Page data operation */
	F_OOB  = BIT(4), /* OOB data operation */
	F_RAW  = BIT(5), /* Raw data operation */
	F_OUT  = BIT(6), /* Data direction, default in */

	F_READ  = F_ADDR | F_CMD2 | F_BUSY,
	F_WRITE = F_ADDR | F_OUT,
	F_ERASE = F_ADDR | F_CMD2 | F_BUSY,
};

struct nand_op {
	uint8_t cmd;
	uint16_t acycle: 4;
	uint16_t dcycle: 12;
	uint8_t addr;
	uint8_t cmd2;
	uint16_t flag;
};

#define CMD(x)		.cmd=(x)
#define ACYCLE(x)	.acycle=(x)
#define DCYCLE(x)	.dcycle=(x)
#define ADDR(x)		.addr=(x)
#define CMD2(x)		.cmd2=(x)
#define FLAG(x)		.flag=(x)

struct bus_info {
	uint32_t cs;
	void *addr;
};

struct ecc_info {
	uint16_t bits;  /* strength bits */
	uint16_t size;  /* sector size */
	uint16_t steps; /* sector number */
	uint16_t bytes; /* bytes per sector */
	uint16_t addr;  /* offset address in oob */
};

struct nand_info {
	union {
		struct {
			uint8_t mfr_id;
			uint8_t dev_id;
		};
		uint8_t id[MAX_ID_LEN];
	};
	uint16_t pagesize;
	uint16_t oobsize;
	uint16_t blockpages;
	uint16_t blocknum;
	uint8_t  eccbits;
	uint16_t eccsize;
	uint16_t features;
	uint16_t opt_cmd;
	uint16_t timingmode;
	uint8_t  addrcycle;
	uint8_t  cellbits;
};

struct nand_chip {
	struct nand_info nand;
	struct ecc_info ecc;
	const struct bus_info bus;
};

struct nand_controller_ops {
	int (*ecc_init)(const struct device *dev, struct nand_chip *chip);
	int (*exec_op)(const struct device *dev, struct nand_chip *chip,
		       const struct nand_op *op, uint8_t *addrs, uint8_t *buf);
	void (*data_in)(const struct device *dev, struct nand_chip *chip,
			uint8_t *buf, uint32_t len);
	void (*data_out)(const struct device *dev, struct nand_chip *chip,
			 const uint8_t *buf, uint32_t len);
};

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_NAND_G1_H__ */
