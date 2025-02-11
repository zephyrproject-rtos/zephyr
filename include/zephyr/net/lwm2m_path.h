/*
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_LWM2M_PATH_H_
#define ZEPHYR_INCLUDE_NET_LWM2M_PATH_H_

/**
 * @file lwm2m.h
 *
 * @brief LwM2M path helper macros
 *
 * @defgroup lwm2m_path_helpers LwM2M path helper macros
 * @ingroup lwm2m_api
 * @{
 */

/**
 * @brief Generate LwM2M string paths using numeric components.
 *
 * Accepts at least one and up to four arguments. Each argument will be
 * stringified by the pre-processor, so calling this with non-literals will
 * likely not do what you want.
 *
 * For example:
 *
 * @code{c}
 * #define MY_OBJ_ID 3
 * LWM2M_PATH(MY_OBJ_ID, 0, 1)
 * @endcode
 *
 * would evaluate to "3/0/1", while
 *
 * @code{c}
 * int x = 3;
 * LWM2M_PATH(x, 0, 1)
 * @endcode
 *
 * evaluates to "x/0/1".
 */
#define LWM2M_PATH(...) \
	LWM2M_PATH_MACRO(__VA_ARGS__, LWM2M_PATH4, LWM2M_PATH3,	\
			 LWM2M_PATH2, LWM2M_PATH1)(__VA_ARGS__)


/** @cond INTERNAL_HIDDEN */
/* Internal helper macros for the LWM2M_PATH macro */
#define LWM2M_PATH_VA_NUM_ARGS(...) \
	LWM2M_PATH_VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)
#define LWM2M_PATH_VA_NUM_ARGS_IMPL(_1, _2, _3, _4, N, ...) N

#define LWM2M_PATH1(_x) #_x
#define LWM2M_PATH2(_x, _y) #_x "/" #_y
#define LWM2M_PATH3(_x, _y, _z) #_x "/" #_y "/" #_z
#define LWM2M_PATH4(_a, _x, _y, _z) #_a "/" #_x "/" #_y "/" #_z

#define LWM2M_PATH_MACRO(_1, _2, _3, _4, NAME, ...) NAME
/** @endcond */

/**
 * @brief Initialize LwM2M object structure
 *
 * Accepts at least one and up to four arguments. Fill up @ref lwm2m_obj_path structure
 * and sets the level.
 *
 * For example:
 *
 * @code{c}
 * struct lwm2m_obj_path p = LWM2M_OBJ(MY_OBJ, 0, RESOURCE);
 * @endcode
 *
 * Can also be used in place of function argument to return the structure allocated from stack
 *
 * @code{c}
 * lwm2m_notify_observer_path(&LWM2M_OBJ(MY_OBJ, inst_id, RESOURCE));
 * @endcode
 *
 */
#define LWM2M_OBJ(...) \
	GET_OBJ_MACRO(__VA_ARGS__, LWM2M_OBJ4, LWM2M_OBJ3, LWM2M_OBJ2, LWM2M_OBJ1)(__VA_ARGS__)

/** @cond INTERNAL_HIDDEN */
/* Internal helper macros for the LWM2M_OBJ macro */
#define GET_OBJ_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define LWM2M_OBJ1(oi) (struct lwm2m_obj_path) {.obj_id = oi, .level = 1}
#define LWM2M_OBJ2(oi, oii) (struct lwm2m_obj_path) {.obj_id = oi, .obj_inst_id = oii, .level = 2}
#define LWM2M_OBJ3(oi, oii, ri) (struct lwm2m_obj_path) \
	{.obj_id = oi, .obj_inst_id = oii, .res_id = ri, .level = 3}
#define LWM2M_OBJ4(oi, oii, ri, rii) (struct lwm2m_obj_path) \
	{.obj_id = oi, .obj_inst_id = oii, .res_id = ri, .res_inst_id = rii, .level = 4}
/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_NET_LWM2M_PATH_H_ */
