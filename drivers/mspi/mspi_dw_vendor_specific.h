/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This header is part of mspi_dw.c extracted only for clarity.
 * It is not supposed to be included by any file other than mspi_dw.c.
 */

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_exmif)

#include <nrf.h>

static inline void vendor_specific_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	NRF_EXMIF->EVENTS_CORE = 0;
	NRF_EXMIF->INTENSET = BIT(EXMIF_INTENSET_CORE_Pos);
}

static inline void vendor_specific_suspend(const struct device *dev)
{
	ARG_UNUSED(dev);

	NRF_EXMIF->TASKS_STOP = 1;
}

static inline void vendor_specific_resume(const struct device *dev)
{
	ARG_UNUSED(dev);

	NRF_EXMIF->TASKS_START = 1;

	/* Try to write an SSI register and wait until the write is successful
	 * to ensure that the clock that drives the SSI core is ready.
	 */
	uint32_t rxftlr = read_rxftlr(dev);
	uint32_t rxftlr_mod = rxftlr ^ 1;

	do {
		write_rxftlr(dev, rxftlr_mod);
		rxftlr = read_rxftlr(dev);
	} while (rxftlr != rxftlr_mod);
}

static inline void vendor_specific_irq_clear(const struct device *dev)
{
	ARG_UNUSED(dev);

	NRF_EXMIF->EVENTS_CORE = 0;
}

#if defined(CONFIG_MSPI_XIP)
static inline int vendor_specific_xip_enable(const struct device *dev,
					     const struct mspi_dev_id *dev_id,
					     const struct mspi_xip_cfg *cfg)
{
	ARG_UNUSED(dev);

	if (dev_id->dev_idx == 0) {
		NRF_EXMIF->EXTCONF1.OFFSET = cfg->address_offset;
		NRF_EXMIF->EXTCONF1.SIZE = cfg->address_offset
					 + cfg->size - 1;
		NRF_EXMIF->EXTCONF1.ENABLE = 1;
	} else if (dev_id->dev_idx == 1) {
		NRF_EXMIF->EXTCONF2.OFFSET = cfg->address_offset;
		NRF_EXMIF->EXTCONF2.SIZE = cfg->address_offset
					 + cfg->size - 1;
		NRF_EXMIF->EXTCONF2.ENABLE = 1;
	} else {
		return -EINVAL;
	}

	return 0;
}

static inline int vendor_specific_xip_disable(const struct device *dev,
					      const struct mspi_dev_id *dev_id,
					      const struct mspi_xip_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	if (dev_id->dev_idx == 0) {
		NRF_EXMIF->EXTCONF1.ENABLE = 0;
	} else if (dev_id->dev_idx == 1) {
		NRF_EXMIF->EXTCONF2.ENABLE = 0;
	} else {
		return -EINVAL;
	}

	return 0;
}
#endif /* defined(CONFIG_MSPI_XIP) */

#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_mspi)
#define MSPI_DT_DRV_COMPAT nordic_nrf_mspi
#include <nrf.h>

static inline void vendor_specific_init(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	preg->EVENTS_CORE = 0;
	preg->EVENTS_DMA.DONE = 0;

	preg->INTENSET |= BIT(QSPI_INTENSET_CORE_Pos);
	preg->INTENSET |= BIT(QSPI_INTENSET_DMADONE_Pos);

}

static inline void vendor_specific_suspend(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	preg->ENABLE = 0;
}

static inline void vendor_specific_resume(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	preg->ENABLE = 1;

	/* Try to write an SSI register and wait until the write is successful
	 * to ensure that the clock that drives the SSI core is ready.
	 */
	uint32_t rxftlr = read_rxftlr(dev);
	uint32_t rxftlr_mod = rxftlr ^ 1;

	do {
		write_rxftlr(dev, rxftlr_mod);
		rxftlr = read_rxftlr(dev);
	} while (rxftlr != rxftlr_mod);
}

static inline void vendor_specific_irq_clear(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	preg->EVENTS_CORE = 0;
	preg->EVENTS_DMA.DONE = 0;
}

/* DMA support */

#define EVDMA_ATTR_LEN_Pos (0UL)
#define EVDMA_ATTR_LEN_Msk (0x00FFFFFFUL)

#define EVDMA_ATTR_ATTR_Pos (24UL)
#define EVDMA_ATTR_ATTR_Msk (0x3FUL << EVDMA_ATTR_ATTR_Pos)

#define EVDMA_ATTR_32AXI_Pos (30UL)
#define EVDMA_ATTR_32AXI_Msk (0x1UL << EVDMA_ATTR_32AXI_Pos)

#define EVDMA_ATTR_EVENTS_Pos (31UL)
#define EVDMA_ATTR_EVENTS_Msk (0x1UL << EVDMA_ATTR_EVENTS_Pos)

typedef enum {
	EVDMA_BYTE_SWAP = 0,
	EVDMA_JOBLIST = 1,
	EVDMA_BUFFER_FILL = 2,
	EVDMA_FIXED_ATTR = 3,
	EVDMA_STATIC_ADDR = 4,
	EVDMA_PLAIN_DATA_BUF_WR = 5,
	EVDMA_PLAIN_DATA = 0x3f
} EVDMA_ATTR_Type;

typedef struct {
	uint8_t *addr;
	uint32_t attr;
} EVDMA_JOB_Type;

#define EVDMA_JOB(BUFFER, SIZE, ATTR) \
	(EVDMA_JOB_Type) { .addr = (uint8_t *)BUFFER, .attr = (ATTR << EVDMA_ATTR_ATTR_Pos | SIZE) }
#define EVDMA_NULL_JOB() \
	(EVDMA_JOB_Type) { .addr = (uint8_t *)0, .attr = 0 }
typedef struct {
	EVDMA_JOB_Type *tx_job;
	EVDMA_JOB_Type *rx_job;
} QSPI_TRANSFER_LIST_Type;

/* Number of jobs needed for transmit trasaction */
#define MAX_NUM_JOBS 4
/* Just support 1 trasaction for each peripheral as concurrent transactions aren't supported yet*/
#define MAX_CONCURR_TRANSACTIONS 1
#define NUM_LISTS DT_NUM_INST_STATUS_OKAY(MSPI_DT_DRV_COMPAT)
#define DMA_TRANSFER_LIST_SIZE (sizeof(QSPI_TRANSFER_LIST_Type) + sizeof(EVDMA_JOB_Type) * \
				MAX_NUM_JOBS * MAX_CONCURR_TRANSACTIONS)
#define DMA_TRANSFER_LIST_ALIGN 4
K_MEM_SLAB_DEFINE(dma_transfer_list_slab, DMA_TRANSFER_LIST_SIZE, NUM_LISTS,
		  DMA_TRANSFER_LIST_ALIGN);

static inline void vendor_specific_enable_dma_irq(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	preg->INTENSET = BIT(QSPI_INTENSET_DMADONE_Pos);
}

static inline void vendor_specific_start_dma_xfer(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	preg->TASKS_START = 1;
}

/* Temporarily hard-coded as not in MDK yet */
#define QSPI_TMOD_OFFSET (0x490UL)
#define QSPI_TMOD_RX_ONLY (0x2)
static inline int vendor_specific_setup_dma_xfer(const struct device *dev,
						 const struct mspi_xfer_packet *packet,
						 const struct mspi_xfer *xfer)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	void *transfer_list_ptr;
	int rc = k_mem_slab_alloc(&dma_transfer_list_slab, &transfer_list_ptr, K_NO_WAIT);

	if (rc < 0) {
		LOG_ERR("Failed to allocate transfer list");
		return rc;
	}

	/* Create DMA transfer list based on whether it is an RX or TX transfer */
	QSPI_TRANSFER_LIST_Type *transfer_list = (QSPI_TRANSFER_LIST_Type *)transfer_list_ptr;

	dev_data->dma_transfer_list = (void *) transfer_list;

	/* Right after transfer_list */
	EVDMA_JOB_Type *joblist = (EVDMA_JOB_Type *)(transfer_list + 1);

	int job_idx = 0;

	if (packet->dir == MSPI_TX) {
		preg->CONFIG.RXTRANSFERLENGTH = 0;

		/* Setting up EVDMA joblist dependin on cmd, addr and data */

		/*
		 * Command address will always have a length of 4 from the DMA's perspective,
		 * QSPI peripheral will use length of data specified in core registers
		 */
		if (xfer->cmd_length > 0) {
			joblist[job_idx++] = EVDMA_JOB(&packet->cmd, 4, EVDMA_PLAIN_DATA);
		}
		if (xfer->addr_length > 0) {
			joblist[job_idx++] = EVDMA_JOB(&packet->address, 4, EVDMA_PLAIN_DATA);
		}
		if (packet->num_bytes > 0) {
			joblist[job_idx++] = EVDMA_JOB(packet->data_buf, packet->num_bytes,
						       EVDMA_PLAIN_DATA);
		}
		/* Always terminate with null job */
		joblist[job_idx] = EVDMA_NULL_JOB();
		/* tx_job should point to first valid job, or null if none */
		if (job_idx > 0) {
			transfer_list->tx_job = &joblist[0];
		} else {
			transfer_list->tx_job = &joblist[job_idx];
		}

		/* rx_job always null for transmit */
		transfer_list->rx_job = &joblist[job_idx];
	} else {
		preg->CONFIG.RXTRANSFERLENGTH = ((packet->num_bytes + xfer->addr_length +
						xfer->cmd_length) >>
						dev_data->bytes_per_frame_exp) - 1;
		joblist[0] = EVDMA_JOB(packet->data_buf, packet->num_bytes, EVDMA_PLAIN_DATA);
		joblist[1] = EVDMA_NULL_JOB();
		transfer_list->tx_job = &joblist[1];
		transfer_list->rx_job = &joblist[0];
		/*
		 * In slave mode, a tmod register in the wrapper also needs to be set. Currently
		 * the address not in MDK so temp fix.
		 */
		uintptr_t addr = (uintptr_t)preg + QSPI_TMOD_OFFSET;

		sys_write32(QSPI_TMOD_RX_ONLY, addr);
	}

	preg->CONFIG.TXBURSTLENGTH = (config->tx_fifo_depth_minus_1+1)-config->dma_tx_data_level;
	preg->CONFIG.RXBURSTLENGTH = config->dma_rx_data_level+1;
	preg->DMA.CONFIG.LISTPTR = (uint32_t)transfer_list;
	preg->INTEN = BIT(QSPI_INTEN_DMADONE_Pos);

	return 0;
}

static inline void vendor_specific_free_dma_transfer_list(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;

	k_mem_slab_free(&dma_transfer_list_slab, dev_data->dma_transfer_list);
	dev_data->dma_transfer_list = NULL;
}

static inline bool vendor_specific_dma_accessible_check(const struct device *dev,
							uint8_t *data_buf)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	return nrf_dma_accessible_check(preg, data_buf);
}

static inline bool vendor_specific_read_dma_irq(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	return (bool) preg->EVENTS_DMA.DONE;
}

#else /* Supply empty vendor specific macros for generic case */
static inline void vendor_specific_init(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline void vendor_specific_suspend(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline void vendor_specific_resume(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline void vendor_specific_irq_clear(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline int vendor_specific_xip_enable(const struct device *dev,
					     const struct mspi_dev_id *dev_id,
					     const struct mspi_xip_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(cfg);

	return 0;
}
static inline int vendor_specific_xip_disable(const struct device *dev,
					      const struct mspi_dev_id *dev_id,
					      const struct mspi_xip_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(cfg);

	return 0;
}
#if defined(CONFIG_MSPI_DMA)
static inline void vendor_specific_enable_dma_irq(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline void vendor_specific_start_dma_xfer(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline int vendor_specific_setup_dma_xfer(const struct device *dev,
						 const struct mspi_xfer_packet *packet,
						 const struct mspi_xfer *xfer)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(packet);
	ARG_UNUSED(xfer);

	return 0;
}
static inline void vendor_specific_free_dma_transfer_list(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline bool vendor_specific_dma_accessible_check(const struct device *dev,
							uint8_t *data_buf) {
	ARG_UNUSED(dev);
	ARG_UNUSED(data_buf);

	return true;
}
static inline bool vendor_specific_read_dma_irq(const struct device *dev)
{
	ARG_UNUSED(dev);

	return true;
}
#endif /* defined(CONFIG_MSPI_DMA) */
#endif /* Empty vendor specific macros */
