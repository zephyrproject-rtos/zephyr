/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LWM2M_REGISTRY_H
#define LWM2M_REGISTRY_H
#include <zephyr/sys/ring_buffer.h>
#include "lwm2m_object.h"

/**
 * @brief Creates and registers an object instance to the registry. Object specified by
 * @p obj_id must exist.
 *
 * @param[in] obj_id Object id of the object instance.
 * @param[in] obj_inst_id Object instance id of the object instance to be created.
 * @param[out] obj_inst Engine object instance buffer pointer.
 * @return 0 for success or negative in case of error.
 */
int lwm2m_create_obj_inst(uint16_t obj_id, uint16_t obj_inst_id,
			  struct lwm2m_engine_obj_inst **obj_inst);

/**
 * @brief Deletes the object instance given by @p obj_id / @p obj_inst_id.
 *
 * @param[in] obj_id Object id of the object instance to be deleted
 * @param[in] obj_inst_id Object instance id of the object instance to be deleted
 * @return 0 for success or negative in case of error.
 */
int lwm2m_delete_obj_inst(uint16_t obj_id, uint16_t obj_inst_id);

/**
 * @brief Get the engine object instance specified by @p msg->path. Usually only used in do_write
 * functions in the different encoders.
 *
 * @param[in] msg lwm2m message containing the path to the object instance.
 * @param[out] obj_inst Engine object instance buffer pointer.
 * @param[out] created Points to a 1 if an object instance was created. Else 0. Can also be NULL.
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_or_create_engine_obj(struct lwm2m_message *msg,
				   struct lwm2m_engine_obj_inst **obj_inst, uint8_t *created);

/**
 * @brief Appends an object to the registry. Usually called in the init function
 * of an object.
 *
 * @param[in] obj Object to be registered.
 */
void lwm2m_register_obj(struct lwm2m_engine_obj *obj);

/**
 * @brief Removes an object from the registry.
 *
 * @param[in] obj Object to be removed
 */
void lwm2m_unregister_obj(struct lwm2m_engine_obj *obj);

/**
 * @brief Get the resource instance specified by @p path. Creates and allocates a new one
 * if the resource instance does not exist.
 *
 * @param[in] path Path to resource instance (i.e 100/100/100/1)
 * @param[out] res Engine resource buffer pointer.
 * @param[out] res_inst Engine resource instance buffer pointer.
 * @return 0 for success or negative in case of error.
 */
int lwm2m_engine_get_create_res_inst(const struct lwm2m_obj_path *path,
				     struct lwm2m_engine_res **res,
				     struct lwm2m_engine_res_inst **res_inst);

/**
 * @brief Gets the resource specified by @p pathstr.
 *
 * @deprecated Use lwm2m_get_resource() instead.
 *
 * @param[in] pathstr Path to resource (i.e 100/100/100/x, the fourth component is optional)
 * @param[out] res Engine resource buffer pointer.
 * @return 0 for success or negative in case of error.
 */
int lwm2m_engine_get_resource(const char *pathstr, struct lwm2m_engine_res **res);

/**
 * @brief Gets the resource specified by @p path.
 *
 * @param[in] path Path to resource (i.e 100/100/100/x, the fourth component is optional)
 * @param[out] res Engine resource buffer pointer.
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_resource(const struct lwm2m_obj_path *path, struct lwm2m_engine_res **res);

/**
 * @brief Returns pointer to the object in the registry specified by @p path.
 * Returns NULL if it does not exist.
 *
 * @param[in] path lwm2m_obj_path to the object.
 * @return Pointer to engine object in the registry
 */
struct lwm2m_engine_obj *lwm2m_engine_get_obj(const struct lwm2m_obj_path *path);

/**
 * @brief Returns pointer to the object instance in the registry specified by @p path.
 * Returns NULL if it does not exist.
 *
 * @param[in] path lwm2m_obj_path to the object instance.
 * @return Pointer to engine object instance in the registry
 */
struct lwm2m_engine_obj_inst *lwm2m_engine_get_obj_inst(const struct lwm2m_obj_path *path);

/**
 * @brief Returns pointer to the resource in the registry specified by @p path.
 * Returns NULL if it does not exist.
 *
 * @param[in] path lwm2m_obj_path to the resource.
 * @return Pointer to engine resource in the registry
 */
struct lwm2m_engine_res *lwm2m_engine_get_res(const struct lwm2m_obj_path *path);

/**
 * @brief Returns pointer to the resource instance in the registry specified by @p path.
 * Returns NULL if it does not exist.
 *
 * @param[in] path lwm2m_obj_path to the resource instance.
 * @return Pointer to the engine resource instance in the registry
 */
struct lwm2m_engine_res_inst *lwm2m_engine_get_res_inst(const struct lwm2m_obj_path *path);

/**
 * @brief Get the engine obj specified by @p obj_id.
 *
 * @param[in] obj_id Object id of the object.
 * @return Pointer to the engine object.
 */
struct lwm2m_engine_obj *get_engine_obj(int obj_id);

/**
 * @brief Get the engine object instance @p obj_id /@p obj_inst_id
 *
 * @param[in] obj_id Object if of the object instance.
 * @param[in] obj_inst_id Object instance id of object instance
 * @return Pointer to the object instance.
 */
struct lwm2m_engine_obj_inst *get_engine_obj_inst(int obj_id, int obj_inst_id);

/**
 * @brief Get object instance, field, resource and resource instance. All variables may be NULL.
 * Example to get resource instance:
 * int ret = path_to_objs(&path, NULL, NULL, NULL, &res_inst);
 *
 * @param[in] path lwm2m object path to the object/object instance/resource/resource instance
 * to get.
 * @param[out] obj_inst Engine object instance buffer pointer.
 * @param[out] obj_field Engine object field buffer pointer.
 * @param[out] res Engine resource buffer pointer.
 * @param[out] res_inst Engine resource instance buffer pointer.
 * @return 0 for success or negative in case of error.
 */
int path_to_objs(const struct lwm2m_obj_path *path, struct lwm2m_engine_obj_inst **obj_inst,
		 struct lwm2m_engine_obj_field **obj_field, struct lwm2m_engine_res **res,
		 struct lwm2m_engine_res_inst **res_inst);

/**
 * @brief Returns the object instance in the registry with object id = @p obj_id that has the
 * smalles object instance id strictly larger than @p obj_inst_id.
 *
 * @param[in] obj_id Object id of the object instance.
 * @param[in] obj_inst_id Lower bound of the object instance id.
 * @return Pointer to the object instance. NULL if not found.
 */
struct lwm2m_engine_obj_inst *next_engine_obj_inst(int obj_id, int obj_inst_id);

/**
 * @brief Returns a boolean to signal whether or not or it's needed to report object version.
 *
 * @param[in] obj Pointer to engine object.
 * @return True if it's needed to report object version, else false
 */
bool lwm2m_engine_shall_report_obj_version(const struct lwm2m_engine_obj *obj);

/**
 * @brief Returns the binding mode in use.
 *
 * Defaults to UDP (U). In LwM2M 1.0 when queue mode is enabled, return "UQ".
 *
 * @param[out] binding Data buffer.
 */
void lwm2m_engine_get_binding(char *binding);

/**
 * @brief Returns the queue mode (Q) if queue mode is enabled. Else an empty string.
 *
 * @param[out] queue Queue mode data buffer
 */
void lwm2m_engine_get_queue_mode(char *queue);

/**
 * @brief Returns the engine object field with resource id @p res_id of the object @p obj.
 *
 * @param[in] obj lwm2m engine object of the field.
 * @param[in] res_id Resource id of the field.
 * @return Pointer to an engine object field, or NULL if it does not exist
 */
struct lwm2m_engine_obj_field *lwm2m_get_engine_obj_field(struct lwm2m_engine_obj *obj, int res_id);

size_t lwm2m_engine_get_opaque_more(struct lwm2m_input_context *in, uint8_t *buf, size_t buflen,
				    struct lwm2m_opaque_context *opaque, bool *last_block);


/* Resources */
sys_slist_t *lwm2m_engine_obj_list(void);
sys_slist_t *lwm2m_engine_obj_inst_list(void);

/* Data cache Internal API */

/**
 * LwM2M Time series resoursce data storage
 */
struct lwm2m_time_series_resource {
	/* object list */
	sys_snode_t node;
	/* Resource Path url */
	struct lwm2m_obj_path path;
	/* Ring buffer */
	struct ring_buf rb;
};

#if defined(CONFIG_LWM2M_RESOURCE_DATA_CACHE_SUPPORT)

#define LWM2M_LIMITED_TIMESERIES_RESOURCE_COUNT 20

struct lwm2m_cache_read_entry {
	struct lwm2m_time_series_resource *cache_data;
	int32_t original_get_head;
	int32_t original_get_tail;
	int32_t original_get_base;
};

struct lwm2m_cache_read_info {
	struct lwm2m_cache_read_entry read_info[CONFIG_LWM2M_MAX_CACHED_RESOURCES];
	int entry_limit;
	int entry_size;
};
#endif

struct lwm2m_time_series_resource *
lwm2m_cache_entry_get_by_object(const struct lwm2m_obj_path *obj_path);
bool lwm2m_cache_write(struct lwm2m_time_series_resource *cache_entry,
		       struct lwm2m_time_series_elem *buf);
bool lwm2m_cache_read(struct lwm2m_time_series_resource *cache_entry,
		      struct lwm2m_time_series_elem *buf);
size_t lwm2m_cache_size(const struct lwm2m_time_series_resource *cache_entry);

#endif /* LWM2M_REGISTRY_H */
