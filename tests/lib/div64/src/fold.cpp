/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fold.h"

extern "C" void div64_check_folded_cpp(void)
{
#if !defined(CONFIG_NO_OPTIMIZATIONS)
	check_folded();
#endif
}
