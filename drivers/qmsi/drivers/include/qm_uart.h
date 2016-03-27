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

#ifndef __QM_UART_H__
#define __QM_UART_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * UART driver for Quark Microcontrollers.
 *
 * @defgroup groupUART UART
 * @{
 */

/* Register fields */
#define QM_UART_LCR_DLAB BIT(7)

#define QM_UART_MCR_AFCE BIT(5)
#define QM_UART_MCR_RTS BIT(1)

#define QM_UART_FCR_FIFOE BIT(0)
#define QM_UART_FCR_RFIFOR BIT(1)
#define QM_UART_FCR_XFIFOR BIT(2)

#define QM_UART_IIR_THR_EMPTY (0x02)
#define QM_UART_IIR_IID_MASK (0x0F)

#define QM_UART_LSR_DR BIT(0)
#define QM_UART_LSR_OE BIT(1)
#define QM_UART_LSR_PE BIT(2)
#define QM_UART_LSR_FE BIT(3)
#define QM_UART_LSR_BI BIT(4)
#define QM_UART_LSR_THRE BIT(5)
#define QM_UART_LSR_TEMT BIT(6)
#define QM_UART_LSR_RFE BIT(7)

/* Enable Transmit Holding Register Empty Interrupt*/
#define QM_UART_IER_ETBEI BIT(1)
/* Enable Received Data Available Interrupt */
#define QM_UART_IER_ERBFI BIT(0)
#define QM_UART_IER_PTIME BIT(7)

#define QM_UART_LSR_ERROR_BITS                                                 \
	(QM_UART_LSR_OE | QM_UART_LSR_PE | QM_UART_LSR_FE | QM_UART_LSR_BI)

#define QM_UART_FIFO_DEPTH (16)
#define QM_UART_FIFO_HALF_DEPTH (QM_UART_FIFO_DEPTH / 2)

/**
 * UART Line control.
 */
typedef enum {
	QM_UART_LC_5N1 = 0x00,   /**< 5 data bits, no parity, 1 stop bit */
	QM_UART_LC_5N1_5 = 0x04, /**< 5 data bits, no parity, 1.5 stop bits */
	QM_UART_LC_5E1 = 0x18,   /**< 5 data bits, even parity, 1 stop bit */
	QM_UART_LC_5E1_5 = 0x1c, /**< 5 data bits, even parity, 1.5 stop bits */
	QM_UART_LC_5O1 = 0x08,   /**< 5 data bits, odd parity, 1 stop bit */
	QM_UART_LC_5O1_5 = 0x0c, /**< 5 data bits, odd parity, 1.5 stop bits */
	QM_UART_LC_6N1 = 0x01,   /**< 6 data bits, no parity, 1 stop bit */
	QM_UART_LC_6N2 = 0x05,   /**< 6 data bits, no parity, 2 stop bits */
	QM_UART_LC_6E1 = 0x19,   /**< 6 data bits, even parity, 1 stop bit */
	QM_UART_LC_6E2 = 0x1d,   /**< 6 data bits, even parity, 2 stop bits */
	QM_UART_LC_6O1 = 0x09,   /**< 6 data bits, odd parity, 1 stop bit */
	QM_UART_LC_6O2 = 0x0d,   /**< 6 data bits, odd parity, 2 stop bits */
	QM_UART_LC_7N1 = 0x02,   /**< 7 data bits, no parity, 1 stop bit */
	QM_UART_LC_7N2 = 0x06,   /**< 7 data bits, no parity, 2 stop bits */
	QM_UART_LC_7E1 = 0x1a,   /**< 7 data bits, even parity, 1 stop bit */
	QM_UART_LC_7E2 = 0x1e,   /**< 7 data bits, even parity, 2 stop bits */
	QM_UART_LC_7O1 = 0x0a,   /**< 7 data bits, odd parity, 1 stop bit */
	QM_UART_LC_7O2 = 0x0e,   /**< 7 data bits, odd parity, 2 stop bits */
	QM_UART_LC_8N1 = 0x03,   /**< 8 data bits, no parity, 1 stop bit */
	QM_UART_LC_8N2 = 0x07,   /**< 8 data bits, no parity, 2 stop bits */
	QM_UART_LC_8E1 = 0x1b,   /**< 8 data bits, even parity, 1 stop bit */
	QM_UART_LC_8E2 = 0x1f,   /**< 8 data bits, even parity, 2 stop bits */
	QM_UART_LC_8O1 = 0x0b,   /**< 8 data bits, odd parity, 1 stop bit */
	QM_UART_LC_8O2 = 0x0f    /**< 8 data bits, odd parity, 2 stop bits */
} qm_uart_lc_t;

/* Masks/offsets for baudrate divisors fields in config structure */
#define QM_UART_CFG_BAUD_DLH_OFFS 16
#define QM_UART_CFG_BAUD_DLL_OFFS 8
#define QM_UART_CFG_BAUD_DLF_OFFS 0
#define QM_UART_CFG_BAUD_DLH_MASK (0xFF << QM_UART_CFG_BAUD_DLH_OFFS)
#define QM_UART_CFG_BAUD_DLL_MASK (0xFF << QM_UART_CFG_BAUD_DLL_OFFS)
#define QM_UART_CFG_BAUD_DLF_MASK (0xFF << QM_UART_CFG_BAUD_DLF_OFFS)

/* Helpers for baudrate divisor packing/unpacking */
#define QM_UART_CFG_BAUD_DL_PACK(dlh, dll, dlf)                                \
	(dlh << QM_UART_CFG_BAUD_DLH_OFFS | dll << QM_UART_CFG_BAUD_DLL_OFFS | \
	 dlf << QM_UART_CFG_BAUD_DLF_OFFS)

#define QM_UART_CFG_BAUD_DLH_UNPACK(packed)                                    \
	((packed & QM_UART_CFG_BAUD_DLH_MASK) >> QM_UART_CFG_BAUD_DLH_OFFS)
#define QM_UART_CFG_BAUD_DLL_UNPACK(packed)                                    \
	((packed & QM_UART_CFG_BAUD_DLL_MASK) >> QM_UART_CFG_BAUD_DLL_OFFS)
#define QM_UART_CFG_BAUD_DLF_UNPACK(packed)                                    \
	((packed & QM_UART_CFG_BAUD_DLF_MASK) >> QM_UART_CFG_BAUD_DLF_OFFS)

/**
 * UART Status type.
 */
typedef enum {
	QM_UART_OK = 0,
	QM_UART_IDLE = 0,
	QM_UART_RX_OE = BIT(1), /* Receiver overrun */
	QM_UART_RX_PE = BIT(2), /* Parity error */
	QM_UART_RX_FE = BIT(3), /* Framing error */
	QM_UART_RX_BI = BIT(4), /* Break interrupt */
	QM_UART_TX_BUSY = BIT(5),
	QM_UART_RX_BUSY = BIT(6),
	QM_UART_TX_NFULL = BIT(7),  /* TX FIFO not full */
	QM_UART_RX_NEMPTY = BIT(8), /* RX FIFO not empty */
	QM_UART_EINVAL = BIT(31),   /* Invalid input parameter */
} qm_uart_status_t;

/**
 * UART configuration type.
 */
typedef struct {
	qm_uart_lc_t line_control;
	uint32_t baud_divisor;
	bool hw_fc;
	bool int_en;
} qm_uart_config_t;

/**
 * UART IRQ transfer structure, holds pre-allocated write and read buffers.
 * Also pointers to user defined callbacks for write, read and errors.
 */
typedef struct {
	uint8_t *data;
	uint32_t data_len;
	void (*fin_callback)(uint32_t id, uint32_t len);
	void (*err_callback)(uint32_t id, qm_uart_status_t status);
	uint32_t id;
} qm_uart_transfer_t;

/**
 * UART 0 Interrupt Service Routine.
 */
void qm_uart_0_isr(void);

/**
 * UART 1 Interrupt Service Routine.
 */
void qm_uart_1_isr(void);

/**
 * Change the configuration of a UART module. This includes line control,
 * baud rate and hardware flow control.
 *
 * @brief Set UART confguration.
 * @param[in] uart Which UART module to configure.
 * @param[in] cfg New configuration for UART.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_uart_set_config(const qm_uart_t uart,
			   const qm_uart_config_t *const cfg);

/**
 * Read the current configuration of a UART module. This includes line
 * control, baud rate and hardware flow control.
 *
 * @brief Get UART confguration.
 * @param[in] uart Which UART module to read the configuration of.
 * @param[in] cfg Current configuration for UART.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_uart_get_config(const qm_uart_t uart, qm_uart_config_t *const cfg);

/**
 * Retrieve UART interface status. Return QM_UART_BUSY if transmitting
 * data; QM_UART_IDLE if available for transfer QM_UART_TX_ERROR if an
 * error has occurred in transmission.
 *
 * @brief Get UART bus status.
 * @param[in] uart Which UART to read the status of.
 * @return qm_uart_status_t Returns UART specific return code.
 */
qm_uart_status_t qm_uart_get_status(const qm_uart_t uart);

/**
 * Perform a single character write on the UART interface.
 * This is a blocking synchronous call.
 *
 * @brief UART character data write.
 * @param [in] uart UART index.
 * @param [in] data Data to write to UART.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_uart_write(const qm_uart_t uart, const uint8_t data);

/**
 * Perform a single character read from the UART interface.
 * This is a blocking synchronous call.
 *
 * @brief UART character data read.
 * @param [in] uart UART index.
 * @param [out] data Data to read from UART.
 * @return qm_uart_status_t Returns UART specific return code.
 */
qm_uart_status_t qm_uart_read(const qm_uart_t uart, uint8_t *data);

/**
 * Perform a single character write on the UART interface.
 * This is a non-blocking synchronous call.
 *
 * @brief UART character data write.
 * @param [in] uart UART index.
 * @param [in] data Data to write to UART.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_uart_write_non_block(const qm_uart_t uart, const uint8_t data);

/**
 * Perform a single character read from the UART interface.
 * This is a non-blocking synchronous call.
 *
 * @brief UART character data read.
 * @param [in] uart UART index.
 * @return uint8_t Character read.
 */
uint8_t qm_uart_read_non_block(const qm_uart_t uart);

/**
 * Perform a write on the UART interface. This is a blocking
 * synchronous call. The function will block until all data has
 * been transferred.
 *
 * @brief UART multi-byte data write.
 * @param [in] uart UART index.
 * @param [in] data Data to write to UART.
 * @param [in] len Length of data to write to UART.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_uart_write_buffer(const qm_uart_t uart, const uint8_t *const data,
			     uint32_t len);

/**
 * Perform an interrupt based TX transfer on the UART bus. The function
 * will replenish the TX FIFOs on UART empty interrupts.
 *
 * @brief Interrupt based TX on UART.
 * @param [in] uart UART index.
 * @param [in] xfer Structure containing pre-allocated write buffer and callback
 * 		    functions. The callbacks cannot be null.
 * @return qm_uart_status_t Returns UART specific return code.
 */
qm_uart_status_t qm_uart_irq_write(const qm_uart_t uart,
				   const qm_uart_transfer_t *const xfer);

/**
 * Perform an interrupt based RX transfer on the UART bus. The function
 * will read back the RX FIFOs on UART empty interrupts.
 *
 * @brief Interrupt based RX on UART.
 * @param [in] uart UART register block pointer.
 * @param [in] xfer Structure containing pre-allocated read
 *              buffer and callback functions. The callbacks cannot be null.
 * @return qm_uart_status_t Returns UART specific return code.
 */
qm_uart_status_t qm_uart_irq_read(const qm_uart_t uart,
				  const qm_uart_transfer_t *const xfer);

/**
 * Terminate the current IRQ or DMA TX transfer on the UART bus.
 * This will cause the relevant callbacks to be called.
 *
 * @brief Terminate UART IRQ/DMA TX transfer.
 * @param [in] uart UART index.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_uart_write_terminate(const qm_uart_t uart);

/**
 * Terminate the current IRQ or DMA RX transfer on the UART bus.
 * This will cause the relevant callbacks to be called.
 *
 * @brief Terminate UART IRQ/DMA RX transfer.
 * @param [in] uart UART index.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_uart_read_terminate(const qm_uart_t uart);

/**
 * @}
 */

#endif /* __QM_UART_H__ */
