/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_ICMSG_PBUF_H_
#define ZEPHYR_INCLUDE_IPC_ICMSG_PBUF_H_

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inter core messaging, secure packet buffer API
 * @ingroup kernel_apis
 * @{
 */

/** @brief Size of packet lenght field. */
#define ICMSG_PBUF_PACKET_LEN_SZ sizeof(uint32_t)

/* Amount of data that is left unused to distinguish between empty and full. */
#define _IDX_SIZE sizeof(uint32_t)

/* Minimal length of the data field in the buffer to store the smalest packet possible.
 * +1 for at least one byte of data.
 * Rounded up to keep wr/rd indexes pointing to aligned address.
 */
#define _MIN_DATA_LEN ROUND_UP(ICMSG_PBUF_PACKET_LEN_SZ + 1 + _IDX_SIZE, _IDX_SIZE)

/** @brief Control block of secure packet buffer.
 *
 * The structure contains configuration data.
 */
struct icmsg_pbuf_cfg {
	volatile uint32_t *rd_idx_loc;	/* Address of the value holding first valid byte in data[]. */
	volatile uint32_t *wr_idx_loc;	/* Address of the value holding first free byte in data[]. */
	uint32_t cache_line_sz;		/* CPU data cache line size in bytes.
					 * Used for validation - could probably be removed
					 */
	uint32_t len;			/* Length of data[] in bytes. */
	uint8_t *data_loc;		/* Location of the buffer data. */
};


struct icmsg_pbuf_data {
	volatile uint32_t wr_idx;	/* Index of the first holding first free byte in data[].
					* Used for writing.
					*/
	volatile uint32_t rd_idx;	/* Index of the first holding first valid byte in data[].
					* Used for reading.
					*/
};


/**
 * @brief Inter core messaging, secure packet buffer.
 *
 * The IcMsg packet buffer implements lightweight unidirectional packet
 * buffer with read/write semantics on top of a memory region shared
 * by the reader and writer. It optionally embeds cache and memory barrier
 * management to ensure correct data access.
 *
 * This structure supports single writer and reader. Data stored in the buffer
 * is encapsulated to a message (with length header). The read/write API is
 * written in a way to protect the data from being corrupted.
 */
struct icmsg_pbuf {
	const struct icmsg_pbuf_cfg *cfg;	/* Configuration of the buffer. */
	struct icmsg_pbuf_data *data;		/* Data used to read and write to the
						 * buffer
						 */
};

/* Helper macro to check whenever the value is aligned. */
#define IS_ALIGNED(address, alignment) \
	((uintptr_t)(address) % (uintptr_t)(alignment) == 0)

/**
 * @brief Macro for configuration initialization.
 *
 * It is recommended to use this macro to initialize IcMsg packed buffer
 * configuration.
 */
#define ICMSG_PBUF_CFG_INIT(mem_addr, size, dcache_line_sz) 					    \
{												    \
	.rd_idx_loc = (uint32_t *)(mem_addr),							    \
	.wr_idx_loc = (uint32_t *)((uint8_t *)(mem_addr) + MAX(dcache_line_sz, _IDX_SIZE)),	    \
	.data_loc = (uint8_t *)((uint8_t *)(mem_addr) + MAX(dcache_line_sz, _IDX_SIZE) + _IDX_SIZE),\
	.len = ((uint32_t)(((size) > 0 && IS_ALIGNED(size, _IDX_SIZE)) ? (size) : (1/0)) >=	    \
		(MAX(dcache_line_sz, _IDX_SIZE) + _IDX_SIZE + _MIN_DATA_LEN)) ?			    \
		(uint32_t)((uint32_t)(size) - MAX(dcache_line_sz, _IDX_SIZE) - _IDX_SIZE) : (1/0),  \
	.cache_line_sz = ((dcache_line_sz) >= 0 ? (dcache_line_sz) : 1/0),			    \
}

/**
 * @brief Initialize the IcMsg packet buffer.
 *
 * This function initializes the packet buffer based on provided configuration.
 * If the configuration is incorrect, the function will return error.
 *
 * It is recommended to use ICMSG_PBUF_CFG_INIT macro for build time
 * configuration initialization.
 *
 * @param spb	Pointer to a secured packed buffer containing
 *		configuration and data. Configuration has to be
 *		fixed before the initialization.
 * @retval 0 on success.
 * @retval -EINVAL when the input parameter is incorrect.
 */
int icmsg_pbuf_init(struct icmsg_pbuf *ib);

/**
 * @brief Write specified amount of data to the packet buffer.
 *
 * This function call writes specified amout of data to the packet buffer if
 * the buffer will fit the data.
 *
 * @param spb	A buffer to which to write.
 * @param buf	Pointer to the data to be written to the buffer.
 * @param len	Number of bytes to be written to the buffer. Must be positive.
 * @retval int	Number of bytes written, negative error code on fail.
 *		-EINVAL, if any of input parameter is incorrect.
 *		-ENOMEM, if len is bigger than the buffer can fit.
 */

int icmsg_pbuf_write(struct icmsg_pbuf *ib, const char *buf, uint16_t len);

/**
 * @brief Read specified amount of data from the packet buffer.
 *
 * Single read allows to read the message send by the single write.
 * The provided %p buf must be big enough to store the whole message.
 *
 * @param pb	A buffer from which data will be read.
 * @param buf	Data pointer to which read data will be written.
 *		If NULL, len of stored message is returned.
 * @param len	Number of bytes to be read from the buffer.
 * @retval int	Bytes read, negative error code on fail.
 *		Bytes to be read, if buf == NULL.
 *		-EINVAL, if any of input parameter is incorrect.
 *		-ENOMEM, if message can not fit in provided buf.
 *		-EAGAIN, if not whole message is ready yet.
 */
int icmsg_pbuf_read(struct icmsg_pbuf *ib, char *buf, uint16_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_ICMSG_PBUF_H_ */
