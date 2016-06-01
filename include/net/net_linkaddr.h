/*
 * Copyright (c) 2016 Intel Corporation.
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
 * @brief Public API for network link address
 */

#ifndef __NET_LINKADDR_H__
#define __NET_LINKADDR_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief Hardware link address structure
 *
 *  Used to hold the link address information
 */
struct net_linkaddr {
	/** The array of byte representing the address */
	uint8_t *addr;

	/** Length of that address array */
	uint8_t len;
};

/**
 *  @brief Hardware link address structure
 *
 *  Used to hold the link address information. This variant is needed
 *  when we have to store the link layer address.
 *  Note that you cannot cast this to net_linkaddr as they store
 *  different things.
 */
struct net_linkaddr_storage {
	/** The real length of the ll address. */
	uint8_t len;

	union {
		/** The array of bytes representing the address */
		uint8_t addr[0];

		struct {
			/* The integer array allocate total of 8 bytes
			 * that can hold currently all the supported
			 * link layer addresses. These int's can be
			 * used when comparing lladdr instead of using
			 * memcmp()
			 */
			uint32_t storage[2];
		};
	};
};

#ifdef __cplusplus
}
#endif

#endif /* __NET_LINKADDR_H__ */
