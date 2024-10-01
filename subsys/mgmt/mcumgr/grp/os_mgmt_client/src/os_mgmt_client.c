/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_client.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <mgmt/mcumgr/transport/smp_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcumgr_grp_os_client, CONFIG_MCUMGR_GRP_OS_CLIENT_LOG_LEVEL);

static struct os_mgmt_client *active_client;
static K_SEM_DEFINE(mcummgr_os_client_grp_sem, 0, 1);
static K_MUTEX_DEFINE(mcummgr_os_client_grp_mutex);

void os_mgmt_client_init(struct os_mgmt_client *client, struct smp_client_object *smp_client)
{
	client->smp_client = smp_client;
}

#ifdef CONFIG_MCUMGR_GRP_OS_CLIENT_RESET

static int reset_res_fn(struct net_buf *nb, void *user_data)
{
	if (!nb) {
		active_client->status = MGMT_ERR_ETIMEOUT;
	} else {
		active_client->status = MGMT_ERR_EOK;
	}
	k_sem_give(user_data);
	return 0;
}

int os_mgmt_client_reset(struct os_mgmt_client *client)
{
	struct net_buf *nb;
	int rc;

	k_mutex_lock(&mcummgr_os_client_grp_mutex, K_FOREVER);
	active_client = client;
	/* allocate buffer */
	nb = smp_client_buf_allocation(active_client->smp_client, MGMT_GROUP_ID_OS,
				       OS_MGMT_ID_RESET, MGMT_OP_WRITE, SMP_MCUMGR_VERSION_1);
	if (!nb) {
		active_client->status = MGMT_ERR_ENOMEM;
		goto end;
	}
	k_sem_reset(&mcummgr_os_client_grp_sem);
	rc = smp_client_send_cmd(active_client->smp_client, nb, reset_res_fn,
				 &mcummgr_os_client_grp_sem, CONFIG_SMP_CMD_DEFAULT_LIFE_TIME);
	if (rc) {
		active_client->status = rc;
		smp_packet_free(nb);
		goto end;
	}
	/* Wait for process end update event */
	k_sem_take(&mcummgr_os_client_grp_sem, K_FOREVER);
end:
	rc = active_client->status;
	active_client = NULL;
	k_mutex_unlock(&mcummgr_os_client_grp_mutex);
	return rc;
}

#endif /* CONFIG_MCUMGR_GRP_OS_CLIENT_RESET */

#ifdef CONFIG_MCUMGR_GRP_OS_CLIENT_ECHO

static int echo_res_fn(struct net_buf *nb, void *user_data)
{
	struct zcbor_string val = {0};
	zcbor_state_t zsd[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS + 2];
	size_t decoded;
	bool ok;
	int rc;
	struct zcbor_map_decode_key_val echo_response[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &val)
		};

	if (!nb) {
		LOG_ERR("Echo command timeout");
		active_client->status = MGMT_ERR_ETIMEOUT;
		goto end;
	}

	/* Init ZCOR decoder state */
	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), nb->data, nb->len, 1, NULL, 0);

	ok = zcbor_map_decode_bulk(zsd, echo_response, ARRAY_SIZE(echo_response), &decoded) == 0;

	if (!ok) {
		active_client->status = MGMT_ERR_ECORRUPT;
		goto end;
	}
	active_client->status = MGMT_ERR_EOK;
end:
	rc = active_client->status;
	k_sem_give(user_data);
	return rc;
}

int os_mgmt_client_echo(struct os_mgmt_client *client, const char *echo_string, size_t max_len)
{
	struct net_buf *nb;
	int rc;
	bool ok;
	zcbor_state_t zse[CONFIG_MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS];

	k_mutex_lock(&mcummgr_os_client_grp_mutex, K_FOREVER);
	active_client = client;
	nb = smp_client_buf_allocation(active_client->smp_client, MGMT_GROUP_ID_OS, OS_MGMT_ID_ECHO,
				       MGMT_OP_WRITE, SMP_MCUMGR_VERSION_1);
	if (!nb) {
		rc = active_client->status = MGMT_ERR_ENOMEM;
		goto end;
	}

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), nb->data + nb->len, net_buf_tailroom(nb), 0);

	ok = zcbor_map_start_encode(zse, 2) &&
	     zcbor_tstr_put_lit(zse, "d") &&
	     zcbor_tstr_put_term(zse, echo_string, max_len) &&
	     zcbor_map_end_encode(zse, 2);

	if (!ok) {
		smp_packet_free(nb);
		rc = active_client->status = MGMT_ERR_ENOMEM;
		goto end;
	}

	nb->len = zse->payload - nb->data;

	LOG_DBG("Echo Command packet len %d", nb->len);
	k_sem_reset(&mcummgr_os_client_grp_sem);
	rc = smp_client_send_cmd(active_client->smp_client, nb, echo_res_fn,
				 &mcummgr_os_client_grp_sem, CONFIG_SMP_CMD_DEFAULT_LIFE_TIME);
	if (rc) {
		smp_packet_free(nb);
	} else {
		k_sem_take(&mcummgr_os_client_grp_sem, K_FOREVER);
		/* Take response status */
		rc = active_client->status;
	}
end:
	active_client = NULL;
	k_mutex_unlock(&mcummgr_os_client_grp_mutex);
	return rc;
}
#endif
