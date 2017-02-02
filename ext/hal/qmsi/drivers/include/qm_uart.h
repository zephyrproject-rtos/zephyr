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

#ifndef __QM_UART_H__
#define __QM_UART_H__

#include "qm_common.h"
#include "qm_soc_regs.h"
#include "qm_dma.h"

/**
 * UART peripheral driver.
 *
 * @defgroup groupUART UART
 * @{
 */

/**
 * UART Line control.
 */
typedef enum {
	QM_UART_LC_5N1 = 0x00,   /**< 5 data bits, no parity, 1 stop bit. */
	QM_UART_LC_5N1_5 = 0x04, /**< 5 data bits, no parity, 1.5 stop bits. */
	QM_UART_LC_5E1 = 0x18,   /**< 5 data bits, even parity, 1 stop bit. */
	QM_UART_LC_5E1_5 = 0x1c, /**< 5 data bits, even par., 1.5 stop bits. */
	QM_UART_LC_5O1 = 0x08,   /**< 5 data bits, odd parity, 1 stop bit. */
	QM_UART_LC_5O1_5 = 0x0c, /**< 5 data bits, odd parity, 1.5 stop bits. */
	QM_UART_LC_6N1 = 0x01,   /**< 6 data bits, no parity, 1 stop bit. */
	QM_UART_LC_6N2 = 0x05,   /**< 6 data bits, no parity, 2 stop bits. */
	QM_UART_LC_6E1 = 0x19,   /**< 6 data bits, even parity, 1 stop bit. */
	QM_UART_LC_6E2 = 0x1d,   /**< 6 data bits, even parity, 2 stop bits. */
	QM_UART_LC_6O1 = 0x09,   /**< 6 data bits, odd parity, 1 stop bit. */
	QM_UART_LC_6O2 = 0x0d,   /**< 6 data bits, odd parity, 2 stop bits. */
	QM_UART_LC_7N1 = 0x02,   /**< 7 data bits, no parity, 1 stop bit. */
	QM_UART_LC_7N2 = 0x06,   /**< 7 data bits, no parity, 2 stop bits. */
	QM_UART_LC_7E1 = 0x1a,   /**< 7 data bits, even parity, 1 stop bit. */
	QM_UART_LC_7E2 = 0x1e,   /**< 7 data bits, even parity, 2 stop bits. */
	QM_UART_LC_7O1 = 0x0a,   /**< 7 data bits, odd parity, 1 stop bit. */
	QM_UART_LC_7O2 = 0x0e,   /**< 7 data bits, odd parity, 2 stop bits. */
	QM_UART_LC_8N1 = 0x03,   /**< 8 data bits, no parity, 1 stop bit. */
	QM_UART_LC_8N2 = 0x07,   /**< 8 data bits, no parity, 2 stop bits. */
	QM_UART_LC_8E1 = 0x1b,   /**< 8 data bits, even parity, 1 stop bit. */
	QM_UART_LC_8E2 = 0x1f,   /**< 8 data bits, even parity, 2 stop bits. */
	QM_UART_LC_8O1 = 0x0b,   /**< 8 data bits, odd parity, 1 stop bit. */
	QM_UART_LC_8O2 = 0x0f    /**< 8 data bits, odd parity, 2 stop bits. */
} qm_uart_lc_t;

/**
 * UART Status type.
 */
typedef enum {
	QM_UART_IDLE = 0,	   /**< IDLE. */
	QM_UART_RX_OE = BIT(1),     /**< Receiver overrun. */
	QM_UART_RX_PE = BIT(2),     /**< Parity error. */
	QM_UART_RX_FE = BIT(3),     /**< Framing error. */
	QM_UART_RX_BI = BIT(4),     /**< Break interrupt. */
	QM_UART_TX_BUSY = BIT(5),   /**< TX Busy flag. */
	QM_UART_RX_BUSY = BIT(6),   /**< RX Busy flag. */
	QM_UART_TX_NFULL = BIT(7),  /**< TX FIFO not full. */
	QM_UART_RX_NEMPTY = BIT(8), /**< RX FIFO not empty. */
} qm_uart_status_t;

/**
 * UART configuration structure type
 */
typedef struct {
	qm_uart_lc_t line_control; /**< Line control (enum). */
	uint32_t baud_divisor;     /**< Baud Divisor. */
	bool hw_fc;		   /**< Hardware Automatic Flow Control. */
} qm_uart_config_t;

/**
 * UART asynchronous transfer structure.
 */
typedef struct {
	uint8_t *data;     /**< Pre-allocated write or read buffer. */
	uint32_t data_len; /**< Number of bytes to transfer. */

	/** Transfer callback
	 *
	 * @param[in] data Callback user data.
	 * @param[in] error 0 on success.
	 *                  Negative @ref errno for possible error codes.
	 * @param[in] status UART module status
	 * @param[in] len Length of the UART transfer if successful, 0
	 * otherwise.
	 */
	void (*callback)(void *data, int error, qm_uart_status_t status,
			 uint32_t len);
	void *callback_data; /**< Callback identifier. */
} qm_uart_transfer_t;

/**
 * Set UART configuration.
 *
 * Change the configuration of a UART module. This includes line control,
 * baud rate and hardware flow control.
 *
 * @param[in] uart Which UART module to configure.
 * @param[in] cfg New configuration for UART. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_set_config(const qm_uart_t uart, const qm_uart_config_t *const cfg);

/**
 * Get UART bus status.
 *
 * Retrieve UART interface status. Return QM_UART_BUSY if transmitting
 * data; QM_UART_IDLE if available for transfer; QM_UART_TX_ERROR if an
 * error has occurred in transmission.
 *
 * The user may call this function before performing an UART transfer in order
 * to guarantee that the UART interface is available.

 * @param[in] uart Which UART to read the status of.
 * @param[out] status Current UART status. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_get_status(const qm_uart_t uart, qm_uart_status_t *const status);

/**
 * UART character data write.
 *
 * Perform a single character write on the UART interface.
 * This is a blocking synchronous call.
 *
 * @param[in] uart UART identifer.
 * @param[in] data Data to write to UART.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_write(const qm_uart_t uart, const uint8_t data);

/**
 * UART character data read.
 *
 * Perform a single character read from the UART interface.
 * This is a blocking synchronous call.
 *
 * @param[in] uart UART identifer.
 * @param[out] data Data to read from UART. This must not be NULL.
 * @param[out] status UART specific status.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_read(const qm_uart_t uart, uint8_t *const data,
		 qm_uart_status_t *const status);

/**
 * UART character data write.
 *
 * Perform a single character write on the UART interface.
 * This is a non-blocking synchronous call.
 *
 * @param[in] uart UART identifier.
 * @param[in] data Data to write to UART.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_write_non_block(const qm_uart_t uart, const uint8_t data);

/**
 * UART character data read.
 *
 * Perform a single character read from the UART interface.
 * This is a non-blocking synchronous call.
 *
 * @param[in] uart UART identifer.
 * @param[out] data Character read. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_read_non_block(const qm_uart_t uart, uint8_t *const data);

/**
 * UART multi-byte data write.
 *
 * Perform a write on the UART interface. This is a blocking
 * synchronous call. The function will block until all data has
 * been transferred.
 *
 * @param[in] uart UART controller identifier
 * @param[in] data Data to write to UART. This must not be NULL.
 * @param[in] len Length of data to write to UART.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_write_buffer(const qm_uart_t uart, const uint8_t *const data,
			 const uint32_t len);

/**
 * Interrupt based TX on UART.
 *
 * Perform an interrupt based TX transfer on the UART bus. The function
 * will replenish the TX FIFOs on UART empty interrupts.
 *
 * @param[in] uart UART identifier.
 * @param[in] xfer Structure containing pre-allocated
 *                 write buffer and callback functions.
 *                 The structure must not be NULL and must be kept valid until
 *                 the transfer is complete.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_irq_write(const qm_uart_t uart,
		      const qm_uart_transfer_t *const xfer);

/**
 * Interrupt based RX on UART.
 *
 * Perform an interrupt based RX transfer on the UART bus. The function
 * will read back the RX FIFOs on UART empty interrupts.
 *
 * @param[in] uart UART identifier.
 * @param[in] xfer Structure containing pre-allocated read
 *                 buffer and callback functions.
 *                 The structure must not be NULL and must be kept valid until
 *                 the transfer is complete.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_irq_read(const qm_uart_t uart,
		     const qm_uart_transfer_t *const xfer);

/**
 * Terminate UART IRQ TX transfer.
 *
 * Terminate the current IRQ TX transfer on the UART bus.
 * This will cause the relevant callbacks to be called.
 *
 * @param[in] uart UART identifier.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_irq_write_terminate(const qm_uart_t uart);

/**
 * Terminate UART IRQ RX transfer.
 *
 * Terminate the current IRQ RX transfer on the UART bus.
 * This will cause the relevant callbacks to be called.
 *
 * @param[in] uart UART identifier.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_irq_read_terminate(const qm_uart_t uart);

/**
 * Configure a DMA channel with a specific transfer direction.
 *
 * The user is responsible for managing the allocation of the pool
 * of DMA channels provided by each DMA core to the different
 * peripheral drivers that require them.
 *
 * This function configures DMA channel parameters that are unlikely to change
 * between transfers, like transaction width, burst size, and handshake
 * interface parameters. The user will likely only call this function once for
 * the lifetime of an application unless the channel needs to be repurposed.
 *
 * Note that qm_dma_init() must first be called before configuring a channel.
 *
 * @param[in] uart UART identifier.
 * @param[in] dma_ctrl_id DMA controller identifier.
 * @param[in] dma_channel_id DMA channel identifier.
 * @param[in] dma_channel_direction DMA channel direction, either
 * QM_DMA_MEMORY_TO_PERIPHERAL (write transfer) or QM_DMA_PERIPHERAL_TO_MEMORY
 * (read transfer).
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_dma_channel_config(
    const qm_uart_t uart, const qm_dma_t dma_ctrl_id,
    const qm_dma_channel_id_t dma_channel_id,
    const qm_dma_channel_direction_t dma_channel_direction);

/**
 * Perform a DMA-based TX transfer on the UART bus.
 *
 * In order for this call to succeed, previously the user
 * must have configured a DMA channel with direction
 * QM_DMA_MEMORY_TO_PERIPHERAL to be used on this UART, calling
 * qm_uart_dma_channel_config(). The transfer length is limited to 4KB.
 *
 * Note that this function uses the UART TX FIFO empty interrupt and therefore,
 * in addition to the DMA interrupts, the ISR of the corresponding UART must be
 * registered before using this function.
 *
 * @param[in] uart UART identifer.
 * @param[in] xfer Structure containing a pre-allocated write buffer
 *                 and callback functions.
 *                 This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_dma_write(const qm_uart_t uart,
		      const qm_uart_transfer_t *const xfer);

/**
 * Perform a DMA-based RX transfer on the UART bus.
 *
 * In order for this call to succeed, previously the user
 * must have configured a DMA channel with direction
 * QM_DMA_PERIPHERAL_TO_MEMORY to be used on this UART, calling
 * qm_uart_dma_channel_config(). The transfer length is limited to 4KB.
 *
 * @param[in] uart UART identifer.
 * @param[in] xfer Structure containing a pre-allocated read buffer
 *                 and callback functions.
 *                 This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_dma_read(const qm_uart_t uart,
		     const qm_uart_transfer_t *const xfer);

/**
 * Terminate the current DMA TX transfer on the UART bus.
 *
 * This will cause the relevant callbacks to be called.
 *
 * @param[in] uart UART identifer.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_dma_write_terminate(const qm_uart_t uart);

/**
 * Terminate the current DMA RX transfer on the UART bus.
 *
 * This will cause the relevant callbacks to be called.
 *
 * @param[in] uart UART identifer.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_dma_read_terminate(const qm_uart_t uart);

/**
 * Save UART context.
 *
 * Saves the configuration of the specified UART peripheral
 * before entering sleep.
 *
 * @param[in] uart UART port index.
 * @param[out] ctx UART context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_save_context(const qm_uart_t uart, qm_uart_context_t *const ctx);

/**
 * Restore UART context.
 *
 * Restore the configuration of the specified UART peripheral
 * after exiting sleep.
 *
 * FIFO control register cannot be read back,
 * the default configuration is applied for this register.
 * Application will need to restore its own parameters.
 *
 * @param[in] uart UART port index.
 * @param[in] ctx UART context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_uart_restore_context(const qm_uart_t uart,
			    const qm_uart_context_t *const ctx);

/**
 * @}
 */

#endif /* __QM_UART_H__ */
