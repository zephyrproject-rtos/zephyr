/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief CoAP implementation for Zephyr.
 */

#ifndef ZEPHYR_INCLUDE_NET_COAP_LINK_FORMAT_H_
#define ZEPHYR_INCLUDE_NET_COAP_LINK_FORMAT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup coap COAP Library
 * @{
 */
/**
 * This resource should be added before all other resources that should be
 * included in the responses of the .well-known/core resource.
 */
#define COAP_WELL_KNOWN_CORE_PATH \
	((const char * const[]) { ".well-known", "core", NULL })

int coap_well_known_core_get(struct coap_resource *resource,
			     struct coap_packet *request,
			     struct coap_packet *response,
			     struct net_pkt *pkt);

/**
 * In case you want to add attributes to the resources included in the
 * 'well-known/core' "virtual" resource, the 'user_data' field should point
 * to a valid coap_core_metadata structure.
 */
struct coap_core_metadata {
	const char * const *attributes;
	void *user_data;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_COAP_LINK_FORMAT_H_ */
