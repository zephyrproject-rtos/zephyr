/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_KERNEL_SIGNAL_H_
#define ZEPHYR_INCLUDE_ZEPHYR_KERNEL_SIGNAL_H_

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Kernel Signal APIs
 * @defgroup k_sig_apis Kernel Signal APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Block signals in the accompanying @ref k_sig_set
 *
 * @see @ref k_sig_mask
 */
#define K_SIG_BLOCK 0

/**
 * @brief Set signals in the accompanying @ref k_sig_set
 *
 * @see @ref k_sig_mask
 */
#define K_SIG_SETMASK 1

/**
 * @brief Unblock signals in the accompanying @ref k_sig_set
 *
 * @see @ref k_sig_mask
 */
#define K_SIG_UNBLOCK 2

/**
 * @brief Minimum realtime signal number
 *
 * Programs should only attempt to iterate over real-time signals relative to K_SIG_RTMIN and
 * @ref K_SIG_NUM_RT.
 *
 * For example:
 *
 * @code{.c}
 * for (int i = 0; i < K_SIG_NUM_RT; ++i) {
 *   int sig = K_SIG_RTMIN + i;
 *   // Do something with sig
 * }
 * @endcode
 *
 * @note K_SIG_RTMAX is intentionally not defined since it would inevitably lead to off-by-one
 * errors.
 */
#define K_SIG_RTMIN 32

/** @brief Number of supported realtime signals */
#define K_SIG_NUM_RT MAX(CONFIG_SIGNAL_SET_SIZE - K_SIG_RTMIN, 0)

/**
 * @brief Signal value
 *
 * This union is passed to @ref k_sig_queue and is embedded in @ref k_sig_info, as an output
 * parameter of @ref k_sig_timedwait.
 */
union k_sig_val {
	int sival_int; /**< Integer signal value */
	void *sival_ptr; /**< Pointer signal value */
};

/**
 * @brief Signal info
 *
 * A pointer to an instance of this structure may be passed to @ref k_sig_timedwait to receive
 * additional signal information.
 */
struct k_sig_info {
	int si_signo; /**< Signal number */
	int si_code; /**< Additional signal code */
	union k_sig_val si_value; /**< Signal value */
};

/** @brief A bitset large enough to contain all signal bits. */
struct k_sig_set {
	unsigned long sig[DIV_ROUND_UP(MAX(CONFIG_SIGNAL_SET_SIZE, 1), BITS_PER_LONG)];
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_KERNEL_SIGNAL_H_ */
