/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#include <zephyr/llext/llext.h>
#include <zephyr/llext/buf_loader.h>

static uint8_t llext_buf[] = {
#include "hello_world_ext.inc"
};

int main(void)
{
	LOG_INF("Calling hello world as a module");

	size_t llext_buf_len = ARRAY_SIZE(llext_buf);
	struct llext_buf_loader buf_loader = LLEXT_BUF_LOADER(llext_buf, llext_buf_len);
	struct llext_loader *ldr = &buf_loader.loader;

	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext;
	int res;

	res = llext_load(ldr, "ext", &ext, &ldr_parm);
	if (res != 0) {
		LOG_ERR("Failed to load extension, return code %d\n", res);
		return res;
	}

	void (*hello_world_fn)() = llext_find_sym(&ext->exp_tab, "hello_world");

	if (hello_world_fn == NULL) {
		LOG_ERR("Failed to find symbol\n");
		return -1;
	}

	hello_world_fn();

	return llext_unload(&ext);
}
