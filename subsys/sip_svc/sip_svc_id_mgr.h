/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SIP_SVC_ID_MGR_H_
#define ZEPHYR_SIP_SVC_ID_MGR_H_

/**
 * @file
 * @brief Arm SiP services ID manager and ID mapping table.
 *
 */

struct sip_svc_id_pool {
	uint32_t size;
	uint32_t head;
	uint32_t tail;
	uint32_t *id_list;
	uint32_t *id_mask;
};

struct sip_svc_id_map_item {
	uint32_t id;
	void *arg1; /* callback function pointer */
	void *arg2; /* high resp data address */
	void *arg3; /* low resp data address */
	void *arg4; /* maximum resp data addr size */
	void *arg5; /* pointer to private data */
	void *arg6; /* client index */
};

struct sip_svc_id_map {
	uint32_t size;
	struct sip_svc_id_map_item *items;
};

struct sip_svc_id_pool *sip_svc_id_mgr_create(uint32_t size);

void sip_svc_id_mgr_delete(struct sip_svc_id_pool *id_pool);

uint32_t sip_svc_id_mgr_alloc(struct sip_svc_id_pool *id_pool);

void sip_svc_id_mgr_free(struct sip_svc_id_pool *id_pool, uint32_t id);

struct sip_svc_id_map *sip_svc_id_map_create(uint32_t size);

void sip_svc_id_map_delete(struct sip_svc_id_map *id_map);

int sip_svc_id_map_insert_item(struct sip_svc_id_map *id_map, uint32_t id, void *arg1, void *arg2,
			       void *arg3, void *arg4, void *arg5, void *arg6);

int sip_svc_id_map_remove_item(struct sip_svc_id_map *id_map, uint32_t id);

struct sip_svc_id_map_item *sip_svc_id_map_query_item(struct sip_svc_id_map *id_map, uint32_t id);

#endif /* ZEPHYR_SIP_SVC_ID_MGR_H_ */
