/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_PBUF_H_
#define ZEPHYR_INCLUDE_IPC_PBUF_H_

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Packed buffer API
 * @defgroup pbuf Packed Buffer API
 * @ingroup ipc
 * @{
 */

/** @brief Size of packet length field. */
#define PBUF_PACKET_LEN_SZ sizeof(uint32_t)

/* Amount of data that is left unused to distinguish between empty and full. */
#define _PBUF_IDX_SIZE sizeof(uint32_t)

/* Minimal length of the data field in the buffer to store the smalest packet
 * possible.
 * (+1) for at least one byte of data.
 * (+_PBUF_IDX_SIZE) to distinguish buffer full and buffer empty.
 * Rounded up to keep wr/rd indexes pointing to aligned address.
 */
#define _PBUF_MIN_DATA_LEN ROUND_UP(PBUF_PACKET_LEN_SZ + 1 + _PBUF_IDX_SIZE, _PBUF_IDX_SIZE)

/** @brief Control block of packet buffer.
 *
 * The structure contains configuration data.
 */
struct pbuf_cfg {
	volatile uint32_t *rd_idx_loc;	/* Address of the variable holding
					 * index value of the first valid byte
					 * in data[].
					 */
	volatile uint32_t *wr_idx_loc;	/* Address of the variable holding
					 * index value of the first free byte
					 * in data[].
					 */
	uint32_t dcache_alignment;	/* CPU data cache line size in bytes.
					 * Used for validation - TODO: To be
					 * replaced by flags.
					 */
	uint32_t len;			/* Length of data[] in bytes. */
	uint8_t *data_loc;		/* Location of the data[]. */
};

/**
 * @brief Data block of the packed buffer.
 *
 * The structure contains local copies of wr and rd indexes used by writer and
 * reader respecitvely.
 */
struct pbuf_data {
	volatile uint32_t wr_idx;	/* Index of the first holding first
					 * free byte in data[]. Used for
					 * writing.
					 */
	volatile uint32_t rd_idx;	/* Index of the first holding first
					 * valid byte in data[]. Used for
					 * reading.
					 */
};


/**
 * @brief Scure packed buffer.
 *
 * The packet buffer implements lightweight unidirectional packet
 * buffer with read/write semantics on top of a memory region shared
 * by the reader and writer. It embeds cache and memory barrier management to
 * ensure correct data access.
 *
 * This structure supports single writer and reader. Data stored in the buffer
 * is encapsulated to a message (with length header). The read/write API is
 * written in a way to protect the data from being corrupted.
 */
struct pbuf {
	const struct pbuf_cfg *const cfg;	/* Configuration of the
						 * buffer.
						 */
	struct pbuf_data data;			/* Data used to read and write
						 * to the buffer
						 */
};

/**
 * @brief Macro for configuration initialization.
 *
 * It is recommended to use this macro to initialize packed buffer
 * configuration.
 *
 * @param mem_addr	Memory address for pbuf.
 * @param size		Size of the memory.
 * @param dcache_align	Data cache alignment.
 */
#define PBUF_CFG_INIT(mem_addr, size, dcache_align)					\
{											\
	.rd_idx_loc = (uint32_t *)(mem_addr),						\
	.wr_idx_loc = (uint32_t *)((uint8_t *)(mem_addr) +				\
				MAX(dcache_align, _PBUF_IDX_SIZE)),			\
	.data_loc = (uint8_t *)((uint8_t *)(mem_addr) +					\
				MAX(dcache_align, _PBUF_IDX_SIZE) + _PBUF_IDX_SIZE),	\
	.len = (uint32_t)((uint32_t)(size) - MAX(dcache_align, _PBUF_IDX_SIZE) -	\
				_PBUF_IDX_SIZE),					\
	.dcache_alignment = (dcache_align),						\
}

/**
 * @brief Macro calculates memory overhead taken by the header in shared memory.
 *
 * It contains the read index, write index and padding.
 *
 * @param dcache_align	Data cache alignment.
 */
#define PBUF_HEADER_OVERHEAD(dcache_align) \
	(MAX(dcache_align, _PBUF_IDX_SIZE) + _PBUF_IDX_SIZE)

/**
 * @brief Statically define and initialize pbuf.
 *
 * @param name			Name of the pbuf.
 * @param mem_addr		Memory address for pbuf.
 * @param size			Size of the memory.
 * @param dcache_align	Data cache line size.
 */
#define PBUF_DEFINE(name, mem_addr, size, dcache_align)					\
	BUILD_ASSERT(dcache_align >= 0,							\
			"Cache line size must be non negative.");			\
	BUILD_ASSERT((size) > 0 && IS_PTR_ALIGNED_BYTES(size, _PBUF_IDX_SIZE),		\
			"Incorrect size.");						\
	BUILD_ASSERT(IS_PTR_ALIGNED_BYTES(mem_addr, MAX(dcache_align, _PBUF_IDX_SIZE)),	\
			"Misaligned memory.");						\
	BUILD_ASSERT(size >= (MAX(dcache_align, _PBUF_IDX_SIZE) + _PBUF_IDX_SIZE +	\
			_PBUF_MIN_DATA_LEN), "Insufficient size.");			\
											\
	static const struct pbuf_cfg cfg_##name =					\
			PBUF_CFG_INIT(mem_addr, size, dcache_align);			\
	static struct pbuf name = {							\
		.cfg = &cfg_##name,							\
	}

/**
 * @brief Initialize the packet buffer.
 *
 * This function initializes the packet buffer based on provided configuration.
 * If the configuration is incorrect, the function will return error.
 *
 * It is recommended to use PBUF_DEFINE macro for build time initialization.
 *
 * @param pb	Pointer to the packed buffer containing
 *		configuration and data. Configuration has to be
 *		fixed before the initialization.
 * @retval 0 on success.
 * @retval -EINVAL when the input parameter is incorrect.
 */
int pbuf_init(struct pbuf *pb);

/**
 * @brief Write specified amount of data to the packet buffer.
 *
 * This function call writes specified amount of data to the packet buffer if
 * the buffer will fit the data.
 *
 * @param pb	A buffer to which to write.
 * @param buf	Pointer to the data to be written to the buffer.
 * @param len	Number of bytes to be written to the buffer. Must be positive.
 * @retval int	Number of bytes written, negative error code on fail.
 *		-EINVAL, if any of input parameter is incorrect.
 *		-ENOMEM, if len is bigger than the buffer can fit.
 */

int pbuf_write(struct pbuf *pb, const char *buf, uint16_t len);

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
int pbuf_read(struct pbuf *pb, char *buf, uint16_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_PBUF_H_ */
