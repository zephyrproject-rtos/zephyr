/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/peci.h>
#include <soc.h>

#define TASK_STACK_SIZE         1024
#define PRIORITY                7

/* PECI Host address */
#define PECI_HOST_ADDR          0x30u
/* PECI Host bitrate 1Mbps */
#define PECI_HOST_BITRATE       1000u

#define PECI_CONFIGINDEX_TJMAX  16u
#define PECI_CONFIGHOSTID       0u
#define PECI_CONFIGPARAM        0u

#define PECI_SAFE_TEMP          72

static const struct device *const peci_dev = DEVICE_DT_GET(DT_ALIAS(peci_0));
static bool peci_initialized;
static uint8_t cpu_tjmax;
static uint8_t rx_fcs;
static void monitor_temperature_func(void *dummy1, void *dummy2, void *dummy3);

static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

int peci_ping(void)
{
	int ret;
	struct peci_msg packet;

	printk("%s\n", __func__);

	packet.addr = PECI_HOST_ADDR;
	packet.cmd_code = PECI_CMD_PING;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_PING_WR_LEN;
	packet.rx_buffer.buf = NULL;
	packet.rx_buffer.len = PECI_PING_RD_LEN;

	ret = peci_transfer(peci_dev, &packet);
	if (ret) {
		printk("ping failed %d\n", ret);
		return ret;
	}

	return 0;
}

int peci_get_tjmax(uint8_t *tjmax)
{
	int ret;
	int retries = 3;
	uint8_t peci_resp;
	struct peci_msg packet;

	uint8_t peci_resp_buf[PECI_RD_PKG_LEN_DWORD+1];
	uint8_t peci_req_buf[] = { PECI_CONFIGHOSTID,
				PECI_CONFIGINDEX_TJMAX,
				PECI_CONFIGPARAM & 0x00FF,
				(PECI_CONFIGPARAM & 0xFF00) >> 8,
	};

	packet.tx_buffer.buf = peci_req_buf;
	packet.tx_buffer.len = PECI_RD_PKG_WR_LEN;
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = PECI_RD_PKG_LEN_DWORD;

	do {
		rx_fcs = 0;
		packet.addr = PECI_HOST_ADDR;
		packet.cmd_code = PECI_CMD_RD_PKG_CFG0;

		ret = peci_transfer(peci_dev, &packet);

		for (int i = 0; i < PECI_RD_PKG_LEN_DWORD; i++) {
			printk("%02x\n", packet.rx_buffer.buf[i]);
		}

		peci_resp = packet.rx_buffer.buf[0];
		rx_fcs = packet.rx_buffer.buf[PECI_RD_PKG_LEN_DWORD];
		k_sleep(K_MSEC(1));
		printk("\npeci_resp %x\n", peci_resp);
		retries--;
	} while ((peci_resp != PECI_CC_RSP_SUCCESS) && (retries > 0));

	*tjmax = packet.rx_buffer.buf[3];

	return 0;
}

int peci_get_temp(int *temperature)
{
	int16_t raw_cpu_temp;
	int ret;
	struct peci_msg packet = {0};

	uint8_t peci_resp_buf[PECI_GET_TEMP_RD_LEN+1];

	rx_fcs = 0;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_GET_TEMP_WR_LEN;
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = PECI_GET_TEMP_RD_LEN;

	packet.addr = PECI_HOST_ADDR;
	packet.cmd_code = PECI_CMD_GET_TEMP0;

	ret = peci_transfer(peci_dev, &packet);
	if (ret) {
		printk("Get temp failed %d\n", ret);
		return ret;
	}

	rx_fcs = packet.rx_buffer.buf[PECI_GET_TEMP_RD_LEN];
	printk("R FCS %x\n", rx_fcs);
	printk("Temp bytes: %02x", packet.rx_buffer.buf[0]);
	printk("%02x\n", packet.rx_buffer.buf[1]);

	raw_cpu_temp = (int16_t)(packet.rx_buffer.buf[0] |
			(int16_t)((packet.rx_buffer.buf[1] << 8) & 0xFF00));

	if (raw_cpu_temp == 0x8000) {
		printk("Invalid temp %x\n", raw_cpu_temp);
		*temperature = PECI_SAFE_TEMP;
		return -1;
	}

	raw_cpu_temp = (raw_cpu_temp >> 6) | 0x7E00;
	*temperature = raw_cpu_temp + cpu_tjmax;

	return 0;
}

void read_temp(void)
{
	int ret;
	int temp;

	ret = peci_get_temp(&temp);

	if (!ret) {
		printk("Temperature %d C\n", temp);
	}
}

void get_max_temp(void)
{
	int ret;

	ret = peci_get_tjmax(&cpu_tjmax);
	if (ret) {
		printk("Fail to obtain maximum temperature: %d\n", ret);
	} else {
		printk("Maximum temperature: %u\n", cpu_tjmax);
	}
}

static void monitor_temperature_func(void *dummy1, void *dummy2, void *dummy3)
{
	while (true) {
		k_sleep(K_MSEC(1000));
		if (peci_initialized) {
			read_temp();
		}
	}
}

int main(void)
{
	int ret;

	printk("PECI sample test\n");

	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE,
		monitor_temperature_func, NULL, NULL, NULL, PRIORITY,
		K_INHERIT_PERMS, K_FOREVER);

	if (!device_is_ready(peci_dev)) {
		printk("Err: PECI device is not ready\n");
		return 0;
	}

	ret = peci_config(peci_dev, 1000u);
	if (ret) {
		printk("Err: Fail to configure bitrate\n");
		return 0;
	}

	peci_enable(peci_dev);

	cpu_tjmax = 100;

	get_max_temp();
	printk("Start thread...\n");
	k_thread_start(&temp_id);

	peci_initialized = true;
	return 0;
}
