/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

/* Max supported firmwares */
#define FW_COUNT 4

struct firmware_info {
	char name[32];
	unsigned int start_addr;
	unsigned int end_addr;
};

int config_get_firmware(char *fw_name, uintptr_t *start_addr,
			unsigned int *size);

#if defined __cplusplus
}
#endif

#endif /* FIRMWARE_H */
