/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM64_NXP_IMX9_A55_SOC_H_
#define ZEPHYR_SOC_ARM64_NXP_IMX9_A55_SOC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Mailbox */
#define SM_MU_INSTANCE 5

/* SMT channel */
#define SM_A2P 0 /* SCMI Agent -> SCMI Platform */

/* Doorbell*/
#define SM_DBIR_A2P 0 /* A2P channel */

/* Shared memory address */
#define SM_SMA_ADDR 0

#ifdef __cplusplus
}
#endif

#endif /*  ZEPHYR_SOC_ARM64_NXP_IMX9_A55_SOC_H_ */
