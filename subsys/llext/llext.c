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
	unsigned int i;
	int ret = 0;

	k_mutex_lock(&llext_lock, K_FOREVER);

	for (node = sys_slist_peek_head(&_llext_list), i = 0;
	     node;
	     node = sys_slist_peek_next(node), i++) {
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
	       struct llext_load_param *ldr_parm)
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

int llext_unload(struct llext **ext)
{
	__ASSERT(*ext, "Expected non-null extension");
	struct llext *tmp = *ext;

	k_mutex_lock(&llext_lock, K_FOREVER);
	__ASSERT(tmp->use_count, "A valid LLEXT cannot have a zero use-count!");

	if (tmp->use_count-- != 1) {
		unsigned int ret = tmp->use_count;

		k_mutex_unlock(&llext_lock);
		return ret;
	}

	/* FIXME: protect the global list */
	sys_slist_find_and_remove(&_llext_list, &tmp->_llext_list);

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
