/*
 * Product identity reported by the Redfish resources.
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/redfish.h>

#include "redfish_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

static struct bmc_redfish_identity identity;

void bmc_redfish_identity_register(const struct bmc_redfish_identity *new_identity)
{
	static const struct bmc_redfish_identity empty;
	const struct bmc_redfish_identity *src = (new_identity != NULL) ? new_identity : &empty;

	identity = *src;

	/* Fill the gaps from Kconfig so that callers never see a NULL field. */
	if (identity.product_name == NULL) {
		identity.product_name = CONFIG_BMC_REDFISH_PRODUCT_NAME;
	}

	if (identity.manufacturer == NULL) {
		identity.manufacturer = CONFIG_BMC_REDFISH_MANUFACTURER;
	}

	if (identity.model == NULL) {
		identity.model = CONFIG_BMC_REDFISH_MODEL;
	}

	if (identity.serial_number == NULL) {
		identity.serial_number = CONFIG_BMC_REDFISH_SERIAL_NUMBER;
	}

	if (identity.firmware_version == NULL) {
		identity.firmware_version = bmc_firmware_version();
	}

	if (identity.processor_model == NULL) {
		identity.processor_model = CONFIG_BMC_REDFISH_PROCESSOR_MODEL;
	}

	if (identity.processor_count == 0) {
		identity.processor_count = CONFIG_BMC_REDFISH_PROCESSOR_COUNT;
	}

	if (identity.memory_gib == 0) {
		identity.memory_gib = CONFIG_BMC_REDFISH_MEMORY_GIB;
	}
}

const struct bmc_redfish_identity *bmc_redfish_identity_get(void)
{
	return &identity;
}

static int redfish_identity_init(void)
{
	if (identity.product_name == NULL) {
		/* No application registered one, so publish the Kconfig defaults. */
		bmc_redfish_identity_register(NULL);
	}

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_redfish_identity, BMC_INIT_PHASE_PLATFORM, redfish_identity_init, false);
