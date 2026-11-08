/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(coap_resource_service_v4, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_RAM(coap_resource_service_v6, Z_LINK_ITERABLE_SUBALIGN)
