/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_ZEPHYR_UTILS_H__
#define NRFX_ZEPHYR_UTILS_H__

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

/*
 * For chips with TrustZone support, MDK provides CMSIS-Core peripheral
 * accessing symbols in two flavors, with secure and non-secure base address
 * mappings. Their names contain the suffix _S or _NS, respectively.
 * Because nrfx HALs and drivers require these peripheral accessing symbols
 * without any suffixes, the following macro is provided that will translate
 * their names according to the kind of the target that is built.
 */
#if defined(NRF_TRUSTZONE_NONSECURE)
#define NRF_PERIPH(P) P##_NS
#else
#define NRF_PERIPH(P) P##_S
#endif

#define NRFX_CONFIG_BIT_DT(node_id, prop, idx) BIT(DT_PROP_BY_IDX(node_id, prop, idx))
#define NRFX_CONFIG_MASK_DT(node_id, prop) \
	(COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		(DT_FOREACH_PROP_ELEM_SEP(node_id, prop, NRFX_CONFIG_BIT_DT, (|))), \
		(0)))

/* If global of local DPPIC peripherals are used, provide the following macro
 * definitions required by the interconnect/apb layer:
 * - NRFX_DPPI_PUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num)
 * - NRFX_DPPI_SUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num)
 * - NRFX_DPPI_PUB_OR_SUB_MASK(inst_num)
 * - NRFX_DPPI_CHANNELS_SINGLE_VAR_NAME_BY_INST_NUM(inst_num)
 * - NRFX_INTERCONNECT_APB_GLOBAL_DPPI_DEFINE
 * - NRFX_INTERCONNECT_APB_LOCAL_DPPI_DEFINE
 * based on information from devicetree.
 */
#if	DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_dppic_global) || \
	DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_dppic_local)
/* Source (publish) channels masks generation. */
#define NRFX_DPPI_PUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_MASK_DT(DT_NODELABEL(_CONCAT(dppic, inst_num)), source_channels)

/* Sink (subscribe) channels masks generation. */
#define NRFX_DPPI_SUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_MASK_DT(DT_NODELABEL(_CONCAT(dppic, inst_num)), sink_channels)

#define NRFX_DPPI_PUB_OR_SUB_MASK(inst_num) \
	UTIL_OR(DT_NODE_HAS_PROP(DT_NODELABEL(_CONCAT(dppic, inst_num)), source_channels), \
		DT_NODE_HAS_PROP(DT_NODELABEL(_CONCAT(dppic, inst_num)), sink_channels))

/* Variables names generation. */
#define NRFX_CONFIG_DPPI_CHANNELS_ENTRY_NAME(node_id) _CONCAT(_CONCAT(m_, node_id), _channels)
#define NRFX_DPPI_CHANNELS_SINGLE_VAR_NAME_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_DPPI_CHANNELS_ENTRY_NAME(DT_NODELABEL(_CONCAT(dppic, inst_num)))

/* Variables entries generation. */
#define NRFX_CONFIG_DPPI_CHANNELS_ENTRY(node_id) \
	static nrfx_atomic_t NRFX_CONFIG_DPPI_CHANNELS_ENTRY_NAME(node_id) \
		__attribute__((used)) = \
		NRFX_CONFIG_MASK_DT(node_id, source_channels) | \
		NRFX_CONFIG_MASK_DT(node_id, sink_channels);
#define NRFX_INTERCONNECT_APB_GLOBAL_DPPI_DEFINE \
	DT_FOREACH_STATUS_OKAY(nordic_nrf_dppic_global, NRFX_CONFIG_DPPI_CHANNELS_ENTRY)
#define NRFX_INTERCONNECT_APB_LOCAL_DPPI_DEFINE \
	DT_FOREACH_STATUS_OKAY(nordic_nrf_dppic_local, NRFX_CONFIG_DPPI_CHANNELS_ENTRY)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_dppic_global) || ... */

/* If local or global DPPIC peripherals are used, provide the following macro
 * definitions required by the interconnect/ipct layer:
 * - NRFX_IPCTx_PUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num)
 * - NRFX_IPCTx_SUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num)
 * - NRFX_IPCT_PUB_OR_SUB_MASK(inst_num)
 * - NRFX_IPCTx_CHANNELS_SINGLE_VAR_NAME_BY_INST_NUM(inst_num)
 * - NRFX_INTERCONNECT_IPCT_GLOBAL_DEFINE
 * - NRFX_INTERCONNECT_IPCT_LOCAL_DEFINE
 * based on information from devicetree.
 */
#if	DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ipct_global) || \
	DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ipct_local)
/* Channels masks generation. */
#define NRFX_CONFIG_IPCT_MASK_DT(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, owned_channels), \
		(NRFX_CONFIG_MASK_DT(node_id, owned_channels)), \
		(COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, nordic_nrf_ipct_local), \
			(BIT_MASK(DT_PROP(node_id, channels))), (0))))

#if defined(NRF_APPLICATION)
#define NRFX_CONFIG_IPCT_LOCAL_NODE DT_NODELABEL(cpuapp_ipct)
#elif defined(NRF_RADIOCORE)
#define NRFX_CONFIG_IPCT_LOCAL_NODE DT_NODELABEL(cpurad_ipct)
#endif
#define NRFX_CONFIG_IPCT_NODE_BY_INST_NUM(inst_num) \
	COND_CODE_1(IS_EMPTY(inst_num), \
		(NRFX_CONFIG_IPCT_LOCAL_NODE), \
		(DT_NODELABEL(_CONCAT(ipct, inst_num))))

#define NRFX_IPCTx_PUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_IPCT_MASK_DT(NRFX_CONFIG_IPCT_NODE_BY_INST_NUM(inst_num))

#define NRFX_IPCTx_SUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_IPCT_MASK_DT(NRFX_CONFIG_IPCT_NODE_BY_INST_NUM(inst_num))

#define NRFX_IPCT_PUB_OR_SUB_MASK(inst_num) \
	COND_CODE_1(IS_EMPTY(inst_num), \
		(DT_NODE_HAS_STATUS_OKAY(NRFX_CONFIG_IPCT_LOCAL_NODE)), \
		(DT_NODE_HAS_PROP(DT_NODELABEL(_CONCAT(ipct, inst_num)), owned_channels)))

/* Variables names generation. */
#define NRFX_CONFIG_IPCT_CHANNELS_ENTRY_NAME(node_id) _CONCAT(_CONCAT(m_, node_id), _channels)
#define NRFX_IPCTx_CHANNELS_SINGLE_VAR_NAME_BY_INST_NUM(inst_num) \
	COND_CODE_1(IS_EMPTY(inst_num), \
		(NRFX_CONFIG_IPCT_CHANNELS_ENTRY_NAME(NRFX_CONFIG_IPCT_LOCAL_NODE)), \
		(NRFX_CONFIG_IPCT_CHANNELS_ENTRY_NAME(DT_NODELABEL(_CONCAT(ipct, inst_num)))))

/* Variables entries generation. */
#define NRFX_CONFIG_IPCT_CHANNELS_ENTRY(node_id) \
	static nrfx_atomic_t NRFX_CONFIG_IPCT_CHANNELS_ENTRY_NAME(node_id) \
		__attribute__((used)) = \
		NRFX_CONFIG_IPCT_MASK_DT(node_id);
#define NRFX_INTERCONNECT_IPCT_LOCAL_DEFINE \
	DT_FOREACH_STATUS_OKAY(nordic_nrf_ipct_local, NRFX_CONFIG_IPCT_CHANNELS_ENTRY)
#define NRFX_INTERCONNECT_IPCT_GLOBAL_DEFINE \
	DT_FOREACH_STATUS_OKAY(nordic_nrf_ipct_global, NRFX_CONFIG_IPCT_CHANNELS_ENTRY)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ipct_global) || ... */

#endif /* NRFX_ZEPHYR_UTILS_H__ */
