/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROCESS_LLEXT_BUF_H_
#define ZEPHYR_INCLUDE_PROCESS_LLEXT_BUF_H_

#include <zephyr/process/process.h>
#include <zephyr/llext/buf_loader.h>
#include <zephyr/llext/llext.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_process_llext {
	struct k_process process;
	struct llext_loader *loader;
	struct llext_load_param *load_param;
	struct llext *ext;
};

struct k_process *k_process_llext_init(struct k_process_llext *subp_llext,
				       const char *name,
				       struct llext_loader *loader,
				       struct llext_load_param *load_param);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PROCESS_LLEXT_FS_H_ */
