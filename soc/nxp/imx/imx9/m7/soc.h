/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <fsl_device_registers.h>

/* Mailbox */
#define SM_MU_INSTANCE 0
#define SM_MU_PTR      MU5_MUA

/* SMT channel */
#define SM_A2P 0 /* SCMI Agent -> SCMI Platform */

/* Doorbell*/
#define SM_DBIR_A2P 0 /* A2P channel */

/* Shared memory address */
#define SM_SMA_ADDR 0

#endif /* _SOC__H_ */
