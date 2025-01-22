/*
 * Copyright 2018, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include "../spi_context.h"

#if CONFIG_NXP_LP_FLEXCOMM
#include <zephyr/drivers/mfd/nxp_lp_flexcomm.h>
#endif

#include <fsl_lpspi.h>

/* If any hardware revisions change this, make it into a DT property.
 * DONT'T make #ifdefs here by platform.
 */
#define LPSPI_CHIP_SELECT_COUNT   4
#define LPSPI_MIN_FRAME_SIZE_BITS 8

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct spi_mcux_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct spi_mcux_data *)(_dev)->data)

/* flag for SDK API for master transfers */
#define LPSPI_MASTER_XFER_CFG_FLAGS(slave)                                                         \
	kLPSPI_MasterPcsContinuous | (slave << LPSPI_MASTER_PCS_SHIFT)

struct spi_mcux_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	uint32_t pcs_sck_delay;
	uint32_t sck_pcs_delay;
	uint32_t transfer_delay;
	const struct pinctrl_dev_config *pincfg;
	lpspi_pin_config_t data_pin_config;
};

#ifdef CONFIG_SPI_MCUX_LPSPI_DMA
#include <zephyr/drivers/dma.h>

struct spi_dma_stream {
	const struct device *dma_dev;
	uint32_t channel; /* stores the channel for dma */
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
};
#endif /* CONFIG_SPI_MCUX_LPSPI_DMA */

struct spi_mcux_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	const struct device *dev;
	lpspi_master_handle_t handle;
	struct spi_context ctx;
	size_t transfer_len;
#ifdef CONFIG_SPI_RTIO
	struct spi_rtio *rtio_ctx;
#endif
#ifdef CONFIG_SPI_MCUX_LPSPI_DMA
	volatile uint32_t status_flags;
	struct spi_dma_stream dma_rx;
	struct spi_dma_stream dma_tx;
	/* dummy value used for transferring NOP when tx buf is null */
	uint32_t dummy_buffer;
#endif
};

/* common configure function that verifies spi_cfg validity and set up configuration parameters */
int spi_mcux_configure(const struct device *dev, const struct spi_config *spi_cfg);

/* Does these things:
 * Set data.dev
 * Check clocks device is ready
 * Configure cs gpio pin if needed
 * Mux pinctrl to lpspi
 * Enable LPSPI IRQ at system level
 */
int spi_nxp_init_common(const struct device *dev);

/* common api function for now */
int spi_mcux_release(const struct device *dev, const struct spi_config *spi_cfg);

/* Argument to MCUX SDK IRQ handler */
#define LPSPI_IRQ_HANDLE_ARG COND_CODE_1(CONFIG_NXP_LP_FLEXCOMM, (LPSPI_GetInstance(base)), (base))

#define SPI_MCUX_LPSPI_IRQ_FUNC_LP_FLEXCOMM(n)                                                     \
	nxp_lp_flexcomm_setirqhandler(DEVICE_DT_GET(DT_INST_PARENT(n)), DEVICE_DT_INST_GET(n),     \
				      LP_FLEXCOMM_PERIPH_LPSPI, lpspi_isr);

#define SPI_MCUX_LPSPI_IRQ_FUNC_DISTINCT(n)                                                        \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), lpspi_isr, DEVICE_DT_INST_GET(n),   \
		    0);                                                                            \
	irq_enable(DT_INST_IRQN(n));

#define SPI_MCUX_LPSPI_IRQ_FUNC(n) COND_CODE_1(DT_NODE_HAS_COMPAT(DT_INST_PARENT(n),		   \
								  nxp_lp_flexcomm),		   \
						(SPI_MCUX_LPSPI_IRQ_FUNC_LP_FLEXCOMM(n)),	   \
						(SPI_MCUX_LPSPI_IRQ_FUNC_DISTINCT(n)))

#define SPI_MCUX_LPSPI_CONFIG_INIT(n)                                                              \
	static const struct spi_mcux_config spi_mcux_config_##n = {                                \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.irq_config_func = spi_mcux_config_func_##n,                                       \
		.pcs_sck_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, pcs_sck_delay),                 \
					  DT_INST_PROP(n, pcs_sck_delay)),                         \
		.sck_pcs_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, sck_pcs_delay),                 \
					  DT_INST_PROP(n, sck_pcs_delay)),                         \
		.transfer_delay = UTIL_AND(DT_INST_NODE_HAS_PROP(n, transfer_delay),               \
					   DT_INST_PROP(n, transfer_delay)),                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.data_pin_config = DT_INST_ENUM_IDX(n, data_pin_config),                           \
	};

#define SPI_NXP_LPSPI_COMMON_INIT(n)                                                               \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void spi_mcux_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		SPI_MCUX_LPSPI_IRQ_FUNC(n)                                                         \
	}

#define SPI_NXP_LPSPI_COMMON_DATA_INIT(n)                                                          \
	SPI_CONTEXT_INIT_LOCK(spi_mcux_data_##n, ctx),                                             \
		SPI_CONTEXT_INIT_SYNC(spi_mcux_data_##n, ctx),                                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)

#define SPI_NXP_LPSPI_HAS_DMAS(n)                                                                  \
	UTIL_AND(DT_INST_DMAS_HAS_NAME(n, tx), DT_INST_DMAS_HAS_NAME(n, rx))
