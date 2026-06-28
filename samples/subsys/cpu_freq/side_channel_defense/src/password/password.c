/*
 * Copyright (c) 2026 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>

#include "password.h"

static const char secret[] = "Password123!";

#define SECRET_LEN (sizeof(secret) - 1)

/*
 * Example of a non-constant execution path, which could leak some internal secret. Of course, it
 * would be quite sad if a password was truly checked in this manner. But this sample demonstrates
 * that changing the cpu frequency randomly at runtime can provide a layer of security for even
 * purposefully vulnerable functions.
 */
bool check_password(const char *guess)
{
	for (int i = 0; i < SECRET_LEN; i++) {
		if (guess[i] != secret[i]) {
			return false;
		}
	}

	/* Doing some login processing */
	k_busy_wait(1000);

	return true;
}
