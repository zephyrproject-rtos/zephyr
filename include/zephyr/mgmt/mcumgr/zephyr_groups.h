/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ZEPHYR_MCUMGR_GRP_H_
#define ZEPHYR_INCLUDE_ZEPHYR_MCUMGR_GRP_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The file contains definitions of Zephyr specific mgmt commands. The group numbers decrease
 * from PERUSER to avoid collision with user defined groups.
 */
#define ZEPHYR_MGMT_GRP_BASE		(MGMT_GROUP_ID_PERUSER - 1)

/* Basic group */
#define ZEPHYR_MGMT_GRP_BASIC		ZEPHYR_MGMT_GRP_BASE
#define ZEPHYR_MGMT_GRP_BASIC_CMD_ERASE_STORAGE	0	/* Command to erase storage partition */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_MCUMGR_GRP_H_ */
