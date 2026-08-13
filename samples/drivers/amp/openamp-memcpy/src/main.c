/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/amp.h>
#include <zephyr/drivers/amp/amp_openamp_ops_rproc.h>
#include <zephyr/drivers/amp/amp_openamp_ops_storage.h>

#include <openamp/open_amp.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_loader.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(openamp_sample, LOG_LEVEL_DBG);

#define DT_SAMPLE_CORE DT_NODELABEL(r5fss_1_0)

static struct remoteproc rproc_inst;
static struct amp_full_identification full_core_id = {
	.core_id = AMP_DT_GET_CORE_IDENTIFICATION(DT_SAMPLE_CORE),
	.dev = DEVICE_DT_GET(DT_NODELABEL(amp)),
};

static const uint8_t remote_firmware_elf[] = {
#include "remote_firmware.elf.inc"
};

static struct amp_memcpy_options store_data = {
	.start_address = (uintptr_t) remote_firmware_elf,
	.image_size = ARRAY_SIZE(remote_firmware_elf),
};

int main(void)
{
	int ret;
	struct metal_init_params metal_param = METAL_INIT_DEFAULTS;
	metal_init(&metal_param);

	LOG_INF("Initializing remoteproc instance");
	struct remoteproc *rproc = remoteproc_init(&rproc_inst, &zephyr_openamp_ops, &full_core_id);
	if (rproc == NULL) {
		LOG_ERR("Error initializing rproc");
		ret = -EIO;
		goto end;
	}

	LOG_INF("Getting remote core config pointer from devicetree");
	void *core_options = amp_get_dt_core_config(full_core_id.dev, &full_core_id.core_id);
	if (!core_options) {
		LOG_ERR("Couldn't get core options from devicetree");
		ret = -EINVAL;
		goto end;
	}

	LOG_INF("Configuring remote core");
	ret = remoteproc_config(rproc, core_options);
	if (ret) {
		LOG_ERR("Error configuring remote core %d", ret);
		goto end;
	}

	LOG_INF("Loading remoteproc ELF into correct memory");
	ret = remoteproc_load(rproc, NULL, &store_data, &zephyr_openamp_load_memcpy_ops, NULL);
	if (ret < 0) {
		LOG_ERR("Error in ELF Loading %d", ret);
		goto end;
	}

	LOG_INF("Starting executing code on remote processor");
	ret = remoteproc_start(rproc);
	if (ret < 0) {
		LOG_ERR("Error starting remote core %d", ret);
		goto end;
	}

	LOG_INF("Waiting 10 seconds before stopping the other core again");
	k_sleep(K_SECONDS(10));

	LOG_INF("Stopping remotecore");
	ret = remoteproc_stop(rproc);
	if (ret < 0) {
		LOG_ERR("Error stopping other core %d", ret);
		goto end;
	}

end:
	LOG_INF("Reached end of sample %s", ret < 0 ? "with errors" : "successfully");

	return 0;
}
