/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <psa/update.h>

#include "tfm_ns_interface.h"

/** Declare a reference to the application logging interface. */
LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

void fwu_disp_ver_active(void)
{
	psa_status_t status;
	psa_image_info_t info = { 0 };

	/* Image type can be one of: */
	/*   FWU_IMAGE_TYPE_NONSECURE */
	/*   FWU_IMAGE_TYPE_SECURE */
	/*   FWU_IMAGE_TYPE_FULL */
	psa_image_id_t image_id = FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_ACTIVE,
							 FWU_IMAGE_TYPE_NONSECURE,
							 0);

	/* Query the active image. */
	status = psa_fwu_query(image_id, &info);

	/* Get the active image version. State can be one of: */
	/*   PSA_IMAGE_UNDEFINED */
	/*   PSA_IMAGE_CANDIDATE */
	/*   PSA_IMAGE_INSTALLED */
	/*   PSA_IMAGE_REJECTED */
	/*   PSA_IMAGE_PENDING_INSTALL */
	/*   PSA_IMAGE_REBOOT_NEEDED */
	if (info.state == PSA_IMAGE_INSTALLED && status == PSA_SUCCESS) {
		printk("Active NS image version: %d.%d.%d-%d\n",
		       info.version.iv_major,
		       info.version.iv_minor,
		       info.version.iv_revision,
		       info.version.iv_build_num);
	} else {
		printk("\nUnable to query active image version!\n");
	}
}

void main(void)
{
	/* Initialize the TFM NS interface */
	tfm_ns_interface_init();

	/* Initialise the logger subsys and dump the current buffer. */
	log_init();

	printk("PSA Firmware API test\n");

	/* Display current firmware version. */
	fwu_disp_ver_active();

	while (1) {

	}
}
