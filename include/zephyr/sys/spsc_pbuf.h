/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_SPSC_PBUF_H_
#define ZEPHYR_INCLUDE_SYS_SPSC_PBUF_H_

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Single producer, single consumer packet buffer API
 * @defgroup spsc_buf SPSC (Single producer, single consumer) packet buffer API
 * @ingroup datastructure_apis
 * @{
 */

/**@defgroup SPSC_PBUF_FLAGS SPSC packet buffer flags
 * @{
 */

/** @brief Flag indicating that cache shall be handled. */
#define SPSC_PBUF_CACHE BIT(0)

/** @brief Size of the field which stores maximum utilization. */
#define SPSC_PBUF_UTILIZATION_BITS 24

/** @brief Offset of the field which stores maximum utilization. */
#define SPSC_PBUF_UTILIZATION_OFFSET 8

/**@} */

#if CONFIG_DCACHE_LINE_SIZE != 0
#define Z_SPSC_PBUF_LOCAL_DCACHE_LINE CONFIG_DCACHE_LINE_SIZE
#else
#define Z_SPSC_PBUF_LOCAL_DCACHE_LINE DT_PROP_OR(CPU, d_cache_line_size, 0)
#endif

#ifndef CONFIG_SPSC_PBUF_REMOTE_DCACHE_LINE
#define CONFIG_SPSC_PBUF_REMOTE_DCACHE_LINE 0
#endif

#define Z_SPSC_PBUF_DCACHE_LINE \
	MAX(CONFIG_SPSC_PBUF_REMOTE_DCACHE_LINE, Z_SPSC_PBUF_LOCAL_DCACHE_LINE)

/** @brief Maximum packet length. */
#define SPSC_PBUF_MAX_LEN 0xFF00

/** @brief First part of packet buffer control block.
 *
 * This part contains only data set during the initialization and data touched
 * by the reader. If packet is shared between to cores then data changed by
 * the reader should be on different cache line than the data changed by the
 * writer.
 */
struct spsc_pbuf_common {
	uint32_t len;		/* Length of data[] in bytes. */
	uint32_t flags;		/* Flags. See @ref SPSC_PBUF_FLAGS */
	uint32_t rd_idx;	/* Index of the first valid byte in data[] */
};

/* Padding to fill cache line. */
#define Z_SPSC_PBUF_PADDING \
	MAX(0, Z_SPSC_PBUF_DCACHE_LINE - (int)sizeof(struct spsc_pbuf_common))

/** @brief Remaining part of a packet buffer when cache is used.
 *
 * It contains data that is only changed by the writer. A gap is added to ensure
 * that it is in different cache line than the data changed by the reader.
 */
struct spsc_pbuf_ext_cache {
	uint8_t reserved[Z_SPSC_PBUF_PADDING];
	uint32_t wr_idx;	/* Index of the first free byte in data[] */
	uint8_t data[];		/* Buffer data. */
};

/** @brief Remaining part of a packet buffer when cache is not used. */
struct spsc_pbuf_ext_nocache {
	uint32_t wr_idx;	/* Index of the first free byte in data[] */
	uint8_t data[];		/* Buffer data. */
};

/**
 * @brief Single producer, single consumer packet buffer
 *
 * The SPSC packet buffer implements lightweight unidirectional packet buffer
 * with read/write semantics on top of a memory region shared
 * by the reader and writer. It optionally embeds cache and memory barrier
 * management to ensure correct data access.
 *
 * This structure supports single writer and reader. Data stored in the buffer
 * is encapsulated to a message (with length header).
 *
 */
struct spsc_pbuf {
	struct spsc_pbuf_common common;
	union {
		struct spsc_pbuf_ext_cache cache;
		struct spsc_pbuf_ext_nocache nocache;
	} ext;
};

/** @brief Get buffer capacity.
 *
 * This value is the amount of data that is dedicated for storing packets. Since
 * each packet is prefixed with 2 byte length header, longest possible packet is
 * less than that.
 *
 * @param pb	A buffer.
 *
 * @return Packet buffer capacity.
 */
static inline uint32_t spsc_pbuf_capacity(struct spsc_pbuf *pb)
{
	return pb->common.len - sizeof(uint32_t);
}

/**
 * @brief Initialize the packet buffer.
 *
 * This function initializes the packet buffer on top of a dedicated
 * memory region.
 *
 * @param buf			Pointer to a memory region on which buffer is
 *				created. When cache is used it must be aligned to
 *				Z_SPSC_PBUF_DCACHE_LINE, otherwise it must
 *				be 32 bit word aligned.
 * @param blen			Length of the buffer. Must be large enough to
 *				contain the internal structure and at least two
 *				bytes of data (one is reserved for written
 *				messages length).
 * @param flags			Option flags. See @ref SPSC_PBUF_FLAGS.
 * @retval struct spsc_pbuf*	Pointer to the created buffer. The pointer
 *				points to the same address as buf.
 * @retval NULL			Invalid buffer alignment.
 */
struct spsc_pbuf *spsc_pbuf_init(void *buf, size_t blen, uint32_t flags);

/**
 * @brief Write specified amount of data to the packet buffer.
 *
 * It combines @ref spsc_pbuf_alloc and @ref spsc_pbuf_commit into a single call.
 *
 * @param pb	A buffer to which to write.
 * @param buf	Pointer to the data to be written to the buffer.
 * @param len	Number of bytes to be written to the buffer. Must be positive
 *		but less than @ref SPSC_PBUF_MAX_LEN.
 * @retval int	Number of bytes written, negative error code on fail.
 *		-EINVAL, if len == 0.
 *		-ENOMEM, if len is bigger than the buffer can fit.
 */
int spsc_pbuf_write(struct spsc_pbuf *pb, const char *buf, uint16_t len);

/**
 * @brief Allocate space in the packet buffer.
 *
 * This function attempts to allocate @p len bytes of continuous memory within
 * the packet buffer. An internal padding is added at the end of the buffer, if
 * wrapping occurred during allocation. Apart from padding, allocation does not
 * change the state of the buffer so if after allocation packet is not needed
 * a commit is not needed.
 *
 * Allocated buffer must be committed (@ref spsc_pbuf_commit) to make the packet
 * available for reading.
 *
 * Packet buffer ensures that allocated buffers are 32 bit word aligned.
 *
 * @note If data cache is used, it is the user responsibility to write back the
 * new data.
 *
 * @param[in]  pb	A buffer to which to write.
 * @param[in]  len	Allocation length. Must be positive. If less than @ref SPSC_PBUF_MAX_LEN
 *			then if requested length cannot be allocated, an attempt to allocate
 *			largest possible is performed (which may include adding wrap padding).
 *			If @ref SPSC_PBUF_MAX_LEN is used then an attempt to allocate largest
 *			buffer without applying wrap padding is performed.
 * @param[out] buf	Location where buffer address is written on successful allocation.
 *
 * @retval non-negative Amount of space that got allocated. Can be equal or smaller than %p len.
 * @retval -EINVAL if @p len is forbidden.
 */
int spsc_pbuf_alloc(struct spsc_pbuf *pb, uint16_t len, char **buf);

/**
 * @brief Commit packet to the buffer.
 *
 * Commit a packet which was previously allocated (@ref spsc_pbuf_alloc).
 * If cache is used, cache writeback is performed on the written data.
 *
 * @param pb	A buffer to which to write.
 * @param len	Packet length. Must be equal or less than the length used for allocation.
 */
void spsc_pbuf_commit(struct spsc_pbuf *pb, uint16_t len);

/**
 * @brief Read specified amount of data from the packet buffer.
 *
 * Single read allows to read the message send by the single write.
 * The provided %p buf must be big enough to store the whole message.
 *
 * It combines @ref spsc_pbuf_claim and @ref spsc_pbuf_free into a single call.
 *
 * @param pb		A buffer from which data will be read.
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
 * @brief Claim packet from the buffer.
 *
 * It claims a single packet from the buffer in the order of the commitment
 * by the @ref spsc_pbuf_commit function. The first committed packet will be claimed first.
 * The returned buffer is 32 bit word aligned and points to the continuous memory.
 * Claimed packet must be freed using the @ref spsc_pbuf_free function.
 *
 * @note If data cache is used, cache is invalidate on the packet.
 *
 * @param[in] pb	A buffer from which packet will be claimed.
 * @param[in,out] buf	A location where claimed packet address is written.
 *                      It is 32 bit word aligned and points to the continuous memory.
 *
 * @retval 0 No packets in the buffer.
 * @retval positive packet length.
 */
uint16_t spsc_pbuf_claim(struct spsc_pbuf *pb, char **buf);

/**
 * @brief Free the packet to the buffer.
 *
 * Packet must be claimed (@ref spsc_pbuf_claim) before it can be freed.
 *
 * @param pb	A packet buffer from which packet was claimed.
 * @param len	Claimed packet length.
 */
void spsc_pbuf_free(struct spsc_pbuf *pb, uint16_t len);

/**
 * @brief Get maximum utilization of the packet buffer.
 *
 * Function can be used to tune the buffer size. Feature is enabled by
 * CONFIG_SPSC_PBUF_UTILIZATION. Utilization is updated by the consumer.
 *
 * @param pb	A packet buffer.
 *
 * @retval -ENOTSUP	Feature not enabled.
 * @retval non-negative	Maximum utilization.
 */
int spsc_pbuf_get_utilization(struct spsc_pbuf *pb);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_SPSC_PBUF_H_ */
