/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Buffers for USB device support
 */

#ifndef ZEPHYR_INCLUDE_USB_BUF_H
#define ZEPHYR_INCLUDE_USB_BUF_H

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/**
 * @brief USB buffer macros and definitions
 * @defgroup usb_buf USB buffer macros and definitions
 * @ingroup usb
 * @since 4.0
 * @version 0.1.1
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 */
#if defined(CONFIG_UDC_BUF_FORCE_NOCACHE) || defined(CONFIG_UHC_BUF_FORCE_NOCACHE)
#define USB_BUF_FORCE_NOCACHE		1
#else
#define USB_BUF_FORCE_NOCACHE		0
#endif

#if defined(CONFIG_DCACHE) && !USB_BUF_FORCE_NOCACHE
/*
 * Here we try to get DMA-safe buffers, but we lack a consistent source of
 * information about data cache properties, such as line cache size, and a
 * consistent source of information about what part of memory is DMA'able.
 * For now, we simply assume that all available memory is DMA'able and use
 * Kconfig option DCACHE_LINE_SIZE for alignment and granularity.
 */
#define USB_PRIV_BUFALIGN		CONFIG_DCACHE_LINE_SIZE
#define USB_PRIV_BUFGRANULARITY		CONFIG_DCACHE_LINE_SIZE
#else
/*
 * Default alignment and granularity to pointer size if the platform does not
 * have a data cache or buffers are placed in nocache memory region.
 */
#define USB_PRIV_BUFALIGN		sizeof(void *)
#define USB_PRIV_BUFGRANULARITY		sizeof(void *)
#endif


#if USB_BUF_FORCE_NOCACHE
/*
 * The usb transfer buffer needs to be in __nocache section
 */
#define USB_PRIV_BUFSECTION	__nocache
#else
#define USB_PRIV_BUFSECTION
#endif
/** @endcond */

/** Buffer alignment required by the UDC driver */
#define USB_BUF_ALIGN		USB_PRIV_BUFALIGN

/** Buffer granularity required by the UDC driver */
#define USB_BUF_GRANULARITY	USB_PRIV_BUFGRANULARITY

/** Round up to the granularity required by the UDC driver */
#define USB_BUF_ROUND_UP(size)	ROUND_UP(size, USB_PRIV_BUFGRANULARITY)

/**
 * @brief Define a UDC driver-compliant static buffer
 *
 * This macro should be used if the application defines its own buffers to be
 * used for USB transfers.
 *
 * @param name Buffer name
 * @param size Buffer size
 */
#define USB_STATIC_BUF_DEFINE(name, size)					\
	static uint8_t USB_PRIV_BUFSECTION __aligned(USB_BUF_ALIGN)		\
	name[USB_BUF_ROUND_UP(size)];

/**
 * @brief Verify that the buffer is aligned as required by the UDC driver
 *
 * @see IS_ALIGNED
 *
 * @param buf Buffer pointer
 */
#define IS_USB_BUF_ALIGNED(buf) IS_ALIGNED(buf, USB_BUF_ALIGN)

/**
 * @cond INTERNAL_HIDDEN
 */
#define USB_BUF_HEAP_DEFINE(name, bytes, in_section)				\
	uint8_t in_section __aligned(USB_BUF_ALIGN)				\
		kheap_##name[MAX(bytes, Z_HEAP_MIN_SIZE)];			\
	STRUCT_SECTION_ITERABLE(k_heap, name) = {				\
		.heap = {							\
			.init_mem = kheap_##name,				\
			.init_bytes = MAX(bytes, Z_HEAP_MIN_SIZE),		\
		 },								\
	}

#define USB_BUF_K_HEAP_DEFINE(name, size)					\
	COND_CODE_1(USB_BUF_FORCE_NOCACHE,					\
		    (USB_BUF_HEAP_DEFINE(name, size, __nocache)),		\
		    (USB_BUF_HEAP_DEFINE(name, size, __noinit)))

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
#define USB_BUF_POOL_VAR_DEFINE(pname, count, size, ud_size, fdestroy)		\
	_NET_BUF_ARRAY_DEFINE(pname, count, ud_size);				\
	USB_BUF_K_HEAP_DEFINE(net_buf_mem_pool_##pname, size);			\
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
#define USB_BUF_POOL_DEFINE(pname, count, size, ud_size, fdestroy)		\
	_NET_BUF_ARRAY_DEFINE(pname, count, ud_size);				\
	BUILD_ASSERT((USB_BUF_GRANULARITY) % (USB_BUF_ALIGN) == 0,		\
		     "Code assumes granurality is multiple of alignment");	\
	static uint8_t USB_PRIV_BUFSECTION __aligned(USB_BUF_ALIGN)		\
		net_buf_data_##pname[count][USB_BUF_ROUND_UP(size)];		\
	static const struct net_buf_pool_fixed net_buf_fixed_##pname = {	\
		.data_pool = (uint8_t *)net_buf_data_##pname,			\
	};									\
	static const struct net_buf_data_alloc net_buf_fixed_alloc_##pname = {	\
		.cb = &net_buf_fixed_cb,					\
		.alloc_data = (void *)&net_buf_fixed_##pname,			\
		.max_alloc_size = USB_BUF_ROUND_UP(size),			\
	};									\
	static STRUCT_SECTION_ITERABLE(net_buf_pool, pname) =			\
		NET_BUF_POOL_INITIALIZER(pname, &net_buf_fixed_alloc_##pname,	\
					 _net_buf_##pname, count, ud_size,	\
					 fdestroy)

/**
 * @brief Buffer alignment required by the UDC driver
 * @see USB_BUF_ALIGN
 */
#define UDC_BUF_ALIGN		USB_BUF_ALIGN

/**
 * @brief Buffer granularity required by the UDC driver
 * @see USB_BUF_GRANULARITY
 */
#define UDC_BUF_GRANULARITY	USB_BUF_GRANULARITY

/**
 * @brief Round up to the granularity required by the UDC driver
 * @see USB_BUF_ROUND_UP
 */
#define UDC_ROUND_UP(size)	USB_BUF_ROUND_UP(size)

/**
 * @brief Define a UDC driver-compliant static buffer
 * @see USB_STATIC_BUF_DEFINE
 */
#define UDC_STATIC_BUF_DEFINE(name, size) USB_STATIC_BUF_DEFINE(name, size)

/**
 * @brief Verify that the buffer is aligned as required by the UDC driver
 * @see IS_UDC_ALIGNED
 */
#define IS_UDC_ALIGNED(buf)	IS_USB_BUF_ALIGNED(buf)

/**
 * @brief Define a new pool for UDC buffers based on fixed-size data
 * @see USB_BUF_POOL_VAR_DEFINE
 */
#define UDC_BUF_POOL_VAR_DEFINE(pname, count, size, ud_size, fdestroy)		\
	USB_BUF_POOL_VAR_DEFINE(pname, count, size, ud_size, fdestroy)

/**
 * @brief Define a new pool for UDC buffers based on fixed-size data
 * @see USB_BUF_POOL_DEFINE
 */
#define UDC_BUF_POOL_DEFINE(pname, count, size, ud_size, fdestroy)		\
	USB_BUF_POOL_DEFINE(pname, count, size, ud_size, fdestroy)
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_UDC_BUF_H */
