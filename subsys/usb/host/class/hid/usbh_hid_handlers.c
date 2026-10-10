/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/class/usbh_hid.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_usbh_hid_start_input_reports(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_USBH_HID(dev, start_input_reports));
	return z_impl_usbh_hid_start_input_reports(dev);
}
#include <zephyr/syscalls/usbh_hid_start_input_reports_mrsh.c>

static inline int z_vrfy_usbh_hid_stop_input_reports(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_USBH_HID(dev, stop_input_reports));
	return z_impl_usbh_hid_stop_input_reports(dev);
}
#include <zephyr/syscalls/usbh_hid_stop_input_reports_mrsh.c>

static inline int z_vrfy_usbh_hid_get_report(const struct device *dev,
					     enum usbh_hid_report_field_type type,
					     uint8_t report_id, size_t length, uint8_t *buffer)
{
	K_OOPS(K_SYSCALL_DRIVER_USBH_HID(dev, get_report));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buffer, length));
	return z_impl_usbh_hid_get_report(dev, type, report_id, length, buffer);
}
#include <zephyr/syscalls/usbh_hid_get_report_mrsh.c>

static inline int z_vrfy_usbh_hid_set_report(const struct device *dev,
					     enum usbh_hid_report_field_type type,
					     uint8_t report_id, size_t data_length,
					     const uint8_t *data)
{
	K_OOPS(K_SYSCALL_DRIVER_USBH_HID(dev, set_report));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, data_length));
	return z_impl_usbh_hid_set_report(dev, type, report_id, data_length, data);
}
#include <zephyr/syscalls/usbh_hid_set_report_mrsh.c>

static inline int z_vrfy_usbh_hid_set_idle_rate(const struct device *dev, uint8_t report_id,
						uint16_t idle_period_ms)
{
	K_OOPS(K_SYSCALL_DRIVER_USBH_HID(dev, set_idle_rate));
	return z_impl_usbh_hid_set_idle_rate(dev, report_id, idle_period_ms);
}
#include <zephyr/syscalls/usbh_hid_set_idle_rate_mrsh.c>

static inline int z_vrfy_usbh_hid_get_idle_rate(const struct device *dev, uint8_t report_id,
						uint16_t *idle_period_ms)
{
	K_OOPS(K_SYSCALL_DRIVER_USBH_HID(dev, get_idle_rate));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(idle_period_ms, sizeof(uint16_t)));
	return z_impl_usbh_hid_get_idle_rate(dev, report_id, idle_period_ms);
}
#include <zephyr/syscalls/usbh_hid_get_idle_rate_mrsh.c>

static inline int z_vrfy_usbh_hid_set_protocol(const struct device *dev, uint8_t protocol_code)
{
	K_OOPS(K_SYSCALL_DRIVER_USBH_HID(dev, set_protocol));
	return z_impl_usbh_hid_set_protocol(dev, protocol_code);
}
#include <zephyr/syscalls/usbh_hid_set_protocol_mrsh.c>

static inline int z_vrfy_usbh_hid_get_protocol(const struct device *dev, uint8_t *protocol_code)
{
	K_OOPS(K_SYSCALL_DRIVER_USBH_HID(dev, get_protocol));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(protocol_code, sizeof(uint8_t)));
	return z_impl_usbh_hid_get_protocol(dev, protocol_code);
}
#include <zephyr/syscalls/usbh_hid_get_protocol_mrsh.c>
