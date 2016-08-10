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

#ifndef __QM_SS_I2C_H__
#define __QM_SS_I2C_H__

#include "qm_common.h"
#include "qm_sensor_regs.h"

/**
 * I2C driver for Sensor Subsystem.
 *
 * @defgroup groupSSI2C SS I2C
 * @{
 */

/* Standard speed High/low period for 50% duty cycle bus clock (in nanosecs). */
#define QM_I2C_SS_50_DC_NS (5000)
/* Fast Speed High/low period for 50% duty cycle bus clock (in nanosecs). */
#define QM_I2C_FS_50_DC_NS (1250)
/* High Speed High/low period for 50% duty cycle bus clock (in nanosecs). */
#define QM_I2C_FSP_50_DC_NS (500)

/*
 * Standard speed minimum low period to meet timing requirements (in nanosecs).
 */
#define QM_I2C_MIN_SS_NS (4700)
/* Fast speed minimum low period to meet timing requirements (in nanosecs). */
#define QM_I2C_MIN_FS_NS (1300)
/* High speed minimum low period to meet timing requirements (in nanosecs). */
#define QM_I2C_MIN_FSP_NS (500)

/**
 * QM SS I2C addressing type.
 */
typedef enum {
	QM_SS_I2C_7_BIT = 0, /**< 7-bit mode. */
	QM_SS_I2C_10_BIT     /**< 10-bit mode. */
} qm_ss_i2c_addr_t;

/**
 * QM SS I2C Speed Type.
 */
typedef enum {
	QM_SS_I2C_SPEED_STD = 1, /**< Standard mode (100 Kbps). */
	QM_SS_I2C_SPEED_FAST = 2 /**< Fast mode (400 Kbps).  */
} qm_ss_i2c_speed_t;

/**
 * QM SS I2C status type.
 */
typedef enum {
	QM_I2C_IDLE = 0,		       /**< Controller idle. */
	QM_I2C_TX_ABRT_7B_ADDR_NOACK = BIT(0), /**< 7-bit address noack. */
	QM_I2C_TX_ABRT_TXDATA_NOACK = BIT(3),  /**< Tx data noack. */
	QM_I2C_TX_ABRT_SBYTE_ACKDET = BIT(7),  /**< Start ACK. */
	QM_I2C_TX_ABRT_MASTER_DIS = BIT(11),   /**< Master disabled. */
	QM_I2C_TX_ARB_LOST = BIT(12),	  /**< Master lost arbitration. */
	QM_I2C_TX_ABRT_SLVFLUSH_TXFIFO = BIT(13), /**< Slave flush tx FIFO. */
	QM_I2C_TX_ABRT_SLV_ARBLOST = BIT(14),     /**< Slave lost bus. */
	QM_I2C_TX_ABRT_SLVRD_INTX = BIT(15),      /**< Slave read completion. */
	QM_I2C_TX_ABRT_USER_ABRT = BIT(16),       /**< User abort. */
	QM_I2C_BUSY = BIT(17)			  /**< Controller busy. */
} qm_ss_i2c_status_t;

/**
 * QM SS I2C configuration type.
 */
typedef struct {
	qm_ss_i2c_speed_t speed;       /**< Standard, Fast Mode. */
	qm_ss_i2c_addr_t address_mode; /**< 7 or 10 bit addressing. */
} qm_ss_i2c_config_t;

/**
 * QM SS I2C transfer type.
 * - if tx len is 0: perform receive-only transaction.
 * - if rx len is 0: perform transmit-only transaction.
 * - both tx and rx len not 0: perform a transmit-then-receive
 *   combined transaction.
*/
typedef struct {
	uint8_t *tx;     /**< Write data. */
	uint32_t tx_len; /**< Write data length. */
	uint8_t *rx;     /**< Read data. */
	uint32_t rx_len; /**< Read buffer length. */
	bool stop;       /**< Generate master STOP. */

	/**
	 * User callback.
	 *
	 * @param[in] data User defined data.
	 * @param[in] rc 0 on success.
	 *                Negative @ref errno for possible error codes.
	 * @param[in] status I2C status.
	 * @param[in] len Length of the transfer if succesfull, 0 otherwise.
	 */
	void (*callback)(void *data, int rc, qm_ss_i2c_status_t status,
			 uint32_t len);
	void *callback_data; /**< User callback data. */
} qm_ss_i2c_transfer_t;

/**
 * Set SS I2C configuration.
 *
 * @param[in] i2c Which I2C to set the configuration of.
 * @param[in] cfg I2C configuration. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_i2c_set_config(const qm_ss_i2c_t i2c,
			 const qm_ss_i2c_config_t *const cfg);

/**
 * Set I2C speed.
 *
 * Fine tune SS I2C clock speed.
 * This will set the SCL low count and the SCL hi count cycles
 * to achieve any required speed.
 *
 * @param[in] i2c I2C index.
 * @param[in] speed Bus speed (Standard or Fast.Fast includes Fast + mode).
 * @param[in] lo_cnt SCL low count.
 * @param[in] hi_cnt SCL high count.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_i2c_set_speed(const qm_ss_i2c_t i2c, const qm_ss_i2c_speed_t speed,
			const uint16_t lo_cnt, const uint16_t hi_cnt);

/**
 * Retrieve SS I2C status.
 *
 * @param[in] i2c Which I2C to read the status of.
 * @param[out] status Get i2c status. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_i2c_get_status(const qm_ss_i2c_t i2c,
			 qm_ss_i2c_status_t *const status);

/**
 * Master write on I2C.
 *
 * Perform a master write on the SS I2C bus.
 * This is a blocking synchronous call.
 *
 * @param[in] i2c Which I2C to write to.
 * @param[in] slave_addr Address of slave to write to.
 * @param[in] data Pre-allocated buffer of data to write.
 *                 This must not be NULL.
 * @param[in] len length of data to write.
 * @param[in] stop Generate a STOP condition at the end of tx.
 * @param[out] status Get i2c status.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_i2c_master_write(const qm_ss_i2c_t i2c, const uint16_t slave_addr,
			   const uint8_t *const data, uint32_t len,
			   const bool stop, qm_ss_i2c_status_t *const status);

/**
 * Master read of I2C.
 *
 * Perform a single byte master read from the SS I2C. This is a blocking call.
 *
 * @param[in] i2c Which I2C to read from.
 * @param[in] slave_addr Address of slave device to read from.
 * @param[out] data Pre-allocated buffer to populate with data.
 *                  This must not be NULL.
 * @param[in] len length of data to read from slave.
 * @param[in] stop Generate a STOP condition at the end of tx.
 * @param[out] status Get i2c status.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_i2c_master_read(const qm_ss_i2c_t i2c, const uint16_t slave_addr,
			  uint8_t *const data, uint32_t len, const bool stop,
			  qm_ss_i2c_status_t *const status);

/**
 * Interrupt based master transfer on I2C.
 *
 * Perform an interrupt based master transfer on the SS I2C bus. The function
 * will replenish/empty TX/RX FIFOs on I2C empty/full interrupts.
 *
 * @param[in] i2c Which I2C to transfer from.
 * @param[in] xfer Transfer structure includes write / read data and length,
 * 		user callback function and the callback context.
 *              The structure must not be NULL and must be kept valid until
 *              the transfer is complete.
 * @param[in] slave_addr Address of slave to transfer data with.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_i2c_master_irq_transfer(const qm_ss_i2c_t i2c,
				  const qm_ss_i2c_transfer_t *const xfer,
				  const uint16_t slave_addr);

/**
 * Terminate I2C IRQ/DMA transfer.
 *
 * Terminate the current IRQ transfer on the SS I2C bus.
 * This will cause the user callback to be called with status
 * QM_I2C_TX_ABRT_USER_ABRT.
 *
 * @param[in] i2c I2C register block pointer.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_i2c_irq_transfer_terminate(const qm_ss_i2c_t i2c);

/**
 * @}
 */

#endif /* __QM_SS_I2C_H__ */
