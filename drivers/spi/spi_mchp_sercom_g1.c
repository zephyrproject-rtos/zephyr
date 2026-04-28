/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <mchp_dt_helper.h>

#define DT_DRV_COMPAT microchip_sercom_g1_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
LOG_MODULE_REGISTER(spi_mchp_sercom_g1);

#include "spi_context.h"

#define SPI_MCHP_MAX_XFER_SIZE  65535
#define SUPPORTED_SPI_WORD_SIZE 8
#define SPI_PIN_CNT             4
#define TIMEOUT_VALUE_US        1000
#define DELAY_US                2

struct mchp_spi_reg_config {
	sercom_registers_t *regs;
	uint32_t pads;
};

struct mchp_spi_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct mchp_spi_dma {
	const struct device *dma_dev;
	uint8_t tx_dma_request;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_request;
	uint8_t rx_dma_channel;
};

struct spi_mchp_dev_config {
	struct mchp_spi_reg_config reg_cfg;
	const struct pinctrl_dev_config *pcfg;

#if CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC
	struct mchp_spi_dma spi_dma;
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC */

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_MCHP_INTERRUPT_DRIVEN)
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_SPI_ASYNC) || CONFIG_SPI_MCHP_INTERRUPT_DRIVEN */

	struct mchp_spi_clock spi_clock;
};

struct spi_mchp_dev_data {
	struct spi_context ctx;

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_MCHP_INTERRUPT_DRIVEN)
	uint8_t dummysize;
#endif /* CONFIG_SPI_ASYNC) || CONFIG_SPI_MCHP_INTERRUPT_DRIVEN */

#if CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC
	const struct device *dev;
	uint32_t dma_segment_len;
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC */
};

/*Wait for synchronization*/
static inline void spi_wait_sync(const struct mchp_spi_reg_config *spi_reg_cfg, uint32_t sync_flag)
{
	if (WAIT_FOR(((spi_reg_cfg->regs->SPIM.SERCOM_SYNCBUSY & sync_flag) == 0), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for SPI SYNCBUSY ENABLE clear");
	}
}

/*Enable the SPI peripheral*/
static void spi_enable(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	spi_wait_sync(spi_reg_cfg, SERCOM_SPIM_SYNCBUSY_ENABLE_Msk);
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLA |= SERCOM_SPIM_CTRLA_ENABLE_Msk;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLA |= SERCOM_SPIS_CTRLA_ENABLE_Msk;
	}
	spi_wait_sync(spi_reg_cfg, SERCOM_SPIM_SYNCBUSY_ENABLE_Msk);
}

/*Disable the SPI peripheral*/
static void spi_disable(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_wait_sync(spi_reg_cfg, SERCOM_SPIM_SYNCBUSY_ENABLE_Msk);
	spi_reg_cfg->regs->SPIM.SERCOM_CTRLA &= ~SERCOM_SPIM_CTRLA_ENABLE_Msk;
	spi_wait_sync(spi_reg_cfg, SERCOM_SPIM_SYNCBUSY_ENABLE_Msk);
}

/*Set the SPI Master Mode*/
static inline void spi_master_mode(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	/* Clear the MODE bit field and set it to SPI Master mode */
	spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
		(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA & ~SERCOM_SPIM_CTRLA_MODE_Msk) |
		SERCOM_SPIM_CTRLA_MODE_SPI_MASTER;
}

/*Set the SPI Slave Mode*/
static inline void spi_slave_mode(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	/* Clear the MODE bit field and set it to SPI Slave mode */
	spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
		(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA & ~SERCOM_SPIS_CTRLA_MODE_Msk) |
		SERCOM_SPIS_CTRLA_MODE_SPI_SLAVE;
}

/*Set the SPI Data Order, MSB first*/
static void spi_msb_first(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	/* Clear the DORD bit field and set it to MSB first */
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA & ~SERCOM_SPIM_CTRLA_DORD_Msk) |
			SERCOM_SPIM_CTRLA_DORD_MSB;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA & ~SERCOM_SPIS_CTRLA_DORD_Msk) |
			SERCOM_SPIS_CTRLA_DORD_MSB;
	}
}

/*Set the SPI Data Order,LSB first*/
static void spi_lsb_first(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	/* Clear the DORD bit field and set it to LSB first */
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA & ~SERCOM_SPIM_CTRLA_DORD_Msk) |
			SERCOM_SPIM_CTRLA_DORD_LSB;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA & ~SERCOM_SPIS_CTRLA_DORD_Msk) |
			SERCOM_SPIS_CTRLA_DORD_LSB;
	}
}

/*Set the SPI Clock Polarity Idle Low*/
static void spi_cpol_idle_low(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	/* Clear the CPOL bit field and set clock polarity to Idle Low */
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA & ~SERCOM_SPIM_CTRLA_CPOL_Msk) |
			SERCOM_SPIM_CTRLA_CPOL_IDLE_LOW;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA & ~SERCOM_SPIS_CTRLA_CPOL_Msk) |
			SERCOM_SPIS_CTRLA_CPOL_IDLE_LOW;
	}
}

/*Set the SPI Clock Polarity Idle High*/
static void spi_cpol_idle_high(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	/* Clear the CPOL bit field and set clock polarity to Idle High */
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA & ~SERCOM_SPIM_CTRLA_CPOL_Msk) |
			SERCOM_SPIM_CTRLA_CPOL_IDLE_HIGH;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA & ~SERCOM_SPIS_CTRLA_CPOL_Msk) |
			SERCOM_SPIS_CTRLA_CPOL_IDLE_HIGH;
	}
}

/*Set the SPI Clock Phase leading Edge*/
static void spi_cpha_lead_edge(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	/* Clear the CPHA bit field and set clock phase to Leading Edge */
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA & ~SERCOM_SPIM_CTRLA_CPHA_Msk) |
			SERCOM_SPIM_CTRLA_CPHA_LEADING_EDGE;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA & ~SERCOM_SPIS_CTRLA_CPHA_Msk) |
			SERCOM_SPIS_CTRLA_CPHA_LEADING_EDGE;
	}
}

/*Set the SPI Clock Phase Trailing Edge*/
static void spi_cpha_trail_edge(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	/* Clear the CPHA bit field and set clock phase to Trailing Edge */
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA & ~SERCOM_SPIM_CTRLA_CPHA_Msk) |
			SERCOM_SPIM_CTRLA_CPHA_TRAILING_EDGE;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
			(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA & ~SERCOM_SPIS_CTRLA_CPHA_Msk) |
			SERCOM_SPIS_CTRLA_CPHA_TRAILING_EDGE;
	}
}

/*Set the SPI Half Duplex Mode*/
static inline int spi_half_duplex_mode(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	LOG_ERR("SPI half-duplex mode is not supported");

	return -ENOTSUP;
}

/*Set the SPI Full Duplex Mode. Since the device is full duplex mode by default this API returns
 *success
 */
static inline int spi_full_duplex_mode(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return 0;
}

/*Set the pads for the SPI Transmission*/
static inline void spi_slave_config_pinout(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	/* Clear the DIPO and DOPO bit fields and apply the new pad configuration */
	spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
		(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA &
		 ~(SERCOM_SPIS_CTRLA_DIPO_Msk | SERCOM_SPIS_CTRLA_DOPO_Msk)) |
		spi_reg_cfg->pads;
}

/*Set the pads for the SPI Transmission*/
static inline void spi_master_config_pinout(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	/* Clear the DIPO and DOPO bit fields and apply the new pad configuration */
	spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
		(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA &
		 ~(SERCOM_SPIM_CTRLA_DIPO_Msk | SERCOM_SPIM_CTRLA_DOPO_Msk)) |
		spi_reg_cfg->pads;
}

/*Set the pads for the SPI Transmission for loopback mode*/
static inline void spi_mode_loopback(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	/* Clear the DIPO and DOPO bit fields and set them to PAD0 */
	spi_reg_cfg->regs->SPIM.SERCOM_CTRLA =
		(spi_reg_cfg->regs->SPIM.SERCOM_CTRLA &
		 ~(SERCOM_SPIM_CTRLA_DIPO_Msk | SERCOM_SPIM_CTRLA_DOPO_Msk)) |
		(SERCOM_SPIM_CTRLA_DIPO_PAD0 | SERCOM_SPIM_CTRLA_DOPO_PAD0);
}

/*Enable the Receiver in SPI peripheral*/
static void spi_rx_enable(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_wait_sync(spi_reg_cfg, SERCOM_SPIM_SYNCBUSY_CTRLB_Msk);
		/* Clear the RXEN bit field and enable Receiver */
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLB =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLB & ~SERCOM_SPIM_CTRLB_RXEN_Msk) |
			SERCOM_SPIM_CTRLB_RXEN_Msk;
		spi_wait_sync(spi_reg_cfg, SERCOM_SPIM_SYNCBUSY_CTRLB_Msk);
	} else {
		spi_wait_sync(spi_reg_cfg, SERCOM_SPIS_SYNCBUSY_CTRLB_Msk);
		/* Clear the RXEN bit field and enable Receiver */
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLB =
			(spi_reg_cfg->regs->SPIS.SERCOM_CTRLB & ~SERCOM_SPIS_CTRLB_RXEN_Msk) |
			SERCOM_SPIS_CTRLB_RXEN_Msk;
		spi_wait_sync(spi_reg_cfg, SERCOM_SPIS_SYNCBUSY_CTRLB_Msk);
	}
}

/*Set the 8 BIT Character Size in SPI peripheral*/
static void spi_8bit_ch_size(const struct mchp_spi_reg_config *spi_reg_cfg, spi_operation_t op)
{
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		/* Clear the CHSIZE bit field and set character size to 8-bit */
		spi_reg_cfg->regs->SPIM.SERCOM_CTRLB =
			(spi_reg_cfg->regs->SPIM.SERCOM_CTRLB & ~SERCOM_SPIM_CTRLB_CHSIZE_Msk) |
			SERCOM_SPIM_CTRLB_CHSIZE_8_BIT;
	} else {
		/* Clear the CHSIZE bit field and set character size to 8-bit */
		spi_reg_cfg->regs->SPIS.SERCOM_CTRLB =
			(spi_reg_cfg->regs->SPIS.SERCOM_CTRLB & ~SERCOM_SPIS_CTRLB_CHSIZE_Msk) |
			SERCOM_SPIS_CTRLB_CHSIZE_8_BIT;
	}
}

/*Set the BAUD Rate value for SPI peripheral*/
static void spi_set_baudrate(const struct mchp_spi_reg_config *spi_reg_cfg,
			     const struct spi_config *config, uint32_t clk_freq_hz)
{
	uint32_t divisor = 2U * config->frequency;

	/* Use the requested or next highest possible frequency */
	uint32_t baud_value = (clk_freq_hz / divisor) - 1;

	if ((clk_freq_hz % divisor) >= (divisor / 2U)) {
		/* Round up the baud_value to ensures SPI clock is as close as possible to
		 * the requested frequency
		 */
		baud_value += 1U;
	}

	baud_value = CLAMP(baud_value, 0, UINT8_MAX);

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_BAUD = baud_value;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_BAUD = baud_value;
	}
}

/*Set the Inter character dpacing*/
static inline void spi_set_icspace(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIM.SERCOM_CTRLC |=
		SERCOM_SPIM_CTRLC_ICSPACE(CONFIG_SPI_MCHP_INTER_CHARACTER_SPACE);
}

/*Write Data into DATA register*/
static inline void spi_write_data(const struct mchp_spi_reg_config *spi_reg_cfg, uint8_t data)
{
	spi_reg_cfg->regs->SPIM.SERCOM_DATA = data;
}

/*Read Data from the SPI MASTER DATA register*/
static inline uint8_t spi_read_data(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return (uint8_t)spi_reg_cfg->regs->SPIM.SERCOM_DATA;
}

/*Read Data from the SPI SLAVE DATA register*/
static inline uint8_t spi_slave_read_data(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return (uint8_t)spi_reg_cfg->regs->SPIS.SERCOM_DATA;
}

/*Return true if receive complete flag is set*/
static inline bool spi_is_rx_comp(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return (spi_reg_cfg->regs->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_RXC_Msk) ==
	       SERCOM_SPIM_INTFLAG_RXC_Msk;
}

/*Return true if transmit complete flag is set*/
static inline bool spi_is_tx_comp(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return ((spi_reg_cfg->regs->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_TXC_Msk) ==
		SERCOM_SPIM_INTFLAG_TXC_Msk);
}

/*Clear the DATA register*/
static inline void spi_clr_data(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	/*Clear the DATA register until the RXC flag is cleared*/
	if (WAIT_FOR(((spi_reg_cfg->regs->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_RXC_Msk) == 0),
		     TIMEOUT_VALUE_US,
		     ((void)spi_reg_cfg->regs->SPIM.SERCOM_DATA, k_busy_wait(DELAY_US))) == false) {
		LOG_ERR("Timeout while clearing RXC");
	}
}

/*Return true if data register empty flag is set*/
static inline bool spi_is_data_empty(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return (spi_reg_cfg->regs->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_DRE_Msk) ==
	       SERCOM_SPIM_INTFLAG_DRE_Msk;
}

#if defined(CONFIG_SPI_MCHP_INTERRUPT_DRIVEN) || (CONFIG_SPI_MCHP_INTERRUPT_DRIVEN_ASYNC)
/*Enable the Receive Complete Interrupt*/
static void spi_enable_rxc_interrupt(const struct mchp_spi_reg_config *spi_reg_cfg,
				     spi_operation_t op)
{
	if (SPI_OP_MODE_GET(op) == SPI_OP_MODE_MASTER) {
		spi_reg_cfg->regs->SPIM.SERCOM_INTENSET = SERCOM_SPIM_INTENSET_RXC_Msk;
	} else {
		spi_reg_cfg->regs->SPIS.SERCOM_INTENSET = SERCOM_SPIS_INTENSET_RXC_Msk;
	}
}
#endif /* CONFIG_SPI_MCHP_INTERRUPT_DRIVEN || CONFIG_SPI_MCHP_INTERRUPT_DRIVEN_ASYNC */

/*Enable the Transmit Complete Interrupt*/
static inline void spi_enable_txc_interrupt(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIM.SERCOM_INTENSET = SERCOM_SPIM_INTENSET_TXC_Msk;
}

/*Enable the Data Register Empty Interrupt*/
static inline void spi_enable_data_empty_interrupt(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIM.SERCOM_INTENSET = SERCOM_SPIM_INTENSET_DRE_Msk;
}

/*Disable the Receive Complete Interrupt*/
static inline void spi_disable_rxc_interrupt(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIM.SERCOM_INTENCLR = SERCOM_SPIM_INTENCLR_RXC_Msk;
}

/*Disable the Transmit Complete Interrupt*/
static inline void spi_disable_txc_interrupt(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIM.SERCOM_INTENCLR = SERCOM_SPIM_INTENCLR_TXC_Msk;
}

/*Disable the Data Register Empty Interrupt*/
static inline void spi_disable_data_empty_interrupt(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIM.SERCOM_INTENCLR = SERCOM_SPIM_INTENCLR_DRE_Msk;
}

/* Enable the preload slave data*/
static inline void spi_slave_preload_enable(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_CTRLB =
		(spi_reg_cfg->regs->SPIS.SERCOM_CTRLB & ~SERCOM_SPIS_CTRLB_PLOADEN_Msk) |
		SERCOM_SPIS_CTRLB_PLOADEN_Msk;
}

/* Enable the slave select detection*/
static inline void spi_slave_select_low_enable(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_CTRLB =
		(spi_reg_cfg->regs->SPIS.SERCOM_CTRLB & ~SERCOM_SPIS_CTRLB_SSDE_Msk) |
		SERCOM_SPIS_CTRLB_SSDE_Msk;
}

/* Enable the Immediate buffer overflow*/
static inline void spi_immediate_buf_overflow(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_CTRLA =
		(spi_reg_cfg->regs->SPIS.SERCOM_CTRLA & ~SERCOM_SPIS_CTRLA_IBON_Msk) |
		SERCOM_SPIS_CTRLA_IBON_Msk;
}

/* Enable slave select line interrupt */
static inline void spi_slave_select_line_enable(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTENSET =
		(spi_reg_cfg->regs->SPIS.SERCOM_INTENSET & ~SERCOM_SPIS_INTENSET_SSL_Msk) |
		SERCOM_SPIS_INTENSET_SSL_Msk;
}

/* Return the slave select line status*/
static inline bool spi_slave_select_line(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return ((spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_SSL_Msk) ==
		SERCOM_SPIS_INTFLAG_SSL_Msk);
}

/* Clear the slave select line interrupt */
static inline void spi_slave_clr_slave_select_line(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG = SERCOM_SPIS_INTFLAG_SSL_Msk;
}

/* Clear buffer overflow flag */
static inline void spi_slave_clr_buf_overflow(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_STATUS = SERCOM_SPIS_STATUS_BUFOVF_Msk;
}

/* Set the Hardware slave select*/
static void spi_slave_select_enable(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_wait_sync(spi_reg_cfg, SERCOM_SPIM_SYNCBUSY_CTRLB_Msk);

	/* Clear the MSSEN bit field and enable Master Slave Select */
	spi_reg_cfg->regs->SPIM.SERCOM_CTRLB =
		(spi_reg_cfg->regs->SPIM.SERCOM_CTRLB & ~SERCOM_SPIM_CTRLB_MSSEN_Msk) |
		SERCOM_SPIM_CTRLB_MSSEN_Msk;
	spi_wait_sync(spi_reg_cfg, SERCOM_SPIM_SYNCBUSY_CTRLB_Msk);
}

/* Enable the Transmit Complete Interrupt */
static inline void spi_slave_enable_txc_interrupt(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTENSET = SERCOM_SPIS_INTENSET_TXC_Msk;
}

/* Clear the DATA register */
static inline void spi_slave_clr_data(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	if (WAIT_FOR(((spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_RXC_Msk) == 0),
		     TIMEOUT_VALUE_US,
		     ((void)spi_reg_cfg->regs->SPIM.SERCOM_DATA, k_busy_wait(DELAY_US))) == false) {
		LOG_ERR("Timeout while clearing RXC");
	}
}

/*Clear the Error Interrupt Flag */
static inline void spi_slave_clr_error_int_flag(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG = (uint8_t)SERCOM_SPIS_INTFLAG_ERROR_Msk;
}

/*Return true if receive complete flag is set*/
static inline bool spi_slave_is_rx_comp(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return (spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_RXC_Msk) ==
	       SERCOM_SPIS_INTFLAG_RXC_Msk;
}

/*Return true if data register empty flag is set*/
static inline bool spi_slave_is_data_empty(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return (spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_DRE_Msk) ==
	       SERCOM_SPIS_INTFLAG_DRE_Msk;
}

/*Return true if transmit complete flag is set*/
static inline bool spi_slave_is_tx_comp(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	return ((spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_TXC_Msk) ==
		SERCOM_SPIS_INTFLAG_TXC_Msk);
}

/*Write Data into DATA register*/
static inline void spi_slave_write_data(const struct mchp_spi_reg_config *spi_reg_cfg, uint8_t data)
{
	spi_reg_cfg->regs->SPIS.SERCOM_DATA = data;
}

/*Disable DRE interrupt*/
static inline void spi_slave_disable_dre_int(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTENCLR = (uint8_t)SERCOM_SPIS_INTENCLR_DRE_Msk;
}

/*Clear transmit complete flag is set*/
static inline void spi_slave_clr_tx_comp_flag(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG = SERCOM_SPIS_INTFLAG_TXC_Msk;
}

/*Disable all SPI Interrupt*/
static inline void spi_slave_disable_interrupts(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTENCLR = SERCOM_SPIS_INTENCLR_Msk;
}

/*Clear all SPI Interrupt*/
static inline void spi_slave_clr_interrupts(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG = SERCOM_SPIS_INTFLAG_Msk;
}

/*Enable the Data Register Empty Interrupt*/
static inline void
spi_slave_enable_data_empty_interrupt(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	spi_reg_cfg->regs->SPIS.SERCOM_INTENSET = SERCOM_SPIS_INTENSET_DRE_Msk;
}

static int spi_configure_pinout(const struct mchp_spi_reg_config *spi_reg_cfg,
				const struct spi_config *config)
{
	if ((config->operation & SPI_MODE_LOOP) != 0U) {
		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
			LOG_ERR("For slave Loopback mode is not supported");

			return -ENOTSUP;
		}
		spi_mode_loopback(spi_reg_cfg);
	} else {
		if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
			spi_slave_config_pinout(spi_reg_cfg);
		} else {
			spi_master_config_pinout(spi_reg_cfg);
		}
	}

	return 0;
}

static void spi_configure_cpol(const struct mchp_spi_reg_config *spi_reg_cfg,
			       const struct spi_config *config)
{
	if ((config->operation & SPI_MODE_CPOL) != 0U) {
		spi_cpol_idle_high(spi_reg_cfg, config->operation);
	} else {
		spi_cpol_idle_low(spi_reg_cfg, config->operation);
	}
}

static void spi_configure_cpha(const struct mchp_spi_reg_config *spi_reg_cfg,
			       const struct spi_config *config)
{
	if ((config->operation & SPI_MODE_CPHA) != 0U) {
		spi_cpha_trail_edge(spi_reg_cfg, config->operation);
	} else {
		spi_cpha_lead_edge(spi_reg_cfg, config->operation);
	}
}

static void spi_configure_bit_order(const struct mchp_spi_reg_config *spi_reg_cfg,
				    const struct spi_config *config)
{
	if ((config->operation & SPI_TRANSFER_LSB) != 0U) {
		spi_lsb_first(spi_reg_cfg, config->operation);
	} else {
		spi_msb_first(spi_reg_cfg, config->operation);
	}
}

static int spi_mchp_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *const data = dev->data;
	int retval;
	uint32_t clock_rate;
	bool has_cs = false;

	spi_disable(spi_reg_cfg);

	if (spi_context_configured(&data->ctx, config) == true) {
		spi_enable(spi_reg_cfg, config->operation);

		return 0;
	}

	/* Select the Character Size */
	if (SPI_WORD_SIZE_GET(config->operation) != SUPPORTED_SPI_WORD_SIZE) {
		LOG_ERR("Unsupported SPI word size: %d bits. Only 8-bit transfers are supported.",
			SPI_WORD_SIZE_GET(config->operation));

		return -ENOTSUP;
	}
	spi_8bit_ch_size(spi_reg_cfg, config->operation);

	spi_rx_enable(spi_reg_cfg, config->operation);

#if CONFIG_SPI_SLAVE
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {

		spi_slave_preload_enable(spi_reg_cfg);

		spi_slave_select_low_enable(spi_reg_cfg);

		spi_immediate_buf_overflow(spi_reg_cfg);

		spi_slave_mode(spi_reg_cfg);
	}
#endif /* CONFIG_SPI_SLAVE */

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {

		spi_set_icspace(spi_reg_cfg);

		clock_control_get_rate(cfg->spi_clock.clock_dev, cfg->spi_clock.gclk_sys,
				       &clock_rate);

		if ((config->frequency != 0) && (clock_rate >= (2 * config->frequency))) {
			spi_set_baudrate(spi_reg_cfg, config, clock_rate);
		} else {
			return -ENOTSUP;
		}

		spi_master_mode(spi_reg_cfg);

#if !DT_SPI_CTX_HAS_NO_CS_GPIOS
		has_cs = (data->ctx.num_cs_gpios != 0);
#endif

		if (has_cs) {
			retval = spi_context_cs_configure_all(&data->ctx);
			if (retval < 0) {
				return retval;
			}
		} else if (cfg->pcfg->states->pin_cnt == SPI_PIN_CNT) {
			spi_slave_select_enable(spi_reg_cfg);
		} else {
			/* Handled by user */
		}
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");

		return -ENOTSUP;
	}

	/*Set the Data out and Pin out Configuration*/
	retval = spi_configure_pinout(spi_reg_cfg, config);
	if (retval < 0) {
		return retval;
	}

	spi_configure_cpol(spi_reg_cfg, config);
	spi_configure_cpha(spi_reg_cfg, config);
	spi_configure_bit_order(spi_reg_cfg, config);

	if ((config->operation & SPI_HALF_DUPLEX) != 0U) {
		retval = spi_half_duplex_mode(spi_reg_cfg);
		if (retval != 0) {
			return retval;
		}
	} else {
		retval = spi_full_duplex_mode(spi_reg_cfg);
		if (retval != 0) {
			return -ENOTSUP;
		}
	}

	spi_enable(spi_reg_cfg, config->operation);

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_MCHP_INTERRUPT_DRIVEN)
	cfg->irq_config_func(dev);
#endif /* CONFIG_SPI_ASYNC || CONFIG_SPI_MCHP_INTERRUPT_DRIVEN */

#if CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC
	if (device_is_ready(cfg->spi_dma.dma_dev) != true) {
		return -ENODEV;
	}
	data->dev = dev;
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC */

	data->ctx.config = config;

	return 0;
}

static int spi_mchp_check_buf_len(const struct spi_buf_set *buf_set)
{
	if ((buf_set == NULL) || (buf_set->buffers == NULL)) {
		return 0;
	}

	for (size_t i = 0; i < buf_set->count; i++) {
		if (buf_set->buffers[i].len > SPI_MCHP_MAX_XFER_SIZE) {
			LOG_ERR("SPI buffer length (%u) exceeds max allowed (%u)",
				buf_set->buffers[i].len, SPI_MCHP_MAX_XFER_SIZE);

			return -EINVAL;
		}
	}

	return 0;
}

#ifndef CONFIG_SPI_MCHP_INTERRUPT_DRIVEN
static bool spi_mchp_transfer_in_progress(struct spi_mchp_dev_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static int spi_mchp_finish(const struct mchp_spi_reg_config *spi_reg_cfg)
{
	/* Wait until transmit complete */
	if (WAIT_FOR((spi_is_tx_comp(spi_reg_cfg) == true), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {

		LOG_ERR("SPI TX complete timeout");

		return -ETIMEDOUT;
	}
	spi_clr_data(spi_reg_cfg);

	return 0;
}

static int spi_mchp_poll_in(const struct mchp_spi_reg_config *spi_reg_cfg,
			    struct spi_mchp_dev_data *data)
{
	uint8_t tx_data;
	uint8_t rx_data;

	/* Check if there is data to transmit */
	if (spi_context_tx_buf_on(&data->ctx) == true) {
		tx_data = *data->ctx.tx_buf;
	} else {
		tx_data = 0U;
	}

	/* wait until the SPI data is empty */
	if (WAIT_FOR((spi_is_data_empty(spi_reg_cfg) == true), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("SPI data empty timeout");

		return -ETIMEDOUT;
	}

	spi_write_data(spi_reg_cfg, tx_data);

	spi_context_update_tx(&data->ctx, 1, 1);

	/* Wait for the reception to complete */
	while (spi_is_rx_comp(spi_reg_cfg) != true) {
		/* Wait for completion */
	};

	rx_data = spi_read_data(spi_reg_cfg);

	/* Check if there is a buffer to store received data */
	if (spi_context_rx_buf_on(&data->ctx) == true) {
		*data->ctx.rx_buf = rx_data;
	}

	spi_context_update_rx(&data->ctx, 1, 1);

	return 0;
}

static int spi_mchp_fast_tx(const struct mchp_spi_reg_config *spi_reg_cfg,
			    const struct spi_buf *tx_buf)
{
	const uint8_t *tx_data_ptr = tx_buf->buf;
	uint8_t tx_data;
	size_t len = tx_buf->len;
	uint8_t dummy_data = 0U;
	int retval;

	/* Transmit each byte in the buffer */
	while (len != 0) {
		if (tx_buf->buf != NULL) {
			tx_data = *tx_data_ptr++;
		} else {
			tx_data = dummy_data;
		}

		/* Wait until the tramist is complete */
		if (WAIT_FOR((spi_is_data_empty(spi_reg_cfg) == true), TIMEOUT_VALUE_US,
			     k_busy_wait(DELAY_US)) == false) {
			LOG_ERR("SPI data empty timeout");

			return -ETIMEDOUT;
		}

		spi_write_data(spi_reg_cfg, tx_data);
		len--;
	}

	retval = spi_mchp_finish(spi_reg_cfg);

	return retval;
}

static int spi_mchp_fast_rx(const struct mchp_spi_reg_config *spi_reg_cfg,
			    const struct spi_buf *rx_buf)
{
	uint8_t *rx_data_ptr = rx_buf->buf;
	size_t len = rx_buf->len;
	uint8_t dummy_data = 0U;
	int retval;

	if (len == 0) {
		return -EINVAL;
	}

	while (len != 0) {

		/* Write a dummy data to receive data */
		spi_write_data(spi_reg_cfg, dummy_data);
		len--;

		/* Wait for completion, and read */
		while (spi_is_rx_comp(spi_reg_cfg) != true) {
			/* Wait for completion */
		};

		if (rx_buf->buf != NULL) {
			*rx_data_ptr = spi_read_data(spi_reg_cfg);
			rx_data_ptr++;
		} else {
			(void)spi_read_data(spi_reg_cfg);
		}
	}

	retval = spi_mchp_finish(spi_reg_cfg);

	return retval;
}

static int spi_mchp_fast_txrx(const struct mchp_spi_reg_config *spi_reg_cfg,
			      const struct spi_buf *tx_buf, const struct spi_buf *rx_buf)
{
	const uint8_t *tx_data_ptr = tx_buf->buf;
	uint8_t *rx_data_ptr = rx_buf->buf;
	size_t len = rx_buf->len;
	uint8_t dummy_data = 0U;
	int retval;

	if (len == 0) {
		return -EINVAL;
	}

	while (len > 0) {
		/* Send the next byte */
		if (tx_data_ptr != NULL) {
			spi_write_data(spi_reg_cfg, *tx_data_ptr);
			tx_data_ptr++;
		} else {
			spi_write_data(spi_reg_cfg, dummy_data);
		}

		/* Wait for completion */
		while (spi_is_rx_comp(spi_reg_cfg) != true) {
			/* Wait for completion */
		};

		/* Read received data */
		if (rx_data_ptr != NULL) {
			*rx_data_ptr = spi_read_data(spi_reg_cfg);
			rx_data_ptr++;
		} else {
			(void)spi_read_data(spi_reg_cfg);
		}
		len--;
	}

	retval = spi_mchp_finish(spi_reg_cfg);

	return retval;
}

static int spi_mchp_fast_transceive(const struct device *dev, const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	size_t tx_count = 0;
	size_t rx_count = 0;
	const struct spi_buf *tx_data_ptr = NULL;
	const struct spi_buf *rx_data_ptr = NULL;
	int retval;

	if (tx_bufs != NULL) {
		tx_data_ptr = tx_bufs->buffers;
		tx_count = tx_bufs->count;
	}

	if (rx_bufs != NULL) {
		rx_data_ptr = rx_bufs->buffers;
		rx_count = rx_bufs->count;
	} else {
		rx_data_ptr = NULL;
	}

	while ((tx_count != 0) && (rx_count != 0)) {
		/* This function is called only if the count is equal*/
		retval = spi_mchp_fast_txrx(spi_reg_cfg, tx_data_ptr, rx_data_ptr);

		tx_data_ptr++;
		tx_count--;
		rx_data_ptr++;
		rx_count--;
	}

	/* Handle remaining transmit buffers */
	while (tx_count > 0) {
		retval = spi_mchp_fast_tx(spi_reg_cfg, tx_data_ptr);
		tx_data_ptr++;
		tx_count--;
	}

	/* Handle remaining receive buffers */
	while (rx_count > 0) {
		retval = spi_mchp_fast_rx(spi_reg_cfg, rx_data_ptr);
		rx_data_ptr++;
		rx_count--;
	}

	return retval;
}

static bool spi_mchp_is_same_len(const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	const struct spi_buf *tx_data_ptr = NULL;
	const struct spi_buf *rx_data_ptr = NULL;
	size_t tx_count = 0;
	size_t rx_count = 0;

	if (tx_bufs != NULL) {
		tx_data_ptr = tx_bufs->buffers;
		tx_count = tx_bufs->count;
	}

	if (rx_bufs != NULL) {
		rx_data_ptr = rx_bufs->buffers;
		rx_count = rx_bufs->count;
	}

	while (tx_count != 0 && rx_count != 0) {
		/* Compare the length of each corresponding TX and RX buffer */
		if (tx_data_ptr->len != rx_data_ptr->len) {
			return false;
		}

		tx_data_ptr++;
		tx_count--;
		rx_data_ptr++;
		rx_count--;
	}

	return true;
}
#endif /* CONFIG_SPI_MCHP_INTERRUPT_DRIVEN*/

#if defined(CONFIG_SPI_MCHP_INTERRUPT_DRIVEN) || (CONFIG_SPI_MCHP_INTERRUPT_DRIVEN_ASYNC)
static int spi_mchp_transceive_interrupt(const struct device *dev, const struct spi_config *config,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *const data = dev->data;
	uint8_t tx_data;

	/* Setup SPI buffers */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	/* Prepare first byte for transmission */
	if (spi_context_tx_buf_on(&data->ctx) == true) {
		tx_data = *data->ctx.tx_buf;
	} else {
		tx_data = 0U;
	}

	spi_clr_data(spi_reg_cfg);

	/* Get the dummysize */
	if ((data->ctx.rx_len) > (data->ctx.tx_len)) {
		data->dummysize = (data->ctx.rx_len) - (data->ctx.tx_len);
	}

	/* Write first data byte to the SPI data register */
	spi_context_update_tx(&data->ctx, 1, 1);
	spi_write_data(spi_reg_cfg, tx_data);

	/* Enable SPI interrupts for RX, TX completion, and data empty events */
	if (data->ctx.rx_len > 0) {
		spi_enable_rxc_interrupt(spi_reg_cfg, config->operation);
	} else {
		spi_enable_data_empty_interrupt(spi_reg_cfg);
	}

#if defined(CONFIG_SPI_MCHP_INTERRUPT_DRIVEN)
	spi_context_wait_for_completion(&data->ctx);
#endif /* CONFIG_SPI_MCHP_INTERRUPT_DRIVEN */

	return 0;
}

#if CONFIG_SPI_SLAVE
static void spi_mchp_slave_write(const struct device *dev)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	uint8_t tx_data;
	uint8_t dummy_data = 0U;
	struct spi_mchp_dev_data *const data = dev->data;
	bool write_ready;

	/* Prepare initial bytes for transmission */
	if (spi_context_tx_buf_on(&data->ctx) == true) {
		write_ready = spi_context_tx_buf_on(&data->ctx);
		write_ready = write_ready && (spi_slave_is_data_empty(spi_reg_cfg) == true);
		while (write_ready == true) {
			tx_data = *data->ctx.tx_buf;
			spi_slave_write_data(spi_reg_cfg, tx_data);

			/* Write data byte to the SPI data register */
			spi_context_update_tx(&data->ctx, 1, 1);
			write_ready = spi_context_tx_buf_on(&data->ctx);
			write_ready = write_ready && (spi_slave_is_data_empty(spi_reg_cfg) == true);
		}
	} else {
		write_ready = (spi_slave_is_data_empty(spi_reg_cfg));
		while (write_ready == true) {
			tx_data = dummy_data;
			spi_slave_write_data(spi_reg_cfg, tx_data);
			write_ready = (spi_slave_is_data_empty(spi_reg_cfg));
		}
	}
	spi_slave_enable_data_empty_interrupt(spi_reg_cfg);
}

static int spi_mchp_slave_transceive_interrupt(const struct device *dev,
					       const struct spi_config *config,
					       const struct spi_buf_set *tx_bufs,
					       const struct spi_buf_set *rx_bufs)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *const data = dev->data;
	int ret = 0;

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	if (spi_context_tx_on(&data->ctx) == true) {
		/* Prepare for transmission */
		spi_mchp_slave_write(dev);
	}

	spi_enable_rxc_interrupt(spi_reg_cfg, config->operation);

	spi_slave_select_line_enable(spi_reg_cfg);

#if defined(CONFIG_SPI_MCHP_INTERRUPT_DRIVEN)
	ret = spi_context_wait_for_completion(&data->ctx);
#endif /* CONFIG_SPI_MCHP_INTERRUPT_DRIVEN */

	return ret;
}

#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_MCHP_INTERRUPT_DRIVEN || (CONFIG_SPI_MCHP_INTERRUPT_DRIVEN_ASYNC */

static int spi_mchp_transceive_sync(const struct device *dev, const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *data = dev->data;
	int ret;

	ret = spi_mchp_check_buf_len(tx_bufs);
	if (ret < 0) {
		return ret;
	}

	ret = spi_mchp_check_buf_len(rx_bufs);
	if (ret < 0) {
		return ret;
	}

	ARG_UNUSED(spi_reg_cfg);

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_mchp_configure(dev, config);
	if (ret != 0) {
		spi_context_release(&data->ctx, ret);

		return ret;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		spi_context_cs_control(&data->ctx, true);
	}

#if CONFIG_SPI_MCHP_INTERRUPT_DRIVEN
#if CONFIG_SPI_SLAVE
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		ret = spi_mchp_slave_transceive_interrupt(dev, config, tx_bufs, rx_bufs);
	}
#endif /* CONFIG_SPI_SLAVE */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		ret = spi_mchp_transceive_interrupt(dev, config, tx_bufs, rx_bufs);
	}
#else
	/* Use optimized fast path if TX and RX buffer lengths match */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		if (spi_mchp_is_same_len(tx_bufs, rx_bufs) == true) {
			spi_mchp_fast_transceive(dev, config, tx_bufs, rx_bufs);
		} else {
			/* Setup SPI buffers and process using polling */
			spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

			do {
				ret = spi_mchp_poll_in(spi_reg_cfg, data);
			} while (spi_mchp_transfer_in_progress(data) && ret == 0);
		}
	}

#if CONFIG_SPI_SLAVE
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		spi_context_release(&data->ctx, ret);

		return -ENOTSUP;
	}
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_MCHP_INTERRUPT_DRIVEN */

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		spi_context_cs_control(&data->ctx, false);
	}

	spi_context_release(&data->ctx, ret);

	return ret;
}

#if CONFIG_SPI_ASYNC
#if CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC
static void spi_mchp_dma_rx_done(const struct device *dma_dev, void *arg, uint32_t id,
				 int error_code);

static int spi_mchp_dma_tx_load(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;

	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int retval;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->spi_dma.tx_dma_request;

	dma_blk.block_size = len;

	if (buf != NULL) {
		dma_blk.source_address = (uint32_t)buf;
	} else {
		static const uint8_t dummy_data;

		dma_blk.source_address = (uint32_t)&dummy_data;
		dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_blk.dest_address = (uint32_t)&spi_reg_cfg->regs->SPIM.SERCOM_DATA;
	dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(cfg->spi_dma.dma_dev, cfg->spi_dma.tx_dma_channel, &dma_cfg);

	if (retval != 0) {
		return retval;
	}

	retval = dma_start(cfg->spi_dma.dma_dev, cfg->spi_dma.tx_dma_channel);

	return retval;
}

static int spi_mchp_dma_rx_load(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *data = dev->data;

	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	int retval;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.user_data = data;
	dma_cfg.dma_callback = spi_mchp_dma_rx_done;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_cfg.dma_slot = cfg->spi_dma.rx_dma_request;

	dma_blk.block_size = len;

	if (buf != NULL) {
		dma_blk.dest_address = (uint32_t)buf;
	} else {
		/* Use a static dummy variable if no buffer is provided */
		static uint8_t dummy_data;

		dma_blk.dest_address = (uint32_t)&dummy_data;
		dma_blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	dma_blk.source_address = (uint32_t)&spi_reg_cfg->regs->SPIM.SERCOM_DATA;
	dma_blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	retval = dma_config(cfg->spi_dma.dma_dev, cfg->spi_dma.rx_dma_channel, &dma_cfg);
	if (retval != 0) {
		return retval;
	}

	retval = dma_start(cfg->spi_dma.dma_dev, cfg->spi_dma.rx_dma_channel);

	return retval;
}

static bool spi_mchp_dma_select_segment(const struct device *dev)
{
	struct spi_mchp_dev_data *data = dev->data;
	uint32_t segment_len;

	/* Pick the shorter buffer of ones that have an actual length */
	if (data->ctx.rx_len != 0) {
		segment_len = data->ctx.rx_len;
		if (data->ctx.tx_len != 0) {
			segment_len = MIN(segment_len, data->ctx.tx_len);
		}
	} else {
		segment_len = data->ctx.tx_len;
	}

	if (segment_len == 0) {
		return false;
	}

	/* Ensure the segment length does not exceed the max allowed value
	 */
	segment_len = MIN(segment_len, 65535);

	data->dma_segment_len = segment_len;

	return true;
}

static int spi_mchp_dma_setup_buffers(const struct device *dev)
{
	struct spi_mchp_dev_data *data = dev->data;
	int retval;

	if (data->dma_segment_len == 0) {
		return -EINVAL;
	}

	/* Load receive buffer first to prepare for incoming data */
	if (data->ctx.rx_len != 0U) {
		retval = spi_mchp_dma_rx_load(dev, data->ctx.rx_buf, data->dma_segment_len);
	} else {
		retval = spi_mchp_dma_rx_load(dev, NULL, data->dma_segment_len);
	}

	if (retval != 0) {
		return retval;
	}

	/* Load transmit buffer, which starts SPI bus clocking */
	if (data->ctx.tx_len != 0U) {
		retval = spi_mchp_dma_tx_load(dev, data->ctx.tx_buf, data->dma_segment_len);
	} else {
		retval = spi_mchp_dma_tx_load(dev, NULL, data->dma_segment_len);
	}

	if (retval != 0) {
		return retval;
	}

	return 0;
}

static void spi_mchp_dma_rx_done(const struct device *dma_dev, void *arg, uint32_t id,
				 int error_code)
{
	struct spi_mchp_dev_data *data = arg;
	const struct device *dev = data->dev;
	const struct spi_mchp_dev_config *cfg = dev->config;
	int retval;

	ARG_UNUSED(id);
	ARG_UNUSED(error_code);

	/* Update TX and RX context with the completed DMA segment */
	spi_context_update_tx(&data->ctx, 1, data->dma_segment_len);
	spi_context_update_rx(&data->ctx, 1, data->dma_segment_len);

	/* Check if more segments need to be transferred */
	if (spi_mchp_dma_select_segment(dev) == false) {
		if (spi_context_is_slave(&data->ctx) == false) {
			spi_context_cs_control(&data->ctx, false);
		}
		/* Transmission complete */
		spi_context_complete(&data->ctx, dev, 0);

		return;
	}

	/* Load the next DMA segment */
	retval = spi_mchp_dma_setup_buffers(dev);
	if (retval != 0) {
		/* Stop DMA and terminate the SPI transaction in case of failure */
		dma_stop(cfg->spi_dma.dma_dev, cfg->spi_dma.tx_dma_channel);
		dma_stop(cfg->spi_dma.dma_dev, cfg->spi_dma.rx_dma_channel);
		if (spi_context_is_slave(&data->ctx) == false) {
			spi_context_cs_control(&data->ctx, false);
		}
		spi_context_complete(&data->ctx, dev, retval);

		return;
	}
}
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC */

static int spi_mchp_transceive_async(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs, spi_callback_t spi_callback,
				     void *userdata)
{
	const struct spi_mchp_dev_config *cfg = dev->config;
	struct spi_mchp_dev_data *data = dev->data;
	int retval;

	retval = spi_mchp_check_buf_len(tx_bufs);
	if (retval < 0) {
		return retval;
	}

	retval = spi_mchp_check_buf_len(rx_bufs);
	if (retval < 0) {
		return retval;
	}

	ARG_UNUSED(cfg);

/*
 * Transmit clocks the output, and we use receive to
 * determine when the transmit is done, so we
 * always need both TX and RX DMA channels.
 */
#if CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC
	if (cfg->spi_dma.tx_dma_channel == 0xFF || cfg->spi_dma.rx_dma_channel == 0xFF) {
		return -ENOTSUP;
	}
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC */

	spi_context_lock(&data->ctx, true, spi_callback, userdata, config);

	retval = spi_mchp_configure(dev, config);
	if (retval != 0) {
		spi_context_release(&data->ctx, retval);

		return retval;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		spi_context_cs_control(&data->ctx, true);
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

/* Prepare and start DMA transfers */
#if CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC
	spi_mchp_dma_select_segment(dev);
	retval = spi_mchp_dma_setup_buffers(dev);
#else
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		retval = spi_mchp_transceive_interrupt(dev, config, tx_bufs, rx_bufs);
	}
#if CONFIG_SPI_SLAVE
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		retval = spi_mchp_slave_transceive_interrupt(dev, config, tx_bufs, rx_bufs);
	}
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC */

	if (retval != 0) {
		/* Stop DMA transfers in case of failure */
#if CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC
		dma_stop(cfg->spi_dma.dma_dev, cfg->spi_dma.tx_dma_channel);
		dma_stop(cfg->spi_dma.dma_dev, cfg->spi_dma.rx_dma_channel);
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC */

		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
			spi_context_cs_control(&data->ctx, false);
		}

		spi_context_release(&data->ctx, retval);
	}

	return retval;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_mchp_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_mchp_dev_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#if defined(CONFIG_SPI_ASYNC) || defined(CONFIG_SPI_MCHP_INTERRUPT_DRIVEN)
#if (CONFIG_SPI_SLAVE)
static void spi_mchp_isr_slave(const struct device *dev)
{
	struct spi_mchp_dev_data *data = dev->data;
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	uint8_t intFlag = spi_reg_cfg->regs->SPIS.SERCOM_INTFLAG;
	static bool transaction_complete;
	uint8_t tx_data = 0U;
	uint8_t rx_data = 0U;

	/* Reset transaction_complete if there is data to send or receive */
	if ((spi_context_tx_buf_on(&data->ctx) == true) ||
	    (spi_context_rx_buf_on(&data->ctx) == true)) {
		transaction_complete = false;
	}

	/* Check if data empty bit is set*/
	if (spi_slave_is_data_empty(spi_reg_cfg) == true) {
		tx_data = *data->ctx.tx_buf;
		if (spi_slave_is_tx_comp(spi_reg_cfg) == true) {
			intFlag = (uint8_t)SERCOM_SPIS_INTFLAG_TXC_Msk;
		}
		spi_slave_write_data(spi_reg_cfg, tx_data);
		if (spi_context_tx_on(&data->ctx) == true) {
			spi_context_update_tx(&data->ctx, 1, 1);
		} else {
			/* Disable DRE interrupt. The last byte sent by the master will be
			 * shifted out automatically
			 */
			spi_slave_disable_dre_int(spi_reg_cfg);
		}
	}

	/* Check if slave select bit is set*/
	if (spi_slave_select_line(spi_reg_cfg) == true) {
		spi_slave_clr_slave_select_line(spi_reg_cfg);
		spi_slave_enable_txc_interrupt(spi_reg_cfg);
	}

	/* Check if buffer overflow error bit is set*/
	if ((spi_reg_cfg->regs->SPIS.SERCOM_STATUS & SERCOM_SPIS_STATUS_BUFOVF_Msk) ==
	    SERCOM_SPIS_STATUS_BUFOVF_Msk) {
		spi_slave_clr_buf_overflow(spi_reg_cfg);
		spi_slave_clr_data(spi_reg_cfg);
		spi_slave_clr_error_int_flag(spi_reg_cfg);
	}

	/* Check if data is available in the receive buffer */
	if (spi_slave_is_rx_comp(spi_reg_cfg) == true) {
		rx_data = spi_slave_read_data(spi_reg_cfg);
		if (spi_context_rx_buf_on(&data->ctx) == true) {
			*data->ctx.rx_buf = rx_data;
			spi_context_update_rx(&data->ctx, 1, 1);
		}
	}

	/* If TX complete, finish transaction if all done */
	if ((intFlag & SERCOM_SPIS_INTFLAG_TXC_Msk) == SERCOM_SPIS_INTFLAG_TXC_Msk) {
		intFlag = 0;
		spi_slave_clr_tx_comp_flag(spi_reg_cfg);
		if ((spi_context_rx_on(&data->ctx) == false) &&
		    (spi_context_tx_on(&data->ctx) == false)) {
			spi_slave_disable_interrupts(spi_reg_cfg);
			spi_slave_clr_interrupts(spi_reg_cfg);
			/* Release the semaphore to unblock waiting threads */
			if (transaction_complete == false) {
				spi_context_complete(&data->ctx, dev, 0);
				transaction_complete = true;
			}
		}
	}
}
#endif /* CONFIG_SPI_SLAVE */

static void spi_mchp_isr_master(const struct device *dev)
{
	struct spi_mchp_dev_data *data = dev->data;
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	uint8_t dummy_data = 0U;
	bool last_byte = false;
	uint8_t tx_data = 0U;
	uint8_t rx_data = 0U;

	/* Check if the transmit buffer is empty and send the next byte */
	if (spi_reg_cfg->regs->SPIM.SERCOM_INTENSET == 0) {
		return;
	}
	/* Check if data is available in the receive buffer */
	if (spi_is_rx_comp(spi_reg_cfg) == true) {
		if (spi_context_rx_buf_on(&data->ctx) == true) {
			rx_data = spi_read_data(spi_reg_cfg);
			*data->ctx.rx_buf = rx_data;
			spi_context_update_rx(&data->ctx, 1, 1);
		}
	}
	/* If data register is empty, send next byte or dummy */
	if (spi_is_data_empty(spi_reg_cfg) == true) {
		spi_disable_data_empty_interrupt(spi_reg_cfg);
		if (spi_context_tx_on(&data->ctx) == true) {
			tx_data = *data->ctx.tx_buf;
			spi_write_data(spi_reg_cfg, tx_data);
			spi_context_update_tx(&data->ctx, 1, 1);
		} else if (data->dummysize > 0) {
			spi_write_data(spi_reg_cfg, dummy_data);
			data->dummysize--;
		} else {
			/* Do Nothing */
		}

		if ((data->dummysize == 0) && (spi_context_tx_on(&data->ctx) == false)) {
			last_byte = true;
		} else if (spi_context_rx_on(&data->ctx) == false) {
			spi_enable_data_empty_interrupt(spi_reg_cfg);
			spi_disable_rxc_interrupt(spi_reg_cfg);
		} else {
			/* Do Nothing */
		}
	}
	/* If TX complete and last byte, finish transaction */
	if ((spi_is_tx_comp(spi_reg_cfg) == true) && (last_byte == true)) {
		if (spi_context_rx_on(&data->ctx) == false) {
			spi_disable_rxc_interrupt(spi_reg_cfg);
			spi_disable_txc_interrupt(spi_reg_cfg);
			spi_disable_data_empty_interrupt(spi_reg_cfg);
			last_byte = false;
			if (spi_context_is_slave(&data->ctx) == false) {
				/* Control chip select for SPI slave mode */
				spi_context_cs_control(&data->ctx, false);
			}
			/* Release the semaphore to unblock waiting threads */
			spi_context_complete(&data->ctx, dev, 0);
		}
	}
	/* Enable TX complete interrupt if last byte */
	if (last_byte == true) {
		spi_enable_txc_interrupt(spi_reg_cfg);
	}
}

static void spi_mchp_isr(const struct device *dev)
{
#if CONFIG_SPI_SLAVE
	struct spi_mchp_dev_data *data = dev->data;

	if (spi_context_is_slave(&data->ctx) == true) {
		spi_mchp_isr_slave(dev);

		return;
	}
#endif /* CONFIG_SPI_SLAVE */

	spi_mchp_isr_master(dev);
}
#endif /* CONFIG_SPI_ASYNC || CONFIG_SPI_MCHP_INTERRUPT_DRIVEN */

static int spi_mchp_init(const struct device *dev)
{
	int retval;
	const struct spi_mchp_dev_config *cfg = dev->config;
	const struct mchp_spi_reg_config *spi_reg_cfg = &cfg->reg_cfg;
	struct spi_mchp_dev_data *const data = dev->data;

	retval = clock_control_on(cfg->spi_clock.clock_dev, cfg->spi_clock.gclk_sys);
	if ((retval < 0) && (retval != -EALREADY)) {
		LOG_ERR("Failed to enable the gclk_sys for SPI: %d", retval);

		return retval;
	}

	retval = clock_control_on(cfg->spi_clock.clock_dev, cfg->spi_clock.mclk_sys);
	if ((retval < 0) && (retval != -EALREADY)) {
		LOG_ERR("Failed to enable the mclk_sys for SPI: %d", retval);

		return retval;
	}

	/* Disable all SPI Interrupts*/
	spi_reg_cfg->regs->SPIM.SERCOM_INTENCLR = SERCOM_SPIM_INTENCLR_Msk;

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		LOG_ERR("pinctrl_apply_state Failed for SPI: %d", retval);

		return retval;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_mchp_api) = {
	.transceive = spi_mchp_transceive_sync,

#if CONFIG_SPI_ASYNC
	.transceive_async = spi_mchp_transceive_async,
#endif /*CONFIG_SPI_ASYNC*/

#if CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif /*CONFIG_SPI_RTIO*/

	.release = spi_mchp_release,
};

#define SPI_MCHP_SERCOM_PADS(n)                                                                    \
	SERCOM_SPIM_CTRLA_DIPO(DT_INST_PROP(n, dipo)) |                                            \
		SERCOM_SPIM_CTRLA_DOPO(DT_INST_PROP(n, dopo))

#define SPI_MCHP_REG_CFG_DEFN(n)                                                                   \
	.reg_cfg.regs = (sercom_registers_t *)DT_INST_REG_ADDR(n),                                 \
	.reg_cfg.pads = SPI_MCHP_SERCOM_PADS(n),

#if CONFIG_SPI_MCHP_INTERRUPT_DRIVEN || CONFIG_SPI_ASYNC
#if DT_INST_IRQ_HAS_IDX(0, 3)
#define SPI_MCHP_IRQ_HANDLER(n)                                                                    \
	static void spi_mchp_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		MCHP_SPI_IRQ_CONNECT(n, 0);                                                        \
		MCHP_SPI_IRQ_CONNECT(n, 1);                                                        \
		MCHP_SPI_IRQ_CONNECT(n, 2);                                                        \
		MCHP_SPI_IRQ_CONNECT(n, 3);                                                        \
	}
#else
#define SPI_MCHP_IRQ_HANDLER(n)                                                                    \
	static void spi_mchp_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		MCHP_SPI_IRQ_CONNECT(n, 0);                                                        \
	}
#endif
#else
#define SPI_MCHP_IRQ_HANDLER(n)
#endif /* CONFIG_SPI_MCHP_INTERRUPT_DRIVEN || CONFIG_SPI_ASYNC */

#define SPI_MCHP_CLOCK_DEFN(n)                                                                     \
	.spi_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                 \
	.spi_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),           \
	.spi_clock.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem))

#if CONFIG_SPI_MCHP_INTERRUPT_DRIVEN || CONFIG_SPI_ASYNC
#define MCHP_SPI_IRQ_CONNECT(n, m)                                                                 \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    spi_mchp_isr, DEVICE_DT_INST_GET(n), 0);                               \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#define SPI_MCHP_IRQ_HANDLER_DECL(n) static void spi_mchp_irq_config_##n(const struct device *dev)
#define SPI_MCHP_IRQ_HANDLER_FUNC(n) .irq_config_func = spi_mchp_irq_config_##n,
#else
#define SPI_MCHP_IRQ_HANDLER_DECL(n)
#define SPI_MCHP_IRQ_HANDLER_FUNC(n)
#endif /* CONFIG_SPI_MCHP_INTERRUPT_DRIVEN || CONFIG_SPI_ASYNC */

#if CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC
#define SPI_MCHP_DMA_CHANNELS(n)                                                                   \
	.spi_dma.dma_dev = DEVICE_DT_GET(MCHP_DT_INST_DMA_CTLR(n, tx)),                            \
	.spi_dma.tx_dma_request = MCHP_DT_INST_DMA_TRIGSRC(n, tx),                                 \
	.spi_dma.tx_dma_channel = MCHP_DT_INST_DMA_CHANNEL(n, tx),                                 \
	.spi_dma.rx_dma_request = MCHP_DT_INST_DMA_TRIGSRC(n, rx),                                 \
	.spi_dma.rx_dma_channel = MCHP_DT_INST_DMA_CHANNEL(n, rx),
#else
#define SPI_MCHP_DMA_CHANNELS(n)
#endif /* CONFIG_SPI_MCHP_DMA_DRIVEN_ASYNC */

#define SPI_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct spi_mchp_dev_config spi_mchp_config_##n = {                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		SPI_MCHP_REG_CFG_DEFN(n) SPI_MCHP_IRQ_HANDLER_FUNC(n) SPI_MCHP_DMA_CHANNELS(n)     \
			SPI_MCHP_CLOCK_DEFN(n)}

#define SPI_MCHP_DEVICE_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	SPI_MCHP_IRQ_HANDLER_DECL(n);                                                              \
	SPI_MCHP_CONFIG_DEFN(n);                                                                   \
	static struct spi_mchp_dev_data spi_mchp_data_##n = {                                      \
		SPI_CONTEXT_INIT_LOCK(spi_mchp_data_##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_mchp_data_##n, ctx),                                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
	DEVICE_DT_INST_DEFINE(n, spi_mchp_init, NULL, &spi_mchp_data_##n, &spi_mchp_config_##n,    \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_mchp_api);               \
	SPI_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(SPI_MCHP_DEVICE_INIT)
