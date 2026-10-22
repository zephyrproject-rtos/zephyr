/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <psa/update.h>

int main(void)
{
	psa_status_t rc;
	psa_fwu_component_info_t info;

	printk("Swapped application booted on %s\n", CONFIG_BOARD);

	rc = psa_fwu_query(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID, &info);
	if (rc != PSA_SUCCESS) {
		printk("psa_fwu_query failed: %d\n", rc);
		return -1;
	}

	if (info.state == PSA_FWU_TRIAL) {
		printk("Component is in TRIAL state, accepting FWU\n");

		/*
		 * In a production application, perform a self-check here before
		 * accepting the update (e.g. verify hardware functionality or
		 * connectivity) to ensure the new firmware is operating correctly.
		 * If the self-check fails, call psa_fwu_reject() to trigger a
		 * rollback.
		 */
		rc = psa_fwu_accept();
		if (rc != PSA_SUCCESS) {
			printk("psa_fwu_accept failed: %d\n", rc);
			return -1;
		}

		rc = psa_fwu_query(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID, &info);
		if (rc != PSA_SUCCESS) {
			printk("psa_fwu_query failed: %d\n", rc);
			return -1;
		}
	}

	if (info.state == PSA_FWU_UPDATED) {
		printk("Cleaning up FWU\n");
		rc = psa_fwu_clean(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID);
		if (rc != PSA_SUCCESS) {
			printk("psa_fwu_clean failed: %d\n", rc);
			return -1;
		}

		rc = psa_fwu_query(CONFIG_SAMPLE_TFM_FWU_COMPONENT_ID, &info);
		if (rc != PSA_SUCCESS) {
			printk("psa_fwu_query failed: %d\n", rc);
			return -1;
		}
	}

	if (info.state == PSA_FWU_READY) {
		printk("FWU completed successfully\n");
	} else {
		printk("FWU failed, unexpected state at end of update: %d\n", info.state);
		return -1;
	}

	return 0;
}
