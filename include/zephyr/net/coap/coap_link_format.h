/*
 * Copyright (c) 2018 Intel Corporation
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

/**
 * @addtogroup coap COAP Library
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This resource should be added before all other resources that should be
 * included in the responses of the .well-known/core resource if is to be used with
 * coap_well_known_core_get.
 */
#define COAP_WELL_KNOWN_CORE_PATH \
	((const char * const[]) { ".well-known", "core", NULL })

/**
 * @brief Build a CoAP response for a .well-known/core CoAP request.
 *
 * @param resource Array of known resources, terminated with an empty resource
 * @param request A pointer to the .well-known/core CoAP request
 * @param response A pointer to a CoAP response, will be initialized
 * @param data A data pointer to be used to build the CoAP response
 * @param data_len The maximum length of the data buffer
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_well_known_core_get(struct coap_resource *resource,
			     const struct coap_packet *request,
			     struct coap_packet *response,
			     uint8_t *data, uint16_t data_len);

/**
 * @brief Build a CoAP response for a .well-known/core CoAP request.
 *
 * @param resources Array of known resources
 * @param resources_len Number of resources in the array
 * @param request A pointer to the .well-known/core CoAP request
 * @param response A pointer to a CoAP response, will be initialized
 * @param data A data pointer to be used to build the CoAP response
 * @param data_len The maximum length of the data buffer
 *
 * @return 0 in case of success or negative in case of error.
 */
int coap_well_known_core_get_len(struct coap_resource *resources,
				 size_t resources_len,
				 const struct coap_packet *request,
				 struct coap_packet *response,
				 uint8_t *data, uint16_t data_len);

/**
 * In case you want to add attributes to the resources included in the
 * 'well-known/core' "virtual" resource, the 'user_data' field should point
 * to a valid coap_core_metadata structure.
 */
struct coap_core_metadata {
	/** List of attributes to add */
	const char * const *attributes;
	/** User specific data */
	void *user_data;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_COAP_LINK_FORMAT_H_ */
