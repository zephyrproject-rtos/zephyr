/*
 * SPDX-License-Identifier: Apache-2.0
 */

__cy_ble_1_start = .;
KEEP(*(i.*))
__cy_ble_1_end = .;

__arm_vfe_header_start = .;
KEEP(*(.arm_vfe_header))
__arm_vfe_header_end = .;

__constdata_start = .;
KEEP(*(.constdata))
__constdata_end = .;

__conststring_start = .;
KEEP(*(.conststring))
__conststring_end = .;
