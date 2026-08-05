/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <psa/update.h>

static const uint8_t swapped_ns_firmware[] = {
#include "zephyr_ns_signed.inc"
};

int main(void)
{
	psa_status_t status;
	psa_fwu_component_info_t info;
	size_t written;
	size_t block_size;

	printk("TF-M FWU on %s\n", CONFIG_BOARD);

	printk("Installing new firmware for component %d\n", CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID);

	status = psa_fwu_query(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID, &info);
	if (status != PSA_SUCCESS) {
		printk("psa_fwu_query failed: %d\n", status);
		return -1;
	}

	if (info.state != PSA_FWU_READY) {
		printk("Component is not in READY state, cannot perform FWU\n");
		return -1;
	}

	printk("Starting FWU\n");
	status = psa_fwu_start(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID, NULL, 0);
	if (status != PSA_SUCCESS) {
		printk("psa_fwu_start failed: %d\n", status);
		return -1;
	}

	printk("Writing %zu bytes\n", sizeof(swapped_ns_firmware));
	for (written = 0; written < sizeof(swapped_ns_firmware); written += block_size) {
		block_size = MIN(PSA_FWU_MAX_WRITE_SIZE, sizeof(swapped_ns_firmware) - written);

		status = psa_fwu_write(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID, written,
				   swapped_ns_firmware + written, block_size);
		if (status != PSA_SUCCESS) {
			printk("psa_fwu_write failed: %d\n", status);
			goto fwu_cancel;
		}
	}

	printk("Finishing FWU\n");
	status = psa_fwu_finish(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID);
	if (status != PSA_SUCCESS) {
		printk("psa_fwu_finish failed: %d\n", status);
		goto fwu_cancel;
	}

	printk("Installing FWU\n");
	status = psa_fwu_install();
	if (status == PSA_SUCCESS) {
		/*
		 * Not expected when updating the running firmware,
		 * as a reboot should be required to apply the update.
		 */
		printk("FWU installed, no reboot or restart required\n");
	} else if (status == PSA_SUCCESS_REBOOT) {
		printk("FWU installed, reboot required\n");

		printk("Rebooting to apply the new firmware...\n");
		status = psa_fwu_request_reboot();
		if (status != PSA_SUCCESS) {
			printk("psa_fwu_request_reboot failed: %d\n", status);
			goto fwu_cancel;
		}
	} else if (status == PSA_SUCCESS_RESTART) {
		/*
		 * Not expected when updating the running firmware,
		 * as a reboot should be required to apply the update.
		 * How to restart a component is implementation specific.
		 */
		printk("FWU installed, restart required\n");
	} else {
		printk("psa_fwu_install failed: %d\n", status);
		goto fwu_cancel;
	}

	return 0;

fwu_cancel:
	printk("Cancelling FWU\n");

	status = psa_fwu_query(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID, &info);
	if (status != PSA_SUCCESS) {
		printk("psa_fwu_query failed: %d\n", status);
		return -1;
	}

	if (info.state == PSA_FWU_WRITING || info.state == PSA_FWU_CANDIDATE) {
		status = psa_fwu_cancel(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID);
		if (status != PSA_SUCCESS) {
			printk("psa_fwu_cancel failed: %d\n", status);
			return -1;
		}

		status = psa_fwu_query(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID, &info);
		if (status != PSA_SUCCESS) {
			printk("psa_fwu_query failed: %d\n", status);
			return -1;
		}
	}

	if (info.state == PSA_FWU_FAILED) {
		status = psa_fwu_clean(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID);
		if (status != PSA_SUCCESS) {
			printk("psa_fwu_clean failed: %d\n", status);
			return -1;
		}
	}

	return -1;
}
