/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/ipm.h>
#include <sys/printk.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <init.h>

#include <ipc/rpmsg_multi_instance.h>

#define IPM_WORK_QUEUE_STACK_SIZE  1024
#define APP_TASK_STACK_SIZE        1024

K_THREAD_STACK_DEFINE(ipm_stack_area_1, IPM_WORK_QUEUE_STACK_SIZE);
K_THREAD_STACK_DEFINE(ipm_stack_area_2, IPM_WORK_QUEUE_STACK_SIZE);

K_THREAD_STACK_DEFINE(thread_stack_1, APP_TASK_STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_stack_2, APP_TASK_STACK_SIZE);

static struct k_thread thread_data_1;
static struct k_thread thread_data_2;

static K_SEM_DEFINE(bound_ept1_sem, 0, 1);
static K_SEM_DEFINE(bound_ept2_sem, 0, 1);

static K_SEM_DEFINE(data_rx1_sem, 0, 1);
static K_SEM_DEFINE(data_rx2_sem, 0, 1);

static volatile uint8_t received_data_1;
static volatile uint8_t received_data_2;

static struct rpmsg_mi_ctx ctx_1;
static struct rpmsg_mi_ctx ctx_2;

static struct rpmsg_mi_ept ept_1;
static struct rpmsg_mi_ept ept_2;

static void boud1_cb(void *priv)
{
	k_sem_give(&bound_ept1_sem);
}

static void received1_cb(const void *data, size_t len, void *priv)
{
	received_data_1 = *((uint8_t *)data);
	k_sem_give(&data_rx1_sem);
}

static void boud2_cb(void *priv)
{
	k_sem_give(&bound_ept2_sem);
}

static void received2_cb(const void *data, size_t len, void *priv)
{
	received_data_2 = *((uint8_t *)data);
	k_sem_give(&data_rx2_sem);
}

static const struct rpsmg_mi_ctx_cfg cfg_1 = {
	.name               = "instance 1",
	.ipm_stack_area     = ipm_stack_area_1,
	.ipm_stack_size     = K_THREAD_STACK_SIZEOF(ipm_stack_area_1),
	.ipm_thread_name    = "ipm_work_q_1",
	.ipm_work_q_prio    = 0,
	.ipm_tx_name        = CONFIG_RPMSG_MULTI_INSTANCE_1_IPM_TX_NAME,
	.ipm_rx_name        = CONFIG_RPMSG_MULTI_INSTANCE_1_IPM_RX_NAME,
};

static const struct rpsmg_mi_ctx_cfg cfg_2 = {
	.name               = "instance 2",
	.ipm_stack_area     = ipm_stack_area_2,
	.ipm_stack_size     = K_THREAD_STACK_SIZEOF(ipm_stack_area_2),
	.ipm_thread_name    = "ipm_work_q_2",
	.ipm_work_q_prio    = 0,
	.ipm_tx_name        = CONFIG_RPMSG_MULTI_INSTANCE_2_IPM_TX_NAME,
	.ipm_rx_name        = CONFIG_RPMSG_MULTI_INSTANCE_2_IPM_RX_NAME,
};

static struct rpmsg_mi_cb cb_1 = {
	.bound    = boud1_cb,
	.received = received1_cb,
};

static struct rpmsg_mi_cb cb_2 = {
	.bound    = boud2_cb,
	.received = received2_cb,
};

static struct rpmsg_mi_ept_cfg ept_cfg_1 = {
	.name   = "ept1",
	.cb     = &cb_1,
	.priv   = &ept_1,
};

static struct rpmsg_mi_ept_cfg ept_cfg_2 = {
	.name   = "ept2",
	.cb     = &cb_2,
	.priv   = &ept_2,
};

void app_task_1(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int status = 0;
	uint8_t message = 0U;

	printk("\r\nRPMsg Multiple instance [master no 1] demo started\r\n");

	/* Initialization of 1 instance */
	status = rpmsg_mi_ctx_init(&ctx_1, &cfg_1);
	if (status < 0) {
		printk("rpmsg_mi_init for [no 1] failed with status %d\n",
		       status);
	}

	status = rpmsg_mi_ept_register(&ctx_1, &ept_1, &ept_cfg_1);
	if (status < 0) {
		printk("rpmsg_mi_ept_register [no 1] failed with status %d\n",
		       status);
	}

	/* Waiting to be bound. */
	k_sem_take(&bound_ept1_sem, K_FOREVER);

	while (message < 100) {
		status = rpmsg_mi_send(&ept_1, &message, sizeof(message));
		if (status < 0) {
			printk("send_message(%d) failed with status %d\n",
			       message, status);
		}

		k_sem_take(&data_rx1_sem, K_FOREVER);
		message = received_data_1;

		printk("Master [no 1] core received a message: %d\n", message);
		message++;
	}

	printk("RPMsg Multiple instance [no 1] demo ended.\n");
}

void app_task_2(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int status = 0;
	uint8_t message = 0U;

	printk("\r\nRPMsg Multiple instance [master no 2] demo started\r\n");

	/* Initialization of 2 instance */
	status = rpmsg_mi_ctx_init(&ctx_2, &cfg_2);
	if (status < 0) {
		printk("rpmsg_mi_init [no 2] failed with status %d\n", status);
	}

	status = rpmsg_mi_ept_register(&ctx_2, &ept_2, &ept_cfg_2);
	if (status < 0) {
		printk("rpmsg_mi_ept_register [no 2] failed with status %d\n",
		       status);
	}

	/* Waiting to be bound. */
	k_sem_take(&bound_ept2_sem, K_FOREVER);

	while (message < 100) {
		status = rpmsg_mi_send(&ept_2, &message, sizeof(message));
		if (status < 0) {
			printk("send_message(%d) failed with status %d\n",
			       message, status);
		}

		k_sem_take(&data_rx2_sem, K_FOREVER);
		message = received_data_2;

		printk("Master [no 2] core received a message: %d\n", message);

		message++;
	}

	printk("RPMsg Multiple instance [no 2] demo ended.\n");
}

void main(void)
{
	printk("Starting application thread!\n");

	k_thread_create(&thread_data_1, thread_stack_1, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task_1,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	k_thread_create(&thread_data_2, thread_stack_2, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task_2,
			NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
}
