/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This library uses CMUX to create multiple data channels, called DLCIs, on a single serial bus.
 * Each DLCI has an address from 1 to 63. DLCI address 0 is reserved for control commands.
 *
 * Design overview:
 *
 *     DLCI1 <-----------+                              +-------> DLCI1
 *                       v                              v
 *     DLCI2 <---> CMUX instance <--> Serial bus <--> Client <--> DLCI2
 *                       ^                              ^
 *     DLCI3 <-----------+                              +-------> DLCI3
 *
 * Writing to and from the CMUX instances is done using the modem_pipe API.
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/modem/pipe.h>

#ifndef ZEPHYR_MODEM_CMUX_
#define ZEPHYR_MODEM_CMUX_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Modem CMUX
 * @defgroup modem_cmux Modem CMUX
 * @ingroup modem
 * @{
 */

struct modem_cmux;

enum modem_cmux_event {
	MODEM_CMUX_EVENT_CONNECTED = 0,
	MODEM_CMUX_EVENT_DISCONNECTED,
};

typedef void (*modem_cmux_callback)(struct modem_cmux *cmux, enum modem_cmux_event event,
				    void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 */

enum modem_cmux_state {
	MODEM_CMUX_STATE_DISCONNECTED = 0,
	MODEM_CMUX_STATE_CONNECTING,
	MODEM_CMUX_STATE_CONNECTED,
	MODEM_CMUX_STATE_DISCONNECTING,
};

enum modem_cmux_receive_state {
	MODEM_CMUX_RECEIVE_STATE_SOF = 0,
	MODEM_CMUX_RECEIVE_STATE_RESYNC_0,
	MODEM_CMUX_RECEIVE_STATE_RESYNC_1,
	MODEM_CMUX_RECEIVE_STATE_RESYNC_2,
	MODEM_CMUX_RECEIVE_STATE_RESYNC_3,
	MODEM_CMUX_RECEIVE_STATE_ADDRESS,
	MODEM_CMUX_RECEIVE_STATE_ADDRESS_CONT,
	MODEM_CMUX_RECEIVE_STATE_CONTROL,
	MODEM_CMUX_RECEIVE_STATE_LENGTH,
	MODEM_CMUX_RECEIVE_STATE_LENGTH_CONT,
	MODEM_CMUX_RECEIVE_STATE_DATA,
	MODEM_CMUX_RECEIVE_STATE_FCS,
	MODEM_CMUX_RECEIVE_STATE_DROP,
	MODEM_CMUX_RECEIVE_STATE_EOF,
};

enum modem_cmux_dlci_state {
	MODEM_CMUX_DLCI_STATE_CLOSED,
	MODEM_CMUX_DLCI_STATE_OPENING,
	MODEM_CMUX_DLCI_STATE_OPEN,
	MODEM_CMUX_DLCI_STATE_CLOSING,
};

struct modem_cmux_dlci {
	sys_snode_t node;

	/* Pipe */
	struct modem_pipe pipe;

	/* Context */
	uint16_t dlci_address;
	struct modem_cmux *cmux;

	/* Receive buffer */
	struct ring_buf receive_rb;
	struct k_mutex receive_rb_lock;

	/* Work */
	struct k_work_delayable open_work;
	struct k_work_delayable close_work;

	/* State */
	enum modem_cmux_dlci_state state;
};

struct modem_cmux_frame {
	uint16_t dlci_address;
	bool cr;
	bool pf;
	uint8_t type;
	const uint8_t *data;
	uint16_t data_len;
};

struct modem_cmux_work {
	struct k_work_delayable dwork;
	struct modem_cmux *cmux;
};

struct modem_cmux {
	/* Bus pipe */
	struct modem_pipe *pipe;

	/* Event handler */
	modem_cmux_callback callback;
	void *user_data;

	/* DLCI channel contexts */
	sys_slist_t dlcis;

	/* State */
	enum modem_cmux_state state;
	bool flow_control_on;

	/* Receive state*/
	enum modem_cmux_receive_state receive_state;

	/* Receive buffer */
	uint8_t *receive_buf;
	uint16_t receive_buf_size;
	uint16_t receive_buf_len;

	/* Transmit buffer */
	struct ring_buf transmit_rb;
	struct k_mutex transmit_rb_lock;

	/* Received frame */
	struct modem_cmux_frame frame;
	uint8_t frame_header[5];
	uint16_t frame_header_len;

	/* Work */
	struct k_work_delayable receive_work;
	struct k_work_delayable transmit_work;
	struct k_work_delayable connect_work;
	struct k_work_delayable disconnect_work;

	/* Synchronize actions */
	struct k_event event;
};

/**
 * @endcond
 */

/**
 * @brief Contains CMUX instance configuration data
 */
struct modem_cmux_config {
	/** Invoked when event occurs */
	modem_cmux_callback callback;
	/** Free to use pointer passed to event handler when invoked */
	void *user_data;
	/** Receive buffer */
	uint8_t *receive_buf;
	/** Size of receive buffer in bytes [127, ...] */
	uint16_t receive_buf_size;
	/** Transmit buffer */
	uint8_t *transmit_buf;
	/** Size of transmit buffer in bytes [149, ...] */
	uint16_t transmit_buf_size;
};

/**
 * @brief Initialize CMUX instance
 * @param cmux CMUX instance
 * @param config Configuration to apply to CMUX instance
 */
void modem_cmux_init(struct modem_cmux *cmux, const struct modem_cmux_config *config);

/**
 * @brief CMUX DLCI configuration
 */
struct modem_cmux_dlci_config {
	/** DLCI channel address */
	uint8_t dlci_address;
	/** Receive buffer used by pipe */
	uint8_t *receive_buf;
	/** Size of receive buffer used by pipe [127, ...] */
	uint16_t receive_buf_size;
};

/**
 * @brief Initialize DLCI instance and register it with CMUX instance
 *
 * @param cmux CMUX instance which the DLCI will be registered to
 * @param dlci DLCI instance which will be registered and configured
 * @param config Configuration to apply to DLCI instance
 */
struct modem_pipe *modem_cmux_dlci_init(struct modem_cmux *cmux, struct modem_cmux_dlci *dlci,
					const struct modem_cmux_dlci_config *config);

/**
 * @brief Attach CMUX instance to pipe
 *
 * @param cmux CMUX instance
 * @param pipe Pipe instance to attach CMUX instance to
 */
int modem_cmux_attach(struct modem_cmux *cmux, struct modem_pipe *pipe);

/**
 * @brief Connect CMUX instance
 *
 * @details This will send a CMUX connect request to target on the serial bus. If successful,
 * DLCI channels can be now be opened using modem_pipe_open()
 *
 * @param cmux CMUX instance
 *
 * @note When connected, the bus pipe must not be used directly
 */
int modem_cmux_connect(struct modem_cmux *cmux);

/**
 * @brief Connect CMUX instance asynchronously
 *
 * @details This will send a CMUX connect request to target on the serial bus. If successful,
 * DLCI channels can be now be opened using modem_pipe_open().
 *
 * @param cmux CMUX instance
 *
 * @note When connected, the bus pipe must not be used directly
 */
int modem_cmux_connect_async(struct modem_cmux *cmux);

/**
 * @brief Close down and disconnect CMUX instance
 *
 * @details This will close all open DLCI channels, and close down the CMUX connection.
 *
 * @param cmux CMUX instance
 *
 * @note The bus pipe must be released using modem_cmux_release() after disconnecting
 * before being reused.
 */
int modem_cmux_disconnect(struct modem_cmux *cmux);

/**
 * @brief Close down and disconnect CMUX instance asynchronously
 *
 * @details This will close all open DLCI channels, and close down the CMUX connection.
 *
 * @param cmux CMUX instance
 *
 * @note The bus pipe must be released using modem_cmux_release() after disconnecting
 * before being reused.
 */
int modem_cmux_disconnect_async(struct modem_cmux *cmux);

/**
 * @brief Release CMUX instance from pipe
 *
 * @details Releases the pipe and hard resets the CMUX instance internally. CMUX should
 * be disconnected using modem_cmux_disconnect().
 *
 * @param cmux CMUX instance
 *
 * @note The bus pipe can be used directly again after CMUX instance is released.
 */
void modem_cmux_release(struct modem_cmux *cmux);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_CMUX_ */
