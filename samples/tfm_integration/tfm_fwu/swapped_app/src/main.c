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

	rc = psa_fwu_query(CONFIG_TFM_FWU_COMPONENT_ID, &info);
	if (rc != PSA_SUCCESS) {
		printk("psa_fwu_query failed: %d\n", rc);
		return -1;
	}

	if (info.state == PSA_FWU_TRIAL) {
		printk("Component is in TRIAL state, accepting FWU\n");

		rc = psa_fwu_accept();

		if (rc != PSA_SUCCESS) {
			printk("psa_fwu_accept failed: %d\n", rc);
			return -1;
		}
	}

	printk("FWU completed successfully\n");

	return 0;
}
