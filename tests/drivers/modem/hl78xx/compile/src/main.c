/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/**
 * @brief Minimal HL78XX build coverage application.
 *
 * The test scenarios intentionally exercise Kconfig/devicetree combinations
 * for the HL7800 and HL7812 modem variants. No runtime modem interaction is
 * required.
 */
int main(void)
{
	return 0;
}
