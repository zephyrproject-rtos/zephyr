/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __K3_CTRL_PARTITIONS_H_
#define __K3_CTRL_PARTITIONS_H_

#define KICK0_UNLOCK_VAL (0x68EF3490U)
#define KICK1_UNLOCK_VAL (0xD172BC5AU)

void k3_unlock_all_ctrl_partitions(void);

#endif /* __K3_CTRL_PARTITIONS_H */
