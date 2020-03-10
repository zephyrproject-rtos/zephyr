/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_ASYNC_H_
#define ZEPHYR_DRIVERS_I2C_ASYNC_H_

#include <kernel.h>
#include <zephyr/types.h>
#include <sys/queued_seq.h>

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_INVALID_ADDRESS 0xFFFF

/* Forward declarations. */
struct i2c_async;
struct i2c_async_op;


#define I2C_SEQ_ACTION_ADDRESS_SET(_qualifier, addr) \
	SYS_SEQ_CUSTOM_ACTION(i2c_async_sys_seq_address_set, _qualifier, \
				uint16_t, (addr))

#define I2C_SEQ_ACTION_MSG(_qualifier, buf, len, flags) \
	SYS_SEQ_CUSTOM_ACTION(i2c_async_sys_seq_xfer, _qualifier, \
			    struct i2c_msg,\
			    (I2C_MSG_INITIALIZER(buf, len, flags)))

#define I2C_SEQ_ACTION_VMSG(value, _flags) \
	SYS_SEQ_CUSTOM_ACTION(i2c_async_sys_seq_xfer, static const, \
			struct i2c_msg,\
			({ \
			   .buf = (uint8_t *)(const char[]){__DEBRACKET value},\
			   .len = sizeof((const char[]){__DEBRACKET value}),\
			   .flags = _flags \
			}))

#define I2C_SEQ_ACTION_TX(_qualifier, buf, len) \
	I2C_SEQ_ACTION_MSG(_qualifier, buf, len, I2C_MSG_WRITE | I2C_MSG_STOP)

#define I2C_SEQ_ACTION_TXV(value) \
	I2C_SEQ_ACTION_VMSG(value, I2C_MSG_WRITE | I2C_MSG_STOP)

#define I2C_SEQ_ACTION_TX_RX(_qualifier, txbuf, txlen, rxbuf, rxlen) \
	I2C_SEQ_ACTION_MSG(_qualifier, txbuf, txlen, I2C_MSG_WRITE), \
	I2C_SEQ_ACTION_MSG(_qualifier, rxbuf, rxlen, \
			I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP)

#define I2C_SEQ_ACTION_TXV_RX(_qualifier, txv, rxbuf, rxlen) \
	I2C_SEQ_ACTION_VMSG(txv, I2C_MSG_WRITE), \
	I2C_SEQ_ACTION_MSG(_qualifier, rxbuf, rxlen, \
			I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP)

/** @brief Signature of the operation completion callback for sys_notify.
 *
 * @param i2c	Instance that performed the transfer.
 * @param op	Completed operation.
 * @param res	Operation result.
 */
typedef void (*i2c_sys_notify)(struct i2c_async *i2c, struct i2c_async_op *op,
				int res);

/** @brief State associated with a i2c asynchronous instance.
 *
 * Structure must be part of i2c device data.
 */
struct i2c_async {
	struct queued_seq_mgr mgrs;
	struct k_timer delay_timer;

	struct sys_notify action_notify;

	/* I2C device. */
	struct device *dev;

	/* I2C address used in the current sequence. */
	uint16_t addr;
};

/** @brief Process function i2c transfer request in sequence manager context.
 *
 * @param mgr Sequence manager instance.
 * @param data Data.
 */
int i2c_async_sys_seq_xfer(struct sys_seq_mgr *mgr, void *data);

/** @brief Process function i2c transfer request in sequence manager context.
 *
 * @param mgr Sequence manager instance.
 * @param data Data.
 */
int i2c_async_sys_seq_address_set(struct sys_seq_mgr *mgr, void *data);

/** @brief Initialize i2c_async structure.
 *
 * Must be called during device initialization.
 *
 * @param i2c_async	the I2C async object.
 * @param dev		I2C device.
 *
 * @return non-negative on successful initialization, negative value on error.
 */
int i2c_async_init(struct i2c_async *i2c_async, struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_ASYNC_H_ */
