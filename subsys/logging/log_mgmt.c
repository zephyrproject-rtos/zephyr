/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/syscall_handler.h>

/* Implementation of functions related to controlling logging sources and backends:
 * - getting/setting source details like name, filtering
 * - controlling backends filtering
 */

/** @brief Get compiled level of the log source.
 *
 * @param source_id Source ID.
 * @return Level.
 */
static inline uint8_t log_compiled_level_get(uint32_t source_id)
{
	return __log_const_start[source_id].level;
}

void z_log_runtime_filters_init(void)
{
	/*
	 * Initialize aggregated runtime filter levels (no backends are
	 * attached yet, so leave backend slots in each dynamic filter set
	 * alone for now).
	 *
	 * Each log source's aggregated runtime level is set to match its
	 * compile-time level. When backends are attached later on in
	 * log_init(), they'll be initialized to the same value.
	 */
	for (int i = 0; i < z_log_sources_count(); i++) {
		uint32_t *filters = z_log_dynamic_filters_get(i);
		uint8_t level = log_compiled_level_get(i);

		level = MAX(level, CONFIG_LOG_OVERRIDE_LEVEL);
		LOG_FILTER_SLOT_SET(filters,
				    LOG_FILTER_AGGR_SLOT_IDX,
				    level);
	}
}

uint32_t log_src_cnt_get(uint32_t domain_id)
{
	return z_log_sources_count();
}

/** @brief Get name of the log source.
 *
 * @param source_id Source ID.
 * @return Name.
 */
static inline const char *log_name_get(uint32_t source_id)
{
	return __log_const_start[source_id].name;
}

const char *log_source_name_get(uint32_t domain_id, uint32_t src_id)
{
	return src_id < z_log_sources_count() ? log_name_get(src_id) : NULL;
}

int log_source_id_get(const char *name)
{
	for (int i = 0; i < log_src_cnt_get(CONFIG_LOG_DOMAIN_ID); i++) {
		if (strcmp(log_source_name_get(CONFIG_LOG_DOMAIN_ID, i),
			   name) == 0) {
			return i;
		}
	}
	return -1;
}

static uint32_t max_filter_get(uint32_t filters)
{
	uint32_t max_filter = LOG_LEVEL_NONE;
	int first_slot = LOG_FILTER_FIRST_BACKEND_SLOT_IDX;
	int i;

	for (i = first_slot; i < LOG_FILTERS_NUM_OF_SLOTS; i++) {
		uint32_t tmp_filter = LOG_FILTER_SLOT_GET(&filters, i);

		if (tmp_filter > max_filter) {
			max_filter = tmp_filter;
		}
	}

	return max_filter;
}

uint32_t z_impl_log_filter_set(struct log_backend const *const backend,
			       uint32_t domain_id, int16_t source_id,
			       uint32_t level)
{
	__ASSERT_NO_MSG(source_id < (int16_t)z_log_sources_count());

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		uint32_t new_aggr_filter;

		uint32_t *filters = z_log_dynamic_filters_get(source_id);

		if (backend == NULL) {
			struct log_backend const *iter_backend;
			uint32_t max = 0U;
			uint32_t current;

			for (int i = 0; i < log_backend_count_get(); i++) {
				iter_backend = log_backend_get(i);
				current = log_filter_set(iter_backend,
							 domain_id,
							 source_id, level);
				max = MAX(current, max);
			}

			level = max;
		} else {
			uint32_t max = log_filter_get(backend, domain_id,
						      source_id, false);

			level = MIN(level, MAX(max, CONFIG_LOG_OVERRIDE_LEVEL));

			LOG_FILTER_SLOT_SET(filters,
					    log_backend_id_get(backend),
					    level);

			/* Once current backend filter is updated recalculate
			 * aggregated maximal level
			 */
			new_aggr_filter = max_filter_get(*filters);

			LOG_FILTER_SLOT_SET(filters,
					    LOG_FILTER_AGGR_SLOT_IDX,
					    new_aggr_filter);
		}
	}

	return level;
}

#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_log_filter_set(struct log_backend const *const backend,
			    uint32_t domain_id,
			    int16_t src_id,
			    uint32_t level)
{
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(backend == NULL,
		"Setting per-backend filters from user mode is not supported"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(domain_id == CONFIG_LOG_DOMAIN_ID,
		"Invalid log domain_id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(src_id < (int16_t)z_log_sources_count(),
		"Invalid log source id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		(level <= LOG_LEVEL_DBG),
		"Invalid log level"));

	return z_impl_log_filter_set(NULL, domain_id, src_id, level);
}
#include <syscalls/log_filter_set_mrsh.c>
#endif

static void backend_filter_set(struct log_backend const *const backend,
			       uint32_t level)
{
	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		for (int i = 0; i < z_log_sources_count(); i++) {
			log_filter_set(backend, CONFIG_LOG_DOMAIN_ID, i, level);
		}
	}
}

const struct log_backend *log_backend_get_by_name(const char *backend_name)
{
	const struct log_backend *ptr = __log_backends_start;

	while (ptr < __log_backends_end) {
		if (strcmp(backend_name, ptr->name) == 0) {
			return ptr;
		}
		ptr++;
	}
	return NULL;
}

void log_backend_enable(struct log_backend const *const backend,
			void *ctx,
			uint32_t level)
{
	/* As first slot in filtering mask is reserved, backend ID has offset.*/
	uint32_t id = LOG_FILTER_FIRST_BACKEND_SLOT_IDX;

	id += backend - log_backend_get(0);

	log_backend_id_set(backend, id);
	backend_filter_set(backend, level);
	log_backend_activate(backend, ctx);

	z_log_notify_backend_enabled();
}

void log_backend_disable(struct log_backend const *const backend)
{
	log_backend_deactivate(backend);
	backend_filter_set(backend, LOG_LEVEL_NONE);
}

uint32_t log_filter_get(struct log_backend const *const backend,
			uint32_t domain_id, int16_t source_id, bool runtime)
{
	__ASSERT_NO_MSG(source_id < (int16_t)z_log_sources_count());

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) && runtime) {
		if (source_id < 0) {
			return LOG_LEVEL_DBG;
		}

		uint32_t *filters = z_log_dynamic_filters_get(source_id);

		return LOG_FILTER_SLOT_GET(filters,
					   log_backend_id_get(backend));
	}

	return log_compiled_level_get(source_id);
}
