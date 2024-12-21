/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USBC_TCPC_UCPD_NUMAKER_H_
#define ZEPHYR_DRIVERS_USBC_TCPC_UCPD_NUMAKER_H_

#include <zephyr/drivers/usb_c/usbc_ppc.h>
#include <zephyr/drivers/usb_c/usbc_vbus.h>

/* TCPC exported for PPC */
int numaker_tcpc_ppc_is_dead_battery_mode(const struct device *dev);
int numaker_tcpc_ppc_exit_dead_battery_mode(const struct device *dev);
int numaker_tcpc_ppc_is_vbus_source(const struct device *dev);
int numaker_tcpc_ppc_is_vbus_sink(const struct device *dev);
int numaker_tcpc_ppc_set_snk_ctrl(const struct device *dev, bool enable);
int numaker_tcpc_ppc_set_src_ctrl(const struct device *dev, bool enable);
int numaker_tcpc_ppc_set_vbus_discharge(const struct device *dev, bool enable);
int numaker_tcpc_ppc_is_vbus_present(const struct device *dev);
int numaker_tcpc_ppc_set_event_handler(const struct device *dev, usbc_ppc_event_cb_t handler,
				       void *data);
int numaker_tcpc_ppc_dump_regs(const struct device *dev);

/* TCPC exported for VBUS */
bool numaker_tcpc_vbus_check_level(const struct device *dev, enum tc_vbus_level level);
int numaker_tcpc_vbus_measure(const struct device *dev, int *vbus_meas);
int numaker_tcpc_vbus_discharge(const struct device *dev, bool enable);
int numaker_tcpc_vbus_enable(const struct device *dev, bool enable);

#endif /* ZEPHYR_DRIVERS_USBC_TCPC_UCPD_NUMAKER_H_ */
