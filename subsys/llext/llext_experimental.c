/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/llext_internal.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#include <string.h>

#include "llext_priv.h"

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

			if (k == n_ext) {
				return -ENOENT;
			}
		}
	}

	return 0;
}

int llext_restore(struct llext **ext, struct llext_loader **ldr, unsigned int n_ext)
{
	struct llext_elf_sect_map **map = llext_alloc_data(sizeof(*map) * n_ext);
	struct llext *next, *tmp, *first = ext[0], *last = ext[n_ext - 1];
	unsigned int i, j, n_map, n_exp_tab;
	int ret;

	if (!map) {
		LOG_ERR("cannot allocate list of maps of %zu", sizeof(*map) * n_ext);
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
		map[n_map] = llext_alloc_data(sizeof(**map) * ext[n_map]->sect_cnt);
		if (!map[n_map]) {
			LOG_ERR("cannot allocate section map of %zu",
				sizeof(**map) * ext[n_map]->sect_cnt);
			ret = -ENOMEM;
			goto free_map;
		}

		/* Not every extension exports symbols, count those that do */
		if (ext[n_map]->exp_tab.sym_cnt) {
			n_exp_tab++;
		}
	}

	/* Array of pointers to exported symbol tables. Can be NULL if n_exp_tab == 0 */
	struct llext_symbol **exp_tab = llext_alloc_data(sizeof(*exp_tab) * n_exp_tab);

	if (n_exp_tab) {
		if (!exp_tab) {
			LOG_ERR("cannot allocate list of exported symbol tables of %zu",
				sizeof(*exp_tab) * n_exp_tab);
			ret = -ENOMEM;
			goto free_map;
		}

		/* Now actually allocate new exported symbol lists */
		for (i = 0, j = 0; i < n_ext; i++) {
			if (ext[i]->exp_tab.sym_cnt) {
				size_t size = sizeof(**exp_tab) * ext[i]->exp_tab.sym_cnt;

				exp_tab[j] = llext_alloc_data(size);
				if (!exp_tab[j]) {
					LOG_ERR("cannot allocate exported symbol table of %zu",
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
		next = llext_alloc_data(sizeof(*next));
		if (!next) {
			LOG_ERR("cannot allocate LLEXT of %zu", sizeof(*next));
			ret = -ENOMEM;
			goto free_llext;
		}

		/* Copy the extension and return a pointer to it to the user */
		*next = *ext[i];
		ext[i] = next;
		if (next->exp_tab.sym_cnt) {
			next->exp_tab.syms = exp_tab[j++];
		}

		sys_slist_append(&llext_list, &next->llext_list);
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

	/* Restore dependencies previously saved by llext_relink_dependency() */
	SYS_SLIST_FOR_EACH_CONTAINER(&llext_list, next, llext_list) {
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
	/* Free only those extensions, that we've saved into ext[] */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&llext_list, next, tmp, llext_list) {
		for (i = 0; i < n_ext; i++) {
			if (ext[i] == next) {
				sys_slist_remove(&llext_list, NULL, &next->llext_list);
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
