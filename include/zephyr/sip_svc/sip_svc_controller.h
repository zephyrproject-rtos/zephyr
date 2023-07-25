/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SIP_SVC_CONTROLLER_H_
#define ZEPHYR_SIP_SVC_CONTROLLER_H_

/**
 * @note This file should only be included by sip_svc driver.
 */

#ifdef CONFIG_ARM_SIP_SVC_SUBSYS

/**
 * @brief Length of SVC conduit name in sip svc subsystem.
 *
 */
#define SIP_SVC_SUBSYS_CONDUIT_NAME_LENGTH (4)

/**
 *  @brief Arm SiP Service client data.
 *
 */
struct sip_svc_client {

	/* Client id internal to sip_svc*/
	uint32_t id;
	/* Client's token id provided back to client during sip_svc_register() */
	uint32_t token;
	/* Client's state */
	uint32_t state;
	/* Total Number of on-going transaction  of the client */
	uint32_t active_trans_cnt;
	/* Private data of each client , Provided during sip_svc_register() */
	void *priv_data;
	/* Transaction id pool for each client */
	struct sip_svc_id_pool *trans_idx_pool;
};

/**
 * @brief Arm SiP Services controller data.
 *
 */
struct sip_svc_controller {

	/* Initialization status*/
	bool init;
	/* Total number of clients*/
	const uint32_t num_clients;
	/* Maximum allowable transactions in the system per controller*/
	const uint32_t max_transactions;
	/* Response size of buffer used for ASYNC transaction*/
	const uint32_t resp_size;
	/* Total Number of active transactions */
	uint32_t active_job_cnt;
	/* Active ASYNC transactions */
	uint32_t active_async_job_cnt;
	/* Supervisory call name , got from dts entry */
	char method[SIP_SVC_SUBSYS_CONDUIT_NAME_LENGTH];
	/* Pointer to driver instance */
	const struct device *dev;
	/* Pointer to client id pool */
	struct sip_svc_id_pool *client_id_pool;
	/* Pointer to database for storing arguments from sip_svc_send() */
	struct sip_svc_id_map *trans_id_map;
	/* Pointer to client array */
	struct sip_svc_client *clients;
	/* Pointer to Buffer used for storing response from lower layers */
	uint8_t *async_resp_data;
	/* Thread id of sip_svc thread */
	k_tid_t tid;

#if CONFIG_ARM_SIP_SVC_SUBSYS_SINGLY_OPEN
	/* Mutex to restrict one client access */
	struct k_mutex open_mutex;
#endif
	/* Mutex for protecting database access */
	struct k_mutex data_mutex;
	/* msgq for sending sip_svc_request to sip_svc thread */
	struct k_msgq req_msgq;
	/* sip_svc thread object */
	struct k_thread thread;
	/* Stack object of sip_svc thread */
	K_KERNEL_STACK_MEMBER(stack, CONFIG_ARM_SIP_SVC_SUBSYS_THREAD_STACK_SIZE);
};

/**
 * @brief Controller define used by sip_svc driver to instantiate each controller.
 * Each sip_svc driver will use this API to instantiate a controller for sip_svc
 * subsystem to consume, for more details check @ref ITERABLE_SECTION_RAM()
 */
#define SIP_SVC_CONTROLLER_DEFINE(inst, conduit_name, sip_dev, sip_num_clients,                    \
				  sip_max_transactions, sip_resp_size)                             \
	static STRUCT_SECTION_ITERABLE(sip_svc_controller, sip_svc_##inst) = {                     \
		.method = conduit_name,                                                            \
		.dev = sip_dev,                                                                    \
		.num_clients = sip_num_clients,                                                    \
		.max_transactions = sip_max_transactions,                                          \
		.resp_size = sip_resp_size,                                                        \
	}

#else
#define SIP_SVC_CONTROLLER_DEFINE(inst, conduit_name, sip_dev, sip_num_clients,                    \
				  sip_max_transactions, sip_resp_size)
#endif

#endif /* ZEPHYR_SIP_SVC_CONTROLLER_H_ */
