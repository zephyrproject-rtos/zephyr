/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_MGMT_HANDLERS_
#define H_MCUMGR_MGMT_HANDLERS_

#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief MCUmgr handler registration API
 * @defgroup mcumgr_handler_api Handlers
 * @ingroup mcumgr
 * @{
 */

/** Type definition for a MCUmgr handler initialisation function */
typedef void (*mcumgr_handler_init_t)(void);

/** @cond INTERNAL_HIDDEN */
struct mcumgr_handler {
	/** Initialisation function to be called */
	const mcumgr_handler_init_t init;
};
/** @endcond  */

/**
 * @brief Define a MCUmgr handler to register.
 *
 * This adds a new entry to the iterable section linker list of MCUmgr handers.
 *
 * @param name	Name of the MCUmgr handler to registger.
 * @param _init	Init function to be called (mcumgr_handler_init_t).
 */
#define MCUMGR_HANDLER_DEFINE(name, _init)			\
	STRUCT_SECTION_ITERABLE(mcumgr_handler, name) = {	\
		.init = _init,					\
	}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* H_MCUMGR_MGMT_HANDLERS_ */
