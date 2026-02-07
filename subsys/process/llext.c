/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/process/llext.h>
#include <zephyr/internal/process.h>

static int process_llext_load(struct k_process *process, k_process_entry_t *entry)
{
	struct k_process_llext *llext;
	int ret;

	llext = CONTAINER_OF(process, struct k_process_llext, process);
	llext->ext = NULL;
	ret = llext_load(llext->loader,
			 k_process_get_name(process),
			 &llext->ext,
			 llext->load_param);
	if (ret) {
		return ret;
	}

	*entry = (k_process_entry_t)llext_find_sym(&llext->ext->exp_tab, "entry");
	if (*entry == NULL) {
		llext_unload(&llext->ext);
		return -EAGAIN;
	}

#ifdef CONFIG_USERSPACE
	ret = llext_add_domain(llext->ext, k_process_get_domain(process));
	if (ret) {
		llext_unload(&llext->ext);
		return ret;
	}
#endif

	return 0;
}

static void process_llext_unload(struct k_process *process)
{
	struct k_process_llext *llext;

	llext = CONTAINER_OF(process, struct k_process_llext, process);
	llext_unload(&llext->ext);
}

struct k_process *k_process_llext_init(struct k_process_llext *llext,
				       const char *name,
				       struct llext_loader *loader,
				       struct llext_load_param *load_param)
{
	llext->loader = loader;
	llext->load_param = load_param;

	k_process_init(&llext->process,
			name,
			process_llext_load,
			process_llext_unload);

	return &llext->process;
}
