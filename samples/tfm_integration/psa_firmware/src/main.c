/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <psa/update.h>

#include "tfm_ns_interface.h"

extern uint8_t _binary_update_image_bin_start[];
extern uint8_t _binary_update_image_bin_end;
extern uint8_t _binary_update_image_bin_size;

extern uint8_t _binary_update_header_bin_start[];
extern uint8_t _binary_update_header_bin_end;
extern uint8_t _binary_update_header_bin_size;
/*
 * The _size symbol is particularly bizarre, since it's a length exposed as
 * a symbol's address, not a variable
 */
const size_t update_image_size = (size_t) &_binary_update_image_bin_size;
const size_t update_header_size = (size_t) &_binary_update_header_bin_size;

/* Turns out you can't flash blocks that are not a multiple of this */
const size_t min_block_size = 512;

/** Declare a reference to the application logging interface. */
LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

bool fwu_query_ver_active(psa_image_id_t image_id, psa_image_info_t *info)
{
	psa_status_t status;

	/* Query the active image. */
	status = psa_fwu_query(image_id, info);

	/* Get the active image version. State can be one of: */
	/*   PSA_IMAGE_UNDEFINED */
	/*   PSA_IMAGE_CANDIDATE */
	/*   PSA_IMAGE_INSTALLED */
	/*   PSA_IMAGE_REJECTED */
	/*   PSA_IMAGE_PENDING_INSTALL */
	/*   PSA_IMAGE_REBOOT_NEEDED */
	return info->state == PSA_IMAGE_INSTALLED && status == PSA_SUCCESS;
}

void fwu_disp_ver_active(void)
{
	/* Image type can be one of: */
	/*   FWU_IMAGE_TYPE_NONSECURE */
	/*   FWU_IMAGE_TYPE_SECURE */
	/*   FWU_IMAGE_TYPE_FULL */
	/* Making the call with FWU_IMAGE_TYPE_FULL will return the information */
	/* for the full image, which is a combination of the secure and nonsecure */
	/* images. This is related to the way images are managed in the */
	/* bootloader. For example, if the secure image and the nonsecure image */
	/* are combined together and then the combined image is signed and padded */
	/* with some metadata to generate the final image, then the final */
	/* generated image is a “full image”. This image management mode is */
	/* enabled via: */
	/*   -DMCUBOOT_IMAGE_NUMBER=1 */
	/* If this mode is enabled when building TF-M, then the application */
	/* should use the “FWU_IMAGE_TYPE_FULL” type when calling the Firmware */
	/* Update service. */
	/* If this mode is not enabled, then the application should use the */
	/* “FWU_IMAGE_TYPE_NONSECURE” or “FWU_IMAGE_TYPE_SECURE” type. */
	psa_image_id_t image_id = FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_ACTIVE,
							 FWU_IMAGE_TYPE_SECURE,
							 0);
	psa_image_info_t info = { 0 };
	bool status = fwu_query_ver_active(image_id, &info);

	if (status) {
		printk("Active S image version: %d.%d.%d-%d\n",
		       info.version.iv_major,
		       info.version.iv_minor,
		       info.version.iv_revision,
		       info.version.iv_build_num);
	} else {
		printk("\nUnable to query active secure image version!\n");
	}

	image_id = FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_ACTIVE,
							 FWU_IMAGE_TYPE_NONSECURE,
							 0);
	status = fwu_query_ver_active(image_id, &info);

	if (status) {
		printk("Active NS image version: %d.%d.%d-%d\n",
		       info.version.iv_major,
		       info.version.iv_minor,
		       info.version.iv_revision,
		       info.version.iv_build_num);
	} else {
		printk("\nUnable to query active nonsecure image version!\n");
	}
}

/**
 * Copy a part of a firmware update from the memory pointed to by `cur_block`,
 * with `size` size, into the `image_id` partition, * at offset `offset`.
 */
void write_update_part(psa_image_id_t image_id, size_t size, size_t offset, void *cur_block)
{
	size_t coppied = 0;

	while (coppied < size) {
		size_t next_block_size = PSA_FWU_MAX_BLOCK_SIZE;
		size_t remainder = size - coppied;

		if (remainder < next_block_size) {
			next_block_size = remainder;
		}
		psa_status_t write_status = psa_fwu_write(
			image_id,
			coppied + offset,
			&((uint8_t *)cur_block)[coppied],
			next_block_size
		);

		switch (write_status) {
		case PSA_ERROR_INVALID_ARGUMENT:
			printk("FW Update failed: Invalid Argument\n");
			k_oops();
			break;
		case PSA_ERROR_INSUFFICIENT_MEMORY:
			printk("FW Update failed: Insufficient Memory\n");
			k_oops();
			break;
		case PSA_ERROR_INSUFFICIENT_STORAGE:
			printk("FW Update failed: Insufficient Storage\n");
			k_oops();
			break;
		case PSA_ERROR_GENERIC_ERROR:
			printk("FW Update failed: Generic Error\n");
			k_oops();
			break;
		case PSA_ERROR_CURRENTLY_INSTALLING:
			printk("FW Update failed: Currently Installing\n");
			k_oops();
			break;
		default:
			break;
		}
		if (write_status != PSA_SUCCESS) {
			k_oops();
		}
		coppied += next_block_size;
	}
}

/**
 * Run a test firmware update
 */
void update_firmware(void)
{
	uintptr_t size = ((update_image_size + (min_block_size - 1)) / min_block_size)
		* min_block_size;
	void *cur_block = &_binary_update_image_bin_start[0];

	printk("Starting FWU; Writing Firmware from %lx size %5ld bytes\n",
		(uintptr_t) cur_block,
		(uintptr_t) update_image_size
	);

	psa_image_id_t image_id = FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_STAGE,
							 FWU_IMAGE_TYPE_NONSECURE,
							 0);
	write_update_part(image_id, size, 0, cur_block);

	size_t header_offset = 0x18000 - update_header_size;

	cur_block = &_binary_update_header_bin_start[0];
	printk("Wrote Firmware; Writing Header from %lx size %5ld bytes\n",
		(uintptr_t) cur_block,
		(uintptr_t) update_header_size
	);
	write_update_part(image_id, update_header_size, header_offset, cur_block);

	printk("Wrote Header; Installing Image\n");

	psa_image_id_t uuid;
	psa_image_version_t version;
	psa_status_t install_stat = psa_fwu_install(image_id, &uuid, &version);

	switch (install_stat) {
	case PSA_SUCCESS:
	case PSA_SUCCESS_REBOOT:
		break;
	case PSA_ERROR_INVALID_ARGUMENT:
		printk("FW Update failed: Invalid Argument\n");
		k_oops();
		break;
	case PSA_ERROR_INVALID_SIGNATURE:
		printk("FW Update failed: Invalid Signature\n");
		k_oops();
		break;
	case PSA_ERROR_GENERIC_ERROR:
		printk("FW Update failed: Generic Error\n");
		k_oops();
		break;
	case PSA_ERROR_STORAGE_FAILURE:
		printk("FW Update failed: Storage Failure\n");
		k_oops();
		break;
	case PSA_ERROR_DEPENDENCY_NEEDED:
		printk(
			"FW Update failed: Dependency Needed: uuid: %x version: %d.%d.%d-%d\n",
			uuid,
			version.iv_major,
			version.iv_minor,
			version.iv_revision,
			version.iv_build_num
		);
		k_oops();
		break;
	}

	psa_image_info_t info = { 0 };

	psa_fwu_query(image_id, &info);

	switch (info.state) {
	case PSA_IMAGE_UNDEFINED:
		printk("Installed New Firmware; Error: State UNDEFINED\n");
		break;
	case PSA_IMAGE_CANDIDATE:
		printk("Installed New Firmware as Candidate; Rebooting\n");
		break;
	case PSA_IMAGE_INSTALLED:
		printk("Installed New Firmware; Rebooting\n");
		break;
	case PSA_IMAGE_REJECTED:
		printk("Installed New Firmware; Error: Rejected\n");
		break;
	case PSA_IMAGE_PENDING_INSTALL:
		printk("Installed New Firmware pending install; Rebooting\n");
		break;
	case PSA_IMAGE_REBOOT_NEEDED:
		printk("Installed New Firmware; Reboot Needed; Rebooting\n");
		break;
	}
	psa_fwu_request_reboot();
}

void tfm_ns_interface_init(void);

void main(void)
{
	/* Initialize the TFM NS interface */
	tfm_ns_interface_init();

	/* Initialise the logger subsys and dump the current buffer. */
	log_init();

	printk("PSA Firmware API test\n");

	/* Display current firmware version. */
	fwu_disp_ver_active();

	update_firmware();
}
