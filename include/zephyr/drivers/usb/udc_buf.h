/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Buffers for USB device support
 */

#ifndef ZEPHYR_INCLUDE_UDC_BUF_H
#define ZEPHYR_INCLUDE_UDC_BUF_H

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#if defined(CONFIG_DCACHE) && !defined(CONFIG_UDC_BUF_FORCE_NOCACHE)
/*
 * Here we try to get DMA-safe buffers, but we lack a consistent source of
 * information about data cache properties, such as line cache size, and a
 * consistent source of information about what part of memory is DMA'able.
 * For now, we simply assume that all available memory is DMA'able and use
 * Kconfig option DCACHE_LINE_SIZE for alignment and granularity.
 */
#define Z_UDC_BUF_ALIGN		CONFIG_DCACHE_LINE_SIZE
#define Z_UDC_BUF_GRANULARITY	CONFIG_DCACHE_LINE_SIZE
#else
/*
 * Default alignment and granularity to pointer size if the platform does not
 * have a data cache or buffers are placed in nocache memory region.
 */
#define Z_UDC_BUF_ALIGN		sizeof(void *)
#define Z_UDC_BUF_GRANULARITY	sizeof(void *)
#endif


#if defined(CONFIG_UDC_BUF_FORCE_NOCACHE)
/*
 * The usb transfer buffer needs to be in __nocache section
 */
#define Z_UDC_BUF_SECTION	__nocache
#else
#define Z_UDC_BUF_SECTION
#endif

/**
 * @brief Buffer macros and definitions used in USB device support
 * @defgroup udc_buf Buffer macros and definitions used in USB device support
 * @ingroup usb
 * @since 4.0
 * @version 0.1.0
 * @{
 */

/** Buffer alignment required by the UDC driver */
#define UDC_BUF_ALIGN		Z_UDC_BUF_ALIGN

/** Buffer granularity required by the UDC driver */
#define UDC_BUF_GRANULARITY	Z_UDC_BUF_GRANULARITY

/**
 * @brief Define a UDC driver-compliant static buffer
 *
 * This macro should be used if the application defines its own buffers to be
 * used for USB transfers.
 *
 * @param name Buffer name
 * @param size Buffer size
 */
#define UDC_STATIC_BUF_DEFINE(name, size)					\
	static uint8_t Z_UDC_BUF_SECTION __aligned(UDC_BUF_ALIGN)		\
	name[ROUND_UP(size, UDC_BUF_GRANULARITY)];

/**
 * @brief Verify that the buffer is aligned as required by the UDC driver
 *
 * @see IS_ALIGNED
 *
 * @param buf Buffer pointer
 */
#define IS_UDC_ALIGNED(buf) IS_ALIGNED(buf, UDC_BUF_ALIGN)

/**
 * @cond INTERNAL_HIDDEN
 */
#define UDC_HEAP_DEFINE(name, bytes, in_section)				\
	uint8_t in_section __aligned(UDC_BUF_ALIGN)				\
		kheap_##name[MAX(bytes, Z_HEAP_MIN_SIZE)];			\
	STRUCT_SECTION_ITERABLE(k_heap, name) = {				\
		.heap = {							\
			.init_mem = kheap_##name,				\
			.init_bytes = MAX(bytes, Z_HEAP_MIN_SIZE),		\
		 },								\
	}

#define UDC_K_HEAP_DEFINE(name, size)						\
	COND_CODE_1(CONFIG_UDC_BUF_FORCE_NOCACHE,				\
		    (UDC_HEAP_DEFINE(name, size, __nocache)),			\
		    (UDC_HEAP_DEFINE(name, size, __noinit)))

extern const struct net_buf_data_cb net_buf_dma_cb;
/** @endcond */

/**
 * @brief Define a new pool for UDC buffers with variable-size payloads
 *
 * This macro is similar to `NET_BUF_POOL_VAR_DEFINE`, but provides buffers
 * with alignment and granularity suitable for use by UDC driver.
 *
 * @see NET_BUF_POOL_VAR_DEFINE
 *
 * @param pname      Name of the pool variable.
 * @param count      Number of buffers in the pool.
 * @param size       Maximum data payload per buffer.
 * @param ud_size    User data space to reserve per buffer.
 * @param fdestroy   Optional destroy callback when buffer is freed.
 */
#define UDC_BUF_POOL_VAR_DEFINE(pname, count, size, ud_size, fdestroy)		\
	_NET_BUF_ARRAY_DEFINE(pname, count, ud_size);				\
	UDC_K_HEAP_DEFINE(net_buf_mem_pool_##pname, size);			\
	static const struct net_buf_data_alloc net_buf_data_alloc_##pname = {	\
		.cb = &net_buf_dma_cb,						\
		.alloc_data = &net_buf_mem_pool_##pname,			\
		.max_alloc_size = 0,						\
	};									\
	static STRUCT_SECTION_ITERABLE(net_buf_pool, pname) =			\
		NET_BUF_POOL_INITIALIZER(pname, &net_buf_data_alloc_##pname,	\
					 _net_buf_##pname, count, ud_size,	\
					 fdestroy)

/**
 * @brief Define a new pool for UDC buffers based on fixed-size data
 *
 * This macro is similar to `NET_BUF_POOL_DEFINE`, but provides buffers
 * with alignment and granularity suitable for use by UDC driver.
 *
 * @see NET_BUF_POOL_DEFINE

 * @param pname      Name of the pool variable.
 * @param count      Number of buffers in the pool.
 * @param size       Maximum data payload per buffer.
 * @param ud_size    User data space to reserve per buffer.
 * @param fdestroy   Optional destroy callback when buffer is freed.
 */
#define UDC_BUF_POOL_DEFINE(pname, count, size, ud_size, fdestroy)		\
	_NET_BUF_ARRAY_DEFINE(pname, count, ud_size);				\
	BUILD_ASSERT((UDC_BUF_GRANULARITY) % (UDC_BUF_ALIGN) == 0,		\
		     "Code assumes granurality is multiple of alignment");	\
	static uint8_t Z_UDC_BUF_SECTION __aligned(UDC_BUF_ALIGN)		\
		net_buf_data_##pname[count][ROUND_UP(size, UDC_BUF_GRANULARITY)];\
	static const struct net_buf_pool_fixed net_buf_fixed_##pname = {	\
		.data_pool = (uint8_t *)net_buf_data_##pname,			\
	};									\
	static const struct net_buf_data_alloc net_buf_fixed_alloc_##pname = {	\
		.cb = &net_buf_fixed_cb,					\
		.alloc_data = (void *)&net_buf_fixed_##pname,			\
		.max_alloc_size = ROUND_UP(size, UDC_BUF_GRANULARITY),		\
	};									\
	static STRUCT_SECTION_ITERABLE(net_buf_pool, pname) =			\
		NET_BUF_POOL_INITIALIZER(pname, &net_buf_fixed_alloc_##pname,	\
					 _net_buf_##pname, count, ud_size,	\
					 fdestroy)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_UDC_BUF_H */
