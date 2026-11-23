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

#include <nrfx.h>

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

static inline void vendor_specific_dev_config(const struct device *dev,
					      const enum mspi_dev_cfg_mask param_mask,
					      const struct mspi_dev_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(param_mask);
	ARG_UNUSED(cfg);
}

static inline void vendor_specific_irq_clear(const struct device *dev)
{
	ARG_UNUSED(dev);

	NRF_EXMIF->EVENTS_CORE = 0;
}

#if defined(CONFIG_MSPI_MEMMAP)
static inline int vendor_specific_xip_enable(const struct device *dev,
					     const struct mspi_dev_id *dev_id,
					     const struct mspi_memmap_cfg *cfg)
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
					      const struct mspi_memmap_cfg *cfg)
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
#endif /* defined(CONFIG_MSPI_MEMMAP) */

#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_mspi)
#include <nrfx.h>

/* Elastic buffer support. Used to handle PVT variation on the MSPI PADs when
 * in Controller mode.
 */
/* Check whether an nrf_mspi peripheral uses elatstic buffer and is controller */
#define NRF_MSPI_EB_ENABLED(inst)					       \
	UTIL_AND(DT_INST_PROP_OR(inst, nordic_enable_elastic_buffer, 0),	       \
		 DT_INST_ENUM_HAS_VALUE(inst, op_mode,			       \
					mspi_op_mode_controller))
#define NRF_MSPI_EB_OR(inst) NRF_MSPI_EB_ENABLED(inst) ||
#define NRF_MSPI_EB_USED (DT_INST_FOREACH_STATUS_OKAY(NRF_MSPI_EB_OR) 0)

/* Check the correct PORT is being used */
#define NRF_MSPI_EB_ADDR(inst)						       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, nordic_hspadctrl),	       \
		(DT_REG_ADDR(DT_INST_PHANDLE(inst, nordic_hspadctrl))), (0))

#define NRF_MSPI_EB_SUPPORTED(inst)					       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, nordic_hspadctrl),	       \
		(DT_PROP(DT_INST_PHANDLE(inst, nordic_hspadctrl),	       \
			 hspadctrl_supported)), (0))

#define NRF_MSPI_EB_ASSERT_PSEL(node_id, prop, idx, port)		       \
	BUILD_ASSERT(NRF_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)) ==        \
		      NRF_PIN_DISCONNECTED ||				       \
		     NRF_GET_PORT(DT_PROP_BY_IDX(node_id, prop, idx)) == (port), \
		"nordic,enable-elastic-buffer needs every MSPI pin routed to the GPIO"\
		" port that nordic,hspadctrl points at");

#define NRF_MSPI_EB_ASSERT_PINS(inst)					       \
	IF_ENABLED(UTIL_AND(DT_INST_PINCTRL_HAS_IDX(inst, 0),		       \
			    DT_INST_NODE_HAS_PROP(inst, nordic_hspadctrl)),    \
		(DT_FOREACH_CHILD_VARGS(DT_INST_PHANDLE(inst, pinctrl_0),      \
					DT_FOREACH_PROP_ELEM_VARGS, psels,     \
					NRF_MSPI_EB_ASSERT_PSEL,		       \
					DT_PROP(DT_INST_PHANDLE(inst,	       \
						nordic_hspadctrl), port))))

#define NRF_MSPI_EB_DATA_INIT(inst)					       \
	IF_ENABLED(NRF_MSPI_EB_ENABLED(inst),				       \
		(.eb_regs = (NRF_GPIOHSPADCTRL_Type *)NRF_MSPI_EB_ADDR(inst),   \
		 .eb_index = DT_INST_PROP_OR(inst, nordic_hspadctrl_index, 0), \
		 .eb_rx_delay = DT_INST_PROP_OR(inst, nordic_eb_rx_delay, 0),))

#define NRF_MSPI_EB_ASSERTS(inst)					       \
	IF_ENABLED(NRF_MSPI_EB_ENABLED(inst), (				       \
		BUILD_ASSERT(NRF_MSPI_EB_SUPPORTED(inst),		       \
			"nordic,enable-elastic-buffer needs nordic,hspadctrl to point"\
			" at a GPIO port that has hspadctrl-supported");       \
		BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, nordic_hspadctrl_index)\
			     && DT_INST_NODE_HAS_PROP(inst, nordic_eb_rx_delay)\
			     && DT_INST_NODE_HAS_PROP(inst, rx_sample_delay_initial), \
			"nordic,enable-elastic-buffer needs nordic,hspadctrl-index,"  \
			" nordic,eb-rx-delay and rx-sample-delay-initial");    \
		NRF_MSPI_EB_ASSERT_PINS(inst)))

#if NRF_MSPI_EB_USED
/* Apply the elastic buffer configuration. */
static void nrf_mspi_eb_apply(const struct device *dev, bool check_delays);
#endif

static inline void vendor_specific_init(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_MSPI_Type *preg = (NRF_MSPI_Type *)config->wrapper_regs;

	preg->EVENTS_CORE = 0;
	preg->EVENTS_DMA.DONE = 0;

	preg->INTENSET = BIT(MSPI_INTENSET_CORE_Pos)
		       | BIT(MSPI_INTENSET_DMADONE_Pos);
}

static inline void vendor_specific_dev_config(const struct device *dev,
					      const enum mspi_dev_cfg_mask param_mask,
					      const struct mspi_dev_cfg *cfg)
{
	ARG_UNUSED(param_mask);
	ARG_UNUSED(cfg);

#if NRF_MSPI_EB_USED
	nrf_mspi_eb_apply(dev, true);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void vendor_specific_suspend(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_MSPI_Type *preg = (NRF_MSPI_Type *)config->wrapper_regs;

	preg->ENABLE = 0;
}

static inline void vendor_specific_resume(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_MSPI_Type *preg = (NRF_MSPI_Type *)config->wrapper_regs;

	preg->ENABLE = 1;

#if NRF_MSPI_EB_USED
	/* GPIOHSPADCTRL is a separate peripheral and pinctrl puts the pads into
	 * their sleep state while suspended, so the buffer is set up again
	 * rather than assumed to have kept its configuration.
	 */
	nrf_mspi_eb_apply(dev, false);
#endif
}

static inline void vendor_specific_irq_clear(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_MSPI_Type *preg = (NRF_MSPI_Type *)config->wrapper_regs;

	preg->EVENTS_CORE = 0;
	preg->EVENTS_DMA.DONE = 0;
}

#if defined(CONFIG_MSPI_MEMMAP)
static inline int vendor_specific_xip_enable(const struct device *dev,
					     const struct mspi_dev_id *dev_id,
					     const struct mspi_memmap_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(cfg);

	return 0;
}

static inline int vendor_specific_xip_disable(const struct device *dev,
					      const struct mspi_dev_id *dev_id,
					      const struct mspi_memmap_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(cfg);

	return 0;
}
#endif /* defined(CONFIG_MSPI_MEMMAP) */

#if defined(CONFIG_MSPI_DMA)
/* DMA support */
#define EVDMA_ATTR_ATTR_Pos (24UL)
#define EVDMA_ATTR_ATTR_Msk (0x3FUL << EVDMA_ATTR_ATTR_Pos)

typedef enum {
	EVDMA_BYTE_SWAP = 0,
	EVDMA_JOBLIST = 1,
	EVDMA_BUFFER_FILL = 2,
	EVDMA_FIXED_ATTR = 3,
	EVDMA_STATIC_ADDR = 4,
	EVDMA_PLAIN_DATA_BUF_WR = 5,
	EVDMA_PLAIN_DATA = 0x3f,
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
} MSPI_TRANSFER_LIST_Type;

/* Number of jobs needed for transmit transaction */
#define MAX_NUM_JOBS 5
#endif /* defined(CONFIG_MSPI_DMA) */

/* Vendor-specific data structure for Nordic MSPI */
typedef struct {
	/* NULL when this instance does not use the elastic buffer */
	NRF_GPIOHSPADCTRL_Type *eb_regs;
	uint8_t eb_index;
	uint8_t eb_rx_delay;
#if defined(CONFIG_MSPI_DMA)
	struct {
		MSPI_TRANSFER_LIST_Type *transfer_list;
		EVDMA_JOB_Type *joblist;
	};
#endif
} nordic_mspi_vendor_data_t;

/* Static allocation macros for vendor-specific data */
#define VENDOR_SPECIFIC_DATA_DEFINE(inst)				       \
	IF_ENABLED(CONFIG_MSPI_DMA,					       \
		(static MSPI_TRANSFER_LIST_Type mspi_dw_##inst##_transfer_list; \
		 static EVDMA_JOB_Type mspi_dw_##inst##_joblist[MAX_NUM_JOBS];))\
	NRF_MSPI_EB_ASSERTS(inst)					       \
	static const nordic_mspi_vendor_data_t mspi_dw_##inst##_vendor_data = { \
		IF_ENABLED(CONFIG_MSPI_DMA,				       \
			(.transfer_list = &mspi_dw_##inst##_transfer_list,      \
			 .joblist = &mspi_dw_##inst##_joblist[0],))	       \
		NRF_MSPI_EB_DATA_INIT(inst)				       \
	};

#define VENDOR_SPECIFIC_DATA_GET(inst) (void *)&mspi_dw_##inst##_vendor_data

#if NRF_MSPI_EB_USED
/* Apply the elastic buffer configuration. */
static void nrf_mspi_eb_apply(const struct device *dev, bool check_delays)
{
	const struct mspi_dw_config *config = dev->config;
	const nordic_mspi_vendor_data_t *vendor_data = config->vendor_specific_data;
	struct mspi_dw_data *dev_data = dev->data;
	uint32_t sck_phase;
	uint32_t ctrl;
	bool cpol;
	bool cpha;

	/* NULL unless the instance is a controller that uses the buffer */
	if (vendor_data->eb_regs == NULL) {
		return;
	}

	cpol = (dev_data->ctrlr0 & CTRLR0_SCPOL_BIT) != 0;
	cpha = (dev_data->ctrlr0 & CTRLR0_SCPH_BIT) != 0;

	/* The feedback clock is always used inverted, so the buffer takes the
	 * edge opposite to the one the configured polarity and phase sample on,
	 * which is the rising edge when the two agree. The polarity and phase
	 * themselves are programmed to match the core.
	 */
	sck_phase = (cpol == cpha) ? GPIOHSPADCTRL_CTRL_SCKPHASE_Falling
				   : GPIOHSPADCTRL_CTRL_SCKPHASE_Rising;

	ctrl = FIELD_PREP(GPIOHSPADCTRL_CTRL_RXDELAY_Msk, vendor_data->eb_rx_delay) |
	       FIELD_PREP(GPIOHSPADCTRL_CTRL_SCKPHASE_Msk, sck_phase) |
	       FIELD_PREP(GPIOHSPADCTRL_CTRL_OUTPUTCPOL_Msk, cpol) |
	       FIELD_PREP(GPIOHSPADCTRL_CTRL_OUTPUTCHPA_Msk, cpha) |
	       FIELD_PREP(GPIOHSPADCTRL_CTRL_CSNPOL_Msk, GPIOHSPADCTRL_CTRL_CSNPOL_LOW) |
	       FIELD_PREP(GPIOHSPADCTRL_CTRL_SCKFBPADEN_Msk, GPIOHSPADCTRL_CTRL_SCKFBPADEN_Enabled);

	vendor_data->eb_regs->CTRL[vendor_data->eb_index] = ctrl;

	/* DATAENABLE/SCKEN must be set in a separate AHB transaction */
	ctrl |= FIELD_PREP(GPIOHSPADCTRL_CTRL_DATAENABLE_Msk,
			   GPIOHSPADCTRL_CTRL_DATAENABLE_Enabled) |
		FIELD_PREP(GPIOHSPADCTRL_CTRL_SCKEN_Msk, GPIOHSPADCTRL_CTRL_SCKEN_Enabled);

	vendor_data->eb_regs->CTRL[vendor_data->eb_index] = ctrl;

	if (!check_delays) {
		return;
	}

	/* With CPHA set, the buffer only covers a round trip delay of up to half
	 * an SCK period. A full period needs CPOL and CPHA swapped relative to
	 * the core plus a wait cycle inserted, which is not implemented.
	 */
	if (cpha) {
		LOG_WRN("Elastic buffer with CPHA=1 currently only covers a round trip "
			"delay of up to half an SCK period");
	}

	/* The core has to sample after the buffer has released the data, which
	 * takes RXDELAY plus two cycles of fixed pipeline, but before the next
	 * SCK cycle overwrites it.
	 */
	if (dev_data->baudr != 0) {
		uint32_t rsd_min = vendor_data->eb_rx_delay + 3;
		uint32_t rsd_max = vendor_data->eb_rx_delay + 2 + dev_data->baudr;

		if (dev_data->rx_sample_dly < rsd_min || dev_data->rx_sample_dly > rsd_max) {
			LOG_WRN("RX sample delay %u outside [%u, %u] expected "
				"for elastic buffer delay %u",
				dev_data->rx_sample_dly, rsd_min, rsd_max,
				vendor_data->eb_rx_delay);
		}
	}
}
#endif /* NRF_MSPI_EB_USED */

#if defined(CONFIG_MSPI_DMA)
static inline void vendor_specific_start_dma_xfer(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *config = dev->config;
	const struct mspi_xfer_packet *packet = &dev_data->sub_pkt;

	NRF_MSPI_Type *preg = (NRF_MSPI_Type *)config->wrapper_regs;

	/* Use vendor-specific data from config - stores job and transfer lists */
	const nordic_mspi_vendor_data_t *vendor_data = (const nordic_mspi_vendor_data_t *)
						       config->vendor_specific_data;

	MSPI_TRANSFER_LIST_Type *transfer_list = vendor_data->transfer_list;
	EVDMA_JOB_Type *joblist = vendor_data->joblist;

	int job_idx = 0;

	/* Set up tx job pointer to the first job */
	transfer_list->tx_job = &joblist[0];

	/*
	 * The Command and Address will always have a length of 4 from the DMA's
	 * perspective. MSPI peripheral will use length of data specified in core registers.
	 * Since the cmd and address are stored as uint32_t, byte swap is never needed.
	 */
	if (dev_data->xfer.cmd_length > 0) {
		joblist[job_idx++] = EVDMA_JOB(&packet->cmd, 4, EVDMA_PLAIN_DATA);
	}
	if (dev_data->xfer.addr_length > 0) {
		joblist[job_idx++] = EVDMA_JOB(&packet->address, 4, EVDMA_PLAIN_DATA);
	}

	if (packet->dir == MSPI_TX) {
		preg->CONFIG.RXTRANSFERLENGTH = 0;

		if (packet->num_bytes > 0) {
			joblist[job_idx++] = EVDMA_JOB(packet->data_buf, packet->num_bytes,
						       EVDMA_PLAIN_DATA);
		}

		/* Always terminate with null job */
		joblist[job_idx] = EVDMA_NULL_JOB();
		/* rx_job is always EVDMA_NULL_JOB() for transmit */
		transfer_list->rx_job = &joblist[job_idx];
	} else {
		preg->CONFIG.RXTRANSFERLENGTH = ((packet->num_bytes) >>
						dev_data->bytes_per_frame_exp);

		/* If sending address or command while being configured as controller */
		if (job_idx > 0 && config->op_mode == MSPI_OP_MODE_CONTROLLER) {
			/* After command and address, setup RX job for data */
			joblist[job_idx++] = EVDMA_NULL_JOB();
			transfer_list->rx_job = &joblist[job_idx];
			joblist[job_idx++] = EVDMA_JOB(packet->data_buf, packet->num_bytes,
						       EVDMA_PLAIN_DATA);
			joblist[job_idx]   = EVDMA_NULL_JOB();
		} else {
			/* Sending command or address while configured as target isn't supported */
			transfer_list->rx_job = &joblist[0];
			joblist[0] = EVDMA_JOB(packet->data_buf, packet->num_bytes,
					       EVDMA_PLAIN_DATA);
			joblist[1] = EVDMA_NULL_JOB();
			transfer_list->tx_job = &joblist[1];
		}
	}

	/* The wrapper's TMOD register is only used in peripheral mode */
	if (config->op_mode == MSPI_OP_MODE_PERIPHERAL) {
		preg->TMOD = (packet->dir == MSPI_TX) ? MSPI_TMOD_TMOD_TXONLY
						      : MSPI_TMOD_TMOD_RXONLY;
	}

	/* The wrapper uses formatting registers regardless of whether it is driving a display
	 * or not in order to format the data when the amount of data is unaligned with 32-bits.
	 */

	preg->FORMAT.BPP = 8;
	/* Set to same value as core DFS register + 1 */
	preg->FORMAT.DFS = FIELD_GET(CTRLR0_DFS_MASK, dev_data->ctrlr0) + 1;
	/* Number of pixels following the command in units of BPP (which is always 8 for now) */
	preg->FORMAT.PIXELS = packet->num_bytes;
	/* Command and address length (in 32-bit words)*/
	preg->FORMAT.CILEN = CEIL_DIV_32(dev_data->xfer.addr_length) +
			     CEIL_DIV_32(dev_data->xfer.cmd_length);

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
	NRF_MSPI_Type *preg = (NRF_MSPI_Type *)config->wrapper_regs;

	return nrf_dma_accessible_check(preg, data_buf);
}

static inline bool vendor_specific_read_dma_irq(const struct device *dev)
{
	const struct mspi_dw_config *config = dev->config;
	NRF_MSPI_Type *preg = (NRF_MSPI_Type *)config->wrapper_regs;

	return (bool) preg->EVENTS_DMA.DONE;
}
#endif /*defined(CONFIG_MSPI_DMA)*/

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
static inline void vendor_specific_dev_config(const struct device *dev,
					      const enum mspi_dev_cfg_mask param_mask,
					      const struct mspi_dev_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(param_mask);
	ARG_UNUSED(cfg);
}
static inline void vendor_specific_irq_clear(const struct device *dev)
{
	ARG_UNUSED(dev);
}
static inline int vendor_specific_xip_enable(const struct device *dev,
					     const struct mspi_dev_id *dev_id,
					     const struct mspi_memmap_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(cfg);

	return 0;
}
static inline int vendor_specific_xip_disable(const struct device *dev,
					      const struct mspi_dev_id *dev_id,
					      const struct mspi_memmap_cfg *cfg)
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
