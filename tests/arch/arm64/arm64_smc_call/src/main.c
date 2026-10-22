/*
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/arch/arm64/arm-smccc.h>

/* SMC function IDs for Standard Service queries */
#define ARM_STD_SMC_CALL_COUNT		0x8400ff00UL
#define ARM_STD_SMC_VERSION		0x8400ff03UL
#define ARM_STD_SMC_UNKNOWN		0xffffffffUL

#define SMC_UNK				-1

typedef void (*smc_call_method_t)(unsigned long, unsigned long,
				  unsigned long, unsigned long,
				  unsigned long, unsigned long,
				  unsigned long, unsigned long,
				  struct arm_smccc_res *);

#ifdef CONFIG_SMC_CALL_USE_HVC
	smc_call_method_t smc_call = arm_smccc_hvc;
#else
	smc_call_method_t smc_call = arm_smccc_smc;
#endif

ZTEST(arm64_smc_call, test_smc_call_func)
{
	struct arm_smccc_res res;

	smc_call(ARM_STD_SMC_CALL_COUNT, 0, 0, 0, 0, 0, 0, 0, &res);
	zassert_true(res.a0 > 0, "Wrong smc call count");

	smc_call(ARM_STD_SMC_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);
	zassert_true((res.a0 >= 0 && res.a1 >= 0),
		"Wrong smc call version");

	smc_call(ARM_STD_SMC_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, &res);
	zassert_true(res.a0 == SMC_UNK, "Wrong return code from smc call");
}

ZTEST_SUITE(arm64_smc_call, NULL, NULL, NULL, NULL, NULL);
