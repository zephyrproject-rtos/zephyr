/*
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_LWM2M_PATH_H_
#define ZEPHYR_INCLUDE_NET_LWM2M_PATH_H_

/**
 * @brief Generate LwM2M string paths using numeric components.
 *
 * Accepts at least one and up to four arguments. Each argument will be
 * stringified by the pre-processor, so calling this with non-literals will
 * likely not do what you want. For example,
 *
 *      #define MY_OBJ_ID 3
 *      LWM2M_PATH(MY_OBJ_ID, 0, 1)
 *
 * would evaluate to "3/0/1", while
 *
 *   int x = 3;
 *   LWM2M_PATH(x, 0, 1)
 *
 * evaluates to "x/0/1".
 */
#define LWM2M_PATH(...) \
	LWM2M_PATH_MACRO(__VA_ARGS__, LWM2M_PATH4, LWM2M_PATH3,	\
			 LWM2M_PATH2, LWM2M_PATH1)(__VA_ARGS__)


/* Internal helper macros for the LWM2M_PATH macro */
#define LWM2M_PATH_VA_NUM_ARGS(...) \
	LWM2M_PATH_VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)
#define LWM2M_PATH_VA_NUM_ARGS_IMPL(_1, _2, _3, _4, N, ...) N

#define LWM2M_PATH1(_x) #_x
#define LWM2M_PATH2(_x, _y) #_x "/" #_y
#define LWM2M_PATH3(_x, _y, _z) #_x "/" #_y "/" #_z
#define LWM2M_PATH4(_a, _x, _y, _z) #_a "/" #_x "/" #_y "/" #_z

#define LWM2M_PATH_MACRO(_1, _2, _3, _4, NAME, ...) NAME



#endif /* ZEPHYR_INCLUDE_NET_LWM2M_PATH_H_ */
