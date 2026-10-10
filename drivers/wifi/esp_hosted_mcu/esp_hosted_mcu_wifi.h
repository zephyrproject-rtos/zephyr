/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_MCU_WIFI_H_
#define ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_MCU_WIFI_H_

/*
 * Run the coprocessor's esp_wifi bring-up again, after
 * esp_hosted_mcu_reattach() put the transport core back in step with a
 * coprocessor that restarted: MAC query, WifiInit, station mode, WifiStart.
 * The coprocessor kept none of that across the restart.
 *
 * A station association or a soft AP of the previous run is reported lost
 * first, the way the coprocessor's own disconnect event would have, and a scan
 * still waiting for the previous run's scan-done event is released with
 * -EAGAIN.
 *
 * Call it once esp_hosted_mcu_reattach() has returned 0. Called while the
 * coprocessor is still coming up, the bring-up requests time out and it
 * returns -EIO.
 *
 * Returns 0 on success, -EIO when the coprocessor refused a step, and -ENODEV
 * when the Wi-Fi device failed its own init at boot: the kernel keeps such a
 * device not ready and the network stack never initialised its interface,
 * which only a reboot of the host clears.
 */
int esp_hosted_mcu_wifi_restart(void);

#endif /* ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_MCU_WIFI_H_ */
