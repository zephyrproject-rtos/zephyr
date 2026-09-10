/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *
 * @brief This file contains structures representing JSON messages
 * exchanged with a hawkBit server
 */

#ifndef __HAWKBIT_PRIV_H__
#define __HAWKBIT_PRIV_H__

#include <zephyr/data/json.h>

#define HAWKBIT_SLEEP_LENGTH 8

enum hawkbit_http_request {
	HAWKBIT_PROBE,
	HAWKBIT_CONFIG_DEVICE,
	HAWKBIT_CANCEL,
	HAWKBIT_PROBE_DEPLOYMENT_BASE,
	HAWKBIT_REPORT,
	HAWKBIT_DOWNLOAD,
};

enum hawkbit_status_fini {
	HAWKBIT_STATUS_FINISHED_SUCCESS,
	HAWKBIT_STATUS_FINISHED_FAILURE,
	HAWKBIT_STATUS_FINISHED_NONE,
};

enum hawkbit_status_exec {
	HAWKBIT_STATUS_EXEC_CLOSED = 0,
	HAWKBIT_STATUS_EXEC_PROCEEDING,
	HAWKBIT_STATUS_EXEC_CANCELED,
	HAWKBIT_STATUS_EXEC_SCHEDULED,
	HAWKBIT_STATUS_EXEC_REJECTED,
	HAWKBIT_STATUS_EXEC_RESUMED,
	HAWKBIT_STATUS_EXEC_NONE,
};

struct hawkbit_href {
	const char *href;
};

struct hawkbit_status_result {
	const char *finished;
};

struct hawkbit_status {
	struct hawkbit_status_result result;
	const char *execution;
};

struct hawkbit_ctl_res_sleep {
	const char *sleep;
};

struct hawkbit_ctl_res_polling {
	struct hawkbit_ctl_res_sleep polling;
};

struct hawkbit_ctl_res_links {
	struct hawkbit_href deploymentBase;
	struct hawkbit_href configData;
	struct hawkbit_href cancelAction;
};

struct hawkbit_ctl_res {
	struct hawkbit_ctl_res_polling config;
	struct hawkbit_ctl_res_links _links;
};

struct hawkbit_cfg_data {
	const char *VIN;
};

struct hawkbit_cfg {
	const char *mode;
	struct hawkbit_cfg_data data;
};

struct hawkbit_cancel {
	struct hawkbit_status status;
};

/* Maximum number of chunks we support */
#define HAWKBIT_DEP_MAX_CHUNKS 1
/* Maximum number of artifacts per chunk. */
#define HAWKBIT_DEP_MAX_CHUNK_ARTS 1

struct hawkbit_dep_res_hashes {
	const char *sha1;
	const char *md5;
	const char *sha256;
};

struct hawkbit_dep_res_links {
	struct hawkbit_href download_http;
	struct hawkbit_href md5sum_http;
};

struct hawkbit_dep_res_arts {
	const char *filename;
	struct hawkbit_dep_res_hashes hashes;
	struct hawkbit_dep_res_links _links;
	int size;
};

struct hawkbit_dep_res_chunk {
	const char *part;
	const char *name;
	const char *version;
	struct hawkbit_dep_res_arts artifacts[HAWKBIT_DEP_MAX_CHUNK_ARTS];
	size_t num_artifacts;
};

struct hawkbit_dep_res_deploy {
	const char *download;
	const char *update;
	struct hawkbit_dep_res_chunk chunks[HAWKBIT_DEP_MAX_CHUNKS];
	size_t num_chunks;
};

struct hawkbit_dep_res {
	const char *id;
	struct hawkbit_dep_res_deploy deployment;
};

struct hawkbit_dep_fbk {
	struct hawkbit_status status;
};

#endif /* __HAWKBIT_PRIV_H__ */
