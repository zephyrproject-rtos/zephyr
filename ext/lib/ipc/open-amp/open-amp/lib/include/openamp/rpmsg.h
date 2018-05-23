/*
 * Remote processor messaging
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RPMSG_H_
#define _RPMSG_H_

#include <openamp/rpmsg_core.h>

#if defined __cplusplus
extern "C" {
#endif

/* The feature bitmap for virtio rpmsg */
#define VIRTIO_RPMSG_F_NS	0	/* RP supports name service notifications */
#define RPMSG_NAME_SIZE     32
#define RPMSG_BUF_HELD      (1U << 31) /* Flag to suggest to hold the buffer */

#define RPMSG_LOCATE_DATA(p)  ((unsigned char *) p + sizeof (struct rpmsg_hdr))

/**
 * struct rpmsg_hdr - common header for all rpmsg messages
 * @src: source address
 * @dst: destination address
 * @reserved: reserved for future use
 * @len: length of payload (in bytes)
 * @flags: message flags
 *
 * Every message sent(/received) on the rpmsg bus begins with this header.
 */
OPENAMP_PACKED_BEGIN
struct rpmsg_hdr {
	uint32_t src;
	uint32_t dst;
	uint32_t reserved;
	uint16_t len;
	uint16_t flags;
} OPENAMP_PACKED_END;

/**
 * struct rpmsg_hdr_reserved - this is the "union" of the rpmsg_hdr->reserved
 * @rfu: reserved for future usage
 * @idx: index of a buffer (not to be returned back to the buffer's pool)
 *
 * This structure has been introduced to keep the backward compatibility. 
 * It could be integrated into rpmsg_hdr struct, replacing the reserved field.
 */
struct rpmsg_hdr_reserved
{
	uint16_t rfu; /* reserved for future usage */
	uint16_t idx;
};

/**
 * struct rpmsg_ns_msg - dynamic name service announcement message
 * @name: name of remote service that is published
 * @addr: address of remote service that is published
 * @flags: indicates whether service is created or destroyed
 *
 * This message is sent across to publish a new service, or announce
 * about its removal. When we receive these messages, an appropriate
 * rpmsg channel (i.e device) is created/destroyed. In turn, the ->probe()
 * or ->remove() handler of the appropriate rpmsg driver will be invoked
 * (if/as-soon-as one is registered).
 */
OPENAMP_PACKED_BEGIN
struct rpmsg_ns_msg {
	char name[RPMSG_NAME_SIZE];
	uint32_t addr;
	uint32_t flags;
} OPENAMP_PACKED_END;

/**
 * enum rpmsg_ns_flags - dynamic name service announcement flags
 *
 * @RPMSG_NS_CREATE: a new remote service was just created
 * @RPMSG_NS_DESTROY: a known remote service was just destroyed
 */
enum rpmsg_ns_flags {
	RPMSG_NS_CREATE = 0,
	RPMSG_NS_DESTROY = 1,
};

#define RPMSG_ADDR_ANY		0xFFFFFFFF

/**
 * rpmsg_channel - devices that belong to the rpmsg bus are called channels
 * @name: channel name
 * @src: local address
 * @dst: destination address
 * rdev: rpmsg remote device
 * @ept: the rpmsg endpoint of this channel
 * @state: channel state
 */
struct rpmsg_channel {
	char name[RPMSG_NAME_SIZE];
	uint32_t src;
	uint32_t dst;
	struct remote_device *rdev;
	struct rpmsg_endpoint *rp_ept;
	unsigned int state;
	struct metal_list node;
};

/**
 * channel_info - channel info
 * @name: channel name
 * @src: local address
 * @dst: destination address
 */

struct channel_info {
	char name[RPMSG_NAME_SIZE];
	uint32_t src;
	uint32_t dest;
};

/**
 * struct rpmsg_endpoint - binds a local rpmsg address to its user
 * @rp_chnl: rpmsg channel device
 * @cb: rx callback handler
 * @addr: local rpmsg address
 * @priv: private data for the driver's use
 *
 * In essence, an rpmsg endpoint represents a listener on the rpmsg bus, as
 * it binds an rpmsg address with an rx callback handler.
 *
 * Simple rpmsg drivers shouldn't use this struct directly, because
 * things just work: every rpmsg driver provides an rx callback upon
 * registering to the bus, and that callback is then bound to its rpmsg
 * address when the driver is probed. When relevant inbound messages arrive
 * (i.e. messages which their dst address equals to the src address of
 * the rpmsg channel), the driver's handler is invoked to process it.
 *
 * More complicated drivers though, that do need to allocate additional rpmsg
 * addresses, and bind them to different rx callbacks, must explicitly
 * create additional endpoints by themselves (see rpmsg_create_ept()).
 */
struct rpmsg_endpoint {
	struct rpmsg_channel *rp_chnl;
	rpmsg_rx_cb_t cb;
	uint32_t addr;
	void *priv;
	struct metal_list node;
};

struct rpmsg_endpoint *rpmsg_create_ept(struct rpmsg_channel *rp_chnl,
					rpmsg_rx_cb_t cb, void *priv,
					uint32_t addr);

void rpmsg_destroy_ept(struct rpmsg_endpoint *rp_ept);

int
rpmsg_send_offchannel_raw(struct rpmsg_channel *, uint32_t, uint32_t,
			  const void *, int, int);

/**
 * rpmsg_send() - send a message across to the remote processor
 * @rpdev: the rpmsg channel
 * @data: payload of message
 * @len: length of payload
 *
 * This function sends @data of length @len on the @rpdev channel.
 * The message will be sent to the remote processor which the @rpdev
 * channel belongs to, using @rpdev's source and destination addresses.
 * In case there are no TX buffers available, the function will block until
 * one becomes available, or a timeout of 15 seconds elapses. When the latter
 * happens, -ERESTARTSYS is returned.
 *
 * Can only be called from process context (for now).
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_send(struct rpmsg_channel *rpdev, const void *data,
			int len)
{
	return rpmsg_send_offchannel_raw(rpdev, rpdev->src, rpdev->dst,
					 data, len, RPMSG_TRUE);
}

/**
 * rpmsg_sendto() - send a message across to the remote processor, specify dst
 * @rpdev: the rpmsg channel
 * @data: payload of message
 * @len: length of payload
 * @dst: destination address
 *
 * This function sends @data of length @len to the remote @dst address.
 * The message will be sent to the remote processor which the @rpdev
 * channel belongs to, using @rpdev's source address.
 * In case there are no TX buffers available, the function will block until
 * one becomes available, or a timeout of 15 seconds elapses. When the latter
 * happens, -ERESTARTSYS is returned.
 *
 * Can only be called from process context (for now).
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_sendto(struct rpmsg_channel *rpdev, const void *data,
			       int len, uint32_t dst)
{
	return rpmsg_send_offchannel_raw(rpdev, rpdev->src, dst, data,
					 len, RPMSG_TRUE);
}

/**
 * rpmsg_send_offchannel() - send a message using explicit src/dst addresses
 * @rpdev: the rpmsg channel
 * @src: source address
 * @dst: destination address
 * @data: payload of message
 * @len: length of payload
 *
 * This function sends @data of length @len to the remote @dst address,
 * and uses @src as the source address.
 * The message will be sent to the remote processor which the @rpdev
 * channel belongs to.
 * In case there are no TX buffers available, the function will block until
 * one becomes available, or a timeout of 15 seconds elapses. When the latter
 * happens, -ERESTARTSYS is returned.
 *
 * Can only be called from process context (for now).
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_send_offchannel(struct rpmsg_channel *rpdev,
					uint32_t src, uint32_t dst,
					const void *data, int len)
{
	return rpmsg_send_offchannel_raw(rpdev, src, dst, data, len,
					 RPMSG_TRUE);
}

/**
 * rpmsg_trysend() - send a message across to the remote processor
 * @rpdev: the rpmsg channel
 * @data: payload of message
 * @len: length of payload
 *
 * This function sends @data of length @len on the @rpdev channel.
 * The message will be sent to the remote processor which the @rpdev
 * channel belongs to, using @rpdev's source and destination addresses.
 * In case there are no TX buffers available, the function will immediately
 * return -ENOMEM without waiting until one becomes available.
 *
 * Can only be called from process context (for now).
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_trysend(struct rpmsg_channel *rpdev, const void *data,
				int len)
{
	return rpmsg_send_offchannel_raw(rpdev, rpdev->src, rpdev->dst,
					 data, len, RPMSG_FALSE);
}

/**
 * rpmsg_trysendto() - send a message across to the remote processor, specify dst
 * @rpdev: the rpmsg channel
 * @data: payload of message
 * @len: length of payload
 * @dst: destination address
 *
 * This function sends @data of length @len to the remote @dst address.
 * The message will be sent to the remote processor which the @rpdev
 * channel belongs to, using @rpdev's source address.
 * In case there are no TX buffers available, the function will immediately
 * return -ENOMEM without waiting until one becomes available.
 *
 * Can only be called from process context (for now).
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_trysendto(struct rpmsg_channel *rpdev, const void *data,
				  int len, uint32_t dst)
{
	return rpmsg_send_offchannel_raw(rpdev, rpdev->src, dst, data, len,
					 RPMSG_FALSE);
}

/**
 * rpmsg_trysend_offchannel() - send a message using explicit src/dst addresses
 * @rpdev: the rpmsg channel
 * @src: source address
 * @dst: destination address
 * @data: payload of message
 * @len: length of payload
 *
 * This function sends @data of length @len to the remote @dst address,
 * and uses @src as the source address.
 * The message will be sent to the remote processor which the @rpdev
 * channel belongs to.
 * In case there are no TX buffers available, the function will immediately
 * return -ENOMEM without waiting until one becomes available.
 *
 * Can only be called from process context (for now).
 *
 * Returns number of bytes it has sent or negative error value on failure.
 */
static inline int rpmsg_trysend_offchannel(struct rpmsg_channel *rpdev,
					   uint32_t src, uint32_t dst,
					   const void *data, int len)
{
	return rpmsg_send_offchannel_raw(rpdev, src, dst, data, len,
					 RPMSG_FALSE);
}

/**
 * @brief Holds the rx buffer for usage outside the receive callback.
 *
 * Calling this function prevents the RPMsg receive buffer from being released
 * back to the pool of shmem buffers. This API can only be called at rx
 * callback context (rpmsg_rx_cb_t). With this API, the application doesn't
 * need to copy the message in rx callback. Instead, the rx buffer base address
 * is saved in application context and further processed in application
 * process. After the message is processed, the application can release the rx
 * buffer for future reuse in vring by calling the rpmsg_release_rx_buffer()
 * function.
 *
 * @param[in] rpdev The rpmsg channel
 * @param[in] rxbuf RX buffer with message payload
 *
 * @see rpmsg_release_rx_buffer
 */
void rpmsg_hold_rx_buffer(struct rpmsg_channel *rpdev, void *rxbuf);

/**
 * @brief Releases the rx buffer for future reuse in vring.
 *
 * This API can be called at process context when the message in rx buffer is
 * processed.
 *
 * @param rpdev - the rpmsg channel
 * @param rxbuf - rx buffer with message payload
 *
 * @see rpmsg_hold_rx_buffer
 */
void rpmsg_release_rx_buffer(struct rpmsg_channel *rpdev, void *rxbuf);

/**
 * @brief Gets the tx buffer for message payload.
 *
 * This API can only be called at process context to get the tx buffer in vring.
 * By this way, the application can directly put its message into the vring tx
 * buffer without copy from an application buffer.
 * It is the application responsibility to correctly fill the allocated tx
 * buffer by data and passing correct parameters to the rpmsg_send_nocopy() or
 * rpmsg_sendto_nocopy() function to perform data no-copy-send mechanism.
 *
 * @param[in] rpdev Pointer to rpmsg channel
 * @param[in] size  Pointer to store tx buffer size
 * @param[in] wait  Boolean, wait or not for buffer to become available
 *
 * @return The tx buffer address on success and NULL on failure
 *
 * @see rpmsg_send_offchannel_nocopy
 * @see rpmsg_sendto_nocopy
 * @see rpmsg_send_nocopy
 */
void *rpmsg_get_tx_payload_buffer(struct rpmsg_channel *rpdev, uint32_t *size,
			    int wait);

/**
 * @brief Sends a message in tx buffer allocated by
 *  rpmsg_get_tx_payload_buffer()
 *
 * using explicit src/dst addresses.
 *
 * This function sends txbuf of length len to the remote dst address,
 * and uses src as the source address.
 * The message will be sent to the remote processor which the rpdev
 * channel belongs to.
 * The application has to take the responsibility for:
 *  1. tx buffer allocation (rpmsg_get_tx_payload_buffer() )
 *  2. filling the data to be sent into the pre-allocated tx buffer
 *  3. not exceeding the buffer size when filling the data
 *  4. data cache coherency
 *
 * After the rpmsg_send_offchannel_nocopy() function is issued the tx buffer is
 * no more owned by the sending task and must not be touched anymore unless the
 * rpmsg_send_offchannel_nocopy() function fails and returns an error. In that
 * case the application should try to re-issue the
 * rpmsg_send_offchannel_nocopy() again and if it is still not possible to send
 * the message and the application wants to give it up from whatever reasons
 * the rpmsg_release_rx_buffer function could be called, passing the pointer to
 * the tx buffer to be released as a parameter.
 *
 * @param[in] rpdev The rpmsg channel
 * @param[in] src   Source address
 * @param[in] dst   Destination address
 * @param[in] txbuf TX buffer with message filled
 * @param[in] len   Length of payload
 *
 * @return number of bytes it has sent or negative error value on failure.
 *
 * @see rpmsg_get_tx_payload_buffer 
 * @see rpmsg_sendto_nocopy
 * @see rpmsg_send_nocopy
 */
int rpmsg_send_offchannel_nocopy(struct rpmsg_channel *rpdev, uint32_t src,
				 uint32_t dst, void *txbuf, int len);

/**
 * @brief Sends a message in tx buffer allocated by
 * rpmsg_get_tx_payload_buffer()
 *
 * across to the remote processor, specify dst.
 *
 * This function sends txbuf of length len to the remote dst address.
 * The message will be sent to the remote processor which the rpdev
 * channel belongs to, using rpdev's source address.
 * The application has to take the responsibility for:
 *  1. tx buffer allocation (rpmsg_get_tx_payload_buffer() )
 *  2. filling the data to be sent into the pre-allocated tx buffer
 *  3. not exceeding the buffer size when filling the data
 *  4. data cache coherency
 *
 * After the rpmsg_sendto_nocopy() function is issued the tx buffer is no more
 * owned by the sending task and must not be touched anymore unless the
 * rpmsg_sendto_nocopy() function fails and returns an error. In that case the
 * application should try to re-issue the rpmsg_sendto_nocopy() again and if
 * it is still not possible to send the message and the application wants to
 * give it up from whatever reasons the rpmsg_release_rx_buffer function
 * could be called,
 * passing the pointer to the tx buffer to be released as a parameter.
 *
 * @param[in] rpdev The rpmsg channel
 * @param[in] txbuf TX buffer with message filled
 * @param[in] len   Length of payload
 * @param[in] dst   Destination address
 *
 * @return number of bytes it has sent or negative error value on failure.
 *
 * @see rpmsg_get_tx_payload_buffer 
 * @see rpmsg_send_offchannel_nocopy
 * @see rpmsg_send_nocopy
 */
static inline
int rpmsg_sendto_nocopy(struct rpmsg_channel *rpdev, void *txbuf, int len,
			uint32_t dst)
{
	if (!rpdev)
		return RPMSG_ERR_PARAM;

	return rpmsg_send_offchannel_nocopy(rpdev, (uint32_t)rpdev->src, dst,
					    txbuf, len);
}

/**
 * @brief Sends a message in tx buffer allocated by
 * rpmsg_get_tx_payload_buffer() across to the remote processor.
 *
 * This function sends txbuf of length len on the rpdev channel.
 * The message will be sent to the remote processor which the rpdev
 * channel belongs to, using rpdev's source and destination addresses.
 * The application has to take the responsibility for:
 *  1. tx buffer allocation (rpmsg_get_tx_payload_buffer() )
 *  2. filling the data to be sent into the pre-allocated tx buffer
 *  3. not exceeding the buffer size when filling the data
 *  4. data cache coherency
 *
 * After the rpmsg_send_nocopy() function is issued the tx buffer is no more
 * owned by the sending task and must not be touched anymore unless the
 * rpmsg_send_nocopy() function fails and returns an error. In that case the
 * application should try to re-issue the rpmsg_send_nocopy() again and if
 * it is still not possible to send the message and the application wants to
 * give it up from whatever reasons the rpmsg_release_rx_buffer function
 * could be called, passing the pointer to the tx buffer to be released as a
 * parameter.
 *
 * @param[in] rpdev The rpmsg channel
 * @param[in] txbuf TX buffer with message filled
 * @param[in] len   Length of payload
 *
 * @return 0 on success and an appropriate error value on failure
 *
 * @see rpmsg_get_tx_payload_buffer 
 * @see rpmsg_send_offchannel_nocopy
 * @see rpmsg_sendto_nocopy
 */
static inline
int rpmsg_send_nocopy(struct rpmsg_channel *rpdev, void *txbuf, int len)
{
	if (!rpdev)
		return RPMSG_ERR_PARAM;

	return rpmsg_send_offchannel_nocopy(rpdev, rpdev->src, rpdev->dst,
					    txbuf, len);
}

/**
 * rpmsg_init
 *
 * Thus function allocates and initializes the rpmsg driver resources for
 * the given hil_proc.The successful return from this function leaves
 * fully enabled IPC link.
 *
 * @param proc              - pointer to hil_proc
 * @param rdev              - pointer to newly created remote device
 * @param channel_created   - callback function for channel creation
 * @param channel_destroyed - callback function for channel deletion
 * @param default_cb        - default callback for channel
 * @param role              - role of the other device, Master or Remote
 * @return - status of function execution
 *
 */

int rpmsg_init(struct hil_proc *proc,
	       struct remote_device **rdev,
	       rpmsg_chnl_cb_t channel_created,
	       rpmsg_chnl_cb_t channel_destroyed,
	       rpmsg_rx_cb_t default_cb, int role);

/**
 * rpmsg_deinit
 *
 * Thus function releases the rpmsg driver resources for given remote
 * instance.
 *
 * @param rdev  -  pointer to device de-init
 *
 * @return - none
 *
 */
void rpmsg_deinit(struct remote_device *rdev);

/**
 * rpmsg_get_buffer_size
 *
 * Returns buffer size available for sending messages.
 *
 * @param channel - pointer to rpmsg channel/device
 *
 * @return - buffer size
 *
 */
int rpmsg_get_buffer_size(struct rpmsg_channel *rp_chnl);

/**
 * rpmsg_create_channel
 *
 * Creates RPMSG channel with the given name for remote device.
 *
 * @param rdev - pointer to rpmsg remote device
 * @param name - channel name
 *
 * @return - pointer to new rpmsg channel
 *
 */
struct rpmsg_channel *rpmsg_create_channel(struct remote_device *rdev,
					   char *name);

/**
 * rpmsg_delete_channel
 *
 * Deletes the given RPMSG channel. The channel must first be created with the
 * rpmsg_create_channel API.
 *
 * @param rp_chnl - pointer to rpmsg channel to delete
 *
 */
void rpmsg_delete_channel(struct rpmsg_channel *rp_chnl);

#if defined __cplusplus
}
#endif

#endif				/* _RPMSG_H_ */
