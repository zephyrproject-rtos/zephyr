/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CAD_QSPI_NOR_LL_H
#define CAD_QSPI_NOR_LL_H

#include <zephyr/device.h>

#define CAD_QSPI_MICRON_N25Q_SUPPORT		CONFIG_CAD_QSPI_MICRON_N25Q_SUPPORT

#define CAD_INVALID				-1
#define CAD_QSPI_ERROR				-2

#define CAD_QSPI_ADDR_FASTREAD			0
#define CAD_QSPI_ADDR_FASTREAD_DUAL_IO		1
#define CAD_QSPI_ADDR_FASTREAD_QUAD_IO		2
#define CAT_QSPI_ADDR_SINGLE_IO			0
#define CAT_QSPI_ADDR_DUAL_IO			1
#define CAT_QSPI_ADDR_QUAD_IO			2

#define CAD_QSPI_BANK_ADDR(x)			((x) >> 24)
#define CAD_QSPI_BANK_ADDR_MSK			GENMASK(31, 24)

#define CAD_QSPI_COMMAND_TIMEOUT		0x10000000

#define CAD_QSPI_CFG				0x0
#define CAD_QSPI_CFG_BAUDDIV_MSK		0xff87ffff
#define CAD_QSPI_CFG_BAUDDIV(x)			FIELD_PREP(0x780000, x)
#define CAD_QSPI_CFG_CS_MSK			~0x3c00
#define CAD_QSPI_CFG_CS(x)			(((x) << 11))
#define CAD_QSPI_CFG_ENABLE			(BIT(0))
#define CAD_QSPI_CFG_ENDMA_CLR_MSK		0xffff7fff
#define CAD_QSPI_CFG_IDLE			(BIT(31))
#define CAD_QSPI_CFG_SELCLKPHASE_CLR_MSK	0xfffffffb
#define CAD_QSPI_CFG_SELCLKPOL_CLR_MSK		0xfffffffd

#define CAD_QSPI_DELAY				0xc
#define CAD_QSPI_DELAY_CSSOT(x)			(FIELD_GET(0xff, (x)) << 0)
#define CAD_QSPI_DELAY_CSEOT(x)			(FIELD_GET(0xff, (x)) << 8)
#define CAD_QSPI_DELAY_CSDADS(x)		(FIELD_GET(0xff, (x)) << 16)
#define CAD_QSPI_DELAY_CSDA(x)			(FIELD_GET(0xff, (x)) << 24)

#define CAD_QSPI_DEVSZ				0x14
#define CAD_QSPI_DEVSZ_ADDR_BYTES(x)		((x) << 0)
#define CAD_QSPI_DEVSZ_BYTES_PER_PAGE(x)	((x) << 4)
#define CAD_QSPI_DEVSZ_BYTES_PER_BLOCK(x)	((x) << 16)

#define CAD_QSPI_DEVWR				0x8
#define CAD_QSPI_DEVRD				0x4
#define CAD_QSPI_DEV_OPCODE(x)			(FIELD_GET(0xff, (x)) << 0)
#define CAD_QSPI_DEV_INST_TYPE(x)		(FIELD_GET(0x03, (x)) << 8)
#define CAD_QSPI_DEV_ADDR_TYPE(x)		(FIELD_GET(0x03, (x)) << 12)
#define CAD_QSPI_DEV_DATA_TYPE(x)		(FIELD_GET(0x03, (x)) << 16)
#define CAD_QSPI_DEV_MODE_BIT(x)		(FIELD_GET(0x01, (x)) << 20)
#define CAD_QSPI_DEV_DUMMY_CLK_CYCLE(x)		(FIELD_GET(0x0f, (x)) << 24)

#define CAD_QSPI_FLASHCMD			0x90
#define CAD_QSPI_FLASHCMD_ADDR			0x94
#define CAD_QSPI_FLASHCMD_EXECUTE		0x1
#define CAD_QSPI_FLASHCMD_EXECUTE_STAT		0x2
#define CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES_MAX	5
#define CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES(x)     (FIELD_PREP(0x000f80, (x)))
#define CAD_QSPI_FLASHCMD_OPCODE(x)             (FIELD_GET(0xff, (x)) << 24)
#define CAD_QSPI_FLASHCMD_ENRDDATA(x)           (FIELD_GET(1, (x)) << 23)
#define CAD_QSPI_FLASHCMD_NUMRDDATABYTES(x)     (FIELD_GET(0xf, (x)) << 20)
#define CAD_QSPI_FLASHCMD_ENCMDADDR(x)          (FIELD_GET(1, (x)) << 19)
#define CAD_QSPI_FLASHCMD_ENMODEBIT(x)		(FIELD_GET(1, (x)) << 18)
#define CAD_QSPI_FLASHCMD_NUMADDRBYTES(x)	(FIELD_GET(0x3, (x)) << 16)
#define CAD_QSPI_FLASHCMD_ENWRDATA(x)		(FIELD_GET(1, (x)) << 15)
#define CAD_QSPI_FLASHCMD_NUMWRDATABYTES(x)	(FIELD_GET(0x7, (x)) << 12)
#define CAD_QSPI_FLASHCMD_NUMDUMMYBYTES(x)	(FIELD_GET(0x1f, (x)) << 7)
#define CAD_QSPI_FLASHCMD_RDDATA0		0xa0
#define CAD_QSPI_FLASHCMD_RDDATA1		0xa4
#define CAD_QSPI_FLASHCMD_WRDATA0		0xa8
#define CAD_QSPI_FLASHCMD_WRDATA1		0xac

#define CAD_QSPI_RDDATACAP			0x10
#define CAD_QSPI_RDDATACAP_BYP(x)		(FIELD_GET(1, (x)) << 0)
#define CAD_QSPI_RDDATACAP_DELAY(x)		(FIELD_GET(0xf, (x)) << 1)

#define CAD_QSPI_REMAPADDR			0x24
#define CAD_QSPI_REMAPADDR_VALUE_SET(x)		(FIELD_GET(0xffffffff, (x)) << 0)

#define CAD_QSPI_SRAMPART			0x18
#define CAD_QSPI_SRAMFILL			0x2c
#define CAD_QSPI_SRAMPART_ADDR(x)		(FIELD_GET(0x3ff, ((x) >> 0)))
#define CAD_QSPI_SRAM_FIFO_ENTRY_COUNT		(512 / sizeof(uint32_t))
#define CAD_QSPI_SRAMFILL_INDWRPART(x)		(FIELD_GET(0x00ffff, ((x) >> 16)))
#define CAD_QSPI_SRAMFILL_INDRDPART(x)		(FIELD_GET(0x00ffff, ((x) >> 0)))

#define CAD_QSPI_SELCLKPHASE(x)			(FIELD_GET(1, (x)) << 2)
#define CAD_QSPI_SELCLKPOL(x)			(FIELD_GET(1, (x)) << 1)

#define CAD_QSPI_STIG_FLAGSR_PROGRAMREADY(x)	(FIELD_GET(1, ((x) >> 7)))
#define CAD_QSPI_STIG_FLAGSR_ERASEREADY(x)	(FIELD_GET(1, ((x) >> 7)))
#define CAD_QSPI_STIG_FLAGSR_ERASEERROR(x)	(FIELD_GET(1, ((x) >> 5)))
#define CAD_QSPI_STIG_FLAGSR_PROGRAMERROR(x)	(FIELD_GET(1, ((x) >> 4)))
#define CAD_QSPI_STIG_OPCODE_CLFSR		0x50
#define CAD_QSPI_STIG_OPCODE_RDID		0x9f
#define CAD_QSPI_STIG_OPCODE_WRDIS		0x4
#define CAD_QSPI_STIG_OPCODE_WREN		0x6
#define CAD_QSPI_STIG_OPCODE_SUBSEC_ERASE	0x20
#define CAD_QSPI_STIG_OPCODE_SEC_ERASE		0xd8
#define CAD_QSPI_STIG_OPCODE_WREN_EXT_REG	0xc5
#define CAD_QSPI_STIG_OPCODE_DIE_ERASE		0xc4
#define CAD_QSPI_STIG_OPCODE_BULK_ERASE		0xc7
#define CAD_QSPI_STIG_OPCODE_RDSR		0x5
#define CAD_QSPI_STIG_OPCODE_RDFLGSR		0x70
#define CAD_QSPI_STIG_OPCODE_RESET_EN		0x66
#define CAD_QSPI_STIG_OPCODE_RESET_MEM		0x99
#define CAD_QSPI_STIG_RDID_CAPACITYID(x)	(FIELD_GET(0xff, ((x) >> 16)))
#define CAD_QSPI_STIG_SR_BUSY(x)		(FIELD_GET(1, ((x))))


#define CAD_QSPI_INST_SINGLE			0
#define CAD_QSPI_INST_DUAL			1
#define CAD_QSPI_INST_QUAD			2

#define CAD_QSPI_INDRDSTADDR			0x68
#define CAD_QSPI_INDRDCNT			0x6c
#define CAD_QSPI_INDRD				0x60
#define CAD_QSPI_INDRD_RD_STAT(x)		(FIELD_GET(1, ((x) >> 2)))
#define CAD_QSPI_INDRD_START			1
#define CAD_QSPI_INDRD_IND_OPS_DONE		0x20

#define CAD_QSPI_INDWR				0x70
#define CAD_QSPI_INDWR_RDSTAT(x)		(FIELD_GET(1, ((x) >> 2)))
#define CAD_QSPI_INDWRSTADDR			0x78
#define CAD_QSPI_INDWRCNT			0x7c
#define CAD_QSPI_INDWR				0x70
#define CAD_QSPI_INDWR_START			0x1
#define CAD_QSPI_INDWR_INDDONE			0x20

#define CAD_QSPI_INT_STATUS_ALL			0x0000ffff

#define CAD_QSPI_N25Q_DIE_SIZE			0x02000000
#define CAD_QSPI_BANK_SIZE			0x01000000
#define CAD_QSPI_PAGE_SIZE			0x00000100

#define CAD_QSPI_IRQMSK				0x44

#define CAD_QSPI_SUBSECTOR_SIZE			CONFIG_CAD_QSPI_NOR_SUBSECTOR_SIZE
#define QSPI_ADDR_BYTES				CONFIG_QSPI_ADDR_BYTES
#define QSPI_BYTES_PER_DEV			CONFIG_QSPI_BYTES_PER_DEV
#define QSPI_BYTES_PER_BLOCK			CONFIG_QSPI_BYTES_PER_BLOCK

#define QSPI_FAST_READ				0xb

#define QSPI_WRITE				0x2

/* QSPI CONFIGURATIONS */

#define QSPI_CONFIG_CPOL			1
#define QSPI_CONFIG_CPHA			1

#define QSPI_CONFIG_CSSOT			0x14
#define QSPI_CONFIG_CSEOT			0x14
#define QSPI_CONFIG_CSDADS			0xff
#define QSPI_CONFIG_CSDA			0xc8

struct cad_qspi_params {
	uintptr_t	reg_base;
	uintptr_t	data_base;
	uint32_t	data_size;
	int		clk_rate;
	uint32_t	qspi_device_size;
	int		cad_qspi_cs;
};

int cad_qspi_init(struct cad_qspi_params *cad_params,
	uint32_t clk_phase, uint32_t clk_pol, uint32_t csda,
	uint32_t csdads, uint32_t cseot, uint32_t cssot,
	uint32_t rddatacap);
void cad_qspi_set_chip_select(struct cad_qspi_params *cad_params,
	int cs);
int cad_qspi_erase(struct cad_qspi_params *cad_params,
	uint32_t offset, uint32_t size);
int cad_qspi_write(struct cad_qspi_params *cad_params, void *buffer,
	uint32_t offset, uint32_t size);
int cad_qspi_read(struct cad_qspi_params *cad_params, void *buffer,
	uint32_t offset, uint32_t size);
int cad_qspi_update(struct cad_qspi_params *cad_params, void *buffer,
	uint32_t offset, uint32_t size);

#endif
