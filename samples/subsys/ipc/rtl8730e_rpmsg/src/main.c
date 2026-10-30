/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ipc_service static-vrings link test. The CA32 host runs two instances:
 *   - ipc0: CA32 <-> KM4 (PSRAM @ 0x60700000)
 *   - ipc1: CA32 <-> KM0 (KM0 SRAM @ 0x2301B000)
 *
 * Devicetree sets each core's role and instances; remotes define only ipc0.
 * Each bound endpoint sends a running counter and logs what it receives.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtl8730e_rpmsg, LOG_LEVEL_INF);

struct link {
	const struct device *dev;
	const char *label;
	struct ipc_ept ep;
	struct ipc_ept_cfg cfg;
	struct k_sem bound_sem;
	uint32_t counter;
};

static void ep_bound(void *priv)
{
	struct link *l = priv;

	k_sem_give(&l->bound_sem);
	LOG_INF("[%s] endpoint bound", l->label);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	struct link *l = priv;
	uint32_t val = 0;

	memcpy(&val, data, MIN(len, sizeof(val)));
	LOG_INF("[%s] received %zu bytes: %u", l->label, len, val);
}

static void ep_error(const char *message, void *priv)
{
	struct link *l = priv;

	LOG_ERR("[%s] rpmsg error: %s", l->label, message);
}

#define LINK_INITIALIZER(node_id, _label)                                      \
	{                                                                      \
		.dev = DEVICE_DT_GET(node_id),                                 \
		.label = _label,                                               \
	}

static struct link links[] = {
	LINK_INITIALIZER(DT_NODELABEL(ipc0), "ipc0"),
#if DT_NODE_EXISTS(DT_NODELABEL(ipc1))
	LINK_INITIALIZER(DT_NODELABEL(ipc1), "ipc1"),
#endif
};

#define NUM_LINKS ARRAY_SIZE(links)

static int link_setup(struct link *l)
{
	int ret;

	k_sem_init(&l->bound_sem, 0, 1);

	l->cfg.name = "rtl8730e_rpmsg";
	l->cfg.priv = l;
	l->cfg.cb.bound = ep_bound;
	l->cfg.cb.received = ep_recv;
	l->cfg.cb.error = ep_error;

	ret = ipc_service_open_instance(l->dev);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("[%s] ipc_service_open_instance() failed: %d", l->label, ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(l->dev, &l->ep, &l->cfg);
	if (ret < 0) {
		LOG_ERR("[%s] ipc_service_register_endpoint() failed: %d", l->label, ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	LOG_INF("RTL8730E rpmsg sample started (%zu link(s))", NUM_LINKS);

	for (size_t i = 0; i < NUM_LINKS; i++) {
		int ret = link_setup(&links[i]);

		if (ret < 0) {
			return ret;
		}
	}

	/* Wait for every link to bind before starting traffic. */
	for (size_t i = 0; i < NUM_LINKS; i++) {
		k_sem_take(&links[i].bound_sem, K_FOREVER);
	}

	while (1) {
		for (size_t i = 0; i < NUM_LINKS; i++) {
			struct link *l = &links[i];
			int ret = ipc_service_send(&l->ep, &l->counter, sizeof(l->counter));

			if (ret < 0 && ret != -ENOMEM) {
				LOG_ERR("[%s] ipc_service_send() failed: %d", l->label, ret);
			} else if (ret >= 0) {
				LOG_INF("[%s] sent %u", l->label, l->counter);
				l->counter++;
			}
		}
		k_msleep(1000);
	}

	return 0;
}
