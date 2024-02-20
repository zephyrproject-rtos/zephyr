/*
 * Copyright (c) 2016-2017 Linaro Limited
 * Copyright (c) 2018 Open Source Foundries Limited
 * Copyright (c) 2018 Foundries.io
 * Copyright (c) 2020 Linumiz
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hawkbit, CONFIG_HAWKBIT_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/data/json.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/storage/flash_map.h>

#include "hawkbit_priv.h"
#include "hawkbit_device.h"
#include <zephyr/mgmt/hawkbit.h>
#include "hawkbit_firmware.h"

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#define CA_CERTIFICATE_TAG 1
#include <zephyr/net/tls_credentials.h>
#endif

#define ADDRESS_ID 1

#define CANCEL_BASE_SIZE 50
#define RECV_BUFFER_SIZE 640
#define URL_BUFFER_SIZE 300
#define SHA256_HASH_SIZE 32
#define STATUS_BUFFER_SIZE 200
#define DOWNLOAD_HTTP_SIZE 200
#define DEPLOYMENT_BASE_SIZE 50
#define RESPONSE_BUFFER_SIZE 1100
#define HAWKBIT_RECV_TIMEOUT (300 * MSEC_PER_SEC)

#define HTTP_HEADER_CONTENT_TYPE_JSON "application/json;charset=UTF-8"

#define SLOT1_LABEL slot1_partition
#define SLOT1_SIZE FIXED_PARTITION_SIZE(SLOT1_LABEL)

#define STORAGE_LABEL storage_partition
#define STORAGE_DEV FIXED_PARTITION_DEVICE(STORAGE_LABEL)
#define STORAGE_OFFSET FIXED_PARTITION_OFFSET(STORAGE_LABEL)

#if ((CONFIG_HAWKBIT_POLL_INTERVAL > 1) && (CONFIG_HAWKBIT_POLL_INTERVAL < 43200))
static uint32_t poll_sleep = (CONFIG_HAWKBIT_POLL_INTERVAL * 60 * MSEC_PER_SEC);
#else
static uint32_t poll_sleep = (300 * MSEC_PER_SEC);
#endif

static struct nvs_fs fs;

struct hawkbit_download {
	int download_status;
	int download_progress;
	size_t downloaded_size;
	size_t http_content_size;
	uint8_t file_hash[SHA256_HASH_SIZE];
};

static struct hawkbit_context {
	int sock;
	int32_t action_id;
	uint8_t *response_data;
	int32_t json_action_id;
	size_t url_buffer_size;
	size_t status_buffer_size;
	struct hawkbit_download dl;
	struct http_request http_req;
	struct flash_img_context flash_ctx;
	uint8_t url_buffer[URL_BUFFER_SIZE];
	uint8_t status_buffer[STATUS_BUFFER_SIZE];
	uint8_t recv_buf_tcp[RECV_BUFFER_SIZE];
	enum hawkbit_response code_status;
	bool final_data_received;
} hb_context;

static union {
	struct hawkbit_dep_res dep;
	struct hawkbit_ctl_res base;
	struct hawkbit_cancel cancel;
} hawkbit_results;

static struct k_work_delayable hawkbit_work_handle;

static struct k_sem probe_sem;

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
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg_data, hwRevision, JSON_TOK_STRING),
};

static const struct json_obj_descr json_cfg_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg, mode, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_cfg, data, json_cfg_data_descr),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg, id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg, time, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_cfg, status, json_status_descr),
};

static const struct json_obj_descr json_close_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_close, id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_close, time, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_close, status, json_status_descr),
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
	JSON_OBJ_DESCR_PRIM(struct hawkbit_dep_fbk, id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_dep_fbk, status, json_status_descr),
};

static bool start_http_client(void)
{
	int ret = -1;
	struct zsock_addrinfo *addr;
	struct zsock_addrinfo hints;
	int resolve_attempts = 10;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	int protocol = IPPROTO_TLS_1_2;
#else
	int protocol = IPPROTO_TCP;
#endif

	(void)memset(&hints, 0, sizeof(hints));

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
	}

	while (resolve_attempts--) {
		ret = zsock_getaddrinfo(CONFIG_HAWKBIT_SERVER, CONFIG_HAWKBIT_PORT, &hints, &addr);
		if (ret == 0) {
			break;
		}

		k_sleep(K_MSEC(1));
	}

	if (ret != 0) {
		LOG_ERR("Failed to resolve dns: %d", ret);
		return false;
	}

	hb_context.sock = zsock_socket(addr->ai_family, SOCK_STREAM, protocol);
	if (hb_context.sock < 0) {
		LOG_ERR("Failed to create TCP socket");
		goto err;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_opt[] = {
		CA_CERTIFICATE_TAG,
	};

	if (setsockopt(hb_context.sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt,
		       sizeof(sec_tag_opt)) < 0) {
		LOG_ERR("Failed to set TLS_TAG option");
		goto err_sock;
	}

	if (setsockopt(hb_context.sock, SOL_TLS, TLS_HOSTNAME, CONFIG_HAWKBIT_SERVER,
		       sizeof(CONFIG_HAWKBIT_SERVER)) < 0) {
		goto err_sock;
	}
#endif

	if (zsock_connect(hb_context.sock, addr->ai_addr, addr->ai_addrlen) < 0) {
		LOG_ERR("Failed to connect to server");
		goto err_sock;
	}

	zsock_freeaddrinfo(addr);
	return true;

err_sock:
	zsock_close(hb_context.sock);
err:
	zsock_freeaddrinfo(addr);
	return false;
}

static void cleanup_connection(void)
{
	if (zsock_close(hb_context.sock) < 0) {
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

	ret = nvs_write(&fs, ADDRESS_ID, &new_value, sizeof(new_value));
	if (ret < 0) {
		LOG_ERR("Failed to write device id: %d", ret);
		return -EIO;
	}

	return 0;
}

/*
 * Update sleep interval, based on results from hawkbit base polling
 * resource
 */
static void hawkbit_update_sleep(struct hawkbit_ctl_res *hawkbit_res)
{
	uint32_t sleep_time;
	const char *sleep = hawkbit_res->config.polling.sleep;

	if (strlen(sleep) != HAWKBIT_SLEEP_LENGTH) {
		LOG_ERR("Invalid poll sleep: %s", sleep);
	} else {
		sleep_time = hawkbit_time2sec(sleep);
		if (sleep_time > 0 && poll_sleep != (MSEC_PER_SEC * sleep_time)) {
			LOG_DBG("New poll sleep %d seconds", sleep_time);
			poll_sleep = sleep_time * MSEC_PER_SEC;
		}
	}
}

/*
 * Find URL component for the device cancel operation and action id
 */
static int hawkbit_find_cancelAction_base(struct hawkbit_ctl_res *res, char *cancel_base)
{
	size_t len;
	const char *href;
	char *helper, *endptr;

	href = res->_links.cancelAction.href;
	if (!href) {
		*cancel_base = '\0';
		return 0;
	}

	LOG_DBG("_links.%s.href=%s", "cancelAction", href);

	helper = strstr(href, "cancelAction/");
	if (!helper) {
		/* A badly formatted cancel base is a server error */
		LOG_ERR("Missing %s/ in href %s", "cancelAction", href);
		return -EINVAL;
	}

	len = strlen(helper);
	if (len > CANCEL_BASE_SIZE - 1) {
		/* Lack of memory is an application error */
		LOG_ERR("%s %s is too big (len %zu, max %zu)", "cancelAction", helper, len,
			CANCEL_BASE_SIZE - 1);
		return -ENOMEM;
	}

	strncpy(cancel_base, helper, CANCEL_BASE_SIZE);

	helper = strtok(helper, "/");
	if (helper == 0) {
		return -EINVAL;
	}

	helper = strtok(NULL, "/");
	if (helper == 0) {
		return -EINVAL;
	}

	hb_context.action_id = strtol(helper, &endptr, 10);
	if (hb_context.action_id <= 0) {
		LOG_ERR("Invalid action_id: %d", hb_context.action_id);
		return -EINVAL;
	}

	return 0;
}

/*
 * Find URL component for the device's deployment operations
 * resource
 */
static int hawkbit_find_deployment_base(struct hawkbit_ctl_res *res, char *deployment_base)
{
	const char *href;
	const char *helper;
	size_t len;

	href = res->_links.deploymentBase.href;
	if (!href) {
		*deployment_base = '\0';
		return 0;
	}

	LOG_DBG("_links.%s.href=%s", "deploymentBase", href);

	helper = strstr(href, "deploymentBase/");
	if (!helper) {
		/* A badly formatted deployment base is a server error */
		LOG_ERR("Missing %s/ in href %s", "deploymentBase", href);
		return -EINVAL;
	}

	len = strlen(helper);
	if (len > DEPLOYMENT_BASE_SIZE - 1) {
		/* Lack of memory is an application error */
		LOG_ERR("%s %s is too big (len %zu, max %zu)", "deploymentBase", helper, len,
			DEPLOYMENT_BASE_SIZE - 1);
		return -ENOMEM;
	}

	strncpy(deployment_base, helper, DEPLOYMENT_BASE_SIZE);
	return 0;
}

/*
 * Find URL component for this device's deployment operations
 * resource.
 */
static int hawkbit_parse_deployment(struct hawkbit_dep_res *res, int32_t *json_action_id,
				    char *download_http, int32_t *file_size)
{
	int32_t size;
	char *endptr;
	const char *href;
	const char *helper;
	struct hawkbit_dep_res_chunk *chunk;
	size_t len, num_chunks, num_artifacts;
	struct hawkbit_dep_res_arts *artifact;

	hb_context.action_id = strtol(res->id, &endptr, 10);
	if (hb_context.action_id < 0) {
		LOG_ERR("Invalid action_id: %d", hb_context.action_id);
		return -EINVAL;
	}

	*json_action_id = hb_context.action_id;

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
	if (hex2bin(artifact->hashes.sha256, SHA256_HASH_SIZE << 1, hb_context.dl.file_hash,
		    sizeof(hb_context.dl.file_hash)) != SHA256_HASH_SIZE) {
		return -EINVAL;
	}

	size = artifact->size;

	if (size > SLOT1_SIZE) {
		LOG_ERR("Artifact file size too big (got %d, max is %d)", size, SLOT1_SIZE);
		return -ENOSPC;
	}

	/*
	 * Find the download-http href. We only support the DEFAULT
	 * tenant on the same hawkbit server.
	 */
	href = artifact->_links.download_http.href;
	if (!href) {
		LOG_ERR("Missing expected %s href", "download-http");
		return -EINVAL;
	}

	helper = strstr(href, "/DEFAULT/controller/v1");
	if (!helper) {
		LOG_ERR("Unexpected %s href format: %s", "download-http", helper);
		return -EINVAL;
	}

	len = strlen(helper);
	if (len == 0) {
		LOG_ERR("Empty %s", "download-http");
		return -EINVAL;
	} else if (len > DOWNLOAD_HTTP_SIZE - 1) {
		LOG_ERR("%s %s is too big (len %zu, max %zu)", "download-http", helper, len,
			DOWNLOAD_HTTP_SIZE - 1);
		return -ENOMEM;
	}

	/* Success. */
	strncpy(download_http, helper, DOWNLOAD_HTTP_SIZE);
	*file_size = size;
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

int hawkbit_init(void)
{
	bool image_ok;
	int ret = 0, rc = 0;
	struct flash_pages_info info;
	int32_t action_id;

	fs.flash_device = STORAGE_DEV;
	if (!device_is_ready(fs.flash_device)) {
		LOG_ERR("Flash device not ready");
		return -ENODEV;
	}

	fs.offset = STORAGE_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		LOG_ERR("Failed to get storage page info: %d", rc);
		return -EIO;
	}

	fs.sector_size = info.size;
	fs.sector_count = 3U;

	rc = nvs_mount(&fs);
	if (rc) {
		LOG_ERR("Failed to mount storage flash: %d", rc);
		return rc;
	}

	rc = nvs_read(&fs, ADDRESS_ID, &action_id, sizeof(action_id));
	LOG_DBG("Current action_id: %d", action_id);

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
		if (ret) {
			LOG_ERR("Failed to erase second slot: %d", ret);
			return ret;
		}
	}

	k_sem_init(&probe_sem, 1, 1);

	return ret;
}

static int enum_for_http_req_string(char *userdata)
{
	int i = 0;
	char *name = http_request[i].http_req_str;

	while (name) {
		if (strcmp(name, userdata) == 0) {
			return http_request[i].n;
		}

		name = http_request[++i].http_req_str;
	}

	return 0;
}

static void response_cb(struct http_response *rsp, enum http_final_call final_data, void *userdata)
{
	static size_t body_len;
	int ret, type, downloaded;
	uint8_t *body_data = NULL, *rsp_tmp = NULL;
	static size_t response_buffer_size = RESPONSE_BUFFER_SIZE;

	type = enum_for_http_req_string(userdata);

	switch (type) {
	case HAWKBIT_PROBE:
		if (hb_context.dl.http_content_size == 0) {
			hb_context.dl.http_content_size = rsp->content_length;
		}

		if (rsp->body_found) {
			body_data = rsp->body_frag_start;
			body_len = rsp->body_frag_len;

			if ((hb_context.dl.downloaded_size + body_len) > response_buffer_size) {
				response_buffer_size <<= 1;
				rsp_tmp = realloc(hb_context.response_data, response_buffer_size);
				if (rsp_tmp == NULL) {
					LOG_ERR("Failed to realloc memory");
					hb_context.code_status = HAWKBIT_METADATA_ERROR;
					break;
				}

				hb_context.response_data = rsp_tmp;
			}
			strncpy(hb_context.response_data + hb_context.dl.downloaded_size, body_data,
				body_len);
			hb_context.dl.downloaded_size += body_len;
		}

		if (final_data == HTTP_DATA_FINAL) {
			if (hb_context.dl.http_content_size != hb_context.dl.downloaded_size) {
				LOG_ERR("HTTP response len mismatch, expected %d, got %d",
					hb_context.dl.http_content_size,
					hb_context.dl.downloaded_size);
				hb_context.code_status = HAWKBIT_METADATA_ERROR;
				break;
			}

			if (rsp->http_status_code / 100 == 4) {
				LOG_ERR("HTTP request denied: %d", rsp->http_status_code);
				hb_context.code_status = HAWKBIT_PERMISSION_ERROR;
				break;
			}

			hb_context.response_data[hb_context.dl.downloaded_size] = '\0';
			ret = json_obj_parse(hb_context.response_data,
					     hb_context.dl.downloaded_size, json_ctl_res_descr,
					     ARRAY_SIZE(json_ctl_res_descr), &hawkbit_results.base);
			if (ret < 0) {
				LOG_ERR("JSON parse error (%s): %d", "HAWKBIT_PROBE", ret);
				hb_context.code_status = HAWKBIT_METADATA_ERROR;
			}
		}

		break;

	case HAWKBIT_CLOSE:
	case HAWKBIT_REPORT:
	case HAWKBIT_CONFIG_DEVICE:
		if (strcmp(rsp->http_status, "OK") < 0) {
			LOG_ERR("Failed to cancel the update");
		}

		break;

	case HAWKBIT_PROBE_DEPLOYMENT_BASE:
		if (hb_context.dl.http_content_size == 0) {
			hb_context.dl.http_content_size = rsp->content_length;
		}

		if (rsp->body_found) {
			body_data = rsp->body_frag_start;
			body_len = rsp->body_frag_len;

			if ((hb_context.dl.downloaded_size + body_len) > response_buffer_size) {
				response_buffer_size <<= 1;
				rsp_tmp = realloc(hb_context.response_data, response_buffer_size);
				if (rsp_tmp == NULL) {
					LOG_ERR("Failed to realloc memory");
					hb_context.code_status = HAWKBIT_METADATA_ERROR;
					break;
				}

				hb_context.response_data = rsp_tmp;
			}
			strncpy(hb_context.response_data + hb_context.dl.downloaded_size, body_data,
				body_len);
			hb_context.dl.downloaded_size += body_len;
		}

		if (final_data == HTTP_DATA_FINAL) {
			if (hb_context.dl.http_content_size != hb_context.dl.downloaded_size) {
				LOG_ERR("HTTP response len mismatch, expected %d, got %d",
					hb_context.dl.http_content_size,
					hb_context.dl.downloaded_size);
				hb_context.code_status = HAWKBIT_METADATA_ERROR;
				break;
			}

			hb_context.response_data[hb_context.dl.downloaded_size] = '\0';
			ret = json_obj_parse(hb_context.response_data,
					     hb_context.dl.downloaded_size, json_dep_res_descr,
					     ARRAY_SIZE(json_dep_res_descr), &hawkbit_results.dep);
			if (ret < 0) {
				LOG_ERR("JSON parse error (%s): %d", "deploymentBase", ret);
				hb_context.code_status = HAWKBIT_METADATA_ERROR;
			}
		}

		break;

	case HAWKBIT_DOWNLOAD:
		if (hb_context.dl.http_content_size == 0) {
			hb_context.dl.http_content_size = rsp->content_length;
		}

		if (rsp->body_found) {
			body_data = rsp->body_frag_start;
			body_len = rsp->body_frag_len;

			ret = flash_img_buffered_write(&hb_context.flash_ctx, body_data, body_len,
						       final_data == HTTP_DATA_FINAL);
			if (ret < 0) {
				LOG_ERR("Failed to write flash: %d", ret);
				hb_context.code_status = HAWKBIT_DOWNLOAD_ERROR;
				break;
			}
		}

		hb_context.dl.downloaded_size = flash_img_bytes_written(&hb_context.flash_ctx);

		downloaded = hb_context.dl.downloaded_size * 100 / hb_context.dl.http_content_size;

		if (downloaded > hb_context.dl.download_progress) {
			hb_context.dl.download_progress = downloaded;
			LOG_DBG("Downloaded: %d%% ", hb_context.dl.download_progress);
		}

		if (final_data == HTTP_DATA_FINAL) {
			hb_context.final_data_received = true;
		}

		break;
	}
}

static bool send_request(enum http_method method, enum hawkbit_http_request type,
			 enum hawkbit_status_fini finished, enum hawkbit_status_exec execution)
{
	int ret = 0;

	struct hawkbit_cfg cfg;
	struct hawkbit_close close;
	struct hawkbit_dep_fbk feedback;
	char acid[11];
	const char *fini = hawkbit_status_finished(finished);
	const char *exec = hawkbit_status_execution(execution);
	char device_id[DEVICE_ID_HEX_MAX_SIZE] = { 0 };
#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
	static const char *const headers[] = {
#ifdef CONFIG_HAWKBIT_DDI_GATEWAY_SECURITY
		"Authorization: GatewayToken " CONFIG_HAWKBIT_DDI_SECURITY_TOKEN "\r\n",
#else
		"Authorization: TargetToken " CONFIG_HAWKBIT_DDI_SECURITY_TOKEN "\r\n",
#endif /* CONFIG_HAWKBIT_DDI_GATEWAY_SECURITY */
		NULL
	};
#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */

	if (!hawkbit_get_device_identity(device_id, DEVICE_ID_HEX_MAX_SIZE)) {
		hb_context.code_status = HAWKBIT_METADATA_ERROR;
	}

	memset(&hb_context.http_req, 0, sizeof(hb_context.http_req));
	memset(&hb_context.recv_buf_tcp, 0, sizeof(hb_context.recv_buf_tcp));
	hb_context.http_req.url = hb_context.url_buffer;
	hb_context.http_req.method = method;
	hb_context.http_req.host = CONFIG_HAWKBIT_SERVER;
	hb_context.http_req.port = CONFIG_HAWKBIT_PORT;
	hb_context.http_req.protocol = "HTTP/1.1";
	hb_context.http_req.response = response_cb;
	hb_context.http_req.recv_buf = hb_context.recv_buf_tcp;
	hb_context.http_req.recv_buf_len = sizeof(hb_context.recv_buf_tcp);
#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
	hb_context.http_req.header_fields = (const char **)headers;
#endif
	hb_context.final_data_received = false;

	switch (type) {
	case HAWKBIT_PROBE:
		ret = http_client_req(hb_context.sock, &hb_context.http_req, HAWKBIT_RECV_TIMEOUT,
				      "HAWKBIT_PROBE");
		if (ret < 0) {
			LOG_ERR("Unable to send HTTP request (%s): %d", "HAWKBIT_PROBE", ret);
			return false;
		}

		break;

	case HAWKBIT_CONFIG_DEVICE:
		memset(&cfg, 0, sizeof(cfg));
		cfg.mode = "merge";
		cfg.data.VIN = device_id;
		cfg.data.hwRevision = "3";
		cfg.id = "";
		cfg.time = "";
		cfg.status.execution = exec;
		cfg.status.result.finished = fini;

		ret = json_obj_encode_buf(json_cfg_descr, ARRAY_SIZE(json_cfg_descr), &cfg,
					  hb_context.status_buffer,
					  hb_context.status_buffer_size - 1);
		if (ret) {
			LOG_ERR("Can't encode the JSON script (%s): %d", "HAWKBIT_CONFIG_DEVICE",
				ret);
			return false;
		}

		hb_context.http_req.content_type_value = HTTP_HEADER_CONTENT_TYPE_JSON;
		hb_context.http_req.payload = hb_context.status_buffer;
		hb_context.http_req.payload_len = strlen(hb_context.status_buffer);

		ret = http_client_req(hb_context.sock, &hb_context.http_req, HAWKBIT_RECV_TIMEOUT,
				      "HAWKBIT_CONFIG_DEVICE");
		if (ret < 0) {
			LOG_ERR("Unable to send HTTP request (%s): %d", "HAWKBIT_CONFIG_DEVICE",
				ret);
			return false;
		}

		break;

	case HAWKBIT_CLOSE:
		memset(&close, 0, sizeof(close));
		memset(&hb_context.status_buffer, 0, sizeof(hb_context.status_buffer));
		snprintk(acid, sizeof(acid), "%d", hb_context.action_id);
		close.id = acid;
		close.time = "";
		close.status.execution = exec;
		close.status.result.finished = fini;

		ret = json_obj_encode_buf(json_close_descr, ARRAY_SIZE(json_close_descr), &close,
					  hb_context.status_buffer,
					  hb_context.status_buffer_size - 1);
		if (ret) {
			LOG_ERR("Can't encode the JSON script (%s): %d", "HAWKBIT_CLOSE", ret);
			return false;
		}

		hb_context.http_req.content_type_value = HTTP_HEADER_CONTENT_TYPE_JSON;
		hb_context.http_req.payload = hb_context.status_buffer;
		hb_context.http_req.payload_len = strlen(hb_context.status_buffer);

		ret = http_client_req(hb_context.sock, &hb_context.http_req, HAWKBIT_RECV_TIMEOUT,
				      "HAWKBIT_CLOSE");
		if (ret < 0) {
			LOG_ERR("Unable to send HTTP request (%s): %d", "HAWKBIT_CLOSE", ret);
			return false;
		}

		break;

	case HAWKBIT_PROBE_DEPLOYMENT_BASE:
		hb_context.http_req.content_type_value = NULL;
		ret = http_client_req(hb_context.sock, &hb_context.http_req, HAWKBIT_RECV_TIMEOUT,
				      "HAWKBIT_PROBE_DEPLOYMENT_BASE");
		if (ret < 0) {
			LOG_ERR("Unable to send HTTP request (%s): %d",
				"HAWKBIT_PROBE_DEPLOYMENT_BASE", ret);
			return false;
		}

		break;

	case HAWKBIT_REPORT:
		if (!fini || !exec) {
			return -EINVAL;
		}

		LOG_INF("Reporting deployment feedback %s (%s) for action %d", fini, exec,
			hb_context.json_action_id);
		/* Build JSON */
		memset(&feedback, 0, sizeof(feedback));
		snprintk(acid, sizeof(acid), "%d", hb_context.json_action_id);
		feedback.id = acid;
		feedback.status.result.finished = fini;
		feedback.status.execution = exec;

		ret = json_obj_encode_buf(json_dep_fbk_descr, ARRAY_SIZE(json_dep_fbk_descr),
					  &feedback, hb_context.status_buffer,
					  hb_context.status_buffer_size - 1);
		if (ret) {
			LOG_ERR("Can't encode the JSON script (%s): %d", "HAWKBIT_REPORT", ret);
			return ret;
		}

		hb_context.http_req.content_type_value = HTTP_HEADER_CONTENT_TYPE_JSON;
		hb_context.http_req.payload = hb_context.status_buffer;
		hb_context.http_req.payload_len = strlen(hb_context.status_buffer);

		ret = http_client_req(hb_context.sock, &hb_context.http_req, HAWKBIT_RECV_TIMEOUT,
				      "HAWKBIT_REPORT");
		if (ret < 0) {
			LOG_ERR("Unable to send HTTP request (%s): %d", "HAWKBIT_REPORT", ret);
			return false;
		}

		break;

	case HAWKBIT_DOWNLOAD:
		ret = http_client_req(hb_context.sock, &hb_context.http_req, HAWKBIT_RECV_TIMEOUT,
				      "HAWKBIT_DOWNLOAD");
		if (ret < 0) {
			LOG_ERR("Unable to send HTTP request (%s): %d", "HAWKBIT_DOWNLOAD", ret);
			return false;
		}

		break;
	}

	return true;
}

enum hawkbit_response hawkbit_probe(void)
{
	int ret;
	int32_t action_id;
	int32_t file_size = 0;
	struct flash_img_check fic;
	char device_id[DEVICE_ID_HEX_MAX_SIZE] = { 0 },
	     cancel_base[CANCEL_BASE_SIZE] = { 0 },
	     download_http[DOWNLOAD_HTTP_SIZE] = { 0 },
	     deployment_base[DEPLOYMENT_BASE_SIZE] = { 0 },
	     firmware_version[BOOT_IMG_VER_STRLEN_MAX] = { 0 };

	if (k_sem_take(&probe_sem, K_NO_WAIT) != 0) {
		return HAWKBIT_PROBE_IN_PROGRESS;
	}

	memset(&hb_context, 0, sizeof(hb_context));
	hb_context.response_data = malloc(RESPONSE_BUFFER_SIZE);

	if (!boot_is_img_confirmed()) {
		LOG_ERR("Current image is not confirmed");
		hb_context.code_status = HAWKBIT_UNCONFIRMED_IMAGE;
		goto error;
	}

	if (!hawkbit_get_firmware_version(firmware_version, BOOT_IMG_VER_STRLEN_MAX)) {
		hb_context.code_status = HAWKBIT_METADATA_ERROR;
		goto error;
	}

	if (!hawkbit_get_device_identity(device_id, DEVICE_ID_HEX_MAX_SIZE)) {
		hb_context.code_status = HAWKBIT_METADATA_ERROR;
		goto error;
	}

	if (!start_http_client()) {
		hb_context.code_status = HAWKBIT_NETWORKING_ERROR;
		goto error;
	}

	/*
	 * Query the hawkbit base polling resource.
	 */
	LOG_INF("Polling target data from hawkBit");

	memset(hb_context.url_buffer, 0, sizeof(hb_context.url_buffer));
	hb_context.dl.http_content_size = 0;
	hb_context.dl.downloaded_size = 0;
	hb_context.url_buffer_size = URL_BUFFER_SIZE;
	snprintk(hb_context.url_buffer, hb_context.url_buffer_size, "%s/%s-%s",
		 HAWKBIT_JSON_URL, CONFIG_BOARD, device_id);
	memset(&hawkbit_results.base, 0, sizeof(hawkbit_results.base));

	if (!send_request(HTTP_GET, HAWKBIT_PROBE, HAWKBIT_STATUS_FINISHED_NONE,
			  HAWKBIT_STATUS_EXEC_NONE)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_PROBE");
		hb_context.code_status = HAWKBIT_NETWORKING_ERROR;
		goto cleanup;
	}

	if (hb_context.code_status == HAWKBIT_METADATA_ERROR) {
		goto cleanup;
	}

	if (hawkbit_results.base.config.polling.sleep) {
		/* Update the sleep time. */
		hawkbit_update_sleep(&hawkbit_results.base);
		LOG_DBG("config.polling.sleep=%s", hawkbit_results.base.config.polling.sleep);
	}


	if (hawkbit_results.base._links.cancelAction.href) {
		ret = hawkbit_find_cancelAction_base(&hawkbit_results.base, cancel_base);
		memset(hb_context.url_buffer, 0, sizeof(hb_context.url_buffer));
		hb_context.dl.http_content_size = 0;
		hb_context.url_buffer_size = URL_BUFFER_SIZE;
		snprintk(hb_context.url_buffer, hb_context.url_buffer_size, "%s/%s-%s/%s/feedback",
			 HAWKBIT_JSON_URL, CONFIG_BOARD, device_id, cancel_base);
		memset(&hawkbit_results.cancel, 0, sizeof(hawkbit_results.cancel));

		if (!send_request(HTTP_POST, HAWKBIT_CLOSE, HAWKBIT_STATUS_FINISHED_SUCCESS,
				  HAWKBIT_STATUS_EXEC_CLOSED)) {
			LOG_ERR("Send request failed (%s)", "HAWKBIT_CLOSE");
			hb_context.code_status = HAWKBIT_NETWORKING_ERROR;
			goto cleanup;
		}

		hb_context.code_status = HAWKBIT_CANCEL_UPDATE;
		goto cleanup;
	}

	if (hawkbit_results.base._links.configData.href) {
		LOG_DBG("_links.%s.href=%s", "configData",
			hawkbit_results.base._links.configData.href);
		memset(hb_context.url_buffer, 0, sizeof(hb_context.url_buffer));
		hb_context.dl.http_content_size = 0;
		hb_context.url_buffer_size = URL_BUFFER_SIZE;
		snprintk(hb_context.url_buffer, hb_context.url_buffer_size, "%s/%s-%s/%s",
			 HAWKBIT_JSON_URL, CONFIG_BOARD, device_id, "configData");

		if (!send_request(HTTP_PUT, HAWKBIT_CONFIG_DEVICE, HAWKBIT_STATUS_FINISHED_SUCCESS,
				  HAWKBIT_STATUS_EXEC_CLOSED)) {
			LOG_ERR("Send request failed (%s)", "HAWKBIT_CONFIG_DEVICE");
			hb_context.code_status = HAWKBIT_NETWORKING_ERROR;
			goto cleanup;
		}
	}

	ret = hawkbit_find_deployment_base(&hawkbit_results.base, deployment_base);
	if (ret < 0) {
		hb_context.code_status = HAWKBIT_METADATA_ERROR;
		LOG_ERR("Unable to find URL for the device's deploymentBase: %d", ret);
		goto cleanup;
	}

	if (strlen(deployment_base) == 0) {
		hb_context.code_status = HAWKBIT_NO_UPDATE;
		goto cleanup;
	}

	memset(hb_context.url_buffer, 0, sizeof(hb_context.url_buffer));
	hb_context.dl.http_content_size = 0;
	hb_context.dl.downloaded_size = 0;
	hb_context.url_buffer_size = URL_BUFFER_SIZE;
	snprintk(hb_context.url_buffer, hb_context.url_buffer_size, "%s/%s-%s/%s", HAWKBIT_JSON_URL,
		 CONFIG_BOARD, device_id, deployment_base);
	memset(&hawkbit_results.dep, 0, sizeof(hawkbit_results.dep));
	memset(hb_context.response_data, 0, RESPONSE_BUFFER_SIZE);

	if (!send_request(HTTP_GET, HAWKBIT_PROBE_DEPLOYMENT_BASE, HAWKBIT_STATUS_FINISHED_NONE,
			  HAWKBIT_STATUS_EXEC_NONE)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_PROBE_DEPLOYMENT_BASE");
		hb_context.code_status = HAWKBIT_NETWORKING_ERROR;
		goto cleanup;
	}

	if (hb_context.code_status == HAWKBIT_METADATA_ERROR) {
		goto cleanup;
	}

	hawkbit_dump_deployment(&hawkbit_results.dep);

	hb_context.dl.http_content_size = 0;
	ret = hawkbit_parse_deployment(&hawkbit_results.dep, &hb_context.json_action_id,
				       download_http, &file_size);
	if (ret < 0) {
		LOG_ERR("Failed to parse deploymentBase: %d", ret);
		goto cleanup;
	}

	nvs_read(&fs, ADDRESS_ID, &action_id, sizeof(action_id));

	if (action_id == (int32_t)hb_context.json_action_id) {
		LOG_INF("Preventing repeated attempt to install %d", hb_context.json_action_id);
		hb_context.dl.http_content_size = 0;
		memset(hb_context.url_buffer, 0, sizeof(hb_context.url_buffer));
		hb_context.url_buffer_size = URL_BUFFER_SIZE;
		snprintk(hb_context.url_buffer, hb_context.url_buffer_size,
			 "%s/%s-%s/%s/%d/feedback", HAWKBIT_JSON_URL, CONFIG_BOARD,
			 device_id, "deploymentBase", hb_context.json_action_id);

		if (!send_request(HTTP_POST, HAWKBIT_REPORT, HAWKBIT_STATUS_FINISHED_SUCCESS,
				  HAWKBIT_STATUS_EXEC_CLOSED)) {
			LOG_ERR("Send request failed (%s)", "HAWKBIT_REPORT");
			hb_context.code_status = HAWKBIT_NETWORKING_ERROR;
			goto cleanup;
		}

		hb_context.code_status = HAWKBIT_OK;
		goto cleanup;
	}

	LOG_INF("Ready to install update");

	hb_context.dl.http_content_size = 0;
	memset(hb_context.url_buffer, 0, sizeof(hb_context.url_buffer));
	hb_context.url_buffer_size = URL_BUFFER_SIZE;

	snprintk(hb_context.url_buffer, hb_context.url_buffer_size, "%s", download_http);

	flash_img_init(&hb_context.flash_ctx);

	if (send_request(HTTP_GET, HAWKBIT_DOWNLOAD, HAWKBIT_STATUS_FINISHED_NONE,
			 HAWKBIT_STATUS_EXEC_NONE)) {
		LOG_ERR("Send request failed (%s)", "HAWKBIT_DOWNLOAD");
		hb_context.code_status = HAWKBIT_NETWORKING_ERROR;
		goto cleanup;
	}

	if (hb_context.code_status == HAWKBIT_DOWNLOAD_ERROR) {
		goto cleanup;
	}

	/* Check if download finished */
	if (!hb_context.final_data_received) {
		LOG_ERR("Download incomplete");
		hb_context.code_status = HAWKBIT_DOWNLOAD_ERROR;
		goto cleanup;
	}

	/* Verify the hash of the stored firmware */
	fic.match = hb_context.dl.file_hash;
	fic.clen = hb_context.dl.downloaded_size;
	if (flash_img_check(&hb_context.flash_ctx, &fic, FIXED_PARTITION_ID(SLOT1_LABEL))) {
		LOG_ERR("Failed to validate stored firmware");
		hb_context.code_status = HAWKBIT_DOWNLOAD_ERROR;
		goto cleanup;
	}

	/* Request mcuboot to upgrade */
	if (boot_request_upgrade(BOOT_UPGRADE_TEST)) {
		LOG_ERR("Failed to mark the image in slot 1 as pending");
		hb_context.code_status = HAWKBIT_DOWNLOAD_ERROR;
		goto cleanup;
	}

	/* If everything is successful */
	hb_context.code_status = HAWKBIT_UPDATE_INSTALLED;
	hawkbit_device_acid_update(hb_context.json_action_id);

	hb_context.dl.http_content_size = 0;

cleanup:
	cleanup_connection();

error:
	free(hb_context.response_data);
	k_sem_give(&probe_sem);
	return hb_context.code_status;
}

static void autohandler(struct k_work *work)
{
	switch (hawkbit_probe()) {
	case HAWKBIT_UNCONFIRMED_IMAGE:
		LOG_ERR("Current image is not confirmed");
		LOG_ERR("Rebooting to previous confirmed image");
		LOG_ERR("If this image is flashed using a hardware tool");
		LOG_ERR("Make sure that it is a confirmed image");
		k_sleep(K_SECONDS(1));
		sys_reboot(SYS_REBOOT_WARM);
		break;

	case HAWKBIT_NO_UPDATE:
		LOG_INF("No update found");
		break;

	case HAWKBIT_CANCEL_UPDATE:
		LOG_INF("hawkBit update cancelled from server");
		break;

	case HAWKBIT_OK:
		LOG_INF("Image is already updated");
		break;

	case HAWKBIT_UPDATE_INSTALLED:
		LOG_INF("Update installed, please reboot");
		break;

	case HAWKBIT_DOWNLOAD_ERROR:
		LOG_INF("Update failed");
		break;

	case HAWKBIT_NETWORKING_ERROR:
		LOG_INF("Network error");
		break;

	case HAWKBIT_PERMISSION_ERROR:
		LOG_INF("Permission error");
		break;

	case HAWKBIT_METADATA_ERROR:
		LOG_INF("Metadata error");
		break;

	case HAWKBIT_PROBE_IN_PROGRESS:
		LOG_INF("hawkBit is already running");
		break;
	}

	k_work_reschedule(&hawkbit_work_handle, K_MSEC(poll_sleep));
}

void hawkbit_autohandler(void)
{
	k_work_init_delayable(&hawkbit_work_handle, autohandler);
	k_work_reschedule(&hawkbit_work_handle, K_NO_WAIT);
}
