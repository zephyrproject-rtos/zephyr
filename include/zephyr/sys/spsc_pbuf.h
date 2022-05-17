/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_SPSC_PBUF_H_
#define ZEPHYR_INCLUDE_SYS_SPSC_PBUF_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Single producer, single consumer packet buffer API
 * @ingroup kernel_apis
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
struct spsc_pbuf {
	uint32_t len;		/* Length of data[] in bytes. */
	uint32_t wr_idx;	/* Index of the first free byte in data[] */
	uint32_t rd_idx;	/* Index of the first valid byte in data[] */
	uint8_t data[];		/* Buffer data. */
};

/**
 * @brief Initialize the packet buffer.
 *
 * This function initializes the packet buffer on top of a dedicated
 * memory region.
 *
 * @param buf			Pointer to a memory region on which buffer is
 *				created.
 * @param blen			Length of the buffer. Must be large enough to
 *				contain the internal structure and at least two
 *				bytes of data (one is reserved for written
 *				messages length).
 * @retval struct spsc_pbuf*	Pointer to the created buffer. The pointer
 *				points to the same address as buf.
 */
struct spsc_pbuf *spsc_pbuf_init(void *buf, size_t blen);

/**
 * @brief Write specified amount of data to the packet buffer.
 *
 * @param pb	A buffer to which to write.
 * @param buf	Pointer to the data to be written to the buffer.
 * @param len	Number of bytes to be written to the buffer.
 * @retval int	Number of bytes written, negative error code on fail.
 *		-EINVAL, if len == 0.
 *		-ENOMEM, if len is bigger than the buffer can fit.
 */
int spsc_pbuf_write(struct spsc_pbuf *pb, const char *buf, uint16_t len);

/**
 * @brief Read specified amount of data from the packet buffer.
 *
 * Single read allows to read the message send by the single write.
 * The provided buf must be big enough to store the whole message.
 *
 * @param pb		A buffer from which data is to be read.
 * @param buf		Data pointer to which read data will be written.
 *			If NULL, len of stored message is returned.
 * @param len		Number of bytes to be read from the buffer.
 * @retval int		Bytes read, negative error code on fail.
 *			Bytes to be read, if buf == NULL.
 *			-ENOMEM, if message can not fit in provided buf.
 *			-EAGAIN, if not whole message is ready yet.
 */
int spsc_pbuf_read(struct spsc_pbuf *pb, char *buf, uint16_t len);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_SPSC_PBUF_H_ */
