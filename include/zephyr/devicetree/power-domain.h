/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_POWER_DOMAIN_H_
#define ZEPHYR_INCLUDE_DEVICETREE_POWER_DOMAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-power-domains "For-each" macros
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get number of power domain children with status "okay" of a power domain
 *
 * @param node_id Power domain's node identifier
 * @return Number of children of specified power domain
 */
#define DT_POWER_DOMAIN_CHILDREN_LEN_OKAY(node_id) \
	DT_CAT(node_id, _POWER_DOMAIN_CHILDREN_LEN_OKAY)

/**
 * @brief Invokes @p fn for every power domain child with status "okay" of a power domain
 *
 * @details The macro @p fn must take one parameter, which will be a node
 * identifier. The macro is expanded once for every power domain child with status
 * "okay".
 *
 * @note The order that nodes are visited in is not specified.
 *
 * @param node_id Power domain's node identifier
 * @param fn Macro to invoke
 */
#define DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY(node_id, fn) \
	DT_CAT(node_id, _POWER_DOMAIN_CHILDREN_FOREACH_OKAY)(fn)

/**
 * @brief Variant of DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY with separator
 *
 * @see DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY()
 *
 * @details The macro @p fn must take one parameter, which will be a node
 * identifier. The macro is expanded once for every power domain child with status
 * "okay". Each invocation will be separated by the @p sep which must be
 * provided in brackets.
 *
 * @param node_id Power domain's node identifier
 * @param fn Macro to invoke
 * @param sep Separator pasted between invocations of @p fn
 */
#define DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY_SEP(node_id, fn, sep) \
	DT_CAT(node_id, _POWER_DOMAIN_CHILDREN_FOREACH_OKAY_SEP)(fn, sep)

/**
 * @brief Variant of DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY with VARGS
 *
 * @see DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY()
 *
 * @details The macro @p fn must take one parameter, which will be a node
 * identifier, followed by variadic args. The macro is expanded once for
 * every power domain child with status "okay".
 *
 * @param node_id Power domain's node identifier
 * @param fn Macro to invoke
 * @param ... Variadic args passed to @p fn
 */
#define DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY_VARGS(node_id, fn, ...) \
	DT_CAT(node_id, _POWER_DOMAIN_CHILDREN_FOREACH_OKAY_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Variant of DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY with separator and VARGS
 *
 * @see DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY()
 *
 * @details The macro @p fn must take one parameter, which will be a node
 * identifier, followed by variadic args. The macro is expanded once for every
 * power domain child with status "okay". Each invocation will be separated by the
 * @p sep which must be provided in brackets.
 *
 * @param node_id Power domain's node identifier
 * @param fn Macro to invoke
 * @param sep Separator pasted between invocations of @p fn
 * @param ... Variadic args passed to @p fn
 */
#define DT_POWER_DOMAIN_CHILDREN_FOREACH_OKAY_SEP_VARGS(node_id, fn, sep, ...) \
	DT_CAT(node_id, _POWER_DOMAIN_CHILDREN_FOREACH_OKAY_SEP_VARGS)(fn, sep, __VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_POWER_DOMAIN_H_ */
