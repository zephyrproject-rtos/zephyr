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
 * @{
 */

/*
 * This file is currently deliberately not defining macros for some
 * existing zephyr,foo chosen nodes, such as zephyr,sram, until there
 * are users for them. Feel free to extend it as needed.
 *
 * Getting doxygen to play along with all the dts-specific ifdeffery
 * proved too complex for DT_CHOSEN_ZEPHYR_FLASH_LABEL, so we document
 * everything under a DT_DOXYGEN define.
 */

#ifdef DT_DOXYGEN
/**
 * @def DT_CHOSEN_ZEPHYR_FLASH_LABEL
 *
 * @brief The value of DT_FLASH_DEV_NAME if that is defined by a dts_fixup.h
 *        file. Otherwise, if there is a chosen zephyr,flash node which has
 *        a label property, that property's value. Undefined otherwise.
 */
#define DT_CHOSEN_ZEPHYR_FLASH_LABEL ""
#endif /* DT_DOXYGEN */

#if defined(DT_FLASH_DEV_NAME)
#define DT_CHOSEN_ZEPHYR_FLASH_LABEL DT_FLASH_DEV_NAME

#elif DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_flash), label)
#define DT_CHOSEN_ZEPHYR_FLASH_LABEL DT_LABEL(DT_CHOSEN(zephyr_flash))
#endif

/**
 * @}
 */

#endif
