/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_MPSC_PBUF_H_
#define ZEPHYR_INCLUDE_SYS_MPSC_PBUF_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/mpsc_packet.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Multi producer, single consumer packet buffer API
 * @defgroup mpsc_buf MPSC (Multi producer, single consumer) packet buffer API
 * @ingroup kernel_apis
 * @{
 */

/*
 * Multi producer, single consumer packet buffer allows to allocate variable
 * length consecutive space for storing a packet. When space is allocated
 * it can be filled by the user (except for the first 2 bits) and when packet
 * is ready it is committed. It is allowed to allocate another packet before
 * committing the previous one.
 *
 * If buffer is full and packet cannot be allocated then null is returned unless
 * overwrite mode is selected. In that mode, oldest entry are dropped (user is
 * notified) until allocation succeeds. It can happen that candidate for
 * dropping is currently being claimed. In that case, it is omitted and next
 * packet is dropped and claimed packet is marked as invalid when freeing.
 *
 * Reading packets is performed in two steps. First packet is claimed. Claiming
 * returns pointer to the packet within the buffer. Packet is freed when no
 * longer in use.
 */

/**@defgroup MPSC_PBUF_FLAGS MPSC packet buffer flags
 * @{
 */

/** @brief Flag indicating that buffer size is power of 2.
 *
 * When buffer size is power of 2 then optimizations are applied.
 */
#define MPSC_PBUF_SIZE_POW2 BIT(0)

/** @brief Flag indicating buffer full policy.
 *
 * If flag is set then when allocating from a full buffer oldest packets are
 * dropped. When flag is not set then allocation returns null.
 */
#define MPSC_PBUF_MODE_OVERWRITE BIT(1)

/** @brief Flag indicating that maximum buffer usage is tracked. */
#define MPSC_PBUF_MAX_UTILIZATION BIT(2)

/** @brief Flag indicated that buffer is currently full. */
#define MPSC_PBUF_FULL BIT(3)

/**@} */

/* Forward declaration */
struct mpsc_pbuf_buffer;

/** @brief Callback prototype for getting length of a packet.
 *
 * @param packet User packet.
 *
 * @return Size of the packet in 32 bit words.
 */
typedef uint32_t (*mpsc_pbuf_get_wlen)(const union mpsc_pbuf_generic *packet);

/** @brief Callback called when packet is dropped.
 *
 * @param buffer Packet buffer.
 *
 * @param packet Packet that is being dropped.
 */
typedef void (*mpsc_pbuf_notify_drop)(const struct mpsc_pbuf_buffer *buffer,
				      const union mpsc_pbuf_generic *packet);

/** @brief MPSC packet buffer structure. */
struct mpsc_pbuf_buffer {
	/** Temporary write index. */
	uint32_t tmp_wr_idx;

	/** Write index. */
	uint32_t wr_idx;

	/** Temporary read index. */
	uint32_t tmp_rd_idx;

	/** Read index. */
	uint32_t rd_idx;

	/** Flags. */
	uint32_t flags;

	/** Lock. */
	struct k_spinlock lock;

	/** User callback called whenever packet is dropped.
	 *
	 * May be NULL if unneeded.
	 */
	mpsc_pbuf_notify_drop notify_drop;

	/** Callback for getting packet length. */
	mpsc_pbuf_get_wlen get_wlen;

	/* Buffer. */
	uint32_t *buf;

	/* Buffer size in 32 bit words. */
	uint32_t size;

	/* Store max buffer usage. */
	uint32_t max_usage;

	struct k_sem sem;
};

/** @brief MPSC packet buffer configuration. */
struct mpsc_pbuf_buffer_config {
	/* Pointer to a memory used for storing packets. */
	uint32_t *buf;

	/* Buffer size in 32 bit words. */
	uint32_t size;

	/* Callbacks. */
	mpsc_pbuf_notify_drop notify_drop;
	mpsc_pbuf_get_wlen get_wlen;

	/* Configuration flags. */
	uint32_t flags;
};

/** @brief Initialize a packet buffer.
 *
 * @param buffer Buffer.
 *
 * @param config Configuration.
 */
void mpsc_pbuf_init(struct mpsc_pbuf_buffer *buffer,
		    const struct mpsc_pbuf_buffer_config *config);

/** @brief Allocate a packet.
 *
 * If a buffer is configured to overwrite mode then if there is no space to
 * allocate a new buffer, oldest packets are dropped. Otherwise allocation
 * fails and null pointer is returned.
 *
 * @param buffer Buffer.
 *
 * @param wlen Number of words to allocate.
 *
 * @param timeout Timeout. If called from thread context it will pend for given
 * timeout if packet cannot be allocated before dropping the oldest or
 * returning null.
 *
 * @return Pointer to the allocated space or null if it cannot be allocated.
 */
union mpsc_pbuf_generic *mpsc_pbuf_alloc(struct mpsc_pbuf_buffer *buffer,
					 size_t wlen, k_timeout_t timeout);

/** @brief Commit a packet.
 *
 * @param buffer Buffer.
 *
 * @param packet Pointer to a packet allocated by @ref mpsc_pbuf_alloc.
 */
void mpsc_pbuf_commit(struct mpsc_pbuf_buffer *buffer,
			union mpsc_pbuf_generic *packet);

/** @brief Put single word packet into a buffer.
 *
 * Function is optimized for storing a packet which fit into a single word.
 * Note that 2 bits of that word is used by the buffer.
 *
 * @param buffer Buffer.
 *
 * @param word Packet content consisting of MPSC_PBUF_HDR with valid bit set
 * and data on remaining bits.
 */
void mpsc_pbuf_put_word(struct mpsc_pbuf_buffer *buffer,
			const union mpsc_pbuf_generic word);

/** @brief Put a packet consisting of a word and a pointer.
 *  *
 * Function is optimized for storing packet consisting of a word and a pointer.
 * Note that 2 bits of a first word is used by the buffer.
 *
 * @param buffer Buffer.
 *
 * @param word First word of a packet consisting of MPSC_PBUF_HDR with valid
 * bit set and data on remaining bits.
 *
 * @param data User data.
 */
void mpsc_pbuf_put_word_ext(struct mpsc_pbuf_buffer *buffer,
			    const union mpsc_pbuf_generic word,
			    const void *data);

/** @brief Put a packet into a buffer.
 *
 * Copy data into a buffer.
 * Note that 2 bits of a first word is used by the buffer.
 *
 * @param buffer Buffer.
 *
 * @param data First word of data must contain MPSC_PBUF_HDR with valid bit set.
 *
 * @param wlen Packet size in words.
 */
void mpsc_pbuf_put_data(struct mpsc_pbuf_buffer *buffer,
			const uint32_t *data, size_t wlen);

/** @brief Claim the first pending packet.
 *
 * @param buffer Buffer.
 *
 * @return Pointer to the claimed packet or null if none available.
 */
const union mpsc_pbuf_generic *mpsc_pbuf_claim(struct mpsc_pbuf_buffer *buffer);

/** @brief Free a packet.
 *
 * @param buffer Buffer.
 *
 * @param packet Packet.
 */
void mpsc_pbuf_free(struct mpsc_pbuf_buffer *buffer,
		    const union mpsc_pbuf_generic *packet);

/** @brief Check if there are any message pending.
 *
 * @param buffer Buffer.
 *
 * @retval true if pending.
 * @retval false if no message is pending.
 */
bool mpsc_pbuf_is_pending(struct mpsc_pbuf_buffer *buffer);

/** @brief Get current memory utilization.
 *
 * @param[in, out] buffer Buffer.
 * @param[out]     size   Buffer size in bytes.
 * @param[out]     now    Current buffer usage in bytes.
 */
void mpsc_pbuf_get_utilization(struct mpsc_pbuf_buffer *buffer,
			       uint32_t *size, uint32_t *now);

/** @brief Get maximum memory utilization.
 *
 * @param[in, out] buffer Buffer.
 * @param[out]     max    Maximum buffer usage in bytes.
 *
 * retval 0 if utilization data collected successfully.
 * retval -ENOTSUP if Collecting utilization data is not supported.
 */
int mpsc_pbuf_get_max_utilization(struct mpsc_pbuf_buffer *buffer, uint32_t *max);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MPSC_PBUF_H_ */
