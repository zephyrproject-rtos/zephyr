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

#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_qspi_v2)
#include <nrf.h>

static inline void vendor_specific_init(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	preg->EVENTS_CORE = 0;
	preg->EVENTS_DMA.DONE = 0;

	preg->INTENSET = BIT(QSPI_INTENSET_CORE_Pos)
		       | BIT(QSPI_INTENSET_DMADONE_Pos);
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
} EVDMA_ATTR_Type;

/* Setup EVDMA attribute with the following configuratrion */
#define EVDMA_ATTRIBUTE (BIT(EVDMA_BYTE_SWAP)   | BIT(EVDMA_JOBLIST)    | \
			 BIT(EVDMA_BUFFER_FILL) | BIT(EVDMA_FIXED_ATTR) | \
			 BIT(EVDMA_STATIC_ADDR) | BIT(EVDMA_PLAIN_DATA_BUF_WR))

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
#define MAX_NUM_JOBS 5

/* Vendor-specific data structure for Nordic QSPI */
typedef struct {
	QSPI_TRANSFER_LIST_Type *transfer_list;
	EVDMA_JOB_Type *joblist;
} nordic_qspi_vendor_data_t;

/* Static allocation macros for vendor-specific data */
#define VENDOR_SPECIFIC_DATA_DEFINE(inst) \
	static QSPI_TRANSFER_LIST_Type mspi_dw_##inst##_transfer_list; \
	static EVDMA_JOB_Type mspi_dw_##inst##_joblist[MAX_NUM_JOBS]; \
	static const nordic_qspi_vendor_data_t mspi_dw_##inst##_vendor_data = { \
		.transfer_list = &mspi_dw_##inst##_transfer_list, \
		.joblist = &mspi_dw_##inst##_joblist[0] \
	};

#define VENDOR_SPECIFIC_DATA_GET(inst) (void *)&mspi_dw_##inst##_vendor_data

/* Temporarily hard-coded as not in MDK yet */
#define QSPI_TMOD_OFFSET	(0x490UL)
#define QSPI_TMOD_TX_AND_RX	(0x0)
#define QSPI_TMOD_TX_ONLY	(0x1)
#define QSPI_TMOD_RX_ONLY	(0x2)
static inline void vendor_specific_start_dma_xfer(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *config = dev->config;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	NRF_QSPI_Type *preg = (NRF_QSPI_Type *)config->wrapper_regs;

	/* Use vendor-specific data from config - stores job and transfer lists */
	const nordic_qspi_vendor_data_t *vendor_data = (const nordic_qspi_vendor_data_t *)
						 config->vendor_specific_data;

	QSPI_TRANSFER_LIST_Type *transfer_list = vendor_data->transfer_list;
	EVDMA_JOB_Type *joblist = vendor_data->joblist;

	int tmod = 0;
	int job_idx = 0;

	/* Set up tx job pointer to the first job */
	transfer_list->tx_job = &joblist[0];

	/*
	 * The Command and Address will always have a length of 4 from the DMA's
	 * perspective. QSPI peripheral will use length of data specified in core registers
	 */
	if (dev_data->xfer.cmd_length > 0) {
		joblist[job_idx++] = EVDMA_JOB(&packet->cmd, 4, EVDMA_ATTRIBUTE);
	}
	if (dev_data->xfer.addr_length > 0) {
		joblist[job_idx++] = EVDMA_JOB(&packet->address, 4, EVDMA_ATTRIBUTE);
	}

	if (packet->dir == MSPI_TX) {
		preg->CONFIG.RXTRANSFERLENGTH = 0;

		if (packet->num_bytes > 0) {
			joblist[job_idx++] = EVDMA_JOB(packet->data_buf, packet->num_bytes,
						EVDMA_ATTRIBUTE);
		}

		/* Always terminate with null job */
		joblist[job_idx] = EVDMA_NULL_JOB();
		/* rx_job is always EVDMA_NULL_JOB() for transmit */
		transfer_list->rx_job = &joblist[job_idx];
		tmod = QSPI_TMOD_TX_ONLY;
	} else {
		preg->CONFIG.RXTRANSFERLENGTH = ((packet->num_bytes + dev_data->xfer.addr_length +
						dev_data->xfer.cmd_length) >>
						dev_data->bytes_per_frame_exp) - 1;

		/* If sending address or command while being configured as controller */
		if (job_idx > 0 && config->op_mode == MSPI_OP_MODE_CONTROLLER) {
			tmod = QSPI_TMOD_TX_AND_RX;

			/* After command and address, setup RX job for data */
			joblist[job_idx++] = EVDMA_NULL_JOB();
			transfer_list->rx_job = &joblist[job_idx];
			joblist[job_idx++] = EVDMA_JOB(packet->data_buf, packet->num_bytes,
						       EVDMA_ATTRIBUTE);
			joblist[job_idx]   = EVDMA_NULL_JOB();
		} else {
			/* Sending command or address while configured as target isn't supported */
			tmod = QSPI_TMOD_RX_ONLY;

			transfer_list->rx_job = &joblist[0];
			joblist[0] = EVDMA_JOB(packet->data_buf, packet->num_bytes,
					       EVDMA_ATTRIBUTE);
			joblist[1] = EVDMA_NULL_JOB();
			transfer_list->tx_job = &joblist[1];
		}
	}

	/*
	 * In slave mode, a tmod register in the wrapper also needs to be set. Currently
	 * the address not in MDK so this is a temporary fix.
	 */
	uintptr_t tmod_addr = (uintptr_t)preg + QSPI_TMOD_OFFSET;

	sys_write32(tmod, tmod_addr);

	preg->CONFIG.TXBURSTLENGTH = config->tx_fifo_depth_minus_1 + 1
				   - config->dma_tx_data_level;
	preg->CONFIG.RXBURSTLENGTH = config->dma_rx_data_level + 1;
	preg->DMA.CONFIG.LISTPTR = (uint32_t)transfer_list;

	preg->TASKS_START = 1;
}

static inline bool vendor_specific_dma_accessible_check(const struct device *dev,
							const uint8_t *data_buf)
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
static inline void vendor_specific_start_dma_xfer(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static inline bool vendor_specific_dma_accessible_check(const struct device *dev,
							const uint8_t *data_buf) {
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

/* Empty macros for generic case - no vendor-specific data */
#ifndef VENDOR_SPECIFIC_DATA_DEFINE
#define VENDOR_SPECIFIC_DATA_DEFINE(inst)
#endif

#ifndef VENDOR_SPECIFIC_DATA_GET
#define VENDOR_SPECIFIC_DATA_GET(inst) (void *)NULL
#endif
