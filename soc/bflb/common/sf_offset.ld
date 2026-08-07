/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

KEEP(*(.vectors.__bflb_reset))
KEEP(*(.vectors.__bflb_reset*))
. = ALIGN(4);
