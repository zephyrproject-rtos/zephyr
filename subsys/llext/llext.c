/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/util.h>
#include <zephyr/llext/elf.h>
#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(llext, CONFIG_LLEXT_LOG_LEVEL);

#include <string.h>

#include "llext_priv.h"

static sys_slist_t _llext_list = SYS_SLIST_STATIC_INIT(&_llext_list);

static struct k_mutex llext_lock = Z_MUTEX_INITIALIZER(llext_lock);

ssize_t llext_find_section(struct llext_loader *ldr, const char *search_name)
{
	/* Note that this API is used after llext_load(), so the ldr->sect_hdrs
	 * cache is already freed. A direct search covers all situations.
	 */

	elf_shdr_t *shdr;
	unsigned int i;
	size_t pos;

	for (i = 0, pos = ldr->hdr.e_shoff;
	     i < ldr->hdr.e_shnum;
	     i++, pos += ldr->hdr.e_shentsize) {
		shdr = llext_peek(ldr, pos);
		if (!shdr) {
			/* The peek() method isn't supported */
			return -ENOTSUP;
		}

		const char *name = llext_peek(ldr,
					      ldr->sects[LLEXT_MEM_SHSTRTAB].sh_offset +
					      shdr->sh_name);

		if (!strcmp(name, search_name)) {
			return shdr->sh_offset;
		}
	}

	return -ENOENT;
}

/*
 * Note, that while we protect the global llext list while searching, we release
 * the lock before returning the found extension to the caller. Therefore it's
 * a responsibility of the caller to protect against races with a freeing
 * context when calling this function.
 */
struct llext *llext_by_name(const char *name)
{
	k_mutex_lock(&llext_lock, K_FOREVER);

	for (sys_snode_t *node = sys_slist_peek_head(&_llext_list);
	     node != NULL;
	     node = sys_slist_peek_next(node)) {
		struct llext *ext = CONTAINER_OF(node, struct llext, _llext_list);

		if (strncmp(ext->name, name, sizeof(ext->name)) == 0) {
			k_mutex_unlock(&llext_lock);
			return ext;
		}
	}

	k_mutex_unlock(&llext_lock);
	return NULL;
}

int llext_iterate(int (*fn)(struct llext *ext, void *arg), void *arg)
{
	sys_snode_t *node;
	int ret = 0;

	k_mutex_lock(&llext_lock, K_FOREVER);

	for (node = sys_slist_peek_head(&_llext_list);
	     node;
	     node = sys_slist_peek_next(node)) {
		struct llext *ext = CONTAINER_OF(node, struct llext, _llext_list);

		ret = fn(ext, arg);
		if (ret) {
			break;
		}
	}

	k_mutex_unlock(&llext_lock);
	return ret;
}

const void *llext_find_sym(const struct llext_symtable *sym_table, const char *sym_name)
{
	if (sym_table == NULL) {
		/* Built-in symbol table */
#ifdef CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID
		/* 'sym_name' is actually a SLID to search for */
		uintptr_t slid = (uintptr_t)sym_name;

		/* TODO: perform a binary search instead of linear.
		 * Note that - as of writing - the llext_const_symbol_area
		 * section is sorted in ascending SLID order.
		 * (see scripts/build/llext_prepare_exptab.py)
		 */
		STRUCT_SECTION_FOREACH(llext_const_symbol, sym) {
			if (slid == sym->slid) {
				return sym->addr;
			}
		}
#else
		STRUCT_SECTION_FOREACH(llext_const_symbol, sym) {
			if (strcmp(sym->name, sym_name) == 0) {
				return sym->addr;
			}
		}
#endif
	} else {
		/* find symbols in module */
		for (size_t i = 0; i < sym_table->sym_cnt; i++) {
			if (strcmp(sym_table->syms[i].name, sym_name) == 0) {
				return sym_table->syms[i].addr;
			}
		}
	}

	return NULL;
}

int llext_load(struct llext_loader *ldr, const char *name, struct llext **ext,
	       const struct llext_load_param *ldr_parm)
{
	int ret;

	*ext = llext_by_name(name);

	k_mutex_lock(&llext_lock, K_FOREVER);

	if (*ext) {
		/* The use count is at least 1 */
		ret = (*ext)->use_count++;
		goto out;
	}

	*ext = llext_alloc(sizeof(struct llext));
	if (*ext == NULL) {
		LOG_ERR("Not enough memory for extension metadata");
		ret = -ENOMEM;
		goto out;
	}

	ret = do_llext_load(ldr, *ext, ldr_parm);
	if (ret < 0) {
		llext_free(*ext);
		*ext = NULL;
		goto out;
	}

	strncpy((*ext)->name, name, sizeof((*ext)->name));
	(*ext)->name[sizeof((*ext)->name) - 1] = '\0';
	(*ext)->use_count++;

	sys_slist_append(&_llext_list, &(*ext)->_llext_list);
	LOG_INF("Loaded extension %s", (*ext)->name);

out:
	k_mutex_unlock(&llext_lock);
	return ret;
}

#include <zephyr/logging/log_ctrl.h>

static void llext_log_flush(void)
{
#ifdef CONFIG_LOG_MODE_DEFERRED
	extern struct k_thread logging_thread;
	int cur_prio = k_thread_priority_get(k_current_get());
	int log_prio = k_thread_priority_get(&logging_thread);
	int target_prio;
	bool adjust_cur, adjust_log;

	/*
	 * Our goal is to raise the logger thread priority above current, but if
	 * current has the highest possble priority, both need to be adjusted,
	 * particularly if the logger thread has the lowest possible priority
	 */
	if (log_prio < cur_prio) {
		adjust_cur = false;
		adjust_log = false;
		target_prio = 0;
	} else if (cur_prio == K_HIGHEST_THREAD_PRIO) {
		adjust_cur = true;
		adjust_log = true;
		target_prio = cur_prio;
		k_thread_priority_set(k_current_get(), cur_prio + 1);
	} else {
		adjust_cur = false;
		adjust_log = true;
		target_prio = cur_prio - 1;
	}

	/* adjust logging thread priority if needed */
	if (adjust_log) {
		k_thread_priority_set(&logging_thread, target_prio);
	}

	log_thread_trigger();
	k_yield();

	if (adjust_log) {
		k_thread_priority_set(&logging_thread, log_prio);
	}
	if (adjust_cur) {
		k_thread_priority_set(&logging_thread, cur_prio);
	}
#endif
}

int llext_unload(struct llext **ext)
{
	__ASSERT(*ext, "Expected non-null extension");
	struct llext *tmp = *ext;

	k_mutex_lock(&llext_lock, K_FOREVER);

	llext_log_flush();

	__ASSERT(tmp->use_count, "A valid LLEXT cannot have a zero use-count!");

	if (tmp->use_count-- != 1) {
		unsigned int ret = tmp->use_count;

		k_mutex_unlock(&llext_lock);
		return ret;
	}

	/* FIXME: protect the global list */
	sys_slist_find_and_remove(&_llext_list, &tmp->_llext_list);

	llext_dependency_remove_all(tmp);

	*ext = NULL;
	k_mutex_unlock(&llext_lock);

	llext_free_regions(tmp);
	llext_free(tmp->sym_tab.syms);
	llext_free(tmp->exp_tab.syms);
	llext_free(tmp);

	return 0;
}

int llext_call_fn(struct llext *ext, const char *sym_name)
{
	void (*fn)(void);

	fn = llext_find_sym(&ext->exp_tab, sym_name);
	if (fn == NULL) {
		return -ENOENT;
	}
	fn();

	return 0;
}

static int call_fn_table(struct llext *ext, bool is_init)
{
	ssize_t ret;

	ret = llext_get_fn_table(ext, is_init, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to get table size: %d", (int)ret);
		return ret;
	}

	typedef void (*elf_void_fn_t)(void);

	int fn_count = ret / sizeof(elf_void_fn_t);
	elf_void_fn_t fn_table[fn_count];

	ret = llext_get_fn_table(ext, is_init, &fn_table, sizeof(fn_table));
	if (ret < 0) {
		LOG_ERR("Failed to get function table: %d", (int)ret);
		return ret;
	}

	for (int i = 0; i < fn_count; i++) {
		LOG_DBG("calling %s function %p()",
			is_init ? "bringup" : "teardown", (void *)fn_table[i]);
		fn_table[i]();
	}

	return 0;
}

inline int llext_bringup(struct llext *ext)
{
	return call_fn_table(ext, true);
}

inline int llext_teardown(struct llext *ext)
{
	return call_fn_table(ext, false);
}

void llext_bootstrap(struct llext *ext, llext_entry_fn_t entry_fn, void *user_data)
{
	int ret;

	/* Call initialization functions */
	ret = llext_bringup(ext);
	if (ret < 0) {
		LOG_ERR("Failed to call init functions: %d", ret);
		return;
	}

	/* Start extension main function */
	LOG_DBG("calling entry function %p(%p)", (void *)entry_fn, user_data);
	entry_fn(user_data);

	/* Call de-initialization functions */
	ret = llext_teardown(ext);
	if (ret < 0) {
		LOG_ERR("Failed to call de-init functions: %d", ret);
		return;
	}
}
