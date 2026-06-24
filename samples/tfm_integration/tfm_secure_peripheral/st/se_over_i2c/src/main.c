/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tfm_ns_interface.h>
#include <zephyr/kernel.h>

#include "se_over_i2c.h"

int main(void)
{
	struct i2c_probe_result r = {0};
	psa_status_t status;

	status = se_i2c_probe(&r);

	if (status != PSA_SUCCESS) {
		printk("I2C probe: service call failed (%d)\n", status);
		return 0;
	}
	if (r.hal_status == 0) {
		printk("I2C probe: device not ready (hal_status=%u isr=0x%02X%02X)\n",
			r.hal_status, r.isr_hi, r.isr_lo);
		return 0;
	}

	uint8_t tx_payload[] = {0x00, 0x61, 0x62, 0x63, 0x64, 0x90, 0xC2};
	uint8_t rx_payload[32] = {0};
	int rx_expected_len = sizeof(tx_payload) + 2; /* len byte pos 2:3 */

	status = se_i2c_echo(tx_payload, sizeof(tx_payload), rx_payload, rx_expected_len);

	if (status == PSA_SUCCESS) {
		printk("I2C echo success. RX: ");
		for (int i = 0; i < rx_expected_len; i++) {
			printk("%02X ", rx_payload[i]);
		}
		printk("\n");
	} else {
		printk("I2C echo failed. Status: %d\n", status);
	}

	return 0;
}
