/*
 * Copyright (c) 2017, Intel Corporation
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

#ifndef __QM_SS_SPI_H__
#define __QM_SS_SPI_H__

#include "qm_common.h"
#include "qm_sensor_regs.h"
/**
 * SPI peripheral driver for Sensor Subsystem.
 *
 * @defgroup groupSSSPI SS SPI
 * @{
 */

/**
 * QM Sensor SPI frame size type
 */
typedef enum {
	QM_SS_SPI_FRAME_SIZE_4_BIT = 3, /**< 4 bit frame. */
	QM_SS_SPI_FRAME_SIZE_5_BIT,     /**< 5 bit frame. */
	QM_SS_SPI_FRAME_SIZE_6_BIT,     /**< 6 bit frame. */
	QM_SS_SPI_FRAME_SIZE_7_BIT,     /**< 7 bit frame. */
	QM_SS_SPI_FRAME_SIZE_8_BIT,     /**< 8 bit frame. */
	QM_SS_SPI_FRAME_SIZE_9_BIT,     /**< 9 bit frame. */
	QM_SS_SPI_FRAME_SIZE_10_BIT,    /**< 10 bit frame. */
	QM_SS_SPI_FRAME_SIZE_11_BIT,    /**< 11 bit frame. */
	QM_SS_SPI_FRAME_SIZE_12_BIT,    /**< 12 bit frame. */
	QM_SS_SPI_FRAME_SIZE_13_BIT,    /**< 13 bit frame. */
	QM_SS_SPI_FRAME_SIZE_14_BIT,    /**< 14 bit frame. */
	QM_SS_SPI_FRAME_SIZE_15_BIT,    /**< 15 bit frame. */
	QM_SS_SPI_FRAME_SIZE_16_BIT,    /**< 16 bit frame. */
#if HAS_SS_SPI_32BIT
	QM_SS_SPI_FRAME_SIZE_17_BIT, /**< 17 bit frame. */
	QM_SS_SPI_FRAME_SIZE_18_BIT, /**< 18 bit frame. */
	QM_SS_SPI_FRAME_SIZE_19_BIT, /**< 19 bit frame. */
	QM_SS_SPI_FRAME_SIZE_20_BIT, /**< 20 bit frame. */
	QM_SS_SPI_FRAME_SIZE_21_BIT, /**< 21 bit frame. */
	QM_SS_SPI_FRAME_SIZE_22_BIT, /**< 22 bit frame. */
	QM_SS_SPI_FRAME_SIZE_23_BIT, /**< 23 bit frame. */
	QM_SS_SPI_FRAME_SIZE_24_BIT, /**< 24 bit frame. */
	QM_SS_SPI_FRAME_SIZE_25_BIT, /**< 25 bit frame. */
	QM_SS_SPI_FRAME_SIZE_26_BIT, /**< 26 bit frame. */
	QM_SS_SPI_FRAME_SIZE_27_BIT, /**< 27 bit frame. */
	QM_SS_SPI_FRAME_SIZE_28_BIT, /**< 28 bit frame. */
	QM_SS_SPI_FRAME_SIZE_29_BIT, /**< 29 bit frame. */
	QM_SS_SPI_FRAME_SIZE_30_BIT, /**< 30 bit frame. */
	QM_SS_SPI_FRAME_SIZE_31_BIT, /**< 31 bit frame. */
	QM_SS_SPI_FRAME_SIZE_32_BIT  /**< 32 bit frame. */
#endif				     /* HAS_SS_SPI_32BIT */
} qm_ss_spi_frame_size_t;

/**
 * SPI transfer mode type.
 */
typedef enum {
	/**
	 * Transmit & Receive mode.
	 *
	 * This mode synchronously receives and transmits data during the
	 * transfer. rx_len and tx_len buffer need to be the same length.
	 */
	QM_SS_SPI_TMOD_TX_RX,

	/**
	 * Transmit-Only mode.
	 *
	 * This mode only transmits data. The rx buffer is not accessed and
	 * rx_len need to be set to 0.
	 */
	QM_SS_SPI_TMOD_TX,

	/**
	 * Receive-Only mode.
	 *
	 * This mode only receives data. The tx buffer is not accessed and
	 * tx_len need to be set to 0.
	 */
	QM_SS_SPI_TMOD_RX,

	/**
	 * EEPROM-Read Mode.
	 *
	 * This mode transmits the data stored in the tx buffer (EEPROM
	 * address). After the transmit is completed it populates the rx buffer
	 * (EEPROM data) with received data.
	 */
	QM_SS_SPI_TMOD_EEPROM_READ
} qm_ss_spi_tmode_t;

/**
 * SPI Bus Mode Type
 */
typedef enum {
	QM_SS_SPI_BMODE_0 = 0, /**< Clock Polarity = 0, Clock Phase = 0. */
	QM_SS_SPI_BMODE_1,     /**< Clock Polarity = 0, Clock Phase = 1. */
	QM_SS_SPI_BMODE_2,     /**< Clock Polarity = 1, Clock Phase = 0. */
	QM_SS_SPI_BMODE_3      /**< Clock Polarity = 1, Clock Phase = 1. */
} qm_ss_spi_bmode_t;

/**
 * SPI Slave select type.
 *
 * Slave selects can be combined by logical OR.
 */
typedef enum {
	QM_SS_SPI_SS_DISABLED = 0, /**< Slave select disable. */
	QM_SS_SPI_SS_0 = BIT(0),   /**< Slave select 0. */
	QM_SS_SPI_SS_1 = BIT(1),   /**< Slave select 1. */
	QM_SS_SPI_SS_2 = BIT(2),   /**< Slave select 2. */
	QM_SS_SPI_SS_3 = BIT(3)    /**< Slave select 3. */
} qm_ss_spi_slave_select_t;

/**
 * SPI status
 */
typedef enum {
	QM_SS_SPI_IDLE,	/**< SPI device is not in use. */
	QM_SS_SPI_BUSY,	/**< SPI device is busy. */
	QM_SS_SPI_RX_OVERFLOW, /**< RX transfer has overflown. */
#if HAS_SS_SPI_VERBOSE_ERROR
	QM_SS_SPI_TX_OVERFLOW, /**< TX transfer has overflown. */
	QM_SS_SPI_RX_UNDERFLOW /**< RX transfer has underflown. */
#endif			       /* HAS_SS_SPI_VERBOSE_ERROR */
} qm_ss_spi_status_t;

/**
* SPI configuration type.
*/

typedef struct {
	qm_ss_spi_frame_size_t frame_size; /**< Frame Size */
	qm_ss_spi_tmode_t transfer_mode;   /**< Transfer mode (enum) */
	qm_ss_spi_bmode_t bus_mode;	/**< Bus mode (enum) */

	/**
	 * SCK = SPI_clock/clk_divider.
	 * A value of 0 will disable SCK. The LSB of this value is ignored.
	 */
	uint16_t clk_divider;
} qm_ss_spi_config_t;

/**
 * SPI asynchronous transfer type.
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

	/**
	 * Transfer callback.
	 *
	 * Called after all data is transmitted/received or if the driver
	 * detects an error during the SPI transfer.
	 *
	 * @param[in] data The callback user data.
	 * @param[in] error 0 on success.
	 *                  Negative @ref errno for possible error codes.
	 * @param[in] status The SPI module status.
	 * @param[in] len The amount of frames transmitted.
	 */
	void (*callback)(void *data, int error, qm_ss_spi_status_t status,
			 uint16_t len);
	void *callback_data; /**< Callback user data */
} qm_ss_spi_async_transfer_t;

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
} qm_ss_spi_transfer_t;

/**
 * Set SPI configuration.
 *
 * Change the configuration of a SPI module.
 * This includes transfer mode, bus mode, clock divider and data frame size.
 *
 * This operation is permitted only when the SPI module is disabled.
 *
 * @param [in] spi SPI module identifier.
 * @param [in] cfg New configuration for SPI. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_spi_set_config(const qm_ss_spi_t spi,
			 const qm_ss_spi_config_t *const cfg);

/**
 * Set Slave Select lines.
 *
 * Select which slaves to perform SPI transmissions on. Select lines can be
 * combined using the | operator. It is only suggested to use this functionality
 * in TX only mode. This operation is permitted only when a SPI transfer is not
 * already in progress; the caller should check that by retrieving the device
 * status.
 *
 * @param[in] spi SPI module identifier.
 * @param[in] ss Select lines to enable when performing transfers.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_spi_slave_select(const qm_ss_spi_t spi,
			   const qm_ss_spi_slave_select_t ss);

/**
 * Get SPI bus status.
 *
 * @param[in] spi SPI module identifier.
 * @param[out] status Reference to the variable where to store the current SPI
 *                    bus status (QM_SS_SPI_BUSY if a transfer is in progress
 *                    or QM_SS_SPI_IDLE if SPI device is IDLE).
 *                    This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_spi_get_status(const qm_ss_spi_t spi,
			 qm_ss_spi_status_t *const status);

/**
 * Perform a blocking SPI transfer.
 *
 * This is a blocking synchronous call. If transfer mode is full duplex
 * (QM_SS_SPI_TMOD_TX_RX) tx_len and rx_len must be equal. Similarly, for
 * transmit-only transfers (QM_SS_SPI_TMOD_TX) rx_len must be 0, while for
 * receive-only transfers (QM_SS_SPI_TMOD_RX) tx_len must be 0.
 *
 * @param[in] spi     SPI module identifier.
 * @param[in] xfer    Structure containing transfer information.
 *                    This must not be NULL.
 * @param[out] status Reference to the variable where to store the SPI status
 *                    at the end of the transfer.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_spi_transfer(const qm_ss_spi_t spi,
		       const qm_ss_spi_transfer_t *const xfer,
		       qm_ss_spi_status_t *const status);

/**
 * Initiate an interrupt based SPI transfer.
 *
 * Perform an interrupt based SPI transfer. If transfer mode is full duplex
 * (QM_SS_SPI_TMOD_TX_RX), then tx_len and rx_len must be equal. Similarly, for
 * transmit-only transfers (QM_SS_SPI_TMOD_TX) rx_len must be 0, while for
 * receive-only transfers (QM_SS_SPI_TMOD_RX) tx_len must be 0. This function is
 * non blocking.
 *
 * @param[in] spi SPI module identifier.
 * @param[in] xfer Structure containing transfer information.
 *                 The structure must not be NULL and must be kept valid until
 *                 the transfer is complete.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_spi_irq_transfer(const qm_ss_spi_t spi,
			   const qm_ss_spi_async_transfer_t *const xfer);

/**
 * Terminate SPI IRQ transfer.
 *
 * Terminate the current IRQ SPI transfer.
 * This function will trigger complete callbacks even
 * if the transfer is not completed.
 *
 * @param[in] spi SPI module identifier.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_spi_irq_transfer_terminate(const qm_ss_spi_t spi);

/**
 * Save SS SPI context.
 *
 * Saves the configuration of the specified SS SPI peripheral
 * before entering sleep.
 *
 * @param[in] spi SPI controller identifier.
 * @param[out] ctx SPI context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_spi_save_context(const qm_ss_spi_t spi,
			   qm_ss_spi_context_t *const ctx);

/**
 * Restore SS SPI context.
 *
 * Restore the configuration of the specified SS SPI peripheral
 * after exiting sleep.
 *
 * @param[in] spi SPI controller identifier.
 * @param[in] ctx SPI context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_spi_restore_context(const qm_ss_spi_t spi,
			      const qm_ss_spi_context_t *const ctx);

/**
 * @}
 */

#endif /* __QM_SS_SPI_H__ */
