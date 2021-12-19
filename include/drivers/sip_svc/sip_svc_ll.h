/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_LL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_LL_H_

/**
 * @file
 * @brief Arm SiP Services driver low level functions.
 */

#include <arch/arm64/arm-smccc.h>
#include <drivers/sip_svc/sip_svc_proto.h>

#define SIP_SVC_CLIENT_ST_INVALID	0
#define SIP_SVC_CLIENT_ST_IDLE		1
#define SIP_SVC_CLIENT_ST_OPEN		2
#define SIP_SVC_CLIENT_ST_ABORT		3

#define SIP_SVC_ID_INVALID		0xFFFFFFFF

#define SIP_SVC_TIME_NO_WAIT		0
#define SIP_SVC_TIME_FOREVER		0xFFFFFFFF


typedef void (sip_svc_fn)(unsigned long, unsigned long,
			  unsigned long, unsigned long,
			  unsigned long, unsigned long,
			  unsigned long, unsigned long,
			  struct arm_smccc_res *);

/** @brief Arm SiP Services client data.
 */
struct sip_svc_client {

	uint32_t id;
	uint32_t token;
	uint32_t state;
	uint32_t active_trans_cnt;
	void *priv_data;
};

/** @brief Arm SiP Services driver controller data.
 */
struct sip_svc_controller {

	const char *method;
	sip_svc_fn *invoke_fn;

	struct sip_svc_id_pool *client_id_pool;
	struct sip_svc_id_pool *trans_id_pool;
	struct sip_svc_id_map *trans_id_map;

	struct k_mutex req_msgq_mutex;
	struct k_msgq req_msgq;

	k_tid_t tid;
	struct k_thread	thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ARM_SIP_SVC_THREAD_STACK_SIZE);

	struct k_mutex open_mutex;
	struct k_mutex data_mutex;
	struct sip_svc_client clients[CONFIG_ARM_SIP_SVC_MAX_CLIENT_COUNT];
	uint32_t active_client_index;
	struct k_timer active_client_wdt;

	uint32_t active_job_cnt;
	uint32_t active_async_job_cnt;
	uint8_t async_resp_data[CONFIG_ARM_SIP_SVC_MAX_ASYNC_RESP_SIZE];
};


typedef void (*sip_svc_cb_fn)(uint32_t c_token, char *res, int size);

typedef uint32_t (*sip_svc_register_fn)(struct sip_svc_controller *,
					void *priv_data);

typedef int (*sip_svc_unregister_fn)(struct sip_svc_controller *,
				     uint32_t c_token);

typedef int (*sip_svc_open_fn)(struct sip_svc_controller *,
			       uint32_t c_token,
			       uint32_t timeout_us);

typedef int (*sip_svc_close_fn)(struct sip_svc_controller *,
				uint32_t c_token);

typedef int (*sip_svc_send_fn)(struct sip_svc_controller *,
			       uint32_t c_token,
			       char *req,
			       int size,
			       sip_svc_cb_fn cb);

typedef void* (*sip_svc_get_priv_data_fn)(struct sip_svc_controller *,
					  uint32_t c_token);

typedef void (*sip_svc_print_info_fn)(struct sip_svc_controller *);

uint32_t sip_svc_ll_register(struct sip_svc_controller *ctrl,
			     void *priv_data);

int sip_svc_ll_unregister(struct sip_svc_controller *ctrl,
			  uint32_t c_token);

int sip_svc_ll_open(struct sip_svc_controller *ctrl,
		    uint32_t c_token, uint32_t timeout_us);

int sip_svc_ll_close(struct sip_svc_controller *ctrl,
		     uint32_t c_token);

int sip_svc_ll_send(struct sip_svc_controller *ctrl,
		    uint32_t c_token,
		    char *req,
		    int size,
		    sip_svc_cb_fn cb);

void *sip_svc_ll_get_priv_data(struct sip_svc_controller *ctrl,
			       uint32_t c_token);

void sip_svc_ll_print_info(struct sip_svc_controller *ctrl);

int sip_svc_ll_init(struct sip_svc_controller *ctrl);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_LL_H_ */
