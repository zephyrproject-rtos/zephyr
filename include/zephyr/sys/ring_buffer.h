/*
 * Copyright (c) 2026 Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Ring buffer API selector header.
 * @ingroup ring_buffer_apis
 *
 * @defgroup ring_buffer_apis Ring Buffer APIs
 * @since 1.0
 * @version 1.0.0
 * @ingroup datastructure_apis
 *
 */

#ifdef CONFIG_RING_BUFFER
#include <zephyr/sys/internal/ring_buffer_claim.h>
#else
#include <zephyr/sys/internal/ring_buffer_slim.h>
/** @cond INTERNAL_HIDDEN */
#define Z_RING_BUF_CLAIM_REMOVED(name)						\
	({									\
		BUILD_ASSERT(0, #name "() is deprecated and not available "	\
			"when CONFIG_RING_BUFFER=n. Enable "			\
			"CONFIG_RING_BUFFER to keep it during the "		\
			"deprecation period, or migrate as described in the "	\
			"Zephyr 4.5 migration guide.");				\
		0;								\
	})

#define Z_RING_BUF_DECLARE_REMOVED(name)					\
	BUILD_ASSERT(0, #name " is deprecated and not available when "		\
		"CONFIG_RING_BUFFER=n. Enable CONFIG_RING_BUFFER "		\
		"to keep it during the deprecation period, or migrate as "	\
		"described in the Zephyr 4.5 migration guide.")

#define ring_buf_put_claim(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_put_claim)
#define ring_buf_put_finish(...)	Z_RING_BUF_CLAIM_REMOVED(ring_buf_put_finish)
#define ring_buf_get_claim(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_get_claim)
#define ring_buf_get_finish(...)	Z_RING_BUF_CLAIM_REMOVED(ring_buf_get_finish)
#define ring_buf_item_init(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_item_init)
#define ring_buf_item_put(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_item_put)
#define ring_buf_item_get(...)		Z_RING_BUF_CLAIM_REMOVED(ring_buf_item_get)
#define ring_buf_item_space_get(...)	Z_RING_BUF_CLAIM_REMOVED(ring_buf_item_space_get)
#define ring_buf_internal_reset(...)	Z_RING_BUF_CLAIM_REMOVED(ring_buf_internal_reset)

#define RING_BUF_ITEM_DECLARE(...)	Z_RING_BUF_DECLARE_REMOVED(RING_BUF_ITEM_DECLARE)
#define RING_BUF_ITEM_DECLARE_SIZE(...)	Z_RING_BUF_DECLARE_REMOVED(RING_BUF_ITEM_DECLARE_SIZE)
#define RING_BUF_ITEM_DECLARE_POW2(...)	Z_RING_BUF_DECLARE_REMOVED(RING_BUF_ITEM_DECLARE_POW2)
/** @endcond */

#endif /* CONFIG_RING_BUFFER */
