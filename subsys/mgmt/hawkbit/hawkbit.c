/*
 * Copyright (c) 2016-2017 Linaro Limited
 * Copyright (c) 2018 Open Source Foundries Limited
 * Copyright (c) 2018 Foundries.io
 * Copyright (c) 2020 Linumiz
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/data/json.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/mgmt/hawkbit/hawkbit.h>
#include <zephyr/mgmt/hawkbit/config.h>
#include <zephyr/mgmt/hawkbit/event.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/settings/settings.h>
#include <zephyr/smf.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>

#include <bootutil/bootutil_public.h>

#include "hawkbit_device.h"
#include "hawkbit_firmware.h"
#include "hawkbit_priv.h"

LOG_MODULE_REGISTER(hawkbit, CONFIG_HAWKBIT_LOG_LEVEL);

#define RECV_BUFFER_SIZE 640
#define URL_BUFFER_SIZE 300
#define SHA256_HASH_SIZE 32
#define RESPONSE_BUFFER_SIZE 1100
#define DDI_SECURITY_TOKEN_SIZE 32
#define HAWKBIT_RECV_TIMEOUT (300 * MSEC_PER_SEC)
#define HAWKBIT_SET_SERVER_TIMEOUT K_MSEC(300)

#define HAWKBIT_JSON_URL "/" CONFIG_HAWKBIT_TENANT "/controller/v1"

#define HTTP_HEADER_CONTENT_TYPE_JSON "application/json;charset=UTF-8"

#define SLOT1_LABEL slot1_partition
#define SLOT1_SIZE FIXED_PARTITION_SIZE(SLOT1_LABEL)

static uint32_t poll_sleep = (CONFIG_HAWKBIT_POLL_INTERVAL * SEC_PER_MIN);

static bool hawkbit_initialized;

#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY

#ifdef CONFIG_HAWKBIT_DDI_GATEWAY_SECURITY
#define AUTH_HEADER_START "Authorization: GatewayToken "
#else
#define AUTH_HEADER_START "Authorization: TargetToken "
#endif /* CONFIG_HAWKBIT_DDI_GATEWAY_SECURITY */

#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
#define AUTH_HEADER_FULL AUTH_HEADER_START "%s" HTTP_CRLF
#else
#define AUTH_HEADER_FULL AUTH_HEADER_START CONFIG_HAWKBIT_DDI_SECURITY_TOKEN HTTP_CRLF
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */

#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */

static struct hawkbit_config {
	int32_t action_id;
#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
	char server_addr[DNS_MAX_NAME_SIZE + 1];
	char server_port[sizeof(STRINGIFY(__UINT16_MAX__))];
#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
	char ddi_security_token[DDI_SECURITY_TOKEN_SIZE + 1];
#endif
#ifdef CONFIG_HAWKBIT_USE_DYNAMIC_CERT_TAG
	sec_tag_t tls_tag;
#endif
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */
} hb_cfg;

#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
#define HAWKBIT_SERVER hb_cfg.server_addr
#define HAWKBIT_PORT hb_cfg.server_port
#define HAWKBIT_PORT_INT atoi(hb_cfg.server_port)
#else
#define HAWKBIT_SERVER CONFIG_HAWKBIT_SERVER
#define HAWKBIT_PORT STRINGIFY(CONFIG_HAWKBIT_PORT)
#define HAWKBIT_PORT_INT CONFIG_HAWKBIT_PORT
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */

#ifdef CONFIG_HAWKBIT_DDI_NO_SECURITY
#define HAWKBIT_DDI_SECURITY_TOKEN NULL
#elif defined(CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME)
#define HAWKBIT_DDI_SECURITY_TOKEN hb_cfg.ddi_security_token
#else
#define HAWKBIT_DDI_SECURITY_TOKEN CONFIG_HAWKBIT_DDI_SECURITY_TOKEN
#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */

#ifdef CONFIG_HAWKBIT_USE_DYNAMIC_CERT_TAG
#define HAWKBIT_CERT_TAG hb_cfg.tls_tag
#elif defined(HAWKBIT_USE_STATIC_CERT_TAG)
#define HAWKBIT_CERT_TAG CONFIG_HAWKBIT_STATIC_CERT_TAG
#else
#define HAWKBIT_CERT_TAG 0
#endif /* CONFIG_HAWKBIT_USE_DYNAMIC_CERT_TAG */

struct hawkbit_download {
	int download_status;
	int download_progress;
	size_t downloaded_size;
	size_t http_content_size;
	uint8_t file_hash[SHA256_HASH_SIZE];
	int32_t file_size;
};

union hawkbit_results {
	struct hawkbit_dep_res dep;
	struct hawkbit_ctl_res base;
};

struct hawkbit_context {
	int sock;
	uint8_t *response_data;
	size_t response_data_size;
	int32_t json_action_id;
	struct hawkbit_download dl;
	struct flash_img_context flash_ctx;
	enum hawkbit_response code_status;
	bool final_data_received;
	enum hawkbit_http_request type;
	union hawkbit_results results;
};

struct s_object {
	struct smf_ctx ctx;
	struct hawkbit_context hb_context;
	char device_id[DEVICE_ID_HEX_MAX_SIZE];
};

static const struct smf_state hawkbit_states[];

enum hawkbit_state {
	S_HAWKBIT_START,
	S_HAWKBIT_HTTP,
	S_HAWKBIT_PROBE,
	S_HAWKBIT_CONFIG_DEVICE,
	S_HAWKBIT_CANCEL,
	S_HAWKBIT_PROBE_DEPLOYMENT_BASE,
	S_HAWKBIT_REPORT,
	S_HAWKBIT_DOWNLOAD,
	S_HAWKBIT_TERMINATE,
};

int hawkbit_default_config_data_cb(const char *device_id, uint8_t *buffer,
			      const size_t buffer_size);

static hawkbit_config_device_data_cb_handler_t hawkbit_config_device_data_cb_handler =
	hawkbit_default_config_data_cb;

K_SEM_DEFINE(probe_sem, 1, 1);

#ifdef CONFIG_HAWKBIT_EVENT_CALLBACKS
static sys_slist_t event_callbacks = SYS_SLIST_STATIC_INIT(&event_callbacks);
#endif

static const struct json_obj_descr json_href_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_href, href, JSON_TOK_STRING),
};

static const struct json_obj_descr json_status_result_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_status_result, finished, JSON_TOK_STRING),
};

static const struct json_obj_descr json_status_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_status, execution, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_status, result, json_status_result_descr),
};

static const struct json_obj_descr json_ctl_res_sleep_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_ctl_res_sleep, sleep, JSON_TOK_STRING),
};

static const struct json_obj_descr json_ctl_res_polling_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_ctl_res_polling, polling, json_ctl_res_sleep_descr),
};

static const struct json_obj_descr json_ctl_res_links_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_ctl_res_links, deploymentBase, json_href_descr),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_ctl_res_links, cancelAction, json_href_descr),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_ctl_res_links, configData, json_href_descr),
};

static const struct json_obj_descr json_ctl_res_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_ctl_res, config, json_ctl_res_polling_descr),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_ctl_res, _links, json_ctl_res_links_descr),
};

static const struct json_obj_descr json_cfg_data_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg_data, VIN, JSON_TOK_STRING),
};

static const struct json_obj_descr json_cfg_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg, mode, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_cfg, data, json_cfg_data_descr),
};

static const struct json_obj_descr json_cancel_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_cancel, status, json_status_descr),
};

static const struct json_obj_descr json_dep_res_hashes_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_hashes, sha1, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_hashes, md5, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_hashes, sha256, JSON_TOK_STRING),
};

static const struct json_obj_descr json_dep_res_links_descr[] = {
	JSON_OBJ_DESCR_OBJECT_NAMED(struct hawkbit_dep_res_links, "download-http", download_http,
				    json_href_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct hawkbit_dep_res_links, "md5sum-http", md5sum_http,
				    json_href_descr),
};

static const struct json_obj_descr json_dep_res_arts_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_arts, filename, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_dep_res_arts, hashes, json_dep_res_hashes_descr),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_arts, size, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_dep_res_arts, _links, json_dep_res_links_descr),
};

static const struct json_obj_descr json_dep_res_chunk_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_chunk, part, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_chunk, version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_chunk, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct hawkbit_dep_res_chunk, artifacts,
				 HAWKBIT_DEP_MAX_CHUNK_ARTS, num_artifacts, json_dep_res_arts_descr,
				 ARRAY_SIZE(json_dep_res_arts_descr)),
};

static const struct json_obj_descr json_dep_res_deploy_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_deploy, download, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res_deploy, update, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJ_ARRAY(struct hawkbit_dep_res_deploy, chunks, HAWKBIT_DEP_MAX_CHUNKS,
				 num_chunks, json_dep_res_chunk_descr,
				 ARRAY_SIZE(json_dep_res_chunk_descr)),
};

static const struct json_obj_descr json_dep_res_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_res, id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_dep_res, deployment, json_dep_res_deploy_descr),
};

static const struct json_obj_descr json_dep_fbk_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_dep_fbk, status, json_status_descr),
};

static int hawkbit_settings_set(const char *name, size_t len, settings_read_cb read_cb,
				void *cb_arg)
{
	const char *next;
	int rc;

	if (settings_name_steq(name, "action_id", &next) && !next) {
		if (len != sizeof(hb_cfg.action_id)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &hb_cfg.action_id, sizeof(hb_cfg.action_id));
		LOG_DBG("<%s> = %d", "hawkbit/action_id", hb_cfg.action_id);
		if (rc >= 0) {
			return 0;
		}

		return rc;
	}

#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
	if (settings_name_steq(name, "server_addr", &next) && !next) {
		if (len != sizeof(hb_cfg.server_addr)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &hb_cfg.server_addr, sizeof(hb_cfg.server_addr));
		LOG_DBG("<%s> = %s", "hawkbit/server_addr", hb_cfg.server_addr);
		if (rc >= 0) {
			return 0;
		}

		return rc;
	}

	if (settings_name_steq(name, "server_port", &next) && !next) {
		if (len != sizeof(uint16_t)) {
			return -EINVAL;
		}

		uint16_t hawkbit_port = atoi(hb_cfg.server_port);

		rc = read_cb(cb_arg, &hawkbit_port, sizeof(hawkbit_port));
		if (hawkbit_port != atoi(hb_cfg.server_port)) {
			snprintf(hb_cfg.server_port, sizeof(hb_cfg.server_port), "%u",
				 hawkbit_port);
		}
		LOG_DBG("<%s> = %s", "hawkbit/server_port", hb_cfg.server_port);
		if (rc >= 0) {
			return 0;
		}

		return rc;
	}

	if (settings_name_steq(name, "ddi_token", &next) && !next) {
#ifdef CONFIG_HAWKBIT_DDI_NO_SECURITY
		rc = read_cb(cb_arg, NULL, 0);
#else
		if (len != sizeof(hb_cfg.ddi_security_token)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &hb_cfg.ddi_security_token, sizeof(hb_cfg.ddi_security_token));
		LOG_DBG("<%s> = %s", "hawkbit/ddi_token", hb_cfg.ddi_security_token);
		if (rc >= 0) {
			return 0;
		}
#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */
		return rc;
	}
#else  /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */
	if (settings_name_steq(name, "server_addr", NULL) ||
	    settings_name_steq(name, "server_port", NULL) ||
	    settings_name_steq(name, "ddi_token", NULL)) {
		rc = read_cb(cb_arg, NULL, 0);
		return 0;
	}
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */

	return -ENOENT;
}

static int hawkbit_settings_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	LOG_DBG("export hawkbit settings");
	(void)cb("hawkbit/action_id", &hb_cfg.action_id, sizeof(hb_cfg.action_id));
#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
	(void)cb("hawkbit/server_addr", &hb_cfg.server_addr, sizeof(hb_cfg.server_addr));
	uint16_t hawkbit_port = atoi(hb_cfg.server_port);
	(void)cb("hawkbit/server_port", &hawkbit_port, sizeof(hawkbit_port));
#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
	(void)cb("hawkbit/ddi_token", &hb_cfg.ddi_security_token,
		 sizeof(hb_cfg.ddi_security_token));
#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(hawkbit, "hawkbit", NULL, hawkbit_settings_set, NULL,
			       hawkbit_settings_export);

static void hawkbit_event_raise(enum hawkbit_event_type event)
{
#ifdef CONFIG_HAWKBIT_EVENT_CALLBACKS
	struct hawkbit_event_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&event_callbacks, cb, tmp, node) {
		if (cb->event == event && cb->handler) {
			cb->handler(cb, event);
		}
	}
#endif
}

#ifdef CONFIG_HAWKBIT_EVENT_CALLBACKS
int hawkbit_event_add_callback(struct hawkbit_event_callback *cb)
{
	int ret = 0;

	if (cb == NULL || cb->handler == NULL) {
		return -EINVAL;
	}

	ret = k_sem_take(&probe_sem, K_FOREVER);
	if (ret == 0) {
		sys_slist_prepend(&event_callbacks, &cb->node);
		k_sem_give(&probe_sem);
	}
	return ret;
}

int hawkbit_event_remove_callback(struct hawkbit_event_callback *cb)
{
	int ret = 0;

	if (cb == NULL || cb->handler == NULL) {
		return -EINVAL;
	}

	ret = k_sem_take(&probe_sem, K_FOREVER);
	if (ret == 0) {
		if (!sys_slist_find_and_remove(&event_callbacks, &cb->node)) {
			ret = -EINVAL;
		}
		k_sem_give(&probe_sem);
	}

	return ret;
}
#endif /* CONFIG_HAWKBIT_EVENT_CALLBACKS */

static bool start_http_client(int *hb_sock)
{
	int ret = -1;
	struct zsock_addrinfo *addr;
	struct zsock_addrinfo hints = {0};
	int resolve_attempts = 10;
	int protocol = IS_ENABLED(CONFIG_HAWKBIT_USE_TLS) ? IPPROTO_TLS_1_2 : IPPROTO_TCP;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
	}

	while (resolve_attempts--) {
		ret = zsock_getaddrinfo(HAWKBIT_SERVER, HAWKBIT_PORT, &hints, &addr);
		if (ret == 0) {
			break;
		}

		k_sleep(K_MSEC(1));
	}

	if (ret != 0) {
		LOG_ERR("Failed to resolve dns: %d", ret);
		return false;
	}

	*hb_sock = zsock_socket(addr->ai_family, SOCK_STREAM, protocol);
	if (*hb_sock < 0) {
		LOG_ERR("Failed to create TCP socket");
		goto err;
	}

#ifdef CONFIG_HAWKBIT_USE_TLS
	sec_tag_t sec_tag_opt[] = {
		HAWKBIT_CERT_TAG,
	};

	if (zsock_setsockopt(*hb_sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt,
			     sizeof(sec_tag_opt)) < 0) {
		LOG_ERR("Failed to set TLS_TAG option");
		goto err_sock;
	}

	if (zsock_setsockopt(*hb_sock, SOL_TLS, TLS_HOSTNAME, HAWKBIT_SERVER,
			     sizeof(CONFIG_HAWKBIT_SERVER)) < 0) {
		goto err_sock;
	}
#endif /* CONFIG_HAWKBIT_USE_TLS */

	if (zsock_connect(*hb_sock, addr->ai_addr, addr->ai_addrlen) < 0) {
		LOG_ERR("Failed to connect to server");
		goto err_sock;
	}

	zsock_freeaddrinfo(addr);
	return true;

err_sock:
	zsock_close(*hb_sock);
err:
	zsock_freeaddrinfo(addr);
	return false;
}

static void cleanup_connection(int *hb_sock)
{
	if (zsock_close(*hb_sock) < 0) {
		LOG_ERR("Failed to close the socket");
	}
}

static int hawkbit_time2sec(const char *s)
{
	int sec;

	/* Time: HH:MM:SS */
	sec = strtol(s, NULL, 10) * (60 * 60);
	sec += strtol(s + 3, NULL, 10) * 60;
	sec += strtol(s + 6, NULL, 10);

	if (sec < 0) {
		return -1;
	} else {
		return sec;
	}
}

static const char *hawkbit_status_finished(enum hawkbit_status_fini f)
{
	switch (f) {
	case HAWKBIT_STATUS_FINISHED_SUCCESS:
		return "success";
	case HAWKBIT_STATUS_FINISHED_FAILURE:
		return "failure";
	case HAWKBIT_STATUS_FINISHED_NONE:
		return "none";
	default:
		LOG_ERR("%d is invalid", (int)f);
		return NULL;
	}
}

static const char *hawkbit_status_execution(enum hawkbit_status_exec e)
{
	switch (e) {
	case HAWKBIT_STATUS_EXEC_CLOSED:
		return "closed";
	case HAWKBIT_STATUS_EXEC_PROCEEDING:
		return "proceeding";
	case HAWKBIT_STATUS_EXEC_CANCELED:
		return "canceled";
	case HAWKBIT_STATUS_EXEC_SCHEDULED:
		return "scheduled";
	case HAWKBIT_STATUS_EXEC_REJECTED:
		return "rejected";
	case HAWKBIT_STATUS_EXEC_RESUMED:
		return "resumed";
	case HAWKBIT_STATUS_EXEC_NONE:
		return "none";
	default:
		LOG_ERR("%d is invalid", (int)e);
		return NULL;
	}
}

static int hawkbit_device_acid_update(int32_t new_value)
{
	int ret;
	hb_cfg.action_id = new_value;

	ret = settings_save_one("hawkbit/action_id", &hb_cfg.action_id, sizeof(hb_cfg.action_id));
	if (ret < 0) {
		LOG_ERR("Failed to write device id: %d", ret);
		return -EIO;
	}

	return 0;
}

int hawkbit_reset_action_id(void)
{
	int ret;

	if (k_sem_take(&probe_sem, K_NO_WAIT) == 0) {
		ret = hawkbit_device_acid_update(0);
		k_sem_give(&probe_sem);
		return ret;
	}
	return -EAGAIN;
}

int32_t hawkbit_get_action_id(void)
{
	return hb_cfg.action_id;
}

uint32_t hawkbit_get_poll_interval(void)
{
	return poll_sleep;
}

/*
 * Update sleep interval, based on results from hawkBit base polling
 * resource
 */
static void hawkbit_update_sleep(struct hawkbit_ctl_res *hawkbit_res)
{
	int sleep_time;
	const char *sleep = hawkbit_res->config.polling.sleep;

	if (strlen(sleep) != HAWKBIT_SLEEP_LENGTH) {
		LOG_ERR("Invalid poll sleep: %s", sleep);
	} else {
		sleep_time = hawkbit_time2sec(sleep);
		if (sleep_time > 0 && poll_sleep != sleep_time) {
			LOG_DBG("New poll sleep %d seconds", sleep_time);
			poll_sleep = (uint32_t)sleep_time;
		}
	}
}

static char *hawkbit_get_url(const char *href)
{
	char *helper;

	helper = strstr(href, "//");
	if (helper != NULL) {
		helper = strstr(helper + 2u, "/");
	}

	if (!helper) {
		LOG_ERR("Unexpected href format: %s", helper);
		return NULL;
	}
	return helper;
}

/*
 * Find URL component for the device cancel action id
 */
static int hawkbit_find_cancel_action_id(struct hawkbit_ctl_res *res,
					  int32_t *cancel_action_id)
{
	char *helper;

	helper = strstr(res->_links.cancelAction.href, "cancelAction/");
	if (!helper) {
		/* A badly formatted cancel base is a server error */
		LOG_ERR("Missing %s/ in href %s", "cancelAction", res->_links.cancelAction.href);
		return -EINVAL;
	}

	helper += sizeof("cancelAction/");

	*cancel_action_id = strtol(helper, NULL, 10);
	if (*cancel_action_id <= 0) {
		LOG_ERR("Invalid action_id: %d", *cancel_action_id);
		return -EINVAL;
	}

	return 0;
}

static int hawkbit_deployment_get_action_id(struct hawkbit_dep_res *res, int32_t *action_id)
{
	int32_t id;

	id = strtol(res->id, NULL, 10);
	if (id <= 0) {
		LOG_ERR("Invalid action_id: %d", id);
		return -EINVAL;
	}

	*action_id = id;

	return 0;
}

/*
 * Find URL component for this device's deployment operations
 * resource.
 */
static int hawkbit_parse_deployment(struct hawkbit_context *hb_context, struct hawkbit_dep_res *res,
				    char **download_http)
{
	const char *href;
	struct hawkbit_dep_res_chunk *chunk;
	size_t num_chunks, num_artifacts;
	struct hawkbit_dep_res_arts *artifact;

	num_chunks = res->deployment.num_chunks;
	if (num_chunks != 1) {
		LOG_ERR("Expecting 1 chunk (got %d)", num_chunks);
		return -ENOSPC;
	}

	chunk = &res->deployment.chunks[0];
	if (strcmp("bApp", chunk->part)) {
		LOG_ERR("Only part 'bApp' is supported; got %s", chunk->part);
		return -EINVAL;
	}

	num_artifacts = chunk->num_artifacts;
	if (num_artifacts != 1) {
		LOG_ERR("Expecting 1 artifact (got %d)", num_artifacts);
		return -EINVAL;
	}

	artifact = &chunk->artifacts[0];
	if (hex2bin(artifact->hashes.sha256, SHA256_HASH_SIZE << 1, hb_context->dl.file_hash,
		    sizeof(hb_context->dl.file_hash)) != SHA256_HASH_SIZE) {
		return -EINVAL;
	}

	hb_context->dl.file_size = artifact->size;

	if (hb_context->dl.file_size > SLOT1_SIZE) {
		LOG_ERR("Artifact file size too big (got %d, max is %d)", hb_context->dl.file_size,
			SLOT1_SIZE);
		return -ENOSPC;
	}

	/*
	 * Find the download-http href.
	 */
	href = artifact->_links.download_http.href;
	if (!href) {
		LOG_ERR("Missing expected %s href", "download-http");
		return -EINVAL;
	}

	/* Success. */
	if (download_http != NULL) {
		*download_http = hawkbit_get_url(href);
		if (*download_http == NULL) {
			LOG_ERR("Failed to parse %s url", "deploymentBase");
			return -EINVAL;
		}
	}
	return 0;
}

static void hawkbit_dump_deployment(struct hawkbit_dep_res *d)
{
	struct hawkbit_dep_res_chunk *c = &d->deployment.chunks[0];
	struct hawkbit_dep_res_arts *a = &c->artifacts[0];
	struct hawkbit_dep_res_links *l = &a->_links;

	LOG_DBG("%s=%s", "id", d->id);
	LOG_DBG("%s=%s", "download", d->deployment.download);
	LOG_DBG("%s=%s", "update", d->deployment.update);
	LOG_DBG("chunks[0].%s=%s", "part", c->part);
	LOG_DBG("chunks[0].%s=%s", "name", c->name);
	LOG_DBG("chunks[0].%s=%s", "version", c->version);
	LOG_DBG("chunks[0].artifacts[0].%s=%s", "filename", a->filename);
	LOG_DBG("chunks[0].artifacts[0].%s=%s", "hashes.sha1", a->hashes.sha1);
	LOG_DBG("chunks[0].artifacts[0].%s=%s", "hashes.md5", a->hashes.md5);
	LOG_DBG("chunks[0].artifacts[0].%s=%s", "hashes.sha256", a->hashes.sha256);
	LOG_DBG("chunks[0].size=%d", a->size);
	LOG_DBG("%s=%s", "download-http", l->download_http.href);
	LOG_DBG("%s=%s", "md5sum-http", l->md5sum_http.href);
}

int hawkbit_set_custom_data_cb(hawkbit_config_device_data_cb_handler_t cb)
{
	if (IS_ENABLED(CONFIG_HAWKBIT_CUSTOM_ATTRIBUTES)) {
		if (cb == NULL) {
			LOG_ERR("Invalid callback");
			return -EINVAL;
		}

		hawkbit_config_device_data_cb_handler = cb;

		return 0;
	}
	return -ENOTSUP;
}

int hawkbit_default_config_data_cb(const char *device_id, uint8_t *buffer, const size_t buffer_size)
{
	struct hawkbit_cfg cfg = {
		.mode = "merge",
		.data.VIN = device_id,
	};

	return json_obj_encode_buf(json_cfg_descr, ARRAY_SIZE(json_cfg_descr), &cfg, buffer,
				   buffer_size);
}

#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
int hawkbit_set_config(struct hawkbit_runtime_config *config)
{
	if (k_sem_take(&probe_sem, HAWKBIT_SET_SERVER_TIMEOUT) == 0) {
		if (config->server_addr != NULL) {
			strncpy(hb_cfg.server_addr, config->server_addr,
				sizeof(hb_cfg.server_addr));
			LOG_DBG("configured %s: %s", "hawkbit/server_addr", hb_cfg.server_addr);
		}
		if (config->server_port != 0) {
			snprintf(hb_cfg.server_port, sizeof(hb_cfg.server_port), "%u",
				 config->server_port);
			LOG_DBG("configured %s: %s", "hawkbit/server_port", hb_cfg.server_port);
		}
#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
		if (config->auth_token != NULL) {
			strncpy(hb_cfg.ddi_security_token, config->auth_token,
				sizeof(hb_cfg.ddi_security_token));
			LOG_DBG("configured %s: %s", "hawkbit/ddi_token",
				hb_cfg.ddi_security_token);
		}
#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */
#ifdef CONFIG_HAWKBIT_USE_DYNAMIC_CERT_TAG
		if (config->tls_tag != 0) {
			hb_cfg.tls_tag = config->tls_tag;
			LOG_DBG("configured %s: %d", "hawkbit/tls_tag", hb_cfg.tls_tag);
		}
#endif /* CONFIG_HAWKBIT_USE_DYNAMIC_CERT_TAG */
		settings_save_subtree("hawkbit");
		k_sem_give(&probe_sem);
	} else {
		LOG_WRN("failed setting config");
		return -EAGAIN;
	}

	return 0;
}
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */

struct hawkbit_runtime_config hawkbit_get_config(void)
{
	struct hawkbit_runtime_config config = {
		.server_addr = HAWKBIT_SERVER,
		.server_port = HAWKBIT_PORT_INT,
		.auth_token = HAWKBIT_DDI_SECURITY_TOKEN,
		.tls_tag = HAWKBIT_CERT_TAG,
	};

	return config;
}

int hawkbit_init(void)
{
	bool image_ok;
	int ret = 0;

	if (hawkbit_initialized) {
		return 0;
	}

	ret = settings_subsys_init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize settings subsystem: %d", ret);
		return ret;
	}

	ret = settings_load_subtree("hawkbit");
	if (ret < 0) {
		LOG_ERR("Failed to load settings: %d", ret);
		return ret;
	}

	LOG_DBG("Current action_id: %d", hb_cfg.action_id);

	image_ok = boot_is_img_confirmed();
	LOG_INF("Current image is%s confirmed", image_ok ? "" : " not");
	if (!image_ok) {
		ret = boot_write_img_confirmed();
		if (ret < 0) {
			LOG_ERR("Failed to confirm current image: %d", ret);
			return ret;
		}

		LOG_DBG("Marked current image as OK");
		ret = boot_erase_img_bank(FIXED_PARTITION_ID(SLOT1_LABEL));
		if (ret < 0) {
			LOG_ERR("Failed to erase second slot: %d", ret);
			return ret;
		}
	}
	hawkbit_initialized = true;

	return ret;
}

static void response_cb(struct http_response *rsp, enum http_final_call final_data, void *userdata)
{
	struct hawkbit_context *hb_context = userdata;
	size_t body_len;
	int ret, downloaded;
	uint8_t *body_data = NULL, *rsp_tmp = NULL;

	if (rsp->http_status_code != 200) {
		LOG_ERR("HTTP request denied: %d", rsp->http_status_code);
		if (rsp->http_status_code == 401 || rsp->http_status_code == 403) {
			hb_context->code_status = HAWKBIT_PERMISSION_ERROR;
		} else {
			hb_context->code_status = HAWKBIT_METADATA_ERROR;
		}
		return;
	}

	switch (hb_context->type) {
	case HAWKBIT_PROBE:
	case HAWKBIT_PROBE_DEPLOYMENT_BASE:
		if (hb_context->dl.http_content_size == 0) {
			hb_context->dl.http_content_size = rsp->content_length;
		}

		if (rsp->body_found) {
			body_data = rsp->body_frag_start;
			body_len = rsp->body_frag_len;

			if ((hb_context->dl.downloaded_size + body_len) >
			    hb_context->response_data_size) {
				hb_context->response_data_size =
					hb_context->dl.downloaded_size + body_len;
				rsp_tmp = k_realloc(hb_context->response_data,
						    hb_context->response_data_size);
				if (rsp_tmp == NULL) {
					LOG_ERR("Failed to realloc memory");
					hb_context->code_status = HAWKBIT_ALLOC_ERROR;
					break;
				}

				hb_context->response_data = rsp_tmp;
			}
			strncpy(hb_context->response_data + hb_context->dl.downloaded_size,
				body_data, body_len);
			hb_context->dl.downloaded_size += body_len;
		}

		if (final_data == HTTP_DATA_FINAL) {
			if (hb_context->dl.http_content_size != hb_context->dl.downloaded_size) {
				LOG_ERR("HTTP response len mismatch, expected %d, got %d",
					hb_context->dl.http_content_size,
					hb_context->dl.downloaded_size);
				hb_context->code_status = HAWKBIT_METADATA_ERROR;
				break;
			}

			hb_context->response_data[hb_context->dl.downloaded_size] = '\0';
			memset(&hb_context->results, 0, sizeof(hb_context->results));
			if (hb_context->type == HAWKBIT_PROBE) {
				ret = json_obj_parse(
					hb_context->response_data, hb_context->dl.downloaded_size,
					json_ctl_res_descr, ARRAY_SIZE(json_ctl_res_descr),
					&hb_context->results.base);
				if (ret < 0) {
					LOG_ERR("JSON parse error (%s): %d", "HAWKBIT_PROBE", ret);
					hb_context->code_status = HAWKBIT_METADATA_ERROR;
				}
			} else {
				ret = json_obj_parse(
					hb_context->response_data, hb_context->dl.downloaded_size,
					json_dep_res_descr, ARRAY_SIZE(json_dep_res_descr),
					&hb_context->results.dep);
				if (ret < 0) {
					LOG_ERR("JSON parse error (%s): %d", "deploymentBase", ret);
					hb_context->code_status = HAWKBIT_METADATA_ERROR;
				}
			}
		}

		break;

	case HAWKBIT_DOWNLOAD:
		if (hb_context->dl.http_content_size == 0) {
			hb_context->dl.http_content_size = rsp->content_length;
		}

		if (rsp->body_found) {
			body_data = rsp->body_frag_start;
			body_len = rsp->body_frag_len;

			ret = flash_img_buffered_write(&hb_context->flash_ctx, body_data, body_len,
						       final_data == HTTP_DATA_FINAL);
			if (ret < 0) {
				LOG_ERR("Failed to write flash: %d", ret);
				hb_context->code_status = HAWKBIT_DOWNLOAD_ERROR;
				break;
			}
		}

		hb_context->dl.downloaded_size = flash_img_bytes_written(&hb_context->flash_ctx);

		downloaded =
			hb_context->dl.downloaded_size * 100 / hb_context->dl.http_content_size;

		if (downloaded > hb_context->dl.download_progress) {
			hb_context->dl.download_progress = downloaded;
			LOG_DBG("Downloaded: %d%% ", hb_context->dl.download_progress);
		}

		if (final_data == HTTP_DATA_FINAL) {
			hb_context->final_data_received = true;
		}

		break;

	default:

		break;
	}
}

static bool send_request(struct hawkbit_context *hb_context, enum hawkbit_http_request type,
			 char *url_buffer, uint8_t *status_buffer)
{
	int ret = 0;
	uint8_t recv_buf_tcp[RECV_BUFFER_SIZE] = {0};
	struct http_request http_req = {0};
#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
	char header[DDI_SECURITY_TOKEN_SIZE + sizeof(AUTH_HEADER_START) + sizeof(HTTP_CRLF) - 1];

	snprintf(header, sizeof(header), AUTH_HEADER_FULL, hb_cfg.ddi_security_token);
	const char *const headers[] = {header, NULL};
#else
	static const char *const headers[] = {AUTH_HEADER_FULL, NULL};
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */
#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */

	http_req.url = url_buffer;
	http_req.host = HAWKBIT_SERVER;
	http_req.port = HAWKBIT_PORT;
	http_req.protocol = "HTTP/1.1";
	http_req.response = response_cb;
	http_req.recv_buf = recv_buf_tcp;
	http_req.recv_buf_len = sizeof(recv_buf_tcp);
#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
	http_req.header_fields = (const char **)headers;
#endif
	hb_context->final_data_received = false;
	hb_context->type = type;

	switch (type) {
	case HAWKBIT_CONFIG_DEVICE:
		/*
		 * Feedback channel for the config data action
		 * PUT: /{tenant}/controller/v1/{controllerId}/configData
		 */
		http_req.method = HTTP_PUT;
		http_req.content_type_value = HTTP_HEADER_CONTENT_TYPE_JSON;
		http_req.payload = status_buffer;
		http_req.payload_len = strlen(status_buffer);

		break;

	case HAWKBIT_CANCEL:
		/*
		 * Feedback channel for cancel actions
		 * POST: /{tenant}/controller/v1/{controllerId}/cancelAction/{actionId}/feedback
		 */
	case HAWKBIT_REPORT:
		/*
		 * Feedback channel for the DeploymentBase action
		 * POST: /{tenant}/controller/v1/{controllerId}/deploymentBase/{actionId}/feedback
		 */
		http_req.method = HTTP_POST;
		http_req.content_type_value = HTTP_HEADER_CONTENT_TYPE_JSON;
		http_req.payload = status_buffer;
		http_req.payload_len = strlen(status_buffer);

		break;

	case HAWKBIT_PROBE:
		/*
		 * Root resource for an individual Target
		 * GET: /{tenant}/controller/v1/{controllerId}
		 */
	case HAWKBIT_PROBE_DEPLOYMENT_BASE:
		/*
		 * Resource for software module (Deployment Base)
		 * GET: /{tenant}/controller/v1/{controllerId}/deploymentBase/{actionId}
		 */
	case HAWKBIT_DOWNLOAD:
		/*
		 * Resource for software module (Deployment Base)
		 * GET: /{tenant}/controller/v1/{controllerId}/softwaremodules/{softwareModuleId}/
		 *      artifacts/{fileName}
		 */
		http_req.method = HTTP_GET;
		hb_context->dl.http_content_size = 0;
		hb_context->dl.downloaded_size = 0;

		break;

	default:
		return false;
	}

	ret = http_client_req(hb_context->sock, &http_req, HAWKBIT_RECV_TIMEOUT, hb_context);
	if (ret < 0) {
		LOG_ERR("Failed to send request: %d", ret);
		hb_context->code_status = HAWKBIT_NETWORKING_ERROR;
		return false;
	}

	if (IN_RANGE(hb_context->code_status, HAWKBIT_NETWORKING_ERROR, HAWKBIT_ALLOC_ERROR)) {
		return false;
	}

	return true;
}

void hawkbit_reboot(void)
{
	hawkbit_event_raise(HAWKBIT_EVENT_BEFORE_REBOOT);
	LOG_PANIC();
	sys_reboot(IS_ENABLED(CONFIG_HAWKBIT_REBOOT_COLD) ? SYS_REBOOT_COLD : SYS_REBOOT_WARM);
}

static bool check_hawkbit_server(void)
{
	if (strlen(HAWKBIT_SERVER) == 0) {
		if (sizeof(CONFIG_HAWKBIT_SERVER) > 1) {
			hawkbit_set_server_addr(CONFIG_HAWKBIT_SERVER);
		} else {
			LOG_ERR("no valid %s found", "hawkbit/server_addr");
			return false;
		}
	}

	if (HAWKBIT_PORT_INT == 0) {
		if (CONFIG_HAWKBIT_PORT > 0) {
			hawkbit_set_server_port(CONFIG_HAWKBIT_PORT);
		} else {
			LOG_ERR("no valid %s found", "hawkbit/server_port");
			return false;
		}
	}

#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
	if (strlen(HAWKBIT_DDI_SECURITY_TOKEN) == 0) {
		if (sizeof(CONFIG_HAWKBIT_DDI_SECURITY_TOKEN) > 1) {
			hawkbit_set_ddi_security_token(CONFIG_HAWKBIT_DDI_SECURITY_TOKEN);
		} else {
			LOG_ERR("no valid %s found", "hawkbit/ddi_token");
			return false;
		}
	}
#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */

	return true;
}

static void s_start(void *o)
{
	struct s_object *s = (struct s_object *)o;

	if (!hawkbit_initialized) {
		smf_set_terminate(SMF_CTX(s), HAWKBIT_NOT_INITIALIZED);
		return;
	}

	if (!check_hawkbit_server()) {
		smf_set_terminate(SMF_CTX(s), HAWKBIT_NETWORKING_ERROR);
		return;
	}

	if (k_sem_take(&probe_sem, K_NO_WAIT) != 0) {
		LOG_INF("hawkBit is already running");
		smf_set_terminate(SMF_CTX(s), HAWKBIT_PROBE_IN_PROGRESS);
		return;
	}

	if (!boot_is_img_confirmed()) {
		LOG_ERR("Current image is not confirmed");
		k_sem_give(&probe_sem);
		smf_set_terminate(SMF_CTX(s), HAWKBIT_UNCONFIRMED_IMAGE);
		return;
	}

	if (!hawkbit_get_device_identity(s->device_id, DEVICE_ID_HEX_MAX_SIZE)) {
		k_sem_give(&probe_sem);
		smf_set_terminate(SMF_CTX(s), HAWKBIT_METADATA_ERROR);
		return;
	}
}

static void s_end(void *o)
{
	ARG_UNUSED(o);
	k_sem_give(&probe_sem);
}

static void s_http_start(void *o)
{
	struct s_object *s = (struct s_object *)o;

	if (!start_http_client(&s->hb_context.sock)) {
		s->hb_context.code_status = HAWKBIT_NETWORKING_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
	}

	s->hb_context.response_data_size = RESPONSE_BUFFER_SIZE;

	s->hb_context.response_data = k_calloc(s->hb_context.response_data_size, sizeof(uint8_t));
	if (s->hb_context.response_data == NULL) {
		cleanup_connection(&s->hb_context.sock);
		s->hb_context.code_status = HAWKBIT_ALLOC_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
	}
}

static void s_http_end(void *o)
{
	struct s_object *s = (struct s_object *)o;

	cleanup_connection(&s->hb_context.sock);
	k_free(s->hb_context.response_data);
}

/*
 * Root resource for an individual Target
 * GET: /{tenant}/controller/v1/{controllerId}
 */
static void s_probe(void *o)
{
	struct s_object *s = (struct s_object *)o;
	char url_buffer[URL_BUFFER_SIZE] = {0};

	LOG_INF("Polling target data from hawkBit");

	snprintk(url_buffer, sizeof(url_buffer), "%s/%s-%s", HAWKBIT_JSON_URL, CONFIG_BOARD,
		 s->device_id);

	if (!send_request(&s->hb_context, HAWKBIT_PROBE, url_buffer, NULL)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_PROBE");
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	if (s->hb_context.results.base.config.polling.sleep) {
		/* Update the sleep time. */
		LOG_DBG("config.polling.sleep=%s", s->hb_context.results.base.config.polling.sleep);
		hawkbit_update_sleep(&s->hb_context.results.base);
	}

	if (s->hb_context.results.base._links.cancelAction.href) {
		LOG_DBG("_links.%s.href=%s", "cancelAction",
			s->hb_context.results.base._links.cancelAction.href);
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_CANCEL]);
	} else if (s->hb_context.results.base._links.configData.href) {
		LOG_DBG("_links.%s.href=%s", "configData",
			s->hb_context.results.base._links.configData.href);
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_CONFIG_DEVICE]);
	} else if (s->hb_context.results.base._links.deploymentBase.href) {
		LOG_DBG("_links.%s.href=%s", "deploymentBase",
			s->hb_context.results.base._links.deploymentBase.href);
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_PROBE_DEPLOYMENT_BASE]);
	} else {
		s->hb_context.code_status = HAWKBIT_NO_UPDATE;
		hawkbit_event_raise(HAWKBIT_EVENT_NO_UPDATE);
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
	}
}

/*
 * Feedback channel for cancel actions
 * POST: /{tenant}/controller/v1/{controllerId}/cancelAction/{actionId}/feedback
 */
static void s_cancel(void *o)
{
	int ret = 0;
	int32_t cancel_action_id = 0;
	struct s_object *s = (struct s_object *)o;
	char *cancel_base;
	uint8_t status_buffer[CONFIG_HAWKBIT_STATUS_BUFFER_SIZE] = {0};
	char url_buffer[URL_BUFFER_SIZE] = {0};
	struct hawkbit_cancel cancel = {0};

	cancel_base = hawkbit_get_url(s->hb_context.results.base._links.cancelAction.href);
	if (cancel_base == NULL) {
		LOG_ERR("Can't find %s url", "cancelAction");
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	snprintk(url_buffer, sizeof(url_buffer), "%s/%s", cancel_base, "feedback");

	ret = hawkbit_find_cancel_action_id(&s->hb_context.results.base, &cancel_action_id);
	if (ret < 0) {
		LOG_ERR("Can't find %s id: %d", "cancelAction", ret);
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	cancel.status.execution = hawkbit_status_execution(HAWKBIT_STATUS_EXEC_CLOSED);
	cancel.status.result.finished = hawkbit_status_finished(
		hb_cfg.action_id == cancel_action_id ? HAWKBIT_STATUS_FINISHED_FAILURE
						     : HAWKBIT_STATUS_FINISHED_SUCCESS);

	ret = json_obj_encode_buf(json_cancel_descr, ARRAY_SIZE(json_cancel_descr), &cancel,
				  status_buffer, sizeof(status_buffer));
	if (ret) {
		LOG_ERR("Can't encode the JSON script (%s): %d", "HAWKBIT_CANCEL", ret);
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	if (!send_request(&s->hb_context, HAWKBIT_CANCEL, url_buffer, status_buffer)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_CANCEL");
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	LOG_INF("From hawkBit server requested update cancellation %s",
		hb_cfg.action_id == cancel_action_id ? "rejected" : "accepted");

	if (hb_cfg.action_id != cancel_action_id) {
		hawkbit_event_raise(HAWKBIT_EVENT_CANCEL_UPDATE);
	}

	smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_PROBE]);
}

/*
 * Feedback channel for the config data action
 * PUT: /{tenant}/controller/v1/{controllerId}/configData
 */
static void s_config_device(void *o)
{
	int ret = 0;
	struct s_object *s = (struct s_object *)o;
	uint8_t status_buffer[CONFIG_HAWKBIT_STATUS_BUFFER_SIZE] = {0};
	char *url_buffer;

	url_buffer = hawkbit_get_url(s->hb_context.results.base._links.configData.href);
	if (url_buffer == NULL) {
		LOG_ERR("Can't find %s url", "configData");
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	ret = hawkbit_config_device_data_cb_handler(s->device_id, status_buffer,
						    sizeof(status_buffer));
	if (ret) {
		LOG_ERR("Can't encode the JSON script (%s): %d", "HAWKBIT_CONFIG_DEVICE", ret);
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	if (!send_request(&s->hb_context, HAWKBIT_CONFIG_DEVICE, url_buffer, status_buffer)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_CONFIG_DEVICE");
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_PROBE]);
}

/*
 * Resource for software module (Deployment Base)
 * GET: /{tenant}/controller/v1/{controllerId}/deploymentBase/{actionId}
 */
static void s_probe_deployment_base(void *o)
{
	int ret = 0;
	struct s_object *s = (struct s_object *)o;
	char *url_buffer;

	url_buffer = hawkbit_get_url(s->hb_context.results.base._links.deploymentBase.href);
	if (url_buffer == NULL) {
		LOG_ERR("Can't find %s url", "deploymentBase");
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	if (!send_request(&s->hb_context, HAWKBIT_PROBE_DEPLOYMENT_BASE, url_buffer, NULL)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_PROBE_DEPLOYMENT_BASE");
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	hawkbit_dump_deployment(&s->hb_context.results.dep);

	ret = hawkbit_deployment_get_action_id(&s->hb_context.results.dep,
					       &s->hb_context.json_action_id);
	if (ret < 0) {
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	if (hb_cfg.action_id == s->hb_context.json_action_id) {
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_REPORT]);
		return;
	}

	LOG_INF("Ready to download update");
	smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_DOWNLOAD]);
}

/*
 * Feedback channel for the DeploymentBase action
 * POST: /{tenant}/controller/v1/{controllerId}/deploymentBase/{actionId}/feedback
 */
static void s_report(void *o)
{
	int ret = 0;
	struct s_object *s = (struct s_object *)o;
	struct hawkbit_dep_fbk feedback = {
		.status.execution = hawkbit_status_execution(HAWKBIT_STATUS_EXEC_CLOSED),
		.status.result.finished = hawkbit_status_finished(HAWKBIT_STATUS_FINISHED_SUCCESS),
	};
	uint8_t status_buffer[CONFIG_HAWKBIT_STATUS_BUFFER_SIZE] = {0};
	char url_buffer[URL_BUFFER_SIZE] = {0};

	snprintk(url_buffer, sizeof(url_buffer), "%s/%s-%s/%s/%d/%s", HAWKBIT_JSON_URL,
		 CONFIG_BOARD, s->device_id, "deploymentBase", s->hb_context.json_action_id,
		 "feedback");

	LOG_INF("Reporting deployment feedback %s (%s) for action %d",
		feedback.status.result.finished, feedback.status.execution,
		s->hb_context.json_action_id);

	ret = json_obj_encode_buf(json_dep_fbk_descr, ARRAY_SIZE(json_dep_fbk_descr), &feedback,
				  status_buffer, sizeof(status_buffer));
	if (ret) {
		LOG_ERR("Can't encode the JSON script (%s): %d", "HAWKBIT_REPORT", ret);
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	if (!send_request(&s->hb_context, HAWKBIT_REPORT, url_buffer, status_buffer)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_REPORT");
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_PROBE]);
}

static void s_download_start(void *o)
{
	hawkbit_event_raise(HAWKBIT_EVENT_START_DOWNLOAD);
}

static void s_download_end(void *o)
{
	hawkbit_event_raise(HAWKBIT_EVENT_END_DOWNLOAD);
}

/*
 * Resource for software module (Deployment Base)
 * GET: /{tenant}/controller/v1/{controllerId}/softwaremodules/{softwareModuleId}/
 *      artifacts/{fileName}
 */
static void s_download(void *o)
{
	int ret = 0;
	struct s_object *s = (struct s_object *)o;
	struct flash_img_check fic = {0};
	const struct flash_area *flash_area_ptr;
	char *url_buffer;

	ret = hawkbit_parse_deployment(&s->hb_context, &s->hb_context.results.dep, &url_buffer);
	if (ret < 0) {
		LOG_ERR("Failed to parse %s: %d", "deploymentBase", ret);
		s->hb_context.code_status = HAWKBIT_METADATA_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	flash_img_init(&s->hb_context.flash_ctx);

	/* The flash_area pointer has to be copied before the download starts
	 * because the flash_area will be set to NULL after the download has finished.
	 */
	flash_area_ptr = s->hb_context.flash_ctx.flash_area;

	if (!send_request(&s->hb_context, HAWKBIT_DOWNLOAD, url_buffer, NULL)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_DOWNLOAD");
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	/* Check if download finished */
	if (!s->hb_context.final_data_received) {
		LOG_ERR("Download incomplete");
		s->hb_context.code_status = HAWKBIT_DOWNLOAD_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	/* Verify the hash of the stored firmware */
	fic.match = s->hb_context.dl.file_hash;
	fic.clen = s->hb_context.dl.downloaded_size;
	if (flash_img_check(&s->hb_context.flash_ctx, &fic, flash_area_ptr->fa_id)) {
		LOG_ERR("Failed to validate stored firmware");
		s->hb_context.code_status = HAWKBIT_DOWNLOAD_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	/* Request mcuboot to upgrade */
	if (boot_set_next(flash_area_ptr, false, false)) {
		LOG_ERR("Failed to mark the image in slot 1 as pending");
		s->hb_context.code_status = HAWKBIT_DOWNLOAD_ERROR;
		smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
		return;
	}

	/* If everything is successful */
	s->hb_context.code_status = HAWKBIT_UPDATE_INSTALLED;
	hawkbit_device_acid_update(s->hb_context.json_action_id);
	hawkbit_event_raise(HAWKBIT_EVENT_UPDATE_DOWNLOADED);

	smf_set_state(SMF_CTX(s), &hawkbit_states[S_HAWKBIT_TERMINATE]);
}

static void s_terminate(void *o)
{
	struct s_object *s = (struct s_object *)o;

#ifdef CONFIG_HAWKBIT_EVENT_CALLBACKS
	if (IN_RANGE(s->hb_context.code_status, HAWKBIT_NETWORKING_ERROR,
		     HAWKBIT_PROBE_IN_PROGRESS)) {
		hawkbit_event_raise(HAWKBIT_EVENT_ERROR);
		switch (s->hb_context.code_status) {
		case HAWKBIT_NETWORKING_ERROR:
			hawkbit_event_raise(HAWKBIT_EVENT_ERROR_NETWORKING);
			break;

		case HAWKBIT_PERMISSION_ERROR:
			hawkbit_event_raise(HAWKBIT_EVENT_ERROR_PERMISSION);
			break;

		case HAWKBIT_METADATA_ERROR:
			hawkbit_event_raise(HAWKBIT_EVENT_ERROR_METADATA);
			break;

		case HAWKBIT_DOWNLOAD_ERROR:
			hawkbit_event_raise(HAWKBIT_EVENT_ERROR_DOWNLOAD);
			break;

		case HAWKBIT_ALLOC_ERROR:
			hawkbit_event_raise(HAWKBIT_EVENT_ERROR_ALLOC);
			break;
		default:
			break;
		}
	}
#endif

	smf_set_terminate(SMF_CTX(s), s->hb_context.code_status);
}

static const struct smf_state hawkbit_states[] = {
	[S_HAWKBIT_START] = SMF_CREATE_STATE(
		s_start,
		NULL,
		s_end,
		NULL,
		NULL),
	[S_HAWKBIT_HTTP] = SMF_CREATE_STATE(
		s_http_start,
		NULL,
		s_http_end,
		&hawkbit_states[S_HAWKBIT_START],
		NULL),
	[S_HAWKBIT_PROBE] = SMF_CREATE_STATE(
		NULL,
		s_probe,
		NULL,
		&hawkbit_states[S_HAWKBIT_HTTP],
		NULL),
	[S_HAWKBIT_CONFIG_DEVICE] = SMF_CREATE_STATE(
		NULL,
		s_config_device,
		NULL,
		&hawkbit_states[S_HAWKBIT_HTTP],
		NULL),
	[S_HAWKBIT_CANCEL] = SMF_CREATE_STATE(
		NULL,
		s_cancel,
		NULL,
		&hawkbit_states[S_HAWKBIT_HTTP],
		NULL),
	[S_HAWKBIT_PROBE_DEPLOYMENT_BASE] = SMF_CREATE_STATE(
		NULL,
		s_probe_deployment_base,
		NULL,
		&hawkbit_states[S_HAWKBIT_HTTP],
		NULL),
	[S_HAWKBIT_REPORT] = SMF_CREATE_STATE(
		NULL,
		s_report,
		NULL,
		&hawkbit_states[S_HAWKBIT_HTTP],
		NULL),
	[S_HAWKBIT_DOWNLOAD] = SMF_CREATE_STATE(
		s_download_start,
		s_download,
		s_download_end,
		&hawkbit_states[S_HAWKBIT_HTTP],
		NULL),
	[S_HAWKBIT_TERMINATE] = SMF_CREATE_STATE(
		s_terminate,
		NULL,
		NULL,
		NULL,
		NULL),
};

enum hawkbit_response hawkbit_probe(void)
{
	int32_t ret = 0;
	struct s_object s_obj = {0};

	smf_set_initial(SMF_CTX(&s_obj), &hawkbit_states[S_HAWKBIT_PROBE]);
	hawkbit_event_raise(HAWKBIT_EVENT_START_RUN);

	while (1) {
		ret = smf_run_state(SMF_CTX(&s_obj));
		if (ret != 0) {
			hawkbit_event_raise(HAWKBIT_EVENT_END_RUN);
			return (enum hawkbit_response)ret;
		}
	}
}
