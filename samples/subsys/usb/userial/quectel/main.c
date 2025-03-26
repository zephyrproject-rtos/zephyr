/*
 * Copyright (c) 2025 Quectel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/util.h>
#include <zephyr/userial/quectel/quec_uhc_app.h>

/*===========================================================================
 * 							  define
 ===========================================================================*/

#define AT_ATE0				"ate0&w\r\n"
#define AT_QSCLK			"at+qsclk=1\r\n"
#define AT_CGDCONT			"at+cgdcont?\r\n"
#define AT_CSQ				"at+csq\r\n"
#define AT_CREG				"at+creg?\r\n"

#define TEST_STASK_SZIE		(1024 * 6)
#define TEST_PORT			QUEC_AT_PORT
#define REC_BUF_SIZE		1024

LOG_MODULE_REGISTER(quec_uhc_demo, LOG_LEVEL_DBG);

/*===========================================================================
 * 							  typedef
 ===========================================================================*/
typedef struct
{
	const struct device *device;
	struct k_thread		tx_thread;
	quec_uhc_api_t		*api;
}quec_demo_param_t;

typedef struct
{
	uint32_t 	event;
	uint8_t		port_id;
	uint32_t	size;
}quec_sys_event_t;

/*===========================================================================
 * 							  variables
 ===========================================================================*/
static uint8_t tx_task_stack[TEST_STASK_SZIE];
static quec_demo_param_t uhc_manager;

K_MSGQ_DEFINE(quec_demo_msgq, sizeof(quec_sys_event_t), 20, 4);
K_MSGQ_DEFINE(quec_rx_test_msgq, sizeof(quec_sys_event_t), 20, 4);
/*===========================================================================
 * 							  function
 ===========================================================================*/
static void quec_event_process(uint32_t event, uint8_t port_id, uint32_t size)
{
	quec_sys_event_t sys_event = {0};
	quec_demo_param_t *uhc_ctl = (quec_demo_param_t *)&uhc_manager;
	uint8_t rec_buf[REC_BUF_SIZE];
	int read_size = 0, total_size = 0;

	if(event & QUEC_RX_ARRIVE)
	{		
		memset(rec_buf, 0, sizeof(rec_buf));

		total_size = uhc_ctl->api->read_aviable(uhc_ctl->device, port_id); 
		while(total_size > 0)
		{
			memset(rec_buf, 0, REC_BUF_SIZE);

			read_size = UHC_MIN(total_size, REC_BUF_SIZE);
			read_size = uhc_ctl->api->read(uhc_ctl->device, port_id, rec_buf, read_size);
			if(read_size > 0)
			{
				total_size -= read_size;
				quec_print("Receive: size %d content %s", size, rec_buf);
			}
			else
			{
				quec_print("read failed");
				break;
			}
		}
	}

	if(event & QUEC_DEVICE_CONNECT)
	{
		quec_print("qcx216 connect");

		if(uhc_ctl->api->open(uhc_ctl->device, TEST_PORT) != 0)
		{
			quec_print("port open error");
			return;
		}

		sys_event.event = QUEC_DEVICE_CONNECT;
		k_msgq_put(&quec_demo_msgq, &sys_event, K_MSEC(0));
	}

	if(event & QUEC_DEVICE_DISCONNECT)
	{
		quec_print("qcx216 disconnect");
	}

	if(event & QUEC_RX_ERROR)
	{
		quec_print("qcx216 rx error");
	}

	if(event & QUEC_TX_ERROR)
	{
		quec_print("qcx216 tx error");
	}
}

static int quec_at_cmd_send(uint8_t *buffer, uint32_t size)
{
	int total_write = 0;
	quec_demo_param_t *uhc_ctl = (quec_demo_param_t *)&uhc_manager;

	total_write = uhc_ctl->api->write(uhc_ctl->device, TEST_PORT, buffer, size);
	if(total_write <= 0)
	{
		quec_print("send failed");
		return -1;
	}	

	quec_print("send: %s", buffer);
	return total_write;
}

void quec_uhc_demo_process(void *ctx1, void *ctx2, void *ctx3)
{
	int ret = 0;
	quec_sys_event_t sys_event = {0};
		
	while(1)
	{
		ret = k_msgq_get(&quec_demo_msgq, &sys_event, K_FOREVER);
		if(ret != 0)
		{
			quec_print("message error");
			continue;
		}

		if(sys_event.event == QUEC_DEVICE_CONNECT)
		{
			while(1)
			{
				if(quec_at_cmd_send(AT_ATE0, strlen(AT_ATE0)) < 0)
				{
					quec_print("send ate0 fail");
					break;
				}
				
				k_sleep(K_MSEC(100));

				if(quec_at_cmd_send(AT_QSCLK, strlen(AT_QSCLK)) < 0)
				{
					quec_print("send qsclk fail");
					break;
				}

				k_sleep(K_MSEC(100));

				if(quec_at_cmd_send(AT_CGDCONT, strlen(AT_CGDCONT)) < 0)
				{
					quec_print("send cgdcont fail");
					break;
				}

				k_sleep(K_MSEC(100));

				if(quec_at_cmd_send(AT_CSQ, strlen(AT_CSQ)) < 0)
				{
					quec_print("send csq fail");
					break;
				}

				k_sleep(K_MSEC(100));

				if(quec_at_cmd_send(AT_CREG, strlen(AT_CREG)) < 0)
				{
					quec_print("send creg fail");
					break;
				}

				k_sleep(K_MSEC(100));
			}
		}
	}
}


int main(void)
{
	quec_print("app enter");

	quec_demo_param_t *uhc_ctl = (quec_demo_param_t *)&uhc_manager;

	uhc_ctl->device = device_get_binding(QUEC_UHC_DRIVER_NAME);	
	if(uhc_ctl->device == NULL)
	{
		quec_print("no invalid device");
		return -1;
	}

	uhc_ctl->api = (quec_uhc_api_t *)uhc_ctl->device->api;
	if(uhc_ctl->api == NULL)
	{
		quec_print("device error");
		return -1;
	}

	int ret = uhc_ctl->api->ioctl(uhc_ctl->device, QUEC_SET_USER_CALLBACK, quec_event_process);
	if(ret < 0)
	{
		quec_print("set callback failed");
		return -1;		
	}

	k_thread_create(&uhc_ctl->tx_thread,
					(k_thread_stack_t *)tx_task_stack,
					TEST_STASK_SZIE, 
					quec_uhc_demo_process,
					NULL, NULL, NULL,
					K_PRIO_PREEMPT(6), 0, K_MSEC(0));

	//为了防止绑定回调函数时,设备已经连接成功了,错过连接成功的事件;因此手动判断一下
	ret = uhc_ctl->api->ioctl(uhc_ctl->device, QUEC_GET_DEVICE_STATUS, NULL);
	if(ret == QUEC_DEVICE_CONNECT)
	{
		ret = uhc_ctl->api->open(uhc_ctl->device, TEST_PORT);
		if(ret < 0)
		{
			quec_print("port open failed");
			return -1;		
		}

		quec_sys_event_t sys_event = {0};
		sys_event.event = QUEC_DEVICE_CONNECT;
		k_msgq_put(&quec_demo_msgq, &sys_event, K_NO_WAIT);
	}

	return 0;
}

