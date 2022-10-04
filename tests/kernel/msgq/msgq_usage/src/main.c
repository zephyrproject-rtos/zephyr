/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/irq_offload.h>
#include <ztest.h>
#include <limits.h>

#define MSGQ_LEN (2)
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define NUM_SERVICES 2
#define TIMEOUT K_MSEC(100)

K_MSGQ_DEFINE(manager_q, sizeof(unsigned long) * 2, 4, 4);
struct k_msgq service1_msgq;
struct k_msgq service2_msgq;
struct k_msgq client_msgq;
K_THREAD_STACK_DEFINE(service_manager_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(service1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(service2_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(client_stack, STACK_SIZE);
K_SEM_DEFINE(service_sema, 2, 2);
K_SEM_DEFINE(service_started, 0, 2);
K_SEM_DEFINE(test_continue, 0, 1);
struct k_thread service_manager;
struct k_thread service1;
struct k_thread service2;
struct k_thread client;
static ZTEST_DMEM unsigned long __aligned(4) service1_buf[MSGQ_LEN];
static ZTEST_DMEM unsigned long __aligned(4) service2_buf[MSGQ_LEN];
static ZTEST_DMEM unsigned long __aligned(4) client_buf[MSGQ_LEN * 2];
static ZTEST_DMEM struct k_msgq *services[NUM_SERVICES];
static ZTEST_DMEM struct k_msgq *pclient;
static ZTEST_DMEM bool service1_run;
static ZTEST_DMEM bool service2_run;
static ZTEST_DMEM k_tid_t tservice_manager, tservice1, tservice2, tclient;

enum message_info {
	QUERRY_SERVICE = 1,
	REGISTER_SERVICE1,
	REGISTER_SERVICE2,
	GET_SERVICE,
	SERVICE1_RUNNING,
	SERVICE2_RUNNING,
	SERVICE_QUIT
};

static void service_manager_entry(void *p1, void *p2, void *p3)
{
	static unsigned long data[2];

	while (1) {
		k_msgq_get(&manager_q, data, K_FOREVER);
		switch (data[0]) {
		case QUERRY_SERVICE:
			pclient = (struct k_msgq *)data[1];
			k_msgq_put(pclient, services, K_NO_WAIT);
			break;
		case REGISTER_SERVICE1:
			services[0] = (struct k_msgq *)data[1];
			k_sem_give(&service_started);
			break;
		case REGISTER_SERVICE2:
			services[1] = (struct k_msgq *)data[1];
			k_sem_give(&service_started);
			break;
		case SERVICE_QUIT:
			for (int i = 0; i < NUM_SERVICES; i++) {
				if (services[i] == (struct k_msgq *)data[1]) {
					services[i] = NULL;
				}
			}
			/* wake up threads waiting for this queue */
			k_msgq_purge((struct k_msgq *)data[1]);
			break;
		default:
			TC_PRINT("Unknown message %ld\n", data[0]);
			break;
		}
		k_msleep(10);
	}
}

static void start_service_manager(void)
{
	int pri = k_thread_priority_get(k_current_get());

	tservice_manager = k_thread_create(&service_manager,
					   service_manager_stack, STACK_SIZE,
					   service_manager_entry, NULL, NULL,
					   NULL, pri, 0, K_NO_WAIT);
}

static void service1_entry(void *p1, void *p2, void *p3)
{
	static unsigned long service_data[2];
	struct k_msgq *client;
	int ret;

	service_data[0] = REGISTER_SERVICE1;
	service_data[1] = (unsigned long)&service1_msgq;

	k_msgq_init(&service1_msgq, (char *)service1_buf,
		    sizeof(service1_buf), 1);
	ret = k_msgq_put(&manager_q, service_data, K_NO_WAIT);
	zassert_equal(ret, 0, "Can't register service");

	k_sem_take(&service_sema, K_NO_WAIT);
	while (service1_run) {
		k_msgq_get(&service1_msgq, service_data, K_FOREVER);
		if (service_data[0] == GET_SERVICE) {
			client = (struct k_msgq *)service_data[1];
			service_data[0] = SERVICE1_RUNNING;
			k_msgq_put(client, service_data, K_NO_WAIT);
		}
		k_msleep(10);
	}

	/* inform services manager */
	service_data[0] = SERVICE_QUIT;
	service_data[1] = (unsigned long)&service1_msgq;
	k_msgq_put(&manager_q, service_data, K_NO_WAIT);
}

static void service2_entry(void *p1, void *p2, void *p3)
{
	static unsigned long service_data[2];
	struct k_msgq *client;
	int ret;

	service_data[0] = REGISTER_SERVICE2;
	service_data[1] = (unsigned long)&service2_msgq;

	k_msgq_init(&service2_msgq, (char *)service2_buf,
		    sizeof(service2_buf), 1);
	ret = k_msgq_put(&manager_q, service_data, K_NO_WAIT);
	zassert_equal(ret, 0, "Can't register service");

	k_sem_take(&service_sema, K_NO_WAIT);
	while (service2_run) {
		k_msgq_get(&service2_msgq, service_data, K_FOREVER);
		if (service_data[0] == GET_SERVICE) {
			client = (struct k_msgq *)service_data[1];
			service_data[0] = SERVICE2_RUNNING;
			k_msgq_put(client, service_data, K_NO_WAIT);
		}
		k_msleep(10);
	}

	/* inform services manager */
	service_data[0] = SERVICE_QUIT;
	service_data[1] = (unsigned long)&service2_msgq;
	k_msgq_put(&manager_q, service_data, K_NO_WAIT);
}

static void register_service(void)
{

	int pri = k_thread_priority_get(k_current_get());

	service1_run = true;
	tservice1 = k_thread_create(&service1, service1_stack, STACK_SIZE,
				    service1_entry, NULL, NULL, NULL, pri,
				    0, K_NO_WAIT);

	service2_run = true;
	tservice2 = k_thread_create(&service2, service2_stack, STACK_SIZE,
				    service2_entry, NULL, NULL, NULL, pri,
				    0, K_NO_WAIT);
}

static void client_entry(void *p1, void *p2, void *p3)
{
	static unsigned long client_data[2];
	static unsigned long service_data[2];
	struct k_msgq *service1q;
	struct k_msgq *service2q;
	bool query_service = false;
	int ret;


	k_msgq_init(&client_msgq, (char *)client_buf,
		    sizeof(unsigned long) * 2, 2);
	client_data[0] = QUERRY_SERVICE;
	client_data[1] = (unsigned long)&client_msgq;

	/* wait all services started */
	k_sem_take(&service_started, K_FOREVER);
	k_sem_take(&service_started, K_FOREVER);

	/* query services */
	k_msgq_put(&manager_q, client_data, K_NO_WAIT);
	ret = k_msgq_get(&client_msgq, service_data, K_FOREVER);
	zassert_equal(ret, 0, NULL);

	service1q = (struct k_msgq *)service_data[0];
	service2q = (struct k_msgq *)service_data[1];
	/* all services should be running */
	zassert_equal(service1q, &service1_msgq, NULL);
	zassert_equal(service2q, &service2_msgq, NULL);
	/* let the test thread continue */
	k_sem_give(&test_continue);

	while (1) {
		/* service might quit */
		if (query_service) {
			client_data[0] = QUERRY_SERVICE;
			client_data[1] = (unsigned long)&client_msgq;
			k_msgq_put(&manager_q, client_data, K_NO_WAIT);
			k_msgq_get(&client_msgq, service_data, K_FOREVER);
			service1q = (struct k_msgq *)service_data[0];
			service2q = (struct k_msgq *)service_data[1];
			query_service = false;
		}

		if (!service1q && !service2q) {
			break;
		}

		client_data[0] = GET_SERVICE;
		client_data[1] = (unsigned long)&client_msgq;

		if (service1q) {
			k_msgq_put(service1q, client_data, K_NO_WAIT);
			ret = k_msgq_get(&client_msgq, service_data, TIMEOUT);
			if (!ret) {
				zassert_equal(service_data[0],
					      SERVICE1_RUNNING, NULL);
			} else {
				/* service might down, query in next loop */
				query_service = true;
			}
		}

		if (service2q) {
			k_msgq_put(service2q, client_data, K_NO_WAIT);
			ret = k_msgq_get(&client_msgq, service_data, TIMEOUT);
			if (!ret) {
				zassert_equal(service_data[0],
					      SERVICE2_RUNNING, NULL);
			} else {
				query_service = true;
			}
		}
		k_msleep(10);
	}
}

static void start_client(void)
{

	int pri = k_thread_priority_get(k_current_get());

	tclient = k_thread_create(&client, client_stack, STACK_SIZE,
				  client_entry, NULL, NULL, NULL, pri,
				  0, K_NO_WAIT);
}

void test_msgq_usage(void)
{
	start_service_manager();
	register_service();
	start_client();
	/* waiting to continue */
	k_sem_take(&test_continue, K_FOREVER);

	/* rather than schedule this thread by k_msleep(), use semaphore with
	 * a timeout value, so there is no give operation over service_sema
	 */
	TC_PRINT("try to kill service1\n");
	k_sem_take(&service_sema, Z_TIMEOUT_MS(500));
	service1_run = false;

	TC_PRINT("try to kill service2\n");
	k_sem_take(&service_sema, Z_TIMEOUT_MS(500));
	service2_run = false;

	k_thread_join(tservice1, K_FOREVER);
	k_thread_join(tservice2, K_FOREVER);
	k_thread_join(tclient, K_FOREVER);
	k_thread_abort(tservice_manager);
}

void test_main(void)
{
	ztest_test_suite(msgq_usage, ztest_unit_test(test_msgq_usage));
	ztest_run_test_suite(msgq_usage);
}
