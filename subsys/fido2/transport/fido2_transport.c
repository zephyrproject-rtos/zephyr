/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fido2/fido2_transport.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

int fido2_transport_init_all(void)
{
	int ret;

	STRUCT_SECTION_FOREACH(fido2_transport, t) {
		if (t->api->init == NULL) {
			continue;
		}

		ret = t->api->init();
		if (ret) {
			LOG_ERR("Transport %s init failed: %d", t->name, ret);
			return ret;
		}

		LOG_INF("Transport %s initialized", t->name);
	}

	return 0;
}

void fido2_transport_register_recv_cb(fido2_transport_recv_cb_t cb, void *user_data)
{
	STRUCT_SECTION_FOREACH(fido2_transport, t) {
		if (t->api->register_recv_cb) {
			t->api->register_recv_cb(cb, user_data);
		}
	}
}

void fido2_transport_shutdown_all(void)
{
	STRUCT_SECTION_FOREACH(fido2_transport, t) {
		if (t->api->shutdown) {
			t->api->shutdown();
		}
	}
}
