/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT microchip_sama7g5_qspi

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/cache.h>
#include "flash_sam_qspi.h"

/* Use PIO for small transfers. */
#define QSPI_DMA_MIN_BYTES	16

/* QSPI TIMEOUT in ms */
#define QSPI_TIMEOUT	1000
#define QSPI_SYNC_TIMEOUT	300

struct sam_qspi_mode {
	uint8_t cmd_buswidth;
	uint8_t addr_buswidth;
	uint8_t data_buswidth;
	uint32_t config;
};

static const struct sam_qspi_mode sam_qspi_sama7g5_modes[] = {
	{ 1, 1, 1, QSPI_IFR_WIDTH_SINGLE_BIT_SPI },
	{ 1, 1, 2, QSPI_IFR_WIDTH_DUAL_OUTPUT },
	{ 1, 1, 4, QSPI_IFR_WIDTH_QUAD_OUTPUT },
	{ 1, 2, 2, QSPI_IFR_WIDTH_DUAL_IO },
	{ 1, 4, 4, QSPI_IFR_WIDTH_QUAD_IO },
	{ 2, 2, 2, QSPI_IFR_WIDTH_DUAL_CMD },
	{ 4, 4, 4, QSPI_IFR_WIDTH_QUAD_CMD },
	{ 1, 1, 8, QSPI_IFR_WIDTH_OCT_OUTPUT },
	{ 1, 8, 8, QSPI_IFR_WIDTH_OCT_IO },
	{ 8, 8, 8, QSPI_IFR_WIDTH_OCT_CMD },
};

static inline bool sam_qspi_is_compatible(const struct qspi_mem_op *op,
					    const struct sam_qspi_mode *mode)
{
	if (mode->cmd_buswidth != spi_flash_protocol_get_inst_nbits(op->proto)) {
		return false;
	}

	if (mode->addr_buswidth != spi_flash_protocol_get_addr_nbits(op->proto)) {
		return false;
	}

	if (mode->data_buswidth != spi_flash_protocol_get_data_nbits(op->proto)) {
		return false;
	}

	return true;
}

static int qspi_reg_sync(qspi_registers_t *qspi)
{
	uint32_t timeout_ms = QSPI_SYNC_TIMEOUT;

	while ((qspi->QSPI_SR & QSPI_SR_SYNCBSY_Msk) && timeout_ms--) {
		k_msleep(1);
	}

	if (!timeout_ms) {
		return -ETIME;
	}

	return 0;
}

static int qspi_update_config(qspi_registers_t *qspi)
{
	int ret;

	ret = qspi_reg_sync(qspi);
	if (ret) {
		return ret;
	}
	qspi->QSPI_CR = QSPI_CR_UPDCFG_Msk;

	return qspi_reg_sync(qspi);
}

static int qspi_find_mode(const struct qspi_mem_op *op)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(sam_qspi_sama7g5_modes); i++) {
		if (sam_qspi_is_compatible(op, &sam_qspi_sama7g5_modes[i])) {
			return i;
		}
	}

	return -EOPNOTSUPP;
}

static int qspi_set_mode_bits(const struct qspi_mem_op *op,
			      uint32_t *icr, uint32_t *ifr)
{
	uint32_t mode_cycle_bits, mode_bits;

	*icr |= QSPI_RICR_RDOPT(op->cmd.modebits);
	*ifr |= QSPI_IFR_OPTEN_Msk;

	switch (*ifr & QSPI_IFR_WIDTH_Msk) {
	case QSPI_IFR_WIDTH_SINGLE_BIT_SPI:
	case QSPI_IFR_WIDTH_DUAL_OUTPUT:
	case QSPI_IFR_WIDTH_QUAD_OUTPUT:
	case QSPI_IFR_WIDTH_OCT_OUTPUT:
		mode_cycle_bits = 1;
		break;
	case QSPI_IFR_WIDTH_DUAL_IO:
	case QSPI_IFR_WIDTH_DUAL_CMD:
		mode_cycle_bits = 2;
		break;
	case QSPI_IFR_WIDTH_QUAD_IO:
	case QSPI_IFR_WIDTH_QUAD_CMD:
		mode_cycle_bits = 4;
		break;
	case QSPI_IFR_WIDTH_OCT_IO:
	case QSPI_IFR_WIDTH_OCT_CMD:
		mode_cycle_bits = 8;
	default:
		return -EINVAL;
	}

	mode_bits = op->cmd.modebits * mode_cycle_bits;
	switch (mode_bits) {
	case 1:
		*ifr |= QSPI_IFR_OPTL_OPTION_1BIT;
		break;
	case 2:
		*ifr |= QSPI_IFR_OPTL_OPTION_2BIT;
		break;
	case 4:
		*ifr |= QSPI_IFR_OPTL_OPTION_4BIT;
		break;
	case 8:
		*ifr |= QSPI_IFR_OPTL_OPTION_8BIT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int qspi_set_cfg(qspi_registers_t *qspi, const struct qspi_mem_op *op)
{
	uint32_t iar, icr, ifr;
	volatile uint32_t dummy;
	int mode, ret;

	iar = 0;
	icr = QSPI_RICR_RDINST(op->cmd.opcode);
	ifr = QSPI_IFR_INSTEN_Msk;

	mode = qspi_find_mode(op);
	if (mode < 0) {
		return mode;
	}
	ifr |= sam_qspi_sama7g5_modes[mode].config;

	if (op->cmd.modebits) {
		ret = qspi_set_mode_bits(op, &icr, &ifr);
		if (ret) {
			return ret;
		}
	}

	/* Set the number of dummy cycles. */
	if (op->cmd.waitstates) {
		ifr |= QSPI_IFR_NBDUM(op->cmd.waitstates);
	}

	if (op->addr.nbytes) {
		ifr |= QSPI_IFR_ADDRL(op->addr.nbytes - 1) | QSPI_IFR_ADDREN_Msk;
		iar = QSPI_IAR_ADDR(op->addr.val);
	}

	if (op->cmd.dtr) {
		ifr |= QSPI_IFR_DDREN_Msk;
	}

	/* Set data enable */
	if (op->data.nbytes) {
		ifr |= QSPI_IFR_DATAEN_Msk;
		if (op->addr.nbytes) {
			ifr |= QSPI_IFR_TFRTYP_Msk;
		}
	}

	/* Clear pending interrupts */
	dummy = (volatile uint32_t)qspi->QSPI_ISR;

	/* Set QSPI Instruction Frame registers */
	if (op->addr.nbytes && !op->data.nbytes) {
		qspi->QSPI_IAR = iar;
	}

	if (op->data.dir == QSPI_MEM_DATA_IN) {
		qspi->QSPI_RICR = icr;
	} else {
		qspi->QSPI_WICR = icr;
		if (op->data.nbytes) {
			qspi->QSPI_WRACNT = QSPI_WRACNT_NBWRA(op->data.nbytes);
		}
	}
	qspi->QSPI_IFR = ifr;

	return qspi_update_config(qspi);
}

static int qspi_dma_transfer(struct qspi_priv *priv,
		const struct qspi_mem_op *op, uint32_t offset)
{
	const struct device *dma;
	struct dma_status stat;
	uint32_t channel;
	static struct dma_config dma_cfg = {0};
	static struct dma_block_config dma_block_cfg;
	uint32_t transferred = 0;

	dma = priv->dma;
	channel = priv->dma_channel;
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.block_count = 1U;
	if (op->data.dir == QSPI_MEM_DATA_IN) {
		dma_block_cfg.source_address = (uint32_t)(priv->mem + offset);
		dma_block_cfg.dest_address = (uint32_t)(op->data.buf.in);
	} else {
		dma_block_cfg.source_address = (uint32_t)(op->data.buf.out);
		dma_block_cfg.dest_address = (uint32_t)(priv->mem + offset);
	}

	dma_block_cfg.block_size = op->data.nbytes;
	dma_cfg.head_block = &dma_block_cfg;
	dma_block_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_block_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	if (dma_config(dma, channel, &dma_cfg)) {
		return -EINVAL;
	}

	if (dma_start(dma, channel)) {
		return -EIO;
	}

	while (transferred != op->data.nbytes) {
		if (dma_get_status(dma, channel, &stat) == 0) {
			transferred = op->data.nbytes - stat.pending_length;
		} else {
			return -EIO;
		}
	}

	if (op->data.dir == QSPI_MEM_DATA_IN) {
		sys_cache_data_invd_range((uint32_t *)op->data.buf.in, op->data.nbytes);
	}

	return 0;
}

static int qspi_transfer(struct qspi_priv *priv, const struct qspi_mem_op *op)
{
	uint32_t timeout_ms = QSPI_TIMEOUT;
	uint32_t offset;
	qspi_registers_t *qspi;
	int ret;

	offset = op->addr.val;
	qspi = priv->base;
	if (!op->data.nbytes) {
		/* Start the transfer. */
		ret = qspi_reg_sync(qspi);
		if (ret) {
			return ret;
		}

		qspi->QSPI_CR = QSPI_CR_STTFR_Msk;
		while (!(qspi->QSPI_ISR & QSPI_ISR_CSRA_Msk) && timeout_ms--) {
			k_msleep(1);
		}

		if (!timeout_ms) {
			return -ETIME;
		}

		return 0;
	}

	/* Send/Receive data. */
	if (op->data.dir == QSPI_MEM_DATA_IN) {
		if ((priv->dma) && (op->data.nbytes > QSPI_DMA_MIN_BYTES) &&
			(((uint32_t)(op->data.buf.in) % CONFIG_DCACHE_LINE_SIZE) == 0)) {
			sys_cache_data_flush_range(op->data.buf.in, op->data.nbytes);
			ret = qspi_dma_transfer(priv, op, offset);
			if (ret) {
				return ret;
			}
		} else {
			memcpy(op->data.buf.in, (void *)(priv->mem + offset), op->data.nbytes);
		}

		if (op->addr.nbytes) {
			while ((qspi->QSPI_SR & QSPI_SR_RBUSY_Msk) && timeout_ms--) {
				k_msleep(1);
			}
			if (!timeout_ms) {
				return -ETIME;
			}
		}
	} else {
		if ((priv->dma) && (op->data.nbytes > QSPI_DMA_MIN_BYTES) &&
			(((uint32_t)(op->data.buf.out) % CONFIG_DCACHE_LINE_SIZE) == 0)) {
			sys_cache_data_flush_range(op->data.buf.in, op->data.nbytes);
			ret = qspi_dma_transfer(priv, op, offset);
			if (ret) {
				return ret;
			}
		} else {
			memcpy((uint8_t *)(priv->mem + offset), op->data.buf.out, op->data.nbytes);
		}

		if (op->addr.nbytes) {
			while (!(qspi->QSPI_ISR & QSPI_ISR_LWRA_Msk) && timeout_ms--) {
				k_msleep(1);
			}
			if (!timeout_ms) {
				return -ETIME;
			}
		}
	}
	/* Release the chip-select. */
	ret = qspi_reg_sync(qspi);
	if (ret) {
		return ret;
	}
	qspi->QSPI_CR = QSPI_CR_LASTXFER_Msk;
	timeout_ms = QSPI_TIMEOUT;
	while (!(qspi->QSPI_ISR & QSPI_ISR_CSRA_Msk) && timeout_ms--) {
		k_msleep(1);
	}
	if (!timeout_ms) {
		return -ETIME;
	}

	return ret;
}

int qspi_sama7g5_init(struct qspi_priv *priv)
{
	qspi_registers_t *qspi;
	uint32_t timeout_ms = QSPI_TIMEOUT;
	int ret;

	qspi = (qspi_registers_t *)(priv->base);

	ret = qspi_reg_sync(qspi);
	if (ret) {
		return ret;
	}
	qspi->QSPI_CR = QSPI_CR_SWRST_Msk;

	qspi->QSPI_CR = QSPI_CR_DLLON_Msk;
	while (!(qspi->QSPI_SR & QSPI_SR_DLOCK_Msk) && timeout_ms--) {
		k_msleep(1);
	}
	if (!timeout_ms) {
		return -ETIME;
	}

	/* Set the QSPI controller by default in Serial Memory Mode */
	qspi->QSPI_MR = QSPI_MR_SMM_Msk | QSPI_MR_DQSDLYEN_Msk;
	ret = qspi_update_config(qspi);
	if (ret) {
		return ret;
	}
	/* Enable the QSPI controller. */
	timeout_ms = QSPI_TIMEOUT;
	qspi->QSPI_CR = QSPI_CR_QSPIEN_Msk;
	while (!(qspi->QSPI_SR & QSPI_SR_QSPIENS_Msk) && timeout_ms--) {
		k_msleep(1);
	}
	if (!timeout_ms) {
		return -ETIME;
	}
	qspi->QSPI_TOUT = QSPI_TOUT_Msk;

	return ret;
}

int qspi_exec_op(struct qspi_priv *priv, const struct qspi_mem_op *op)
{
	qspi_registers_t *qspi;
	int err;

	qspi = priv->base;

	if (op->addr.nbytes > 4) {
		return -EOPNOTSUPP;
	}
	err = qspi_set_cfg(qspi, op);
	if (err) {
		return err;
	}

	return qspi_transfer(priv, op);
}
