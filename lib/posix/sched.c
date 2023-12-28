/*
 * Copyright (c) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pthread_sched.h"

#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/sched.h>

/**
 * @brief Get minimum priority value for a given policy
 *
 * See IEEE 1003.1
 */
int sched_get_priority_min(int policy)
{
	if (!valid_posix_policy(policy)) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

/**
 * @brief Get maximum priority value for a given policy
 *
 * See IEEE 1003.1
 */
int sched_get_priority_max(int policy)
{
	if (IS_ENABLED(CONFIG_COOP_ENABLED) && policy == SCHED_FIFO) {
		return CONFIG_NUM_COOP_PRIORITIES - 1;
	} else if (IS_ENABLED(CONFIG_PREEMPT_ENABLED) &&
		   (policy == SCHED_RR || policy == SCHED_OTHER)) {
		return CONFIG_NUM_PREEMPT_PRIORITIES - 1;
	}

	errno = EINVAL;
	return -1;
}

/**
 * @brief Get scheduling parameters
 *
 * See IEEE 1003.1
 */
int sched_getparam(pid_t pid, struct sched_param *param)
{
	/* TODO: Ensure the Zephyr POSIX process id (PID) value is assigned
	 * with its first thread's id (TID).
	 */
	struct sched_param dummy_param = {0};
	pthread_t tid = (pthread_t)pid;
	int t_policy = -1;
	int ret = -1;

	if (param == NULL) {
		param = &dummy_param;
	}

	if (tid == 0) {
		tid = pthread_self();
	}

	ret = pthread_getschedparam(tid, &t_policy, param);
	errno = ret;

	return ((ret) ? -1 : 0);
}

/**
 * @brief Get scheduling policy
 *
 * See IEEE 1003.1
 */
int sched_getscheduler(pid_t pid)
{
	/* TODO: Ensure the Zephyr POSIX process id (PID) value is assigned
	 * with its first thread's id (TID).
	 */
	struct sched_param dummy_param = {0};
	pthread_t tid = (pthread_t)pid;
	int t_policy = -1;
	int ret = -1;

	if (tid == 0) {
		tid = pthread_self();
	}

	ret = pthread_getschedparam(tid, &t_policy, &dummy_param);
	errno = ret;

	return (ret == 0) ? t_policy : -1;
}

/**
 * @brief Set scheduling parameters
 *
 * See IEEE 1003.1
 */
int sched_setparam(pid_t pid, const struct sched_param *param)
{
	struct sched_param dummy_param = {0};
	pthread_t tid = (pthread_t)pid;
	int t_policy = -1;
	int ret = -1;
	
	if (param == NULL) {
		param = &dummy_param;
	}
	
	if (tid == 0) {
		tid = pthread_self();
	}
	
	if (param == NULL) {
		ret = -1;
	}else{
		ret = pthread_getschedparam(tid, &t_policy, &dummy_param);
		if(ret != -1){
			ret = pthread_setschedparam(tid, t_policy, param);
		}
	}
	
	errno = ret;
	return ret;
}

/**
 * @brief Set scheduling policy
 *
 * See IEEE 1003.1
 */
int sched_setscheduler(pid_t pid, int policy,const struct sched_param *param)
{
	struct sched_param dummy_param = {0};
	pthread_t tid = (pthread_t)pid;
	int ret = -1;
	
	if (param == NULL) {
		param = &dummy_param;
	}
	
	if (tid == 0) {
		tid = pthread_self();
	}
	
	if (param == NULL) {
		ret = -1;
	}else{
		if(ret != -1){
			ret = pthread_setschedparam(tid, policy, param);
		}
	}
	
	errno = ret;
	return ret;
}	