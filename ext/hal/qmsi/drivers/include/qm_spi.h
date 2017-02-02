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
 * Slave selects can combined by logical OR if multiple slaves are selected
 * during one transfer. Setting only QM_SPI_SS_DISABLED prevents the controller
 * from starting the transfer.
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
	QM_SPI_IDLE,	/**< SPI device is not in use. */
	QM_SPI_BUSY,	/**< SPI device is busy. */
	QM_SPI_RX_OVERFLOW, /**< RX transfer has overflown. */
	QM_SPI_RX_FULL,     /**< Appl. Rx buffer full (slave only). */
	QM_SPI_TX_EMPTY     /**< Appl. Tx buffer empty (slave only) . */
} qm_spi_status_t;

/**
 * QM SPI Frame Format
 */
typedef enum {
	/**< Standard SPI mode */
	QM_SPI_FRAME_FORMAT_STANDARD = 0x0,
} qm_spi_frame_format_t;

/*
 * SPI update type
 *
 * Used by qm_spi_irq_update to know what to update, RX or TX.
 * Logical OR can be used in order to update both RX and TX.
 */
typedef enum {
	QM_SPI_UPDATE_RX = BIT(0), /* Update RX. */
	QM_SPI_UPDATE_TX = BIT(1), /* Update TX. */
} qm_spi_update_t;

/*
 * SPI configuration type.
 */
typedef struct {
	qm_spi_frame_size_t frame_size;     /**< Frame Size. */
	qm_spi_tmode_t transfer_mode;       /**< Transfer mode (enum). */
	qm_spi_bmode_t bus_mode;	    /**< Bus mode (enum). */
	qm_spi_frame_format_t frame_format; /* Data frame format for TX/RX */

	/**
	 * SCK = SPI_clock/clk_divider.
	 *
	 * A value of 0 will disable SCK.
	 */
	uint16_t clk_divider;
} qm_spi_config_t;

/**
 * SPI aynchronous transfer type.
 *
 * If the frame size is 8 bits or less, 1 byte is needed per data frame. If the
 * frame size is 9-16 bits, 2 bytes are needed per data frame and frames of more
 * than 16 bits require 4 bytes. In each case, the least significant bits are
 * sent while the extra bits are discarded. The most significant bits of the
 * frame are sent first.
 */
typedef struct {
	void *tx;	  /**< Write data. */
	void *rx;	  /**< Read data. */
	uint16_t tx_len;   /**< Number of data frames to write. */
	uint16_t rx_len;   /**< Number of data frames to read. */
	bool keep_enabled; /**< Keep device on once transfer is done. */

	/**
	 * Transfer callback.
	 *
	 * Called after all data is transmitted/received or if the driver
	 * detects an error during the SPI transfer.
	 * For slave device it also allows the application to update
	 * transfer information by calling the qm_spi_irq_update function.
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
 * SPI synchronous transfer type.
 *
 * If the frame size is 8 bits or less, 1 byte is needed per data frame. If the
 * frame size is 9-16 bits, 2 bytes are needed per data frame and frames of more
 * than 16 bits require 4 bytes. In each case, the least significant bits are
 * sent while the extra bits are discarded. The most significant bits of the
 * frame are sent first.
 */
typedef struct {
	void *tx;	/**< Write data. */
	void *rx;	/**< Read data. */
	uint16_t tx_len; /**< Number of data frames to write. */
	uint16_t rx_len; /**< Number of data frames to read. */
} qm_spi_transfer_t;

/**
 * Set SPI configuration.
 *
 * Change the configuration of a SPI module.
 * This includes transfer mode, bus mode, clock divider and data frame size.
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
 * Retrieve SPI bus status. Return QM_SPI_BUSY if transmitting data or SPI TX
 * FIFO not empty; QM_SPI_IDLE is available for transfer; QM_SPI_RX_OVERFLOW if
 * an RX overflow has occurred.
 *
 * The user may call this function before performing an SPI transfer in order to
 * guarantee that the SPI interface is available.
 *
 * @param[in] spi Which SPI to read the status of.
 * @param[out] status Current SPI status. This must not be null.
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
			volatile const qm_spi_async_transfer_t *const xfer);

/**
 * Update parameters of Interrupt based transfer on SPI.
 *
 * Allow the application to transmit and/or receive more data over the current
 * SPI communication.
 * The application is supposed to call this function only inside the registered
 * callback, once notified from the driver.
 * It is strongly recommended to use this function for slave-based applications
 * only, as slave controllers usually do not know how many frames an external
 * master will send or request before starting the communication.
 * Master controllers should not use this function as it will most likely
 * corrupt the transaction.
 *
 * @param[in] spi Which SPI to transfer to / from.
 * @param[in] xfer Transfer structure includes write / read buffers, length,
 *                 user callback function and the callback context data.
 *                 The structure must not be NULL and must be kept valid until
 *                 the transfer is complete.
 * @param[in] update Specify if only RX has to be updated, or only TX or both.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_irq_update(const qm_spi_t spi,
		      volatile const qm_spi_async_transfer_t *const xfer,
		      const qm_spi_update_t update);

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
 * This function configures the DMA source transfer width according to the
 * currently set SPI frame size. Therefore, whenever the SPI frame is updated
 * (using qm_spi_set_config) this function needs to be called again as the
 * previous source transfer width configuration is no longer valid. Note that if
 * the current frame size lies between 17 and 24 bits, this function fails
 * (returning -EINVAL) as the DMA core cannot handle 3-byte source width
 * transfers with buffers containing 1 padding byte between consecutive frames.
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
 * rx_len must be equal. Similarly, for transmit-only transfers (QM_SPI_TMOD_TX)
 * rx_len must be 0 while for receive-only transfers (QM_SPI_TMOD_RX) tx_len
 * must be 0. Transfer length is limited to 4KB.
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
 *                 buffers and callback functions. This must not be NULL and
 *                 must be kept valid until the transfer is complete.
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
 * Save SPI context.
 *
 * Saves the configuration of the specified SPI peripheral
 * before entering sleep.
 *
 * @param[in] spi SPI controller identifier.
 * @param[out] ctx SPI context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_save_context(const qm_spi_t spi, qm_spi_context_t *const ctx);

/**
 * Restore SPI context.
 *
 * Restore the configuration of the specified SPI peripheral
 * after exiting sleep.
 *
 * @param[in] spi SPI controller identifier.
 * @param[in] ctx SPI context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_spi_restore_context(const qm_spi_t spi,
			   const qm_spi_context_t *const ctx);

/**
 * @}
 */
#endif /* __QM_SPI_H__ */
