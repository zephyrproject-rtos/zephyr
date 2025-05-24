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

int llext_section_shndx(const struct llext_loader *ldr, const struct llext *ext,
			const char *sect_name)
{
	unsigned int i;

	for (i = 1; i < ext->sect_cnt; i++) {
		const char *name = llext_section_name(ldr, ext, ext->sect_hdrs + i);

		if (!strcmp(name, sect_name)) {
			return i;
		}
	}

	return -ENOENT;
}

int llext_get_section_header(struct llext_loader *ldr, struct llext *ext, const char *search_name,
			     elf_shdr_t *shdr)
{
	int ret;

	ret = llext_section_shndx(ldr, ext, search_name);
	if (ret < 0) {
		return ret;
	}

	*shdr = ext->sect_hdrs[ret];
	return 0;
}

ssize_t llext_find_section(struct llext_loader *ldr, const char *search_name)
{
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

		if (strncmp(ext->name, name, LLEXT_MAX_NAME_LEN) == 0) {
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

	/* The (*ext)->name array is LLEXT_MAX_NAME_LEN + 1 bytes long */
	strncpy((*ext)->name, name, LLEXT_MAX_NAME_LEN);
	(*ext)->name[LLEXT_MAX_NAME_LEN] = '\0';
	(*ext)->use_count++;

	sys_slist_append(&_llext_list, &(*ext)->_llext_list);
	LOG_INF("Loaded extension %s", (*ext)->name);

out:
	k_mutex_unlock(&llext_lock);
	return ret;
}

#include <zephyr/logging/log_ctrl.h>

int llext_unload(struct llext **ext)
{
	__ASSERT(*ext, "Expected non-null extension");
	struct llext *tmp = *ext;

	/* Flush pending log messages, as the deferred formatting may be referencing
	 * strings/args in the extension we are about to unload
	 */
	log_flush();

	k_mutex_lock(&llext_lock, K_FOREVER);

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

	if (tmp->sect_hdrs_on_heap) {
		llext_free(tmp->sect_hdrs);
	}

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

/*
 * Prepare a set of extension copies for future restoring. The user has copied
 * multiple extensions and their dependencies into a flat array. We have to
 * relink dependency pointers to copies in this array for later restoring. Note,
 * that all dependencies have to be in this array, if any are missing, restoring
 * will fail, so we return an error in such cases.
 */
int llext_relink_dependency(struct llext *ext, unsigned int n_ext)
{
	unsigned int i, j, k;

	for (i = 0; i < n_ext; i++) {
		for (j = 0; ext[i].dependency[j] && j < ARRAY_SIZE(ext[i].dependency); j++) {
			for (k = 0; k < n_ext; k++) {
				if (!strncmp(ext[k].name, ext[i].dependency[j]->name,
				    sizeof(ext[k].name))) {
					ext[i].dependency[j] = ext + k;
					LOG_DBG("backup %s depends on %s",
						ext[i].name, ext[k].name);
					break;
				}
			}

			if (k == n_ext)
				return -ENOENT;
		}
	}

	return 0;
}

int llext_restore(struct llext **ext, struct llext_loader **ldr, unsigned int n_ext)
{
	struct llext_elf_sect_map **map = llext_alloc(sizeof(*map) * n_ext);
	struct llext *next, *tmp, *first = ext[0], *last = ext[n_ext - 1];
	unsigned int i, j, n_map, n_exp_tab;
	int ret;

	if (!map) {
		LOG_ERR("cannot allocate list of maps of %u", sizeof(*map) * n_ext);
		return -ENOMEM;
	}

	/*
	 * Each extension has a section map, so if this loop completes
	 * successfully in the end n_map == n_ext. But if it's terminated
	 * because of an allocation failure, we need to know how many maps have
	 * to be freed.
	 */
	for (n_map = 0, n_exp_tab = 0; n_map < n_ext; n_map++) {
		/* Need to allocate individually, because that's how they're freed */
		map[n_map] = llext_alloc(sizeof(**map) * ext[n_map]->sect_cnt);
		if (!map[n_map]) {
			LOG_ERR("cannot allocate section map of %u",
				sizeof(**map) * ext[n_map]->sect_cnt);
			ret = -ENOMEM;
			goto free_map;
		}

		/* Not every extension exports symbols, count those that do */
		if (ext[n_map]->exp_tab.sym_cnt)
			n_exp_tab++;
	}

	/* Array of pointers to exported symbol tables. Can be NULL if n_exp_tab == 0 */
	struct llext_symbol **exp_tab = llext_alloc(sizeof(*exp_tab) * n_exp_tab);

	if (n_exp_tab) {
		if (!exp_tab) {
			LOG_ERR("cannot allocate list of exported symbol tables of %u",
				sizeof(*exp_tab) * n_exp_tab);
			ret = -ENOMEM;
			goto free_map;
		}

		/* Now actually allocate new exported symbol lists */
		for (i = 0, j = 0; i < n_ext; i++) {
			if (ext[i]->exp_tab.sym_cnt) {
				size_t size = sizeof(**exp_tab) * ext[i]->exp_tab.sym_cnt;

				exp_tab[j] = llext_alloc(size);
				if (!exp_tab[j]) {
					LOG_ERR("cannot allocate exported symbol table of %u",
						size);
					ret = -ENOMEM;
					goto free_sym;
				}
				memcpy(exp_tab[j++], ext[i]->exp_tab.syms, size);
			}
		}
	}

	k_mutex_lock(&llext_lock, K_FOREVER);

	/* Allocate extensions and add them to the global list */
	for (i = 0, j = 0; i < n_ext; i++) {
		next = llext_alloc(sizeof(*next));
		if (!next) {
			LOG_ERR("cannot allocate LLEXT of %u", sizeof(*next));
			ret = -ENOMEM;
			goto free_llext;
		}

		*next = *ext[i];
		ext[i] = next;
		if (next->exp_tab.sym_cnt) {
			next->exp_tab.syms = exp_tab[j++];
		}

		sys_slist_append(&_llext_list, &next->_llext_list);
	}

	k_mutex_unlock(&llext_lock);

	/* Copy section maps */
	for (i = 0; i < n_ext; i++) {
		size_t map_size = sizeof(struct llext_elf_sect_map) * ext[i]->sect_cnt;

		memcpy(map[i], ldr[i]->sect_map, map_size);
		ldr[i]->sect_map = map[i];
	}

	llext_free(map);
	llext_free(exp_tab);

	/* Restore dependencies */
	SYS_SLIST_FOR_EACH_CONTAINER(&_llext_list, next, _llext_list) {
		for (j = 0; next->dependency[j] && j < ARRAY_SIZE(next->dependency); j++) {
			if (next->dependency[j] < first || next->dependency[j] >= last) {
				/* Inconsistency detected */
				LOG_ERR("dependency out of range");
				ret = -EINVAL;
				goto free_locked;
			}

			next->dependency[j] = llext_by_name(next->dependency[j]->name);
			if (!next->dependency[j]) {
				/* Bug in the algorithm */
				LOG_ERR("dependency not found");
				ret = -EFAULT;
				goto free_locked;
			}

			LOG_DBG("restore %s depends on %s",
				next->name, next->dependency[j]->name);
		}
	}

	return 0;

free_locked:
	k_mutex_lock(&llext_lock, K_FOREVER);

free_llext:
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&_llext_list, next, tmp, _llext_list) {
		for (i = 0; i < n_ext; i++) {
			if (ext[i] == next) {
				sys_slist_remove(&_llext_list, NULL, &next->_llext_list);
				llext_free(next);
				ext[i] = NULL;
				break;
			}
		}
	}

	k_mutex_unlock(&llext_lock);

free_sym:
	for (i = 0; i < n_exp_tab && exp_tab[i]; i++) {
		llext_free(exp_tab[i]);
	}

	llext_free(exp_tab);

free_map:
	for (i = 0; i < n_map; i++) {
		llext_free(map[i]);
	}

	llext_free(map);

	return ret;
}
