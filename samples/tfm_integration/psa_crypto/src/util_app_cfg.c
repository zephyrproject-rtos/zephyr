/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <logging/log.h>

#include "psa/error.h"
#include "psa/protected_storage.h"
#include "util_app_cfg.h"
#include "util_app_log.h"

/** The 64-bit UID associated with the config record in secure storage. */
static psa_storage_uid_t cfg_data_uid = 0x0000000055CFDA7A;

LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

/**
 * @brief Default config settings. These settings will be used when a new
 *        config record is created, or when the config record is reset.
 */
static struct cfg_data cfg_data_dflt = {
	.magic = 0x55CFDA7A,
	.version = 1,
	.scratch = { 0 }
};

psa_status_t cfg_create_data(void)
{
	psa_status_t status;

	LOG_INF("app_cfg: Creating new config file with UID 0x%llX",
		(uint64_t)cfg_data_uid);

	/*
	 * psa_ps_create can also be used here, which enables to the use of
	 * the psa_ps_set_extended function, but this requires us to set a
	 * maximum file size for resource allocation. Since the upper limit
	 * isn't known at present, we opt here for the simpler psa_ps_set
	 * call which also creates the secure storage record if necessary,
	 * but precludes the use of psa_ps_set_extended.
	 */
	status = psa_ps_set(cfg_data_uid, sizeof(cfg_data_dflt),
			    (void *)&cfg_data_dflt, 0);
	if (status) {
		goto err;
	}

err:
	return (status ? al_psa_status(status, __func__) : status);
}

psa_status_t cfg_load_data(struct cfg_data *p_cfg_data)
{
	psa_status_t status;
	struct psa_storage_info_t p_info;

	memset(&p_info, 0, sizeof(p_info));

	/* Check if the config record exists, if not create it. */
	status = psa_ps_get_info(cfg_data_uid, &p_info);
	if (status == PSA_ERROR_DOES_NOT_EXIST) {
		/* Create a new config file. */
		status = cfg_create_data();
		/* Copy default values to the cfg_data placeholder. */
		memcpy(p_cfg_data, &cfg_data_dflt, sizeof(cfg_data_dflt));
	}
	if (status) {
		goto err;
	}

err:
	return (status ? al_psa_status(status, __func__) : status);
}
