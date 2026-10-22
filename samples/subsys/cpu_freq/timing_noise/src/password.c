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
 * Deliberately non-constant-time check used as the attack surface for the
 * sample's cycle-count ranking experiment. Of course, a real password check
 * should not be written this way. Constant-time comparison remains the
 * correct fix; frequency jitter only affects this particular measurement.
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
