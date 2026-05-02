/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_HOST_H_
#define _NUVOTON_NPCX_SOC_HOST_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes all host sub-modules in Core domain.
 *
 * This routine initializes all host sub-modules which HW blocks belong to
 * Core domain. And it also saves the pointer of eSPI callback list to report
 * any peripheral events application layer.
 *
 * @param dev Pointer to the device structure for the host bus driver instance.
 * @param callbacks A pointer to the list of espi callback functions.
 *
 * @retval 0 If successful.
 * @retval -EIO if cannot turn on host sub-module source clocks in core domain.
 */
int npcx_host_init_subs_core_domain(const struct device *host_bus_dev,
							sys_slist_t *callbacks);

/**
 * @brief Initializes all host sub-modules in Host domain.
 *
 * This routine initializes all host sub-modules which HW blocks belong to
 * Host domain. Please notice it must be executed after receiving PLT_RST
 * de-asserted signal and eSPI peripheral channel is enabled and ready.
 */
void npcx_host_init_subs_host_domain(void);

/**
 * @brief Reads data from a host sub-module which is updated via eSPI.
 *
 * This routine provides a generic interface to read a host sub-module which
 * information was updated by an eSPI transaction through peripheral channel.
 *
 * @param op Enum representing opcode for peripheral type and read request.
 * @param data Parameter to be read from to the host sub-module.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI peripheral is off or not supported.
 * @retval -EINVAL for unimplemented lpc opcode, but in range.
 */
int npcx_host_periph_read_request(enum lpc_peripheral_opcode op,
								uint32_t *data);

/**
 * @brief Writes data to a host sub-module which generates an eSPI transaction.
 *
 * This routine provides a generic interface to write data to a host sub-module
 * which triggers an eSPI transaction through peripheral channel.
 *
 * @param op Enum representing an opcode for peripheral type and write request.
 * @param data Represents the parameter passed to the host sub-module.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI peripheral is off or not supported.
 * @retval -EINVAL for unimplemented lpc opcode, but in range.
 */
int npcx_host_periph_write_request(enum lpc_peripheral_opcode op,
							const uint32_t *data);

/**
 * @brief Enable host access wake-up interrupt. Usually, it is used to wake up
 * ec during system is in Modern standby power mode.
 */
void npcx_host_enable_access_interrupt(void);

/**
 * @brief Disable host access wake-up interrupt.
 */
void npcx_host_disable_access_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_HOST_H_ */
