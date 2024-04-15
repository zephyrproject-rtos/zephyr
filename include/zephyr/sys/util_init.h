/*
 * Copyright (c) 2024, Tomasz Bursztyka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_SYS_UTIL_INIT_H_
#define ZEPHYR_INCLUDE_ZEPHYR_SYS_UTIL_INIT_H_

#include <zephyr/devicetree.h>

#include <zinit.h>

#ifdef __cplusplus
extern "C" {
#endif
/** @{ */

#define ZINIT_EXISTS(init_id)                                                   \
	IS_ENABLED(DT_CAT3(ZINIT_, init_id, _EXISTS))

#define ZINIT_EXPLICIT_LEVEL_EXISTS(init_id)                                    \
	IS_ENABLED(DT_CAT3(ZINIT_, init_id, _INIT_EXPLICIT_LEVEL_EXISTS))

#define ZINIT_EXPLICIT_LEVEL(init_id)                                           \
	DT_CAT3(ZINIT_, init_id, _INIT_EXPLICIT_LEVEL)

#define ZINIT_IMPLICIT_LEVEL_EXISTS(init_id)                                    \
	IS_ENABLED(DT_CAT3(ZINIT_, init_id, _INIT_IMPLICIT_LEVEL_EXISTS))

#define ZINIT_IMPLICIT_LEVEL(init_id)                                           \
	DT_CAT3(ZINIT_, init_id, _INIT_IMPLICIT_LEVEL)

#define ZINIT_PRIORITY(init_id)                                                 \
	DT_CAT3(ZINIT_, init_id, _INIT_PRIORITY)

#define ZINIT_GET_PRIORITY(node_id, dev_id)                                     \
	COND_CODE_1(ZINIT_EXISTS(node_id),                                      \
		    (ZINIT_PRIORITY(node_id)),                                  \
	 (COND_CODE_1(ZINIT_EXISTS(dev_id),                                     \
		      (ZINIT_PRIORITY(dev_id)),                                 \
		      (0))))

/* Helper definition to evaluate level predominance */
#define ZINIT_LEVEL_PREDOMINANCE_PRE_KERNEL_1_VS_EARLY        1
#define ZINIT_LEVEL_PREDOMINANCE_PRE_KERNEL_2_VS_EARLY        1
#define ZINIT_LEVEL_PREDOMINANCE_POST_KERNEL_VS_EARLY         1
#define ZINIT_LEVEL_PREDOMINANCE_APPLICATION_VS_EARLY         1
#define ZINIT_LEVEL_PREDOMINANCE_SMP_VS_EARLY                 1
#define ZINIT_LEVEL_PREDOMINANCE_MANUAL_VS_EARLY              1

#define ZINIT_LEVEL_PREDOMINANCE_PRE_KERNEL_2_VS_PRE_KERNEL_1 1
#define ZINIT_LEVEL_PREDOMINANCE_POST_KERNEL_VS_PRE_KERNEL_1  1
#define ZINIT_LEVEL_PREDOMINANCE_APPLICATION_VS_PRE_KERNEL_1  1
#define ZINIT_LEVEL_PREDOMINANCE_SMP_VS_PRE_KERNEL_1          1
#define ZINIT_LEVEL_PREDOMINANCE_MANUAL_VS_PRE_KERNEL_1       1

#define ZINIT_LEVEL_PREDOMINANCE_POST_KERNEL_VS_PRE_KERNEL_2  1
#define ZINIT_LEVEL_PREDOMINANCE_APPLICATION_VS_PRE_KERNEL_2  1
#define ZINIT_LEVEL_PREDOMINANCE_SMP_VS_PRE_KERNEL_2          1
#define ZINIT_LEVEL_PREDOMINANCE_MANUAL_VS_PRE_KERNEL_2       1

#define ZINIT_LEVEL_PREDOMINANCE_APPLICATION_VS_POST_KERNEL   1
#define ZINIT_LEVEL_PREDOMINANCE_SMP_VS_POST_KERNEL           1
#define ZINIT_LEVEL_PREDOMINANCE_MANUAL_VS_POST_KERNEL        1

#define ZINIT_LEVEL_PREDOMINANCE_SMP_VS_APPLICATION           1
#define ZINIT_LEVEL_PREDOMINANCE_MANUAL_VS_APPLICATION        1

#define ZINIT_LEVEL_PREDOMINANCE_MANUAL_VS_SMP                1

#define ZINIT_LEVEL_PREDOMINANCE(default_level, level)                            \
	COND_CODE_1(_CONCAT_4(ZINIT_LEVEL_PREDOMINANCE_, default_level, _, level),\
		    (default_level), (level))

#define ZINIT_GET_LEVEL(node_id, level)                                           \
	COND_CODE_1(ZINIT_EXISTS(node_id),                                        \
	 (COND_CODE_1(ZINIT_EXPLICIT_LEVEL_EXISTS(node_id),                       \
		     (ZINIT_EXPLICIT_LEVEL(node_id)),                             \
	 (COND_CODE_1(ZINIT_IMPLICIT_LEVEL_EXISTS(node_id),                       \
		      (ZINIT_LEVEL_PREDOMINANCE(level,                            \
						 ZINIT_IMPLICIT_LEVEL(node_id))), \
		      (level))))),                                                \
	 (level))

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_SYS_UTIL_INIT_H_ */
