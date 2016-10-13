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

#ifndef __QM_I2C_H__
#define __QM_I2C_H__

#include "qm_common.h"
#include "qm_soc_regs.h"
#include "qm_dma.h"

/**
 * I2C.
 *
 * @defgroup groupI2C I2C
 * @{
 */

/* High/low period for 50% duty cycle bus clock (in nanoseconds). */
#define QM_I2C_SS_50_DC_NS (5000)
#define QM_I2C_FS_50_DC_NS (1250)
#define QM_I2C_FSP_50_DC_NS (500)

/* Minimum low period to meet timing requirements (in nanoseconds). */
#define QM_I2C_MIN_SS_NS (4700)
#define QM_I2C_MIN_FS_NS (1300)
#define QM_I2C_MIN_FSP_NS (500)

/* Data command register masks and values. */
#define DATA_COMMAND_READ_COMMAND_BYTE (QM_I2C_IC_DATA_CMD_READ >> 8)
#define DATA_COMMAND_STOP_BIT_BYTE (QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL >> 8)

/**
 * QM I2C addressing type.
 */
typedef enum {
	QM_I2C_7_BIT = 0, /**< 7-bit mode. */
	QM_I2C_10_BIT     /**< 10-bit mode. */
} qm_i2c_addr_t;

/**
 * QM I2C master / slave mode type.
 */
typedef enum {
	QM_I2C_MASTER = 0, /**< Master mode. */
	QM_I2C_SLAVE       /**< Slave mode. */
} qm_i2c_mode_t;

/**
 * QM I2C speed type.
 */
typedef enum {
	QM_I2C_SPEED_STD = 1,      /**< Standard mode (100 Kbps). */
	QM_I2C_SPEED_FAST = 2,     /**< Fast mode (400 Kbps). */
	QM_I2C_SPEED_FAST_PLUS = 3 /**< Fast plus mode (1 Mbps). */
} qm_i2c_speed_t;

/**
 * I2C status type.
 */
typedef enum {
	QM_I2C_IDLE = 0,		       /**< Controller idle. */
	QM_I2C_TX_ABRT_7B_ADDR_NOACK = BIT(0), /**< 7-bit address noack. */
	QM_I2C_TX_ABRT_10ADDR1_NOACK = BIT(1), /**< 10-bit address noack. */

	/** 10-bit second address byte address noack. */
	QM_I2C_TX_ABRT_10ADDR2_NOACK = BIT(2),
	QM_I2C_TX_ABRT_TXDATA_NOACK = BIT(3), /**< Tx data noack. */
	QM_I2C_TX_ABRT_GCALL_NOACK = BIT(4),  /**< General call noack. */
	QM_I2C_TX_ABRT_GCALL_READ = BIT(5),   /**< Read after general call. */
	QM_I2C_TX_ABRT_HS_ACKDET = BIT(6),    /**< High Speed master ID ACK. */
	QM_I2C_TX_ABRT_SBYTE_ACKDET = BIT(7), /**< Start ACK. */

	/** High Speed with restart disabled. */
	QM_I2C_TX_ABRT_HS_NORSTRT = BIT(8),

	/** 10-bit address read and restart disabled. */
	QM_I2C_TX_ABRT_10B_RD_NORSTRT = BIT(10),
	QM_I2C_TX_ABRT_MASTER_DIS = BIT(11), /**< Master disabled. */
	QM_I2C_TX_ARB_LOST = BIT(12),	/**< Master lost arbitration. */
	QM_I2C_TX_ABRT_SLVFLUSH_TXFIFO = BIT(13), /**< Slave flush tx FIFO. */
	QM_I2C_TX_ABRT_SLV_ARBLOST = BIT(14),     /**< Slave lost bus. */
	QM_I2C_TX_ABRT_SLVRD_INTX = BIT(15),      /**< Slave read completion. */
	QM_I2C_TX_ABRT_USER_ABRT = BIT(16),       /**< User abort. */
	QM_I2C_BUSY = BIT(17),			  /**< Controller busy. */
	QM_I2C_TX_ABORT = BIT(18),		  /**< Tx abort. */
	QM_I2C_TX_OVER = BIT(19),		  /**< Tx overflow. */
	QM_I2C_RX_OVER = BIT(20),		  /**< Rx overflow. */
	QM_I2C_RX_UNDER = BIT(21)		  /**< Rx underflow. */
} qm_i2c_status_t;

/**
 * QM I2C slave stop detect behaviour
 */
typedef enum {
	/** Interrupt regardless of whether this slave is addressed or not. */
	QM_I2C_SLAVE_INTERRUPT_ALWAYS = 0x0,

	/** Only interrupt if this slave is being addressed. */
	QM_I2C_SLAVE_INTERRUPT_WHEN_ADDRESSED = 0x1
} qm_i2c_slave_stop_t;

/**
 * I2C configuration type.
 */
typedef struct {
	qm_i2c_speed_t speed;       /**< Standard, fast or fast plus mode. */
	qm_i2c_addr_t address_mode; /**< 7 bit or 10 bit addressing. */
	qm_i2c_mode_t mode;	 /**< Master or slave mode. */
	uint16_t slave_addr;	/**< I2C address when in slave mode. */

	/** Slave stop detect behaviour */
	qm_i2c_slave_stop_t stop_detect_behaviour;
} qm_i2c_config_t;

/**
 * I2C transfer type.
 * Master mode:
 * - If tx len is 0: perform receive-only transaction.
 * - If rx len is 0: perform transmit-only transaction.
 * - Both tx and rx len not 0: perform a transmit-then-receive
 *   combined transaction.
 * Slave mode:
 * - If read or write exceed the buffer, then wrap around.
*/
typedef struct {
	uint8_t *tx;     /**< Write data. */
	uint32_t tx_len; /**< Write data length. */
	uint8_t *rx;     /**< Read data. */
	uint32_t rx_len; /**< Read buffer length. */
	bool stop;       /**< Generate master STOP. */

	/**
	 * Transfer callback.
	 *
	 * Called after all data is transmitted/received or if the driver
	 * detects an error during the I2C transfer.
	 *
	 * @param[in] data User defined data.
	 * @param[in] rc 0 on success.
	 *            Negative @ref errno for possible error codes.
	 * @param[in] status I2C status.
	 * @param[in] len Length of the transfer if successful, 0 otherwise.
	 */
	void (*callback)(void *data, int rc, qm_i2c_status_t status,
			 uint32_t len);
	void *callback_data; /**< User callback data. */
} qm_i2c_transfer_t;

/**
 * Set I2C configuration.
 *
 * @param[in] i2c Which I2C to set the configuration of.
 * @param[out] cfg I2C configuration. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_set_config(const qm_i2c_t i2c, const qm_i2c_config_t *const cfg);

/**
 * Set I2C speed.
 *
 * Fine tune I2C clock speed. This will set the SCL low count
 * and the SCL hi count cycles. To achieve any required speed.
 *
 * @param[in] i2c I2C index.
 * @param[in] speed Bus speed (Standard or Fast. Fast includes Fast+ mode).
 * @param[in] lo_cnt SCL low count.
 * @param[in] hi_cnt SCL high count.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_set_speed(const qm_i2c_t i2c, const qm_i2c_speed_t speed,
		     const uint16_t lo_cnt, const uint16_t hi_cnt);

/**
 * Retrieve I2C bus status.
 *
 * @param[in] i2c Which I2C to read the status of.
 * @param[out] status Current I2C status. This must not be NULL.
 *
 * The user may call this function before performing an I2C transfer in order to
 * guarantee that the I2C interface is available.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_get_status(const qm_i2c_t i2c, qm_i2c_status_t *const status);

/**
 * Master write on I2C.
 *
 * Perform a master write on the I2C bus. This is a blocking synchronous call.
 *
 * @param[in] i2c Which I2C to write to.
 * @param[in] slave_addr Address of slave to write to.
 * @param[in] data Pre-allocated buffer of data to write. This must not be NULL.
 * @param[in] len Length of data to write.
 * @param[in] stop Generate a STOP condition at the end of tx.
 * @param[out] status Get I2C status.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_master_write(const qm_i2c_t i2c, const uint16_t slave_addr,
			const uint8_t *const data, uint32_t len,
			const bool stop, qm_i2c_status_t *const status);

/**
 * Master read of I2C.
 *
 * Perform a single byte master read from the I2C. This is a blocking call.
 *
 * @param[in] i2c Which I2C to read from.
 * @param[in] slave_addr Address of slave device to read from.
 * @param[out] data Pre-allocated buffer to populate with data. This must not be
 *		NULL.
 * @param[in] len Length of data to read from slave.
 * @param[in] stop Generate a STOP condition at the end of rx.
 * @param[out] status Get I2C status.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_master_read(const qm_i2c_t i2c, const uint16_t slave_addr,
		       uint8_t *const data, uint32_t len, const bool stop,
		       qm_i2c_status_t *const status);

/**
 * Interrupt based master transfer on I2C.
 *
 * Perform an interrupt based master transfer on the I2C bus. The function will
 * replenish/empty TX/RX FIFOs on I2C empty/full interrupts.
 *
 * @param[in] i2c Which I2C to transfer from.
 * @param[in] xfer Transfer structure includes write / read buffers, length,
 *		user callback function and the callback context.
 *		The structure must not be NULL and must be kept valid until
 *		the transfer is complete.
 * @param[in] slave_addr Address of slave to transfer data with.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_master_irq_transfer(const qm_i2c_t i2c,
			       const qm_i2c_transfer_t *const xfer,
			       const uint16_t slave_addr);

/**
 * Interrupt based slave transfer on I2C.
 *
 * Perform an interrupt based slave transfer on the I2C bus. The function will
 * replenish/empty TX/RX FIFOs on I2C empty/full interrupts.
 *
 * @param[in] i2c Which I2C to transfer from.
 * @param[in] xfer Transfer structure includes write / read buffers, length,
 *                 user callback function and the callback context. This must
 *                 not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_slave_irq_transfer(const qm_i2c_t i2c,
			      const qm_i2c_transfer_t *const xfer);

/**
 * Terminate I2C IRQ transfer.
 *
 * Terminate the current IRQ or DMA transfer on the I2C bus.
 * This will cause the user callback to be called with status
 * QM_I2C_TX_ABRT_USER_ABRT.
 *
 * @param[in] i2c I2C controller identifier.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_irq_transfer_terminate(const qm_i2c_t i2c);

/**
 * Configure a DMA channel with a specific transfer direction.
 *
 * Configure a DMA channel with a specific transfer direction. The user is
 * responsible for managing the allocation of the pool of DMA channels provided
 * by each DMA core to the different peripheral drivers that require them. Note
 * that a I2C controller cannot use different DMA cores to manage transfers in
 * different directions.
 *
 * This function configures DMA channel parameters that are unlikely to change
 * between transfers, like transaction width, burst size, and handshake
 * interface parameters. The user will likely only call this function once for
 * the lifetime of an application unless the channel needs to be repurposed.
 *
 * Note that qm_dma_init() must first be called before configuring a channel.
 *
 * @param[in] i2c I2C controller identifier.
 * @param[in] dma_controller_id DMA controller identifier.
 * @param[in] channel_id DMA channel identifier.
 * @param[in] direction DMA channel direction, either
 * QM_DMA_MEMORY_TO_PERIPHERAL (TX transfer) or QM_DMA_PERIPHERAL_TO_MEMORY
 * (RX transfer).
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_dma_channel_config(const qm_i2c_t i2c,
			      const qm_dma_t dma_controller_id,
			      const qm_dma_channel_id_t channel_id,
			      const qm_dma_channel_direction_t direction);

/**
 * Perform a DMA based master transfer on the I2C bus.
 *
 * Perform a DMA based master transfer on the I2C bus. If the transfer is TX
 * only, it will enable DMA operation for the controller and start the transfer.
 *
 * If it's an RX only transfer, it will require 2 channels, one for writing the
 * READ commands and another one for reading the bytes from the bus. Both DMA
 * operations will start in parallel.
 *
 * If this is a combined transaction, both TX and RX operations will be set up,
 * but only TX will be started. On TX finish (callback), the TX channel will be
 * used for writing the READ commands and the RX operation will start.
 *
 * Note that qm_i2c_dma_channel_config() must first be called in order to
 * configure all DMA channels needed for a transfer.
 *
 * @param[in] i2c I2C controller identifier.
 * @param[in] xfer Structure containing pre-allocated write and read data
 *		   buffers and callback functions. This must not be NULL
 *		   and must be kept valid until the transfer is complete.
 * @param[in] slave_addr Address of slave to transfer data with.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_master_dma_transfer(const qm_i2c_t i2c,
			       qm_i2c_transfer_t *const xfer,
			       const uint16_t slave_addr);

/**
 * Perform a DMA based slave transfer on the I2C bus.
 *
 * Note that qm_i2c_dma_channel_config() must first be called in order to
 * configure all DMA channels needed for a transfer.
 *
 * @param[in] i2c I2C controller identifier.
 * @param[in] xfer Structure containing pre-allocated write and read data
 *                 buffers and callback functions. This pointer must be kept
 *                 valid until the transfer is complete.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_slave_dma_transfer(const qm_i2c_t i2c,
			      const qm_i2c_transfer_t *const xfer);

/**
 * Terminate any DMA transfer going on on the controller.
 *
 * Calls the DMA driver to stop any ongoing DMA transfer and calls
 * qm_i2c_irq_transfer_terminate.
 *
 * @param[in] i2c Which I2C to terminate transfers from.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2c_dma_transfer_terminate(const qm_i2c_t i2c);

/**
 * @}
 */

#endif /* __QM_I2C_H__ */
