/** @file
 *  @brief Bluetooth H:4 UART driver vendor extension interface.
 */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H4_H_
#define ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H4_H_

#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth/hci_lockstep.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth H:4 UART driver vendor extension interface
 * @defgroup bt_hci_h4 Bluetooth H:4 UART driver vendor extension interface
 * @ingroup bt_hci_api
 * @since 4.5
 * @version 0.1.0
 *
 * Functions that a vendor extension of the H:4 UART driver implements, in
 * addition to bt_hci_transport_setup() and bt_hci_transport_teardown(), which
 * any board or extension may implement.
 *
 * @{
 */

/**
 * @brief Configure the controller while the H:4 transport is opened.
 *
 * Implemented by the vendor extension that selects
 * @kconfig{CONFIG_BT_H4_VND_OPEN}. The H:4 driver calls it last in its open(),
 * once it transmits and receives on the UART and has restored the initial
 * command allowance of @p ls, and fails the open() with what it returns.
 *
 * The extension sends its commands with bt_hci_lockstep_cmd_send_sync() on
 * @p ls: the driver feeds every packet it receives to the helper, so the
 * responses reach the extension. A command carries at most
 * @kconfig{CONFIG_BT_BUF_CMD_TX_SIZE} parameter bytes and a longer one fails
 * with -EMSGSIZE, so an extension with longer commands needs that option
 * raised, for example with a Kconfig default. The extension reads the public
 * address to configure with bt_hci_get_public_addr(), and may reconfigure the
 * UART, for a baud rate change the controller has acknowledged. Work that is
 * not HCI, such as uploading a firmware image over the bare UART, belongs in
 * bt_hci_transport_setup(), which runs before the driver takes the UART.
 *
 * On failure the driver undoes its own open(); the extension leaves nothing
 * of its own running that a later open() would trip over.
 *
 * @param dev  HCI device, for bt_hci_get_public_addr()
 * @param uart UART device of the transport, for reconfiguring it
 * @param ls   Lockstep helper of the transport
 *
 * @retval 0 The controller is configured.
 * @return Negative errno value on failure, returned by bt_hci_open().
 */
int bt_h4_vnd_open(const struct device *dev, const struct device *uart, struct bt_hci_lockstep *ls);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H4_H_ */
