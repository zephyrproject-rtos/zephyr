/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <psa/update.h>

static uint8_t zephyr_ns_firmware[] = {
#include "zephyr_ns_signed.inc"
};

int main(void)
{
	psa_status_t rc;
	psa_fwu_component_info_t info;
	size_t written;

	printk("TF-M FWU on %s\n", CONFIG_BOARD);

	printk("Installing new firmware for component %d\n", CONFIG_TFM_FWU_COMPONENT_ID);

	rc = psa_fwu_query(CONFIG_TFM_FWU_COMPONENT_ID, &info);
	if (rc != PSA_SUCCESS) {
		printk("psa_fwu_query failed: %d\n", rc);
		return -1;
	}

	if (info.state != PSA_FWU_READY) {
		printk("Component is not in READY state, cannot perform FWU\n");
		return -1;
	}

	printk("Starting FWU\n");
	rc = psa_fwu_start(CONFIG_TFM_FWU_COMPONENT_ID, NULL, 0);
	if (rc != PSA_SUCCESS) {
		printk("psa_fwu_start failed: %d\n", rc);
		return -1;
	}

	printk("Writing %zu bytes\n", sizeof(zephyr_ns_firmware));
	written = 0;
	while (written < sizeof(zephyr_ns_firmware)) {
		size_t block_size = PSA_FWU_MAX_WRITE_SIZE;

		if (sizeof(zephyr_ns_firmware) - written < block_size) {
			block_size = sizeof(zephyr_ns_firmware) - written;
		}
		rc = psa_fwu_write(CONFIG_TFM_FWU_COMPONENT_ID, written,
				   zephyr_ns_firmware + written, block_size);
		if (rc != PSA_SUCCESS) {
			printk("psa_fwu_write failed: %d\n", rc);
			goto fwu_cancel;
		}
		written += block_size;
	}

	printk("Finishing FWU\n");
	rc = psa_fwu_finish(CONFIG_TFM_FWU_COMPONENT_ID);
	if (rc != PSA_SUCCESS) {
		printk("psa_fwu_finish failed: %d\n", rc);
		goto fwu_clean;
	}

	printk("Installing FWU\n");
	rc = psa_fwu_install();

	if (rc == PSA_SUCCESS) {
		printk("FWU installed, no reboot or restart required\n");
	} else if (rc == PSA_SUCCESS_REBOOT) {
		printk("FWU installed, reboot required\n");

		printk("Rebooting to apply the new firmware...\n");
		rc = psa_fwu_request_reboot();
		if (rc != PSA_SUCCESS) {
			printk("psa_fwu_request_reboot failed: %d\n", rc);
		}
	} else if (rc == PSA_SUCCESS_RESTART) {
		printk("FWU installed, restart required\n");
	} else {
		printk("psa_fwu_install failed: %d\n", rc);
		goto fwu_clean;
	}

	return 0;

fwu_cancel:
	printk("Cancelling FWU\n");
	rc = psa_fwu_cancel(CONFIG_TFM_FWU_COMPONENT_ID);
	if (rc != PSA_SUCCESS) {
		printk("psa_fwu_cancel failed: %d\n", rc);
		return -1;
	}

fwu_clean:
	printk("Cleaning up FWU\n");
	rc = psa_fwu_clean(CONFIG_TFM_FWU_COMPONENT_ID);
	if (rc != PSA_SUCCESS) {
		printk("psa_fwu_clean failed: %d\n", rc);
		return -1;
	}

	return -1;
}
