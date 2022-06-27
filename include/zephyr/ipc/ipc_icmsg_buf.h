/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_SERVICE_IPC_ICMSG_BUF_H_
#define ZEPHYR_INCLUDE_IPC_SERVICE_IPC_ICMSG_BUF_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Service ICMsg buffer API
 * @ingroup ipc_service_icmsg_buffer_api IPC service ICMsg buffer API
 * @{
 */

/**
 * @brief Inter core messaging buffer
 *
 * The inter core messaging buffer implements lightweight unidirectional
 * messaging buffer with read/write semantics on top of a memory region shared
 * by the reader and writer. It embeds cache and memory barier management to
 * ensure correct data access.
 *
 * This structure supports single writter and reader. Data stored in the buffer
 * is encapsulated to a message.
 *
 */
struct icmsg_buf {
	uint32_t len;		/* Length of data[] in bytes. */
	uint32_t wr_idx;	/* Index of the first free byte in data[] */
	uint32_t rd_idx;	/* Index of the first valid byte in data[] */
	uint8_t data[];		/* Buffer data. */
};

/**
 * @brief Initialize inter core messaging buffer.
 *
 * This function initializes inter core messaging buffer on top of dedicated
 * memory region.
 *
 * @param buf			Pointer to a memory region on which buffer is
 *				created.
 * @param blen			Length of the buffer. Must be large enough to
 *				contain the internal structure and at least two
 *				bytes of data (one is reserved for written
 *				messages length).
 * @retval struct icmsg_buf*	Pointer to the created buffer. The pointer
 *				points to the same address as buf.
 */
struct icmsg_buf *icmsg_buf_init(void *buf, size_t blen);

/**
 * @brief Write specified amount of data to the inter core messaging buffer.
 *
 * @param ib	A icmsg buffer to which to write.
 * @param buf	Pointer to the data to be written to icmsg buffer.
 * @param len	Number of bytes to be written to the icmsg buffer.
 * @retval int	Number of bytes written, negative error code on fail.
 *		-EINVAL, if len == 0.
 *		-ENOMEM, if len is bigger than the icmsg buffer can fit.
 */
int icmsg_buf_write(struct icmsg_buf *ib, const char *buf, uint16_t len);

/**
 * @brief Read specified amount of data from the inter core messaging buffer.
 *
 * Single read allows to read the message send by the single write.
 * The provided buf must be big enough to store the whole message.
 *
 * @param ib		A icmsg buffer to which data are to be written
 * @param buf		Data pointer to which read data will be written.
 *			If NULL, len of stored message is returned.
 * @param len		Number of bytes to be read from the icmsg buffer.
 * @retval int		Bytes read, negative error code on fail.
 *			Bytes to be read, if buf == NULL.
 *			-ENOMEM, if message can not fit in provided buf.
 *			-EAGAIN, if not whole message is ready yet.
 */
int icmsg_buf_read(struct icmsg_buf *ib, char *buf, uint16_t len);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_SERVICE_IPC_ICMSG_BUF_H_ */
