/*
 *  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_CBS_LOG_H_
#define ZEPHYR_CBS_LOG_H_

#include <zephyr/kernel.h>
#include <zephyr/sched_server/cbs.h>

#ifdef __cplusplus
extern "C" {
#endif

enum cbs_evt {
	CBS_PUSH_JOB,
	CBS_SWITCH_TO,
	CBS_SWITCH_AWAY,
	CBS_COMPLETED_JOB,
	CBS_BUDGET_RAN_OUT,
	CBS_BUDGET_CONDITION_MET
};

void cbs_log(enum cbs_evt event, struct k_cbs *cbs);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_CBS_LOG_H_ */
