/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	sys.h
 * @brief	System primitives for libmetal.
 * @brief	Top level include internal to libmetal library code.
 */

#ifndef __METAL_SYS__H__
#define __METAL_SYS__H__

#include <stdlib.h>

#include <metal/log.h>
#include <metal/list.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup system Top Level Interfaces
 *  @{ */

/** Physical address type. */
typedef unsigned long metal_phys_addr_t;

/** Interrupt request number. */
typedef int metal_irq_t;

/** Bad offset into shared memory or I/O region. */
#define METAL_BAD_OFFSET	((unsigned long)-1)

/** Bad physical address value. */
#define METAL_BAD_PHYS		((metal_phys_addr_t)-1)

/** Bad virtual address value. */
#define METAL_BAD_VA		((void *)-1)

/** Bad IRQ. */
#define METAL_BAD_IRQ		((metal_irq_t)-1)

/**
 * Initialization configuration for libmetal.
 */
struct metal_init_params {

	/** log message handler (defaults to stderr). */
	metal_log_handler		log_handler;

	/** default log message level (defaults to emergency). */
	enum metal_log_level		log_level;
};

/**
 * System independent runtime state for libmetal.  This is part of a system
 * specific singleton data structure (@see _metal).
 */
struct metal_common_state {
	/** Current log level. */
	enum metal_log_level		log_level;

	/** Current log handler (null for none). */
	metal_log_handler		log_handler;

	/** List of registered buses. */
	struct metal_list		bus_list;

	/** Generic statically defined shared memory segments. */
	struct metal_list		generic_shmem_list;

	/** Generic statically defined devices. */
	struct metal_list		generic_device_list;
};

struct metal_state;

#include <metal/system/@PROJECT_SYSTEM@/sys.h>

#ifndef METAL_INIT_DEFAULTS
#define METAL_INIT_DEFAULTS				\
{							\
	.log_handler	= metal_default_log_handler,	\
	.log_level	= METAL_LOG_INFO,		\
}
#endif

/** System specific runtime data. */
extern struct metal_state _metal;

/**
 * @brief	Initialize libmetal.
 *
 * Initialize the libmetal library.
 *
 * @param[in]	params	Initialization params (@see metal_init_params).
 *
 * @return	0 on success, or -errno on failure.
 *
 * @see metal_finish
 */
extern int metal_init(const struct metal_init_params *params);

/**
 * @brief	Shutdown libmetal.
 *
 * Shutdown the libmetal library, and release all reserved resources.
 *
 * @see metal_init
 */
extern void metal_finish(void);

#ifdef METAL_INTERNAL

/**
 * @brief	libmetal system initialization.
 *
 * This function initializes libmetal on Linux or Generic platforms.  This
 * involves obtaining necessary pieces of system information (sysfs mount path,
 * page size, etc.).
 *
 * @param[in]	params	Initialization parameters (@see metal_init_params).
 * @return	0 on success, or -errno on failure.
 */
extern int metal_sys_init(const struct metal_init_params *params);

/**
 * @brief	libmetal system shutdown.
 *
 * This function shuts down and releases resources held by libmetal Linux or
 * Generic platform layers.
 *
 * @see metal_sys_init
 */
extern void metal_sys_finish(void);

#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_SYS__H__ */
