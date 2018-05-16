/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log_msg.h>
#include "log_list.h"
#include <logging/log.h>
#include <logging/log_backend.h>
#include <logging/log_ctrl.h>
#include <logging/log_output.h>
#include <misc/printk.h>
#include <assert.h>
#include <stdio.h>

#ifndef CONFIG_LOG_PRINTK_MAX_STRING_LENGTH
#define CONFIG_LOG_PRINTK_MAX_STRING_LENGTH 1
#endif

#ifdef CONFIG_LOG_BACKEND_CONSOLE
#include <logging/log_backend_console.h>
LOG_BACKEND_CONSOLE_DEFINE(log_backend_console);
#endif

static struct log_list_t list;

static bool panic_mode;
static bool initialized;

static u32_t nullstamp_func(void)
{
	return 0;
}

static timestamp_get_t timestamp_func = nullstamp_func;



static inline void msg_finalize(struct log_msg *p_msg,
				struct log_msg_ids src_level)
{
	p_msg->hdr.ids = src_level;
	p_msg->hdr.timestamp = timestamp_func();
	unsigned int key = irq_lock();

	log_list_add_tail(&list, p_msg);

	irq_unlock(key);

	if (panic_mode) {
		(void)log_process();
	}
}
void log_0(const char *p_str, struct log_msg_ids src_level)
{
	struct log_msg *p_msg = log_msg_create_0(p_str);

	if (p_msg == NULL) {
		return;
	}
	msg_finalize(p_msg, src_level);
}

void log_1(const char *p_str,
	   u32_t arg0,
	   struct log_msg_ids src_level)
{
	struct log_msg *p_msg = log_msg_create_1(p_str, arg0);

	if (p_msg == NULL) {
		return;
	}
	msg_finalize(p_msg, src_level);
}

void log_2(const char *p_str,
	   u32_t arg0,
	   u32_t arg1,
	   struct log_msg_ids src_level)
{
	struct log_msg *p_msg = log_msg_create_2(p_str, arg0, arg1);

	if (p_msg == NULL) {
		return;
	}

	msg_finalize(p_msg, src_level);
}

void log_3(const char *p_str,
	   u32_t arg0,
	   u32_t arg1,
	   u32_t arg2,
	   struct log_msg_ids src_level)
{
	struct log_msg *p_msg = log_msg_create_3(p_str, arg0, arg1, arg2);

	if (p_msg == NULL) {
		return;
	}

	msg_finalize(p_msg, src_level);
}

void log_4(const char *p_str,
	   u32_t arg0,
	   u32_t arg1,
	   u32_t arg2,
	   u32_t arg3,
	   struct log_msg_ids src_level)
{
	struct log_msg *p_msg = log_msg_create_4(p_str, arg0, arg1,
						 arg2, arg3);

	if (p_msg == NULL) {
		return;
	}

	msg_finalize(p_msg, src_level);
}

void log_5(const char *p_str,
	   u32_t arg0,
	   u32_t arg1,
	   u32_t arg2,
	   u32_t arg3,
	   u32_t arg4,
	   struct log_msg_ids src_level)
{
	struct log_msg *p_msg = log_msg_create_5(p_str, arg0, arg1,
						 arg2, arg3, arg4);

	if (p_msg == NULL) {
		return;
	}

	msg_finalize(p_msg, src_level);
}

void log_6(const char *p_str,
	   u32_t arg0,
	   u32_t arg1,
	   u32_t arg2,
	   u32_t arg3,
	   u32_t arg4,
	   u32_t arg5,
	   struct log_msg_ids src_level)
{
	struct log_msg *p_msg = log_msg_create_6(p_str, arg0, arg1, arg2,
						 arg3, arg4, arg5);

	if (p_msg == NULL) {
		return;
	}

	msg_finalize(p_msg, src_level);
}

void log_hexdump(const u8_t *p_data,
		 u32_t length,
		 struct log_msg_ids src_level)
{
	struct log_msg *p_msg = log_msg_hexdump_create(p_data, length);

	if (p_msg == NULL) {
		return;
	}

	msg_finalize(p_msg, src_level);
}

int log_printk(const char *fmt, va_list ap)
{
	if (IS_ENABLED(CONFIG_LOG_PRINTK)) {
		if (!initialized) {
			log_init(nullstamp_func, 0);
		}

		u8_t formatted_str[CONFIG_LOG_PRINTK_MAX_STRING_LENGTH];

		int length = vsnprintf(formatted_str, sizeof(formatted_str), fmt, ap);


		length = (length > sizeof(formatted_str)) ?
			 sizeof(formatted_str) : length;

		struct log_msg *p_msg = log_msg_hexdump_create(formatted_str,
							       length);
		if (p_msg == NULL) {
			return 0;
		}

		p_msg->hdr.params.hexdump.raw_string = 1;
		struct log_msg_ids empty_id = { 0 };
		msg_finalize(p_msg, empty_id);

		return length;
	} else   {
		return 0;
	}
}

int log_init(timestamp_get_t timestamp_getter, u32_t freq)
{

	assert(log_backend_count_get() < LOG_FILTERS_NUM_OF_SLOTS);

	if (timestamp_getter) {
		timestamp_func = timestamp_getter;
		log_output_timestamp_freq_set(freq);
	} else   {
		return -EINVAL;
	}

	if (!initialized) {
		log_list_init(&list);

		/* Assign ids to backends. */
		for (int i = 0; i < log_backend_count_get(); i++) {
			log_backend_id_set(log_backend_get(i),
					   i + LOG_FILTER_FIRST_BACKEND_SLOT_IDX);
		}

		panic_mode = false;
		initialized = true;
	}

#ifdef CONFIG_LOG_BACKEND_CONSOLE
	log_backend_console_init();
	log_backend_enable(&log_backend_console,
			   NULL,
			   CONFIG_LOG_DEFAULT_LEVEL);
#endif
	return 0;
}

void log_panic(void)
{
	struct log_backend const *p_backend;

	for (int i = 0; i < log_backend_count_get(); i++) {
		p_backend = log_backend_get(i);

		if (log_backend_is_active(p_backend)) {
			log_backend_panic(p_backend);
		}
	}

	panic_mode = true;

	/* Flush */
	while (log_process() == true) {
	}
}

static bool msg_filter_check(struct log_backend const *p_backend,
			     struct log_msg *p_msg)
{
	u32_t backend_level;
	u32_t msg_level;

	backend_level = log_filter_get(p_backend,
				       log_msg_domain_id_get(p_msg),
				       log_msg_source_id_get(p_msg),
				       true /*enum RUNTIME, COMPILETIME*/);
	msg_level = log_msg_level_get(p_msg);

	return (msg_level <= backend_level);
}

static void msg_process(struct log_msg *p_msg)
{
	struct log_backend const *p_backend;

	for (int i = 0; i < log_backend_count_get(); i++) {
		p_backend = log_backend_get(i);

		if (log_backend_is_active(p_backend) &&
		    msg_filter_check(p_backend, p_msg)) {
			log_backend_put(p_backend, p_msg);
		}
	}
	log_msg_put(p_msg);
}

bool log_process(void)
{
	struct log_msg *p_msg;

	unsigned int key = irq_lock();

	p_msg = log_list_head_get(&list);
	irq_unlock(key);

	if (p_msg != NULL) {
		msg_process(p_msg);
	}

	return (log_list_head_peek(&list) != NULL);
}

u32_t log_src_cnt_get(u32_t domain_id)
{
	return LOG_ROM_SECTION_ITEM_COUNT();
}

const char *log_source_name_get(u32_t domain_id, u32_t src_id)
{
	assert(src_id < LOG_ROM_SECTION_ITEM_COUNT());

	return LOG_SOURCE_NAME_GET(src_id);
}

static u32_t max_filter_get(u32_t filters)
{
	u32_t max_filter = LOG_LEVEL_NONE;
	int i;

	for (i = LOG_FILTER_FIRST_BACKEND_SLOT_IDX; i < LOG_FILTERS_NUM_OF_SLOTS; i++) {
		u32_t tmp_filter = LOG_FILTER_SLOT_GET(&filters, i);

		if (tmp_filter > max_filter) {
			max_filter = tmp_filter;
		}
	}
	return max_filter;
}

void log_filter_set(struct log_backend const *const p_backend,
		    u32_t domain_id,
		    u32_t src_id,
		    u32_t level)
{
	assert(src_id < LOG_ROM_SECTION_ITEM_COUNT());

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		struct log_module_ram_data *p_ram_data;
		u32_t new_aggr_filter;

		p_ram_data = LOG_RAM_SECTION_ITEM_GET(src_id);

		if (p_backend == NULL) {
			struct log_backend const *p_backend;

			for (int i = 0; i < log_backend_count_get(); i++) {
				p_backend = log_backend_get(i);
				log_filter_set(p_backend, domain_id,
					       src_id, level);
			}
		} else   {
			LOG_FILTER_SLOT_SET(&p_ram_data->filters,
					    log_backend_id_get(p_backend),
					    level);

			/* Once current backend filter is updated recalculate
			 * aggregated maximal level */
			new_aggr_filter = max_filter_get(p_ram_data->filters);

			LOG_FILTER_SLOT_SET(&p_ram_data->filters,
					    LOG_FILTER_AGGR_SLOT_IDX,
					    new_aggr_filter);
		}
	}
}

static void backend_filter_set(struct log_backend const *const p_backend,
			       u32_t level)
{
	int i;

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		for (i = 0; i < LOG_RAM_SECTION_ITEM_COUNT; i++) {
			log_filter_set(p_backend,
				       CONFIG_LOG_DOMAIN_ID,
				       i,
				       level);

		}
	}
}

void log_backend_enable(struct log_backend const *const p_backend,
			void *p_ctx,
			u32_t level)
{
	log_backend_activate(p_backend, p_ctx);
	backend_filter_set(p_backend, level);
}

void log_backend_disable(struct log_backend const *const p_backend)
{
	log_backend_deactivate(p_backend);
	backend_filter_set(p_backend, LOG_LEVEL_NONE);
}

u32_t log_filter_get(struct log_backend const *const p_backend,
		     u32_t domain_id,
		     u32_t src_id,
		     bool runtime)
{
	assert(src_id < LOG_ROM_SECTION_ITEM_COUNT());

	if (IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) && runtime) {
		struct log_module_ram_data *p_ram_data;
		p_ram_data = LOG_RAM_SECTION_ITEM_GET(src_id);

		return LOG_FILTER_SLOT_GET(&p_ram_data->filters,
					   log_backend_id_get(p_backend));
	} else   {
		return LOG_SOURCE_STATIC_LEVEL_GET(src_id);
	}
}
