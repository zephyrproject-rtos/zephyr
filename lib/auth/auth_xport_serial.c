/**
 * Copyright (c) 2021 Golden Bits Software, Inc.
 *
 *  @file  auth_xport_serial.c
 *
 *  @brief  Lower serial transport layer.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <init.h>
#include <sys/atomic.h>
#include <drivers/uart.h>

#include <auth/auth_lib.h>
#include <auth/auth_xport.h>

#include "auth_internal.h"


#define LOG_LEVEL CONFIG_AUTH_LOGLEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(auth_serial_xport, CONFIG_AUTH_LOG_LEVEL);


#define SERIAL_LINK_MTU                     (600u)
#define SERIAL_XP_BUFFER_LEN                SERIAL_LINK_MTU
#define NUM_BUFFERS                         (7u)

/* Number of events on the msg queue. NOTE: This queue is shared by all instances. */
#define RX_EVENT_MSGQ_COUNT                 (10u)

#define MAX_SERIAL_INSTANCES                (3u)
#define SERIAL_XP_RECV_THRD_PRIORITY        (0)
#define SERIAL_XP_RECV_STACK_SIZE           (1024u)

#define SERIAL_TX_WAIT_MSEC                 (1500u)



/* Serial transport instance */
struct serial_xp_instance {
	bool in_use;
	auth_xport_hdl_t xport_hdl;

	/* The UART device instance */
	const struct device *uart_dev;

	/* Current transmit buffer */
	uint8_t *tx_buf;
	uint16_t tx_bytes;      /* number of bytes to send */
	uint16_t curr_tx_cnt;   /* current tx send count */

	/* Semaphore for TX */
	struct k_sem tx_sem;

	/* current rx buffer */
	uint8_t *rx_buf;
	uint16_t curr_rx_cnt;
};


/* Buffer used for TX/RX */
struct serial_xp_buffer {
	bool in_use;
	uint32_t bufidx; /* Buffer bit index. */
	uint8_t buffer[SERIAL_XP_BUFFER_LEN];
};

/**
 * Used to pass bytes to the receive thread from the
 * UART interrupt routine.
 */
struct serial_recv_event {

	struct serial_xp_instance *serial_inst;
	uint8_t *rx_buf;
	uint16_t frag_offset;
	uint16_t frag_len;

	/* struct needs to be multiple of 4 bytes to work with the
	 * message queue, add padding here */
	uint8_t pad[4];
};


static struct serial_xp_instance serial_xp_inst[CONFIG_NUM_AUTH_INSTANCES];


/**
 * Serial receive thread.  Used to forward bytes from the ISR to the
 * transport receive queue.  One thread used for all serial instances.
 */
static void auth_xp_serial_recv_thrd(void *arg1, void *arg2, void *arg3);

K_THREAD_DEFINE(serial_recv, SERIAL_XP_RECV_STACK_SIZE, auth_xp_serial_recv_thrd,
		NULL, NULL, NULL, SERIAL_XP_RECV_THRD_PRIORITY, 0, 0);

/**
 * Receive event message queue
 */
K_MSGQ_DEFINE(recv_event_queue, sizeof(struct serial_recv_event), RX_EVENT_MSGQ_COUNT, 4);


/* Atomic bits to determine if a buffer is in use.  If bit is set
 * buffer is in use. */
ATOMIC_DEFINE(buffer_in_use, NUM_BUFFERS);


/**
 * Serial buffer pool, used across all serial instances.
 *
 * @note: This is a simple implementation of a buffer pool.  Another
 *        option considered was creating a memory slab, this buffer
 *        pool was selected because of its simplicity.
 *
 * @note:  There is a trade-off between the number of buffers and the
 *         serial MTU size (SERIAL_LINK_MTU).  The larger the MTU size the
 *         fewer buffers needed.
 */
static struct serial_xp_buffer serial_xp_bufs[NUM_BUFFERS] = {
	{ .in_use = false, .bufidx = 0 },
	{ .in_use = false, .bufidx = 1 },
	{ .in_use = false, .bufidx = 2 },
	{ .in_use = false, .bufidx = 3 },
	{ .in_use = false, .bufidx = 4 },
	{ .in_use = false, .bufidx = 5 },
	{ .in_use = false, .bufidx = 6 }
};


/**
 * Returns information about a serial buffer.
 *
 * @param buf Pointer to buffer.
 *
 * @return  Pointer to serial buffer info.
 */
static struct serial_xp_buffer *serial_xp_buffer_info(const uint8_t *buf)
{
	if (buf == NULL) {
		return NULL;
	}

	/* get pointer to containing struct*/
	struct serial_xp_buffer *xp_buf =
		(struct serial_xp_buffer *)CONTAINER_OF(buf, struct serial_xp_buffer, buffer);

	return xp_buf;
}


/**
 * Gets a free buffer.
 *
 * @param buflen  Requested buffer size, if too large NULL return.
 *
 * @return  Pointer to buffer, else NULL on error.
 */
static uint8_t *serial_xp_get_buffer(uint32_t buflen)
{
	int cnt;

	if (buflen > SERIAL_LINK_MTU) {
		LOG_ERR("Buffer request too large: %d", buflen);
		return NULL;
	}

	/* check array of tx buffers */
	for (cnt = 0; cnt < NUM_BUFFERS; cnt++) {
		if (!atomic_test_and_set_bit(buffer_in_use, cnt)) {
			serial_xp_bufs[cnt].in_use = true;
			return serial_xp_bufs[cnt].buffer;
		}
	}

	return NULL;
}


/**
 * Free buffer.
 *
 * @param buffer  Buffer to free.
 */
static void serial_xp_free_buffer(const uint8_t *buffer)
{
	if (buffer == NULL) {
		return;
	}
	struct serial_xp_buffer *xp_buffer = serial_xp_buffer_info(buffer);

	xp_buffer->in_use = false;
	atomic_clear_bit(buffer_in_use, xp_buffer->bufidx);
}


/**
 * Gets a serial transport instance.
 *
 * @return Pointer to serial transport, else NULL on error.
 */
static struct serial_xp_instance *auth_xp_serial_get_instance(void)
{
	uint32_t cnt;

	for (cnt = 0; cnt < MAX_SERIAL_INSTANCES; cnt++) {

		if (!serial_xp_inst[cnt].in_use) {
			serial_xp_inst[cnt].in_use = true;
			return &serial_xp_inst[cnt];
		}
	}

	return NULL;
}

/**
 * Free serial transport instance.
 *
 * @param serial_inst  Pointer to serial transport instance.
 */
static void auth_xp_serial_free_instance(struct serial_xp_instance *serial_inst)
{
	if (serial_inst != NULL) {

		serial_inst->in_use = false;
		serial_inst->xport_hdl = NULL;
		serial_inst->uart_dev = NULL;

		/* free tx and rx buffers */
		if (serial_inst->tx_buf != NULL) {
			serial_xp_free_buffer(serial_inst->tx_buf);
			serial_inst->tx_buf = NULL;
		}

		if (serial_inst->rx_buf != NULL) {
			serial_xp_free_buffer(serial_inst->rx_buf);
			serial_inst->rx_buf = NULL;
		}

		serial_inst->tx_bytes = 0;
		serial_inst->curr_tx_cnt = 0;
		serial_inst->curr_rx_cnt = 0;
	}
}


/**
 * UART receive thread. Handles bytes from the UART interrupt routine
 * and forwards to the common transport receive queue.
 *
 * Handles receive byes from all of the serial instances.
 *
 */
static void auth_xp_serial_recv_thrd(void *arg1, void *arg2, void *arg3)
{
	struct serial_recv_event recv_event;

	/* unused */
	(void)arg1;
	(void)arg2;
	(void)arg3;

	while (true) {

		if (k_msgq_get(&recv_event_queue, &recv_event, K_FOREVER) == 0) {

			auth_message_assemble(recv_event.serial_inst->xport_hdl,
					      recv_event.rx_buf + recv_event.frag_offset,
					      recv_event.frag_len);

			/* free RX buffer */
			serial_xp_free_buffer(recv_event.rx_buf);
		}

	} /* end while() */

}


/**
 * Reads bytes from UART into a rx buffer.  When enough bytes have
 * accumulated to fill a message fragment, put fragment onto message queue.
 * The serial RX thread will read this message and forward bytes to upper
 * layer transport.
 *
 * This function is called in an ISR context.
 *
 * @param uart     Pointer to UART device
 * @param xp_inst  Pointer to serial transport instance.
 *
 */
static void auth_xp_serial_irq_recv_fragment(struct serial_xp_instance *xp_inst)
{
	int num_bytes;
	int total_cnt;
	uint8_t *new_rxbuf = NULL;
	uint16_t frag_beg_offset;
	uint16_t frag_bytes;
	uint16_t remaining_buffer_bytes;
	struct serial_recv_event recv_event;

	if (xp_inst->rx_buf == NULL) {
		/* try to allocate buffer */
		xp_inst->rx_buf = serial_xp_get_buffer(SERIAL_XP_BUFFER_LEN);
		xp_inst->curr_rx_cnt = 0;

		if (xp_inst->rx_buf == NULL) {
			LOG_ERR("Out of free buffers for Rx.");
			return;
		}
	}

	/* Check if rx buffer is full, if so then something went wrong.
	 * Log and error and drop bytes received so far */
	if (xp_inst->curr_rx_cnt == SERIAL_XP_BUFFER_LEN) {
		xp_inst->curr_rx_cnt = 0;
		LOG_ERR("Receive buffer full, dropped %d bytes.", SERIAL_XP_BUFFER_LEN);
	}

	num_bytes = uart_fifo_read(xp_inst->uart_dev, xp_inst->rx_buf + xp_inst->curr_rx_cnt,
				   SERIAL_XP_BUFFER_LEN - xp_inst->curr_rx_cnt);
	total_cnt += num_bytes;

	xp_inst->curr_rx_cnt += num_bytes;


	/* Is there a full frame? */
	if (auth_message_get_fragment(xp_inst->rx_buf, xp_inst->curr_rx_cnt,
				      &frag_beg_offset, &frag_bytes)) {

		/* A full message fragment is present in the input buffer
		 * starting at frag_beg_offset and frag_bytes in length.
		 * It's possible to have the beginning of a second fragment
		 * following this first fragment. */

		/* get new rx buffer */
		new_rxbuf = serial_xp_get_buffer(SERIAL_XP_BUFFER_LEN);

		/* If there's garbage before the frame start,then skip.
		 * If there is another fragment or partial fragment following
		 * then copy to new buffer. 'remaining_buffer_bytes' is the
		 * number of valid bytes after the current fragment. */
		remaining_buffer_bytes = xp_inst->curr_rx_cnt - frag_beg_offset - frag_bytes;

		/* If fragment bytes are less than the current, then the buffer contains bytes
		 * for the next fragment. */
		if ((remaining_buffer_bytes != 0) && (new_rxbuf != NULL)) {
			/* copy extra bytes to new buffer */
			memcpy(new_rxbuf, xp_inst->rx_buf + frag_beg_offset + frag_bytes,
			       remaining_buffer_bytes);
		}


		/* Setup receive event */
		recv_event.rx_buf = xp_inst->rx_buf;
		recv_event.frag_offset = frag_beg_offset;
		recv_event.frag_len = frag_bytes;
		recv_event.serial_inst = xp_inst;

		/* send fragment to receive thread via message queue */
		if (k_msgq_put(&recv_event_queue, &recv_event, K_NO_WAIT) != 0) {
			/* an error occurred, free buffer */
			serial_xp_free_buffer(recv_event.rx_buf);
			LOG_ERR("Failed to queue recv event, dropping recv bytes.");
		}

		/* now setup new RX fragment */
		if (new_rxbuf != NULL) {
			xp_inst->rx_buf = new_rxbuf;
			xp_inst->curr_rx_cnt = remaining_buffer_bytes;
		} else {
			/* no free buffers */
			xp_inst->rx_buf = NULL;
			xp_inst->curr_rx_cnt = 0;
		}
	}
}

/**
 * For interrupt driven IO for serial link.
 * @param dev Pointer to UART device.
 * @param user_data  Used to store serial instance.
 */
static void auth_xp_serial_irq_cb(const struct device *uart_dev, void *user_data)
{
	int num_bytes;
	static int total_cnt = 0;

	enum uart_rx_stop_reason rx_stop;
	struct serial_xp_instance *xp_inst = (struct serial_xp_instance *) user_data;

	uart_irq_update(uart_dev);

	/* did an error happen */
	rx_stop = uart_err_check(uart_dev);

	if (rx_stop != 0) {
		/* An error, either:
		 *  UART_ERROR_OVERRUN, UART_ERROR_PARITY,
		 *  UART_ERROR_FRAMING, UART_ERROR_BREAK
		 */
		LOG_ERR("UART error: %d", rx_stop);
		return;
	}

	/* read any chars first */
	while (uart_irq_rx_ready(uart_dev)) {
		auth_xp_serial_irq_recv_fragment(xp_inst);
	}

	/* Any bytes ready to send? */
	if (xp_inst->tx_bytes == 0) {
		/* check if we can disable TX */
		if (uart_irq_tx_complete(uart_dev)) {
			uart_irq_tx_disable(uart_dev);
		}

		return;
	}

	total_cnt = 0;
	while (uart_irq_tx_ready(uart_dev) && xp_inst->tx_buf != NULL) {

		num_bytes = uart_fifo_fill(uart_dev,
					   xp_inst->tx_buf + xp_inst->curr_tx_cnt,
					   xp_inst->tx_bytes);

		/* check return can this be an error? */
		xp_inst->tx_bytes -= num_bytes;
		xp_inst->curr_tx_cnt += num_bytes;

		total_cnt += num_bytes;

		/* if no more data to send, then break */
		if (xp_inst->tx_bytes == 0) {
			LOG_INF("Sent tx buffer, bytes: %d.", xp_inst->curr_tx_cnt);
			break;
		}
	}

	/* we're done sending */
	if ((xp_inst->tx_bytes == 0) && (xp_inst->tx_buf != NULL)) {
		serial_xp_free_buffer(xp_inst->tx_buf);
		xp_inst->tx_buf = NULL;
		xp_inst->curr_tx_cnt = 0;

		/* signal TX complete */
		k_sem_give(&xp_inst->tx_sem);
	}
}


/**
 * Send bytes on the serial link.
 * @note: This call will block!!! Should not call from an ISR.
 *
 * @param xport_hdl  Transport handle.
 * @param data       Bytes to send.
 * @param len        Number of bytes to send.
 *
 * @return  Number of bytes sent on success, else negative error value.
 */
static int auth_xp_serial_send(auth_xport_hdl_t xport_hdl, const uint8_t *data,
			       const size_t len)
{
	if (len > SERIAL_LINK_MTU) {
		LOG_ERR("Too many bytes to send.");
		return AUTH_ERROR_INVALID_PARAM;
	}

	struct serial_xp_instance *serial_inst =
		(struct serial_xp_instance *)auth_xport_get_context(xport_hdl);

	/* is there a pending TX operation?  If so return busy error */
	if (serial_inst->tx_buf != NULL) {
		LOG_ERR("TX operation in process.");
		return -EAGAIN;
	}

	/* get free buffer for tx */
	serial_inst->tx_buf = serial_xp_get_buffer(len);

	if (serial_inst->tx_buf == NULL) {
		LOG_ERR("No free TX buffer.");
		serial_inst->tx_bytes = 0;
		serial_inst->curr_tx_cnt = 0;
		return AUTH_ERROR_NO_RESOURCE;
	}

	/* fill buffer, set as _in use */
	memcpy(serial_inst->tx_buf, data, len);
	serial_inst->tx_bytes = len;
	serial_inst->curr_tx_cnt = 0;

	/* should kick off an interrupt */
	uart_irq_tx_enable(serial_inst->uart_dev);

	LOG_INF("Started TX operation");

	/* Wait on semaphore for TX to complete */
	int ret = k_sem_take(&serial_inst->tx_sem, K_MSEC(SERIAL_TX_WAIT_MSEC));

	if (ret) {
		return ret;
	}

	return (int)len;
}


/**
 * @see auth_xport.h
 */
int auth_xp_serial_init(const auth_xport_hdl_t xport_hdl, uint32_t flags,
			void *xport_param)
{
	struct auth_xp_serial_params *serial_param =
		(struct auth_xp_serial_params *)xport_param;

	struct serial_xp_instance *serial_inst = auth_xp_serial_get_instance();

	if (serial_inst == NULL) {
		LOG_ERR("No free serial xport instances.");
		return AUTH_ERROR_NO_RESOURCE;
	}

	serial_inst->xport_hdl = xport_hdl;
	serial_inst->uart_dev = serial_param->uart_dev;

	/* set serial irq callback */
	uart_irq_callback_user_data_set(serial_inst->uart_dev,
					auth_xp_serial_irq_cb,
					serial_inst);

	/* set context into xport handle */
	auth_xport_set_context(xport_hdl, serial_inst);

	auth_xport_set_sendfunc(xport_hdl, auth_xp_serial_send);

	/* Init semaphore used to wait for TX to complete. */
	k_sem_init(&serial_inst->tx_sem, 0, 1);

	/* reset tx vars */
	serial_inst->tx_buf = NULL;
	serial_inst->tx_bytes = 0;
	serial_inst->curr_tx_cnt = 0;

	/* get rx buffer */
	serial_inst->rx_buf = serial_xp_get_buffer(SERIAL_XP_BUFFER_LEN);
	serial_inst->curr_rx_cnt = 0;

	/* enable rx interrupts */
	uart_irq_rx_enable(serial_param->uart_dev);

	/* enable error irq */
	uart_irq_err_enable(serial_param->uart_dev);

	return AUTH_SUCCESS;
}

/**
 * @see auth_xport.h
 */
int auth_xp_serial_deinit(const auth_xport_hdl_t xport_hdl)
{
	struct serial_xp_instance *serial_inst = auth_xport_get_context(xport_hdl);

	auth_xp_serial_free_instance(serial_inst);

	auth_xport_set_context(xport_hdl, NULL);

	return AUTH_SUCCESS;
}


/**
 * @see auth_xport.h
 */
int auth_xp_serial_event(const auth_xport_hdl_t xporthdl,
			 struct auth_xport_evt *event)
{
	/* stub for now */
	return AUTH_ERROR_INTERNAL;
}

/**
 * @see auth_xport.h
 */
int auth_xp_serial_get_max_payload(const auth_xport_hdl_t xporthdl)
{
	return SERIAL_LINK_MTU;
}





