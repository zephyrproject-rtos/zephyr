/**
 * @file
 * @brief File for easily accessing DT Romstart options
 */

/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Siemens Mobility GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_ROMSTART_H_
#define ZEPHYR_INCLUDE_DEVICETREE_ROMSTART_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-chosen-romstart Devicetree ROMSTART API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Check whether the chosen zephyr,romstart property exists
 *
 * @return 1, if a zephyr,romstart property exists
 */
#define DT_CHOSEN_ROMSTART_EXISTS DT_NODE_EXISTS(DT_CHOSEN(zephyr_romstart))

/**
 * @brief Get memory region name of chosen zephyr,romstart property
 *
 * @return Memory region name of chosen zephyr,romstart property, if it exists
 */
#define DT_ROMSTART_REGION DT_PROP(DT_CHOSEN(zephyr_romstart), zephyr_memory_region)

/**
 * @}
 */

/* This can't be a BUILD_ASSERT since linker doesn't support static asserts */
#if DT_CHOSEN_ROMSTART_EXISTS && !DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_romstart), zephyr_memory_region)
#error "The chosen romstart property references a node that doesn't has a zephyr,memory-region" \
	"attribute to create a new linker script memory region"
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_ROMSTART_H_ */
