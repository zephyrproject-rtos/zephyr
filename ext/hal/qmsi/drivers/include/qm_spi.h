/*
 * Copyright (c) 2016, Intel Corporation
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
#include "qm_dma.h"

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
	QM_SPI_FRAME_SIZE_4_BIT = 3, /**< 4 bit frame. */
	QM_SPI_FRAME_SIZE_5_BIT,     /**< 5 bit frame. */
	QM_SPI_FRAME_SIZE_6_BIT,     /**< 6 bit frame. */
	QM_SPI_FRAME_SIZE_7_BIT,     /**< 7 bit frame. */
	QM_SPI_FRAME_SIZE_8_BIT,     /**< 8 bit frame. */
	QM_SPI_FRAME_SIZE_9_BIT,     /**< 9 bit frame. */
	QM_SPI_FRAME_SIZE_10_BIT,    /**< 10 bit frame. */
	QM_SPI_FRAME_SIZE_11_BIT,    /**< 11 bit frame. */
	QM_SPI_FRAME_SIZE_12_BIT,    /**< 12 bit frame. */
	QM_SPI_FRAME_SIZE_13_BIT,    /**< 13 bit frame. */
	QM_SPI_FRAME_SIZE_14_BIT,    /**< 14 bit frame. */
	QM_SPI_FRAME_SIZE_15_BIT,    /**< 15 bit frame. */
	QM_SPI_FRAME_SIZE_16_BIT,    /**< 16 bit frame. */
	QM_SPI_FRAME_SIZE_17_BIT,    /**< 17 bit frame. */
	QM_SPI_FRAME_SIZE_18_BIT,    /**< 18 bit frame. */
	QM_SPI_FRAME_SIZE_19_BIT,    /**< 19 bit frame. */
	QM_SPI_FRAME_SIZE_20_BIT,    /**< 20 bit frame. */
	QM_SPI_FRAME_SIZE_21_BIT,    /**< 21 bit frame. */
	QM_SPI_FRAME_SIZE_22_BIT,    /**< 22 bit frame. */
	QM_SPI_FRAME_SIZE_23_BIT,    /**< 23 bit frame. */
	QM_SPI_FRAME_SIZE_24_BIT,    /**< 24 bit frame. */
	QM_SPI_FRAME_SIZE_25_BIT,    /**< 25 bit frame. */
	QM_SPI_FRAME_SIZE_26_BIT,    /**< 26 bit frame. */
	QM_SPI_FRAME_SIZE_27_BIT,    /**< 27 bit frame. */
	QM_SPI_FRAME_SIZE_28_BIT,    /**< 28 bit frame. */
	QM_SPI_FRAME_SIZE_29_BIT,    /**< 29 bit frame. */
	QM_SPI_FRAME_SIZE_30_BIT,    /**< 30 bit frame. */
	QM_SPI_FRAME_SIZE_31_BIT,    /**< 31 bit frame. */
	QM_SPI_FRAME_SIZE_32_BIT     /**< 32 bit frame. */
} qm_spi_frame_size_t;

/**
 * SPI transfer mode type.
 */
typedef enum {
	QM_SPI_TMOD_TX_RX,      /**< Transmit & Receive. */
	QM_SPI_TMOD_TX,		/**< Transmit Only. */
	QM_SPI_TMOD_RX,		/**< Receive Only. */
	QM_SPI_TMOD_EEPROM_READ /**< EEPROM Read. */
} qm_spi_tmode_t;

/**
 * SPI bus mode type.
 */
typedef enum {
	QM_SPI_BMODE_0, /**< Clock Polarity = 0, Clock Phase = 0. */
	QM_SPI_BMODE_1, /**< Clock Polarity = 0, Clock Phase = 1. */
	QM_SPI_BMODE_2, /**< Clock Polarity = 1, Clock Phase = 0. */
	QM_SPI_BMODE_3  /**< Clock Polarity = 1, Clock Phase = 1. */
} qm_spi_bmode_t;

/**
 * SPI slave select type.
 *
 * QM_SPI_SS_DISABLED prevents the controller from starting a transfer.
 */
typedef enum {
	QM_SPI_SS_DISABLED = 0, /**< Slave select disable. */
	QM_SPI_SS_0 = BIT(0),   /**< Slave Select 0. */
	QM_SPI_SS_1 = BIT(1),   /**< Slave Select 1. */
	QM_SPI_SS_2 = BIT(2),   /**< Slave Select 2. */
	QM_SPI_SS_3 = BIT(3),   /**< Slave Select 3. */
} qm_spi_slave_select_t;

/**
 * SPI status
 */
typedef enum {
	QM_SPI_IDLE,       /**< SPI device is not in use. */
	QM_SPI_BUSY,       /**< SPI device is busy. */
	QM_SPI_RX_OVERFLOW /**< RX transfer has overflown. */
} qm_spi_status_t;

/**
 * SPI configuration type.
 */
typedef struct {
	qm_spi_frame_size_t frame_size; /**< Frame Size. */
	qm_spi_tmode_t transfer_mode;   /**< Transfer mode (enum). */
	qm_spi_bmode_t bus_mode;	/**< Bus mode (enum). */

	/**
	 * SCK = SPI_clock/clk_divider.
	 *
	 * A value of 0 will disable SCK.
	 */
	uint16_t clk_divider;
} qm_spi_config_t;

/**
 * SPI IRQ transfer type.
 */
typedef struct {
	uint8_t *tx;     /**< Write data. */
	uint16_t tx_len; /**< Write data Length. */
	uint8_t *rx;     /**< Read data. */
	uint16_t rx_len; /**< Read buffer length. */

	/**
	 * Transfer callback.
	 *
	 * Called after all data is transmitted/received or if the driver
	 * detects an error during the SPI transfer.
	 *
	 * @param[in] data The callback user data.
	 * @param[in] error 0 on success.
	 *            Negative @ref errno for possible error codes.
         * @param[in] status SPI driver status.
         * @param[in] len Length of the SPI transfer if successful, 0
	 * 	      otherwise.
	 */
	void (*callback)(void *data, int error, qm_spi_status_t status,
			 uint16_t len);
	void *callback_data; /**< Callback user data. */
} qm_spi_async_transfer_t;

/**
 * SPI transfer type.
 */
typedef struct {
	uint8_t *tx;     /**< Write Data. */
	uint16_t tx_len; /**< Write Data Length. */
	uint8_t *rx;     /**< Read Data. */
	uint16_t rx_len; /**< Receive Data Length. */
} qm_spi_transfer_t;

/**
 * Set SPI configuration.
 *
 * Change the configuration of a SPI module.
 * This includes transfer mode, bus mode and clock divider.
 *
 * @param[in] spi Which SPI module to configure.
 * @param[in] cfg New configuration for SPI. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_set_config(const qm_spi_t spi, const qm_spi_config_t *const cfg);

/**
 * Select which slave to perform SPI transmissions on.
 *
 * @param[in] spi Which SPI module to configure.
 * @param[in] ss Which slave select line to enable when doing transmissions.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_slave_select(const qm_spi_t spi, const qm_spi_slave_select_t ss);

/**
 * Get SPI bus status.
 *
 * Retrieve SPI bus status. Return QM_SPI_BUSY if transmitting data or data Tx
 * FIFO not empty.
 *
 * @param[in] spi Which SPI to read the status of.
 * @param[out] status Get spi status. This must not be null.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_get_status(const qm_spi_t spi, qm_spi_status_t *const status);

/**
 * Multi-frame read / write on SPI.
 *
 * Perform a multi-frame read/write on the SPI bus. This is a blocking
 * synchronous call. If the SPI is currently in use, the function will wait
 * until the SPI is free before beginning the transfer. If transfer mode is
 * full duplex (QM_SPI_TMOD_TX_RX), then tx_len and rx_len must be equal.
 * Similarly, for transmit-only transfers (QM_SPI_TMOD_TX) rx_len must be 0,
 * while for receive-only transfers (QM_SPI_TMOD_RX) tx_len must be 0.
 *
 * For starting a transfer, this controller demands at least one slave
 * select line (SS) to be enabled. Thus, a call to qm_spi_slave_select()
 * with one of the four SS valid lines is mandatory. This is true even if
 * the native slave select line is not used (i.e. when a GPIO is used to
 * drive the SS signal manually).
 *
 * @param[in] spi Which SPI to read/write on.
 * @param[in] xfer Structure containing pre-allocated write and read data
 *                 buffers. This must not be NULL.
 * @param[out] status Get spi status.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_transfer(const qm_spi_t spi, const qm_spi_transfer_t *const xfer,
		    qm_spi_status_t *const status);

/**
 * Interrupt based transfer on SPI.
 *
 * Perform an interrupt based transfer on the SPI bus. The function will
 * replenish/empty TX/RX FIFOs on SPI empty/full interrupts. If transfer
 * mode is full duplex (QM_SPI_TMOD_TX_RX), then tx_len and rx_len must be
 * equal. For transmit-only transfers (QM_SPI_TMOD_TX) rx_len must be 0
 * while for receive-only transfers (QM_SPI_TMOD_RX) tx_len must be 0.
 *
 * For starting a transfer, this controller demands at least one slave
 * select line (SS) to be enabled. Thus, a call to qm_spi_slave_select()
 * with one of the four SS valid lines is mandatory. This is true even if
 * the native slave select line is not used (i.e. when a GPIO is used to
 * drive the SS signal manually).
 *
 * @param[in] spi Which SPI to transfer to / from.
 * @param[in] xfer Transfer structure includes write / read buffers, length,
 *                 user callback function and the callback context data.
 *                 The structure must not be NULL and must be kept valid until
 *                 the transfer is complete.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_irq_transfer(const qm_spi_t spi,
			const qm_spi_async_transfer_t *const xfer);

/**
 * Configure a DMA channel with a specific transfer direction.
 *
 * The user is responsible for managing the allocation of the pool of DMA
 * channels provided by each DMA core to the different peripheral drivers
 * that require them.
 *
 * Note that a SPI controller cannot use different DMA cores to manage
 * transfers in different directions.
 *
 * This function configures DMA channel parameters that are unlikely to change
 * between transfers, like transaction width, burst size, and handshake
 * interface parameters. The user will likely only call this function once for
 * the lifetime of an application unless the channel needs to be repurposed.
 *
 * Note that qm_dma_init() must first be called before configuring a channel.
 *
 * @param[in] spi SPI controller identifier.
 * @param[in] dma_ctrl_id DMA controller identifier.
 * @param[in] dma_channel_id DMA channel identifier.
 * @param[in] dma_channel_direction DMA channel direction, either
 * QM_DMA_MEMORY_TO_PERIPHERAL (TX transfer) or QM_DMA_PERIPHERAL_TO_MEMORY
 * (RX transfer).
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_dma_channel_config(
    const qm_spi_t spi, const qm_dma_t dma_ctrl_id,
    const qm_dma_channel_id_t dma_channel_id,
    const qm_dma_channel_direction_t dma_channel_direction);

/**
 * Perform a DMA-based transfer on the SPI bus.
 *
 * If transfer mode is full duplex (QM_SPI_TMOD_TX_RX), then tx_len and
 * rx_len must be equal and neither of both callbacks can be NULL.
 * Similarly, for transmit-only transfers (QM_SPI_TMOD_TX) rx_len must be 0
 * and tx_callback cannot be NULL, while for receive-only transfers
 * (QM_SPI_TMOD_RX) tx_len must be 0 and rx_callback cannot be NULL.
 * Transfer length is limited to 4KB.
 *
 * For starting a transfer, this controller demands at least one slave
 * select line (SS) to be enabled. Thus, a call to qm_spi_slave_select()
 * with one of the four SS valid lines is mandatory. This is true even if
 * the native slave select line is not used (i.e. when a GPIO is used to
 * drive the SS signal manually).
 *
 * Note that qm_spi_dma_channel_config() must first be called in order to
 * configure all DMA channels needed for a transfer.
 *
 * @param[in] spi SPI controller identifier.
 * @param[in] xfer Structure containing pre-allocated write and read data
 *                 buffers and callback functions. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_dma_transfer(const qm_spi_t spi,
			const qm_spi_async_transfer_t *const xfer);

/**
 * Terminate SPI IRQ transfer.
 *
 * Terminate the current IRQ transfer on the SPI bus.
 * This will cause the user callback to be called with
 * error code set to -ECANCELED.
 *
 * @param[in] spi Which SPI to cancel the current transfer.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_irq_transfer_terminate(const qm_spi_t spi);

/**
 * Terminate the current DMA transfer on the SPI bus.
 *
 * Terminate the current DMA transfer on the SPI bus.
 * This will cause the relevant callbacks to be invoked.
 *
 * @param[in] spi SPI controller identifier.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_dma_transfer_terminate(const qm_spi_t spi);
/**
 * @}
 */
#endif /* __QM_SPI_H__ */
