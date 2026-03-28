/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LWM2M_OBJ_ACCESS_CONTROL_H
#define LWM2M_OBJ_ACCESS_CONTROL_H
#include "lwm2m_engine.h"
#include "lwm2m_object.h"

/**
 * @brief Main access control logic. Checks if the server with instance id @p server_obj_inst are
 * allowed to do @p operation on the object instance of object id @p obj_id
 * and object instance id @p obj_inst_id. If access control is enabled, this should
 * be called before every operation to test access.
 *
 * @param obj_id object id of the object instance having its rights checked.
 * @param obj_inst_id object instance id of the object instance having its rights checked.
 * @param server_obj_inst object instance id of the server attempting to do the operation.
 * @param operation lwm2m operation / permission (like LWM2M_OP_READ)
 * @param bootstrap_mode 1/0. Bootstrap servers should have complete access during bootstrap.
 * @return int to signal access:
 *		 0		- server has access
 *		-EACCES	- unauthorized
 *		-EPERM	- method not allowed
 */
int access_control_check_access(uint16_t obj_id, uint16_t obj_inst_id, uint16_t server_obj_inst,
				uint16_t operation, bool bootstrap_mode);

/**
 * @brief Creates an access control object instance. Should be called every
 * time an object instance is created.
 *
 * @param obj_id object id of the object instance getting an access control.
 * @param obj_inst_id object instance id of the object instance getting access control.
 * @param server_obj_inst_id object instance id of the server creating the object instance.
 */
void access_control_add(uint16_t obj_id, uint16_t obj_inst_id, int server_obj_inst_id);

/**
 * @brief Creates an access control object instance for objects. Should be called if servers should
 * have access to create object instances of object id @p obj_id.
 *
 * @param obj_id object id of the object getting access control.
 * @param server_obj_inst_id object instance id of the server creating the access control
 * object instance.
 */
void access_control_add_obj(uint16_t obj_id, int server_obj_inst_id);

/**
 * @brief Removes the access control instance that contains the access rights concerning
 * the object instance of object id @p obj_id and object instance id @p obj_inst_id.
 * Does nothing if obj_id == 2 (i.e. object id for access control). Should be called
 * automatically any time an object instance is unregistered/deleted.
 *
 * @param obj_id object id of the object instance getting removed.
 * @param obj_inst_id object instance id of the object instance getting removed.
 */
void access_control_remove(uint16_t obj_id, uint16_t obj_inst_id);

/**
 * @brief Removes the access control instance that contains the access rights concerning
 * the object with object id @p obj_id.
 *
 * @param obj_id object id of the object getting removed.
 */
void access_control_remove_obj(uint16_t obj_id);

#endif
