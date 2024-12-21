/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_bt_hci_entropy

#include <zephyr/drivers/entropy.h>
#include <zephyr/bluetooth/hci.h>
#include <string.h>

static int entropy_bt_init(const struct device *dev)
{
	/* Nothing to do */
	return 0;
}

static int entropy_bt_get_entropy(const struct device *dev,
				  uint8_t *buffer, uint16_t length)
{
	/* Do not wait for BT to be ready (i.e. bt_is_ready()) before issueing
	 * the command. The reason is that when crypto is enabled and the PSA
	 * Crypto API support is provided through Mbed TLS, random number generator
	 * needs to be available since the very first call to psa_crypto_init()
	 * which is usually done before BT is completely initialized.
	 * On the other hand, in devices like the nrf5340, the crytographically
	 * secure RNG is owned by the cpu_net, so the cpu_app needs to poll it
	 * to get random data. Again, there is no need to wait for BT to be
	 * completely initialized for this kind of support. Just try to send the
	 * request through HCI. If the command fails for any reason, then
	 * we return failure anyway.
	 */

	return bt_hci_le_rand(buffer, length);
}

/* HCI commands cannot be run from an interrupt context */
static DEVICE_API(entropy, entropy_bt_api) = {
	.get_entropy = entropy_bt_get_entropy,
	.get_entropy_isr = NULL
};

#define ENTROPY_BT_HCI_INIT(inst)				  \
	DEVICE_DT_INST_DEFINE(inst, entropy_bt_init,		  \
			      NULL, NULL, NULL,			  \
			      PRE_KERNEL_1,			  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &entropy_bt_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_BT_HCI_INIT)
