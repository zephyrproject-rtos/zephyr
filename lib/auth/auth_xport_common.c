/**
 *  Copyright (c) 2021 Golden Bits Software, Inc.
 *
 *  @file  auth_xport_common.c
 *
 *  @brief  Common transport routines.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <init.h>
#include <random/rand32.h>

#define LOG_LEVEL CONFIG_AUTH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(auth_lib, CONFIG_AUTH_LOG_LEVEL);

#include <auth/auth_lib.h>
#include <auth/auth_xport.h>
#include "auth_internal.h"

/**
 * Size of a buffer used for Rx.
 */
#define XPORT_IOBUF_LEN      (4096u)


/**
 * @brief Circular buffer used to save received data.
 */
struct auth_xport_io_buffer {
	struct k_mutex buf_mutex;
	struct k_sem buf_sem;

	uint32_t head_index;
	uint32_t tail_index;
	uint32_t num_valid_bytes;

	uint8_t io_buffer[XPORT_IOBUF_LEN];
};


/**
 * Contains buffer used to assemble a message from multiple fragments.
 */
struct auth_message_recv {
	/* pointer to buffer where message is assembled */
	uint8_t rx_buffer[XPORT_MAX_MESSAGE_SIZE];

	/* vars used for re-assembling frames into a message */
	uint32_t rx_curr_offset;
	bool rx_first_frag;
};


/**
 * Transport instance, contains send and recv circular queues.
 */
struct auth_xport_instance {
	/* Send & Recv circular queues */
	struct auth_xport_io_buffer send_buf;
	struct auth_xport_io_buffer recv_buf;

	/* Lower transport type */
	enum auth_xport_type xport_type;

	void *xport_ctx; /* transport specific context */

	/* If the lower transport has a send function */
	send_xport_t send_func;

	/* Struct for handling assembling message from multiple fragments */
	struct auth_message_recv recv_msg;

	uint32_t payload_size; /* Max payload size for lower transport. */
};

/* transport instances */
static struct auth_xport_instance xport_inst[CONFIG_NUM_AUTH_INSTANCES];


/* ================ local static funcs ================== */

/**
 *  Initializes common transport IO buffers.
 *
 *  @param iobuf  Pointer to IO buffer to initialize.
 *
 *  @return 0 for success, else negative error value.
 */
static void auth_xport_iobuffer_init(struct auth_xport_io_buffer *iobuf)
{
	/* init mutex*/
	k_mutex_init(&iobuf->buf_mutex);

	/* init semaphore */
	k_sem_init(&iobuf->buf_sem, 0, 1);

	iobuf->head_index = 0;
	iobuf->tail_index = 0;
	iobuf->num_valid_bytes = 0;
}

/**
 * Reset queue counters
 *
 *  @param iobuf  Pointer to IO buffer to reset.
 *
 * @return 0 for success
 */
static void auth_xport_iobuffer_reset(struct auth_xport_io_buffer *iobuf)
{
	iobuf->head_index = 0;
	iobuf->tail_index = 0;
	iobuf->num_valid_bytes = 0;
}


/**
 * Puts data into the transport buffer.
 *
 * @param iobuf      IO buffer to add data.
 * @param in_buf     Data to put into IO buffer.
 * @param num_bytes  Number of bytes to put.
 *
 * @return On success, number of bytes put into IO buffer, can be less than requested.
 *         Negative number on error.
 */
static int auth_xport_buffer_put(struct auth_xport_io_buffer *iobuf,
				 const uint8_t *in_buf, size_t num_bytes)
{
	/* Is the buffer full? */
	if (iobuf->num_valid_bytes == XPORT_IOBUF_LEN) {
		return AUTH_ERROR_IOBUFF_FULL;
	}

	/* don't put zero bytes */
	if (num_bytes == 0) {
		return 0;
	}

	/* lock mutex */
	int err = k_mutex_lock(&iobuf->buf_mutex, K_FOREVER);

	if (err) {
		return err;
	}

	uint32_t free_space = XPORT_IOBUF_LEN - iobuf->num_valid_bytes;
	uint32_t copy_cnt = MIN(free_space, num_bytes);
	uint32_t total_copied = 0;
	uint32_t byte_cnt;

	if (iobuf->head_index < iobuf->tail_index) {
		/* only enough room from head to tail, don't over-write */
		uint32_t max_copy_cnt = iobuf->tail_index - iobuf->head_index;

		copy_cnt = MIN(max_copy_cnt, copy_cnt);

		memcpy(iobuf->io_buffer + iobuf->head_index, in_buf, copy_cnt);

		total_copied += copy_cnt;
		iobuf->head_index += copy_cnt;
		iobuf->num_valid_bytes += copy_cnt;

	} else {

		/* copy from head to end of buffer */
		byte_cnt = XPORT_IOBUF_LEN - iobuf->head_index;

		if (byte_cnt > copy_cnt) {
			byte_cnt = copy_cnt;
		}

		memcpy(iobuf->io_buffer + iobuf->head_index, in_buf, byte_cnt);

		total_copied += byte_cnt;
		in_buf += byte_cnt;
		copy_cnt -= byte_cnt;
		iobuf->head_index += byte_cnt;

		iobuf->num_valid_bytes += byte_cnt;

		/* if wrapped, then copy from beginning of buffer */
		if (copy_cnt > 0) {
			memcpy(iobuf->io_buffer, in_buf, copy_cnt);

			total_copied += copy_cnt;
			iobuf->num_valid_bytes += copy_cnt;
			iobuf->head_index = copy_cnt;
		}
	}

	/* unlock */
	k_mutex_unlock(&iobuf->buf_mutex);

	/* after putting data into buffer, signal semaphore */
	k_sem_give(&iobuf->buf_sem);

	return (int)total_copied;
}

/**
 * Used to read bytes from an io buffer.
 *
 * @param iobuf       IO buffer to read.
 * @param out_buf     Buffer to copy bytes into.
 * @param num_bytes   Number of bytes to read
 * @param peek        If true, then queue pointers are not updated.
 *
 * @return On success, number of bytes copied into buffer.  Can be less than requested.
 *         Negative number on error.
 */
static int auth_xport_buffer_get_internal(struct auth_xport_io_buffer *iobuf,
					  uint8_t *out_buf, size_t num_bytes, bool peek)
{
	/* if no valid bytes, just return zero */
	if (iobuf->num_valid_bytes == 0) {
		return 0;
	}

	/* lock mutex */
	int err = k_mutex_lock(&iobuf->buf_mutex, K_FOREVER);

	if (err) {
		return err;
	}

	/* number bytes to copy */
	uint32_t copy_cnt = MIN(iobuf->num_valid_bytes, num_bytes);
	uint32_t total_copied = 0;
	uint32_t byte_cnt = 0;

	if (iobuf->head_index <= iobuf->tail_index) {

		/* How may bytes are available? */
		byte_cnt = XPORT_IOBUF_LEN - iobuf->tail_index;

		if (byte_cnt > copy_cnt) {
			byte_cnt = copy_cnt;
		}

		/* copy from tail to end of buffer */
		memcpy(out_buf, iobuf->io_buffer + iobuf->tail_index, byte_cnt);

		if (!peek) {
			/* update tail index */
			iobuf->tail_index += byte_cnt;
		}

		out_buf += byte_cnt;
		total_copied += byte_cnt;

		/* update copy count and num valid bytes */
		copy_cnt -= byte_cnt;

		if (!peek) {
			iobuf->num_valid_bytes -= byte_cnt;
		}

		/* wrapped around, copy from beginning of buffer until
		 * copy_count is satisfied
		 */
		if (copy_cnt > 0) {

			memcpy(out_buf, iobuf->io_buffer, copy_cnt);

			if (!peek) {
				iobuf->tail_index = copy_cnt;
				iobuf->num_valid_bytes -= copy_cnt;
			}

			total_copied += copy_cnt;
		}

	} else if (iobuf->head_index > iobuf->tail_index) {

		byte_cnt = iobuf->head_index - iobuf->tail_index;

		if (byte_cnt > copy_cnt) {
			byte_cnt = copy_cnt;
		}

		memcpy(out_buf, iobuf->io_buffer + iobuf->tail_index, byte_cnt);

		total_copied += byte_cnt;
		copy_cnt -= byte_cnt;

		if (!peek) {
			iobuf->tail_index += byte_cnt;
			iobuf->num_valid_bytes -= byte_cnt;
		}
	}

	/* unlock */
	k_mutex_unlock(&iobuf->buf_mutex);

	return (int)total_copied;
}

/**
 * Peek at the contents of the input buffer.  Copies bytes but does not advance
 * the queue pointers.
 *
 * @param iobuf       IO buffer to peek.
 * @param out_buf     Buffer to copy bytes into.
 * @param num_bytes   Number of bytes to peek.
 *
 * @return On success, number of bytes copied into buffer.  Can be less than requested.
 *         Negative number on error.
 */
static int auth_xport_buffer_peek(struct auth_xport_io_buffer *iobuf,
				  uint8_t *out_buf, size_t num_bytes)
{
	return auth_xport_buffer_get_internal(iobuf, out_buf, num_bytes, true);
}

/**
 * Gets data from a IO buffer.
 *
 * @param iobuf       IO buffer to get data from.
 * @param out_buf     Buffer to copy bytes into.
 * @param num_bytes   Number of bytes requested.
 *
 * @return On success, number of bytes copied into buffer.  Can be less than requested.
 *         Negative number on error.
 */
static int auth_xport_buffer_get(struct auth_xport_io_buffer *iobuf,
				 uint8_t *out_buf, size_t num_bytes)
{
	return auth_xport_buffer_get_internal(iobuf, out_buf, num_bytes, false);
}

/**
 * Get data from an IO buffer, if no data present wait.
 *
 * @param iobuf      The IO buffer to get from.
 * @param out_buf    Buffer to copy bytes into.
 * @param num_bytes  Number of bytes requested.
 * @param waitmsec   Number of milliseconds to wait if no data.
 *
 * @return  On success, number of bytes copied, can be less than requested amount.
 *          Negative number on error. -EAGAIN if a timeout occurred.
 */
static int auth_xport_buffer_get_wait(struct auth_xport_io_buffer *iobuf,
				      uint8_t *out_buf,
				      int num_bytes, int waitmsec)
{
	/* return any bytes that might be sitting in the buffer */
	int bytecount = auth_xport_buffer_get(iobuf, out_buf, num_bytes);

	if (bytecount > 0) {
		/* bytes are avail, return them */
		return bytecount;
	}

	do {
		int err = k_sem_take(&iobuf->buf_sem, K_MSEC(waitmsec));

		if (err) {
			return err; /* timed out -EAGAIN or error */
		}

		/* return byte count or error (bytecount < 0) */
		bytecount = auth_xport_buffer_get(iobuf, out_buf, num_bytes);

	} while (bytecount == 0);

	return bytecount;
}

/**
 * Get the number of bytes in an IO buffer.
 *
 * @param iobuf  The IO buffer to check.
 *
 * @return On success, number of bytes.  Negative number on error.
 */
static int auth_xport_buffer_bytecount(struct auth_xport_io_buffer *iobuf)
{
	int err = k_mutex_lock(&iobuf->buf_mutex, K_FOREVER);

	if (!err) {
		err = (int)iobuf->num_valid_bytes;
	}

	/* unlock */
	k_mutex_unlock(&iobuf->buf_mutex);

	return err;
}


/**
 * Wait for a non-zero byte count.  Used to wait until data is received
 * from the sender. Wait until waitmsec has timed out or data has been received.
 *
 * @param iobuf     IO buffer to wait on bytes.
 * @param waitmsec  Number of milliseconds to wait.
 *
 * @return  On success, number of bytes in the IO buffer.
 *          -EAGAIN on timeout.
 */
static int auth_xport_buffer_bytecount_wait(struct auth_xport_io_buffer *iobuf,
					    uint32_t waitmsec)
{
	int num_bytes = auth_xport_buffer_bytecount(iobuf);

	/* an error occurred or there are bytes sitting in the io buffer. */
	if (num_bytes != 0) {
		return num_bytes;
	}

	/* wait for byte to fill the io buffer */
	int err = k_sem_take(&iobuf->buf_sem, K_MSEC(waitmsec));

	if (err) {
		return err; /* timed out -EAGAIN or error */
	}

	/* return the number of bytes in the queue */
	return auth_xport_buffer_bytecount(iobuf);
}


/**
 * Get the frees pace within an IO bufer.
 *
 * @param iobuf   IO Buffer to check.
 *
 * @return Amount of free space.
 */
static int auth_xport_buffer_avail_bytes(struct auth_xport_io_buffer *iobuf)
{
	return sizeof(iobuf->io_buffer) - auth_xport_buffer_bytecount(iobuf);
}

/**
 * Internal function to send data to peer
 *
 * @param xporthdl  Transport handle
 * @param data      Buffer to send.
 * @param len       Number of bytes to send.
 *
 * @return  Number of bytes sent on success, can be less than requested.
 *          On error, negative error code.
 */
static int auth_xport_internal_send(const auth_xport_hdl_t xporthdl,
				    const uint8_t *data, size_t len)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	/* if the lower transport set a send function, call it */
	if (xp_inst->send_func != NULL) {
		return xp_inst->send_func(xporthdl, data, len);
	}

	/* queue the send bytes into tx buffer */
	return auth_xport_buffer_put(&xp_inst->send_buf, data, len);
}

/**
 * Initializes message receive struct.  Used to re-assemble message
 * fragments received.
 *
 * @param recv_msg Received message buffer.
 */
static void auth_message_frag_init(struct auth_message_recv *recv_msg)
{
	recv_msg->rx_curr_offset = 0;
	recv_msg->rx_first_frag = true;
}


/**
 * Returns the lower transport type associated with the opaque
 * transport handle.
 *
 * @param xporthdl  The transport handle.
 *
 * @return Transport type
 */
static enum auth_xport_type auth_get_xport_type(auth_xport_hdl_t xporthdl)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	if (xp_inst == NULL) {
		return AUTH_XP_TYPE_NONE;
	}

	return xp_inst->xport_type;
}


/* ==================== Non static funcs ================== */

/**
 * @see auth_xport.h
 */
int auth_xport_init(auth_xport_hdl_t *xporthdl, enum auth_instance_id instance,
		    enum auth_xport_type xport_type, void *xport_params)
{
	int ret = -1;

	/* verify instance */
	if ((instance < 0) || (instance >= AUTH_MAX_INSTANCES)) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	if ((xport_type != AUTH_XP_TYPE_BLUETOOTH) &&
	    (xport_type != AUTH_XP_TYPE_SERIAL)) {
		return AUTH_ERROR_INVALID_PARAM;
	}


	*xporthdl = &xport_inst[instance];

	/* init IO buffers */
	auth_xport_iobuffer_init(&xport_inst[instance].send_buf);
	auth_xport_iobuffer_init(&xport_inst[instance].recv_buf);
	auth_message_frag_init(&xport_inst[instance].recv_msg);

	/* Set the lower transport type */
	xport_inst[instance].xport_type = xport_type;


#if defined(CONFIG_BT_XPORT)
	if (xport_type == AUTH_XP_TYPE_BLUETOOTH) {
		ret = auth_xp_bt_init(*xporthdl, 0, xport_params);
	}
#endif

#if defined(CONFIG_SERIAL_XPORT)
	if (xport_type == AUTH_XP_TYPE_SERIAL) {
		ret = auth_xp_serial_init(*xporthdl, 0, xport_params);
	}
#endif


	return ret;
}

/**
 * @see auth_xport.h
 */
int auth_xport_deinit(const auth_xport_hdl_t xporthdl)
{
	int ret = 0;
	enum auth_xport_type xport_type = AUTH_XP_TYPE_NONE;
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	if (xp_inst == NULL) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	xport_type = auth_get_xport_type(xporthdl);

	/* reset queues */
	auth_xport_iobuffer_reset(&xp_inst->send_buf);
	auth_xport_iobuffer_reset(&xp_inst->recv_buf);


#if defined(CONFIG_BT_XPORT)
	if (xport_type == AUTH_XP_TYPE_BLUETOOTH) {
		ret = auth_xp_bt_deinit(xporthdl);
	}
#endif

#if defined(CONFIG_SERIAL_XPORT)
	if (xport_type == AUTH_XP_TYPE_SERIAL) {
		ret = auth_xp_serial_deinit(xporthdl);
	}
#endif

	xp_inst->xport_type = AUTH_XP_TYPE_NONE;

	return ret;
}

/**
 * @see auth_xport.h
 */
int auth_xport_event(const auth_xport_hdl_t xporthdl, struct auth_xport_evt *event)
{
	int ret = 0;
	enum auth_xport_type xport_type = auth_get_xport_type(xporthdl);

#if defined(CONFIG_BT_XPORT)
	if (xport_type == AUTH_XP_TYPE_BLUETOOTH) {
		ret = auth_xp_bt_event(xporthdl, event);
	}
#endif

#if defined(CONFIG_SERIAL_XPORT)
	if (xport_type == AUTH_XP_TYPE_SERIAL) {
		ret = auth_xp_serial_event(xporthdl, event);
	}
#endif

	return ret;
}

/**
 * @see auth_xport.h
 */
int auth_xport_get_max_payload(const auth_xport_hdl_t xporthdl)
{
	int mtu = 0;
	enum auth_xport_type xport_type = auth_get_xport_type(xporthdl);

#if defined(CONFIG_BT_XPORT)
	if (xport_type == AUTH_XP_TYPE_BLUETOOTH) {
		mtu = auth_xp_bt_get_max_payload(xporthdl);
	}
#endif

#if defined(CONFIG_SERIAL_XPORT)
	if (xport_type == AUTH_XP_TYPE_SERIAL) {
		mtu = auth_xp_serial_get_max_payload(xporthdl);
	}
#endif
	return mtu;
}

/**
 * @see auth_xport.h
 */
int auth_xport_send(const auth_xport_hdl_t xporthdl, const uint8_t *data, size_t len)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;
	int fragment_bytes;
	int payload_bytes;
	int send_count = 0;
	int num_fragments = 0;
	int send_ret = AUTH_SUCCESS;
	struct auth_message_fragment msg_frag;


	/* If the lower transport MTU size isn't set, get it.  This can happen
	 * when the the MTU is negotiated after the initial connection.
	 */
	if (xp_inst->payload_size == 0) {
		xp_inst->payload_size = auth_xport_get_max_payload(xporthdl);
	}

	const uint16_t max_frame = MIN(sizeof(msg_frag), xp_inst->payload_size);
	const uint16_t max_payload = max_frame - XPORT_FRAG_HDR_BYTECNT;

	/* sanity check */
	if (xp_inst == NULL) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	/* set frame header */
	msg_frag.hdr.sync_flags = XPORT_FRAG_SYNC_BITS | XPORT_FRAG_BEGIN;

	/* Break up data to fit into lower transport MTU */
	while (len > 0) {

		/* get payload bytes */
		payload_bytes = MIN(max_payload, len);

		fragment_bytes = payload_bytes + XPORT_FRAG_HDR_BYTECNT;

		/* is this the last frame? */
		if ((len - payload_bytes) == 0) {

			msg_frag.hdr.sync_flags = XPORT_FRAG_SYNC_BITS | XPORT_FRAG_END;

			/* now check if we're only sending one frame, then set
			 * the frame begin flag
			 */
			if (num_fragments == 0) {
				msg_frag.hdr.sync_flags |= XPORT_FRAG_BEGIN;
			}
		}

		/* copy body */
		memcpy(msg_frag.frag_payload, data, payload_bytes);
		msg_frag.hdr.payload_len = payload_bytes;

		/* convert header to Big Endian, network byte order */
		auth_message_hdr_to_be16(&msg_frag.hdr);

		/* send frame */
		send_ret = auth_xport_internal_send(xporthdl,
						    (const uint8_t *)&msg_frag, fragment_bytes);

		if (send_ret < 0) {
			LOG_ERR("Failed to send xport frame, error: %d", send_ret);
			return AUTH_ERROR_XPORT_SEND;
		}

		/* verify all bytes were sent */
		if (send_ret != fragment_bytes) {
			LOG_ERR("Failed to to send all bytes, send: %d, requested: %d", send_ret, fragment_bytes);
			return AUTH_ERROR_XPORT_SEND;
		}

		/* set next flags */
		msg_frag.hdr.sync_flags = XPORT_FRAG_SYNC_BITS | XPORT_FRAG_NEXT;

		len -= payload_bytes;
		data += payload_bytes;
		send_count += payload_bytes;
		num_fragments++;
	}

	return send_count;
}


/**
 * @see auth_xport.h
 *
 * Put bytes received from lower transport into receive queue
 */
int auth_xport_put_recv(const auth_xport_hdl_t xporthdl, const uint8_t *buf,
			size_t buflen)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	if (xp_inst == NULL) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	return auth_xport_buffer_put(&xp_inst->recv_buf, buf, buflen);
}


/**
 * @see auth_xport.h
 */
int auth_xport_recv(const auth_xport_hdl_t xporthdl, uint8_t *buf,
		    uint32_t buf_len, uint32_t timeoutMsec)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	if (xp_inst == NULL) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	return auth_xport_buffer_get_wait(&xp_inst->recv_buf, buf, buf_len,
					  timeoutMsec);
}

/**
 * @see auth_xport.h
 */
int auth_xport_recv_peek(const auth_xport_hdl_t xporthdl, uint8_t *buff,
			 uint32_t buf_len)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	if (xp_inst == NULL) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	return auth_xport_buffer_peek(&xp_inst->recv_buf, buff, buf_len);
}

/**
 * @see auth_xport.h
 */
int auth_xport_getnum_send_queued_bytes(const auth_xport_hdl_t xporthdl)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	if (xp_inst == NULL) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	return auth_xport_buffer_bytecount(&xp_inst->send_buf);
}

/**
 * @see auth_xport.h
 */
int auth_xport_getnum_recvqueue_bytes(const auth_xport_hdl_t xporthdl)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	if (xp_inst == NULL) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	return auth_xport_buffer_bytecount(&xp_inst->recv_buf);
}

/**
 * @see auth_xport.h
 */
int auth_xport_getnum_recvqueue_bytes_wait(const auth_xport_hdl_t xporthdl,
					   uint32_t waitmsec)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	if (xp_inst == NULL) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	return auth_xport_buffer_bytecount_wait(&xp_inst->recv_buf, waitmsec);
}


/**
 * @see auth_internal.h
 */
bool auth_message_get_fragment(const uint8_t *buffer, uint16_t buflen,
			       uint16_t *frag_beg_offset, uint16_t *frag_byte_cnt)
{
	uint16_t cur_offset;
	uint16_t temp_payload_len;
	struct auth_message_frag_hdr *frm_hdr;
	bool found_sync_bytes = false;

	/* quick check */
	if (buflen < XPORT_MIN_FRAGMENT) {
		return false;
	}

	/* look for sync bytes  */
	for (cur_offset = 0; cur_offset < buflen; cur_offset++, buffer++) {
		/* look for the first sync byte */
		if (*buffer == XPORT_FRAG_SYNC_BYTE_HIGH) {
			if (cur_offset + 1 < buflen) {
				if ((*(buffer + 1) & XPORT_FRAG_LOWBYTE_MASK) ==
				    XPORT_FRAG_SYNC_BYTE_LOW) {
					/* found sync bytes */
					found_sync_bytes = true;
					break;
				}
			}
		}
	}

	/* Didn't find Fragment sync bytes */
	if (!found_sync_bytes) {
		return false;
	}

	/* should have a full header, check frame len */
	frm_hdr = (struct auth_message_frag_hdr *)buffer;

	/* convert from be to cpu */
	temp_payload_len = sys_be16_to_cpu(frm_hdr->payload_len) +
			   XPORT_FRAG_HDR_BYTECNT;

	/* does the buffer contain all of the fragment bytes?
	 * Including the header.
	 */
	if (temp_payload_len > (buflen - cur_offset)) {
		/* not enough bytes for a full frame */
		return false;
	}

	/* Put header vars into CPU byte order. */
	auth_message_hdr_to_cpu(frm_hdr);

	/* Have a full fragment */
	*frag_beg_offset = cur_offset;

	/* Return fragment byte count, including the header */
	*frag_byte_cnt = frm_hdr->payload_len + XPORT_FRAG_HDR_BYTECNT;

	return true;
}

/**
 * @see auth_internal.h
 */
int auth_message_assemble(const auth_xport_hdl_t xporthdl, const uint8_t *buf,
			  size_t buflen)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;
	struct auth_message_recv *msg_recv;
	struct auth_message_fragment *rx_frag;
	int free_buf_space;
	int recv_ret = 0;

	/* check input params */
	if ((xp_inst == NULL) || (buf == NULL) || (buflen == 0u)) {
		return AUTH_ERROR_INVALID_PARAM;
	}

	msg_recv = (struct auth_message_recv *)&xp_inst->recv_msg;

	/* If max payload size isn't set, get it from the lower transport.
	 * This can happen if the lower transports frame/MTU size is set
	 * after an initial connection.
	 */
	if (xp_inst->payload_size == 0) {
		xp_inst->payload_size = auth_xport_get_max_payload(xporthdl);
	}

	/* Reassemble a message from one for more fragments. */
	rx_frag = (struct auth_message_fragment *)buf;

	/* check for start flag */
	if (msg_recv->rx_first_frag) {

		msg_recv->rx_first_frag = false;

		if (!(rx_frag->hdr.sync_flags & XPORT_FRAG_BEGIN)) {
			/* reset vars */
			msg_recv->rx_curr_offset = 0;
			msg_recv->rx_first_frag = true;

			LOG_ERR("RX-Missing beginning fragment");
			return AUTH_ERROR_XPORT_FRAME;
		}

		LOG_DBG("RX-Got BEGIN fragment.");
	}

	/* check fragment sync bytes */
	if ((rx_frag->hdr.sync_flags & XPORT_FRAG_SYNC_MASK) != XPORT_FRAG_SYNC_BITS) {
		/* reset vars */
		msg_recv->rx_curr_offset = 0;
		msg_recv->rx_first_frag = true;

		LOG_ERR("RX-Invalid fragment.");
		return AUTH_ERROR_XPORT_FRAME;
	}


	/* Subtract out fragment header */
	buflen -= XPORT_FRAG_HDR_BYTECNT;

	/* move beyond fragment header */
	buf += XPORT_FRAG_HDR_BYTECNT;

	/* sanity check, if zero or negative */
	if (buflen <= 0) {
		/* reset vars */
		msg_recv->rx_curr_offset = 0;
		msg_recv->rx_first_frag = true;
		LOG_ERR("RX-Empty fragment!!");
		return AUTH_ERROR_XPORT_FRAME;
	}

	/* ensure there's enough free space in our temp buffer */
	free_buf_space = sizeof(msg_recv->rx_buffer) - msg_recv->rx_curr_offset;

	if (free_buf_space < buflen) {
		/* reset vars */
		msg_recv->rx_curr_offset = 0;
		msg_recv->rx_first_frag = true;
		LOG_ERR("RX-not enough free space");
		return AUTH_ERROR_XPORT_FRAME;
	}

	/* copy payload bytes */
	memcpy(msg_recv->rx_buffer + msg_recv->rx_curr_offset, buf, buflen);

	msg_recv->rx_curr_offset += buflen;

	/* returned the number of bytes queued */
	recv_ret = buflen;

	/* Is this the last fragment of the message? */
	if (rx_frag->hdr.sync_flags & XPORT_FRAG_END) {

		/* log number payload bytes received. */
		LOG_INF("RX-Got LAST fragment, total bytes: %d", msg_recv->rx_curr_offset);

		int free_bytes = auth_xport_buffer_avail_bytes(&xp_inst->recv_buf);

		/* Is there enough free space to write entire message? */
		if (free_bytes >= msg_recv->rx_curr_offset) {

			/* copy message into receive buffer */
			recv_ret = auth_xport_buffer_put(&xp_inst->recv_buf,
							 msg_recv->rx_buffer,
							 msg_recv->rx_curr_offset);

		} else {
			int need = msg_recv->rx_curr_offset - free_bytes;
			LOG_ERR("Not enough room in RX buffer, free: %d, need %d bytes.", free_bytes, need);
		}

		/* reset vars */
		msg_recv->rx_curr_offset = 0;
		msg_recv->rx_first_frag = true;
	}

	return recv_ret;
}

/**
 * @see auth_internal.h
 */
void auth_message_hdr_to_cpu(struct auth_message_frag_hdr *frag_hdr)
{
	frag_hdr->sync_flags = sys_be16_to_cpu(frag_hdr->sync_flags);
	frag_hdr->payload_len = sys_be16_to_cpu(frag_hdr->payload_len);
}

/**
 * @see auth_internal.h
 */
void auth_message_hdr_to_be16(struct auth_message_frag_hdr *frag_hdr)
{
	frag_hdr->sync_flags = sys_cpu_to_be16(frag_hdr->sync_flags);
	frag_hdr->payload_len = sys_cpu_to_be16(frag_hdr->payload_len);
}

/**
 * @see auth_internal.h
 */
void auth_get_random(uint8_t *buf, size_t num)
{
#if defined(CONFIG_HARDWARE_DEVICE_CS_GENERATOR)
	/* if hardware based random num generator available */
	sys_csrand_get(buf, num);
#else
	sys_rand_get(buf, num);
#endif
}


/**
 * @see auth_xport.h
 */
void auth_xport_set_sendfunc(auth_xport_hdl_t xporthdl, send_xport_t send_func)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	xp_inst->send_func = send_func;
}

/**
 * @see auth_xport.h
 */
void auth_xport_set_context(auth_xport_hdl_t xporthdl, void *context)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	xp_inst->xport_ctx = context;
}

/**
 * @see auth_xport.h
 */
void *auth_xport_get_context(auth_xport_hdl_t xporthdl)
{
	struct auth_xport_instance *xp_inst = (struct auth_xport_instance *)xporthdl;

	return xp_inst->xport_ctx;
}
