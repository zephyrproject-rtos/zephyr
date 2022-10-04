/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/ps2.h>
#include <soc.h>
#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME main

LOG_MODULE_REGISTER();

#define TASK_STACK_SIZE 1024
#define PRIORITY 7
/*Minimum safe time interval between regular PS/2 calls */
#define MS_BETWEEN_REGULAR_CALLS 8
/*Minimum safe time interval between BAT/Reset PS/2 calls */
#define MS_BETWEEN_RESET_CALLS 500

static void to_port_60_thread(void *dummy1, void *dummy2, void *dummy3);
static void saturate_ps2(struct k_timer *timer);
static bool host_blocked;

K_THREAD_DEFINE(aux_thread_id, TASK_STACK_SIZE, to_port_60_thread,
		NULL, NULL, NULL, PRIORITY, 0, 0);
K_SEM_DEFINE(p60_sem, 0, 1);
K_MSGQ_DEFINE(aux_to_host_queue, sizeof(uint8_t), 8, 4);

/* We use a timer to saturate the queue */
K_TIMER_DEFINE(block_ps2_timer, saturate_ps2, NULL);

static const struct device *ps2_0_dev;

static void saturate_ps2(struct k_timer *timer)
{
	LOG_DBG("block host\n");
	host_blocked = true;
	ps2_disable_callback(ps2_0_dev);
	k_sleep(K_MSEC(500));
	host_blocked = false;
	ps2_enable_callback(ps2_0_dev);
}

static void mb_callback(const struct device *dev, uint8_t value)
{
	if (k_msgq_put(&aux_to_host_queue, &value, K_NO_WAIT) != 0) {
		ps2_disable_callback(ps2_0_dev);
	}

	if (!host_blocked) {
		k_sem_give(&p60_sem);
	}
}

/* This data is sent to BIOS and eventually is consumed by the host OS */
static void to_port_60_thread(void *dummy1, void *dummy2, void *dummy3)
{
	uint8_t data;

	while (true) {
		k_sem_take(&p60_sem, K_FOREVER);
		while (!k_msgq_get(&aux_to_host_queue, &data, K_NO_WAIT)) {
			LOG_DBG("\nmb data :%02x\n", data);
		}
	}
}

/* Commands expected from BIOS and Windows */
void initialize_mouse(void)
{
	LOG_DBG("mouse->f4\n");
	ps2_write(ps2_0_dev, 0xf4);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Reset mouse->ff\n");
	ps2_write(ps2_0_dev, 0xff);
	k_msleep(MS_BETWEEN_RESET_CALLS);
	LOG_DBG("Reset mouse->ff\n");
	ps2_write(ps2_0_dev, 0xff);
	k_msleep(MS_BETWEEN_RESET_CALLS);
	LOG_DBG("Read ID mouse->f2\n");
	ps2_write(ps2_0_dev, 0xf2);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set resolution mouse->e8\n");
	ps2_write(ps2_0_dev, 0xe8);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("mouse->00\n");
	ps2_write(ps2_0_dev, 0x00);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set scaling 1:1 mouse->e6\n");
	ps2_write(ps2_0_dev, 0xe6);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set scaling 1:1 mouse->e6\n");
	ps2_write(ps2_0_dev, 0xe6);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set scaling 1:1 mouse->e6\n");
	ps2_write(ps2_0_dev, 0xe6);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("mouse->e9\n");
	ps2_write(ps2_0_dev, 0xe9);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set resolution mouse->e8\n");
	ps2_write(ps2_0_dev, 0xe8);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("8 Counts/mm mouse->0x03\n");
	ps2_write(ps2_0_dev, 0x03);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set sample rate mouse->0xF3\n");
	ps2_write(ps2_0_dev, 0xf3);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("decimal 200 ->0xc8\n");
	ps2_write(ps2_0_dev, 0xc8);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set sample rate mouse->0xF3\n");
	ps2_write(ps2_0_dev, 0xf3);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("decimal 100 ->0x64\n");
	ps2_write(ps2_0_dev, 0x64);
	LOG_DBG("Set sample rate mouse->0xF3\n");
	ps2_write(ps2_0_dev, 0xf3);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("decimal 80 ->0x50\n");
	ps2_write(ps2_0_dev, 0x50);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Read device type->0xf2\n");
	ps2_write(ps2_0_dev, 0xf2);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set sample rate mouse->0xF3\n");
	ps2_write(ps2_0_dev, 0xf3);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("decimal 200 ->0xc8\n");
	ps2_write(ps2_0_dev, 0xc8);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set sample rate mouse->0xF3\n");
	ps2_write(ps2_0_dev, 0xf3);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("decimal 200 ->0xc8\n");
	ps2_write(ps2_0_dev, 0xc8);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set sample rate mouse->0xF3\n");
	ps2_write(ps2_0_dev, 0xf3);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("decimal 80 ->0x50\n");
	ps2_write(ps2_0_dev, 0x50);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Read device type->0xf2\n");
	ps2_write(ps2_0_dev, 0xf2);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set sample rate mouse->0xF3\n");
	ps2_write(ps2_0_dev, 0xf3);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("decimal 100 ->0x64\n");
	ps2_write(ps2_0_dev, 0x64);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("Set resolution mouse->e8\n");
	ps2_write(ps2_0_dev, 0xe8);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("8 Counts/mm mouse->0x03\n");
	ps2_write(ps2_0_dev, 0x03);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
	LOG_DBG("mouse->f4\n");
	ps2_write(ps2_0_dev, 0xf4);
	k_msleep(MS_BETWEEN_REGULAR_CALLS);
}

void main(void)
{
	printk("PS/2 test with mouse\n");
	/* Wait for the PS/2 BAT to finish */
	k_msleep(MS_BETWEEN_RESET_CALLS);

	/* The ps2 blocks are generic, therefore, it is allowed to swap
	 * keyboard and mouse as desired
	 */
#if DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_ps2)
	ps2_0_dev = DEVICE_DT_GET_ONE(microchip_xec_ps2);
	if (!device_is_ready(ps2_0_dev)) {
		printk("%s: device not ready.\n", ps2_0_dev->name);
		return;
	}
	ps2_config(ps2_0_dev, mb_callback);
	/*Make sure there is a PS/2 device connected */
	initialize_mouse();
#endif
	k_timer_start(&block_ps2_timer, K_SECONDS(2), K_SECONDS(1));
}
