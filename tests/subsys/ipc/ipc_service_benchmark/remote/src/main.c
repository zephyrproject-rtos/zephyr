/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>
#include "common.h"

LOG_MODULE_REGISTER(remote, LOG_LEVEL_DBG);

struct test_data {
#if defined(CONFIG_MULTITHREADING)
	struct k_sem sem;
#else
	atomic_t bound_flag;
#endif
	struct ipc_ept ep;
	struct ipc_ept_cfg cfg;
};

static struct test_data ep_data[1];
static char test_name[128];
static uint32_t test_start_stamp;

static void ep_bound(void *priv)
{
	struct test_data *tdata = priv;
#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&tdata->sem);
#else
	tdata->bound_flag = 1;
#endif
	LOG_INF("Ep:%s bounded", tdata->cfg.name);
}

void test_start(struct test_data *unused, const uint8_t *data, size_t len)
{
	ARG_UNUSED(unused);
	memcpy(test_name, data, len);
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		(void)cpu_load_get(true);
	}
	test_start_stamp = k_cycle_get_32();
}

void test_end(struct test_data *unused, const uint8_t *data, size_t len)
{
	ARG_UNUSED(unused);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	uint32_t t = k_cycle_get_32() - test_start_stamp;

	t = k_cyc_to_us_floor32(t);

	printk("Test %s took %d us ", test_name, t);
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		uint32_t load = cpu_load_get(true);

		printk("CPU load: %d.%d", load / 10, load % 10);
	}
	printk("\n");

}

static void ep_recv(const void *data, size_t len, void *priv)
{
	struct test_data *tdata = priv;
	const struct data_packet *pkt = data;

	switch (pkt->type) {
	case TYPE_RSP:
	{
		static bool once;
		int ret;

		ret = ipc_service_send(&tdata->ep, data, len);
		if ((ret < 0) && !once) {
			once = true;
			LOG_ERR("failed to send len:%d", len);
		}
		break;
	}
	case TYPE_TEST_START:
		test_start(tdata, pkt->data, len - sizeof(pkt->type));
		break;
	case TYPE_TEST_END:
		test_end(tdata, pkt->data, len - sizeof(pkt->type));
		break;
	default:
		break;
	}
}

int main(void)
{
	const struct device *ipc0 = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	int ret;


	LOG_INF("IPC-service REMOTE demo started");

	ret = ipc_service_open_instance(ipc0);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	ep_data[0].cfg.name = "ep0";
	ep_data[0].cfg.cb.bound = ep_bound;
	ep_data[0].cfg.cb.received = ep_recv;
	ep_data[0].cfg.priv = &ep_data[0];
#if defined(CONFIG_MULTITHREADING)
	k_sem_init(&ep_data[0].sem, 0, 1);
#else
	ep_data[0].bound_flag = 0;
#endif

	ret = ipc_service_register_endpoint(ipc0, &ep_data[0].ep, &ep_data[0].cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return ret;
	}

#if defined(CONFIG_MULTITHREADING)
	k_sem_take(&ep_data[0].sem, K_FOREVER);
#else
	while (ep_data[0].bound_flag == 0) {
	};
#endif

	k_sleep(K_FOREVER);
	return 0;
}
