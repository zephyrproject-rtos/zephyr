/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The sleep API is made of inline functions in a header, so it gets compiled
 * again in every translation unit that includes it, C++ ones included.  C++
 * is the stricter of the two: Z_TIMEOUT_TICKS_INIT() is a braced initializer
 * there, which rejects a narrowing conversion that C would accept silently.
 *
 * Nothing here needs to assert anything.  The point is that this file is
 * compiled as C++ with warnings promoted to errors, so a conversion that only
 * C tolerates cannot reach a release again.
 */

#include <zephyr/kernel.h>

extern "C" void sleep_convert_cpp_build(void)
{
	volatile int32_t v = 0;

	(void)k_sleep(K_MSEC(0));
	(void)k_sleep(K_NO_WAIT);
	(void)k_msleep(0);
	(void)k_msleep(v);
	(void)k_usleep(0);
	(void)k_usleep(v);
	(void)k_sleep_ticks(K_NO_WAIT);

	(void)z_sleep_ticks_to_int32_ms(0);
	(void)z_sleep_ticks_to_int32_us(0);
}
