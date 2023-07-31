/*
 * Copyright (c) 2023 Chad Karaginides <quic_chadk@quicinc.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file implements the ARM64 EL3 service.
 *
 * Service available:
 * Secure Monitor Call (SMC) handler.
 *
 * See https://developer.arm.com/docs/den0028/latest
 */

#include <zephyr/toolchain.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/arm-smccc.h>


/* SMC function IDs for Standard Service queries */
#define ARM_STD_SMC_CALL_COUNT          0x8400ff00UL
#define ARM_STD_SMC_VERSION             0x8400ff03UL
#define ARM_STD_SMC_UNKNOWN             0xffffffffUL
#define SMC_UNK                         -1


K_KERNEL_STACK_ARRAY_DEFINE(z_arm64_el3_service_stacks,
			    CONFIG_MP_MAX_NUM_CPUS,
			    CONFIG_ARM64_EL3_SERVICE_STACK_SIZE);


/*
 * Default SMC handler executed in EL3.  Dummy values are used for CI testing.
 * To be overridden by user specific function.
 */
__weak void z_arm64_smc_handler(arm_smccc_res_t *cmd_rsp)
{
	switch (cmd_rsp->a0) {
	case ARM_STD_SMC_CALL_COUNT:
		cmd_rsp->a0 = 1;
		break;

	case ARM_STD_SMC_VERSION:
		cmd_rsp->a0 = 1;
		cmd_rsp->a1 = 1;
		break;

	case ARM_STD_SMC_UNKNOWN:
	default:
		cmd_rsp->a0 = SMC_UNK;
		break;
	}
}
