/*
 * Copyright (c) 2024 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_bbram.h>

#include "it8xxx2.h"

#define DT_DRV_COMPAT ite_it8xxx2_bbram

struct bbram_it8xxx2_emul_config {
	const struct device *dev;
};

#define GET_CONFIG(target)                                                                         \
	((const struct bbram_it8xxx2_config                                                        \
		  *)(((const struct bbram_it8xxx2_emul_config *)((target)->cfg))->dev->config))

static int it8xxx2_emul_backend_set_data(const struct emul *target, size_t offset, size_t count,
					 const uint8_t *buffer)
{
	const struct bbram_it8xxx2_config *config = GET_CONFIG(target);

	if (offset + count > config->size) {
		return -ERANGE;
	}

	bytecpy(((uint8_t *)config->base_addr + offset), buffer, count);
	return 0;
}

static int it8xxx2_emul_backend_get_data(const struct emul *target, size_t offset, size_t count,
					 uint8_t *buffer)
{
	const struct bbram_it8xxx2_config *config = GET_CONFIG(target);

	if (offset + count > config->size) {
		return -ERANGE;
	}

	bytecpy(buffer, ((uint8_t *)config->base_addr + offset), count);
	return 0;
}

static const struct emul_bbram_backend_api it8xxx2_emul_backend_api = {
	.set_data = it8xxx2_emul_backend_set_data,
	.get_data = it8xxx2_emul_backend_get_data,
};

#define BBRAM_EMUL_INIT(inst)                                                                      \
	static struct bbram_it8xxx2_emul_config bbram_it8xxx2_emul_config_##inst = {               \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(inst, NULL, NULL, &bbram_it8xxx2_emul_config_##inst, NULL,             \
			    &it8xxx2_emul_backend_api)

DT_INST_FOREACH_STATUS_OKAY(BBRAM_EMUL_INIT);
