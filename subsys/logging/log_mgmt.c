/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_link.h>
#include "log_cache.h"

LOG_MODULE_REGISTER(log_mgmt);

#ifndef CONFIG_LOG_DOMAIN_NAME_CACHE_ENTRY_SIZE
#define CONFIG_LOG_DOMAIN_NAME_CACHE_ENTRY_SIZE 1
#endif

#ifndef CONFIG_LOG_DOMAIN_NAME_CACHE_ENTRY_COUNT
#define CONFIG_LOG_DOMAIN_NAME_CACHE_ENTRY_COUNT 1
#endif

#ifndef CONFIG_LOG_SOURCE_NAME_CACHE_ENTRY_SIZE
#define CONFIG_LOG_SOURCE_NAME_CACHE_ENTRY_SIZE 1
#endif

#ifndef CONFIG_LOG_SOURCE_NAME_CACHE_ENTRY_COUNT
#define CONFIG_LOG_SOURCE_NAME_CACHE_ENTRY_COUNT 1
#endif

#define DCACHE_BUF_SIZE \
	(CONFIG_LOG_DOMAIN_NAME_CACHE_ENTRY_SIZE + sizeof(struct log_cache_entry)) * \
	CONFIG_LOG_DOMAIN_NAME_CACHE_ENTRY_COUNT

#define SCACHE_BUF_SIZE \
	(CONFIG_LOG_SOURCE_NAME_CACHE_ENTRY_SIZE + sizeof(struct log_cache_entry)) * \
	CONFIG_LOG_SOURCE_NAME_CACHE_ENTRY_COUNT

static uint8_t dname_cache_buffer[DCACHE_BUF_SIZE] __aligned(sizeof(uint32_t));
static uint8_t sname_cache_buffer[SCACHE_BUF_SIZE] __aligned(sizeof(uint32_t));

static struct log_cache dname_cache;
static struct log_cache sname_cache;

struct log_source_id {
	uint8_t domain_id;
	uint16_t source_id;
};

union log_source_ids {
	struct log_source_id id;
	uintptr_t raw;
};

static bool domain_id_cmp(uintptr_t id0, uintptr_t id1)
{
	return id0 == id1;
}

static bool source_id_cmp(uintptr_t id0, uintptr_t id1)
{
	union log_source_ids s0 = { .raw = id0 };
	union log_source_ids s1 = { .raw = id1 };

	return (s0.id.source_id == s1.id.source_id) &&
		(s0.id.domain_id == s1.id.domain_id);
}

/* Implementation of functions related to controlling logging sources and backends:
 * - getting/setting source details like name, filtering
 * - controlling backends filtering
 */

/** @brief Return link and relative domain id based on absolute domain id.
 *
 * @param[in]  domain_id	Aboslute domain ID.
 * @param[out] rel_domain_id	Domain ID elative to the link domain ID as output.
 *
 * @return Link to which given domain belongs. NULL if link was not found.
 */
static const struct log_link *get_link_domain(uint8_t domain_id, uint8_t *rel_domain_id)
{
	uint8_t domain_max;

	STRUCT_SECTION_FOREACH(log_link, link) {
		domain_max = link->ctrl_blk->domain_offset +
				link->ctrl_blk->domain_cnt;
		if (domain_id < domain_max) {

			*rel_domain_id = domain_id - link->ctrl_blk->domain_offset;
			return link;
		}
	}

	*rel_domain_id = 0;

	return NULL;
}

/** @brief Get source offset used for getting runtime filter.
 *
 * Runtime filters for each link are dynamically allocated as an array of
 * filters for all domains in the link. In order to fetch link associated with
 * given source an index in the array must be retrieved.
 */
static uint32_t get_source_offset(const struct log_link *link,
				  uint8_t rel_domain_id)
{
	uint32_t offset = 0;

	for (uint8_t i = 0; i < rel_domain_id; i++) {
		offset += log_link_sources_count(link, i);
	}

	return offset;
}

uint32_t *z_log_link_get_dynamic_filter(uint8_t domain_id, uint32_t source_id)
{
	uint8_t rel_domain_id;
	const struct log_link *link = get_link_domain(domain_id, &rel_domain_id);
	uint32_t source_offset = 0;

	__ASSERT_NO_MSG(link != NULL);

	source_offset = get_source_offset(link, rel_domain_id);

	return &link->ctrl_blk->filters[source_offset + source_id];
}

#ifdef CONFIG_LOG_MULTIDOMAIN
static int link_filters_init(const struct log_link *link)
{
	uint32_t total_cnt = get_source_offset(link, link->ctrl_blk->domain_cnt);

	link->ctrl_blk->filters = k_malloc(sizeof(uint32_t) * total_cnt);
	if (link->ctrl_blk->filters == NULL) {
		LOG_ERR("Failed to allocate buffer for runtime filtering.");
		__ASSERT(0, "Failed to allocate buffer.");
		return -ENOMEM;
	}

	memset(link->ctrl_blk->filters, 0, sizeof(uint32_t) * total_cnt);
	LOG_DBG("%s: heap used for filters:%d",
		link->name, (int)(total_cnt * sizeof(uint32_t)));

	return 0;
}
#endif

static void cache_init(void)
{
	int err;
	static const struct log_cache_config dname_cache_config = {
		.buf = dname_cache_buffer,
		.buf_len = sizeof(dname_cache_buffer),
		.item_size = CONFIG_LOG_DOMAIN_NAME_CACHE_ENTRY_SIZE,
		.cmp = domain_id_cmp
	};
	static const struct log_cache_config sname_cache_config = {
		.buf = sname_cache_buffer,
		.buf_len = sizeof(sname_cache_buffer),
		.item_size = CONFIG_LOG_SOURCE_NAME_CACHE_ENTRY_SIZE,
		.cmp = source_id_cmp
	};

	err = log_cache_init(&dname_cache, &dname_cache_config);
	__ASSERT_NO_MSG(err == 0);

	err = log_cache_init(&sname_cache, &sname_cache_config);
	__ASSERT_NO_MSG(err == 0);
}

uint8_t z_log_ext_domain_count(void)
{
	uint8_t cnt = 0;

	STRUCT_SECTION_FOREACH(log_link, link) {
		cnt += log_link_domains_count(link);
	}

	return cnt;
}

static uint16_t link_source_count(uint8_t domain_id)
{
	uint8_t rel_domain_id;
	const struct log_link *link = get_link_domain(domain_id, &rel_domain_id);

	__ASSERT_NO_MSG(link != NULL);

	return log_link_sources_count(link, rel_domain_id);
}

uint32_t log_src_cnt_get(uint32_t domain_id)
{
	if (z_log_is_local_domain(domain_id)) {
		return z_log_sources_count();
	}

	return link_source_count(domain_id);
}

/* First check in cache if not there fetch from remote.
 * When fetched from remote put in cache.
 *
 * @note Execution time depends on whether entry is in cache.
 */
static const char *link_source_name_get(uint8_t domain_id, uint32_t source_id)
{
	uint8_t *cached;
	size_t cache_size = sname_cache.item_size;
	union log_source_ids id = {
		.id = {
			.domain_id = domain_id,
			.source_id = source_id
		}
	};

	/* If not in cache fetch from link and cache it. */
	if (!log_cache_get(&sname_cache, id.raw, &cached)) {
		uint8_t rel_domain_id;
		const struct log_link *link = get_link_domain(domain_id, &rel_domain_id);
		int err;

		__ASSERT_NO_MSG(link != NULL);

		err = log_link_get_source_name(link, rel_domain_id, source_id,
					       cached, &cache_size);
		if (err < 0) {
			return NULL;
		}

		log_cache_put(&sname_cache, cached);
	}

	return (const char *)cached;
}

const char *log_source_name_get(uint32_t domain_id, uint32_t source_id)
{
	if (z_log_is_local_domain(domain_id)) {
		if (source_id < log_src_cnt_get(domain_id)) {
			return TYPE_SECTION_START(log_const)[source_id].name;
		} else {
			return NULL;
		}
	}

	return link_source_name_get(domain_id, source_id);
}

/* First check in cache if not there fetch from remote.
 * When fetched from remote put in cache.
 *
 * @note Execution time depends on whether entry is in cache.
 */
static const char *link_domain_name_get(uint8_t domain_id)
{
	uint8_t *cached;
	size_t cache_size = dname_cache.item_size;
	uintptr_t id = (uintptr_t)domain_id;
	static const char *invalid_domain = "invalid";

	/* If not in cache fetch from link and cache it. */
	if (!log_cache_get(&dname_cache, id, &cached)) {
		uint8_t rel_domain_id;
		const struct log_link *link = get_link_domain(domain_id, &rel_domain_id);
		int err;

		__ASSERT_NO_MSG(link != NULL);

		err = log_link_get_domain_name(link, rel_domain_id, cached, &cache_size);
		if (err < 0) {
			log_cache_release(&dname_cache, cached);
			return invalid_domain;
		}

		log_cache_put(&dname_cache, cached);
	}

	return (const char *)cached;
}

const char *log_domain_name_get(uint32_t domain_id)
{
	if (z_log_is_local_domain(domain_id)) {
		return CONFIG_LOG_DOMAIN_NAME;
	}

	return link_domain_name_get(domain_id);
}

static uint8_t link_compiled_level_get(uint8_t domain_id, uint32_t source_id)
{
	uint8_t rel_domain_id;
	const struct log_link *link = get_link_domain(domain_id, &rel_domain_id);
	uint8_t level;

	__ASSERT_NO_MSG(link != NULL);

	return !log_link_get_levels(link, rel_domain_id, source_id, &level, NULL) ?
		level : 0;
}

uint8_t log_compiled_level_get(uint8_t domain_id, uint32_t source_id)
{
	if (z_log_is_local_domain(domain_id)) {
		if (source_id < log_src_cnt_get(domain_id)) {
			return TYPE_SECTION_START(log_const)[source_id].level;
		} else {
			return LOG_LEVEL_NONE;
		}
	}

	return link_compiled_level_get(domain_id, source_id);
}

int z_log_link_set_runtime_level(uint8_t domain_id, uint16_t source_id, uint8_t level)
{
	uint8_t rel_domain_id;
	const struct log_link *link = get_link_domain(domain_id, &rel_domain_id);

	__ASSERT_NO_MSG(link != NULL);

	return log_link_set_runtime_level(link, rel_domain_id, source_id, level);
}

static uint32_t *get_dynamic_filter(uint8_t domain_id, uint32_t source_id)
{
	if (z_log_is_local_domain(domain_id)) {
		return &__log_dynamic_start[source_id].filters;
	}

	return z_log_link_get_dynamic_filter(domain_id, source_id);
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
		uint8_t level = log_compiled_level_get(Z_LOG_LOCAL_DOMAIN_ID, i);

		level = MAX(level, CONFIG_LOG_OVERRIDE_LEVEL);
		LOG_FILTER_SLOT_SET(filters,
				    LOG_FILTER_AGGR_SLOT_IDX,
				    level);
	}
}

int log_source_id_get(const char *name)
{
	for (int i = 0; i < log_src_cnt_get(Z_LOG_LOCAL_DOMAIN_ID); i++) {
		const char *sname = log_source_name_get(Z_LOG_LOCAL_DOMAIN_ID, i);

		if ((sname != NULL) && (strcmp(sname, name) == 0)) {
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

static void set_runtime_filter(uint8_t backend_id, uint8_t domain_id,
			       uint32_t source_id, uint32_t level)
{
	uint32_t prev_max;
	uint32_t new_max;
	uint32_t *filters = get_dynamic_filter(domain_id, source_id);

	prev_max = LOG_FILTER_SLOT_GET(filters, LOG_FILTER_AGGR_SLOT_IDX);

	LOG_FILTER_SLOT_SET(filters, backend_id, level);

	/* Once current backend filter is updated recalculate
	 * aggregated maximal level
	 */
	new_max = max_filter_get(*filters);

	LOG_FILTER_SLOT_SET(filters, LOG_FILTER_AGGR_SLOT_IDX, new_max);

	if (!z_log_is_local_domain(domain_id) && (new_max != prev_max)) {
		(void)z_log_link_set_runtime_level(domain_id, source_id, level);
	}
}

uint32_t z_impl_log_filter_set(struct log_backend const *const backend,
			       uint32_t domain_id, int16_t source_id,
			       uint32_t level)
{
	if (!IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		return level;
	}

	__ASSERT_NO_MSG(source_id < log_src_cnt_get(domain_id));


	if (backend == NULL) {
		uint32_t max = 0U;

		STRUCT_SECTION_FOREACH(log_backend, iter_backend) {
			uint32_t current = log_filter_set(iter_backend,
						 domain_id, source_id, level);

			max = MAX(current, max);
		}

		return max;
	}

	level = MIN(level, MAX(log_filter_get(backend, domain_id, source_id, false),
			       CONFIG_LOG_OVERRIDE_LEVEL));
	set_runtime_filter(log_backend_id_get(backend), domain_id, source_id, level);

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
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(domain_id == Z_LOG_LOCAL_DOMAIN_ID,
		"Invalid log domain_id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(src_id < (int16_t)log_src_cnt_get(domain_id),
		"Invalid log source id"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(
		(level <= LOG_LEVEL_DBG),
		"Invalid log level"));

	return z_impl_log_filter_set(NULL, domain_id, src_id, level);
}
#include <syscalls/log_filter_set_mrsh.c>
#endif

static void link_filter_set(const struct log_link *link,
			    struct log_backend const *const backend,
			    uint32_t level)
{
	if (log_link_is_active(link) != 0) {
		return;
	}

	for (uint8_t d = link->ctrl_blk->domain_offset;
	     d < link->ctrl_blk->domain_offset + link->ctrl_blk->domain_cnt; d++) {
		for (uint16_t s = 0; s < log_src_cnt_get(d); s++) {
			log_filter_set(backend, d, s, level);
		}
	}
}

static void backend_filter_set(struct log_backend const *const backend,
			       uint32_t level)
{
	if (!IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		return;
	}

	for (uint16_t s = 0; s < log_src_cnt_get(0); s++) {
		log_filter_set(backend, 0, s, level);
	}

	if (!IS_ENABLED(CONFIG_LOG_MULTIDOMAIN)) {
		return;
	}

	/* Set level in activated links. */
	STRUCT_SECTION_FOREACH(log_link, link) {
		link_filter_set(link, backend, level);
	}
}

const struct log_backend *log_backend_get_by_name(const char *backend_name)
{
	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (strcmp(backend_name, backend->name) == 0) {
			return backend;
		}
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
	backend->cb->level = level;
	backend_filter_set(backend, level);
	log_backend_activate(backend, ctx);

	z_log_notify_backend_enabled();
}

void log_backend_disable(struct log_backend const *const backend)
{
	if (log_backend_is_active(backend)) {
		backend_filter_set(backend, LOG_LEVEL_NONE);
	}

	log_backend_deactivate(backend);
}

uint32_t log_filter_get(struct log_backend const *const backend,
			uint32_t domain_id, int16_t source_id, bool runtime)
{
	__ASSERT_NO_MSG(source_id < log_src_cnt_get(domain_id));

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) && runtime) {
		if (source_id < 0) {
			return LOG_LEVEL_DBG;
		}

		return LOG_FILTER_SLOT_GET(get_dynamic_filter(domain_id, source_id),
					   log_backend_id_get(backend));
	}

	return log_compiled_level_get(domain_id, source_id);
}

void z_log_links_initiate(void)
{
	int err;

	cache_init();

	STRUCT_SECTION_FOREACH(log_link, link) {
#ifdef CONFIG_MPSC_PBUF
		if (link->mpsc_pbuf) {
			mpsc_pbuf_init(link->mpsc_pbuf, link->mpsc_pbuf_config);
		}
#endif

		err = log_link_initiate(link, NULL);
		__ASSERT(err == 0, "Failed to initialize link");
	}
}

#ifdef CONFIG_LOG_MULTIDOMAIN
static void backends_link_init(const struct log_link *link)
{
	for (int i = 0; i < log_backend_count_get(); i++) {
		const struct log_backend *backend = log_backend_get(i);


		if (!log_backend_is_active(backend)) {
			continue;
		}

		link_filter_set(link, backend, backend->cb->level);
	}
}

uint32_t z_log_links_activate(uint32_t active_mask, uint8_t *offset)
{
	uint32_t mask = 0x1;
	uint32_t out_mask = 0;

	/* Initiate offset to 1. */
	if (*offset == 0) {
		*offset = 1;
	}

	STRUCT_SECTION_FOREACH(log_link, link) {
		if (active_mask & mask) {
			int err = log_link_activate(link);

			if (err == 0) {
				uint8_t domain_cnt = log_link_domains_count(link);

				link->ctrl_blk->domain_offset = *offset;
				link->ctrl_blk->domain_cnt = domain_cnt;
				*offset += domain_cnt;
				if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
					link_filters_init(link);
					backends_link_init(link);
				}
			} else {
				__ASSERT_NO_MSG(err == -EINPROGRESS);
				out_mask |= mask;
			}
		}

		mask <<= 1;
	}

	return out_mask;
}
#endif
