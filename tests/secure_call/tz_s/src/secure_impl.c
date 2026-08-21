/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Secure-side implementations for the TZ integration test.
 *
 * z_secure_vrfy_*  – called by the generated z_secure_mrsh_* entry functions;
 *                    validates NS pointers before touching them.
 * z_secure_impl_*  – the actual TEE-side business logic.
 */

#include <stdint.h>
#include <stddef.h>
#include <zephyr/internal/secure_call_handler.h>

int z_secure_impl_sc_add(int a, int b)
{
	return a + b;
}

void z_secure_impl_sc_fill(uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)i;
	}
}

int z_secure_impl_sc_nop(void)
{
	return 42;
}

int z_secure_vrfy_sc_add(int a, int b)
{
	/* Scalar args — no pointer validation needed */
	return z_secure_impl_sc_add(a, b);
}

void z_secure_vrfy_sc_fill(uint8_t *buf, size_t len)
{
	/* Validate that the entire buffer is in NS-accessible read-write memory */
	Z_SECURE_MEMORY_WRITE(buf, len);
	z_secure_impl_sc_fill(buf, len);
}

int z_secure_vrfy_sc_nop(void)
{
	return z_secure_impl_sc_nop();
}
