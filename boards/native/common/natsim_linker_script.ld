/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extra linker command/script file for the native simulator runner
 */

SECTIONS
 {
	symbol_to_keep :
	{
		KEEP(*(symbol_to_keep*));
	}
 } INSERT AFTER .data;

/*
 * Note this script augments the default native simulator linker script
 */
