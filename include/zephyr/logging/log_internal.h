/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_INTERNAL_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_INTERNAL_H_

#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/sys/mpsc_pbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Header contains declarations of functions used internally in the logging,
 * shared between various portions of logging subsystem. Functions are internal
 * not intended to be used outside, including logging backends.
 */

/** @brief Structure wrapper to be used for memory section. */
struct log_mpsc_pbuf {
	struct mpsc_pbuf_buffer buf;
};

/** @brief Structure wrapper to be used for memory section. */
struct log_msg_ptr {
	union log_msg_generic *msg;
};

/** @brief Indicate to the log core that one log message has been dropped.
 *
 * @param buffered True if dropped message was already buffered and it is being
 * dropped to free space for another message. False if message is being dropped
 * because allocation failed.
 */
void z_log_dropped(bool buffered);

/** @brief Read and clear current drop indications counter.
 *
 * @return Dropped count.
 */
uint32_t z_log_dropped_read_and_clear(void);

/** @brief Check if there are any pending drop notifications.
 *
 * @retval true Pending unreported drop indications.
 * @retval false No pending unreported drop indications.
 */
bool z_log_dropped_pending(void);

/** @brief Free allocated buffer.
 *
 * @param buf Buffer.
 */
void z_log_free(void *buf);

/* Initialize runtime filters */
void z_log_runtime_filters_init(void);

/* Initialize links. */
void z_log_links_initiate(void);

/* Activate links.
 * Attemp to activate links,
 *
 * @param active_mask     Mask with links to activate. N bit set indicates that Nth
 * link should be activated.
 *
 * @param[in, out] offset Offset assigned to domains. Initialize to 0 before first use.
 *
 * @return Mask with links that still remain inactive.
 */
uint32_t z_log_links_activate(uint32_t active_mask, uint8_t *offset);

/* Notify log_core that a backend was enabled. */
void z_log_notify_backend_enabled(void);

/** @brief Get pointer to the filter set of the log source.
 *
 * @param source_id Source ID.
 *
 * @return Pointer to the filter set.
 */
static inline uint32_t *z_log_dynamic_filters_get(uint32_t source_id)
{
	return &TYPE_SECTION_START(log_dynamic)[source_id].filters;
}

/** @brief Get number of registered sources. */
static inline uint32_t z_log_sources_count(void)
{
	return log_const_source_id(TYPE_SECTION_END(log_const));
}

/** @brief Return number of external domains.
 *
 * @return Number of external domains.
 */
uint8_t z_log_ext_domain_count(void);

/** @brief Initialize module for handling logging message. */
void z_log_msg_init(void);

/** @brief Commit log message.
 *
 * @param msg Message.
 */
void z_log_msg_commit(struct log_msg *msg);

/** @brief Get pending log message.
 *
 * @param[out] backoff Recommended backoff needed to maintain ordering of processed
 * messages. Used only when links are using dedicated buffers.
 *
 * @param Message or null if no pending messages.
 */
union log_msg_generic *z_log_msg_claim(k_timeout_t *backoff);

/** @brief Free message.
 *
 * @param msg Message.
 */
void z_log_msg_free(union log_msg_generic *msg);

/** @brief Check if there are any message pending.
 *
 * @retval true if at least one message is pending.
 * @retval false if no message is pending.
 */
bool z_log_msg_pending(void);

static inline void z_log_notify_drop(const struct mpsc_pbuf_buffer *buffer,
				     const union mpsc_pbuf_generic *item)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(item);

	z_log_dropped(true);
}

/** @brief Get tag.
 *
 * @return Tag. Null if feature is disabled.
 */
const char *z_log_get_tag(void);

/** @brief Check if domain is local.
 *
 * @param domain_id Domain ID.
 *
 * @return True if domain is local.
 */
static inline bool z_log_is_local_domain(uint8_t domain_id)
{
	return !IS_ENABLED(CONFIG_LOG_MULTIDOMAIN) ||
			(domain_id == Z_LOG_LOCAL_DOMAIN_ID);
}

/** @brief Get timestamp.
 *
 * @return Timestamp.
 */
log_timestamp_t z_log_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_INTERNAL_H_ */
