/* uart.h - Nordic BLE UART based Bluetooth driver */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * IPC Uart power management driver.
 */
extern struct driver ipc_uart_ns16550_driver;

int nble_open(void);

/**
 * This function triggers the sending of PDU buffer over UART.
 *
 * This constructs an IPC message header and triggers the sending of it and
 * message buffer. If a transmission is already ongoing, it will fail.
 * In this case upper layer needs to queue the message buffer.
 *
 * @param dev structure of the opened device
 * @param handle structure of opened IPC uart channel
 * @param len length of message to send
 * @param p_data message buffer to send
 *
 * @return IPC_UART_ERROR_OK TX has been initiated, IPC_UART_TX_BUSY a
 * transmission is already going, message needs to be queued
 *
 * @note This function needs to be executed with (UART) irq off to avoid
 * pre-emption from uart_ipc_isr causing state variable corruption.
 * It also called from uart_ipc_isr() to send the next IPC message.
 */
int ipc_uart_ns16550_send_pdu(struct device *dev, void *handle, int len,
			      void *p_data);

/**
 * This function registers a callback function being called on TX start/end.
 *
 * This constructs an IPC message header and triggers the sending of it and
 * message buffer. If a transmission is already ongoing, it will fail. In
 * this case upper layer needs to queue the message buffer.
 *
 * @param dev structure of the opened device
 * @param cb callback function for OOB sleep mode handling called at tx start
 * and end
 * @param param parameter passed to cb when being called
 */
void ipc_uart_ns16550_set_tx_cb(struct device *dev, void (*cb)(bool, void*),
				void *param);

/**
 * Opens a UART channel for QRK/BLE Core IPC, and defines the callback function
 * for receiving IPC messages.
 *
 * @param channel IPC channel ID to use
 * @param cb      Callback to handle messages
 *
 * @return
 *         - Pointer to channel structure if success,
 *         - NULL if opening fails.
 */
void *ipc_uart_channel_open(int channel, int (*cb)(int chan, int request,
						   int len, void *data));

/**
 * The frame is a message.
 */
#define IPC_MSG_TYPE_MESSAGE 0x1

/**
 * Requests to free a message.
 */
#define IPC_MSG_TYPE_FREE    0x2

/**
 * Sets the MessageBox as synchronized.
 */
#define IPC_MSG_TYPE_SYNC    0x3

/**
 * Allocates a port.
 *
 * This request is always flowing from a slave to the master.
 */
#define IPC_REQUEST_ALLOC_PORT         0x10

/**
 * Registers a service.
 *
 * This request is always flowing from a slave to the master.
 */
#define IPC_REQUEST_REGISTER_SERVICE   0x11

/**
 * Unregisters a service.
 */
#define IPC_REQUEST_DEREGISTER_SERVICE 0x12

/**
 * The message is for test commands engine.
 */
#define IPC_REQUEST_REG_TCMD_ENGINE    0x13

/**
 * Registers a Service Manager Proxy to the Service Manager.
 *
 * This request always flow from a slave to the master.
 */
#define IPC_REQUEST_REGISTER_PROXY     0x14

/**
 * Notifies a panic (for log dump).
 */
#define IPC_PANIC_NOTIFICATION         0x15

/**
 * The message is for power management.
 */
#define IPC_REQUEST_POWER_MANAGEMENT   0x16

/**
 * Sends the log of a slave to the master (for log aggregation).
 */
#define IPC_REQUEST_LOGGER             0x17

/**
 * The message is for power management (deep sleep).
 */
#define IPC_REQUEST_INFRA_PM           0x18
