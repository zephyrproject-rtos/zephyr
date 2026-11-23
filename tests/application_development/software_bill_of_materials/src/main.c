/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * Copyright (c) 2026 The Linux Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <answer.h>

int main(void)
{
	/* Reference the "used" module so its code is linked into the image. */
	return sbom_used_module_answer() == 42 ? 0 : 1;
}
