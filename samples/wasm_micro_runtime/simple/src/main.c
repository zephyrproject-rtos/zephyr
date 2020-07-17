/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include "bh_platform.h"
#include "wasm_export.h"

int iwasm_main(void);

void main(void)
{
	iwasm_main();
	for (;;) {
		k_sleep(Z_TIMEOUT_MS(1000));
	}
}
