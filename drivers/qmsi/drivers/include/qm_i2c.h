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

#ifndef __QM_I2C_H__
#define __QM_I2C_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * I2C driver for Quark Microcontrollers.
 *
 * @defgroup groupI2C I2C
 * @{
 */

/* High/low period for 50% duty cycle bus clock (in nanoseconds) */
#define QM_I2C_SS_50_DC_NS (5000)
#define QM_I2C_FS_50_DC_NS (1250)
#define QM_I2C_FSP_50_DC_NS (500)

/* Minimum low period to meet timing requirements (in nanoseconds) */
#define QM_I2C_MIN_SS_NS (4700)
#define QM_I2C_MIN_FS_NS (1300)
#define QM_I2C_MIN_FSP_NS (500)

/**
 * QM I2C addressing type.
 */
typedef enum { QM_I2C_7_BIT = 0, QM_I2C_10_BIT } qm_i2c_addr_t;

/**
 * QM I2C master / slave mode type.
 */
typedef enum { QM_I2C_MASTER, QM_I2C_SLAVE } qm_i2c_mode_t;

/**
 * QM I2C Speed Type.
 */
typedef enum {
	QM_I2C_SPEED_STD = 1,      /* Standard mode (100 Kbps) */
	QM_I2C_SPEED_FAST = 2,     /* Fast mode (400 Kbps)  */
	QM_I2C_SPEED_FAST_PLUS = 3 /* Fast plus mode (1 Mbps) */
} qm_i2c_speed_t;

/**
 * I2C status type.
 */
typedef enum {
	QM_I2C_IDLE = 0,
	QM_I2C_TX_ABRT_7B_ADDR_NOACK = BIT(0),
	QM_I2C_TX_ABRT_10ADDR1_NOACK = BIT(1),
	QM_I2C_TX_ABRT_10ADDR2_NOACK = BIT(2),
	QM_I2C_TX_ABRT_TXDATA_NOACK = BIT(3),
	QM_I2C_TX_ABRT_GCALL_NOACK = BIT(4),
	QM_I2C_TX_ABRT_GCALL_READ = BIT(5),
	QM_I2C_TX_ABRT_HS_ACKDET = BIT(6),
	QM_I2C_TX_ABRT_SBYTE_ACKDET = BIT(7),
	QM_I2C_TX_ABRT_HS_NORSTRT = BIT(8),
	QM_I2C_TX_ABRT_10B_RD_NORSTRT = BIT(10),
	QM_I2C_TX_ABRT_MASTER_DIS = BIT(11),
	QM_I2C_TX_ARB_LOST = BIT(12),
	QM_I2C_TX_ABRT_SLVFLUSH_TXFIFO = BIT(13),
	QM_I2C_TX_ABRT_SLV_ARBLOST = BIT(14),
	QM_I2C_TX_ABRT_SLVRD_INTX = BIT(15),
	QM_I2C_TX_ABRT_USER_ABRT = BIT(16),
	QM_I2C_BUSY = BIT(17),
} qm_i2c_status_t;

/**
 * I2C configuration type.
 */
typedef struct {
	qm_i2c_speed_t speed;       /* Standard, Fast Mode */
	qm_i2c_addr_t address_mode; /* 7 or 10 bit addressing */
	qm_i2c_mode_t mode;	 /* Master or slave mode */
	uint16_t slave_addr;	/* I2C address when in slave mode */
} qm_i2c_config_t;

/**
 * I2C transfer type.
 * Master mode:
 * - if Tx len is 0: perform receive-only transaction
 * - if Rx len is 0: perform transmit-only transaction
 * - both Tx and Rx len not 0: perform a transmit-then-
 * receive combined transaction
 * Slave mode:
 * - If read or write exceed the buffer, then wrap around.
*/
typedef struct {
	uint8_t *tx;     /* Write data */
	uint32_t tx_len; /* Write data length */
	uint8_t *rx;     /* Read data */
	uint32_t rx_len; /* Read buffer length */
	uint32_t id;     /* Callback identifier */
	bool stop;       /* Generate master STOP */
	void (*tx_callback)(uint32_t id, uint32_t len); /* Write callback -
							   required if tx !=
							   NULL*/
	void (*rx_callback)(uint32_t id, uint32_t len); /* Read callback -
							   required if rx !=
							   NULL */
	void (*err_callback)(uint32_t id,
			     qm_i2c_status_t status); /* Error callback -
							 required*/
} qm_i2c_transfer_t;

/**
 * I2C 0 Interrupt Service Routine.
 */
void qm_i2c_0_isr(void);

/**
 * I2C 1 Interrupt Service Routine.
 */
void qm_i2c_1_isr(void);

/**
 * Set I2C configuration.
 *
 * @param [in] i2c Which I2C to set the configuration of.
 * @param [out] cfg I2C configuration.
 * @return qm_rc_t Returns QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_i2c_set_config(const qm_i2c_t i2c, const qm_i2c_config_t *const cfg);

/**
 * Retrieve I2C configuration.
 *
 * @param [in] i2c Which I2C to read the configuration of.
 * @param [out] cfg I2C configuration.
 * @return qm_rc_t Returns QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_i2c_get_config(const qm_i2c_t i2c, qm_i2c_config_t *const cfg);

/**
 * Fine tune I2C clock speed. This will set the SCL low count
 * and the SCL hi count cycles. To achieve any required speed.
 * @brief Set I2C speed.
 * @param [in] i2c I2C index.
 * @param [in] speed Bus speed (Standard or Fast. Fast includes Fast+ mode)
 * @param [in] lo_cnt SCL low count.
 * @param [in] hi_cnt SCL high count.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_i2c_set_speed(const qm_i2c_t i2c, qm_i2c_speed_t speed,
			 uint16_t lo_cnt, uint16_t hi_cnt);

/**
 * Retrieve I2C status.
 *
 * @param [in] i2c Which I2C to read the status of.
 * @return qm_i2c_status_t Returns Free of Busy.
 */
qm_i2c_status_t qm_i2c_get_status(const qm_i2c_t i2c);

/**
 * Perform a master write on the I2C bus. This is a blocking synchronous call.
 *
 * @brief Master write on I2C.
 * @param [in] i2c Which I2C to write to.
 * @param [in] slave_addr Address of slave to write to.
 * @param [in] data Pre-allocated buffer of data to write.
 * @param [in] len length of data to write.
 * @param [in] stop Generate a STOP condition at the end of Tx
 * @return qm_rc_t Returns QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_i2c_master_write(const qm_i2c_t i2c, const uint16_t slave_addr,
			    const uint8_t *const data, uint32_t len, bool stop);

/**
 * Perform a single byte master read from the I2C. This is a blocking call.
 *
 * @brief Master read of I2C.
 * @param [in] i2c Which I2C to read from.
 * @param [in] slave_addr Address of slave device to read from.
 * @param [out] data Pre-allocated buffer to populate with data.
 * @param [in] len length of data to read from slave.
 * @param [in] stop Generate a STOP condition at the end of Tx
 * @return qm_rc_t Returns QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_i2c_master_read(const qm_i2c_t i2c, const uint16_t slave_addr,
			   uint8_t *const data, uint32_t len, bool stop);

/**
 * Perform an interrupt based master transfer on the I2C bus. The function will
 * replenish/empty TX/RX FIFOs on I2C empty/full interrupts.
 *
 * @brief Interrupt based master transfer on I2C.
 * @param[in] i2c Which I2C to transfer from.
 * @param[in] xfer Transfer structure includes write / read data and length,
 * 		   write, read and error callback functions and a callback
 * 		   identifier.
 * @param [in] slave_addr Address of slave to transfer data with.
 * @return qm_rc_t Returns QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_i2c_master_irq_transfer(const qm_i2c_t i2c,
				   const qm_i2c_transfer_t *const xfer,
				   const uint16_t slave_addr);

/**
 * Terminate the current IRQ or DMA transfer on the I2C bus.
 * This will cause the error callback to be called with status
 * QM_I2C_TX_ABRT_USER_ABRT.
 *
 * @brief Terminate I2C IRQ/DMA transfer.
 * @param [in] i2c I2C register block pointer.
 * @return qm_rc_t Returns QM_QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_i2c_transfer_terminate(const qm_i2c_t i2c);

/**
 * @}
 */

#endif /* __QM_I2C_H__ */
