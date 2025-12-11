/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_POWER_ATMEL_SAM_SUPC_H_
#define ZEPHYR_INCLUDE_DRIVERS_POWER_ATMEL_SAM_SUPC_H_

#define SAM_DT_SUPC_CONTROLLER DEVICE_DT_GET(DT_NODELABEL(supc))

#define SAM_DT_SUPC_WAKEUP_SOURCE_ID(node_id) \
	DT_PROP_BY_IDX(node_id, wakeup_source_id wakeup_source_id)

#define SAM_DT_INST_SUPC_WAKEUP_SOURCE_ID(inst) \
	SAM_DT_SUPC_WAKEUP_SOURCE_ID(DT_DRV_INST(inst))

#endif /* ZEPHYR_INCLUDE_DRIVERS_POWER_ATMEL_SAM_SUPC_H_ */
