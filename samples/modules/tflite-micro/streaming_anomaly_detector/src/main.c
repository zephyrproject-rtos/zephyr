/*
 * SPDX-FileCopyrightText: Copyright 2020 The TensorFlow Authors. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main_functions.h"

int main(void)
{
	setup();

	while (1) {
		loop();
	}

	return 0;
}
