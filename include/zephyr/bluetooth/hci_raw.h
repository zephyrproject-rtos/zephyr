/** @file
 *  @brief Bluetooth HCI RAW channel handling
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HCI_RAW_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HCI_RAW_H_

/**
 * @brief HCI RAW channel
 * @defgroup hci_raw HCI RAW channel
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Send packet to the Bluetooth controller
 *
 * Send packet to the Bluetooth controller. Caller needs to
 * implement netbuf pool.
 *
 * @param buf netbuf packet to be send
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_send(struct net_buf *buf);

enum {
	/** Passthrough mode
	 *
	 *  While in this mode the buffers are passed as is between the stack
	 *  and the driver.
	 */
	BT_HCI_RAW_MODE_PASSTHROUGH = 0x00,

	/** H:4 mode
	 *
	 *  While in this mode H:4 headers will added into the buffers
	 *  according to the buffer type when coming from the stack and will be
	 *  removed and used to set the buffer type.
	 */
	BT_HCI_RAW_MODE_H4 = 0x01,
};

/** @brief Set Bluetooth RAW channel mode
 *
 *  Set access mode of Bluetooth RAW channel.
 *
 *  @param mode Access mode.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_hci_raw_set_mode(uint8_t mode);

/** @brief Get Bluetooth RAW channel mode
 *
 *  Get access mode of Bluetooth RAW channel.
 *
 *  @return Access mode.
 */
uint8_t bt_hci_raw_get_mode(void);

#define BT_HCI_ERR_EXT_HANDLED  0xff

/** Helper macro to define a command extension
 *
 *  @param _op Opcode of the command.
 *  @param _min_len Minimal length of the command.
 *  @param _func Handler function to be called.
 */
#define BT_HCI_RAW_CMD_EXT(_op, _min_len, _func) \
	{ \
		.op = _op, \
		.min_len = _min_len, \
		.func = _func, \
	}

struct bt_hci_raw_cmd_ext {
	/** Opcode of the command */
	uint16_t  op;

	/** Minimal length of the command */
	size_t min_len;

	/** Handler function.
	 *
	 *  Handler function to be called when a command is intercepted.
	 *
	 *  @param buf Buffer containing the command.
	 *
	 *  @return HCI Status code or BT_HCI_ERR_EXT_HANDLED if command has
	 *  been handled already and a response has been sent as oppose to
	 *  BT_HCI_ERR_SUCCESS which just indicates that the command can be
	 *  sent to the controller to be processed.
	 */
	uint8_t   (*func)(struct net_buf *buf);
};

/** @brief Register Bluetooth RAW command extension table
 *
 *  Register Bluetooth RAW channel command extension table, opcodes in this
 *  table are intercepted to sent to the handler function.
 *
 *  @param cmds Pointer to the command extension table.
 *  @param size Size of the command extension table.
 */
void bt_hci_raw_cmd_ext_register(struct bt_hci_raw_cmd_ext *cmds, size_t size);

/** @brief Enable Bluetooth RAW channel:
 *
 *  Enable Bluetooth RAW HCI channel.
 *
 *  @param rx_queue netbuf queue where HCI packets received from the Bluetooth
 *  controller are to be queued. The queue is defined in the caller while
 *  the available buffers pools are handled in the stack.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_enable_raw(struct k_fifo *rx_queue);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HCI_RAW_H_ */
