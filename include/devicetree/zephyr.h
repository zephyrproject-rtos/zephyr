/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr-specific devicetree /chosen properties
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_ZEPHYR_H_
#define ZEPHYR_INCLUDE_DEVICETREE_ZEPHYR_H_

/**
 * @defgroup devicetree-zephyr Zephyr's /chosen nodes
 * @ingroup devicetree
 * @{
 */

/*
 * This file is currently deliberately not defining macros for some
 * existing zephyr,foo chosen nodes, such as zephyr,sram, until there
 * are users for them. Feel free to extend it as needed.
 *
 * Getting doxygen to play along with all the dts-specific ifdeffery
 * proved too complex for DT_CHOSEN_ZEPHYR_ENTROPY_LABEL, so we document
 * everything under a DT_DOXYGEN define.
 */

#ifdef DT_DOXYGEN
/**
 * @def DT_CHOSEN_ZEPHYR_ENTROPY_LABEL
 *
 * @brief If there is a chosen node zephyr,entropy property which has
 *        a label property, that property's value. Undefined otherwise.
 */
#define DT_CHOSEN_ZEPHYR_ENTROPY_LABEL ""

/**
 * @def DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL
 *
 * @brief If there is a chosen node zephyr,flash-controller property which has
 *        a label property, that property's value. Undefined otherwise.
 */
#define DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL ""

/**
 * @def DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL
 *
 * @brief If there is a chosen node zephyr,can-primary property which has
 *        a label property, that property's value. Undefined otherwise.
 */
#define DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL ""

#endif /* DT_DOXYGEN */

#if DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_entropy), label)
#define DT_CHOSEN_ZEPHYR_ENTROPY_LABEL DT_LABEL(DT_CHOSEN(zephyr_entropy))
#endif

#if DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_flash_controller), label)
#define DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL \
	DT_LABEL(DT_CHOSEN(zephyr_flash_controller))
#endif

#if DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_can_primary), label)
#define DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL \
	DT_LABEL(DT_CHOSEN(zephyr_can_primary))
#endif
/**
 * @}
 */

#endif
