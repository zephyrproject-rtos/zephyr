/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <drivers/lora/lora_context.h>
#include <logging/sys_log.h>
#include <power.h>
#include <soc_power.h>
#include <gpio.h>
static struct k_sem quit_lock;
static struct k_thread* pMyThread;

static void lora_joined(void)
{
	SYS_LOG_DBG("Joined LoRa network");
}

static void lora_device_ready(bool success)
{

}

static struct lora_context_cb lora_callbacks = {
		.ready = lora_device_ready,
		.joined = lora_joined
};

void main(void)
{

	struct device *lora_dev = device_get_binding(LORA_DEV_NAME);

    //k_sem_init(&quit_lock, 0, UINT_MAX);

/*
    while(true)
    {
        printk("HI");
#define BUSY_WAIT_DELAY_US		(2 * 1000000)
        k_busy_wait(BUSY_WAIT_DELAY_US);
    }
*/
/*
	lora_context_init(&lora_callbacks);
	struct lora_driver_api* lora_api = (struct lora_driver_api *) lora_dev->driver_api;
	lora_api->callback_register(lora_dev, &lora_callbacks);

    int ret = lora_api->join(lora_dev);*/
SYS_LOG_DBG("Oink");
#define BUSY_WAIT_DELAY_US		(20 * 1000000)

while(true)
{
#define LPS_STATE_ENTER_TO		(30 * 1000)
    k_sleep(300);
  //  k_busy_wait(BUSY_WAIT_DELAY_US);
}

    /*
#define BUSY_WAIT_DELAY_US		(20 * 1000000)
	k_busy_wait(BUSY_WAIT_DELAY_US);
#define LPS_STATE_ENTER_TO		(30 * 1000)
	k_sleep(LPS_STATE_ENTER_TO);*/

	//_sys_soc_suspend(1000*100);
	//while(true);
    //k_sem_take(&quit_lock, K_FOREVER);
}
