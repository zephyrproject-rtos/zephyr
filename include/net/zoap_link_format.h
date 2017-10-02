/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief CoAP implementation for Zephyr.
 */

#ifndef __LINK_FORMAT_H__
#define __LINK_FORMAT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup zoap COAP Library
 * @{
 */

/**
 * @deprecated This macro is deprecated.
 */
#define _ZOAP_WELL_KNOWN_CORE_PATH \
	((const char * const[]) { ".well-known", "core", NULL })

/**
 * @deprecated This api is deprecated.
 */
int _zoap_well_known_core_get(struct zoap_resource *resource,
			      struct zoap_packet *request,
			      const struct sockaddr *from);

/**
 * This resource should be added before all other resources that should be
 * included in the responses of the .well-known/core resource.
 */
/**
 * @deprecated This macro is deprecated.
 */
#define ZOAP_WELL_KNOWN_CORE_RESOURCE		\
	{ .get = _zoap_well_known_core_get,	\
	  .path = _ZOAP_WELL_KNOWN_CORE_PATH,	\
	}

/**
 * In case you want to add attributes to the resources included in the
 * 'well-known/core' "virtual" resource, the 'user_data' field should point
 * to a valid zoap_core_metadata structure.
 */
/**
 * @deprecated This struct is deprecated.
 */
struct zoap_core_metadata {
	const char * const *attributes;
	void *user_data;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __LINK_FORMAT_H__ */
