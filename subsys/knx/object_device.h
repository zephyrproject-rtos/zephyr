/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OBJECT_DEVICE__
#define __OBJECT_DEVICE__

#include <stdint.h>
#include <stdbool.h>

uint16_t device_individual_address(void);
void device_individual_address_set(uint16_t value);

bool device_prog_mode(void);
void device_prog_mode_set(bool value);

/* Set PID_DEVICE_CONTROL bit 1 (IA duplication detected). */
void device_set_ia_duplication(void);

/* Clear PID_DEVICE_CONTROL bit 2 (Verify Mode) — call when a TL connection closes. */
void device_clear_verify_mode(void);

/* DEBUG ONLY — not called anywhere in the stack. Wipes the individual
 * address and every downloaded table back to factory defaults and persists
 * immediately. Call manually (e.g. from main()) to reset during testing.
 */
void device_factory_format(void);

void device_individual_address_changed_set_cb(void (*callback)(uint16_t newvalue));

uint16_t device_descriptor(void);
uint16_t device_manufacturer_id(void);
uint8_t device_serial_number(int i);
uint8_t device_hardware_type(int i);
uint16_t device_version(void);
uint8_t device_routing_count(void);

#endif
