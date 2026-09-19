/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the log link interface.
 * @ingroup log_link
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_LINK_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_LINK_H_

#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup log_link Log links
 * @ingroup logger
 * @brief Interface for log links bringing in messages from remote domains.
 * @{
 */

struct log_link;

/**
 * @brief Callback invoked by a link for each received log message.
 *
 * @param link Link instance.
 * @param msg  Message received from the remote domain.
 */
typedef void (*log_link_callback_t)(const struct log_link *link,
				    union log_msg_generic *msg);

/**
 * @brief Callback invoked by a link to report dropped messages.
 *
 * @param link    Link instance.
 * @param dropped Number of messages dropped since the previous notification.
 */
typedef void (*log_link_dropped_cb_t)(const struct log_link *link,
				      uint32_t dropped);

/** @brief Log link configuration passed to the link at initiation time. */
struct log_link_config {
	log_link_callback_t msg_cb;        /**< Callback for received messages. */
	log_link_dropped_cb_t dropped_cb;  /**< Callback for dropped messages. */
};

/**
 * @brief Log link API.
 *
 * Set of operations implemented by a log link backend.
 */
struct log_link_api {
	/** @brief Initiate the link (see log_link_initiate()). */
	int (*initiate)(const struct log_link *link, struct log_link_config *config);
	/** @brief Complete link activation (see log_link_activate()). */
	int (*activate)(const struct log_link *link);
	/** @brief Get a source name (see log_link_get_source_name()). */
	int (*get_source_name)(const struct log_link *link, uint16_t source_id,
			char *buf, size_t *length);
	/** @brief Get level settings of a source (see log_link_get_levels()). */
	int (*get_levels)(const struct log_link *link, uint16_t source_id,
			uint8_t *level, uint8_t *runtime_level);
	/** @brief Set runtime level of a source (see log_link_set_runtime_level()). */
	int (*set_runtime_level)(const struct log_link *link,
				uint16_t source_id, uint8_t level);
	/** @brief Get a message from the link (see log_link_get_msg()). */
	union log_msg_generic *(*get_msg)(const struct log_link *link);
	/** @brief Release a message back to the link (see log_link_put_msg()). */
	void (*put_msg)(const struct log_link *link, union log_msg_generic *msg);
};

/** @brief Run-time control block for a @ref log_link instance. */
struct log_link_ctrl_blk {
	/** @cond INTERNAL_HIDDEN */
	uint16_t source_cnt;
	uint16_t domain_offset;
	const char **log_str_ptr;
	struct log_source_const_data *sources;
	uint32_t *filters;
	/** @endcond */
};

/** @brief Log link instance. */
struct log_link {
	const struct log_link_api *api; /**< Link operations. */
	const char *name;               /**< Unique link name. */
	struct log_link_ctrl_blk *ctrl_blk; /**< Run-time control block. */
	void *ctx;                      /**< Context associated with the link. */
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
 * @param _ctx      Context (void *) associated with the link.
 */
#define LOG_LINK_DEFINE(_name, _api, _ctx) \
	static struct log_link_ctrl_blk _name##_ctrl_blk; \
	static const STRUCT_SECTION_ITERABLE(log_link, _name) = \
	{ \
		.api = &_api, \
		.name = STRINGIFY(_name), \
		.ctrl_blk = &_name##_ctrl_blk, \
		.ctx = _ctx, \
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
 * Function checks if link is initialized and completes initialization process.
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
	return link->ctrl_blk->source_cnt > 0 ? 0 : -EINPROGRESS;
}

/** @brief Get number of sources in the domain.
 *
 * @param[in] link		Log link instance.
 *
 * @return Source count.
 */
static inline uint16_t log_link_sources_count(const struct log_link *link)
{
	__ASSERT_NO_MSG(link);

	return link->ctrl_blk->source_cnt;
}

/** @brief Get source name.
 *
 * @param[in] link	Log link instance.
 * @param[in] source_id	Source ID.
 * @param[out] buf	Output buffer filled with source name.
 * @param[in,out] length	Buffer size. Name is trimmed if it does not fit
 *				in the buffer and field is set to actual name
 *				length.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_source_name(const struct log_link *link, uint16_t source_id,
					   char *buf, size_t *length)
{
	__ASSERT_NO_MSG(link);
	__ASSERT_NO_MSG(buf);

	return link->api->get_source_name(link, source_id, buf, length);
}

/** @brief Get level settings of the given source.
 *
 * @param[in] link	Log link instance.
 * @param[in] source_id	Source ID.
 * @param[out] level	Location to store requested compile time level.
 * @param[out] runtime_level Location to store requested runtime time level.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_get_levels(const struct log_link *link, uint16_t source_id,
				      uint8_t *level, uint8_t *runtime_level)
{
	__ASSERT_NO_MSG(link);

	return link->api->get_levels(link, source_id, level, runtime_level);
}

/** @brief Set runtime level of the given source.
 *
 * @param[in] link	Log link instance.
 * @param[in] source_id	Source ID.
 * @param[in] level	Requested level.
 *
 * @return 0 on success or error code.
 */
static inline int log_link_set_runtime_level(const struct log_link *link,
					     uint16_t source_id, uint8_t level)
{
	__ASSERT_NO_MSG(link);
	__ASSERT_NO_MSG(level);

	return link->api->set_runtime_level(link, source_id, level);
}

/** @brief Get a message from the link.
 *
 * Multiple calls without putting back (releasing) the message will return the same message.
 *
 * @param link Log link instance.
 *
 * @return Pointer to a message or NULL if no message is available.
 */
static inline union log_msg_generic *log_link_get_msg(const struct log_link *link)
{
	return link->api->get_msg(link);
}

/** @brief Release a message back to the link.
 *
 * After releasing the message, it can be freed.
 *
 * @param link Log link instance.
 * @param msg Pointer to a message.
 */
static inline void log_link_put_msg(const struct log_link *link, union log_msg_generic *msg)
{
	link->api->put_msg(link, msg);
}

/**
 * @brief Notify logging thread that new messages are available.
 *
 * @param new_msgs Number of new messages.
 */
void z_log_msg_remote_notify(size_t new_msgs);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_LINK_H_ */
