/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STATE_BINDING_H
#define _STATE_BINDING_H

void state_binding(u8_t lightness, u8_t temperature);
void calculate_lightness_target_values(u8_t type);
void calculate_temp_target_values(u8_t type);

#endif
