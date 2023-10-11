/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_LINK_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_LINK_H_

#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log link API
 * @defgroup log_link Log link API
 * @ingroup logger
 * @{
 */

struct log_link;

typedef void (*log_link_callback_t)(const struct log_link *link,
				    union log_msg_generic *msg);

typedef void (*log_link_dropped_cb_t)(const struct log_link *link,
				      uint32_t dropped);

struct log_link_config {
	log_link_callback_t msg_cb;
	log_link_dropped_cb_t dropped_cb;
};

struct log_link_api {
	int (*initiate)(const struct log_link *link, struct log_link_config *config);
	int (*activate)(const struct log_link *link);
	int (*get_domain_name)(const struct log_link *link, uint32_t domain_id,
				char *buf, size_t *length);
	int (*get_source_name)(const struct log_link *link, uint32_t domain_id,
				uint16_t source_id, char *buf, size_t *length);
	int (*get_levels)(const struct log_link *link, uint32_t domain_id,
				uint16_t source_id, uint8_t *level,
				uint8_t *runtime_level);
	int (*set_runtime_level)(const struct log_link *link, uint32_t domain_id,
				uint16_t source_id, uint8_t level);
};

struct log_link_ctrl_blk {
	uint32_t domain_cnt;
	uint16_t source_cnt[1 + COND_CODE_1(CONFIG_LOG_MULTIDOMAIN,
					    (CONFIG_LOG_REMOTE_DOMAIN_MAX_COUNT),
					    (0))];
	uint32_t domain_offset;
	uint32_t *filters;
};

struct log_link {
	const struct log_link_api *api;
	const char *name;
	struct log_link_ctrl_blk *ctrl_blk;
	void *ctx;
	struct mpsc_pbuf_buffer *mpsc_pbuf;
	const struct mpsc_pbuf_buffer_config *mpsc_pbuf_config;
};

/** @brief Create instance of a log link.
 *
 * Link can have dedicated buffer for messages if @p _buf_len is positive. In
 * that case messages will be processed in an order since logging core will
 * attempt to fetch message from all available buffers (default and links) and
 * process the one with the earliest timestamp. If strict ordering is not needed
 * then dedicated buffer may be omitted (@p _buf_len set to 0). That results in
 * better memory utilization but unordered messages passed to backends.
 *
 * @param _name     Instance name.
 * @param _api      API list. See @ref log_link_api.
 * @param _buf_wlen Size (in words) of dedicated buffer for messages from this buffer.
 *		    If 0 default buffer is used.
 * @param _ctx      Context (void *) associated with the link.
 */
#define LOG_LINK_DEF(_name, _api, _buf_wlen, _ctx) \
	static uint32_t __aligned(Z_LOG_MSG2_ALIGNMENT) _name##_buf32[_buf_wlen]; \
	static const struct mpsc_pbuf_buffer_config _name##_mpsc_pbuf_config = { \
		.buf = (uint32_t *)_name##_buf32, \
		.size = _buf_wlen, \
		.notify_drop = z_log_notify_drop, \
		.get_wlen = log_msg_generic_get_wlen, \
		.flags = IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW) ? \
			MPSC_PBUF_MODE_OVERWRITE : 0 \
	}; \
	COND_CODE_0(_buf_wlen, (), (static STRUCT_SECTION_ITERABLE(log_msg_ptr, \
								   _name##_log_msg_ptr);)) \
	static STRUCT_SECTION_ITERABLE_ALTERNATE(log_mpsc_pbuf, \
						 mpsc_pbuf_buffer, \
						 _name##_log_mpsc_pbuf); \
	static struct log_link_ctrl_blk _name##_ctrl_blk; \
	static const STRUCT_SECTION_ITERABLE(log_link, _name) = \
	{ \
		.api = &_api, \
		.name = STRINGIFY(_name), \
		.ctrl_blk = &_name##_ctrl_blk, \
		.ctx = _ctx, \
		.mpsc_pbuf = _buf_wlen ? &_name##_log_mpsc_pbuf : NULL, \
		.mpsc_pbuf_config = _buf_wlen ? &_name##_mpsc_pbuf_config : NULL \
	}

/** @brief Initiate log link.
 *
 * Function initiates the link. Since initialization procedure may be time
 * consuming, function returns before link is ready to not block logging
 * initialization. @ref log_link_activate is called to complete link initialization.
 *
 * @param link		Log link instance.
 * @param config	Configuration.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_initiate(const struct log_link *link,
				   struct log_link_config *config)
{
	__ASSERT_NO_MSG(link);

	return link->api->initiate(link, config);
}

/** @brief Activate log link.
 *
 * Function checks if link is initilized and completes initialization process.
 * When successfully returns, link is ready with domain and sources count fetched
 * and timestamp details updated.
 *
 * @param link		Log link instance.
 *
 * @retval 0 When successfully activated.
 * @retval -EINPROGRESS Activation in progress.
 */
static inline int log_link_activate(const struct log_link *link)
{
	__ASSERT_NO_MSG(link);

	return link->api->activate(link);
}

/** @brief Check if link is activated.
 *
 * @param link		Log link instance.
 *
 * @retval 0 When successfully activated.
 * @retval -EINPROGRESS Activation in progress.
 */
static inline int log_link_is_active(const struct log_link *link)
{
	return link->ctrl_blk->domain_offset > 0 ? 0 : -EINPROGRESS;
}

/** @brief Get number of domains in the link.
 *
 * @param[in] link	Log link instance.
 *
 * @return Number of domains.
 */
static inline uint8_t log_link_domains_count(const struct log_link *link)
{
	__ASSERT_NO_MSG(link);

	return link->ctrl_blk->domain_cnt;
}

/** @brief Get number of sources in the domain.
 *
 * @param[in] link		Log link instance.
 * @param[in] domain_id		Relative domain ID.
 *
 * @return Source count.
 */
static inline uint16_t log_link_sources_count(const struct log_link *link,
					      uint32_t domain_id)
{
	__ASSERT_NO_MSG(link);

	return link->ctrl_blk->source_cnt[domain_id];
}

/** @brief Get domain name.
 *
 * @param[in] link		Log link instance.
 * @param[in] domain_id		Relative domain ID.
 * @param[out] buf		Output buffer filled with domain name. If NULL
 *				then name length is returned.
 * @param[in,out] length	Buffer size. Name is trimmed if it does not fit
 *				in the buffer and field is set to actual name
 *				length.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_domain_name(const struct log_link *link,
					   uint32_t domain_id, char *buf,
					   size_t *length)
{
	__ASSERT_NO_MSG(link);

	return link->api->get_domain_name(link, domain_id, buf, length);
}

/** @brief Get source name.
 *
 * @param[in] link	Log link instance.
 * @param[in] domain_id	Relative domain ID.
 * @param[in] source_id	Source ID.
 * @param[out] buf	Output buffer filled with source name.
 * @param[in,out] length	Buffer size. Name is trimmed if it does not fit
 *				in the buffer and field is set to actual name
 *				length.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_source_name(const struct log_link *link,
					   uint32_t domain_id, uint16_t source_id,
					   char *buf, size_t *length)
{
	__ASSERT_NO_MSG(link);
	__ASSERT_NO_MSG(buf);

	return link->api->get_source_name(link, domain_id, source_id,
					buf, length);
}

/** @brief Get level settings of the given source.
 *
 * @param[in] link	Log link instance.
 * @param[in] domain_id	Relative domain ID.
 * @param[in] source_id	Source ID.
 * @param[out] level	Location to store requested compile time level.
 * @param[out] runtime_level Location to store requested runtime time level.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_levels(const struct log_link *link,
				      uint32_t domain_id, uint16_t source_id,
				      uint8_t *level, uint8_t *runtime_level)
{
	__ASSERT_NO_MSG(link);

	return link->api->get_levels(link, domain_id, source_id,
				     level, runtime_level);
}

/** @brief Set runtime level of the given source.
 *
 * @param[in] link	Log link instance.
 * @param[in] domain_id	Relative domain ID.
 * @param[in] source_id	Source ID.
 * @param[out] level	Requested level.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_set_runtime_level(const struct log_link *link,
					     uint32_t domain_id, uint16_t source_id,
					     uint8_t level)
{
	__ASSERT_NO_MSG(link);
	__ASSERT_NO_MSG(level);

	return link->api->set_runtime_level(link, domain_id, source_id, level);
}

/**
 * @brief Enqueue external log message.
 *
 * Add log message to processing queue. Log message is created outside local
 * core. For example it maybe coming from external domain.
 *
 * @param link Log link instance.
 * @param data Message from remote domain.
 * @param len  Length in bytes.
 */
void z_log_msg_enqueue(const struct log_link *link, const void *data, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_LINK_H_ */
