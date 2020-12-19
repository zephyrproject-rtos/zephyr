/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARM_SMCCC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ARM_SMCCC_H_

/*
 * Result from SMC/HVC call
 * @a0-a3 result values from registers 0 to 3
 */
struct arm_smccc_res {
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
};

enum arm_smccc_conduit {
	SMCCC_CONDUIT_NONE,
	SMCCC_CONDUIT_SMC,
	SMCCC_CONDUIT_HVC,
};

/*
 * @brief Make HVC calls
 *
 * @param a0 function identifier
 * @param a1-a7 parameters registers
 * @param res results
 */
void arm_smccc_hvc(unsigned long a0, unsigned long a1,
		   unsigned long a2, unsigned long a3,
		   unsigned long a4, unsigned long a5,
		   unsigned long a6, unsigned long a7,
		   struct arm_smccc_res *res);

/*
 * @brief Make SMC calls
 *
 * @param a0 function identifier
 * @param a1-a7 parameters registers
 * @param res results
 */
void arm_smccc_smc(unsigned long a0, unsigned long a1,
		   unsigned long a2, unsigned long a3,
		   unsigned long a4, unsigned long a5,
		   unsigned long a6, unsigned long a7,
		   struct arm_smccc_res *res);

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARM_SMCCC_H_ */
