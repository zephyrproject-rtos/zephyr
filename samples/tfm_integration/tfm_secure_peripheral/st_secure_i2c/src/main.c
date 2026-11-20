/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tfm_ns_interface.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <string.h>

#include "st_secure_i2c_partition.h"

static int ns_access_denied(void)
{
	int denied = 1;

	if (I2C2->TIMINGR != 0U) {
		printk("Isolation: I2C2 readable from NS\n");
		denied = 0;
	}

	if (TIM6->PSC != 0U || TIM6->CR1 != 0U) {
		printk("Isolation: TIM6 readable from NS\n");
		denied = 0;
	}

	if ((GPIOH->MODER & (GPIO_MODER_MODE4 | GPIO_MODER_MODE5)) != 0U ||
	    (GPIOF->MODER & GPIO_MODER_MODE11) != 0U) {
		printk("Isolation: secure GPIOs readable from NS\n");
		denied = 0;
	}

	return denied;
}

int main(void)
{
	uint8_t tx_payload[] = {0x00, 0x61, 0x62, 0x63, 0x64, 0x90, 0xC2};
	uint8_t rx_payload[32] = {0};
	size_t rx_expected_len = sizeof(tx_payload) + 2; /* len byte pos 2:3 */
	struct i2c_probe_result r = {0};
	psa_status_t status;

	status = st_secure_i2c_probe(&r);
	if (status != PSA_SUCCESS) {
		printk("I2C probe: service call failed (%d)\n", status);
		return 0;
	}

	/* hal_status carries the HAL_StatusTypeDef returned by the secure side,
	 * HAL_OK (0) meaning the device acknowledged its address.
	 */
	if (r.hal_status != 0) {
		printk("I2C probe: device not ready (hal_status=%u isr=0x%02X%02X)\n", r.hal_status,
		       r.isr_hi, r.isr_lo);
		return 0;
	}
	printk("I2C probe: device ready\n");

	status = st_secure_i2c_echo(tx_payload, sizeof(tx_payload), rx_payload, rx_expected_len);
	if (status != PSA_SUCCESS) {
		printk("I2C echo failed. Status: %d\n", status);
		return 0;
	}

	/* Status | length (data + CRC) | echoed data | CRC16 */
	const uint8_t expected[] = {0x00, 0x00, 0x06, 0x61, 0x62, 0x63, 0x64, 0x90, 0xC2};

	if (memcmp(rx_payload, expected, sizeof(expected)) != 0) {
		printk("I2C echo: unexpected response\n");
		return 0;
	}

	printk("I2C echo success. RX: ");
	for (size_t i = 0; i < rx_expected_len; i++) {
		printk("%02X ", rx_payload[i]);
	}
	printk("\n");

	if (!ns_access_denied()) {
		return 0;
	}
	printk("Isolation: NS access to I2C2, TIM6 and GPIOs denied\n");

	return 0;
}
