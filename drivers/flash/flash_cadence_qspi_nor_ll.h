/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2019, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CAD_QSPI_NOR_LL_H
#define CAD_QSPI_NOR_LL_H

#include <device.h>

#define CAD_QSPI_MICRON_N25Q_SUPPORT		1

#define CAD_INVALID				-1
#define CAD_QSPI_ERROR				-2

#define CAD_QSPI_ADDR_FASTREAD			0
#define CAD_QSPI_ADDR_FASTREAD_DUAL_IO		1
#define CAD_QSPI_ADDR_FASTREAD_QUAD_IO		2
#define CAT_QSPI_ADDR_SINGLE_IO			0
#define CAT_QSPI_ADDR_DUAL_IO			1
#define CAT_QSPI_ADDR_QUAD_IO			2

#define CAD_QSPI_BANK_ADDR(x)			((x) >> 24)
#define CAD_QSPI_BANK_ADDR_MSK			0xff000000

#define CAD_QSPI_COMMAND_TIMEOUT		0x10000000

#define CAD_QSPI_CFG				0x0
#define CAD_QSPI_CFG_BAUDDIV_MSK		0xff87ffff
#define CAD_QSPI_CFG_BAUDDIV(x)			(((x) << 19) & 0x780000)
#define CAD_QSPI_CFG_CS_MSK			~0x3c00
#define CAD_QSPI_CFG_CS(x)			(((x) << 11))
#define CAD_QSPI_CFG_ENABLE			(1 << 0)
#define CAD_QSPI_CFG_ENDMA_CLR_MSK		0xffff7fff
#define CAD_QSPI_CFG_IDLE			(1U << 31)
#define CAD_QSPI_CFG_SELCLKPHASE_CLR_MSK	0xfffffffb
#define CAD_QSPI_CFG_SELCLKPOL_CLR_MSK		0xfffffffd

#define CAD_QSPI_DELAY				0xc
#define CAD_QSPI_DELAY_CSSOT(x)			(((x) & 0xff) << 0)
#define CAD_QSPI_DELAY_CSEOT(x)			(((x) & 0xff) << 8)
#define CAD_QSPI_DELAY_CSDADS(x)		(((x) & 0xff) << 16)
#define CAD_QSPI_DELAY_CSDA(x)			(((x) & 0xff) << 24)

#define CAD_QSPI_DEVSZ				0x14
#define CAD_QSPI_DEVSZ_ADDR_BYTES(x)		((x) << 0)
#define CAD_QSPI_DEVSZ_BYTES_PER_PAGE(x)	((x) << 4)
#define CAD_QSPI_DEVSZ_BYTES_PER_BLOCK(x)	((x) << 16)

#define CAD_QSPI_DEVWR				0x8
#define CAD_QSPI_DEVRD				0x4
#define CAD_QSPI_DEV_OPCODE(x)			(((x) & 0xff) << 0)
#define CAD_QSPI_DEV_INST_TYPE(x)		(((x) & 0x03) << 8)
#define CAD_QSPI_DEV_ADDR_TYPE(x)		(((x) & 0x03) << 12)
#define CAD_QSPI_DEV_DATA_TYPE(x)		(((x) & 0x03) << 16)
#define CAD_QSPI_DEV_MODE_BIT(x)		(((x) & 0x01) << 20)
#define CAD_QSPI_DEV_DUMMY_CLK_CYCLE(x)		(((x) & 0x0f) << 24)

#define CAD_QSPI_FLASHCMD			0x90
#define CAD_QSPI_FLASHCMD_ADDR			0x94
#define CAD_QSPI_FLASHCMD_EXECUTE		0x1
#define CAD_QSPI_FLASHCMD_EXECUTE_STAT		0x2
#define CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES_MAX	5
#define CAD_QSPI_FLASHCMD_NUM_DUMMYBYTES(x)	(((x) << 7) & 0x000f80)
#define CAD_QSPI_FLASHCMD_OPCODE(x)		(((x) & 0xff) << 24)
#define CAD_QSPI_FLASHCMD_ENRDDATA(x)		(((x) & 1) << 23)
#define CAD_QSPI_FLASHCMD_NUMRDDATABYTES(x)	(((x) & 0xf) << 20)
#define CAD_QSPI_FLASHCMD_ENCMDADDR(x)		(((x) & 1) << 19)
#define CAD_QSPI_FLASHCMD_ENMODEBIT(x)		(((x) & 1) << 18)
#define CAD_QSPI_FLASHCMD_NUMADDRBYTES(x)	(((x) & 0x3) << 16)
#define CAD_QSPI_FLASHCMD_ENWRDATA(x)		(((x) & 1) << 15)
#define CAD_QSPI_FLASHCMD_NUMWRDATABYTES(x)	(((x) & 0x7) << 12)
#define CAD_QSPI_FLASHCMD_NUMDUMMYBYTES(x)	(((x) & 0x1f) << 7)
#define CAD_QSPI_FLASHCMD_RDDATA0		0xa0
#define CAD_QSPI_FLASHCMD_RDDATA1		0xa4
#define CAD_QSPI_FLASHCMD_WRDATA0		0xa8
#define CAD_QSPI_FLASHCMD_WRDATA1		0xac

#define CAD_QSPI_RDDATACAP			0x10
#define CAD_QSPI_RDDATACAP_BYP(x)		(((x) & 1) << 0)
#define CAD_QSPI_RDDATACAP_DELAY(x)		(((x) & 0xf) << 1)

#define CAD_QSPI_REMAPADDR			0x24
#define CAD_QSPI_REMAPADDR_VALUE_SET(x)		(((x) & 0xffffffff) << 0)

#define CAD_QSPI_SRAMPART			0x18
#define CAD_QSPI_SRAMFILL			0x2c
#define CAD_QSPI_SRAMPART_ADDR(x)		(((x) >> 0) & 0x3ff)
#define CAD_QSPI_SRAM_FIFO_ENTRY_COUNT		(512 / sizeof(uint32_t))
#define CAD_QSPI_SRAMFILL_INDWRPART(x)		(((x) >> 16) & 0x00ffff)
#define CAD_QSPI_SRAMFILL_INDRDPART(x)		(((x) >> 0) & 0x00ffff)

#define CAD_QSPI_SELCLKPHASE(x)			(((x) & 1) << 2)
#define CAD_QSPI_SELCLKPOL(x)			(((x) & 1) << 1)

#define CAD_QSPI_STIG_FLAGSR_PROGRAMREADY(x)	(((x) >> 7) & 1)
#define CAD_QSPI_STIG_FLAGSR_ERASEREADY(x)	(((x) >> 7) & 1)
#define CAD_QSPI_STIG_FLAGSR_ERASEERROR(x)	(((x) >> 5) & 1)
#define CAD_QSPI_STIG_FLAGSR_PROGRAMERROR(x)	(((x) >> 4) & 1)
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
#define CAD_QSPI_STIG_RDID_CAPACITYID(x)	(((x) >> 16) & 0xff)
#define CAD_QSPI_STIG_SR_BUSY(x)		(((x) >> 0) & 1)


#define CAD_QSPI_INST_SINGLE			0
#define CAD_QSPI_INST_DUAL			1
#define CAD_QSPI_INST_QUAD			2

#define CAD_QSPI_INDRDSTADDR			0x68
#define CAD_QSPI_INDRDCNT			0x6c
#define CAD_QSPI_INDRD				0x60
#define CAD_QSPI_INDRD_RD_STAT(x)		(((x) >> 2) & 1)
#define CAD_QSPI_INDRD_START			1
#define CAD_QSPI_INDRD_IND_OPS_DONE		0x20

#define CAD_QSPI_INDWR				0x70
#define CAD_QSPI_INDWR_RDSTAT(x)		(((x) >> 2) & 1)
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

#if defined(CONFIG_CAD_QSPI_NOR_SUBSECTOR_SIZE)
#define CAD_QSPI_SUBSECTOR_SIZE			CONFIG_CAD_QSPI_NOR_SUBSECTOR_SIZE
#else
#define CAD_QSPI_SUBSECTOR_SIZE			0x1000
#endif /* if defined(CONFIG_CAD_QSPI_NOR_SUBSECTOR_SIZE) */

#if defined(CONFIG_QSPI_ADDR_BYTES)
#define QSPI_ADDR_BYTES				CONFIG_QSPI_ADDR_BYTES
#else
#define QSPI_ADDR_BYTES				2
#endif /* if defined(CONFIG_QSPI_ADDR_BYTES) */

#if defined(CONFIG_QSPI_BYTES_PER_DEV)
#define QSPI_BYTES_PER_DEV			CONFIG_QSPI_BYTES_PER_DEV
#else
#define QSPI_BYTES_PER_DEV			256
#endif /* if defined(CONFIG_QSPI_BYTES_PER_DEV) */

#if defined(CONFIG_QSPI_BYTES_PER_BLOCK)
#define QSPI_BYTES_PER_BLOCK			CONFIG_QSPI_BYTES_PER_BLOCK
#else
#define QSPI_BYTES_PER_BLOCK			16
#endif /* if defined(CONFIG_QSPI_BYTES_PER_BLOCK) */

#define QSPI_FAST_READ				0xb

#define QSPI_WRITE				0x2

/* QSPI CONFIGURATIONS */

#define QSPI_CONFIG_CPOL			1
#define QSPI_CONFIG_CPHA			1

#define QSPI_CONFIG_CSSOT			0x14
#define QSPI_CONFIG_CSEOT			0x14
#define QSPI_CONFIG_CSDADS			0xff
#define QSPI_CONFIG_CSDA			0xc8

typedef struct cad_qspi_params {
	uintptr_t	reg_base;
	uintptr_t	data_base;
	int		clk_rate;
	uint32_t	qspi_device_size;
	int		cad_qspi_cs;
} cad_qspi_params_t;

int cad_qspi_init(cad_qspi_params_t *cad_params, uint32_t clk_phase,
	uint32_t clk_pol, uint32_t csda, uint32_t csdads,
	uint32_t cseot, uint32_t cssot, uint32_t rddatacap);
void cad_qspi_set_chip_select(cad_qspi_params_t *cad_params, int cs);
int cad_qspi_erase(cad_qspi_params_t *cad_params, uint32_t offset,
	uint32_t size);
int cad_qspi_write(cad_qspi_params_t *cad_params, void *buffer,
	uint32_t offset, uint32_t size);
int cad_qspi_read(cad_qspi_params_t *cad_params, void *buffer,
	uint32_t offset, uint32_t size);
int cad_qspi_update(cad_qspi_params_t *cad_params, void *buffer,
	uint32_t offset, uint32_t size);

#endif

