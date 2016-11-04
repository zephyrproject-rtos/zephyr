/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief CoAP implementation for Zephyr.
 */

#ifndef __LINK_FORMAT_H__
#define __LINK_FORMAT_H__

#define _ZOAP_WELL_KNOWN_CORE_PATH \
	((const char * const[]) { ".well-known", "core", NULL })

int _zoap_well_known_core_get(struct zoap_resource *resource,
			      struct zoap_packet *request,
			      const struct sockaddr *from);

/**
 * This resource should be added before all other resources that should be
 * included in the responses of the .well-known/core resource.
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
struct zoap_core_metadata {
	const char * const *attributes;
	void *user_data;
};

#endif /* __LINK_FORMAT_H__ */
