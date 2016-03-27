/*
 * Copyright (c) 2015, Intel Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_SPI_H__
#define __QM_SPI_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * SPI peripheral driver for Quark Microcontrollers.
 *
 * @defgroup groupSPI SPI
 * @{
 */

/**
 * QM SPI frame size type.
 */
typedef enum {
	QM_SPI_FRAME_SIZE_4_BIT = 3, /* Min. size is 4 bits. */
	QM_SPI_FRAME_SIZE_5_BIT,
	QM_SPI_FRAME_SIZE_6_BIT,
	QM_SPI_FRAME_SIZE_7_BIT,
	QM_SPI_FRAME_SIZE_8_BIT,
	QM_SPI_FRAME_SIZE_9_BIT,
	QM_SPI_FRAME_SIZE_10_BIT,
	QM_SPI_FRAME_SIZE_11_BIT,
	QM_SPI_FRAME_SIZE_12_BIT,
	QM_SPI_FRAME_SIZE_13_BIT,
	QM_SPI_FRAME_SIZE_14_BIT,
	QM_SPI_FRAME_SIZE_15_BIT,
	QM_SPI_FRAME_SIZE_16_BIT,
	QM_SPI_FRAME_SIZE_17_BIT,
	QM_SPI_FRAME_SIZE_18_BIT,
	QM_SPI_FRAME_SIZE_19_BIT,
	QM_SPI_FRAME_SIZE_20_BIT,
	QM_SPI_FRAME_SIZE_21_BIT,
	QM_SPI_FRAME_SIZE_22_BIT,
	QM_SPI_FRAME_SIZE_23_BIT,
	QM_SPI_FRAME_SIZE_24_BIT,
	QM_SPI_FRAME_SIZE_25_BIT,
	QM_SPI_FRAME_SIZE_26_BIT,
	QM_SPI_FRAME_SIZE_27_BIT,
	QM_SPI_FRAME_SIZE_28_BIT,
	QM_SPI_FRAME_SIZE_29_BIT,
	QM_SPI_FRAME_SIZE_30_BIT,
	QM_SPI_FRAME_SIZE_31_BIT,
	QM_SPI_FRAME_SIZE_32_BIT
} qm_spi_frame_size_t;

/**
 * SPI transfer mode type.
 */
typedef enum {
	QM_SPI_TMOD_TX_RX,      /**< Transmit & Receive */
	QM_SPI_TMOD_TX,		/**< Transmit Only */
	QM_SPI_TMOD_RX,		/**< Receive Only */
	QM_SPI_TMOD_EEPROM_READ /**< EEPROM Read */
} qm_spi_tmode_t;

/**
 * SPI bus mode type.
 */
typedef enum {
	QM_SPI_BMODE_0, /**< Clock Polarity = 0, Clock Phase = 0 */
	QM_SPI_BMODE_1, /**< Clock Polarity = 0, Clock Phase = 1 */
	QM_SPI_BMODE_2, /**< Clock Polarity = 1, Clock Phase = 0 */
	QM_SPI_BMODE_3  /**< Clock Polarity = 1, Clock Phase = 1 */
} qm_spi_bmode_t;

/**
 * SPI slave select type.
 */
typedef enum {
	QM_SPI_SS_NONE = 0,
	QM_SPI_SS_0 = BIT(0),
	QM_SPI_SS_1 = BIT(1),
	QM_SPI_SS_2 = BIT(2),
	QM_SPI_SS_3 = BIT(3),
} qm_spi_slave_select_t;

typedef enum {
	QM_SPI_FREE,
	QM_SPI_BUSY,
	QM_SPI_TX_ERROR,
	QM_SPI_EINVAL
} qm_spi_status_t;

/**
 * SPI configuration type.
 */
typedef struct {
	qm_spi_frame_size_t frame_size; /**< Frame Size */
	qm_spi_tmode_t transfer_mode;   /**< Transfer mode (enum) */
	qm_spi_bmode_t bus_mode;	/**< Bus mode (enum) */
	uint16_t clk_divider; /**< SCK = SPI_clock/clk_divider. A value of 0
				 will disable SCK. */
} qm_spi_config_t;

/**
 * SPI IRQ transfer type.
 */
typedef struct {
	uint8_t *tx;     /* Write data */
	uint32_t tx_len; /* Write data Length */
	uint8_t *rx;     /* Read data */
	uint32_t rx_len; /* Read buffer length */
	/* Write callback */
	void (*tx_callback)(uint32_t id, uint32_t len);
	/* Read callback */
	void (*rx_callback)(uint32_t id, uint32_t len);
	/* Error callback */
	void (*err_callback)(uint32_t id, qm_rc_t status);
	uint32_t id; /* Callback identifier */
} qm_spi_async_transfer_t;

/**
 * SPI transfer type.
 */
typedef struct {
	uint8_t *tx;     /* Write Data */
	uint32_t tx_len; /* Write Data Length */
	uint8_t *rx;     /* Read Data */
	uint32_t rx_len; /* Receive Data Length */
} qm_spi_transfer_t;

/**
 * Change the configuration of a SPI module. This includes transfer mode, bus
 * mode and clock divider.
 *
 * @brief Set SPI configuration.
 * @param [in] spi Which SPI module to configure.
 * @param [in] cfg New configuration for SPI.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_spi_set_config(const qm_spi_t spi,
			  const qm_spi_config_t *const cfg);

/**
 * Get the current configuration of a SPI module. This includes transfer mode,
 * bus mode and clock divider.
 *
 * @brief Get SPI configuration.
 * @param [in] spi Which SPI module to read the configuration of.
 * @param [in] cfg Current configuration of SPI.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_spi_get_config(const qm_spi_t spi, qm_spi_config_t *const cfg);

/**
 * Select which slave to perform SPI transmissions on.
 *
 * @param [in] spi Which SPI module to configure.
 * @param [in] ss Which slave select line to enable when doing transmissions.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_spi_slave_select(const qm_spi_t spi,
			    const qm_spi_slave_select_t ss);

/**
 * Retrieve SPI bus status. Return QM_SPI_BUSY if transmitting data or data Tx
 * FIFO not empty.
 *
 * @brief Get SPI bus status.
 * @param [in] spi Which SPI to read the status of.
 * @return qm_spi_status_t Returns SPI specific return code.
 */
qm_spi_status_t qm_spi_get_status(const qm_spi_t spi);

/**
 * Perform a multi-frame read/write on the SPI bus. This is a blocking
 * synchronous call. If the SPI is currently in use, the function will wait
 * until the SPI is free before beginning the transfer. If transfer mode is
 * full duplex (QM_SPI_TMOD_TX_RX), then tx_len and rx_len must be equal.
 * Similarly, for transmit-only transfers (QM_SPI_TMOD_TX) rx_len must be 0,
 * while for receive-only transfers (QM_SPI_TMOD_RX) tx_len must be 0.
 *
 * @brief Multi-frame read / write on SPI.
 * @param [in] spi Which SPI to read/write on.
 * @param [in] xfer Structure containing pre-allocated write and read data
 * 				 buffers.
 * @return qm_rc_t Returns QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_spi_transfer(const qm_spi_t spi, qm_spi_transfer_t *const xfer);

/**
 * Perform an interrupt based transfer on the SPI bus. The function will
 * replenish/empty TX/RX FIFOs on SPI empty/full interrupts. If transfer
 * mode is full duplex (QM_SPI_TMOD_TX_RX), then tx_len and rx_len must be
 * equal and both callbacks cannot be null. Similarly, for transmit-only
 * transfers (QM_SPI_TMOD_TX) rx_len must be 0 and tx_callback cannot be null,
 * while for receive-only transfers (QM_SPI_TMOD_RX) tx_len must be 0 and
 * rx_callback cannot be null.
 *
 * @brief Interrupt based transfer on SPI.
 * @param [in] spi Which SPI to transfer to / from.
 * @param [in] xfer Transfer structure includes write / read data and length;
 * 		    write, read and error callback functions and a callback
 *		    identifier. This pointer must be kept valid until the
 *		    transfer is complete. The error callback cannot be null.
 * @return qm_rc_t Returns QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_spi_irq_transfer(const qm_spi_t spi,
			    qm_spi_async_transfer_t *const xfer);

/**
 * Interrupt service routine for the SPI masters
 *
 */
void qm_spi_master_0_isr(void);
#if (QUARK_SE)
void qm_spi_master_1_isr(void);
#endif /* QUARK_SE */

/**
 * Terminate the current IRQ or DMA transfer on the SPI bus.
 * This will cause the relevant callbacks to be called.
 *
 * @brief Terminate SPI IRQ/DMA transfer.
 * @param [in] spi Which SPI to cancel the current transfer.
 * @return qm_rc_t Returns QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_spi_transfer_terminate(const qm_spi_t spi);

/**
 * @}
 */
#endif /* __QM_SPI_H__ */
