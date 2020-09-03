/*
 * Copyright (c) 2020, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEPRECATED_H_
#define ZEPHYR_INCLUDE_DEPRECATED_H_
/*
 * Add Kconfig items that are marked as deprecated the list below to generate
 * a build warning to the user. When the deprecation process is completed and
 * the feature/config is removed from the code base, the warning below can be
 * removed as well.
 */

#ifdef CONFIG_NET_TCP1
/*
 * Please move your applications to NET_TCP2 (already the default) as NET_TCP1
 * will be removed from the code base on release x.x
 */
#warning Legacy TCP stack is being deprecated in favor of NET_TCP2
#endif

#endif /* ZEPHYR_INCLUDE_DEPRECATED_H_ */
