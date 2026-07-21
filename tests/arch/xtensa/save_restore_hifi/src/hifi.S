/*
 * Copyright (c) 2024, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hifi_get - Retrieve the HiFi aedX registers
 */
.global hifi_get
.align 4
hifi_get:
	entry a1, 32

	ae_s64.i aed0, a2, 0
	ae_s64.i aed1, a2, 8
	ae_s64.i aed2, a2, 16
	ae_s64.i aed3, a2, 24
	ae_s64.i aed4, a2, 32
	ae_s64.i aed5, a2, 40
	ae_s64.i aed6, a2, 48
	ae_s64.i aed7, a2, 56

	addi a2, a2, 64

	ae_s64.i aed8, a2, 0
	ae_s64.i aed9, a2, 8
	ae_s64.i aed10, a2, 16
	ae_s64.i aed11, a2, 24
	ae_s64.i aed12, a2, 32
	ae_s64.i aed13, a2, 40
	ae_s64.i aed14, a2, 48
	ae_s64.i aed15, a2, 56

	retw.n

/*
 * hifi_set - Set the HiFi aedX registers
 */
.global hifi_set
.align 4
hifi_set:
	entry a1, 32

	ae_l64.i aed0, a2, 0
	ae_l64.i aed1, a2, 8
	ae_l64.i aed2, a2, 16
	ae_l64.i aed3, a2, 24
	ae_l64.i aed4, a2, 32
	ae_l64.i aed5, a2, 40
	ae_l64.i aed6, a2, 48
	ae_l64.i aed7, a2, 56

	addi a2, a2, 64

	ae_l64.i aed8, a2, 0
	ae_l64.i aed9, a2, 8
	ae_l64.i aed10, a2, 16
	ae_l64.i aed11, a2, 24
	ae_l64.i aed12, a2, 32
	ae_l64.i aed13, a2, 40
	ae_l64.i aed14, a2, 48
	ae_l64.i aed15, a2, 56

	retw.n
