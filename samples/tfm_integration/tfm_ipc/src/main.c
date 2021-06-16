/*
 * Copyright (c) 2019,2020, 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <power/reboot.h>

#include "tfm_api.h"
#include "tfm_ns_interface.h"
#ifdef TFM_PSA_API
#include "psa_manifest/sid.h"
#endif

/**
 * \brief Retrieve the version of the PSA Framework API.
 *
 * \note This is a functional test only and doesn't
 *       mean to test all possible combinations of
 *       input parameters and return values.
 */
static void tfm_ipc_test_01(void)
{
	uint32_t version;

	version = psa_framework_version();
	if (version == PSA_FRAMEWORK_VERSION) {
		printk("The version of the PSA Framework API is %d.\n",
		       version);
	} else {
		printk("The version of the PSA Framework API is not valid!\n");
		return;
	}
}

/**
 * Retrieve the minor version of a RoT Service.
 *
 * @return N/A
 */
static void tfm_ipc_test_02(void)
{
	uint32_t version;

	version = psa_version(TFM_CRYPTO_SID);
	if (version == PSA_VERSION_NONE) {
		printk("RoT Service is not implemented or caller is not ");
		printk("authorized to access it!\n");
		return;
	}

	/* Valid version number */
	printk("The minor version is %d.\n", version);
}

/**
 * Connect to a RoT Service by its SID.
 *
 * @return N/A
 */
static void tfm_ipc_test_03(void)
{
	psa_handle_t handle;

	handle = psa_connect(TFM_CRYPTO_SID,
			     TFM_CRYPTO_VERSION);
	if (handle > 0) {
		printk("Connect success!\n");
	} else {
		printk("The RoT Service has refused the connection!\n");
		return;
	}
	psa_close(handle);
}

void main(void)
{
	tfm_ipc_test_01();
	tfm_ipc_test_02();
	tfm_ipc_test_03();

	printk("TF-M IPC on %s\n", CONFIG_BOARD);

	k_sleep(K_MSEC(5000));
	sys_reboot(0);
}
