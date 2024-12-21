/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_SERVICE_H
#define IPC_SERVICE_H

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include "spsc_qm.h"

#define GET_IPC_INSTANCE(dev) (dev)
typedef struct device ipc_device_wrapper_t;

#include <stdint.h>
#include <stdbool.h>

/*
 * Must be large enough to contain the internal struct (spsc_pbuf struct) and
 * at least two bytes of data (one is reserved for written message length)
 */
#define _MIN_SPSC_SIZE            (sizeof(spsc_queue_t) + sizeof(uint32_t))
/*
 * TODO: Unsure why some additional bytes are needed for overhead.
 */
#define WIFI_IPC_GET_SPSC_SIZE(x) (_MIN_SPSC_SIZE + 12 + (x))

/* 4 x cmd location 32-bit pointers of 400 bytes each */
#define WIFI_IPC_CMD_SIZE      400
#define WIFI_IPC_CMD_NUM       4
#define WIFI_IPC_CMD_SPSC_SIZE WIFI_IPC_GET_SPSC_SIZE(WIFI_IPC_CMD_NUM * sizeof(uint32_t))

/* 7 x event location 32-bit pointers of 1000 bytes each */
#define WIFI_IPC_EVENT_SIZE      1000
#define WIFI_IPC_EVENT_NUM       7
#define WIFI_IPC_EVENT_SPSC_SIZE WIFI_IPC_GET_SPSC_SIZE(WIFI_IPC_EVENT_NUM * sizeof(uint32_t))

/**
 * @enum wifi_ipc_status_t
 * @brief Status codes for the Wi-Fi IPC service.
 *
 * This enumeration defines various status codes that represent
 * the state or result of operations in the Wi-Fi IPC service.
 */
typedef enum {
	/** Status indicating the operation completed successfully. */
	WIFI_IPC_STATUS_OK = 0,

	/** Error indicating failure to register IPC service for the Busy queue. */
	WIFI_IPC_STATUS_INIT_ERR,

	/** Error indicating that the Free queue has not been initialized. */
	WIFI_IPC_STATUS_FREEQ_UNINIT_ERR,

	/** Status indicating that the Free queue is empty. */
	WIFI_IPC_STATUS_FREEQ_EMPTY,

	/** Error indicating that the value passed to wifi_ipc_busyq_send()
	 * does not match the value from the Free queue.
	 */
	WIFI_IPC_STATUS_FREEQ_INVALID,

	/** Status indicating that the Free queue is full. */
	WIFI_IPC_STATUS_FREEQ_FULL,

	/** Error indicating that the IPC service for the Busy queue connection
	 * has not been established.
	 */
	WIFI_IPC_STATUS_BUSYQ_NOTREADY,

	/** Status indicating that the Busy queue is full. */
	WIFI_IPC_STATUS_BUSYQ_FULL,

	/** Critical error indicating an IPC transfer failure. This should never happen. */
	WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR,
} wifi_ipc_status_t;

/**
 * Structure to hold context information for busy queue.
 */
typedef struct {
	const ipc_device_wrapper_t *ipc_inst;
	struct ipc_ept ipc_ep;
	struct ipc_ept_cfg ipc_ep_cfg;
	void (*recv_cb)(const void *data, const void *priv);
	const void *priv;
	volatile bool ipc_ready;
} wifi_ipc_busyq_t;

/**
 * Top-level structure to hold context information for sending data between RPU
 * and the Host.
 */
typedef struct {
	spsc_queue_t *free_q;

	wifi_ipc_busyq_t busy_q;
	wifi_ipc_busyq_t *linked_ipc;
} wifi_ipc_t;

/**
 * Performs memory-to-memory copy via MVDMA.
 *
 * Enters low power state by issuing wait-for-interrupt (WFI) while waiting for
 * MVDMA event to complete.
 *
 * @param[in] p_dest : Pointer to destination memory to be copied to.
 * @param[in] p_src : Pointer to source memory to be copied from.
 * @param[in] len : Number of bytes to be copied.
 */
void wifi_ipc_mvdma_copy(void *p_dest, const void *p_src, size_t len);

/**
 * Bind either TX or RX context to one IPC service. This utilises the half-duplex
 * capability of the IPC service.
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[in] ipc_inst : Pointer to the IPC instance.
 * @param[in] rx_cb : If binding RX context, this is the callback function.
 *                    Leave NULL if binding to a TX.
 * @param[in] priv : If binding RX context, this is the private data to be passed
 *                   along with the callback function.
 *                   Leave NULL if binding to a TX.
 * @return : wifi_ipc_status_ok if successful, otherwise wifi_ipc_status_init_err.
 */
wifi_ipc_status_t wifi_ipc_bind_ipc_service(wifi_ipc_t *p_context,
						const ipc_device_wrapper_t *ipc_inst,
						void (*rx_cb)(void *data, void *priv), void *priv);

/**
 * Bind both TX and RX contexts to a single IPC service. This utilises the
 * full-duplex capability of the IPC service.
 *
 * @param[in] p_tx : Pointer to wifi_ipc_t struct to bind IPC TX mailbox to.
 * @param[in] p_rx : Pointer to wifi_ipc_t struct to bind IPC RX mailbox to.
 * @param[in] ipc_inst : Pointer to the IPC instance.
 * @param[in] rx_cb : Callback function to bind to data in received from RX mailbox.
 * @param[in] priv : Private data to the callback function.
 * @return : wifi_ipc_status_ok if successful, otherwise wifi_ipc_status_init_err.
 */
wifi_ipc_status_t wifi_ipc_bind_ipc_service_tx_rx(wifi_ipc_t *p_tx, wifi_ipc_t *p_rx,
						  const ipc_device_wrapper_t *ipc_inst,
						  void (*rx_cb)(void *data, void *priv),
						  void *priv);

/**
 * Get data from the free queue.
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[out] data : Pointer to the data to read to.
 * @return : wifi_ipc_status_ok if successful, otherwise wifi_ipc_status_freeq_empty.
 */
wifi_ipc_status_t wifi_ipc_freeq_get(wifi_ipc_t *p_context, uint32_t *data);

/**
 * Send data to the free queue.
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[in] data : 32-bit data to send.
 * @return : wifi_ipc_status_ok if successful, otherwise wifi_ipc_status_freeq_full.
 */
wifi_ipc_status_t wifi_ipc_freeq_send(wifi_ipc_t *p_context, uint32_t data);

/**
 * Send data to the busy queue over IPC service, and pop the same data from the
 * free queue.
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[in] data : Pointer to the data to send to.
 * @return : wifi_ipc_status_ok if successful, otherwise one of the following:
 *              - wifi_ipc_status_busyq_notready
 *              - wifi_ipc_status_busyq_full
 *              - wifi_ipc_status_busyq_critical_err
 *              - wifi_ipc_status_freeq_invalid
 *              - wifi_ipc_status_freeq_empty
 */
wifi_ipc_status_t wifi_ipc_busyq_send(wifi_ipc_t *p_context, uint32_t *data);

/**
 * Prepares and initialises the Host for sending a command to RPU.
 *
 * The free queue points to the already allocated free queue from the RPU.
 *
 * The busy queue using IPC service must be initialised using @see wifi_ipc_bind_ipc_service()
 * or @see wifi_ipc_bind_ipc_service_tx_rx().
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[in] addr_freeq : Address of the allocated free queue.
 * @return : wifi_ipc_status_ok if successful.
 */
wifi_ipc_status_t wifi_ipc_host_cmd_init(wifi_ipc_t *p_context, uint32_t addr_freeq);

/**
 * Prepares and initialises the Host for receiving an event from RPU.
 *
 * The free queue points to the already allocated free queue from the RPU.
 *
 * The busy queue using IPC service must be initialised using @see wifi_ipc_bind_ipc_service()
 * or @see wifi_ipc_bind_ipc_service_tx_rx().
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[in] addr_freeq : Address of the allocated SPSC free queue.
 * @return : wifi_ipc_status_ok if successful.
 */
wifi_ipc_status_t wifi_ipc_host_event_init(wifi_ipc_t *p_context, uint32_t addr_freeq);

/**
 * Get a command location from the free queue.
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[out] p_data : Pointer to data to write event location to.
 * @return : wifi_ipc_status_ok if successful, otherwise wifi_ipc_status_freeq_empty.
 */
wifi_ipc_status_t wifi_ipc_host_cmd_get(wifi_ipc_t *p_context, uint32_t *p_data);

/**
 * Send an event location pointer to the Host and frees up the event location
 * pointer from the free queue.
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[in] p_data : Pointer to command location to be sent.
 * @return : wifi_ipc_status_ok if successful, otherwise one of the following
 *              - wifi_ipc_status_busyq_notready
 *              - wifi_ipc_status_busyq_full
 *              - wifi_ipc_status_busyq_critical_err
 *              - wifi_ipc_status_freeq_invalid
 *              - wifi_ipc_status_freeq_empty
 */
wifi_ipc_status_t wifi_ipc_host_cmd_send(wifi_ipc_t *p_context, uint32_t *p_data);

/**
 * Send a command from the Host to RPU using standard memcpy.
 *
 * 1. Retrieves an address pointer of Packet RAM from the free queue.
 * 2. Copies local message to the retrieved address pointer via memcpy.
 * 3. Sends the address pointer to the busy queue via IPC service.
 * 4. Upon successful transmission, removes the address pointer the free queue.
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[in] p_msg : Pointer the local message to be copied to Packet RAM via memcpy.
 * @param[in] len : Length of the local message in bytes.
 * @return : wifi_ipc_status_ok if send is successful, otherwise one of
 *           status code from wifi_ipc_status_t will be returned.
 */
wifi_ipc_status_t wifi_ipc_host_cmd_send_memcpy(wifi_ipc_t *p_context, const void *p_msg,
						size_t len);

/**
 * Send a tx data pointer from the Host to RPU and raises RPU interrupt.
 *
 * @param[in] p_context : Pointer to wifi_ipc_t struct.
 * @param[in] p_msg : Pointer to message.
 * @return : wifi_ipc_status_ok if send is successful, otherwise one of
 *           status code from wifi_ipc_status_t will be returned.
 */
wifi_ipc_status_t wifi_ipc_host_tx_send(wifi_ipc_t *p_context, const void *p_msg);

#endif /* IPC_SERVICE_H */
