/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

#include "tfm_api.h"
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
static void tfm_ipc_test_1001(void)
{
	u32_t version;

	version = tfm_psa_framework_version_veneer();
	if (version == PSA_FRAMEWORK_VERSION) {
		printk("The version of the PSA Framework API is %d.\r\n",
				version);
	} else {
		printk("The version of the PSA Framework API is not valid!\r\n");
		return;
	}
}

/**
 * Retrieve the minor version of a RoT Service.
 *
 * @return N/A
 */
static void tfm_ipc_test_1002(void)
{
	u32_t version;

	version = tfm_psa_version_veneer(IPC_SERVICE_TEST_BASIC_SID);
	if (version == PSA_VERSION_NONE) {
		printk("RoT Service is not implemented or caller is not authorized");
		printk("to access it!\r\n");
		return;
	}

	/* Valid version number */
	printk("The minor version is %d.\r\n", version);
}

/**
 * Connect to a RoT Service by its SID.
 *
 * @return N/A
 */
static void tfm_ipc_test_1003(void)
{
	psa_handle_t handle;

	handle = tfm_psa_connect_veneer(IPC_SERVICE_TEST_BASIC_SID,
				IPC_SERVICE_TEST_BASIC_VERSION);
	if (handle > 0) {
		printk("Connect success!\r\n");
	} else {
		printk("The RoT Service has refused the connection!\r\n");
		return;
	}
	tfm_psa_close_veneer(handle);
}

psa_status_t psa_call(psa_handle_t handle, int32_t type,
						const psa_invec *in_vec,
						size_t in_len,
						psa_outvec *out_vec,
						size_t out_len)
{
	/* FixMe: sanity check can be added to offload some NS thread
	 * checks from TFM secure API
	 */

	/* Due to v8M restrictions, TF-M NS API needs to add another layer of
	 * serialization in order for NS to pass arguments to S
	 */
	psa_invec in_vecs, out_vecs;

	in_vecs.base = in_vec;
	in_vecs.len = in_len;
	out_vecs.base = out_vec;
	out_vecs.len = out_len;
	return tfm_psa_call_veneer(handle, (uint32_t)type,
					(psa_invec *) &in_vecs,
					(psa_invec *) &out_vecs);
}

/**
 * Call a RoT Service.
 *
 * @return N/A
 */
static void tfm_ipc_test_1004(void)
{
	char str1[] = "str1";
	char str2[] = "str2";
	char str3[128], str4[128];
	struct psa_invec invecs[2] = {{str1, sizeof(str1)/sizeof(char)},
					{str2, sizeof(str2)/sizeof(char)} };
	struct psa_outvec outvecs[2] = {{str3, sizeof(str3)/sizeof(char)},
					{str4, sizeof(str4)/sizeof(char)} };
	psa_handle_t handle;
	psa_status_t status;
	u32_t min_version;

	min_version = tfm_psa_version_veneer(IPC_SERVICE_TEST_BASIC_SID);
	printk("TFM service support minor version is %d.\r\n", min_version);
	handle = tfm_psa_connect_veneer(IPC_SERVICE_TEST_BASIC_SID,
					IPC_SERVICE_TEST_BASIC_VERSION);
	status = psa_call(handle, PSA_IPC_CALL, invecs, 2, outvecs, 2);
	if (status >= 0) {
		printk("psa_call is successful!\r\n");
	} else {
		printk("psa_call is failed!\r\n");
		return;
	}

	printk("outvec1 is: %s\r\n", (char *)outvecs[0].base);
	printk("outvec2 is: %s\r\n", (char *)outvecs[1].base);
	tfm_psa_close_veneer(handle);
}

/**
 * \brief Call IPC_CLIENT_TEST_BASIC_SID RoT Service to run the IPC basic test.
 */
static void tfm_ipc_test_1005(void)
{
	psa_handle_t handle;
	psa_status_t status;
	int test_result;
	struct psa_outvec outvecs[1] = {{&test_result, sizeof(test_result)} };

	handle = tfm_psa_connect_veneer(IPC_CLIENT_TEST_BASIC_SID,
						IPC_CLIENT_TEST_BASIC_VERSION);
	if (handle > 0) {
		printk("Connect success!\r\n");
	} else {
		printk("The RoT Service has refused the connection!\r\n");
		return;
	}

	status = psa_call(handle, PSA_IPC_CALL, NULL, 0, outvecs, 1);
	if (status >= 0) {
		printk("Call IPC_INIT_BASIC_TEST service");
		if (test_result > 0) {
			printk(" Pass\r\n");
		} else {
			printk(" Failed\r\n");
		}
	} else {
		printk("Call failed!");
		printk("Call IPC_INIT_BASIC_TEST service Failed\r\n");
	}

	tfm_psa_close_veneer(handle);
}

/**
 * \brief Call IPC_CLIENT_TEST_PSA_ACCESS_APP_MEM_SID RoT Service
 *  to run the IPC PSA access APP mem test.
 */
static void tfm_ipc_test_1006(void)
{
	psa_handle_t handle;
	psa_status_t status;
	int test_result;
	struct psa_outvec outvecs[1] = {{&test_result, sizeof(test_result)} };

	handle = tfm_psa_connect_veneer(IPC_CLIENT_TEST_PSA_ACCESS_APP_MEM_SID,
				IPC_CLIENT_TEST_PSA_ACCESS_APP_MEM_VERSION);
	if (handle > 0) {
		printk("Connect success!\r\n");
	} else {
		printk("The RoT Service has refused the connection!\r\n");
		return;
	}

	status = psa_call(handle, PSA_IPC_CALL, NULL, 0, outvecs, 1);
	if (status >= 0) {
		printk("Call PSA RoT access APP RoT memory test service");
		if (test_result > 0) {
			printk(" Pass\r\n");
		} else {
			printk(" Failed\r\n");
		}
	} else {
		printk("Call failed!");
		printk("Call PSA RoT access APP RoT memory test service Failed\r\n");
	}

	tfm_psa_close_veneer(handle);
}

void main(void)
{
	tfm_ipc_test_1001();
	tfm_ipc_test_1002();
	tfm_ipc_test_1003();
	tfm_ipc_test_1004();
	tfm_ipc_test_1005();
	tfm_ipc_test_1006();

	printk("TF-M IPC on %s\n", CONFIG_BOARD);
}
