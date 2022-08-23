/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = ALIGN(4);

_shared_irq_start = .;

KEEP(*(.shared_irq))
KEEP(*(.shared_irq_arg))

_shared_irq_end = .;
