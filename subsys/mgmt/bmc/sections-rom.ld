/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_ROM(bmc_component, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_ROM(bmc_sensor, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_ROM(bmc_redfish_oem, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_ROM(bmc_redfish_resource, Z_LINK_ITERABLE_SUBALIGN)

#ifdef CONFIG_BMC_HTTP
ITERABLE_SECTION_ROM(http_resource_desc_bmc_http_service, Z_LINK_ITERABLE_SUBALIGN)
#endif

#ifdef CONFIG_BMC_HTTPS
ITERABLE_SECTION_ROM(http_resource_desc_bmc_https_service, Z_LINK_ITERABLE_SUBALIGN)
#endif
