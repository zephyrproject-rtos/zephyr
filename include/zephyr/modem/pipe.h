/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#ifndef ZEPHYR_MODEM_PIPE_
#define ZEPHYR_MODEM_PIPE_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Modem Pipe
 * @defgroup modem_pipe Modem Pipe
 * @ingroup modem
 * @{
 */

/** Modem pipe event */
enum modem_pipe_event {
	MODEM_PIPE_EVENT_OPENED = 0,
	MODEM_PIPE_EVENT_RECEIVE_READY,
	MODEM_PIPE_EVENT_TRANSMIT_IDLE,
	MODEM_PIPE_EVENT_CLOSED,
};

/**
 * @cond INTERNAL_HIDDEN
 */

struct modem_pipe;

/**
 * @endcond
 */

typedef void (*modem_pipe_api_callback)(struct modem_pipe *pipe, enum modem_pipe_event event,
					void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 */

typedef int (*modem_pipe_api_open)(void *data);

typedef int (*modem_pipe_api_transmit)(void *data, const uint8_t *buf, size_t size);

typedef int (*modem_pipe_api_receive)(void *data, uint8_t *buf, size_t size);

typedef int (*modem_pipe_api_close)(void *data);

struct modem_pipe_api {
	modem_pipe_api_open open;
	modem_pipe_api_transmit transmit;
	modem_pipe_api_receive receive;
	modem_pipe_api_close close;
};

enum modem_pipe_state {
	MODEM_PIPE_STATE_CLOSED = 0,
	MODEM_PIPE_STATE_OPEN,
};

struct modem_pipe {
	void *data;
	struct modem_pipe_api *api;
	modem_pipe_api_callback callback;
	void *user_data;
	enum modem_pipe_state state;
	struct k_mutex lock;
	struct k_condvar condvar;
	uint8_t receive_ready_pending : 1;
	uint8_t transmit_idle_pending : 1;
};

/**
 * @brief Initialize a modem pipe
 *
 * @param pipe Pipe instance to initialize
 * @param data Pipe data to bind to pipe instance
 * @param api Pipe API implementation to bind to pipe instance
 */
void modem_pipe_init(struct modem_pipe *pipe, void *data, struct modem_pipe_api *api);

/**
 * @endcond
 */

/**
 * @brief Open pipe
 *
 * @param pipe Pipe instance
 *
 * @retval 0 if pipe was successfully opened or was already open
 * @retval -errno code otherwise
 */
int modem_pipe_open(struct modem_pipe *pipe);

/**
 * @brief Open pipe asynchronously
 *
 * @param pipe Pipe instance
 *
 * @note The MODEM_PIPE_EVENT_OPENED event is invoked immediately if pipe is
 * already opened.
 *
 * @retval 0 if pipe open was called successfully or pipe was already open
 * @retval -errno code otherwise
 */
int modem_pipe_open_async(struct modem_pipe *pipe);

/**
 * @brief Attach pipe to callback
 *
 * @param pipe Pipe instance
 * @param callback Callback called when pipe event occurs
 * @param user_data Free to use user data passed with callback
 *
 * @note The MODEM_PIPE_EVENT_RECEIVE_READY event is invoked immediately if pipe has pending
 * data ready to receive.
 */
void modem_pipe_attach(struct modem_pipe *pipe, modem_pipe_api_callback callback, void *user_data);

/**
 * @brief Transmit data through pipe
 *
 * @param pipe Pipe to transmit through
 * @param buf Destination for reveived data
 * @param size Capacity of destination for recevied data
 *
 * @return Number of bytes placed in pipe
 *
 * @warning This call must be non-blocking
 */
int modem_pipe_transmit(struct modem_pipe *pipe, const uint8_t *buf, size_t size);

/**
 * @brief Reveive data through pipe
 *
 * @param pipe Pipe to receive from
 * @param buf Destination for reveived data
 * @param size Capacity of destination for recevied data
 *
 * @return Number of bytes received from pipe if any
 * @return -EPERM if pipe is closed
 * @return -errno code on error
 *
 * @warning This call must be non-blocking
 */
int modem_pipe_receive(struct modem_pipe *pipe, uint8_t *buf, size_t size);

/**
 * @brief Clear callback
 *
 * @param pipe Pipe instance
 */
void modem_pipe_release(struct modem_pipe *pipe);

/**
 * @brief Close pipe
 *
 * @param pipe Pipe instance
 *
 * @retval 0 if pipe open was called closed or pipe was already closed
 * @retval -errno code otherwise
 */
int modem_pipe_close(struct modem_pipe *pipe);

/**
 * @brief Close pipe asynchronously
 *
 * @param pipe Pipe instance
 *
 * @note The MODEM_PIPE_EVENT_CLOSED event is invoked immediately if pipe is
 * already closed.
 *
 * @retval 0 if pipe close was called successfully or pipe was already closed
 * @retval -errno code otherwise
 */
int modem_pipe_close_async(struct modem_pipe *pipe);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief Notify user of pipe that it has opened
 *
 * @param pipe Pipe instance
 *
 * @note Invoked from instance which initialized the pipe instance
 */
void modem_pipe_notify_opened(struct modem_pipe *pipe);

/**
 * @brief Notify user of pipe that it has closed
 *
 * @param pipe Pipe instance
 *
 * @note Invoked from instance which initialized the pipe instance
 */
void modem_pipe_notify_closed(struct modem_pipe *pipe);

/**
 * @brief Notify user of pipe that data is ready to be received
 *
 * @param pipe Pipe instance
 *
 * @note Invoked from instance which initialized the pipe instance
 */
void modem_pipe_notify_receive_ready(struct modem_pipe *pipe);

/**
 * @brief Notify user of pipe that pipe has no more data to transmit
 *
 * @param pipe Pipe instance
 *
 * @note Invoked from instance which initialized the pipe instance
 */
void modem_pipe_notify_transmit_idle(struct modem_pipe *pipe);

/**
 * @endcond
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_PIPE_ */
