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
#include <zephyr/arch/arm64/arm-smccc.h>

/*
 * Default SMC handler executed in EL3.
 * To be overridden by user specific function.
 */ 
__weak void z_arm64_smc_handler(arm_smccc_cmd_rsp_t * cmd_rsp)
{
}