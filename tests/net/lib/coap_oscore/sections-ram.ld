/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

#if defined(CONFIG_COAP_OSCORE)
ITERABLE_SECTION_RAM(coap_resource_oscore_secure_service, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_RAM(coap_resource_oscore_mixed_service, Z_LINK_ITERABLE_SUBALIGN)
#else
ITERABLE_SECTION_RAM(coap_resource_coap_service, Z_LINK_ITERABLE_SUBALIGN)
#endif
