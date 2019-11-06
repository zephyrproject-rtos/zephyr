/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_LINK_H__
#define LOG_LINK_H__

#include <zephyr/types.h>
#include <sys/__assert.h>
#include <logging/log_msg.h>

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

typedef void (*log_link_callback_t)(const struct log_link *link, u8_t *data,
					u32_t length);

struct log_link_api {
	int (*init)(const struct log_link *link, log_link_callback_t callback);
	u16_t (*get_source_count)(const struct log_link *link, u8_t domain_id);
	int (*get_domain_name)(const struct log_link *link, u8_t domain_id,
				char *buf, u32_t *length);
	int (*get_source_name)(const struct log_link *link, u8_t domain_id,
				u16_t source_id, char *buf, u32_t legth);
	int (*get_compiled_level)(const struct log_link *link, u8_t domain_id,
				u16_t source_id, u8_t *level);
	int (*get_runtime_level)(const struct log_link *link, u8_t domain_id,
				u16_t source_id, u8_t *level);
	int (*set_runtime_level)(const struct log_link *link, u8_t domain_id,
				u16_t source_id, u8_t level);
};

struct log_link_ctrl_blk {
	u32_t *filters;
	char **domain_names;
	int timestamp_offset;
	u8_t domain_cnt;
	u8_t domain_offset;
};

struct log_link {
	const struct log_link_api *api;
	const char *name;
	struct log_link_ctrl_blk *ctrl_blk;
	void *ctx;
};

extern const struct log_link __log_links_start[0];
extern const struct log_link __log_links_end[0];

#define LOG_LINK_DEF(_name, _api, _ctx) \
	static struct log_link_ctrl_blk _name##_ctrl_blk; \
	static const Z_STRUCT_SECTION_ITERABLE(log_link, _name) = \
	{ \
		.api = &_api, \
		.name = STRINGIFY(_name), \
		.ctrl_blk = &_name##_ctrl_blk, \
		.ctx = _ctx, \
	}

/** @brief Initialize log link.
 *
 * Init function must initialize domain_cnt field.
 *
 * @param link		Log link instance.
 * @param callback	Callback called on received log data.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_init(const struct log_link *link,
				log_link_callback_t callback)
{
	__ASSERT_NO_MSG(link);

	return link->api->init(link, callback);
}

/** @brief Get number of domains in the link.
 *
 * @param[in] link			Log link instance.
 *
 * @return Number of domains.
 */
static inline u8_t log_link_domains_count(const struct log_link *link)
{
	__ASSERT_NO_MSG(link);

	return link->ctrl_blk->domain_cnt;
}

/** @brief Get number of sources in the domain.
 *
 * @param[in] link		Log link instance.
 * @param[in] domain_id		Domain ID.
 *
 * @return Source count.
 */
static inline u16_t log_link_sources_count(const struct log_link *link,
					      u8_t domain_id)
{
	__ASSERT_NO_MSG(link);

	return link->api->get_source_count(link, domain_id);
}

/** @brief Get domain name.
 *
 * @param[in] link		Log link instance.
 * @param[in] domain_id		Domain ID.
 * @param[out] buf		Output buffer filled with domain name. If NULL
 *				then name length is returned.
 * @param[in,out] length	Buffer size. Name is trimmed if it does not fit
 *				in the buffer and field is set to actual name
 *				length.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_domain_name(const struct log_link *link,
					   u8_t domain_id, char *buf,
					   u32_t *length)
{
	__ASSERT_NO_MSG(link);

	return link->api->get_domain_name(link, domain_id, buf, length);
}

/** @brief Get source name.
 *
 * @param[in] link	Log link instance.
 * @param[in] domain_id	Domain ID.
 * @param[in] source_id	Source ID.
 * @param[out] buf	Output buffer filled with source name.
 * @param[in] length	Buffer size. If name is longer than buffer size then up
 *			to buffer size is copied and null terminated.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_source_name(const struct log_link *link,
					   u8_t domain_id, u16_t source_id,
					   char *buf, u32_t length)
{
	__ASSERT_NO_MSG(link);
	__ASSERT_NO_MSG(buf);

	return link->api->get_source_name(link, domain_id, source_id,
					buf, length);
}

/** @brief Get compiled level of the given source.
 *
 * @param[in] link	Log link instance.
 * @param[in] domain_id	Domain ID.
 * @param[in] source_id	Source ID.
 * @param[out] level	Location to store requested level.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_compiled_level(const struct log_link *link,
					     u8_t domain_id, u16_t source_id,
					     u8_t *level)
{
	__ASSERT_NO_MSG(link);
	__ASSERT_NO_MSG(level);

	return link->api->get_compiled_level(link, domain_id, source_id, level);
}

/** @brief Get runtime level of the given source.
 *
 * @param[in] link	Log link instance.
 * @param[in] domain_id	Domain ID.
 * @param[in] source_id	Source ID.
 * @param[out] level	Location to store requested level.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_runtime_level(const struct log_link *link,
					     u8_t domain_id, u16_t source_id,
					     u8_t *level)
{
	__ASSERT_NO_MSG(link);
	__ASSERT_NO_MSG(level);

	return link->api->get_runtime_level(link, domain_id, source_id, level);
}

/** @brief Set runtime level of the given source.
 *
 * @param[in] link	Log link instance.
 * @param[in] domain_id	Domain ID.
 * @param[in] source_id	Source ID.
 * @param[out] level	Requested level.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_set_runtime_level(const struct log_link *link,
					     u8_t domain_id, u16_t source_id,
					     u8_t level)
{
	__ASSERT_NO_MSG(link);
	__ASSERT_NO_MSG(level);

	return link->api->set_runtime_level(link, domain_id, source_id, level);
}

/**
 * @brief Get number of log links.
 *
 * @return Number of log links.
 */
static inline int log_link_count(void)
{
	return __log_links_end - __log_links_start;
}

/**
 * @brief Get log link.
 *
 * @param[in] idx  Index of the log link instance.
 *
 * @return    Log link instance.
 */
static inline const struct log_link *log_link_get(u32_t idx)
{
	return &__log_links_start[idx];
}

/**
 * @brief Prepare remote log message.
 *
 * Align timestamp and reset reference counter.
 *
 * @param link	Log link instance.
 * @param msg	Log message.
 */
static inline void log_link_msg_prepare(const struct log_link *link,
					struct log_msg *msg)
{
	msg->hdr.ref_cnt = 1;
	msg->hdr.timestamp += link->ctrl_blk->timestamp_offset;
	msg->hdr.ids.domain_id += link->ctrl_blk->domain_offset;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LOG_LINK_H__ */
