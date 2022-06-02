/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <net/lwm2m.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define CLIENT_MANUFACTURER	"Zephyr"

static struct lwm2m_ctx client;

#define ENDPOINT_NAME	"ztest"
#define SERVER_ADDR	"coap://192.0.2.2"

static struct k_sem stop_lock;
static struct k_sem disconnect_lock;


static int device_reboot_cb(uint16_t obj_inst_id,
			    uint8_t *args, uint16_t args_len)
{
	k_sem_give(&stop_lock);
	return 0;
}

static void lwm2m_setup(void)
{
	/* cannot be const because LwM2M engine modifies the URL */
	static char addr[] = SERVER_ADDR;
	/* setup SECURITY object */
	lwm2m_engine_set_res_data("0/0/0", addr, sizeof(SERVER_ADDR), 0);

	/* Security Mode */
	lwm2m_engine_set_u8("0/0/2",
			    IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) ? 0 : 3);

	/* Match Security object instance with a Server object instance with
	 * Short Server ID.
	 */
	lwm2m_engine_set_u16("0/0/10", 101);
	lwm2m_engine_set_u16("1/0/0", 101);

	/* setup SERVER object */

	/* setup DEVICE object */

	lwm2m_engine_set_res_data("3/0/0", CLIENT_MANUFACTURER,
				  sizeof(CLIENT_MANUFACTURER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_register_exec_callback("3/0/4", device_reboot_cb);
}

static void rd_client_event(struct lwm2m_ctx *client,
			    enum lwm2m_rd_client_event client_event)
{
	switch (client_event) {
	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		k_sem_give(&disconnect_lock);
		break;
	default:
		break;
	}
}

void main(void)
{
	if (IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX)) {
		srand(time(NULL) + getpid());
	}
	k_sem_init(&stop_lock, 0, K_SEM_MAX_LIMIT);
	k_sem_init(&disconnect_lock, 0, K_SEM_MAX_LIMIT);
	lwm2m_setup();
	lwm2m_rd_client_start(&client, ENDPOINT_NAME, 0, rd_client_event, NULL);
	printf("running\n");
	k_sem_take(&stop_lock, K_FOREVER);
	printf("stopped\n");
	lwm2m_rd_client_stop(&client, rd_client_event, true);
	k_sem_take(&disconnect_lock, K_SECONDS(10));

	if (IS_ENABLED(CONFIG_ARCH_POSIX)) {
		posix_exit(0);
	}
}
