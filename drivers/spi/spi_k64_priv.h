/* spi_k64_priv.h - Freescale K64 SPI driver private definitions */

/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SPI_K64_PRIV_H__
#define __SPI_K64_PRIV_H__

typedef void (*spi_k64_config_t)(void);

struct spi_k64_config {
	uint32_t regs;					/* base address of SPI module registers */
	uint32_t clk_gate_reg;			/* SPI module's clock gate register addr. */
	uint32_t clk_gate_bit;			/* SPI module's clock gate bit position */
	uint32_t irq;					/* SPI module IRQ number */
	spi_k64_config_t config_func;	/* IRQ configuration function pointer */
};

struct spi_k64_data {
	uint8_t frame_sz;				/* frame/word size, in bits */
	uint8_t cont_pcs_sel;			/* continuous slave/PCS selection enable */
	uint8_t pcs;					/* slave/PCS selection */
	const uint8_t *tx_buf;
	uint32_t tx_buf_len;
	uint8_t *rx_buf;
	uint32_t rx_buf_len;
	uint32_t xfer_len;
	device_sync_call_t sync_info;   /* sync call information */
	uint8_t error;                  /* error condition */
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t device_power_state;
#endif
};

/* Data transfer signal timing delays */

enum spi_k64_delay_id {
	DELAY_PCS_TO_SCK,
	DELAY_AFTER_SCK,
	DELAY_AFTER_XFER
};

/* Register offsets */

#define SPI_K64_REG_MCR			(0x00)
#define SPI_K64_REG_TCR			(0x08)
#define SPI_K64_REG_CTAR0		(0x0C)
#define SPI_K64_REG_CTAR1		(0x10)
#define SPI_K64_REG_SR			(0x2C)
#define SPI_K64_REG_RSER		(0x30)
#define SPI_K64_REG_PUSHR		(0x34)
#define SPI_K64_REG_POPR		(0x38)
#define SPI_K64_REG_TXFR0		(0x3C)
#define SPI_K64_REG_RXFR0		(0x7C)

/* Module Control Register (MCR) settings */

#define SPI_K64_MCR_HALT		(0x1)
#define SPI_K64_MCR_HALT_BIT	(0)

#define SPI_K64_MCR_SMPL_PT_MSK	(0x3 << 8)

#define SPI_K64_MCR_CLR_RXF		(0x1 << 10)
#define SPI_K64_MCR_CLR_TXF		(0x1 << 11)
#define SPI_K64_MCR_DIS_RXF		(0x1 << 12)
#define SPI_K64_MCR_DIS_TXF		(0x1 << 13)
#define SPI_K64_MCR_MDIS		(0x1 << 14)
#define SPI_K64_MCR_MDIS_BIT	(14)
#define SPI_K64_MCR_DOZE		(0x1 << 15)

#define SPI_K64_MCR_PCSIS_MSK	(0x3F << 16)
#define SPI_K64_MCR_PCSIS_SET(pcsis)	((pcsis) << 16)

#define SPI_K64_MCR_ROOE		(0x1 << 24)
#define SPI_K64_MCR_PCSSE		(0x1 << 25)
#define SPI_K64_MCR_MTFE		(0x1 << 26)
#define SPI_K64_MCR_FRZ			(0x1 << 27)

#define SPI_K64_MCR_DCONF_MSK	(0x3 << 28)

#define SPI_K64_MCR_CONT_SCKE	(0x1 << 30)
#define SPI_K64_MCR_CONT_SCKE_SET(cont)	((cont) << 30)

#define SPI_K64_MCR_MSTR		(0x1 << 31)

/* Clock and Transfer Attributes Register (CTAR) settings */

#define SPI_K64_CTAR_BR_MSK			(0xF)

#define SPI_K64_CTAR_DT_MSK			(0xF << 4)
#define SPI_K64_CTAR_DT_SET(dt)		((dt) << 4)
#define SPI_K64_CTAR_ASC_MSK		(0xF << 8)
#define SPI_K64_CTAR_ASC_SET(asc)	((asc) << 8)
#define SPI_K64_CTAR_CSSCK_MSK		(0xF << 12)
#define SPI_K64_CTAR_CSSCK_SET(cssck)	((cssck) << 12)

#define SPI_K64_CTAR_PBR_MSK		(0x3 << 16)
#define SPI_K64_CTAR_PBR_SET(pbr)	((pbr) << 16)

#define SPI_K64_CTAR_PDT_MSK		(0xF << 18)
#define SPI_K64_CTAR_PDT_SET(pdt)	((pdt) << 18)
#define SPI_K64_CTAR_PASC_MSK		(0xF << 20)
#define SPI_K64_CTAR_PASC_SET(pasc)	((pasc) << 20)
#define SPI_K64_CTAR_PCSSCK_MSK		(0xF << 22)
#define SPI_K64_CTAR_PCSSCK_SET(pcssck)	((pcssck) << 22)

#define SPI_K64_CTAR_LSBFE			(0x1 << 24)
#define SPI_K64_CTAR_CPHA			(0x1 << 25)
#define SPI_K64_CTAR_CPOL			(0x1 << 26)

#define SPI_K64_CTAR_FRMSZ_MSK		(0xF << 27)
#define SPI_K64_CTAR_FRMSZ_SET(sz)	((sz) << 27)

#define SPI_K64_CTAR_DBR			(0x1 << 31)
#define SPI_K64_CTAR_DBR_SET(dbr)	((dbr) << 31)

/* Status Register (SR) settings */

#define SPI_K64_SR_POPNXTPTR_MSK	(0xF)
#define SPI_K64_SR_RXCTR_MSK		(0xF << 4)
#define SPI_K64_SR_TXNXTPTR_MSK		(0xF << 8)
#define SPI_K64_SR_TXCTR_MSK		(0xF << 12)

#define SPI_K64_SR_RFDF				(0x1 << 17)
#define SPI_K64_SR_RFOF				(0x1 << 19)
#define SPI_K64_SR_TFFF				(0x1 << 25)
#define SPI_K64_SR_TFUF				(0x1 << 27)
#define SPI_K64_SR_EOQF				(0x1 << 28)
#define SPI_K64_SR_TXRXS			(0x1 << 30)
#define SPI_K64_SR_TCF				(0x1 << 31)

/* DMA/Interrupt Request Select and Enable Register (RSER) settings */

#define SPI_K64_RSER_RFDF_DIRS		(0x1 << 16)
#define SPI_K64_RSER_RFDF_RE		(0x1 << 17)
#define SPI_K64_RSER_RFOF_RE		(0x1 << 19)
#define SPI_K64_RSER_TFFF_DIRS		(0x1 << 24)
#define SPI_K64_RSER_TFFF_RE		(0x1 << 25)
#define SPI_K64_RSER_TFUF_RE		(0x1 << 27)
#define SPI_K64_RSER_EOQF_RE		(0x1 << 28)
#define SPI_K64_RSER_TCF_RE			(0x1 << 31)

/* Push Tx FIFO Register (PUSHR) settings */

#define SPI_K64_PUSHR_TXDATA_MSK	(0xFF)
#define SPI_K64_PUSHR_PCS_MSK		(0x3F << 16)
#define SPI_K64_PUSHR_PCS_SET(pcs)	((pcs) << 16)

#define SPI_K64_PUSHR_CTCNT			(0x1 << 26)
#define SPI_K64_PUSHR_EOQ			(0x1 << 27)

#define SPI_K64_PUSHR_CTAS_MSK		(0x7 << 28)

#define SPI_K64_PUSHR_CONT			(0x1 << 31)
#define SPI_K64_PUSHR_CONT_SET(cont)	((cont) << 31)

/* Tx FIFO Register (TXFR) settings */

#define SPI_K64_TXFR_TXDATA_MSK	(0xFFFF)
#define SPI_K64_TXFR_TXCMD_MSK	(0xFFFF << 16)


#endif /* __SPI_K64_PRIV_H__ */
