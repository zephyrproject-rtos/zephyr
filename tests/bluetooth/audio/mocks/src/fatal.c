/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fatal.h>
#include <zephyr/kernel.h>

void z_fatal_error(unsigned int reason, const struct arch_esf *esf, const struct arch_csf *csf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(esf);
	ARG_UNUSED(csf);

	ztest_test_fail();
}
