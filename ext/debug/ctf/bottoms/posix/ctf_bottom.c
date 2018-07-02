/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/__assert.h>
#include "ctf_bottom.h"

ctf_bottom_ctx_t ctf_bottom;


void ctf_bottom_configure()
{
	ctf_bottom.pathname = "zephyr_trace.ctf";
	ctf_bottom.ostream = fopen(ctf_bottom.pathname, "wb");
	__ASSERT(ctf_bottom.ostream != NULL,
		"CTF stream: Problem opening file %s for write",
		ctf_bottom.pathname
	);
}

void ctf_bottom_start()
{
	/* TODO emit CTF header */
}
