/*
 * Copyright (c) 2023 Pawel Osypiuk <pawelosyp@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <nrf53_cpunet_mgmt.h>
#include <../subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/debug.h>

#include <hal/nrf_spu.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_nrf53_support);

int bt_hci_transport_teardown(const struct device *dev)
{
	ARG_UNUSED(dev);
    /* Put the Network MCU in Forced-OFF mode. */
	nrf53_cpunet_enable(false);
	LOG_DBG("Network MCU placed in Forced-OFF mode");

	return 0;
}

int bt_hci_transport_setup(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Route Bluetooth Controller Debug Pins */
	DEBUG_SETUP();

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	nrf_spu_extdomain_set((NRF_SPU_Type *)DT_REG_ADDR(DT_NODELABEL(spu)), 0, true, false);
#endif /* !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */

	/* Release the Network MCU, 'Release force off signal' */
	nrf53_cpunet_enable(true);

	return 0;
}
